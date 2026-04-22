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

#include "fixed_ratio_retention_strategy.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include "avcodec_log.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_SFD, "FixedRatioRetention"};
constexpr double ACCUMULATOR_THRESHOLD = 1.0;
}

namespace OHOS {
namespace MediaAVCodec {
namespace SFD {

FixedRatioRetentionStrategy::FixedRatioRetentionStrategy(double ratio)
{
    if (std::isnan(ratio)) {
        AVCODEC_LOGE("Invalid retention ratio: NaN detected. Fallback to safe default 1.0 (Full Retention)");
        targetRatio_ = 1.0;
    } else {
        targetRatio_ = std::clamp(ratio, MIN_RETENTION_RATIO, MAX_RETENTION_RATIO);
    }
    AVCODEC_LOGD("FixedRatioRetentionStrategy created with targetRatio: %{public}.3f", targetRatio_);
}

bool FixedRatioRetentionStrategy::MakeRetentionDecision(const FrameRetentionContext& ctx)
{
    cnt_ += ctx.preDroppedCount + 1;
    if (isFirstFrame_) {
        isFirstFrame_ = false;
        lastReason_ = SfdDecisionReason::POST_RETAIN_FIXED_ACC;
        AVCODEC_LOGD("FixedRatioRetentionStrategy decision: true (first frame)");
        return true;
    }

    bool isRetain = false;
    accumulator_ += targetRatio_ + (ctx.preDroppedCount * targetRatio_);
    if (accumulator_ >= ACCUMULATOR_THRESHOLD) {
        accumulator_ -= ACCUMULATOR_THRESHOLD;
        isRetain = true;
        lastReason_ = SfdDecisionReason::POST_RETAIN_FIXED_ACC;
    } else {
        dropCnt_++;
        lastReason_ = SfdDecisionReason::POST_DROP_FIXED_ACC;
    }

    AVCODEC_LOGD("FixedRatioRetentionStrategy decision: %{public}d "
                 "(cnt:%{public}d, drop:%{public}d, accum:%{public}.3f)",
                 isRetain, cnt_, dropCnt_, accumulator_);
    return isRetain;
}

void FixedRatioRetentionStrategy::Reset()
{
    accumulator_ = 0.0;
    isFirstFrame_ = true;
}

RetentionStrategyType FixedRatioRetentionStrategy::GetMode() const
{
    return RetentionStrategyType::FIXED_RATIO;
}

double FixedRatioRetentionStrategy::GetRatio() const
{
    return targetRatio_;
}

SfdDecisionReason FixedRatioRetentionStrategy::GetLastDecisionReason() const
{
    return lastReason_;
}

} // namespace SFD
} // namespace MediaAVCodec
} // namespace OHOS
