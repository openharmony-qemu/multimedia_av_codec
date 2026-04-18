/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#include "auto_fallback_retention_strategy.h"
#include "avcodec_log.h"

namespace OHOS {
namespace MediaAVCodec {
namespace SFD {

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_SFD, "AutoFallbackStrategy"};
// In the absence of a better PPS-based scheme, 0.8 is the safe fallback ratio.
constexpr double DEFAULT_FALLBACK_RATIO = 0.8;
}

AutoFallbackRetentionStrategy::AutoFallbackRetentionStrategy() noexcept
{
    fallbackDropper_ = std::make_unique<FixedRatioRetentionStrategy>(DEFAULT_FALLBACK_RATIO);
    AVCODEC_LOGD("AutoFallbackRetentionStrategy created (Delegating to FixedRatio: %{public}.2f)",
                 DEFAULT_FALLBACK_RATIO);
}

bool AutoFallbackRetentionStrategy::MakeRetentionDecision(const FrameRetentionContext& ctx)
{
    if (fallbackDropper_) {
        return fallbackDropper_->MakeRetentionDecision(ctx);
    }
    return true;
}

void AutoFallbackRetentionStrategy::UpdatePlaybackSpeed(double speed)
{
    if (fallbackDropper_) {
        fallbackDropper_->UpdatePlaybackSpeed(speed);
    }
}

void AutoFallbackRetentionStrategy::Reset()
{
    if (fallbackDropper_) {
        fallbackDropper_->Reset();
    }
}

RetentionStrategyType AutoFallbackRetentionStrategy::GetMode() const
{
    return RetentionStrategyType::AUTO_RATIO;
}

double AutoFallbackRetentionStrategy::GetRatio() const
{
    if (fallbackDropper_) {
        return fallbackDropper_->GetRatio();
    }
    return 1.0;
}

SfdDecisionReason AutoFallbackRetentionStrategy::GetLastDecisionReason() const
{
    if (fallbackDropper_) {
        SfdDecisionReason internalReason = fallbackDropper_->GetLastDecisionReason();
        if (internalReason == SfdDecisionReason::POST_RETAIN_FIXED_ACC) {
            return SfdDecisionReason::POST_RETAIN_AUTO_FALLBACK;
        }
        return internalReason;
    }
    return SfdDecisionReason::POST_RETAIN_AUTO_FALLBACK;
}

} // namespace SFD
} // namespace MediaAVCodec
} // namespace OHOS
