/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#include "preprocessor_manager.h"
#include "shared_surface_manager.h"
#include "avcodec_log.h"
#include "avcodec_errors.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "PreprocessorManager"};
constexpr int32_t BUFFER_REQUEST_STRIDE_ALIGNMENT = 32;

class ConsumerListener : public OHOS::IBufferConsumerListener {
public:
    explicit ConsumerListener(std::weak_ptr<OHOS::MediaAVCodec::PreprocessorManager> mgr) : mgr_(mgr) {}
    void OnBufferAvailable() override
    {
        auto mgr = mgr_.lock();
        if (mgr) {
            mgr->NotifyNewBufferAvailable();
        }
    }
private:
    std::weak_ptr<OHOS::MediaAVCodec::PreprocessorManager> mgr_;
};
}

namespace OHOS {
namespace MediaAVCodec {

PreprocessorManager::PreprocessorManager()
{
    AVCODEC_LOGI("PreprocessorManager created");
}

PreprocessorManager::~PreprocessorManager()
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto &[encoderId, info] : encoders_) {
        if (info) {
            info->Stop();
        }
    }
    AVCODEC_LOGI("PreprocessorManager destroyed");
}

void PreprocessorManager::RegisterEncoder(std::string_view encoderId, std::shared_ptr<Preprocessor> preprocessor,
                                          sptr<Surface> encoderSurface)
{
    auto info = std::make_shared<EncoderInfo>(std::string(encoderId));
    info->Init(preprocessor, encoderSurface);

    std::lock_guard<std::mutex> lock(mutex_);
    encoders_.emplace(std::string(encoderId), info);
    AVCODEC_LOGI("Encoder registered: %{public}s", std::string(encoderId).c_str());
}

void PreprocessorManager::UnregisterEncoder(std::string_view encoderId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    encoders_.erase(std::string(encoderId));
    AVCODEC_LOGI("Encoder unregistered: %{public}s", std::string(encoderId).c_str());
}

void PreprocessorManager::SetEncoderRunning(std::string_view encoderId, bool isRunning)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto iter = encoders_.find(std::string(encoderId));
    if (iter == encoders_.end() || !iter->second) {
        AVCODEC_LOGE("Encoder not found: %{public}s", std::string(encoderId).c_str());
        return;
    }

    if (isRunning) {
        iter->second->Start();
        AVCODEC_LOGI("Encoder running: %{public}s", std::string(encoderId).c_str());
    } else {
        iter->second->Stop();
        AVCODEC_LOGI("Encoder stopped: %{public}s", std::string(encoderId).c_str());
    }
}

void PreprocessorManager::CreateSharedSurface()
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (sharedConsumerSurface_ != nullptr) {
        AVCODEC_LOGW("Shared surface is already created");
        return;
    }

    auto sharedSurfMgr = std::make_shared<SharedSurfaceManager>();
    int32_t ret = sharedSurfMgr->Create();
    if (ret != AVCS_ERR_OK) {
        AVCODEC_LOGE("Failed to create shared surface, ret=%{public}d", ret);
        return;
    }

    sharedConsumerSurface_ = sharedSurfMgr->GetConsumerSurface();
    sharedProducerSurface_ = sharedSurfMgr->GetProducerSurface();

    sptr<IBufferConsumerListener> listener = sptr<ConsumerListener>::MakeSptr(shared_from_this());
    sharedConsumerSurface_->RegisterConsumerListener(listener);

    AVCODEC_LOGI("PreprocessorManager SharedSurface created");
}

sptr<Surface> PreprocessorManager::GetSharedSurface()
{
    return sharedProducerSurface_;
}

