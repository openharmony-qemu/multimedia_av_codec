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

#include <shared_mutex>
#include "media_codec.h"
#include "avcodec_log.h"
#include <iostream>
#include "osal/task/autolock.h"
#include "plugin/plugin_manager.h"

#define INPUT_BUFFER_QUEUE_NAME "MediaCodecInputBufferQueue"
#define DEFAULT_BUFFER_NUM 8
#define TIME_OUT_MS 500

namespace OHOS {
namespace Media {
    class InputBufferAvailableListener : public IConsumerListener {
    public:
        InputBufferAvailableListener(MediaCodec *mediaCodec) {
            mediaCodec_ = mediaCodec;
        }

        void OnBufferAvailable() override {
            mediaCodec_->ProcessInputBuffer();
        }

    private:
        MediaCodec *mediaCodec_;
    };

    int32_t MediaCodec::Init(const std::string &mime, bool isEncoder) {
        AutoLock lock(stateMutex_);
        FALSE_RETURN_V(state_ == CodecState::UNINITIALIZED, (int32_t)Status::ERROR_INVALID_STATE);
        Plugins::PluginType type = Plugins::PluginType::INVALID_TYPE;
        if (isEncoder) {
          type = Plugins::PluginType::AUDIO_ENCODER;
        } else {
          type = Plugins::PluginType::AUDIO_DECODER;
        }
        codecPlugin_ = CreatePlugin(mime, type);
        codecPlugin_->Init();
        state_ = CodecState::INITIALIZED;
        return (int32_t)Status::OK;
    }

    int32_t MediaCodec::Init(const std::string &name) {
        AutoLock lock(stateMutex_);
        MEDIA_LOG_I("MediaCodec::Init");
        FALSE_RETURN_V(state_ == CodecState::UNINITIALIZED, (int32_t)Status::ERROR_INVALID_STATE);
        Plugins::PluginType type = Plugins::PluginType::INVALID_TYPE;
        if (name.find("Encoder") != name.npos) {
          type = Plugins::PluginType::AUDIO_ENCODER;
        } else if (name.find("Decoder")) {
          type = Plugins::PluginType::AUDIO_DECODER;
        }
        FALSE_RETURN_V(type != Plugins::PluginType::INVALID_TYPE,
                       (int32_t)Status::ERROR_INVALID_PARAMETER);
        auto plugin =
            Plugins::PluginManager::Instance().CreatePlugin(name, type);
        FALSE_RETURN_V(plugin != nullptr, (int32_t)Status::ERROR_INVALID_PARAMETER);
        codecPlugin_ =
            std::reinterpret_pointer_cast<Plugins::CodecPlugin>(plugin);
        Status ret = codecPlugin_->Init();
        FALSE_RETURN_V_MSG_E(ret == Status::OK, (int32_t)Status::ERROR_INVALID_PARAMETER, "info is nullptr");
        state_ = CodecState::INITIALIZED;
        return (int32_t)Status::OK;
    }

    std::shared_ptr<Plugins::CodecPlugin>
    MediaCodec::CreatePlugin(const std::string &mime,
                             Plugins::PluginType pluginType) {
      auto names = Plugins::PluginManager::Instance().ListPlugins(pluginType);
      std::string pluginName = "";
      for (auto &name : names) {
        auto info =
            Plugins::PluginManager::Instance().GetPluginInfo(pluginType, name);
        FALSE_RETURN_V_MSG_E(info != nullptr, nullptr, "info is nullptr");
        auto capSet = info->inCaps;
        FALSE_RETURN_V_MSG_E(capSet.size() == 1, nullptr,
                             "capSet size is not 1");
        if (mime.compare(capSet[0].mime) == 0) {
          pluginName = name;
          break;
        }
      }
      MEDIA_LOG_I("pluginName %{public}s", pluginName.c_str());
      if (!pluginName.empty()) {
        auto plugin = Plugins::PluginManager::Instance().CreatePlugin(
            pluginName, pluginType);
        return std::reinterpret_pointer_cast<Plugins::CodecPlugin>(plugin);
      } else {
        MEDIA_LOG_E("No plugins matching output format");
      }
      return nullptr;
    }

