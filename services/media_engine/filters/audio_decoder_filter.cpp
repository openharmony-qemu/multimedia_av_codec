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

#include "audio_decoder_filter.h"
#include "filter/filter_factory.h"
#include "common/media_core.h"

namespace OHOS {
namespace Media {
namespace Pipeline {
using namespace OHOS::Media::Plugins;
static AutoRegisterFilter<AudioDecoderFilter> g_registerAudioDecoderFilter("builtin.player.audiodecoder",
    FilterType::FILTERTYPE_ADEC, [](const std::string& name, const FilterType type) {
        return std::make_shared<AudioDecoderFilter>(name, FilterType::FILTERTYPE_ADEC);
    });

class AudioDecoderFilterLinkCallback : public FilterLinkCallback {
public:
    explicit AudioDecoderFilterLinkCallback(std::shared_ptr<AudioDecoderFilter> codecFilter)
        : codecFilter_(codecFilter) {}

    ~AudioDecoderFilterLinkCallback() = default;

    void OnLinkedResult(const sptr<AVBufferQueueProducer> &queue, std::shared_ptr<Meta> &meta) override
    {
        if (auto codecFilter = codecFilter_.lock()) {
            codecFilter->OnLinkedResult(queue, meta);
        } else {
            MEDIA_LOG_I("invalid codecFilter");
        }
    }

    void OnUnlinkedResult(std::shared_ptr<Meta> &meta) override
    {
        if (auto codecFilter = codecFilter_.lock()) {
            codecFilter->OnUnlinkedResult(meta);
        } else {
            MEDIA_LOG_I("invalid codecFilter");
        }
    }

    void OnUpdatedResult(std::shared_ptr<Meta> &meta) override
    {
        if (auto codecFilter = codecFilter_.lock()) {
            codecFilter->OnUpdatedResult(meta);
        } else {
            MEDIA_LOG_I("invalid codecFilter");
        }
    }
private:
    std::weak_ptr<AudioDecoderFilter> codecFilter_;
};

class CodecBrokerListener : public IBrokerListener {
public:
    explicit CodecBrokerListener(std::shared_ptr<AudioDecoderFilter> codecFilter)
        : codecFilter_(codecFilter) {}

    sptr<IRemoteObject> AsObject() override
    {
        return nullptr;
    }

