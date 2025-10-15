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
#include "wma/ffmpeg_wma_decoder_plugin.h"
#include <algorithm>
#include "avcodec_log.h"
#include "avcodec_codec_name.h"
#include "plugin/codec_plugin.h"
#include "plugin/plugin_definition.h"
#include "avcodec_audio_common.h"
#include "meta/mime_type.h"

namespace {
using namespace OHOS::Media;
using namespace OHOS::Media::Plugins;
using namespace OHOS::Media::Plugins::Ffmpeg;
using namespace OHOS::MediaAVCodec;

constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {
    LOG_CORE, LOG_DOMAIN_AUDIO, "AvCodec-FFmpegWMADecoderPlugin"
};

constexpr int MIN_CHANNELS = 1;

constexpr int MAX_CHANNELS_WMA_LEGACY = 2;
constexpr int MAX_CHANNELS_WMAPRO      = 8;

constexpr int MAX_SR_WMA_LEGACY = 48000;
constexpr int MAX_SR_WMAPRO     = 96000;

constexpr int32_t INPUT_BUFFER_SIZE_DEFAULT  = 8192;
constexpr int32_t OUTPUT_BUFFER_SIZE_DEFAULT = 4 * 2048 * 8;
} // namespace

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Ffmpeg {

FFmpegWMADecoderPlugin::FFmpegWMADecoderPlugin(const std::string& name)
    : CodecPlugin(name), base_(std::make_unique<FfmpegBaseDecoder>())
{
    if (name == AVCodecCodecName::AUDIO_DECODER_WMAV1_NAME) {
        ffCodecName_ = "wmav1";
    } else if (name == AVCodecCodecName::AUDIO_DECODER_WMAV2_NAME) {
        ffCodecName_ = "wmav2";
    } else {
        ffCodecName_ = "wmapro";
    }
    AVCODEC_LOGI("create wma decoder plugin by %{public}s", name.c_str());
}

FFmpegWMADecoderPlugin::~FFmpegWMADecoderPlugin()
{
    base_->Release();
    base_.reset();
}

Status FFmpegWMADecoderPlugin::Init()    { return Status::OK; }
Status FFmpegWMADecoderPlugin::Prepare() { return Status::OK; }
Status FFmpegWMADecoderPlugin::Start()   { return Status::OK; }
Status FFmpegWMADecoderPlugin::Stop()    { return Status::OK; }
Status FFmpegWMADecoderPlugin::Reset()   { return base_->Reset(); }
Status FFmpegWMADecoderPlugin::Flush()   { return base_->Flush(); }
Status FFmpegWMADecoderPlugin::Release() { return base_->Release(); }

Status FFmpegWMADecoderPlugin::SetParameter(const std::shared_ptr<Meta> &meta)
{
    if (!CheckFormat(meta)) {
        return Status::ERROR_INVALID_PARAMETER;
    }
    auto ret = base_->AllocateContext(ffCodecName_);
    if (ret != Status::OK) {
        AVCODEC_LOGE("AllocateContext failed, ret=%{public}d", ret);
        return ret;
    }

    if (!CheckSampleFormat(meta)) {
        return Status::ERROR_INVALID_PARAMETER;
    }

    ret = base_->InitContext(meta);
    if (ret != Status::OK) {
        AVCODEC_LOGE("InitContext failed, ret=%{public}d", ret);
        return ret;
    }

    int32_t blockAlign = 1;
    if (meta->GetData(Tag::AUDIO_BLOCK_ALIGN, blockAlign)) {
        base_->SetBlockAlignContext(blockAlign);
        AVCODEC_LOGI("set audio_block_align:%{public}d", blockAlign);
    } else {
        AVCODEC_LOGE("audio_block_align is not set");
        return Status::ERROR_INVALID_PARAMETER;
    }

    auto fmt = base_->GetFormat();
    fmt->SetData(Tag::AUDIO_MAX_INPUT_SIZE, GetInputBufferSize());
    fmt->SetData(Tag::AUDIO_MAX_OUTPUT_SIZE, GetOutputBufferSize());
    fmt->SetData(Tag::MIME_TYPE,
        (ffCodecName_ == "wmav1" ? MimeType::AUDIO_WMAV1 :
         ffCodecName_ == "wmav2" ? MimeType::AUDIO_WMAV2 :
                                   MimeType::AUDIO_WMAPRO));

    return base_->OpenContext();
}

Status FFmpegWMADecoderPlugin::GetParameter(std::shared_ptr<Meta> &meta)
{
    meta = base_->GetFormat();
    return Status::OK;
}

Status FFmpegWMADecoderPlugin::QueueInputBuffer(const std::shared_ptr<AVBuffer> &in)
{
    return base_->ProcessSendData(in);
}

Status FFmpegWMADecoderPlugin::QueueOutputBuffer(std::shared_ptr<AVBuffer> &out)
{
    return base_->ProcessReceiveData(out);
}

bool FFmpegWMADecoderPlugin::CheckFormat(const std::shared_ptr<Meta> &meta)
{
    return CheckChannelCount(meta) && CheckSampleRate(meta);
}

bool FFmpegWMADecoderPlugin::CheckChannelCount(const std::shared_ptr<Meta> &meta)
{
    if (!meta->GetData(Tag::AUDIO_CHANNEL_COUNT, channels_)) {
        AVCODEC_LOGE("parameter channel_count missing");
        return false;
    }

    int maxChannels = 0;
    if (ffCodecName_ == "wmav1" || ffCodecName_ == "wmav2") {
        maxChannels = MAX_CHANNELS_WMA_LEGACY;
    } else { // wmapro
        maxChannels = MAX_CHANNELS_WMAPRO;
    }

    if (channels_ < MIN_CHANNELS || channels_ > maxChannels) {
        AVCODEC_LOGE("channel_count invalid: %{public}d (max=%{public}d for %{public}s)",
            channels_, maxChannels, ffCodecName_.c_str());
        return false;
    }
    return true;
}

bool FFmpegWMADecoderPlugin::CheckSampleRate(const std::shared_ptr<Meta> &meta) const
{
    int32_t sr = 0;
    if (!meta->GetData(Tag::AUDIO_SAMPLE_RATE, sr) || sr <= 0) {
        AVCODEC_LOGE("parameter sample_rate missing/invalid");
        return false;
    }

    int maxSr = 0;
    if (ffCodecName_ == "wmav1" || ffCodecName_ == "wmav2") {
        maxSr = MAX_SR_WMA_LEGACY;
    } else { // wmapro
        maxSr = MAX_SR_WMAPRO;
    }

    if (sr > maxSr) {
        AVCODEC_LOGE("sample_rate %{public}d exceeds max %{public}d for %{public}s",
            sr, maxSr, ffCodecName_.c_str());
        return false;
    }
    return true;
}

bool FFmpegWMADecoderPlugin::CheckSampleFormat(const std::shared_ptr<Meta> &meta)
{
    return base_->CheckSampleFormat(meta, channels_);
}

int32_t FFmpegWMADecoderPlugin::GetInputBufferSize()
{
    int32_t maxSize = base_->GetMaxInputSize();
    if (maxSize <= 0 || maxSize > INPUT_BUFFER_SIZE_DEFAULT) {
        maxSize = INPUT_BUFFER_SIZE_DEFAULT;
    }
    return maxSize;
}

int32_t FFmpegWMADecoderPlugin::GetOutputBufferSize()
{
    return OUTPUT_BUFFER_SIZE_DEFAULT;
}

} // namespace Ffmpeg
} // namespace Plugins
} // namespace Media
} // namespace OHOS