    int32_t MediaCodec::Configure(const std::shared_ptr<Meta> &meta) {
        AutoLock lock(stateMutex_);
        FALSE_RETURN_V(state_ == CodecState::INITIALIZED, (int32_t)Status::ERROR_INVALID_STATE);
        codecPlugin_->SetParameter(meta);
        codecPlugin_->SetDataCallback(this);
        state_ = CodecState::CONFIGURED;
        return (int32_t)Status::OK;
    }

    int32_t MediaCodec::SetOutputBufferQueue(const sptr<AVBufferQueueProducer> &bufferQueueProducer) {
        AutoLock lock(stateMutex_);
        FALSE_RETURN_V(state_ == CodecState::INITIALIZED || state_ == CodecState::CONFIGURED, (int32_t)Status::ERROR_INVALID_STATE);
        outputBufferQueueProducer_ = bufferQueueProducer;
        isBufferMode_ = true;
        return (int32_t)Status::OK;
    }

    int32_t MediaCodec::SetCodecCallback(const std::shared_ptr<CodecCallback> &codecCallback) {
        AutoLock lock(stateMutex_);
        FALSE_RETURN_V(state_ == CodecState::INITIALIZED || state_ == CodecState::CONFIGURED, (int32_t)Status::ERROR_INVALID_STATE);
        codecCallback_ = codecCallback;
        codecPlugin_->SetDataCallback(this);
        return (int32_t)Status::OK;
    }

    int32_t MediaCodec::SetOutputSurface(sptr<Surface> surface) {
        AutoLock lock(stateMutex_);
        FALSE_RETURN_V(state_ == CodecState::INITIALIZED || state_ == CodecState::CONFIGURED, (int32_t)Status::ERROR_INVALID_STATE);
        isSurfaceMode_ = true;
        return (int32_t)Status::OK;
    }

    int32_t MediaCodec::Prepare() {
        AutoLock lock(stateMutex_);
        FALSE_RETURN_V(state_ != CodecState::PREPARED, (int32_t)Status::OK);
        FALSE_RETURN_V(state_ == CodecState::CONFIGURED, (int32_t)Status::ERROR_INVALID_STATE);
        if (isBufferMode_ && isSurfaceMode_) {
            MEDIA_LOG_E("state error");
            return (int32_t)Status::ERROR_UNKNOWN;
        }
        int32_t ret;
        ret = (int32_t)PrepareInputBufferQueue();
        if (ret != (int32_t)Status::OK) {
            MEDIA_LOG_E("PrepareInputBufferQueue failed");
            return (int32_t)ret;
        }
        ret = (int32_t)PrepareOutputBufferQueue();
        if (ret != (int32_t)Status::OK) {
            MEDIA_LOG_E("PrepareOutputBufferQueue failed");
            return (int32_t)ret;
        }
        state_ = CodecState::PREPARED;
        MEDIA_LOG_I("Prepare, ret = %{public}d", (int32_t)ret);
        return (int32_t)Status::OK;
    }

    sptr<AVBufferQueueProducer> MediaCodec::GetInputBufferQueue() {
        AutoLock lock(stateMutex_);
        FALSE_RETURN_V(state_ == CodecState::PREPARED, sptr<AVBufferQueueProducer>());
        if (isSurfaceMode_) {
            return nullptr;
        }
        isBufferMode_ = true;
        return inputBufferQueueProducer_;
    }

    sptr<Surface> MediaCodec::GetInputSurface() {
        AutoLock lock(stateMutex_);
        FALSE_RETURN_V(state_ == CodecState::PREPARED, nullptr);
        if (isBufferMode_) {
            return nullptr;
        }
        isSurfaceMode_ = true;
        return nullptr;
    }

    int32_t MediaCodec::Start() {
        AutoLock lock(stateMutex_);
         MEDIA_LOG_I("Start enter");
        FALSE_RETURN_V(state_ != CodecState::RUNNING, (int32_t)Status::OK);
        FALSE_RETURN_V(state_ == CodecState::PREPARED || state_ == CodecState::FLUSHED, (int32_t)Status::ERROR_INVALID_STATE);
        Status ret;
        ret = codecPlugin_->Start();
        if (ret != Status::OK) {
            MEDIA_LOG_E("Start, ret = %{public}d", (int32_t)ret);
            return (int32_t)ret;
        }
        state_ = CodecState::RUNNING;
        return (int32_t)ret;
    }

