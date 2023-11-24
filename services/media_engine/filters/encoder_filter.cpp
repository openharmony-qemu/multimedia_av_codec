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

#include "encoder_filter.h"
#include "filter/filter_factory.h"

namespace OHOS {
namespace Media {
namespace Pipeline {
static AutoRegisterFilter<EncoderFilter> g_registerAudioEncoderFilter("builtin.recorder.audioencoder", FilterType::FILTERTYPE_AENC, 
    [](const std::string& name, const FilterType type) {return std::make_shared<EncoderFilter>(name, FilterType::FILTERTYPE_AENC); });
static AutoRegisterFilter<EncoderFilter> g_registerVideoEncoderFilter("builtin.recorder.videoencoder", FilterType::FILTERTYPE_VENC, 
    [](const std::string& name, const FilterType type) {return std::make_shared<EncoderFilter>(name, FilterType::FILTERTYPE_VENC); });

class CodecFilterLinkCallback : public FilterLinkCallback {
public:
    CodecFilterLinkCallback(std::shared_ptr<EncoderFilter> encoderFilter) {
        encoderFilter_ = encoderFilter;
    }

    ~CodecFilterLinkCallback() = default;

    void OnLinkedResult(const sptr<AVBufferQueueProducer> &queue, std::shared_ptr<Meta> &meta) override {
        MEDIA_LOG_I("OnLinkedResult enter.");
        encoderFilter_->OnLinkedResult(queue, meta);
    }

    void OnUnlinkedResult(std::shared_ptr<Meta> &meta) override {
        encoderFilter_->OnUnlinkedResult(meta);
    }

    void OnUpdatedResult(std::shared_ptr<Meta> &meta) override {
        encoderFilter_->OnUpdatedResult(meta);
    }
private:
    std::shared_ptr<EncoderFilter> encoderFilter_;
};

class CodecBrokerListener : public IBrokerListener {
public:
    CodecBrokerListener(std::shared_ptr<EncoderFilter> encoderFilter) {
        encoderFilter_ = encoderFilter;
    }

    sptr<IRemoteObject> AsObject() override {  
        return nullptr;    
    }

    void OnBufferFilled(std::shared_ptr<AVBuffer> &avBuffer) override {
        encoderFilter_->OnBufferFilled(avBuffer);
    }

private:
    std::shared_ptr<EncoderFilter> encoderFilter_;

};

EncoderFilter::EncoderFilter(std::string name, FilterType type): Filter(name, type) { 
    filterType_ = type; 
}

EncoderFilter::~EncoderFilter()
{
}

Status EncoderFilter::SetCodecFormat(const std::shared_ptr<Meta> &format) {
    MEDIA_LOG_I("SetCodecFormat enter.");
    FALSE_RETURN_V(format->Get<Tag::MIME_TYPE>(codecMimeType_), Status::ERROR_INVALID_PARAMETER);
    return Status::OK;
}

void EncoderFilter::Init(const std::shared_ptr<EventReceiver> &receiver, const std::shared_ptr<FilterCallback> &callback) {
    MEDIA_LOG_I("Init enter.");
    eventReceiver_ = receiver;
    filterCallback_ = callback;
    mediaCodec_ = std::make_shared<MediaCodec>();
    mediaCodec_->Init(codecMimeType_, true);
}

Status EncoderFilter::Configure(const std::shared_ptr<Meta> &parameter) {
    MEDIA_LOG_I("Configure enter.");
    configureParameter_ = parameter;
    return mediaCodec_->Configure(parameter);
}

sptr<Surface> EncoderFilter::GetInputSurface() {
    MEDIA_LOG_I("GetInputSurface enter.");
    return mediaCodec_->GetInputSurface();
}

Status EncoderFilter::Prepare() {
    MEDIA_LOG_I("Prepare enter.");
    switch (filterType_) {
        case FilterType::FILTERTYPE_AENC:
            filterCallback_->OnCallback(shared_from_this(), FilterCallBackCommand::NEXT_FILTER_NEEDED,
                                        StreamType::STREAMTYPE_ENCODED_AUDIO);
            break;
        case FilterType::FILTERTYPE_VENC:
            filterCallback_->OnCallback(shared_from_this(), FilterCallBackCommand::NEXT_FILTER_NEEDED,
                                        StreamType::STREAMTYPE_ENCODED_VIDEO);
            break;
        default:
            break;
    }
    return Status::OK;
}

Status EncoderFilter::Start() {
    MEDIA_LOG_I("Start enter.");
    nextFilter_->Start();
    return mediaCodec_->Start();
}

Status EncoderFilter::Pause() {
    MEDIA_LOG_I("Pause enter.");
    latestPausedTime_ = latestBufferTime_;
    return mediaCodec_->Stop();
}

Status EncoderFilter::Resume() {
    MEDIA_LOG_I("Resume enter.");
    refreshTotalPauseTime_ = true;
    return mediaCodec_->Start();
}

Status EncoderFilter::Stop() {
    MEDIA_LOG_I("Stop enter.");
    latestBufferTime_ = TIME_NONE;
    latestPausedTime_ = TIME_NONE;
    totalPausedTime_ = 0;
    refreshTotalPauseTime_ = false;
    mediaCodec_->Stop();
    nextFilter_->Stop();
    return Status::OK;
}

Status EncoderFilter::Flush() {
    MEDIA_LOG_I("Flush enter.");
    return mediaCodec_->Flush();
}

Status EncoderFilter::Release() {
    MEDIA_LOG_I("Release enter.");
    return mediaCodec_->Release();
}

Status EncoderFilter::NotifyEOS() {
    return mediaCodec_->NotifyEOS();
}

void EncoderFilter::SetParameter(const std::shared_ptr<Meta> &parameter) {
    MEDIA_LOG_I("SetParameter enter.");
    mediaCodec_->SetParameter(parameter);
}

void EncoderFilter::GetParameter(std::shared_ptr<Meta> &parameter) {
}

Status EncoderFilter::LinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType) {
    MEDIA_LOG_I("LinkNext enter.");
    nextFilter_ = nextFilter;
    std::shared_ptr<FilterLinkCallback> filterLinkCallback = std::make_shared<CodecFilterLinkCallback>(shared_from_this());
    nextFilter->OnLinked(outType, configureParameter_, filterLinkCallback);
    nextFilter->Prepare();
    return Status::OK;
}

Status EncoderFilter::UpdateNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType) {
    return Status::OK;
}

Status EncoderFilter::UnLinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType) {
    return Status::OK;
}

