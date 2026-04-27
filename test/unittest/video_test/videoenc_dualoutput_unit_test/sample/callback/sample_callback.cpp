/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "codec_buffer_info.h"
#include "meta/any.h"
#include "sample_callback.h"
#include "vcodec_signal.h"
#include "venc_dualoutput_sample.h"

namespace OHOS {
namespace MediaAVCodec {

void OnErrorVoid(OH_AVCodec *codec, int32_t errorCode, void *userData)
{
    (void)codec;
    (void)errorCode;
    (void)userData;
}

void OnStreamChangedVoid(OH_AVCodec *codec, OH_AVFormat *format, void *userData)
{
    (void)codec;
    (void)format;
    (void)userData;
}

void InBufferHandle(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData)
{
    (void)codec;
    auto signal = reinterpret_cast<VCodecSignal *>(userData);
    auto codecSample = signal->codecSample_.lock();
    codecSample->HandleInputFrame(std::make_shared<CodecBufferInfo>(index, Any(buffer)));
}

void OutBufferHandle(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData)
{
    (void)codec;
    auto signal = reinterpret_cast<VCodecSignal *>(userData);
    auto codecSample = signal->codecSample_.lock();
    codecSample->HandleOutputFrame(std::make_shared<CodecBufferInfo>(index, Any(buffer)));
}

void OutBufferHandleSecondary(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData)
{
    (void)codec;
    auto signal = reinterpret_cast<VCodecSignal *>(userData);
    auto codecSample = signal->codecSample_.lock();
    auto dualSample = std::static_pointer_cast<VideoEncDualOutputSample>(codecSample);
    dualSample->HandleOutputFrameByEncoderType(EncoderType::SECONDARY,
        std::make_shared<CodecBufferInfo>(index, Any(buffer)));
}

void InBufferVoid(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData)
{
    (void)codec;
    (void)index;
    (void)buffer;
    (void)userData;
}

} // namespace MediaAVCodec
} // namespace OHOS
