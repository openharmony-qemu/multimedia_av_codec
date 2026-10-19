/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#include "ffmpeg_dsd_decoder_plugin.h"
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
using namespace OHOS::Media::Plugins::Ffmpeg;

constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_AUDIO, "AvCodec-FFmpegDSDDecoderPlugin"};
// DSD containers (DSF/DFF) support up to 6 channels, see libavformat/iff.c dsd_layout.
constexpr int32_t MIN_CHANNELS = 1;
constexpr int32_t MAX_CHANNELS = 6;
constexpr int32_t MAX_BYTES_PER_SAMPLE = 4;
// One PCM sample is produced per DSD byte (8 DSD bits) per channel. The DSF demuxer uses a
// block_align of 4096 bytes per channel, see libavformat/dsfdec.c.
constexpr int32_t SAMPLES = 4096;
// Values of OH_AVCodec_DSDCodecType, defined in interfaces/kits/c/native_avcodec_audiobase.h.
constexpr int32_t DSD_CODEC_LSBF_PLANAR = 0;
constexpr int32_t DSD_CODEC_MSBF_PLANAR = 1;
constexpr int32_t DSD_CODEC_MSBF = 2;
} // namespace

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Ffmpeg {
FFmpegDsdDecoderPlugin::FFmpegDsdDecoderPlugin(const std::string& name)
    : CodecPlugin(name), channels(0), basePlugin(std::make_unique<FfmpegBaseDecoder>())
{
}

FFmpegDsdDecoderPlugin::~FFmpegDsdDecoderPlugin()
{
    basePlugin->Release();
    basePlugin.reset();
    basePlugin = nullptr;
}

Status FFmpegDsdDecoderPlugin::Init()
{
    return Status::OK;
}

Status FFmpegDsdDecoderPlugin::Prepare()
{
    return Status::OK;
}

Status FFmpegDsdDecoderPlugin::Reset()
{
    return basePlugin->Reset();
}

Status FFmpegDsdDecoderPlugin::Start()
{
    return Status::OK;
}

Status FFmpegDsdDecoderPlugin::Stop()
{
    return Status::OK;
}

Status FFmpegDsdDecoderPlugin::SetParameter(const std::shared_ptr<Meta> &parameter)
{
    std::string decoderName = GetDecoderName(parameter);
    Status ret = basePlugin->AllocateContext(decoderName);
    Status checkresult = CheckFormat(parameter);
    AVCODEC_LOGI("decoderName: %{public}s", decoderName.data());
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
        AVCODEC_LOGE("init failed, because OpenContext failed. ret=%{public}d", ret);
        return ret;
    }
    auto format = basePlugin->GetFormat();
    format->SetData(Tag::AUDIO_MAX_INPUT_SIZE, GetInputBufferSize());
    format->SetData(Tag::AUDIO_MAX_OUTPUT_SIZE, GetOutputBufferSize());
    return Status::OK;
}

Status FFmpegDsdDecoderPlugin::GetParameter(std::shared_ptr<Meta> &parameter)
{
    parameter = basePlugin->GetFormat();
    return Status::OK;
}

Status FFmpegDsdDecoderPlugin::QueueInputBuffer(const std::shared_ptr<AVBuffer> &inputBuffer)
{
    return basePlugin->ProcessSendData(inputBuffer);
}

Status FFmpegDsdDecoderPlugin::QueueOutputBuffer(std::shared_ptr<AVBuffer> &outputBuffer)
{
    return basePlugin->ProcessReceiveData(outputBuffer);
}

Status FFmpegDsdDecoderPlugin::GetInputBuffers(std::vector<std::shared_ptr<AVBuffer>> &inputBuffers)
{
    return Status::OK;
}

Status FFmpegDsdDecoderPlugin::GetOutputBuffers(std::vector<std::shared_ptr<AVBuffer>> &outputBuffers)
{
    return Status::OK;
}

Status FFmpegDsdDecoderPlugin::Flush()
{
    return basePlugin->Flush();
}

Status FFmpegDsdDecoderPlugin::Release()
{
    return basePlugin->Release();
}

std::string FFmpegDsdDecoderPlugin::GetDecoderName(const std::shared_ptr<Meta> &format)
{
    // Default is DSD_CODEC_LSBF_PLANAR(0), the DSF container format.
    int32_t dsdCodecType = DSD_CODEC_LSBF_PLANAR;
    format->GetData(Tag::AUDIO_DSD_CODEC_TYPE, dsdCodecType);
    switch (dsdCodecType) {
        case DSD_CODEC_MSBF_PLANAR:
            return "dsd_msbf_planar";
        case DSD_CODEC_MSBF:
            return "dsd_msbf";
        case DSD_CODEC_LSBF_PLANAR:
        default:
            return "dsd_lsbf_planar";
    }
}

Status FFmpegDsdDecoderPlugin::CheckFormat(const std::shared_ptr<Meta> &format)
{
    int32_t channelCount = 0;
    int32_t sampleRate = 0;
    format->GetData(Tag::AUDIO_CHANNEL_COUNT, channelCount);
    format->GetData(Tag::AUDIO_SAMPLE_RATE, sampleRate);
    // The demuxer passes the raw DSD rate / 8 (e.g. DSD64: 2822400 / 8 = 352800). Like the FFmpeg
    // DSD decoders (libavcodec/dsddec.c), there is no upper limit on the sample rate.
    if (sampleRate <= 0) {
        AVCODEC_LOGE("init failed, because sampleRate=%{public}d is invalid.", sampleRate);
        return Status::ERROR_INVALID_PARAMETER;
    } else if (channelCount < MIN_CHANNELS || channelCount > MAX_CHANNELS) {
        AVCODEC_LOGE("init failed, because channelCount=%{public}d not support.", channelCount);
        return Status::ERROR_INVALID_PARAMETER;
    }
    if (!basePlugin->CheckSampleFormat(format, channelCount)) {
        AVCODEC_LOGE("init failed, because CheckSampleFormat failed.");
        return Status::ERROR_INVALID_PARAMETER;
    }
    channels = channelCount;
    return Status::OK;
}

int32_t FFmpegDsdDecoderPlugin::GetInputBufferSize()
{
    // DSD input is one byte per sample per channel.
    int32_t inputBufferSize = SAMPLES * MAX_CHANNELS;
    int32_t maxSize = basePlugin->GetMaxInputSize();
    if (maxSize < 0 || maxSize > inputBufferSize) {
        maxSize = inputBufferSize;
    }
    return maxSize;
}

int32_t FFmpegDsdDecoderPlugin::GetOutputBufferSize()
{
    return SAMPLES * MAX_CHANNELS * MAX_BYTES_PER_SAMPLE;
}
} // namespace Ffmpeg
} // namespace Plugins
} // namespace Media
} // namespace OHOS