    int32_t MediaCodec::Stop() {
        AutoLock lock(stateMutex_);
        FALSE_RETURN_V(state_ != CodecState::PREPARED, (int32_t)Status::OK);
        FALSE_RETURN_V(state_ == CodecState::RUNNING || state_ == CodecState::END_OF_STREAM || state_ == CodecState::FLUSHED, (int32_t)Status::ERROR_INVALID_STATE);
        Status ret;
        ret = codecPlugin_->Stop();
        if (ret != Status::OK) {
            return (int32_t)ret;
        }
        state_ = CodecState::PREPARED;
        return (int32_t)ret;
    }

    int32_t MediaCodec::Flush() {
        AutoLock lock(stateMutex_);
        FALSE_RETURN_V(state_ != CodecState::FLUSHED, (int32_t)Status::OK);
        FALSE_RETURN_V(state_ == CodecState::RUNNING || state_ == CodecState::END_OF_STREAM, (int32_t)Status::ERROR_INVALID_STATE);
        Status ret;
        ret = codecPlugin_->Flush();
        if (ret != Status::OK) {
            return (int32_t)ret;
        }
        state_ = CodecState::FLUSHED;
        return (int32_t)ret;
    }

    int32_t MediaCodec::Reset() {
        AutoLock lock(stateMutex_);
        Status ret;
        FALSE_RETURN_V(state_ == CodecState::UNINITIALIZED || state_ == CodecState::INITIALIZED, (int32_t)Status::OK);
        ret = codecPlugin_->Reset();
        if (ret != Status::OK) {
            return (int32_t)ret;
        }
        state_ = CodecState::INITIALIZED;
        return (int32_t)ret;
    }

    int32_t MediaCodec::Release() {
        AutoLock lock(stateMutex_);
        Status ret;
        FALSE_RETURN_V(state_ == CodecState::UNINITIALIZED, (int32_t)Status::OK);
        ret = codecPlugin_->Release();
        if (ret != Status::OK) {
            return (int32_t)ret;
        }
        state_ = CodecState::UNINITIALIZED;
        return (int32_t)ret;
    }

    int32_t MediaCodec::NotifyEos() {
        AutoLock lock(stateMutex_);
        FALSE_RETURN_V(state_ != CodecState::END_OF_STREAM, (int32_t)Status::OK);
        FALSE_RETURN_V(state_ == CodecState::RUNNING, (int32_t)Status::ERROR_INVALID_STATE);
        state_ = CodecState::END_OF_STREAM;
        return (int32_t)Status::OK;
    }

    int32_t MediaCodec::SetParameter(const std::shared_ptr<Meta> &parameter) {
        AutoLock lock(stateMutex_);
        FALSE_RETURN_V(state_ == CodecState::INITIALIZED || state_ == CodecState::PREPARED, (int32_t)Status::ERROR_INVALID_STATE);
        codecPlugin_->SetParameter(parameter);
        return (int32_t)Status::OK;
    }

    int32_t MediaCodec::GetOutputFormat(std::shared_ptr<Meta> &parameter) {
        AutoLock lock(stateMutex_);
        FALSE_RETURN_V(state_ == CodecState::PREPARED || state_ == CodecState::CONFIGURED || state_ == CodecState::RUNNING, (int32_t)Status::ERROR_INVALID_STATE);
        FALSE_RETURN_V(codecPlugin_ != nullptr, (int32_t)Status::ERROR_INVALID_STATE);
        codecPlugin_->GetParameter(parameter);
        return (int32_t)Status::OK;
    }

