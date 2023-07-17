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

#include "audio_ffmpeg_aac_decoder_plugin.h"
#include <set>
#include "avcodec_dfx.h"
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "media_description.h"
#include "avcodec_mime_type.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AvCodec-AudioFFMpegAacDecoderPlugin"};
constexpr int32_t INPUT_BUFFER_SIZE_DEFAULT = 8192;
constexpr int32_t OUTPUT_BUFFER_SIZE_DEFAULT = 4 * 1024 * 8;
constexpr std::string_view AUDIO_CODEC_NAME = "aac";
constexpr int32_t MIN_CHANNELS = 1;
constexpr int32_t MAX_CHANNELS = 8;
static std::set<int32_t> supportedSampleRate = {96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000,
                                                11025, 8000, 7350};
}

namespace OHOS {
namespace MediaAVCodec {
AudioFFMpegAacDecoderPlugin::AudioFFMpegAacDecoderPlugin() : basePlugin(std::make_unique<AudioFfmpegDecoderPlugin>()) {}

AudioFFMpegAacDecoderPlugin::~AudioFFMpegAacDecoderPlugin()
{
    basePlugin->Release();
    basePlugin.reset();
    basePlugin = nullptr;
}

bool AudioFFMpegAacDecoderPlugin::CheckChannelCount(const Format &format) const
{
    int32_t channelCount;
    if (!format.GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, channelCount)) {
        AVCODEC_LOGE("parameter channel_count missing");
        return false;
    }
    if (channelCount < MIN_CHANNELS || channelCount > MAX_CHANNELS) {
        AVCODEC_LOGE("channelCount=%{public}d not support.", channelCount);
        return false;
    }
    return true;
}

bool AudioFFMpegAacDecoderPlugin::CheckSampleRate(const Format &format) const
{
    int32_t sampleRate;
    if (!format.GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, sampleRate)) {
        AVCODEC_LOGE("parameter sample_rate missing");
        return false;
    }
    if (supportedSampleRate.find(sampleRate) == supportedSampleRate.end()) {
        AVCODEC_LOGE("sample rate: %{public}d not supported", sampleRate);
        return false;
    }
    return true;
}

bool AudioFFMpegAacDecoderPlugin::CheckFormat(const Format &format) const
{
    if (!CheckChannelCount(format)) {
        return false;
    }
    if (!CheckSampleRate(format)) {
        return false;
    }
    return true;
}

int32_t AudioFFMpegAacDecoderPlugin::Init(const Format &format)
{
    if (!CheckFormat(format)) {
        return AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL;
    }
    int type;
    if (format.GetIntValue(MediaDescriptionKey::MD_KEY_AAC_IS_ADTS, type)) {
        if (type != 1 && type != 0) {
            AVCODEC_LOGE("aac_is_adts value invalid");
            return AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL;
        }
    } else {
        AVCODEC_LOGW("aac_is_adts parameter is missing");
        type = 1;
    }
    std::string aacName = (type == 1 ? "aac" : "aac_latm");
    int32_t ret = basePlugin->AllocateContext(aacName);
    if (ret != AVCodecServiceErrCode::AVCS_ERR_OK) {
        AVCODEC_LOGE("AllocateContext failed, ret=%{public}d", ret);
        return ret;
    }
    ret = basePlugin->InitContext(format);
    if (ret != AVCodecServiceErrCode::AVCS_ERR_OK) {
        AVCODEC_LOGE("InitContext failed, ret=%{public}d", ret);
        return ret;
    }
    return basePlugin->OpenContext();
}

int32_t AudioFFMpegAacDecoderPlugin::ProcessSendData(const std::shared_ptr<AudioBufferInfo> &inputBuffer)
{
    return basePlugin->ProcessSendData(inputBuffer);
}

int32_t AudioFFMpegAacDecoderPlugin::ProcessRecieveData(std::shared_ptr<AudioBufferInfo> &outBuffer)
{
    return basePlugin->ProcessRecieveData(outBuffer);
}

int32_t AudioFFMpegAacDecoderPlugin::Reset()
{
    return basePlugin->Reset();
}

int32_t AudioFFMpegAacDecoderPlugin::Release()
{
    return basePlugin->Release();
}

int32_t AudioFFMpegAacDecoderPlugin::Flush()
{
    return basePlugin->Flush();
}

int32_t AudioFFMpegAacDecoderPlugin::GetInputBufferSize() const
{
    int32_t maxSize = basePlugin->GetMaxInputSize();
    if (maxSize < 0 || maxSize > INPUT_BUFFER_SIZE_DEFAULT) {
        maxSize = INPUT_BUFFER_SIZE_DEFAULT;
    }
    return maxSize;
}

int32_t AudioFFMpegAacDecoderPlugin::GetOutputBufferSize() const
{
    return OUTPUT_BUFFER_SIZE_DEFAULT;
}

Format AudioFFMpegAacDecoderPlugin::GetFormat() const noexcept
{
    auto format = basePlugin->GetFormat();
    format.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_AAC);
    return format;
}

std::string_view AudioFFMpegAacDecoderPlugin::GetCodecType() const noexcept
{
    return AUDIO_CODEC_NAME;
}
} // namespace MediaAVCodec
} // namespace OHOS