FilterType EncoderFilter::GetFilterType() {
    return filterType_;
}

Status EncoderFilter::OnLinked(StreamType inType, const std::shared_ptr<Meta> &meta, const std::shared_ptr<FilterLinkCallback> &callback) {
    MEDIA_LOG_I("OnLinked enter.");
    onLinkedResultCallback_ = callback;
    return Status::OK;
}

Status EncoderFilter::OnUpdated(StreamType inType, const std::shared_ptr<Meta> &meta,
                                const std::shared_ptr<FilterLinkCallback> &callback) {
    return Status::OK;
}

Status EncoderFilter::OnUnLinked(StreamType inType, const std::shared_ptr<FilterLinkCallback>& callback) {
    return Status::OK;
}

void EncoderFilter::OnLinkedResult(const sptr<AVBufferQueueProducer> &outputBufferQueue, std::shared_ptr<Meta> &meta) {
    MEDIA_LOG_I("OnLinkedResult enter.");
    mediaCodec_->SetOutputBufferQueue(outputBufferQueue);
    sptr<IBrokerListener> listener = new CodecBrokerListener(shared_from_this());
    outputBufferQueue->SetBufferFilledListener(listener);
    outputBufferQueueProducer_ = outputBufferQueue;
    mediaCodec_->Prepare();
    onLinkedResultCallback_->OnLinkedResult(mediaCodec_->GetInputBufferQueue(), meta);
}

void EncoderFilter::OnUpdatedResult(std::shared_ptr<Meta> &meta) {
}

void EncoderFilter::OnUnlinkedResult(std::shared_ptr<Meta> &meta) {
}

void EncoderFilter::OnBufferFilled(std::shared_ptr<AVBuffer> &inputBuffer) {
    MEDIA_LOG_I("OnBufferFilled enter.");
    latestBufferTime_ = inputBuffer->pts_;
    if (refreshTotalPauseTime_) {
        if (latestPausedTime_ != TIME_NONE && latestBufferTime_ > latestPausedTime_) {
            totalPausedTime_ += latestBufferTime_ - latestPausedTime_;
        }
        refreshTotalPauseTime_ = false;
    }
    inputBuffer->pts_ -= totalPausedTime_;
    outputBufferQueueProducer_->ReturnBuffer(inputBuffer, true);
}

} //namespace Pipeline
} //namespace MEDIA
} //namespace OHOS