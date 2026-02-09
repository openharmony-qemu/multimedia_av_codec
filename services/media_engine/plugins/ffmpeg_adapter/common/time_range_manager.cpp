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

#include <algorithm>
#include "time_range_manager.h"

namespace OHOS {
namespace Media {
namespace Plugins {

constexpr int32_t IDX_INTERVAL = 2;

bool TimeRangeManager::IsInTimeRanges(const int64_t targetTs, TimeRange &timeRange)
{
    auto it = std::find_if(timeRanges_.begin(), timeRanges_.end(),
        [targetTs](const TimeRange& range) {
            return targetTs >= range.start_ts && targetTs <= range.end_ts;
        });
    if (it != timeRanges_.end()) {
        timeRange.start_ts = it->start_ts;
        timeRange.end_ts = it->end_ts;
        return true;
    }
    return false;
}

void TimeRangeManager::AddTimeRange(const TimeRange &range)
{
    if (range.start_ts > range.end_ts) {
        return;
    }
    ReduceRanges();
    TimeRange newRange = range;
    auto it = timeRanges_.upper_bound({newRange.end_ts, newRange.end_ts}); // 首个start_ts大于range.end_ts的元素
    while (it != timeRanges_.begin()) {
        --it; // 最后一个start_ts小于等于range.end_ts的元素
        if (it->end_ts < newRange.start_ts) {
            break;
        }
        newRange.start_ts = std::min(newRange.start_ts, it->start_ts);
        newRange.end_ts = std::max(newRange.end_ts, it->end_ts);
        it = timeRanges_.erase(it);
    }
    timeRanges_.insert(newRange);
}

void TimeRangeManager::ReduceRanges()
{
    if (timeRanges_.size() < static_cast<size_t>(maxEntries_)) {
        return;
    }
    std::vector<TimeRange> ranges(timeRanges_.begin(), timeRanges_.end());
    timeRanges_.clear();
    for (size_t i = 0; i < ranges.size(); ++i) {
        if (i == 0 || i == ranges.size() - 1) {
            timeRanges_.insert(ranges[i]);
        } else if (i % IDX_INTERVAL == 0) {
            timeRanges_.insert(ranges[i]);
        }
    }
}

} // namespace Plugins
} // namespace Media
} // namespace OHOS