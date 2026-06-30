/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "interstitial_controller.h"

#include <algorithm>
#include <cmath>

#include "common/media_source.h"
#include "common/log.h"
#include "interstitial_scheduler.h"
#include "interstitial_vod_strategies.h"
#include "interstitial_live_strategies.h"
#include "media_demuxer.h"

namespace OHOS {
namespace Media {

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_DEMUXER, "InterstitialController"};
constexpr int64_t US_PER_MS = 1000;
constexpr int64_t AD_EXPIRE_WINDOW_MS = 200;
constexpr int64_t ADS_SEEK_POS = 0;
constexpr float ADS_PLAYBACK_SPEED = 1.0f;
}

InterstitialController::InterstitialController() = default;
InterstitialController::~InterstitialController() = default;

Status InterstitialController::Init(const std::shared_ptr<MediaDemuxer>& mainDemuxer,
    const std::shared_ptr<MediaSyncManager>& syncMgr)
{
    FALSE_RETURN_V_MSG_E(mainDemuxer, Status::ERROR_INVALID_PARAMETER, "Init failed: mainDemuxer is null");
    mainDemuxer_ = mainDemuxer;
    syncMgr_ = syncMgr;
    MEDIA_LOG_I("Init success, syncMgr is null=" PUBLIC_LOG_D32, syncMgr == nullptr);
    return Status::OK;
}

void InterstitialController::SetSyncCenter(std::shared_ptr<MediaSyncManager> syncMgr)
{
    syncMgr_ = std::move(syncMgr);
    MEDIA_LOG_I("SetSyncCenter OK");
}

void InterstitialController::SetScheduler(std::shared_ptr<InterstitialScheduler> scheduler)
{
    scheduler_ = scheduler;
    MEDIA_LOG_I("SetScheduler success");
}

void InterstitialController::SetMainContentUri(const std::string& uri)
{
    mainContentUri_ = uri;
    MEDIA_LOG_I("SetMainContentUri: " PUBLIC_LOG_S, uri.c_str());
}

void InterstitialController::SetLiveSource(bool isLive)
{
    isLive_.store(isLive);
    if (isLive) {
        scheduleStrategy_ = std::make_shared<LiveScheduleStrategy>();
    } else {
        scheduleStrategy_ = std::make_shared<VodScheduleStrategy>();
    }
    if (scheduler_) {
        scheduler_->SetScheduleStrategy(scheduleStrategy_);
    }
    MEDIA_LOG_I("SetLiveSource: isLive=" PUBLIC_LOG_D32, isLive);
}

void InterstitialController::SetAdsEventCallback(AdsEventCallback cb)
{
    adsEventCallback_ = cb;
    MEDIA_LOG_I("SetAdsEventCallback success");
}

void InterstitialController::SetSpeedChangeCallback(SpeedChangeCallback cb)
{
    speedChangeCallback_ = cb;
    MEDIA_LOG_I("SetSpeedChangeCallback success");
}

Status InterstitialController::AddAdsMediaSource(const std::shared_ptr<MediaSource>& source,
    int64_t startMs, std::string& outId)
{
    FALSE_RETURN_V_MSG_E(source, Status::ERROR_INVALID_PARAMETER, "AddAdsMediaSource failed: source is null");
    std::string id;
    {
        std::lock_guard<std::mutex> lock(adMutex_);
        std::string uri = source->GetSourceUri();

        for (const auto& kv : entries_) {
            if (kv.second.resourceUri == uri && kv.second.startMs == startMs) {
                outId = kv.first;
                MEDIA_LOG_I("AddAdsMediaSource: duplicate uri=" PUBLIC_LOG_S ", startMs=" PUBLIC_LOG_D64
                    ", existing id=" PUBLIC_LOG_S, uri.c_str(), startMs, kv.first.c_str());
                return Status::OK;
            }
        }

        id = CreateAndAddEntry(uri, startMs, source);
        outId = id;
        MEDIA_LOG_I("AddAdsMediaSource: id=" PUBLIC_LOG_S ", uri=" PUBLIC_LOG_S ", startMs=" PUBLIC_LOG_D64,
            id.c_str(), uri.c_str(), startMs);
    }

    if (startMs == 0 && playState_ == InterstitialPlayState::IDLE && preRollAdId_.empty()) {
        MEDIA_LOG_I("AddAdsMediaSource: pre-roll ad detected, will trigger on DoStart");
        preRollAdId_ = id;
    }

    return Status::OK;
}

