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

#ifndef AUDIO_FFMPEG_ENCODER_PLUGIN
#define AUDIO_FFMPEG_ENCODER_PLUGIN

#include <mutex>
#include <fstream>
#include "audio_base_codec.h"
#include "nocopyable.h"
#ifdef __cplusplus
extern "C" {
#endif
#include <libavutil/opt.h>
#include "libavcodec/avcodec.h"
#ifdef __cplusplus
};
#endif

namespace OHOS {
namespace MediaAVCodec {
class AudioFfmpegEncoderPlugin : NoCopyable {
public:
    AudioFfmpegEncoderPlugin();
    ~AudioFfmpegEncoderPlugin();
    int32_t ProcessSendData(const std::shared_ptr<AudioBufferInfo> &inputBuffer);
    int32_t ProcessRecieveData(std::shared_ptr<AudioBufferInfo> &outBuffer);
    int32_t Reset();
    int32_t Release();
    int32_t Flush();

    int32_t AllocateContext(const std::string &name);
    int32_t InitContext(const Format &format);
    int32_t OpenContext();
    int32_t InitFrame();

    std::shared_ptr<AVCodecContext> GetCodecContext() const;
    int32_t CloseCtxLocked();
    int32_t GetMaxInputSize() const noexcept;

private:
    int32_t maxInputSize_;
    std::shared_ptr<AVCodec> avCodec_;
    std::shared_ptr<AVCodecContext> avCodecContext_;
    std::shared_ptr<AVFrame> cachedFrame_;
    std::shared_ptr<AVPacket> avPacket_;
    mutable std::mutex avMutext_;
    std::mutex parameterMutex_;
    int64_t prevPts_;

private:
    int32_t SendBuffer(const std::shared_ptr<AudioBufferInfo> &inputBuffer);
    int32_t ReceiveBuffer(std::shared_ptr<AudioBufferInfo> &outBuffer);
    int32_t ReceivePacketSucc(std::shared_ptr<AudioBufferInfo> &outBuffer);
    int32_t PcmFillFrame(const std::shared_ptr<AudioBufferInfo> &inputBuffer);
    int32_t ReAllocateContext();
    bool codecContextValid_;
    uint32_t channelsBytesPerSample_ {1};
};
} // namespace MediaAVCodec
} // namespace OHOS

#endif