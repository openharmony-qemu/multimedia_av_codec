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

#include "audio_g711mu_encoder_plugin.h"
#include "media_description.h"
#include "avcodec_errors.h"
#include "avcodec_trace.h"
#include "avcodec_log.h"
#include "avcodec_mime_type.h"
#include "avcodec_audio_common.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_AUDIO, "AvCodec-AudioG711muEncoderPlugin"};
constexpr std::string_view AUDIO_CODEC_NAME = "g711mu-encode";
constexpr int32_t INITVAL = -1;
constexpr int32_t SUPPORT_CHANNELS = 1;
constexpr int SUPPORT_SAMPLE_RATE = 8000;
constexpr int INPUT_BUFFER_SIZE_DEFAULT = 1280; // 20ms:320
constexpr int OUTPUT_BUFFER_SIZE_DEFAULT = 640; // 20ms:160

constexpr int AVCODEC_G711MU_LINEAR_BIAS = 0x84;
constexpr int AVCODEC_G711MU_CLIP = 8159;
constexpr uint16_t AVCODEC_G711MU_SEG_NUM = 8;
static const short AVCODEC_G711MU_SEG_END[8] = {
    0x3F, 0x7F, 0xFF, 0x1FF, 0x3FF, 0x7FF, 0xFFF, 0x1FFF
};
} // namespace

namespace OHOS {
namespace MediaAVCodec {
AudioG711muEncoderPlugin::AudioG711muEncoderPlugin() : channels_(-1), sampleFmt_(-1), sampleRate_(-1) {}

AudioG711muEncoderPlugin::~AudioG711muEncoderPlugin()
{
    Release();
    Reset();
}

static bool CheckSampleRate(int32_t sampleRate)
{
    if (sampleRate == SUPPORT_SAMPLE_RATE) {
        return true;
    } else {
        return false;
    }
}

int32_t AudioG711muEncoderPlugin::CheckSampleFormat()
{
    if (channels_ != SUPPORT_CHANNELS) {
        AVCODEC_LOGE("AudioG711muEncoderPlugin channels not supported");
        return AVCodecServiceErrCode::AVCS_ERR_CONFIGURE_MISMATCH_CHANNEL_COUNT;
    }

    if (!CheckSampleRate(sampleRate_)) {
        AVCODEC_LOGE("AudioG711muEncoderPlugin sampleRate not supported");
        return AVCodecServiceErrCode::AVCS_ERR_MISMATCH_SAMPLE_RATE;
    }

    if (sampleFmt_ != OHOS::MediaAVCodec::AudioSampleFormat::SAMPLE_S16LE) {
        AVCODEC_LOGE("AudioG711muEncoderPlugin sampleFmt not supported");
        return AVCodecServiceErrCode::AVCS_ERR_CONFIGURE_ERROR;
    }
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

int32_t AudioG711muEncoderPlugin::CheckFormat(const Format &format)
{
    if (!format.ContainKey(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT) ||
        !format.ContainKey(MediaDescriptionKey::MD_KEY_SAMPLE_RATE) ||
        !format.ContainKey(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT)) {
        AVCODEC_LOGE("Format parameter missing");
        return AVCS_ERR_UNSUPPORT_AUD_PARAMS;
    }

    channels_ = INITVAL;
    sampleRate_ = INITVAL;
    sampleFmt_ = INITVAL;

    format.GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, channels_);
    format.GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, sampleRate_);
    format.GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, sampleFmt_);

