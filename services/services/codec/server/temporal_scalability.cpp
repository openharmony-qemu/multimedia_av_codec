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
#include "codec_ability_singleton.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "TemporalScalability"};
} // namespace

constexpr int32_t DEFAULT_GOPSIZE = 60; // DEFAULT_FRAMERATE * DEFAULT_I_FRAME_INTERVAL / 1000;
constexpr int32_t DEFAULT_TEMPORAL_GOPSIZE = 4;
constexpr int32_t DEFAULT_VIDEO_LTR_FRAME_NUM = 2;
constexpr int32_t ENABLE_PARAMETER_CALLBACK = 1;

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

void TemporalScalability::ValidateTemporalGopParam(Format &format)
{
    if (!format.GetIntValue("video_encoder_gop_size", gopSize_)) {
        gopSize_ = DEFAULT_GOPSIZE;
    }

    if (format.GetIntValue(Tag::VIDEO_ENCODER_TEMPORAL_GOP_SIZE, temporalGopSize_)) {
        AVCODEC_LOGI("Set temporal gop size successfully, value is %{public}d.", temporalGopSize_);
    } else {
        temporalGopSize_ = gopSize_ <= DEFAULT_TEMPORAL_GOPSIZE ? MIN_TEMPORAL_GOPSIZE : DEFAULT_TEMPORAL_GOPSIZE;
        AVCODEC_LOGI("Get temporal gop size failed, use default value %{public}d.", temporalGopSize_);
    }
    if (format.GetIntValue(Tag::VIDEO_ENCODER_TEMPORAL_GOP_REFERENCE_MODE, tRefMode_)) {
        AVCODEC_LOGI("Set temporal reference mode successfully.");
    } else {
        tRefMode_ = static_cast<int32_t>(TemporalGopReferenceMode::ADJACENT_REFERENCE);
        AVCODEC_LOGI("Get temporal reference mode failed, use default value ADJACENT_REFERENCE.");
    }

    format.PutIntValue(Tag::VIDEO_ENCODER_LTR_FRAME_COUNT, DEFAULT_VIDEO_LTR_FRAME_NUM);
    format.PutIntValue(Tag::VIDEO_ENCODER_ENABLE_SURFACE_INPUT_CALLBACK, ENABLE_PARAMETER_CALLBACK);
    AVCODEC_LOGI("Set temporal gop parameter successfully.");
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
    buffer->flag_ |= flag;
    frameFlagMap_.erase(outputFrameCounter_);
    outputFrameCounter_++;
}

void TemporalScalability::LTRDecision()
{
    poc_ = frameNum_ % gopSize_;
    temporalPoc_ = poc_ % temporalGopSize_;

    if (temporalPoc_ == 0) {
        isMarkLTR_ = 1;
        if (poc_ == 0) {
            isUseLTR_ = false;
            ltrPoc_ = 0;
        } else {
            isUseLTR_ = true;
            ltrPoc_ = poc_ - temporalGopSize_;
        }
    } else if (temporalPoc_ == 1) {
        isMarkLTR_ = 0;
        isUseLTR_ = false;
        ltrPoc_ = poc_ - 1;
    } else {
        isMarkLTR_ = 0;
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
    if (isMarkLTR_ == 0) {
        if (tRefMode_ == static_cast<int32_t>(TemporalGopReferenceMode::ADJACENT_REFERENCE) &&
            temporalPoc_ != temporalGopSize_ - 1 && poc_ != gopSize_ - 1) {
            flag = AVCODEC_BUFFER_FLAG_DISPOSABLE_EXT;
        } else {
            flag = AVCODEC_BUFFER_FLAG_DISPOSABLE;
        }
    }
    frameFlagMap_.emplace(inputFrameCounter_, flag);
    inputFrameCounter_++;
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
        if (isUseLTR_) {
            inputBufferMap_[index]->meta_->SetData(Tag::VIDEO_ENCODER_PER_FRAME_USE_LTR, ltrPoc_);
        } else {
            inputBufferMap_[index]->meta_->Remove(Tag::VIDEO_ENCODER_PER_FRAME_USE_LTR);
        }
        inputBufferMap_.erase(index);
        inputIndexQueue_->Pop();
        AVCODEC_LOGD("frame: %{public}d set ltrParam, isMarkLTR: %{public}d, isUseLTR: %{public}d, ltrPoc: %{public}d",
                     frameNum_, isMarkLTR_, isUseLTR_, ltrPoc_);
        frameNum_++;
    } else {
        AVCODEC_LOGE("Find matched buffer failed, buffer ID is %{public}u.", index);
    }
}
} // namespace MediaAVCodec
} // namespace OHOS