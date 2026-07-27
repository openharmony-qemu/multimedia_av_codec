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

#include "interstitial_vod_strategies.h"

namespace OHOS {
namespace Media {

std::string VodScheduleStrategy::GetName() const
{
    return "VodSchedule";
}

void VodScheduleStrategy::OnSeek(std::vector<TimedEventEntry>& events, int64_t seekTargetMs)
{
    for (auto& entry : events) {
        if (entry.metadata == nullptr) {
            continue;
        }
        if (entry.metadata->start > seekTargetMs) {
            entry.notified = false;
        } else {
            entry.notified = true;
        }
    }
}

int64_t VodScheduleStrategy::GetResumeSeekMs(int64_t resumePointMs, const std::shared_ptr<MediaDemuxer>& demuxer)
{
    (void)demuxer;
    return resumePointMs;
}

bool VodScheduleStrategy::ShouldSaveResumePoint() const
{
    return true;
}

bool VodScheduleStrategy::ShouldHandleSeek() const
{
    return true;
}

} // namespace Media
} // namespace OHOS
