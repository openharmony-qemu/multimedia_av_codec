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

#include "interstitial_scheduler.h"

#include <cinttypes>

#include "common/log.h"

namespace OHOS {
namespace Media {

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_DEMUXER, "InterstitialScheduler"};
}

InterstitialScheduler::InterstitialScheduler()
{
    MEDIA_LOG_I("InterstitialScheduler created");
}

InterstitialScheduler::~InterstitialScheduler()
{
    MEDIA_LOG_I("InterstitialScheduler destroyed");
}

void InterstitialScheduler::SetSyncCenter(std::shared_ptr<MediaSyncManager> syncCenter)
{
    syncCenter_ = std::move(syncCenter);
    MEDIA_LOG_I("SetSyncCenter OK");
}

void InterstitialScheduler::SetNotifyCallback(NotifyCallback callback)
{
    notifyCallback_ = std::move(callback);
    MEDIA_LOG_I("SetNotifyCallback OK");
}

void InterstitialScheduler::SetScheduleStrategy(std::shared_ptr<IScheduleStrategy> strategy)
{
    scheduleStrategy_ = std::move(strategy);
    MEDIA_LOG_I("SetScheduleStrategy OK");
}

std::shared_ptr<IScheduleStrategy> InterstitialScheduler::GetScheduleStrategy() const
{
    return scheduleStrategy_;
}

void InterstitialScheduler::CollectEvent(const std::shared_ptr<MediaAVCodec::AVTimedMetaData>& metadata)
{
    FALSE_RETURN_MSG(metadata != nullptr, "CollectEvent: metadata is null");

    bool immediateNotify = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& entry : pendingEvents_) {
            if (entry.metadata != nullptr && entry.metadata->id == metadata->id) {
                MEDIA_LOG_D("CollectEvent: duplicate event id=%{public}s, skip", metadata->id.c_str());
                return;
            }
        }
        pendingEvents_.push_back(TimedEventEntry{metadata, false});
        if (metadata->start <= NOTIFY_ADVANCE_MS) {
            pendingEvents_.back().notified = true;
            immediateNotify = true;
        }
    }
    if (immediateNotify && notifyCallback_) {
        notifyCallback_(metadata);
    }
    MEDIA_LOG_I("collected event id=%{public}s, startMs=%{public}lld", metadata->id.c_str(),
             static_cast<long long>(metadata->start));
}

void InterstitialScheduler::OnPlaybackTick()
{
    FALSE_RETURN_MSG(syncCenter_ != nullptr, "OnPlaybackTick: syncCenter is null");
    int64_t currentPosMs = syncCenter_->GetMediaTimeNow() / US_PER_MS;
    if (currentPosMs <= 0) {
        return;
    }

    CheckAndNotify(currentPosMs);

    if (scheduleStrategy_) {
        std::lock_guard<std::mutex> lock(mutex_);
        scheduleStrategy_->TrimExpiredEvents(pendingEvents_, currentPosMs);
    }
}

void InterstitialScheduler::OnSeek(int64_t seekTargetMs)
{
    if (scheduleStrategy_) {
        std::lock_guard<std::mutex> lock(mutex_);
        scheduleStrategy_->OnSeek(pendingEvents_, seekTargetMs);
    }
    MEDIA_LOG_I("OnSeek: seekTargetMs=%" PRId64, seekTargetMs);
}

void InterstitialScheduler::OnStop()
{
    std::lock_guard<std::mutex> lock(mutex_);
    pendingEvents_.clear();
    MEDIA_LOG_I("OnStop: cleared all events");
}

void InterstitialScheduler::CheckAndNotify(int64_t currentPosMs)
{
    std::vector<std::shared_ptr<MediaAVCodec::AVTimedMetaData>> toNotify;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& entry : pendingEvents_) {
            if (entry.metadata == nullptr || entry.notified) {
                continue;
            }
            if (currentPosMs >= (entry.metadata->start - NOTIFY_ADVANCE_MS)) {
                entry.notified = true;
                toNotify.push_back(entry.metadata);
                MEDIA_LOG_I("CheckAndNotify: marking notify for id=%{public}s, startMs=%" PRId64,
                    entry.metadata->id.c_str(), entry.metadata->start);
            }
        }
    }

    for (const auto& meta : toNotify) {
        if (notifyCallback_) {
            notifyCallback_(meta);
        }
    }
}

} // namespace Media
} // namespace OHOS
