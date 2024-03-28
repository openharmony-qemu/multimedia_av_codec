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

#include "audio_ffmpeg_mp3_decoder_plugin.h"
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "media_description.h"
#include "avcodec_mime_type.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_AUDIO, "AvCodec-AudioFFMpegMp3DecoderPlugin"};
constexpr int MIN_CHANNELS = 1;
constexpr int MAX_CHANNELS = 2;
constexpr int SAMPLE_RATE_RATIO = 31;
constexpr int SUPPORT_SAMPLE_RATE = 9;
constexpr int BUFFER_DIFF = 128;
constexpr int MIN_OUTBUF_SIZE = 2500;
constexpr int INPUT_BUFFER_SIZE_DEFAULT = 8192;
} // namespace

namespace OHOS {
namespace MediaAVCodec {
constexpr std::string_view AUDIO_CODEC_NAME = "mp3";
AudioFFMpegMp3DecoderPlugin::AudioFFMpegMp3DecoderPlugin() : basePlugin(std::make_unique<AudioFfmpegDecoderPlugin>())
{
    channels = 0;
    sampleRate = 0;
}

AudioFFMpegMp3DecoderPlugin::~AudioFFMpegMp3DecoderPlugin()
{
    basePlugin->Release();
    basePlugin.reset();
    basePlugin = nullptr;
}

int32_t AudioFFMpegMp3DecoderPlugin::Init(const Format &format)
{
    int32_t ret = basePlugin->AllocateContext("mp3");
    int32_t checkresult = AudioFFMpegMp3DecoderPlugin::Checkinit(format);
    if (checkresult != AVCodecServiceErrCode::AVCS_ERR_OK) {
        return checkresult;
    }
    if (ret != AVCodecServiceErrCode::AVCS_ERR_OK) {
        AVCODEC_LOGE("mp3 init error.");
        return ret;
    }
    ret = basePlugin->InitContext(format);
    if (ret != AVCodecServiceErrCode::AVCS_ERR_OK) {
        AVCODEC_LOGE("mp3 init error.");
        return ret;
    }
    return basePlugin->OpenContext();
}

int32_t AudioFFMpegMp3DecoderPlugin::ProcessSendData(const std::shared_ptr<AudioBufferInfo> &inputBuffer)
{
    return basePlugin->ProcessSendData(inputBuffer);
}

int32_t AudioFFMpegMp3DecoderPlugin::ProcessRecieveData(std::shared_ptr<AudioBufferInfo> &outBuffer)
{
    return basePlugin->ProcessRecieveData(outBuffer);
}

int32_t AudioFFMpegMp3DecoderPlugin::Reset()
{
    return basePlugin->Reset();
}

int32_t AudioFFMpegMp3DecoderPlugin::Release()
{
    return basePlugin->Release();
}

int32_t AudioFFMpegMp3DecoderPlugin::Flush()
{
    return basePlugin->Flush();
}

int32_t AudioFFMpegMp3DecoderPlugin::GetInputBufferSize() const
{
    int32_t maxSize = basePlugin->GetMaxInputSize();
    if (maxSize < 0 || maxSize > INPUT_BUFFER_SIZE_DEFAULT) {
        maxSize = INPUT_BUFFER_SIZE_DEFAULT;
    }
    return maxSize;
}

int32_t AudioFFMpegMp3DecoderPlugin::GetOutputBufferSize() const
{
    int32_t maxSize = (sampleRate / SAMPLE_RATE_RATIO + BUFFER_DIFF) * channels * sizeof(short);
    int32_t minSize = MIN_OUTBUF_SIZE * channels * sizeof(short);
    if (maxSize < minSize) {
        maxSize = minSize;
    }
    return maxSize;
}

Format AudioFFMpegMp3DecoderPlugin::GetFormat() const noexcept
{
    auto format = basePlugin->GetFormat();
    format.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_MPEG);
    return format;
}

int32_t AudioFFMpegMp3DecoderPlugin::Checkinit(const Format &format)
{
    int sampleRatePick[SUPPORT_SAMPLE_RATE] = {8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000};
    format.GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, channels);
    format.GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, sampleRate);
    if (channels < MIN_CHANNELS || channels > MAX_CHANNELS) {
        return AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL;
    }

    for (int i = 0; i < SUPPORT_SAMPLE_RATE; i++) {
        if (sampleRate == sampleRatePick[i]) {
            break;
        } else if (i == SUPPORT_SAMPLE_RATE - 1) {
            return AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL;
        }
    }

    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

std::string_view AudioFFMpegMp3DecoderPlugin::GetCodecType() const noexcept
{
    return AUDIO_CODEC_NAME;
}
} // namespace MediaAVCodec
} // namespace OHOS