    int32_t MediaCodec::PrepareInputBufferQueue() {
        Status ret;
        std::vector<std::shared_ptr<AVBuffer>> inputBuffers;
        ret = codecPlugin_->GetInputBuffers(inputBuffers);
        FALSE_RETURN_V_MSG_E(codecPlugin_ != nullptr, (int32_t)Status::ERROR_UNKNOWN, "codecPlugin_ is nullptr");
        if (ret != Status::OK) {
            MEDIA_LOG_E("GetInputBuffers failed");
            return (int32_t)ret;
        }
        if (inputBuffers.empty()) {
            int inputBufferNum = DEFAULT_BUFFER_NUM;
            MemoryType memoryType = MemoryType::SHARED_MEMORY;
#ifndef MEDIA_OHOS
            memoryType = MemoryType::VIRTUAL_MEMORY;
#endif
            inputBufferQueue_ = AVBufferQueue::Create(inputBufferNum, memoryType,
                                                      INPUT_BUFFER_QUEUE_NAME);
            FALSE_RETURN_V_MSG_E(inputBufferQueue_ != nullptr, (int32_t)Status::ERROR_UNKNOWN, "inputBufferQueue_ is nullptr");
            inputBufferQueueProducer_ = inputBufferQueue_->GetProducer();
            std::shared_ptr<Meta> inputBufferConfig = std::make_shared<Meta>();
            FALSE_RETURN_V_MSG_E(codecPlugin_ != nullptr, (int32_t)Status::ERROR_UNKNOWN, "codecPlugin_ is nullptr");
            ret = codecPlugin_->GetParameter(inputBufferConfig);
            if (ret != Status::OK) {
                MEDIA_LOG_E("GetParameter failed");
                return (int32_t)ret;
            }
            int32_t capacity = 0;
            FALSE_RETURN_V_MSG_E(inputBufferConfig != nullptr, (int32_t)Status::ERROR_UNKNOWN, "inputBufferConfig is nullptr");
            FALSE_RETURN_V(inputBufferConfig->Get<Tag::AUDIO_MAX_INPUT_SIZE>(capacity),
                           (int32_t)Status::ERROR_INVALID_PARAMETER);
            for (int i = 0; i < inputBufferNum; i++) {
                std::shared_ptr<AVAllocator> avAllocator;
#ifndef MEDIA_OHOS
                MEDIA_LOG_D("CreateVirtualAllocator,i=%{public}d capacity=%{public}d", i, capacity);
                avAllocator = AVAllocatorFactory::CreateVirtualAllocator();
#else
                MEDIA_LOG_D("CreateSharedAllocator,i=%{public}d capacity=%{public}d", i, capacity);
                avAllocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
#endif
                std::shared_ptr<AVBuffer> inputBuffer = AVBuffer::CreateAVBuffer(avAllocator, capacity);
                FALSE_RETURN_V_MSG_E(inputBufferQueueProducer_ != nullptr, (int32_t)Status::ERROR_UNKNOWN, "inputBufferQueueProducer_ is nullptr");
                inputBufferQueueProducer_->AttachBuffer(inputBuffer, false);
            }
        } else {
            inputBufferQueue_ = AVBufferQueue::Create(inputBuffers.size(), MemoryType::HARDWARE_MEMORY,
                                                      INPUT_BUFFER_QUEUE_NAME);
            FALSE_RETURN_V_MSG_E(inputBufferQueue_ != nullptr, (int32_t)Status::ERROR_UNKNOWN, "inputBufferQueue_ is nullptr");
            inputBufferQueueProducer_ = inputBufferQueue_->GetProducer();
            for (uint32_t i = 0; i < inputBuffers.size(); i++) {
                inputBufferQueueProducer_->AttachBuffer(inputBuffers[i], false);
            }
        }
        FALSE_RETURN_V_MSG_E(inputBufferQueue_ != nullptr, (int32_t)Status::ERROR_UNKNOWN, "inputBufferQueue_ is nullptr");
        inputBufferQueueConsumer_ = inputBufferQueue_->GetConsumer();
        sptr<IConsumerListener> listener = new InputBufferAvailableListener(this);
        FALSE_RETURN_V_MSG_E(inputBufferQueueConsumer_ != nullptr, (int32_t)Status::ERROR_UNKNOWN, "inputBufferQueueConsumer_ is nullptr");
        inputBufferQueueConsumer_->SetBufferAvailableListener(listener);
        return (int32_t)ret;
    }

