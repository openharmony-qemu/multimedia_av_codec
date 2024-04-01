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

#include "ffmpeg_flac_encoder_plugin.h"
#include <set>
#include "media_description.h"
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "avcodec_mime_type.h"
#include "avcodec_audio_common.h"
#include "ffmpeg_converter.h"

namespace {
using namespace OHOS::Media;
using namespace OHOS::Media::Plugins;
using namespace Ffmpeg;
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_AUDIO, "AvCodec-AudioFFMpegFlacEncoderPlugin"};
constexpr int32_t MIN_CHANNELS = 1;
constexpr int32_t MAX_CHANNELS = 8;
constexpr int32_t FLAC_MIN_BIT_RATE = 32000;
constexpr int32_t FLAC_MAX_BIT_RATE = 1536000;
constexpr int32_t MIN_COMPLIANCE_LEVEL = -2;
constexpr int32_t MAX_COMPLIANCE_LEVEL = 2;
constexpr int32_t MAX_BITS_PER_SAMPLE = 4;
constexpr int32_t SAMPLES = 9216;
static const int32_t FLAC_ENCODER_SAMPLE_RATE_TABLE[] = {
    8000, 16000, 22050, 24000, 32000, 44100, 48000, 88200, 96000,
};
static const uint64_t FLAC_CHANNEL_LAYOUT_TABLE[] = {AV_CH_LAYOUT_MONO,    AV_CH_LAYOUT_STEREO,  AV_CH_LAYOUT_SURROUND,
                                                     AV_CH_LAYOUT_QUAD,    AV_CH_LAYOUT_5POINT0, AV_CH_LAYOUT_5POINT1,
                                                     AV_CH_LAYOUT_6POINT1, AV_CH_LAYOUT_7POINT1};
