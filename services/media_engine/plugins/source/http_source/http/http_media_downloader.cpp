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
#include "common/media_core.h"

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
constexpr int MAX_BUFFER_SIZE = 50 * 1024 * 1024;
constexpr int WATER_LINE = 8192; //  WATER_LINE:8192
constexpr int CURRENT_BIT_RATE = 1 * 1024 * 1024;
#endif
constexpr int RECORD_TIME_INTERVAL = 3000;
constexpr int START_PLAY_WATER_LINE = 512 * 1024;
constexpr int DATA_USAGE_NTERVAL = 300 * 1000;
constexpr int AVG_SPEED_SUM_SCALE = 10000;
constexpr double ZERO_THRESHOLD = 1e-9;
constexpr int PLAY_WATER_LINE = 5 * 1024;
constexpr int FIRST_CACHE_WATER_LINE = 50 * 1024;
constexpr int SECOND_CACHE_WATER_LINE = 100 * 1024;
constexpr int CACHE_WATER_LINE = 48 * 10 * 1024;
constexpr int BUFFERING_TIME_OUT = 1000;
constexpr int BUFFERING_SLEEP_TIME = 10;
constexpr int FIRST_TIMES = 1;
constexpr int SECOND_TIMES = 2;
constexpr int REQUEST_SLEEP_TIME = 5;
constexpr int CACHEDATA_SLEEP_TIME = 100;
constexpr int SECOND_TO_MICROSECOND = 1000;
constexpr int FIVE_MICROSECOND = 5;
constexpr int ONE_HUNDRED_MICROSECOND = 100;
constexpr uint32_t READ_SLEEP_TIME_OUT = 30 * 1000;
}

HttpMediaDownloader::HttpMediaDownloader(std::string url)
{
    if (static_cast<int32_t>(url.find(".flv")) != -1) {
        MEDIA_LOG_I("isflv.");
        isFlv_ = true;
    }

    if (isFlv_) {
        buffer_ = std::make_shared<RingBuffer>(RING_BUFFER_SIZE);
        buffer_->Init();
    } else {
        cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferImpl>();
        cacheMediaBuffer_->Init(MAX_CACHE_BUFFER_SIZE, CHUNK_SIZE);
        MEDIA_LOG_I("setting buffer size: " PUBLIC_LOG_U64, MAX_CACHE_BUFFER_SIZE);
    }
    isBuffering_ = true;
    downloader_ = std::make_shared<Downloader>("http");
    steadyClock_.Reset();

    downloadTask_ = std::make_shared<Task>(std::string("HttpdownloadTask"), "", TaskType::SINGLETON);
    downloadTask_->RegisterJob([this] {
        CacheData();
        return 0;
    });
    wantReadLength_ = PLAY_WATER_LINE;
    downloadTask_->Start();
}

void HttpMediaDownloader::InitRingBuffer(uint32_t expectBufferDuration)
{
    int totalBufferSize = CURRENT_BIT_RATE * static_cast<int32_t>(expectBufferDuration);
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
}

void HttpMediaDownloader::InitCacheBuffer(uint32_t expectBufferDuration)
{
    int totalBufferSize = CURRENT_BIT_RATE * static_cast<int32_t>(expectBufferDuration);
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
}