Status InterstitialController::RemoveAdsMediaSource(const std::string& id)
{
    std::lock_guard<std::mutex> lock(adMutex_);

    auto it = entries_.find(id);
    if (it == entries_.end()) {
        MEDIA_LOG_W("RemoveAdsMediaSource: ad id=" PUBLIC_LOG_S " not found", id.c_str());
        return Status::ERROR_INVALID_PARAMETER;
    }

    if (playState_ == InterstitialPlayState::PLAYING && currentAdId_ == id) {
        MEDIA_LOG_W("RemoveAdsMediaSource: cannot remove currently playing ad id=" PUBLIC_LOG_S, id.c_str());
        return Status::OK;
    }

    entries_.erase(it);

    MEDIA_LOG_I("RemoveAdsMediaSource: id=" PUBLIC_LOG_S, id.c_str());
    return Status::OK;
}

Status InterstitialController::DisableAllAdsMediaSource()
{
    isDisableAds_.store(true);
    std::lock_guard<std::mutex> lock(adMutex_);
    if (playState_ != InterstitialPlayState::PLAYING) {
        entries_.clear();
        return Status::OK;
    }
    
    MEDIA_LOG_W("DisableAllAdsMediaSource: currently playing ad id=" PUBLIC_LOG_S, currentAdId_.c_str());
    auto it = entries_.find(currentAdId_);
    if (it != entries_.end()) {
        InterstitialEntry currentEntry = it->second;
        entries_.clear();
        entries_[currentAdId_] = currentEntry;
    } else {
        entries_.clear();
    }

    MEDIA_LOG_I("DisableAllAdsMediaSource success, remaining=" PUBLIC_LOG_ZU, entries_.size());
    return Status::OK;
}

Status InterstitialController::SkipCurrentAdsMediaSource()
{
    {
        std::lock_guard<std::mutex> lock(adMutex_);
        if (playState_ != InterstitialPlayState::PLAYING) {
            MEDIA_LOG_W("SkipCurrentAdsMediaSource: not playing ad, state=" PUBLIC_LOG_U32, playState_.load());
            return Status::OK;
        }
        MEDIA_LOG_I("SkipCurrentAdsMediaSource: skipping current ad id=" PUBLIC_LOG_S, currentAdId_.c_str());
    }
    FinishCurrentAdAndContinue(AdsEventType::SKIPPED);
    return Status::OK;
}

void InterstitialController::OnPreloadTick()
{
    MEDIA_LOG_I("OnPreloadTick");
    FALSE_RETURN_MSG_W(!isDisableAds_.load(), "InterstitialController::OnPreloadTick failed: ads are disabled");

    int64_t targetStartMs = -1;
    {
        std::lock_guard<std::mutex> lock(adMutex_);
        if (playState_ != InterstitialPlayState::IDLE) {
            MEDIA_LOG_W("is playing ad");
            return;
        }
        FALSE_RETURN_MSG_W(syncMgr_, "InterstitialController::OnPreloadTick failed: syncMgr is null");
        int64_t currentPosMs = syncMgr_->GetMediaTimeNow() / US_PER_MS;
        std::string nextId = FindNextAdId(currentPosMs);
        if (nextId.empty()) {
            return;
        }
        targetStartMs = entries_[nextId].startMs;
    }

    if (!TrySwitchAdCandidates(targetStartMs)) {
        DoResume();
    }
}

