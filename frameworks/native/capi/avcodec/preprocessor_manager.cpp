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
constexpr int32_t DEFAULT_BUFFER_COUNT = 8;
// constexpr int64_t REQUEST_BUFFER_TIMEOUT = 5000000; // 5ms
}

namespace OHOS {
namespace MediaAVCodec {

PreprocessorManager::PreprocessorManager()
{
    AVCODEC_LOGI("PreprocessorManager created");
}

PreprocessorManager::~PreprocessorManager()
{
    Stop();
    AVCODEC_LOGI("PreprocessorManager destroyed");
}

void PreprocessorManager::RegisterEncoder(std::string_view encoderId, std::shared_ptr<Preprocessor> preprocessor,
                                          sptr<Surface> encoderSurface)
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    EncoderInfo info;
    info.preprocessor = preprocessor;
    info.encoderSurface = encoderSurface;
    info.isRunning = false;
    encoders_.emplace(encoderId, info);
    
    AVCODEC_LOGI("Encoder registered: %{public}s", std::string(encoderId).c_str());
}

void PreprocessorManager::UnregisterEncoder(std::string_view encoderId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    runningEncoders_.erase(encoderId);

    auto it = encoderThreads_.find(encoderId);
    if (it != encoderThreads_.end()) {
        it->second.join();
        encoderThreads_.erase(it);
    }
    
    AVCODEC_LOGI("Encoder unregistered: %{public}s", std::string(encoderId).c_str());
}

void PreprocessorManager::SetEncoderRunning(std::string_view encoderId, bool isRunning)
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (encoders_.find(encoderId) == encoders_.end()) {
        AVCODEC_LOGE("Encoder not found: %{public}s", std::string(encoderId).c_str());
        return;
    }
    
    if (isRunning) {
        if (!encoderThreads_[encoderId].joinable()) {
            encoderThreads_[encoderId] = std::thread(&PreprocessorManager::EncoderThreadLoop, this, encoderId);
        }
        encoders_[encoderId].isRunning = true;
        runningEncoders_.insert(encoderId);
        AVCODEC_LOGI("Encoder running: %{public}s", std::string(encoderId).c_str());
    } else {
        encoders_[encoderId].isRunning = false;
        runningEncoders_.erase(encoderId);    
        MarkAsSkipped(encoderId);
        AVCODEC_LOGI("Encoder stopped: %{public}s", std::string(encoderId).c_str());
    }
    
    cv_.notify_all();
}

void PreprocessorManager::OnEncoderFrameDone(std::string_view encoderId, sptr<SurfaceBuffer> buffer)
{
    uint32_t bufferId = buffer->GetSeqNum();
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = bufferPending_.find(bufferId);
    if (it == bufferPending_.end()) {
        return;
    }
    
    it->second.processedEncoders.insert(encoderId);
    it->second.pendingEncoders.erase(encoderId);
    
    CheckAndRelease(bufferId);
}

int32_t PreprocessorManager::Start()
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (isRunning_) {
        AVCODEC_LOGW("PreprocessorManager already running");
        return AVCS_ERR_OK;
    }
    
    AVCODEC_LOGI("PreprocessorManager started");
    return AVCS_ERR_OK;
}

void PreprocessorManager::CreateSharedSurface()
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (isRunning_) {
        AVCODEC_LOGW("PreprocessorManager already running");
        return;
    }
    
    auto sharedSurfMgr = std::make_shared<SharedSurfaceManager>();
    int32_t ret = sharedSurfMgr->Create(0, 0, DEFAULT_BUFFER_COUNT);
    if (ret != AVCS_ERR_OK) {
        AVCODEC_LOGE("Failed to create shared surface, ret=%{public}d", ret);
        return;
    }
    
    sharedConsumerSurface_ = sharedSurfMgr->GetConsumerSurface();
    sharedProducerSurface_ = sharedSurfMgr->GetProducerSurface();
    
    isRunning_ = true;
    mainThread_ = std::thread(&PreprocessorManager::MainThreadLoop, this);
    
    AVCODEC_LOGI("PreprocessorManager SharedSurface created");
}

void PreprocessorManager::Stop()
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!isRunning_) {
            return;
        }
        isRunning_ = false;
        cv_.notify_all();
    }
    
    if (mainThread_.joinable()) {
        mainThread_.join();
    }
    
    for (auto& [encoderId, info] : encoders_) {
        if (encoderThreads_[encoderId].joinable()) {
            encoderThreads_[encoderId].join();
        }
    }
    
    bufferPending_.clear();
    
    AVCODEC_LOGI("PreprocessorManager stopped");
}

sptr<Surface> PreprocessorManager::GetSharedSurface()
{
    return sharedProducerSurface_;
}

