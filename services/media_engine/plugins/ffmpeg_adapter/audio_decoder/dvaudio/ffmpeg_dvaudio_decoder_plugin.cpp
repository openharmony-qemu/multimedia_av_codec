/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#include "ffmpeg_dvaudio_decoder_plugin.h"
#include "avcodec_codec_name.h"
#include "avcodec_log.h"
#include "plugin/codec_plugin.h"
#include "plugin/plugin_definition.h"
#include "avcodec_mime_type.h"

namespace {
using namespace OHOS::Media;
using namespace OHOS::Media::Plugins;
using namespace Ffmpeg;

constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_AUDIO, "AvCodec-AudioFFMpegDvaudioDecoderPlugin"};
constexpr int32_t CHANNELS = 2;
constexpr int32_t SUPPORT_SAMPLE_RATE = 48000;
constexpr int32_t INPUT_BUFFER_SIZE_DEFAULT = 8640;
constexpr int32_t OUTPUT_BUFFER_SIZE_DEFAULT = 4 * 1024 * 8;
constexpr int32_t BLOCK_ALIGN = 8640;
} // namespace

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Ffmpeg {
FFmpegDvaudioDecoderPlugin::FFmpegDvaudioDecoderPlugin(const std::string& name)
    : CodecPlugin(name), channels(0), sampleRate(0), basePlugin(std::make_unique<FfmpegBaseDecoder>())
{
}

FFmpegDvaudioDecoderPlugin::~FFmpegDvaudioDecoderPlugin()
{
    basePlugin->Release();
    basePlugin.reset();
    basePlugin = nullptr;
}

Status FFmpegDvaudioDecoderPlugin::Init()
{
    return Status::OK;
}

Status FFmpegDvaudioDecoderPlugin::Prepare()
{
    return Status::OK;
}

Status FFmpegDvaudioDecoderPlugin::Reset()
{
    return basePlugin->Reset();
}

Status FFmpegDvaudioDecoderPlugin::Start()
{
    return Status::OK;
}

Status FFmpegDvaudioDecoderPlugin::Stop()
{
    return Status::OK;
}

Status FFmpegDvaudioDecoderPlugin::SetParameter(const std::shared_ptr<Meta> &parameter)
{
    Status ret = basePlugin->AllocateContext("dvaudio");
    Status checkresult = CheckInit(parameter);
    if (checkresult != Status::OK) {
        return checkresult;
    }
    if (ret != Status::OK) {
        AVCODEC_LOGE("dvaudio init error.");
        return ret;
    }
    ret = basePlugin->InitContext(parameter);
    if (ret != Status::OK) {
        AVCODEC_LOGE("dvaudio init error.");
        return ret;
    }
    basePlugin->SetBlockAlignContext(BLOCK_ALIGN);
    auto format = basePlugin->GetFormat();
    format->SetData(Tag::AUDIO_MAX_INPUT_SIZE, GetInputBufferSize());
    format->SetData(Tag::AUDIO_MAX_OUTPUT_SIZE, GetOutputBufferSize());
    return basePlugin->OpenContext();
}

Status FFmpegDvaudioDecoderPlugin::GetParameter(std::shared_ptr<Meta> &parameter)
{
    auto format = basePlugin->GetFormat();
    format->SetData(Tag::MIME_TYPE, MediaAVCodec::AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_DVAUDIO);
    parameter = format;
    return Status::OK;
}

Status FFmpegDvaudioDecoderPlugin::QueueInputBuffer(const std::shared_ptr<AVBuffer> &inputBuffer)
{
    return basePlugin->ProcessSendData(inputBuffer);
}

Status FFmpegDvaudioDecoderPlugin::QueueOutputBuffer(std::shared_ptr<AVBuffer> &outputBuffer)
{
    return basePlugin->ProcessReceiveData(outputBuffer);
}

Status FFmpegDvaudioDecoderPlugin::GetInputBuffers(std::vector<std::shared_ptr<AVBuffer>> &inputBuffers)
{
    return Status::OK;
}

Status FFmpegDvaudioDecoderPlugin::GetOutputBuffers(std::vector<std::shared_ptr<AVBuffer>> &outputBuffers)
{
    return Status::OK;
}

Status FFmpegDvaudioDecoderPlugin::Flush()
{
    return basePlugin->Flush();
}

Status FFmpegDvaudioDecoderPlugin::Release()
{
    return basePlugin->Release();
}

Status FFmpegDvaudioDecoderPlugin::CheckInit(const std::shared_ptr<Meta> &format)
{
    format->GetData(Tag::AUDIO_CHANNEL_COUNT, channels);
    format->GetData(Tag::AUDIO_SAMPLE_RATE, sampleRate);
    if (channels != CHANNELS) {
        AVCODEC_LOGE("check init failed, because channel:%{public}d not support", channels);
        return Status::ERROR_INVALID_PARAMETER;
    }

    if (sampleRate != SUPPORT_SAMPLE_RATE) {
        AVCODEC_LOGE("check init failed, because sampleRate:%{public}d not support", sampleRate);
        return Status::ERROR_INVALID_PARAMETER;
    }

    if (!basePlugin->CheckSampleFormat(format, channels)) {
        AVCODEC_LOGE("check init failed, because CheckSampleFormat failed.");
        return Status::ERROR_INVALID_PARAMETER;
    }
    return Status::OK;
}

int32_t FFmpegDvaudioDecoderPlugin::GetInputBufferSize()
{
    int32_t maxSize = basePlugin->GetMaxInputSize();
    if (maxSize < 0 || maxSize > INPUT_BUFFER_SIZE_DEFAULT) {
        maxSize = INPUT_BUFFER_SIZE_DEFAULT;
    }
    return maxSize;
}

int32_t FFmpegDvaudioDecoderPlugin::GetOutputBufferSize()
{
    return OUTPUT_BUFFER_SIZE_DEFAULT;
}
} // namespace Ffmpeg
} // namespace Plugins
} // namespace Media
} // namespace OHOS
