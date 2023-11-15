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

#include "codec_filter.h"
#include "filter/filter_factory.h"

namespace OHOS {
namespace Media {
namespace Pipeline {
static AutoRegisterFilter<CodecFilter> g_registerAudioEncoderFilter("builtin.recorder.audioencoder", FilterType::FILTERTYPE_AENC, 
    [](const std::string& name, const FilterType type) {return std::make_shared<CodecFilter>(name, FilterType::FILTERTYPE_AENC); });
static AutoRegisterFilter<CodecFilter> g_registerAudioDecoderFilter("builtin.recorder.audiodecoder", FilterType::FILTERTYPE_ADEC, 
    [](const std::string& name, const FilterType type) {return std::make_shared<CodecFilter>(name, FilterType::FILTERTYPE_ADEC); });
static AutoRegisterFilter<CodecFilter> g_registerVideoEncoderFilter("builtin.recorder.videoencoder", FilterType::FILTERTYPE_VENC, 
    [](const std::string& name, const FilterType type) {return std::make_shared<CodecFilter>(name, FilterType::FILTERTYPE_VENC); });
static AutoRegisterFilter<CodecFilter> g_registerVideoDecoderFilter("builtin.recorder.videodecoder", FilterType::FILTERTYPE_VDEC, 
    [](const std::string& name, const FilterType type) {return std::make_shared<CodecFilter>(name, FilterType::FILTERTYPE_VDEC); });

class CodecFilterLinkCallback : public FilterLinkCallback {
public:
    CodecFilterLinkCallback(std::shared_ptr<CodecFilter> codecFilter) {
        codecFilter_ = codecFilter;
    }

    ~CodecFilterLinkCallback() = default;

    void OnLinkedResult(const sptr<AVBufferQueueProducer> &queue, std::shared_ptr<Meta> &meta) override {
        codecFilter_->OnLinkedResult(queue, meta);
    }

    void OnUnlinkedResult(std::shared_ptr<Meta> &meta) override {
        codecFilter_->OnUnlinkedResult(meta);
    }

    void OnUpdatedResult(std::shared_ptr<Meta> &meta) override {
        codecFilter_->OnUpdatedResult(meta);
    }
private:
    std::shared_ptr<CodecFilter> codecFilter_;
};

class CodecBrokerListener : public IBrokerListener {
public:
    CodecBrokerListener(std::shared_ptr<CodecFilter> codecFilter) {
        codecFilter_ = codecFilter;
    }

    sptr<IRemoteObject> AsObject() override {  
        return nullptr;    
    }

