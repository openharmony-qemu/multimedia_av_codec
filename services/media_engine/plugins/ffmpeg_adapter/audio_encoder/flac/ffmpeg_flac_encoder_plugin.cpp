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
#include <algorithm>
#include "common/pcm_convert.h"
#include "avcodec_log.h"
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
constexpr int32_t MAX_BYTES_PER_SAMPLE = 4;
constexpr int32_t SAMPLES = 9216;
static const int32_t FLAC_ENCODER_SAMPLE_RATE_TABLE[] = {
    8000, 16000, 22050, 24000, 32000, 44100, 48000, 88200, 96000,
};
static const uint64_t FLAC_CHANNEL_LAYOUT_TABLE[] = {AV_CH_LAYOUT_MONO,    AV_CH_LAYOUT_STEREO,  AV_CH_LAYOUT_SURROUND,
                                                     AV_CH_LAYOUT_QUAD,    AV_CH_LAYOUT_5POINT0, AV_CH_LAYOUT_5POINT1,
                                                     AV_CH_LAYOUT_6POINT1, AV_CH_LAYOUT_7POINT1};

static std::set<AudioSampleFormat> supportedSampleFormats = {
    AudioSampleFormat::SAMPLE_S16LE,
    AudioSampleFormat::SAMPLE_S32LE,
};