const std::map<int32_t, int32_t> BITS_PER_RAW_SAMPLE_MAP = {
    {OHOS::MediaAVCodec::AudioSampleFormat::SAMPLE_S16LE, 16},
    {OHOS::MediaAVCodec::AudioSampleFormat::SAMPLE_S24LE, 24},
    {OHOS::MediaAVCodec::AudioSampleFormat::SAMPLE_S32LE, 32},
};
static std::set<OHOS::MediaAVCodec::AudioSampleFormat> supportedSampleFormats = {
    OHOS::MediaAVCodec::AudioSampleFormat::SAMPLE_S16LE,
    OHOS::MediaAVCodec::AudioSampleFormat::SAMPLE_S32LE,
};
} // namespace

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Ffmpeg {
FFmpegFlacEncoderPlugin::FFmpegFlacEncoderPlugin(const std::string& name)
    : CodecPlugin(name), channels_(0), basePlugin(std::make_unique<FFmpegBaseEncoder>())
{
}

FFmpegFlacEncoderPlugin::~FFmpegFlacEncoderPlugin()
{
    basePlugin->Release();
    basePlugin.reset();
    basePlugin = nullptr;
}

static bool CheckSampleRate(int32_t sampleRate)
{
    bool isExist = std::any_of(std::begin(FLAC_ENCODER_SAMPLE_RATE_TABLE),
        std::end(FLAC_ENCODER_SAMPLE_RATE_TABLE), [sampleRate](int32_t value) {
        return value == sampleRate;
    });
    return isExist;
}

static bool CheckChannelLayout(uint64_t channelLayout)
{
    bool isExist = std::any_of(std::begin(FLAC_CHANNEL_LAYOUT_TABLE),
        std::end(FLAC_CHANNEL_LAYOUT_TABLE), [channelLayout](uint64_t value) {
        return value == channelLayout;
    });
    return isExist;
}

static bool CheckBitsPerSample(int32_t bitsPerCodedSample)
{
    bool isExist = std::any_of(BITS_PER_RAW_SAMPLE_MAP.begin(),
        BITS_PER_RAW_SAMPLE_MAP.end(), [bitsPerCodedSample](const std::pair<int32_t, int32_t>& value) {
        return value.first == bitsPerCodedSample;
    });
    return isExist;
}

Status FFmpegFlacEncoderPlugin::SetContext(const std::shared_ptr<Meta> &format)
{
    int32_t complianceLevel;
    int32_t bitsPerCodedSample;
    auto avCodecContext = basePlugin->GetCodecContext();
    format->GetData(Tag::AUDIO_FLAC_COMPLIANCE_LEVEL, complianceLevel);
    format->GetData(Tag::AUDIO_BITS_PER_CODED_SAMPLE, bitsPerCodedSample);
    avCodecContext->strict_std_compliance = complianceLevel;
    if (BITS_PER_RAW_SAMPLE_MAP.find(bitsPerCodedSample) == BITS_PER_RAW_SAMPLE_MAP.end()) {
        return Status::ERROR_INVALID_PARAMETER;
    }
    avCodecContext->bits_per_raw_sample = BITS_PER_RAW_SAMPLE_MAP.at(bitsPerCodedSample);
    return Status::OK;
}

bool FFmpegFlacEncoderPlugin::CheckBitRate(const std::shared_ptr<Meta> &format) const
{
    int64_t bitRate;
    if (!format->GetData(Tag::MEDIA_BITRATE, bitRate)) {
        AVCODEC_LOGE("parameter bit_rate type invalid");
        return false;
    }
    if (bitRate < FLAC_MIN_BIT_RATE || bitRate > FLAC_MAX_BIT_RATE) {
        AVCODEC_LOGE("parameter bit_rate illegal");
        return false;
    }
    return true;
}

Status FFmpegFlacEncoderPlugin::CheckFormat(const std::shared_ptr<Meta> &format)
{
    int32_t channelCount;
    int32_t sampleRate;
    int32_t bitsPerCodedSample;
    int32_t complianceLevel;
    format->GetData(Tag::AUDIO_CHANNEL_COUNT, channelCount);
    format->GetData(Tag::AUDIO_SAMPLE_RATE, sampleRate);
    format->GetData(Tag::AUDIO_BITS_PER_CODED_SAMPLE, bitsPerCodedSample);
    format->GetData(Tag::AUDIO_FLAC_COMPLIANCE_LEVEL, complianceLevel);

    AudioChannelLayout channelLayout;
    format->GetData(Tag::AUDIO_CHANNEL_LAYOUT, channelLayout);
    auto ffChannelLayout =
        FFMpegConverter::ConvertOHAudioChannelLayoutToFFMpeg(static_cast<AudioChannelLayout>(channelLayout));
    if (ffChannelLayout == AV_CH_LAYOUT_NATIVE) {
        AVCODEC_LOGE("InitContext failed, because ffChannelLayout is AV_CH_LAYOUT_NATIVE");
        return Status::ERROR_INVALID_PARAMETER;
    }

    AudioSampleFormat sampleFormat;
    format->GetData(Tag::AUDIO_SAMPLE_FORMAT, sampleFormat);
    if (!sampleFormat) {
        AVCODEC_LOGE("input sample format not supported");
        return Status::ERROR_INVALID_PARAMETER;
    }

    if (!CheckSampleRate(sampleRate)) {
        AVCODEC_LOGE("init failed, because sampleRate=%{public}d not in table.", sampleRate);
        return Status::ERROR_INVALID_PARAMETER;
    } else if (channelCount < MIN_CHANNELS || channelCount > MAX_CHANNELS) {
        AVCODEC_LOGE("init failed, because channelCount=%{public}d not support.", channelCount);
        return Status::ERROR_INVALID_PARAMETER;
    } else if (!CheckBitsPerSample(bitsPerCodedSample)) {
        AVCODEC_LOGE("init failed, because bitsPerCodedSample=%{public}d not support.", bitsPerCodedSample);
        return Status::ERROR_INVALID_PARAMETER;
    } else if (complianceLevel < MIN_COMPLIANCE_LEVEL || complianceLevel > MAX_COMPLIANCE_LEVEL) {
        AVCODEC_LOGE("init failed, because complianceLevel=%{public}d not support.", complianceLevel);
        return Status::ERROR_INVALID_PARAMETER;
    } else if (!CheckChannelLayout(ffChannelLayout)) {
        AVCODEC_LOGE("init failed, because ffChannelLayout=%{public}" PRId64 "not support.", ffChannelLayout);
        return Status::ERROR_INVALID_PARAMETER;
    } else if (!CheckBitRate(format)) {
        return Status::ERROR_INVALID_PARAMETER;
    }
    channels_ = channelCount;
    return Status::OK;
}

Status FFmpegFlacEncoderPlugin::GetParameter(std::shared_ptr<Meta> &parameter)
{
    auto maxInputSize = GetInputBufferSize();
    auto maxOutputSize = GetOutputBufferSize();
    audioParameter_.SetData(Tag::AUDIO_MAX_INPUT_SIZE, maxInputSize);
    audioParameter_.SetData(Tag::AUDIO_MAX_OUTPUT_SIZE, maxOutputSize);
    *parameter = audioParameter_;
    return Status::OK;
}

Status FFmpegFlacEncoderPlugin::SetParameter(const std::shared_ptr<Meta> &parameter)
{
    Status ret = basePlugin->AllocateContext("flac");
    if (ret != Status::OK) {
        AVCODEC_LOGE("init failed, because AllocateContext failed. ret=%{public}d", ret);
        return ret;
    }

    ret = CheckFormat(parameter);
    if (ret != Status::OK) {
        AVCODEC_LOGE("init failed, because CheckFormat failed. ret=%{public}d", ret);
        return ret;
    }

    ret = basePlugin->InitContext(parameter);
    if (ret != Status::OK) {
        AVCODEC_LOGE("init failed, because InitContext failed. ret=%{public}d", ret);
        return ret;
    }

    ret = SetContext(parameter);
    if (ret != Status::OK) {
        AVCODEC_LOGE("init failed, because SetContext failed. ret=%{public}d", ret);
        return ret;
    }

    ret = basePlugin->OpenContext();
    if (ret != Status::OK) {
        AVCODEC_LOGE("init failed, because OpenContext failed. ret=%{public}d", ret);
        return ret;
    }
    
    audioParameter_ = *parameter;

    ret = basePlugin->InitFrame();
    if (ret != Status::OK) {
        AVCODEC_LOGE("init failed, because InitFrame failed. ret=%{public}d", ret);
        return ret;
    }

    return Status::OK;
}

Status FFmpegFlacEncoderPlugin::Init()
{
    return Status::OK;
}

Status FFmpegFlacEncoderPlugin::Start()
{
    return Status::OK;
}

Status FFmpegFlacEncoderPlugin::Stop()
{
    return basePlugin->Stop();
}

Status FFmpegFlacEncoderPlugin::Reset()
{
    return basePlugin->Reset();
}

Status FFmpegFlacEncoderPlugin::Release()
{
    return basePlugin->Release();
}

Status FFmpegFlacEncoderPlugin::Flush()
{
    return basePlugin->Flush();
}

Status FFmpegFlacEncoderPlugin::QueueInputBuffer(const std::shared_ptr<AVBuffer> &inputBuffer)
{
    return basePlugin->ProcessSendData(inputBuffer);
}

Status FFmpegFlacEncoderPlugin::QueueOutputBuffer(std::shared_ptr<AVBuffer> &outBuffer)
{
    return basePlugin->ProcessReceiveData(outBuffer);
}

Status FFmpegFlacEncoderPlugin::GetInputBuffers(std::vector<std::shared_ptr<AVBuffer>> &inputBuffers)
{
    return Status::OK;
}

Status FFmpegFlacEncoderPlugin::GetOutputBuffers(std::vector<std::shared_ptr<AVBuffer>> &outputBuffers)
{
    return Status::OK;
}

int32_t FFmpegFlacEncoderPlugin::GetInputBufferSize()
{
    int32_t inputBufferSize = SAMPLES * channels_ * MAX_BITS_PER_SAMPLE;
    int32_t maxSize = basePlugin->GetMaxInputSize();
    if (maxSize < 0 || maxSize > inputBufferSize) {
        maxSize = inputBufferSize;
    }
    return maxSize;
}

int32_t FFmpegFlacEncoderPlugin::GetOutputBufferSize()
{
    int32_t outputBufferSize = SAMPLES * channels_ * MAX_BITS_PER_SAMPLE;
    return outputBufferSize;
}
} // namespace Ffmpeg
} // namespace Plugins
} // namespace Media
} // namespace OHOS
