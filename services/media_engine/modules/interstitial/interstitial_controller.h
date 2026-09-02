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

#ifndef INTERSTITIAL_CONTROLLER_H
#define INTERSTITIAL_CONTROLLER_H

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <map>
#include <vector>

#include "common/status.h"
#include "avcodec_common.h"
#include "interstitial_strategies.h"

namespace OHOS {
namespace Media {
namespace Pipeline {
    class MediaSyncManager;
}

namespace Plugins {
    class MediaSource;
}

using MediaSyncManager = OHOS::Media::Pipeline::MediaSyncManager;
using MediaSource = OHOS::Media::Plugins::MediaSource;

class MediaDemuxer;
class InterstitialScheduler;

enum class InterstitialPlayState : uint32_t {
    IDLE = 0,
    PLAYING,
};

enum class AdsEventType : int32_t {
    STARTED = 0,
    COMPLETED = 1,
    SKIPPED = 2,
    ERROR = 3,
};

struct AdsChangeEvent {
    AdsEventType type;
    std::string eventId;
    int64_t durationMs{-1};
    int32_t errorCode{0};
    std::string errorMessage;
};

struct InterstitialEntry {
    std::string eventId;
    std::string resourceUri;
    int64_t startMs{0};
    int64_t durationMs{0};
    std::shared_ptr<MediaSource> mediaSource;
    bool played{false};
    int32_t order{0};
};

class InterstitialController : public std::enable_shared_from_this<InterstitialController> {
public:
    InterstitialController();
    ~InterstitialController();

    // Init
    Status Init(const std::shared_ptr<MediaDemuxer>& mainDemuxer, const std::shared_ptr<MediaSyncManager>& syncMgr);

    // Configuration
    void SetSyncCenter(std::shared_ptr<MediaSyncManager> syncMgr);
    void SetScheduler(std::shared_ptr<InterstitialScheduler> scheduler);
    void SetMainContentUri(const std::string& uri);
    void SetLiveSource(bool isLive);
    using AdsEventCallback = std::function<void(const AdsChangeEvent& event)>;
    void SetAdsEventCallback(AdsEventCallback cb);
    using SpeedChangeCallback = std::function<void(float speed)>;
    void SetSpeedChangeCallback(SpeedChangeCallback cb);

    // Ad management
    Status AddAdsMediaSource(const std::shared_ptr<MediaSource>& source, int64_t startMs, std::string& outId);
    Status RemoveAdsMediaSource(const std::string& id);
    Status DisableAllAdsMediaSource();
    Status SkipCurrentAdsMediaSource();

    // External events
    void OnPreloadTick();
    void OnAdEos();
    void TryPreRollAd();
    void OnStop();
    void OnSeek(int64_t seekTargetMs);

    // State queries
    bool IsPlayingInterstitial() const;
    bool IsAdsDisabled() const;
    InterstitialPlayState GetPlayState() const;
    bool HasPendingEvents() const;
    bool IsLiveSource() const;
    std::shared_ptr<IScheduleStrategy> GetScheduleStrategy() const;

private:
    bool DoSwitch(const std::string& id);
    bool TrySwitchAdCandidates(int64_t startMs);
    void DoResume();
    void EmitAdStartEvent();
    Status DoSourceSwitch(const std::string& uri, int64_t seekMs, float speed,
        std::function<void()> preStartAction = nullptr);
    void FinishCurrentAdAndContinue(AdsEventType eventType);

    // Helpers
    int64_t ClampSeekMs(int64_t seekMs);
    std::string FindNextAdId(int64_t currentPosMs);
    std::vector<std::string> CollectSameStartMsAds(int64_t startMs);
    std::string GenerateAdId();
    std::string CreateAndAddEntry(const std::string& uri, int64_t startMs, const std::shared_ptr<MediaSource>& source);
    void EmitStartEvent(const InterstitialEntry& entry);
    void EmitEndEvent(const InterstitialEntry& entry, AdsEventType eventType,
        int32_t errorCode = 0, const std::string& errorMessage = "");
    void SetPlayState(InterstitialPlayState state);

    // Dependencies
    std::shared_ptr<MediaDemuxer> mainDemuxer_;
    std::shared_ptr<MediaSyncManager> syncMgr_;
    std::shared_ptr<InterstitialScheduler> scheduler_;
    std::shared_ptr<IScheduleStrategy> scheduleStrategy_;

    // Configuration
    std::string mainContentUri_;
    std::atomic<bool> isLive_{false};
    std::atomic<bool> isDisableAds_{false};

    // Callbacks
    AdsEventCallback adsEventCallback_;
    SpeedChangeCallback speedChangeCallback_;

    // Ad state
    std::map<std::string, InterstitialEntry> entries_;
    mutable std::mutex adMutex_;
    std::atomic<int32_t> adCounter_{0};
    std::string currentAdId_;
    std::string preRollAdId_;

    // Playback state
    std::atomic<InterstitialPlayState> playState_{InterstitialPlayState::IDLE};
    std::atomic<int64_t> resumePointMs_{0};
    std::atomic<float> originalSpeed_{1.0f};
};

} // namespace Media
} // namespace OHOS

#endif // INTERSTITIAL_CONTROLLER_H
