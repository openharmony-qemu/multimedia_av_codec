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
constexpr int MAX_BUFFER_SIZE = 19 * 1024 * 1024;
constexpr int WATER_LINE = 8192; //  WATER_LINE:8192
constexpr int CURRENT_BIT_RATE = 1 * 1024 * 1024;
#endif
constexpr uint32_t SAMPLE_INTERVAL = 1000; // Sample time interval, ms
constexpr int START_PLAY_WATER_LINE = 512 * 1024;
constexpr int DATA_USAGE_NTERVAL = 300 * 1000;
constexpr double ZERO_THRESHOLD = 1e-9;
constexpr size_t PLAY_WATER_LINE = 5 * 1024;
constexpr size_t DEFAULT_WATER_LINE_ABOVE = 48 * 10 * 1024;
constexpr int64_t SECOND_TO_MILLIONSECOND = 1000;
constexpr int FIVE_MICROSECOND = 5;
constexpr int ONE_HUNDRED_MILLIONSECOND = 100;
constexpr uint32_t READ_SLEEP_TIME_OUT = 30 * 1000;
constexpr int32_t SAVE_DATA_LOG_FEQUENCE = 10;
constexpr int IS_DOWNLOAD_MIN_BIT = 100; // Determine whether it is downloading
constexpr float DEFAULT_CACHE_TIME = 0.3;
constexpr uint32_t DURATION_CHANGE_AMOUT_MILLIONSECOND = 500;
constexpr int64_t BYTES_TO_BIT = 8;
constexpr int32_t DEFAULT_BIT_RATE = 1638400;
constexpr int UPDATE_CACHE_STEP = 5 * 1024;
constexpr size_t MIN_WATER_LINE_ABOVE = 10 * 1024;
constexpr int32_t ONE_SECONDS = 1000;
constexpr int32_t TEN_MILLISECONDS = 10;
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
    waterLineAbove_ = PLAY_WATER_LINE;
    recordData_ = std::make_shared<RecordData>();
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
    waterLineAbove_ = PLAY_WATER_LINE;
    recordData_ = std::make_shared<RecordData>();
}

HttpMediaDownloader::~HttpMediaDownloader()
{
    MEDIA_LOG_I("%{public}p ~HttpMediaDownloader dtor", this);
    Close(false);
}

bool HttpMediaDownloader::Open(const std::string& url, const std::map<std::string, std::string>& httpHeader)
{
    MEDIA_LOG_I("Open download");
    isDownloadFinish_ = false;
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
        if (downloadRequest_ != nullptr && downloadRequest_->IsEos()
            && (totalBits_ / 8) >= downloadRequest_->GetFileContentLengthNoWait()) {
            isDownloadFinish_ = true;
            int64_t nowTime = steadyClock_.ElapsedMilliseconds();
            double downloadTime = (static_cast<double>(nowTime) - static_cast<double>(startDownloadTime_)) /
                SECOND_TO_MILLIONSECOND;
            if (downloadTime > ZERO_THRESHOLD) {
                avgDownloadSpeed_ = totalBits_ / downloadTime;
            }
            MEDIA_LOG_D("Download done, average download speed: " PUBLIC_LOG_D32 " bit/s",
                static_cast<int32_t>(avgDownloadSpeed_));
            MEDIA_LOG_I("Download done, data usage: " PUBLIC_LOG_U64 " bits in " PUBLIC_LOG_D64 "ms",
                totalBits_, static_cast<int64_t>(downloadTime * SECOND_TO_MILLIONSECOND));
        }
        HandleBuffering();
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
            SECOND_TO_MILLIONSECOND;
        if (downloadTime > ZERO_THRESHOLD) {
            avgDownloadSpeed_ = totalBits_ / downloadTime;
        }
        MEDIA_LOG_D("Download close, average download speed: " PUBLIC_LOG_D32 " bit/s",
            static_cast<int32_t>(avgDownloadSpeed_));
        MEDIA_LOG_D("Download close, data usage: " PUBLIC_LOG_U64 " bits in " PUBLIC_LOG_D64 "ms",
            totalBits_, static_cast<int64_t>(downloadTime * SECOND_TO_MILLIONSECOND));
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
    if (!isBuffering_ || downloadRequest_->IsChunkedVod()) {
        return false;
    }
    UpdateCachedPercent(BufferingInfoType::BUFFERING_PERCENT);
    size_t fileRemain = 0;
    size_t fileContenLen = downloadRequest_->GetFileContentLength();
    if (fileContenLen > readOffset_) {
        fileRemain = fileContenLen - readOffset_;
        waterLineAbove_ = std::min(fileRemain, waterLineAbove_);
    }
    if (!canWrite_) {
        MEDIA_LOG_I("canWrite_ false");
        isBuffering_ = false;
    }
    if (GetCurrentBufferSize() >= waterLineAbove_) {
        MEDIA_LOG_I("Buffer is enough");
        isBuffering_ = false;
    }
    if (HandleBreak()) {
        MEDIA_LOG_I("HandleBreak");
        isBuffering_ = false;
    }

    if (!isBuffering_ && isFirstFrameArrived_ && callback_ != nullptr) {
        MEDIA_LOG_I("CacheData onEvent BUFFERING_END");
        UpdateCachedPercent(BufferingInfoType::BUFFERING_END);
        callback_->OnEvent({PluginEventType::BUFFERING_END, {BufferingInfoType::BUFFERING_END}, "end"});
    }
    MEDIA_LOG_D("HandleBuffering bufferSize: " PUBLIC_LOG_ZU ", waterLineAbove_: " PUBLIC_LOG_ZU
        ", isBuffering: " PUBLIC_LOG_D32 ", canWrite: " PUBLIC_LOG_D32,
        GetCurrentBufferSize(), waterLineAbove_, isBuffering_, canWrite_.load());
    return isBuffering_;
}

