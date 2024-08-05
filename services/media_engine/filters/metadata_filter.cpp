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

#include "metadata_filter.h"
#include <ctime>
#include <sys/time.h>
#include "sync_fence.h"
#include "filter/filter_factory.h"
#include "avcodec_info.h"
#include "avcodec_common.h"
#include "avcodec_trace.h"
#include "common/log.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_METADATA, "HiStreamer" };
}

namespace OHOS {
namespace Media {
namespace Pipeline {
static AutoRegisterFilter<MetaDataFilter> g_registerTimedMetaSurfaceFilter("builtin.recorder.timed_metadata",
    FilterType::TIMED_METADATA,
    [](const std::string& name, const FilterType type) {
        return std::make_shared<MetaDataFilter>(name, FilterType::TIMED_METADATA);
    });

class MetaDataFilterLinkCallback : public FilterLinkCallback {
public:
    explicit MetaDataFilterLinkCallback(std::shared_ptr<MetaDataFilter> metaDataFilter)
        : metaDataFilter_(std::move(metaDataFilter))
    {
    }

    ~MetaDataFilterLinkCallback() = default;

    void OnLinkedResult(const sptr<AVBufferQueueProducer> &queue, std::shared_ptr<Meta> &meta) override
    {
        if (auto metaDataFilter = metaDataFilter_.lock()) {
            metaDataFilter->OnLinkedResult(queue, meta);
        } else {
            MEDIA_LOG_I("invalid metaDataFilter");
        }
    }

    void OnUnlinkedResult(std::shared_ptr<Meta> &meta) override
    {
        if (auto metaDataFilter = metaDataFilter_.lock()) {
            metaDataFilter->OnUnlinkedResult(meta);
        } else {
            MEDIA_LOG_I("invalid metaDataFilter");
        }
    }

    void OnUpdatedResult(std::shared_ptr<Meta> &meta) override
    {
        if (auto metaDataFilter = metaDataFilter_.lock()) {
            metaDataFilter->OnUpdatedResult(meta);
        } else {
            MEDIA_LOG_I("invalid metaDataFilter");
        }
    }
private:
    std::weak_ptr<MetaDataFilter> metaDataFilter_;
};

class MetaDataSurfaceBufferListener : public IBufferConsumerListener {
public:
    explicit MetaDataSurfaceBufferListener(std::shared_ptr<MetaDataFilter> metaDataFilter)
        : metaDataFilter_(std::move(metaDataFilter))
    {
    }
    
