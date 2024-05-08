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
constexpr size_t RETRY_TIMES = 200;  // Retry 200 times
constexpr size_t REQUEST_QUEUE_SIZE = 50;
constexpr long LIVE_CONTENT_LENGTH = 2147483646;
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
        return Seekable::UNSEEKABLE;
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
    while (!isHeaderUpdated && times_ < RETRY_TIMES && !isInterruptNeeded_) {
        Task::SleepInTask(SLEEP_TIME);
        times_++;
    }
    MEDIA_LOG_D("isHeaderUpdated " PUBLIC_LOG_D32 ", times " PUBLIC_LOG_ZU, isHeaderUpdated, times_);
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
    uint32_t bitRate = static_cast<uint32_t>(realRecvContentLen_ / timeGap * 1000 *
                       1 * 8); // 1000:ms to sec 1:weight 8:byte to bit
    return bitRate;
}

Downloader::Downloader(const std::string& name) noexcept : name_(std::move(name))
{
    shouldStartNextRequest = true;

    client_ = std::make_shared<HttpCurlClient>(&RxHeaderData, &RxBodyData, this);
    client_->Init();
    requestQue_ = std::make_shared<BlockingQueue<std::shared_ptr<DownloadRequest>>>(name_ + "RequestQue",
        REQUEST_QUEUE_SIZE);
    task_ = std::make_shared<Task>(std::string("OS_" + name_ + "Downloader"), "", TaskType::SINGLETON);
    task_->RegisterJob([this] { return HttpDownloadLoop(); });
}

