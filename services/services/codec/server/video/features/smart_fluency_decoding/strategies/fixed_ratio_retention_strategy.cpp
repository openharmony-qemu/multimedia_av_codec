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
constexpr double MIN_RETENTION_RATIO = 0.01;
constexpr double MAX_RETENTION_RATIO = 1.0;
constexpr double ACCUMULATOR_THRESHOLD = 1.0;
}

namespace OHOS {
namespace MediaAVCodec {
namespace SFD {

FixedRatioRetentionStrategy::FixedRatioRetentionStrategy(double ratio)
{
    retentionRatio_ = std::max(MIN_RETENTION_RATIO, std::min(ratio, MAX_RETENTION_RATIO));
    AVCODEC_LOGI("Uniform Ratio Construct in, retention ratio is %{public}.2f", ratio);
}

bool FixedRatioRetentionStrategy::MakeRetentionDecision(const FrameRetentionContext& ctx)
{
    cnt_ += ctx.preDroppedCount + 1;
    if (isFirstFrame_) {
        isFirstFrame_ = false;
        return true;
    }

    bool isRetain = false;
    accumulator_ += retentionRatio_ + (ctx.preDroppedCount * retentionRatio_);
    if (accumulator_ >= ACCUMULATOR_THRESHOLD) {
        accumulator_ -= ACCUMULATOR_THRESHOLD;
        isRetain = true;
    } else {
        dropCnt_++;
    }
    
    AVCODEC_LOGI("cnt %{public}d, dropCnt_ %{public}d, accumulator_ %{public}.2f, preDroppedCount %{public}u",
        cnt_, dropCnt_, accumulator_, ctx.preDroppedCount);
    return isRetain;
}

void FixedRatioRetentionStrategy::Reset()
{
    accumulator_ = 0.0;
    isFirstFrame_ = true;
}

void FixedRatioRetentionStrategy::UpdatePlaybackSpeed(double speed)
{
    (void)speed;
}

RetentionStrategyType FixedRatioRetentionStrategy::GetMode() const
{
    return RetentionStrategyType::FIXED_RATIO;
}

double FixedRatioRetentionStrategy::GetCurrentRetentionRatio() const
{
    return retentionRatio_;
}

} // namespace SFD
} // namespace MediaAVCodec
} // namespace OHOS