static std::set<AudioSampleFormat> extendSampleFormats = {
    AudioSampleFormat::SAMPLE_U8,
    AudioSampleFormat::SAMPLE_S16LE,
    AudioSampleFormat::SAMPLE_S24LE,
    AudioSampleFormat::SAMPLE_S32LE,
    AudioSampleFormat::SAMPLE_F32LE,
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

Status FFmpegFlacEncoderPlugin::SetContext(const std::shared_ptr<Meta> &format)
{
    int32_t complianceLevel;
    auto avCodecContext = basePlugin->GetCodecContext();
    format->GetData(Tag::AUDIO_FLAC_COMPLIANCE_LEVEL, complianceLevel);
    avCodecContext->strict_std_compliance = complianceLevel;
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
    int32_t complianceLevel;
    format->GetData(Tag::AUDIO_CHANNEL_COUNT, channelCount);
    format->GetData(Tag::AUDIO_SAMPLE_RATE, sampleRate);
    format->GetData(Tag::AUDIO_FLAC_COMPLIANCE_LEVEL, complianceLevel);

    AudioChannelLayout channelLayout;
    format->GetData(Tag::AUDIO_CHANNEL_LAYOUT, channelLayout);
    auto ffChannelLayout =
        FFMpegConverter::ConvertOHAudioChannelLayoutToFFMpeg(static_cast<AudioChannelLayout>(channelLayout));
    CHECK_AND_RETURN_RET_LOG(ffChannelLayout, Status::ERROR_INVALID_PARAMETER,
        "InitContext failed, because channelLayout is not supported.");

    if (!format->GetData(Tag::AUDIO_SAMPLE_FORMAT, audioSampleFormat_)) {
        AVCODEC_LOGE("flac sample format must be set.");
        return Status::ERROR_INVALID_PARAMETER;
    }

    if (isEnableFormatConvert_ &&
        extendSampleFormats.find(audioSampleFormat_) == extendSampleFormats.end()) {
        AVCODEC_LOGE("enable convert but input sample format not support:=%{public}d",
            static_cast<int32_t>(audioSampleFormat_));
        return Status::ERROR_INVALID_PARAMETER;
    }

    convertSampleFormat_ = audioSampleFormat_;

    // FLAC FFmpeg encoder only supports S16 and S32, convert U8/S24LE/F32LE to S32LE
    if (isEnableFormatConvert_ && (audioSampleFormat_ == AudioSampleFormat::SAMPLE_U8
        || audioSampleFormat_ == AudioSampleFormat::SAMPLE_S24LE
        || audioSampleFormat_ == AudioSampleFormat::SAMPLE_F32LE)) {
        convertSampleFormat_ = AudioSampleFormat::SAMPLE_S32LE;
        AVCODEC_LOGI("need extend convert sample format from:%{public}d to S32LE",
            static_cast<int32_t>(audioSampleFormat_));
    }

    if (!CheckSampleRate(sampleRate)) {
        AVCODEC_LOGE("init failed, because sampleRate=%{public}d not in table.", sampleRate);
        return Status::ERROR_INVALID_PARAMETER;
    } else if (channelCount < MIN_CHANNELS || channelCount > MAX_CHANNELS) {
        AVCODEC_LOGE("init failed, because channelCount=%{public}d not support.", channelCount);
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
    auto maxInputBufferSize = basePlugin->GetMaxInputBufferSize();
    if (maxInputBufferSize > 0) {
        audioParameter_.SetData(Tag::AUDIO_MAX_INPUT_SIZE, maxInputBufferSize);
    } else {
        audioParameter_.SetData(Tag::AUDIO_MAX_INPUT_SIZE, maxInputSize);
    }
    audioParameter_.SetData(Tag::AUDIO_MAX_OUTPUT_SIZE, maxOutputSize);
    *parameter = audioParameter_;
    return Status::OK;
}

Status FFmpegFlacEncoderPlugin::SetParameter(const std::shared_ptr<Meta> &parameter)
{
    Status ret = basePlugin->AllocateContext("flac");
    CHECK_AND_RETURN_RET_LOG(ret == Status::OK, ret,
        "init failed, because AllocateContext failed. ret=%{public}d", ret);

    parameter->Get<Tag::AUDIO_ENCODER_ENABLE_SAMPLE_FORMAT_CONVERT>(isEnableFormatConvert_);

    ret = CheckFormat(parameter);
    CHECK_AND_RETURN_RET_LOG(ret == Status::OK, ret,
        "init failed, because CheckFormat failed. ret=%{public}d", ret);

    // Create modified meta with converted sample format for basePlugin
    std::shared_ptr<Meta> initMeta = std::make_shared<Meta>(*parameter);
    initMeta->SetData(Tag::AUDIO_SAMPLE_FORMAT, convertSampleFormat_);

    // Configure format conversion in base encoder before InitContext
    if (isEnableFormatConvert_ && audioSampleFormat_ != convertSampleFormat_) {
        basePlugin->SetFormatConvert(true, audioSampleFormat_, convertSampleFormat_);
    }

    basePlugin->InitContext(initMeta);

    SetContext(parameter);

    ret = basePlugin->OpenContext();
    CHECK_AND_RETURN_RET_LOG(ret == Status::OK, ret, "init failed, because OpenContext failed. ret=%{public}d", ret);
    
    audioParameter_ = *parameter;
    int32_t ptsMode = DEFAULT_ENCODE_PTS_MODE;
    if (parameter->Get<Tag::AUDIO_ENCODE_PTS_MODE>(ptsMode)) {
        basePlugin->SetPtsMode(ptsMode);
    }

    ret = basePlugin->InitFrame();
    CHECK_AND_RETURN_RET_LOG(ret == Status::OK, ret, "init failed, because InitFrame failed. ret=%{public}d", ret);

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
    isEnableFormatConvert_ = false;
    audioSampleFormat_ = AudioSampleFormat::INVALID_WIDTH;
    convertSampleFormat_ = AudioSampleFormat::INVALID_WIDTH;
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
    int32_t inputBufferSize = SAMPLES * channels_ * MAX_BYTES_PER_SAMPLE;
    int32_t maxSize = basePlugin->GetMaxInputSize();
    if (maxSize < 0 || maxSize > inputBufferSize) {
        maxSize = inputBufferSize;
    }
    return maxSize;
}

int32_t FFmpegFlacEncoderPlugin::GetOutputBufferSize()
{
    int32_t outputBufferSize = SAMPLES * channels_ * MAX_BYTES_PER_SAMPLE;
    return outputBufferSize;
}
} // namespace Ffmpeg
} // namespace Plugins
} // namespace Media
} // namespace OHOS
