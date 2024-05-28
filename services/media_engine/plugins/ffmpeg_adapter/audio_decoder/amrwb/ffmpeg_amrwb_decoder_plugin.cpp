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
#include "ffmpeg_amrwb_decoder_plugin.h"
#include "avcodec_codec_name.h"
#include "avcodec_log.h"
#include "plugin/codec_plugin.h"
#include "plugin/plugin_definition.h"
#include "avcodec_mime_type.h"

namespace {
using namespace OHOS::Media;
using namespace OHOS::Media::Plugins;
using namespace Ffmpeg;

constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_AUDIO, "AvCodec-AudioFFMpegAmrWbDecoderPlugin"};
constexpr int32_t MIN_CHANNELS = 1;
constexpr int32_t MAX_CHANNELS = 1;
constexpr int32_t SUPPORT_SAMPLE_RATE = 1;
constexpr int32_t MIN_OUTBUF_SIZE = 1280;
constexpr int32_t INPUT_BUFFER_SIZE_DEFAULT = 150;
constexpr int32_t SAMPLE_RATE_PICK[SUPPORT_SAMPLE_RATE] = {16000};

} // namespace

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Ffmpeg {
FFmpegAmrWbDecoderPlugin::FFmpegAmrWbDecoderPlugin(const std::string& name)
    : CodecPlugin(name), channels(0), sampleRate(0), basePlugin(std::make_unique<FfmpegBaseDecoder>())
{
}

FFmpegAmrWbDecoderPlugin::~FFmpegAmrWbDecoderPlugin()
{
    basePlugin->Release();
    basePlugin.reset();
    basePlugin = nullptr;
}

Status FFmpegAmrWbDecoderPlugin::Init()
{
    return Status::OK;
}

Status FFmpegAmrWbDecoderPlugin::Prepare()
{
    return Status::OK;
}

Status FFmpegAmrWbDecoderPlugin::Reset()
{
    return basePlugin->Reset();
}

Status FFmpegAmrWbDecoderPlugin::Start()
{
    return Status::OK;
}

Status FFmpegAmrWbDecoderPlugin::Stop()
{
    return Status::OK;
}

Status FFmpegAmrWbDecoderPlugin::SetParameter(const std::shared_ptr<Meta> &parameter)
{
    Status ret = basePlugin->AllocateContext("amrwb");
    Status checkresult = CheckInit(parameter);
    if (checkresult != Status::OK) {
        return checkresult;
    }
    if (ret != Status::OK) {
        AVCODEC_LOGE("amrwb init error.");
        return ret;
    }
    ret = basePlugin->InitContext(parameter);
    if (ret != Status::OK) {
        AVCODEC_LOGE("amrwb init error.");
        return ret;
    }
    auto format = basePlugin->GetFormat();
    format->SetData(Tag::AUDIO_MAX_INPUT_SIZE, GetInputBufferSize());
    format->SetData(Tag::AUDIO_MAX_OUTPUT_SIZE, GetOutputBufferSize());
    ret = basePlugin->OpenContext();
    basePlugin->DisableNeedResample();
    return ret;
}

Status FFmpegAmrWbDecoderPlugin::GetParameter(std::shared_ptr<Meta> &parameter)
{
    auto format = basePlugin->GetFormat();
    format->SetData(Tag::MIME_TYPE, MediaAVCodec::AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_AMRWB);
    parameter = format;
    return Status::OK;
}

Status FFmpegAmrWbDecoderPlugin::QueueInputBuffer(const std::shared_ptr<AVBuffer> &inputBuffer)
{
    return basePlugin->ProcessSendData(inputBuffer);
}

Status FFmpegAmrWbDecoderPlugin::QueueOutputBuffer(std::shared_ptr<AVBuffer> &outputBuffer)
{
    return basePlugin->ProcessReceiveData(outputBuffer);
}

Status FFmpegAmrWbDecoderPlugin::GetInputBuffers(std::vector<std::shared_ptr<AVBuffer>> &inputBuffers)
{
    return Status::OK;
}

Status FFmpegAmrWbDecoderPlugin::GetOutputBuffers(std::vector<std::shared_ptr<AVBuffer>> &outputBuffers)
{
    return Status::OK;
}

Status FFmpegAmrWbDecoderPlugin::Flush()
{
    return basePlugin->Flush();
}

Status FFmpegAmrWbDecoderPlugin::Release()
{
    return basePlugin->Release();
}

Status FFmpegAmrWbDecoderPlugin::CheckInit(const std::shared_ptr<Meta> &format)
{
    format->GetData(Tag::AUDIO_CHANNEL_COUNT, channels);
    format->GetData(Tag::AUDIO_SAMPLE_RATE, sampleRate);
    if (channels < MIN_CHANNELS || channels > MAX_CHANNELS) {
        return Status::ERROR_INVALID_PARAMETER;
    }

    for (int32_t i = 0; i < SUPPORT_SAMPLE_RATE; i++) {
        if (sampleRate == SAMPLE_RATE_PICK[i]) {
            break;
        } else if (i == SUPPORT_SAMPLE_RATE - 1) {
            return Status::ERROR_INVALID_PARAMETER;
        }
    }
    if (!basePlugin->CheckSampleFormat(format, channels)) {
        return Status::ERROR_INVALID_PARAMETER;
    }
    return Status::OK;
}

int32_t FFmpegAmrWbDecoderPlugin::GetInputBufferSize()
{
    int32_t maxSize = basePlugin->GetMaxInputSize();
    if (maxSize < 0 || maxSize > INPUT_BUFFER_SIZE_DEFAULT) {
        maxSize = INPUT_BUFFER_SIZE_DEFAULT;
    }
    return maxSize;
}

int32_t FFmpegAmrWbDecoderPlugin::GetOutputBufferSize()
{
    return MIN_OUTBUF_SIZE;
}
} // namespace Ffmpeg
} // namespace Plugins
} // namespace Media
} // namespace OHOS
