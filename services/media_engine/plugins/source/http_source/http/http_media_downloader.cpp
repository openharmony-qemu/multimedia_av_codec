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
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <regex>
#include <sys/types.h>
#include <sys/socket.h>

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
constexpr int RECORD_TIME_INTERVAL = 3000;
constexpr int START_PLAY_WATER_LINE = 512 * 1024;
constexpr int DATA_USAGE_NTERVAL = 300 * 1000;
constexpr int AVG_SPEED_SUM_SCALE = 10000;
constexpr double ZERO_THRESHOLD = 1e-9;
}

HttpMediaDownloader::HttpMediaDownloader() noexcept
{
    buffer_ = std::make_shared<RingBuffer>(RING_BUFFER_SIZE);
    buffer_->Init();
    downloader_ = std::make_shared<Downloader>("http");
    steadyClock_.Reset();
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
    steadyClock_.Reset();
}

HttpMediaDownloader::~HttpMediaDownloader()
{
    MEDIA_LOG_I("~HttpMediaDownloader dtor");
    Close(false);
}

bool HttpMediaDownloader::Open(const std::string& url, const std::map<std::string, std::string>& httpHeader)
{
    MEDIA_LOG_I("Open download " PUBLIC_LOG_S, url.c_str());
    openTime_ = steadyClock_.ElapsedMilliseconds();
    auto saveData =  [this] (uint8_t*&& data, uint32_t&& len) {
        return SaveData(std::forward<decltype(data)>(data), std::forward<decltype(len)>(len));
    };
    FALSE_RETURN_V(statusCallback_ != nullptr, false);
    auto realStatusCallback = [this] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                                  std::shared_ptr<DownloadRequest>& request) {
        statusCallback_(status, downloader_, std::forward<decltype(request)>(request));
    };
    auto downloadDoneCallback = [this] (const std::string &url, const std::string& location) {
        isDownloadFinish_= true;
        int64_t nowTime = steadyClock_.ElapsedMilliseconds();
        double downloadTime = (static_cast<double>(nowTime) - static_cast<double>(startDownloadTime_)) / 1000;
        if (downloadTime > ZERO_THRESHOLD) {
            avgDownloadSpeed_ = totalBits_ / downloadTime;
        }
        MEDIA_LOG_D("Download done, average download speed: " PUBLIC_LOG_D32 " bit/s",
                    static_cast<int32_t>(avgDownloadSpeed_));
        MEDIA_LOG_D("Download done, data usage: " PUBLIC_LOG_U64 " bits in " PUBLIC_LOG_D64 "ms",
                    totalBits_, static_cast<int64_t>(downloadTime * 1000));
    };
    MediaSouce mediaSouce;
    mediaSouce.url = url;
    mediaSouce.httpHeader = httpHeader;
    downloadRequest_ = std::make_shared<DownloadRequest>(saveData, realStatusCallback, mediaSouce);
    downloadRequest_->SetDownloadDoneCb(downloadDoneCallback);
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
    if (!isDownloadFinish_) {
        int64_t nowTime = steadyClock_.ElapsedMilliseconds();
        double downloadTime = (static_cast<double>(nowTime) - static_cast<double>(startDownloadTime_)) / 1000;
        if (downloadTime > ZERO_THRESHOLD) {
            avgDownloadSpeed_ = totalBits_ / downloadTime;
        }
        MEDIA_LOG_D("Download close, average download speed: " PUBLIC_LOG_D32 " bit/s",
            static_cast<int32_t>(avgDownloadSpeed_));
        MEDIA_LOG_D("Download close, data usage: " PUBLIC_LOG_U64 " bits in " PUBLIC_LOG_D64 "ms",
            totalBits_, static_cast<int64_t>(downloadTime * 1000));
    }
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

