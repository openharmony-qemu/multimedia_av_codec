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

bool RatioDropStrategy::ShouldDropFrame(int64_t pts, uint64_t frameIndex)
{
    (void)pts;
    if (targetFrameRate_ <= 0.0 || srcFrameRate_ <= 0.0) {
        return false;
    }
    totalFrames_++;
    double keepRatio = targetFrameRate_ / srcFrameRate_;
    if (keptFrames_ < static_cast<uint64_t>(std::ceil(totalFrames_ * keepRatio))) {
        keptFrames_++;
        return false;
    }
    return true;
}

DropStrategyType RatioDropStrategy::GetType() const
{
    return DropStrategyType::RATIO;
}

TimestampDropStrategy::TimestampDropStrategy(double targetFrameRate)
    : frameIntervalUs_(static_cast<int64_t>(1000000.0 / targetFrameRate))
{
}

bool TimestampDropStrategy::ShouldDropFrame(int64_t pts, uint64_t frameIndex)
{
    (void)frameIndex;
    if (lastKeptPts_ < 0) {
        lastKeptPts_ = pts;
        return false;
    }
    if (pts - lastKeptPts_ >= frameIntervalUs_) {
        lastKeptPts_ = pts;
        return false;
    }
    return true;
}

DropStrategyType TimestampDropStrategy::GetType() const
{
    return DropStrategyType::TIMESTAMP;
}

} // namespace PreProcessing
} // namespace MediaAVCodec
} // namespace OHOS
