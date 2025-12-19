/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

#define HST_LOG_TAG "SampleQueue"

#include <sstream>
#include <securec.h>
#include "common/log.h"
#include "sample_queue.h"
#include "avcodec_trace.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_PLAYER, "SampleQueue" };
constexpr int32_t INVALID_TRACK_ID = -1;
}

namespace OHOS {
namespace Media {

class SampleBufferConsumerListener : public IConsumerListener {
public:
    explicit SampleBufferConsumerListener(std::shared_ptr<SampleQueue> sampleQueue)
        : sampleQueue_(std::move(sampleQueue))
    {}
    virtual ~SampleBufferConsumerListener() = default;

    void OnBufferAvailable() override
    {
        if (auto sampleQueue = sampleQueue_.lock()) {
            sampleQueue->OnBufferConsumer();
        } else {
            MEDIA_LOG_E("consumer listener: Invalid sampleQueue instance.");
        }
    }

private:
    std::weak_ptr<SampleQueue> sampleQueue_;
};

class SampleBufferProducerListener : public IRemoteStub<IProducerListener> {
public:
    explicit SampleBufferProducerListener(std::shared_ptr<SampleQueue> sampleQueue)
        : sampleQueue_(std::move(sampleQueue))
    {}
    virtual ~SampleBufferProducerListener() = default;

