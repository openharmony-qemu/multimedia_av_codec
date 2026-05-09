/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "side_output_surface_processor.h"
#include "buffer/avbuffer.h"
#include "common/log.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_SYSTEM_PLAYER, "SideOutputSurfaceProcessor" };
constexpr uint32_t WAIT_FOR_EVER = std::numeric_limits<uint32_t>::max();
constexpr uint32_t BUFFER_QUEUE_SIZE = 5;
constexpr int32_t STRIDE_ALIGNMENT = 32;
constexpr int32_t WAIT_TRIGGER_TIMEOUT = 200;
constexpr int32_t GET_VIDEO_SAMPLE_TIMEOUT = 1000;
constexpr size_t MAX_RENDER_BUFFER_QUEUE_SIZE = 5;
}

namespace OHOS {
namespace Media {

static bool IsSideOutputSurfaceSupported(const std::shared_ptr<Meta>& meta)
{
    FALSE_RETURN_V(meta != nullptr, false);
    return true;
}

static AutoRegisterPostProcessor<SideOutputSurfaceProcessor> g_registerSideOutputSurfaceProcessor(
    VideoPostProcessorType::SIDE_OUTPUT, []() -> std::shared_ptr<BaseVideoPostProcessor> {
        auto postProcessor = std::make_shared<SideOutputSurfaceProcessor>();
        return postProcessor;
    }, &IsSideOutputSurfaceSupported);

class ConsumerSurfaceBufferListener : public IBufferConsumerListener {
public:
    explicit ConsumerSurfaceBufferListener(std::weak_ptr<SideOutputSurfaceProcessor> processor)
        : interPostProcessor_(processor) {}

