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
#include "opus_plugin_ext.h"
#include "common/log.h"

namespace OHOS {
namespace MediaAVCodec {
constexpr int32_t MAX_INPUT = 1500;
constexpr int32_t LSB_DEPTH = 16;
constexpr int32_t DOUBLE = 2;
constexpr int32_t TIME_MS = 20;
constexpr int32_t TIME_S_MS = 1000;

AudioOpusEncoderPlugin::AudioOpusEncoderPlugin()
    : channels_(0), ret_(0), frameSize_(0), samplingRate_(0), bitrate_(0), complianceLevel_(0)
{}

AudioOpusEncoderPlugin::~AudioOpusEncoderPlugin()
{}

int32_t AudioOpusEncoderPlugin::SetPluginParameter(int32_t key, int32_t value)
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

int32_t AudioOpusEncoderPlugin::Init()
{
    int32_t dtxSwitch = 0;
    int32_t fecSwitch = 0;
    int32_t vbrSwitch = 1;

    enc_ = opus_encoder_create(samplingRate_, channels_, OPUS_APPLICATION_AUDIO, &(ret_));
    if (ret_ != OPUS_OK) {
        MEDIA_LOG_E("AudioOpusEncoderPlugin create encoder failed!");
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
            MEDIA_LOG_E("initBitrate: %{public}d, bitrate: %{public}d", initBitrate, bitrate_);
            MEDIA_LOG_E("initBandwidth: %{public}d", initBandwidth);
            MEDIA_LOG_E("initVBR: %{public}d, vbrSwitch: %{public}d", initVBR, vbrSwitch);
            MEDIA_LOG_E("initVBRConstraint: %{public}d", initVBRConstraint);
            MEDIA_LOG_E("initComplexity: %{public}d, complianceLevel: %{public}d", initComplexity, complianceLevel_);
            MEDIA_LOG_E("initInbandFEC: %{public}d, fecSwitch: %{public}d", initInbandFEC, fecSwitch);
            MEDIA_LOG_E("initForceChannels: %{public}d", initForceChannels);
            MEDIA_LOG_E("initDTX: %{public}d, dtxSwitch: %{public}d", initDTX, dtxSwitch);
            MEDIA_LOG_E("initLSBDepth: %{public}d", initLSBDepth);
            MEDIA_LOG_E("initExpFrameDuration: %{public}d", initExpFrameDuration);
            return -1;
    }
    frameSize_ = static_cast<int32_t>(samplingRate_ * TIME_MS / TIME_S_MS);  // mono frame size
    data_ = (uint8_t *)malloc(MAX_INPUT);
    if (!data_) {
        MEDIA_LOG_E("AudioOpusEncoderPlugin allocate data memory failed");
        return -1;
    }
    return 0;
}

int32_t AudioOpusEncoderPlugin::ProcessSendData(void *inputBuffer, int32_t *len)
{
    std::lock_guard<std::mutex> lock(avMutext_);
    if (enc_ == nullptr) {
        MEDIA_LOG_E("AudioOpusEncoderPlugin enc is null");
        return -1;
    }
    if (!inputBuffer) {
        MEDIA_LOG_E("AudioOpusEncoderPlugin inputBuffer is null");
        return -1;
    }
    in_ = static_cast<int16_t*>(inputBuffer);
    *len = opus_encode(enc_, in_, frameSize_, data_, MAX_INPUT);
    if (*len >= 0) {
        return 0;
    } else {
        MEDIA_LOG_E("AudioOpusEncoderPlugin encode_len is invalid");
        return -1;
    }
}

int32_t AudioOpusEncoderPlugin::ProcessRecieveData(unsigned char **outBuffer, int32_t len)
{
    std::lock_guard<std::mutex> lock(avMutext_);
    *outBuffer = data_;
    if (len > 0) {
        ret_ = 0;
    } else {
        MEDIA_LOG_E("AudioOpusEncoderPlugin len is invalid");
        ret_ = -1;
    }
    return ret_;
}

int32_t AudioOpusEncoderPlugin::Reset()
{
    if (enc_ != nullptr) {
        opus_encoder_ctl(enc_, OPUS_RESET_STATE);
    }
    return 0;
}

int32_t AudioOpusEncoderPlugin::Release()
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

int32_t AudioOpusEncoderPlugin::Flush()
{
    if (enc_ != nullptr) {
        opus_encoder_ctl(enc_, OPUS_RESET_STATE);
    }
    return 0;
}

int32_t AudioOpusEncoderPlugin::GetInputBufferSize()
{
    return frameSize_ * channels_ * sizeof(int16_t) * DOUBLE;
}

int32_t AudioOpusEncoderPlugin::GetOutputBufferSize()
{
    return MAX_INPUT;
}

AudioOpusDecoderPlugin::AudioOpusDecoderPlugin() : channels_(0), ret_(0), frameSize_(0), samplingRate_(0)
{}

AudioOpusDecoderPlugin::~AudioOpusDecoderPlugin()
{}

int32_t AudioOpusDecoderPlugin::SetPluginParameter(int32_t key, int32_t value)
{
    if (key == 0) {
        channels_ = value;
    }
    if (key == 1) {
        samplingRate_ = value;
    }
    return 0;
}

int32_t AudioOpusDecoderPlugin::Init()
{
    dec_ = opus_decoder_create(samplingRate_, channels_, &(ret_));
    constexpr int32_t decTimeMs = 120;  // 120 ms
    frameSize_ = static_cast<int32_t>(samplingRate_ * decTimeMs / TIME_S_MS);  // mono frame size
    data_ = (short *)malloc(frameSize_ * sizeof(int16_t) * channels_ * DOUBLE);
    if (!data_) {
        MEDIA_LOG_E("AudioOpusDecoderPlugin allocate data memory failed");
        return -1;
    }
    return 0;
}

int32_t AudioOpusDecoderPlugin::ProcessSendData(void *inputBuffer, int32_t *len)
{
    std::lock_guard<std::mutex> lock(avMutext_);
    if (dec_ == nullptr) {
        MEDIA_LOG_E("AudioOpusDecoderPlugin dec is null");
        return -1;
    }
    if (inputBuffer == nullptr) {
        MEDIA_LOG_E("AudioOpusDecoderPlugin inputBuffer is null");
        return -1;
    }
    in_ = static_cast<uint8_t*>(inputBuffer);
    *len = opus_decode(dec_, in_, *len, data_, frameSize_ * DOUBLE, 0);

    if (*len >= 0) {
        return 0;
    } else {
        MEDIA_LOG_E("AudioOpusDecoderPlugin decode_len is invalid");
        return -1;
    }
}

int32_t AudioOpusDecoderPlugin::ProcessRecieveData(unsigned char **outBuffer, int32_t len)
{
    std::lock_guard<std::mutex> lock(avMutext_);
    *outBuffer = reinterpret_cast<uint8_t*>(data_);
    if (len < 0) {
        MEDIA_LOG_E("AudioOpusDecoderPlugin len is invalid");
        return -1;
    }
    return 0;
}


int32_t AudioOpusDecoderPlugin::Reset()
{
    if (dec_ != nullptr) {
        opus_decoder_ctl(dec_, OPUS_RESET_STATE);
    }
    return 0;
}

int32_t AudioOpusDecoderPlugin::Release()
{
    if (dec_ != nullptr) {
        opus_decoder_ctl(dec_, OPUS_RESET_STATE);
        opus_decoder_destroy(dec_);
        if (data_) {
            free(data_);
            data_ = nullptr;
        }
    }
    return 0;
}

int32_t AudioOpusDecoderPlugin::Flush()
{
    opus_decoder_ctl(dec_, OPUS_RESET_STATE);
    return 0;
}

int32_t AudioOpusDecoderPlugin::GetInputBufferSize()
{
    return MAX_INPUT;
}

int32_t AudioOpusDecoderPlugin::GetOutputBufferSize()
{
    return frameSize_ * channels_ * sizeof(short) * DOUBLE;
}
}  // namespace MediaAVCodec
}  // namespace OHOS

int32_t OpusPluginClassEncoderCreate(OHOS::MediaAVCodec::AudioBaseCodecExt **pluginPtr)
{
    *pluginPtr = new OHOS::MediaAVCodec::AudioOpusEncoderPlugin();
    return 0;
}

int32_t OpusPluginClassDecoderCreate(OHOS::MediaAVCodec::AudioBaseCodecExt **pluginPtr)
{
    *pluginPtr = new OHOS::MediaAVCodec::AudioOpusDecoderPlugin();
    return 0;
}