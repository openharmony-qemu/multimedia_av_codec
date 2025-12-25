/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef TIME_RANGE_MANAGER_H
#define TIME_RANGE_MANAGER_H

#include <set>
#include <cstdint>
#include "ffmpeg_utils.h"

namespace OHOS {
namespace Media {
namespace Plugins {

#define MAX_INDEX_CACHE_SIZE (70 * 1024) // 70KB

struct TimeRange {
    int64_t start_ts {AV_NOPTS_VALUE};
    int64_t end_ts {AV_NOPTS_VALUE};

    bool operator < (const TimeRange& other) const {
        return start_ts < other.start_ts || 
            (start_ts == other.start_ts && end_ts < other.end_ts);
    }
};

class TimeRangeManager {
public:
    TimeRangeManager() = default;
    ~TimeRangeManager() = default;
    bool IsInTimeRanges(const int64_t targetTs, TimeRange &timeRange);
    void AddTimeRange(const TimeRange &range);
    void ReduceRanges();
private:
    std::set<TimeRange> timeRanges_;
    int32_t maxEntries_ = MAX_INDEX_CACHE_SIZE / sizeof(TimeRange);
};

class TimeoutGuard {
public:
    explicit TimeoutGuard(uint32_t timeoutMs)
        : timeoutMs_(timeoutMs), startTime_(std::chrono::high_resolution_clock::now()) {}
    bool IsTimeout() const {
        if (timeoutMs_ == 0) {
            return false;
        }
        auto currentTime = std::chrono::high_resolution_clock::now();
        auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime_).count();
        return static_cast<int64_t>(elapsedMs) >= timeoutMs_;
    }
private:
    uint32_t timeoutMs_ {0};
    std::chrono::high_resolution_clock::time_point startTime_;
};


} // namespace Plugins
} // namespace Media
} // namespace OHOS

#endif // TIME_RANGE_MANAGER_H