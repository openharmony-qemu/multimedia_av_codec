/*
* Copyright (c) 2023-2023 Huawei Device Co., Ltd.
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/


#define HST_LOG_TAG "MediaSyncManager"

#include "media_sync_manager.h"
#include <algorithm>
#include <cmath>
#include "common/log.h"
#include "osal/utils/steady_clock.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_SYSTEM_PLAYER, "MediaSyncManager" };
}

namespace OHOS {
namespace Media {
namespace Pipeline {
namespace {
using namespace std::chrono;
constexpr int MEDIA_TUPLE_START_INDEX = 1;
constexpr int MEDIA_TUPLE_END_INDEX = 2;
}

MediaSyncManager::~MediaSyncManager()
{
    MEDIA_LOG_D_SHORT("~MediaSyncManager enter.");
}

void MediaSyncManager::AddSynchronizer(IMediaSynchronizer* syncer)
{
    if (syncer != nullptr) {
        OHOS::Media::AutoLock lock(syncersMutex_);
        if (std::find(syncers_.begin(), syncers_.end(), syncer) != syncers_.end()) {
            return;
        }
        syncers_.emplace_back(syncer);
    }
}

void MediaSyncManager::RemoveSynchronizer(IMediaSynchronizer* syncer)
{
    if (syncer != nullptr) {
        OHOS::Media::AutoLock lock(syncersMutex_);
        auto ite = std::find(syncers_.begin(), syncers_.end(), syncer);
        if (ite != syncers_.end()) {
            syncers_.erase(ite);
        }
    }
}

Status MediaSyncManager::SetPlaybackRate(float rate)
{
    if (rate < 0) {
        return Status::ERROR_INVALID_PARAMETER;
    }
    OHOS::Media::AutoLock lock(clockMutex_);
    MEDIA_LOG_I_SHORT("set play rate " PUBLIC_LOG_F, rate);
    if (currentAbsMediaTime_ == HST_TIME_NONE || currentAnchorClockTime_ == HST_TIME_NONE
        || currentAnchorMediaTime_ == HST_TIME_NONE || delayTime_ == HST_TIME_NONE) {
        SimpleUpdatePlayRate(rate);
        return Status::OK;
    }
    int64_t now = GetSystemClock();
    int64_t currentMedia = SimpleGetMediaTimeExactly(currentAnchorClockTime_, delayTime_, now,
        currentAnchorMediaTime_, playRate_);
    int64_t currentAbsMedia = SimpleGetMediaTimeExactly(currentAnchorClockTime_, delayTime_, now,
        currentAbsMediaTime_, playRate_);
    SimpleUpdateTimeAnchor(now, currentMedia, currentAbsMedia);
    SimpleUpdatePlayRate(rate);
    return Status::OK;
}

float MediaSyncManager::GetPlaybackRate()
{
    OHOS::Media::AutoLock lock(clockMutex_);
    return playRate_;
}

void MediaSyncManager::SetMediaTimeStartEnd(int32_t trackId, int32_t index, int64_t val)
{
    auto target = std::find_if(trackMediaTimeRange_.begin(), trackMediaTimeRange_.end(), [&trackId]
        (const std::tuple<int32_t, int64_t, int64_t>& item) -> bool {
            return std::get<0>(item) == trackId;
        });
    if (target == trackMediaTimeRange_.end()) {
        trackMediaTimeRange_.emplace_back(std::tuple<int32_t, int64_t, int64_t>(trackId,
            HST_TIME_NONE, HST_TIME_NONE));
        if (index == MEDIA_TUPLE_START_INDEX) {
            std::get<MEDIA_TUPLE_START_INDEX>(*trackMediaTimeRange_.rbegin()) = val;
        } else if (index == MEDIA_TUPLE_END_INDEX) {
            std::get<MEDIA_TUPLE_END_INDEX>(*trackMediaTimeRange_.rbegin()) = val;
        } else {
            MEDIA_LOG_W_SHORT("invalid index");
        }
    } else {
        if (index == MEDIA_TUPLE_START_INDEX) {
            std::get<MEDIA_TUPLE_START_INDEX>(*target) = val;
        } else if (index == MEDIA_TUPLE_END_INDEX) {
            std::get<MEDIA_TUPLE_END_INDEX>(*target) = val;
        } else {
            MEDIA_LOG_W_SHORT("invalid index");
        }
    }
}

void MediaSyncManager::SetMediaTimeRangeStart(int64_t startMediaTime, int32_t trackId, IMediaSynchronizer* supplier)
{
    if (!IsSupplierValid(supplier) || supplier->GetPriority() < currentRangeStartPriority_) {
        return;
    }
    currentRangeStartPriority_ = supplier->GetPriority();
    OHOS::Media::AutoLock lock(clockMutex_);
    SetMediaTimeStartEnd(trackId, MEDIA_TUPLE_START_INDEX, startMediaTime);
    if (minRangeStartOfMediaTime_ == HST_TIME_NONE || startMediaTime < minRangeStartOfMediaTime_) {
        minRangeStartOfMediaTime_ = startMediaTime;
        MEDIA_LOG_I_SHORT("set media started at " PUBLIC_LOG_D64, minRangeStartOfMediaTime_);
    }
}

void MediaSyncManager::SetMediaTimeRangeEnd(int64_t endMediaTime, int32_t trackId, IMediaSynchronizer* supplier)
{
    if (!IsSupplierValid(supplier) || supplier->GetPriority() < currentRangeEndPriority_) {
        return;
    }
    currentRangeEndPriority_ = supplier->GetPriority();
    OHOS::Media::AutoLock lock(clockMutex_);
    SetMediaTimeStartEnd(trackId, MEDIA_TUPLE_END_INDEX, endMediaTime);
    if (maxRangeEndOfMediaTime_ == HST_TIME_NONE || endMediaTime > maxRangeEndOfMediaTime_) {
        maxRangeEndOfMediaTime_ = endMediaTime;
        MEDIA_LOG_I_SHORT("set media end at " PUBLIC_LOG_D64, maxRangeEndOfMediaTime_);
    }
}

void MediaSyncManager::WaitAllPrerolled(bool prerolled)
{
    allSyncerShouldPrerolled_ = prerolled;
}

void MediaSyncManager::SetAllSyncShouldWaitNoLock()
{
    if (allSyncerShouldPrerolled_ && !alreadySetSyncersShouldWait_) {
        prerolledSyncers_.clear();
        {
            OHOS::Media::AutoLock lock1(syncersMutex_);
            if (syncers_.size() > 1) {
                for (const auto &supplier: syncers_) {
                    supplier->WaitAllPrerolled(true);
                }
            }
        }
        alreadySetSyncersShouldWait_ = true;
    }
}

Status MediaSyncManager::Resume()
{
    OHOS::Media::AutoLock lock(clockMutex_);
    // update time anchor after a pause during normal playing
    if (clockState_ == State::PAUSED && pausedExactAbsMediaTime_ != HST_TIME_NONE
        && pausedExactMediaTime_ != HST_TIME_NONE && alreadySetSyncersShouldWait_) {
        SimpleUpdateTimeAnchor(GetSystemClock(), pausedExactMediaTime_, pausedExactAbsMediaTime_);
        pausedMediaTime_ = HST_TIME_NONE;
        pausedExactMediaTime_ = HST_TIME_NONE;
        pausedClockTime_ = HST_TIME_NONE;
        pausedAbsMediaTime_ = HST_TIME_NONE;
        pausedExactAbsMediaTime_ = HST_TIME_NONE;
    }
    if (clockState_ == State::RESUMED) {
        return Status::OK;
    }
    SetAllSyncShouldWaitNoLock();
    MEDIA_LOG_I_SHORT("resume");
    clockState_ = State::RESUMED;
    return Status::OK;
}

int64_t MediaSyncManager::GetSystemClock()
{
    return Plugins::HstTime2Us(SteadyClock::GetCurrentTimeNanoSec());
}

Status MediaSyncManager::Pause()
{
    OHOS::Media::AutoLock lock(clockMutex_);
    if (clockState_ == State::PAUSED) {
        return Status::OK;
    }
    pausedClockTime_ = GetSystemClock();
    if (currentAnchorMediaTime_ != HST_TIME_NONE && currentAnchorClockTime_ != HST_TIME_NONE) {
        pausedMediaTime_ = SimpleGetMediaTime(currentAnchorClockTime_, delayTime_, pausedClockTime_,
            currentAnchorMediaTime_, playRate_);
        pausedExactMediaTime_ = SimpleGetMediaTimeExactly(currentAnchorClockTime_, delayTime_, pausedClockTime_,
            currentAnchorMediaTime_, playRate_);
        pausedAbsMediaTime_ = SimpleGetMediaTime(currentAnchorClockTime_, delayTime_, pausedClockTime_,
            currentAbsMediaTime_, playRate_);
        pausedExactAbsMediaTime_ = SimpleGetMediaTimeExactly(currentAnchorClockTime_, delayTime_, pausedClockTime_,
            currentAbsMediaTime_, playRate_);
    } else {
        pausedMediaTime_ = HST_TIME_NONE;
        pausedExactMediaTime_ = HST_TIME_NONE;
        pausedAbsMediaTime_ = HST_TIME_NONE;
        pausedExactAbsMediaTime_ = HST_TIME_NONE;
    }
    pausedMediaTime_ = ClipMediaTime(pausedMediaTime_);
    pausedExactMediaTime_ = ClipMediaTime(pausedExactMediaTime_);
    pausedAbsMediaTime_ = ClipMediaTime(pausedAbsMediaTime_);
    pausedExactAbsMediaTime_ = ClipMediaTime(pausedExactAbsMediaTime_);
    MEDIA_LOG_I_SHORT("pause with clockTime " PUBLIC_LOG_D64 ", mediaTime " PUBLIC_LOG_D64 ", exactMediaTime "
        PUBLIC_LOG_D64, pausedClockTime_, pausedAbsMediaTime_, pausedExactAbsMediaTime_);
    clockState_ = State::PAUSED;
    return Status::OK;
}

Status MediaSyncManager::Seek(int64_t mediaTime, bool isClosest)
{
    OHOS::Media::AutoLock lock(clockMutex_);
    if (minRangeStartOfMediaTime_ == HST_TIME_NONE || maxRangeEndOfMediaTime_ == HST_TIME_NONE) {
        return Status::ERROR_INVALID_OPERATION;
    }
    isSeeking_ = true;
    MEDIA_LOG_I_SHORT("isSeeking_ mediaTime: %{public}" PRId64, mediaTime);
    seekingMediaTime_ = mediaTime;
    alreadySetSyncersShouldWait_ = false; // set already as false
    SetAllSyncShouldWaitNoLock(); // all suppliers should sync preroll again after seek
    ResetTimeAnchorNoLock(); // reset the time anchor
    frameAfterSeeked_ = true;
    if (isClosest) {
        firstMediaTimeAfterSeek_ = mediaTime;
    } else {
        firstMediaTimeAfterSeek_ = HST_TIME_NONE;
    }
    return Status::OK;
}

Status MediaSyncManager::Reset()
{
    MEDIA_LOG_I_SHORT("do Reset");
    Stop();
    {
        OHOS::Media::AutoLock lock1(syncersMutex_);
        syncers_.clear();
        prerolledSyncers_.clear();
    }
    frameAfterSeeked_ = false;
    lastReportMediaTime_ = HST_TIME_NONE;
    firstMediaTimeAfterSeek_ = HST_TIME_NONE;
    return Status::OK;
}

Status MediaSyncManager::Stop()
{
    MEDIA_LOG_I_SHORT("do Stop");
    OHOS::Media::AutoLock lock(clockMutex_);
    clockState_ = State::PAUSED;
    ResetTimeAnchorNoLock();
    pausedClockTime_ = HST_TIME_NONE;
    playRate_ = 1.0f;
    alreadySetSyncersShouldWait_ = false;
    allSyncerShouldPrerolled_ = true;
    isSeeking_ = false;
    seekCond_.notify_all();
    seekingMediaTime_ = HST_TIME_NONE;
    trackMediaTimeRange_.clear();
    minRangeStartOfMediaTime_ = HST_TIME_NONE;
    maxRangeEndOfMediaTime_ = HST_TIME_NONE;
    
    return Status::OK;
}

int64_t MediaSyncManager::ClipMediaTime(int64_t inTime)
{
    int64_t ret = inTime;
    if (minRangeStartOfMediaTime_ != HST_TIME_NONE && ret < minRangeStartOfMediaTime_) {
        ret = minRangeStartOfMediaTime_;
        MEDIA_LOG_D_SHORT("clip to min media time " PUBLIC_LOG_D64, ret);
    }
    if (maxRangeEndOfMediaTime_ != HST_TIME_NONE && ret > maxRangeEndOfMediaTime_) {
        ret = maxRangeEndOfMediaTime_;
        MEDIA_LOG_D_SHORT("clip to max media time " PUBLIC_LOG_D64, ret);
    }
    return ret;
}

void MediaSyncManager::ResetTimeAnchorNoLock()
{
    pausedMediaTime_ = HST_TIME_NONE;
    pausedExactMediaTime_ = HST_TIME_NONE;
    pausedAbsMediaTime_ = HST_TIME_NONE;
    pausedExactAbsMediaTime_ = HST_TIME_NONE;
    currentSyncerPriority_ = IMediaSynchronizer::NONE;
    SimpleUpdateTimeAnchor(HST_TIME_NONE, HST_TIME_NONE, HST_TIME_NONE);
}

void MediaSyncManager::SimpleUpdatePlayRate(float playRate)
{
    playRate_ = playRate;
}

void MediaSyncManager::SimpleUpdateTimeAnchor(int64_t clockTime, int64_t mediaTime, int64_t mediaAbsTime)
{
    currentAnchorMediaTime_ = mediaTime;
    currentAnchorClockTime_ = clockTime;
    currentAbsMediaTime_ = mediaAbsTime;
}

bool MediaSyncManager::IsSupplierValid(IMediaSynchronizer* supplier)
{
    OHOS::Media::AutoLock lock(syncersMutex_);
    return std::find(syncers_.begin(), syncers_.end(), supplier) != syncers_.end();
}

void MediaSyncManager::UpdateFirstPtsAfterSeek(int64_t mediaTime)
{
    if (firstMediaTimeAfterSeek_ == HST_TIME_NONE) {
        firstMediaTimeAfterSeek_ = mediaTime;
        return;
    }
    if (mediaTime > firstMediaTimeAfterSeek_) {
        firstMediaTimeAfterSeek_ = mediaTime;
    }
}

bool MediaSyncManager::UpdateTimeAnchor(int64_t clockTime, int64_t delayTime, int64_t mediaTime,
    int64_t absMediaTime, int64_t maxMediaTime, IMediaSynchronizer* supplier)
{
    OHOS::Media::AutoLock lock(clockMutex_);
    bool render = true;
    if (clockTime == HST_TIME_NONE || mediaTime == HST_TIME_NONE
        || delayTime == HST_TIME_NONE || supplier == nullptr) {
        return render;
    }
    clockTime += delayTime;
    delayTime_ = delayTime;
    if (IsSupplierValid(supplier) && supplier->GetPriority() >= currentSyncerPriority_) {
        currentSyncerPriority_ = supplier->GetPriority();
        SimpleUpdateTimeAnchor(clockTime, mediaTime, absMediaTime);
        MEDIA_LOG_D_SHORT("update time anchor to priority " PUBLIC_LOG_D32 ", mediaTime " PUBLIC_LOG_D64 ", clockTime "
        PUBLIC_LOG_D64, currentSyncerPriority_, currentAnchorMediaTime_, currentAnchorClockTime_);
        if (isSeeking_) {
            MEDIA_LOG_I_SHORT("leaving seeking_");
            isSeeking_ = false;
            UpdateFirstPtsAfterSeek(mediaTime);
            seekCond_.notify_all();
        }
    }
    return render;
}

int64_t MediaSyncManager::SimpleGetMediaTime(int64_t anchorClockTime, int64_t delayTime, int64_t nowClockTime,
    int64_t anchorMediaTime, float playRate)
{
    if (std::fabs(playRate - 0) < 1e-9) { // 0 threshold
        return HST_TIME_NONE;
    }
    if (anchorClockTime == HST_TIME_NONE || nowClockTime == HST_TIME_NONE
        || anchorMediaTime == HST_TIME_NONE || delayTime== HST_TIME_NONE) {
        return HST_TIME_NONE;
    }
    return anchorMediaTime;
}

int64_t MediaSyncManager::SimpleGetMediaTimeExactly(int64_t anchorClockTime, int64_t delayTime, int64_t nowClockTime,
    int64_t anchorMediaTime, float playRate)
{
    if (std::fabs(playRate - 0) < 1e-9) { // 0 threshold
        return HST_TIME_NONE;
    }
    if (anchorClockTime == HST_TIME_NONE || nowClockTime == HST_TIME_NONE
        || anchorMediaTime == HST_TIME_NONE || delayTime== HST_TIME_NONE) {
        return HST_TIME_NONE;
    }
    return anchorMediaTime + (nowClockTime - anchorClockTime + delayTime) * static_cast<double>(playRate) - delayTime;
}

int64_t MediaSyncManager::BoundMediaProgress(int64_t newMediaProgressTime)
{
    if ((newMediaProgressTime >= lastReportMediaTime_) || frameAfterSeeked_) {
        lastReportMediaTime_ = newMediaProgressTime;
    } else {
        MEDIA_LOG_W_SHORT("Avoid media time to go back without seek, from %{public}" PRId64 " to %{public}" PRId64,
            lastReportMediaTime_.load(), newMediaProgressTime);
    }
    frameAfterSeeked_ = false;
    return lastReportMediaTime_;
}

int64_t MediaSyncManager::GetMediaTimeNow()
{
    OHOS::Media::AutoLock lock(clockMutex_);
    if (isSeeking_) {
        // no need to bound media progress during seek
        MEDIA_LOG_D_SHORT("GetMediaTimeNow seekingMediaTime_: %{public}" PRId64, seekingMediaTime_);
        return seekingMediaTime_;
    }
    int64_t currentMediaTime;
    if (clockState_ == State::PAUSED) {
        currentMediaTime = pausedExactAbsMediaTime_;
    } else {
        currentMediaTime = SimpleGetMediaTimeExactly(currentAnchorClockTime_, delayTime_, GetSystemClock(),
            currentAbsMediaTime_, playRate_);
    }
    if (currentMediaTime == HST_TIME_NONE) {
        return 0;
    }
    if (firstMediaTimeAfterSeek_ != HST_TIME_NONE && currentMediaTime < firstMediaTimeAfterSeek_) {
        MEDIA_LOG_W_SHORT("audio has not been rendered since seek");
        currentMediaTime = firstMediaTimeAfterSeek_;
    }
    if (startPts_ != HST_TIME_NONE) {
        currentMediaTime -= startPts_;
    }
    currentMediaTime = BoundMediaProgress(currentMediaTime);
    MEDIA_LOG_D_SHORT("GetMediaTimeNow currentMediaTime: %{public}" PRId64, currentMediaTime);
    return currentMediaTime;
}

int64_t MediaSyncManager::GetClockTimeNow()
{
    {
        OHOS::Media::AutoLock lock(clockMutex_);
        if (clockState_ == State::PAUSED) {
            return pausedClockTime_;
        }
    }
    return GetSystemClock();
}
int64_t MediaSyncManager::SimpleGetClockTime(int64_t anchorClockTime, int64_t nowMediaTime,
    int64_t anchorMediaTime, float playRate)
{
    if (std::fabs(playRate - 0) < 1e-9) { // 0 threshold
        return HST_TIME_NONE;
    }
    if (anchorClockTime == HST_TIME_NONE || nowMediaTime == HST_TIME_NONE || anchorMediaTime == HST_TIME_NONE) {
        return HST_TIME_NONE;
    }
    return anchorClockTime + (nowMediaTime - anchorMediaTime) / static_cast<double>(playRate);
}

int64_t MediaSyncManager::GetClockTime(int64_t mediaTime)
{
    OHOS::Media::AutoLock lock(clockMutex_);
    if (minRangeStartOfMediaTime_ != HST_TIME_NONE && mediaTime < minRangeStartOfMediaTime_) {
        MEDIA_LOG_D_SHORT("media time " PUBLIC_LOG_D64 " less than min media time " PUBLIC_LOG_D64,
                mediaTime, minRangeStartOfMediaTime_);
    }
    if (maxRangeEndOfMediaTime_ != HST_TIME_NONE && mediaTime > maxRangeEndOfMediaTime_) {
        MEDIA_LOG_D_SHORT("media time " PUBLIC_LOG_D64 " exceed max media time " PUBLIC_LOG_D64,
                mediaTime, maxRangeEndOfMediaTime_);
    }
    return SimpleGetClockTime(currentAnchorClockTime_, mediaTime, currentAnchorMediaTime_, playRate_);
}

void MediaSyncManager::ReportPrerolled(IMediaSynchronizer* supplier)
{
    if (supplier == nullptr) {
        return;
    }
    if (!allSyncerShouldPrerolled_) {
        return;
    }
    OHOS::Media::AutoLock lock(syncersMutex_);
    auto ite = std::find(prerolledSyncers_.begin(), prerolledSyncers_.end(), supplier);
    if (ite != prerolledSyncers_.end()) {
        MEDIA_LOG_I_SHORT("supplier already reported prerolled");
        return;
    }
    prerolledSyncers_.emplace_back(supplier);
    if (prerolledSyncers_.size() == syncers_.size()) {
        for (const auto& prerolled : prerolledSyncers_) {
            prerolled->NotifyAllPrerolled();
        }
        prerolledSyncers_.clear();
    }
}

int64_t MediaSyncManager::GetSeekTime()
{
    return seekingMediaTime_;
}

bool MediaSyncManager::InSeeking()
{
    return isSeeking_;
}

void MediaSyncManager::SetMediaStartPts(int64_t startPts)
{
    startPts_ = startPts;
}

void MediaSyncManager::ReportEos(IMediaSynchronizer* supplier)
{
    if (supplier == nullptr) {
        return;
    }
    OHOS::Media::AutoLock lock(clockMutex_);
    if (IsSupplierValid(supplier) && supplier->GetPriority() >= currentSyncerPriority_) {
        currentSyncerPriority_ = IMediaSynchronizer::NONE;
    }
}
} // namespace Pipeline
} // namespace Media
} // namespace OHOS