bool HttpMediaDownloader::StartBuffering(int32_t wantReadLength)
{
    if (!canWrite_ || downloadRequest_->IsChunkedVod() || wantReadLength <= 0 || callback_ == nullptr) {
        return false;
    }
    size_t cacheWaterLine = 0;
    size_t fileRemain = 0;
    size_t fileContenLen = downloadRequest_->GetFileContentLength();
    cacheWaterLine = std::max(static_cast<size_t>(wantReadLength), PLAY_WATER_LINE);
    if (fileContenLen > readOffset_) {
        fileRemain = fileContenLen - readOffset_;
        cacheWaterLine = std::min(fileRemain, cacheWaterLine);
    }

    bool isEos = false;
    if (downloadRequest_ != nullptr) {
        isEos = downloadRequest_->IsEos();
    }
    if (isFirstFrameArrived_ && GetCurrentBufferSize() < cacheWaterLine && !isEos && !HandleBreak()) {
        waterLineAbove_ = std::max(MIN_WATER_LINE_ABOVE, static_cast<size_t>(GetWaterLineAbove()));
        waterLineAbove_ = std::min(waterLineAbove_, fileRemain);

        if (!isBuffering_) {
            MEDIA_LOG_I("readOffset " PUBLIC_LOG_ZU " bufferSize " PUBLIC_LOG_ZU " wantReadLength " PUBLIC_LOG_ZU,
                readOffset_, GetCurrentBufferSize(), waterLineAbove_);
            isBuffering_ = true;
            MEDIA_LOG_I("CacheData OnEvent BUFFERING_START, waterLineAbove: " PUBLIC_LOG_ZU, waterLineAbove_);
            UpdateCachedPercent(BufferingInfoType::BUFFERING_START);
            callback_->OnEvent({PluginEventType::BUFFERING_START, {BufferingInfoType::BUFFERING_START}, "start"});
            return true;
        }
    }
    return false;
}