void PreprocessorManager::NotifyNewBufferAvailable()
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto bufferWrapper = std::make_shared<SurfaceBufferWrapper>(
        sharedConsumerSurface_, nullptr, nullptr, SurfaceBufferWrapper::SurfaceType::CONSUMER);
    Rect damage;

    auto ret = sharedConsumerSurface_->AcquireBuffer(
        bufferWrapper->buffer, bufferWrapper->fence, bufferWrapper->timestamp, damage);
    if (ret != GSERROR_OK) {
        AVCODEC_LOGE("Failed to acquire buffer from shared surface, ret: %{public}d", ret);
    }
    for (auto &[_, encoderInfo] : encoders_) {
        if (encoderInfo) {
            encoderInfo->OnNewPendingBufferAvailable(bufferWrapper);
        }
    }
}

PreprocessorManager::SurfaceBufferWrapper::~SurfaceBufferWrapper()
{
    auto surfacePtr = surface.promote();
    if (!buffer || !surfacePtr) {
        return;
    }
    if (!isProcessed) {
        surfacePtr->CancelBuffer(buffer);
        AVCODEC_LOG_LIMIT_IN_TIME(AVCODEC_LOGW, 5000, 5,    // 5 times in 5000 milliseconds
            "Buffer not processed before wrapper destruction, bufferId=%{public}u",
            buffer->GetSeqNum());
        return;
    }

    auto ret = surfaceType == SurfaceType::PRODUCER ?
        surfacePtr->FlushBuffer(buffer, -1, flushConfig) :
        surfacePtr->ReleaseBuffer(buffer, fence);
    if (ret != GSERROR_OK) {
        AVCODEC_LOGE("Failed to %{public}s buffer, ret=%{public}d",
            surfaceType == SurfaceType::PRODUCER ? "flush" : "release", ret);
    }
}

PreprocessorManager::EncoderInfo::~EncoderInfo()
{
    Stop();
    if (encoderSurface_) {
        encoderSurface_->UnRegisterReleaseListener();
    }
    AVCODEC_LOGI("EncoderInfo destroyed: %{public}s", encoderId_.data());
}

void PreprocessorManager::EncoderInfo::Init(std::shared_ptr<Preprocessor> preprocessor, sptr<Surface> surface)
{
    if (!preprocessor || !surface) {
        AVCODEC_LOGE("Get a nullptr, %s %s", preprocessor ? "" : "preprocessor", surface ? "" : "surface");
    }
    preprocessor_ = preprocessor;

    std::weak_ptr<EncoderInfo> weakThis = shared_from_this();
    encoderSurface_ = surface;
    encoderSurface_->Connect();
    encoderSurface_->RegisterReleaseListener([weakThis](uint32_t seq, const sptr<SyncFence>& fence) {
        std::shared_ptr<EncoderInfo> encoderInfo = weakThis.lock();
        if (encoderInfo == nullptr) {
            return GSERROR_OK;
        }

        encoderInfo->OnNewEncoderSurfaceBufferAvailable(seq, fence);
        return GSERROR_OK;
    });
}

int32_t PreprocessorManager::EncoderInfo::Start()
{
    std::unique_lock<std::mutex> lock(bufferQueueMutex_);
    if (isRunning_) {
        AVCODEC_LOGW("Encoder already running: %{public}s", encoderId_.data());
        return AVCS_ERR_OK;
    }
    if (!encoderSurface_ || !preprocessor_) {
        AVCODEC_LOGE("EncoderSurface or Preprocessor not initialized for encoder: %{public}s", encoderId_.data());
        return AVCS_ERR_UNKNOWN;
    }
    CheckAndAllocateSurfaceBufferFromEncSurface();
    isRunning_ = true;
    processThread_ = std::thread(&EncoderInfo::EncoderThreadLoop, this);
    if (!processThread_.joinable()) {
        isRunning_ = false;
        AVCODEC_LOGE("Failed to start encoder thread: %{public}s", encoderId_.data());
        return AVCS_ERR_UNKNOWN;
    }
    return AVCS_ERR_OK;
}

