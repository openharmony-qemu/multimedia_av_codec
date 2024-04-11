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
#include "avcodec_trace.h"
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "media_description.h"
#include "avcodec_mime_type.h"
#include "avcodec_audio_common.h"
#include "ffmpeg_converter.h"

namespace {
constexpr int32_t MIN_CHANNELS = 1;
constexpr int32_t MAX_CHANNELS = 8;
constexpr AVSampleFormat DEFAULT_FFMPEG_SAMPLE_FORMAT = AV_SAMPLE_FMT_FLT;
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_AUDIO, "AvCodec-AudioFFMpegAacDecoderPlugin"};
constexpr int32_t INPUT_BUFFER_SIZE_DEFAULT = 8192;
constexpr int32_t OUTPUT_BUFFER_SIZE_DEFAULT = 4 * 1024 * 8;
constexpr std::string_view AUDIO_CODEC_NAME = "aac";
static std::set<int32_t> supportedSampleRate = {96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000,
                                                11025, 8000, 7350};
static std::set<OHOS::MediaAVCodec::AudioSampleFormat> supportedSampleFormats = {
    OHOS::MediaAVCodec::AudioSampleFormat::SAMPLE_S16LE,
    OHOS::MediaAVCodec::AudioSampleFormat::SAMPLE_F32LE};
static std::set<int32_t> supportedSampleRates = {7350, 8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000,
                                                 64000, 88200, 96000};
}

namespace OHOS {
namespace MediaAVCodec {
AudioFFMpegAacDecoderPlugin::AudioFFMpegAacDecoderPlugin()
    : basePlugin(std::make_unique<AudioFfmpegDecoderPlugin>()), channels_(0) {}

AudioFFMpegAacDecoderPlugin::~AudioFFMpegAacDecoderPlugin()
{
    basePlugin->Release();
    basePlugin.reset();
    basePlugin = nullptr;
}

bool AudioFFMpegAacDecoderPlugin::CheckAdts(const Format &format)
{
    int type;
    if (format.GetIntValue(MediaDescriptionKey::MD_KEY_AAC_IS_ADTS, type)) {
        if (type != 1 && type != 0) {
            AVCODEC_LOGE("aac_is_adts value invalid");
            return false;
        }
    } else {
        AVCODEC_LOGW("aac_is_adts parameter is missing");
        type = 1;
    }
    aacName_ = (type == 1 ? "aac" : "aac_latm");

    return true;
}

bool AudioFFMpegAacDecoderPlugin::CheckSampleFormat(const Format &format)
{
    int32_t sampleFormat;
    if (!format.GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, sampleFormat)) {
        AVCODEC_LOGW("Sample format missing, set to default f32le");
        if (channels_ != 1) {
            basePlugin->EnableResample(DEFAULT_FFMPEG_SAMPLE_FORMAT);
        }
        return true;
    }

    if (supportedSampleFormats.find(static_cast<AudioSampleFormat>(sampleFormat)) == supportedSampleFormats.end()) {
        AVCODEC_LOGE("Output sample format not support");
        return false;
    }
    if (channels_ == 1 && sampleFormat == AudioSampleFormat::SAMPLE_F32LE) {
        return true;
    }
    auto destFmt = FFMpegConverter::ConvertOHAudioFormatToFFMpeg(static_cast<AudioSampleFormat>(sampleFormat));
    if (destFmt == AV_SAMPLE_FMT_NONE) {
        AVCODEC_LOGE("Convert format failed, avSampleFormat not found");
        return false;
    }
    basePlugin->EnableResample(destFmt);
    return true;
}

bool AudioFFMpegAacDecoderPlugin::CheckChannelCount(const Format &format)
{
    if (!format.GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, channels_)) {
        AVCODEC_LOGE("parameter channel_count missing");
        return false;
    }
    if (channels_ < MIN_CHANNELS || channels_ > MAX_CHANNELS) {
        AVCODEC_LOGE("parameter channel_count invaild");
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
    if (supportedSampleRates.find(sampleRate) == supportedSampleRates.end()) {
        AVCODEC_LOGE("parameter sample_rate invalid, %{public}d", sampleRate);
        return false;
    }
    return true;
}

bool AudioFFMpegAacDecoderPlugin::CheckFormat(const Format &format)
{
    if (!CheckAdts(format) || !CheckChannelCount(format) || !CheckSampleFormat(format) || !CheckSampleRate(format)) {
        return false;
    }
    return true;
}

int32_t AudioFFMpegAacDecoderPlugin::Init(const Format &format)
{
    if (!CheckFormat(format)) {
        return AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL;
    }
    int32_t ret = basePlugin->AllocateContext(aacName_);
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