Status HttpMediaDownloader::ReadRingBuffer(unsigned char* buff, ReadDataInfo& readDataInfo)
{
    readTime_ = 0;
    size_t fileContentLength = downloadRequest_->GetFileContentLength();
    uint64_t mediaOffset = buffer_->GetMediaOffset();
    if (fileContentLength > mediaOffset) {
        uint64_t remain = fileContentLength - mediaOffset;
        readDataInfo.wantReadLength_ = remain < readDataInfo.wantReadLength_ ? remain :
            readDataInfo.wantReadLength_;
    }
    int64_t startTime = steadyClock_.ElapsedMilliseconds();
    readDataInfo.realReadLength_ = 0;
    while (buffer_->GetSize() < readDataInfo.wantReadLength_ && !isInterruptNeeded_.load()) {
        if (downloadRequest_ != nullptr) {
            readDataInfo.isEos_ = downloadRequest_->IsEos();
        }
        if (readDataInfo.isEos_) {
            return CheckIsEosRingBuffer(buff, readDataInfo);
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
            MEDIA_LOG_I("HttpMediaDownloader read return, isClosed: " PUBLIC_LOG_D32, isClosed);
            readDataInfo.realReadLength_ = 0;
            return Status::END_OF_STREAM;
        }
        Task::SleepInTask(FIVE_MICROSECOND);
        int64_t endTime = steadyClock_.ElapsedMilliseconds();
        readTime_ = static_cast<uint64_t>(endTime - startTime);
    }
    if (isInterruptNeeded_.load()) {
        readDataInfo.realReadLength_ = 0;
        return Status::END_OF_STREAM;
    }
    readDataInfo.realReadLength_ = buffer_->ReadBuffer(buff, readDataInfo.wantReadLength_, 2);  // wait 2 times
    MEDIA_LOG_D("Read: wantReadLength " PUBLIC_LOG_D32 ", realReadLength " PUBLIC_LOG_D32 ", isEos "
                PUBLIC_LOG_D32, readDataInfo.wantReadLength_, readDataInfo.realReadLength_, readDataInfo.isEos_);
    return Status::OK;
}

Status HttpMediaDownloader::ReadCacheBuffer(unsigned char* buff, ReadDataInfo& readDataInfo)
{
    size_t remain = cacheMediaBuffer_->GetBufferSize(readOffset_);
    // This prevents the read operation from failing to read data when the seek operation is not triggered.
    if (remain < readDataInfo.wantReadLength_ && isServerAcceptRange_ &&
        (writeOffset_ < readOffset_ || writeOffset_ >= readOffset_ + remain)) {
        ChangeDownloadPos();
    }
    size_t hasReadSize = 0;
    readTime_ = 0;
    int64_t startTime = steadyClock_.ElapsedMilliseconds();
    while (hasReadSize < readDataInfo.wantReadLength_ && !isInterruptNeeded_.load()) {
        if (downloadRequest_ != nullptr) {
            readDataInfo.isEos_ = downloadRequest_->IsEos();
        }
        if (readDataInfo.isEos_) {
            return CheckIsEosCacheBuffer(buff, readDataInfo);
        }
        if (readTime_ >= READ_SLEEP_TIME_OUT || downloadErrorState_) {
            return HandleDownloadErrorState(readDataInfo.realReadLength_);
        }
        bool isClosed = downloadRequest_->IsClosed();
        if (isClosed && cacheMediaBuffer_->GetBufferSize(readOffset_) == 0) {
            MEDIA_LOG_D("HttpMediaDownloader read return, isClosed: " PUBLIC_LOG_D32, isClosed);
            readDataInfo.realReadLength_ = 0;
            return Status::END_OF_STREAM;
        }
        auto size = cacheMediaBuffer_->Read(buff + hasReadSize, readOffset_ + hasReadSize,
            readDataInfo.wantReadLength_ - hasReadSize);
        if (size == 0) {
            Task::SleepInTask(FIVE_MICROSECOND); // 5
        } else {
            hasReadSize += size;
        }
        int64_t endTime = steadyClock_.ElapsedMilliseconds();
        readTime_ = static_cast<uint64_t>(endTime - startTime);
    }
    if (isInterruptNeeded_.load()) {
        readDataInfo.realReadLength_ = hasReadSize;
        return Status::END_OF_STREAM;
    }
    readDataInfo.realReadLength_ = hasReadSize;
    readOffset_ += hasReadSize;
    canWrite_ = true;
    MEDIA_LOG_D("Read Success: wantReadLength " PUBLIC_LOG_D32 ", realReadLength " PUBLIC_LOG_D32 ", isEos "
        PUBLIC_LOG_D32 " readOffset_ " PUBLIC_LOG_ZU, readDataInfo.wantReadLength_,
        readDataInfo.realReadLength_, readDataInfo.isEos_, readOffset_);
    return Status::OK;
}

