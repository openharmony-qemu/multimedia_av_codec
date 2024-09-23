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

#define MEDIA_PLUGIN
#define HST_LOG_TAG "Downloader"

#include "avcodec_trace.h"
#include "downloader.h"
#include "http_curl_client.h"
#include "osal/utils/steady_clock.h"
#include "securec.h"
#include "plugin/plugin_time.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
namespace {
constexpr int PER_REQUEST_SIZE = 48 * 1024 * 10;
constexpr unsigned int SLEEP_TIME = 5;    // Sleep 5ms
constexpr size_t RETRY_TIMES = 6000;  // Retry 6000 times
constexpr size_t REQUEST_QUEUE_SIZE = 50;
constexpr long LIVE_CONTENT_LENGTH = 2147483646;
constexpr int32_t DOWNLOAD_LOG_FEQUENCE = 10;
constexpr int32_t LOOP_TIMES = 5;
constexpr int32_t LOOP_LOG_FEQUENCE = 50;
}

DownloadRequest::DownloadRequest(const std::string& url, DataSaveFunc saveData, StatusCallbackFunc statusCallback,
                                 bool requestWholeFile)
    : url_(url), saveData_(std::move(saveData)), statusCallback_(std::move(statusCallback)),
    requestWholeFile_(requestWholeFile)
{
    (void)memset_s(&headerInfo_, sizeof(HeaderInfo), 0x00, sizeof(HeaderInfo));
    headerInfo_.fileContentLen = 0;
    headerInfo_.contentLen = 0;
}

DownloadRequest::DownloadRequest(const std::string& url,
                                 double duration,
                                 DataSaveFunc saveData,
                                 StatusCallbackFunc statusCallback,
                                 bool requestWholeFile)
    : url_(url), duration_(duration), saveData_(std::move(saveData)), statusCallback_(std::move(statusCallback)),
    requestWholeFile_(requestWholeFile)
{
    (void)memset_s(&headerInfo_, sizeof(HeaderInfo), 0x00, sizeof(HeaderInfo));
    headerInfo_.fileContentLen = 0;
    headerInfo_.contentLen = 0;
}

DownloadRequest::DownloadRequest(DataSaveFunc saveData, StatusCallbackFunc statusCallback, MediaSouce mediaSouce,
                                 bool requestWholeFile)
    : saveData_(std::move(saveData)), statusCallback_(std::move(statusCallback)), mediaSouce_(mediaSouce),
    requestWholeFile_(requestWholeFile)
{
    (void)memset_s(&headerInfo_, sizeof(HeaderInfo), 0x00, sizeof(HeaderInfo));
    headerInfo_.fileContentLen = 0;
    headerInfo_.contentLen = 0;
    url_ = mediaSouce.url;
    httpHeader_ = mediaSouce.httpHeader;
}

DownloadRequest::DownloadRequest(double duration,
                                 DataSaveFunc saveData,
                                 StatusCallbackFunc statusCallback,
                                 MediaSouce mediaSouce,
                                 bool requestWholeFile)
    : duration_(duration), saveData_(std::move(saveData)), statusCallback_(std::move(statusCallback)),
    mediaSouce_(mediaSouce), requestWholeFile_(requestWholeFile)
{
    (void)memset_s(&headerInfo_, sizeof(HeaderInfo), 0x00, sizeof(HeaderInfo));
    headerInfo_.fileContentLen = 0;
    headerInfo_.contentLen = 0;
    url_ = mediaSouce.url;
    httpHeader_ = mediaSouce.httpHeader;
}

size_t DownloadRequest::GetFileContentLength() const
{
    WaitHeaderUpdated();
    return headerInfo_.GetFileContentLength();
}

size_t DownloadRequest::GetFileContentLengthNoWait() const
{
    return headerInfo_.fileContentLen;
}

void DownloadRequest::SaveHeader(const HeaderInfo* header)
{
    MediaAVCodec::AVCodecTrace trace("DownloadRequest::SaveHeader");
    headerInfo_.Update(header);
    isHeaderUpdated = true;
}

Seekable DownloadRequest::IsChunked(bool isInterruptNeeded)
{
    isInterruptNeeded_ = isInterruptNeeded;
    WaitHeaderUpdated();
    if (isInterruptNeeded) {
        MEDIA_LOG_I("Canceled");
        return Seekable::INVALID;
    }
    if (headerInfo_.isChunked) {
        return GetFileContentLength() == LIVE_CONTENT_LENGTH ? Seekable::SEEKABLE : Seekable::UNSEEKABLE;
    } else {
        return Seekable::SEEKABLE;
    }
};

