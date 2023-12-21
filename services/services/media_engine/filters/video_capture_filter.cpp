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

#include "video_capture_filter.h"
#include "filter/filter_factory.h"
#include "avcodec_info.h"
#include "avcodec_common.h"
#include "sync_fence.h"
#include <time.h>
#include <sys/time.h>

#define TIME_OUT_MS 100
#define TIME_STAMP "timeStamp"
#define DATA_SIZE "dataSize"


namespace OHOS {
namespace Media {
namespace Pipeline {
static AutoRegisterFilter<VideoCaptureFilter> g_registerSurfaceEncoderFilter("builtin.recorder.videocapture",
    FilterType::FILTERTYPE_VCAPTURE, 
    [](const std::string& name, const FilterType type) {
        return std::make_shared<VideoCaptureFilter>(name, FilterType::FILTERTYPE_VCAPTURE); });

class VideoCaptureFilterLinkCallback : public FilterLinkCallback {
public:
    VideoCaptureFilterLinkCallback(std::shared_ptr<VideoCaptureFilter> videoCaptureFilter) {
        videoCaptureFilter_ = videoCaptureFilter;
    }

    ~VideoCaptureFilterLinkCallback() = default;

    void OnLinkedResult(const sptr<AVBufferQueueProducer> &queue, std::shared_ptr<Meta> &meta) override {
        videoCaptureFilter_->OnLinkedResult(queue, meta);
    }

    void OnUnlinkedResult(std::shared_ptr<Meta> &meta) override {
        videoCaptureFilter_->OnUnlinkedResult(meta);
    }

    void OnUpdatedResult(std::shared_ptr<Meta> &meta) override {
        videoCaptureFilter_->OnUpdatedResult(meta);
    }
private:
    std::shared_ptr<VideoCaptureFilter> videoCaptureFilter_;
};

class ConsumerSurfaceBufferListener : public IBufferConsumerListener {
public:
    explicit ConsumerSurfaceBufferListener(std::shared_ptr<VideoCaptureFilter> videoCaptureFilter) {
        videoCaptureFilter_ = videoCaptureFilter;
    }
    
    void OnBufferAvailable() {
        videoCaptureFilter_->OnBufferAvailable();
    }

private:
    std::shared_ptr<VideoCaptureFilter> videoCaptureFilter_;
};

VideoCaptureFilter::VideoCaptureFilter(std::string name, FilterType type): Filter(name, type) { 
}

VideoCaptureFilter::~VideoCaptureFilter() {
}

Status VideoCaptureFilter::SetCodecFormat(const std::shared_ptr<Meta> &format) {
    MEDIA_LOG_I("SetCodecFormat");
    return Status::OK;
}

void VideoCaptureFilter::Init(const std::shared_ptr<EventReceiver> &receiver, const std::shared_ptr<FilterCallback> &callback) {
    MEDIA_LOG_I("Init");
    eventReceiver_ = receiver;
    filterCallback_ = callback;
}

Status VideoCaptureFilter::Configure(const std::shared_ptr<Meta> &parameter) {
    MEDIA_LOG_I("Configure");
    configureParameter_ = parameter;
    return Status::OK;
}

sptr<Surface> VideoCaptureFilter::GetInputSurface() {
    MEDIA_LOG_I("GetInputSurface");
    if (inputSurface_ != nullptr) {
        MEDIA_LOG_E("inputSurface_ already exists.");
        return nullptr;
    }
    sptr<Surface> consumerSurface  = Surface::CreateSurfaceAsConsumer("AVRecorderSurface");
    if (consumerSurface == nullptr) {
        MEDIA_LOG_E("Create the surface consummer fail");
        return nullptr;
    }
    GSError err = consumerSurface->SetDefaultUsage(ENCODE_USAGE);
    if (err == GSERROR_OK) {
        MEDIA_LOG_I("set consumer usage 0x%{public}x succ", ENCODE_USAGE);
    } else {
        MEDIA_LOG_E("set consumer usage 0x%{public}x failed", ENCODE_USAGE);
    }

    sptr<IBufferProducer> producer = consumerSurface->GetProducer();
    if (producer == nullptr) {
        MEDIA_LOG_E("Get the surface producer fail");
        return nullptr;
    }

    sptr<Surface> producerSurface  = Surface::CreateSurfaceAsProducer(producer);
    if (producerSurface == nullptr) {
        MEDIA_LOG_E("CreateSurfaceAsProducer fail");
        return nullptr;
    }

    sptr<IBufferConsumerListener> listener = new ConsumerSurfaceBufferListener(shared_from_this());
    consumerSurface->RegisterConsumerListener(listener);

    inputSurface_ = consumerSurface;
    return producerSurface;
}

Status VideoCaptureFilter::Prepare() {
    MEDIA_LOG_I("Prepare");
    filterCallback_->OnCallback(shared_from_this(), FilterCallBackCommand::NEXT_FILTER_NEEDED,
                                StreamType::STREAMTYPE_ENCODED_VIDEO);
    return Status::OK;
}

Status VideoCaptureFilter::Start() {
    MEDIA_LOG_I("Start");
    isStop_ = false;
    nextFilter_->Start();
    return Status::OK;
}

Status VideoCaptureFilter::Pause() {
    MEDIA_LOG_I("Pause");
    isStop_ = true;
    latestPausedTime_ = latestBufferTime_;
    return Status::OK;
}

Status VideoCaptureFilter::Resume() {
    MEDIA_LOG_I("Resume");
    isStop_ = false;
    refreshTotalPauseTime_ = true;
    return Status::OK;
}

Status VideoCaptureFilter::Stop() {
    MEDIA_LOG_I("Stop");
    isStop_ = true;
    latestBufferTime_ = TIME_NONE;
    latestPausedTime_ = TIME_NONE;
    totalPausedTime_ = 0;
    refreshTotalPauseTime_ = false;
    nextFilter_->Stop();
    return Status::OK;
}

Status VideoCaptureFilter::Flush() {
    MEDIA_LOG_I("Flush");
    return Status::OK;
}

Status VideoCaptureFilter::Release() {
    MEDIA_LOG_I("Release");
    return Status::OK;
}

Status VideoCaptureFilter::NotifyEOS() {
    return Status::OK;
}

void VideoCaptureFilter::SetParameter(const std::shared_ptr<Meta> &parameter) {
    MEDIA_LOG_I("SetParameter");
}

void VideoCaptureFilter::GetParameter(std::shared_ptr<Meta> &parameter) {
}

Status VideoCaptureFilter::LinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType) {
    MEDIA_LOG_I("LinkNext enter.");
    nextFilter_ = nextFilter;
    std::shared_ptr<FilterLinkCallback> filterLinkCallback =
        std::make_shared<VideoCaptureFilterLinkCallback>(shared_from_this());
    nextFilter->OnLinked(outType, configureParameter_, filterLinkCallback);
    nextFilter->Prepare();
    return Status::OK;
}

Status VideoCaptureFilter::UpdateNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType) {
    return Status::OK;
}

Status VideoCaptureFilter::UnLinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType) {
    return Status::OK;
}

