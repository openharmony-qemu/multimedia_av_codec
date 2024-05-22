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

#include "audio_ffmpeg_amrnb_decoder_plugin.h"
#include <set>
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "media_description.h"
#include "avcodec_mime_type.h"
#include "avcodec_audio_common.h"
#include "ffmpeg_converter.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_AUDIO, "AvCodec-AudioFFMpegAmrnbDecoderPlugin"};
constexpr int SUPPORT_CHANNELS = 1;
constexpr int SUPPORT_SAMPLE_RATE = 8000;
constexpr AVSampleFormat DEFAULT_FFMPEG_SAMPLE_FORMAT = AV_SAMPLE_FMT_S16;
constexpr int INPUT_BUFFER_SIZE_DEFAULT = 150;
constexpr int OUTPUT_BUFFER_SIZE_DEFAULT = 1280;
static std::set<OHOS::MediaAVCodec::AudioSampleFormat> supportedSampleFormats = {
    OHOS::MediaAVCodec::AudioSampleFormat::SAMPLE_S16LE,
    OHOS::MediaAVCodec::AudioSampleFormat::SAMPLE_F32LE};
} // namespace

namespace OHOS {
namespace MediaAVCodec {
constexpr std::string_view AUDIO_CODEC_NAME = "amrnb";
AudioFFMpegAmrnbDecoderPlugin::AudioFFMpegAmrnbDecoderPlugin()
    : basePlugin(std::make_unique<AudioFfmpegDecoderPlugin>())
{
    channels = 0;
    sampleRate = 0;
    bitRate = 0;
}

AudioFFMpegAmrnbDecoderPlugin::~AudioFFMpegAmrnbDecoderPlugin()
{
    basePlugin->Release();
    basePlugin.reset();
    basePlugin = nullptr;
}

int32_t AudioFFMpegAmrnbDecoderPlugin::Init(const Format &format)
{
    int32_t ret = basePlugin->AllocateContext("amrnb");
    int32_t checkresult = AudioFFMpegAmrnbDecoderPlugin::Checkinit(format);
    if (checkresult != AVCodecServiceErrCode::AVCS_ERR_OK) {
        return checkresult;
    }
    if (ret != AVCodecServiceErrCode::AVCS_ERR_OK) {
        AVCODEC_LOGE("amrwb init error.");
        return ret;
    }
    ret = basePlugin->InitContext(format);
    if (ret != AVCodecServiceErrCode::AVCS_ERR_OK) {
        AVCODEC_LOGE("amrwb init error.");
        return ret;
    }
    ret = basePlugin->OpenContext();
    basePlugin->DisableNeedResample();
    return ret;
}

int32_t AudioFFMpegAmrnbDecoderPlugin::ProcessSendData(const std::shared_ptr<AudioBufferInfo> &inputBuffer)
{
    return basePlugin->ProcessSendData(inputBuffer);
}

int32_t AudioFFMpegAmrnbDecoderPlugin::ProcessRecieveData(std::shared_ptr<AudioBufferInfo> &outBuffer)
{
    return basePlugin->ProcessRecieveData(outBuffer);
}

int32_t AudioFFMpegAmrnbDecoderPlugin::Reset()
{
    return basePlugin->Reset();
}

int32_t AudioFFMpegAmrnbDecoderPlugin::Release()
{
    return basePlugin->Release();
}

int32_t AudioFFMpegAmrnbDecoderPlugin::Flush()
{
    return basePlugin->Flush();
}

int32_t AudioFFMpegAmrnbDecoderPlugin::GetInputBufferSize() const
{
    int32_t maxSize = basePlugin->GetMaxInputSize();
    if (maxSize < 0 || maxSize > INPUT_BUFFER_SIZE_DEFAULT) {
        maxSize = INPUT_BUFFER_SIZE_DEFAULT;
    }
    return maxSize;
}

int32_t AudioFFMpegAmrnbDecoderPlugin::GetOutputBufferSize() const
{
    return OUTPUT_BUFFER_SIZE_DEFAULT;
}

Format AudioFFMpegAmrnbDecoderPlugin::GetFormat() const noexcept
{
    auto format = basePlugin->GetFormat();
    format.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_MPEG);
    return format;
}

int32_t AudioFFMpegAmrnbDecoderPlugin::Checkinit(const Format &format)
{
    format.GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, channels);
    format.GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, sampleRate);
    format.GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, bitRate);
    if (channels != SUPPORT_CHANNELS) {
        return AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL;
    }

    if (sampleRate != SUPPORT_SAMPLE_RATE) {
        return AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL;
    }
    if (!CheckSampleFormat(format)) {
        return AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL;
    }
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

bool AudioFFMpegAmrnbDecoderPlugin::CheckSampleFormat(const Format &format)
{
    int32_t sampleFormat;
    if (!format.GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, sampleFormat)) {
        AVCODEC_LOGW("Sample format missing, set to default s16le");
        basePlugin->EnableResample(DEFAULT_FFMPEG_SAMPLE_FORMAT);
        return true;
    }
    if (supportedSampleFormats.find(static_cast<AudioSampleFormat>(sampleFormat)) == supportedSampleFormats.end()) {
        AVCODEC_LOGE("Output sample format not support");
        return false;
    }
    if (sampleFormat == SAMPLE_F32LE) {
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

std::string_view AudioFFMpegAmrnbDecoderPlugin::GetCodecType() const noexcept
{
    return AUDIO_CODEC_NAME;
}
} // namespace MediaAVCodec
} // namespace OHOS