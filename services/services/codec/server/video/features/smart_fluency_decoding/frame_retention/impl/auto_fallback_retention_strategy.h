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
#include "i_frame_retention_strategy.h"
#include "fixed_ratio_retention_strategy.h"

namespace OHOS {
namespace MediaAVCodec {
namespace SFD {

class AutoFallbackRetentionStrategy : public IFrameRetentionStrategy {
public:
    AutoFallbackRetentionStrategy();
    ~AutoFallbackRetentionStrategy() override = default;

    bool MakeRetentionDecision(const FrameRetentionContext& ctx) override;

    void UpdatePlaybackSpeed(double speed) override;

    void Reset() override;

    FrameRetentionMode GetMode() const override;

    double GetCurrentRetentionRatio() const override;

private:
    enum State { 
        PROFILING, 
        STABLE
    };
    State currentState_{PROFILING};
    // 嵌套持有一个纯比例丢帧器，实现代码零冗余
    std::unique_ptr<FixedRatioRetentionStrategy> stableDropper_{nullptr};
    double currentSpeed_{-1.0};
    double sourceFps_{30.0}; 

    int32_t decodedFrameCount_{0};
    int64_t firstPts_{-1};
    int64_t lastSysTime_{0};
};

} // namespace SFD
} // namespace MediaAVCodec
} // namespace OHOS

#endif 
