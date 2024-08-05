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

#ifndef AVCODEC_SAMPLE_VIDEO_ENCODER_H
#define AVCODEC_SAMPLE_VIDEO_ENCODER_H

#include "native_avcodec_videoencoder.h"
#include "video_codec_base.h"

namespace OHOS {
namespace MediaAVCodec {
namespace Sample {
class VideoEncoder : public VideoCodecBase {
public:
    int32_t Create(const std::string &codecMime, bool isSoftware = false) override;
    int32_t Config(SampleInfo &sampleInfo, uintptr_t * const sampleContext) override;
    int32_t Start() override;
    int32_t Flush() override;
    int32_t Stop() override;
    int32_t Reset() override;
    OH_AVFormat *GetFormat() override;

private:
    int32_t NotifyEndOfStream();
    int32_t Configure(const SampleInfo &sampleInfo) override;
    int32_t GetSurface(SampleInfo &sampleInfo);
};

/********************* API10 *********************/
class VideoEncoderAPI10 : public VideoEncoder {
public:
    int32_t FreeOutput(uint32_t bufferIndex) override;

protected:
    int32_t SetCallback(uintptr_t * const sampleContext) override;
};

class VideoEncoderAPI10Buffer : public VideoEncoderAPI10 {
public:
    int32_t PushInput(CodecBufferInfo &info) override;
};

class VideoEncoderAPI10Surface : public VideoEncoderAPI10 {
public:
    int32_t PushInput(CodecBufferInfo &info) override;
};

/********************* API11 *********************/
class VideoEncoderAPI11 : public VideoEncoder {
public:
    int32_t FreeOutput(uint32_t bufferIndex) override;

protected:
    int32_t SetCallback(uintptr_t * const sampleContext) override;
};

class VideoEncoderAPI11Buffer : public VideoEncoderAPI11 {
public:
    int32_t PushInput(CodecBufferInfo &info) override;
};

class VideoEncoderAPI11Surface : public VideoEncoderAPI11 {
public:
    int32_t PushInput(CodecBufferInfo &info) override;
};
} // Sample
} // MediaAVCodec
} // OHOS
#endif // AVCODEC_SAMPLE_VIDEO_ENCODER_H