void PreprocessorManager::EncoderInfo::Stop()
{
    {
        std::unique_lock<std::mutex> lock(bufferQueueMutex_);
        isRunning_ = false;
    }
    bufferQueueCv_.notify_all();
    if (processThread_.joinable()) {
        processThread_.join();
    }
}

int32_t PreprocessorManager::EncoderInfo::CheckAndAllocateSurfaceBufferFromEncSurface()
{
    uint32_t allocTimes = encoderSurface_->GetQueueSize() - encoderSurfaceBufferPool.size();
    if (allocTimes == 0) {
        return AV_ERR_OK;
    }

    BufferRequestConfig requestConfig = {
        .width = preprocessor_->GetConfiguredWidth(),
        .height = preprocessor_->GetConfiguredHeight(),
        .format = preprocessor_->GetConfiguredPixelFormat(),
    };
    requestConfig.strideAlignment = BUFFER_REQUEST_STRIDE_ALIGNMENT;
    requestConfig.usage = encoderSurface_->GetDefaultUsage() |
        BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_MMZ_CACHE;

    for (uint32_t idx = 0; idx < allocTimes; idx++) {
        auto surfaceBuffer = SurfaceBuffer::Create();
        if (!surfaceBuffer) {
            AVCODEC_LOGE("Create surface buffer failed");
            return AV_ERR_NO_MEMORY;
        }

        auto ret = surfaceBuffer->Alloc(requestConfig);
        if (ret != GSERROR_OK) {
            AVCODEC_LOGE("Alloc surfacebuffer %{public}u failed, GSError: %{public}d, "
                "%{public}dx%{public}d, format: %{public}d, usage: %{public}" PRId64,
                idx, ret, requestConfig.width, requestConfig.height, requestConfig.format, requestConfig.usage);
            return ret == GSERROR_NO_MEM ? AVCS_ERR_NO_MEMORY : AVCS_ERR_UNKNOWN;
        }

        ret = encoderSurface_->AttachBufferToQueue(surfaceBuffer);
        if (ret != GSERROR_OK) {
            AVCODEC_LOGE("Attach surfacebuffer %{public}u to queue failed, GSError: %{public}d", idx, ret);
            return AVCS_ERR_UNKNOWN;
        }

        encoderSurfaceBufferPool.emplace(surfaceBuffer->GetSeqNum(), surfaceBuffer);
        auto bufferWrapper = std::make_shared<SurfaceBufferWrapper>(
            encoderSurface_, surfaceBuffer, nullptr, SurfaceBufferWrapper::SurfaceType::PRODUCER);
        availableInputBuffers_.push(bufferWrapper);
    }
    AVCODEC_LOGI("Alloc %{public}u surface buffers, %{public}dx%{public}d, format: %{public}d",
        allocTimes, requestConfig.width, requestConfig.height, requestConfig.format);
    return AV_ERR_OK;
}

void PreprocessorManager::EncoderInfo::OnNewPendingBufferAvailable(std::shared_ptr<SurfaceBufferWrapper> buffer)
{
    std::lock_guard<std::mutex> lock(bufferQueueMutex_);
    pendingBuffers_.push(buffer);
    bufferQueueCv_.notify_all();
}

void PreprocessorManager::EncoderInfo::OnNewEncoderSurfaceBufferAvailable(uint32_t seq, const sptr<SyncFence> &fence)
{
    std::lock_guard<std::mutex> lock(bufferQueueMutex_);
    auto surfaceBuffer = encoderSurfaceBufferPool.find(seq);
    if (surfaceBuffer == encoderSurfaceBufferPool.end()) {
        AVCODEC_LOGW("Seqnum=%{public}u don't belong to this encoder: %{public}s, ignore", seq, encoderId_.data());
        return;
    }
    auto bufferWrapper = std::make_shared<SurfaceBufferWrapper>(
        encoderSurface_, surfaceBuffer->second, fence, SurfaceBufferWrapper::SurfaceType::PRODUCER);
    availableInputBuffers_.push(bufferWrapper);
    bufferQueueCv_.notify_all();
}

