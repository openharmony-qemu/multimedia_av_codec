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

#include "audio_encoder_filter.h"
#include "filter/filter_factory.h"
#include "media_codec/media_codec.h"

namespace OHOS {
namespace Media {
namespace Pipeline {
static AutoRegisterFilter<AudioEncoderFilter> g_registerAudioEncoderFilter("builtin.recorder.audioencoder",
    FilterType::FILTERTYPE_AENC,
    [](const std::string& name, const FilterType type) {
        return std::make_shared<AudioEncoderFilter>(name, FilterType::FILTERTYPE_AENC);
    });

class AudioEncoderFilterLinkCallback : public FilterLinkCallback {
public:
    explicit AudioEncoderFilterLinkCallback(std::shared_ptr<AudioEncoderFilter> audioEncoderFilter)
        : audioEncoderFilter_(std::move(audioEncoderFilter))
    {
    }

    ~AudioEncoderFilterLinkCallback() = default;

    void OnLinkedResult(const sptr<AVBufferQueueProducer> &queue, std::shared_ptr<Meta> &meta) override
    {
        if (auto encoderFilter = audioEncoderFilter_.lock()) {
            encoderFilter->OnLinkedResult(queue, meta);
        } else {
            MEDIA_LOG_I("invalid encoderFilter");
        }
    }

    void OnUnlinkedResult(std::shared_ptr<Meta> &meta) override
    {
        if (auto encoderFilter = audioEncoderFilter_.lock()) {
            encoderFilter->OnUnlinkedResult(meta);
        } else {
            MEDIA_LOG_I("invalid encoderFilter");
        }
    }

    void OnUpdatedResult(std::shared_ptr<Meta> &meta) override
    {
        if (auto encoderFilter = audioEncoderFilter_.lock()) {
            encoderFilter->OnUpdatedResult(meta);
        } else {
            MEDIA_LOG_I("invalid encoderFilter");
        }
    }
private:
    std::weak_ptr<AudioEncoderFilter> audioEncoderFilter_;
};

AudioEncoderFilter::AudioEncoderFilter(std::string name, FilterType type): Filter(name, type)
{
    filterType_ = type;
    MEDIA_LOG_I(PUBLIC_LOG_S "audio encoder filter create", logTag_.c_str());
}

AudioEncoderFilter::~AudioEncoderFilter()
{
    MEDIA_LOG_I(PUBLIC_LOG_S "audio encoder filter destroy", logTag_.c_str());
}

Status AudioEncoderFilter::SetCodecFormat(const std::shared_ptr<Meta> &format)
{
    MEDIA_LOG_I(PUBLIC_LOG_S "SetCodecFormat", logTag_.c_str());
    FALSE_RETURN_V(format->Get<Tag::MIME_TYPE>(codecMimeType_), Status::ERROR_INVALID_PARAMETER);
    return Status::OK;
}

void AudioEncoderFilter::Init(const std::shared_ptr<EventReceiver> &receiver,
    const std::shared_ptr<FilterCallback> &callback)
{
    MEDIA_LOG_I(PUBLIC_LOG_S "Init", logTag_.c_str());
    eventReceiver_ = receiver;
    filterCallback_ = callback;
    mediaCodec_ = std::make_shared<MediaCodec>();
    mediaCodec_->Init(codecMimeType_, true);
}

void AudioEncoderFilter::SetLogTag(std::string logTag)
{
    logTag_ = std::move(logTag);
}

Status AudioEncoderFilter::Configure(const std::shared_ptr<Meta> &parameter)
{
    MEDIA_LOG_I(PUBLIC_LOG_S "Configure", logTag_.c_str());
    configureParameter_ = parameter;
    int32_t ret = mediaCodec_->Configure(parameter);
    if (ret != 0) {
        return Status::ERROR_UNKNOWN;
    }
    return Status::OK;
}

sptr<Surface> AudioEncoderFilter::GetInputSurface()
{
    MEDIA_LOG_I(PUBLIC_LOG_S "GetInputSurface", logTag_.c_str());
    return mediaCodec_->GetInputSurface();
}

Status AudioEncoderFilter::Prepare()
{
    MEDIA_LOG_I(PUBLIC_LOG_S "Prepare", logTag_.c_str());
    switch (filterType_) {
        case FilterType::FILTERTYPE_AENC:
            filterCallback_->OnCallback(shared_from_this(), FilterCallBackCommand::NEXT_FILTER_NEEDED,
                StreamType::STREAMTYPE_ENCODED_AUDIO);
            break;
        default:
            break;
    }
    return Status::OK;
}

Status AudioEncoderFilter::Start()
{
    MEDIA_LOG_I(PUBLIC_LOG_S "Start", logTag_.c_str());
    Status status = nextFilter_->Start();
    if (status != Status::OK) {
        return status;
    }
    int32_t ret = mediaCodec_->Start();
    if (ret != 0) {
        return Status::ERROR_UNKNOWN;
    }
    return Status::OK;
}

Status AudioEncoderFilter::Pause()
{
    MEDIA_LOG_I(PUBLIC_LOG_S "Pause", logTag_.c_str());
    return Status::OK;
}

Status AudioEncoderFilter::Resume()
{
    MEDIA_LOG_I(PUBLIC_LOG_S "Resume", logTag_.c_str());
    return Status::OK;
}

Status AudioEncoderFilter::Stop()
{
    MEDIA_LOG_I(PUBLIC_LOG_S "Stop", logTag_.c_str());
    Status status = nextFilter_->Stop();
    if (status != Status::OK) {
        return status;
    }
    int32_t ret = mediaCodec_->Stop();
    if (ret != 0) {
        return Status::ERROR_UNKNOWN;
    }
    return Status::OK;
}

Status AudioEncoderFilter::Flush()
{
    MEDIA_LOG_I(PUBLIC_LOG_S "Flush", logTag_.c_str());
    int32_t ret = mediaCodec_->Flush();
    if (ret != 0) {
        return Status::ERROR_UNKNOWN;
    }
    return Status::OK;
}

Status AudioEncoderFilter::Release()
{
    MEDIA_LOG_I(PUBLIC_LOG_S "Release", logTag_.c_str());
    int32_t ret = mediaCodec_->Release();
    if (ret != 0) {
        return Status::ERROR_UNKNOWN;
    }
    return Status::OK;
}

Status AudioEncoderFilter::NotifyEos()
{
    MEDIA_LOG_I(PUBLIC_LOG_S "NotifyEos", logTag_.c_str());
    int32_t ret = mediaCodec_->NotifyEos();
    if (ret != 0) {
        return Status::ERROR_UNKNOWN;
    }
    return Status::OK;
}

void AudioEncoderFilter::SetParameter(const std::shared_ptr<Meta> &parameter)
{
    MEDIA_LOG_I(PUBLIC_LOG_S "SetParameter", logTag_.c_str());
    mediaCodec_->SetParameter(parameter);
}

void AudioEncoderFilter::GetParameter(std::shared_ptr<Meta> &parameter)
{
    MEDIA_LOG_I(PUBLIC_LOG_S "GetParameter", logTag_.c_str());
}

Status AudioEncoderFilter::LinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType)
{
    MEDIA_LOG_I(PUBLIC_LOG_S "LinkNext", logTag_.c_str());
    nextFilter_ = nextFilter;
    std::shared_ptr<FilterLinkCallback> filterLinkCallback =
        std::make_shared<AudioEncoderFilterLinkCallback>(shared_from_this());
    auto ret = nextFilter->OnLinked(outType, configureParameter_, filterLinkCallback);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, PUBLIC_LOG_S "OnLinked failed", logTag_.c_str());
    nextFilter->Prepare();
    return Status::OK;
}

Status AudioEncoderFilter::UpdateNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType)
{
    MEDIA_LOG_I(PUBLIC_LOG_S "UpdateNext", logTag_.c_str());
    return Status::OK;
}

