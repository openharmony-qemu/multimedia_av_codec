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
constexpr int32_t INPUT_BUFFER_SIZE_DEFAULT = 300000;
constexpr int32_t OUTPUT_BUFFER_SIZE_DEFAULT = 50000;
constexpr int32_t EXTRA_DATA_SIZE = 6;
} // namespace

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Ffmpeg {
FFmpegAPEDecoderPlugin::FFmpegAPEDecoderPlugin(const std::string& name)
    : CodecPlugin(name), channels_(0), basePlugin(std::make_unique<FfmpegBaseDecoder>())
{
}

FFmpegAPEDecoderPlugin::~FFmpegAPEDecoderPlugin()
{
    basePlugin->Release();
    basePlugin.reset();
    basePlugin = nullptr;
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

void FFmpegAPEDecoderPlugin::SetSamplerate(const std::shared_ptr<Meta> &parameter)
{
    int32_t sampleRate;
    parameter->GetData(Tag::AUDIO_SAMPLE_RATE, sampleRate);
    if (sampleRate <= 0) {
        parameter->SetData(Tag::AUDIO_SAMPLE_RATE, 16000); // set 16000 sample rate
    }
}

Status FFmpegAPEDecoderPlugin::SetParameter(const std::shared_ptr<Meta> &parameter)
{
    Status ret = basePlugin->AllocateContext("ape");
    SetSamplerate(parameter);
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
    AudioSampleFormat samplefmt;
    parameter->GetData(Tag::AUDIO_SAMPLE_FORMAT, samplefmt);

    if (samplefmt == SAMPLE_S16LE) {
        codecCtx->bits_per_coded_sample = 16; // sample bit = 16 bit
        codecCtx->sample_fmt = AV_SAMPLE_FMT_S16;
    }
    if (samplefmt == SAMPLE_U8) {
        codecCtx->bits_per_coded_sample = 8; // sample bit = 8 bit
        codecCtx->sample_fmt = AV_SAMPLE_FMT_U8;
    }
    if (samplefmt == SAMPLE_S32LE) {
        codecCtx->bits_per_coded_sample = 32; // sample bit = 32 bit
        codecCtx->sample_fmt = AV_SAMPLE_FMT_S32;
    }
    AVCODEC_LOGI("samplefmt be set %{publib}d.", codecCtx->bits_per_coded_sample);
    if (codecCtx->extradata == nullptr) {
        codecCtx->extradata_size = EXTRA_DATA_SIZE; // 6bits
        codecCtx->extradata = static_cast<uint8_t *>(av_mallocz(EXTRA_DATA_SIZE + AV_INPUT_BUFFER_PADDING_SIZE));
        int16_t fakedata[3]; // 3 int16_t data
        fakedata[0] = 3990;  // 3990 version
        fakedata[1] = 2000;  // 2000 complexity
        fakedata[2] = 0;     // flags 0
        if (memcpy_s(codecCtx->extradata, EXTRA_DATA_SIZE, fakedata, EXTRA_DATA_SIZE) != EOK) {
            AVCODEC_LOGE("extradata memcpy_s failed.");
            return Status::ERROR_INVALID_PARAMETER;
        }
    }
    auto format = basePlugin->GetFormat();
    format->SetData(Tag::AUDIO_MAX_INPUT_SIZE, GetInputBufferSize());
    format->SetData(Tag::AUDIO_MAX_OUTPUT_SIZE, GetOutputBufferSize());
    basePlugin->CheckSampleFormat(format, codecCtx->channels);
    ret = basePlugin->OpenContext();
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
    int32_t maxSize = basePlugin->GetMaxInputSize();
    if (maxSize < 0 || maxSize > INPUT_BUFFER_SIZE_DEFAULT) {
        maxSize = INPUT_BUFFER_SIZE_DEFAULT;
    }
    return maxSize;
}

int32_t FFmpegAPEDecoderPlugin::GetOutputBufferSize()
{
    return OUTPUT_BUFFER_SIZE_DEFAULT;
}
} // namespace Ffmpeg
} // namespace Plugins
} // namespace Media
} // namespace OHOS
