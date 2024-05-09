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

#ifndef AV_CODEC_AUDIO_RESAMPLE_H
#define AV_CODEC_AUDIO_RESAMPLE_H

#include <vector>
#include <memory>
#include "avcodec_errors.h"
#ifdef __cplusplus
extern "C" {
#endif
#include "libswresample/swresample.h"
#ifdef __cplusplus
};
#endif

namespace OHOS {
namespace MediaAVCodec {
struct ResamplePara {
    uint32_t channels {2}; // 2: STEREO
    int32_t sampleRate {0};
    int32_t bitsPerSample {0};
    AVChannelLayout channelLayout;
    AVSampleFormat srcFmt {AV_SAMPLE_FMT_NONE};
    int32_t destSamplesPerFrame {0};
    AVSampleFormat destFmt {AV_SAMPLE_FMT_S16};
};

class AudioResample {
public:
    int32_t Init(const ResamplePara& resamplePara);
    int32_t Convert(const uint8_t* srcBuffer, const size_t srcLength, uint8_t*& destBuffer, size_t& destLength);
    int32_t InitSwrContext(const ResamplePara& resamplePara);
    int32_t ConvertFrame(AVFrame *outputFrame, const AVFrame *inputFrame);

private:
    ResamplePara resamplePara_ {};
    std::vector<uint8_t> resampleCache_ {};
    std::vector<uint8_t*> resampleChannelAddr_ {};
    std::shared_ptr<SwrContext> swrCtx_ {nullptr};
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif
