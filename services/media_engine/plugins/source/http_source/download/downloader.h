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
 
#ifndef HISTREAMER_DOWNLOADER_H
#define HISTREAMER_DOWNLOADER_H

#include <functional>
#include <memory>
#include <string>
#include "osal/task/task.h"
#include "osal/task/mutex.h"
#include "osal/task/blocking_queue.h"
#include "osal/utils/util.h"
#include "network_client.h"
#include <chrono>
#include "securec.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_STREAM_SOURCE, "HiStreamer" };

enum struct DownloadStatus {
    PARTTAL_DOWNLOAD,
};

struct HeaderInfo {
    char contentType[32]; // 32 chars
    size_t fileContentLen {0};
    mutable size_t retryTimes {0};
    const static size_t maxRetryTimes {100};
    const static int sleepTime {10};
    long contentLen {0};
    bool isChunked {false};
    std::atomic<bool> isClosed {false};
    bool isServerAcceptRange {false};

    void Update(const HeaderInfo* info)
    {
        NZERO_LOG(memcpy_s(contentType, sizeof(contentType), info->contentType, sizeof(contentType)));
        fileContentLen = info->fileContentLen;
        contentLen = info->contentLen;
        isChunked = info->isChunked;
    }

    size_t GetFileContentLength() const
    {
        while (fileContentLen == 0 && !isChunked && !isClosed && retryTimes < maxRetryTimes) {
            OSAL::SleepFor(sleepTime); // 10, wait for fileContentLen updated
            retryTimes++;
        }
        return fileContentLen;
    }
};

// uint8_t* : the data should save
// uint32_t : length
using DataSaveFunc = std::function<bool(uint8_t*, uint32_t)>;
class Downloader;
class DownloadRequest;
using StatusCallbackFunc = std::function<void(DownloadStatus, std::shared_ptr<Downloader>&,
    std::shared_ptr<DownloadRequest>&)>;
using DownloadDoneCbFunc = std::function<void(const std::string&, const std::string&)>;

struct MediaSouce {
    std::string url;
    std::map<std::string, std::string> httpHeader;
    int32_t timeoutMs{-1};
};

class DownloadRequest {
public:
    DownloadRequest(const std::string& url, DataSaveFunc saveData, StatusCallbackFunc statusCallback,
                    bool requestWholeFile = false);
    DownloadRequest(const std::string& url, double duration, DataSaveFunc saveData, StatusCallbackFunc statusCallback,
                    bool requestWholeFile = false);
    DownloadRequest(DataSaveFunc saveData, StatusCallbackFunc statusCallback, MediaSouce mediaSouce,
                    bool requestWholeFile = false);
    DownloadRequest(double duration, DataSaveFunc saveData, StatusCallbackFunc statusCallback, MediaSouce mediaSouce,
                    bool requestWholeFile = false);

    size_t GetFileContentLength() const;
    size_t GetFileContentLengthNoWait() const;
    void SaveHeader(const HeaderInfo* header);
    Seekable IsChunked(bool isInterruptNeeded);
    bool IsEos() const;
    int GetRetryTimes() const;
    NetworkClientErrorCode GetClientError() const;
    NetworkServerErrorCode GetServerError() const;
    bool IsSame(const std::shared_ptr<DownloadRequest>& other) const
    {
        return url_ == other->url_ && startPos_ == other->startPos_;
    }
    const std::string GetUrl() const
    {
        return url_;
    }
    bool IsClosed() const;
    void Close();
    double GetDuration() const;
    void SetStartTimePos(int64_t startTimePos);
    void SetRangePos(int64_t startPos, int64_t endPos);
    void SetDownloadDoneCb(DownloadDoneCbFunc downloadDoneCallback);
    int64_t GetNowTime();
    uint32_t GetBitRate() const;
    bool IsChunkedVod() const;
    bool IsM3u8Request() const;
    bool IsServerAcceptRange() const;
    void GetLocation(std::string& location) const;
private:
    void WaitHeaderUpdated() const;
    std::string url_;
    double duration_ {0.0};
    DataSaveFunc saveData_;
    StatusCallbackFunc statusCallback_;
    DownloadDoneCbFunc downloadDoneCallback_;