Status HttpMediaDownloader::Read(unsigned char* buff, ReadDataInfo& readDataInfo)
{
    FALSE_RETURN_V(buffer_ != nullptr, Status::END_OF_STREAM);
    readDataInfo.isEos_ = false;
    readTime_ = 0;
    size_t fileContentLength = downloadRequest_->GetFileContentLength();
    uint64_t mediaOffset = buffer_->GetMediaOffset();
    if (fileContentLength > mediaOffset) {
        uint64_t remain = fileContentLength - mediaOffset;
        readDataInfo.wantReadLength_ = remain < readDataInfo.wantReadLength_ ? remain : readDataInfo.wantReadLength_;
    }

    while (buffer_->GetSize() < readDataInfo.wantReadLength_ && !isInterruptNeeded_.load()) {
        if (downloadRequest_ != nullptr) {
            readDataInfo.isEos_ = downloadRequest_->IsEos();
        };
        if (readDataInfo.isEos_ && buffer_->GetSize() == 0) {
            MEDIA_LOG_D("HttpMediaDownloader read return, isEos: " PUBLIC_LOG_D32, readDataInfo.isEos_);
            readDataInfo.realReadLength_ = 0;
            return Status::END_OF_STREAM;
        }
        if (downloadErrorState_ || isTimeOut_) {
            isTimeOut_ = true;
            if (downloader_ != nullptr) {
                // avoid deadlock caused by ringbuffer write stall
                buffer_->SetActive(false);
                // the downloader is unavailable after this
                downloader_->Pause(true);
            }
            if (downloader_ != nullptr && !downloadRequest_->IsClosed()) {
                downloadRequest_->Close();
            }
            if (callback_ != nullptr) {
                MEDIA_LOG_I("Read time out, OnEvent");
                callback_->OnEvent({PluginEventType::CLIENT_ERROR, {NetworkClientErrorCode::ERROR_TIME_OUT}, "read"});
            }
            readDataInfo.realReadLength_ = 0;
            return Status::END_OF_STREAM;
        }
        bool isClosed = downloadRequest_->IsClosed();
        if (isClosed && buffer_->GetSize() == 0) {
            MEDIA_LOG_D("HttpMediaDownloader read return, isClosed: " PUBLIC_LOG_D32, isClosed);
            readDataInfo.realReadLength_ = 0;
            return Status::END_OF_STREAM;
        }
        Task::SleepInTask(5); // 5
        readTime_ += 5;    // 5
    }
    readDataInfo.realReadLength_ = buffer_->ReadBuffer(buff, readDataInfo.wantReadLength_, 2);  // wait 2 times
    MEDIA_LOG_D("Read: wantReadLength " PUBLIC_LOG_D32 ", realReadLength " PUBLIC_LOG_D32 ", isEos "
                PUBLIC_LOG_D32, readDataInfo.wantReadLength_, readDataInfo.realReadLength_, readDataInfo.isEos_);
    return Status::OK;
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
    } else {
        seekFailedCount_ ++;
        MEDIA_LOG_D("Seek failed count: " PUBLIC_LOG_D32, seekFailedCount_);
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
    OnWriteRingBuffer(len);
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

void HttpMediaDownloader::OnWriteRingBuffer(uint32_t len)
{
    if (startDownloadTime_ == 0) {
        int64_t nowTime = steadyClock_.ElapsedMilliseconds();
        startDownloadTime_ = nowTime;
        lastReportUsageTime_ = nowTime;
    }
    uint32_t writeBits = len * 8;
    totalBits_ += writeBits;
    dataUsage_ += writeBits;
    if ((totalBits_ > START_PLAY_WATER_LINE) && (playDelayTime_ == 0)) {
        auto startPlayTime = steadyClock_.ElapsedMilliseconds();
        playDelayTime_ = startPlayTime - openTime_;
        MEDIA_LOG_D("Start play delay time: " PUBLIC_LOG_D64, playDelayTime_);
    }
    DownloadReportLoop();
}

constexpr int IS_DOWNLOAD_MIN_BIT = 1000;     // 判断下载是否在进行的阈值 bit
void HttpMediaDownloader::DownloadReportLoop()
{
    int64_t now = steadyClock_.ElapsedMilliseconds();
    if ((now - lastCheckTime_) > RECORD_TIME_INTERVAL) {
        uint64_t curDownloadBits = totalBits_ - lastBits_;
        if (curDownloadBits >= IS_DOWNLOAD_MIN_BIT) {
            // 有效下载数据量
            downloadBits_ += curDownloadBits;
            double downloadDuration = static_cast<double>(now - lastCheckTime_) / 1000;
            double downloadSpeed = 0;
            if (downloadDuration > ZERO_THRESHOLD) {
                downloadSpeed = downloadBits_ / downloadDuration;
            }
            avgSpeedSum_ += downloadSpeed / AVG_SPEED_SUM_SCALE;
            recordSpeedCount_ ++;
            MEDIA_LOG_D("Current download speed : " PUBLIC_LOG_D32 " bit/s", static_cast<int32_t>(downloadSpeed));
        }
        // 下载总数据量
        lastBits_ = totalBits_;
        lastCheckTime_ = now;
        if (buffer_ != nullptr) {
            uint64_t remainingBuffer = buffer_->GetSize() * 8;
            MEDIA_LOG_D("The remaining of the buffer : " PUBLIC_LOG_U64, remainingBuffer);
        }
    }

    if (!isDownloadFinish_ && (now - lastReportUsageTime_) > DATA_USAGE_NTERVAL) {
        MEDIA_LOG_D("Data usage: " PUBLIC_LOG_U64 " bits in " PUBLIC_LOG_D32 "ms", dataUsage_, DATA_USAGE_NTERVAL);
        dataUsage_ = 0;
        lastReportUsageTime_ = now;
    }
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

void HttpMediaDownloader::GetDownloadInfo(DownloadInfo& downloadInfo)
{
    if (recordSpeedCount_ == 0) {
        MEDIA_LOG_E("HttpMediaDownloader is 0, can't get avgDownloadRate");
        downloadInfo.avgDownloadRate = 0;
    } else {
        downloadInfo.avgDownloadRate = (avgSpeedSum_ / recordSpeedCount_) * AVG_SPEED_SUM_SCALE;
    }
    downloadInfo.avgDownloadSpeed = avgDownloadSpeed_;
    downloadInfo.totalDownLoadBits = totalBits_;
    downloadInfo.isTimeOut = isTimeOut_;
}
}
}
}
}