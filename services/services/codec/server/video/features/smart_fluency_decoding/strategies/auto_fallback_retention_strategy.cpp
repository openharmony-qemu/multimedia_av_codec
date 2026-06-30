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

#include "auto_fallback_retention_strategy.h"
#include "avcodec_log.h"
#include "double_utils.h"
#include "smart_fluency_decoding_types.h"

namespace OHOS {
namespace MediaAVCodec {
namespace SFD {

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_SFD, "AutoFallbackStrategy"};
}

AutoFallbackRetentionStrategy::AutoFallbackRetentionStrategy()
{
    AVCODEC_LOGD("AutoFallbackRetentionStrategy created (passthrough, waiting for sourceFps)");
}

double AutoFallbackRetentionStrategy::CalculateFallbackRatio() const
{
    double renderFps = sourceFps_ * currentSpeed_;
    if (DoubleUtils::IsLessOrEqual(renderFps, 0.0)) {
        return 1.0;
    }
    return std::clamp(FALLBACK_TARGET_FPS / renderFps, MIN_RETENTION_RATIO, MAX_RETENTION_RATIO);
}

void AutoFallbackRetentionStrategy::ApplyCalculatedRatio()
{
    double ratio = CalculateFallbackRatio();
    if (fallbackDropper_ == nullptr) {
        fallbackDropper_ = std::make_unique<FixedRatioRetentionStrategy>(ratio);
        AVCODEC_LOGI("AutoFallback activates calculated ratio: %{public}.3f "
                     "(sourceFps=%{public}.3f, speed=%{public}.3f)", ratio, sourceFps_, currentSpeed_);
    } else {
        fallbackDropper_->UpdateTargetRatio(ratio);
        AVCODEC_LOGI("AutoFallback recalculated ratio to %{public}.3f "
                     "(sourceFps=%{public}.3f, speed=%{public}.3f)", ratio, sourceFps_, currentSpeed_);
    }
}

bool AutoFallbackRetentionStrategy::MakeRetentionDecision(const FrameRetentionContext& ctx)
{
    if (!hasSourceFps_) {
        AVCODEC_LOGD("AutoFallback passthrough: retaining all frames (waiting for sourceFps)");
        return true;
    }
    return fallbackDropper_->MakeRetentionDecision(ctx);
}

void AutoFallbackRetentionStrategy::UpdatePlaybackSpeed(double speed)
{
    AVCODEC_LOGD("AutoFallback UpdatePlaybackSpeed: %{public}.3f -> %{public}.3f", currentSpeed_, speed);
    currentSpeed_ = DoubleUtils::IsGreater(speed, 0.0) ? speed : 1.0;
    if (hasSourceFps_) {
        ApplyCalculatedRatio();
    }
}

void AutoFallbackRetentionStrategy::UpdateSourceFramerate(double sourceFps)
{
    AVCODEC_LOGD("AutoFallback UpdateSourceFramerate: %{public}.3f -> %{public}.3f", sourceFps_, sourceFps);
    if (DoubleUtils::IsGreater(sourceFps, 0.0)) {
        sourceFps_ = sourceFps;
        hasSourceFps_ = true;
        ApplyCalculatedRatio();
    }
}

void AutoFallbackRetentionStrategy::Reset()
{
    hasSourceFps_ = false;
    sourceFps_ = 0.0;
    currentSpeed_ = 1.0;
    if (fallbackDropper_) {
        fallbackDropper_->Reset();
    }
    AVCODEC_LOGD("AutoFallbackRetentionStrategy reset (back to passthrough)");
}

RetentionStrategyType AutoFallbackRetentionStrategy::GetMode() const
{
    return RetentionStrategyType::AUTO_RATIO;
}

double AutoFallbackRetentionStrategy::GetRatio() const
{
    if (!hasSourceFps_) {
        return 1.0;
    }
    return fallbackDropper_ ? fallbackDropper_->GetRatio() : 1.0;
}

SfdDecisionReason AutoFallbackRetentionStrategy::GetLastDecisionReason() const
{
    if (!hasSourceFps_) {
        return SfdDecisionReason::POST_RETAIN_FULL;
    }
    SfdDecisionReason internalReason = fallbackDropper_->GetLastDecisionReason();
    if (internalReason == SfdDecisionReason::POST_RETAIN_FIXED_ACC) {
        return SfdDecisionReason::POST_RETAIN_AUTO_FALLBACK;
    }
    return internalReason;
}

bool AutoFallbackRetentionStrategy::IsPassthrough() const
{
    return !hasSourceFps_;
}

bool AutoFallbackRetentionStrategy::RequiresNaluAnalysis() const
{
    return hasSourceFps_;
}

bool AutoFallbackRetentionStrategy::RequiresMvAnalysis() const
{
    return false;
}

} // namespace SFD
} // namespace MediaAVCodec
} // namespace OHOS
