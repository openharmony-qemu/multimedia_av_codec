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

#include "ffmpeg_gsm_ms_decoder_plugin.h"
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

constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_AUDIO, "AvCodec-FFmpegGsmMsDecoderPlugin"};
constexpr int32_t MIN_CHANNELS = 1;
constexpr int32_t MAX_CHANNELS = 1;
constexpr int32_t GSM_MS_INPUT_BUFFER_SIZE = 4096;
constexpr int32_t GSM_MS_OUTPUT_BUFFER_SIZE = 320 * 1 * 2;  //SAMPLES_PER_BLOCK * CHANNELS * BYTES_PER_SAMPLE(S16LE)

static const int32_t GSM_MS_DECODER_SAMPLE_RATE_TABLE[] = {8000};
} // namespace

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Ffmpeg {
FFmpegGsmMsDecoderPlugin::FFmpegGsmMsDecoderPlugin(const std::string &name)
    : CodecPlugin(name), channels(0), basePlugin(std::make_unique<FfmpegBaseDecoder>()) {}

FFmpegGsmMsDecoderPlugin::~FFmpegGsmMsDecoderPlugin()
{
    basePlugin->Release();
    basePlugin.reset();
    basePlugin = nullptr;
}

Status FFmpegGsmMsDecoderPlugin::Init()
{
    return Status::OK;
}

Status FFmpegGsmMsDecoderPlugin::Prepare()
{
    return Status::OK;
}

Status FFmpegGsmMsDecoderPlugin::Reset()
{
    return basePlugin->Reset();
}

Status FFmpegGsmMsDecoderPlugin::Start()
{
    return Status::OK;
}

Status FFmpegGsmMsDecoderPlugin::Stop()
{
    return Status::OK;
}

Status FFmpegGsmMsDecoderPlugin::SetParameter(const std::shared_ptr<Meta> &parameter)
{
    Status ret = basePlugin->AllocateContext("gsm_ms");
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

Status FFmpegGsmMsDecoderPlugin::GetParameter(std::shared_ptr<Meta> &parameter)
{
    parameter = basePlugin->GetFormat();
    return Status::OK;
}

Status FFmpegGsmMsDecoderPlugin::QueueInputBuffer(const std::shared_ptr<AVBuffer> &inputBuffer)
{
    return basePlugin->ProcessSendData(inputBuffer);
}

Status FFmpegGsmMsDecoderPlugin::QueueOutputBuffer(std::shared_ptr<AVBuffer> &outputBuffer)
{
    return basePlugin->ProcessReceiveData(outputBuffer);
}

Status FFmpegGsmMsDecoderPlugin::GetInputBuffers(std::vector<std::shared_ptr<AVBuffer>> &inputBuffers)
{
    return Status::OK;
}

Status FFmpegGsmMsDecoderPlugin::GetOutputBuffers(std::vector<std::shared_ptr<AVBuffer>> &outputBuffers)
{
    return Status::OK;
}

Status FFmpegGsmMsDecoderPlugin::Flush()
{
    return basePlugin->Flush();
}

Status FFmpegGsmMsDecoderPlugin::Release()
{
    return basePlugin->Release();
}

bool FFmpegGsmMsDecoderPlugin::CheckSampleRate(int32_t sampleRate) const noexcept
{
    bool isExist = std::any_of(std::begin(GSM_MS_DECODER_SAMPLE_RATE_TABLE), std::end(GSM_MS_DECODER_SAMPLE_RATE_TABLE),
        [sampleRate](int32_t value) { return value == sampleRate; });
    return isExist;
}

Status FFmpegGsmMsDecoderPlugin::CheckFormat(const std::shared_ptr<Meta> &format)
{
    int32_t channelCount;
    int32_t sampleRate;
    format->GetData(Tag::AUDIO_CHANNEL_COUNT, channelCount);
    format->GetData(Tag::AUDIO_SAMPLE_RATE, sampleRate);
    if (!CheckSampleRate(sampleRate)) {
        AVCODEC_LOGE("SampleRate=%{public}d not support.", sampleRate);
        return Status::ERROR_INVALID_PARAMETER;
    } else if (channelCount < MIN_CHANNELS) {
        AVCODEC_LOGE("ChannelCount=%{public}d invalid, %{public}d at least.", channelCount, MIN_CHANNELS);
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

int32_t FFmpegGsmMsDecoderPlugin::GetInputBufferSize()
{
    return GSM_MS_INPUT_BUFFER_SIZE;
}

int32_t FFmpegGsmMsDecoderPlugin::GetOutputBufferSize()
{
    return GSM_MS_OUTPUT_BUFFER_SIZE;
}

} // namespace Ffmpeg
} // namespace Plugins
} // namespace Media
} // namespace OHOS
