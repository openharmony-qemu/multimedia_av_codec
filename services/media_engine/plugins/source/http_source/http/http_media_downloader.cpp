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
#define HST_LOG_TAG "HttpMediaDownloader"

#include "http/http_media_downloader.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
namespace {
#ifdef OHOS_LITE
constexpr int RING_BUFFER_SIZE = 5 * 48 * 1024;
constexpr int WATER_LINE = RING_BUFFER_SIZE / 30; // 30 WATER_LINE:8192
#else
constexpr int RING_BUFFER_SIZE = 5 * 1024 * 1024;
constexpr int MAX_BUFFER_SIZE = 20 * 1024 * 1024;
constexpr int WATER_LINE = 8192; //  WATER_LINE:8192
constexpr int CURRENT_BIT_RATE = 1 * 1024 * 1024;
#endif
constexpr int32_t TIME_OUT = 3 * 1000;
}

HttpMediaDownloader::HttpMediaDownloader() noexcept
{
    buffer_ = std::make_shared<RingBuffer>(RING_BUFFER_SIZE);
    buffer_->Init();
    downloader_ = std::make_shared<Downloader>("http");
}

HttpMediaDownloader::HttpMediaDownloader(uint32_t expectBufferDuration)
{
    int totalBufferSize = CURRENT_BIT_RATE * expectBufferDuration;
    if (totalBufferSize < RING_BUFFER_SIZE) {
        MEDIA_LOG_I("Failed setting buffer size: " PUBLIC_LOG_D32 ". already lower than the min buffer size: "
        PUBLIC_LOG_D32 ", setting buffer size: " PUBLIC_LOG_D32 ". ",
        totalBufferSize, RING_BUFFER_SIZE, RING_BUFFER_SIZE);
        buffer_ = std::make_shared<RingBuffer>(RING_BUFFER_SIZE);
        totalBufferSize_ = RING_BUFFER_SIZE;
    } else if (totalBufferSize > MAX_BUFFER_SIZE) {
        MEDIA_LOG_I("Failed setting buffer size: " PUBLIC_LOG_D32 ". already exceed the max buffer size: "
        PUBLIC_LOG_D32 ", setting buffer size: " PUBLIC_LOG_D32 ". ",
        totalBufferSize, MAX_BUFFER_SIZE, MAX_BUFFER_SIZE);
        buffer_ = std::make_shared<RingBuffer>(MAX_BUFFER_SIZE);
        totalBufferSize_ = MAX_BUFFER_SIZE;
    } else {
        buffer_ = std::make_shared<RingBuffer>(totalBufferSize);
        totalBufferSize_ = totalBufferSize;
        MEDIA_LOG_I("Success setted buffer size: " PUBLIC_LOG_D32, totalBufferSize);
    }
    buffer_->Init();
    downloader_ = std::make_shared<Downloader>("http");
}

HttpMediaDownloader::~HttpMediaDownloader()
{
    MEDIA_LOG_I("~HttpMediaDownloader dtor");
    Close(false);
}

bool HttpMediaDownloader::Open(const std::string& url, const std::map<std::string, std::string>& httpHeader)
{
    MEDIA_LOG_I("Open download " PUBLIC_LOG_S, url.c_str());
    auto saveData =  [this] (uint8_t*&& data, uint32_t&& len) {
        return SaveData(std::forward<decltype(data)>(data), std::forward<decltype(len)>(len));
    };
    FALSE_RETURN_V(statusCallback_ != nullptr, false);
    auto realStatusCallback = [this] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                                  std::shared_ptr<DownloadRequest>& request) {
        statusCallback_(status, downloader_, std::forward<decltype(request)>(request));
    };
    MediaSouce mediaSouce;
    mediaSouce.url = url;
    mediaSouce.httpHeader = httpHeader;
    downloadRequest_ = std::make_shared<DownloadRequest>(saveData, realStatusCallback, mediaSouce);
    downloader_->Download(downloadRequest_, -1); // -1
    buffer_->SetMediaOffset(0);
    downloader_->Start();
    return true;
}

void HttpMediaDownloader::Close(bool isAsync)
{
    buffer_->SetActive(false);
    downloader_->Stop(isAsync);
    cvReadWrite_.NotifyOne();
}

void HttpMediaDownloader::Pause()
{
    bool cleanData = GetSeekable() != Seekable::SEEKABLE;
    buffer_->SetActive(false, cleanData);
    downloader_->Pause();
    cvReadWrite_.NotifyOne();
}

void HttpMediaDownloader::Resume()
{
    buffer_->SetActive(true);
    downloader_->Resume();
    cvReadWrite_.NotifyOne();
}

bool HttpMediaDownloader::Read(unsigned char* buff, unsigned int wantReadLength,
                               unsigned int& realReadLength, bool& isEos)
{
    FALSE_RETURN_V(buffer_ != nullptr, false);
    isEos = false;
    readTime_ = 0;
    uint32_t remain = downloadRequest_->GetFileContentLength() - buffer_->GetMediaOffset();
    if (remain > 0) {
        wantReadLength = remain < wantReadLength ? remain : wantReadLength;
    }
    while (buffer_->GetSize() < wantReadLength && !isInterruptNeeded_.load()) {
        if (downloadRequest_ != nullptr) {
            isEos = downloadRequest_->IsEos();
        };
        if (isEos && buffer_->GetSize() == 0) {
            MEDIA_LOG_D("HttpMediaDownloader read return, isEos: " PUBLIC_LOG_D32, isEos);
            realReadLength = 0;
            return false;
        }
        if (readTime_ >= TIME_OUT || downloadErrorState_ || isTimeOut_) {
            isTimeOut_ = true;
            if (downloader_ != nullptr) {
                downloader_->Pause();
            }
            if (downloader_ != nullptr && !downloadRequest_->IsClosed()) {
                downloadRequest_->Close();
            }
            if (callback_ != nullptr) {
                MEDIA_LOG_I("Read time out, OnEvent");
                callback_->OnEvent({PluginEventType::CLIENT_ERROR, {NetworkClientErrorCode::ERROR_TIME_OUT}, "read"});
            }
            realReadLength = 0;
            return false;
        }
        bool isClosed = downloadRequest_->IsClosed();
        if (isClosed && buffer_->GetSize() == 0) {
            MEDIA_LOG_D("HttpMediaDownloader read return, isClosed: " PUBLIC_LOG_D32, isClosed);
            realReadLength = 0;
            return false;
        }
        Task::SleepInTask(5); // 5
        readTime_ += 5;    // 5
    }
    realReadLength = buffer_->ReadBuffer(buff, wantReadLength, 2);  // wait 2 times
    MEDIA_LOG_D("Read: wantReadLength " PUBLIC_LOG_D32 ", realReadLength " PUBLIC_LOG_D32 ", isEos "
                PUBLIC_LOG_D32, wantReadLength, realReadLength, isEos);
    return true;
}

bool HttpMediaDownloader::SeekToPos(int64_t offset)
{
    FALSE_RETURN_V(buffer_ != nullptr, false);
    MEDIA_LOG_I("Seek: buffer size " PUBLIC_LOG_ZU ", offset " PUBLIC_LOG_D64, buffer_->GetSize(), offset);
    if (buffer_->Seek(offset)) {
        MEDIA_LOG_I("buffer_ seek success.");
        return true;
    }
    buffer_->SetActive(false); // First clear buffer, avoid no available buffer then task pause never exit.
    downloader_->Pause();
    buffer_->Clear();
    buffer_->SetActive(true);
    bool result = downloader_->Seek(offset);
    if (result) {
        buffer_->SetMediaOffset(offset);
    }
    downloader_->Resume();
    return result;
}

size_t HttpMediaDownloader::GetContentLength() const
{
    if (downloadRequest_->IsClosed()) {
        return 0; // 0
    }
    return downloadRequest_->GetFileContentLength();
}

int64_t HttpMediaDownloader::GetDuration() const
{
    return 0;
}

Seekable HttpMediaDownloader::GetSeekable() const
{
    return downloadRequest_->IsChunked(isInterruptNeeded_);
}

void HttpMediaDownloader::SetCallback(Callback* cb)
{
    callback_ = cb;
}

void HttpMediaDownloader::SetStatusCallback(StatusCallbackFunc cb)
{
    statusCallback_ = cb;
}

bool HttpMediaDownloader::GetStartedStatus()
{
    return startedPlayStatus_;
}

void HttpMediaDownloader::SetReadBlockingFlag(bool isReadBlockingAllowed)
{
    FALSE_RETURN(buffer_ != nullptr);
    buffer_->SetReadBlocking(isReadBlockingAllowed);
}

bool HttpMediaDownloader::SaveData(uint8_t* data, uint32_t len)
{
    FALSE_RETURN_V(buffer_->WriteBuffer(data, len), false);
    cvReadWrite_.NotifyOne();
    size_t bufferSize = buffer_->GetSize();
    double ratio = (static_cast<double>(bufferSize)) / RING_BUFFER_SIZE;
    if ((bufferSize >= WATER_LINE ||
        bufferSize >= downloadRequest_->GetFileContentLength() / 2) && !aboveWaterline_) { // 2
        aboveWaterline_ = true;
        MEDIA_LOG_I("Send http aboveWaterline event, ringbuffer ratio " PUBLIC_LOG_F, ratio);
        if (callback_ != nullptr) {
            callback_->OnEvent({PluginEventType::ABOVE_LOW_WATERLINE, {ratio}, "http"});
        }
        startedPlayStatus_ = true;
    } else if (bufferSize < WATER_LINE && aboveWaterline_) {
        aboveWaterline_ = false;
        MEDIA_LOG_I("Send http belowWaterline event, ringbuffer ratio " PUBLIC_LOG_F, ratio);
        if (callback_ != nullptr) {
            callback_->OnEvent({PluginEventType::BELOW_LOW_WATERLINE, {ratio}, "http"});
        }
    }
    return true;
}

void HttpMediaDownloader::SetDemuxerState()
{
    MEDIA_LOG_I("SetDemuxerState");
    isReadFrame_ = true;
}

void HttpMediaDownloader::SetDownloadErrorState()
{
    MEDIA_LOG_I("SetDownloadErrorState");
    downloadErrorState_ = true;
    if (buffer_ != nullptr && buffer_->GetSize() == 0) {
        Close(true);
    }
}

void HttpMediaDownloader::SetInterruptState(bool isInterruptNeeded)
{
    isInterruptNeeded_ = isInterruptNeeded;
}

int HttpMediaDownloader::GetBufferSize()
{
    return totalBufferSize_;
}

RingBuffer& HttpMediaDownloader::GetBuffer()
{
    return *buffer_;
}

bool HttpMediaDownloader::GetReadFrame()
{
    return isReadFrame_;
}

bool HttpMediaDownloader::GetDownloadErrorState()
{
    return downloadErrorState_;
}

StatusCallbackFunc HttpMediaDownloader::GetStatusCallbackFunc()
{
    return statusCallback_;
}
}
}
}
}