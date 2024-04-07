/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef VCODEC_SIGNAL_H
#define VCODEC_SIGNAL_H
#include <atomic>
#include <memory>
#include <mutex>
#include <pthread.h>
#include <queue>
#include <string>
#include <thread>
#include "native_avbuffer.h"
#include "native_avformat.h"
#include "native_avmemory.h"

namespace OHOS {
namespace MediaAVCodec {
template <typename T>
class VCodecSignal {
public:
    explicit VCodecSignal(std::shared_ptr<T> codec) : codec_(codec) {}
    ~VCodecSignal()
    {
        if (outFile_ != nullptr && outFile_->is_open()) {
            outFile_->close();
        }
        if (inFile_ != nullptr && inFile_->is_open()) {
            inFile_->close();
        }
    }
    std::mutex eosMutex_;
    std::condition_variable eosCond_;
    std::atomic<bool> isInEos_ = false;
    std::atomic<bool> isOutEos_ = false;

    std::mutex inMutex_;
    std::condition_variable inCond_;
    std::queue<uint32_t> inQueue_;
    std::queue<OH_AVMemory *> inMemoryQueue_;
    std::queue<OH_AVBuffer *> inBufferQueue_;

    std::mutex outMutex_;
    std::condition_variable outCond_;
    std::queue<uint32_t> outQueue_;
    std::queue<OH_AVCodecBufferAttr> outAttrQueue_;
    std::queue<OH_AVMemory *> outMemoryQueue_;
    std::queue<OH_AVBuffer *> outBufferQueue_;

    std::vector<int32_t> errors_;
    std::atomic<int32_t> controlNum_ = 0;
    std::atomic<bool> isRunning_ = false;
    std::atomic<bool> isFlushing_ = false;

    std::unique_ptr<std::ifstream> inFile_;
    std::unique_ptr<std::ofstream> outFile_;
    std::weak_ptr<T> codec_;

    void PopInQueue()
    {
        inQueue_.pop();
        inMemoryQueue_.pop();
        inBufferQueue_.pop();
    }

    void PopOutQueue()
    {
        outQueue_.pop();
        outAttrQueue_.pop();
        outMemoryQueue_.pop();
        outBufferQueue_.pop();
    }
};

template <typename T>
class FlushGuard {
public:
    explicit FlushGuard(std::shared_ptr<T> signal)
    {
        if (signal == nullptr) {
            return;
        }
        signal_ = signal;
        signal_->isFlushing_ = true;
        signal_->inCond_.notify_all();
        signal_->outCond_.notify_all();
        {
            std::scoped_lock lock(signal_->inMutex_, signal_->outMutex_);
            FlushInQueue();
            FlushOutQueue();
        }
    }

    ~FlushGuard()
    {
        if (signal_ == nullptr) {
            return;
        }
        signal_->isFlushing_ = false;
        signal_->inCond_.notify_all();
        signal_->outCond_.notify_all();
    }

private:
    void FlushInQueue()
    {
        std::queue<uint32_t> tempIndex;
        swap(tempIndex, signal_->inQueue_);
        std::queue<OH_AVMemory *> tempInMemory;
        swap(tempInMemory, signal_->inMemoryQueue_);
        std::queue<OH_AVBuffer *> tempInBuffer;
        swap(tempInBuffer, signal_->inBufferQueue_);
    }

    void FlushOutQueue()
    {
        std::queue<uint32_t> tempIndex;
        swap(tempIndex, signal_->outQueue_);
        std::queue<OH_AVCodecBufferAttr> tempOutAttr;
        swap(tempOutAttr, signal_->outAttrQueue_);
        std::queue<OH_AVMemory *> tempOutMemory;
        swap(tempOutMemory, signal_->outMemoryQueue_);
        std::queue<OH_AVBuffer *> tempOutBuffer;
        swap(tempOutBuffer, signal_->outBufferQueue_);
    }
    std::shared_ptr<T> signal_ = nullptr;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // VCODEC_SIGNAL_H