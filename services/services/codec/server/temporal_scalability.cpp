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

#include "temporal_scalability.h"
#include "meta/video_types.h"
#include "avcodec_log.h"
#include "avcodec_common.h"
#include "avcodec_errors.h"
#include <cstdint>

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "TemporalScalability"};
} // namespace

constexpr int32_t MIN_TEMPORAL_GOPSIZE = 2;
constexpr int32_t DEFAULT_TEMPORAL_GOPSIZE = 4;
constexpr int32_t DEFAULT_VIDEO_LTR_FRAME_NUM = 2;
constexpr int32_t SECOND_TO_MILL = 1000;
constexpr int32_t DEFAULT_I_FRAME_INTERVAL = 2000;
constexpr int32_t ENABLE_PARAMETER_CALLBACK = 1;
constexpr double DEFAULT_FRAME_RATE = 30.0;

namespace OHOS {
namespace MediaAVCodec {
using namespace Media;
using namespace Plugins;

TemporalScalability::TemporalScalability()
{
    inputIndexQueue_ = std::make_shared<BlockQueue<uint32_t>>("inputIndexQueue");
}

TemporalScalability::~TemporalScalability()
{
    inputIndexQueue_->Clear();
}

void TemporalScalability::ConfigFrameGop(Format &format)
{
    if (format.GetDoubleValue(Tag::VIDEO_FRAME_RATE, frameRate_) && frameRate_ > 0.0) {
        AVCODEC_LOGI("Set frame rate successfully, value is %{public}.2lf.", frameRate_);
    } else {
        frameRate_ = DEFAULT_FRAME_RATE;
        format.PutDoubleValue(Tag::VIDEO_FRAME_RATE, DEFAULT_FRAME_RATE);
        AVCODEC_LOGI("Get frame rate failed, use default value %{public}.2lf.", frameRate_);
    }
    if (format.GetIntValue(Tag::VIDEO_I_FRAME_INTERVAL, frameInterval_) && frameInterval_ != 0) {
        AVCODEC_LOGI("Set i frame interval successfully, value is %{public}d.", frameInterval_);
    } else {
        frameInterval_ = DEFAULT_I_FRAME_INTERVAL;
        format.PutIntValue(Tag::VIDEO_I_FRAME_INTERVAL, DEFAULT_I_FRAME_INTERVAL);
        AVCODEC_LOGI("Get i frame interval failed, use default value %{public}d.", frameInterval_);
    }
    if (frameInterval_ < 0) {
        gopSize_ = INT32_MAX;
    } else {
        gopSize_ = static_cast<int32_t>(frameRate_ * frameInterval_ / SECOND_TO_MILL);
    }
}

int32_t TemporalScalability::ValidateTemporalGopParam(Format &format)
{
    ConfigFrameGop(format);
    if (gopSize_ <= MIN_TEMPORAL_GOPSIZE) {
        AVCODEC_LOGE("Unsuppoted gop size, should be greater than 2!");
        return AVCS_ERR_INVALID_VAL;
    }
    if (format.GetIntValue(Tag::VIDEO_ENCODER_TEMPORAL_GOP_SIZE, temporalGopSize_)) {
        if (temporalGopSize_ <= 1) {
            AVCODEC_LOGE("Set temporal gop size failed, value is %{public}d, should be greater than 1!",
                         temporalGopSize_);
            return AVCS_ERR_INVALID_VAL;
        } else if (temporalGopSize_ >= static_cast<int32_t>(gopSize_)) {
            AVCODEC_LOGE(
                "Set temporal gop size failed, value is %{public}d, should be less than gop size, which is %{public}u!",
                temporalGopSize_, gopSize_);
            return AVCS_ERR_INVALID_VAL;
        } else {
            AVCODEC_LOGI("Set temporal gop size successfully, value is %{public}d.", temporalGopSize_);
        }
    } else {
        temporalGopSize_ = gopSize_ <= DEFAULT_TEMPORAL_GOPSIZE ? MIN_TEMPORAL_GOPSIZE : DEFAULT_TEMPORAL_GOPSIZE;
        AVCODEC_LOGI("Get temporal gop size failed, use default value %{public}d.", temporalGopSize_);
    }
    if (format.GetIntValue(Tag::VIDEO_ENCODER_TEMPORAL_GOP_REFERENCE_MODE, tRefMode_)) {
        if (tRefMode_ >= static_cast<int32_t>(TemporalGopReferenceMode::ADJACENT_REFERENCE) &&
            tRefMode_ <= static_cast<int32_t>(TemporalGopReferenceMode::JUMP_REFERENCE)) {
            AVCODEC_LOGI("Set temporal reference mode successfully.");
        } else {
            AVCODEC_LOGE("Set temporal reference mode failed, should be ADJACENT_REFERENCE or JUMP_REFERENCE!");
            return AVCS_ERR_INVALID_VAL;
        }
    } else {
        tRefMode_ = static_cast<int32_t>(TemporalGopReferenceMode::ADJACENT_REFERENCE);
        AVCODEC_LOGI("Get temporal reference mode failed, use default value ADJACENT_REFERENCE.");
    }
    format.PutIntValue(Tag::VIDEO_ENCODER_LTR_FRAME_COUNT, DEFAULT_VIDEO_LTR_FRAME_NUM);
    format.PutIntValue(Tag::VIDEO_ENCODER_ENABLE_SURFACE_INPUT_CALLBACK, ENABLE_PARAMETER_CALLBACK);
    AVCODEC_LOGI("Set temporal gop parameter successfully.");
    return AVCS_ERR_OK;
}

void TemporalScalability::StoreAVBuffer(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    inputIndexQueue_->Push(index);
    std::lock_guard<std::shared_mutex> inputBufLock(inputBufMutex_);
    inputBufferMap_.emplace(index, buffer);
}

uint32_t TemporalScalability::GetFirstBufferIndex()
{
    return inputIndexQueue_->Front();
}

void TemporalScalability::SetBlockQueueActive()
{
    inputIndexQueue_->SetActive(false, false);
}

void TemporalScalability::SetDisposableFlag(std::shared_ptr<Media::AVBuffer> buffer)
{
    uint32_t flag = frameFlagMap_[outputFrameCounter_];
    if (flag != AVCODEC_BUFFER_FLAG_NONE) {
        buffer->flag_ |= flag;
    }
    frameFlagMap_.erase(outputFrameCounter_);
    outputFrameCounter_++;
}

void TemporalScalability::LTRDecision()
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
        if (tRefMode_ == static_cast<int32_t>(TemporalGopReferenceMode::ADJACENT_REFERENCE)) {
            isUseLTR_ = false;
            ltrPoc_ = poc_ - 1;
        } else {
            isUseLTR_ = true;
            ltrPoc_ = poc_ - temporalPoc_;
        }
    }
}

