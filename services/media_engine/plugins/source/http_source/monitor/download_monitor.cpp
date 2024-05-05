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
    constexpr int RETRY_TIMES_TO_REPORT_ERROR = 2;
}
DownloadMonitor::DownloadMonitor(std::shared_ptr<MediaDownloader> downloader) noexcept
    : downloader_(std::move(downloader))
{
    auto statusCallback = [this] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {
        OnDownloadStatus(std::forward<decltype(downloader)>(downloader), std::forward<decltype(request)>(request));
    };
    downloader_->SetStatusCallback(statusCallback);
    task_ = std::make_shared<Task>(std::string("OS_HttpMonitor"), "", TaskType::SINGLETON);
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
    return 50 * 1000; // retry after 50ms
}

bool DownloadMonitor::Open(const std::string& url, const std::map<std::string, std::string>& httpHeader)
{
    isPlaying_ = true;
    retryTasks_.clear();
    return downloader_->Open(url, httpHeader);
}

void DownloadMonitor::Pause()
{
    downloader_->Pause();
    isPlaying_ = false;
    lastReadTime_ = 0;
}

void DownloadMonitor::Resume()
{
    downloader_->Resume();
    isPlaying_ = true;
}

void DownloadMonitor::Close(bool isAsync)
{
    retryTasks_.clear();
    downloader_->Close(isAsync);
    task_->Stop();
    isPlaying_ = false;
}

bool DownloadMonitor::Read(unsigned char *buff, unsigned int wantReadLength,
                           unsigned int &realReadLength, bool &isEos)
{
    bool ret = downloader_->Read(buff, wantReadLength, realReadLength, isEos);
    time(&lastReadTime_);
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
    auto serverError = request->GetServerError();
    auto retryTimes = request->GetRetryTimes();
    MEDIA_LOG_I("NeedRetry: clientError = " PUBLIC_LOG_D32 ", serverError = " PUBLIC_LOG_D32
        ", retryTimes = " PUBLIC_LOG_D32 ",", clientError, serverError, retryTimes);
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
    FALSE_RETURN_MSG(downloader != nullptr, "downloader is null, url is " PUBLIC_LOG_S, request->GetUrl().c_str());
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

void DownloadMonitor::SetDemuxerState()
{
    downloader_->SetDemuxerState();
}

void DownloadMonitor::SetReadBlockingFlag(bool isReadBlockingAllowed)
{
    FALSE_RETURN_MSG(downloader_ != nullptr, "SetReadBlockingFlag downloader is null");
    downloader_->SetReadBlockingFlag(isReadBlockingAllowed);
}

void DownloadMonitor::SetInterruptState(bool isInterruptNeeded)
{
    if (downloader_ != nullptr) {
        downloader_->SetInterruptState(isInterruptNeeded);
    }
}
}
}
}
}