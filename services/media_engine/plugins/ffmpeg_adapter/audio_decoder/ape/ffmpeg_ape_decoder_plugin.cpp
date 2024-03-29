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
constexpr int32_t INPUT_BUFFER_SIZE_DEFAULT = 200000;
constexpr int32_t OUTPUT_BUFFER_SIZE_DEFAULT = 20000;
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

Status FFmpegAPEDecoderPlugin::SetParameter(const std::shared_ptr<Meta> &parameter)
{
    Status ret = basePlugin->AllocateContext("ape");
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
    codecCtx->bits_per_coded_sample = 16; // sample bit = 16 bit

    if (codecCtx->extradata == nullptr) {
        AVCODEC_LOGE("no extradata! auto fix in.");
        codecCtx->extradata_size = 6; // 6bits
        uint8_t *fake_data = (uint8_t *)malloc(6); // 6bits
        int16_t *fileversion = (int16_t *)&fake_data[0];
        int16_t *compression_level = (int16_t *)&fake_data[2];
        int16_t *flags = (int16_t *)&fake_data[4];
        *fileversion = 3990; // 3990 version
        *compression_level = 2000; // 2000 complexity
        *flags = 0;
        codecCtx->extradata = fake_data;
    }
    auto format = basePlugin->GetFormat();
    format->SetData(Tag::AUDIO_MAX_INPUT_SIZE, GetInputBufferSize());
    format->SetData(Tag::AUDIO_MAX_OUTPUT_SIZE, GetOutputBufferSize());
    format->SetData(Tag::MIME_TYPE, MediaAVCodec::AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_APE);
    basePlugin->CheckSampleFormat(format, codecCtx->channels);
    ret = basePlugin->OpenContext();
    return ret;
}

Status FFmpegAPEDecoderPlugin::GetParameter(std::shared_ptr<Meta> &parameter)
{
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

bool FFmpegAPEDecoderPlugin::CheckSampleFormat(const std::shared_ptr<Meta> &format)
{
    return basePlugin->CheckSampleFormat(format, channels_);
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