Status HttpMediaDownloader::ReadDelegate(unsigned char* buff, ReadDataInfo& readDataInfo)
{
    if (isFlv_) {
        FALSE_RETURN_V_MSG(buffer_ != nullptr, Status::END_OF_STREAM, "buffer_ = nullptr");
        FALSE_RETURN_V_MSG(!isInterruptNeeded_.load(), Status::END_OF_STREAM, "isInterruptNeeded");
        FALSE_RETURN_V_MSG(readDataInfo.wantReadLength_ > 0, Status::END_OF_STREAM, "wantReadLength_ <= 0");
        if (isBuffering_ && !downloadRequest_->IsChunkedVod() && CheckBufferingOneSeconds()) {
            MEDIA_LOG_I("Return error again.");
            return Status::ERROR_AGAIN;
        }
        if (StartBuffering(readDataInfo.wantReadLength_)) {
            return Status::ERROR_AGAIN;
        }
        return ReadRingBuffer(buff, readDataInfo);
    } else {
        FALSE_RETURN_V_MSG(cacheMediaBuffer_ != nullptr, Status::END_OF_STREAM, "cacheMediaBuffer_ = nullptr");
        FALSE_RETURN_V_MSG(!isInterruptNeeded_.load(), Status::END_OF_STREAM, "isInterruptNeeded");
        FALSE_RETURN_V_MSG(readDataInfo.wantReadLength_ > 0, Status::END_OF_STREAM, "wantReadLength_ <= 0");
        if (isBuffering_ && !downloadRequest_->IsChunkedVod() && canWrite_ && CheckBufferingOneSeconds()) {
            MEDIA_LOG_I("Return error again.");
            return Status::ERROR_AGAIN;
        }
        if (StartBuffering(readDataInfo.wantReadLength_)) {
            return Status::ERROR_AGAIN;
        }
        return ReadCacheBuffer(buff, readDataInfo);
    }
}

Status HttpMediaDownloader::Read(unsigned char* buff, ReadDataInfo& readDataInfo)
{
    uint64_t now = static_cast<uint64_t>(steadyClock_.ElapsedMilliseconds());
    auto ret = ReadDelegate(buff, readDataInfo);
    readTotalBytes_ += readDataInfo.realReadLength_;
    if (now > lastReadCheckTime_ && now - lastReadCheckTime_ > SAMPLE_INTERVAL) {
        readRecordDuringTime_ = now - lastReadCheckTime_;   // ms
        double readDuration = static_cast<double>(readRecordDuringTime_) / SECOND_TO_MILLIONSECOND; // s
        if (readDuration > ZERO_THRESHOLD) {
            double readSpeed = static_cast<double>(readTotalBytes_ * BYTES_TO_BIT) / readDuration; // bps
            currentBitrate_ = static_cast<uint64_t>(readSpeed);     // bps
            size_t curBufferSize = GetCurrentBufferSize();
            MEDIA_LOG_D("Current read speed: " PUBLIC_LOG_D32 " Kbit/s,Current buffer size: " PUBLIC_LOG_U64
            " KByte", static_cast<int32_t>(readSpeed / 1024), static_cast<uint64_t>(curBufferSize / 1024));
            readTotalBytes_ = 0;
        }
        lastReadCheckTime_ = now;
        readRecordDuringTime_ = 0;
    }
    
    return ret;
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
    MEDIA_LOG_I("DownloadErrorState, return eos");
    return Status::END_OF_STREAM;
}

Status HttpMediaDownloader::CheckIsEosRingBuffer(unsigned char* buff, ReadDataInfo& readDataInfo)
{
    if (buffer_->GetSize() == 0) {
        MEDIA_LOG_I("HttpMediaDownloader read return, isEos: " PUBLIC_LOG_D32, readDataInfo.isEos_);
        return readDataInfo.realReadLength_ == 0 ? Status::END_OF_STREAM : Status::OK;
    } else {
        readDataInfo.realReadLength_ = buffer_->ReadBuffer(buff, readDataInfo.wantReadLength_, 2); // 2
        return Status::OK;
    }
}

Status HttpMediaDownloader::CheckIsEosCacheBuffer(unsigned char* buff, ReadDataInfo& readDataInfo)
{
    if (cacheMediaBuffer_->GetBufferSize(readOffset_) == 0) {
        MEDIA_LOG_I("HttpMediaDownloader read return, isEos: " PUBLIC_LOG_D32, readDataInfo.isEos_);
        return readDataInfo.realReadLength_ == 0 ? Status::END_OF_STREAM : Status::OK;
    } else {
        canWrite_ = true;
        readDataInfo.realReadLength_ = cacheMediaBuffer_->Read(buff, readOffset_, readDataInfo.wantReadLength_);
        readOffset_ += readDataInfo.realReadLength_;
        return Status::OK;
    }
}

