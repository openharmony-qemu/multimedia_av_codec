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
#include "meta/video_types.h"
#include "avcodec_log.h"
#include "avcodec_errors.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "CodecServer"};
} // namespace

constexpr int32_t DEFAULT_TEMPORAL_GOPSIZE = 4;
constexpr int32_t DEFAULT_VIDEO_LTR_FRAME_NUM = 2;
constexpr int32_t SECOND_TO_MILL = 1000;
constexpr int32_t DEFAULT_I_FRAME_INTERVAL = 2000;
constexpr double DEFAULT_FRAME_RATE = 30.0;

namespace OHOS {
namespace MediaAVCodec {
using namespace Media;
using namespace Plugins;

void TemporalLevelScale::ConfigFrameGop(Format &format)
{
    if (format.GetDoubleValue(Tag::VIDEO_FRAME_RATE, frameRate_)) {
        if (frameRate_ <= 0.0) {
            frameRate_ = DEFAULT_FRAME_RATE;
            format.PutDoubleValue(Tag::VIDEO_FRAME_RATE, DEFAULT_FRAME_RATE);
            AVCODEC_LOGW("Farme rate param error, use default frame rate %{public}.2lf!", frameRate_);
        } else {
            AVCODEC_LOGI("Frame rate %{public}.2lf set successful!", frameRate_);
        }
    } else {
        frameRate_ = DEFAULT_FRAME_RATE;
        format.PutDoubleValue(Tag::VIDEO_FRAME_RATE, DEFAULT_FRAME_RATE);
        AVCODEC_LOGI("Farme rate param miss, use default frame rate %{public}.2lf!", frameRate_);
    }
    if (format.GetIntValue(Tag::VIDEO_I_FRAME_INTERVAL, frameInterval_)) {
        if (frameInterval_ == 0) {
            frameInterval_ = DEFAULT_I_FRAME_INTERVAL;
            format.PutIntValue(Tag::VIDEO_I_FRAME_INTERVAL, DEFAULT_I_FRAME_INTERVAL);
            AVCODEC_LOGW("I frame interval 0 is error, use default value %{public}d!", frameInterval_);
        } else {
            AVCODEC_LOGI("I frame interval param %{public}d set successful!", frameInterval_);
        }
    } else {
        frameInterval_ = DEFAULT_I_FRAME_INTERVAL;
        format.PutIntValue(Tag::VIDEO_I_FRAME_INTERVAL, DEFAULT_I_FRAME_INTERVAL);
        AVCODEC_LOGI("I frame interval param miss, use default i frame interval %{public}d!", frameInterval_);
    }
    if (frameInterval_ < 0) {
        gopSize_ = INT32_MAX;
    } else {
        gopSize_ = static_cast<int32_t>(frameRate_ * frameInterval_ / SECOND_TO_MILL);
    }
}

int32_t TemporalLevelScale::CheckTemporalLevelScaleParam(Format &format)
{
    ConfigFrameGop(format);
    if (format.GetIntValue(Tag::VIDEO_ENCODER_TEMPORAL_GOP_SIZE, temporalGopSize_)) {
        if (temporalGopSize_ <= 1 || temporalGopSize_ >= static_cast<int32_t>(gopSize_)) {
            AVCODEC_LOGE("Temporal level scale encode temporal gop size param error!");
            return AVCS_ERR_INVALID_VAL;
        } else {
            AVCODEC_LOGI("Temporal level scale encode temporal gop size set successful!");
        }
    } else {
        temporalGopSize_ = DEFAULT_TEMPORAL_GOPSIZE;
        AVCODEC_LOGW("Temporal level scale encode enable, but temporal gop size param miss, use default param!");
    }
    if (format.GetIntValue(Tag::VIDEO_ENCODER_TEMPORAL_GOP_REFERENCE_MODE, tRefMode_)) {
        if (tRefMode_ >= static_cast<int32_t>(TemporalGopReferenceMode::ADJACENT_REFERENCE_MODE) &&
            tRefMode_ <= static_cast<int32_t>(TemporalGopReferenceMode::JUMP_REFERENCE_MODE)) {
            AVCODEC_LOGI("Temporal level scale encode temporal reference mode set successful!");
        } else {
            AVCODEC_LOGE("Temporal level scale encode reference mode param error!");
            return AVCS_ERR_INVALID_VAL;
        }
    } else {
        tRefMode_ = static_cast<int32_t>(TemporalGopReferenceMode::ADJACENT_REFERENCE_MODE);
        AVCODEC_LOGW("Temporal level scale encode enable, but reference mode param miss, use default param!");
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
    poc_ = frameNum_ % gopSize_;
    temporalPoc_ = poc_ % temporalGopSize_;

    if (temporalPoc_ == 0) {
        isMarkLTR_ = true;
        if (poc_ == 0) {
            isUseLTR_ = false;
            ltrPoc_ = 0;
        } else {
            isUseLTR_ = true;
            ltrPoc_ = poc_ - temporalGopSize_;
        }
    } else if (temporalPoc_ == 1) {
        isMarkLTR_ = false;
        isUseLTR_ = false;
        ltrPoc_ = poc_ - 1;
    } else {
        isMarkLTR_ = false;
        if (tRefMode_ == static_cast<int32_t>(TemporalGopReferenceMode::ADJACENT_REFERENCE_MODE)) {
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
            frameNum_ = 0;
            AVCODEC_LOGI("Temporal level scale encode sync IDR frame successful!");
        }
        LTRDecision();
        inputBufferMap_[index]->meta_->SetData(Tag::VIDEO_ENCODER_PER_FRAME_MARK_LTR, isMarkLTR_);
        inputBufferMap_[index]->meta_->SetData(Tag::VIDEO_ENCODER_PER_FRAME_USE_LTR, isUseLTR_);
        inputBufferMap_[index]->meta_->SetData(Tag::VIDEO_PER_FRAME_POC, ltrPoc_);
        inputBufferMap_.erase(index);
        AVCODEC_LOGD("frame: %{public}d set ltrParm, isMarkLTR: %{public}d, isUseLTR: %{public}d, ltrPoc: %{public}d",
                     frameNum_, isMarkLTR_, isUseLTR_, ltrPoc_);
        frameNum_++;
    } else {
        AVCODEC_LOGE("Not find matched buffer ID: %{public}d", index);
    }
}
} // namespace MediaAVCodec
} // namespace OHOS