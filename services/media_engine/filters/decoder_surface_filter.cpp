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

#include "decoder_surface_filter.h"
#include "filter/filter_factory.h"
#include "avcodec_info.h"
#include "avcodec_common.h"
#include <time.h>

#define TIME_OUT_MS 8

namespace OHOS {
namespace Media {
namespace Pipeline {
static AutoRegisterFilter<DecoderSurfaceFilter> g_registerDecoderSurfaceFilter("builtin.player.videodecoder", FilterType::FILTERTYPE_VDEC,
    [](const std::string& name, const FilterType type) {return std::make_shared<DecoderSurfaceFilter>(name, FilterType::FILTERTYPE_VDEC); });

class DecoderSurfaceFilterLinkCallback : public FilterLinkCallback {
public:
    DecoderSurfaceFilterLinkCallback(std::shared_ptr<DecoderSurfaceFilter> decoderSurfaceFilter) {
        decoderSurfaceFilter_ = decoderSurfaceFilter;
    }

    ~DecoderSurfaceFilterLinkCallback() = default;

    void OnLinkedResult(const sptr<AVBufferQueueProducer> &queue, std::shared_ptr<Meta> &meta) override {
        MEDIA_LOG_I("OnLinkedResult enter.");
        decoderSurfaceFilter_->OnLinkedResult(queue, meta);
    }

    void OnUnlinkedResult(std::shared_ptr<Meta> &meta) override {
        decoderSurfaceFilter_->OnUnlinkedResult(meta);
    }

    void OnUpdatedResult(std::shared_ptr<Meta> &meta) override {
        decoderSurfaceFilter_->OnUpdatedResult(meta);
    }
private:
    std::shared_ptr<DecoderSurfaceFilter> decoderSurfaceFilter_;
};

class MediaCodecCallback : public OHOS::MediaAVCodec::VideoCodecCallback {
public:
    MediaCodecCallback(std::shared_ptr<DecoderSurfaceFilter> decoderSurfaceFilter) {
        decoderSurfaceFilter_ = decoderSurfaceFilter;
    }

    ~MediaCodecCallback() = default;

    void OnError(MediaAVCodec::AVCodecErrorType errorType, int32_t errorCode) {

    }

    void OnOutputFormatChanged(const MediaAVCodec::Format &format) {

    }

    void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) {
        decoderSurfaceFilter_->DrainInputBuffer(buffer);
    }

    void OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) {
        decoderSurfaceFilter_->DrainOutputBuffer(buffer);
    }

private:
    std::shared_ptr<DecoderSurfaceFilter> decoderSurfaceFilter_;
};

class AVBufferAvailableListener : public IConsumerListener {
public:
    AVBufferAvailableListener(std::shared_ptr<DecoderSurfaceFilter> decoderSurfaceFilter) {
        decoderSurfaceFilter_ = decoderSurfaceFilter;
    }

    void OnBufferAvailable() override {
        decoderSurfaceFilter_->UpdateAvailableInputBufferCount();
    }
private:
    std::shared_ptr<DecoderSurfaceFilter> decoderSurfaceFilter_;
};

DecoderSurfaceFilter::DecoderSurfaceFilter(std::string name, FilterType type): Filter(name, type) {
    mediaCodec_ = MediaAVCodec::CodecServer::Create();
    filterType_ = type;
}

DecoderSurfaceFilter::~DecoderSurfaceFilter()
{
    mediaCodec_->Release();
}

void DecoderSurfaceFilter::Init(const std::shared_ptr<EventReceiver> &receiver, const std::shared_ptr<FilterCallback> &callback) {
    MEDIA_LOG_I("Init enter.");
    eventReceiver_ = receiver;
    filterCallback_ = callback;
}

Status DecoderSurfaceFilter::Configure(const std::shared_ptr<Meta> &parameter) {
    MEDIA_LOG_I("Configure enter.");
    configureParameter_ = parameter;
    // meta need to transfer to format
    // mediaCodec_->Configure(parameter);
    return Status::OK;
}

Status DecoderSurfaceFilter::Prepare() {
    MEDIA_LOG_I("Prepare enter.");
    if (onLinkedResultCallback_ != nullptr) {
        onLinkedResultCallback_->OnLinkedResult(GetInputBufferQueue(), meta_);
    }
    return Status::OK;
}

Status DecoderSurfaceFilter::Start() {
    MEDIA_LOG_I("Start enter.");
    Filter::Start();
    mediaCodec_->Start();
    return Status::OK;
}

Status DecoderSurfaceFilter::Pause() {
    MEDIA_LOG_I("Pause enter.");
    latestPausedTime_ = latestBufferTime_;
    mediaCodec_->Stop();
    return Status::OK;
}

Status DecoderSurfaceFilter::Resume() {
    MEDIA_LOG_I("Resume enter.");
    refreshTotalPauseTime_ = true;
    mediaCodec_->Start();
    return Status::OK;
}

Status DecoderSurfaceFilter::Stop() {
    MEDIA_LOG_I("Stop enter.");
    latestBufferTime_ = TIME_NONE;
    latestPausedTime_ = TIME_NONE;
    totalPausedTime_ = 0;
    refreshTotalPauseTime_ = false;

    timeval tv;
    gettimeofday(&tv, 0);
    stopTime_ = (int64_t)tv.tv_sec * 1000000 + (int64_t)tv.tv_usec;
    return Status::OK;
}

Status DecoderSurfaceFilter::Flush() {
    MEDIA_LOG_I("Flush enter.");
    mediaCodec_->Flush();
    return Status::OK;
}

Status DecoderSurfaceFilter::Release() {
    MEDIA_LOG_I("Release enter.");
    mediaCodec_->Release();
    return Status::OK;
}

void DecoderSurfaceFilter::SetParameter(const std::shared_ptr<Meta> &parameter) {
    MEDIA_LOG_I("SetParameter enter.");
    // mediaCodec_->SetParameter(parameter);
}

void DecoderSurfaceFilter::GetParameter(std::shared_ptr<Meta> &parameter) {
    // mediaCodec_->GetOutputFormat
}

Status DecoderSurfaceFilter::LinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType) {
    MEDIA_LOG_I("LinkNext enter.");
    nextFilter_ = nextFilter;
    nextFiltersMap_[outType].push_back(nextFilter_);
    std::shared_ptr<FilterLinkCallback> filterLinkCallback = std::make_shared<DecoderSurfaceFilterLinkCallback>(shared_from_this());
    nextFilter->OnLinked(outType, configureParameter_, filterLinkCallback);
    nextFilter->Prepare();
    return Status::OK;
}

