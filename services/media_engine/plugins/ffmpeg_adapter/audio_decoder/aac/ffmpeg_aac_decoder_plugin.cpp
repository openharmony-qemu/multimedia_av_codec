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
#include "ffmpeg_aac_decoder_plugin.h"
#include <algorithm>
#include "avcodec_codec_name.h"
#include "avcodec_log.h"
#include "plugin/codec_plugin.h"
#include "plugin/plugin_definition.h"
#include "avcodec_mime_type.h"
#include "avcodec_audio_common.h"
namespace {
using namespace OHOS::Media;
using namespace OHOS::Media::Plugins;
using namespace Ffmpeg;

constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_AUDIO, "AvCodec-FFmpegAACDecoderPlugin"};
constexpr int MIN_CHANNELS = 1;
constexpr int MAX_CHANNELS = 8;
constexpr int32_t INPUT_BUFFER_SIZE_DEFAULT = 8192;
constexpr int32_t OUTPUT_BUFFER_SIZE_DEFAULT = 4 * 1024 * 8;
static std::vector<int32_t> supportedSampleRate = {96000, 88200, 64000, 48000, 44100, 32000, 24000,
                                                   22050, 16000, 12000, 11025, 8000,  7350};
static std::vector<int32_t> supportedSampleRates = {7350,  8000,  11025, 12000, 16000, 22050, 24000,
                                                    32000, 44100, 48000, 64000, 88200, 96000};
} // namespace

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Ffmpeg {
FFmpegAACDecoderPlugin::FFmpegAACDecoderPlugin(const std::string& name)
    : CodecPlugin(name), channels_(0), basePlugin(std::make_unique<FfmpegBaseDecoder>())
{
}

FFmpegAACDecoderPlugin::~FFmpegAACDecoderPlugin()
{
    basePlugin->Release();
    basePlugin.reset();
    basePlugin = nullptr;
}

Status FFmpegAACDecoderPlugin::Init()
{
    return Status::OK;
}

Status FFmpegAACDecoderPlugin::Prepare()
{
    return Status::OK;
}

Status FFmpegAACDecoderPlugin::Reset()
{
    return basePlugin->Reset();
}

Status FFmpegAACDecoderPlugin::Start()
{
    return Status::OK;
}

Status FFmpegAACDecoderPlugin::Stop()
{
    return Status::OK;
}

Status FFmpegAACDecoderPlugin::SetParameter(const std::shared_ptr<Meta> &parameter)
{
    if (!CheckFormat(parameter)) {
        return Status::ERROR_INVALID_PARAMETER;
    }
    Status ret = basePlugin->AllocateContext(aacName_);
    if (ret != Status::OK) {
        AVCODEC_LOGE("AllocateContext failed, ret=%{public}d", ret);
        return ret;
    }
    ret = basePlugin->InitContext(parameter);
    if (ret != Status::OK) {
        AVCODEC_LOGE("InitContext failed, ret=%{public}d", ret);
        return ret;
    }
    auto format = basePlugin->GetFormat();
    format->SetData(Tag::AUDIO_MAX_INPUT_SIZE, GetInputBufferSize());
    format->SetData(Tag::AUDIO_MAX_OUTPUT_SIZE, GetOutputBufferSize());
    format->SetData(Tag::MIME_TYPE, MediaAVCodec::AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_AAC);
    return basePlugin->OpenContext();
}

Status FFmpegAACDecoderPlugin::GetParameter(std::shared_ptr<Meta> &parameter)
{
    parameter = basePlugin->GetFormat();
    return Status::OK;
}

Status FFmpegAACDecoderPlugin::QueueInputBuffer(const std::shared_ptr<AVBuffer> &inputBuffer)
{
    return basePlugin->ProcessSendData(inputBuffer);
}

Status FFmpegAACDecoderPlugin::QueueOutputBuffer(std::shared_ptr<AVBuffer> &outputBuffer)
{
    return basePlugin->ProcessReceiveData(outputBuffer);
}

Status FFmpegAACDecoderPlugin::GetInputBuffers(std::vector<std::shared_ptr<AVBuffer>> &inputBuffers)
{
    return Status::OK;
}

Status FFmpegAACDecoderPlugin::GetOutputBuffers(std::vector<std::shared_ptr<AVBuffer>> &outputBuffers)
{
    return Status::OK;
}

Status FFmpegAACDecoderPlugin::Flush()
{
    return basePlugin->Flush();
}

Status FFmpegAACDecoderPlugin::Release()
{
    return basePlugin->Release();
}

bool FFmpegAACDecoderPlugin::CheckAdts(const std::shared_ptr<Meta> &format)
{
    int type;
    if (format->GetData(Tag::AUDIO_AAC_IS_ADTS, type)) {
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

bool FFmpegAACDecoderPlugin::CheckSampleFormat(const std::shared_ptr<Meta> &format)
{
    return basePlugin->CheckSampleFormat(format, channels_);
}

bool FFmpegAACDecoderPlugin::CheckFormat(const std::shared_ptr<Meta> &format)
{
    if (!CheckAdts(format) || !CheckChannelCount(format) || !CheckSampleFormat(format) || !CheckSampleRate(format)) {
        return false;
    }
    return true;
}

bool FFmpegAACDecoderPlugin::CheckChannelCount(const std::shared_ptr<Meta> &format)
{
    if (!format->GetData(Tag::AUDIO_CHANNEL_COUNT, channels_)) {
        AVCODEC_LOGE("parameter channel_count missing");
        return false;
    }
    if (channels_ < MIN_CHANNELS || channels_ > MAX_CHANNELS) {
        AVCODEC_LOGE("parameter channel_count invaild");
        return false;
    }
    return true;
}

bool FFmpegAACDecoderPlugin::CheckSampleRate(const std::shared_ptr<Meta> &format) const
{
    int32_t sampleRate;
    if (!format->GetData(Tag::AUDIO_SAMPLE_RATE, sampleRate)) {
        AVCODEC_LOGE("parameter sample_rate missing");
        return false;
    }

    if (std::find(supportedSampleRates.begin(), supportedSampleRates.end(), sampleRate) == supportedSampleRates.end()) {
        AVCODEC_LOGE("parameter sample_rate invalid, %{public}d", sampleRate);
        return false;
    }
    return true;
}

int32_t FFmpegAACDecoderPlugin::GetInputBufferSize()
{
    int32_t maxSize = basePlugin->GetMaxInputSize();
    if (maxSize < 0 || maxSize > INPUT_BUFFER_SIZE_DEFAULT) {
        maxSize = INPUT_BUFFER_SIZE_DEFAULT;
    }
    return maxSize;
}

int32_t FFmpegAACDecoderPlugin::GetOutputBufferSize()
{
    return OUTPUT_BUFFER_SIZE_DEFAULT;
}
} // namespace Ffmpeg
} // namespace Plugins
} // namespace Media
} // namespace OHOS
