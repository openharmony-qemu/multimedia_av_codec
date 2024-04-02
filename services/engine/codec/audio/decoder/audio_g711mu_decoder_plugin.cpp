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

#include "audio_g711mu_decoder_plugin.h"
#include "avcodec_errors.h"
#include "media_description.h"
#include "avcodec_trace.h"
#include "avcodec_log.h"
#include "avcodec_mime_type.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_AUDIO, "AvCodec-AudioG711muDecoderPlugin"};
constexpr int SUPPORT_CHANNELS = 1;
constexpr int SUPPORT_SAMPLE_RATE = 8000;
constexpr int INPUT_BUFFER_SIZE_DEFAULT = 640; // 20ms:160
constexpr int OUTPUT_BUFFER_SIZE_DEFAULT = 1280; // 20ms:320
constexpr int AUDIO_G711MU_SIGN_BIT = 0x80;
constexpr int AVCODEC_G711MU_QUANT_MASK = 0xf;
constexpr int AVCODEC_G711MU_SHIFT = 4;
constexpr int AVCODEC_G711MU_SEG_MASK = 0x70;
constexpr int G711MU_LINEAR_BIAS = 0x84;
} // namespace

namespace OHOS {
namespace MediaAVCodec {
constexpr std::string_view AUDIO_CODEC_NAME = "g711mu-decode";
AudioG711muDecoderPlugin::AudioG711muDecoderPlugin()
{
}

AudioG711muDecoderPlugin::~AudioG711muDecoderPlugin()
{
    Release();
    Reset();
}

int32_t AudioG711muDecoderPlugin::Init(const Format &format)
{
    int32_t checkresult = CheckInit(format);
    if (checkresult != AVCodecServiceErrCode::AVCS_ERR_OK) {
        return checkresult;
    }
    decodeResult_.reserve(OUTPUT_BUFFER_SIZE_DEFAULT);

    format_ = format;
    format_.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_G711MU);
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

int16_t AudioG711muDecoderPlugin::G711MuLawDecode(uint8_t muLawValue)
{
    uint16_t tmp;
    muLawValue = ~muLawValue;

    tmp = ((muLawValue & AVCODEC_G711MU_QUANT_MASK) << 3) + G711MU_LINEAR_BIAS;  // left shift 3 bits
    tmp <<= ((unsigned)muLawValue & AVCODEC_G711MU_SEG_MASK) >> AVCODEC_G711MU_SHIFT;

    return ((muLawValue & AUDIO_G711MU_SIGN_BIT) ? (G711MU_LINEAR_BIAS - tmp) : (tmp - G711MU_LINEAR_BIAS));
}

int32_t AudioG711muDecoderPlugin::ProcessSendData(const std::shared_ptr<AudioBufferInfo> &inputBuffer)
{
    if (!inputBuffer) {
        AVCODEC_LOGE("AudioG711muDecoder inputBuffer is null.");
        return AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL;
    }
    auto attr = inputBuffer->GetBufferAttr();
    if (attr.size < 0) {
        AVCODEC_LOGE("AudioG711muDecoder inputBuffer size invalid, size: %{public}d", attr.size);
        return AVCodecServiceErrCode::AVCS_ERR_NO_MEMORY;
    }
    auto memory = inputBuffer->GetBuffer();
    if (attr.size > memory->GetSize()) {
        AVCODEC_LOGE("AudioG711muDecoder inputBuffer too big, size:%{public}d, allocat size:%{public}d",
            attr.size, memory->GetSize());
        return AVCodecServiceErrCode::AVCS_ERR_NO_MEMORY;
    }
    {
        std::lock_guard<std::mutex> lock(avMutext_);
        int32_t decodeNum = attr.size / sizeof(uint8_t);
        uint8_t *muValueToDecode = reinterpret_cast<uint8_t *>(memory->GetBase());
        decodeResult_.clear();
        for (int32_t i = 0; i < decodeNum ; ++i) {
            decodeResult_.push_back(G711MuLawDecode(muValueToDecode[i]));
        }
    }
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}


int32_t AudioG711muDecoderPlugin::ProcessRecieveData(std::shared_ptr<AudioBufferInfo> &outBuffer)
{
    if (!outBuffer) {
        AVCODEC_LOGE("AudioG711muDecoder outBuffer is null.");
        return AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL;
    }
    {
        std::lock_guard<std::mutex> lock(avMutext_);
        auto memory = outBuffer->GetBuffer();
        if (memory->GetSize() < OUTPUT_BUFFER_SIZE_DEFAULT) {
            AVCODEC_LOGE("AudioG711muDecoder outBuffer size not enough, buffer size: %{public}d", memory->GetSize());
            return AVCodecServiceErrCode::AVCS_ERR_NO_MEMORY;
        }
        memory->Write(reinterpret_cast<const uint8_t *>(decodeResult_.data()),
            (sizeof(int16_t) * decodeResult_.size()));
        auto attr = outBuffer->GetBufferAttr();
        auto outSize = sizeof(int16_t) * decodeResult_.size();
        attr.size = static_cast<size_t>(outSize);
        outBuffer->SetBufferAttr(attr);
    }
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

int32_t AudioG711muDecoderPlugin::Reset()
{
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

int32_t AudioG711muDecoderPlugin::Release()
{
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

int32_t AudioG711muDecoderPlugin::Flush()
{
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

int32_t AudioG711muDecoderPlugin::GetInputBufferSize() const
{
    return INPUT_BUFFER_SIZE_DEFAULT;
}

int32_t AudioG711muDecoderPlugin::GetOutputBufferSize() const
{
    return OUTPUT_BUFFER_SIZE_DEFAULT;
}

Format AudioG711muDecoderPlugin::GetFormat() const noexcept
{
    return format_;
}

int32_t AudioG711muDecoderPlugin::CheckInit(const Format &format)
{
    int channels = 0;
    int sampleRate = 0;
    format.GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, channels);
    format.GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, sampleRate);

    if (channels != SUPPORT_CHANNELS) {
        return AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL;
    }
    if (sampleRate != SUPPORT_SAMPLE_RATE) {
        return AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL;
    }
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

std::string_view AudioG711muDecoderPlugin::GetCodecType() const noexcept
{
    return AUDIO_CODEC_NAME;
}
} // namespace MediaAVCodec
} // namespace OHOS