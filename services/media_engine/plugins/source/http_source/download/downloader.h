/*
 * Copyright (c) 2024-2024 Huawei Device Co., Ltd.
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
#include "osal/task/condition_variable.h"
#include "osal/task/blocking_queue.h"
#include "osal/utils/util.h"
#include "network/network_client.h"
#include "network/network_typs.h"
#include <chrono>
#include "securec.h"
#include "common/media_source.h"
#include "media_source_loading_request.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {

enum struct DownloadStatus {
    PARTTAL_DOWNLOAD,
};

struct HeaderInfo {
    char contentType[32] = {}; // 32 chars
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
using DataSaveFunc = std::function<uint32_t(uint8_t*, uint32_t, bool)>;
class Downloader;
class DownloadRequest;
using StatusCallbackFunc = std::function<void(DownloadStatus, std::shared_ptr<Downloader>&,
    std::shared_ptr<DownloadRequest>&)>;
using DownloadDoneCbFunc = std::function<void(const std::string&, const std::string&)>;

enum  class RequestProtocolType : int32_t {
    HTTP = 0,
    HLS = 1,
    DASH = 2,
};

class DownloadRequest {
public:
    DownloadRequest(const std::string& url, DataSaveFunc saveData, StatusCallbackFunc statusCallback,
                    bool requestWholeFile = false);
    DownloadRequest(const std::string& url, double duration, DataSaveFunc saveData, StatusCallbackFunc statusCallback,
                    bool requestWholeFile = false);
    DownloadRequest(DataSaveFunc saveData, StatusCallbackFunc statusCallback, RequestInfo requestInfo,
                    bool requestWholeFile = false);
    DownloadRequest(double duration, DataSaveFunc saveData, StatusCallbackFunc statusCallback, RequestInfo requestInfo,
                    bool requestWholeFile = false);
    ~DownloadRequest();
    size_t GetFileContentLength() const;
    size_t GetFileContentLengthNoWait() const;
    void SaveHeader(const HeaderInfo* header);
    Seekable IsChunked(bool isInterruptNeeded);
    bool IsEos() const;
    int GetRetryTimes() const;
    int32_t GetClientError() const;
    int32_t GetServerError() const;
    bool IsSame(const std::shared_ptr<DownloadRequest>& other) const
    {
        return url_ == other->url_ && startPos_ == other->startPos_;
    }
    const std::string GetUrl() const
    {
        return url_;
    }
    const std::map<std::string, std::string>& GetHttpHeader() const
    {
        return httpHeader_;
    }
    void SetUrl(const std::string& url)
    {
        url_ = url;
    }
    void SetIsAuthRequest(bool isAuthRequest)
    {
        isAuthRequest_ = isAuthRequest;
    }
    bool IsAuthRequest() const
    {
        return isAuthRequest_;
    }
    void SetIsIndexM3u8Request(bool isIndexM3u8Request)
    {
        isIndexM3u8Request_ = isIndexM3u8Request;
    }
    bool IsIndexM3u8Request() const
    {
        return isIndexM3u8Request_;
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
    void SetRequestProtocolType(RequestProtocolType protocolType);
    void SetIsM3u8Request(bool isM3u8Request);
    std::atomic<bool> isHeaderUpdated_ {false};
    std::atomic<bool> haveRedirectRetry_ {false};
private:
    void WaitHeaderUpdated() const;
    std::string url_;
    double duration_ {0.0};
    DataSaveFunc saveData_;
    StatusCallbackFunc statusCallback_;
    DownloadDoneCbFunc downloadDoneCallback_;
    mutable std::atomic<bool> isHeaderUpdating_ {false};
    HeaderInfo headerInfo_;
    std::map<std::string, std::string> httpHeader_;
    RequestInfo requestInfo_ {};
    bool isEos_ {false}; // file download finished
    int64_t startPos_ {0};
    int64_t endPos_ {-1};
    int64_t startTimePos_ {0};
    bool isDownloading_ {false};
    bool requestWholeFile_ {false};
    int requestSize_ {0};
    int preRequestSize_ {0};
    int retryTimes_ {0};
    int32_t clientError_ {0};
    int32_t serverError_ {0};
    bool shouldSaveData_ {true};
    int64_t downloadStartTime_ {0};
    int64_t downloadDoneTime_ {0};
    int64_t realRecvContentLen_ {0};
    friend class Downloader;
    std::string location_;
    mutable std::atomic<size_t> times_ {0};
    std::atomic<bool> isInterruptNeeded_{false};
    std::atomic<bool> retryOnGoing_ {false};
    int64_t dropedDataLen_ {0};
    std::atomic<bool> isFirstRangeRequestReady_ {false};
    bool isM3u8Request_ {false};
    bool isIndexM3u8Request_ {false};
    bool isAuthRequest_ {false};
    RequestProtocolType protocolType_ {RequestProtocolType::HTTP};
};

class Downloader {
public:
    explicit Downloader(const std::string& name) noexcept;
    explicit Downloader(const std::string& name, std::shared_ptr<MediaSourceLoaderCombinations> sourceLoader) noexcept;
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
    void SetAppUid(int32_t appUid);
    const std::shared_ptr<DownloadRequest>& GetCurrentRequest();
    void SetInterruptState(bool isInterruptNeeded);
    void SetAppState(bool isAppBackground);
    void StopBufferring();
    std::string GetContentType();

private:
    bool BeginDownload();
    void HttpDownloadLoop();
    void RequestData();
    void HandlePlayingFinish();
    void HandleRetOK();
    static size_t RxBodyData(void* buffer, size_t size, size_t nitems, void* userParam);
    static size_t RxHeaderData(void* buffer, size_t size, size_t nitems, void* userParam);
    static bool HandleContentRange(HeaderInfo* info, char* key, char* next, size_t size, size_t nitems);
    static bool HandleContentType(HeaderInfo* info, char* key, char* next, size_t headerSize,
                                  Downloader* mediaDownloader);
    static bool HandleContentEncode(HeaderInfo* info, char* key, char* next, size_t size, size_t nitems);
    static bool HandleContentLength(HeaderInfo* info, char* key, char* next, Downloader* mediaDownloader);
    static bool HandleRange(HeaderInfo* info, char* key, char* next, size_t size, size_t nitems);
    static void UpdateHeaderInfo(Downloader* mediaDownloader);
    static size_t DropRetryData(void* buffer, size_t dataLen, Downloader* mediaDownloader);
    static bool IsDropDataRetryRequest(Downloader* mediaDownloader);
    static void UpdateCurRequest(Downloader* mediaDownloader, HeaderInfo* header);
    static void HandleFileContentLen(HeaderInfo* header);
    static void UpdateRequestSize(Downloader* mediaDownloader);
    static void ToLower(char* str);
    void PauseLoop(bool isAsync = false);
    void WaitLoopPause();
    void NotifyLoopPause();
    void HandleRetErrorCode();
    void DonwloaderInit(const std::string& name);
    void OpenAppUri();
    void HandleRedirect(Status& ret);

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
        IDLE,
        START,
        PAUSE,
    };
    std::atomic<LoopStatus> loopStatus_ {LoopStatus::IDLE};
    FairMutex loopPauseMutex_ {};
    ConditionVariable loopPauseCond_;
    std::atomic<bool> isAppBackground_ {false};

    int64_t uuid_ {0};
    FairMutex closeMutex_ {};
    std::shared_ptr<MediaSourceLoaderCombinations> sourceLoader_;
    std::shared_ptr<IMediaSourceLoadingRequest> loadingReques_;
    bool isNotBlock_ {false};
    std::string appPreviousRequestUrl_ {};
    FairMutex deinitMutex_ {};
    std::string contentType_;
    bool isContentTypeUpdated_{false};
    ConditionVariable sleepCond_;
    FairMutex sleepMutex_;
};
}
}
}
}
#endif