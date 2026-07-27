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

#ifndef INTERSTITIAL_STRATEGIES_H
#define INTERSTITIAL_STRATEGIES_H

#include <memory>
#include <string>
#include <vector>

#include "avcodec_common.h"

namespace OHOS {
namespace Media {

struct TimedEventEntry {
    std::shared_ptr<MediaAVCodec::AVTimedMetaData> metadata;
    bool notified{false};
};

class MediaDemuxer;

class IScheduleStrategy {
public:
    virtual ~IScheduleStrategy() = default;
    virtual std::string GetName() const = 0;
    virtual void OnSeek(std::vector<TimedEventEntry>& events, int64_t seekTargetMs) = 0;
    virtual void TrimExpiredEvents(std::vector<TimedEventEntry>& events, int64_t currentPosMs)
    {
        (void)events;
        (void)currentPosMs;
    }
    virtual int64_t GetResumeSeekMs(int64_t resumePointMs, const std::shared_ptr<MediaDemuxer>& demuxer) = 0;
    virtual bool ShouldSaveResumePoint() const = 0;
    virtual bool ShouldHandleSeek() const = 0;
};

} // namespace Media
} // namespace OHOS

#endif // INTERSTITIAL_STRATEGIES_H
