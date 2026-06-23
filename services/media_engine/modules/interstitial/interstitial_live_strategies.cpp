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

#include "interstitial_live_strategies.h"

#include <algorithm>

#include "media_demuxer.h"
#include "common/log.h"

namespace OHOS {
namespace Media {

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_DEMUXER, "LiveScheduleStrategy"};
constexpr int64_t US_PER_MS = 1000;
}

std::string LiveScheduleStrategy::GetName() const
{
    return "LiveSchedule";
}

void LiveScheduleStrategy::OnSeek(std::vector<TimedEventEntry>& events, int64_t seekTargetMs)
{
    (void)events;
    (void)seekTargetMs;
}

void LiveScheduleStrategy::TrimExpiredEvents(std::vector<TimedEventEntry>& events, int64_t currentPosMs)
{
    events.erase(
        std::remove_if(events.begin(), events.end(),
            [currentPosMs](const TimedEventEntry& entry) {
                if (entry.metadata == nullptr) {
                    return true;
                }
                return entry.notified && entry.metadata->start < currentPosMs;
            }),
        events.end());
}

int64_t LiveScheduleStrategy::GetResumeSeekMs(int64_t resumePointMs,
    const std::shared_ptr<MediaDemuxer>& demuxer)
{
    (void)resumePointMs;
    FALSE_RETURN_V_MSG_E(demuxer, 0, "GetResumeSeekMs: demuxer is null");
    int64_t durationUs = 0;
    if (demuxer->GetDuration(durationUs) && durationUs > 0) {
        int64_t durationMs = durationUs / US_PER_MS;
        MEDIA_LOG_I("GetResumeSeekMs: live edge=%{public}lld", static_cast<long long>(durationMs));
        return durationMs;
    }
    MEDIA_LOG_I("GetResumeSeekMs: duration unavailable, seek to 0");
    return 0;
}

bool LiveScheduleStrategy::ShouldSaveResumePoint() const
{
    return false;
}

bool LiveScheduleStrategy::ShouldHandleSeek() const
{
    return false;
}

} // namespace Media
} // namespace OHOS
