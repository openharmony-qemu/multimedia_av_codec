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

#ifndef AUDIO_G711MU_ENCODER_PLUGIN_H
#define AUDIO_G711MU_ENCODER_PLUGIN_H

#include <mutex>
#include "audio_base_codec.h"
#include "avcodec_codec_name.h"

namespace OHOS {
namespace MediaAVCodec {
class AudioG711muEncoderPlugin : public AudioBaseCodec::CodecRegister<AudioG711muEncoderPlugin> {
public:
    AudioG711muEncoderPlugin();
    ~AudioG711muEncoderPlugin() override;

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
        return std::string(AVCodecCodecName::AUDIO_ENCODER_G711MU_NAME);
    }

private:
    int32_t CheckFormat(const Format &format);
    int32_t CheckSampleFormat();
    uint8_t G711MuLawEncode(int16_t pcmValue);

    std::mutex avMutext_;  // mutable
    std::vector<int8_t> encodeResult_;
    int32_t channels_;
    Format format_;
    int32_t sampleFmt_;
    int32_t sampleRate_;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif