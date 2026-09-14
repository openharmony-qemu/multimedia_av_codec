/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include <cstdio>
#include <iostream>
#include "opus_plugin_base.h"
#include "common/log.h"

namespace OHOS {
namespace MediaAVCodec {
constexpr int32_t MAX_INPUT = 1500;
constexpr int32_t LSB_DEPTH = 16;
constexpr int32_t TIME_MS = 20;
constexpr int32_t TIME_MS_MAX = 120; // capability slove 120ms frame
constexpr int32_t TIME_S_MS = 1000;
constexpr int32_t PARAM_STREAMS = 4;
constexpr int32_t PARAM_COUPLED = 5;
constexpr int32_t PARAM_MAPPING = 6;
constexpr uint8_t BITS_8 = 8;
constexpr uint8_t BITS_16 = 16;
constexpr uint8_t BITS_24 = 24;

AudioOpusEncoderPluginBase::AudioOpusEncoderPluginBase()
    : channels_(0), ret_(0), frameSize_(0), samplingRate_(0), bitrate_(0), complianceLevel_(0), padding_(0)
{}

AudioOpusEncoderPluginBase::~AudioOpusEncoderPluginBase()
{}

int32_t AudioOpusEncoderPluginBase::SetPluginParameter(int32_t key, int32_t value)
{
    switch (key) {
        case static_cast<int32_t>(PluginParam::CHANNEL):
            channels_ = value;
            break;
        case static_cast<int32_t>(PluginParam::SAMPLE_RATE):
            samplingRate_ = value;
            break;
        case static_cast<int32_t>(PluginParam::BIT_RATE):
            bitrate_ = value;
            break;
        case static_cast<int32_t>(PluginParam::COMPLEXITY):
            complianceLevel_ = value;
            break;
        default:
            break;
    }
    return 0;
}

int32_t AudioOpusEncoderPluginBase::Init()
{
    int32_t dtxSwitch = 0;
    int32_t fecSwitch = 0;
    int32_t vbrSwitch = 1;
    enc_ = opus_encoder_create(samplingRate_, channels_, OPUS_APPLICATION_AUDIO, &(ret_));
    if (ret_ != OPUS_OK) {
        MEDIA_LOG_E("AudioOpusEncoderPluginBase create encoder failed");
        return -1;
    }

    int initBitrate = opus_encoder_ctl(enc_, OPUS_SET_BITRATE(bitrate_));
    int initBandwidth = opus_encoder_ctl(enc_, OPUS_SET_BANDWIDTH(OPUS_AUTO));
    int initVBR = opus_encoder_ctl(enc_, OPUS_SET_VBR(vbrSwitch));
    int initVBRConstraint = opus_encoder_ctl(enc_, OPUS_SET_VBR_CONSTRAINT(1));
    int initComplexity = opus_encoder_ctl(enc_, OPUS_SET_COMPLEXITY(complianceLevel_));
    int initInbandFEC = opus_encoder_ctl(enc_, OPUS_SET_INBAND_FEC(fecSwitch));
    int initForceChannels = opus_encoder_ctl(enc_, OPUS_SET_FORCE_CHANNELS(OPUS_AUTO));
    int initDTX = opus_encoder_ctl(enc_, OPUS_SET_DTX(dtxSwitch));
    int initLSBDepth = opus_encoder_ctl(enc_, OPUS_SET_LSB_DEPTH(LSB_DEPTH));
    int initExpFrameDuration = opus_encoder_ctl(enc_, OPUS_SET_EXPERT_FRAME_DURATION(OPUS_FRAMESIZE_ARG));
    if (initBitrate || initBandwidth || initVBR || initVBRConstraint || initComplexity ||
        initInbandFEC || initForceChannels || initDTX || initLSBDepth || initExpFrameDuration) {
            MEDIA_LOG_E("AudioOpusEncoderPlugin init failed");
            MEDIA_LOG_E("initBitrate: %{public}d, bitrate_: %{public}d", initBitrate, bitrate_);
            MEDIA_LOG_E("initBandwidth: %{public}d", initBandwidth);
            MEDIA_LOG_E("initVBR: %{public}d, vbrSwitch: %{public}d", initVBR, vbrSwitch);
            MEDIA_LOG_E("initVBRConstraint: %{public}d", initVBRConstraint);
            MEDIA_LOG_E("initComplexity: %{public}d, complianceLevel_: %{public}d", initComplexity, complianceLevel_);
            MEDIA_LOG_E("initInbandFEC: %{public}d, fecSwitch: %{public}d", initInbandFEC, fecSwitch);
            MEDIA_LOG_E("initForceChannels: %{public}d", initForceChannels);
            MEDIA_LOG_E("initDTX: %{public}d, dtxSwitch: %{public}d", initDTX, dtxSwitch);
            MEDIA_LOG_E("initLSBDepth: %{public}d", initLSBDepth);
            MEDIA_LOG_E("initExpFrameDuration: %{public}d", initExpFrameDuration);
            return -1;
    }

    opus_encoder_ctl(enc_, OPUS_GET_LOOKAHEAD(&padding_));

    frameSize_ = static_cast<int32_t>(samplingRate_ * TIME_MS / TIME_S_MS);  // mono frame size
    data_ = (uint8_t *)malloc(MAX_INPUT);
    if (!data_) {
        MEDIA_LOG_E("AudioOpusEncoderPluginBase allocate data memory failed");
        return -1;
    }
    return 0;
}

int32_t AudioOpusEncoderPluginBase::ProcessSendData(void *inputBuffer, int32_t *len)
{
        std::lock_guard<std::mutex> lock(avMutext_);
    if (enc_ == nullptr) {
        MEDIA_LOG_E("AudioOpusEncoderPluginBase enc is null");
        return -1;
    }
    if (!inputBuffer) {
        MEDIA_LOG_E("AudioOpusEncoderPluginBase inputBuffer is null");
        return -1;
    }
    in_ = static_cast<int16_t*>(inputBuffer);
    if (!data_) {
        MEDIA_LOG_E("AudioOpusEncoderPluginBase data is null");
        return -1;
    }
    *len = opus_encode(enc_, in_, frameSize_, data_, MAX_INPUT);
    if (*len >= 0) {
        return 0;
    } else {
        MEDIA_LOG_E("AudioOpusEncoderPluginBase encode_len is invalid");
        return -1;
    }
}

int32_t AudioOpusEncoderPluginBase::ProcessRecieveData(unsigned char **outBuffer, int32_t len)
{
    std::lock_guard<std::mutex> lock(avMutext_);
    *outBuffer = data_;
    if (len > 0) {
        ret_ = 0;
    } else {
        MEDIA_LOG_E("AudioOpusEncoderPluginBase len is invalid");
        ret_ = -1;
    }
    return ret_;
}

int32_t AudioOpusEncoderPluginBase::Reset()
{
    if (enc_ != nullptr) {
        opus_encoder_ctl(enc_, OPUS_RESET_STATE);
    }
    return 0;
}

int32_t AudioOpusEncoderPluginBase::Release()
{
    if (enc_ != nullptr) {
        opus_encoder_ctl(enc_, OPUS_RESET_STATE);
        opus_encoder_destroy(enc_);
        if (data_) {
            free(data_);
            data_ = nullptr;
        }
    }
    return 0;
}

int32_t AudioOpusEncoderPluginBase::Flush()
{
    if (enc_ != nullptr) {
        opus_encoder_ctl(enc_, OPUS_RESET_STATE);
    }
    return 0;
}

int32_t AudioOpusEncoderPluginBase::GetInputBufferSize()
{
    return frameSize_ * channels_ * sizeof(int16_t);
}

int32_t AudioOpusEncoderPluginBase::GetOutputBufferSize()
{
    return MAX_INPUT;
}

int32_t AudioOpusEncoderPluginBase::SetCodecConfig(std::vector<uint8_t> &codecConfig)
{
    constexpr int32_t OPUS_HEAD_SAMPLE_RATE = 48000;
    int32_t scaledPadding = (samplingRate_ > 0)
        ? padding_ * OPUS_HEAD_SAMPLE_RATE / samplingRate_
        : padding_;
    uint16_t writePadding = static_cast<uint16_t>(scaledPadding);
    uint32_t sampleRate = static_cast<uint32_t>(samplingRate_);
    std::vector<uint8_t> config = {
        'O', 'p', 'u', 's', 'H', 'e', 'a', 'd',
        0x1,
        static_cast<uint8_t>(channels_),
        (writePadding & 0xff),
        ((writePadding >> BITS_8) & 0xff),
        (sampleRate & 0xff),
        ((sampleRate >> BITS_8) & 0xff),
        ((sampleRate >> BITS_16) & 0xff),
        ((sampleRate >> BITS_24) & 0xff),
        0, 0, 0
    };
    codecConfig.assign(config.begin(), config.end());
    return 0;
}

AudioOpusDecoderPluginBase::AudioOpusDecoderPluginBase()
    : channels_(0), ret_(0), frameSizeMax_(0), samplingRate_(0), isMultistream_(false), streams_(0), coupled_(0)
{}

AudioOpusDecoderPluginBase::~AudioOpusDecoderPluginBase()
{}

int32_t AudioOpusDecoderPluginBase::SetPluginParameter(int32_t key, int32_t value)
{
    if (key == 0) {
        channels_ = value;
    }
    if (key == 1) {
        samplingRate_ = value;
    }
    if (key == PARAM_STREAMS) {
        streams_ = value;
    }
    if (key == PARAM_COUPLED) {
        coupled_ = value;
    }
    return 0;
}

int32_t AudioOpusDecoderPluginBase::SetPluginParameter(int32_t key, std::vector<uint8_t> value)
{
    if (key == PARAM_MAPPING) {
        mapping_ = value;
    }
    return 0;
}

int32_t AudioOpusDecoderPluginBase::Init()
{
    if (channels_ > 2) {  // 2
        isMultistream_ = true;
        msDec_ = opus_multistream_decoder_create(
            samplingRate_, channels_, streams_, coupled_, mapping_.data(), &(ret_));
    } else {
        dec_ = opus_decoder_create(samplingRate_, channels_, &(ret_));
    }
    if (ret_ != OPUS_OK) {
        MEDIA_LOG_E("AudioOpusDecoderPluginBase create decoder failed");
        return -1;
    }

    frameSizeMax_ = static_cast<int32_t>(samplingRate_ * TIME_MS_MAX / TIME_S_MS);  // mono frame size
    data_ = (short *)malloc(frameSizeMax_ * sizeof(short) * channels_);
    if (!data_) {
        MEDIA_LOG_E("AudioOpusDecoderPluginBase allocate data memory failed");
        return -1;
    }
    return 0;
}

int32_t AudioOpusDecoderPluginBase::ProcessSendData(void *inputBuffer, int32_t *len)
{
    std::lock_guard<std::mutex> lock(avMutext_);
    if (isMultistream_) {
        if (msDec_ == nullptr) {
            MEDIA_LOG_E("AudioOpusDecoderPluginBase dec is null");
            return -1;
        }
    } else {
        if (dec_ == nullptr) {
            MEDIA_LOG_E("AudioOpusDecoderPluginBase dec is null");
            return -1;
        }
    }

    if (inputBuffer == nullptr) {
        MEDIA_LOG_E("AudioOpusDecoderPluginBase inputBuffer is null");
        return -1;
    }

    in_ = static_cast<uint8_t*>(inputBuffer);
    if (!data_) {
        MEDIA_LOG_E("AudioOpusDecoderPluginBase data is null");
        return -1;
    }

    if (isMultistream_) {
        int32_t frameSize = opus_packet_get_nb_samples(in_, *len, samplingRate_);
        if (*len == 0) {
            frameSize = frameSizeMax_;
        }
        *len = opus_multistream_decode(msDec_, in_, *len, data_, frameSize, 0);
    } else {
        *len = opus_decode(dec_, in_, *len, data_, frameSizeMax_, 0);
    }

    if (*len >= 0) {
        return 0;
    } else {
        MEDIA_LOG_E("AudioOpusDecoderPluginBase decode_len is invalid");
        return -1;
    }
}

int32_t AudioOpusDecoderPluginBase::ProcessRecieveData(unsigned char **outBuffer, int32_t len)
{
    std::lock_guard<std::mutex> lock(avMutext_);
    *outBuffer = reinterpret_cast<uint8_t*>(data_);
    if (len < 0) {
        MEDIA_LOG_E("AudioOpusDecoderPluginBase len is invalid");
        return -1;
    }
    return 0;
}

int32_t AudioOpusDecoderPluginBase::Reset()
{
    if (dec_ != nullptr) {
        opus_decoder_ctl(dec_, OPUS_RESET_STATE);
    } else if (isMultistream_ && msDec_ != nullptr) {
        opus_multistream_decoder_ctl(msDec_, OPUS_RESET_STATE);
    }
    return 0;
}

int32_t AudioOpusDecoderPluginBase::Release()
{
    if (dec_ != nullptr) {
        opus_decoder_ctl(dec_, OPUS_RESET_STATE);
        opus_decoder_destroy(dec_);
        if (data_) {
            free(data_);
            data_ = nullptr;
        }
    } else if (isMultistream_ && msDec_ != nullptr) {
        opus_multistream_decoder_ctl(msDec_, OPUS_RESET_STATE);
        opus_multistream_decoder_destroy(msDec_);
        if (data_) {
            free(data_);
            data_ = nullptr;
        }
    }
    return 0;
}

int32_t AudioOpusDecoderPluginBase::Flush()
{
    if (dec_ != nullptr) {
        opus_decoder_ctl(dec_, OPUS_RESET_STATE);
    } else if (isMultistream_ && msDec_ != nullptr) {
        opus_multistream_decoder_ctl(msDec_, OPUS_RESET_STATE);
    }
    return 0;
}

int32_t AudioOpusDecoderPluginBase::GetInputBufferSize()
{
    return MAX_INPUT;
}

int32_t AudioOpusDecoderPluginBase::GetOutputBufferSize()
{
    return frameSizeMax_ * channels_ * sizeof(int16_t);
}

}  // namespace MediaAVCodec
}  // namespace OHOS

using namespace OHOS::MediaAVCodec;
int32_t OpusPluginClassEncoderCreate(AudioOpusEncoderPluginBase **pluginPtr)
{
    *pluginPtr = new AudioOpusEncoderPluginBase();
    return 0;
}

int32_t OpusPluginClassDecoderCreate(AudioOpusDecoderPluginBase **pluginPtr)
{
    *pluginPtr = new AudioOpusDecoderPluginBase();
    return 0;
}