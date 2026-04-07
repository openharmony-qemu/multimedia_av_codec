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
#include <cstdio>
#include "avcodec_log.h"

namespace {
// constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_SFD, "UniformFallbackDropper"};
// 固定 30fps 输出限制
// static constexpr int32_t TARGET_FPS = 30;
// static constexpr int64_t FRAME_INTERVAL_US = 1000000 / TARGET_FPS;  // 33.33ms
}

namespace OHOS {
namespace MediaAVCodec {
namespace SFD {

AutoFallbackRetentionStrategy::AutoFallbackRetentionStrategy()
{
}

void AutoFallbackRetentionStrategy::UpdatePlaybackSpeed(double speed)
{
    if (speed <= 0.0) return;
    currentSpeed_ = speed;
    
    // 动态适配：如果在稳态期切了倍速，立即重新计算比例并重置嵌套的 Dropper
    if (currentState_ == STABLE && sourceFps_ > 0) {
        double newRatio = 30.0 / (sourceFps_ * currentSpeed_);
        stableDropper_ = std::make_unique<FixedRatioRetentionStrategy>(newRatio);
    }
}

bool AutoFallbackRetentionStrategy::MakeRetentionDecision(const FrameRetentionContext& ctx)
{
    // --- 1. 稳态期：直接委托给内部的 RatioDropper ---
    if (currentState_ == STABLE && stableDropper_) {
        return stableDropper_->MakeRetentionDecision(ctx);
    }

    // --- 2. 统计期 (PROFILING) 逻辑 ---
    if (firstPts_ == -1) firstPts_ = ctx.ptsMs;
    // 之前逻辑漏掉了前置丢帧，会导致 sourceFps 估算偏低
    decodedFrameCount_ += (1 + ctx.preDroppedCount);

    // 探针：是否满足切换到稳态的条件 (统计 30 帧且耗时大于 100ms)
    if (currentSpeed_ > 0.0 && decodedFrameCount_ >= 30) {
        double durationSec = (ctx.ptsMs - firstPts_) / 1000.0;
        
        // 增加最小时长阈值 (0.1s) 防御，防止 PTS 抖动导致 FPS 估算爆炸
        if (durationSec > 0.1) {
            sourceFps_ = decodedFrameCount_ / durationSec;
            double calculatedRatio = 30.0 / (sourceFps_ * currentSpeed_);
            
            // 状态切换并实例化嵌套 Dropper
            stableDropper_ = std::make_unique<FixedRatioRetentionStrategy>(calculatedRatio);
            currentState_ = STABLE;
            
            return stableDropper_->MakeRetentionDecision(ctx);
        }
    }

    // 物理时间防突发截断 (按 30fps 约 33ms 间隔控制)
    if (lastSysTime_ == 0 || (ctx.sysTimeMs - lastSysTime_) >= 33) {
        lastSysTime_ = ctx.sysTimeMs;
        return true;
    }

    return false;
}

void AutoFallbackRetentionStrategy::Reset()
{
    currentState_ = PROFILING;
    stableDropper_.reset();
    decodedFrameCount_ = 0;
    firstPts_ = -1;
    lastSysTime_ = 0;
}

FrameRetentionMode AutoFallbackRetentionStrategy::GetMode() const
{
    return FrameRetentionMode::AUTO_RATIO;
}

double AutoFallbackRetentionStrategy::GetCurrentRetentionRatio() const
{
    if (currentState_ == STABLE && stableDropper_) {
        return stableDropper_->GetCurrentRetentionRatio();
    }
    return 1.0;
}

} // namespace SFD
} // namespace MediaAVCodec
} // namespace OHOS
