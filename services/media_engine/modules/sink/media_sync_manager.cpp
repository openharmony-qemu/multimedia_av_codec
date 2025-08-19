/*
* Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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
#include <functional>
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
    FALSE_RETURN_V_MSG_W(rate >= 0, Status::ERROR_INVALID_PARAMETER, "Invalid playback Rate: %{public}f", rate);
    OHOS::Media::AutoLock lock(clockMutex_);
    MEDIA_LOG_I_SHORT("set play rate " PUBLIC_LOG_F, rate);
    int64_t currentClockTime = GetSystemClock();
    int64_t currentMediaTime = std::min(GetMediaTime(currentClockTime), GetMaxMediaProgress());
    if (currentMediaTime != HST_TIME_NONE) {
        SimpleUpdateTimeAnchor(currentClockTime, currentMediaTime);
    }
    playRate_ = rate;
    return Status::OK;
}

float MediaSyncManager::GetPlaybackRate()
{
    OHOS::Media::AutoLock lock(clockMutex_);
    return playRate_;
}

void MediaSyncManager::SetMediaTimeRangeStart(int64_t startMediaTime, int32_t trackId, IMediaSynchronizer* supplier)
{
    FALSE_RETURN_NOLOG(IsSupplierValid(supplier) && supplier->GetPriority() >= currentRangeStartPriority_);
    currentRangeStartPriority_ = supplier->GetPriority();
    OHOS::Media::AutoLock lock(clockMutex_);
    if (minRangeStartOfMediaTime_ == HST_TIME_NONE || startMediaTime < minRangeStartOfMediaTime_) {
        minRangeStartOfMediaTime_ = startMediaTime;
        MEDIA_LOG_I_SHORT("set media started at " PUBLIC_LOG_D64, minRangeStartOfMediaTime_);
    }
}

void MediaSyncManager::SetMediaTimeRangeEnd(int64_t endMediaTime, int32_t trackId, IMediaSynchronizer* supplier)
{
    FALSE_RETURN_NOLOG(IsSupplierValid(supplier) && supplier->GetPriority() >= currentRangeEndPriority_);
    currentRangeEndPriority_ = supplier->GetPriority();
    OHOS::Media::AutoLock lock(clockMutex_);
    if (maxRangeEndOfMediaTime_ == HST_TIME_NONE || endMediaTime > maxRangeEndOfMediaTime_) {
        maxRangeEndOfMediaTime_ = endMediaTime;
        MEDIA_LOG_I_SHORT("set media end at " PUBLIC_LOG_D64, maxRangeEndOfMediaTime_);
    }
}

void MediaSyncManager::SetInitialVideoFrameRate(double frameRate)
{
    videoInitialFrameRate_ = frameRate;
}

double MediaSyncManager::GetInitialVideoFrameRate()
{
    return videoInitialFrameRate_;
}

void MediaSyncManager::SetAllSyncShouldWaitNoLock()
{
    if (!alreadySetSyncersShouldWait_) {
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
    if (clockState_ == State::PAUSED && pausedMediaTime_ != HST_TIME_NONE && alreadySetSyncersShouldWait_) {
        SimpleUpdateTimeAnchor(GetSystemClock(), pausedMediaTime_);
        pausedClockTime_ = HST_TIME_NONE;
        pausedMediaTime_ = HST_TIME_NONE;
    }
    FALSE_RETURN_V_NOLOG(clockState_ != State::RESUMED, Status::OK);
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
    FALSE_RETURN_V_NOLOG(clockState_ != State::PAUSED, Status::OK);
    pausedClockTime_ = GetSystemClock();
    pausedMediaTime_ = std::min(GetMediaTime(pausedClockTime_), GetMaxMediaProgress());
    MEDIA_LOG_I("pause with clockTime " PUBLIC_LOG_D64 ", mediaTime " PUBLIC_LOG_D64,
        pausedClockTime_, pausedMediaTime_);
    clockState_ = State::PAUSED;
    return Status::OK;
}

Status MediaSyncManager::Seek(int64_t mediaTime, bool isClosest)
{
    OHOS::Media::AutoLock lock(clockMutex_);
    FALSE_RETURN_V_NOLOG(minRangeStartOfMediaTime_ != HST_TIME_NONE && maxRangeEndOfMediaTime_ != HST_TIME_NONE,
        Status::ERROR_INVALID_OPERATION);
    isSeeking_ = true;
    MEDIA_LOG_I_SHORT("isSeeking_ mediaTime: %{public}" PRId64, mediaTime);
    seekingMediaTime_ = mediaTime;
    alreadySetSyncersShouldWait_ = false; // set already as false
    SetAllSyncShouldWaitNoLock(); // all suppliers should sync preroll again after seek
    ResetTimeAnchorNoLock(); // reset the time anchor
    isFrameAfterSeeked_ = true;
    if (isClosest) {
        firstMediaTimeAfterSeek_ = mediaTime;
    } else {
        firstMediaTimeAfterSeek_ = HST_TIME_NONE;
    }
    return Status::OK;
}

Status MediaSyncManager::Reset()
{
    MEDIA_LOG_D("do Reset");
    Stop();
    {
        OHOS::Media::AutoLock lock1(syncersMutex_);
        syncers_.clear();
        prerolledSyncers_.clear();
    }
    isFrameAfterSeeked_ = false;
    lastReportMediaTime_ = HST_TIME_NONE;
    firstMediaTimeAfterSeek_ = HST_TIME_NONE;
    return Status::OK;
}

Status MediaSyncManager::Stop()
{
    MEDIA_LOG_D("do Stop");
    OHOS::Media::AutoLock lock(clockMutex_);
    clockState_ = State::PAUSED;
    ResetTimeAnchorNoLock();
    pausedClockTime_ = HST_TIME_NONE;
    playRate_ = 1.0f;
    alreadySetSyncersShouldWait_ = false;
    isSeeking_ = false;
    seekCond_.notify_all();
    seekingMediaTime_ = HST_TIME_NONE;
    minRangeStartOfMediaTime_ = HST_TIME_NONE;
    maxRangeEndOfMediaTime_ = HST_TIME_NONE;
    lastVideoBufferAbsPts_ = HST_TIME_NONE;
    
    return Status::OK;
}

void MediaSyncManager::ResetTimeAnchorNoLock()
{
    pausedMediaTime_ = HST_TIME_NONE;
    currentSyncerPriority_ = IMediaSynchronizer::NONE;
    SimpleUpdateTimeAnchor(HST_TIME_NONE, HST_TIME_NONE);
}

void MediaSyncManager::SimpleUpdateTimeAnchor(int64_t clockTime, int64_t mediaTime)
{
    currentAnchorClockTime_ = clockTime;
    currentAnchorMediaTime_ = mediaTime;
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

bool MediaSyncManager::UpdateTimeAnchor(int64_t clockTime, int64_t delayTime, IMediaTime iMediaTime,
    IMediaSynchronizer* supplier)
{
    OHOS::Media::AutoLock lock(clockMutex_);
    bool render = true;
    if (clockTime == HST_TIME_NONE || iMediaTime.mediaTime == HST_TIME_NONE
        || delayTime == HST_TIME_NONE || supplier == nullptr) {
        return render;
    }
    clockTime += delayTime;
    delayTime_ = delayTime;
    if (IsSupplierValid(supplier) && supplier->GetPriority() >= currentSyncerPriority_) {
        currentSyncerPriority_ = supplier->GetPriority();
        SimpleUpdateTimeAnchor(clockTime, iMediaTime.mediaTime);
        MEDIA_LOG_D_SHORT("update time anchor to priority " PUBLIC_LOG_D32 ", mediaTime " PUBLIC_LOG_D64 ", clockTime "
        PUBLIC_LOG_D64, currentSyncerPriority_, currentAnchorMediaTime_, currentAnchorClockTime_);
        if (isSeeking_) {
            MEDIA_LOG_I_SHORT("leaving seeking_");
            isSeeking_ = false;
            seekCond_.notify_all();
            UpdateFirstPtsAfterSeek(iMediaTime.mediaTime);
        }
    }
    return render;
}


void MediaSyncManager::SetLastAudioBufferDuration(int64_t durationUs)
{
    if (durationUs > 0) {
        lastAudioBufferDuration_ = durationUs;
    } else {
        lastAudioBufferDuration_ = 0; // If buffer duration is unavailable, treat it as 0.
    }
}

void MediaSyncManager::SetLastVideoBufferPts(int64_t bufferPts)
{
    lastVideoBufferPts_ = bufferPts;
}

bool MediaSyncManager::CheckSeekingMediaTime(int64_t& mediaTime)
{
    FALSE_RETURN_V_NOLOG(isSeeking_, true);
    // no need to bound media progress during seek
    MEDIA_LOG_D_SHORT("GetMediaTimeNow seekingMediaTime_: %{public}" PRId64, seekingMediaTime_);
    mediaTime = seekingMediaTime_;
    return false;
}

bool MediaSyncManager::CheckPausedMediaTime(int64_t& mediaTime)
{
    mediaTime = (clockState_ == State::PAUSED) ? pausedMediaTime_ : GetMediaTime(GetSystemClock());
    return true;
}

bool MediaSyncManager::CheckIfMediaTimeIsNone(int64_t& mediaTime)
{
    FALSE_RETURN_V_NOLOG(mediaTime == HST_TIME_NONE, true);
    mediaTime = 0;
    return false;
}

bool MediaSyncManager::CheckFirstMediaTimeAfterSeek(int64_t& mediaTime)
{
    bool isAudioNotRendered = (firstMediaTimeAfterSeek_ != HST_TIME_NONE && mediaTime < firstMediaTimeAfterSeek_);
    FALSE_RETURN_V_NOLOG(isAudioNotRendered, true);
    MEDIA_LOG_W_SHORT("audio has not been rendered since seek");
    mediaTime = firstMediaTimeAfterSeek_;
    return true;
}

void MediaSyncManager::UpdataPausedMediaTime(int32_t pausedMediaTime)
{
    pausedMediaTime_ = pausedMediaTime;
}

int64_t MediaSyncManager::GetMaxMediaProgress()
{
    FALSE_RETURN_V_NOLOG(currentSyncerPriority_ != IMediaSynchronizer::AUDIO_SINK,
        currentAnchorMediaTime_ + lastAudioBufferDuration_);
    FALSE_RETURN_V_NOLOG(currentSyncerPriority_ != IMediaSynchronizer::VIDEO_SINK,
        lastVideoBufferPts_);
    return std::max(currentAnchorMediaTime_, lastReportMediaTime_.load());
}

int64_t MediaSyncManager::BoundMediaProgress(int64_t newMediaProgressTime)
{
    int64_t maxMediaProgress = GetMaxMediaProgress();

    FALSE_RETURN_V_MSG_W(newMediaProgressTime <= maxMediaProgress,
        maxMediaProgress,
        "Media progress lag for %{public}" PRId64 " us, currentSyncerPriority_ is %{public}" PRId32,
        newMediaProgressTime - maxMediaProgress, currentSyncerPriority_);

    FALSE_RETURN_V_MSG_W(newMediaProgressTime >= lastReportMediaTime_ || isFrameAfterSeeked_,
        lastReportMediaTime_,
        "Avoid media time to go back without seek, from %{public}" PRId64 " to %{public}" PRId64,
        lastReportMediaTime_.load(), newMediaProgressTime);
    isFrameAfterSeeked_ = false;
    return newMediaProgressTime;
}

int64_t MediaSyncManager::GetMediaTimeNow()
{
    OHOS::Media::AutoLock lock(clockMutex_);
    int64_t currentMediaTime = HST_TIME_NONE;
    for (const auto &func : setMediaTimeFuncs) {
        FALSE_RETURN_V_NOLOG(func(this, currentMediaTime), currentMediaTime);
    }
    currentMediaTime = BoundMediaProgress(currentMediaTime);
    lastReportMediaTime_ = currentMediaTime;
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

bool MediaSyncManager::IsPlayRateValid(float playRate)
{
    static constexpr float MIN_PLAYRATE = 1e-9;
    FALSE_RETURN_V_NOLOG(std::fabs(playRate_) >= MIN_PLAYRATE, false);
    return true;
}

int64_t MediaSyncManager::GetMediaTime(int64_t clockTime)
{
    bool isMediaTimeValid = (clockTime != HST_TIME_NONE && currentAnchorClockTime_ != HST_TIME_NONE
        && currentAnchorMediaTime_ != HST_TIME_NONE && delayTime_ != HST_TIME_NONE);
    FALSE_RETURN_V_NOLOG(IsPlayRateValid(playRate_) && isMediaTimeValid, HST_TIME_NONE);
    return currentAnchorMediaTime_ + (clockTime - currentAnchorClockTime_ + delayTime_)
        * static_cast<double>(playRate_) - delayTime_;
}

int64_t MediaSyncManager::GetAnchoredClockTime(int64_t mediaTime)
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

    bool isAnchoredClockTimeValid = (mediaTime != HST_TIME_NONE && currentAnchorClockTime_ != HST_TIME_NONE
        && currentAnchorMediaTime_ != HST_TIME_NONE);
    if (!IsPlayRateValid(playRate_) || !isAnchoredClockTimeValid) {
        return HST_TIME_NONE;
    }
    return currentAnchorClockTime_ + (mediaTime - currentAnchorMediaTime_) / static_cast<double>(playRate_);
}

void MediaSyncManager::ReportPrerolled(IMediaSynchronizer* supplier)
{
    if (supplier == nullptr) {
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
    bool isStartPtsValid = startPts_ == HST_TIME_NONE || startPts < startPts_;
    if (isStartPtsValid) {
        startPts_ = startPts;
    }
}

void MediaSyncManager::ResetMediaStartPts()
{
    startPts_ = HST_TIME_NONE;
}

int64_t MediaSyncManager::GetMediaStartPts()
{
    return startPts_;
}

void MediaSyncManager::ReportEos(IMediaSynchronizer* supplier)
{
    if (supplier == nullptr) {
        return;
    }
    OHOS::Media::AutoLock lock(clockMutex_);
    if (IsSupplierValid(supplier) && supplier->GetPriority() >= currentSyncerPriority_) {
        currentSyncerPriority_ = IMediaSynchronizer::NONE;
        if (isSeeking_) {
            MEDIA_LOG_I_SHORT("reportEos leaving seeking_");
            isSeeking_ = false;
            seekCond_.notify_all();
        }
    }
}
void MediaSyncManager::SetLastVideoBufferAbsPts(int64_t lastVideoBufferAbsPts)
{
    MEDIA_LOG_D("SetLastVideoBufferAbsPts " PUBLIC_LOG_D64, lastVideoBufferAbsPts);
    lastVideoBufferAbsPts_ = lastVideoBufferAbsPts;
}
 
int64_t MediaSyncManager::GetLastVideoBufferAbsPts() const
{
    MEDIA_LOG_D("GetLastVideoBufferAbsPts" PUBLIC_LOG_D64, lastVideoBufferAbsPts_);
    return lastVideoBufferAbsPts_;
}
} // namespace Pipeline
} // namespace Media
} // namespace OHOS