    int32_t ret = CheckSampleFormat();
    if (ret != AVCodecServiceErrCode::AVCS_ERR_OK) {
        AVCODEC_LOGE("AudioG711muEncoderPlugin parameter not supported");
        return ret;
    }
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

int32_t AudioG711muEncoderPlugin::Init(const Format &format)
{
    int32_t ret = CheckFormat(format);
    if (ret != AVCodecServiceErrCode::AVCS_ERR_OK) {
        AVCODEC_LOGE("Format check failed.");
        return AVCodecServiceErrCode::AVCS_ERR_UNSUPPORT_AUD_PARAMS;
    }
    encodeResult_.reserve(OUTPUT_BUFFER_SIZE_DEFAULT);

    format_ = format;
    format_.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_G711MU);
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

uint8_t AudioG711muEncoderPlugin::G711MuLawEncode(int16_t pcmValue)
{
    uint16_t mask;
    uint16_t seg;

    if (pcmValue < 0) {
        pcmValue = -pcmValue;
        mask = 0x7F;
    } else {
        mask = 0xFF;
    }
    uint16_t pcmShort = static_cast<uint16_t>(pcmValue);
    pcmShort = pcmShort >> 2; // right shift 2 bits
    if (pcmShort > AVCODEC_G711MU_CLIP) {
        pcmShort = AVCODEC_G711MU_CLIP;
    }
    pcmShort += (AVCODEC_G711MU_LINEAR_BIAS >> 2); // right shift 2 bits

    for (int16_t i = 0; i < AVCODEC_G711MU_SEG_NUM; i++) {
        if (pcmShort <= AVCODEC_G711MU_SEG_END[i]) {
            seg = i;
            break;
        }
    }
    if (pcmShort > AVCODEC_G711MU_SEG_END[7]) { // last index 7
        seg = 8;                                 // last segment index 8
    }

    if (seg >= 8) {                             // last segment index 8
        return (uint8_t)(0x7F ^ mask);
    } else {
        uint8_t muLawValue = (uint8_t)(seg << 4) | ((pcmShort >> (seg + 1)) & 0xF); // left shift 4 bits
        return (muLawValue ^ mask);
    }
}

int32_t AudioG711muEncoderPlugin::ProcessSendData(const std::shared_ptr<AudioBufferInfo> &inputBuffer)
{
    if (!inputBuffer) {
        AVCODEC_LOGE("AudioG711muEncoderPlugin inputBuffer is nullptr");
        return AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL;
    }
    {
        std::lock_guard<std::mutex> lock(avMutext_);
        auto attr = inputBuffer->GetBufferAttr();
        if (attr.size < 0) {
            AVCODEC_LOGE("AudioG711muEncoderPlugin InputBuffer size invalid, size: %{public}d", attr.size);
            return AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL;
        }
        auto memory = inputBuffer->GetBuffer();
        if (attr.size > memory->GetSize()) {
                AVCODEC_LOGE("AudioG711muEncoderPlugin InputBuffer too big, size:%{public}d, allocat size:%{public}d",
                    attr.size, memory->GetSize());
            return AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL;
        }
        int32_t sampleNum = attr.size / sizeof(int16_t);
        int32_t tmp = attr.size % sizeof(int16_t);
        int16_t *pcmToEncode = reinterpret_cast<int16_t *>(memory->GetBase());
        encodeResult_.clear();
        int32_t i;
        for (i = 0; i < sampleNum; ++i) {
            encodeResult_.push_back(G711MuLawEncode(pcmToEncode[i]));
        }
        if (tmp == 1 && i == sampleNum) {
            AVCODEC_LOGE("AudioG711muEncoderPlugin inputBuffer size in bytes is odd and the last byte is ignored");
            return AVCodecServiceErrCode::AVCS_ERR_INVALID_DATA;
        }
        return AVCodecServiceErrCode::AVCS_ERR_OK;
    }
}

int32_t AudioG711muEncoderPlugin::ProcessRecieveData(std::shared_ptr<AudioBufferInfo> &outBuffer)
{
    if (!outBuffer) {
        AVCODEC_LOGE("AudioG711muEncoderPlugin outBuffer is nullptr");
        return AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL;
    }
    {
        std::lock_guard<std::mutex> lock(avMutext_);
        auto memory = outBuffer->GetBuffer();
        if (memory->GetSize() < OUTPUT_BUFFER_SIZE_DEFAULT) {
            AVCODEC_LOGE("AudioG711muEncoderPlugin Output buffer size not enough, buffer size: %{public}d",
                memory->GetSize());
            return AVCodecServiceErrCode::AVCS_ERR_NO_MEMORY;
        }
        memory->Write(reinterpret_cast<const uint8_t *>(encodeResult_.data()),
                      (sizeof(uint8_t) * encodeResult_.size()));
        auto attr = outBuffer->GetBufferAttr();
        auto outSize = sizeof(uint8_t) * encodeResult_.size();
        attr.size = reinterpret_cast<size_t>(outSize);
        outBuffer->SetBufferAttr(attr);
    }
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

int32_t AudioG711muEncoderPlugin::Reset()
{
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

int32_t AudioG711muEncoderPlugin::Release()
{
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

int32_t AudioG711muEncoderPlugin::Flush()
{
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

int32_t AudioG711muEncoderPlugin::GetInputBufferSize() const
{
    return INPUT_BUFFER_SIZE_DEFAULT;
}

int32_t AudioG711muEncoderPlugin::GetOutputBufferSize() const
{
    return OUTPUT_BUFFER_SIZE_DEFAULT;
}

Format AudioG711muEncoderPlugin::GetFormat() const noexcept
{
    return format_;
}

std::string_view AudioG711muEncoderPlugin::GetCodecType() const noexcept
{
    return AUDIO_CODEC_NAME;
}
} // namespace MediaAVCodec
} // namespace OHOS