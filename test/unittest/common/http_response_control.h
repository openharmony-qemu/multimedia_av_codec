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
#ifndef HTTP_RESPONSE_CONTROL_H
#define HTTP_RESPONSE_CONTROL_H

#include <cerrno>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <sys/socket.h>

namespace OHOS::MediaAVCodec {
// A server-side gate: no natural response can make a broken cancellation pass.
class HttpResponseControl {
public:
    enum class Point { NONE, BEFORE_HEADERS, BEFORE_BODY, AFTER_FIRST_CHUNK };
    static constexpr int32_t WAIT_MS = 3000;
    static constexpr int32_t HOLD_MS = 10000;
    static constexpr int32_t POLL_MS = 10;

    bool WaitPaused()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        return cv_.wait_for(lock, std::chrono::milliseconds(WAIT_MS), [this] { return paused_; });
    }

    bool WaitDisconnected()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        return cv_.wait_for(lock, std::chrono::milliseconds(WAIT_MS), [this] { return disconnected_; });
    }

    void Release()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        released_ = true;
        cv_.notify_all();
    }

    bool Hold(int32_t fd)
    {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(HOLD_MS);
        std::unique_lock<std::mutex> lock(mutex_);
        paused_ = true;
        cv_.notify_all();
        while (!released_ && std::chrono::steady_clock::now() < deadline) {
            cv_.wait_for(lock, std::chrono::milliseconds(POLL_MS));
            if (released_) {
                return true;
            }
            lock.unlock();
            char byte {};
            auto count = recv(fd, &byte, sizeof(byte), MSG_PEEK | MSG_DONTWAIT);
            bool ended = count == 0 || (count < 0 && errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR);
            lock.lock();
            if (ended) {
                disconnected_ = true;
                cv_.notify_all();
                return false;
            }
        }
        return released_;
    }

private:
    std::mutex mutex_;
    std::condition_variable cv_;
    bool paused_ {false};
    bool released_ {false};
    bool disconnected_ {false};
};
} // namespace OHOS::MediaAVCodec
#endif
