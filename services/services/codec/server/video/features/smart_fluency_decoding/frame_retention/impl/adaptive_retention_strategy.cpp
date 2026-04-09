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

#include "adaptive_retention_strategy.h"

namespace OHOS {
namespace MediaAVCodec {
namespace SFD {

namespace {
constexpr double MAX_SPEED_FOR_RETENTION = 1.2;
constexpr double HIGH_CONSISTENCY_THRESHOLD = 0.8;
constexpr double MAGNITUDE_THRESHOLD = 30.0;
constexpr double LOW_CONSISTENCY_THRESHOLD = 0.3;
constexpr double MAX_DROPS_MULTIPLIER_HIGH = 30.0;
constexpr double REF_DIST_MULTIPLIER = 5.0;
constexpr double MAX_DROPS_MULTIPLIER_LOW = 15.0;
}

bool AdaptiveRetentionStrategy::MakeRetentionDecision(const FrameRetentionContext& ctx)
{
    if (currentSpeed_ <= MAX_SPEED_FOR_RETENTION) {
        return true;
    }

    if (ctx.mvStats && ctx.mvStats->validBlocks > 0) {
        double consistency = ctx.mvStats->motionConsistency;
        double magnitude = ctx.mvStats->perceptualMagnitude;
        if (consistency > HIGH_CONSISTENCY_THRESHOLD) {
            dropCount_ = 0;
            currentTargetRatio_ = 0.0;
            return true;
        }
        if (magnitude > MAGNITUDE_THRESHOLD && consistency < LOW_CONSISTENCY_THRESHOLD) {
            dropCount_++;
            uint32_t maxDrops = static_cast<uint32_t>((currentSpeed_ - 1.0) * MAX_DROPS_MULTIPLIER_HIGH);
            currentTargetRatio_ = static_cast<double>(maxDrops) / (maxDrops + 1.0);
            if (dropCount_ <= maxDrops) {
                return false;
            } else {
                dropCount_ = 0;
                return true;
            }
        }
        if (ctx.mvStats->avgRefDist > REF_DIST_MULTIPLIER * currentSpeed_) {
            dropCount_++;
            uint32_t maxDrops = static_cast<uint32_t>((currentSpeed_ - 1.0) * MAX_DROPS_MULTIPLIER_LOW);
            currentTargetRatio_ = static_cast<double>(maxDrops) / (maxDrops + 1.0);
            if (dropCount_ <= maxDrops) {
                return false;
            }
        }
        dropCount_ = 0;
        currentTargetRatio_ = 0.0;
    }
    return true;
}

void AdaptiveRetentionStrategy::UpdatePlaybackSpeed(double speed)
{
    currentSpeed_ = speed > 0.0 ? speed : 1.0;
}

void AdaptiveRetentionStrategy::Reset()
{
    dropCount_ = 0;
}

FrameRetentionMode AdaptiveRetentionStrategy::GetMode() const
{
    return FrameRetentionMode::ADAPTIVE;
}

double AdaptiveRetentionStrategy::GetCurrentRetentionRatio() const
{
    return 1.0 - currentTargetRatio_;
}

} // namespace SFD
} // namespace MediaAVCodec
} // namespace OHOS
