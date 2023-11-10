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

#ifndef AUDIO_G711MU_DECODER_PLUGIN_H
#define AUDIO_G711MU_DECODER_PLUGIN_H

#include <mutex>
#include "audio_base_codec.h"
#include "avcodec_codec_name.h"

namespace OHOS {
namespace MediaAVCodec {
class AudioG711muDecoderPlugin : public AudioBaseCodec::CodecRegister<AudioG711muDecoderPlugin> {
public:
    AudioG711muDecoderPlugin();
    ~AudioG711muDecoderPlugin() override;

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
        return std::string(AVCodecCodecName::AUDIO_DECODER_G711MU_NAME);
    }

private:
    int32_t CheckInit(const Format &format);
    int16_t G711MuLawDecode(uint8_t muLawValue);
    Format format_;
    std::mutex avMutext_;  // mutable
    std::vector<int16_t> decodeResult_;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif