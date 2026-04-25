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

#include "frame_drop_filter.h"
#include <cinttypes>
#include "hitrace_meter.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "FDFilt"};
} // namespace

namespace OHOS {
namespace MediaAVCodec {
namespace PreProcessing {

int32_t FrameDropFilter::Configure(double dropToFps, double frameRate)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!std::isfinite(dropToFps) || dropToFps <= 0.0) {
        AVCODEC_LOGI("Drop filter disabled");
        return AVCS_ERR_OK;
    }
    dropToFps_ = dropToFps;
    frameRate_ = frameRate;
    strategyDecided_ = false;
    hasFirstPts_ = false;
    AVCODEC_LOGI("Drop %.2f->%.2f", frameRate_, dropToFps_);
    return AVCS_ERR_OK;
}

bool FrameDropFilter::ShouldDropFrame(uint64_t pts)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!strategyDecided_) {
        if (!hasFirstPts_) {
            hasFirstPts_ = true;
            firstPts_ = pts;
            AVCODEC_LOGI("First frame kept, pts=%{public}" PRIu64, pts);
            HITRACE_METER_FMT(HITRACE_TAG_ZMEDIA, "Drop pts=%{public}" PRIu64 " keep", pts);
            return false;
        }
        strategyDecided_ = true;
        if (pts <= firstPts_) {
            AVCODEC_LOGI("Drop strategy: ratio %.2f->%.2f", frameRate_, dropToFps_);
            activeStrategy_ = std::make_unique<RatioDropStrategy>(frameRate_, dropToFps_);
        } else {
            AVCODEC_LOGI("Drop strategy: timestamp ->%.2f", dropToFps_);
            activeStrategy_ = std::make_unique<TimestampDropStrategy>(dropToFps_);
        }
        // Retroactively feed first frame to let strategy initialize its internal state
        (void)activeStrategy_->ShouldDropFrame(firstPts_);
    }
    CHECK_AND_RETURN_RET_LOG(activeStrategy_ != nullptr, false, "activeStrategy_ is null");

    bool dropped = activeStrategy_->ShouldDropFrame(pts);
    HITRACE_METER_FMT(HITRACE_TAG_ZMEDIA, "Drop pts=%{public}" PRIu64 " %{public}s", pts, dropped ? "drop" : "keep");
    return dropped;
}

void FrameDropFilter::FlushTimeStamp()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (activeStrategy_ != nullptr) {
        activeStrategy_->FlushTimeStamp();
    }
    hasFirstPts_ = false;
}
} // namespace PreProcessing
} // namespace MediaAVCodec
} // namespace OHOS
