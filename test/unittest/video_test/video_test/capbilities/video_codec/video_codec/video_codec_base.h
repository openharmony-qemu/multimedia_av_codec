/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef AVCODEC_SAMPLE_VIDEO_CODEC_BASE_H
#define AVCODEC_SAMPLE_VIDEO_CODEC_BASE_H

#include "sample_info.h"

namespace OHOS {
namespace MediaAVCodec {
namespace Sample {
class VideoCodecBase {
public:
    virtual ~VideoCodecBase() {};
    virtual int32_t Create(const std::string &codecMime, bool isSoftware = false) = 0;
    virtual int32_t Config(SampleInfo &sampleInfo, uintptr_t * const sampleContext) = 0;
    virtual int32_t Start() = 0;
    virtual int32_t Flush() = 0;
    virtual int32_t Stop() = 0;
    virtual int32_t Reset() = 0;
    virtual OH_AVFormat *GetFormat() = 0;
    virtual int32_t PushInput(CodecBufferInfo &info) = 0;
    virtual int32_t FreeOutput(uint32_t bufferIndex) = 0;

protected:
    virtual int32_t SetCallback(uintptr_t * const sampleContext) = 0;
    virtual int32_t Configure(const SampleInfo &sampleInfo) = 0;
    static std::string GetCodecName(const std::string &codecMime, bool isEncoder, bool isSoftware);

    std::shared_ptr<OH_AVCodec> codec_ = nullptr;
};

class VideoCodecFactory {
public:
    static std::shared_ptr<VideoCodecBase> CreateVideoCodec(CodecType type, CodecRunMode mode);

private:
    static std::shared_ptr<VideoCodecBase> CreateVideoDecoder(CodecRunMode mode);
    static std::shared_ptr<VideoCodecBase> CreateVideoEncoder(CodecRunMode mode);
};
} // Sample
} // MediaAVCodec
} // OHOS
#endif // AVCODEC_SAMPLE_VIDEO_CODEC_BASE_H