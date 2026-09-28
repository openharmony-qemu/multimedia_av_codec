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
#ifndef SNIFFER_TEST_SUPPORT_H
#define SNIFFER_TEST_SUPPORT_H

#include <chrono>
#include <atomic>
#include <climits>
#include <condition_variable>
#include <future>
#include <fcntl.h>
#include <mutex>
#include <thread>
#include <unistd.h>
#include "sniffer/sniffer_coordinator.h"

namespace OHOS::Media::Plugins::HttpPlugin::SnifferTest {
constexpr int32_t WAIT_MS = 3000;
constexpr int32_t POLL_MS = 5;
constexpr int32_t TEST_UID = 12345; // Synthetic identity, never used for an external network request.
constexpr int32_t WRITE_ERROR = 23;
constexpr int32_t ABORT_ERROR = 42;
constexpr size_t READ_SIZE = 4096;
constexpr char HLS[] = "#EXTM3U\n#EXT-X-TARGETDURATION:6\n#EXTINF:6,\nseg.ts\n#EXT-X-ENDLIST\n";

class Signal {
public:
    void Set()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        ready_ = true;
        cv_.notify_all();
    }

    bool Wait(int32_t timeoutMs = WAIT_MS)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        return cv_.wait_for(lock, std::chrono::milliseconds(timeoutMs), [this] { return ready_; });
    }

private:
    std::mutex mutex_;
    std::condition_variable cv_;
    bool ready_ {false};
};

struct ReleaseObservation {
    Signal queried;
    std::atomic<bool> outsideLock {false};
    std::atomic<bool> replacementVisible {false};
};

inline void ObserveRelease(const std::shared_ptr<ReleaseObservation>& state, std::function<bool()> getter)
{
    constexpr int32_t DESTRUCTOR_QUERY_MS = 1000;
    std::thread reader([state, getter = std::move(getter)] {
        state->replacementVisible.store(getter());
        state->queried.Set();
    });
    bool completed = state->queried.Wait(DESTRUCTOR_QUERY_MS);
    state->outsideLock.store(completed);
    if (completed) {
        reader.join();
    } else {
        // A failing implementation may hold the pointer lock until this destructor returns.
        reader.detach(); // The reader owns its state; the test waits for it after releasing the pointer.
    }
}

template<typename Predicate>
bool WaitFor(Predicate predicate)
{
    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(WAIT_MS);
    while (!predicate() && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(POLL_MS));
    }
    return predicate();
}

inline std::shared_ptr<SnifferCoordinator> Pending()
{
    auto probe = std::make_shared<SnifferCoordinator>();
    probe->result_ = std::make_shared<SniffResult>();
    probe->state_ = SnifferCoordinator::State::PROBING;
    probe->deadline_ = std::chrono::steady_clock::now() + std::chrono::milliseconds(WAIT_MS);
    return probe;
}

class TempFile {
public:
    explicit TempFile(const std::string& bytes)
    {
        fd = mkstemp(path_);
        if (fd >= 0) {
            written = write(fd, bytes.data(), bytes.size());
        }
    }

    ~TempFile()
    {
        if (fd >= 0) {
            close(fd);
        }
        unlink(path_);
    }

    std::string Uri() const
    {
        return "fd://" + std::to_string(fd);
    }

    bool MakeWriteOnly()
    {
        int replacement = open(path_, O_WRONLY);
        if (replacement < 0) {
            return false;
        }
        close(fd);
        fd = replacement;
        return true;
    }

    int fd {-1};
    ssize_t written {-1};

private:
    char path_[PATH_MAX] = "/data/test/media/sniffer_case_XXXXXX";
};
} // namespace OHOS::Media::Plugins::HttpPlugin::SnifferTest
#endif