    void OnBufferAvailable()
    {
        if (auto metaDataFilter = metaDataFilter_.lock()) {
            metaDataFilter->OnBufferAvailable();
        } else {
            MEDIA_LOG_I("invalid metaDataFilter");
        }
    }

private:
    std::weak_ptr<MetaDataFilter> metaDataFilter_;
};

MetaDataFilter::MetaDataFilter(std::string name, FilterType type): Filter(name, type)
{
    MEDIA_LOG_I("timed meta data filter create");
}

MetaDataFilter::~MetaDataFilter()
{
    MEDIA_LOG_I("timed meta data filter destroy");
}

Status MetaDataFilter::SetCodecFormat(const std::shared_ptr<Meta> &format)
{
    MEDIA_LOG_I("SetCodecFormat");
    return Status::OK;
}

void MetaDataFilter::Init(const std::shared_ptr<EventReceiver> &receiver,
    const std::shared_ptr<FilterCallback> &callback)
{
    MEDIA_LOG_I("Init");
    MediaAVCodec::AVCodecTrace trace("MetaDataFilter::Init");
    eventReceiver_ = receiver;
    filterCallback_ = callback;
}

Status MetaDataFilter::Configure(const std::shared_ptr<Meta> &parameter)
{
    MEDIA_LOG_I("Configure");
    configureParameter_ = parameter;
    return Status::OK;
}

Status MetaDataFilter::SetInputMetaSurface(sptr<Surface> surface)
{
    MEDIA_LOG_I("SetInputMetaSurface");
    MediaAVCodec::AVCodecTrace trace("MetaDataFilter::SetInputMetaSurface");
    if (surface == nullptr) {
        MEDIA_LOG_E("surface is nullptr");
        return Status::ERROR_UNKNOWN;
    }
    inputSurface_ = surface;
    sptr<IBufferConsumerListener> listener = new MetaDataSurfaceBufferListener(shared_from_this());
    inputSurface_->RegisterConsumerListener(listener);
    return Status::OK;
}

sptr<Surface> MetaDataFilter::GetInputMetaSurface()
{
    MEDIA_LOG_I("GetInputMetaSurface");
    MediaAVCodec::AVCodecTrace trace("MetaDataFilter::GetInputMetaSurface");
    sptr<Surface> consumerSurface = Surface::CreateSurfaceAsConsumer("MetadataSurface");
    if (consumerSurface == nullptr) {
        MEDIA_LOG_E("Create the surface consumer fail");
        return nullptr;
    }
    GSError err = consumerSurface->SetDefaultUsage(METASURFACE_USAGE);
    if (err == GSERROR_OK) {
        MEDIA_LOG_I("set consumer usage 0x%{public}x succ", METASURFACE_USAGE);
    } else {
        MEDIA_LOG_E("set consumer usage 0x%{public}x fail", METASURFACE_USAGE);
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
    sptr<IBufferConsumerListener> listener = new MetaDataSurfaceBufferListener(shared_from_this());
    inputSurface_->RegisterConsumerListener(listener);
    return producerSurface;
}

Status MetaDataFilter::DoPrepare()
{
    MEDIA_LOG_I("Prepare");
    MediaAVCodec::AVCodecTrace trace("MetaDataFilter::Prepare");
    filterCallback_->OnCallback(shared_from_this(), FilterCallBackCommand::NEXT_FILTER_NEEDED,
        StreamType::STREAMTYPE_ENCODED_VIDEO);
    return Status::OK;
}

Status MetaDataFilter::DoStart()
{
    MEDIA_LOG_I("Start");
    MediaAVCodec::AVCodecTrace trace("MetaDataFilter::Start");
    isStop_ = false;
    return Status::OK;
}

Status MetaDataFilter::DoPause()
{
    MEDIA_LOG_I("Pause");
    MediaAVCodec::AVCodecTrace trace("MetaDataFilter::Pause");
    isStop_ = true;
    latestPausedTime_ = latestBufferTime_;
    return Status::OK;
}

Status MetaDataFilter::DoResume()
{
    MEDIA_LOG_I("Resume");
    MediaAVCodec::AVCodecTrace trace("MetaDataFilter::Resume");
    isStop_ = false;
    refreshTotalPauseTime_ = true;
    return Status::OK;
}

Status MetaDataFilter::DoStop()
{
    MEDIA_LOG_I("Stop");
    MediaAVCodec::AVCodecTrace trace("MetaDataFilter::Stop");
    isStop_ = true;
    latestBufferTime_ = TIME_NONE;
    latestPausedTime_ = TIME_NONE;
    totalPausedTime_ = 0;
    refreshTotalPauseTime_ = false;
    return Status::OK;
}

Status MetaDataFilter::DoFlush()
{
    MEDIA_LOG_I("Flush");
    return Status::OK;
}

Status MetaDataFilter::DoRelease()
{
    MEDIA_LOG_I("Release");
    MediaAVCodec::AVCodecTrace trace("MetaDataFilter::Release");
    return Status::OK;
}

Status MetaDataFilter::NotifyEos()
{
    MEDIA_LOG_I("NotifyEos");
    return Status::OK;
}

void MetaDataFilter::SetParameter(const std::shared_ptr<Meta> &parameter)
{
    MEDIA_LOG_I("SetParameter");
    MediaAVCodec::AVCodecTrace trace("MetaDataFilter::SetParameter");
}

void MetaDataFilter::GetParameter(std::shared_ptr<Meta> &parameter)
{
    MEDIA_LOG_I("GetParameter");
    MediaAVCodec::AVCodecTrace trace("MetaDataFilter::GetParameter");
}

Status MetaDataFilter::LinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType)
{
    MEDIA_LOG_I("LinkNext");
    MediaAVCodec::AVCodecTrace trace("MetaDataFilter::LinkNext");
    nextFilter_ = nextFilter;
    nextFiltersMap_[outType].push_back(nextFilter_);
    std::shared_ptr<FilterLinkCallback> filterLinkCallback =
        std::make_shared<MetaDataFilterLinkCallback>(shared_from_this());
    nextFilter->OnLinked(outType, configureParameter_, filterLinkCallback);
    return Status::OK;
}

Status MetaDataFilter::UpdateNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType)
{
    MEDIA_LOG_I("UpdateNext");
    return Status::OK;
}

Status MetaDataFilter::UnLinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType)
{
    MEDIA_LOG_I("UnLinkNext");
    return Status::OK;
}

FilterType MetaDataFilter::GetFilterType()
{
    MEDIA_LOG_I("GetFilterType");
    return filterType_;
}

Status MetaDataFilter::OnLinked(StreamType inType, const std::shared_ptr<Meta> &meta,
    const std::shared_ptr<FilterLinkCallback> &callback)
{
    MEDIA_LOG_I("OnLinked");
    return Status::OK;
}

Status MetaDataFilter::OnUpdated(StreamType inType, const std::shared_ptr<Meta> &meta,
    const std::shared_ptr<FilterLinkCallback> &callback)
{
    MEDIA_LOG_I("OnUpdated");
    return Status::OK;
}

Status MetaDataFilter::OnUnLinked(StreamType inType, const std::shared_ptr<FilterLinkCallback>& callback)
{
    MEDIA_LOG_I("OnUnLinked");
    return Status::OK;
}

void MetaDataFilter::OnLinkedResult(const sptr<AVBufferQueueProducer> &outputBufferQueue,
    std::shared_ptr<Meta> &meta)
{
    MEDIA_LOG_I("OnLinkedResult");
    MediaAVCodec::AVCodecTrace trace("MetaDataFilter::OnLinkedResult");
    outputBufferQueueProducer_ = outputBufferQueue;
}

void MetaDataFilter::OnUpdatedResult(std::shared_ptr<Meta> &meta)
{
    MEDIA_LOG_I("OnUpdatedResult");
}

void MetaDataFilter::OnUnlinkedResult(std::shared_ptr<Meta> &meta)
{
    MEDIA_LOG_I("OnUnlinkedResult");
}

void MetaDataFilter::OnBufferAvailable()
{
    MediaAVCodec::AVCodecTrace trace("MetaDataFilter::OnBufferAvailable");
    sptr<SurfaceBuffer> buffer;
    sptr<SyncFence> fence;
    int64_t timestamp;
    int32_t bufferSize = 0;
    OHOS::Rect damage;
    GSError ret = inputSurface_->AcquireBuffer(buffer, fence, timestamp, damage);
    FALSE_RETURN(ret == GSERROR_OK && buffer != nullptr);
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
    MEDIA_LOG_D("timestamp:%{public}ld, dataSize:%{public}d", timestamp, bufferSize);
    if (timestamp == 0) {
        MEDIA_LOG_E("timestamp invalid.");
        inputSurface_->ReleaseBuffer(buffer, -1);
        return;
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
    bufferMem->Write((const uint8_t *)buffer->GetVirAddr(), bufferSize, 0);
    UpdateBufferConfig(emptyOutputBuffer, timestamp);
    status = outputBufferQueueProducer_->PushBuffer(emptyOutputBuffer, true);
    FALSE_LOG_MSG(status == Status::OK, "PushBuffer fail");
    inputSurface_->ReleaseBuffer(buffer, -1);
}

void MetaDataFilter::UpdateBufferConfig(std::shared_ptr<AVBuffer> buffer, int64_t timestamp)
{
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
    buffer->pts_ = timestamp - startBufferTime_ - totalPausedTime_;
    MediaAVCodec::AVCodecTrace trace("MetaDataFilter::UpdateBufferConfig");
    MEDIA_LOG_D("UpdateBufferConfig buffer->pts" PUBLIC_LOG_D64, buffer->pts_);
}

} // namespace Pipeline
} // namespace MEDIA
} // namespace OHOS
