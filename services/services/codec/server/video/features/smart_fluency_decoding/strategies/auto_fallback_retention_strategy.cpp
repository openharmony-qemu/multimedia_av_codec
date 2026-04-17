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

namespace OHOS {
namespace MediaAVCodec {
namespace SFD {

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_SFD, "AutoFallbackStrategy"};
constexpr double TARGET_FPS = 30.0;
constexpr double MS_TO_S = 1000.0;
constexpr double PROFILING_DURATION_MIN_SEC = 0.1;
constexpr int32_t PROFILING_MIN_FRAMES = 30;
constexpr int64_t FALLBACK_FRAME_INTERVAL_MS = 33;
}

AutoFallbackRetentionStrategy::AutoFallbackRetentionStrategy() noexcept
{
     AVCODEC_LOGD("AutoFallbackRetentionStrategy created");
}

void AutoFallbackRetentionStrategy::UpdatePlaybackSpeed(double speed)
{
    if (speed <= 0.0) {
        return;
    }
    currentSpeed_ = speed;
    if (currentState_ == STABLE && sourceFps_ > 0) {
        double newRatio = TARGET_FPS / (sourceFps_ * currentSpeed_);
        stableDropper_ = std::make_unique<FixedRatioRetentionStrategy>(newRatio);
        AVCODEC_LOGI("AutoFallback: Speed updated during stable state, new ratio: %{public}.2f", newRatio);
    }
}

bool AutoFallbackRetentionStrategy::MakeRetentionDecision(const FrameRetentionContext& ctx)
{
    bool isRetain = false;
    if (currentState_ == STABLE && stableDropper_) {
        isRetain = stableDropper_->MakeRetentionDecision(ctx);
        AVCODEC_LOGD("AutoFallback decision: %{public}d (stable)", isRetain);
        return isRetain;
    }
    if (firstPts_ == -1) {
        firstPts_ = ctx.ptsMs;
    }
    decodedFrameCount_ += static_cast<int32_t>(1 + ctx.preDroppedCount);
    if (currentSpeed_ > 0.0 && decodedFrameCount_ >= PROFILING_MIN_FRAMES) {
        double durationSec = (ctx.ptsMs - firstPts_) / MS_TO_S;
        if (durationSec > PROFILING_DURATION_MIN_SEC) {
            sourceFps_ = decodedFrameCount_ / durationSec;
            double calculatedRatio = TARGET_FPS / (sourceFps_ * currentSpeed_);
            stableDropper_ = std::make_unique<FixedRatioRetentionStrategy>(calculatedRatio);
            currentState_ = STABLE;
            AVCODEC_LOGI("AutoFallback state transition: PROFILING -> STABLE, sourceFps: %{public}.2f, ratio: %{public}.2f", 
                         sourceFps_, calculatedRatio);
            isRetain = stableDropper_->MakeRetentionDecision(ctx);
            return isRetain;
        }
    }
    if (lastSysTime_ == 0 || (ctx.sysTimeMs - lastSysTime_) >= FALLBACK_FRAME_INTERVAL_MS) {
        lastSysTime_ = ctx.sysTimeMs;
        isRetain = true;
    } else {
        isRetain = false;
    }
    AVCODEC_LOGD("AutoFallback decision: %{public}d (profiling)", isRetain);
    return isRetain;
}

void AutoFallbackRetentionStrategy::Reset()
{
    currentState_ = PROFILING;
    stableDropper_.reset();
    decodedFrameCount_ = 0;
    firstPts_ = -1;
    lastSysTime_ = 0;
}

RetentionStrategyType AutoFallbackRetentionStrategy::GetMode() const
{
    return RetentionStrategyType::AUTO_RATIO;
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