void InterstitialController::OnAdEos()
{
    {
        std::lock_guard<std::mutex> lock(adMutex_);
        if (playState_ != InterstitialPlayState::PLAYING) {
            MEDIA_LOG_W("OnAdEos: not playing ad, state=" PUBLIC_LOG_U32, playState_.load());
            return;
        }
        MEDIA_LOG_I("OnAdEos: ad EOS detected, currentAdId=" PUBLIC_LOG_S, currentAdId_.c_str());
    }
    FinishCurrentAdAndContinue(AdsEventType::COMPLETED);
}

void InterstitialController::TryPreRollAd()
{
    if (preRollAdId_.empty()) {
        return;
    }
    if (isDisableAds_.load() || playState_ != InterstitialPlayState::IDLE) {
        MEDIA_LOG_I("TryPreRollAd: skipped, disabled=" PUBLIC_LOG_D32 ", state=" PUBLIC_LOG_U32,
            isDisableAds_.load(), playState_.load());
        preRollAdId_.clear();
        return;
    }

    {
        std::lock_guard<std::mutex> lock(adMutex_);
        auto it = entries_.find(preRollAdId_);
        if (it == entries_.end() || it->second.played) {
            preRollAdId_.clear();
            return;
        }
    }

    MEDIA_LOG_I("TryPreRollAd: triggering pre-roll ad " PUBLIC_LOG_S, preRollAdId_.c_str());
    if (!TrySwitchAdCandidates(0)) {
        DoResume();
    }
}

void InterstitialController::OnStop()
{
    std::lock_guard<std::mutex> lock(adMutex_);

    if (playState_ == InterstitialPlayState::PLAYING) {
        MEDIA_LOG_W("OnStop: currently playing ad, will stop");
        SetPlayState(InterstitialPlayState::IDLE);
    }

    entries_.clear();
    currentAdId_.clear();
    preRollAdId_.clear();
    resumePointMs_ = 0;

    MEDIA_LOG_I("OnStop success");
}

void InterstitialController::OnSeek(int64_t seekTargetMs)
{
    MEDIA_LOG_I("OnSeek: seekTargetMs=" PUBLIC_LOG_D64, seekTargetMs);
    if (scheduleStrategy_ && !scheduleStrategy_->ShouldHandleSeek()) {
        MEDIA_LOG_I("OnSeek: strategy says no-op");
        return;
    }

    std::string preRollToTrigger;
    {
        std::lock_guard<std::mutex> lock(adMutex_);

        for (auto& pair : entries_) {
            if (pair.second.startMs < seekTargetMs) {
                pair.second.played = true;
            } else {
                pair.second.played = false;
            }
        }

        if (seekTargetMs == 0 && !preRollAdId_.empty()) {
            auto it = entries_.find(preRollAdId_);
            if (it != entries_.end() && !it->second.played) {
                preRollToTrigger = preRollAdId_;
            }
        }
    }
    FALSE_RETURN_MSG_W(!isDisableAds_.load(), "OnSeek switch failed: ads are disabled");
    FALSE_RETURN_MSG(!preRollToTrigger.empty(), "OnSeek: no pre-roll ad to trigger");
    if (!TrySwitchAdCandidates(0)) {
        DoResume();
    }
}

bool InterstitialController::IsPlayingInterstitial() const
{
    return playState_ == InterstitialPlayState::PLAYING;
}

bool InterstitialController::IsAdsDisabled() const
{
    return isDisableAds_.load();
}

InterstitialPlayState InterstitialController::GetPlayState() const
{
    return playState_.load();
}

bool InterstitialController::HasPendingEvents() const
{
    std::lock_guard<std::mutex> lock(adMutex_);
    for (const auto& pair : entries_) {
        if (!pair.second.played) {
            return true;
        }
    }
    return false;
}

bool InterstitialController::IsLiveSource() const
{
    return isLive_.load();
}

std::shared_ptr<IScheduleStrategy> InterstitialController::GetScheduleStrategy() const
{
    return scheduleStrategy_;
}

