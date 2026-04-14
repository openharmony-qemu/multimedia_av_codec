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

#include "retention_strategy_factory.h"
#include "avcodec_log.h"
#include "retention_strategy.h"
#include "adaptive_retention_strategy.h"
#include "full_retention_strategy.h"
#include "auto_fallback_retention_strategy.h"
#include "fixed_ratio_retention_strategy.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_SFD, "StrategyFactory"};
}

namespace OHOS {
namespace MediaAVCodec {
namespace SFD {

std::unique_ptr<IRetentionStrategy> RetentionStrategyFactory::CreateStrategy(
    RetentionStrategyType mode,
    const std::optional<double>& retentionRate)
{
    switch (mode) {
        case RetentionStrategyType::FULL:
            return std::make_unique<FullRetentionStrategy>();
        case RetentionStrategyType::ADAPTIVE:
            return std::make_unique<AdaptiveRetentionStrategy>();
        case RetentionStrategyType::AUTO_RATIO:
            return std::make_unique<AutoFallbackRetentionStrategy>();
        case RetentionStrategyType::FIXED_RATIO:
            if (retentionRate.has_value()) {
                return std::make_unique<FixedRatioRetentionStrategy>(retentionRate.value());
            }
            return std::make_unique<AutoFallbackRetentionStrategy>();
        default:
            AVCODEC_LOGE("Unknown mode: %{public}d", static_cast<int>(mode));
            return nullptr;
    }
}
} // namespace SFD
} // namespace MediaAVCodec
} // namespace OHOS
