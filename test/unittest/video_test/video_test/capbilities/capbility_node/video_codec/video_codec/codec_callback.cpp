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
#include "sample_context.h"
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
    (void)userData;
    AVCODEC_LOGE("On codec error, error code: %{public}d", errorCode);
}

void CodecCallback::OnCodecFormatChange(OH_AVCodec *codec, OH_AVFormat *format, void *userData)
{
    CHECK_AND_RETURN_LOG(userData != nullptr, "User data is nullptr");
    (void)codec;
    SampleContext *context = static_cast<SampleContext *>(userData);
    auto originVideoWidth = context->sampleInfo->videoWidth;
    auto originVideoHeight = context->sampleInfo->videoHeight;
    OH_AVFormat_GetIntValue(format, OH_MD_KEY_VIDEO_PIC_WIDTH, &context->sampleInfo->videoWidth);
    OH_AVFormat_GetIntValue(format, OH_MD_KEY_VIDEO_PIC_HEIGHT, &context->sampleInfo->videoHeight);
    OH_AVFormat_GetIntValue(format, OH_MD_KEY_VIDEO_STRIDE, &context->sampleInfo->videoStrideWidth);
    OH_AVFormat_GetIntValue(format, OH_MD_KEY_VIDEO_SLICE_HEIGHT, &context->sampleInfo->videoSliceHeight);

    auto &videoSliceHeight = context->sampleInfo->videoSliceHeight;
    videoSliceHeight = videoSliceHeight == 0 ? context->sampleInfo->videoHeight : videoSliceHeight;

    AVCODEC_LOGW("Resolution: %{public}d*%{public}d => %{public}d*%{public}d(%{public}d*%{public}d)",
        originVideoWidth, originVideoHeight, context->sampleInfo->videoWidth, context->sampleInfo->videoHeight,
        context->sampleInfo->videoStrideWidth, context->sampleInfo->videoSliceHeight);
}

void CodecCallback::OnInputBufferAvailable(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, void *userData)
{
    CHECK_AND_RETURN_LOG(userData != nullptr, "User data is nullptr");
    (void)codec;
    SampleContext *context = static_cast<SampleContext *>(userData);
    CodecBufferInfo bufferInfo = CodecBufferInfo(index, data);
    context->inputBufferQueue.QueueBuffer(bufferInfo);
}

void CodecCallback::OnOutputBufferAvailable(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data,
                                            OH_AVCodecBufferAttr *attr, void *userData)
{
    CHECK_AND_RETURN_LOG(userData != nullptr, "User data is nullptr");
    (void)codec;
    SampleContext *context = static_cast<SampleContext *>(userData);
    CodecBufferInfo bufferInfo = CodecBufferInfo(index, data, *attr);
    context->outputBufferQueue.QueueBuffer(bufferInfo);
}

void CodecCallback::OnNeedInputBuffer(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData)
{
    CHECK_AND_RETURN_LOG(userData != nullptr, "User data is nullptr");
    (void)codec;
    SampleContext *context = static_cast<SampleContext *>(userData);
    CodecBufferInfo bufferInfo = CodecBufferInfo(index, buffer);
    context->inputBufferQueue.QueueBuffer(bufferInfo);
}

void CodecCallback::OnNewOutputBuffer(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData)
{
    CHECK_AND_RETURN_LOG(userData != nullptr, "User data is nullptr");
    (void)codec;
    SampleContext *context = static_cast<SampleContext *>(userData);
    CodecBufferInfo bufferInfo = CodecBufferInfo(index, buffer);
    context->outputBufferQueue.QueueBuffer(bufferInfo);
}
} // Sample
} // MediaAVCodec
} // OHOS