bool InterstitialController::DoSwitch(const std::string& id)
{
    MEDIA_LOG_I("DoSwitch: id=" PUBLIC_LOG_S, id.c_str());
    FALSE_RETURN_V_MSG_E(syncMgr_, false, "DoSwitch: syncMgr_ is null, cannot switch");

    if (scheduleStrategy_ && scheduleStrategy_->ShouldSaveResumePoint() && resumePointMs_ == 0) {
        resumePointMs_ = syncMgr_->GetMediaTimeNow() / US_PER_MS;
        MEDIA_LOG_I("DoSwitch: recorded resumePointMs=" PUBLIC_LOG_D64, resumePointMs_.load());
    }

    auto it = entries_.find(id);
    if (it == entries_.end()) {
        MEDIA_LOG_E("DoSwitch: entry not found for id=" PUBLIC_LOG_S, id.c_str());
        return false;
    }

    originalSpeed_ = syncMgr_->GetPlaybackRate();
    MEDIA_LOG_I("DoSwitch: saved originalSpeed=" PUBLIC_LOG_F, originalSpeed_.load());
    currentAdId_ = id;
    auto preStartAction = [this]() {
        auto it = entries_.find(currentAdId_);
        if (it != entries_.end()) {
            int64_t durationUs = 0;
            if (mainDemuxer_->GetDuration(durationUs) && durationUs > 0) {
                it->second.durationMs = durationUs / US_PER_MS;
                MEDIA_LOG_I("DoSwitch: updated durationMs=" PUBLIC_LOG_D64 " for id=" PUBLIC_LOG_S,
                    durationUs, currentAdId_.c_str());
            }
            EmitStartEvent(it->second);
        }
    };

    SetPlayState(InterstitialPlayState::PLAYING);

    auto ret = DoSourceSwitch(it->second.resourceUri, ADS_SEEK_POS, ADS_PLAYBACK_SPEED, preStartAction);
    if (ret != Status::OK) {
        MEDIA_LOG_E("DoSwitch: DoSourceSwitch failed for id=" PUBLIC_LOG_S, id.c_str());
        EmitEndEvent(it->second, AdsEventType::ERROR, static_cast<int32_t>(ret), "DoSourceSwitch failed");
        SetPlayState(InterstitialPlayState::IDLE);
        currentAdId_.clear();
        return false;
    }

    MEDIA_LOG_I("DoSwitch success: change to ads");
    return true;
}

bool InterstitialController::TrySwitchAdCandidates(int64_t startMs)
{
    MEDIA_LOG_I("TrySwitchAdCandidates: startMs=" PUBLIC_LOG_D64, startMs);
    
    std::vector<std::string> candidates;
    {
        std::lock_guard<std::mutex> lock(adMutex_);
        candidates = CollectSameStartMsAds(startMs);
    }

    for (const auto& id : candidates) {
        if (DoSwitch(id)) {
            return true;
        }
        std::lock_guard<std::mutex> lock(adMutex_);
        entries_[id].played = true;
    }
    return false;
}

void InterstitialController::DoResume()
{
    int64_t seekMs = resumePointMs_;
    if (scheduleStrategy_) {
        seekMs = scheduleStrategy_->GetResumeSeekMs(resumePointMs_, mainDemuxer_);
    }

    MEDIA_LOG_I("DoResume: mainContentUri=" PUBLIC_LOG_S ", seekMs=" PUBLIC_LOG_D64,
        mainContentUri_.c_str(), seekMs);

    if (mainContentUri_.empty() || !syncMgr_) {
        MEDIA_LOG_E("DoResume: cannot resume, mainContentUri is empty=%{public}d, syncMgr is null=%{public}d",
            mainContentUri_.empty(), !syncMgr_);
        SetPlayState(InterstitialPlayState::IDLE);
        currentAdId_.clear();
        resumePointMs_ = 0;
        return;
    }

    float speed = originalSpeed_.load();
    DoSourceSwitch(mainContentUri_, seekMs, speed);
    SetPlayState(InterstitialPlayState::IDLE);
    currentAdId_.clear();
    resumePointMs_ = 0;
    MEDIA_LOG_I("DoResume success: change to main, speed=" PUBLIC_LOG_F, speed);
}

