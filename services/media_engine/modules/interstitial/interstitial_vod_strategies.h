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

#ifndef INTERSTITIAL_VOD_STRATEGIES_H
#define INTERSTITIAL_VOD_STRATEGIES_H

#include "interstitial_strategies.h"

namespace OHOS {
namespace Media {

class VodScheduleStrategy : public IScheduleStrategy {
public:
    std::string GetName() const override;
    void OnSeek(std::vector<TimedEventEntry>& events, int64_t seekTargetMs) override;
    int64_t GetResumeSeekMs(int64_t resumePointMs, const std::shared_ptr<MediaDemuxer>& demuxer) override;
    bool ShouldSaveResumePoint() const override;
    bool ShouldHandleSeek() const override;
};

} // namespace Media
} // namespace OHOS

#endif // INTERSTITIAL_VOD_STRATEGIES_H