void HttpMediaDownloader::ChangeDownloadPos()
{
    MEDIA_LOG_D("ChangeDownloadPos in.");

    if (!canWrite_) {
        MEDIA_LOG_I("HTTP CacheMediaBuffer clear.");
        isNeedDropData_ = true;
        downloader_->Pause();
        cacheMediaBuffer_->Clear();
    } else {
        isNeedDropData_ = true;
        downloader_->Pause();
    }
    
    isNeedDropData_ = false;
    size_t downloadOffset = static_cast<size_t>(readOffset_) + cacheMediaBuffer_->GetBufferSize(readOffset_);
    isHitSeeking_ = true;
    bool result = downloader_->Seek(downloadOffset);
    isHitSeeking_ = false;
    if (result) {
        writeOffset_ = downloadOffset;
    } else {
        MEDIA_LOG_E("Downloader seek fail.");
    }
    downloader_->Resume();
    MEDIA_LOG_D("ChangeDownloadPos out.");
}

bool HttpMediaDownloader::HandleSeekHit(int64_t offset)
{
    MEDIA_LOG_D("Seek hit.");
    readOffset_ = static_cast<size_t>(offset);
    cacheMediaBuffer_->Seek(offset);

    if (!isServerAcceptRange_) {
        MEDIA_LOG_D("Don't support range, return true.");
        return true;
    }

    size_t fileContentLength = downloadRequest_->GetFileContentLength();
    size_t downloadOffset = static_cast<size_t>(offset) + cacheMediaBuffer_->GetBufferSize(offset);
    if (downloadOffset > fileContentLength) {
        MEDIA_LOG_W("downloadOffset invalid, offset " PUBLIC_LOG_D64 " downloadOffset " PUBLIC_LOG_ZU
            " fileContentLength " PUBLIC_LOG_ZU, offset, downloadOffset, fileContentLength);
        return true;
    }
    if (writeOffset_ != downloadOffset && cacheMediaBuffer_->GetBufferSize(offset) < DEFAULT_WATER_LINE_ABOVE) {
        ChangeDownloadPos();
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
    cacheMediaBuffer_->Seek(offset);
    readOffset_ = static_cast<size_t>(offset);

    if (!isServerAcceptRange_) {
        MEDIA_LOG_D("Don't support range, return true.");
        return true;
    }

    isNeedClean_ = true;
    downloader_->Pause();
    isNeedClean_ = false;
    size_t downloadEndOffset = cacheMediaBuffer_->GetNextBufferOffset(offset);
    if (downloadEndOffset > static_cast<size_t>(offset)) {
        downloader_->SetRequestSize(downloadEndOffset - offset);
    }
    bool result = false;
    result = downloader_->Seek(offset);
    if (result) {
        writeOffset_ = static_cast<size_t>(offset);
        MEDIA_LOG_D("Seek out.");
    } else {
        MEDIA_LOG_D("Download seek fail.");
    }
    downloader_->Resume();
    return result;
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
    }
}

bool HttpMediaDownloader::SaveRingBufferData(uint8_t* data, uint32_t len)
{
    FALSE_RETURN_V(buffer_->WriteBuffer(data, len), false);
    OnWriteBuffer(len);
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
    bool ret = true;
    if (isFlv_) {
        ret = SaveRingBufferData(data, len);
    } else {
        ret = SaveCacheBufferData(data, len);
    }
    HandleBuffering();
    return ret;
}

bool HttpMediaDownloader::SaveCacheBufferData(uint8_t* data, uint32_t len)
{
    OnWriteBuffer(len);
    if (isNeedClean_) {
        return true;
    }

    isServerAcceptRange_ = downloadRequest_->IsServerAcceptRange();

    size_t hasWriteSize = 0;
    while (hasWriteSize < len && !isInterruptNeeded_.load() && !isInterrupt_) {
        if (isNeedClean_) {
            MEDIA_LOG_D("isNeedClean true.");
            return true;
        }
        size_t res = cacheMediaBuffer_->Write(data + hasWriteSize, writeOffset_, len - hasWriteSize);
        writeOffset_ += res;
        hasWriteSize += res;
        MEDIA_LOG_D("writeOffset " PUBLIC_LOG_ZU " res " PUBLIC_LOG_ZU, writeOffset_, res);
        if (res > 0 || hasWriteSize == len) {
            HandleCachedDuration();
            continue;
        }
        MEDIA_LOG_W("CacheMediaBuffer full.");
        canWrite_ = false;
        HandleBuffering();
        while (!isInterrupt_ && !isNeedClean_ && !canWrite_ && !isInterruptNeeded_.load()) {
            MEDIA_LOGI_LIMIT(SAVE_DATA_LOG_FEQUENCE, "CacheMediaBuffer full, waiting seek or read.");
            if (isHitSeeking_ || isNeedDropData_) {
                canWrite_ = true;
                return true;
            }
            OSAL::SleepFor(ONE_HUNDRED_MILLIONSECOND);
        }
        canWrite_ = true;
    }
    if (isInterruptNeeded_.load() || isInterrupt_) {
        MEDIA_LOG_D("isInterruptNeeded true, return false.");
        return false;
    }
    return true;
}

