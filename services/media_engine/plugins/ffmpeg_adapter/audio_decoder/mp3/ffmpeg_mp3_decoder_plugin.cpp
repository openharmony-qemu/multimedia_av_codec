/*
 * Copyright (c) 2023-2023 Huawei Device Co., Ltd.
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
#include "ffmpeg_mp3_decoder_plugin.h"
#include "avcodec_codec_name.h"
#include "avcodec_log.h"
#include "plugin/codec_plugin.h"
#include "plugin/plugin_definition.h"
#include "avcodec_mime_type.h"

namespace {
using namespace OHOS::Media;
using namespace OHOS::Media::Plugins;
using namespace Ffmpeg;

constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_AUDIO, "AvCodec-AudioFFMpegMp3DecoderPlugin"};
constexpr int32_t MIN_CHANNELS = 1;
constexpr int32_t MAX_CHANNELS = 2;
constexpr int32_t SAMPLE_RATE_RATIO = 31;
constexpr int32_t SUPPORT_SAMPLE_RATE = 9;
constexpr int32_t BUFFER_DIFF = 128;
constexpr int32_t MIN_OUTBUF_SIZE = 2500;
constexpr int32_t INPUT_BUFFER_SIZE_DEFAULT = 8192;
constexpr int32_t SAMPLE_RATE_PICK[SUPPORT_SAMPLE_RATE] = {8000, 11025, 12000, 16000, 22050,
                                                           24000, 32000, 44100, 48000};
} // namespace

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Ffmpeg {
FFmpegMp3DecoderPlugin::FFmpegMp3DecoderPlugin(const std::string& name)
    : CodecPlugin(name), channels(0), sampleRate(0), basePlugin(std::make_unique<FfmpegBaseDecoder>())
{
}

FFmpegMp3DecoderPlugin::~FFmpegMp3DecoderPlugin()
{
    basePlugin->Release();
    basePlugin.reset();
    basePlugin = nullptr;
}

Status FFmpegMp3DecoderPlugin::Init()
{
    return Status::OK;
}

Status FFmpegMp3DecoderPlugin::Prepare()
{
    return Status::OK;
}

Status FFmpegMp3DecoderPlugin::Reset()
{
    return basePlugin->Reset();
}

Status FFmpegMp3DecoderPlugin::Start()
{
    return Status::OK;
}

Status FFmpegMp3DecoderPlugin::Stop()
{
    return Status::OK;
}

Status FFmpegMp3DecoderPlugin::SetParameter(const std::shared_ptr<Meta> &parameter)
{
    Status ret = basePlugin->AllocateContext("mp3");
    Status checkresult = CheckInit(parameter);
    if (checkresult != Status::OK) {
        return checkresult;
    }
    if (ret != Status::OK) {
        AVCODEC_LOGE("mp3 init error.");
        return ret;
    }
    ret = basePlugin->InitContext(parameter);
    if (ret != Status::OK) {
        AVCODEC_LOGE("mp3 init error.");
        return ret;
    }
    auto format = basePlugin->GetFormat();
    format->SetData(Tag::AUDIO_MAX_INPUT_SIZE, GetInputBufferSize());
    format->SetData(Tag::AUDIO_MAX_OUTPUT_SIZE, GetOutputBufferSize());
    return basePlugin->OpenContext();
}

Status FFmpegMp3DecoderPlugin::GetParameter(std::shared_ptr<Meta> &parameter)
{
    auto format = basePlugin->GetFormat();
    format->SetData(Tag::MIME_TYPE, MediaAVCodec::AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_MPEG);
    parameter = format;
    return Status::OK;
}

Status FFmpegMp3DecoderPlugin::QueueInputBuffer(const std::shared_ptr<AVBuffer> &inputBuffer)
{
    return basePlugin->ProcessSendData(inputBuffer);
}

Status FFmpegMp3DecoderPlugin::QueueOutputBuffer(std::shared_ptr<AVBuffer> &outputBuffer)
{
    return basePlugin->ProcessReceiveData(outputBuffer);
}

Status FFmpegMp3DecoderPlugin::GetInputBuffers(std::vector<std::shared_ptr<AVBuffer>> &inputBuffers)
{
    return Status::OK;
}

Status FFmpegMp3DecoderPlugin::GetOutputBuffers(std::vector<std::shared_ptr<AVBuffer>> &outputBuffers)
{
    return Status::OK;
}

Status FFmpegMp3DecoderPlugin::Flush()
{
    return basePlugin->Flush();
}

Status FFmpegMp3DecoderPlugin::Release()
{
    return basePlugin->Release();
}

Status FFmpegMp3DecoderPlugin::CheckInit(const std::shared_ptr<Meta> &format)
{
    format->GetData(Tag::AUDIO_CHANNEL_COUNT, channels);
    format->GetData(Tag::AUDIO_SAMPLE_RATE, sampleRate);
    if (channels < MIN_CHANNELS || channels > MAX_CHANNELS) {
        AVCODEC_LOGE("check init failed, because channel:%{public}d not support", channels);
        return Status::ERROR_INVALID_PARAMETER;
    }

    for (int32_t i = 0; i < SUPPORT_SAMPLE_RATE; i++) {
        if (sampleRate == SAMPLE_RATE_PICK[i]) {
            break;
        } else if (i == SUPPORT_SAMPLE_RATE - 1) {
            AVCODEC_LOGE("check init failed, because sampleRate:%{public}d not support", sampleRate);
            return Status::ERROR_INVALID_PARAMETER;
        }
    }
    if (!basePlugin->CheckSampleFormat(format, channels)) {
        AVCODEC_LOGE("check init failed, because CheckSampleFormat failed.");
        return Status::ERROR_INVALID_PARAMETER;
    }
    return Status::OK;
}

int32_t FFmpegMp3DecoderPlugin::GetInputBufferSize()
{
    int32_t maxSize = basePlugin->GetMaxInputSize();
    if (maxSize < 0 || maxSize > INPUT_BUFFER_SIZE_DEFAULT) {
        maxSize = INPUT_BUFFER_SIZE_DEFAULT;
    }
    return maxSize;
}

int32_t FFmpegMp3DecoderPlugin::GetOutputBufferSize()
{
    int32_t maxSize = (sampleRate / SAMPLE_RATE_RATIO + BUFFER_DIFF) * channels * sizeof(short);
    int32_t minSize = MIN_OUTBUF_SIZE * channels * sizeof(short);
    if (maxSize < minSize) {
        maxSize = minSize;
    }
    return maxSize;
}
} // namespace Ffmpeg
} // namespace Plugins
} // namespace Media
} // namespace OHOS
