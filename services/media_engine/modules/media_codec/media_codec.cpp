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
#include "osal/task/autolock.h"

#define INPUT_BUFFER_QUEUE_NAME "MediaCodecInputBufferQueue"
#define DEFAULT_BUFFER_NUM 8
#define TIME_OUT_MS 8

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

    Status MediaCodec::Init(const std::string &mime, bool isEncoder) {
        AutoLock lock(stateMutex_);
        FALSE_RETURN_V(state_ == CodecState::UNINITIALIZED, Status::ERROR_INVALID_STATE);
        isEncoder_ = isEncoder;
        // codecPlugin_ = std::make_shared<Plugin::Ffmpeg::AudioFfmpegDecoderPlugin>("mp3");
        codecPlugin_->Init();
        state_ = CodecState::INITIALIZED;
        return Status::OK;
    }

    Status MediaCodec::Init(const std::string &name) {
        AutoLock lock(stateMutex_);
        FALSE_RETURN_V(state_ == CodecState::UNINITIALIZED, Status::ERROR_INVALID_STATE);
        // codecPlugin_ = std::make_shared<Plugin::Ffmpeg::AudioFfmpegDecoderPlugin>("mp3");
        state_ = CodecState::INITIALIZED;
        return Status::OK;
    }


    Status MediaCodec::Configure(const std::shared_ptr<Meta> &meta) {
        AutoLock lock(stateMutex_);
        FALSE_RETURN_V(state_ == CodecState::INITIALIZED, Status::ERROR_INVALID_STATE);
        codecPlugin_->SetParameter(meta);
        state_ = CodecState::CONFIGURED;
        return Status::OK;
    }

    Status MediaCodec::SetOutputBufferQueue(const sptr<AVBufferQueueProducer> &bufferQueueProducer) {
        AutoLock lock(stateMutex_);
        FALSE_RETURN_V(state_ == CodecState::INITIALIZED || state_ == CodecState::CONFIGURED, Status::ERROR_INVALID_STATE);
        outputBufferQueueProducer_ = bufferQueueProducer;
        isBufferMode_ = true;
        return Status::OK;
    }

    Status MediaCodec::SetCodecCallback(const std::shared_ptr<CodecCallback> &codecCallback) {
        AutoLock lock(stateMutex_);
        FALSE_RETURN_V(state_ == CodecState::INITIALIZED || state_ == CodecState::CONFIGURED, Status::ERROR_INVALID_STATE);
        codecCallback_ = codecCallback;
        codecPlugin_->SetDataCallback(this);
        return Status::OK;
    }

    Status MediaCodec::SetOutputSurface(sptr<Surface> surface) {
        AutoLock lock(stateMutex_);
        FALSE_RETURN_V(state_ == CodecState::INITIALIZED || state_ == CodecState::CONFIGURED, Status::ERROR_INVALID_STATE);
        isSurfaceMode_ = true;
        return Status::OK;
    }

    Status MediaCodec::Prepare() {
        AutoLock lock(stateMutex_);
        FALSE_RETURN_V(state_ != CodecState::PREPARED, Status::OK);
        FALSE_RETURN_V(state_ == CodecState::CONFIGURED, Status::ERROR_INVALID_STATE);
        if (isBufferMode_ && isSurfaceMode_) {
            return Status::ERROR_UNKNOWN;
        }
        Status ret;
        ret = PrepareInputBufferQueue();
        if (ret != Status::OK) {
            return ret;
        }
        ret = PrepareOutputBufferQueue();
        if (ret != Status::OK) {
            return ret;
        }
        state_ = CodecState::PREPARED;
        return ret;
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

    Status MediaCodec::Start() {
        AutoLock lock(stateMutex_);
        FALSE_RETURN_V(state_ != CodecState::RUNNING, Status::OK);
        FALSE_RETURN_V(state_ == CodecState::PREPARED || state_ == CodecState::FLUSHED, Status::ERROR_INVALID_STATE);
        Status ret;
        ret = codecPlugin_->Start();
        if (ret != Status::OK) {
            return ret;
        }
        state_ = CodecState::RUNNING;
        return ret;
    }

    Status MediaCodec::Stop() {
        AutoLock lock(stateMutex_);
        FALSE_RETURN_V(state_ != CodecState::PREPARED, Status::OK);
        FALSE_RETURN_V(state_ == CodecState::RUNNING || state_ == CodecState::END_OF_STREAM || state_ == CodecState::FLUSHED, Status::ERROR_INVALID_STATE);
        Status ret;
        ret = codecPlugin_->Stop();
        if (ret != Status::OK) {
            return ret;
        }
        state_ = CodecState::PREPARED;
        return ret;
    }

    Status MediaCodec::Flush() {
        AutoLock lock(stateMutex_);
        FALSE_RETURN_V(state_ != CodecState::FLUSHED, Status::OK);
        FALSE_RETURN_V(state_ == CodecState::RUNNING || state_ == CodecState::END_OF_STREAM, Status::ERROR_INVALID_STATE);
        Status ret;
        ret = codecPlugin_->Flush();
        if (ret != Status::OK) {
            return ret;
        }
        state_ = CodecState::FLUSHED;
        return ret;
    }

    Status MediaCodec::Reset() {
        AutoLock lock(stateMutex_);
        Status ret;
        FALSE_RETURN_V(state_ == CodecState::UNINITIALIZED || state_ == CodecState::INITIALIZED, Status::OK);
        ret = codecPlugin_->Reset();
        if (ret != Status::OK) {
            return ret;
        }
        state_ = CodecState::INITIALIZED;
        return ret;
    }

    Status MediaCodec::Release() {
        AutoLock lock(stateMutex_);
        Status ret;
        FALSE_RETURN_V(state_ == CodecState::UNINITIALIZED, Status::OK);
        ret = codecPlugin_->Release();
        if (ret != Status::OK) {
            return ret;
        }
        state_ = CodecState::UNINITIALIZED;
        return ret;
    }

    Status MediaCodec::NotifyEOS() {
        AutoLock lock(stateMutex_);
        FALSE_RETURN_V(state_ != CodecState::END_OF_STREAM, Status::OK);
        FALSE_RETURN_V(state_ == CodecState::RUNNING, Status::ERROR_INVALID_STATE);
        state_ = CodecState::END_OF_STREAM;
        return Status::OK;
    }

    Status MediaCodec::SetParameter(const std::shared_ptr<Meta> &parameter) {
        AutoLock lock(stateMutex_);
        FALSE_RETURN_V(state_ == CodecState::INITIALIZED || state_ == CodecState::PREPARED, Status::ERROR_INVALID_STATE);
        codecPlugin_->SetParameter(parameter);
        return Status::OK;
    }

    std::shared_ptr<Meta> MediaCodec::GetOutputFormat() {
        AutoLock lock(stateMutex_);
        FALSE_RETURN_V(state_ == CodecState::PREPARED, nullptr);
        std::shared_ptr<Meta> outputFormat = std::make_shared<Meta>();
        codecPlugin_->GetParameter(outputFormat);
        return outputFormat;
    }

    Status MediaCodec::PrepareInputBufferQueue() {
        Status ret;
        std::vector<std::shared_ptr<AVBuffer>> inputBuffers;
        ret = codecPlugin_->GetInputBuffers(inputBuffers);
        if (ret != Status::OK) {
            return ret;
        }
        if (inputBuffers.empty()) {
            int inputBufferNum = DEFAULT_BUFFER_NUM;
            MemoryType memoryType = MemoryType::SHARED_MEMORY;
#ifndef MEDIA_OHOS
            memoryType = MemoryType::VIRTUAL_MEMORY;
#endif
            inputBufferQueue_ = AVBufferQueue::Create(inputBufferNum, memoryType,
                                                      INPUT_BUFFER_QUEUE_NAME);
            inputBufferQueueProducer_ = inputBufferQueue_->GetProducer();
            std::shared_ptr<Meta> inputBufferConfig = std::make_shared<Meta>();
            ret = codecPlugin_->GetParameter(inputBufferConfig);
            if (ret != Status::OK) {
                return ret;
            }
            int32_t capacity = 0;
            FALSE_RETURN_V(inputBufferConfig->Get<Tag::AUDIO_MAX_INPUT_SIZE>(capacity),
                           Status::ERROR_INVALID_PARAMETER);
            for (int i = 0; i < inputBufferNum; i++) {
                std::shared_ptr<AVAllocator> avAllocator;
#ifndef MEDIA_OHOS
                avAllocator = AVAllocatorFactory::CreateVirtualAllocator();
#else
                avAllocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
#endif
                std::shared_ptr<AVBuffer> inputBuffer = AVBuffer::CreateAVBuffer(avAllocator, capacity);
                inputBufferQueueProducer_->AttachBuffer(inputBuffer, false);
            }
        } else {
            inputBufferQueue_ = AVBufferQueue::Create(inputBuffers.size(), MemoryType::HARDWARE_MEMORY,
                                                      INPUT_BUFFER_QUEUE_NAME);
            inputBufferQueueProducer_ = inputBufferQueue_->GetProducer();
            for (int i = 0; i < inputBuffers.size(); i++) {
                inputBufferQueueProducer_->AttachBuffer(inputBuffers[i], false);
            }
        }
        inputBufferQueueConsumer_ = inputBufferQueue_->GetConsumer();
        sptr<IConsumerListener> listener = new InputBufferAvailableListener(this);
        inputBufferQueueConsumer_->SetBufferAvailableListener(listener);
        return ret;
    }

    Status MediaCodec::PrepareOutputBufferQueue() {
        Status ret;
        std::vector<std::shared_ptr<AVBuffer>> outputBuffers;
        ret = codecPlugin_->GetOutputBuffers(outputBuffers);
        if (ret != Status::OK) {
            return ret;
        }
        if (outputBuffers.empty()) {
            int outputBufferNum = DEFAULT_BUFFER_NUM;
            std::shared_ptr<Meta> outputBufferConfig = std::make_shared<Meta>();
            ret = codecPlugin_->GetParameter(outputBufferConfig);
            if (ret != Status::OK) {
                return ret;
            }
            int32_t capacity = 0;
            FALSE_RETURN_V(outputBufferConfig->Get<Tag::AUDIO_MAX_OUTPUT_SIZE>(capacity),
                           Status::ERROR_INVALID_PARAMETER);
            for (int i = 0; i < outputBufferNum; i++) {
                std::shared_ptr<AVAllocator> avAllocator;
#ifndef MEDIA_OHOS
                avAllocator = AVAllocatorFactory::CreateVirtualAllocator();
#else
                avAllocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
#endif
                std::shared_ptr<AVBuffer> outputBuffer = AVBuffer::CreateAVBuffer(avAllocator, capacity);
                outputBufferQueueProducer_->AttachBuffer(outputBuffer, false);

            }
        } else {
            for (int i = 0; i < outputBuffers.size(); i++) {
                outputBufferQueueProducer_->AttachBuffer(outputBuffers[i], false);
            }
        }
        return ret;
    }

    void MediaCodec::ProcessInputBuffer() {
        Status ret = Status::OK;
        std::shared_ptr<AVBuffer> filledInputBuffer;
        ret = inputBufferQueueConsumer_->AcquireBuffer(filledInputBuffer);
        if (ret != Status::OK) {
            return;
        }
        if (state_ != CodecState::RUNNING) {
            inputBufferQueueConsumer_->ReleaseBuffer(filledInputBuffer);
            return;
        }
        ret = codecPlugin_->QueueInputBuffer(filledInputBuffer);
        if (ret != Status::OK) {
            inputBufferQueueConsumer_->ReleaseBuffer(filledInputBuffer);
            return;
        }
        while (true) {
            std::shared_ptr<AVBuffer> emptyOutputBuffer;
            AVBufferConfig avBufferConfig;
            ret = outputBufferQueueProducer_->RequestBuffer(emptyOutputBuffer, avBufferConfig, TIME_OUT_MS);
            if (ret != Status::OK) {
                return;
            }
            ret = codecPlugin_->QueueOutputBuffer(emptyOutputBuffer);
            if (ret == Status::ERROR_NOT_ENOUGH_DATA) {
                continue;
            } else if (ret != Status::OK) {
                outputBufferQueueProducer_->PushBuffer(emptyOutputBuffer, true);
                return;
            } else {
                return;
            }
        }
    }

    void MediaCodec::OnInputBufferDone(const std::shared_ptr<AVBuffer> &inputBuffer) {
        inputBufferQueueConsumer_->ReleaseBuffer(inputBuffer);
    }

    void MediaCodec::OnOutputBufferDone(const std::shared_ptr<AVBuffer> &outputBuffer) {
        outputBufferQueueProducer_->PushBuffer(outputBuffer, false);
    }

    void MediaCodec::OnEvent(const std::shared_ptr<Plugin::PluginEvent> event) {
    }
} //namespace Media
} //namespace OHOS
