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
#define HST_LOG_TAG "DownloadMonitor"

#include "monitor/download_monitor.h"
#include "cpp_ext/algorithm_ext.h"
#include "common/event.h"
#include "utils/string_utils.h"
#include <sstream>

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
namespace {
    constexpr int RETRY_TIMES_TO_REPORT_ERROR = 5;
    constexpr int APP_DOWNLOAD_RETRY_TIMES = 5;
    constexpr int64_t RETRY_DELAY_TIME[] = {
        20, 100, 200, 400, 800,
        1600, 3200, 6400, 12800, 25600
    };
    constexpr int SERVER_ERROR_THRESHOLD = 500;
    constexpr int32_t READ_LOG_FREQUENCY = 50;
    constexpr int64_t MICROSECONDS_TO_MILLISECOND = 1000;
    constexpr int64_t RETRY_SEG = 50;
    constexpr int32_t REDIRECT_CODE = 302;
    const std::string CLIENT_ERROR_MSG = "client error";
    const std::string SERVER_ERROR_MSG = "server error";
    // Framework errors: no retry, no error report to App (internal handling)
    const std::set<int32_t> CLIENT_NOT_RETRY_ERROR_CODES = {
        992,
    };
    // App errors: no retry, but must report error back to App
    const std::set<int32_t> APP_NOT_RETRY_ERROR_CODES = {
        -2, // LOADING_ERROR_NO_RESOURCE
        -3, // LOADING_ERROR_INVAID_HANDLE
        -4, // LOADING_ERROR_ACCESS_DENIED
        -5, // LOADING_ERROR_ACCESS_TIMEOUT
        -6, // LOADING_ERROR_AUTHORIZE_FAILED
    };
    const std::set<int32_t> CLIENT_RETRY_ERROR_CODES = {
        -1, // Application resource not ready for access
        23, // notBlock
        25, // Upload failed.
        26, // Faild to open/read local data from file/application.
        28, // Timeout was reached.
        56,
        18,
        6,
        7,
        0,
    };
    const std::set<int32_t> SERVER_RETRY_ERROR_CODES = {
        300,
        301,
        302,
        303,
        304,
        305,
        403,
        500,
        0,
    };
    const std::unordered_map<RequestType, std::string> REQUEST_TYPE_MAP = {
        {RequestType::DEFAULT, "DEFAULT"},
        {RequestType::HTTP_MEDIA_DATA, "HTTP_MEDIA_DATA"},
        {RequestType::HLS_MEDIA_DATA, "HLS_MEDIA_DATA"},
        {RequestType::HLS_PLAYLIST, "HLS_PLAYLIST"},
        {RequestType::HLS_KEY, "HLS_KEY"},
        {RequestType::HLS_MAP, "HLS_MAP"},
        {RequestType::DASH_MPD, "DASH_MPD"},
        {RequestType::DASH_MEDIA_DATA, "DASH_MEDIA_DATA"}
    };
}

DownloadMonitor::DownloadMonitor(std::shared_ptr<MediaDownloader> downloader) noexcept
    : downloader_(std::move(downloader))
{
}

void DownloadMonitor::Init()
{
    downloader_->Init();
    
    auto weakDownloader = weak_from_this();
    auto statusCallback = [weakDownloader] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {
        auto shareDownloader = weakDownloader.lock();
        FALSE_RETURN_MSG(shareDownloader != nullptr, "statusCallback, Downloader monitor already destructed.");
        FALSE_RETURN_MSG_W(!shareDownloader->isClosed_, "statusCallback, Downloader monitor is already closed.");
        shareDownloader->OnDownloadStatus(std::forward<decltype(downloader)>(downloader),
            std::forward<decltype(request)>(request));
    };
    downloader_->SetStatusCallback(statusCallback);
    task_ = std::make_shared<Task>(std::string("OS_HttpMonitor"));
    task_->RegisterJob([this] { return HttpMonitorLoop(); });
    task_->Start();
}

int64_t DownloadMonitor::HttpMonitorLoop()
{
    RetryRequest task;
    {
        AutoLock lock(taskMutex_);
        if (!retryTasks_.empty()) {
            task = retryTasks_.front();
            retryTasks_.pop_front();
        }
    }
    int64_t retryDelayTime = RETRY_SEG;
    if (task.request && task.function) {
        task.function();
        auto retryTimes = task.request->GetRetryTimes();
        retryDelayTime = retryTimes > APP_DOWNLOAD_RETRY_TIMES ? RETRY_SEG : RETRY_DELAY_TIME[retryTimes];
    }
    return retryDelayTime * MICROSECONDS_TO_MILLISECOND;
}

bool DownloadMonitor::Open(const std::string& url, const std::map<std::string, std::string>& httpHeader)
{
    isPlaying_ = true;
    {
        AutoLock lock(taskMutex_);
        retryTasks_.clear();
    }
    return downloader_->Open(url, httpHeader);
}

void DownloadMonitor::Pause()
{
    if (downloader_ != nullptr) {
        downloader_->Pause();
    }
}

void DownloadMonitor::Resume()
{
    if (downloader_ != nullptr) {
        downloader_->Resume();
    }
}

void DownloadMonitor::Close(bool isAsync)
{
    isClosed_ = true;
    {
        AutoLock lock(taskMutex_);
        retryTasks_.clear();
    }
    if (isAsync) {
        downloader_->Close(true);
        task_->Stop();
    } else {
        task_->Stop();
        downloader_->Close(false);
    }
    isPlaying_ = false;
}

Status DownloadMonitor::Read(unsigned char* buff, ReadDataInfo& readDataInfo)
{
    auto ret = downloader_->Read(buff, readDataInfo);
    time(&lastReadTime_);
    if (ULLONG_MAX - haveReadData_ > readDataInfo.realReadLength_) {
        haveReadData_ += readDataInfo.realReadLength_;
    }
    MEDIA_LOGI_LIMIT(READ_LOG_FREQUENCY, "DownloadMonitor: haveReadData " PUBLIC_LOG_U64, haveReadData_);
    if (readDataInfo.isEos_ && ret == Status::END_OF_STREAM) {
        MEDIA_LOG_I("buffer is empty, read eos." PUBLIC_LOG_U64, haveReadData_);
    }
    if (isFirstRequest_.load() && readDataInfo.realReadLength_ > 0) {
        isFirstRequest_.store(false);
    }
    return ret;
}

bool DownloadMonitor::SeekToPos(int64_t offset, bool& isSeekHit)
{
    isPlaying_ = true;
    bool res = downloader_->SeekToPos(offset, isSeekHit);
    if (!isSeekHit) {
        AutoLock lock(taskMutex_);
        retryTasks_.clear();
    }
    return res;
}

size_t DownloadMonitor::GetContentLength() const
{
    return downloader_->GetContentLength();
}

int64_t DownloadMonitor::GetDuration() const
{
    return downloader_->GetDuration();
}

std::pair<int64_t, bool> DownloadMonitor::GetStartInfo() const
{
    return downloader_->GetStartInfo();
}

Seekable DownloadMonitor::GetSeekable() const
{
    return downloader_->GetSeekable();
}

std::vector<SeekRange> DownloadMonitor::GetSeekableRanges() const
{
    return downloader_->GetSeekableRanges();
}

void DownloadMonitor::SetMediaDuration(int64_t duration)
{
    downloader_->SetMediaDuration(duration);
}

bool DownloadMonitor::SeekToTime(int64_t seekTime, SeekMode mode)
{
    isPlaying_ = true;
    {
        AutoLock lock(taskMutex_);
        retryTasks_.clear();
    }
    return downloader_->SeekToTime(seekTime, mode);
}

bool DownloadMonitor::MediaSeekTimeByStreamId(int64_t seekTime, SeekMode mode, int32_t streamId)
{
    isPlaying_ = true;
    {
        AutoLock lock(taskMutex_);
        retryTasks_.clear();
    }
    return downloader_ != nullptr && downloader_->MediaSeekTimeByStreamId(seekTime, mode, streamId);
}

std::vector<uint32_t> DownloadMonitor::GetBitRates()
{
    return downloader_->GetBitRates();
}

bool DownloadMonitor::SelectBitRate(uint32_t bitRate)
{
    return downloader_->SelectBitRate(bitRate);
}

bool DownloadMonitor::AutoSelectBitRate(uint32_t bitRate)
{
    return downloader_->AutoSelectBitRate(bitRate);
}

void DownloadMonitor::SetCallback(const std::shared_ptr<Callback>& cb)
{
    callback_ = cb;
    downloader_->SetCallback(cb);
}

void DownloadMonitor::SetStatusCallback(StatusCallbackFunc cb)
{
    (void)cb;
}

// Notify client and server error.
void DownloadMonitor::ReportLoadingErrorEvent(const std::shared_ptr<DownloadRequest>& request,
    int32_t clientErrorCode, int32_t serverErrorCode)
{
    auto callback = callback_.lock();
    if (callback == nullptr) {
        MEDIA_LOG_E("callback_ is nullptr, report loading error failed.");
        return;
    }
    
    int32_t requestStage = LoadingRequestStage::LOADING_STAGE_MEDIA_DATA;
    if (isFirstRequest_.load()) {
        requestStage = LoadingRequestStage::LOADING_STAGE_CONNECTION;
    } else if (request != nullptr && (request->GetDownloaderName() == "hlsPlayList" ||
        request->GetDownloaderName() == "dashMpd")) {
        requestStage = LoadingRequestStage::LOADING_STAGE_PLAYLIST;
    } else {
        // do nothing
    }
    
    int64_t requestTimestamp = 0;
    if (request != nullptr) {
        requestTimestamp = request->GetDownloadStartSteadyTime();
    }
    
    int32_t errorCode = clientErrorCode != 0 ? clientErrorCode : serverErrorCode;
    
    callback->OnDfxEvent(PluginDfxEvent{DFX_EVENT_LOADING_ERROR,
        std::make_tuple(requestStage, requestTimestamp, errorCode)});
    MEDIA_LOG_I("ReportLoadingErrorEvent: stage=" PUBLIC_LOG_D32 ", timestamp=" PUBLIC_LOG_D64
        ", errorCode=" PUBLIC_LOG_D32, requestStage, requestTimestamp, errorCode);
}

void DownloadMonitor::NotifyError(NetworkErrorInfo networkErrorInfo)
{
    int32_t clientErrorCode = networkErrorInfo.clientErrorCode_;
    int32_t serverErrorCode = networkErrorInfo.serverErrorCode_;
    auto callback = callback_.lock();
    if (callback == nullptr) {
        MEDIA_LOG_E("callback_ is nullptr, notify error failed.");
        return;
    }
    if (clientErrorCode != 0) {
        int32_t errorCode = MediaServiceErrCode::MSERR_DATA_SOURCE_IO_ERROR;
        GetClientMediaServiceErrorCode(clientErrorCode, errorCode);
        if (downloader_ != nullptr) {
            downloader_->SetIsReportedErrorCode();
        }
        networkErrorInfo.isClient_ = true;
        callback->OnEvent({PluginEventType::SERVER_ERROR, {errorCode}, BuildErrorDescription(networkErrorInfo)});
        MEDIA_LOG_E("Notify http client error, code " PUBLIC_LOG_D32, clientErrorCode);
    }
    if (serverErrorCode != 0) {
        int32_t errorCode = MediaServiceErrCode::MSERR_DATA_SOURCE_IO_ERROR;
        GetServerMediaServiceErrorCode(serverErrorCode, errorCode);
        if (downloader_ != nullptr) {
            downloader_->SetIsReportedErrorCode();
        }
        networkErrorInfo.isClient_ = false;
        callback->OnEvent({PluginEventType::SERVER_ERROR, {errorCode}, BuildErrorDescription(networkErrorInfo)});
        MEDIA_LOG_E("Notify http server error, code " PUBLIC_LOG_D32, serverErrorCode);
    }
}

// description: The Description is used to upload error information to the application, where errorType indicates
// whether the error is from the client or server, errorCode represents the response status code, contentType
// indicates the download content type, and url represents the access link.
std::string DownloadMonitor::BuildErrorDescription(const NetworkErrorInfo& networkErrorInfo)
{
    std::ostringstream oss;
    
    auto it = REQUEST_TYPE_MAP.find(networkErrorInfo.requestType_);
    std::string contentType = (it != REQUEST_TYPE_MAP.end() ? it->second : "NULL");
    
    bool isClient = networkErrorInfo.isClient_;
    std::string errorType = isClient ? CLIENT_ERROR_MSG : SERVER_ERROR_MSG;
    int errorCode = isClient ? networkErrorInfo.clientErrorCode_ : networkErrorInfo.serverErrorCode_;
    std::string url = StringUtil::MaskUrl(networkErrorInfo.url_);

    oss << "requestError: true"
        << ", errorType: " << errorType
        << ", errorCode: " << errorCode
        << ", contentType: " << contentType
        << ", maskUrl: " << url;

    return oss.str();
}

void DownloadMonitor::SetSourceStatisticsDfx(
    std::shared_ptr<OHOS::MediaAVCodec::SourceStatisticsReportInfo> rpInfoPtr)
{
    if (downloader_ != nullptr) {
        downloader_->SetSourceStatisticsDfx(rpInfoPtr);
    }
}

void DownloadMonitor::ReportErrorAndClose(const std::shared_ptr<DownloadRequest>& request,
    int32_t clientError, int32_t serverError)
{
    auto networkErrorInfo = BuildNetworkErrorInfo(request);
    ReportLoadingErrorEvent(request, clientError, serverError);
    NotifyError(networkErrorInfo);
    if (downloader_ != nullptr) {
        downloader_->SetDownloadErrorState();
    }
    request->Close();
}

bool DownloadMonitor::CheckRetryLimitReached(int32_t clientError, int32_t retryTimes)
{
    int retryTimesTmp = clientError == -1 ? APP_DOWNLOAD_RETRY_TIMES : RETRY_TIMES_TO_REPORT_ERROR;
    if (retryTimes > retryTimesTmp) {
        MEDIA_LOG_W("Retry times readches the upper limit.");
        return true;
    }
    return false;
}

void DownloadMonitor::ReportAndStopDownload(const std::shared_ptr<DownloadRequest>& request,
    int32_t clientError, int32_t serverError, const NetworkErrorInfo& networkErrorInfo)
{
    ReportLoadingErrorEvent(request, clientError, serverError);
    NotifyError(networkErrorInfo);
    if (downloader_ != nullptr) {
        downloader_->SetDownloadErrorState();
    }
}

bool DownloadMonitor::NeedRetry(const std::shared_ptr<DownloadRequest>& request)
{
    auto clientError = request->GetClientError();
    int serverError = request->GetServerError();
    auto retryTimes = request->GetRetryTimes();
    auto networkErrorInfo = BuildNetworkErrorInfo(request);
    isNeedClearBuffer_ = serverError == REDIRECT_CODE;
    MEDIA_LOG_I("NeedRetry: clientError = " PUBLIC_LOG_D32 ", serverError = " PUBLIC_LOG_D32
        ", retryTimes = " PUBLIC_LOG_D32, clientError, serverError, retryTimes);

    if (CLIENT_NOT_RETRY_ERROR_CODES.find(static_cast<int32_t>(clientError)) != CLIENT_NOT_RETRY_ERROR_CODES.end()) {
        MEDIA_LOG_I("Client error code " PUBLIC_LOG_D32 " not retry.", static_cast<int32_t>(clientError));
        return false;
    }
    if (APP_NOT_RETRY_ERROR_CODES.find(static_cast<int32_t>(clientError)) != APP_NOT_RETRY_ERROR_CODES.end()) {
        MEDIA_LOG_I("App error code " PUBLIC_LOG_D32 " not retry, report error.", static_cast<int32_t>(clientError));
        ReportAndStopDownload(request, clientError, serverError, networkErrorInfo);
        return false;
    }
    if (downloader_ != nullptr && downloader_->IsNotRetry(request)) {
        ReportAndStopDownload(request, clientError, serverError, networkErrorInfo);
        return false;
    }
    if (clientError == 0 && serverError == 0) {
        return false;
    }
    if ((GetPlayable() && !GetReadTimeOut(clientError == -1)) &&
        retryTimes <= RETRY_TIMES_TO_REPORT_ERROR) {
        MEDIA_LOG_W("PlayableRetry: clientError = " PUBLIC_LOG_D32 ", retryTimes = " PUBLIC_LOG_D32,
            clientError, retryTimes);
        return true;
    }
    if (CLIENT_RETRY_ERROR_CODES.find(clientError) == CLIENT_RETRY_ERROR_CODES.end() ||
        SERVER_RETRY_ERROR_CODES.find(serverError) == SERVER_RETRY_ERROR_CODES.end() ||
        serverError > SERVER_ERROR_THRESHOLD) {
        MEDIA_LOG_W("error code dont't need to retry.");
        ReportErrorAndClose(request, clientError, serverError);
        return false;
    }
    if (CheckRetryLimitReached(clientError, retryTimes)) {
        ReportAndStopDownload(request, clientError, serverError, networkErrorInfo);
        return false;
    }
    MEDIA_LOG_W("DoRetryL clientError = " PUBLIC_LOG_D32 ", serverError = " PUBLIC_LOG_D32
        ", retryTimes = " PUBLIC_LOG_D32 ",", clientError, serverError, retryTimes);
    return true;
}

NetworkErrorInfo DownloadMonitor::BuildNetworkErrorInfo(const std::shared_ptr<DownloadRequest>& request)
{
    NetworkErrorInfo networkErrorInfo;
    networkErrorInfo.url_ = request->GetUrl();
    networkErrorInfo.clientErrorCode_ = request->GetClientError();
    networkErrorInfo.serverErrorCode_ = request->GetServerError();
    networkErrorInfo.requestType_ = request->GetRequestType();
    return networkErrorInfo;
}

void DownloadMonitor::OnDownloadStatus(std::shared_ptr<Downloader>& downloader,
                                       std::shared_ptr<DownloadRequest>& request)
{
    FALSE_RETURN_MSG(downloader != nullptr && request != nullptr, "downloader is nullptr.");
    if (NeedRetry(request)) {
        if (isNeedClearBuffer_) {
            downloader_->ClearBuffer();
        }
        AutoLock lock(taskMutex_);
        bool exists = CppExt::AnyOf(retryTasks_.begin(), retryTasks_.end(), [&](const RetryRequest& item) {
            return item.request->IsSame(request);
        });
        if (!exists) {
            RetryRequest retryRequest {request, [downloader, request] { downloader->Retry(request); }};
            retryTasks_.emplace_back(std::move(retryRequest));
        }
    }
}

void DownloadMonitor::SetIsTriggerAutoMode(bool isAuto)
{
    downloader_->SetIsTriggerAutoMode(isAuto);
}

void DownloadMonitor::SetDemuxerState(int32_t streamId)
{
    downloader_->SetDemuxerState(streamId);
}

void DownloadMonitor::SetReadBlockingFlag(bool isReadBlockingAllowed)
{
    FALSE_RETURN_MSG(downloader_ != nullptr, "SetReadBlockingFlag downloader is null");
    downloader_->SetReadBlockingFlag(isReadBlockingAllowed);
}

void DownloadMonitor::SetPlayStrategy(const std::shared_ptr<PlayStrategy>& playStrategy)
{
    if (downloader_ != nullptr) {
        downloader_->SetPlayStrategy(playStrategy);
    }
}

void DownloadMonitor::SetInterruptState(bool isInterruptNeeded)
{
    MEDIA_LOG_W("SetInterruptState: %{public}d", isInterruptNeeded);
    if (downloader_ != nullptr) {
        downloader_->SetInterruptState(isInterruptNeeded);
    }
}

Status DownloadMonitor::GetStreamInfo(std::vector<StreamInfo>& streams, bool isUpdate)
{
    if (downloader_ != nullptr) {
        return downloader_->GetStreamInfo(streams, isUpdate);
    }
    return Status::ERROR_NULL_POINTER;
}

Status DownloadMonitor::SelectStream(int32_t streamId)
{
    return downloader_->SelectStream(streamId);
}

void DownloadMonitor::GetDownloadInfo(DownloadInfo& downloadInfo)
{
    if (downloader_ != nullptr) {
        MEDIA_LOG_I("DownloadMonitor GetDownloadInfo");
        downloader_->GetDownloadInfo(downloadInfo);
    }
}

std::pair<int32_t, int32_t> DownloadMonitor::GetDownloadInfo()
{
    MEDIA_LOG_I("DownloadMonitor GetDownloadInfo");
    if (downloader_ == nullptr) {
        return std::make_pair(0, 0);
    }
    return downloader_->GetDownloadInfo();
}

void DownloadMonitor::GetPlaybackInfo(PlaybackInfo& playbackInfo)
{
    if (downloader_ != nullptr) {
        MEDIA_LOG_I("DownloadMonitor GetPlaybackInfo");
        downloader_->GetPlaybackInfo(playbackInfo);
    }
}

uint64_t DownloadMonitor::GetBufferSize() const
{
    FALSE_RETURN_V(downloader_ != nullptr, 0);
    return downloader_->GetBufferSize();
}

Status DownloadMonitor::SetCurrentBitRate(int32_t bitRate, int32_t streamID)
{
    MEDIA_LOG_I("SetCurrentBitRate");
    if (downloader_ == nullptr) {
        MEDIA_LOG_E("SetCurrentBitRate failed, downloader_ is nullptr");
        return Status::ERROR_INVALID_OPERATION;
    }
    return downloader_->SetCurrentBitRate(bitRate, streamID);
}

void DownloadMonitor::SetAppUid(int32_t appUid)
{
    if (downloader_) {
        downloader_->SetAppUid(appUid);
    }
}

bool DownloadMonitor::GetPlayable()
{
    if (downloader_) {
        return downloader_->GetPlayable();
    }
    return false;
}

bool DownloadMonitor::GetReadTimeOut(bool isDelay)
{
    if (downloader_) {
        return downloader_->GetReadTimeOut(isDelay);
    }
    return false;
}

size_t DownloadMonitor::GetSegmentOffset(StreamType streamType)
{
    if (downloader_) {
        return downloader_->GetSegmentOffset(streamType);
    }
    return 0;
}

bool DownloadMonitor::GetHLSDiscontinuity()
{
    if (downloader_) {
        return downloader_->GetHLSDiscontinuity();
    }
    return false;
}

Status DownloadMonitor::StopBufferring(bool isAppBackground)
{
    MEDIA_LOG_W("StopBufferring isBackground: %{public}d", isAppBackground);
    if (downloader_ == nullptr) {
        MEDIA_LOG_E("StopBufferring failed, downloader_ is nullptr");
        return Status::ERROR_NULL_POINTER;
    }
    return downloader_->StopBufferring(isAppBackground);
}

void DownloadMonitor::WaitForBufferingEnd()
{
    FALSE_RETURN_MSG(downloader_ != nullptr, "WaitForBufferingEnd downloader is nullptr");
    downloader_->WaitForBufferingEnd();
}