bool DownloadRequest::IsEos() const
{
    return isEos_;
}

int DownloadRequest::GetRetryTimes() const
{
    return retryTimes_;
}

NetworkClientErrorCode DownloadRequest::GetClientError() const
{
    return clientError_;
}

NetworkServerErrorCode DownloadRequest::GetServerError() const
{
    return serverError_;
}

bool DownloadRequest::IsClosed() const
{
    return headerInfo_.isClosed;
}

void DownloadRequest::Close()
{
    headerInfo_.isClosed = true;
}

void DownloadRequest::WaitHeaderUpdated() const
{
    MediaAVCodec::AVCodecTrace trace("DownloadRequest::WaitHeaderUpdated");

    // Wait Header(fileContentLen etc.) updated
    while (!isHeaderUpdated && times_ < RETRY_TIMES && !isInterruptNeeded_ && !headerInfo_.isClosed) {
        Task::SleepInTask(SLEEP_TIME);
        times_++;
    }
    MEDIA_LOG_D("isHeaderUpdated " PUBLIC_LOG_D32 ", times " PUBLIC_LOG_ZU ", isClosed " PUBLIC_LOG_D32,
        isHeaderUpdated, times_, headerInfo_.isClosed.load());
}

double DownloadRequest::GetDuration() const
{
    return duration_;
}

void DownloadRequest::SetStartTimePos(int64_t startTimePos)
{
    startTimePos_ = startTimePos;
    if (startTimePos_ > 0) {
        shouldSaveData_ = false;
    }
}

void DownloadRequest::SetRangePos(int64_t startPos, int64_t endPos)
{
    startPos_ = startPos;
    endPos_ = endPos;
}

void DownloadRequest::SetDownloadDoneCb(DownloadDoneCbFunc downloadDoneCallback)
{
    downloadDoneCallback_ = downloadDoneCallback;
}

int64_t DownloadRequest::GetNowTime()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>
           (std::chrono::system_clock::now().time_since_epoch()).count();
}

uint32_t DownloadRequest::GetBitRate() const
{
    if ((downloadDoneTime_ == 0) || (downloadStartTime_ == 0) || (realRecvContentLen_ == 0)) {
        return 0;
    }
    int64_t timeGap = downloadDoneTime_ - downloadStartTime_;
    if (timeGap == 0) {
        return 0;
    }
    uint32_t bitRate = static_cast<uint32_t>(realRecvContentLen_ * 1000 *
                        1 * 8 / timeGap); // 1000:ms to sec 1:weight 8:byte to bit
    return bitRate;
}

bool DownloadRequest::IsChunkedVod() const
{
    return headerInfo_.isChunked && headerInfo_.GetFileContentLength() == LIVE_CONTENT_LENGTH;
}

bool DownloadRequest::IsM3u8Request() const
{
    if (url_.find(".ts") != std::string::npos ||
        url_.find(".m3u8") != std::string::npos) {
        MEDIA_LOG_I("request is m3u8.");
        return true;
    }
    return false;
}

bool DownloadRequest::IsServerAcceptRange() const
{
    if (headerInfo_.isChunked) {
        return false;
    }
    return headerInfo_.isServerAcceptRange;
}

void DownloadRequest::GetLocation(std::string& location) const
{
    location = location_;
}

Downloader::Downloader(const std::string& name) noexcept : name_(std::move(name))
{
    shouldStartNextRequest = true;

    client_ = std::make_shared<HttpCurlClient>(&RxHeaderData, &RxBodyData, this);
    client_->Init();
    requestQue_ = std::make_shared<BlockingQueue<std::shared_ptr<DownloadRequest>>>(name_ + "RequestQue",
        REQUEST_QUEUE_SIZE);
    task_ = std::make_shared<Task>(std::string("OS_" + name_ + "Downloader"));
    task_->RegisterJob([this] {
        HttpDownloadLoop();
        NotifyLoopPause();
        return 0;
    });
}

Downloader::~Downloader()
{
    isDestructor_ = true;
    MEDIA_LOG_I("~Downloader In");
    Stop(false);
    if (task_ != nullptr) {
        task_ = nullptr;
    }
    if (client_ != nullptr) {
        client_->Deinit();
        client_ = nullptr;
    }
}

bool Downloader::Download(const std::shared_ptr<DownloadRequest>& request, int32_t waitMs)
{
    MEDIA_LOG_I("In");
    requestQue_->SetActive(true);
    if (waitMs == -1) { // wait until push success
        requestQue_->Push(request);
        return true;
    }
    return requestQue_->Push(request, static_cast<int>(waitMs));
}

void Downloader::Start()
{
    MediaAVCodec::AVCodecTrace trace("Downloader::Start");
    MEDIA_LOG_I("start Begin");
    requestQue_->SetActive(true);
    task_->Start();
    MEDIA_LOG_I("start End");
}

void Downloader::Pause(bool isAsync)
{
    MediaAVCodec::AVCodecTrace trace("Downloader::Pause");
    MEDIA_LOG_I("Pause Begin");
    requestQue_->SetActive(false, false);
    if (client_ != nullptr) {
        isClientClose_ = true;
        client_->Close(isAsync);
    }
    PauseLoop(true);
    if (!isAsync) {
        WaitLoopPause();
    }
    MEDIA_LOG_I("Pause End");
}

void Downloader::Cancel()
{
    MEDIA_LOG_I("Cancel Begin");
    requestQue_->SetActive(false, true);
    if (currentRequest_ != nullptr) {
        currentRequest_->Close();
    }
    if (client_ != nullptr) {
        client_->Close(false);
    }
    shouldStartNextRequest = true;
    PauseLoop(true);
    WaitLoopPause();
    MEDIA_LOG_I("Cancel End");
}


void Downloader::Resume()
{
    MediaAVCodec::AVCodecTrace trace("Downloader::Resume");
    FALSE_RETURN_MSG(!isDestructor_ && !isInterruptNeeded_, "not Resume. is Destructor or InterruptNeeded");
    {
        AutoLock lock(operatorMutex_);
        MEDIA_LOG_I("resume Begin");
        if (isClientClose_ && client_ != nullptr && currentRequest_ != nullptr) {
            isClientClose_ = false;
            client_->Open(currentRequest_->url_, currentRequest_->httpHeader_, currentRequest_->mediaSouce_.timeoutMs);
        }
        requestQue_->SetActive(true);
        if (currentRequest_ != nullptr) {
            currentRequest_->isEos_ = false;
        }
    }
    Start();
    MEDIA_LOG_I("resume End");
}

void Downloader::Stop(bool isAsync)
{
    MediaAVCodec::AVCodecTrace trace("Downloader::Stop");
    MEDIA_LOG_I("Stop Begin");
    isDestructor_ = true;
    if (requestQue_ != nullptr) {
        requestQue_->SetActive(false);
    }
    if (currentRequest_ != nullptr) {
        currentRequest_->isInterruptNeeded_ = true;
        currentRequest_->Close();
    }
    if (client_ != nullptr) {
        client_->Close(isAsync);
        if (!isAsync) {
            client_->Deinit();
        }
    }
    shouldStartNextRequest = true;
    if (task_ != nullptr) {
        if (isAsync) {
            task_->StopAsync();
        } else {
            task_->Stop();
        }
    }
    MEDIA_LOG_I("Stop End");
}

bool Downloader::Seek(int64_t offset)
{
    MediaAVCodec::AVCodecTrace trace("Downloader::Seek, offset: " + std::to_string(offset));
    FALSE_RETURN_V_MSG(!isDestructor_ && !isInterruptNeeded_, false, "not Seek. is Destructor or InterruptNeeded");
    AutoLock lock(operatorMutex_);
    FALSE_RETURN_V(currentRequest_ != nullptr, false);
    size_t contentLength = currentRequest_->GetFileContentLength();
    MEDIA_LOG_I("Seek Begin, offset = " PUBLIC_LOG_D64 ", contentLength = " PUBLIC_LOG_ZU, offset, contentLength);
    if (offset >= 0 && offset < static_cast<int64_t>(contentLength)) {
        currentRequest_->startPos_ = offset;
    }
    size_t temp = currentRequest_->GetFileContentLength() - static_cast<size_t>(currentRequest_->startPos_);
    currentRequest_->requestSize_ = static_cast<int>(std::min(temp, static_cast<size_t>(PER_REQUEST_SIZE)));
    if (downloadRequestSize_ > 0) {
        currentRequest_->requestSize_ = std::min(currentRequest_->requestSize_,
            static_cast<int>(downloadRequestSize_));
        downloadRequestSize_ = 0;
    }
    currentRequest_->isEos_ = false;
    shouldStartNextRequest = false; // Reuse last request when seek
    return true;
}

void Downloader::SetRequestSize(size_t downloadRequestSize)
{
    downloadRequestSize_ = downloadRequestSize;
}

void Downloader::GetIp(std::string &ip)
{
    if (client_ != nullptr) {
        client_->GetIp(ip);
    }
}

// Pause download thread before use currentRequest_
bool Downloader::Retry(const std::shared_ptr<DownloadRequest>& request)
{
    FALSE_RETURN_V_MSG(client_ != nullptr && !isDestructor_ && !isInterruptNeeded_, false,
        "not Retry, client null or isDestructor or isInterruptNeeded");
    {
        AutoLock lock(operatorMutex_);
        MEDIA_LOG_I("Retry Begin");
        FALSE_RETURN_V(client_ != nullptr && !shouldStartNextRequest && !isDestructor_ && !isInterruptNeeded_, false);
        requestQue_->SetActive(false, false);
    }
    PauseLoop(true);
    WaitLoopPause();
    {
        AutoLock lock(operatorMutex_);
        FALSE_RETURN_V(client_ != nullptr && !shouldStartNextRequest && !isDestructor_ && !isInterruptNeeded_, false);
        client_->Close(false);
        if (currentRequest_ != nullptr) {
            if (currentRequest_->IsSame(request) && !shouldStartNextRequest) {
                currentRequest_->retryTimes_++;
                currentRequest_->retryOnGoing_ = true;
                currentRequest_->dropedDataLen_ = 0;
                MEDIA_LOG_D("Do retry.");
            }
            client_->Open(currentRequest_->url_, currentRequest_->httpHeader_, currentRequest_->mediaSouce_.timeoutMs);
            requestQue_->SetActive(true);
            currentRequest_->isEos_ = false;
        }
    }
    task_->Start();
    return true;
}

bool Downloader::BeginDownload()
{
    MEDIA_LOG_I("BeginDownload");
    std::string url = currentRequest_->url_;
    std::map<std::string, std::string> httpHeader = currentRequest_->httpHeader_;
    
    int32_t timeoutMs = currentRequest_->mediaSouce_.timeoutMs;
    FALSE_RETURN_V(!url.empty(), false);
    if (client_) {
        client_->Open(url, httpHeader, timeoutMs);
    }

    if (currentRequest_->endPos_ <= 0) {
        currentRequest_->startPos_ = 0;
        currentRequest_->requestSize_ = 2; // 2
    } else {
        int64_t temp = currentRequest_->endPos_ - currentRequest_->startPos_ + 1;
        currentRequest_->requestSize_ = static_cast<int>(std::min(temp, static_cast<int64_t>(PER_REQUEST_SIZE)));
    }
    currentRequest_->isEos_ = false;
    currentRequest_->retryTimes_ = 0;
    currentRequest_->downloadStartTime_ = currentRequest_->GetNowTime();
    MEDIA_LOG_I("End");
    return true;
}

void Downloader::HttpDownloadLoop()
{
    AutoLock lock(operatorMutex_);
    MEDIA_LOGI_LIMIT(LOOP_LOG_FEQUENCE, "Downloader loop shouldStartNextRequest %{public}d",
        shouldStartNextRequest.load());
    if (shouldStartNextRequest) {
        std::shared_ptr<DownloadRequest> tempRequest = requestQue_->Pop(1000); // 1000ms超时限制
        if (!tempRequest) {
            MEDIA_LOG_W("HttpDownloadLoop tempRequest is null.");
            noTaskLoopTimes_++;
            if (noTaskLoopTimes_ >= LOOP_TIMES) {
                PauseLoop(true);
            }
            return;
        }
        noTaskLoopTimes_ = 0;
        currentRequest_ = tempRequest;
        BeginDownload();
        shouldStartNextRequest = currentRequest_->IsClosed();
    }
    if (currentRequest_ == nullptr || client_ == nullptr) {
        MEDIA_LOG_I("currentRequest_ %{public}d client_ %{public}d nullptr",
                    currentRequest_ != nullptr, client_ != nullptr);
        PauseLoop(true);
        return;
    }
    RequestData();
    return;
}

void Downloader::RequestData()
{
    MediaAVCodec::AVCodecTrace trace("Downloader::HttpDownloadLoop, startPos: "
        + std::to_string(currentRequest_->startPos_) + ", reqSize: " + std::to_string(currentRequest_->requestSize_));
    NetworkClientErrorCode clientCode = NetworkClientErrorCode::ERROR_UNKNOWN;
    NetworkServerErrorCode serverCode = 0;
    int64_t startPos = currentRequest_->startPos_;
    if (currentRequest_->requestWholeFile_ && currentRequest_->shouldSaveData_) {
        startPos = -1;
    }
    Status ret = client_->RequestData(startPos, currentRequest_->requestSize_,
                                      serverCode, clientCode);
    currentRequest_->clientError_ = clientCode;
    currentRequest_->serverError_ = serverCode;
    if (isDestructor_) {
        return;
    }
	
    if (ret == Status::OK) {
        HandleRetOK();
    } else {
        PauseLoop(true);
        MEDIA_LOG_E("Client request data failed. ret = " PUBLIC_LOG_D32 ", clientCode = " PUBLIC_LOG_D32
                    ",request queue size: " PUBLIC_LOG_U64,
                    static_cast<int32_t>(ret), static_cast<int32_t>(clientCode),
                    static_cast<int64_t>(requestQue_->Size()));
        std::shared_ptr<Downloader> unused;
        currentRequest_->statusCallback_(DownloadStatus::PARTTAL_DOWNLOAD, unused, currentRequest_);
    }
}

void Downloader::HandlePlayingFinish()
{
    if (requestQue_->Empty()) {
        PauseLoop(true);
    }
    shouldStartNextRequest = true;
    if (currentRequest_->downloadDoneCallback_ && !isDestructor_) {
        currentRequest_->downloadDoneTime_ = currentRequest_->GetNowTime();
        currentRequest_->downloadDoneCallback_(currentRequest_->GetUrl(), currentRequest_->location_);
    }
}

void Downloader::HandleRetOK()
{
    if (currentRequest_->retryTimes_ > 0) {
        currentRequest_->retryTimes_ = 0;
    }
    if (currentRequest_->headerInfo_.isChunked && requestQue_->Empty()) {
        currentRequest_->isEos_ = true;
        PauseLoop(true);
        return;
    }

    int64_t remaining = 0;
    if (currentRequest_->endPos_ <= 0) {
        remaining = static_cast<int64_t>(currentRequest_->headerInfo_.fileContentLen) -
                    currentRequest_->startPos_;
    } else {
        remaining = currentRequest_->endPos_ - currentRequest_->startPos_ + 1;
    }
    if (currentRequest_->headerInfo_.fileContentLen > 0 && remaining <= 0) { // 检查是否播放结束
        MEDIA_LOG_D("http transfer reach end, startPos_ " PUBLIC_LOG_D64, currentRequest_->startPos_);
        currentRequest_->isEos_ = true;
        HandlePlayingFinish();
        return;
    }
    if (currentRequest_->headerInfo_.fileContentLen == 0 && remaining <= 0) {
        currentRequest_->isEos_ = true;
        currentRequest_->Close();
        HandlePlayingFinish();
        return;
    }
    if (remaining < PER_REQUEST_SIZE) {
        currentRequest_->requestSize_ = remaining;
    } else {
        currentRequest_->requestSize_ = PER_REQUEST_SIZE;
    }
}

void Downloader::UpdateHeaderInfo(Downloader* mediaDownloader)
{
    if (mediaDownloader->currentRequest_->isHeaderUpdated) {
        return;
    }
    MEDIA_LOG_I("UpdateHeaderInfo enter.");
    HeaderInfo* info = &(mediaDownloader->currentRequest_->headerInfo_);
    if (info->contentLen > 0 && info->contentLen < LIVE_CONTENT_LENGTH) {
        info->isChunked = false;
    }
    if (info->contentLen <= 0 && !mediaDownloader->currentRequest_->IsM3u8Request()) {
        info->isChunked = true;
    }
    mediaDownloader->currentRequest_->SaveHeader(info);
}

bool Downloader::IsDropDataRetryRequest(Downloader* mediaDownloader)
{
    bool isWholeFileRetry = mediaDownloader->currentRequest_->requestWholeFile_ &&
                            mediaDownloader->currentRequest_->shouldSaveData_ &&
                            mediaDownloader->currentRequest_->retryOnGoing_;
    if (!isWholeFileRetry) {
        return false;
    }
    if (mediaDownloader->currentRequest_->startPos_ > 0) {
        return true;
    } else {
        mediaDownloader->currentRequest_->retryOnGoing_ = false;
        return false;
    }
}

size_t Downloader::DropRetryData(void* buffer, size_t dataLen, Downloader* mediaDownloader)
{
    auto currentRequest_ = mediaDownloader->currentRequest_;
    int64_t needDropLen = currentRequest_->startPos_ - currentRequest_->dropedDataLen_;
    int64_t writeOffSet = -1;
    if (needDropLen > 0) {
        writeOffSet = needDropLen >= static_cast<int64_t>(dataLen) ? 0 : needDropLen; // 0:drop all
    }
    bool dropRet = false;
    if (writeOffSet > 0) {
        int64_t secondParam = static_cast<int64_t>(dataLen) - writeOffSet;
        if (secondParam < 0) {
            secondParam = 0;
        }
        dropRet = currentRequest_->saveData_(static_cast<uint8_t *>(buffer) + writeOffSet,
                                             static_cast<uint32_t>(secondParam));
        currentRequest_->dropedDataLen_ = currentRequest_->dropedDataLen_ + writeOffSet;
        MEDIA_LOG_D("DropRetryData: last drop, droped len " PUBLIC_LOG_D64 ", startPos_ " PUBLIC_LOG_D64,
                    currentRequest_->dropedDataLen_, currentRequest_->startPos_);
    } else if (writeOffSet == 0) {
        currentRequest_->dropedDataLen_ = currentRequest_->dropedDataLen_ +
                                            static_cast<int64_t>(dataLen);
        dropRet = true;
        MEDIA_LOG_D("DropRetryData: drop, droped len " PUBLIC_LOG_D64 ", startPos_ " PUBLIC_LOG_D64,
                    currentRequest_->dropedDataLen_, currentRequest_->startPos_);
    } else {
        MEDIA_LOG_E("drop data error");
    }
    if (dropRet && currentRequest_->startPos_ == currentRequest_->dropedDataLen_) {
        currentRequest_->retryOnGoing_ = false;
        currentRequest_->dropedDataLen_ = 0;
        if (writeOffSet > 0) {
            currentRequest_->startPos_ = currentRequest_->startPos_ +
                                         static_cast<int64_t>(dataLen) - writeOffSet;
        }
        MEDIA_LOG_I("drop data finished, startPos_ " PUBLIC_LOG_D64, currentRequest_->startPos_);
    }
    return dropRet ? dataLen : 0; // 0:save data failed or drop error
}

void Downloader::UpdateCurRequest(Downloader* mediaDownloader, HeaderInfo* header)
{
    int64_t hstTime = 0;
    Sec2HstTime(mediaDownloader->currentRequest_->GetDuration(), hstTime);
    int64_t startTimePos = mediaDownloader->currentRequest_->startTimePos_;
    int64_t contenLen = static_cast<int64_t>(header->fileContentLen);
    int64_t startPos = 0;
    if (hstTime != 0) {
        startPos = contenLen * startTimePos / (HstTime2Ns(hstTime));
    }
    mediaDownloader->currentRequest_->startPos_ = startPos;
    mediaDownloader->currentRequest_->shouldSaveData_ = true;
    mediaDownloader->currentRequest_->requestWholeFile_ = false;
    mediaDownloader->currentRequest_->requestSize_ = PER_REQUEST_SIZE;
    mediaDownloader->currentRequest_->startTimePos_ = 0;
}

size_t Downloader::RxBodyData(void* buffer, size_t size, size_t nitems, void* userParam)
{
    auto mediaDownloader = static_cast<Downloader *>(userParam);
    size_t dataLen = size * nitems;
    int64_t curLen = mediaDownloader->currentRequest_->realRecvContentLen_;
    int64_t realRecvContentLen = static_cast<int64_t>(dataLen) + curLen;
    UpdateHeaderInfo(mediaDownloader);
    MediaAVCodec::AVCodecTrace trace("Downloader::RxBodyData, dataLen: " + std::to_string(dataLen)
        + ", realRecvContentLen: " + std::to_string(realRecvContentLen));
    if (mediaDownloader->currentRequest_->IsClosed()) {
        return 0;
    }
    if (IsDropDataRetryRequest(mediaDownloader)) {
        return DropRetryData(buffer, dataLen, mediaDownloader);
    }
    HeaderInfo* header = &(mediaDownloader->currentRequest_->headerInfo_);
    if (!mediaDownloader->currentRequest_->shouldSaveData_) {
        UpdateCurRequest(mediaDownloader, header);
        return dataLen;
    }
    if (header->fileContentLen == 0) {
        if (header->contentLen > 0) {
            MEDIA_LOG_W("Unsupported range, use content length as content file length");
            header->fileContentLen = header->contentLen;
        } else {
            MEDIA_LOG_D("fileContentLen and contentLen are both zero.");
        }
    }
    if (!mediaDownloader->currentRequest_->isDownloading_) {
        mediaDownloader->currentRequest_->isDownloading_ = true;
    }
    if (!mediaDownloader->currentRequest_->saveData_(static_cast<uint8_t *>(buffer), dataLen)) {
        MEDIA_LOG_W("Save data failed.");
        return 0; // save data failed, make perform finished.
    }
    mediaDownloader->currentRequest_->realRecvContentLen_ = realRecvContentLen;
    mediaDownloader->currentRequest_->isDownloading_ = false;
    MEDIA_LOGI_LIMIT(DOWNLOAD_LOG_FEQUENCE, "RxBodyData: dataLen " PUBLIC_LOG_ZU ", startPos_ " PUBLIC_LOG_D64, dataLen,
                     mediaDownloader->currentRequest_->startPos_);
    mediaDownloader->currentRequest_->startPos_ = mediaDownloader->currentRequest_->startPos_ + dataLen;
    return dataLen;
}

namespace {
char* StringTrim(char* str)
{
    if (str == nullptr) {
        return nullptr;
    }
    char* p = str;
    char* p1 = p + strlen(str) - 1;

    while (*p && isspace(static_cast<int>(*p))) {
        p++;
    }
    while (p1 > p && isspace(static_cast<int>(*p1))) {
        *p1-- = 0;
    }
    return p;
}
}

bool Downloader::HandleContentRange(HeaderInfo* info, char* key, char* next, size_t size, size_t nitems)
{
    if (!strncmp(key, "Content-Range", strlen("Content-Range")) ||
        !strncmp(key, "content-range", strlen("content-range"))) {
        char* token = strtok_s(nullptr, ":", &next);
        FALSE_RETURN_V(token != nullptr, false);
        char* strRange = StringTrim(token);
        size_t start;
        size_t end;
        size_t fileLen;
        FALSE_LOG_MSG(sscanf_s(strRange, "bytes %lu-%lu/%lu", &start, &end, &fileLen) != -1,
            "sscanf get range failed");
        if (info->fileContentLen > 0 && info->fileContentLen != fileLen) {
            MEDIA_LOG_E("FileContentLen doesn't equal to fileLen");
        }
        if (info->fileContentLen == 0) {
            info->fileContentLen = fileLen;
        }
    }
    return true;
}

bool Downloader::HandleContentType(HeaderInfo* info, char* key, char* next, size_t size, size_t nitems)
{
    if (!strncmp(key, "Content-Type", strlen("Content-Type"))) {
        char* token = strtok_s(nullptr, ":", &next);
        FALSE_RETURN_V(token != nullptr, false);
        char* type = StringTrim(token);
        std::string tokenStr = (std::string)token;
        MEDIA_LOG_I("Content-Type: " PUBLIC_LOG_S, tokenStr.c_str());
        NZERO_LOG(memcpy_s(info->contentType, sizeof(info->contentType), type, strlen(type)));
    }
    return true;
}

bool Downloader::HandleContentEncode(HeaderInfo* info, char* key, char* next, size_t size, size_t nitems)
{
    if (!strncmp(key, "Content-Encode", strlen("Content-Encode")) ||
        !strncmp(key, "content-encode", strlen("content-encode"))) {
        char* token = strtok_s(nullptr, ":", &next);
        FALSE_RETURN_V(token != nullptr, false);
        std::string tokenStr = (std::string)token;
        MEDIA_LOG_I("Content-Encode: " PUBLIC_LOG_S, tokenStr.c_str());
    }
    return true;
}

bool Downloader::HandleContentLength(HeaderInfo* info, char* key, char* next, Downloader* mediaDownloader)
{
    FALSE_RETURN_V(key != nullptr, false);
    if (!strncmp(key, "Content-Length", strlen("Content-Length")) ||
        !strncmp(key, "content-length", strlen("content-length"))) {
        FALSE_RETURN_V(next != nullptr, false);
        char* token = strtok_s(nullptr, ":", &next);
        FALSE_RETURN_V(token != nullptr, false);
        if (info != nullptr && mediaDownloader != nullptr) {
            info->contentLen = atol(StringTrim(token));
            if (info->contentLen <= 0 && !mediaDownloader->currentRequest_->IsM3u8Request()) {
                info->isChunked = true;
            }
        }
    }
    return true;
}

bool Downloader::HandleContentLength(HeaderInfo* info, char* key, char* next, size_t size, size_t nitems)
{
    if (!strncmp(key, "Content-Length", strlen("Content-Length")) ||
        !strncmp(key, "content-length", strlen("content-length"))) {
        char* token = strtok_s(nullptr, ":", &next);
        FALSE_RETURN_V(token != nullptr, false);
        info->contentLen = atol(StringTrim(token));
        if (info->contentLen <= 0) {
            info->isChunked = true;
        }
    }
    return true;
}

// Check if this server supports range download. (HTTP)
bool Downloader::HandleRange(HeaderInfo* info, char* key, char* next, size_t size, size_t nitems)
{
    if (!strncmp(key, "Accept-Ranges", strlen("Accept-Ranges")) ||
        !strncmp(key, "accept-ranges", strlen("accept-ranges"))) {
        char* token = strtok_s(nullptr, ":", &next);
        FALSE_RETURN_V(token != nullptr, false);
        if (!strncmp(StringTrim(token), "bytes", strlen("bytes"))) {
            info->isServerAcceptRange = true;
            MEDIA_LOG_I("Accept-Ranges: bytes");
        }
    }
    if (!strncmp(key, "Content-Range", strlen("Content-Range")) ||
        !strncmp(key, "content-range", strlen("content-range"))) {
        char* token = strtok_s(nullptr, ":", &next);
        FALSE_RETURN_V(token != nullptr, false);
        std::string tokenStr = (std::string)token;
        if (tokenStr.find("bytes") != std::string::npos) {
            info->isServerAcceptRange = true;
            MEDIA_LOG_I("Content-Range: " PUBLIC_LOG_S, tokenStr.c_str());
        }
    }
    return true;
}