void PreprocessorManager::MainThreadLoop()
{
    AVCODEC_LOGI("MainThreadLoop started");
    
    while (isRunning_) {
        sptr<SurfaceBuffer> buffer;
        int32_t fence;
        int64_t timestamp;
        Rect damage;
        
        GSError ret = sharedConsumerSurface_->AcquireBuffer(buffer, fence, timestamp, damage);
        if (ret != GSERROR_OK) {
            if (isRunning_) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            continue;
        }
        
        uint32_t bufferId = buffer->GetSeqNum();
        
        {
            std::unique_lock<std::mutex> lock(mutex_);
            
            BufferPending pending;
            pending.buffer = buffer;
            pending.pendingEncoders = runningEncoders_;
            bufferPending_[bufferId] = pending;
            
            NotifyEncoders();
            
            cv_.wait(lock, [this, bufferId]() {
                auto it = bufferPending_.find(bufferId);
                if (it == bufferPending_.end()) {
                    return true;
                }
                return it->second.pendingEncoders.empty();
            });
            
            releasedBuffers_.insert(bufferId);
        }
        
        sharedConsumerSurface_->ReleaseBuffer(buffer, -1);
        
        {
            std::lock_guard<std::mutex> lock(mutex_);
            bufferPending_.erase(bufferId);
            releasedBuffers_.erase(bufferId);
        }
    }
    
    AVCODEC_LOGI("MainThreadLoop exited");
}

void PreprocessorManager::EncoderThreadLoop(std::string_view encoderId)
{
    AVCODEC_LOGI("EncoderThreadLoop started: %{public}s", std::string(encoderId).c_str());
    
    while (isRunning_) {
        sptr<SurfaceBuffer> bufferToProcess;
        
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this, encoderId]() {
                if (!isRunning_) {
                    return true;
                }
                auto encIt = encoders_.find(encoderId);
                if (encIt == encoders_.end()) {
                    return false;
                }
                return encIt->second.isRunning;
            });
            
            if (!isRunning_) {
                break;
            }
            
            auto encIt = encoders_.find(encoderId);
            if (encIt == encoders_.end() || !encIt->second.isRunning) {
                continue;
            }
            
            for (auto& [bufferId, pending] : bufferPending_) {
                if (pending.pendingEncoders.count(encoderId) > 0) {
                    bufferToProcess = pending.buffer;
                    break;
                }
            }
            
            if (bufferToProcess == nullptr) {
                continue;
            }
        }
        
        auto encIt = encoders_.find(encoderId);
        if (encIt == encoders_.end()) {
            continue;
        }
        
        sptr<SurfaceBuffer> prodBuffer;
        // TODO: implement RequestBuffer
        int32_t fence;
        BufferRequestConfig requestConfig;
        requestConfig.width = bufferToProcess->GetWidth();
        requestConfig.height = bufferToProcess->GetHeight();
        requestConfig.format = GRAPHIC_PIXEL_FMT_YCBCR_420_SP;
        requestConfig.usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE |
                              BUFFER_USAGE_MEM_DMA | BUFFER_USAGE_MEM_MMZ_CACHE;
        requestConfig.strideAlignment = 8;
        requestConfig.timeout = 0;
        GSError ret = encIt->second.encoderSurface->RequestBuffer(prodBuffer, fence, requestConfig);
        if (ret != GSERROR_OK || prodBuffer == nullptr) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }
        
        encIt->second.preprocessor->Process(bufferToProcess, prodBuffer);
        // TODO::flush buffer config
        BufferFlushConfig flushConfig;
        flushConfig.damage = {0, 0, prodBuffer->GetWidth(), prodBuffer->GetHeight()};
        flushConfig.timestamp = 0;
        flushConfig.desiredPresentTimestamp = 0;
        encIt->second.encoderSurface->FlushBuffer(prodBuffer, -1, flushConfig);
        
        OnEncoderFrameDone(encoderId, bufferToProcess);
    }
    
    AVCODEC_LOGI("EncoderThreadLoop exited: %{public}s", std::string(encoderId).c_str());
}

void PreprocessorManager::NotifyEncoders()
{
    cv_.notify_all();
}

void PreprocessorManager::MarkAsSkipped(std::string_view encoderId)
{
    for (auto& [bufferId, pending] : bufferPending_) {
        if (pending.pendingEncoders.count(encoderId) > 0) {
            pending.pendingEncoders.erase(encoderId);
            pending.processedEncoders.insert(encoderId);
            CheckAndRelease(bufferId);
        }
    }
}

bool PreprocessorManager::ShouldRelease(uint32_t bufferId)
{
    auto it = bufferPending_.find(bufferId);
    if (it == bufferPending_.end()) {
        return false;
    }
    
    return it->second.pendingEncoders.empty();
}

void PreprocessorManager::CheckAndRelease(uint32_t bufferId)
{
    if (ShouldRelease(bufferId)) {
        cv_.notify_all();
    }
}

} // namespace MediaAVCodec
} // namespace OHOS