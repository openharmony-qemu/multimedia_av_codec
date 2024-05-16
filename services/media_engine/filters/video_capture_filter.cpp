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
#include <ctime>
#include <sys/time.h>
#include "sync_fence.h"
#include "filter/filter_factory.h"
#include "avcodec_info.h"
#include "avcodec_common.h"
#include "avcodec_trace.h"

namespace OHOS {
namespace Media {
namespace Pipeline {
static AutoRegisterFilter<VideoCaptureFilter> g_registerSurfaceEncoderFilter("builtin.recorder.videocapture",
    FilterType::VIDEO_CAPTURE,
    [](const std::string& name, const FilterType type) {
        return std::make_shared<VideoCaptureFilter>(name, FilterType::VIDEO_CAPTURE);
    });

class VideoCaptureFilterLinkCallback : public FilterLinkCallback {
public:
    explicit VideoCaptureFilterLinkCallback(std::shared_ptr<VideoCaptureFilter> videoCaptureFilter)
        : videoCaptureFilter_(std::move(videoCaptureFilter))
    {
    }

    ~VideoCaptureFilterLinkCallback() = default;

    void OnLinkedResult(const sptr<AVBufferQueueProducer> &queue, std::shared_ptr<Meta> &meta) override
    {
        if (auto videoCaptureFilter = videoCaptureFilter_.lock()) {
            videoCaptureFilter->OnLinkedResult(queue, meta);
        } else {
            MEDIA_LOG_I("invalid videoCaptureFilter");
        }
    }

    void OnUnlinkedResult(std::shared_ptr<Meta> &meta) override
    {
        if (auto videoCaptureFilter = videoCaptureFilter_.lock()) {
            videoCaptureFilter->OnUnlinkedResult(meta);
        } else {
            MEDIA_LOG_I("invalid videoCaptureFilter");
        }
    }

    void OnUpdatedResult(std::shared_ptr<Meta> &meta) override
    {
        if (auto videoCaptureFilter = videoCaptureFilter_.lock()) {
            videoCaptureFilter->OnUpdatedResult(meta);
        } else {
            MEDIA_LOG_I("invalid videoCaptureFilter");
        }
    }
private:
    std::weak_ptr<VideoCaptureFilter> videoCaptureFilter_;
};

class ConsumerSurfaceBufferListener : public IBufferConsumerListener {
public:
    explicit ConsumerSurfaceBufferListener(std::shared_ptr<VideoCaptureFilter> videoCaptureFilter)
        : videoCaptureFilter_(std::move(videoCaptureFilter))
    {
    }
    
    void OnBufferAvailable()
    {
        if (auto videoCaptureFilter = videoCaptureFilter_.lock()) {
            videoCaptureFilter->OnBufferAvailable();
        } else {
            MEDIA_LOG_I("invalid videoCaptureFilter");
        }
    }

private:
    std::weak_ptr<VideoCaptureFilter> videoCaptureFilter_;
};

VideoCaptureFilter::VideoCaptureFilter(std::string name, FilterType type): Filter(name, type)
{
    MEDIA_LOG_I("video capture filter create");
}

VideoCaptureFilter::~VideoCaptureFilter()
{
    MEDIA_LOG_I("video capture filter destroy");
}

Status VideoCaptureFilter::SetCodecFormat(const std::shared_ptr<Meta> &format)
{
    MEDIA_LOG_I("SetCodecFormat");
    return Status::OK;
}

void VideoCaptureFilter::Init(const std::shared_ptr<EventReceiver> &receiver,
    const std::shared_ptr<FilterCallback> &callback)
{
    MEDIA_LOG_I("Init");
    MediaAVCodec::AVCodecTrace trace("VideoCaptureFilter::Init");
    eventReceiver_ = receiver;
    filterCallback_ = callback;
}

Status VideoCaptureFilter::Configure(const std::shared_ptr<Meta> &parameter)
{
    MEDIA_LOG_I("Configure");
    configureParameter_ = parameter;
    return Status::OK;
}

Status VideoCaptureFilter::SetInputSurface(sptr<Surface> surface)
{
    MEDIA_LOG_I("SetInputSurface");
    MediaAVCodec::AVCodecTrace trace("VideoCaptureFilter::SetInputSurface");
    if (surface == nullptr) {
        MEDIA_LOG_E("surface is nullptr");
        return Status::ERROR_UNKNOWN;
    }
    inputSurface_ = surface;
    sptr<IBufferConsumerListener> listener = new ConsumerSurfaceBufferListener(shared_from_this());
    inputSurface_->RegisterConsumerListener(listener);
    return Status::OK;
}

sptr<Surface> VideoCaptureFilter::GetInputSurface()
{
    MEDIA_LOG_I("GetInputSurface");
    MediaAVCodec::AVCodecTrace trace("VideoCaptureFilter::GetInputSurface");
    sptr<Surface> consumerSurface = Surface::CreateSurfaceAsConsumer("EncoderSurface");
    if (consumerSurface == nullptr) {
        MEDIA_LOG_E("Create the surface consumer fail");
        return nullptr;
    }
    GSError err = consumerSurface->SetDefaultUsage(ENCODE_USAGE);
    if (err == GSERROR_OK) {
        MEDIA_LOG_E("set consumer usage 0x%{public}x succ", ENCODE_USAGE);
    } else {
        MEDIA_LOG_E("set consumer usage 0x%{public}x fail", ENCODE_USAGE);
    }
    sptr<IBufferProducer> producer = consumerSurface->GetProducer();
    if (producer == nullptr) {
        MEDIA_LOG_E("Get the surface producer fail");
        return nullptr;
    }
    sptr<Surface> producerSurface = Surface::CreateSurfaceAsProducer(producer);
    if (producerSurface == nullptr) {
        MEDIA_LOG_E("CreateSurfaceAsProducer fail");
        return nullptr;
    }
    inputSurface_ = consumerSurface;
    sptr<IBufferConsumerListener> listener = new ConsumerSurfaceBufferListener(shared_from_this());
    inputSurface_->RegisterConsumerListener(listener);
    return producerSurface;
}

Status VideoCaptureFilter::DoPrepare()
{
    MEDIA_LOG_I("Prepare");
    MediaAVCodec::AVCodecTrace trace("VideoCaptureFilter::Prepare");
    filterCallback_->OnCallback(shared_from_this(), FilterCallBackCommand::NEXT_FILTER_NEEDED,
        StreamType::STREAMTYPE_ENCODED_VIDEO);
    return Status::OK;
}

Status VideoCaptureFilter::DoStart()
{
    MEDIA_LOG_I("Start");
    MediaAVCodec::AVCodecTrace trace("VideoCaptureFilter::Start");
    isStop_ = false;
    return Status::OK;
}

Status VideoCaptureFilter::DoPause()
{
    MEDIA_LOG_I("Pause");
    MediaAVCodec::AVCodecTrace trace("VideoCaptureFilter::Pause");
    isStop_ = true;
    latestPausedTime_ = latestBufferTime_;
    return Status::OK;
}

Status VideoCaptureFilter::DoResume()
{
    MEDIA_LOG_I("Resume");
    MediaAVCodec::AVCodecTrace trace("VideoCaptureFilter::Resume");
    isStop_ = false;
    refreshTotalPauseTime_ = true;
    return Status::OK;
}

Status VideoCaptureFilter::DoStop()
{
    MEDIA_LOG_I("Stop");
    MediaAVCodec::AVCodecTrace trace("VideoCaptureFilter::Stop");
    isStop_ = true;
    latestBufferTime_ = TIME_NONE;
    latestPausedTime_ = TIME_NONE;
    totalPausedTime_ = 0;
    refreshTotalPauseTime_ = false;
    return Status::OK;
}

Status VideoCaptureFilter::DoFlush()
{
    MEDIA_LOG_I("Flush");
    return Status::OK;
}

Status VideoCaptureFilter::DoRelease()
{
    MEDIA_LOG_I("Release");
    MediaAVCodec::AVCodecTrace trace("VideoCaptureFilter::Release");
    return Status::OK;
}

Status VideoCaptureFilter::NotifyEos()
{
    MEDIA_LOG_I("NotifyEos");
    return Status::OK;
}

void VideoCaptureFilter::SetParameter(const std::shared_ptr<Meta> &parameter)
{
    MEDIA_LOG_I("SetParameter");
    MediaAVCodec::AVCodecTrace trace("VideoCaptureFilter::SetParameter");
}

void VideoCaptureFilter::GetParameter(std::shared_ptr<Meta> &parameter)
{
    MEDIA_LOG_I("GetParameter");
    MediaAVCodec::AVCodecTrace trace("VideoCaptureFilter::GetParameter");
}

Status VideoCaptureFilter::LinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType)
{
    MEDIA_LOG_I("LinkNext");
    MediaAVCodec::AVCodecTrace trace("VideoCaptureFilter::LinkNext");
    nextFilter_ = nextFilter;
    nextFiltersMap_[outType].push_back(nextFilter_);
    std::shared_ptr<FilterLinkCallback> filterLinkCallback =
        std::make_shared<VideoCaptureFilterLinkCallback>(shared_from_this());
    nextFilter->OnLinked(outType, configureParameter_, filterLinkCallback);
    return Status::OK;
}

Status VideoCaptureFilter::UpdateNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType)
{
    MEDIA_LOG_I("UpdateNext");
    return Status::OK;
}

Status VideoCaptureFilter::UnLinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType)
{
    MEDIA_LOG_I("UnLinkNext");
    return Status::OK;
}

FilterType VideoCaptureFilter::GetFilterType()
{
    MEDIA_LOG_I("GetFilterType");
    return filterType_;
}

Status VideoCaptureFilter::OnLinked(StreamType inType, const std::shared_ptr<Meta> &meta,
    const std::shared_ptr<FilterLinkCallback> &callback)
{
    MEDIA_LOG_I("OnLinked");
    return Status::OK;
}