void TemporalScalability::DisposableDecision()
{
    uint32_t flag = AVCODEC_BUFFER_FLAG_NONE;
    if (!isMarkLTR_) {
        if (tRefMode_ == static_cast<int32_t>(TemporalGopReferenceMode::ADJACENT_REFERENCE) &&
            temporalPoc_ != static_cast<uint32_t>(temporalGopSize_ - 1)) {
            flag = AVCODEC_BUFFER_FLAG_DISPOSABLE_EXT;
        } else {
            flag = AVCODEC_BUFFER_FLAG_DISPOSABLE;
        }
    }
    frameFlagMap_.emplace(inputFrameCounter_, flag);
}

void TemporalScalability::ConfigureLTR(uint32_t index)
{
    std::lock_guard<std::shared_mutex> inputBufLock(inputBufMutex_);
    if (inputBufferMap_.find(index) != inputBufferMap_.end()) {
        bool syncIDR;
        if (inputBufferMap_[index]->meta_->GetData(Tag::VIDEO_REQUEST_I_FRAME, syncIDR) && syncIDR) {
            frameNum_ = 0;
            AVCODEC_LOGI("Request IDR frame.");
        }
        LTRDecision();
        DisposableDecision();
        inputBufferMap_[index]->meta_->SetData(Tag::VIDEO_ENCODER_PER_FRAME_MARK_LTR, isMarkLTR_);
        inputBufferMap_[index]->meta_->SetData(Tag::VIDEO_ENCODER_PER_FRAME_USE_LTR, isUseLTR_);
        inputBufferMap_[index]->meta_->SetData(Tag::VIDEO_PER_FRAME_POC, ltrPoc_);
        inputBufferMap_.erase(index);
        inputIndexQueue_->Pop();
        AVCODEC_LOGD("frame: %{public}d set ltrParam, isMarkLTR: %{public}d, isUseLTR: %{public}d, ltrPoc: %{public}d",
                     frameNum_, isMarkLTR_, isUseLTR_, ltrPoc_);
        frameNum_++;
        inputFrameCounter_++;
    } else {
        AVCODEC_LOGE("Find matched buffer failed, buffer ID is %{public}u.", index);
    }
}
} // namespace MediaAVCodec
} // namespace OHOS