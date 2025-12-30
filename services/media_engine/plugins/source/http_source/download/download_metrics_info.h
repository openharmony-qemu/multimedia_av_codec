/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef HISTREAMER_DOWNLOAD_CALLBACK_H
#define HISTREAMER_DOWNLOAD_CALLBACK_H

#include <chrono>
#include <atomic>
#include <mutex>

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
class DownloadMetricsInfo {
public:
    DownloadMetricsInfo() = default;
    ~DownloadMetricsInfo() = default;
    void UpdateTotalDownloadCount()
    {
        totalDownloadCount_.fetch_add(1);
    }
    void UpdateFirstDownloadTime(int64_t firstDownloadTime, int64_t firstDownloadTimestamp)
    {
        std::lock_guard<std::mutex> lock(firstDownloadMutex_);
        if (firstDownloadTimestamp < firstDownloadTimestamp_.load() || firstDownloadTimestamp_.load() == 0) {
            firstDownloadTime_.store(firstDownloadTime);
            firstDownloadTimestamp_.store(firstDownloadTimestamp);
        }
    }

    void UpdateTotalDownloadTimeAndBytes(int64_t totalDownloadTime, int64_t totalDownloadBytes)
    {
        totalDownloadTime_.fetch_add(totalDownloadTime);
        totalDownLoadBytes_.fetch_add(totalDownloadBytes);
    }

    int32_t GetTotalDownloadCount()
    {
        return totalDownloadCount_.load();
    }

    int64_t GetTotalFirstDownloadTime()
    {
        return firstDownloadTime_.load();
    }

    int64_t GetTotalFirstDownloadTimestamp()
    {
        return firstDownloadTimestamp_.load();
    }

    int64_t GetTotalTotalDownloadTime()
    {
        return totalDownloadTime_.load();
    }

    int64_t GetTotalTotalDownLoadBytes()
    {
        return totalDownLoadBytes_.load();
    }

private:
    std::atomic<int64_t> totalDownLoadBytes_ {0};
    std::atomic<int64_t> totalDownloadTime_ {0};
    std::atomic<int32_t> totalDownloadCount_ {0};
    std::atomic<int64_t> firstDownloadTime_ {0};
    std::atomic<int64_t> firstDownloadTimestamp_ {0};
    std::mutex firstDownloadMutex_;
};
}
}
}
}
#endif