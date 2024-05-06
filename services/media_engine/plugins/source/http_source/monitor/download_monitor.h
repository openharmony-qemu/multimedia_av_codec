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

#ifndef HISTREAMER_DOWNLOAD_MONITOR_H
#define HISTREAMER_DOWNLOAD_MONITOR_H

#include <ctime>
#include <list>
#include <memory>
#include <string>
#include "osal/task/task.h"
#include "osal/task/mutex.h"
#include "osal/task/blocking_queue.h"
#include "osal/utils/ring_buffer.h"
#include "plugin/plugin_base.h"
#include "download/downloader.h"
#include "media_downloader.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
struct RetryRequest {
    std::shared_ptr<DownloadRequest> request;
    std::function<void()> function;
};

class DownloadMonitor : public MediaDownloader {
public:
    explicit DownloadMonitor(std::shared_ptr<MediaDownloader> downloader) noexcept;
    ~DownloadMonitor() override = default;
    bool Open(const std::string& url, const std::map<std::string, std::string>& httpHeader) override;
    void Close(bool isAsync) override;
    bool Read(unsigned char* buff, unsigned int wantReadLength, unsigned int& realReadLength, bool& isEos) override;
    bool SeekToPos(int64_t offset) override;
    void Pause() override;
    void Resume() override;
    size_t GetContentLength() const override;
    int64_t GetDuration() const override;
    Seekable GetSeekable() const override;
    void SetCallback(Callback *cb) override;
    void SetStatusCallback(StatusCallbackFunc cb) override;
    bool GetStartedStatus() override;
    bool SeekToTime(int64_t seekTime, SeekMode mode) override;
    std::vector<uint32_t> GetBitRates() override;
    bool SelectBitRate(uint32_t bitRate) override;
    void SetIsTriggerAutoMode(bool isAuto) override;
    void SetReadBlockingFlag(bool isReadBlockingAllowed) override;
    void SetDemuxerState() override;
    void SetInterruptState(bool isInterruptNeeded) override;

private:
    int64_t HttpMonitorLoop();
    void OnDownloadStatus(std::shared_ptr<Downloader>& downloader, std::shared_ptr<DownloadRequest>& request);
    bool NeedRetry(const std::shared_ptr<DownloadRequest>& request);

    std::shared_ptr<MediaDownloader> downloader_;
    std::list<RetryRequest> retryTasks_;
    std::atomic<bool> isPlaying_ {false};
    std::shared_ptr<Task> task_;
    time_t lastReadTime_ {0};
    Callback* callback_ {nullptr};
    Mutex taskMutex_ {};
};
}
}
}
}
#endif