Downloader::~Downloader()
{
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

void Downloader::Pause()
{
    MediaAVCodec::AVCodecTrace trace("Downloader::Pause");
    {
        AutoLock lock(operatorMutex_);
        MEDIA_LOG_I("pause Begin");
        requestQue_->SetActive(false, false);
    }
    task_->Pause();
    MEDIA_LOG_I("pause End");
}

void Downloader::Cancel()
{
    requestQue_->SetActive(false, true);
    if (currentRequest_ != nullptr) {
        currentRequest_->Close();
    }
    if (client_ != nullptr) {
        client_->Close();
    }
    shouldStartNextRequest = true;
    task_->Pause();
}


void Downloader::Resume()
{
    MediaAVCodec::AVCodecTrace trace("Downloader::Resume");
    {
        AutoLock lock(operatorMutex_);
        MEDIA_LOG_I("resume Begin");
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
    requestQue_->SetActive(false);
    if (currentRequest_ != nullptr) {
        currentRequest_->Close();
    }
    if (client_ != nullptr) {
        client_->Close();
    }
    shouldStartNextRequest = true;
    if (isAsync) {
        task_->StopAsync();
    } else {
        task_->Stop();
    }
    MEDIA_LOG_I("Stop End");
}

bool Downloader::Seek(int64_t offset)
{
    AutoLock lock(operatorMutex_);
    FALSE_RETURN_V(currentRequest_ != nullptr, false);
    size_t contentLength = currentRequest_->GetFileContentLength();
    MEDIA_LOG_I("Seek Begin, offset = " PUBLIC_LOG_D64 ", contentLength = " PUBLIC_LOG_ZU, offset, contentLength);
    if (offset >= 0 && offset < static_cast<int64_t>(contentLength)) {
        currentRequest_->startPos_ = offset;
    }
    int64_t temp = currentRequest_->GetFileContentLength() - currentRequest_->startPos_;
    currentRequest_->requestSize_ = static_cast<int>(std::min(temp, static_cast<int64_t>(PER_REQUEST_SIZE)));
    currentRequest_->isEos_ = false;
    shouldStartNextRequest = false; // Reuse last request when seek
    return true;
}

// Pause download thread before use currentRequest_
bool Downloader::Retry(const std::shared_ptr<DownloadRequest>& request)
{
    {
        AutoLock lock(operatorMutex_);
        MEDIA_LOG_I(PUBLIC_LOG_S " Retry Begin, url : " PUBLIC_LOG_S, name_.c_str(), request->url_.c_str());
        FALSE_RETURN_V(client_ != nullptr && !shouldStartNextRequest, false);
        requestQue_->SetActive(false, false);
    }
    task_->Pause();
    {
        AutoLock lock(operatorMutex_);
        FALSE_RETURN_V(client_ != nullptr && !shouldStartNextRequest, false);
        client_->Close();
        if (currentRequest_ != nullptr) {
            if (currentRequest_->IsSame(request) && !shouldStartNextRequest) {
                currentRequest_->retryTimes_++;
                MEDIA_LOG_D("Do retry.");
            }
            client_->Open(currentRequest_->url_, currentRequest_->httpHeader_);
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
    FALSE_RETURN_V(!url.empty(), false);
    if (client_) {
        client_->Close();
        client_->Open(url, httpHeader);
    }

    currentRequest_->requestSize_ = 2; // 2
    currentRequest_->startPos_ = 0;
    currentRequest_->isEos_ = false;
    currentRequest_->retryTimes_ = 0;
    currentRequest_->downloadStartTime_ = currentRequest_->GetNowTime();
    MEDIA_LOG_I("End");
    return true;
}

int64_t Downloader::HttpDownloadLoop()
{
    AutoLock lock(operatorMutex_);
    if (shouldStartNextRequest) {
        std::shared_ptr<DownloadRequest> tempRequest = requestQue_->Pop(1000); // 1000ms超时限制
        if (!tempRequest) {
            MEDIA_LOG_W("HttpDownloadLoop tempRequest is null.");
            return 0;
        }
        currentRequest_ = tempRequest;
        BeginDownload();
        shouldStartNextRequest = false;
    }
    if (currentRequest_ == nullptr) {
        MEDIA_LOG_I("currentRequest is null");
        task_->PauseAsync();
        return -1;
    }
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
    if (ret == Status::OK) {
        HandleRetOK();
    } else {
        task_->PauseAsync();
        MEDIA_LOG_E("Client request data failed. ret = " PUBLIC_LOG_D32 ", clientCode = " PUBLIC_LOG_D32
                    ",request queue size: " PUBLIC_LOG_U64,
                    static_cast<int32_t>(ret), static_cast<int32_t>(clientCode),
                    static_cast<int64_t>(requestQue_->Size()));
        std::shared_ptr<Downloader> unused;
        currentRequest_->statusCallback_(DownloadStatus::PARTTAL_DOWNLOAD, unused, currentRequest_);
    }
    return 0;
}

void Downloader::HandleRetOK()
{
    if (currentRequest_->retryTimes_ > 0) {
        currentRequest_->retryTimes_ = 0;
    }
    int64_t remaining = static_cast<int64_t>(currentRequest_->headerInfo_.fileContentLen) -
        currentRequest_->startPos_;
    if (currentRequest_->headerInfo_.fileContentLen > 0 && remaining <= 0) { // 检查是否播放结束
        MEDIA_LOG_I("http transfer reach end, startPos_ " PUBLIC_LOG_D64 " url: " PUBLIC_LOG_S,
            currentRequest_->startPos_, currentRequest_->url_.c_str());
        currentRequest_->isEos_ = true;
        if (requestQue_->Empty()) {
            task_->PauseAsync();
        }
        shouldStartNextRequest = true;
        if (currentRequest_->downloadDoneCallback_) {
            currentRequest_->downloadDoneTime_ = currentRequest_->GetNowTime();
            currentRequest_->downloadDoneCallback_(currentRequest_->GetUrl(), currentRequest_->location_);
        }
        return;
    }
    if (currentRequest_->headerInfo_.fileContentLen == 0 && remaining <= 0) {
        currentRequest_->isEos_ = true;
        currentRequest_->Close();
        if (requestQue_->Empty()) {
            task_->PauseAsync();
        }
        shouldStartNextRequest = true;
        if (currentRequest_->downloadDoneCallback_) {
            currentRequest_->downloadDoneTime_ = currentRequest_->GetNowTime();
            currentRequest_->downloadDoneCallback_(currentRequest_->GetUrl(), currentRequest_->location_);
        }
        return;
    }
    if (remaining < PER_REQUEST_SIZE) {
        currentRequest_->requestSize_ = remaining;
    } else {
        currentRequest_->requestSize_ = PER_REQUEST_SIZE;
    }
}

size_t Downloader::RxBodyData(void* buffer, size_t size, size_t nitems, void* userParam)
{
    auto mediaDownloader = static_cast<Downloader *>(userParam);
    size_t dataLen = size * nitems;
    int64_t curLen = mediaDownloader->currentRequest_->realRecvContentLen_;
    int64_t realRecvContentLen = static_cast<int64_t>(dataLen) + curLen;
    MediaAVCodec::AVCodecTrace trace("Downloader::RxBodyData, dataLen: " + std::to_string(dataLen)
        + ", realRecvContentLen: " + std::to_string(realRecvContentLen));
    if (mediaDownloader->currentRequest_->IsClosed()) {
        return 0;
    }
    HeaderInfo* header = &(mediaDownloader->currentRequest_->headerInfo_);
    if (!mediaDownloader->currentRequest_->shouldSaveData_) {
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
    MEDIA_LOG_I("RxBodyData: dataLen " PUBLIC_LOG_ZU ", startPos_ " PUBLIC_LOG_D64, dataLen,
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

void Downloader::FLVProcess(bool &isTrunck, long &contentLen, const std::string url)
{
    if (isTrunck != true) {
        if (static_cast<int32_t>(url.find(".flv")) != -1) {
            contentLen = LIVE_CONTENT_LENGTH;
        }
    }
}

size_t Downloader::StrncmpContentRange(HeaderInfo* info, char* key, char* next, size_t size, size_t nitems)
{
    if (!strncmp(key, "Content-Range", strlen("Content-Range")) ||
        !strncmp(key, "content-range", strlen("content-range"))) {
        char* token = strtok_s(nullptr, ":", &next);
        FALSE_RETURN_V(token != nullptr, size * nitems);
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
    return size * nitems;
}

size_t Downloader::RxHeaderData(void* buffer, size_t size, size_t nitems, void* userParam)
{
    MediaAVCodec::AVCodecTrace trace("Downloader::RxHeaderData");
    auto mediaDownloader = reinterpret_cast<Downloader *>(userParam);
    HeaderInfo* info = &(mediaDownloader->currentRequest_->headerInfo_);
    char* next = nullptr;
    char* key = strtok_s(reinterpret_cast<char*>(buffer), ":", &next);
    FALSE_RETURN_V(key != nullptr, size * nitems);
    if (!strncmp(key, "Content-Type", strlen("Content-Type"))) {
        char* token = strtok_s(nullptr, ":", &next);
        FALSE_RETURN_V(token != nullptr, size * nitems);
        char* type = StringTrim(token);
        NZERO_LOG(memcpy_s(info->contentType, sizeof(info->contentType), type, strlen(type)));
    }

    if (!strncmp(key, "Content-Length", strlen("Content-Length")) ||
        !strncmp(key, "content-length", strlen("content-length"))) {
        char* token = strtok_s(nullptr, ":", &next);
        FALSE_RETURN_V(token != nullptr, size * nitems);
        info->contentLen = atol(StringTrim(token));
        if (info->contentLen <= 0) {
            FLVProcess(info->isChunked, info->contentLen, mediaDownloader->currentRequest_->url_);
        }
    }

    if (!strncmp(key, "Transfer-Encoding", strlen("Transfer-Encoding")) ||
        !strncmp(key, "transfer-encoding", strlen("transfer-encoding"))) {
        char* token = strtok_s(nullptr, ":", &next);
        FALSE_RETURN_V(token != nullptr, size * nitems);
        if (!strncmp(StringTrim(token), "chunked", strlen("chunked"))) {
            info->isChunked = true;
        }
    }

    if (!strncmp(key, "Location", strlen("Location")) ||
        !strncmp(key, "location", strlen("location"))) {
        FALSE_RETURN_V(next != nullptr, size * nitems);
        char* location = StringTrim(next);
        mediaDownloader->currentRequest_->location_ = location;
    }

    StrncmpContentRange(info, key, next, size, nitems);

    if (info->contentLen <= 0) {
        FLVProcess(info->isChunked, info->contentLen, mediaDownloader->currentRequest_->url_);
    } else {
        info->isChunked = false;
    }

    mediaDownloader->currentRequest_->SaveHeader(info);
    return size * nitems;
}
}
}
}
}