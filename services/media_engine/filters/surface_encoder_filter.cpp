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

#include "surface_encoder_filter.h"
#include "filter/filter_factory.h"
#include "avcodec_info.h"
#include "avcodec_common.h"
#include <time.h>

#define TIME_OUT_MS 8

namespace OHOS {
namespace Media {
namespace Pipeline {
static AutoRegisterFilter<SurfaceEncoderFilter> g_registerSurfaceEncoderFilter("builtin.recorder.videoencoder", FilterType::FILTERTYPE_VENC, 
    [](const std::string& name, const FilterType type) {return std::make_shared<SurfaceEncoderFilter>(name, FilterType::FILTERTYPE_VENC); });

class SurfaceEncoderFilterLinkCallback : public FilterLinkCallback {
public:
    SurfaceEncoderFilterLinkCallback(std::shared_ptr<SurfaceEncoderFilter> surfaceEncoderFilter) {
        surfaceEncoderFilter_ = surfaceEncoderFilter;
    }

    ~SurfaceEncoderFilterLinkCallback() = default;

    void OnLinkedResult(const sptr<AVBufferQueueProducer> &queue, std::shared_ptr<Meta> &meta) override {
        MEDIA_LOG_I("OnLinkedResult enter.");
        surfaceEncoderFilter_->OnLinkedResult(queue, meta);
    }

    void OnUnlinkedResult(std::shared_ptr<Meta> &meta) override {
        surfaceEncoderFilter_->OnUnlinkedResult(meta);
    }

    void OnUpdatedResult(std::shared_ptr<Meta> &meta) override {
        surfaceEncoderFilter_->OnUpdatedResult(meta);
    }
private:
    std::shared_ptr<SurfaceEncoderFilter> surfaceEncoderFilter_;
};

class MediaCodecCallback : public OHOS::MediaAVCodec::VideoCodecCallback {
public:
    MediaCodecCallback(std::shared_ptr<SurfaceEncoderFilter> surfaceEncoderFilter) {
        surfaceEncoderFilter_ = surfaceEncoderFilter;
    }

    ~MediaCodecCallback() = default;
    
    void OnError(MediaAVCodec::AVCodecErrorType errorType, int32_t errorCode) {

    }

    void OnOutputFormatChanged(const MediaAVCodec::Format &format) {

    }

    void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) {

    }

    void OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) {
        surfaceEncoderFilter_->OnBufferFilled(buffer);
    }

private:
    std::shared_ptr<SurfaceEncoderFilter> surfaceEncoderFilter_;
};

SurfaceEncoderFilter::SurfaceEncoderFilter(std::string name, FilterType type): Filter(name, type) { 
    // filterType_ = type; 
}

SurfaceEncoderFilter::~SurfaceEncoderFilter()
{
}

Status SurfaceEncoderFilter::SetCodecFormat(const std::shared_ptr<Meta> &format) {
    MEDIA_LOG_I("SetCodecFormat enter.");
    FALSE_RETURN_V(format->Get<Tag::MIME_TYPE>(codecMimeType_), Status::ERROR_INVALID_PARAMETER);
    return Status::OK;
}

void SurfaceEncoderFilter::Init(const std::shared_ptr<EventReceiver> &receiver, const std::shared_ptr<FilterCallback> &callback) {
    MEDIA_LOG_I("Init enter.");
    eventReceiver_ = receiver;
    filterCallback_ = callback;
    mediaCodec_ = MediaAVCodec::CodecServer::Create();
    mediaCodec_->Init(MediaAVCodec::AVCodecType::AVCODEC_TYPE_VIDEO_ENCODER, true, codecMimeType_);
}

Status SurfaceEncoderFilter::Configure(const std::shared_ptr<Meta> &parameter) {
    MEDIA_LOG_I("Configure enter.");
    configureParameter_ = parameter;
    // mediaCodec_->Configure(parameter);
    return Status::OK;
}

sptr<Surface> SurfaceEncoderFilter::GetInputSurface() {
    MEDIA_LOG_I("GetInputSurface enter.");
    return mediaCodec_->CreateInputSurface();
}

Status SurfaceEncoderFilter::Prepare() {
    MEDIA_LOG_I("Prepare enter.");
    filterCallback_->OnCallback(shared_from_this(), FilterCallBackCommand::NEXT_FILTER_NEEDED,
                                StreamType::STREAMTYPE_ENCODED_VIDEO);
    return Status::OK;
}

Status SurfaceEncoderFilter::Start() {
    MEDIA_LOG_I("Start enter.");
    nextFilter_->Start();
    mediaCodec_->Start();
    return Status::OK;
}

Status SurfaceEncoderFilter::Pause() {
    MEDIA_LOG_I("Pause enter.");
    latestPausedTime_ = latestBufferTime_;
    mediaCodec_->Stop();
    return Status::OK;
}

Status SurfaceEncoderFilter::Resume() {
    MEDIA_LOG_I("Resume enter.");
    refreshTotalPauseTime_ = true;
    mediaCodec_->Start();
    return Status::OK;
}

Status SurfaceEncoderFilter::Stop() {
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

Status SurfaceEncoderFilter::Flush() {
    MEDIA_LOG_I("Flush enter.");
    mediaCodec_->Flush();
    return Status::OK;
}

Status SurfaceEncoderFilter::Release() {
    MEDIA_LOG_I("Release enter.");
    mediaCodec_->Release();
    return Status::OK;
}

Status SurfaceEncoderFilter::NotifyEOS() {
    mediaCodec_->NotifyEos();
    return Status::OK;
}

void SurfaceEncoderFilter::SetParameter(const std::shared_ptr<Meta> &parameter) {
    MEDIA_LOG_I("SetParameter enter.");
    // mediaCodec_->SetParameter(parameter);
}

void SurfaceEncoderFilter::GetParameter(std::shared_ptr<Meta> &parameter) {
}

Status SurfaceEncoderFilter::LinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType) {
    MEDIA_LOG_I("LinkNext enter.");
    nextFilter_ = nextFilter;
    std::shared_ptr<FilterLinkCallback> filterLinkCallback = std::make_shared<SurfaceEncoderFilterLinkCallback>(shared_from_this());
    nextFilter->OnLinked(outType, configureParameter_, filterLinkCallback);
    nextFilter->Prepare();
    return Status::OK;
}

Status SurfaceEncoderFilter::UpdateNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType) {
    return Status::OK;
}

