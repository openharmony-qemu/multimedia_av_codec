/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#include "frame_drop_strategy.h"
#include <cmath>
#include <cstdint>

namespace OHOS {
namespace MediaAVCodec {
namespace PreProcessing {

RatioDropStrategy::RatioDropStrategy(double srcFrameRate, double targetFrameRate)
    : srcFrameRate_(srcFrameRate), targetFrameRate_(targetFrameRate)
{
}

bool RatioDropStrategy::ShouldDropFrame(uint64_t pts)
{
    (void)pts;
    if (!std::isfinite(targetFrameRate_) || !std::isfinite(srcFrameRate_) || targetFrameRate_ <= 0.0 ||
        srcFrameRate_ <= 0.0) {
        return false;
    }
    totalFrames_++;
    double keepRatio = targetFrameRate_ / srcFrameRate_;
    double targetCount = totalFrames_ * keepRatio;
    if (!std::isfinite(targetCount) || targetCount < 0.0) {
        return false;
    }
    if (keptFrames_ < static_cast<uint64_t>(std::ceil(targetCount))) {
        keptFrames_++;
        return false;
    }
    return true;
}

DropStrategyType RatioDropStrategy::GetType() const
{
    return DropStrategyType::RATIO;
}

void RatioDropStrategy::FlushTimeStamp() {}

TimestampDropStrategy::TimestampDropStrategy(double targetFrameRate) : frameIntervalUs_(0)
{
    if (std::isfinite(targetFrameRate) && targetFrameRate > 0.0) {
        double interval = 1000000.0 / targetFrameRate;
        if (std::isfinite(interval) && interval <= static_cast<double>(UINT64_MAX)) {
            frameIntervalUs_ = static_cast<uint64_t>(interval);
        }
    }
}

bool TimestampDropStrategy::ShouldDropFrame(uint64_t pts)
{
    if (frameIntervalUs_ == 0) {
        return false;
    }
    if (!hasBase_ || pts < basePts_) {
        basePts_ = pts;
        hasBase_ = true;
        return false;
    }
    uint64_t elapsed = pts - basePts_;
    if (elapsed >= frameIntervalUs_) {
        if (basePts_ <= UINT64_MAX - frameIntervalUs_) {
            basePts_ += frameIntervalUs_;
        } else {
            basePts_ = pts;
        }
        return false;
    }
    return true;
}

DropStrategyType TimestampDropStrategy::GetType() const
{
    return DropStrategyType::TIMESTAMP;
}

void TimestampDropStrategy::FlushTimeStamp()
{
    hasBase_ = false;
}

} // namespace PreProcessing
} // namespace MediaAVCodec
} // namespace OHOS
