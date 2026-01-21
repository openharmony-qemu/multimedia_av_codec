/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#ifndef UTILS_BLOCK_QUEUE_H
#define UTILS_BLOCK_QUEUE_H
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <deque>
#include "common/log.h"

namespace OHOS {
namespace Media {

template <typename T>
class BlockQueue {
public:
    explicit BlockQueue(std::string name, size_t capacity = 10) // 10 is default capacity
        : name_(std::move(name)), capacity_(capacity), isActive_(true)
    {
    }

    ~BlockQueue() = default;

    size_t Size()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return que_.size();
    }

    size_t Capacity()
    {
        return capacity_;
    }

    bool Empty()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return que_.empty();
    }

    bool Push(const T& block)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        MEDIA_LOG_D("In, block queue " PUBLIC_LOG_S, name_.c_str());
        if (!isActive_) {
            MEDIA_LOG_D("Block queue " PUBLIC_LOG_S " is inactive", name_.c_str());
            return false;
        }
        if (que_.size() >= capacity_) {
            MEDIA_LOG_D("Block queue " PUBLIC_LOG_S " is full", name_.c_str());
            condFull_.wait(lock, [this] { return !isActive_ || que_.size() < capacity_; });
        }
        if (!isActive_) {
            MEDIA_LOG_D("Block queue " PUBLIC_LOG_S ": inactive: " PUBLIC_LOG_D32 ", isFull: " PUBLIC_LOG_D32,
                name_.c_str(), isActive_.load(), que_.size() < capacity_);
            return false;
        }
        que_.push_back(block);
        condEmpty_.notify_one();
        MEDIA_LOG_D("Out, block queue " PUBLIC_LOG_S, name_.c_str());
        return true;
    }

    T Pop()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        MEDIA_LOG_D("In, block queue " PUBLIC_LOG_S, name_.c_str());
        if (que_.empty() && !isActive_) {
            MEDIA_LOG_D("Block queue " PUBLIC_LOG_S " is inactive", name_.c_str());
            return {};
        }
        if (que_.empty() && isActive_) {
            MEDIA_LOG_D("Block queue " PUBLIC_LOG_S " is empty", name_.c_str());
            condEmpty_.wait(lock, [this] { return !isActive_ || !que_.empty(); });
        }
        if (que_.empty()) {
            MEDIA_LOG_D("Block queue " PUBLIC_LOG_S ": inactive: " PUBLIC_LOG_D32 ", size: " PUBLIC_LOG_ZU,
                name_.c_str(), isActive_.load(), que_.size());
            return {};
        }
        T element = que_.front();
        que_.pop_front();
        condFull_.notify_one();
        MEDIA_LOG_D("Out, block queue " PUBLIC_LOG_S, name_.c_str());
        return element;
    }

    T Front()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        MEDIA_LOG_D("In, block queue " PUBLIC_LOG_S, name_.c_str());
        if (que_.empty() && !isActive_) {
            MEDIA_LOG_D("Block queue " PUBLIC_LOG_S " is inactive", name_.c_str());
            return {};
        }
        if (que_.empty() && isActive_) {
            MEDIA_LOG_D("Block queue " PUBLIC_LOG_S " is empty", name_.c_str());
            condEmpty_.wait(lock, [this] { return !isActive_ || !que_.empty(); });
        }
        if (que_.empty()) {
            MEDIA_LOG_D("Block queue " PUBLIC_LOG_S ": inactive: " PUBLIC_LOG_D32 ", size: " PUBLIC_LOG_ZU,
                name_.c_str(), isActive_.load(), que_.size());
            return {};
        }
        T element = que_.front();
        condFull_.notify_one();
        MEDIA_LOG_D("Out, block queue " PUBLIC_LOG_S, name_.c_str());
        return element;
    }

    T Back()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        MEDIA_LOG_D("In, block queue " PUBLIC_LOG_S, name_.c_str());
        if (que_.empty() && !isActive_) {
            MEDIA_LOG_D("Block queue " PUBLIC_LOG_S " is inactive", name_.c_str());
            return {};
        }
        if (que_.empty() && isActive_) {
            MEDIA_LOG_D("Block queue " PUBLIC_LOG_S " is empty", name_.c_str());
            condEmpty_.wait(lock, [this] { return !isActive_ || !que_.empty(); });
        }
        if (que_.empty()) {
            MEDIA_LOG_D("Block queue " PUBLIC_LOG_S ": inactive: " PUBLIC_LOG_D32 ", size: " PUBLIC_LOG_ZU,
                name_.c_str(), isActive_.load(), que_.size());
            return {};
        }
        T element = que_.back();
        condFull_.notify_one();
        MEDIA_LOG_D("Out, block queue " PUBLIC_LOG_S, name_.c_str());
        return element;
    }

    void Clear()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        ClearUnprotected();
    }

    void SetActive(bool active, bool cleanData = true)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        MEDIA_LOG_D("SetActive " PUBLIC_LOG_S ": " PUBLIC_LOG_D32, name_.c_str(), isActive_.load());
        isActive_ = active;
        if (!active) {
            if (cleanData) {
                ClearUnprotected();
            }
            condEmpty_.notify_one();
        }
    }

private:
    static constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_DEMUXER, "BlockQueue" };
    void ClearUnprotected()
    {
        if (que_.empty()) {
            return;
        }
        bool needNotify = que_.size() == capacity_;
        std::deque<T>().swap(que_);
        if (needNotify) {
            condFull_.notify_one();
        }
    }

    std::mutex mutex_;
    std::condition_variable condFull_;
    std::condition_variable condEmpty_;
    std::deque<T> que_;
    std::string name_;
    const size_t capacity_;
    std::atomic<bool> isActive_;
};
} // namespace Media
} // namespace OHOS
#endif // !UTILS_BLOCK_QUEUE_H