    int32_t MediaCodec::PrepareOutputBufferQueue() {
        Status ret;
        std::vector<std::shared_ptr<AVBuffer>> outputBuffers;
        FALSE_RETURN_V_MSG_E(codecPlugin_ != nullptr, (int32_t)Status::ERROR_INVALID_STATE, "codecPlugin_ is nullptr");
        ret = codecPlugin_->GetOutputBuffers(outputBuffers);
        if (ret != Status::OK) {
            MEDIA_LOG_E("GetOutputBuffers failed");
            return (int32_t)ret;
        }
        if (outputBuffers.empty()) {
            int outputBufferNum = 30;
            std::shared_ptr<Meta> outputBufferConfig = std::make_shared<Meta>();
            ret = codecPlugin_->GetParameter(outputBufferConfig);
            if (ret != Status::OK) {
                MEDIA_LOG_E("GetParameter failed");
                return (int32_t)ret;
            }
            int32_t capacity = 0;
            FALSE_RETURN_V_MSG_E(outputBufferConfig != nullptr, (int32_t)Status::ERROR_INVALID_STATE, "outputBufferConfig is nullptr");
            FALSE_RETURN_V(outputBufferConfig->Get<Tag::AUDIO_MAX_OUTPUT_SIZE>(capacity),
                           (int32_t)Status::ERROR_INVALID_PARAMETER);
            for (int i = 0; i < outputBufferNum; i++) {
                std::shared_ptr<AVAllocator> avAllocator;
#ifndef MEDIA_OHOS
                avAllocator = AVAllocatorFactory::CreateVirtualAllocator();
#else
                avAllocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
#endif
                std::shared_ptr<AVBuffer> outputBuffer = AVBuffer::CreateAVBuffer(avAllocator, capacity);
                FALSE_RETURN_V_MSG_E(outputBufferQueueProducer_ != nullptr, (int32_t)Status::ERROR_INVALID_STATE, "outputBufferQueueProducer_ is nullptr");
                outputBufferQueueProducer_->AttachBuffer(outputBuffer, false);

            }
        } else {
            FALSE_RETURN_V_MSG_E(outputBufferQueueProducer_ != nullptr, (int32_t)Status::ERROR_INVALID_STATE, "outputBufferQueueProducer_ is nullptr");
            for (uint32_t i = 0; i < outputBuffers.size(); i++) {
                outputBufferQueueProducer_->AttachBuffer(outputBuffers[i], false);
            }
        }
        return (int32_t)ret;
    }

    void MediaCodec::ProcessInputBuffer() {
        Status ret = Status::OK;
        std::shared_ptr<AVBuffer> filledInputBuffer;
        uint32_t eosStatus = 0;
        ret = inputBufferQueueConsumer_->AcquireBuffer(filledInputBuffer);
        if (ret != Status::OK) {
            MEDIA_LOG_E("ProcessInputBuffer AcquireBuffer fail");
            return;
        }
        if (state_ != CodecState::RUNNING) {
            MEDIA_LOG_E("ProcessInputBuffer ReleaseBuffer name:MediaCodecInputBufferQueue");
            inputBufferQueueConsumer_->ReleaseBuffer(filledInputBuffer);
            return;
        }
        ret = codecPlugin_->QueueInputBuffer(filledInputBuffer);
        if (ret != Status::OK) {
            MEDIA_LOG_E("ProcessInputBuffer queueInputbuffer fail");
            inputBufferQueueConsumer_->ReleaseBuffer(filledInputBuffer);
            return;
        }
        eosStatus = filledInputBuffer->flag_;
        while (true) {
            std::shared_ptr<AVBuffer> emptyOutputBuffer;
            AVBufferConfig avBufferConfig;

            ret = outputBufferQueueProducer_->RequestBuffer(emptyOutputBuffer, avBufferConfig, TIME_OUT_MS);
            if (ret != Status::OK) {
                continue;
            }
            ret = codecPlugin_->QueueOutputBuffer(emptyOutputBuffer);
            emptyOutputBuffer->flag_ = eosStatus;
            if (ret == Status::ERROR_NOT_ENOUGH_DATA) {
                MEDIA_LOG_E("QueueOutputBuffer ERROR_NOT_ENOUGH_DATA");
                outputBufferQueueProducer_->PushBuffer(emptyOutputBuffer, false);
                return;
            } else if (ret != Status::OK) {
                MEDIA_LOG_E("QueueOutputBuffer error");
                outputBufferQueueProducer_->PushBuffer(emptyOutputBuffer, false);
                return;
            } else {
                return;
            }
        }
    }

    void MediaCodec::OnInputBufferDone(const std::shared_ptr<AVBuffer> &inputBuffer) {
        Status ret = inputBufferQueueConsumer_->ReleaseBuffer(inputBuffer);
        FALSE_RETURN_MSG(ret == Status::OK, "OnInputBufferDone fail");
    }

    void MediaCodec::OnOutputBufferDone(const std::shared_ptr<AVBuffer> &outputBuffer) {
        Status ret = outputBufferQueueProducer_->PushBuffer(outputBuffer, true);
        FALSE_RETURN_MSG(ret == Status::OK, "OnOutputBufferDone fail");
    }

    void
    MediaCodec::OnEvent(const std::shared_ptr<Plugins::PluginEvent> event) {}
} //namespace Media
} //namespace OHOS