HttpMediaDownloader::HttpMediaDownloader(std::string url, uint32_t expectBufferDuration)
{
    if (static_cast<int32_t>(url.find(".flv")) != -1) {
        MEDIA_LOG_I("isflv.");
        isFlv_ = true;
    }
    if (isFlv_) {
        InitRingBuffer(expectBufferDuration);
    } else {
        InitCacheBuffer(expectBufferDuration);
    }
    isBuffering_ = true;
    downloader_ = std::make_shared<Downloader>("http");
    steadyClock_.Reset();

    downloadTask_ = std::make_shared<Task>(std::string("HttpdownloadTask"), "", TaskType::SINGLETON);
    downloadTask_->RegisterJob([this] {
        CacheData();
        return 0;
    });
    wantReadLength_ = PLAY_WATER_LINE;
    downloadTask_->Start();
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
        double downloadTime = (static_cast<double>(nowTime) - static_cast<double>(startDownloadTime_)) /
            SECOND_TO_MICROSECOND;
        if (downloadTime > ZERO_THRESHOLD) {
            avgDownloadSpeed_ = totalBits_ / downloadTime;
        }
        MEDIA_LOG_D("Download done, average download speed: " PUBLIC_LOG_D32 " bit/s",
            static_cast<int32_t>(avgDownloadSpeed_));
        MEDIA_LOG_D("Download done, data usage: " PUBLIC_LOG_U64 " bits in " PUBLIC_LOG_D64 "ms",
            totalBits_, static_cast<int64_t>(downloadTime * SECOND_TO_MICROSECOND));
    };
    MediaSouce mediaSouce;
    mediaSouce.url = url;
    mediaSouce.httpHeader = httpHeader;
    downloadRequest_ = std::make_shared<DownloadRequest>(saveData, realStatusCallback, mediaSouce);
    downloadRequest_->SetDownloadDoneCb(downloadDoneCallback);
    downloader_->Download(downloadRequest_, -1); // -1
    if (isFlv_) {
        buffer_->SetMediaOffset(0);
    }
    downloader_->Start();
    return true;
}

void HttpMediaDownloader::Close(bool isAsync)
{
    if (isFlv_) {
        buffer_->SetActive(false);
    }
    isInterrupt_ = true;
    downloader_->Stop(isAsync);
    cvReadWrite_.NotifyOne();
    if (!isDownloadFinish_) {
        int64_t nowTime = steadyClock_.ElapsedMilliseconds();
        double downloadTime = (static_cast<double>(nowTime) - static_cast<double>(startDownloadTime_)) /
            SECOND_TO_MICROSECOND;
        if (downloadTime > ZERO_THRESHOLD) {
            avgDownloadSpeed_ = totalBits_ / downloadTime;
        }
        MEDIA_LOG_D("Download close, average download speed: " PUBLIC_LOG_D32 " bit/s",
            static_cast<int32_t>(avgDownloadSpeed_));
        MEDIA_LOG_D("Download close, data usage: " PUBLIC_LOG_U64 " bits in " PUBLIC_LOG_D64 "ms",
            totalBits_, static_cast<int64_t>(downloadTime * SECOND_TO_MICROSECOND));
    }
}

void HttpMediaDownloader::Pause()
{
    if (isFlv_) {
        bool cleanData = GetSeekable() != Seekable::SEEKABLE;
        buffer_->SetActive(false, cleanData);
    }
    isInterrupt_ = true;
    downloader_->Pause();
    cvReadWrite_.NotifyOne();
}

void HttpMediaDownloader::Resume()
{
    if (isFlv_) {
        buffer_->SetActive(true);
    }
    isInterrupt_ = false;
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

bool HttpMediaDownloader::HandleBuffering()
{
    MEDIA_LOG_I("HandleBuffering in.");
    int32_t sleepTime = 0;
    while (sleepTime < BUFFERING_TIME_OUT) {    // 1s
        if (!isBuffering_ || isInterruptNeeded_) {
            break;
        }
        OSAL::SleepFor(BUFFERING_SLEEP_TIME);
        sleepTime += BUFFERING_SLEEP_TIME;
    }
    MEDIA_LOG_I("HandleBuffering end.");
    return isBuffering_ || isInterruptNeeded_;
}

bool HttpMediaDownloader::StartDownloadTask()
{
    bool isEos = downloadRequest_->IsEos();
    if (isFirstFrameArrived_ && GetCurrentBufferSize() < PLAY_WATER_LINE && !isEos) {
        bufferingTimes_++;
        if (bufferingTimes_ == FIRST_TIMES) {
            wantReadLength_ = FIRST_CACHE_WATER_LINE;
        } else if (bufferingTimes_ == SECOND_TIMES) {
            wantReadLength_ = SECOND_CACHE_WATER_LINE;
        } else {
            wantReadLength_ = CACHE_WATER_LINE;
        }

        if (downloadTask_ != nullptr && !isBuffering_) {
            MEDIA_LOG_I("downloadTask_ start.");
            isBuffering_ = true;
            downloadTask_->Start();
            MEDIA_LOG_I("CacheData OnEvent BUFFERING_START.");
            callback_->OnEvent({PluginEventType::BUFFERING_START, {BufferingInfoType::BUFFERING_START}, "start"});
            return true;
        }
    }
    return false;
}

Status HttpMediaDownloader::ReadRingBuffer(unsigned char* buff, ReadDataInfo& readDataInfo)
{
    readDataInfo.isEos_ = false;
    readTime_ = 0;
    size_t fileContentLength = downloadRequest_->GetFileContentLength();
    uint64_t mediaOffset = buffer_->GetMediaOffset();
    if (fileContentLength > mediaOffset) {
        uint64_t remain = fileContentLength - mediaOffset;
        readDataInfo.wantReadLength_ = remain < readDataInfo.wantReadLength_ ? remain :
            readDataInfo.wantReadLength_;
    }
    while (buffer_->GetSize() < readDataInfo.wantReadLength_ && !isInterruptNeeded_.load()) {
        if (CheckIsEosRingBuffer(buff, readDataInfo)) {
            return Status::END_OF_STREAM;
        }
        if (readTime_ >= READ_SLEEP_TIME_OUT || downloadErrorState_ || isTimeOut_) {
            isTimeOut_ = true;
            if (downloader_ != nullptr) {
                buffer_->SetActive(false); // avoid deadlock caused by ringbuffer write stall
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
        if (isClosed && buffer_->GetSize() == 0) {
            MEDIA_LOG_D("HttpMediaDownloader read return, isClosed: " PUBLIC_LOG_D32, isClosed);
            readDataInfo.realReadLength_ = 0;
            return Status::END_OF_STREAM;
        }
        Task::SleepInTask(FIVE_MICROSECOND);
        readTime_ += FIVE_MICROSECOND;
    }
    FALSE_RETURN_V(!isInterruptNeeded_.load(), Status::OK);
    readDataInfo.realReadLength_ = buffer_->ReadBuffer(buff, readDataInfo.wantReadLength_, 2);  // wait 2 times
    MEDIA_LOG_D("Read: wantReadLength " PUBLIC_LOG_D32 ", realReadLength " PUBLIC_LOG_D32 ", isEos "
                PUBLIC_LOG_D32, readDataInfo.wantReadLength_, readDataInfo.realReadLength_, readDataInfo.isEos_);
    return Status::OK;
}

Status HttpMediaDownloader::ReadCacheBuffer(unsigned char* buff, ReadDataInfo& readDataInfo)
{
    size_t hasReadSize = 0;
    readTime_ = 0;
    while (hasReadSize < readDataInfo.wantReadLength_ && !isInterruptNeeded_.load()) {
        if (CheckIsEosBeforeTimeout(buff, readDataInfo)) {
            return Status::END_OF_STREAM;
        }
        if (readTime_ >= READ_SLEEP_TIME_OUT || downloadErrorState_) {
            return HandleDownloadErrorState(readDataInfo.realReadLength_);
        }
        bool isClosed = downloadRequest_->IsClosed();
        if (isClosed && cacheMediaBuffer_->GetBufferSize(readOffset_) == 0) {
            MEDIA_LOG_I("HttpMediaDownloader read return, isClosed: " PUBLIC_LOG_D32, isClosed);
            readDataInfo.realReadLength_ = 0;
            return Status::END_OF_STREAM;
        }
        auto size = cacheMediaBuffer_->Read(buff + hasReadSize, readOffset_ + hasReadSize,
            readDataInfo.wantReadLength_ - hasReadSize);
        if (size == 0) {
            Task::SleepInTask(FIVE_MICROSECOND); // 5
            readTime_ += FIVE_MICROSECOND;
        } else {
            hasReadSize += size;
        }
    }
    FALSE_RETURN_V(!isInterruptNeeded_.load(), Status::OK);
    readDataInfo.realReadLength_ = hasReadSize;
    readOffset_ += hasReadSize;
    canWrite_ = true;
    MEDIA_LOG_D("Read Success: wantReadLength " PUBLIC_LOG_D32 ", realReadLength " PUBLIC_LOG_D32 ", isEos "
        PUBLIC_LOG_D32 " readOffset_ " PUBLIC_LOG_ZU, readDataInfo.wantReadLength_,
        readDataInfo.realReadLength_, readDataInfo.isEos_, readOffset_);
    return Status::OK;
}

Status HttpMediaDownloader::Read(unsigned char* buff, ReadDataInfo& readDataInfo)
{
    if (isFlv_) {
        FALSE_RETURN_V_MSG(buffer_ != nullptr, Status::END_OF_STREAM, "buffer_ = nullptr");
        FALSE_RETURN_V_MSG(!isInterruptNeeded_.load(), Status::END_OF_STREAM, "isInterruptNeeded");
        FALSE_RETURN_V_MSG(readDataInfo.wantReadLength_ > 0, Status::END_OF_STREAM, "wantReadLength_ <= 0");
        if (isBuffering_) {
            if (HandleBuffering()) {
                MEDIA_LOG_I("Return error again.");
                return Status::ERROR_AGAIN;
            }
        }
        if (StartDownloadTask()) {
            return Status::ERROR_AGAIN;
        }
        return ReadRingBuffer(buff, readDataInfo);
    } else {
        FALSE_RETURN_V_MSG(cacheMediaBuffer_ != nullptr, Status::END_OF_STREAM, "cacheMediaBuffer_ = nullptr");
        FALSE_RETURN_V_MSG(!isInterruptNeeded_.load(), Status::END_OF_STREAM, "isInterruptNeeded");
        FALSE_RETURN_V_MSG(readDataInfo.wantReadLength_ > 0, Status::END_OF_STREAM, "wantReadLength_ <= 0");
        if (isBuffering_) {
            if (HandleBuffering()) {
                MEDIA_LOG_I("Return error again.");
                return Status::ERROR_AGAIN;
            }
        }
        if (StartDownloadTask()) {
            return Status::ERROR_AGAIN;
        }
        return ReadCacheBuffer(buff, readDataInfo);
    }
}

Status HttpMediaDownloader::HandleDownloadErrorState(unsigned int& realReadLength)
{
    if (downloader_ != nullptr) {
        downloader_->Pause(true); // the downloader is unavailable after this
    }
    if (downloader_ != nullptr && !downloadRequest_->IsClosed()) {
        downloadRequest_->Close();
    }
    OnClientErrorEvent();
    realReadLength = 0;
    MEDIA_LOG_D("DownloadErrorState, return eos");
    return Status::END_OF_STREAM;
}

bool HttpMediaDownloader::CheckIsEosRingBuffer(unsigned char* buff, ReadDataInfo& readDataInfo)
{
    if (downloadRequest_ != nullptr) {
        readDataInfo.isEos_ = downloadRequest_->IsEos();
    }
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

bool HttpMediaDownloader::CheckIsEosBeforeTimeout(unsigned char* buff, ReadDataInfo& readDataInfo)
{
    if (downloadRequest_ != nullptr) {
        readDataInfo.isEos_ = downloadRequest_->IsEos();
    }
    if (readDataInfo.isEos_ && cacheMediaBuffer_->GetBufferSize(readOffset_) == 0) {
        MEDIA_LOG_D("HttpMediaDownloader read return, isEos: " PUBLIC_LOG_D32, readDataInfo.isEos_);
        readDataInfo.realReadLength_ = 0;
        return true;
    }
    return false;
}

bool HttpMediaDownloader::HandleSeekHit(int64_t offset)
{
    MEDIA_LOG_D("Seek hit.");
    readOffset_ = static_cast<size_t>(offset);
    cacheMediaBuffer_->Seek(offset);

    size_t fileContentLength = downloadRequest_->GetFileContentLength();
    size_t downloadOffset = static_cast<size_t>(offset) + cacheMediaBuffer_->GetBufferSize(offset);
    if (downloadOffset > fileContentLength) {
        MEDIA_LOG_W("downloadOffset invalid, offset " PUBLIC_LOG_D64 " downloadOffset " PUBLIC_LOG_ZU
            " fileContentLength " PUBLIC_LOG_ZU, offset, downloadOffset, fileContentLength);
        return true;
    }
    if (writeOffset_ != downloadOffset) {
        isNeedDropData_ = true;
        downloader_->Pause();
        isNeedDropData_ = false;
        downloadOffset = static_cast<size_t>(offset) + cacheMediaBuffer_->GetBufferSize(offset);
        isHitSeeking_ = true;
        bool result = downloader_->Seek(downloadOffset);
        isHitSeeking_ = false;
        if (result) {
            writeOffset_ = downloadOffset;
        } else {
            MEDIA_LOG_E("Downloader seek fail.");
        }
        downloader_->Resume();
    } else {
        MEDIA_LOG_D("Seek hit, continue download.");
    }
    MEDIA_LOG_D("Seek out.");
    return true;
}

bool HttpMediaDownloader::SeekRingBuffer(int64_t offset)
{
    FALSE_RETURN_V(buffer_ != nullptr, false);
    MEDIA_LOG_I("Seek in, buffer size " PUBLIC_LOG_ZU ", offset " PUBLIC_LOG_D64, buffer_->GetSize(), offset);
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
        MEDIA_LOG_D("Seek failed");
    }
    downloader_->Resume();
    return result;
}

bool HttpMediaDownloader::SeekCacheBuffer(int64_t offset)
{
    size_t remain = cacheMediaBuffer_->GetBufferSize(offset);
    MEDIA_LOG_D("Seek: buffer size " PUBLIC_LOG_ZU ", offset " PUBLIC_LOG_D64, remain, offset);
    if (remain > 0) {
        return HandleSeekHit(offset);
    }
    MEDIA_LOG_D("Seek miss.");

    isNeedClean_ = true;
    downloader_->Pause();
    isNeedClean_ = false;
    size_t downloadEndOffset = cacheMediaBuffer_->GetNextBufferOffset(offset);
    if (downloadEndOffset > offset) {
        downloader_->SetRequestSize(downloadEndOffset - offset);
    }
    bool result = false;
    result = downloader_->Seek(offset);
    if (result) {
        writeOffset_ = static_cast<size_t>(offset);
        cacheMediaBuffer_->Seek(offset);
        readOffset_ = offset;
        downloader_->Resume();
        MEDIA_LOG_D("Seek out.");
        return true;
    } else {
        downloader_->Resume();
        MEDIA_LOG_D("Download seek fail.");
        return false;
    }
    return false;
}

bool HttpMediaDownloader::SeekToPos(int64_t offset)
{
    if (isFlv_) {
        return SeekRingBuffer(offset);
    } else {
        return SeekCacheBuffer(offset);
    }
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
    if (isFlv_) {
        FALSE_RETURN(buffer_ != nullptr);
        buffer_->SetReadBlocking(isReadBlockingAllowed);
    } else {
        MEDIA_LOG_W("SetReadBlockingFlag is unimplemented.");
    }
}

bool HttpMediaDownloader::SaveRingBufferData(uint8_t* data, uint32_t len)
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

bool HttpMediaDownloader::SaveData(uint8_t* data, uint32_t len)
{
    if (isFlv_) {
        return SaveRingBufferData(data, len);
    }
    if (isNeedClean_) {
        return true;
    }
    size_t hasWriteSize = 0;
    while (hasWriteSize < len && !isInterruptNeeded_.load()) {
        if (isNeedClean_) {
            MEDIA_LOG_D("isNeedClean true.");
            return true;
        }
        size_t res = cacheMediaBuffer_->Write(data + hasWriteSize, writeOffset_, len - hasWriteSize);
        writeOffset_ += res;
        hasWriteSize += res;
        MEDIA_LOG_D("writeOffset " PUBLIC_LOG_ZU " res " PUBLIC_LOG_ZU, writeOffset_, res);
        if (res > 0) {
            continue;
        }
        MEDIA_LOG_W("CacheMediaBuffer write fail.");
        canWrite_ = false;
        while (!isNeedClean_ && !canWrite_ && !isInterruptNeeded_.load()) {
            MEDIA_LOG_D("CacheMediaBuffer can not write, drop data.");
            if (isHitSeeking_ || isNeedDropData_) {
                return true;
            }
            OSAL::SleepFor(ONE_HUNDRED_MICROSECOND);
        }
    }
    if (isInterruptNeeded_.load()) {
        MEDIA_LOG_D("isInterruptNeeded true, return false.");
        return false;
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
            double downloadDuration = static_cast<double>(now - lastCheckTime_) / SECOND_TO_MICROSECOND;
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
        if (isFlv_) {
            if (buffer_ != nullptr) {
                uint64_t remainingBuffer = buffer_->GetSize() * 8;
                MEDIA_LOG_D("The remaining of the buffer : " PUBLIC_LOG_U64, remainingBuffer);
            }
        } else {
            if (cacheMediaBuffer_ != nullptr) {
                uint64_t remainingBuffer = cacheMediaBuffer_->GetBufferSize(readOffset_) * 8;
                MEDIA_LOG_D("The remaining of the buffer : " PUBLIC_LOG_U64, remainingBuffer);
            }
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
    isFirstFrameArrived_ = true;
}

void HttpMediaDownloader::SetDownloadErrorState()
{
    MEDIA_LOG_I("SetDownloadErrorState");
    downloadErrorState_ = true;
    if (isFlv_) {
        if (buffer_ != nullptr && buffer_->GetSize() == 0) {
            Close(true);
        }
    } else {
        if (cacheMediaBuffer_ != nullptr && cacheMediaBuffer_->GetBufferSize(readOffset_) == 0) {
            Close(true);
        }
    }
}

void HttpMediaDownloader::SetInterruptState(bool isInterruptNeeded)
{
    isInterruptNeeded_ = isInterruptNeeded;
    if (buffer_ != nullptr && isInterruptNeeded) {
        buffer_->SetActive(false);
    }
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
    return isFirstFrameArrived_;
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

bool HttpMediaDownloader::HandleBreak()
{
    bool isEos = false;
    if (downloadRequest_ != nullptr) {
        isEos = downloadRequest_->IsEos();
    }
    if (isEos && GetCurrentBufferSize() == 0) {
        MEDIA_LOG_I("CacheData over, isEos: " PUBLIC_LOG_D32, isEos);
        isBuffering_ = false;
        return true;
    }
    if (downloadErrorState_) {
        MEDIA_LOG_I("downloadErrorState_ break");
        isBuffering_ = false;
        return true;
    }
    bool isClosed = false;
    if (downloadRequest_ != nullptr) {
        isClosed = downloadRequest_->IsClosed();
    }
    if (isClosed && GetCurrentBufferSize() == 0) {
        MEDIA_LOG_I("isClosed break");
        isBuffering_ = false;
        return true;
    }
    return false;
}

void HttpMediaDownloader::CacheData()
{
    MEDIA_LOG_I("CacheData in");
    while (GetCurrentBufferSize() < wantReadLength_ && !isInterrupt_) {
        if (downloadRequest_ == nullptr) {
            OSAL::SleepFor(REQUEST_SLEEP_TIME);
            continue;
        }
        if (HandleBreak()) {
            break;
        }
        OSAL::SleepFor(CACHEDATA_SLEEP_TIME);
    }
    if (!isReadFrame_) {
        isReadFrame_ = true;
        MEDIA_LOG_I("CacheData play start");
    } else {
        MEDIA_LOG_I("CacheData OnEvent BUFFERING_END");
        callback_->OnEvent({PluginEventType::BUFFERING_END, {BufferingInfoType::BUFFERING_END}, "end"});
    }

    isBuffering_ = false;
    if (downloadTask_ != nullptr) {
        MEDIA_LOG_I("downloadTask_ PauseAsync, GetSize: " PUBLIC_LOG_ZU, GetCurrentBufferSize());
        downloadTask_->PauseAsync();
    }
    MEDIA_LOG_I("CacheData out");
}

size_t HttpMediaDownloader::GetCurrentBufferSize()
{
    size_t bufferSize = 0;
    if (isFlv_) {
        bufferSize = buffer_->GetSize();
    } else {
        bufferSize = cacheMediaBuffer_->GetBufferSize(readOffset_);
    }
    return bufferSize;
}
}
}
}
}