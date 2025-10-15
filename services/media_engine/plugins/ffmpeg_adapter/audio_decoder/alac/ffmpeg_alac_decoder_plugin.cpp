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

#include "ffmpeg_alac_decoder_plugin.h"
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

constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_AUDIO, "AvCodec-FFmpegAlacDecoderPlugin"};
constexpr int32_t MIN_CHANNELS = 1;
constexpr int32_t MAX_CHANNELS = 8;
constexpr int32_t MAX_BYTES_PER_SAMPLE = 4;
constexpr int32_t SAMPLES_PER_FRAME = 4096;
constexpr int32_t INPUT_BUFFER_SIZE_DEFAULT = 16384;
constexpr int32_t RANK_SIZE = 100;

static const int32_t ALAC_DECODER_SAMPLE_RATE_TABLE[] = {
    8000,  11025, 12000, 16000, 22050, 24000,
    32000, 44100, 48000, 88200, 96000, 176400, 192000
};

Status RegisterAudioDecoderPlugins(const std::shared_ptr<Register> &reg)
{
    CodecPluginDef definition;
    definition.name = std::string(OHOS::MediaAVCodec::AVCodecCodecName::AUDIO_DECODER_ALAC_NAME);
    definition.pluginType = PluginType::AUDIO_DECODER;
    definition.rank = RANK_SIZE;
    definition.SetCreator([](const std::string &name) -> std::shared_ptr<CodecPlugin> {
        return std::make_shared<FFmpegAlacDecoderPlugin>(name);
    });
    Capability cap;
    cap.SetMime(MimeType::AUDIO_ALAC);
    cap.AppendFixedKey<CodecMode>(Tag::MEDIA_CODEC_MODE, CodecMode::SOFTWARE);
    definition.AddInCaps(cap);
    if (reg->AddPlugin(definition) != Status::OK) {
        AVCODEC_LOGE("AudioALACDecoderPlugin Register Failure");
        return Status::ERROR_UNKNOWN;
    }
    return Status::OK;
}

void UnRegisterAudioDecoderPlugin() {}

PLUGIN_DEFINITION(AlacAudioDecoder, LicenseType::LGPL, RegisterAudioDecoderPlugins, UnRegisterAudioDecoderPlugin);
} // namespace

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Ffmpeg {

FFmpegAlacDecoderPlugin::FFmpegAlacDecoderPlugin(const std::string &name)
    : CodecPlugin(name), channels(0), basePlugin(std::make_unique<FfmpegBaseDecoder>()) {}

FFmpegAlacDecoderPlugin::~FFmpegAlacDecoderPlugin()
{
    basePlugin->Release();
    basePlugin.reset();
    basePlugin = nullptr;
}

Status FFmpegAlacDecoderPlugin::Init()
{
    return Status::OK;
}

Status FFmpegAlacDecoderPlugin::Prepare()
{
    return Status::OK;
}

Status FFmpegAlacDecoderPlugin::Reset()
{
    return basePlugin->Reset();
}

Status FFmpegAlacDecoderPlugin::Start()
{
    AVCODEC_LOGW("ALACPlugin::Start");
    return Status::OK;
}

Status FFmpegAlacDecoderPlugin::Stop()
{
    return Status::OK;
}

Status FFmpegAlacDecoderPlugin::SetParameter(const std::shared_ptr<Meta> &parameter)
{
    Status ret = basePlugin->AllocateContext("alac");
    Status checkResult = CheckFormat(parameter);
    if (checkResult != Status::OK) {
        return checkResult;
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
        AVCODEC_LOGE("OpenContext failed, ret=%{public}d", ret);
        return ret;
    }
    auto format = basePlugin->GetFormat();
    format->SetData(Tag::AUDIO_MAX_INPUT_SIZE, GetInputBufferSize());
    format->SetData(Tag::AUDIO_MAX_OUTPUT_SIZE, GetOutputBufferSize());
    return Status::OK;
}

Status FFmpegAlacDecoderPlugin::GetParameter(std::shared_ptr<Meta> &parameter)
{
    parameter = basePlugin->GetFormat();
    return Status::OK;
}

Status FFmpegAlacDecoderPlugin::QueueInputBuffer(const std::shared_ptr<AVBuffer> &inputBuffer)
{
    return basePlugin->ProcessSendData(inputBuffer);
}

Status FFmpegAlacDecoderPlugin::QueueOutputBuffer(std::shared_ptr<AVBuffer> &outputBuffer)
{
    return basePlugin->ProcessReceiveData(outputBuffer);
}

Status FFmpegAlacDecoderPlugin::GetInputBuffers(std::vector<std::shared_ptr<AVBuffer>> &inputBuffers)
{
    return Status::OK;
}

Status FFmpegAlacDecoderPlugin::GetOutputBuffers(std::vector<std::shared_ptr<AVBuffer>> &outputBuffers)
{
    return Status::OK;
}

Status FFmpegAlacDecoderPlugin::Flush()
{
    return basePlugin->Flush();
}

Status FFmpegAlacDecoderPlugin::Release()
{
    return basePlugin->Release();
}

bool FFmpegAlacDecoderPlugin::CheckSampleRate(int32_t sampleRate) const noexcept
{
    return std::any_of(std::begin(ALAC_DECODER_SAMPLE_RATE_TABLE), std::end(ALAC_DECODER_SAMPLE_RATE_TABLE),
        [sampleRate](int32_t value) { return value == sampleRate; });
}

Status FFmpegAlacDecoderPlugin::CheckFormat(const std::shared_ptr<Meta> &format)
{
    int32_t channelCount;
    int32_t sampleRate;
    format->GetData(Tag::AUDIO_CHANNEL_COUNT, channelCount);
    format->GetData(Tag::AUDIO_SAMPLE_RATE, sampleRate);
    if (!CheckSampleRate(sampleRate)) {
        AVCODEC_LOGE("SampleRate=%{public}d not support.", sampleRate);
        return Status::ERROR_INVALID_PARAMETER;
    } else if (channelCount < MIN_CHANNELS || channelCount > MAX_CHANNELS) {
        AVCODEC_LOGE("ChannelCount=%{public}d invalid, range [%{public}d, %{public}d]",
            channelCount, MIN_CHANNELS, MAX_CHANNELS);
        return Status::ERROR_INVALID_PARAMETER;
    }

    if (!basePlugin->CheckSampleFormat(format, channelCount)) {
        AVCODEC_LOGE("CheckSampleFormat failed.");
        return Status::ERROR_INVALID_PARAMETER;
    }
    channels = channelCount;
    return Status::OK;
}

int32_t FFmpegAlacDecoderPlugin::GetInputBufferSize()
{
    int32_t inputBufferSize = SAMPLES_PER_FRAME * MAX_CHANNELS * MAX_BYTES_PER_SAMPLE;
    int32_t maxSize = basePlugin->GetMaxInputSize();
    if (maxSize < 0 || maxSize > inputBufferSize) {
        maxSize = std::max(inputBufferSize, INPUT_BUFFER_SIZE_DEFAULT);
    }
    return maxSize;
}

int32_t FFmpegAlacDecoderPlugin::GetOutputBufferSize()
{
    return SAMPLES_PER_FRAME * MAX_CHANNELS * MAX_BYTES_PER_SAMPLE;
}
} // namespace Ffmpeg
} // namespace Plugins
} // namespace Media
} // namespace OHOS