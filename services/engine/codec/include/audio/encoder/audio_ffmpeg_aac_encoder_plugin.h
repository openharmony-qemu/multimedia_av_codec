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

#ifndef AUDIO_FFMPEG_AAC_ENCODER_PLUGIN_H
#define AUDIO_FFMPEG_AAC_ENCODER_PLUGIN_H

#include <mutex>
#include "audio_base_codec.h"
#include "avcodec_codec_name.h"
#include "audio_base_codec.h"
#include "nocopyable.h"
#include "audio_resample.h"
#ifdef __cplusplus
extern "C" {
#endif
#include "libavcodec/avcodec.h"
#ifdef __cplusplus
};
#endif

namespace OHOS {
namespace MediaAVCodec {
class AudioFFMpegAacEncoderPlugin : public AudioBaseCodec::CodecRegister<AudioFFMpegAacEncoderPlugin> {
public:
    AudioFFMpegAacEncoderPlugin();
    ~AudioFFMpegAacEncoderPlugin() override;

    int32_t Init(const Format &format) override;
    int32_t ProcessSendData(const std::shared_ptr<AudioBufferInfo> &inputBuffer) override;
    int32_t ProcessRecieveData(std::shared_ptr<AudioBufferInfo> &outBuffer) override;
    int32_t Reset() override;
    int32_t Release() override;
    int32_t Flush() override;
    int32_t GetInputBufferSize() const override;
    int32_t GetOutputBufferSize() const override;
    Format GetFormat() const noexcept override;
    std::string_view GetCodecType() const noexcept override;

    const static std::string Identify()
    {
        return std::string(AVCodecCodecName::AUDIO_ENCODER_AAC_NAME);
    }

private:
    Format format_;
    int32_t maxInputSize_;
    std::shared_ptr<AVCodec> avCodec_;
    std::shared_ptr<AVCodecContext> avCodecContext_;
    std::shared_ptr<AVFrame> cachedFrame_;
    std::shared_ptr<AVPacket> avPacket_;
    mutable std::mutex avMutext_;
    int64_t prevPts_;
    std::shared_ptr<AudioResample> resample_;
    bool needResample_;
    AVSampleFormat srcFmt_;
    int64_t srcLayout_;
    bool codecContextValid_;

private:
    bool CheckFormat(const Format &format);
    bool CheckBitRate(const Format &format) const;
    void SetFormat(const Format &format) noexcept;
    int32_t AllocateContext(const std::string &name);
    int32_t InitContext(const Format &format);
    int32_t OpenContext();
    int32_t InitFrame();
    int32_t SendBuffer(const std::shared_ptr<AudioBufferInfo> &inputBuffer);
    int32_t ReceiveBuffer(std::shared_ptr<AudioBufferInfo> &outBuffer);
    int32_t ReceivePacketSucc(std::shared_ptr<AudioBufferInfo> &outBuffer);
    int32_t PcmFillFrame(const std::shared_ptr<AudioBufferInfo> &inputBuffer);
    int32_t CloseCtxLocked();
    int32_t ReAllocateContext();
    bool CheckResample() const;
    bool CheckSampleRate(const int sampleRate);
    bool CheckSampleFormat(const Format &format);
    bool CheckChannelLayout(const Format &format, int channels);
    int32_t GetAdtsHeader(std::string &adtsHeader, int32_t &headerSize, std::shared_ptr<AVCodecContext> ctx,
                          int aacLength);
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif