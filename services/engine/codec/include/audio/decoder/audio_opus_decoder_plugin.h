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

#ifndef AUDIO_FFMPEG_OPUS_DECODER_PLUGIN_H
#define AUDIO_FFMPEG_OPUS_DECODER_PLUGIN_H

#include <mutex>
#include "audio_base_codec.h"
#include "avcodec_codec_name.h"
#include "audio_base_codec.h"
#include "nocopyable.h"
#include "audio_base_codec_ext.h"

namespace OHOS {
namespace MediaAVCodec {
enum ParaType : int32_t {
    CHANNEL = 0,
    SAMPLE_RATE = 1,
};

class AudioOpusDecoderPlugin : public AudioBaseCodec::CodecRegister<AudioOpusDecoderPlugin> {
public:
    AudioOpusDecoderPlugin();
    ~AudioOpusDecoderPlugin() override;

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
        return std::string(AVCodecCodecName::AUDIO_DECODER_OPUS_NAME);
    }

private:
    Format format_;
    mutable std::mutex avMutext_;
    int32_t CheckSampleFormat();
    OHOS::MediaAVCodec::AudioBaseCodecExt *PluginCodecPtr;
    int32_t ret;
    unsigned char *fbytes;
    int32_t len;
    unsigned char *codeData;

    int32_t channels;
    int32_t sampleRate;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif