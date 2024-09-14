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
#define HST_LOG_TAG "DownloadMonitor"

#include "monitor/download_monitor.h"
#include "cpp_ext/algorithm_ext.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
namespace {
    constexpr int RETRY_TIMES_TO_REPORT_ERROR = 10;
    constexpr int RETRY_THRESHOLD = 1;
    constexpr int SERVER_ERROR_THRESHOLD = 500;
    constexpr int32_t READ_LOG_FEQUENCE = 50;
    constexpr int64_t MICROSECONDS_TO_MILLISECOND = 1000;
    constexpr int64_t RETRY_SEG = 50;
    const std::set<int32_t> CLIENT_RETRY_ERROR_CODES = {
        25, // Upload faild.
        26, // Faild to open/read local data from file/application.
        28, // Timeout was reached.
    };

    const std::set<int32_t> SERVER_RETRY_ERROR_CODES = {
        300,
        301,
        302,
        303,
        304,
        305,
        400,
        401,
    };
}
DownloadMonitor::DownloadMonitor(std::shared_ptr<MediaDownloader> downloader) noexcept
    : downloader_(std::move(downloader))
{
    auto statusCallback = [this] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {
        if (isClosed_) {
            MEDIA_LOG_W("Downloader monitor is already closed.");
            return;
        }
        OnDownloadStatus(std::forward<decltype(downloader)>(downloader), std::forward<decltype(request)>(request));
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
    if (task.request && task.function) {
        task.function();
    }
    return RETRY_SEG * MICROSECONDS_TO_MILLISECOND; // retry after 50ms
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
    MEDIA_LOGI_LIMIT(READ_LOG_FEQUENCE, "DownloadMonitor: haveReadData " PUBLIC_LOG_U64, haveReadData_);
    if (readDataInfo.isEos_ && ret == Status::END_OF_STREAM) {
        MEDIA_LOG_I("buffer is empty, read eos." PUBLIC_LOG_U64, haveReadData_);
    }
    return ret;
}

bool DownloadMonitor::SeekToPos(int64_t offset)
{
    isPlaying_ = true;
    {
        AutoLock lock(taskMutex_);
        retryTasks_.clear();
    }
    return downloader_->SeekToPos(offset);
}

size_t DownloadMonitor::GetContentLength() const
{
    return downloader_->GetContentLength();
}

int64_t DownloadMonitor::GetDuration() const
{
    return downloader_->GetDuration();
}

Seekable DownloadMonitor::GetSeekable() const
{
    return downloader_->GetSeekable();
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

std::vector<uint32_t> DownloadMonitor::GetBitRates()
{
    return downloader_->GetBitRates();
}

bool DownloadMonitor::SelectBitRate(uint32_t bitRate)
{
    return downloader_->SelectBitRate(bitRate);
}

void DownloadMonitor::SetCallback(Callback* cb)
{
    callback_ = cb;
    downloader_->SetCallback(cb);
}

void DownloadMonitor::SetStatusCallback(StatusCallbackFunc cb)
{
}

bool DownloadMonitor::GetStartedStatus()
{
    return downloader_->GetStartedStatus();
}

void DownloadMonitor::NotifyError(int32_t clientErrorCode, int32_t serverErrorCode)
{
    if (callback_ == nullptr) {
        MEDIA_LOG_E("callback_ is nullptr, notify error failed.");
        return;
    }
    if (serverErrorCode != 0) {
        int32_t errorCode = MediaServiceErrCode::MSERR_DATA_SOURCE_IO_ERROR;
        GetServerMediaServiceErrorCode(serverErrorCode, errorCode);
        callback_->OnEvent({PluginEventType::SERVER_ERROR, {errorCode}, "server error"});
        MEDIA_LOG_I("Notify http server error, code " PUBLIC_LOG_D32, serverErrorCode);
    }
    if (clientErrorCode != 0) {
        int32_t errorCode = MediaServiceErrCode::MSERR_DATA_SOURCE_IO_ERROR;
        GetClientMediaServiceErrorCode(clientErrorCode, errorCode);
        callback_->OnEvent({PluginEventType::SERVER_ERROR, {errorCode}, "client error"});
        MEDIA_LOG_I("Notify http client error, code " PUBLIC_LOG_D32, clientErrorCode);
    }
}

bool DownloadMonitor::NeedRetry(const std::shared_ptr<DownloadRequest>& request)
{
    auto retryTimes = request->GetRetryTimes();
    int32_t clientError = request->GetClientError();
    int32_t serverError = request->GetServerError();
    MEDIA_LOG_I("NeedRetry: clientError = " PUBLIC_LOG_D32 ", serverError = " PUBLIC_LOG_D32
        ", retryTimes = " PUBLIC_LOG_D32 ",", clientError, serverError, retryTimes);

    if (clientError == 0 && serverError == 0) {
        return false;
    }
    if (retryTimes <= RETRY_THRESHOLD) {
        return true;
    }

    if (CLIENT_RETRY_ERROR_CODES.find(clientError) == CLIENT_RETRY_ERROR_CODES.end() ||
        SERVER_RETRY_ERROR_CODES.find(serverError) == SERVER_RETRY_ERROR_CODES.end() ||
        serverError >= SERVER_ERROR_THRESHOLD) {
        if (!GetPlayable() || GetBufferingTimeOut()) {
            MEDIA_LOG_I("error code dont't need to retry.");
            downloader_->SetDownloadErrorState();
            NotifyError(clientError, serverError);
            request->Close();
            return false;
        }
    }
    if (retryTimes > RETRY_TIMES_TO_REPORT_ERROR) { // Report error to upper layer
        if (!GetPlayable() || GetBufferingTimeOut()) {
            MEDIA_LOG_I("Retry times readches the upper limit.");
            downloader_->SetDownloadErrorState();
            NotifyError(clientError, serverError);
            return false;
        }
    }
    return true;
}

void DownloadMonitor::OnDownloadStatus(std::shared_ptr<Downloader>& downloader,
                                       std::shared_ptr<DownloadRequest>& request)
{
    FALSE_RETURN_MSG(downloader != nullptr, "downloader is nullptr.");
    if (NeedRetry(request)) {
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
    if (downloader_ != nullptr) {
        downloader_->SetInterruptState(isInterruptNeeded);
    }
}

Status DownloadMonitor::GetStreamInfo(std::vector<StreamInfo>& streams)
{
    return downloader_->GetStreamInfo(streams);
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

size_t DownloadMonitor::GetBufferSize() const
{
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

bool DownloadMonitor::GetBufferingTimeOut()
{
    if (downloader_) {
        return downloader_->GetBufferingTimeOut();
    } else {
        return false;
    }
}

size_t DownloadMonitor::GetSegmentOffset()
{
    if (downloader_) {
        return downloader_->GetSegmentOffset();
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
    MEDIA_LOG_I("DownloadMonitor::StopBufferring");
    if (downloader_ == nullptr) {
        MEDIA_LOG_E("StopBufferring failed, downloader_ is nullptr");
        return Status::ERROR_NULL_POINTER;
    }
    return downloader_->StopBufferring(isAppBackground);
}

}
}
}
}