void DownloadMonitor::GetServerMediaServiceErrorCode(int32_t errorCode, int32_t& serverCode)
{
    if (serverErrorCodeMap_.find(errorCode) == serverErrorCodeMap_.end()) {
        MEDIA_LOG_W("Unknown server error code.");
    } else {
        serverCode = serverErrorCodeMap_[errorCode];
        MEDIA_LOG_D("serverCode: " PUBLIC_LOG_D32, static_cast<int32_t>(serverCode));
    }
}

void DownloadMonitor::GetClientMediaServiceErrorCode(int32_t errorCode, int32_t& clientCode)
{
    if (clientErrorCodeMap_.find(errorCode) == clientErrorCodeMap_.end()) {
        MEDIA_LOG_W("Unknown client error code.");
    } else {
        clientCode = clientErrorCodeMap_[errorCode];
        MEDIA_LOG_D("clientCode: " PUBLIC_LOG_D32, static_cast<int32_t>(clientCode));
    }
}

bool DownloadMonitor::SetInitialBufferSize(int32_t offset, int32_t size)
{
    return downloader_->SetInitialBufferSize(offset, size);
}

void DownloadMonitor::NotifyInitSuccess()
{
    FALSE_RETURN_MSG(downloader_ != nullptr, "NotifyInitSuccess downloader is nullptr");
    downloader_->NotifyInitSuccess();
}

uint64_t DownloadMonitor::GetCachedDuration()
{
    FALSE_RETURN_V_MSG_E(downloader_ != nullptr, 0, "downloader_ is nullptr");
    return downloader_->GetCachedDuration();
}

void DownloadMonitor::RestartAndClearBuffer()
{
    FALSE_RETURN_MSG(downloader_ != nullptr, "downloader_ is nullptr");
    return downloader_->RestartAndClearBuffer();
}

bool DownloadMonitor::IsFlvLive()
{
    FALSE_RETURN_V_MSG_E(downloader_ != nullptr, false, "downloader_ is nullptr");
    return downloader_->IsFlvLive();
}

void DownloadMonitor::SetStartPts(int64_t startPts)
{
    if (downloader_) {
        downloader_->SetStartPts(startPts);
    }
}

void DownloadMonitor::SetExtraCache(uint64_t cacheDuration)
{
    if (downloader_) {
        downloader_->SetExtraCache(cacheDuration);
    }
}

void DownloadMonitor::SetMediaStreams(const MediaStreamList& mediaStreams)
{
    if (downloader_) {
        downloader_->SetMediaStreams(mediaStreams);
    }
}

bool DownloadMonitor::IsHlsFmp4()
{
    FALSE_RETURN_V_MSG_E(downloader_ != nullptr, false, "downloader_ is nullptr");
    return downloader_->IsHlsFmp4();
}

std::string DownloadMonitor::GetContentType()
{
    FALSE_RETURN_V(downloader_ != nullptr, "");
    return downloader_->GetContentType();
}

uint64_t DownloadMonitor::GetMemorySize()
{
    FALSE_RETURN_V_MSG_E(downloader_ != nullptr, 0, "downloader_ is nullptr");
    return downloader_->GetMemorySize();
}

std::string DownloadMonitor::GetCurUrl()
{
    FALSE_RETURN_V(downloader_ != nullptr, "");
    return downloader_->GetCurUrl();
}

std::string DownloadMonitor::GetMaskUrlByReadIndex(int32_t streamId)
{
    FALSE_RETURN_V(downloader_ != nullptr, "");
    return downloader_->GetMaskUrlByReadIndex(streamId);
}

void DownloadMonitor::SetDefaultStreamId(int32_t &videoStreamId, int32_t &audioStreamId, int32_t &subTitleStreamId)
{
    if (downloader_) {
        downloader_->SetDefaultStreamId(videoStreamId, audioStreamId, subTitleStreamId);
    }
}

bool DownloadMonitor::IsHlsEnd(int32_t streamId)
{
    FALSE_RETURN_V_MSG_E(downloader_ != nullptr, false, "downloader_ is nullptr");
    return downloader_->IsHlsEnd(streamId);
}
}
}
}
}