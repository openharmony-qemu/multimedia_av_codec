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
#include <inttypes.h>

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "FDFilt"};
} // namespace

namespace OHOS {
namespace MediaAVCodec {
namespace PreProcessing {

int32_t FrameDropFilter::Configure(double dropToFps, double frameRate)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (dropToFps <= 0.0) {
        AVCODEC_LOGI("Drop filter not configured, disabled");
        return AVCS_ERR_OK;
    }
    dropToFps_ = dropToFps;
    frameRate_ = frameRate;
    strategyDecided_ = false;
    firstPts_ = -1;
    totalFrames_ = 0;
    droppedFrames_ = 0;
    AVCODEC_LOGI("Configured: dropToFps=%{public}.2f, frameRate=%{public}.2f", dropToFps_, frameRate_);
    return AVCS_ERR_OK;
}

bool FrameDropFilter::ShouldDropFrame(int64_t pts)
{
    std::lock_guard<std::mutex> lock(mutex_);
    totalFrames_++;

    if (!strategyDecided_) {
        if (firstPts_ < 0) {
            firstPts_ = pts;
            AVCODEC_LOGI("First frame kept, pts=%{public}" PRId64, pts);
            return false;
        }

        strategyDecided_ = true;
        if (pts == firstPts_) {
            AVCODEC_LOGI("Timestamp unchanged, using ratio strategy (frameRate=%{public}.2f)", frameRate_);
            activeStrategy_ = std::make_unique<RatioDropStrategy>(frameRate_, dropToFps_);
        } else {
            AVCODEC_LOGI("Timestamp changed, using timestamp strategy (targetFps=%{public}.2f)", dropToFps_);
            activeStrategy_ = std::make_unique<TimestampDropStrategy>(dropToFps_);
        }

        bool shouldDrop = activeStrategy_->ShouldDropFrame(pts, totalFrames_);
        if (shouldDrop) {
            droppedFrames_++;
        }
        return shouldDrop;
    }

    if (activeStrategy_ == nullptr) {
        AVCODEC_LOGE("Strategy decided but activeStrategy_ is null, keeping frame");
        return false;
    }

    bool shouldDrop = activeStrategy_->ShouldDropFrame(pts, totalFrames_);
    if (shouldDrop) {
        droppedFrames_++;
        AVCODEC_LOGD("Drop frame: index=%{public}" PRIu64 ", pts=%{public}" PRId64, totalFrames_, pts);
    }

    return shouldDrop;
}

int32_t FrameDropFilter::Reset()
{
    std::lock_guard<std::mutex> lock(mutex_);
    activeStrategy_.reset();
    strategyDecided_ = false;
    firstPts_ = -1;
    totalFrames_ = 0;
    droppedFrames_ = 0;
    AVCODEC_LOGI("Filter reset");
    return AVCS_ERR_OK;
}
} // namespace PreProcessing
} // namespace MediaAVCodec
} // namespace OHOS
