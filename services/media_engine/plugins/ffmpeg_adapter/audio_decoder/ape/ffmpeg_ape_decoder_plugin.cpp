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
#include "ffmpeg_ape_decoder_plugin.h"
#include <algorithm>
#include "avcodec_codec_name.h"
#include "avcodec_log.h"
#include "plugin/codec_plugin.h"
#include "plugin/plugin_definition.h"
#include "avcodec_mime_type.h"
#include "avcodec_audio_common.h"
#include <iostream>
namespace {
using namespace OHOS::Media;
using namespace OHOS::Media::Plugins;
using namespace Ffmpeg;

constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_AUDIO, "AvCodec-FFmpegAPEDecoderPlugin"};

constexpr int MIN_CHANNELS = 1;
constexpr int MAX_CHANNELS = 2;
constexpr int32_t INPUT_BUFFER_SIZE_MIN = 300000;
constexpr int32_t OUTPUT_BUFFER_SIZE_MIN = 50000;
constexpr int32_t DEFAULT_FRAME_BLOCK = 73728;
constexpr int32_t FRAME_BLOCK_NUM = 4;
constexpr int32_t EXTRA_DATA_SIZE = 6;
constexpr int32_t OUT_BUFFER_NUM = 20;
constexpr int32_t LOW_VERSION_BLOCK = 9216;
constexpr int32_t INSANE_LEVEL = 5000;
constexpr int32_t EXTRA_HIGH_LEVEL = 4000;
constexpr int32_t HIGH_LEVEL = 3000;
constexpr int32_t NORMAL_LEVEL = 2000;

constexpr int32_t VERSION_3990 = 3990;
constexpr int32_t VERSION_3980 = 3980;
constexpr int32_t VERSION_3950 = 3950;
constexpr int32_t VERSION_3900 = 3900;
constexpr int32_t VERSION_3800 = 3800;

constexpr int32_t BITS_S8 = 8;
constexpr int32_t BITS_S16 = 16;
constexpr int32_t BITS_S24 = 24;
constexpr int32_t BITS_S32 = 32;
constexpr int32_t BITS_TO_BYTES = 8;

