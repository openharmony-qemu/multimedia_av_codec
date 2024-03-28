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

#include "audio_ffmpeg_flac_decoder_plugin.h"
#include "avcodec_errors.h"
#include "media_description.h"
#include "avcodec_trace.h"
#include "avcodec_log.h"
#include "avcodec_mime_type.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_AUDIO, "AvCodec-AudioFFMpegFlacDecoderPlugin"};
constexpr int32_t MAX_BITS_PER_SAMPLE = 4;
constexpr int32_t SAMPLES = 9216;
constexpr int32_t MIN_CHANNELS = 1;
constexpr int32_t MAX_CHANNELS = 8;
static const int32_t FLAC_DECODER_SAMPLE_RATE_TABLE[] = {
    8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000, 64000, 88200, 96000, 192000,
};
} // namespace

namespace OHOS {
namespace MediaAVCodec {
constexpr std::string_view AUDIO_CODEC_NAME = "flac";
AudioFFMpegFlacDecoderPlugin::AudioFFMpegFlacDecoderPlugin() : basePlugin(std::make_unique<AudioFfmpegDecoderPlugin>())
{
    channels = 0;
}

AudioFFMpegFlacDecoderPlugin::~AudioFFMpegFlacDecoderPlugin()
{
    basePlugin->Release();
    basePlugin.reset();
    basePlugin = nullptr;
}

static bool CheckSampleRate(int32_t sampleRate)
{
    for (auto i : FLAC_DECODER_SAMPLE_RATE_TABLE) {
        if (i == sampleRate) {
            return true;
        }
    }
    return false;
}

int32_t AudioFFMpegFlacDecoderPlugin::CheckFormat(const Format &format)
{
    int32_t channelCount;
    int32_t sampleRate;
    format.GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, channelCount);
    format.GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, sampleRate);
    if (!CheckSampleRate(sampleRate)) {
        AVCODEC_LOGE("init failed, because sampleRate=%{public}d not in table.", sampleRate);
        return AVCodecServiceErrCode::AVCS_ERR_MISMATCH_SAMPLE_RATE;
    } else if (channelCount < MIN_CHANNELS || channelCount > MAX_CHANNELS) {
        AVCODEC_LOGE("init failed, because channelCount=%{public}d not support.", channelCount);
        return AVCodecServiceErrCode::AVCS_ERR_CONFIGURE_MISMATCH_CHANNEL_COUNT;
    }
    channels = channelCount;
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

int32_t AudioFFMpegFlacDecoderPlugin::Init(const Format &format)
{
    int32_t ret = basePlugin->AllocateContext("flac");
    if (ret != AVCodecServiceErrCode::AVCS_ERR_OK) {
        AVCODEC_LOGE("init failed, because AllocateContext failed. ret=%{public}d", ret);
        return ret;
    }

    ret = CheckFormat(format);
    if (ret != AVCodecServiceErrCode::AVCS_ERR_OK) {
        AVCODEC_LOGE("init failed, because CheckFormat failed. ret=%{public}d", ret);
        return ret;
    }

    ret = basePlugin->InitContext(format);
    if (ret != AVCodecServiceErrCode::AVCS_ERR_OK) {
        AVCODEC_LOGE("init failed, because InitContext failed. ret=%{public}d", ret);
        return ret;
    }

    ret = basePlugin->OpenContext();
    if (ret != AVCodecServiceErrCode::AVCS_ERR_OK) {
        AVCODEC_LOGE("init failed, because OpenContext failed. ret=%{public}d", ret);
        return ret;
    }

    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

int32_t AudioFFMpegFlacDecoderPlugin::ProcessSendData(const std::shared_ptr<AudioBufferInfo> &inputBuffer)
{
    return basePlugin->ProcessSendData(inputBuffer);
}

int32_t AudioFFMpegFlacDecoderPlugin::ProcessRecieveData(std::shared_ptr<AudioBufferInfo> &outBuffer)
{
    return basePlugin->ProcessRecieveData(outBuffer);
}

int32_t AudioFFMpegFlacDecoderPlugin::Reset()
{
    return basePlugin->Reset();
}

int32_t AudioFFMpegFlacDecoderPlugin::Release()
{
    return basePlugin->Release();
}

int32_t AudioFFMpegFlacDecoderPlugin::Flush()
{
    return basePlugin->Flush();
}

int32_t AudioFFMpegFlacDecoderPlugin::GetInputBufferSize() const
{
    int32_t inputBufferSize = SAMPLES * channels * MAX_BITS_PER_SAMPLE;
    int32_t maxSize = basePlugin->GetMaxInputSize();
    if (maxSize < 0 || maxSize > inputBufferSize) {
        maxSize = inputBufferSize;
    }
    return maxSize;
}

int32_t AudioFFMpegFlacDecoderPlugin::GetOutputBufferSize() const
{
    int32_t outputBufferSize = SAMPLES * channels * MAX_BITS_PER_SAMPLE;
    return outputBufferSize;
}

Format AudioFFMpegFlacDecoderPlugin::GetFormat() const noexcept
{
    auto format = basePlugin->GetFormat();
    format.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_FLAC);
    return format;
}

std::string_view AudioFFMpegFlacDecoderPlugin::GetCodecType() const noexcept
{
    return AUDIO_CODEC_NAME;
}
} // namespace MediaAVCodec
} // namespace OHOS