    void OnBufferAvailable() override
    {
        if (auto sampleQueue = sampleQueue_.lock()) {
            sampleQueue->OnBufferAvailable();
        } else {
            MEDIA_LOG_E("prodecer listener: Invalid sampleQueue instance.");
        }
    }

private:
    std::weak_ptr<SampleQueue> sampleQueue_;
};

Status SampleQueue::Init(const Config& config)
{
    config_ = config;
    config_.queueSize_ = std::min(config.queueSize_, DEFAULT_SAMPLE_QUEUE_SIZE);
    config_.bufferCap_ = std::min(config.bufferCap_, MAX_SAMPLE_BUFFER_CAP);
    config_.queueName_ = "SampleQueue_" + std::to_string(config_.queueId_);
    sampleBufferQueue_ = AVBufferQueue::Create(config_.queueSize_, MemoryType::VIRTUAL_MEMORY, config_.queueName_);
    FALSE_RETURN_V_MSG_E(sampleBufferQueue_ != nullptr, Status::ERROR_NO_MEMORY, "AVBufferQueue::Create failed");
    if (config_.isFlvLiveStream_) {
        config_.queueSize_ = MAX_SAMPLE_QUEUE_SIZE;
        sampleBufferQueue_->SetLargerQueueSize(config_.queueSize_);
    }
    
    sampleBufferQueueProducer_ = sampleBufferQueue_->GetProducer();
    sptr<IProducerListener> producerListener = OHOS::sptr<SampleBufferProducerListener>::MakeSptr(shared_from_this());
    FALSE_RETURN_V_MSG_E(producerListener != nullptr, Status::ERROR_NO_MEMORY, "SampleBufferProducerListener nullptr");
    sampleBufferQueueProducer_->SetBufferAvailableListener(producerListener);

    sampleBufferQueueConsumer_ = sampleBufferQueue_->GetConsumer();
    sptr<IConsumerListener> consumerListener = new(std::nothrow) SampleBufferConsumerListener(shared_from_this());
    FALSE_RETURN_V_MSG_E(consumerListener != nullptr, Status::ERROR_NO_MEMORY, "SampleBufferConsumerListener nullptr");
    sampleBufferQueueConsumer_->SetBufferAvailableListener(consumerListener);

    MEDIA_LOG_I(PUBLIC_LOG_S " AVBufferQueue::Create queueSize_" PUBLIC_LOG_U32,
        config_.queueName_.c_str(),
        config_.queueSize_);
    return AttachBuffer();
}

Status SampleQueue::SetLargerQueueSize(uint32_t size)
{
    if (size != config_.queueSize_) {
        Status status = sampleBufferQueue_->SetLargerQueueSize(size);
        FALSE_RETURN_V_MSG_E(status == Status::OK, status, "SetLargerQueueSize failed status=" PUBLIC_LOG_D32,
            static_cast<int32_t>(status));
        MEDIA_LOG_I("sampleBufferQueue size is change to " PUBLIC_LOG_U32, size);
        config_.queueSize_ = size;
    }
    return Status::OK;
}

Status SampleQueue::AddQueueSize(uint32_t size)
{
    Status status = sampleBufferQueue_->SetLargerQueueSize(config_.queueSize_ + size);
    FALSE_RETURN_V(status == Status::OK, status);
    config_.queueSize_ = config_.queueSize_ + size;
    MEDIA_LOG_I("sampleBufferQueue size is add to " PUBLIC_LOG_U32, config_.queueSize_);
    return Status::OK;
}

Status SampleQueue::AttachBuffer()
{
    for (uint32_t i = 0; i < config_.queueSize_; i++) {
        auto avAllocator = AVAllocatorFactory::CreateVirtualAllocator();
        std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer(avAllocator, config_.bufferCap_);
        FALSE_RETURN_V_MSG_E(buffer != nullptr, Status::ERROR_NO_MEMORY, "CreateAVBuffer failed");
        Status status = sampleBufferQueueProducer_->AttachBuffer(buffer, false);
        FALSE_RETURN_V_MSG_E(
            status == Status::OK, status, "AttachBuffer failed status=" PUBLIC_LOG_D32, static_cast<int32_t>(status));
    }
    return Status::OK;
}

Status SampleQueue::SetSampleQueueCallback(std::shared_ptr<SampleQueueCallback> sampleQueueCb)
{
    sampleQueueCb_ = sampleQueueCb;
    return Status::OK;
}

sptr<AVBufferQueueProducer> SampleQueue::GetBufferQueueProducer() const
{
    return sampleBufferQueueProducer_;
}

Status SampleQueue::RequestBuffer(
    std::shared_ptr<AVBuffer> &sampleBuffer, const AVBufferConfig &config, int32_t timeoutMs)
{
    MEDIA_TRACE_DEBUG("SampleQueue::RequestBuffer");
    FALSE_RETURN_V(sampleBufferQueueProducer_ != nullptr, Status::ERROR_NULL_POINT_BUFFER);
    MEDIA_LOG_DD(PUBLIC_LOG_S " sampleBufferQueueProducer_ size=" PUBLIC_LOG_U32,
        config_.queueName_.c_str(),
        sampleBufferQueueProducer_->GetQueueSize());
    return sampleBufferQueueProducer_->RequestBuffer(sampleBuffer, config, timeoutMs);
}

Status SampleQueue::PushBuffer(std::shared_ptr<AVBuffer>& sampleBuffer, bool available)
{
    MEDIA_TRACE_DEBUG("SampleQueue::PushBuffer");
    FALSE_RETURN_V(sampleBuffer != nullptr && sampleBufferQueueProducer_ != nullptr, Status::ERROR_NULL_POINT_BUFFER);
    MEDIA_LOG_DD(PUBLIC_LOG_S " sampleBufferQueueProducer_ size=" PUBLIC_LOG_U32,
        config_.queueName_.c_str(),
        sampleBufferQueueProducer_->GetQueueSize());
    Status status = sampleBufferQueueProducer_->PushBuffer(sampleBuffer, available);
    FALSE_RETURN_V(available && status == Status::OK, status);

    if (!IsEosFrame(sampleBuffer)) {
        UpdateLastEnterSamplePts(sampleBuffer->pts_);
    }
    if (lastOutSamplePts_ == Plugins::HST_TIME_NONE) {
        lastOutSamplePts_ = lastEnterSamplePts_ - 1;
    }
    if (lastEnterSamplePts_ < lastOutSamplePts_) {
        lastOutSamplePts_ = lastEnterSamplePts_ - 1;
    }
    MEDIA_LOG_DD(PUBLIC_LOG_S " PushBuffer pts=" PUBLIC_LOG_D64 " dts=" PUBLIC_LOG_D64 " duration=" PUBLIC_LOG_D64,
        config_.queueName_.c_str(), sampleBuffer->pts_, sampleBuffer->dts_, sampleBuffer->duration_);
    if (!config_.isSupportBitrateSwitch_) {
        return Status::OK;
    }

    if (!IsKeyFrame(sampleBuffer)) {
        return Status::OK;
    }
    MEDIA_LOG_I(PUBLIC_LOG_S " insert Key Frame pts=" PUBLIC_LOG_D64, config_.queueName_.c_str(), sampleBuffer->pts_);
    {
        std::lock_guard<std::mutex> ptsLock(ptsMutex_);
        keyFramePtsSet_.insert(sampleBuffer->pts_);
    }

    {
        std::lock_guard<std::mutex> statusLock(statusMutex_);
        if (IsSwitchBitrateOK()) {
            NotifySwitchBitrateOK();
        }
    }
    return Status::OK;
}

Status SampleQueue::AttachOneBuffer(uint32_t size)
{
    for (uint32_t i = 0; i < size; i++) {
        auto avAllocator = AVAllocatorFactory::CreateVirtualAllocator();
        std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer(avAllocator, config_.bufferCap_);
        FALSE_RETURN_V_MSG_E(buffer != nullptr, Status::ERROR_NO_MEMORY, "CreateAVBuffer failed");
        Status status = sampleBufferQueueProducer_->AttachBuffer(buffer, false);
        FALSE_RETURN_V_MSG_E(
            status == Status::OK, status, "AttachBuffer failed status=" PUBLIC_LOG_D32, static_cast<int32_t>(status));
    }
    return Status::OK;
}

Status SampleQueue::AcquireBuffer(std::shared_ptr<AVBuffer>& sampleBuffer)
{
    MEDIA_TRACE_DEBUG("SampleQueue::AcquireBuffer");
    // return from rollbackBufferQueue_ first
    if (!rollbackBufferQueue_.empty()) {
        sampleBuffer = rollbackBufferQueue_.front();
        rollbackBufferQueue_.pop_front();
        MEDIA_LOG_DD(PUBLIC_LOG_S " AcquireBuffer from rollbackBufferQueue_", config_.queueName_.c_str());
    } else {
        FALSE_RETURN_V(sampleBufferQueueConsumer_ != nullptr, Status::ERROR_NULL_POINT_BUFFER);
        Status ret = sampleBufferQueueConsumer_->AcquireBuffer(sampleBuffer);
        FALSE_RETURN_V_NOLOG(ret == Status::OK, ret);
        MEDIA_LOG_DD(PUBLIC_LOG_S " bufferId: " PUBLIC_LOG_U64 ", pts: " PUBLIC_LOG_D64
                                 " GetCacheDuration= " PUBLIC_LOG_U64 " GetFilledBufferSize= " PUBLIC_LOG_U32,
            config_.queueName_.c_str(),
            sampleBuffer->GetUniqueId(),
            sampleBuffer->pts_,
            GetCacheDuration(),
            sampleBufferQueueConsumer_->GetFilledBufferSize());
    }

    if (!config_.isSupportBitrateSwitch_) {
        MEDIA_LOG_DD(PUBLIC_LOG_S " not SupportBitrateSwitch", config_.queueName_.c_str());
        return Status::OK;
    }
   
    if (IsKeyFrame(sampleBuffer)) {
        MEDIA_LOG_I(
            PUBLIC_LOG_S " erase Key Frame pts=" PUBLIC_LOG_D64, config_.queueName_.c_str(), sampleBuffer->pts_);
        std::lock_guard<std::mutex> ptsLock(ptsMutex_);
        keyFramePtsSet_.erase(sampleBuffer->pts_);
    }

    return Status::OK;
}

Status SampleQueue::AcquireCopyToDstBuffer(std::shared_ptr<AVBuffer>& dstBuffer)
{
    MEDIA_TRACE_DEBUG("SampleQueue::AcquireCopyToDstBuffer");
    MEDIA_LOG_DD(PUBLIC_LOG_S " AcquireCopyToDstBuffer in", config_.queueName_.c_str());
    FALSE_RETURN_V(dstBuffer != nullptr, Status::ERROR_NULL_POINT_BUFFER);

    std::shared_ptr<AVBuffer> srcBuffer;
    Status ret = AcquireBuffer(srcBuffer);
    FALSE_RETURN_V(ret == Status::OK && srcBuffer != nullptr, ret);

    ret = CopyBuffer(srcBuffer, dstBuffer);
    if (ret != Status::OK) {
        MEDIA_LOG_W(PUBLIC_LOG_S " AcquireCopyToDstBuffer fail ret=" PUBLIC_LOG_D32, config_.queueName_.c_str(), ret);
        RollbackBuffer(srcBuffer);
        return ret;
    }
    UpdateLastOutSamplePts(dstBuffer->pts_);

    ret = ReleaseBuffer(srcBuffer);
    MEDIA_LOG_DD(PUBLIC_LOG_S " AcquireCopyToDstBuffer out", config_.queueName_.c_str());
    return ret;
}

Status SampleQueue::CopyBuffer(std::shared_ptr<AVBuffer>& srcBuffer, std::shared_ptr<AVBuffer>& dstBuffer)
{
    // copy basic data
    dstBuffer->pts_ = srcBuffer->pts_;
    dstBuffer->dts_ = srcBuffer->dts_;
    dstBuffer->duration_ = srcBuffer->duration_;
    dstBuffer->flag_ = srcBuffer->flag_;

    CopyMeta(srcBuffer, dstBuffer);

    if (IsEosFrame(dstBuffer)) {
        MEDIA_LOG_I(PUBLIC_LOG_S " receive  IsEosFrame", config_.queueName_.c_str());
        return Status::OK;
    }
    return CopyAVMemory(srcBuffer, dstBuffer);
}


void SampleQueue::CopyMeta(std::shared_ptr<AVBuffer>& srcBuffer, std::shared_ptr<AVBuffer>& dstBuffer)
{
    if (srcBuffer->meta_ == nullptr) {
        dstBuffer->meta_ = nullptr;
        return;
    }

    int32_t trackId = INVALID_TRACK_ID;
    if (!dstBuffer->meta_->GetData(Tag::REGULAR_TRACK_ID, trackId)) {
        MEDIA_LOG_DD("trackId not found");
    }

    dstBuffer->meta_ = std::make_shared<Meta>(*(srcBuffer->meta_));
    if (dstBuffer->meta_ == nullptr) {
        return;
    }

    if (trackId != INVALID_TRACK_ID) {
        dstBuffer->meta_->SetData(Tag::REGULAR_TRACK_ID, trackId);
    }
}

Status SampleQueue::CopyAVMemory(std::shared_ptr<AVBuffer>& srcBuffer, std::shared_ptr<AVBuffer>& dstBuffer)
{
    std::shared_ptr<AVMemory>& srcMemory = srcBuffer->memory_;
    std::shared_ptr<AVMemory>& dstMemory = dstBuffer->memory_;
    if (!srcMemory || !dstMemory) {
        return Status::ERROR_NULL_POINT_BUFFER;
    }
    if (srcMemory->GetSize() > dstMemory->GetCapacity()) {
        MEDIA_LOG_E(PUBLIC_LOG_S " srcMemory->GetSize() " PUBLIC_LOG_U32 "dstMemory->GetCapacity()" PUBLIC_LOG_U32
                                    " srcMemory->GetOffset()" PUBLIC_LOG_U32,
            config_.queueName_.c_str(),
            srcMemory->GetSize(),
            dstMemory->GetCapacity(),
            srcMemory->GetOffset());
        return Status::ERROR_INVALID_BUFFER_SIZE;
    }

    errno_t copyRet = memcpy_s(dstMemory->GetAddr(),
        dstMemory->GetCapacity(),
        srcMemory->GetAddr() + srcMemory->GetOffset(),
        srcMemory->GetSize());
    if (copyRet != EOK) {
        return Status::ERROR_UNKNOWN;
    }
    dstMemory->SetSize(srcMemory->GetSize());
    dstMemory->SetOffset(srcMemory->GetOffset());
    return Status::OK;
}

Status SampleQueue::CopyBufferSlice(std::shared_ptr<AVBuffer>& srcBuffer, std::shared_ptr<AVBuffer>& dstBuffer,
    int32_t sliceSize)
{
    MEDIA_TRACE_DEBUG("SampleQueue::CopyPartBuffer");
    MEDIA_LOG_DD(PUBLIC_LOG_S " CopyPartBuffer in", config_.queueName_.c_str());
    FALSE_RETURN_V(dstBuffer, Status::ERROR_NULL_POINT_BUFFER);
    FALSE_RETURN_V(srcBuffer, Status::ERROR_NULL_POINT_BUFFER);

    Status ret = InnerCopySliceAVBuffer(srcBuffer, dstBuffer, sliceSize);
    if (ret != Status::OK) {
        MEDIA_LOG_W(PUBLIC_LOG_S " CopyPartBuffer fail ret: " PUBLIC_LOG_D32, config_.queueName_.c_str(), ret);
        RollbackBuffer(srcBuffer);
        return ret;
    }
    UpdateLastOutSamplePts(dstBuffer->pts_);
    MEDIA_LOG_DD(PUBLIC_LOG_S " CopyPartBuffer out", config_.queueName_.c_str());
    return ret;
}

Status SampleQueue::InnerCopySliceAVBuffer(std::shared_ptr<AVBuffer>& srcBuffer, std::shared_ptr<AVBuffer>& dstBuffer,
    int32_t sliceSize)
{
    // copy basic data
    dstBuffer->pts_ = srcBuffer->pts_;
    dstBuffer->dts_ = srcBuffer->dts_;
    dstBuffer->duration_ = srcBuffer->duration_;
    dstBuffer->flag_ = srcBuffer->flag_;

    CopyMeta(srcBuffer, dstBuffer);
    if (IsEosFrame(dstBuffer)) {
        MEDIA_LOG_I(PUBLIC_LOG_S " receive IsEosFrame", config_.queueName_.c_str());
        return Status::OK;
    }
    return InnerCopySliceAVMemory(srcBuffer, dstBuffer, sliceSize);
}

Status SampleQueue::InnerCopySliceAVMemory(std::shared_ptr<AVBuffer>& srcBuffer, std::shared_ptr<AVBuffer>& dstBuffer,
    int32_t sliceSize)
{
    auto &srcMemory = srcBuffer->memory_;
    auto &dstMemory = dstBuffer->memory_;
    if (!srcMemory || !dstMemory) {
        return Status::ERROR_NULL_POINT_BUFFER;
    }
    errno_t copyRet = memcpy_s(dstMemory->GetAddr(), dstMemory->GetCapacity(),
        srcMemory->GetAddr() + srcMemory->GetOffset(), sliceSize);
    if (copyRet != EOK) {
        return Status::ERROR_UNKNOWN;
    }
    int32_t offset = srcMemory->GetOffset() + sliceSize;
    srcMemory->SetOffset(offset);
    dstMemory->SetSize(sliceSize);
    dstMemory->SetOffset(0);
    MEDIA_LOG_D("CopyPartAVMemory sliceSize: %{public}d, srcOffset after: %{public}d", sliceSize,
        srcMemory->GetOffset());
    return Status::OK;
}

Status SampleQueue::ReleaseBuffer(std::shared_ptr<AVBuffer>& sampleBuffer)
{
    MEDIA_LOG_DD(PUBLIC_LOG_S " ReleaseBuffer", config_.queueName_.c_str());
    FALSE_RETURN_V(sampleBufferQueueConsumer_ != nullptr, Status::ERROR_NULL_POINT_BUFFER);
    MEDIA_LOG_DD(PUBLIC_LOG_S " bufferId: " PUBLIC_LOG_U64 ", pts: " PUBLIC_LOG_D64, config_.queueName_.c_str(),
        sampleBuffer->GetUniqueId(), sampleBuffer->pts_);
    // release AVMemory of srcBuffer as it's not allocate by SampleQueue
    if (sampleBuffer->memory_ != nullptr) {
        sampleBuffer->memory_ = nullptr;
    }
    Status status = sampleBufferQueueConsumer_->ReleaseBuffer(sampleBuffer);
    FALSE_RETURN_V_MSG_E(
        status == Status::OK, status, PUBLIC_LOG_S "ReleaseBuffer failed ", config_.queueName_.c_str());
    return status;
}

Status SampleQueue::RollbackBuffer(std::shared_ptr<AVBuffer>& sampleBuffer)
{
    MEDIA_LOG_DD(PUBLIC_LOG_S " RollbackBuffer", config_.queueName_.c_str());
    rollbackBufferQueue_.push_back(sampleBuffer);
    return Status::OK;
}

Status SampleQueue::QuerySizeForNextAcquireBuffer(size_t& size)
{
    std::shared_ptr<AVBuffer> sampleBuffer;
    if (!rollbackBufferQueue_.empty()) {
        sampleBuffer = rollbackBufferQueue_.front();
    } else {
        Status ret = AcquireBuffer(sampleBuffer);
        FALSE_RETURN_V_MSG_D(
            ret == Status::OK, ret, PUBLIC_LOG_S " failed ret=" PUBLIC_LOG_D32, config_.queueName_.c_str(), ret);
        SampleQueue::RollbackBuffer(sampleBuffer);
    }
    FALSE_RETURN_V(sampleBuffer != nullptr && sampleBuffer->memory_ != nullptr, Status::ERROR_NULL_POINT_BUFFER);
    size = sampleBuffer->memory_->GetSize();
    MEDIA_LOG_DD(PUBLIC_LOG_S " QuerySizeForNextAcquireBuffer size=" PUBLIC_LOG_ZU, config_.queueName_.c_str(), size);
    return Status::OK;
}

Status SampleQueue::Clear()
{
    MEDIA_LOG_I(PUBLIC_LOG_S " SampleQueue Clear", config_.queueName_.c_str());
    while (!rollbackBufferQueue_.empty()) {
        auto sampleBuffer = rollbackBufferQueue_.front();
        MEDIA_LOG_I(PUBLIC_LOG_S" clear rollbackBufferQueue_ bufferId: " PUBLIC_LOG_U64
        ", pts: " PUBLIC_LOG_D64, config_.queueName_.c_str(), sampleBuffer->GetUniqueId(), sampleBuffer->pts_);
        rollbackBufferQueue_.pop_front();
        ReleaseBuffer(sampleBuffer);
    }
    if (sampleBufferQueueProducer_ != nullptr) {
        sampleBufferQueueProducer_->Clear();
    }
    std::lock_guard<std::mutex> ptsLock(ptsMutex_);
    keyFramePtsSet_.clear();
    return Status::OK;
}

Status SampleQueue::DiscardSampleAfter(int64_t startPts)
{
    MEDIA_LOG_I(PUBLIC_LOG_S "DiscardSampleAfter startPts=" PUBLIC_LOG_D64, config_.queueName_.c_str(), startPts);
    {
        std::lock_guard<std::mutex> ptsLock(ptsMutex_);
        MEDIA_LOG_I("before DiscardSampleAfter keyFramePtsSet_ =" PUBLIC_LOG_S, SetToString(keyFramePtsSet_).c_str());
        auto it = keyFramePtsSet_.lower_bound(startPts);
        keyFramePtsSet_.erase(it, keyFramePtsSet_.end());
        lastEndSamplePts_ = startPts;
    }
    FALSE_RETURN_V(sampleBufferQueueProducer_ != nullptr, Status::ERROR_NULL_POINT_BUFFER);
    auto isNewerSample = [startPts](const std::shared_ptr<AVBuffer>& buffer) {
        return (buffer != nullptr) && (buffer->pts_ >= startPts);
    };
    return sampleBufferQueueProducer_->ClearBufferIf(isNewerSample);
}

Status SampleQueue::ReadySwitchBitrate(uint32_t bitrate)
{
    MediaAVCodec::AVCodecTrace trace("SampleQueue::ReadySwitchBitrate");
    if (!config_.isSupportBitrateSwitch_) {
        MEDIA_LOG_W("invalid operation for ReadySwitchBitrate=" PUBLIC_LOG_U32, bitrate);
        return Status::ERROR_INVALID_OPERATION;
    }
    std::lock_guard<std::mutex> statusLock(statusMutex_);
    if (switchStatus_ == SelectBitrateStatus::NORMAL) {
        nextSwitchBitrate_ = bitrate;
        switchStatus_ = SelectBitrateStatus::READY_SWITCH;
        if (IsSwitchBitrateOK()) {
            return NotifySwitchBitrateOK();
        }
    } else if (switchStatus_ == SelectBitrateStatus::READY_SWITCH) {
        // replace the old bitrate before SWITCHING
        MEDIA_LOG_W("replace new request bitrate from " PUBLIC_LOG_U32 " to"
             PUBLIC_LOG_U32, nextSwitchBitrate_, bitrate);
        nextSwitchBitrate_ = bitrate;
    } else if (switchStatus_ == SelectBitrateStatus::SWITCHING) {
        // incomming new bitrate just put switchBitrateWaitList_ when switching
        std::lock_guard<std::mutex> lockList(waitListMutex_);
        // drop the oldest bitrate in switchBitrateWaitList_
        if (switchBitrateWaitList_.size() >= MAX_BITRATE_SWITCH_WAIT_NUMBER) {
            uint32_t oldestBitrate = switchBitrateWaitList_.front();
            switchBitrateWaitList_.pop_front();
            MEDIA_LOG_I("switchBitrateWaitList_ remove oldestBitrate: " PUBLIC_LOG_U32, oldestBitrate);
        }
        MEDIA_LOG_I("switchBitrateWaitList_ add new bitrate: " PUBLIC_LOG_U32, bitrate);
        switchBitrateWaitList_.push_back(bitrate);
    }
    return Status::OK;
}

Status SampleQueue::NotifySwitchBitrateOK()
{
    {
        auto sampleQueueCb = sampleQueueCb_.lock();
        FALSE_RETURN_V(sampleQueueCb != nullptr, Status::ERROR_NULL_POINT_BUFFER);
        sampleQueueCb->OnSelectBitrateOk(startPtsToSwitch_, nextSwitchBitrate_);
    }
    switchStatus_ = SelectBitrateStatus::SWITCHING;
    MEDIA_LOG_I("SelectBitrateStatus::SWITCHING for startPtsToSwitch_=" PUBLIC_LOG_D64 ",nextSwitchBitrate_="
    PUBLIC_LOG_U32, startPtsToSwitch_, nextSwitchBitrate_);
    return Status::OK;
}

Status SampleQueue::UpdateLastEndSamplePts(int64_t lastEndSamplePts)
{
    MEDIA_LOG_DD("UpdateLastEndSamplePts lastEndSamplePts=" PUBLIC_LOG_D64, lastEndSamplePts);
    lastEndSamplePts_ = lastEndSamplePts;
    return Status::OK;
}

Status SampleQueue::UpdateLastOutSamplePts(int64_t lastOutSamplePts)
{
    MEDIA_LOG_DD("UpdateLastOutSamplePts lastOutSamplePts=" PUBLIC_LOG_D64, lastOutSamplePts);
    lastOutSamplePts_ = lastOutSamplePts;
    return Status::OK;
}

Status SampleQueue::UpdateLastEnterSamplePts(int64_t lastEnterSamplePts)
{
    MEDIA_LOG_DD("UpdateLastEnterSamplePts lastEnterSamplePts=" PUBLIC_LOG_D64, lastEnterSamplePts);
    lastEnterSamplePts_ = lastEnterSamplePts;
    return Status::OK;
}

Status SampleQueue::ResponseForSwitchDone(int64_t startPtsOnSwitch)
{
    MEDIA_LOG_I(PUBLIC_LOG_S " ResponseForSwitchDone startPtsOnSwitch=" PUBLIC_LOG_D64,
        config_.queueName_.c_str(),
        startPtsOnSwitch);

    Status ret = DiscardSampleAfter(startPtsOnSwitch);
    FALSE_RETURN_V_NOLOG(ret == Status::OK, ret);
    {
        std::lock_guard<std::mutex> statusLock(statusMutex_);
        if (switchStatus_ == SelectBitrateStatus::SWITCHING) {
            switchStatus_ = SelectBitrateStatus::NORMAL;
        }
        CheckSwitchBitrateWaitList();
    }
    return Status::OK;
}

void SampleQueue::CheckSwitchBitrateWaitList()
{
    std::lock_guard<std::mutex> lockList(waitListMutex_);
    auto it = switchBitrateWaitList_.begin();
    while (it != switchBitrateWaitList_.end()) {
        if (*it != nextSwitchBitrate_) {
            nextSwitchBitrate_ = *it;
            switchStatus_ = SelectBitrateStatus::READY_SWITCH;
            MEDIA_LOG_I("READY_SWITCH to nextSwitchBitrate_=" PUBLIC_LOG_U32, nextSwitchBitrate_);
            switchBitrateWaitList_.erase(it);
            break;
        } else {
            it = switchBitrateWaitList_.erase(it);
        }
    }
}

bool SampleQueue::IsSwitchBitrateOK()
{
    if (switchStatus_ != SelectBitrateStatus::READY_SWITCH) {
        return false;
    }

    if (!IsKeyFrameAvailable()) {
        return false;
    }
    int64_t cacheDiff = startPtsToSwitch_ - lastEndSamplePts_;
    MEDIA_LOG_I("IsSwitchBitrateOK cacheDiff=" PUBLIC_LOG_D64 ", startPtsToSwitch_=" PUBLIC_LOG_D64
                ", lastEndSamplePts_=" PUBLIC_LOG_D64,
        cacheDiff,
        startPtsToSwitch_,
        lastEndSamplePts_);
    return true;
}


bool SampleQueue::IsKeyFrameAvailable()
{
    std::lock_guard<std::mutex> ptsLock(ptsMutex_);
    MEDIA_LOG_I("keyFramePtsSet_ =" PUBLIC_LOG_S, SetToString(keyFramePtsSet_).c_str());
    auto it = keyFramePtsSet_.lower_bound(lastEndSamplePts_ + MIN_SWITCH_BITRATE_TIME_US);
    if (it != keyFramePtsSet_.end()) {
        startPtsToSwitch_ = *it;
        MEDIA_LOG_I("ok cache MIN_SWITCH_BITRATE_TIME_US with keyframe startpts=" PUBLIC_LOG_D64, startPtsToSwitch_);
    } else if (!keyFramePtsSet_.empty()) {
        startPtsToSwitch_ = *(keyFramePtsSet_.rbegin());
        MEDIA_LOG_I("ok with last keyframe startpts=" PUBLIC_LOG_D64, startPtsToSwitch_);
    } else {
        return false;
    }
    return true;
}

std::string SampleQueue::SetToString(std::set<int64_t> localSet)
{
    std::stringstream ss;
    for (auto it = localSet.begin(); it != localSet.end(); ++it) {
        if (it != localSet.begin()) {
            ss << " ";
        }
        ss << *it;
    }
    return ss.str();
}

bool SampleQueue::IsKeyFrame(std::shared_ptr<AVBuffer>& sampleBuffer) const
{
    return (sampleBuffer != nullptr) &&
           (sampleBuffer->flag_ & static_cast<uint32_t>(Plugins::AVBufferFlag::SYNC_FRAME));
}

bool SampleQueue::IsEosFrame(std::shared_ptr<AVBuffer>& sampleBuffer) const
{
    return (sampleBuffer != nullptr) && (sampleBuffer->flag_ & static_cast<uint32_t>(Plugins::AVBufferFlag::EOS));
}

void SampleQueue::OnBufferAvailable()
{
    MEDIA_LOG_DD(PUBLIC_LOG_S " OnBufferAvailable sampleBufferQueueProducer_ size=" PUBLIC_LOG_U32,
        config_.queueName_.c_str(),
        sampleBufferQueueProducer_->GetQueueSize());
    auto sampleQueueCb = sampleQueueCb_.lock();
    if (sampleQueueCb != nullptr) {
        MEDIA_LOG_D(PUBLIC_LOG_S " OnSampleQueueBufferAvailable ", config_.queueName_.c_str());
        sampleQueueCb->OnSampleQueueBufferAvailable(config_.queueId_);
    }
}

void SampleQueue::OnBufferConsumer()
{
    MEDIA_LOG_DD(PUBLIC_LOG_S " OnBufferConsumer ", config_.queueName_.c_str());
    auto sampleQueueCb = sampleQueueCb_.lock();
    if (sampleQueueCb != nullptr) {
        MEDIA_LOG_DD(PUBLIC_LOG_S " OnSampleQueueBufferConsume ", config_.queueName_.c_str());
        sampleQueueCb->OnSampleQueueBufferConsume(config_.queueId_);
    }
}

void SampleQueue::UpdateQueueId(int32_t queueId)
{
    MEDIA_LOG_I(PUBLIC_LOG_S " change queueId to " PUBLIC_LOG_D32, config_.queueName_.c_str(), queueId);
    config_.queueId_ = queueId;
}

uint64_t SampleQueue::GetCacheDuration() const
{
    if (lastEnterSamplePts_ == Plugins::HST_TIME_NONE || lastOutSamplePts_ == Plugins::HST_TIME_NONE) {
        return 0;
    }
    int64_t diff = lastEnterSamplePts_ - lastOutSamplePts_;
    MEDIA_LOG_DD(PUBLIC_LOG_S " lastEnterSamplePts_=" PUBLIC_LOG_D64 " lastEndSamplePts_=" PUBLIC_LOG_D64
        " diff=" PUBLIC_LOG_D64, config_.queueName_.c_str(), lastEnterSamplePts_, lastOutSamplePts_, diff);
    return (diff > 0) ? static_cast<uint64_t>(diff) : 0;
}

uint64_t SampleQueue::NewGetCacheDuration() const
{
    auto lastEnterSamplePts = GetLastEnterSamplePts();
    auto lastOutSamplePts = GetLastOutSamplePts();
    if (lastEnterSamplePts >= 0 && lastOutSamplePts >= 0 && lastEnterSamplePts > lastOutSamplePts) {
        return lastEnterSamplePts - lastOutSamplePts;
    } else {
        return 0;
    }
}

int64_t SampleQueue::GetLastEnterSamplePts() const
{
    if (lastEnterSamplePts_ == Plugins::HST_TIME_NONE || lastEnterSamplePts_ < 0) {
        return 0;
    }
    return lastEnterSamplePts_;
}

int64_t SampleQueue::GetLastOutSamplePts() const
{
    if (lastOutSamplePts_ == Plugins::HST_TIME_NONE || lastOutSamplePts_ < 0) {
        return 0;
    }
    return lastOutSamplePts_;
}

uint32_t SampleQueue::GetMemoryUsage()
{
    FALSE_RETURN_V_MSG_E(sampleBufferQueue_ != nullptr, 0, "bufferQueue nullptr");
    return sampleBufferQueue_->GetMemoryUsage();
}

std::string SampleQueue::StringifyMeta(std::shared_ptr<Meta> &meta)
{
    FALSE_RETURN_V(meta != nullptr, "");
    std::stringstream dumpStream;
    for (auto iter = meta->begin(); iter != meta->end(); ++iter) {
        switch (meta->GetValueType(iter->first)) {
            case AnyValueType::INT32_T:
                dumpStream << iter->first << " = " << std::to_string(AnyCast<int32_t>(iter->second)) << " | ";
                break;
            case AnyValueType::UINT32_T:
                dumpStream << iter->first << " = " << std::to_string(AnyCast<uint32_t>(iter->second)) << " | ";
                break;
            case AnyValueType::BOOL:
                dumpStream << iter->first << " = " << std::to_string(AnyCast<bool>(iter->second)) << " | ";
                break;
            case AnyValueType::DOUBLE:
                dumpStream << iter->first << " = " << std::to_string(AnyCast<double>(iter->second)) << " | ";
                break;
            case AnyValueType::INT64_T:
                dumpStream << iter->first << " = " << std::to_string(AnyCast<int64_t>(iter->second)) << " | ";
                break;
            case AnyValueType::FLOAT:
                dumpStream << iter->first << " = " << std::to_string(AnyCast<float>(iter->second)) << " | ";
                break;
            case AnyValueType::STRING:
                dumpStream << iter->first << " = " << AnyCast<std::string>(iter->second) << " | ";
                break;
            default:
                dumpStream << iter->first << " = " << "unknown type | ";
                break;
        }
    }
    return dumpStream.str();
}

bool SampleQueue::IsEmpty()
{
    return sampleBufferQueueConsumer_->GetFilledBufferSize() == 0;
}

uint32_t SampleQueue::GetFilledBufferSize()
{
    auto filledSize = sampleBufferQueueConsumer_->GetFilledBufferSize();
    MEDIA_LOG_DD("GetFilledBufferSize filled buffer: %{public}u, queue size: %{public}u, track id: %{public}d",
        filledSize, config_.queueSize_, config_.queueId_);
    return filledSize;
}
} // namespace Media
} // namespace OHOS