Status SurfaceEncoderFilter::UnLinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType) {
    return Status::OK;
}

FilterType SurfaceEncoderFilter::GetFilterType() {
    return filterType_;
}

Status SurfaceEncoderFilter::OnLinked(StreamType inType, const std::shared_ptr<Meta> &meta, const std::shared_ptr<FilterLinkCallback> &callback) {
    MEDIA_LOG_I("OnLinked enter.");
    return Status::OK;
}

Status SurfaceEncoderFilter::OnUpdated(StreamType inType, const std::shared_ptr<Meta> &meta,
                                const std::shared_ptr<FilterLinkCallback> &callback) {
    return Status::OK;
}

Status SurfaceEncoderFilter::OnUnLinked(StreamType inType, const std::shared_ptr<FilterLinkCallback>& callback) {
    return Status::OK;
}

void SurfaceEncoderFilter::OnLinkedResult(const sptr<AVBufferQueueProducer> &outputBufferQueue, std::shared_ptr<Meta> &meta) {
    MEDIA_LOG_I("OnLinkedResult enter.");
    outputBufferQueueProducer_ = outputBufferQueue;
    std::shared_ptr<MediaAVCodec::VideoCodecCallback> mediaCodecCallback = std::make_shared<MediaCodecCallback>(shared_from_this());
    mediaCodec_->SetCallback(mediaCodecCallback);
}

void SurfaceEncoderFilter::OnUpdatedResult(std::shared_ptr<Meta> &meta) {
}

void SurfaceEncoderFilter::OnUnlinkedResult(std::shared_ptr<Meta> &meta) {
}

void SurfaceEncoderFilter::OnBufferFilled(std::shared_ptr<AVBuffer> &inputBuffer) {
    MEDIA_LOG_I("OnBufferFilled enter.");
    latestBufferTime_ = inputBuffer->pts_;
    if (isStop_) {
        return;
    }
    if (latestBufferTime_ > stopTime_) {
        DoStop();
        return;
    }
    if (refreshTotalPauseTime_) {
        if (latestPausedTime_ != TIME_NONE && latestBufferTime_ > latestPausedTime_) {
            totalPausedTime_ += latestBufferTime_ - latestPausedTime_;
        }
        refreshTotalPauseTime_ = false;
    }
    inputBuffer->pts_ -= totalPausedTime_;

    int32_t size = inputBuffer->memory_->GetSize();
    std::shared_ptr<AVBuffer> emptyOutputBuffer;
    AVBufferConfig avBufferConfig;
    avBufferConfig.capacity = size;
    outputBufferQueueProducer_->RequestBuffer(emptyOutputBuffer, avBufferConfig, TIME_OUT_MS);
    std::shared_ptr<AVMemory> &bufferMem = emptyOutputBuffer->memory_;
    bufferMem->Write(inputBuffer->memory_->GetAddr(), size, 0);
    *(emptyOutputBuffer->meta_) = *(inputBuffer->meta_);
    outputBufferQueueProducer_->PushBuffer(emptyOutputBuffer, true);
}

Status SurfaceEncoderFilter::DoStop() {
    mediaCodec_->Stop();
    nextFilter_->Stop();
    isStop_ = true;
}

} //namespace Pipeline
} //namespace MEDIA
} //namespace OHOS