Status DecoderSurfaceFilter::UpdateNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType) {
    return Status::OK;
}

Status DecoderSurfaceFilter::UnLinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType) {
    return Status::OK;
}

FilterType DecoderSurfaceFilter::GetFilterType() {
    return filterType_;
}

Status DecoderSurfaceFilter::OnLinked(StreamType inType, const std::shared_ptr<Meta> &meta, const std::shared_ptr<FilterLinkCallback> &callback) {
    MEDIA_LOG_I("OnLinked enter.");
    meta_ = meta;
    FALSE_RETURN_V(format->Get<Tag::MIME_TYPE>(codecMimeType_), Status::ERROR_INVALID_PARAMETER);
    mediaCodec_->Init(MediaAVCodec::AVCodecType::AVCODEC_TYPE_VIDEO_DECODER, true, codecMimeType_);
    onLinkedResultCallback_ = callback;
    return Status::OK;
}

Status DecoderSurfaceFilter::OnUpdated(StreamType inType, const std::shared_ptr<Meta> &meta,
                                       const std::shared_ptr<FilterLinkCallback> &callback) {
    return Status::OK;
}

Status DecoderSurfaceFilter::OnUnLinked(StreamType inType, const std::shared_ptr<FilterLinkCallback>& callback) {
    return Status::OK;
}

void DecoderSurfaceFilter::OnLinkedResult(const sptr<AVBufferQueueProducer> &outputBufferQueue, std::shared_ptr<Meta> &meta) {
    MEDIA_LOG_I("OnLinkedResult enter.");
    std::shared_ptr<MediaAVCodec::VideoCodecCallback> mediaCodecCallback = std::make_shared<MediaCodecCallback>(shared_from_this());
    mediaCodec_->SetCallback(mediaCodecCallback);
}

void DecoderSurfaceFilter::OnUpdatedResult(std::shared_ptr<Meta> &meta) {
}

void DecoderSurfaceFilter::OnUnlinkedResult(std::shared_ptr<Meta> &meta) {
}

void DecoderSurfaceFilter::DrainInputBuffer(std::shared_ptr<AVBuffer> &inputBuffer) {
    MEDIA_LOG_I("DrainInputBuffer enter.");
    AutoLock lock(inputCntMutex_);
    cond_.Wait(lock, [this] { return availableInputBufferCnt_ > 0; });

}

void DecoderSurfaceFilter::DrainOutputBuffer(std::shared_ptr<AVBuffer> &outputBuffer) {
    MEDIA_LOG_I("DrainOutputBuffer enter.");
}

sptr<AVBufferQueueProducer> DecoderSurfaceFilter::GetInputBufferQueue(){
    MEDIA_LOG_I("DrainOutputBuffer enter.");
    if (inputBufferQueue_ != nullptr && inputBufferQueue_-> GetQueueSize() > 0) {
        MEDIA_LOG_I("InputBufferQueue already create");
        return inputBufferQueueProducer_;
    }
    int inputBufferSize = 8;
    MemoryType memoryType = MemoryType::SHARED_MEMORY;
#ifndef MEDIA_OHOS
    memoryType = MemoryType::VIRTUAL_MEMORY;
#endif
    inputBufferQueue_ = AVBufferQueue::Create(inputBufferSize, memoryType, INPUT_BUFFER_QUEUE_NAME);
    inputBufferQueueProducer_ = inputBufferQueue_->GetProducer();
    inputBufferQueueConsumer_ = inputBufferQueue_->GetConsumer();
    sptr<IConsumerListener> listener = new AVBufferAvailableListener(shared_from_this());
    MEDIA_LOG_I("InputBufferQueue setlistener");
    inputBufferQueueConsumer_->SetBufferAvailableListener(listener);
}

#ifndef OHOS_LITE
Status DecoderSurfaceFilter::SetVideoSurface(sptr<Surface> surface)
{
    if (!surface) {
        MEDIA_LOG_W("surface is null");
        return Status::ERROR_INVALID_PARAMETER;
    }
    mediaCodec_->SetOutputSurface(surface);
    MEDIA_LOG_D("SetVideoSurface success");
    return Status::OK;
}
#endif

void UpdateAvailableInputBufferCount()
{
    AutoLock lock(inputCntMutex_);
    availableInputBufferCnt_ += 1;
    cond_.NotifyOne();
}

} //namespace Pipeline
} //namespace MEDIA
} //namespace OHOS