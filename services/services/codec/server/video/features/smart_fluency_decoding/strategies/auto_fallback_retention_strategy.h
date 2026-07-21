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

#ifndef AUTO_FALLBACK_RETENTION_STRATEGY_H
#define AUTO_FALLBACK_RETENTION_STRATEGY_H

#include <memory>
#include "retention_strategy.h"
#include "fixed_ratio_retention_strategy.h"

namespace OHOS {
namespace MediaAVCodec {
namespace SFD {

class AutoFallbackRetentionStrategy : public IRetentionStrategy {
public:
    AutoFallbackRetentionStrategy();
    ~AutoFallbackRetentionStrategy() override = default;

    bool MakeRetentionDecision(const FrameRetentionContext& ctx) override;
    void UpdatePlaybackSpeed(double speed) override;
    void UpdateSourceFramerate(double sourceFps) override;
    void Reset() override;
    RetentionStrategyType GetMode() const override;
    double GetRatio() const override;
    SfdDecisionReason GetLastDecisionReason() const override;
    bool IsPassthrough() const override;
    bool RequiresNaluAnalysis() const override;
    bool RequiresMvAnalysis() const override;
    bool ShouldReportDfx() const override;

private:
    double CalculateFallbackRatio() const;
    void ApplyCalculatedRatio();

    bool hasSourceFps_{false};
    double sourceFps_{0.0};
    double currentSpeed_{1.0};
    std::unique_ptr<FixedRatioRetentionStrategy> fallbackDropper_{nullptr};
};

} // namespace SFD
} // namespace MediaAVCodec
} // namespace OHOS
#endif