void PreprocessorManager::EncoderInfo::SetBufferFlushConfig(std::shared_ptr<SurfaceBufferWrapper> &inputBuffer,
                                                            std::shared_ptr<SurfaceBufferWrapper> &outputBuffer)
{
    if (!(inputBuffer && inputBuffer->buffer) || !outputBuffer || !preprocessor_) {
        return;
    }
    outputBuffer->flushConfig.damage = {0, 0, preprocessor_->GetOutputWidth(), preprocessor_->GetOutputHeight()};
    outputBuffer->flushConfig.timestamp = inputBuffer->timestamp;
}

void PreprocessorManager::EncoderInfo::EncoderThreadLoop()
{
    std::string threadName = std::string("OS_Preproc_") + encoderId_ + "_Loop";
    pthread_setname_np(pthread_self(), threadName.c_str());
    AVCODEC_LOGI("EncoderThreadLoop started: %{public}s", encoderId_.data());

    while (isRunning_) {
        std::shared_ptr<SurfaceBufferWrapper> inputBuffer;
        std::shared_ptr<SurfaceBufferWrapper> outputBuffer;

        {
            std::unique_lock<std::mutex> lock(bufferQueueMutex_);
            bufferQueueCv_.wait(lock, [this]() {
                if (!isRunning_ || !(pendingBuffers_.empty() || availableInputBuffers_.empty())) {
                    return true;
                }
                return false;
            });
            if (!isRunning_) {
                break;
            }
            if (pendingBuffers_.empty() || availableInputBuffers_.empty()) {
                continue;
            }
            inputBuffer = pendingBuffers_.front();
            pendingBuffers_.pop();
            if (!(inputBuffer && inputBuffer->buffer)) {
                AVCODEC_LOGE("Invalid input buffer in encoder thread: %{public}s", encoderId_.data());
                continue;
            }
            outputBuffer = availableInputBuffers_.front();
            availableInputBuffers_.pop();
            if (!(outputBuffer && outputBuffer->buffer)) {
                AVCODEC_LOGE("Invalid output buffer in encoder thread: %{public}s", encoderId_.data());
                continue;
            }
        }

        auto ret = preprocessor_->Process(inputBuffer->buffer, outputBuffer->buffer, inputBuffer->timestamp);
        if (ret != AV_ERR_OK) {
            AVCODEC_LOGW("Preprocess failed, skip ..");
            continue;
        }
        inputBuffer->isProcessed = outputBuffer->isProcessed = true;
        SetBufferFlushConfig(inputBuffer, outputBuffer);
    }

    AVCODEC_LOGI("EncoderThreadLoop exited: %{public}s", encoderId_.data());
}

// === PreprocessorManager: Buffer data query for GetInputDescription ===

bool PreprocessorManager::PeekPendingBufferData(std::string_view encoderId, BufferDataInfo& outInfo) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto iter = encoders_.find(std::string(encoderId));
    if (iter == encoders_.end() || !iter->second) {
        return false;
    }
    return iter->second->PeekPendingBufferData(outInfo);
}

// === EncoderInfo: Peek buffer data from pendingBuffers_ front directly ===

bool PreprocessorManager::EncoderInfo::PeekPendingBufferData(BufferDataInfo& outInfo) const
{
    std::lock_guard<std::mutex> lock(bufferQueueMutex_);
    if (pendingBuffers_.empty()) {
        return false;
    }
    const auto& wrapper = pendingBuffers_.front();
    if (!wrapper || !wrapper->buffer) {
        return false;
    }
    outInfo.width = wrapper->buffer->GetWidth();
    outInfo.height = wrapper->buffer->GetHeight();
    outInfo.format = wrapper->buffer->GetFormat();
    outInfo.stride = wrapper->buffer->GetStride();
    return true;
}
} // namespace MediaAVCodec
} // namespace OHOS
