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

#include "i_frame_retention_strategy.h"
#include <memory>

namespace OHOS {
namespace MediaAVCodec {
namespace SFD {

class AdaptiveRetentionStrategy : public IFrameRetentionStrategy {
public:
    AdaptiveRetentionStrategy() = default;
    ~AdaptiveRetentionStrategy() override = default;

    bool MakeRetentionDecision(const FrameRetentionContext& ctx) override {
        if (currentSpeed_ <= 1.2) {
            return true;
        }

        if (ctx.mvStats && ctx.mvStats->validBlocks > 0) {
            double consistency = ctx.mvStats->motionConsistency;
            double magnitude = ctx.mvStats->perceptualMagnitude;
            
            // 规则 1：极高的一致性 (如全局平移) -> 严厉禁止丢帧
            if (consistency > 0.8) {
                dropCount_ = 0;
                currentTargetRatio_ = 0.0;
                return true; 
            }

            // 规则 2：高幅度且极低致性 (如混乱爆炸场景) -> 激进丢帧 (人眼难以追踪)
            if (magnitude > 30.0 && consistency < 0.3) {
                dropCount_++;
                uint32_t maxDrops = static_cast<uint32_t>((currentSpeed_ - 1.0) * 30.0);
                currentTargetRatio_ = static_cast<double>(maxDrops) / (maxDrops + 1.0);
                if (dropCount_ <= maxDrops) {
                    return false;
                } else {
                    dropCount_ = 0;
                    return true;
                }
            }
            
            // 规则 3：默认回退逻辑 (根据平均参考距离，处理中度运动)
            if (ctx.mvStats->avgRefDist > 5.0 * currentSpeed_) {
                dropCount_++;
                uint32_t maxDrops = static_cast<uint32_t>((currentSpeed_ - 1.0) * 15.0);
                currentTargetRatio_ = static_cast<double>(maxDrops) / (maxDrops + 1.0);
                if (dropCount_ <= maxDrops) {
                    return false; 
                }
            }
            
            dropCount_ = 0; 
            currentTargetRatio_ = 0.0;
        }
        
        return true;
    }

    void UpdatePlaybackSpeed(double speed) override {
        currentSpeed_ = speed > 0.0 ? speed : 1.0;
    }

    void Reset() override {
        dropCount_ = 0;
    }

    FrameRetentionMode GetMode() const override {
        return FrameRetentionMode::ADAPTIVE;
    }

    double GetCurrentRetentionRatio() const override {
        return 1.0 - currentTargetRatio_;
    }

private:
    double currentSpeed_{1.0};
    uint32_t dropCount_{0};
    double currentTargetRatio_{0.0};
};

} // namespace SFD
} // namespace MediaAVCodec
} // namespace OHOS

#endif