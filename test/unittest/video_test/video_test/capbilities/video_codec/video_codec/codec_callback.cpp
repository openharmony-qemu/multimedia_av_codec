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

#include "codec_callback.h"
#include "av_codec_sample_log.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_TEST, "SampleCallback"};
}

namespace OHOS {
namespace MediaAVCodec {
namespace Sample {
void CodecCallback::OnCodecError(OH_AVCodec *codec, int32_t errorCode, void *userData)
{
    (void)codec;
    (void)errorCode;
    (void)userData;
    AVCODEC_LOGE("On codec error, error code: %{public}d", errorCode);
}

void CodecCallback::OnCodecFormatChange(OH_AVCodec *codec, OH_AVFormat *format, void *userData)
{
    if (userData == nullptr) {
        return;
    }
    (void)codec;
    CodecUserData *codecUserData = static_cast<CodecUserData *>(userData);
    std::unique_lock<std::mutex> lock(codecUserData->outputMutex_);
    auto originVideoWidth = codecUserData->sampleInfo->videoWidth;
    auto originVideoHeight = codecUserData->sampleInfo->videoHeight;
    OH_AVFormat_GetIntValue(format, OH_MD_KEY_WIDTH, &codecUserData->sampleInfo->videoWidth);
    OH_AVFormat_GetIntValue(format, OH_MD_KEY_HEIGHT, &codecUserData->sampleInfo->videoHeight);

    AVCODEC_LOGW("Resolution: %{public}d*%{public}d => %{public}d*%{public}d", originVideoWidth, originVideoHeight,
        codecUserData->sampleInfo->videoWidth, codecUserData->sampleInfo->videoHeight);
}

void CodecCallback::OnInputBufferAvailable(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, void *userData)
{
    if (userData == nullptr) {
        return;
    }
    (void)codec;
    CodecUserData *codecUserData = static_cast<CodecUserData *>(userData);
    std::unique_lock<std::mutex> lock(codecUserData->inputMutex_);
    codecUserData->inputBufferInfoQueue_.emplace(index, data);
    codecUserData->inputCond_.notify_all();
}

void CodecCallback::OnOutputBufferAvailable(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data,
                                            OH_AVCodecBufferAttr *attr, void *userData)
{
    if (userData == nullptr) {
        return;
    }
    (void)codec;
    CodecUserData *codecUserData = static_cast<CodecUserData *>(userData);
    std::unique_lock<std::mutex> lock(codecUserData->outputMutex_);
    codecUserData->outputBufferInfoQueue_.emplace(index, data, *attr);
    codecUserData->outputCond_.notify_all();
}

void CodecCallback::OnNeedInputBuffer(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData)
{
    if (userData == nullptr) {
        return;
    }
    (void)codec;
    CodecUserData *codecUserData = static_cast<CodecUserData *>(userData);
    std::unique_lock<std::mutex> lock(codecUserData->inputMutex_);
    codecUserData->inputBufferInfoQueue_.emplace(index, buffer);
    codecUserData->inputCond_.notify_all();
}

void CodecCallback::OnNewOutputBuffer(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData)
{
    if (userData == nullptr) {
        return;
    }
    (void)codec;
    CodecUserData *codecUserData = static_cast<CodecUserData *>(userData);
    std::unique_lock<std::mutex> lock(codecUserData->outputMutex_);
    codecUserData->outputBufferInfoQueue_.emplace(index, buffer);
    codecUserData->outputCond_.notify_all();
}
} // Sample
} // MediaAVCodec
} // OHOS