    void OnBufferFilled(std::shared_ptr<AVBuffer> &avBuffer) override
    {
        if (auto codecFilter = codecFilter_.lock()) {
            codecFilter->OnBufferFilled(avBuffer);
        } else {
            MEDIA_LOG_I("invalid codecFilter");
        }
    }

private:
    std::weak_ptr<AudioDecoderFilter> codecFilter_;
};

AudioDecoderFilter::AudioDecoderFilter(std::string name, FilterType type): Filter(name, type)
{
    filterType_ = type;
    MEDIA_LOG_I("audio decoder filter create");
}

AudioDecoderFilter::~AudioDecoderFilter()
{
    mediaCodec_->Release();
    MEDIA_LOG_I("audio decoder filter destroy");
}

void AudioDecoderFilter::Init(const std::shared_ptr<EventReceiver> &receiver,
    const std::shared_ptr<FilterCallback> &callback)
{
    MEDIA_LOG_I("AudioDecoderFilter::Init.");
    eventReceiver_ = receiver;
    filterCallback_ = callback;
    mediaCodec_ = std::make_shared<MediaCodec>();
}

Status AudioDecoderFilter::Prepare()
{
    MEDIA_LOG_I("AudioDecoderFilter::Prepare.");
    switch (filterType_) {
        case FilterType::FILTERTYPE_AENC:
            MEDIA_LOG_I("AudioDecoderFilter::FILTERTYPE_AENC.");
            filterCallback_->OnCallback(shared_from_this(), FilterCallBackCommand::NEXT_FILTER_NEEDED,
                StreamType::STREAMTYPE_ENCODED_AUDIO);
            break;
        case FilterType::FILTERTYPE_ADEC:
            filterCallback_->OnCallback(shared_from_this(), FilterCallBackCommand::NEXT_FILTER_NEEDED,
                StreamType::STREAMTYPE_RAW_AUDIO);
            break;
        case FilterType::FILTERTYPE_VENC:
            filterCallback_->OnCallback(shared_from_this(), FilterCallBackCommand::NEXT_FILTER_NEEDED,
                StreamType::STREAMTYPE_ENCODED_VIDEO);
            break;
        case FilterType::FILTERTYPE_VDEC:
            filterCallback_->OnCallback(shared_from_this(), FilterCallBackCommand::NEXT_FILTER_NEEDED,
                StreamType::STREAMTYPE_RAW_VIDEO);
            break;
        default:
            break;
    }
    return Filter::Prepare();
}

Status AudioDecoderFilter::Start()
{
    MEDIA_LOG_E("AudioDecoderFilter::Start.");
    Filter::Start();
    return (Status)mediaCodec_->Start();
}

Status AudioDecoderFilter::Pause()
{
    MEDIA_LOG_E("AudioDecoderFilter::Pause.");
    latestPausedTime_ = latestBufferTime_;
    return Filter::Pause();
}

Status AudioDecoderFilter::Resume()
{
    MEDIA_LOG_E("AudioDecoderFilter::Resume.");
    refreshTotalPauseTime_ = true;
    Filter::Resume();
    return (Status)mediaCodec_->Start();
}

Status AudioDecoderFilter::Stop()
{
    MEDIA_LOG_E("AudioDecoderFilter::Stop.");
    latestBufferTime_ = HST_TIME_NONE;
    latestPausedTime_ = HST_TIME_NONE;
    totalPausedTime_ = 0;
    refreshTotalPauseTime_ = false;
    Filter::Stop();
    return (Status)mediaCodec_->Stop();
}

Status AudioDecoderFilter::Flush()
{
    MEDIA_LOG_E("AudioDecoderFilter::Flush.");
    return (Status)mediaCodec_->Flush();
}

Status AudioDecoderFilter::Release()
{
    MEDIA_LOG_E("AudioDecoderFilter::Release.");
    return (Status)mediaCodec_->Release();
}

void AudioDecoderFilter::SetParameter(const std::shared_ptr<Meta> &parameter)
{
    MEDIA_LOG_E("AudioDecoderFilter::SetParameter.");
    mediaCodec_->SetParameter(parameter);
}

void AudioDecoderFilter::GetParameter(std::shared_ptr<Meta> &parameter)
{
    mediaCodec_->GetOutputFormat(parameter);
}

Status AudioDecoderFilter::LinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType)
{
    MEDIA_LOG_E("AudioDecoderFilter::LinkNext.");
    nextFilter_ = nextFilter;
    nextFiltersMap_[outType].push_back(nextFilter_);
    std::shared_ptr<FilterLinkCallback> filterLinkCallback =
        std::make_shared<AudioDecoderFilterLinkCallback>(shared_from_this());
    nextFilter->OnLinked(outType, meta_, filterLinkCallback);
    return Status::OK;
}

Status AudioDecoderFilter::UpdateNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType)
{
    return Status::OK;
}

Status AudioDecoderFilter::UnLinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType)
{
    return Status::OK;
}

FilterType AudioDecoderFilter::GetFilterType()
{
    return filterType_;
}