void HttpMediaDownloader::OnWriteBuffer(uint32_t len)
{
    if (startDownloadTime_ == 0) {
        int64_t nowTime = steadyClock_.ElapsedMilliseconds();
        startDownloadTime_ = nowTime;
        lastReportUsageTime_ = nowTime;
    }
    uint32_t writeBits = len * BYTES_TO_BIT;
    totalBits_ += writeBits;
    dataUsage_ += writeBits;
    if ((totalBits_ > START_PLAY_WATER_LINE) && (playDelayTime_ == 0)) {
        auto startPlayTime = steadyClock_.ElapsedMilliseconds();
        playDelayTime_ = startPlayTime - openTime_;
        MEDIA_LOG_D("Start play delay time: " PUBLIC_LOG_D64, playDelayTime_);
    }
    DownloadReport();
}

double HttpMediaDownloader::CalculateCurrentDownloadSpeed()
{
    double downloadRate = 0;
    double tmpNumerator = static_cast<double>(downloadBits_);
    double tmpDenominator = static_cast<double>(downloadDuringTime_) / SECOND_TO_MILLIONSECOND;
    totalDownloadDuringTime_ += downloadDuringTime_;
    if (tmpDenominator > ZERO_THRESHOLD) {
        downloadRate = tmpNumerator / tmpDenominator;
        avgDownloadSpeed_ = downloadRate;
        downloadDuringTime_ = 0;
        downloadBits_ = 0;
    }
    return downloadRate;
}

void HttpMediaDownloader::DownloadReport()
{
    uint64_t now = static_cast<uint64_t>(steadyClock_.ElapsedMilliseconds());
    if ((static_cast<int64_t>(now) - lastCheckTime_) > static_cast<int64_t>(SAMPLE_INTERVAL)) {
        uint64_t curDownloadBits = totalBits_ - lastBits_;
        if (curDownloadBits >= IS_DOWNLOAD_MIN_BIT) {
            downloadDuringTime_ = now - static_cast<uint64_t>(lastCheckTime_);
            downloadBits_ = curDownloadBits;
            double downloadRate = CalculateCurrentDownloadSpeed();
            // remaining buffer size
            size_t remainingBuffer = GetCurrentBufferSize();    // Byte
            MEDIA_LOG_D("Current download speed : " PUBLIC_LOG_D32 " Kbit/s,Current buffer size : " PUBLIC_LOG_U64
                " KByte", static_cast<int32_t>(downloadRate / 1024), static_cast<uint64_t>(remainingBuffer / 1024));
            // Remaining playable time: s
            uint64_t bufferDuration = 0;
            if (currentBitrate_ > 0) {
                bufferDuration = static_cast<uint64_t>(remainingBuffer * BYTES_TO_BIT) / currentBitrate_;
            } else {
                bufferDuration = static_cast<uint64_t>(remainingBuffer * BYTES_TO_BIT) / CURRENT_BIT_RATE;
            }
            if (recordData_ != nullptr) {
                recordData_->downloadRate = downloadRate;
                recordData_->bufferDuring = bufferDuration;
            }
        }
        lastBits_ = totalBits_;
        lastCheckTime_ = static_cast<int64_t>(now);
    }

    if (!isDownloadFinish_ && (static_cast<int64_t>(now) - lastReportUsageTime_) > DATA_USAGE_NTERVAL) {
        MEDIA_LOG_D("Data usage: " PUBLIC_LOG_U64 " bits in " PUBLIC_LOG_D32 "ms", dataUsage_, DATA_USAGE_NTERVAL);
        dataUsage_ = 0;
        lastReportUsageTime_ = static_cast<int64_t>(now);
    }
}

