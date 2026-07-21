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

#ifndef ADAPTIVE_RETENTION_STRATEGY_H
#define ADAPTIVE_RETENTION_STRATEGY_H

#include "retention_strategy.h"
#include <memory>

namespace OHOS {
namespace MediaAVCodec {
namespace SFD {
class AdaptiveRetentionStrategy : public IRetentionStrategy {
public:
    AdaptiveRetentionStrategy();
    ~AdaptiveRetentionStrategy() override = default;

    bool MakeRetentionDecision(const FrameRetentionContext& ctx) override;
    void UpdatePlaybackSpeed(double speed) override;
    void UpdateSourceFramerate(double sourceFps) override;
    void Reset() override;
    RetentionStrategyType GetMode() const override;
    double GetRatio() const override;
    SfdDecisionReason GetLastDecisionReason() const override;
    bool RequiresMvAnalysis() const override;
    bool ShouldReportDfx() const override;

private:
    double CalculateTargetRatio(const FrameRetentionContext& ctx) const;
    double EvaluateStutterRisk(const MVStats& mvStats) const;
    bool ExecuteAccumulatorDecision(double dynamicRatio, uint32_t preDroppedCount);

    double currentSpeed_{1.0};
    double targetRatio_{1.0};
    double accumulator_{0.0};
    double sourceFps_{30.0}; // 30.0 fps default
    SfdDecisionReason lastReason_{SfdDecisionReason::UNSPECIFIED};
};

} // namespace SFD
} // namespace MediaAVCodec
} // namespace OHOS
#endif