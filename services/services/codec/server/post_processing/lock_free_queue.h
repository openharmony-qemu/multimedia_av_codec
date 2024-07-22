/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef AVCODEC_LOCK_FREE_QUEUE_H
#define AVCODEC_LOCK_FREE_QUEUE_H

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <memory>
#include <type_traits>
#include "utils.h"


namespace OHOS {
namespace MediaAVCodec {

enum QueueResult {
    OK,
    FULL,
    EMPTY,
    INACTIVE,
    NO_MEMORY
};
constexpr size_t QUEUE_RESULT_NUM{5};
constexpr const char* QUEUE_RESULT_DESCRIPTION[QUEUE_RESULT_NUM]{
    "OK",
    "Full",
    "Empty",
    "Inactive",
    "NoMemory"
};

/*
    A simple lock free ring buffer queue for 1 producer and 1 consumer
*/

template<typename T, std::size_t N>
class LockFreeQueue {
public:
    using UnderlyingType = T;

    explicit LockFreeQueue(const std::string& name) : name_(name) {}
    ~LockFreeQueue()
    {
        AVCODEC_LOGD("Queue %{public}s dtor", name_.data());
    }

    static std::shared_ptr<LockFreeQueue<T, N>> Create(const std::string& name = "")
    {
        auto p = std::make_unique<LockFreeQueue<T, N>>(name);
        CHECK_AND_RETURN_RET_LOG(p && p->Alloc(), nullptr, "Create queue failed");
        return p;
    }

    QueueResult PushWait(const T& data)
    {
        CHECK_AND_RETURN_RET_LOG(data_, QueueResult::NO_MEMORY, "Queue %{public}s has no memory", name_.data());
        if (!active_.load()) {
            AVCODEC_LOGD("Queue %{public}s is inactive", name_.data());
            return QueueResult::INACTIVE;
        }
        size_t currentTail{0};
        size_t newTail{0};
        bool canPush{false};
        std::unique_lock<std::mutex> lock(canPushMtx_, std::defer_lock);
        do {
            currentTail = tail_.load();
            newTail = (currentTail + 1) % queueSize_;
            // when queue is full, wait until at least 1 data is popped
            if (newTail == head_.load()) {
                lock.lock();
                canPushCv_.wait(lock, [&newTail, this]() { return newTail != head_.load() || !active_.load(); });
                lock.unlock();
            }
            if (!active_.load()) {
                AVCODEC_LOGD("Queue %{public}s is inactive", name_.data());
                return QueueResult::INACTIVE;
            }
            canPush = tail_.compare_exchange_strong(currentTail, newTail);
        } while (!canPush);

        data_[currentTail].data = data;
        canPopCv_.notify_one();

        return QueueResult::OK;
    }

    bool PopWait(T& data)
    {
        CHECK_AND_RETURN_RET_LOG(data_, QueueResult::NO_MEMORY, "Queue %{public}s has no memory", name_.data());
        if (!active_.load()) {
            AVCODEC_LOGD("Queue %{public}s is inactive", name_.data());
            return QueueResult::INACTIVE;
        }
        size_t currentHead{0};
        size_t newHead{0};
        bool canPop{false};
        std::unique_lock<std::mutex> lock(canPopMtx_, std::defer_lock);

        do {
            currentHead = head_.load();
            newHead = (currentHead + 1) % queueSize_;
            // when queue is empty, wait until at least 1 data is pushed.
            if (currentHead == tail_.load()) {
                lock.lock();
                canPopCv_.wait(lock, [&currentHead, this]() { return currentHead != tail_.load() || !active_.load(); });
                lock.unlock();
            }
            if (!active_.load()) {
                AVCODEC_LOGD("Queue %{public}s is inactive", name_.data());
                return QueueResult::INACTIVE;
            }
            canPop = head_.compare_exchange_strong(currentHead, newHead);
        } while (!canPop);

        data = data_[currentHead].data;
        canPushCv_.notify_one();

        return QueueResult::OK;
    }

    bool Empty() const
    {
        return head_.load() == tail_.load();
    }

    bool Full() const
    {
        return head_.load() == (tail_.load() + 1) % queueSize_;
    }

    void Clear()
    {
        Deactivate();
        head_.store(0);
        tail_.store(0);
    }

    void Deactivate()
    {
        active_.store(false);
        canPushCv_.notify_all();
        canPopCv_.notify_all();
    }

    void Activate()
    {
        active_.store(true);
    }
private:
    struct Node {
        T data;
    };

    bool Alloc()
    {
        // when queue is full, there is a void element between head and tail. So the real length of the data is N + 1.
        data_ = std::make_unique<Node[]>(N + 1);
        return data_ != nullptr;
    }

    std::atomic<size_t> head_{0};
    std::atomic<size_t> tail_{0};
    std::atomic<bool> active_{true};
    std::unique_ptr<Node[]> data_{nullptr};
    static constexpr std::size_t queueSize_{N + 1};
    std::mutex canPushMtx_;
    std::mutex canPopMtx_;
    std::condition_variable canPushCv_;
    std::condition_variable canPopCv_;
    std::string name_;
    static constexpr HiviewDFX::HiLogLabel LABEL{LogLabel("LockFreeQueue")};
};


} // namespace MediaAVCodec
} // namespace OHOS

#endif // AVCODEC_LOCK_FREE_QUEUE_H