Status InterstitialController::DoSourceSwitch(const std::string& uri, int64_t seekMs, float speed,
    std::function<void()> preStartAction)
{
    MEDIA_LOG_I("DoSourceSwitch: uri=" PUBLIC_LOG_S ", seekMs=" PUBLIC_LOG_D64 ", speed=" PUBLIC_LOG_F,
        uri.c_str(), seekMs, speed);

    FALSE_RETURN_V_MSG_E(syncMgr_, Status::ERROR_INVALID_OPERATION, "DoSourceSwitch: syncMgr_ is null, cannot switch");
    uint32_t capturedBitRate = mainDemuxer_->GetCurrentBitRate();
    MEDIA_LOG_I("DoSourceSwitch: captured bitrate=" PUBLIC_LOG_U32, capturedBitRate);

    if (speedChangeCallback_) {
        speedChangeCallback_(speed);
    }

    mainDemuxer_->HandleForSourceSwitch();

    auto source = std::make_shared<MediaSource>(uri);
    auto status = mainDemuxer_->SetDataSource(source);
    FALSE_RETURN_V_MSG_E(status == Status::OK, status, "DoSourceSwitch: SetDataSource failed");
    status = mainDemuxer_->ReselectTracks();
    FALSE_RETURN_V_MSG_E(status == Status::OK, status, "DoSourceSwitch: ReselectTracks failed");

    seekMs = ClampSeekMs(seekMs);
    int64_t realSeekTime = 0;
    status = mainDemuxer_->SeekTo(seekMs, Plugins::SeekMode::SEEK_CLOSEST_INNER, realSeekTime);
    FALSE_RETURN_V_MSG_E(status == Status::OK, status, "DoSourceSwitch: SeekTo failed");
    MEDIA_LOG_I("DoSourceSwitch: SeekTo success, realSeekTime=" PUBLIC_LOG_D64, realSeekTime);
    syncMgr_->Seek(seekMs * US_PER_MS, true);
    MEDIA_LOG_I("DoSourceSwitch: syncMgr_->Seek done");

    if (preStartAction) {
        preStartAction();
    }

    status = mainDemuxer_->Start();
    FALSE_RETURN_V_MSG_E(status == Status::OK, status, "DoSourceSwitch: Start failed");
    if (capturedBitRate > 0) {
        mainDemuxer_->SelectBitRate(capturedBitRate, false, true);
    }
    MEDIA_LOG_I("DoSourceSwitch success");
    return Status::OK;
}

void InterstitialController::FinishCurrentAdAndContinue(AdsEventType eventType)
{
    InterstitialEntry currentEntry;
    int64_t startMs = -1;

    {
        std::lock_guard<std::mutex> lock(adMutex_);
        auto it = entries_.find(currentAdId_);
        if (it != entries_.end()) {
            currentEntry = it->second;
            startMs = it->second.startMs;
            it->second.played = true;
        }
    }

    EmitEndEvent(currentEntry, eventType);
    FALSE_RETURN_MSG_W(!isDisableAds_.load(), "FinishCurrentAdAndContinue failed: ads are disabled");
    if (startMs >= 0 && TrySwitchAdCandidates(startMs)) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(adMutex_);
        SetPlayState(InterstitialPlayState::IDLE);
        currentAdId_.clear();
    }
    DoResume();
}

int64_t InterstitialController::ClampSeekMs(int64_t seekMs)
{
    int64_t durationUs = 0;
    if (mainDemuxer_->GetDuration(durationUs) && durationUs > 0) {
        int64_t durationMs = durationUs / US_PER_MS;
        if (seekMs > durationMs) {
            MEDIA_LOG_W("ClampSeekMs: seekMs " PUBLIC_LOG_D64 " exceeds duration " PUBLIC_LOG_D64 ", clamping",
                seekMs, durationMs);
            return durationMs;
        }
    }
    return seekMs;
}