Status VideoCaptureFilter::OnUpdated(StreamType inType, const std::shared_ptr<Meta> &meta,
    const std::shared_ptr<FilterLinkCallback> &callback)
{
    MEDIA_LOG_I("OnUpdated");
    return Status::OK;
}

Status VideoCaptureFilter::OnUnLinked(StreamType inType, const std::shared_ptr<FilterLinkCallback>& callback)
{
    MEDIA_LOG_I("OnUnLinked");
    return Status::OK;
}

void VideoCaptureFilter::OnLinkedResult(const sptr<AVBufferQueueProducer> &outputBufferQueue,
    std::shared_ptr<Meta> &meta)
{
    MEDIA_LOG_I("OnLinkedResult");
    MediaAVCodec::AVCodecTrace trace("VideoCaptureFilter::OnLinkedResult");
    outputBufferQueueProducer_ = outputBufferQueue;
}

void VideoCaptureFilter::OnUpdatedResult(std::shared_ptr<Meta> &meta)
{
    MEDIA_LOG_I("OnUpdatedResult");
}

void VideoCaptureFilter::OnUnlinkedResult(std::shared_ptr<Meta> &meta)
{
    MEDIA_LOG_I("OnUnlinkedResult");
}

void VideoCaptureFilter::OnBufferAvailable()
{
    MEDIA_LOG_I("OnBufferAvailable");
    sptr<SurfaceBuffer> buffer;
    sptr<SyncFence> fence;
    int64_t timestamp;
    int32_t bufferSize = 0;
    OHOS::Rect damage;
    GSError ret = inputSurface_->AcquireBuffer(buffer, fence, timestamp, damage);
    if (ret != GSERROR_OK || buffer == nullptr) {
        MEDIA_LOG_E("AcquireBuffer failed");
        return;
    }
    constexpr uint32_t waitForEver = -1;
    (void)fence->Wait(waitForEver);
    if (isStop_) {
        inputSurface_->ReleaseBuffer(buffer, -1);
        return;
    }
    auto extraData = buffer->GetExtraData();
    if (extraData) {
        extraData->ExtraGet("timeStamp", timestamp);
        extraData->ExtraGet("dataSize", bufferSize);
    }

    std::shared_ptr<AVBuffer> emptyOutputBuffer;
    AVBufferConfig avBufferConfig;
    avBufferConfig.size = bufferSize;
    avBufferConfig.memoryType = MemoryType::SHARED_MEMORY;
    avBufferConfig.memoryFlag = MemoryFlag::MEMORY_READ_WRITE;
    int32_t timeOutMs = 100;
    Status status = outputBufferQueueProducer_->RequestBuffer(emptyOutputBuffer, avBufferConfig, timeOutMs);
    if (status != Status::OK) {
        MEDIA_LOG_E("RequestBuffer fail.");
        inputSurface_->ReleaseBuffer(buffer, -1);
        return;
    }
    std::shared_ptr<AVMemory> &bufferMem = emptyOutputBuffer->memory_;
    if (emptyOutputBuffer->memory_ == nullptr) {
        MEDIA_LOG_I("emptyOutputBuffer->memory_ is nullptr.");
        inputSurface_->ReleaseBuffer(buffer, -1);
        return;
    }
    emptyOutputBuffer->flag_ = 0;
    bufferMem->Write((const uint8_t *)buffer->GetVirAddr(), bufferSize, 0);
    UpdateBufferConfig(emptyOutputBuffer, timestamp);
    status = outputBufferQueueProducer_->PushBuffer(emptyOutputBuffer, true);
    if (status != Status::OK) {
        MEDIA_LOG_E("PushBuffer fail");
    }
    inputSurface_->ReleaseBuffer(buffer, -1);
}

void VideoCaptureFilter::UpdateBufferConfig(std::shared_ptr<AVBuffer> buffer, int64_t timestamp)
{
    if (startBufferTime_ == TIME_NONE) {
        startBufferTime_ = timestamp;
        buffer->flag_ =
            (uint32_t)Plugins::AVBufferFlag::SYNC_FRAME | (uint32_t)Plugins::AVBufferFlag::CODEC_DATA;
    }
    latestBufferTime_ = timestamp;
    if (refreshTotalPauseTime_) {
        if (latestPausedTime_ != TIME_NONE && latestBufferTime_ > latestPausedTime_) {
            totalPausedTime_ += latestBufferTime_ - latestPausedTime_;
        }
        refreshTotalPauseTime_ = false;
    }
    buffer->pts_ = timestamp - startBufferTime_ - totalPausedTime_;
    MediaAVCodec::AVCodecTrace trace("VideoCaptureFilter::UpdateBufferConfig");
    MEDIA_LOG_I("UpdateBufferConfig buffer->pts" PUBLIC_LOG_D64, buffer->pts_);
}

} // namespace Pipeline
} // namespace MEDIA
} // namespace OHOS