Status AudioDecoderFilter::OnLinked(StreamType inType, const std::shared_ptr<Meta> &meta,
    const std::shared_ptr<FilterLinkCallback> &callback)
{
    MEDIA_LOG_I("AudioDecoderFilter::OnLinked.");
    onLinkedResultCallback_ = callback;
    meta_ = meta;
    std::string mime;
    bool mimeGetRes = meta_->GetData(Tag::MIME_TYPE, mime);
    if (!mimeGetRes && eventReceiver_ != nullptr) {
        MEDIA_LOG_I("AudioDecoderFilter cannot get mime");
        eventReceiver_->OnEvent({"audioDecoder", EventType::EVENT_ERROR, MSERR_UNSUPPORT_AUD_DEC_TYPE});
        return Status::ERROR_UNSUPPORTED_FORMAT;
    }
    meta->SetData(Tag::AUDIO_SAMPLE_FORMAT, Plugins::SAMPLE_S16LE);
    SetParameter(meta);
    mediaCodec_->Init(mime, false);
    auto ret = mediaCodec_->Configure(meta);
    if (ret != (int32_t)Status::OK && ret != (int32_t)Status::ERROR_INVALID_STATE) {
        MEDIA_LOG_I("AudioDecoderFilter unsupport format");
        eventReceiver_->OnEvent({"audioDecoder", EventType::EVENT_ERROR, MSERR_UNSUPPORT_AUD_DEC_TYPE});
        return Status::ERROR_UNSUPPORTED_FORMAT;
    }
    return Status::OK;
}

Status AudioDecoderFilter::OnUpdated(StreamType inType, const std::shared_ptr<Meta> &meta,
    const std::shared_ptr<FilterLinkCallback> &callback)
{
    return Status::OK;
}

Status AudioDecoderFilter::OnUnLinked(StreamType inType, const std::shared_ptr<FilterLinkCallback>& callback)
{
    return Status::OK;
}

sptr<AVBufferQueueProducer> AudioDecoderFilter::GetInputBufferQueue()
{
    MEDIA_LOG_E("AudioDecoderFilter::GetInputBufferQueue.");
    inputBufferQueueProducer_ = mediaCodec_->GetInputBufferQueue();
    sptr<IBrokerListener> listener = new CodecBrokerListener(shared_from_this());
    FALSE_RETURN_V(inputBufferQueueProducer_ != nullptr, sptr<AVBufferQueueProducer>());
    inputBufferQueueProducer_->SetBufferFilledListener(listener);
    return inputBufferQueueProducer_;
}

void AudioDecoderFilter::OnLinkedResult(const sptr<AVBufferQueueProducer> &outputBufferQueue,
    std::shared_ptr<Meta> &meta)
{
    MEDIA_LOG_E("AudioDecoderFilter::OnLinkedResult.");
    FALSE_RETURN(mediaCodec_ != nullptr);
    mediaCodec_->SetOutputBufferQueue(outputBufferQueue);
    mediaCodec_->Prepare();
    inputBufferQueueProducer_ = mediaCodec_->GetInputBufferQueue();
    FALSE_RETURN(inputBufferQueueProducer_ != nullptr);
    sptr<IBrokerListener> listener = new CodecBrokerListener(shared_from_this());
    inputBufferQueueProducer_->SetBufferFilledListener(listener);
    FALSE_RETURN(onLinkedResultCallback_ != nullptr);
    onLinkedResultCallback_->OnLinkedResult(inputBufferQueueProducer_, meta);
}

void AudioDecoderFilter::OnUpdatedResult(std::shared_ptr<Meta> &meta)
{
    meta_ = meta;
}

void AudioDecoderFilter::OnUnlinkedResult(std::shared_ptr<Meta> &meta)
{
    meta_ = meta;
}

void AudioDecoderFilter::OnBufferFilled(std::shared_ptr<AVBuffer> &inputBuffer)
{
    MEDIA_LOG_D("AudioDecoderFilter::OnBufferFilled. pts: %{public}" PRId64,
            (inputBuffer == nullptr ? -1 : inputBuffer->pts_));
    FALSE_RETURN(inputBufferQueueProducer_ != nullptr);
    FALSE_RETURN(inputBuffer != nullptr);
    inputBufferQueueProducer_->ReturnBuffer(inputBuffer, true);
}
} // namespace Pipeline
} // namespace MEDIA
} // namespace OHOS