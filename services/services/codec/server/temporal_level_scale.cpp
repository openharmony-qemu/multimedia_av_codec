/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include "temporal_level_scale.h"
#include <cstdint>
#include "securec.h"
#include "meta/meta.h"
#include "avcodec_log.h"
#include "avcodec_errors.h"
#include "buffer/avbuffer.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "CodecServer"};
} // namespace

constexpr int32_t DEFAULT_TEMPORAL_GOPSIZE = 4;
constexpr int32_t DEFAULT_TEMPORAL_REFERENCE_MODE = 1;
constexpr int32_t DEFAULT_VIDEO_LTR_FRAME_NUM = 2;
constexpr int32_t ADJACENT_REFERENCE_MODE = 1;
constexpr int32_t JUMP_REFERENCE_MODE = 2;
constexpr int32_t FRAME_INTERVAL_TRANS_RATE = 1000;

namespace OHOS {
namespace MediaAVCodec {
using namespace Media;

int32_t TemporalLevelScale::CheckTemporalLevelScaleParam(Format &format)
{
    format.GetDoubleValue(Tag::VIDEO_FRAME_RATE, frameRate_);
    format.GetIntValue(Tag::VIDEO_I_FRAME_INTERVAL, frameInterval_);
    gopSize_ = static_cast<int32_t>(frameRate_ * frameInterval_ / FRAME_INTERVAL_TRANS_RATE);
    bool hasScaleEncGopSize = false;
    bool hasScaleEncRefMode = false;
    if (format.ContainKey(Tag::VIDEO_ENCODER_TEMPORAL_GOP_SIZE)) {
        hasScaleEncGopSize = true;
        format.GetIntValue(Tag::VIDEO_ENCODER_TEMPORAL_GOP_SIZE, tGopSize_);
    }
    if (format.ContainKey(Tag::VIDEO_ENCODER_TEMPORAL_GOP_REFERENCE_MODE)) {
        hasScaleEncRefMode = true;
        format.GetIntValue(Tag::VIDEO_ENCODER_TEMPORAL_GOP_REFERENCE_MODE, tRefMode_);
    }
    if (!hasScaleEncGopSize && !hasScaleEncRefMode) {
        tGopSize_ = DEFAULT_TEMPORAL_GOPSIZE;
        tRefMode_ = DEFAULT_TEMPORAL_REFERENCE_MODE;
        AVCODEC_LOGW("Temporal level scale encode enable, but param miss, use default param!");
    } else if (hasScaleEncGopSize && !hasScaleEncRefMode) {
        tRefMode_ = DEFAULT_TEMPORAL_REFERENCE_MODE;
        AVCODEC_LOGW("Temporal level scale encode enable, but reference mode param miss, use default param!");
    } else if (!hasScaleEncGopSize && hasScaleEncRefMode) {
        tGopSize_ = DEFAULT_TEMPORAL_GOPSIZE;
        AVCODEC_LOGW("Temporal level scale encode enable, but temporal gop size param miss, use default param!");
    } else {
        if (tRefMode_ != ADJACENT_REFERENCE_MODE && tRefMode_ != JUMP_REFERENCE_MODE) {
            AVCODEC_LOGE("Temporal level scale encode reference mode param error!");
            return AVCS_ERR_INVALID_VAL;
        }
        AVCODEC_LOGI("Temporal level scale encode reference mode param set successful!");
        if (tGopSize_ <= 1 || tGopSize_ >= static_cast<int32_t>(gopSize_)) {
            AVCODEC_LOGE("Temporal level scale encode temporal gop size param error!");
            return AVCS_ERR_INVALID_VAL;
        }
        AVCODEC_LOGI("Temporal level scale encode temporal gop size param set successful!");
    }
    format.PutIntValue(Tag::VIDEO_ENCODER_LTR_FRAME_NUM, DEFAULT_VIDEO_LTR_FRAME_NUM);
    AVCODEC_LOGI("Temporal level scale encode parameter set successful!");
    return AVCS_ERR_OK;
}

void TemporalLevelScale::StoreAVBuffer(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    std::lock_guard<std::shared_mutex> temporalLevelScaleLock(temporalLevelScaleMutex_);
    inputBufferMap_.emplace(index, buffer);
}

void TemporalLevelScale::LTRDecision()
{
    poc_ = syncFrameNum_ % gopSize_;
    temporalPoc_ = poc_ % tGopSize_;

    if (temporalPoc_ == 0) {
        isMarkLTR_ = true;
        if (poc_ == 0) {
            isUseLTR_ = false;
            ltrPoc_ = 0;
        } else {
            isUseLTR_ = true;
            ltrPoc_ = poc_ - tGopSize_;
        }
    } else if (temporalPoc_ == 1) {
        isMarkLTR_ = false;
        isUseLTR_ = false;
        ltrPoc_ = poc_ - 1;
    } else {
        isMarkLTR_ = false;
        if (tRefMode_ == ADJACENT_REFERENCE_MODE) {
            isUseLTR_ = false;
            ltrPoc_ = poc_ - 1;
        } else {
            isUseLTR_ = true;
            ltrPoc_ = poc_ - temporalPoc_;
        }
    }
}

void TemporalLevelScale::ConfigureLTR(uint32_t index)
{
    std::lock_guard<std::shared_mutex> temporalLevelScaleLock(temporalLevelScaleMutex_);
    if (inputBufferMap_.find(index) != inputBufferMap_.end()) {
        bool syncIDR;
        if (inputBufferMap_[index]->meta_->GetData(Tag::VIDEO_REQUEST_I_FRAME, syncIDR) && syncIDR) {
            syncFrameNum_ = 0;
            inputBufferMap_[index]->meta_->SetData(Tag::VIDEO_REQUEST_I_FRAME, syncIDR);
            AVCODEC_LOGI("Temporal level scale encode sync IDR frame successful!");
        }
        LTRDecision();
        inputBufferMap_[index]->meta_->SetData(Tag::VIDEO_ENCODER_PER_FRAME_MARK_LTR, isMarkLTR_);
        inputBufferMap_[index]->meta_->SetData(Tag::VIDEO_ENCODER_PER_FRAME_USE_LTR, isUseLTR_);
        inputBufferMap_[index]->meta_->SetData(Tag::VIDEO_PER_FRAME_POC, ltrPoc_);
        inputBufferMap_.erase(index);
        AVCODEC_LOGD("frame: %{public}d set ltrParm, isMarkLTR: %{public}d, isUseLTR: %{public}d, ltrPoc: %{public}d",
                     syncFrameNum_, isMarkLTR_, isUseLTR_, ltrPoc_);
    }
    syncFrameNum_++;
}
} // namespace MediaAVCodec
} // namespace OHOS