constexpr int32_t REDUNDANCY_SET = 20;
} // namespace

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Ffmpeg {
FFmpegAPEDecoderPlugin::FFmpegAPEDecoderPlugin(const std::string& name)
    : CodecPlugin(name), channels_(0), basePlugin(std::make_unique<FfmpegBaseDecoder>()), version_(0),
      sampleSizePerFrame_(0), sampleSizePerOutBuffer_(0), compressionLevel_(0), sampleRate_(0), depth_(0)
{
}

FFmpegAPEDecoderPlugin::~FFmpegAPEDecoderPlugin()
{
    if (basePlugin != nullptr) {
        basePlugin->Release();
        basePlugin.reset();
    }
}

Status FFmpegAPEDecoderPlugin::Init()
{
    return Status::OK;
}

Status FFmpegAPEDecoderPlugin::Prepare()
{
    return Status::OK;
}

Status FFmpegAPEDecoderPlugin::Reset()
{
    return basePlugin->Reset();
}

Status FFmpegAPEDecoderPlugin::Start()
{
    return Status::OK;
}

Status FFmpegAPEDecoderPlugin::Stop()
{
    return Status::OK;
}

bool FFmpegAPEDecoderPlugin::SetSamplerate(const std::shared_ptr<Meta> &parameter)
{
    parameter->GetData(Tag::AUDIO_SAMPLE_RATE, sampleRate_);
    if (sampleRate_ < 0) {
        return false;
    }
    if (sampleRate_ == 0) {
        parameter->SetData(Tag::AUDIO_SAMPLE_RATE, 16000); // set 16000 sample rate
    }
    return true;
}

Status FFmpegAPEDecoderPlugin::SetParameter(const std::shared_ptr<Meta> &parameter)
{
    Status ret = basePlugin->AllocateContext("ape");
    if (!SetSamplerate(parameter)) {
        return Status::ERROR_INVALID_PARAMETER;
    }
    if (!CheckChannelCount(parameter)) {
        return Status::ERROR_INVALID_PARAMETER;
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
    auto codecCtx = basePlugin->GetCodecContext();
    if (codecCtx->extradata == nullptr) {
        codecCtx->extradata_size = EXTRA_DATA_SIZE; // 6bits
        codecCtx->extradata = static_cast<uint8_t *>(av_mallocz(EXTRA_DATA_SIZE + AV_INPUT_BUFFER_PADDING_SIZE));
        if (!codecCtx->extradata) {
            AVCODEC_LOGE("Fail to apply extradata.");
            return Status::ERROR_NO_MEMORY;
        }
        int16_t fakedata[3]; // 3 int16_t data
        fakedata[0] = VERSION_3990;  // 3990 version_
        fakedata[1] = NORMAL_LEVEL;  // 2000 complexity
        fakedata[2] = 0;     // flags 0
        if (memcpy_s(codecCtx->extradata, EXTRA_DATA_SIZE, fakedata, EXTRA_DATA_SIZE) != EOK) {
            AVCODEC_LOGE("extradata memcpy_s failed.");
            return Status::ERROR_INVALID_PARAMETER;
        }
    }
    auto format = basePlugin->GetFormat();

    basePlugin->CheckSampleFormat(format, codecCtx->ch_layout.nb_channels);
    AudioSampleFormat sampleFmt = SAMPLE_S16LE;
    parameter->GetData(Tag::AUDIO_SAMPLE_FORMAT, sampleFmt);
    parameter->GetData(Tag::AUDIO_BITS_PER_CODED_SAMPLE, codecCtx->bits_per_coded_sample);
    if (codecCtx->bits_per_coded_sample == 0) {
        codecCtx->bits_per_coded_sample = SetBitsDepth(sampleFmt);
    }
    depth_ = codecCtx->bits_per_coded_sample;
    CalcBufferSize(parameter, codecCtx->extradata);
    format->SetData(Tag::AUDIO_MAX_INPUT_SIZE, GetInputBufferSize());
    format->SetData(Tag::AUDIO_MAX_OUTPUT_SIZE, GetOutputBufferSize());
    ret = basePlugin->OpenContext();
    return ret;
}

int32_t FFmpegAPEDecoderPlugin::SetBitsDepth(AudioSampleFormat sampleFmt)
{
    int32_t ret = BITS_S16; // default sample bit = 16 bit
    if (sampleFmt == SAMPLE_S16LE || sampleFmt == SAMPLE_S16P) {
        ret = BITS_S16; // sample bit = 16 bit
    } else if (sampleFmt == SAMPLE_U8 || sampleFmt == SAMPLE_U8P) {
        ret = BITS_S8; // sample bit = 8 bit
    } else if (sampleFmt == SAMPLE_S32LE || sampleFmt == SAMPLE_S32P) {
        ret = BITS_S24; // sample bit = 24 bit
    }
    AVCODEC_LOGI("sample depth_ be set %{public}d.", ret);
    return ret;
}

Status FFmpegAPEDecoderPlugin::GetParameter(std::shared_ptr<Meta> &parameter)
{
    parameter->SetData(Tag::MIME_TYPE, MediaAVCodec::AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_APE);
    parameter = basePlugin->GetFormat();
    return Status::OK;
}

Status FFmpegAPEDecoderPlugin::QueueInputBuffer(const std::shared_ptr<AVBuffer> &inputBuffer)
{
    return basePlugin->ProcessSendData(inputBuffer);
}

Status FFmpegAPEDecoderPlugin::QueueOutputBuffer(std::shared_ptr<AVBuffer> &outputBuffer)
{
    return basePlugin->ProcessReceiveData(outputBuffer);
}

Status FFmpegAPEDecoderPlugin::GetInputBuffers(std::vector<std::shared_ptr<AVBuffer>> &inputBuffers)
{
    return Status::OK;
}

Status FFmpegAPEDecoderPlugin::GetOutputBuffers(std::vector<std::shared_ptr<AVBuffer>> &outputBuffers)
{
    return Status::OK;
}

Status FFmpegAPEDecoderPlugin::Flush()
{
    return basePlugin->Flush();
}

Status FFmpegAPEDecoderPlugin::Release()
{
    return basePlugin->Release();
}

bool FFmpegAPEDecoderPlugin::CheckChannelCount(const std::shared_ptr<Meta> &format)
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

int32_t FFmpegAPEDecoderPlugin::GetInputBufferSize()
{
    if (INPUT_BUFFER_SIZE_MIN > sampleSizePerFrame_) {
        sampleSizePerFrame_ = INPUT_BUFFER_SIZE_MIN;
    }
    return sampleSizePerFrame_;
}

int32_t FFmpegAPEDecoderPlugin::GetOutputBufferSize()
{
    if (OUTPUT_BUFFER_SIZE_MIN > sampleSizePerOutBuffer_) {
        sampleSizePerOutBuffer_ = OUTPUT_BUFFER_SIZE_MIN;
    }
    return sampleSizePerOutBuffer_;
}

void FFmpegAPEDecoderPlugin::CalcBufferSize(const std::shared_ptr<Meta> &parameter, void *extradata)
{
    int32_t samplePreFrame = 0;
    if (depth_ == BITS_S24) { // depth_ S24 means S32 in auctual
        depth_ = BITS_S32;
    }
    depth_ = depth_ / BITS_TO_BYTES; // divide by 8
    
    parameter->GetData(Tag::AUDIO_SAMPLE_PER_FRAME, samplePreFrame);
    if (samplePreFrame != 0) {
        sampleSizePerFrame_ = channels_ * samplePreFrame * depth_;
        sampleSizePerOutBuffer_ = sampleSizePerFrame_ / REDUNDANCY_SET;
        AVCODEC_LOGI("APE sampleSize inbuffer: %{public}d outbuffer: %{public}d",
                     sampleSizePerFrame_, sampleSizePerOutBuffer_);
        return;
    }
    
    int16_t *extradata16 = nullptr;
    extradata16 = (int16_t *)extradata;
    version_ = *extradata16;
    compressionLevel_ =  *(extradata16 + 1);
    // As for APE, different version has frame block in differ
    if (version_ >= VERSION_3980) {
        if (compressionLevel_ == INSANE_LEVEL) {
            sampleSizePerFrame_ = DEFAULT_FRAME_BLOCK * FRAME_BLOCK_NUM * FRAME_BLOCK_NUM * channels_ * depth_;
        } else if (compressionLevel_ >= HIGH_LEVEL) {
            sampleSizePerFrame_ = DEFAULT_FRAME_BLOCK * FRAME_BLOCK_NUM * channels_ * depth_;
        } else {
            sampleSizePerFrame_ = DEFAULT_FRAME_BLOCK * channels_ * depth_;
        }
    } else if (version_ >= VERSION_3950) {
        sampleSizePerFrame_ = DEFAULT_FRAME_BLOCK * FRAME_BLOCK_NUM * channels_ * depth_;
    } else if (version_ >= VERSION_3900 || (version_ >= VERSION_3800 && compressionLevel_ >= EXTRA_HIGH_LEVEL)) {
        sampleSizePerFrame_ = DEFAULT_FRAME_BLOCK * channels_ * depth_;
    } else {
        sampleSizePerFrame_ = LOW_VERSION_BLOCK * channels_ * depth_;
    }
    basePlugin->SetMaxInputSize(sampleSizePerFrame_);
    sampleSizePerOutBuffer_ = sampleSizePerFrame_/OUT_BUFFER_NUM;
    AVCODEC_LOGI("APE version %{public}d compressLevel %{public}d sampleSize %{public}d %{public}d", version_,
                 compressionLevel_, sampleSizePerFrame_, sampleSizePerOutBuffer_);
    AVCODEC_LOGI("APE channel %{public}d deth %{public}d", channels_, depth_);
}
} // namespace Ffmpeg
} // namespace Plugins
} // namespace Media
} // namespace OHOS