    HeaderInfo headerInfo_;
    std::map<std::string, std::string> httpHeader_;
    MediaSouce mediaSouce_ {};

    bool isHeaderUpdated {false};
    bool isEos_ {false}; // file download finished
    int64_t startPos_ {0};
    int64_t endPos_ {-1};
    int64_t startTimePos_ {0};
    bool isDownloading_ {false};
    bool requestWholeFile_ {false};
    int requestSize_ {0};
    int retryTimes_ {0};
    NetworkClientErrorCode clientError_ {NetworkClientErrorCode::ERROR_OK};
    NetworkServerErrorCode serverError_ {0};
    bool shouldSaveData_ {true};
    int64_t downloadStartTime_ {0};
    int64_t downloadDoneTime_ {0};
    int64_t realRecvContentLen_ {0};
    friend class Downloader;
    std::string location_;
    mutable size_t times_ {0};
    std::atomic<bool> isInterruptNeeded_{false};
    std::atomic<bool> retryOnGoing_ {false};
    int64_t dropedDataLen_ {0};
};

class Downloader {
public:
    explicit Downloader(const std::string& name) noexcept;
    virtual ~Downloader();

    bool Download(const std::shared_ptr<DownloadRequest>& request, int32_t waitMs);
    void Start();
    void Pause(bool isAsync = false);
    void Resume();
    void Stop(bool isAsync = false);
    bool Seek(int64_t offset);
    void Cancel();
    bool Retry(const std::shared_ptr<DownloadRequest>& request);
    void SetRequestSize(size_t downloadRequestSize);
    void GetIp(std::string &ip);
    const std::shared_ptr<DownloadRequest>& GetCurrentRequest();
    void SetInterruptState(bool isInterruptNeeded);
private:
    bool BeginDownload();

    void HttpDownloadLoop();
    void RequestData();
    void HandlePlayingFinish();
    void HandleRetOK();
    static size_t RxBodyData(void* buffer, size_t size, size_t nitems, void* userParam);
    static size_t RxHeaderData(void* buffer, size_t size, size_t nitems, void* userParam);
    static bool HandleContentRange(HeaderInfo* info, char* key, char* next, size_t size, size_t nitems);
    static bool HandleContentType(HeaderInfo* info, char* key, char* next, size_t size, size_t nitems);
    static bool HandleContentEncode(HeaderInfo* info, char* key, char* next, size_t size, size_t nitems);
    static bool HandleContentLength(HeaderInfo* info, char* key, char* next, Downloader* mediaDownloader);
    static bool HandleContentLength(HeaderInfo* info, char* key, char* next, size_t size, size_t nitems);
    static bool HandleRange(HeaderInfo* info, char* key, char* next, size_t size, size_t nitems);
    static void UpdateHeaderInfo(Downloader* mediaDownloader);
    static size_t DropRetryData(void* buffer, size_t dataLen, Downloader* mediaDownloader);
    static bool IsDropDataRetryRequest(Downloader* mediaDownloader);
    static void UpdateCurRequest(Downloader* mediaDownloader, HeaderInfo* header);
    void PauseLoop(bool isAsync = false);
    void WaitLoopPause();
    void NotifyLoopPause();

    std::string name_;
    std::shared_ptr<NetworkClient> client_;
    std::shared_ptr<BlockingQueue<std::shared_ptr<DownloadRequest>>> requestQue_;
    FairMutex operatorMutex_{};
    std::shared_ptr<DownloadRequest> currentRequest_;
    std::atomic<bool> shouldStartNextRequest {false};
    int32_t noTaskLoopTimes_ {0};
    size_t downloadRequestSize_ {0};
    std::shared_ptr<Task> task_;
    std::atomic<bool> isDestructor_ {false};
    std::atomic<bool> isClientClose_ {false};
    std::atomic<bool> isInterruptNeeded_{false};

    enum struct LoopStatus {
        NORMAL,
        PAUSE,
        PAUSEDONE,
    };
    std::atomic<LoopStatus> loopStatus_ {LoopStatus::NORMAL};
    FairMutex loopPauseMutex_ {};
    ConditionVariable loopPauseCond_;
};
}
}
}
}
#endif