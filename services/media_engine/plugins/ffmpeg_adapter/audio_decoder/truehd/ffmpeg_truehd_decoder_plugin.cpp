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

#include "ffmpeg_truehd_decoder_plugin.h"
#include "avcodec_audio_common.h"
#include "avcodec_codec_name.h"
#include "avcodec_log.h"
#include "avcodec_mime_type.h"
#include "plugin/codec_plugin.h"
#include "plugin/plugin_definition.h"
#include <algorithm>
namespace {
using namespace OHOS::Media;
using namespace OHOS::Media::Plugins;
using namespace Ffmpeg;

constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_AUDIO, "AvCodec-FFmpegTruehdDecoderPlugin"};
constexpr int32_t MIN_CHANNELS = 1;
constexpr int32_t MAX_CHANNELS = 8;
constexpr int32_t MAX_BYTES_PER_SAMPLE = 4;
constexpr int32_t SAMPLES = 7680;

static const int32_t TRUEHD_DECODER_SAMPLE_RATE_TABLE[] = {44100, 48000, 88200, 96000, 176400, 192000};
} // namespace

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Ffmpeg {
FFmpegTruehdDecoderPlugin::FFmpegTruehdDecoderPlugin(const std::string &name)
    : CodecPlugin(name), channels(0), basePlugin(std::make_unique<FfmpegBaseDecoder>())
{
}

FFmpegTruehdDecoderPlugin::~FFmpegTruehdDecoderPlugin()
{
    basePlugin->Release();
    basePlugin.reset();
    basePlugin = nullptr;
}

Status FFmpegTruehdDecoderPlugin::Init()
{
    return Status::OK;
}

Status FFmpegTruehdDecoderPlugin::Prepare()
{
    return Status::OK;
}

Status FFmpegTruehdDecoderPlugin::Reset()
{
    return basePlugin->Reset();
}

Status FFmpegTruehdDecoderPlugin::Start()
{
    return Status::OK;
}

Status FFmpegTruehdDecoderPlugin::Stop()
{
    return Status::OK;
}

Status FFmpegTruehdDecoderPlugin::SetParameter(const std::shared_ptr<Meta> &parameter)
{
    Status ret = basePlugin->AllocateContext("truehd");
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

Status FFmpegTruehdDecoderPlugin::GetParameter(std::shared_ptr<Meta> &parameter)
{
    parameter = basePlugin->GetFormat();
    return Status::OK;
}

Status FFmpegTruehdDecoderPlugin::QueueInputBuffer(const std::shared_ptr<AVBuffer> &inputBuffer)
{
    return basePlugin->ProcessSendData(inputBuffer);
}

Status FFmpegTruehdDecoderPlugin::QueueOutputBuffer(std::shared_ptr<AVBuffer> &outputBuffer)
{
    return basePlugin->ProcessReceiveData(outputBuffer);
}

Status FFmpegTruehdDecoderPlugin::GetInputBuffers(std::vector<std::shared_ptr<AVBuffer>> &inputBuffers)
{
    return Status::OK;
}

Status FFmpegTruehdDecoderPlugin::GetOutputBuffers(std::vector<std::shared_ptr<AVBuffer>> &outputBuffers)
{
    return Status::OK;
}

Status FFmpegTruehdDecoderPlugin::Flush()
{
    return basePlugin->Flush();
}

Status FFmpegTruehdDecoderPlugin::Release()
{
    return basePlugin->Release();
}

bool FFmpegTruehdDecoderPlugin::CheckSampleRate(int32_t sampleRate) const noexcept
{
    bool isExist = std::any_of(std::begin(TRUEHD_DECODER_SAMPLE_RATE_TABLE),
        std::end(TRUEHD_DECODER_SAMPLE_RATE_TABLE), [sampleRate](int32_t value) { return value == sampleRate; });
    return isExist;
}

Status FFmpegTruehdDecoderPlugin::CheckFormat(const std::shared_ptr<Meta> &format)
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

int32_t FFmpegTruehdDecoderPlugin::GetInputBufferSize()
{
    int32_t inputBufferSize = SAMPLES * MAX_CHANNELS * MAX_BYTES_PER_SAMPLE;
    int32_t maxSize = basePlugin->GetMaxInputSize();
    if (maxSize < 0 || maxSize > inputBufferSize) {
        maxSize = inputBufferSize;
    }
    return maxSize;
}

int32_t FFmpegTruehdDecoderPlugin::GetOutputBufferSize()
{
    int32_t outputBufferSize = SAMPLES * MAX_CHANNELS * MAX_BYTES_PER_SAMPLE;
    return outputBufferSize;
}

} // namespace Ffmpeg
} // namespace Plugins
} // namespace Media
} // namespace OHOS
