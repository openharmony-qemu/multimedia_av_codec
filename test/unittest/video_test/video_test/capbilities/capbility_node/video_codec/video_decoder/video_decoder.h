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

#ifndef AVCODEC_SAMPLE_VIDEO_DECODER_H
#define AVCODEC_SAMPLE_VIDEO_DECODER_H

#include "native_avcodec_videodecoder.h"
#include "video_codec_base.h"

namespace OHOS {
namespace MediaAVCodec {
namespace Sample {
class VideoDecoder : public VideoCodecBase {
public:
    int32_t Create(const std::string &codecMime, bool isSoftware = false) override;
    int32_t DealWithSurface(std::shared_ptr<WindowWrapper> &windowWrapper) override;
    int32_t Configure(const SampleInfo &sampleInfo) override;
    int32_t Prepare() override;
    int32_t Start() override;
    int32_t Flush() override;
    int32_t Stop() override;
    int32_t Reset() override;
    std::shared_ptr<OH_AVFormat> GetFormat() override;
    int32_t QueryInput(uint32_t &index, int64_t timeoutUs) override;
    int32_t QueryOutput(uint32_t &index, int64_t timeoutUs) override;
    std::optional<CodecBufferInfo> GetInput(uint32_t index) override;
    std::optional<CodecBufferInfo> GetOutput(uint32_t index) override;

protected:
};

/********************* API10 *********************/
class VideoDecoderAPI10 : public VideoDecoder {
public:
    int32_t PushInput(CodecBufferInfo &info) override;
    int32_t SetCallback(uintptr_t * const sampleContext) override;
};

class VideoDecoderAPI10Buffer : public VideoDecoderAPI10 {
public:
    int32_t FreeOutput(uint32_t bufferIndex) override;
};

class VideoDecoderAPI10Surface : public VideoDecoderAPI10 {
public:
    int32_t FreeOutput(uint32_t bufferIndex) override;
};

/********************* API11 *********************/
class VideoDecoderAPI11 : public VideoDecoder {
public:
    int32_t PushInput(CodecBufferInfo &info) override;
    int32_t SetCallback(uintptr_t * const sampleContext) override;
};

class VideoDecoderAPI11Buffer : public VideoDecoderAPI11 {
public:
    int32_t FreeOutput(uint32_t bufferIndex) override;
};

class VideoDecoderAPI11Surface : public VideoDecoderAPI11 {
public:
    int32_t FreeOutput(uint32_t bufferIndex) override;
};
} // Sample
} // MediaAVCodec
} // OHOS
#endif // AVCODEC_SAMPLE_VIDEO_DECODER_H