std::string InterstitialController::FindNextAdId(int64_t currentPosMs)
{
    MEDIA_LOG_I("FindNextAdId: currentPosMs=" PUBLIC_LOG_D64, currentPosMs);
    FALSE_RETURN_V_MSG_E(syncMgr_, "", "FindNextAdId: syncMgr is null");
    std::string bestId;
    int64_t bestStartMs = INT64_MAX;
    float speed = syncMgr_->GetPlaybackRate();
    for (auto it = entries_.begin(); it != entries_.end(); ++it) {
        if (it->second.played) {
            continue;
        }
        int64_t startMs = it->second.startMs;
        int64_t endMs = startMs + static_cast<int64_t>(AD_EXPIRE_WINDOW_MS * std::max(ADS_PLAYBACK_SPEED, speed));
        if (currentPosMs > endMs) {
            it->second.played = true;
            continue;
        }
        if (startMs <= currentPosMs && startMs < bestStartMs) {
            bestStartMs = startMs;
            bestId = it->first;
        }
    }
    return bestId;
}

std::vector<std::string> InterstitialController::CollectSameStartMsAds(int64_t startMs)
{
    std::vector<std::pair<int32_t, std::string>> ordered;
    for (auto& pair : entries_) {
        if (!pair.second.played && pair.second.startMs == startMs) {
            ordered.push_back({pair.second.order, pair.first});
        }
    }
    std::sort(ordered.begin(), ordered.end());
    std::vector<std::string> result;
    result.reserve(ordered.size());
    for (auto& p : ordered) {
        result.push_back(p.second);
    }
    return result;
}

std::string InterstitialController::GenerateAdId()
{
    int32_t counter = adCounter_.fetch_add(1);
    return "ad_" + std::to_string(counter);
}

std::string InterstitialController::CreateAndAddEntry(const std::string& uri, int64_t startMs,
    const std::shared_ptr<MediaSource>& source)
{
    int32_t seq = adCounter_.fetch_add(1);
    std::string id = "ad_" + std::to_string(seq);
    InterstitialEntry entry;
    entry.eventId = id;
    entry.resourceUri = uri;
    entry.startMs = startMs;
    entry.mediaSource = source;
    entry.order = seq;
    entries_[id] = std::move(entry);
    return id;
}

void InterstitialController::EmitStartEvent(const InterstitialEntry& entry)
{
    if (!adsEventCallback_) {
        return;
    }

    AdsChangeEvent event;
    event.type = AdsEventType::STARTED;
    event.eventId = entry.eventId;
    event.durationMs = entry.durationMs;

    MEDIA_LOG_I("EmitStartEvent: eventId=" PUBLIC_LOG_S ", durationMs=" PUBLIC_LOG_D64,
        entry.eventId.c_str(), entry.durationMs);
    adsEventCallback_(event);
}

void InterstitialController::EmitEndEvent(const InterstitialEntry& entry, AdsEventType eventType,
    int32_t errorCode, const std::string& errorMessage)
{
    if (!adsEventCallback_ || entry.eventId.empty()) {
        return;
    }

    AdsChangeEvent event;
    event.type = eventType;
    event.eventId = entry.eventId;
    event.durationMs = entry.durationMs;
    event.errorCode = errorCode;
    event.errorMessage = errorMessage;

    MEDIA_LOG_I("EmitEndEvent: eventId=" PUBLIC_LOG_S ", durationMs=" PUBLIC_LOG_D64
        ", type=" PUBLIC_LOG_D32 ", errorCode=" PUBLIC_LOG_D32,
        entry.eventId.c_str(), entry.durationMs, static_cast<int32_t>(eventType), errorCode);
    adsEventCallback_(event);
}

void InterstitialController::SetPlayState(InterstitialPlayState state)
{
    playState_.store(state);
    MEDIA_LOG_I("SetPlayState: " PUBLIC_LOG_U32, state);
}

} // namespace Media
} // namespace OHOS