void HttpMediaDownloader::SetDemuxerState(int32_t streamId)
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
    if (downloader_ != nullptr) {
        downloader_->SetInterruptState(isInterruptNeeded);
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
        downloadInfo.avgDownloadRate = avgSpeedSum_ / recordSpeedCount_;
    }
    downloadInfo.avgDownloadSpeed = avgDownloadSpeed_;
    downloadInfo.totalDownLoadBits = totalBits_;
    downloadInfo.isTimeOut = isTimeOut_;
}

std::pair<int32_t, int32_t> HttpMediaDownloader::GetDownloadInfo()
{
    MEDIA_LOG_I("HttpMediaDownloader::GetDownloadInfo.");
    if (recordSpeedCount_ == 0) {
        MEDIA_LOG_E("recordSpeedCount_ is 0, can't get avgDownloadRate");
        return std::make_pair(0, static_cast<int32_t>(avgDownloadSpeed_));
    }
    auto rateAndSpeed = std::make_pair(avgSpeedSum_ / recordSpeedCount_, static_cast<int32_t>(avgDownloadSpeed_));
    return rateAndSpeed;
}

std::pair<int32_t, int32_t> HttpMediaDownloader::GetDownloadRateAndSpeed()
{
    MEDIA_LOG_I("HttpMediaDownloader::GetDownloadRateAndSpeed.");
    if (recordSpeedCount_ == 0) {
        MEDIA_LOG_E("recordSpeedCount_ is 0, can't get avgDownloadRate");
        return std::make_pair(0, static_cast<int32_t>(avgDownloadSpeed_));
    }
    auto rateAndSpeed = std::make_pair(avgSpeedSum_ / recordSpeedCount_, static_cast<int32_t>(avgDownloadSpeed_));
    return rateAndSpeed;
}

void HttpMediaDownloader::GetPlaybackInfo(PlaybackInfo& playbackInfo)
{
    if (downloader_ != nullptr) {
        downloader_->GetIp(playbackInfo.serverIpAddress);
    }
    double tmpDownloadTime = static_cast<double>(totalDownloadDuringTime_) / SECOND_TO_MILLIONSECOND;
    if (tmpDownloadTime > ZERO_THRESHOLD) {
        playbackInfo.averageDownloadRate = static_cast<int64_t>(totalBits_ / tmpDownloadTime);
    } else {
        playbackInfo.averageDownloadRate = 0;
    }
    playbackInfo.isDownloading = isDownloadFinish_ ? false : true;
    if (recordData_ != nullptr) {
        playbackInfo.downloadRate = static_cast<int64_t>(recordData_->downloadRate);
        size_t remainingBuffer = GetCurrentBufferSize();
        uint64_t bufferDuration = 0;
        if (currentBitrate_ > 0) {
            bufferDuration = static_cast<uint64_t>(remainingBuffer * BYTES_TO_BIT) / currentBitrate_;
        } else {
            bufferDuration = static_cast<uint64_t>(remainingBuffer * BYTES_TO_BIT) / CURRENT_BIT_RATE;
        }
        playbackInfo.bufferDuration = static_cast<int64_t>(bufferDuration);
    } else {
        playbackInfo.downloadRate = 0;
        playbackInfo.bufferDuration = 0;
    }
}

bool HttpMediaDownloader::HandleBreak()
{
    if (downloadErrorState_) {
        MEDIA_LOG_I("downloadErrorState_ break");
        return true;
    }
    if (downloadRequest_ == nullptr) {
        MEDIA_LOG_I("downloadRequest is nullptr");
        return true;
    }
    if (downloadRequest_->IsEos()) {
        MEDIA_LOG_I("CacheData over, isEos");
        return true;
    }
    if (downloadRequest_->IsClosed()) {
        MEDIA_LOG_I("CacheData over, IsClosed");
        return true;
    }
    return false;
}

size_t HttpMediaDownloader::GetCurrentBufferSize()
{
    size_t bufferSize = 0;
    if (isFlv_) {
        if (buffer_ != nullptr) {
            bufferSize = buffer_->GetSize();
        }
    } else {
        if (cacheMediaBuffer_ != nullptr) {
            bufferSize = cacheMediaBuffer_->GetBufferSize(readOffset_);
        }
    }
    return bufferSize;
}

