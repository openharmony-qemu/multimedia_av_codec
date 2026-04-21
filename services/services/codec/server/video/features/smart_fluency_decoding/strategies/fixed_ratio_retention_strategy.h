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

#ifndef FIXED_RATIO_RETENTION_STRATEGY_H
#define FIXED_RATIO_RETENTION_STRATEGY_H

#include <memory>
#include <optional>
#include "retention_strategy.h"

namespace OHOS {
namespace MediaAVCodec {
namespace SFD {

class FixedRatioRetentionStrategy : public IRetentionStrategy {
public:
    explicit FixedRatioRetentionStrategy(double ratio);
    ~FixedRatioRetentionStrategy() override = default;

    bool MakeRetentionDecision(const FrameRetentionContext& ctx) override;
    void Reset() override;
    RetentionStrategyType GetMode() const override;
    double GetRatio() const override;
    SfdDecisionReason GetLastDecisionReason() const override;

private:
    double accumulator_{0.0};
    double targetRatio_{1.0};
    bool isFirstFrame_{true};
    uint32_t cnt_{0};
    uint32_t dropCnt_{0};
    SfdDecisionReason lastReason_{SfdDecisionReason::UNSPECIFIED};
};

} // namespace SFD
} // namespace MediaAVCodec
} // namespace OHOS
#endif