    void OnBufferAvailable() override
    {
        auto processor = interPostProcessor_.lock();
        if (processor != nullptr) {
            processor->OnConsumerBufferAvailable();
        }
    }

private:
    std::weak_ptr<SideOutputSurfaceProcessor> interPostProcessor_;
};

SideOutputSurfaceProcessor::SideOutputSurfaceProcessor()
{
    MEDIA_LOG_I("SideOutputSurfaceProcessor ctor");
}

SideOutputSurfaceProcessor::~SideOutputSurfaceProcessor()
{
    MEDIA_LOG_I("SideOutputSurfaceProcessor dtor");
    Release();
}

Status SideOutputSurfaceProcessor::Init()
{
    MEDIA_LOG_I("SideOutputSurfaceProcessor Init");
    std::lock_guard<std::mutex> lock(lock_);
    if (isInitialized_.load()) {
        MEDIA_LOG_I("Already initialize!");
        return Status::OK;
    }

    sampleTask_ = std::make_shared<Task>("SideOutputSample", "SideOutputSurfaceProcessor",
        TaskType::SINGLETON, TaskPriority::HIGH, false);

    isRunning_.store(true);
    worker_ = std::thread([this]() {
        while (isRunning_.load()) {
            if (!WaitTrigger()) {
                MEDIA_LOG_I("continue-----------");
                continue;
            }
            ProcessBuffers();
        }
    });
    isInitialized_.store(true);
    return Status::OK;
}

Status SideOutputSurfaceProcessor::Flush()
{
    MEDIA_LOG_I("SideOutputSurfaceProcessor Flush");
    {
        std::lock_guard<std::mutex> lock(lock_);
        if (producerSurface_ != nullptr) {
            MEDIA_LOG_I("Flush: CleanCache producerSurface");
            producerSurface_->CleanCache(false);
        }
        if (sideSurface_ != nullptr) {
            MEDIA_LOG_I("Flush: CleanCache sideSurface");
            sideSurface_->CleanCache(false);
        }
    }

    ClearBufferQueues();

    return Status::OK;
}

Status SideOutputSurfaceProcessor::Stop()
{
    MEDIA_LOG_I("SideOutputSurfaceProcessor Stop");
    std::lock_guard<std::mutex> lock(lock_);
    state_.store(ProcessorState::STOPPING);
    isPaused_.store(true);
    cvTrigger_.notify_one();
    return Status::OK;
}

Status SideOutputSurfaceProcessor::Start()
{
    MEDIA_LOG_I("SideOutputSurfaceProcessor Start");
    std::lock_guard<std::mutex> lock(lock_);
    if (state_.load() == ProcessorState::RUNNING) {
        return Status::OK;
    }
    state_.store(ProcessorState::RUNNING);
    isPaused_.store(false);
    cvTrigger_.notify_one();
    return Status::OK;
}

Status SideOutputSurfaceProcessor::Release()
{
    MEDIA_LOG_I("SideOutputSurfaceProcessor Release");
    {
        std::lock_guard<std::mutex> lock(lock_);
        if (!isInitialized_.load()) {
            MEDIA_LOG_I("Already deinitialize!");
            return Status::OK;
        }
        isRunning_.store(false);
        isInitialized_.store(false);
        isPaused_.store(false);
        if (state_.load() == ProcessorState::RUNNING) {
            state_.store(ProcessorState::STOPPING);
        }
    }
    cvTrigger_.notify_all();
    if (worker_.joinable()) {
        MEDIA_LOG_I("Wait for thread ending...");
        worker_.join();
    }

    if (sampleTask_ != nullptr) {
        sampleTask_->Stop();
        sampleTask_ = nullptr;
    }

    ClearBufferQueues();

    {
        std::lock_guard<std::mutex> lock(lock_);
        callback_ = nullptr;
        if (consumerSurface_ != nullptr) {
            consumerSurface_->UnregisterConsumerListener();
            consumerSurface_ = nullptr;
        }
        if (producerSurface_ != nullptr) {
            producerSurface_->UnRegisterReleaseListener();
            producerSurface_->Disconnect();
            producerSurface_->CleanCache(true);
            producerSurface_ = nullptr;
        }
        sideSurface_ = nullptr;
    }
    return Status::OK;
}

sptr<Surface> SideOutputSurfaceProcessor::GetInputSurface()
{
    MEDIA_LOG_I("SideOutputSurfaceProcessor GetInputSurface");
    std::lock_guard<std::mutex> lock(lock_);
    if (consumerSurface_ != nullptr) {
        return producerSurface_;
    }

    consumerSurface_ = Surface::CreateSurfaceAsConsumer("SideOutputSurfaceProcessorConsumer");
    FALSE_RETURN_V_MSG_I(consumerSurface_ != nullptr, nullptr, "consumerSurface create failed");

    sptr<IBufferProducer> producer = consumerSurface_->GetProducer();
    FALSE_RETURN_V_MSG_I(producer != nullptr, nullptr, "Get surface producer fail");
    producerSurface_ = Surface::CreateSurfaceAsProducer(producer);
    if (producerSurface_ == nullptr) {
        MEDIA_LOG_E("CreateSurfaceAsProducer fail");
        consumerSurface_ = nullptr;
        return nullptr;
    }

    producerSurface_->SetDefaultUsage(BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE |
        BUFFER_USAGE_MEM_DMA | BUFFER_USAGE_MEM_MMZ_CACHE | BUFFER_USAGE_HW_COMPOSER);
    consumerSurface_->SetQueueSize(BUFFER_QUEUE_SIZE);
    MEDIA_LOG_I("Set input buffer queue size to %{public}u", BUFFER_QUEUE_SIZE);

    sptr<IBufferConsumerListener> listener = sptr<ConsumerSurfaceBufferListener>::MakeSptr(shared_from_this());
    consumerSurface_->RegisterConsumerListener(listener);

    return producerSurface_;
}

Status SideOutputSurfaceProcessor::SetOutputSurface(sptr<Surface> surface)
{
    MEDIA_LOG_I("SideOutputSurfaceProcessor SetOutputSurface");
    FALSE_RETURN_V_MSG(surface != nullptr, Status::ERROR_NULL_SURFACE, "SetOutputSurface null surface");
    FALSE_RETURN_V_MSG_W(!surface->IsConsumer(), Status::ERROR_INVALID_PARAMETER, "surface is NOT producer");

    std::lock_guard<std::mutex> lock(lock_);
    if (producerSurface_ != nullptr) {
        if (producerSurface_->GetUniqueId() == surface->GetUniqueId()) {
            return Status::OK;
        }
        producerSurface_->UnRegisterReleaseListener();
        producerSurface_->CleanCache(true);
    }
    surface->UnRegisterReleaseListener();
    std::weak_ptr<SideOutputSurfaceProcessor> weakProcessor = shared_from_this();
    GSError err = surface->RegisterReleaseListener([weakProcessor](sptr<SurfaceBuffer>& buffer) {
        (void)buffer;
        auto processor = weakProcessor.lock();
        if (processor != nullptr) {
            return processor->OnProducerBufferReleased();
        }
        return GSERROR_OK;
    });
    FALSE_RETURN_V_MSG(err == GSERROR_OK, Status::ERROR_INVALID_OPERATION, "RegisterReleaseListener failed");
    surface->SetQueueSize(BUFFER_QUEUE_SIZE);
    surface->Connect();
    surface->CleanCache();

    producerSurface_ = surface;
    return Status::OK;
}
 
Status SideOutputSurfaceProcessor::ReleaseOutputBuffer(uint32_t index, bool render)
{
    MEDIA_LOG_I("SideOutputSurfaceProcessor ReleaseOutputBuffer index: %{public}u, render: %{public}d", index, render);
    return RenderOutputBufferInner(index, -1, render);
}

Status SideOutputSurfaceProcessor::RenderOutputBufferAtTime(uint32_t index, int64_t renderTimestampNs)
{
    return RenderOutputBufferInner(index, renderTimestampNs, true);
}

Status SideOutputSurfaceProcessor::SetCallback(const std::shared_ptr<PostProcessorCallback> callback)
{
    MEDIA_LOG_I("SideOutputSurfaceProcessor SetCallback");
    callback_ = callback;
    return Status::OK;
}

Status SideOutputSurfaceProcessor::SetEventReceiver(const std::shared_ptr<Pipeline::EventReceiver> &receiver)
{
    MEDIA_LOG_I("SideOutputSurfaceProcessor SetEventReceiver");
    eventReceiver_ = receiver;
    return Status::OK;
}

Status SideOutputSurfaceProcessor::SetParameter(const Format &format)
{
    MEDIA_LOG_I("SideOutputSurfaceProcessor SetParameter");
    return Status::OK;
}

Status SideOutputSurfaceProcessor::SetPostProcessorOn(bool isPostProcessorOn)
{
    MEDIA_LOG_I("SideOutputSurfaceProcessor SetPostProcessorOn isPostProcessorOn: %{public}d", isPostProcessorOn);
    return Status::OK;
}

Status SideOutputSurfaceProcessor::SetVideoWindowSize(int32_t width, int32_t height)
{
    MEDIA_LOG_I("SideOutputSurfaceProcessor SetVideoWindowSize width: %{public}d, height: %{public}d", width, height);
    return Status::OK;
}

Status SideOutputSurfaceProcessor::NotifyEos(int64_t eosPts)
{
    MEDIA_LOG_I("SideOutputSurfaceProcessor NotifyEos eosPts: " PUBLIC_LOG_D64, eosPts);
    return Status::OK;
}

void SideOutputSurfaceProcessor::SetVideoOutput(sptr<Surface> sideOutputSurface)
{
    MEDIA_LOG_I("SideOutputSurfaceProcessor SetVideoOutput");
    std::lock_guard<std::mutex> lock(lock_);
    sideSurface_ = sideOutputSurface;
}

void SideOutputSurfaceProcessor::OnConsumerBufferAvailable()
{
    MEDIA_LOG_I("OnConsumerBufferAvailable start");
    if (!isRunning_.load()) {
        MEDIA_LOG_I("Skip when died.");
        return;
    }
    if (state_.load() != ProcessorState::RUNNING) {
        MEDIA_LOG_I("NOT running now!");
        return;
    }

    sptr<Surface> consumerSurface;
    {
        std::lock_guard<std::mutex> lock(lock_);
        consumerSurface = consumerSurface_;
    }

    if (consumerSurface == nullptr) {
        MEDIA_LOG_E("consumerSurface is null!");
        return;
    }

    SurfaceBufferInfo bufferInfo{};
    OHOS::Rect damage;
    GSError err = consumerSurface->AcquireBuffer(bufferInfo.buffer, bufferInfo.fence, bufferInfo.timestamp, damage);
    if (err != GSERROR_OK || bufferInfo.buffer == nullptr) {
        MEDIA_LOG_E("Failed to acquire buffer, ret: %{public}d!", err);
        return;
    }
    if (bufferInfo.fence != nullptr) {
        bufferInfo.fence->Wait(WAIT_FOR_EVER);
    }

    PushBufferToConsumer(bufferInfo);
}

void SideOutputSurfaceProcessor::PushBufferToConsumer(SurfaceBufferInfo& bufferInfo)
{
    bool needPrepare = false;
    bool resolutionChanged = false;
    {
        std::lock_guard<std::mutex> consumerBufferLock(consumerBufferLock_);
        if (!isBufferQueueReady_.load()) {
            isBufferQueueReady_ = true;
            requestCfg_.usage = bufferInfo.buffer->GetUsage();
            requestCfg_.timeout = 0;
            requestCfg_.strideAlignment = STRIDE_ALIGNMENT;
            requestCfg_.width = bufferInfo.buffer->GetWidth();
            requestCfg_.height = bufferInfo.buffer->GetHeight();
            requestCfg_.format = bufferInfo.buffer->GetFormat();
            MEDIA_LOG_I("Use requestCfg_ to prepare buffers.");
            needPrepareBuffers_.store(true);
        } else if (requestCfg_.width != bufferInfo.buffer->GetWidth() ||
                requestCfg_.height != bufferInfo.buffer->GetHeight()) {
            requestCfg_.width = bufferInfo.buffer->GetWidth();
            requestCfg_.height = bufferInfo.buffer->GetHeight();
            requestCfg_.format = bufferInfo.buffer->GetFormat();
            resolutionChanged = true;
            MEDIA_LOG_E("Resolution changed: %{public}u x %{public}u", requestCfg_.width, requestCfg_.height);
        }
        consumerBufferQueue_.push(bufferInfo);
        if (needPrepareBuffers_.load()) {
            needPrepareBuffers_.store(false);
            needPrepare = true;
        }
    }
    if (needPrepare) {
        OnProducerBufferReleased();
    }
    if (resolutionChanged) {
        sptr<Surface> producerSurface;
        {
            std::lock_guard<std::mutex> lock(lock_);
            producerSurface = producerSurface_;
        }
        if (producerSurface != nullptr) {
            producerSurface->SetDefaultWidthAndHeight(requestCfg_.width, requestCfg_.height);
            std::lock_guard<std::mutex> bufferLock(bufferLock_);
            while (!producerBufferQueue_.empty()) {
                auto oldBuffer = producerBufferQueue_.front();
                producerBufferQueue_.pop();
                if (oldBuffer.buffer != nullptr) {
                    producerSurface->CancelBuffer(oldBuffer.buffer);
                }
            }
        }
        OnProducerBufferReleased();
    }
    cvTrigger_.notify_one();
}

GSError SideOutputSurfaceProcessor::OnProducerBufferReleased()
{
    MEDIA_LOG_I("OnProducerBufferReleased step in");
    if (!isRunning_.load()) {
        MEDIA_LOG_I("Skip when died.");
        return GSERROR_OK;
    }
    if (state_.load() == ProcessorState::STOPPING) {
        MEDIA_LOG_I("Skip when stopping.");
        return GSERROR_OK;
    }

    if (!isBufferQueueReady_.load()) {
        MEDIA_LOG_W("Skip OnProducerBufferReleased, buffer queue not ready");
        return GSERROR_OK;
    }

    sptr<Surface> producerSurface;
    {
        std::lock(lock_, bufferLock_);
        std::lock_guard<std::mutex> bufferGuard(bufferLock_, std::adopt_lock);
        std::lock_guard<std::mutex> lockGuard(lock_, std::adopt_lock);

        producerSurface = producerSurface_;

        if (producerSurface == nullptr) {
            return GSERROR_OK;
        }

        GSError err = GSERROR_OK;
        SurfaceBufferInfo bufferInfo{};
        err = producerSurface->RequestBuffer(bufferInfo.buffer, bufferInfo.fence, requestCfg_);
        if (err != GSERROR_OK || bufferInfo.buffer == nullptr) {
            MEDIA_LOG_W("Failed to RequestBuffer, ret: %{public}d", err);
            if (bufferInfo.buffer != nullptr && producerSurface != nullptr) {
                producerSurface->CancelBuffer(bufferInfo.buffer);
            }
            return err;
        }
        producerBufferQueue_.push(bufferInfo);
        MEDIA_LOG_I("producerSurface->RequestBuffer id: %{public}u", bufferInfo.buffer->GetSeqNum());
    }

    if (state_.load() != ProcessorState::IDLE) {
        cvTrigger_.notify_one();
    }
    MEDIA_LOG_I("OnProducerBufferReleased step out");
    return GSERROR_OK;
}

bool SideOutputSurfaceProcessor::GetConsumerAndProducerBuffer(SurfaceBufferInfo& srcBufferInfo,
                                                              SurfaceBufferInfo& dstBufferInfo)
{
    std::lock(consumerBufferLock_, bufferLock_);
    std::lock_guard<std::mutex> consumerBufferGuard(consumerBufferLock_, std::adopt_lock);
    std::lock_guard<std::mutex> bufferGuard(bufferLock_, std::adopt_lock);

    if (producerBufferQueue_.empty() || consumerBufferQueue_.empty()) {
        return false;
    }
    srcBufferInfo = consumerBufferQueue_.front();
    if (srcBufferInfo.buffer == nullptr) {
        consumerBufferQueue_.pop();
        MEDIA_LOG_W("input buffer is null!");
        return false;
    }
    dstBufferInfo = producerBufferQueue_.front();
    if (dstBufferInfo.buffer == nullptr) {
        producerBufferQueue_.pop();
        MEDIA_LOG_W("output buffer is null!");
        return false;
    }
    consumerBufferQueue_.pop();
    producerBufferQueue_.pop();
    return true;
}

bool SideOutputSurfaceProcessor::ProcessBuffer(SurfaceBufferInfo& srcBufferInfo, SurfaceBufferInfo& dstBufferInfo)
{
    MEDIA_LOG_D("ProcessBuffer srcBuffer(consumer input): width=%{public}d, height=%{public}d, format=%{public}d",
        srcBufferInfo.buffer->GetWidth(), srcBufferInfo.buffer->GetHeight(), srcBufferInfo.buffer->GetFormat());
    MEDIA_LOG_D("ProcessBuffer dstBuffer(producer output): width=%{public}d, height=%{public}d, format=%{public}d",
        dstBufferInfo.buffer->GetWidth(), dstBufferInfo.buffer->GetHeight(), dstBufferInfo.buffer->GetFormat());

    srcBufferInfo.buffer->InvalidateCache();

    dstBufferInfo.timestamp = srcBufferInfo.timestamp;

    void* srcAddr = srcBufferInfo.buffer->GetVirAddr();
    void* dstAddr = dstBufferInfo.buffer->GetVirAddr();

    if (srcAddr != nullptr && dstAddr != nullptr) {
        uint32_t size = srcBufferInfo.buffer->GetSize();
        if (size > dstBufferInfo.buffer->GetSize()) {
            MEDIA_LOG_E("Buffer size mismatch: src=%{public}u > dst=%{public}u", size, dstBufferInfo.buffer->GetSize());
            sptr<Surface> consumerSurface;
            sptr<Surface> producerSurface;
            {
                std::lock_guard<std::mutex> lock(lock_);
                consumerSurface = consumerSurface_;
                producerSurface = producerSurface_;
            }
            if (consumerSurface != nullptr) {
                consumerSurface->ReleaseBuffer(srcBufferInfo.buffer, -1);
            }
            if (producerSurface != nullptr && dstBufferInfo.buffer != nullptr) {
                producerSurface->CancelBuffer(dstBufferInfo.buffer);
            }
            OnProducerBufferReleased();
            return false;
        }
        if (size > 0) {
            errno_t ret = memcpy_s(dstAddr, size, srcAddr, size);
            if (ret != EOK) {
                MEDIA_LOG_E("ProcessBuffer memcpy_s failed, ret=%{public}d", ret);
                return false;
            }
        }
    }

    sptr<Surface> consumerSurface;
    {
        std::lock_guard<std::mutex> lock(lock_);
        consumerSurface = consumerSurface_;
    }

    if (consumerSurface != nullptr) {
        consumerSurface->ReleaseBuffer(srcBufferInfo.buffer, -1);
    }

    OutputBuffer(dstBufferInfo);
    return true;
}
 
void SideOutputSurfaceProcessor::OutputBuffer(const SurfaceBufferInfo& bufferInfo)
{
    sptr<SurfaceBuffer> bufferToCancel = nullptr;
    {
        std::lock_guard<std::mutex> bufferLock(bufferLock_);
        if (renderBufferQueue_.size() >= MAX_RENDER_BUFFER_QUEUE_SIZE) {
            auto oldest = renderBufferQueue_.begin();
            if (oldest != renderBufferQueue_.end() && oldest->second.buffer != nullptr) {
                MEDIA_LOG_W("renderBufferQueue overflow, drop oldest buffer id: %{public}u",
                    oldest->second.buffer->GetSeqNum());
                bufferToCancel = oldest->second.buffer;
            }
            renderBufferQueue_.erase(oldest);
        }
        renderBufferQueue_[bufferInfo.buffer->GetSeqNum()] = bufferInfo;
        MEDIA_LOG_D("renderBufferQueue_.push id: %{public}u size: %{public}zu",
            bufferInfo.buffer->GetSeqNum(), renderBufferQueue_.size());
    }

    if (bufferToCancel != nullptr) {
        sptr<Surface> producerSurface;
        {
            std::lock_guard<std::mutex> lock(lock_);
            producerSurface = producerSurface_;
        }
        if (producerSurface != nullptr) {
            producerSurface->CancelBuffer(bufferToCancel);
        }
    }

    std::shared_ptr<PostProcessorCallback> callback;
    {
        std::lock_guard<std::mutex> lock(lock_);
        callback = callback_;
    }

    if (callback != nullptr) {
        MEDIA_LOG_I("OnOutputBufferAvailable start");
        auto avBuffer = AVBuffer::CreateAVBuffer(bufferInfo.buffer);
        FALSE_RETURN_MSG(avBuffer != nullptr, "avBuffer is nullptr");
        avBuffer->pts_ = bufferInfo.timestamp;
        callback->OnOutputBufferAvailable(bufferInfo.buffer->GetSeqNum(), avBuffer);
    }
}

Status SideOutputSurfaceProcessor::RenderOutputBufferInner(uint32_t index, int64_t renderTimestamp, bool render)
{
    if (!isRunning_.load()) {
        MEDIA_LOG_I("Skip when died.");
        return Status::ERROR_INVALID_OPERATION;
    }

    SurfaceBufferInfo bufferInfo;
    if (!FindBufferFromRenderQueue(index, bufferInfo)) {
        return Status::ERROR_INVALID_PARAMETER;
    }

    if (render) {
        return FlushBufferToProducer(bufferInfo, renderTimestamp);
    } else {
        DealNotRenderBuffer(bufferInfo);
        return Status::OK;
    }
}

bool SideOutputSurfaceProcessor::FindBufferFromRenderQueue(uint32_t index, SurfaceBufferInfo& bufferInfo)
{
    std::lock_guard<std::mutex> bufferLock(bufferLock_);
    auto it = renderBufferQueue_.find(index);
    if (it != renderBufferQueue_.end()) {
        bufferInfo = it->second;
        renderBufferQueue_.erase(it);
        return true;
    }

    for (auto iter = renderBufferQueue_.begin(); iter != renderBufferQueue_.end(); ++iter) {
        if (iter->second.buffer->GetSeqNum() == index) {
            bufferInfo = iter->second;
            renderBufferQueue_.erase(iter);
            return true;
        }
    }

    MEDIA_LOG_E("Invalid input: index=%{public}u!", index);
    return false;
}

Status SideOutputSurfaceProcessor::FlushBufferToProducer(SurfaceBufferInfo& bufferInfo, int64_t renderTimestamp)
{
    BufferFlushConfig flushCfg;
    flushCfg.damage.w = bufferInfo.buffer->GetWidth();
    flushCfg.damage.h = bufferInfo.buffer->GetHeight();
    flushCfg.timestamp = (renderTimestamp == -1) ? bufferInfo.timestamp : renderTimestamp;

    {
        std::lock_guard<std::mutex> bufferLock(bufferLock_);
        pendingSideSrcBuffer_ = bufferInfo.buffer;
        pendingSideTimestamp_ = bufferInfo.timestamp;
        MEDIA_LOG_I("FlushBufferToProducer: saved buffer ref for GetVideoSample");
    }

    sptr<Surface> producerSurface;
    {
        std::lock_guard<std::mutex> lock(lock_);
        producerSurface = producerSurface_;
    }

    FALSE_RETURN_V_MSG(producerSurface != nullptr, Status::ERROR_INVALID_OPERATION, "Output surface is null!");
    auto ret = producerSurface->FlushBuffer(bufferInfo.buffer, -1, flushCfg);
    MEDIA_LOG_D("producerSurface->FlushBuffer id: %{public}u ret: %{public}d",
        bufferInfo.buffer->GetSeqNum(), ret);
    if (ret != GSERROR_OK) {
        return Status::ERROR_INVALID_OPERATION;
    }
    OnProducerBufferReleased();
    return Status::OK;
}

void SideOutputSurfaceProcessor::DealNotRenderBuffer(SurfaceBufferInfo& bufferInfo)
{
    MEDIA_LOG_I("reback id: %{public}u", bufferInfo.buffer->GetSeqNum());
    {
        std::lock_guard<std::mutex> bufferLock(bufferLock_);
        producerBufferQueue_.push(bufferInfo);
    }
    if (state_.load() != ProcessorState::IDLE) {
        cvTrigger_.notify_one();
    }
}

void SideOutputSurfaceProcessor::ProcessBuffers()
{
    if (!isRunning_.load()) {
        MEDIA_LOG_I("Skip when died.");
        return;
    }
    {
        std::lock_guard<std::mutex> lock(lock_);
        if (consumerSurface_ == nullptr || producerSurface_ == nullptr) {
            MEDIA_LOG_W("consumer or producer is null!");
            return;
        }
    }

    while (isRunning_.load()) {
        isProcessing_ = true;

        SurfaceBufferInfo srcBufferInfo;
        SurfaceBufferInfo dstBufferInfo;
        if (!GetConsumerAndProducerBuffer(srcBufferInfo, dstBufferInfo)) {
            MEDIA_LOG_D("GetConsumerAndProducerBuffer break");
            isProcessing_ = false;
            cvDone_.notify_all();
            break;
        }
        MEDIA_LOG_I("ProcessBuffer start");
        ProcessBuffer(srcBufferInfo, dstBufferInfo);
    }

    isProcessing_ = false;
    cvDone_.notify_all();
}

bool SideOutputSurfaceProcessor::WaitTrigger()
{
    FALSE_RETURN_V_MSG(isRunning_.load(), false, "WaitTrigger died.");

    uint32_t consumerSize = 0;
    uint32_t producerSize = 0;
    std::unique_lock<std::mutex> waitLock(waitLock_);
    if (!cvTrigger_.wait_for(waitLock, std::chrono::seconds(WAIT_TRIGGER_TIMEOUT),
        [this, &consumerSize, &producerSize] {
        FALSE_RETURN_V_MSG(isRunning_.load(), true, "Skip WaitTrigger when died.");
        FALSE_RETURN_V_MSG(!isPaused_.load(), false, "Paused, waiting for resume.");
        std::lock(consumerBufferLock_, bufferLock_);
        std::lock_guard<std::mutex> consumerBufferGuard(consumerBufferLock_, std::adopt_lock);
        std::lock_guard<std::mutex> bufferGuard(bufferLock_, std::adopt_lock);
        consumerSize = consumerBufferQueue_.size();
        producerSize = producerBufferQueue_.size();
        MEDIA_LOG_D("run: %{public}d stopping: %{public}d consumerBQ: %{public}u producerBQ: %{public}u "
            "renderBQ: %{public}zu pendingBuf: %{public}d",
            isRunning_.load(), state_.load() == ProcessorState::STOPPING, consumerSize, producerSize,
            renderBufferQueue_.size(), pendingSideSrcBuffer_ != nullptr);
        return !isRunning_.load() || state_.load() == ProcessorState::STOPPING || consumerSize > 0;
        })) {
        MEDIA_LOG_I("Video processing timeout.");
        return false;
    }
    if (isPaused_.load()) {
        MEDIA_LOG_I("Paused after wakeup, waiting for resume.");
        cvTrigger_.wait(waitLock, [this] {
            return !isPaused_.load() || !isRunning_.load();
        });
        if (!isRunning_.load()) {
            MEDIA_LOG_I("Die during pause.");
            return false;
        }
        MEDIA_LOG_I("Resumed from pause.");
        return false;
    }

    if (consumerSize == 0) {
        MEDIA_LOG_I("Empty! consumerBQ: %{public}u", consumerSize);
        return false;
    }
    if (state_.load() == ProcessorState::STOPPING) {
        MEDIA_LOG_I("Stop, skip processing");
        return false;
    }
    return true;
}

void SideOutputSurfaceProcessor::ClearBufferQueues()
{
    std::queue<SurfaceBufferInfo> tempQueue;
    sptr<Surface> consumerSurface;

    {
        std::lock(lock_, consumerBufferLock_, bufferLock_);
        std::lock_guard<std::mutex> lockGuard(lock_, std::adopt_lock);
        std::lock_guard<std::mutex> consumerBufferGuard(consumerBufferLock_, std::adopt_lock);
        std::lock_guard<std::mutex> bufferGuard(bufferLock_, std::adopt_lock);

        isBufferQueueReady_ = false;
        consumerBufferQueue_.swap(tempQueue);
        while (!producerBufferQueue_.empty()) {
            producerBufferQueue_.pop();
        }
        renderBufferQueue_.clear();
        pendingSideSrcBuffer_ = nullptr;

        consumerSurface = consumerSurface_;
    }

    while (!tempQueue.empty()) {
        auto bufferInfo = tempQueue.front();
        if (consumerSurface != nullptr) {
            auto err = consumerSurface->ReleaseBuffer(bufferInfo.buffer, -1);
            if (err != GSERROR_OK) {
                MEDIA_LOG_W("Failed to ReleaseBuffer id: %{public}u ret: %{public}d",
                    bufferInfo.buffer->GetSeqNum(), err);
            }
        }
        tempQueue.pop();
    }
}
 
void SideOutputSurfaceProcessor::GetVideoSample(int32_t &result)
{
    MEDIA_LOG_I("SideOutputSurfaceProcessor GetVideoSample");

    result = 1;

    {
        std::lock_guard<std::mutex> lock(sampleMutex_);
        if (sampleProcessing_.load()) {
            MEDIA_LOG_W("GetVideoSample: previous sample still processing, skip");
            return;
        }

        sampleResult_ = 1;
        sampleDone_ = false;
        sampleProcessing_ = true;
    }

    if (sampleTask_ != nullptr) {
        sampleTask_->SubmitJobOnce([this]() { HandleSampleTask(); });
    } else {
        MEDIA_LOG_W("GetVideoSample: sampleTask_ is null, execute directly");
        HandleSampleTask();
    }

    {
        std::unique_lock<std::mutex> lock(sampleMutex_);
        if (!sampleDoneCond_.wait_for(lock, std::chrono::milliseconds(GET_VIDEO_SAMPLE_TIMEOUT),
            [this]() { return sampleDone_.load(); })) {
            MEDIA_LOG_E("GetVideoSample: timeout waiting for HandleSampleTask");
        }
        sampleProcessing_ = false;
        result = sampleResult_;
    }

    MEDIA_LOG_I("SideOutputSurfaceProcessor GetVideoSample done, result:%{public}d", result);
}

void SideOutputSurfaceProcessor::HandleSampleTask()
{
    MEDIA_LOG_I("SideOutputSurfaceProcessor HandleSampleTask start");

    sptr<SurfaceBuffer> srcBuffer;
    int64_t srcTimestamp = 0;

    {
        std::lock_guard<std::mutex> lock(bufferLock_);
        if (pendingSideSrcBuffer_ != nullptr) {
            srcBuffer = pendingSideSrcBuffer_;
            srcTimestamp = pendingSideTimestamp_;
            MEDIA_LOG_I("HandleSampleTask: got pending buffer ref");
        } else {
            MEDIA_LOG_W("HandleSampleTask: pendingSideSrcBuffer_ is empty");
        }
    }

    if (srcBuffer == nullptr) {
        DealNoImage();
        return;
    }

    int32_t copyResult = CopyBufferToSideSurface(srcBuffer, srcTimestamp);

    {
        std::lock_guard<std::mutex> lock(sampleMutex_);
        sampleResult_ = copyResult;
        sampleDone_ = true;
        sampleDoneCond_.notify_all();
    }

    MEDIA_LOG_I("SideOutputSurfaceProcessor HandleSampleTask end, result:%{public}d", copyResult);
}

bool SideOutputSurfaceProcessor::CopyBufferToSideSurface(sptr<SurfaceBuffer> srcBuffer, int64_t timestamp)
{
    if (srcBuffer == nullptr) {
        MEDIA_LOG_E("CopyBufferToSideSurface: srcBuffer is nullptr");
        return false;
    }

    MEDIA_LOG_I("[SIDE_OUTPUT] srcBuffer(pending source): width=%{public}d, height=%{public}d, format=%{public}d",
        srcBuffer->GetWidth(), srcBuffer->GetHeight(), srcBuffer->GetFormat());

    sptr<Surface> sideSurface;
    {
        std::lock_guard<std::mutex> lock(lock_);
        sideSurface = sideSurface_;
    }

    if (sideSurface == nullptr) {
        MEDIA_LOG_E("CopyBufferToSideSurface: sideSurface is nullptr");
        return false;
    }

    {
        std::lock_guard<std::mutex> consumerLock(consumerBufferLock_);
        if (requestCfg_.width == 0 || requestCfg_.height == 0) {
            requestCfg_.width = srcBuffer->GetWidth();
            requestCfg_.height = srcBuffer->GetHeight();
            requestCfg_.format = srcBuffer->GetFormat();
            requestCfg_.usage = srcBuffer->GetUsage();
            requestCfg_.timeout = 0;
            requestCfg_.strideAlignment = STRIDE_ALIGNMENT;
            MEDIA_LOG_I("CopyBufferToSideSurface: update requestCfg_ from srcBuffer, "
                "width=%{public}u, height=%{public}u", requestCfg_.width, requestCfg_.height);
        }
    }

    SurfaceBufferInfo sideBufferInfo;
    if (!RequestSideBuffer(sideBufferInfo, sideSurface)) {
        return false;
    }

    MEDIA_LOG_I("[SIDE_OUTPUT] sideBuffer(side output): width=%{public}d, height=%{public}d, format=%{public}d",
        sideBufferInfo.buffer->GetWidth(), sideBufferInfo.buffer->GetHeight(), sideBufferInfo.buffer->GetFormat());

    if (!CopyBufferData(srcBuffer, sideBufferInfo, sideSurface)) {
        return false;
    }

    return FlushSideBuffer(sideBufferInfo, timestamp, sideSurface);
}

bool SideOutputSurfaceProcessor::RequestSideBuffer(SurfaceBufferInfo& sideBufferInfo, sptr<Surface> sideSurface)
{
    BufferRequestConfig cfg = requestCfg_;
    if (cfg.width == 0 || cfg.height == 0) {
        cfg.usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA;
        cfg.timeout = 0;
        cfg.strideAlignment = STRIDE_ALIGNMENT;
    }

    GSError err = sideSurface->RequestBuffer(sideBufferInfo.buffer, sideBufferInfo.fence, cfg);
    if (err != GSERROR_OK || sideBufferInfo.buffer == nullptr) {
        MEDIA_LOG_E("RequestSideBuffer failed ret=%{public}d", err);
        if (sideBufferInfo.buffer != nullptr) {
            sideSurface->CancelBuffer(sideBufferInfo.buffer);
        }
        return false;
    }
    return true;
}

bool SideOutputSurfaceProcessor::CopyBufferData(sptr<SurfaceBuffer> srcBuffer,
    SurfaceBufferInfo& sideBufferInfo, sptr<Surface> sideSurface)
{
    srcBuffer->InvalidateCache();

    void* srcAddr = srcBuffer->GetVirAddr();
    void* sideAddr = sideBufferInfo.buffer->GetVirAddr();

    if (srcAddr == nullptr || sideAddr == nullptr) {
        MEDIA_LOG_E("CopyBufferData: virAddr is null");
        sideSurface->CancelBuffer(sideBufferInfo.buffer);
        return false;
    }

    uint32_t srcSize = srcBuffer->GetSize();
    uint32_t sideSize = sideBufferInfo.buffer->GetSize();
    uint32_t copySize = (srcSize < sideSize) ? srcSize : sideSize;

    if (copySize > 0) {
        errno_t ret = memcpy_s(sideAddr, copySize, srcAddr, copySize);
        if (ret != EOK) {
            MEDIA_LOG_E("CopyBufferData: memcpy_s failed ret=%{public}d", ret);
            sideSurface->CancelBuffer(sideBufferInfo.buffer);
            return false;
        }
    }
    return true;
}

bool SideOutputSurfaceProcessor::FlushSideBuffer(SurfaceBufferInfo& sideBufferInfo,
    int64_t timestamp, sptr<Surface> sideSurface)
{
    BufferFlushConfig flushConfig = {
        .damage = {0, 0, sideBufferInfo.buffer->GetWidth(), sideBufferInfo.buffer->GetHeight()},
        .timestamp = timestamp
    };

    GSError flushRet = sideSurface->FlushBuffer(sideBufferInfo.buffer, -1, flushConfig);
    if (flushRet == GSERROR_OK) {
        MEDIA_LOG_I("FlushSideBuffer success");
        return true;
    } else {
        MEDIA_LOG_E("FlushSideBuffer failed ret=%{public}d", flushRet);
        return false;
    }
}

void SideOutputSurfaceProcessor::DealNoImage()
{
    std::lock_guard<std::mutex> lock(sampleMutex_);
    sampleResult_ = 1;
    sampleDone_ = true;
    sampleDoneCond_.notify_all();
    MEDIA_LOG_W("HandleSampleTask: no data available");
}
} // namespace Media
} // namespace OHOS