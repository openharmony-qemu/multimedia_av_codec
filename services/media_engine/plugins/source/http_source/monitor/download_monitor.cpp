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
#include <set>

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
}

DownloadMonitor::DownloadMonitor(std::shared_ptr<MediaDownloader> downloader) noexcept
    : downloader_(std::move(downloader))
{
    auto statusCallback = [this] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {
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

bool DownloadMonitor::NeedRetry(const std::shared_ptr<DownloadRequest>& request)
{
    auto clientError = request->GetClientError();
    int serverError = static_cast<int>(request->GetServerError());
    auto retryTimes = request->GetRetryTimes();
    std::set<int> notRetryErrorSet = {400, 401, 403};
    MEDIA_LOG_I("NeedRetry: clientError = " PUBLIC_LOG_D32 ", serverError = " PUBLIC_LOG_D32
        ", retryTimes = " PUBLIC_LOG_D32 ",", clientError, serverError, retryTimes);
    if (clientError == NetworkClientErrorCode::ERROR_NOT_RETRY ||
        notRetryErrorSet.find(serverError) != notRetryErrorSet.end() ||
        serverError >= SERVER_ERROR_THRESHOLD) {
        if (retryTimes > RETRY_THRESHOLD) {
            if (callback_ != nullptr) {
                MEDIA_LOG_I("Send http client error, code " PUBLIC_LOG_D32, static_cast<int32_t>(clientError));
                downloader_->SetDownloadErrorState();
            }
            request->Close();
            return false;
        }
        return true;
    }
    if ((clientError != NetworkClientErrorCode::ERROR_OK && clientError != NetworkClientErrorCode::ERROR_NOT_RETRY)
        || serverError != 0) {
        if (retryTimes > RETRY_TIMES_TO_REPORT_ERROR) { // Report error to upper layer
            if (clientError != NetworkClientErrorCode::ERROR_OK && callback_ != nullptr) {
                MEDIA_LOG_I("Send http client error, code " PUBLIC_LOG_D32, static_cast<int32_t>(clientError));
                downloader_->SetDownloadErrorState();
            }
            if (serverError != 0 && callback_ != nullptr) {
                MEDIA_LOG_I("Send http server error, code " PUBLIC_LOG_D32, serverError);
                downloader_->SetDownloadErrorState();
            }
            request->Close();
            return false;
        }
        return true;
    }
    return false;
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

Status DownloadMonitor::SetCurrentBitRate(int32_t bitRate, int32_t streamID)
{
    MEDIA_LOG_I("SetCurrentBitRate");
    if (downloader_ == nullptr) {
        MEDIA_LOG_E("SetCurrentBitRate failed, downloader_ is nullptr");
        return Status::ERROR_INVALID_OPERATION;
    }
    return downloader_->SetCurrentBitRate(bitRate, streamID);
}
}
}
}
}