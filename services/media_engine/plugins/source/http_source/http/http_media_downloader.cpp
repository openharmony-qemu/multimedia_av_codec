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
#include <regex>

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
namespace {
#ifdef OHOS_LITE
constexpr int RING_BUFFER_SIZE = 5 * 48 * 1024;
constexpr int WATER_LINE = RING_BUFFER_SIZE / 30; // 30 WATER_LINE:8192
#else
constexpr int RING_BUFFER_SIZE = 50 * 1024 * 1024;
constexpr int MAX_BUFFER_SIZE = 50 * 1024 * 1024;
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
    cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferImpl>();
    cacheMediaBuffer_->Init(MAX_CACHE_BUFFER_SIZE, CHUNK_SIZE);
    MEDIA_LOG_I("setting buffer size: " PUBLIC_LOG_U64, MAX_CACHE_BUFFER_SIZE);
    
    downloader_ = std::make_shared<Downloader>("http");
    steadyClock_.Reset();
}

HttpMediaDownloader::HttpMediaDownloader(uint32_t expectBufferDuration)
{
    int totalBufferSize = CURRENT_BIT_RATE * expectBufferDuration;
    cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferImpl>();
    if (totalBufferSize < RING_BUFFER_SIZE) {
        MEDIA_LOG_I("Failed setting buffer size: " PUBLIC_LOG_D32 ". already lower than the min buffer size: "
        PUBLIC_LOG_D32 ", setting buffer size: " PUBLIC_LOG_D32 ". ",
        totalBufferSize, RING_BUFFER_SIZE, RING_BUFFER_SIZE);
        cacheMediaBuffer_->Init(RING_BUFFER_SIZE, CHUNK_SIZE);
        totalBufferSize_ = RING_BUFFER_SIZE;
    } else if (totalBufferSize > MAX_BUFFER_SIZE) {
        MEDIA_LOG_I("Failed setting buffer size: " PUBLIC_LOG_D32 ". already exceed the max buffer size: "
        PUBLIC_LOG_D32 ", setting buffer size: " PUBLIC_LOG_D32 ". ",
        totalBufferSize, MAX_BUFFER_SIZE, MAX_BUFFER_SIZE);
        cacheMediaBuffer_->Init(MAX_BUFFER_SIZE, CHUNK_SIZE);
        totalBufferSize_ = MAX_BUFFER_SIZE;
    } else {
        cacheMediaBuffer_->Init(totalBufferSize, CHUNK_SIZE);
        totalBufferSize_ = totalBufferSize;
        MEDIA_LOG_I("Success setted buffer size: " PUBLIC_LOG_D32, totalBufferSize);
    }
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
    downloader_->Start();
    return true;
}

void HttpMediaDownloader::Close(bool isAsync)
{
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
    downloader_->Pause();
    cvReadWrite_.NotifyOne();
}

void HttpMediaDownloader::Resume()
{
    downloader_->Resume();
    cvReadWrite_.NotifyOne();
}

void HttpMediaDownloader::OnClientErrorEvent()
{
    if (callback_ != nullptr) {
        MEDIA_LOG_I("Read time out, OnEvent");
        callback_->OnEvent({PluginEventType::CLIENT_ERROR, {NetworkClientErrorCode::ERROR_TIME_OUT}, "read"});
    }
}

Status HttpMediaDownloader::Read(unsigned char* buff, ReadDataInfo& readDataInfo)
{
    FALSE_RETURN_V_MSG(cacheMediaBuffer_ != nullptr, Status::END_OF_STREAM, "cacheMediaBuffer_ = nullptr");
    FALSE_RETURN_V_MSG(!isInterruptNeeded_.load(), Status::END_OF_STREAM, "isInterruptNeeded");
    FALSE_RETURN_V_MSG(readDataInfo.wantReadLength_ > 0, Status::END_OF_STREAM, "wantReadLength_ <= 0");
    while (cacheMediaBuffer_->GetBufferSize(readOffset_) < readDataInfo.wantReadLength_) {
        if (CheckIsEosBeforeTimeout(buff, readDataInfo)) {
            return Status::END_OF_STREAM;
        }
        if (downloadErrorState_) {
            if (downloader_ != nullptr) {
                downloader_->Pause(true); // the downloader is unavailable after this
            }
            if (downloader_ != nullptr && !downloadRequest_->IsClosed()) {
                downloadRequest_->Close();
            }
            OnClientErrorEvent();
            readDataInfo.realReadLength_ = 0;
            return Status::END_OF_STREAM;
        }
        bool isClosed = downloadRequest_->IsClosed();
        if (isClosed && cacheMediaBuffer_->GetBufferSize(readOffset_) == 0) {
            MEDIA_LOG_D("HttpMediaDownloader read return, isClosed: " PUBLIC_LOG_D32, isClosed);
            readDataInfo.realReadLength_ = 0;
            return Status::END_OF_STREAM;
        }
        Task::SleepInTask(5); // 5
    }
    readDataInfo.realReadLength_ = cacheMediaBuffer_->Read(buff, readOffset_, readDataInfo.wantReadLength_);
    if (readDataInfo.realReadLength_ == 0) {
        cacheMediaBuffer_->Dump(0);
    }
    readOffset_ += readDataInfo.realReadLength_;

    MEDIA_LOG_D("Read: wantReadLength " PUBLIC_LOG_D32 ", realReadLength " PUBLIC_LOG_D32 ", isEos "
                PUBLIC_LOG_D32, readDataInfo.wantReadLength_, readDataInfo.realReadLength_, readDataInfo.isEos_);
    return Status::OK;
}

bool HttpMediaDownloader::CheckIsEosBeforeTimeout(unsigned char* buff, ReadDataInfo& readDataInfo)
{
    if (downloadRequest_ != nullptr) {
        readDataInfo.isEos_ = downloadRequest_->IsEos();
    };
    if (readDataInfo.isEos_ && buffer_->GetSize() == 0) {
        MEDIA_LOG_D("HttpMediaDownloader read return, isEos: " PUBLIC_LOG_D32, readDataInfo.isEos_);
        readDataInfo.realReadLength_ = 0;
        return true;
    }
    if (readDataInfo.isEos_ && downloadRequest_->IsChunkedVod()) {
        readDataInfo.realReadLength_ = buffer_->ReadBuffer(buff, buffer_->GetSize(), 2); // wait 2 times
        return true;
    }
    return false;
}

bool HttpMediaDownloader::HandleSeekHit(int64_t offset)
{
    size_t seekOffset = offset + cacheMediaBuffer_->GetBufferSize(offset);
    MEDIA_LOG_I("Seek: buffer size " PUBLIC_LOG_ZU ", offset " PUBLIC_LOG_D64, buffer_->GetSize(), offset);

    readOffset_ = offset;
    cacheMediaBuffer_->Seek(offset);

    size_t fileContentLength = downloadRequest_->GetFileContentLength();
    if (seekOffset < fileContentLength) {
        seekOffset = offset + cacheMediaBuffer_->GetBufferSize(offset);
        if (writeOffset_ != seekOffset) {
            downloader_->Pause(true);
            seekOffset = offset + cacheMediaBuffer_->GetBufferSize(offset);
            bool result = downloader_->Seek(seekOffset);
            if (result) {
                writeOffset_ = seekOffset;
            }
            downloader_->Resume();
        } else {
            MEDIA_LOG_D("Hit continue download");
        }
    }
    return true;
}

bool HttpMediaDownloader::SeekToPos(int64_t offset)
{
    size_t remain = cacheMediaBuffer_->GetBufferSize(offset);
    if (remain == 0) {
        downloader_->Pause(true);
        bool result = false;
        if (cacheMediaBuffer_->GetBufferSize(offset) <= 0) {
            result = downloader_->Seek(offset);
            if (result) {
                writeOffset_ = offset;
                cacheMediaBuffer_->Seek(offset);
                readOffset_ = offset;
                downloader_->Resume();
                return true;
            } else {
                downloader_->Resume();
                seekFailedCount_++;
                return false;
            }
        }
    }
    return HandleSeekHit(offset);
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
    size_t res = cacheMediaBuffer_->Write(data, writeOffset_, len);
    writeOffset_ += res;

    if (res == 0) {
        cacheMediaBuffer_->Dump(0);
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