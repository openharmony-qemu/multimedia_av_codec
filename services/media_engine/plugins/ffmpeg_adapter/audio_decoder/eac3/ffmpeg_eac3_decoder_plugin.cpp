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

#include "ffmpeg_eac3_decoder_plugin.h"
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

constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_AUDIO, "AvCodec-FFmpegEAC3DecoderPlugin"};
constexpr int32_t MIN_CHANNELS = 1;
constexpr int32_t MAX_CHANNELS = 16;
constexpr int32_t MAX_BYTES_PER_SAMPLE = 4;
constexpr int32_t SAMPLES = 1536;

static const int32_t EAC3_DECODER_SAMPLE_RATE_TABLE[] = {16000, 22050, 24000, 32000, 44100, 48000};

Status RegisterAudioDecoderPlugins(const std::shared_ptr<Register> &reg)
{
    CodecPluginDef definition;
    definition.name = std::string(OHOS::MediaAVCodec::AVCodecCodecName::AUDIO_DECODER_EAC3_NAME);
    definition.pluginType = PluginType::AUDIO_DECODER;
    definition.rank = 100; // 100
    definition.SetCreator([](const std::string &name) -> std::shared_ptr<CodecPlugin> {
        return std::make_shared<FFmpegEAC3DecoderPlugin>(name);
    });
    Capability cap;
    cap.SetMime(MimeType::AUDIO_EAC3);
    cap.AppendFixedKey<CodecMode>(Tag::MEDIA_CODEC_MODE, CodecMode::SOFTWARE);
    definition.AddInCaps(cap);
    if (reg->AddPlugin(definition) != Status::OK) {
        AVCODEC_LOGE("AudioEAC3DecoderPlugin Register Failure");
        return Status::ERROR_UNKNOWN;
    }
    return Status::OK;
}

void UnRegisterAudioDecoderPlugin() {}

