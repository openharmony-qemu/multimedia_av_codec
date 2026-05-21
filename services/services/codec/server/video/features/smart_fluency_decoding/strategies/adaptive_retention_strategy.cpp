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
#include "double_utils.h"
#include "avcodec_log.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_SFD, "AdaptiveStrategy"};
constexpr double MIN_SPEED_FOR_DYNAMIC_DROP = 1.0;
constexpr double ZERO_MOTION_RATIO_THRESHOLD = 0.85;
constexpr double TARGET_SAFE_FPS = 45.0;
constexpr double TARGET_EXTREME_FPS = 22.0;
constexpr double MIN_SOURCE_FPS_FOR_DROP = 24.0;
constexpr double HIGH_RISK_TH = 0.8;
constexpr double LOW_RISK_TH = 0.2;
constexpr double CON_LOW_TH = 0.33;
constexpr double CON_HIGH_TH = 0.66;
constexpr double MAG_LOW_TH = 6.5;
constexpr double MAG_HIGH_TH = 21.0;
constexpr double MAG_CAP = 30.0;
} // namespace

namespace OHOS {
namespace MediaAVCodec {
namespace SFD {

AdaptiveRetentionStrategy::AdaptiveRetentionStrategy()
{
    AVCODEC_LOGD("AdaptiveRetentionStrategy created");
}

double AdaptiveRetentionStrategy::EvaluateStutterRisk(const MVStats& mvStats) const
{
    double mag = std::clamp(mvStats.perceptualMagnitude, 0.0, MAG_CAP);
    double con = std::clamp(mvStats.motionConsistency, 0.0, 1.0);
    double rawRisk = 0.0;
    if (DoubleUtils::IsLessOrEqual(con, CON_LOW_TH)) {
        rawRisk = LOW_RISK_TH;
    } else if (DoubleUtils::IsLessOrEqual(con, CON_HIGH_TH)) {
        rawRisk = LOW_RISK_TH + ((con - CON_LOW_TH) / (CON_HIGH_TH - CON_LOW_TH)) * (HIGH_RISK_TH - LOW_RISK_TH);
    } else {
        rawRisk = HIGH_RISK_TH + ((con - CON_HIGH_TH) / (1.0 - CON_HIGH_TH)) * (1.0 - HIGH_RISK_TH);
    }

    double magWeight = 1.0;
    if (DoubleUtils::IsLessOrEqual(mag, MAG_LOW_TH)) {
        magWeight = 0.4 + (mag / MAG_LOW_TH) * 0.6;
    } else if (DoubleUtils::IsLessOrEqual(mag, MAG_HIGH_TH)) {
        magWeight = 1.0;
    } else {
        magWeight = 1.0 - ((mag - MAG_HIGH_TH) / (MAG_CAP - MAG_HIGH_TH)) * 0.7;
    }
    double finalRisk = rawRisk * magWeight;
    return std::clamp(finalRisk, 0.0, 1.0);
}

double AdaptiveRetentionStrategy::CalculateTargetRatio(const FrameRetentionContext& ctx) const
{
    if (sourceFps_ <= 0.0) {
        return 1.0;
    }
    double renderFps = sourceFps_ * currentSpeed_;
    if (DoubleUtils::IsLessOrEqual(renderFps, MIN_SOURCE_FPS_FOR_DROP)) {
        return 1.0;
    }
    double baseRatioCeiling = std::clamp(TARGET_SAFE_FPS / renderFps, 0.0, 1.0);
    double maxDropFloor = std::clamp(TARGET_EXTREME_FPS / renderFps, 0.0, 1.0);
    if (!ctx.mvStats) {
        return baseRatioCeiling;
    }
    if (ctx.mvStats->zeroMotionRatio >= ZERO_MOTION_RATIO_THRESHOLD) {
        return maxDropFloor;
    }
    if (ctx.mvStats->validBlocks > 0) {
        double risk = EvaluateStutterRisk(*(ctx.mvStats));
        if (risk >= HIGH_RISK_TH) {
            return baseRatioCeiling;
        } else if (risk <= LOW_RISK_TH) {
            return maxDropFloor;
        } else {
            double factor = (risk - LOW_RISK_TH) / (HIGH_RISK_TH - LOW_RISK_TH);
            return maxDropFloor + factor * (baseRatioCeiling - maxDropFloor);
        }
    }
    return baseRatioCeiling;
}

bool AdaptiveRetentionStrategy::ExecuteAccumulatorDecision(double dynamicRatio, uint32_t preDroppedCount)
{
    accumulator_ += dynamicRatio + (preDroppedCount * dynamicRatio);
    if (DoubleUtils::IsGreaterOrEqual(accumulator_, 1.0)) {
        accumulator_ -= 1.0;
        if (DoubleUtils::IsLess(currentSpeed_, MIN_SPEED_FOR_DYNAMIC_DROP)) {
            lastReason_ = SfdDecisionReason::POST_RETAIN_ADAPTIVE_SPEED;
        } else {
            lastReason_ = SfdDecisionReason::POST_RETAIN_ADAPTIVE_DT;
        }
        return true;
    }
    lastReason_ = SfdDecisionReason::POST_DROP_ADAPTIVE_DT;
    return false;
}

bool AdaptiveRetentionStrategy::MakeRetentionDecision(const FrameRetentionContext& ctx)
{
    double dynamicTargetRatio = CalculateTargetRatio(ctx);
    bool isRetain = ExecuteAccumulatorDecision(dynamicTargetRatio, ctx.preDroppedCount);
    targetRatio_ = dynamicTargetRatio;
    AVCODEC_LOGD("Adaptive dynRatio: %{public}.3f, accumulator: %{public}.3f, isRetain: %{public}d, speed: %{public}.3f",
                 dynamicTargetRatio, accumulator_, isRetain, currentSpeed_);
    return isRetain;
}

void AdaptiveRetentionStrategy::UpdatePlaybackSpeed(double speed)
{
    AVCODEC_LOGI("UpdatePlaybackSpeed last is %{public}.3f, now is %{public}.3f", currentSpeed_, speed);
    currentSpeed_ = DoubleUtils::IsGreater(speed, 0.0) ? speed : 1.0;
}

void AdaptiveRetentionStrategy::UpdateSourceFramerate(double sourceFps)
{
    AVCODEC_LOGI("UpdateSourceFramerate last is %{public}.3f, now is %{public}.3f", sourceFps_, sourceFps);
    sourceFps_ = sourceFps;
}

void AdaptiveRetentionStrategy::Reset()
{
    accumulator_ = 0.0;
    targetRatio_ = 1.0;
    sourceFps_ = -1.0;
    lastReason_ = SfdDecisionReason::UNSPECIFIED;
    AVCODEC_LOGI("AdaptiveRetentionStrategy reset completed");
}

RetentionStrategyType AdaptiveRetentionStrategy::GetMode() const
{
    return RetentionStrategyType::ADAPTIVE;
}

double AdaptiveRetentionStrategy::GetRatio() const
{
    return targetRatio_;
}

SfdDecisionReason AdaptiveRetentionStrategy::GetLastDecisionReason() const
{
    return lastReason_;
}

bool AdaptiveRetentionStrategy::RequiresMvAnalysis() const
{
    return true;
}
} // namespace SFD
} // namespace MediaAVCodec
} // namespace OHOS
