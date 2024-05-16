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
 
#ifndef HISTREAMER_HTTP_MEDIA_DOWNLOADER_H
#define HISTREAMER_HTTP_MEDIA_DOWNLOADER_H

#include <string>
#include <memory>
#include "osal/utils/ring_buffer.h"
#include "osal/utils/steady_clock.h"
#include "download/downloader.h"
#include "media_downloader.h"
#include "timer.h"
#include <unistd.h>

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
class HttpMediaDownloader : public MediaDownloader {
public:
    HttpMediaDownloader() noexcept;
    explicit HttpMediaDownloader(uint32_t expectBufferDuration);
    ~HttpMediaDownloader() override;
    bool Open(const std::string& url, const std::map<std::string, std::string>& httpHeader) override;
    void Close(bool isAsync) override;
    void Pause() override;
    void Resume() override;
    bool Read(unsigned char* buff, unsigned int wantReadLength, unsigned int& realReadLength, bool& isEos) override;
    bool SeekToPos(int64_t offset) override;
    size_t GetContentLength() const override;
    int64_t GetDuration() const override;
    Seekable GetSeekable() const override;
    void SetCallback(Callback* cb) override;
    void SetStatusCallback(StatusCallbackFunc cb) override;
    bool GetStartedStatus() override;
    void SetReadBlockingFlag(bool isReadBlockingAllowed) override;
    void SetDemuxerState() override;
    void SetDownloadErrorState() override;
    void SetInterruptState(bool isInterruptNeeded) override;
    std::pair<int32_t, int32_t> GetDownloadInfo() override;
    int GetBufferSize();
    RingBuffer& GetBuffer();
    bool GetReadFrame();
    bool GetDownloadErrorState();
    StatusCallbackFunc GetStatusCallbackFunc();
    void OnWriteRingBuffer(uint32_t len);
    void DownloadReportLoop();
private:
    bool SaveData(uint8_t* data, uint32_t len);

private:
    std::shared_ptr<RingBuffer> buffer_;
    std::shared_ptr<Downloader> downloader_;
    std::shared_ptr<DownloadRequest> downloadRequest_;
    Mutex mutex_;
    ConditionVariable cvReadWrite_;
    Callback* callback_ {nullptr};
    StatusCallbackFunc statusCallback_ {nullptr};
    bool aboveWaterline_ {false};
    bool startedPlayStatus_ {false};
    uint64_t readTime_ {0};
    bool isReadFrame_ {false};
    bool isTimeOut_ {false};
    bool downloadErrorState_ {false};
    std::atomic<bool> isInterruptNeeded_{false};
    int totalBufferSize_ {0};
    SteadyClock steadyClock_;
    int32_t seekFailedCount_ {0};
    uint64_t totalBits_ {0};
    uint64_t lastBits_ {0};
    uint64_t downloadBits_ {0};
    int64_t openTime_ {0};
    int64_t startDownloadTime_ {0};
    int64_t playDelayTime_ {0};
    int64_t lastCheckTime_ {0};
    int32_t avgDownloadSpeed_ {0};
    bool isDownloadFinish_ {false};
    int32_t avgSpeedSum_ {0};
    uint32_t recordSpeedCount_ {0};
    int64_t lastReportUsageTime_ {0};
    uint64_t dataUsage_ {0};
};
}
}
}
}
#endif