PLUGIN_DEFINITION(Eac3AudioDecoder, LicenseType::LGPL, RegisterAudioDecoderPlugins, UnRegisterAudioDecoderPlugin);
} // namespace

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Ffmpeg {
FFmpegEAC3DecoderPlugin::FFmpegEAC3DecoderPlugin(const std::string &name)
    : CodecPlugin(name), channels(0), basePlugin(std::make_unique<FfmpegBaseDecoder>()) {}

FFmpegEAC3DecoderPlugin::~FFmpegEAC3DecoderPlugin()
{
    basePlugin->Release();
    basePlugin.reset();
    basePlugin = nullptr;
}

Status FFmpegEAC3DecoderPlugin::Init()
{
    return Status::OK;
}

Status FFmpegEAC3DecoderPlugin::Prepare()
{
    return Status::OK;
}

Status FFmpegEAC3DecoderPlugin::Reset()
{
    return basePlugin->Reset();
}

Status FFmpegEAC3DecoderPlugin::Start()
{
    AVCODEC_LOGW("EAC3pLug::Start");
    return Status::OK;
}

Status FFmpegEAC3DecoderPlugin::Stop()
{
    return Status::OK;
}

Status FFmpegEAC3DecoderPlugin::SetParameter(const std::shared_ptr<Meta> &parameter)
{
    Status ret = basePlugin->AllocateContext("eac3");
    Status checkresult = CheckFormat(parameter);
    if (checkresult != Status::OK) {
        return checkresult;
    }
    if (ret != Status::OK) {
        AVCODEC_LOGE("AllocateContext failed, ret=%{public}d", ret);
        return ret;
    }
    ret = basePlugin->InitContext(parameter);
    if (ret != Status::OK) {
        AVCODEC_LOGE("InitContext failed, ret=%{public}d", ret);
        return ret;
    }
    ret = basePlugin->OpenContext();
    if (ret != Status::OK) {
        AVCODEC_LOGE("OpenContext failed. ret=%{public}d", ret);
        return ret;
    }
    auto format = basePlugin->GetFormat();
    format->SetData(Tag::AUDIO_MAX_INPUT_SIZE, GetInputBufferSize());
    format->SetData(Tag::AUDIO_MAX_OUTPUT_SIZE, GetOutputBufferSize());
    return Status::OK;
}

Status FFmpegEAC3DecoderPlugin::GetParameter(std::shared_ptr<Meta> &parameter)
{
    parameter = basePlugin->GetFormat();
    return Status::OK;
}

Status FFmpegEAC3DecoderPlugin::QueueInputBuffer(const std::shared_ptr<AVBuffer> &inputBuffer)
{
    return basePlugin->ProcessSendData(inputBuffer);
}

Status FFmpegEAC3DecoderPlugin::QueueOutputBuffer(std::shared_ptr<AVBuffer> &outputBuffer)
{
    return basePlugin->ProcessReceiveData(outputBuffer);
}

Status FFmpegEAC3DecoderPlugin::GetInputBuffers(std::vector<std::shared_ptr<AVBuffer>> &inputBuffers)
{
    return Status::OK;
}

Status FFmpegEAC3DecoderPlugin::GetOutputBuffers(std::vector<std::shared_ptr<AVBuffer>> &outputBuffers)
{
    return Status::OK;
}

Status FFmpegEAC3DecoderPlugin::Flush()
{
    return basePlugin->Flush();
}

Status FFmpegEAC3DecoderPlugin::Release()
{
    return basePlugin->Release();
}

bool FFmpegEAC3DecoderPlugin::CheckSampleRate(int32_t sampleRate) const noexcept
{
    bool isExist = std::any_of(std::begin(EAC3_DECODER_SAMPLE_RATE_TABLE), std::end(EAC3_DECODER_SAMPLE_RATE_TABLE),
        [sampleRate](int32_t value) { return value == sampleRate; });
    return isExist;
}

Status FFmpegEAC3DecoderPlugin::CheckFormat(const std::shared_ptr<Meta> &format)
{
    int32_t channelCount;
    int32_t sampleRate;
    format->GetData(Tag::AUDIO_CHANNEL_COUNT, channelCount);
    format->GetData(Tag::AUDIO_SAMPLE_RATE, sampleRate);
    if (!CheckSampleRate(sampleRate)) {
        AVCODEC_LOGE("SampleRate=%{public}d not support.", sampleRate);
        return Status::ERROR_INVALID_PARAMETER;
    } else if (channelCount < MIN_CHANNELS) {
        AVCODEC_LOGE("ChannelCount=%{public}d invalid, less than 0.", channelCount);
        return Status::ERROR_INVALID_PARAMETER;
    } else if (channelCount > MAX_CHANNELS) {
        AVCODEC_LOGE("ChannelCount=%{public}d invalid, %{public}d at most.", channelCount, MAX_CHANNELS);
        return Status::ERROR_INVALID_PARAMETER;
    }
    if (!basePlugin->CheckSampleFormat(format, channelCount)) {
        AVCODEC_LOGE("CheckSampleFormat failed.");
        return Status::ERROR_INVALID_PARAMETER;
    }
    channels = channelCount;
    return Status::OK;
}

int32_t FFmpegEAC3DecoderPlugin::GetInputBufferSize()
{
    int32_t inputBufferSize = SAMPLES * MAX_CHANNELS * MAX_BYTES_PER_SAMPLE;
    int32_t maxSize = basePlugin->GetMaxInputSize();
    if (maxSize < 0 || maxSize > inputBufferSize) {
        maxSize = inputBufferSize;
    }
    return maxSize;
}

int32_t FFmpegEAC3DecoderPlugin::GetOutputBufferSize()
{
    int32_t outputBufferSize = SAMPLES * MAX_CHANNELS * MAX_BYTES_PER_SAMPLE;
    return outputBufferSize;
}
} // namespace Ffmpeg
} // namespace Plugins
} // namespace Media
} // namespace OHOS
