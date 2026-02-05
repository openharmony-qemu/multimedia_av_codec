/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

#include "downloader.h"
#include "avcodec_trace.h"
#include "securec.h"
#include "plugin/plugin_time.h"
#include "syspara/parameter.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
namespace {
constexpr int32_t PER_REQUEST_SIZE = 2 * 1024 * 1024;
constexpr int32_t BITRATE_REQUEST_SIZE = 4;
constexpr unsigned int SLEEP_TIME = 5;    // Sleep 5ms
constexpr size_t RETRY_TIMES = 6000;  // Retry 6000 times
constexpr size_t REQUEST_QUEUE_SIZE = 50;
constexpr long LIVE_CONTENT_LENGTH = 2147483646;
constexpr int32_t DOWNLOAD_LOG_FEQUENCE = 10;
constexpr int32_t LOOP_TIMES = 5;
constexpr int32_t MAX_LEN = 128;
const std::string USER_AGENT = "User-Agent";
const std::string DISPLAYVERSION = "const.product.software.version";
constexpr int FIRST_REQUEST_SIZE = 8 * 1024;
constexpr int MIN_REQUEST_SIZE = 2;
constexpr int SERVER_RANGE_ERROR_CODE = 416;
constexpr int32_t LOOP_LOG_FEQUENCE = 50;
constexpr int REQUEST_OFTEN_ERROR_CODE = 500;
constexpr int SLEEP_TEN_MICRO_SEC = 10; // 10ms
constexpr int APP_OPEN_RETRY_TIMES = 10;
constexpr int32_t REDIRECT_CODE = 302;
constexpr uint32_t MAX_LOOP_TIMES = 100;
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

DownloadRequest::DownloadRequest(DataSaveFunc saveData, StatusCallbackFunc statusCallback, RequestInfo requestInfo,
                                 bool requestWholeFile)
    : saveData_(std::move(saveData)), statusCallback_(std::move(statusCallback)), requestInfo_(requestInfo),
    requestWholeFile_(requestWholeFile)
{
    (void)memset_s(&headerInfo_, sizeof(HeaderInfo), 0x00, sizeof(HeaderInfo));
    headerInfo_.fileContentLen = 0;
    headerInfo_.contentLen = 0;
    url_ = requestInfo.url;
    httpHeader_ = requestInfo.httpHeader;
}

DownloadRequest::DownloadRequest(double duration,
                                 DataSaveFunc saveData,
                                 StatusCallbackFunc statusCallback,
                                 RequestInfo requestInfo,
                                 bool requestWholeFile)
    : duration_(duration), saveData_(std::move(saveData)), statusCallback_(std::move(statusCallback)),
    requestInfo_(requestInfo), requestWholeFile_(requestWholeFile)
{
    (void)memset_s(&headerInfo_, sizeof(HeaderInfo), 0x00, sizeof(HeaderInfo));
    headerInfo_.fileContentLen = 0;
    headerInfo_.contentLen = 0;
    url_ = requestInfo.url;
    httpHeader_ = requestInfo.httpHeader;
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
    isHeaderUpdated_ = true;
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

int32_t DownloadRequest::GetClientError() const
{
    return clientError_;
}

int32_t DownloadRequest::GetServerError() const
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
    isHeaderUpdating_ = true;
    MediaAVCodec::AVCodecTrace trace("DownloadRequest::WaitHeaderUpdated");
    // Wait Header(fileContentLen etc.) updated
    while (!isHeaderUpdated_ && times_ < RETRY_TIMES && !isInterruptNeeded_ && !headerInfo_.isClosed) {
        Task::SleepInTask(SLEEP_TIME);
        times_++;
    }
    MEDIA_LOG_D("isHeaderUpdated_ " PUBLIC_LOG_D32 ", times " PUBLIC_LOG_ZU ", isClosed " PUBLIC_LOG_D32,
        isHeaderUpdated_.load(), times_.load(), headerInfo_.isClosed.load());
    isHeaderUpdating_ = false;
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
    return protocolType_ == RequestProtocolType::HLS;
}

void DownloadRequest::SetRequestProtocolType(RequestProtocolType protocolType)
{
    protocolType_ = protocolType;
}

bool DownloadRequest::IsServerAcceptRange() const
{
    if (headerInfo_.isChunked) {
        return false;
    }
    return headerInfo_.isServerAcceptRange;
}

DownloadRequest::~DownloadRequest()
{
    MEDIA_LOG_D("~DownloadRequest dtor in.");
    int sleepTmpTime = 0;
    while (isHeaderUpdating_.load() && sleepTmpTime < RETRY_TIMES) {
        Task::SleepInTask(SLEEP_TIME);
        sleepTmpTime++;
    }
}

void DownloadRequest::GetLocation(std::string& location) const
{
    location = location_;
}

void DownloadRequest::SetBitRateToRequestSize(const int32_t videoBitrate)
{
    bitRateToRequestSize_ = videoBitrate / BITRATE_REQUEST_SIZE;
}

Downloader::Downloader(const std::string& name) noexcept : name_(std::move(name))
{
    shouldStartNextRequest_ = true;

    client_ = NetworkClient::GetInstance(&RxHeaderData, &RxBodyData, this);
}
 
void Downloader::Init()
{
    client_->Init();
    requestQue_ = std::make_shared<BlockingQueue<std::shared_ptr<DownloadRequest>>>(name_ + "RequestQue",
        REQUEST_QUEUE_SIZE);
    task_ = std::make_shared<Task>(std::string("OS_" + name_ + "Downloader"));
    auto weakDownloader = weak_from_this();
    task_->RegisterJob([weakDownloader] {
        auto shareDownloader = weakDownloader.lock();
        FALSE_RETURN_V_MSG(shareDownloader != nullptr, 0, "downloader is destructed");
        {
            AutoLock lk(shareDownloader->loopPauseMutex_);
            if (shareDownloader->loopStatus_ == LoopStatus::PAUSE) {
                MEDIA_LOG_I("0x%{public}06" PRIXPTR " loopStatus PAUSE to START", FAKE_POINTER(shareDownloader.get()));
            }
            shareDownloader->loopStatus_ = LoopStatus::START;
        }
        shareDownloader->HttpDownloadLoop();
        shareDownloader->NotifyLoopPause();
        return 0;
    });
    MEDIA_LOG_I("0x%{public}06" PRIXPTR " Downloader ctor", FAKE_POINTER(this));
}

Downloader::Downloader(const std::string& name, std::shared_ptr<MediaSourceLoaderCombinations> sourceLoader) noexcept
{
    name_ = name;
    shouldStartNextRequest_ = true;
    if (sourceLoader != nullptr) {
        isNotBlock_ = true;
        sourceLoader_ = sourceLoader;
        client_ = NetworkClient::GetAppInstance(&RxHeaderData, &RxBodyData, this);
        client_->SetLoader(sourceLoader->loader_);
        MEDIA_LOG_I("0x%{public}06" PRIXPTR "Get app instance success", FAKE_POINTER(this));
    } else {
        MEDIA_LOG_I("0x%{public}06" PRIXPTR "Get libcurl instance success", FAKE_POINTER(this));
        client_ = NetworkClient::GetInstance(&RxHeaderData, &RxBodyData, this);
    }
}

Downloader::~Downloader()
{
    isDestructor_ = true;
    MEDIA_LOG_I("~Downloader In");
    {
        AutoLock lock(closeMutex_);
        if (sourceLoader_ != nullptr) {
            sourceLoader_->Close(sourceId_);
            sourceLoader_ = nullptr;
        }
    }
    Stop(false);
    if (task_ != nullptr) {
        task_ = nullptr;
    }
    if (client_ != nullptr) {
        client_->Deinit();
        client_ = nullptr;
    }
    MEDIA_LOG_I("0x%{public}06" PRIXPTR " ~Downloader dtor", FAKE_POINTER(this));
}

bool Downloader::Download(const std::shared_ptr<DownloadRequest>& request, int32_t waitMs)
{
    MEDIA_LOG_I("In");
    if (isInterruptNeeded_) {
        request->isInterruptNeeded_ = true;
    }
    requestQue_->SetActive(true);
    if (waitMs == -1) { // wait until push success
        requestQue_->Push(request);
        return true;
    }
    return requestQue_->Push(request, static_cast<int>(waitMs));
}

std::string Downloader::GetContentType()
{
    if (isDestructor_) {
        MEDIA_LOG_E("Get " PUBLIC_LOG_S " content type failed, sourceId " PUBLIC_LOG_D64, name_.c_str(), sourceId_);
        return contentType_;
    }
    FALSE_RETURN_V_NOLOG(!isContentTypeUpdated_, contentType_);
    AutoLock lock(sleepMutex_);
    MEDIA_LOG_I("GetContentType wait begin ");
    sleepCond_.WaitFor(lock, SLEEP_TIME * RETRY_TIMES, [this]() {
        return isInterruptNeeded_.load() || isContentTypeUpdated_;
    });
    MEDIA_LOG_I("ContentType: %{public}s", contentType_.c_str());
    return contentType_;
}

void Downloader::Start()
{
    MediaAVCodec::AVCodecTrace trace("Downloader::Start");
    MEDIA_LOG_I("start Begin");
    requestQue_->SetActive(true);
    task_->Start();
    MEDIA_LOG_I("start End");
}

void Downloader::ReStart()
{
    MEDIA_LOG_I("Downloader::ReStart");
    shouldStartNextRequest_ = true;
}

void Downloader::Pause(bool isAsync)
{
    MediaAVCodec::AVCodecTrace trace("Downloader::Pause");
    MEDIA_LOG_I("0x%{public}06" PRIXPTR " Pause Begin", FAKE_POINTER(this));
    requestQue_->SetActive(false, false);
    if (client_ != nullptr) {
        isClientClose_ = true;
        client_->Close(isAsync);
    }
    PauseLoop(true);
    if (!isAsync) {
        WaitLoopPause();
    }
    MEDIA_LOG_I("0x%{public}06" PRIXPTR " Pause End", FAKE_POINTER(this));
}

void Downloader::Cancel()
{
    MEDIA_LOG_I("0x%{public}06" PRIXPTR " Cancel Begin", FAKE_POINTER(this));
    if (currentRequest_ != nullptr && currentRequest_->retryTimes_ > 0) {
        currentRequest_->retryTimes_ = 0;
    }
    requestQue_->SetActive(false, true);
    shouldStartNextRequest_ = true;
    if (client_ != nullptr) {
        client_->Close(false);
    }
    PauseLoop(true);
    WaitLoopPause();
    MEDIA_LOG_I("0x%{public}06" PRIXPTR " Cancel End", FAKE_POINTER(this));
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
            client_->Open(currentRequest_->url_, currentRequest_->httpHeader_, currentRequest_->requestInfo_.timeoutMs);
        }
        requestQue_->SetActive(true);
        if (currentRequest_ != nullptr) {
            currentRequest_->isEos_ = false;

            int64_t fileLength = static_cast<int64_t>(currentRequest_->GetFileContentLength());
            if (currentRequest_->startPos_ + currentRequest_->requestSize_ > fileLength) {
                int correctRequestSize = fileLength - currentRequest_->startPos_;
                MEDIA_LOG_E("resume error startPos = " PUBLIC_LOG_D64 ", requestSize = " PUBLIC_LOG_D32
                    ", fileLength = " PUBLIC_LOG_D64 ", correct requestSize = " PUBLIC_LOG_D32,
                    currentRequest_->startPos_, currentRequest_->requestSize_, fileLength, correctRequestSize);
                if (currentRequest_->startPos_ < fileLength) {
                    currentRequest_->requestSize_ = correctRequestSize;
                }
            }
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
    shouldStartNextRequest_ = true;
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
    FALSE_RETURN_V_MSG(!isDestructor_ && !isInterruptNeeded_, false, "Seek fail, is Destructor or InterruptNeeded");
    AutoLock lock(operatorMutex_);
    FALSE_RETURN_V_MSG(currentRequest_ != nullptr, false, "Seek fail, currentRequest nullptr");
    size_t contentLength = currentRequest_->GetFileContentLength();
    MEDIA_LOG_I("Seek Begin, offset = " PUBLIC_LOG_D64 ", contentLength = " PUBLIC_LOG_ZU, offset, contentLength);
    if (offset >= 0 && offset < static_cast<int64_t>(contentLength)) {
        currentRequest_->startPos_ = offset;
    }
    size_t temp = currentRequest_->GetFileContentLength() - static_cast<size_t>(currentRequest_->startPos_);
    currentRequest_->requestSize_ = static_cast<int>(std::min(temp,
        static_cast<size_t>(std::max(currentRequest_->bitRateToRequestSize_, PER_REQUEST_SIZE))));
    if (downloadRequestSize_ > 0) {
        currentRequest_->requestSize_ = std::min(currentRequest_->requestSize_,
            static_cast<int>(downloadRequestSize_));
        downloadRequestSize_ = 0;
    }
    currentRequest_->isEos_ = false;
    shouldStartNextRequest_ = false; // Reuse last request when seek
    if (currentRequest_->retryTimes_ > 0) {
        currentRequest_->retryTimes_ = 0;
    }
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
    if (isAppBackground_) {
        Pause(true);
        MEDIA_LOG_I("Retry avoid, forground to background.");
        return true;
    }
    {
        AutoLock lock(operatorMutex_);
        MEDIA_LOG_I("0x%{public}06" PRIXPTR " Retry Begin", FAKE_POINTER(this));
        FALSE_RETURN_V(client_ != nullptr && !shouldStartNextRequest_ && !isDestructor_ && !isInterruptNeeded_, false);
        requestQue_->SetActive(false, false);
    }
    PauseLoop(true);
    WaitLoopPause();
    {
        AutoLock lock(operatorMutex_);
        FALSE_RETURN_V(client_ != nullptr && !shouldStartNextRequest_ && !isDestructor_ && !isInterruptNeeded_, false);
        client_->Close(false);
        if (currentRequest_ != nullptr) {
            if (currentRequest_->IsSame(request) && !shouldStartNextRequest_) {
                currentRequest_->retryTimes_++;
                currentRequest_->retryOnGoing_ = true;
                currentRequest_->dropedDataLen_ = 0;
            }
            client_->Open(currentRequest_->url_, currentRequest_->httpHeader_, currentRequest_->requestInfo_.timeoutMs);
            requestQue_->SetActive(true);
            currentRequest_->isEos_ = false;
            if (currentRequest_->endPos_ > 0 && currentRequest_->startPos_ >= 0 &&
                currentRequest_->endPos_ >= currentRequest_->startPos_) {
                currentRequest_->requestSize_ = currentRequest_->endPos_ - currentRequest_->startPos_ + 1;
            }
        }
    }
    task_->Start();
    MEDIA_LOG_I("Do retry.");
    return true;
}

std::string GetSystemParam(const std::string &key)
{
    char value[MAX_LEN] = {0};
    int32_t ret = GetParameter(key.c_str(), "", value, MAX_LEN);
    if (ret < 0) {
        return "";
    }
    return std::string(value);
}

std::string GetUserAgent()
{
    std::string displayVersion = GetSystemParam(DISPLAYVERSION);
    std::string userAgent = " AVPlayerLib " + displayVersion;
    return userAgent;
}

bool Downloader::BeginDownload()
{
    MEDIA_LOG_I("BeginDownload %{public}s", name_.c_str());
    std::string url = currentRequest_->url_;
    std::map<std::string, std::string> httpHeader = currentRequest_->httpHeader_;

    if (currentRequest_->httpHeader_.count(USER_AGENT) <= 0) {
        currentRequest_->httpHeader_[USER_AGENT] = GetUserAgent();
        httpHeader[USER_AGENT] = GetUserAgent();
        MEDIA_LOG_I("Set default UA.");
    }
    
    int32_t timeoutMs = currentRequest_->requestInfo_.timeoutMs;
    FALSE_RETURN_V(!url.empty(), false);
    if (client_) {
        client_->Open(url, httpHeader, timeoutMs);
    }

    if (currentRequest_->endPos_ <= 0) {
        currentRequest_->startPos_ = 0;
        currentRequest_->requestSize_ = FIRST_REQUEST_SIZE;
    } else {
        int64_t temp = currentRequest_->endPos_ - currentRequest_->startPos_ + 1;
        currentRequest_->requestSize_ = static_cast<int>(std::min(temp,
            static_cast<int64_t>(std::max(currentRequest_->bitRateToRequestSize_, PER_REQUEST_SIZE))));
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
        shouldStartNextRequest_.load());
    if (shouldStartNextRequest_) {
        std::shared_ptr<DownloadRequest> tempRequest = requestQue_->Pop(1000); // 1000ms timeout limit.
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
        if (isInterruptNeeded_) {
            currentRequest_->isInterruptNeeded_ = true;
        }
        BeginDownload();
        shouldStartNextRequest_ = currentRequest_->IsClosed();
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

void Downloader::OpenAppUri()
{
    {
        AutoLock lock(closeMutex_);
        if (currentRequest_ != nullptr) {
            appPreviousRequestUrl_ = currentRequest_->GetUrl();
        }
        if (sourceLoader_ != nullptr && currentRequest_ != nullptr) {
            if (sourceId_ != 0) {
                sourceLoader_->Close(sourceId_);
                MEDIA_LOG_D("0x%{public}06" PRIXPTR "LoaderCombinations Close uuid " PUBLIC_LOG_D64,
                    FAKE_POINTER(this), sourceId_);
            }

            int64_t uuid = 0;
            for (int i = 0; i < APP_OPEN_RETRY_TIMES; i++) {
                uuid = sourceLoader_->Open(appPreviousRequestUrl_, currentRequest_->GetHttpHeader(), client_);
                if (uuid > 0) {
                    break;
                }
                MEDIA_LOG_D("0x%{public}06" PRIXPTR "Open retry " PUBLIC_LOG_D64 " retryTimes: " PUBLIC_LOG_D32,
                FAKE_POINTER(this), uuid, i);
            }
            MEDIA_LOG_I("0x%{public}06" PRIXPTR "LoaderCombinations Open uuid " PUBLIC_LOG_D64,
                FAKE_POINTER(this), uuid);
            if (uuid != 0) {
                client_->SetUuid(uuid);
                sourceId_ = uuid;
            } else {
                MEDIA_LOG_E("0x%{public}06" PRIXPTR "Open faild, uuid " PUBLIC_LOG_D64,
                FAKE_POINTER(this), uuid);
                std::shared_ptr<Downloader> unused;
                currentRequest_->statusCallback_(DownloadStatus::PARTTAL_DOWNLOAD, unused, currentRequest_);
            }
        }
    }
}

void Downloader::HandleRedirect(Status& ret)
{
    if (!currentRequest_->location_.empty() && !currentRequest_->haveRedirectRetry_.load()) {
        currentRequest_->clientError_ = 0;
        currentRequest_->serverError_ = REDIRECT_CODE;
        ret = Status::ERROR_SERVER;
        currentRequest_->url_ = currentRequest_->location_;
        currentRequest_->haveRedirectRetry_.store(true);
        currentRequest_->headerInfo_.contentLen = 0;
    }
}

void Downloader::HandleResponseCb(int32_t clientCode, int32_t serverCode, Status& ret)
{
    currentRequest_->clientError_ = clientCode;
    currentRequest_->serverError_ = serverCode;

    HandleRedirect(ret);

    if (isDestructor_) {
        return;
    }
    if (currentRequest_->preRequestSize_ == FIRST_REQUEST_SIZE && !currentRequest_->isFirstRangeRequestReady_
        && currentRequest_->serverError_ == SERVER_RANGE_ERROR_CODE) {
        MEDIA_LOG_I("first request is above filesize, need retry.");
        currentRequest_->startPos_ = 0;
        currentRequest_->requestSize_ = MIN_REQUEST_SIZE;
        currentRequest_->isHeaderUpdated_ = false;
        currentRequest_->isFirstRangeRequestReady_ = true;
        currentRequest_->headerInfo_.fileContentLen = 0;
        return;
    }
    if (ret == Status::OK) {
        MEDIA_LOG_I("Client request data OK. ret = " PUBLIC_LOG_D32 ", clientCode = " PUBLIC_LOG_D32
            ", request queue size: " PUBLIC_LOG_U64, static_cast<int32_t>(ret),
            static_cast<int32_t>(clientCode), static_cast<int64_t>(requestQue_->Size()));
        HandleRetOK();
    } else {
        PauseLoop(true);
        MEDIA_LOG_E("Client request data failed. ret = " PUBLIC_LOG_D32 ", clientCode = " PUBLIC_LOG_D32
            ", request queue size: " PUBLIC_LOG_U64, static_cast<int32_t>(ret),
            static_cast<int32_t>(clientCode), static_cast<int64_t>(requestQue_->Size()));
        HandleRetErrorCode();
        std::shared_ptr<Downloader> unused;
        currentRequest_->statusCallback_(DownloadStatus::PARTTAL_DOWNLOAD, unused, currentRequest_);
    }
}

void Downloader::RequestData()
{
    MediaAVCodec::AVCodecTrace trace("Downloader::HttpDownloadLoop, startPos: "
        + std::to_string(currentRequest_->startPos_) + ", reqSize: " + std::to_string(currentRequest_->requestSize_));
    int64_t startPos = currentRequest_->startPos_;
    if (currentRequest_->requestWholeFile_ && currentRequest_->shouldSaveData_) {
        startPos = -1;
    }
    RequestInfo sourceInfo;
    sourceInfo.url = currentRequest_->url_;
    sourceInfo.httpHeader = currentRequest_->httpHeader_;
    sourceInfo.timeoutMs = currentRequest_->requestInfo_.timeoutMs;

    if ((currentRequest_->protocolType_ == RequestProtocolType::HTTP && sourceId_ == 0) ||
        currentRequest_->protocolType_ == RequestProtocolType::HLS ||
        currentRequest_->protocolType_ == RequestProtocolType::DASH) {
        OpenAppUri();
    }

    std::weak_ptr<Downloader> weakDownloader = weak_from_this();
    auto handleResponseCb = [weakDownloader](int32_t clientCode, int32_t serverCode, Status ret) {
        auto shareDownloader = weakDownloader.lock();
        FALSE_RETURN_MSG(shareDownloader != nullptr, "handleResponseCb, Downloader is nullptr!");
        shareDownloader->HandleResponseCb(clientCode, serverCode, ret);
    };
    MEDIA_LOG_I("0x%{public}06" PRIXPTR " RequestData enter.", FAKE_POINTER(this));

    int len = currentRequest_->requestSize_;
    if (currentRequest_->requestWholeFile_) {
        len = 0;
    }
    if (!isFirstDownload_) {
        startDownTime_ = GetCurrentMillisecond();
        isFirstDownload_ = true;
    }
    if (downloadCallback_ != nullptr) {
        downloadCallback_->UpdateTotalDownloadCount();
    }
    client_->RequestData(startPos, len, sourceInfo, handleResponseCb);
    MEDIA_LOG_I("0x%{public}06" PRIXPTR " RequestData end.", FAKE_POINTER(this));
}

void Downloader::HandleRetErrorCode()
{
    if (currentRequest_ && currentRequest_->serverError_ == REQUEST_OFTEN_ERROR_CODE) {
        int sleepTime = 0;
        while (!isInterruptNeeded_ && sleepTime <= 1000) { // 1000:sleep 1s
            Task::SleepInTask(SLEEP_TEN_MICRO_SEC);
            sleepTime += SLEEP_TEN_MICRO_SEC;
        }
    }
}
void Downloader::HandlePlayingFinish()
{
    if (requestQue_->Empty()) {
        PauseLoop(true);
    }
    shouldStartNextRequest_ = true;
    if (currentRequest_->downloadDoneCallback_ && !isDestructor_) {
        currentRequest_->downloadDoneTime_ = currentRequest_->GetNowTime();
        currentRequest_->downloadDoneCallback_(currentRequest_->GetUrl(), currentRequest_->location_);
        currentRequest_->isFirstRangeRequestReady_ = false;
    }
}

void Downloader::HandleRetOK()
{
    if (currentRequest_ == nullptr) {
        return;
    }
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

        remaining = currentRequest_->headerInfo_.fileContentLen > currentRequest_->startPos_?
            static_cast<int64_t>(currentRequest_->headerInfo_.fileContentLen) - currentRequest_->startPos_ : 0;
    } else {
        remaining = currentRequest_->endPos_ - currentRequest_->startPos_ + 1;
    }
    if (currentRequest_->headerInfo_.fileContentLen > 0 && remaining <= 1) { // Check whether the playback ends.
        MEDIA_LOG_I("http transfer reach end, startPos_ " PUBLIC_LOG_D64, currentRequest_->startPos_);
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
    currentRequest_->requestSize_ = static_cast<int>(std::min(remaining,
        static_cast<int64_t>(std::max(currentRequest_->bitRateToRequestSize_, PER_REQUEST_SIZE))));
}

void Downloader::UpdateHeaderInfo(Downloader* mediaDownloader)
{
    if (mediaDownloader->currentRequest_->isHeaderUpdated_) {
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
    if (info->fileContentLen > 0 && info->isChunked && !mediaDownloader->currentRequest_->IsM3u8Request()) {
        info->isChunked = false;
    }
    mediaDownloader->currentRequest_->SaveHeader(info);
    if (!mediaDownloader->isContentTypeUpdated_) {
        {
            AutoLock lock(mediaDownloader->sleepMutex_);
            mediaDownloader->isContentTypeUpdated_ = true;
            mediaDownloader->contentType_ = info->contentType;
        }
        mediaDownloader->sleepCond_.NotifyOne();
    }
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
    auto currentRequest = mediaDownloader->currentRequest_;
    int64_t needDropLen = currentRequest->startPos_ - currentRequest->dropedDataLen_;
    int64_t writeOffSet = -1;
    if (needDropLen > 0) {
        writeOffSet = needDropLen >= static_cast<int64_t>(dataLen) ? 0 : needDropLen; // 0:drop all
    }
    bool dropRet = false;
    uint32_t writeLen = 0;
    if (writeOffSet > 0) {
        int64_t secondParam = static_cast<int64_t>(dataLen) - writeOffSet;
        if (secondParam < 0) {
            secondParam = 0;
        }
        writeLen = currentRequest->saveData_(static_cast<uint8_t *>(buffer) + writeOffSet,
            static_cast<uint32_t>(secondParam), mediaDownloader->isNotBlock_);
        dropRet = writeLen == secondParam;
        currentRequest->dropedDataLen_ = currentRequest->dropedDataLen_ + writeOffSet;
        MEDIA_LOG_D("DropRetryData: last drop, droped len " PUBLIC_LOG_D64 ", startPos_ " PUBLIC_LOG_D64,
                    currentRequest->dropedDataLen_, currentRequest->startPos_);
    } else if (writeOffSet == 0) {
        currentRequest->dropedDataLen_ += static_cast<int64_t>(dataLen);
        dropRet = true;
        MEDIA_LOG_D("DropRetryData: drop, droped len " PUBLIC_LOG_D64 ", startPos_ " PUBLIC_LOG_D64,
                    currentRequest->dropedDataLen_, currentRequest->startPos_);
    } else {
        MEDIA_LOG_E("drop data error");
    }
    if (dropRet && currentRequest->startPos_ == currentRequest->dropedDataLen_) {
        currentRequest->retryOnGoing_ = false;
        currentRequest->dropedDataLen_ = 0;
        if (writeOffSet > 0) {
            currentRequest->startPos_ += static_cast<int64_t>(dataLen) - writeOffSet;
        }
        MEDIA_LOG_I("drop data finished, startPos_ " PUBLIC_LOG_D64, currentRequest->startPos_);
    }
    if (!dropRet && mediaDownloader->sourceLoader_ != nullptr) {
        mediaDownloader->currentRequest_->startPos_ += static_cast<int64_t>(writeLen);
        mediaDownloader->PauseLoop(true);
        mediaDownloader->currentRequest_->retryOnGoing_ = true;
        mediaDownloader->currentRequest_->dropedDataLen_ = 0;
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
    mediaDownloader->currentRequest_->requestSize_ =
        static_cast<int>(std::max(mediaDownloader->currentRequest_->bitRateToRequestSize_, PER_REQUEST_SIZE));
    mediaDownloader->currentRequest_->startTimePos_ = 0;
}

void Downloader::HandleFileContentLen(HeaderInfo* header)
{
    if (header->fileContentLen == 0) {
        if (header->contentLen > 0) {
            MEDIA_LOG_W("Unsupported range, use content length as content file length, contentLen: " PUBLIC_LOG_D32,
                static_cast<int32_t>(header->contentLen));
            header->fileContentLen = static_cast<size_t>(header->contentLen);
        } else {
            MEDIA_LOG_D("fileContentLen and contentLen are both zero.");
        }
    }
}

void Downloader::UpdateRequestSize(Downloader* mediaDownloader)
{
    int64_t remaining = 0;
    if (mediaDownloader->currentRequest_->endPos_ <= 0) {
        remaining = static_cast<int64_t>(mediaDownloader->currentRequest_->headerInfo_.fileContentLen) -
                    mediaDownloader->currentRequest_->startPos_;
    } else {
        remaining = mediaDownloader->currentRequest_->endPos_ - mediaDownloader->currentRequest_->startPos_ + 1;
    }

    mediaDownloader->currentRequest_->preRequestSize_ = mediaDownloader->currentRequest_->requestSize_;
    mediaDownloader->currentRequest_->requestSize_ = static_cast<int>(std::min(remaining,
        static_cast<int64_t>(std::max(mediaDownloader->currentRequest_->bitRateToRequestSize_, PER_REQUEST_SIZE))));
}

size_t Downloader::RxBodyData(void* buffer, size_t size, size_t nitems, void* userParam)
{
    auto mediaDownloader = static_cast<Downloader *>(userParam);
    size_t dataLen = size * nitems;
    int64_t curLen = mediaDownloader->currentRequest_->realRecvContentLen_;
    int64_t realRecvContentLen = static_cast<int64_t>(dataLen) + curLen;

    if (!mediaDownloader->currentRequest_->location_.empty() &&
        !mediaDownloader->currentRequest_->haveRedirectRetry_.load()) {
        MEDIA_LOG_W("Receive location, need retry.");
        return 0;
    }

    UpdateHeaderInfo(mediaDownloader);
    MediaAVCodec::AVCodecTrace trace("Downloader::RxBodyData, dataLen: " + std::to_string(dataLen)
        + ", realRecvContentLen: " + std::to_string(realRecvContentLen));
    mediaDownloader->currentRequest_->realRecvContentLen_ = realRecvContentLen;
 
    if (IsDropDataRetryRequest(mediaDownloader) && !mediaDownloader->currentRequest_->IsIndexM3u8Request()) {
        return DropRetryData(buffer, dataLen, mediaDownloader);
    }
    HeaderInfo* header = &(mediaDownloader->currentRequest_->headerInfo_);
    if (!mediaDownloader->currentRequest_->shouldSaveData_) {
        UpdateCurRequest(mediaDownloader, header);
        return dataLen;
    }
    HandleFileContentLen(header);
    if (!mediaDownloader->currentRequest_->isDownloading_) {
        mediaDownloader->currentRequest_->isDownloading_ = true;
    }
    UpdateDownloadInfo(mediaDownloader, dataLen);
    uint32_t writeLen = mediaDownloader->currentRequest_->saveData_(static_cast<uint8_t *>(buffer),
        static_cast<uint32_t>(dataLen), mediaDownloader->isNotBlock_);
    MEDIA_LOGI_LIMIT(DOWNLOAD_LOG_FEQUENCE, "RxBodyData: dataLen " PUBLIC_LOG_ZU ", startPos_ " PUBLIC_LOG_D64, dataLen,
                     mediaDownloader->currentRequest_->startPos_);
    mediaDownloader->currentRequest_->startPos_ = mediaDownloader->currentRequest_->startPos_ +
                                                  static_cast<int64_t>(writeLen);
    UpdateRequestSize(mediaDownloader);
 
    if (writeLen != dataLen) {
        if (mediaDownloader->sourceLoader_ != nullptr) {
            mediaDownloader->PauseLoop(true);
            mediaDownloader->currentRequest_->retryOnGoing_ = true;
            mediaDownloader->currentRequest_->dropedDataLen_ = 0;
        }
        MEDIA_LOG_W("Save data failed.");
        return 0; // save data failed, make perform finished.
    }
 
    mediaDownloader->currentRequest_->isDownloading_ = false;
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
    if (!strncmp(key, "content-range", strlen("content-range"))) {
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

bool Downloader::HandleContentType(HeaderInfo* info, char* key, char* next, size_t headerSize,
                                   Downloader* mediaDownloader)
{
    if (!strncmp(key, "content-type", strlen("content-type"))) {
        char* token = strtok_s(nullptr, ":", &next);
        FALSE_RETURN_V(token != nullptr, false);
        char* type = StringTrim(token);
        std::string headStr = (std::string)token;
        MEDIA_LOG_I("content-type: " PUBLIC_LOG_S, headStr.c_str());
        NZERO_LOG(memcpy_s(info->contentType, sizeof(info->contentType), type, strlen(type)));
    }
    return true;
}

bool Downloader::HandleContentEncode(HeaderInfo* info, char* key, char* next, size_t size, size_t nitems)
{
    if (!strncmp(key, "content-encode", strlen("content-encode"))) {
        char* token = strtok_s(nullptr, ":", &next);
        FALSE_RETURN_V(token != nullptr, false);
        std::string headStr = (std::string)token;
        MEDIA_LOG_I("content-encode: " PUBLIC_LOG_S, headStr.c_str());
    }
    return true;
}

bool Downloader::HandleContentLength(HeaderInfo* info, char* key, char* next, Downloader* mediaDownloader)
{
    FALSE_RETURN_V(key != nullptr, false);
    if (!strncmp(key, "content-length", strlen("content-length"))) {
        FALSE_RETURN_V(next != nullptr, false);
        char* token = strtok_s(nullptr, ":", &next);
        FALSE_RETURN_V(token != nullptr, false);
        if (info != nullptr && mediaDownloader != nullptr) {
            info->contentLen = atol(StringTrim(token));
            MEDIA_LOG_I("content-length: " PUBLIC_LOG_D32, static_cast<int32_t>(info->contentLen));
            if (info->contentLen <= 0 && !mediaDownloader->currentRequest_->IsM3u8Request()) {
                info->isChunked = true;
            }
        }
    }
    return true;
}

// Check if this server supports range download. (HTTP)
bool Downloader::HandleRange(HeaderInfo* info, char* key, char* next, size_t size, size_t nitems)
{
    if (!strncmp(key, "accept-ranges", strlen("accept-ranges"))) {
        char* token = strtok_s(nullptr, ":", &next);
        FALSE_RETURN_V(token != nullptr, false);
        if (!strncmp(StringTrim(token), "bytes", strlen("bytes"))) {
            info->isServerAcceptRange = true;
            MEDIA_LOG_I("accept-ranges: bytes");
        }
    }
    if (!strncmp(key, "content-range", strlen("content-range"))) {
        char* token = strtok_s(nullptr, ":", &next);
        FALSE_RETURN_V(token != nullptr, false);
        std::string headStr = (std::string)token;
        if (headStr.find("bytes") != std::string::npos) {
            info->isServerAcceptRange = true;
            MEDIA_LOG_I("content-range: " PUBLIC_LOG_S, headStr.c_str());
        }
    }
    return true;
}

void Downloader::ToLower(char* str)
{
    FALSE_RETURN(str != nullptr);
    for (int i = 0; str[i] != '\0'; ++i) {
        if (i > MAX_LOOP_TIMES) {
            break;
        }
        if (str[i] >= 'A' && str[i] <= 'Z') {
            str[i] = tolower(static_cast<unsigned char>(str[i]));
        }
    }
}

size_t Downloader::RxHeaderData(void* buffer, size_t size, size_t nitems, void* userParam)
{
    MediaAVCodec::AVCodecTrace trace("Downloader::RxHeaderData");
    auto mediaDownloader = static_cast<Downloader *>(userParam);
    HeaderInfo* info = &(mediaDownloader->currentRequest_->headerInfo_);
    if (mediaDownloader->currentRequest_->isHeaderUpdated_) {
        return size * nitems;
    }
    char* next = nullptr;
    char* key = strtok_s(reinterpret_cast<char*>(buffer), ":", &next);
    FALSE_RETURN_V(key != nullptr, size * nitems);
    ToLower(key);

    if (!strncmp(key, "transfer-encoding", strlen("transfer-encoding"))) {
        char* token = strtok_s(nullptr, ":", &next);
        FALSE_RETURN_V(token != nullptr, size * nitems);
        if (!strncmp(StringTrim(token), "chunked", strlen("chunked")) &&
            !mediaDownloader->currentRequest_->IsM3u8Request()) {
            info->isChunked = true;
            if (mediaDownloader->currentRequest_->url_.find(".flv") == std::string::npos) {
                info->contentLen = LIVE_CONTENT_LENGTH;
            } else {
                info->contentLen = 0;
            }
            std::string headStr = (std::string)token;
            MEDIA_LOG_I("transfer-encoding: " PUBLIC_LOG_S, headStr.c_str());
        }
    }
    if (!strncmp(key, "location", strlen("location"))) {
        FALSE_RETURN_V(next != nullptr, size * nitems);
        char* headTrim = StringTrim(next);
        MEDIA_LOG_I("redirect: " PUBLIC_LOG_S, headTrim);
        mediaDownloader->currentRequest_->location_ = headTrim;
    }

    if (!HandleContentRange(info, key, next, size, nitems) ||
        !HandleContentType(info, key, next, size * nitems, mediaDownloader) ||
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

void Downloader::SetAppUid(int32_t appUid)
{
    if (client_) {
        client_->SetAppUid(appUid);
    }
}

const std::shared_ptr<DownloadRequest>& Downloader::GetCurrentRequest()
{
    return currentRequest_;
}

void Downloader::SetInterruptState(bool isInterruptNeeded)
{
    MEDIA_LOG_I("0x%{public}06" PRIXPTR " Downloader SetInterruptState %{public}d",
        FAKE_POINTER(this), isInterruptNeeded);
    {
        AutoLock lk(loopPauseMutex_);
        AutoLock lock(sleepMutex_);
        isInterruptNeeded_ = isInterruptNeeded;
    }
    if (currentRequest_ != nullptr) {
        currentRequest_->isInterruptNeeded_ = isInterruptNeeded;
    }
    if (isInterruptNeeded) {
        MEDIA_LOG_I("SetInterruptState, Notify.");
        sleepCond_.NotifyOne();
    }
    if (sourceLoader_ != nullptr) {
        client_->Close(false);
    }
    NotifyLoopPause();
}

void Downloader::NotifyLoopPause()
{
    AutoLock lk(loopPauseMutex_);
    if (loopStatus_ == LoopStatus::PAUSE || isInterruptNeeded_) {
        MEDIA_LOG_I("0x%{public}06" PRIXPTR " Downloader NotifyLoopPause", FAKE_POINTER(this));
        loopStatus_ = LoopStatus::IDLE;
        loopPauseCond_.NotifyAll();
    } else {
        loopStatus_ = LoopStatus::IDLE;
        MEDIA_LOG_I("Downloader not NotifyLoopPause loopStatus %{public}d isInterruptNeeded %{public}d",
            loopStatus_.load(), isInterruptNeeded_.load());
    }
}

void Downloader::WaitLoopPause()
{
    AutoLock lk(loopPauseMutex_);
    if (loopStatus_ == LoopStatus::IDLE) {
        MEDIA_LOG_I("0x%{public}06" PRIXPTR "Downloader not WaitLoopPause loopStatus is idle", FAKE_POINTER(this));
        return;
    }
    MEDIA_LOG_I("0x%{public}06" PRIXPTR "Downloader WaitLoopPause task loopStatus_ %{public}d",
        FAKE_POINTER(this), loopStatus_.load());
    loopStatus_ = LoopStatus::PAUSE;
    loopPauseCond_.Wait(lk, [this]() {
        MEDIA_LOG_I("0x%{public}06" PRIXPTR " WaitLoopPause wake loopStatus %{public}d",
            FAKE_POINTER(this), loopStatus_.load());
        return loopStatus_ != LoopStatus::PAUSE || isInterruptNeeded_;
    });
}

void Downloader::SetAppState(bool isAppBackground)
{
    isAppBackground_ = isAppBackground;
}

void Downloader::StopBufferring()
{
    MediaAVCodec::AVCodecTrace trace("Downloader::StopBufferring");
    FALSE_RETURN_MSG(task_ != nullptr && currentRequest_ != nullptr, "task or request is null");
    if (isAppBackground_) {
        FALSE_RETURN_NOLOG(client_ != nullptr);
        MEDIA_LOG_I("StopBufferring: is task not running.");
        client_->Close(false);
        client_->Deinit();
        {
            AutoLock lk(operatorMutex_);
            client_ = nullptr;
        }
    } else {
        {
            AutoLock lk(operatorMutex_);
            if (client_ == nullptr) {
                MEDIA_LOG_I("StopBufferring: restart task.");
                client_ = NetworkClient::GetInstance(&RxHeaderData, &RxBodyData, this);
                client_->Init();
                client_->Open(currentRequest_->url_, currentRequest_->httpHeader_,
                    currentRequest_->requestInfo_.timeoutMs);
                if (currentRequest_->startPos_ > 0 && currentRequest_->protocolType_ != RequestProtocolType::HTTP) {
                    currentRequest_->retryOnGoing_ = true;
                    currentRequest_->dropedDataLen_ = 0;
                }
                MEDIA_LOG_I("StopBufferring: begin pos " PUBLIC_LOG_U64, currentRequest_->startPos_);
            }
        }
        Start();
    }
}

void Downloader::SetDownloadCallback(const std::shared_ptr<DownloadMetricsInfo> &callback)
{
    downloadCallback_ = callback;
}

int64_t Downloader::GetCurrentMillisecond()
{
    auto duration = std::chrono::steady_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

void Downloader::UpdateDownloadInfo(Downloader *downloader, size_t dataLen)
{
    if (downloader) {
        auto now = downloader->GetCurrentMillisecond();
        if (downloader->lastDownloadTime_  == 0) {
            int64_t firstDownloadTime = now - downloader->startDownTime_;
            downloader->lastDownloadTime_ = downloader->startDownTime_;
            if (downloader->downloadCallback_ != nullptr) {
                downloader->downloadCallback_->UpdateFirstDownloadTime(firstDownloadTime, now);
            }
        }
        int64_t downloadTime = now - downloader->lastDownloadTime_;
        downloader->lastDownloadTime_ = now;
        if (downloader->downloadCallback_ != nullptr) {
            downloader->downloadCallback_->UpdateTotalDownloadTimeAndBytes(downloadTime, dataLen);
        }
    }
}
}
}
}
}
