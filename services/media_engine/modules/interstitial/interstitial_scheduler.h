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

#ifndef INTERSTITIAL_SCHEDULER_H
#define INTERSTITIAL_SCHEDULER_H

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "avcodec_common.h"
#include "interstitial_strategies.h"
#include "media_sync_manager.h"

namespace OHOS {
namespace Media {
namespace Pipeline {
    class MediaSyncManager;
}

using MediaSyncManager = OHOS::Media::Pipeline::MediaSyncManager;
class InterstitialScheduler {
public:
    InterstitialScheduler();
    ~InterstitialScheduler();

    using NotifyCallback = std::function<void(const std::shared_ptr<MediaAVCodec::AVTimedMetaData>& metadata)>;
    void SetNotifyCallback(NotifyCallback callback);
    void SetSyncCenter(std::shared_ptr<MediaSyncManager> syncCenter);
    void SetScheduleStrategy(std::shared_ptr<IScheduleStrategy> strategy);
    std::shared_ptr<IScheduleStrategy> GetScheduleStrategy() const;

    void CollectEvent(const std::shared_ptr<MediaAVCodec::AVTimedMetaData>& metadata);
    void OnPlaybackTick();
    void OnSeek(int64_t seekTargetMs);
    void OnStop();

private:
    void CheckAndNotify(int64_t currentPosMs);

    std::vector<TimedEventEntry> pendingEvents_;
    std::shared_ptr<MediaSyncManager> syncCenter_;
    std::shared_ptr<IScheduleStrategy> scheduleStrategy_;
    NotifyCallback notifyCallback_;
    mutable std::mutex mutex_;

    static constexpr int64_t NOTIFY_ADVANCE_MS = 2000;
    static constexpr int64_t US_PER_MS = 1000;
    static constexpr int64_t MAX_PENDING_EVENTS = 1000;
};

} // namespace Media
} // namespace OHOS

#endif // INTERSTITIAL_SCHEDULER_H