FilterType VideoCaptureFilter::GetFilterType() {
    return filterType_;
}

Status VideoCaptureFilter::OnLinked(StreamType inType, const std::shared_ptr<Meta> &meta,
    const std::shared_ptr<FilterLinkCallback> &callback) {
    MEDIA_LOG_I("OnLinked");
    return Status::OK;
}

Status VideoCaptureFilter::OnUpdated(StreamType inType, const std::shared_ptr<Meta> &meta,
                                const std::shared_ptr<FilterLinkCallback> &callback) {
    return Status::OK;
}

Status VideoCaptureFilter::OnUnLinked(StreamType inType, const std::shared_ptr<FilterLinkCallback>& callback) {
    return Status::OK;
}

void VideoCaptureFilter::OnLinkedResult(const sptr<AVBufferQueueProducer> &outputBufferQueue,
    std::shared_ptr<Meta> &meta) {
    MEDIA_LOG_I("OnLinkedResult");
    outputBufferQueueProducer_ = outputBufferQueue;
}

void VideoCaptureFilter::OnUpdatedResult(std::shared_ptr<Meta> &meta) {
}

void VideoCaptureFilter::OnUnlinkedResult(std::shared_ptr<Meta> &meta) {
}

void VideoCaptureFilter::OnBufferAvailable() {
    MEDIA_LOG_I("OnBufferAvailable");

    sptr<SurfaceBuffer> buffer;
    sptr<SyncFence> fence;
    int64_t timestamp;
    int32_t bufferSize;
    OHOS::Rect damage;
    GSError ret = inputSurface_->AcquireBuffer(buffer, fence, timestamp, damage);
    if (ret != GSERROR_OK || buffer == nullptr) {
        MEDIA_LOG_E("AcquireBuffer failed");
        return;
    }
    if (isStop_) {
        inputSurface_->ReleaseBuffer(buffer, -1);
        return;
    }
    buffer->GetExtraData()->ExtraGet(TIME_STAMP, timestamp);
    buffer->GetExtraData()->ExtraGet(DATA_SIZE, bufferSize);

    std::shared_ptr<AVBuffer> emptyOutputBuffer;
    AVBufferConfig avBufferConfig;
    avBufferConfig.size = bufferSize;
    avBufferConfig.memoryFlag = MemoryFlag::MEMORY_READ_WRITE;
    Status status = outputBufferQueueProducer_->RequestBuffer(emptyOutputBuffer, avBufferConfig, TIME_OUT_MS);
    if (status != Status::OK) {
        MEDIA_LOG_E("RequestBuffer fail.");
    }
    std::shared_ptr<AVMemory> &bufferMem = emptyOutputBuffer->memory_;
    if (emptyOutputBuffer->memory_ == nullptr) {
        MEDIA_LOG_I("emptyOutputBuffer->memory_ is nullptr.");
        return;
    }
    bufferMem->Write((const uint8_t *)buffer->GetVirAddr(), bufferSize, 0);

    if (startBufferTime_ == TIME_NONE) {
        startBufferTime_ = timestamp;
    }
    latestBufferTime_ = timestamp;
    if (refreshTotalPauseTime_) {
        if (latestPausedTime_ != TIME_NONE && latestBufferTime_ > latestPausedTime_) {
            totalPausedTime_ += latestBufferTime_ - latestPausedTime_;
        }
        refreshTotalPauseTime_ = false;
    }

    emptyOutputBuffer->pts_ = timestamp - startBufferTime_ - totalPausedTime_;
    status = outputBufferQueueProducer_->PushBuffer(emptyOutputBuffer, true);
    if (status != Status::OK) {
        MEDIA_LOG_E("PushBuffer fail");
    }
    inputSurface_->ReleaseBuffer(buffer, -1);
}

} //namespace Pipeline
} //namespace MEDIA
} //namespace OHOS