    void OnBufferFilled(std::shared_ptr<AVBuffer> &avBuffer) override {
        codecFilter_->OnBufferFilled(avBuffer);
    }

private:
    std::shared_ptr<CodecFilter> codecFilter_;

};

CodecFilter::CodecFilter(std::string name, FilterType type): Filter(name, type) { 
    filterType_ = type; 
}

CodecFilter::~CodecFilter()
{
}

void CodecFilter::Init(const std::shared_ptr<EventReceiver> &receiver, const std::shared_ptr<FilterCallback> &callback) {
    MEDIA_LOG_I("CodecFilter::Init.");
    eventReceiver_ = receiver;
    filterCallback_ = callback;
    mediaCodec_ = std::make_shared<MediaCodec>();
    mediaCodec_->Init("mediacodec");
}

void CodecFilter::SetCodecFormat(const std::shared_ptr<Meta> &format, bool isEncoder) {
    
}

sptr<Surface> CodecFilter::GetInputSurface() {
    return mediaCodec_->GetInputSurface();
}

Status CodecFilter::Prepare() {
    MEDIA_LOG_I("CodecFilter::Prepare.");
    switch (filterType_) {
        case FilterType::FILTERTYPE_AENC:
            MEDIA_LOG_I("CodecFilter::FILTERTYPE_AENC.");
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
    return Status::OK;
}

Status CodecFilter::Start() {
    MEDIA_LOG_E("CodecFilter::Start.");
    return mediaCodec_->Start();
}

Status CodecFilter::Pause() {
    MEDIA_LOG_E("CodecFilter::Pause.");
    latestPausedTime_ = latestBufferTime_;
    return mediaCodec_->Stop();
}

Status CodecFilter::Resume() {
    MEDIA_LOG_E("CodecFilter::Resume.");
    refreshTotalPauseTime_ = true;
    return mediaCodec_->Start();
}

Status CodecFilter::Stop() {
    MEDIA_LOG_E("CodecFilter::Stop.");
    latestBufferTime_ = TIME_NONE;
    latestPausedTime_ = TIME_NONE;
    totalPausedTime_ = 0;
    refreshTotalPauseTime_ = false;
    return mediaCodec_->Stop();
}

Status CodecFilter::Flush() {
    MEDIA_LOG_E("CodecFilter::Flush.");
    return mediaCodec_->Flush();
}

Status CodecFilter::Release() {
    MEDIA_LOG_E("CodecFilter::Release.");
    return mediaCodec_->Release();
}

void CodecFilter::SetParameter(const std::shared_ptr<Meta> &parameter) {
    MEDIA_LOG_E("CodecFilter::SetParameter.");
    mediaCodec_->SetParameter(parameter);
}

void CodecFilter::GetParameter(std::shared_ptr<Meta> &parameter) {
}

Status CodecFilter::LinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType) {
    MEDIA_LOG_E("CodecFilter::LinkNext.");
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    switch (nextFilter->GetFilterType()) {
        case FilterType::FILTERTYPE_MUXER:
            if (filterType_ == FilterType::FILTERTYPE_AENC) {

            } else {

            }
            break;
        case FilterType::FILTERTYPE_ASINK:
            break;
        case FilterType::FILTERTYPE_FSINK:
            break;
        default:
            break;
    }
    std::shared_ptr<FilterLinkCallback> filterLinkCallback = std::make_shared<CodecFilterLinkCallback>(shared_from_this());
    nextFilter->OnLinked(outType, meta, filterLinkCallback);
    nextFilter->Prepare();
    return Status::OK;
}

Status CodecFilter::UpdateNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType) {
    return Status::OK;
}

Status CodecFilter::UnLinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType) {
    return Status::OK;
}

FilterType CodecFilter::GetFilterType() {
    return filterType_;
}

Status CodecFilter::OnLinked(StreamType inType, const std::shared_ptr<Meta> &meta, const std::shared_ptr<FilterLinkCallback> &callback) {
    MEDIA_LOG_E("CodecFilter::OnLinked.");
    onLinkedResultCallback_ = callback;
    mediaCodec_->Configure(meta);
    return Status::OK;
}

Status CodecFilter::OnUpdated(StreamType inType, const std::shared_ptr<Meta> &meta,
                                const std::shared_ptr<FilterLinkCallback> &callback) {
    return Status::OK;
}

Status CodecFilter::OnUnLinked(StreamType inType, const std::shared_ptr<FilterLinkCallback>& callback) {
    return Status::OK;
}

sptr<AVBufferQueueProducer> CodecFilter::GetInputBufferQueue() {
    MEDIA_LOG_E("CodecFilter::GetInputBufferQueue.");
    inputBufferQueueProducer_ = mediaCodec_->GetInputBufferQueue();
    sptr<IBrokerListener> listener = new CodecBrokerListener(shared_from_this());
    inputBufferQueueProducer_->SetBufferFilledListener(listener);
    return inputBufferQueueProducer_;
}

void CodecFilter::OnLinkedResult(const sptr<AVBufferQueueProducer> &outputBufferQueue, std::shared_ptr<Meta> &meta) {
    MEDIA_LOG_E("CodecFilter::OnLinkedResult.");
    mediaCodec_->SetOutputBufferQueue(outputBufferQueue);
    mediaCodec_->Prepare();
    onLinkedResultCallback_->OnLinkedResult(mediaCodec_->GetInputBufferQueue(), meta);
}

void CodecFilter::OnUpdatedResult(std::shared_ptr<Meta> &meta) {
}

void CodecFilter::OnUnlinkedResult(std::shared_ptr<Meta> &meta) {
}

void CodecFilter::OnBufferFilled(std::shared_ptr<AVBuffer> &inputBuffer) {
    MEDIA_LOG_E("CodecFilter::OnBufferFilled.");
//        latestBufferTime_ = bufferPtr->pts;
    if (refreshTotalPauseTime_) {
        if (latestPausedTime_ != TIME_NONE && latestBufferTime_ > latestPausedTime_) {
            totalPausedTime_ += latestBufferTime_ - latestPausedTime_;
        }
        refreshTotalPauseTime_ = false;
    }
//        bufferPtr->pts -= totalPausedTime_;
}

} //namespace Pipeline
} //namespace MEDIA
} //namespace OHOS