Status HttpMediaDownloader::SetCurrentBitRate(int32_t bitRate, int32_t streamID)
{
    MEDIA_LOG_I("SetCurrentBitRate: " PUBLIC_LOG_D32, bitRate);
    if (bitRate <= 0) {
        currentBitRate_ = DEFAULT_BIT_RATE;
    } else {
        currentBitRate_ = bitRate;
    }
    return Status::OK;
}

int32_t HttpMediaDownloader::GetWaterLineAbove()
{
    int32_t waterLineAbove = DEFAULT_WATER_LINE_ABOVE;
    if (currentBitRate_ > 0) {
        waterLineAbove = static_cast<int32_t>(DEFAULT_CACHE_TIME * currentBitRate_ / BYTES_TO_BIT);
        MEDIA_LOG_I("GetWaterLineAbove: " PUBLIC_LOG_D32, waterLineAbove);
    } else {
        MEDIA_LOG_I("GetWaterLineAbove default: " PUBLIC_LOG_D32, waterLineAbove);
    }
    return waterLineAbove;
}

void HttpMediaDownloader::HandleCachedDuration()
{
    if (currentBitRate_ <= 0 || callback_ == nullptr) {
        return;
    }
    uint64_t cachedDuration = static_cast<uint64_t>((static_cast<int64_t>(GetCurrentBufferSize()) *
        BYTES_TO_BIT * SECOND_TO_MILLIONSECOND) / static_cast<int64_t>(currentBitRate_));
    if ((cachedDuration > lastDurationReacord_ &&
        cachedDuration - lastDurationReacord_ > DURATION_CHANGE_AMOUT_MILLIONSECOND) ||
        (lastDurationReacord_ > cachedDuration &&
        lastDurationReacord_ - cachedDuration > DURATION_CHANGE_AMOUT_MILLIONSECOND)) { // 无符号整数需要先判断大小再相减
        MEDIA_LOG_I("OnEvet cachedDuration: " PUBLIC_LOG_U64, cachedDuration);
        callback_->OnEvent({PluginEventType::CACHED_DURATION, {cachedDuration}, "buffering_duration"});
        lastDurationReacord_ = cachedDuration;
    }
}

void HttpMediaDownloader::UpdateCachedPercent(BufferingInfoType infoType)
{
    if (waterLineAbove_ == 0 || callback_ == nullptr) {
        MEDIA_LOG_E("UpdateCachedPercent: ERROR");
        return;
    }
    if (infoType == BufferingInfoType::BUFFERING_START) {
        callback_->OnEvent({PluginEventType::EVENT_BUFFER_PROGRESS, {0}, "buffer percent"}); // 0
        lastCachedSize_ = 0;
        return;
    }
    if (infoType == BufferingInfoType::BUFFERING_END) {
        callback_->OnEvent({PluginEventType::EVENT_BUFFER_PROGRESS, {100}, "buffer percent"}); // 100
        lastCachedSize_ = 0;
        return;
    }
    if (infoType != BufferingInfoType::BUFFERING_PERCENT) {
        return;
    }
    int32_t bufferSize = static_cast<int32_t>(GetCurrentBufferSize());
    if (bufferSize < lastCachedSize_) {
        return;
    }
    int32_t deltaSize = bufferSize - lastCachedSize_;
    if (deltaSize >= static_cast<int32_t>(UPDATE_CACHE_STEP)) {
        int percent = (bufferSize >= static_cast<int32_t>(waterLineAbove_)) ?
                        100 : bufferSize * 100 / static_cast<int32_t>(waterLineAbove_); // 100
        callback_->OnEvent({PluginEventType::EVENT_BUFFER_PROGRESS, {percent}, "buffer percent"});
        lastCachedSize_ = bufferSize;
    }
}

bool HttpMediaDownloader::CheckBufferingOneSeconds()
{
    MEDIA_LOG_I("CheckBufferingOneSeconds in");
    int32_t sleepTime = 0;
    // return error again 1 time 1s, avoid ffmpeg error
    while (sleepTime < ONE_SECONDS && !isInterruptNeeded_.load()) {
        if (!isBuffering_) {
            break;
        }
        if (HandleBreak()) {
            isBuffering_ = false;
            break;
        }
        OSAL::SleepFor(TEN_MILLISECONDS);
        sleepTime += TEN_MILLISECONDS;
    }
    MEDIA_LOG_I("CheckBufferingOneSeconds out");
    return isBuffering_;
}
}
}
}
}