size_t Downloader::RxHeaderData(void* buffer, size_t size, size_t nitems, void* userParam)
{
    MediaAVCodec::AVCodecTrace trace("Downloader::RxHeaderData");
    auto mediaDownloader = reinterpret_cast<Downloader *>(userParam);
    HeaderInfo* info = &(mediaDownloader->currentRequest_->headerInfo_);
    if (mediaDownloader->currentRequest_->isHeaderUpdated) {
        return size * nitems;
    }
    char* next = nullptr;
    char* key = strtok_s(reinterpret_cast<char*>(buffer), ":", &next);
    FALSE_RETURN_V(key != nullptr, size * nitems);

    if (!strncmp(key, "Transfer-Encoding", strlen("Transfer-Encoding")) ||
        !strncmp(key, "transfer-encoding", strlen("transfer-encoding"))) {
        char* token = strtok_s(nullptr, ":", &next);
        FALSE_RETURN_V(token != nullptr, size * nitems);
        if (!strncmp(StringTrim(token), "chunked", strlen("chunked")) &&
            !mediaDownloader->currentRequest_->IsM3u8Request()) {
            info->isChunked = true;
            if (static_cast<int32_t>(mediaDownloader->currentRequest_->url_.find(".flv") == std::string::npos)) {
                info->contentLen = LIVE_CONTENT_LENGTH;
            } else {
                info->contentLen = 0;
            }
            std::string tokenStr = (std::string)token;
            MEDIA_LOG_I("Transfer-Encoding: " PUBLIC_LOG_S, tokenStr.c_str());
        }
    }
    if (!strncmp(key, "Location", strlen("Location")) ||
        !strncmp(key, "location", strlen("location"))) {
        FALSE_RETURN_V(next != nullptr, size * nitems);
        char* location = StringTrim(next);
        mediaDownloader->currentRequest_->location_ = location;
    }

    if (!HandleContentRange(info, key, next, size, nitems) || !HandleContentType(info, key, next, size, nitems) ||
        !HandleContentEncode(info, key, next, size, nitems) ||
        !HandleContentLength(info, key, next, mediaDownloader) ||
        !HandleRange(info, key, next, size, nitems)) {
        return size * nitems;
    }

    return size * nitems;
}

void Downloader::PauseLoop(bool isAsync)
{
    if (task_ == nullptr) {
        return;
    }
    if (isAsync) {
        task_->PauseAsync();
    } else {
        task_->Pause();
    }
}

const std::shared_ptr<DownloadRequest>& Downloader::GetCurrentRequest()
{
    return currentRequest_;
}

void Downloader::SetInterruptState(bool isInterruptNeeded)
{
    isInterruptNeeded_ = isInterruptNeeded;
    if (currentRequest_ != nullptr) {
        currentRequest_->isInterruptNeeded_ = isInterruptNeeded;
    }
    NotifyLoopPause();
}

void Downloader::NotifyLoopPause()
{
    FALSE_RETURN(!(loopStatus_ == LoopStatus::PAUSE || isInterruptNeeded_));
    AutoLock lk(loopPauseMutex_);
    if (loopStatus_ == LoopStatus::PAUSE || isInterruptNeeded_) {
        MEDIA_LOG_I("Downloader NotifyLoopPause");
        loopStatus_ = LoopStatus::PAUSEDONE;
        loopPauseCond_.NotifyAll();
    }
}

void Downloader::WaitLoopPause()
{
    FALSE_RETURN(task_->IsTaskRunning());
    AutoLock lk(loopPauseMutex_);
    MEDIA_LOG_I("Downloader WaitLoopPause task Running %{public}d, loopStatus_ %{public}d",
        task_->IsTaskRunning(), loopStatus_.load());
    FALSE_RETURN(task_->IsTaskRunning());
    loopStatus_ = LoopStatus::PAUSE;
    loopPauseCond_.Wait(lk, [this]() {
        return loopStatus_ == LoopStatus::PAUSEDONE || isInterruptNeeded_;
    });
    loopStatus_ = LoopStatus::NORMAL;
}

}
}
}
}