Status AudioEncoderFilter::UnLinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType)
{
    MEDIA_LOG_I(PUBLIC_LOG_S "UnLinkNext", logTag_.c_str());
    return Status::OK;
}

FilterType AudioEncoderFilter::GetFilterType()
{
    MEDIA_LOG_I(PUBLIC_LOG_S "GetFilterType", logTag_.c_str());
    return filterType_;
}

Status AudioEncoderFilter::OnLinked(StreamType inType, const std::shared_ptr<Meta> &meta,
    const std::shared_ptr<FilterLinkCallback> &callback)
{
    MEDIA_LOG_I(PUBLIC_LOG_S "OnLinked", logTag_.c_str());
    onLinkedResultCallback_ = callback;
    return Status::OK;
}

Status AudioEncoderFilter::OnUpdated(StreamType inType, const std::shared_ptr<Meta> &meta,
    const std::shared_ptr<FilterLinkCallback> &callback)
{
    MEDIA_LOG_I(PUBLIC_LOG_S "OnUpdated", logTag_.c_str());
    return Status::OK;
}

Status AudioEncoderFilter::OnUnLinked(StreamType inType, const std::shared_ptr<FilterLinkCallback>& callback)
{
    MEDIA_LOG_I(PUBLIC_LOG_S "OnUnLinked", logTag_.c_str());
    return Status::OK;
}

void AudioEncoderFilter::OnLinkedResult(const sptr<AVBufferQueueProducer> &outputBufferQueue,
    std::shared_ptr<Meta> &meta)
{
    MEDIA_LOG_I(PUBLIC_LOG_S "OnLinkedResult", logTag_.c_str());
    mediaCodec_->SetOutputBufferQueue(outputBufferQueue);
    mediaCodec_->Prepare();
    onLinkedResultCallback_->OnLinkedResult(mediaCodec_->GetInputBufferQueue(), meta);
}

void AudioEncoderFilter::OnUpdatedResult(std::shared_ptr<Meta> &meta)
{
    MEDIA_LOG_I(PUBLIC_LOG_S "OnUpdatedResult", logTag_.c_str());
    (void) meta;
}

void AudioEncoderFilter::OnUnlinkedResult(std::shared_ptr<Meta> &meta)
{
    MEDIA_LOG_I(PUBLIC_LOG_S "OnUnlinkedResult", logTag_.c_str());
    (void) meta;
}
} // namespace Pipeline
} // namespace MEDIA
} // namespace OHOS