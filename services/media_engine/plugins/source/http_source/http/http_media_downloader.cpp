/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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
#include "network/network_typs.h"
#include "common/media_core.h"
#include "avcodec_trace.h"

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
constexpr int FIVE_MICROSECOND = 5;
constexpr int ONE_HUNDRED_MILLIONSECOND = 100;
constexpr int32_t SAVE_DATA_LOG_FREQUENCE = 50;
constexpr int IS_DOWNLOAD_MIN_BIT = 100; // Determine whether it is downloading
constexpr uint32_t DURATION_CHANGE_AMOUT_MILLIONSECOND = 500;
constexpr int32_t DEFAULT_BIT_RATE = 1638400;
constexpr int UPDATE_CACHE_STEP = 5 * 1024;
constexpr size_t MIN_WATER_LINE_ABOVE = 10 * 1024;
constexpr int32_t ONE_SECONDS = 1000;
constexpr int32_t TWO_SECONDS = 2000;
constexpr int32_t TEN_MILLISECONDS = 10;
constexpr float WATER_LINE_ABOVE_LIMIT_RATIO = 0.6;
constexpr float CACHE_LEVEL_1 = 0.3;
constexpr float DEFAULT_CACHE_TIME = 5;
constexpr size_t MAX_BUFFERING_TIME_OUT = 30 * 1000;
constexpr uint32_t OFFSET_NOT_UPDATE_THRESHOLD = 8;
constexpr float DOWNLOAD_WATER_LINE_RATIO = 0.95;
constexpr uint32_t ALLOW_SEEK_MIN_SIZE = 1 * 1024 * 1024;
constexpr uint64_t ALLOW_CLEAR_MIDDLE_DATA_MIN_SIZE = 1 * 1024 * 1024;
constexpr size_t AUDIO_WATER_LINE_ABOVE = 16 * 1024;
constexpr uint32_t CLEAR_SAVE_DATA_SIZE = 64 * 1024;
}

HttpMediaDownloader::HttpMediaDownloader(std::string url)
{
    if (static_cast<int32_t>(url.find(".flv")) != -1) {
        MEDIA_LOG_I("HTTP isflv.");
        isRingBuffer_ = true;
    }

    if (isRingBuffer_) {
        ringBuffer_ = std::make_shared<RingBuffer>(RING_BUFFER_SIZE);
        ringBuffer_->Init();
    } else {
        cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferImpl>();
        cacheMediaBuffer_->Init(MAX_CACHE_BUFFER_SIZE, CHUNK_SIZE);
        totalBufferSize_ = MAX_CACHE_BUFFER_SIZE;
        MEDIA_LOG_I("HTTP setting buffer size: " PUBLIC_LOG_U64, MAX_CACHE_BUFFER_SIZE);
    }
    isBuffering_ = true;
    downloader_ = std::make_shared<Downloader>("http");
    writeBitrateCaculator_ = std::make_shared<WriteBitrateCaculator>();
    steadyClock_.Reset();
    waterLineAbove_ = PLAY_WATER_LINE;
    recordData_ = std::make_shared<RecordData>();
}

void HttpMediaDownloader::InitRingBuffer(uint32_t expectBufferDuration)
{
    int totalBufferSize = CURRENT_BIT_RATE * static_cast<int32_t>(expectBufferDuration);
    if (totalBufferSize < RING_BUFFER_SIZE) {
        MEDIA_LOG_I("HTTP Failed setting ring buffer size: " PUBLIC_LOG_D32 ". already lower than the min buffer size: "
        PUBLIC_LOG_D32 ", setting buffer size: " PUBLIC_LOG_D32 ". ",
        totalBufferSize, RING_BUFFER_SIZE, RING_BUFFER_SIZE);
        ringBuffer_ = std::make_shared<RingBuffer>(RING_BUFFER_SIZE);
        totalBufferSize_ = RING_BUFFER_SIZE;
    } else if (totalBufferSize > MAX_BUFFER_SIZE) {
        MEDIA_LOG_I("HTTP Failed setting ring buffer size: " PUBLIC_LOG_D32 ". already exceed the max buffer size: "
        PUBLIC_LOG_D32 ", setting buffer size: " PUBLIC_LOG_D32 ". ",
        totalBufferSize, MAX_BUFFER_SIZE, MAX_BUFFER_SIZE);
        ringBuffer_ = std::make_shared<RingBuffer>(MAX_BUFFER_SIZE);
        totalBufferSize_ = MAX_BUFFER_SIZE;
    } else {
        ringBuffer_ = std::make_shared<RingBuffer>(totalBufferSize);
        totalBufferSize_ = totalBufferSize;
        MEDIA_LOG_I("HTTP Success setted ring buffer size: " PUBLIC_LOG_D32, totalBufferSize);
    }
    ringBuffer_->Init();
}

void HttpMediaDownloader::InitCacheBuffer(uint32_t expectBufferDuration)
{
    int totalBufferSize = CURRENT_BIT_RATE * static_cast<int32_t>(expectBufferDuration);
    cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferImpl>();
    if (totalBufferSize < RING_BUFFER_SIZE) {
        MEDIA_LOG_I("HTTP Failed setting cache buffer size: " PUBLIC_LOG_D32
                    ". already lower than the min buffer size: " PUBLIC_LOG_D32
                    ", setting buffer size: " PUBLIC_LOG_D32 ". ", totalBufferSize,
                    RING_BUFFER_SIZE, RING_BUFFER_SIZE);
        cacheMediaBuffer_->Init(RING_BUFFER_SIZE, CHUNK_SIZE);
        totalBufferSize_ = RING_BUFFER_SIZE;
    } else if (totalBufferSize > MAX_BUFFER_SIZE) {
        MEDIA_LOG_I("HTTP Failed setting cache buffer size: " PUBLIC_LOG_D32 ". already exceed the max buffer size: "
        PUBLIC_LOG_D32 ", setting buffer size: " PUBLIC_LOG_D32 ". ",
        totalBufferSize, MAX_BUFFER_SIZE, MAX_BUFFER_SIZE);
        cacheMediaBuffer_->Init(MAX_BUFFER_SIZE, CHUNK_SIZE);
        totalBufferSize_ = MAX_BUFFER_SIZE;
    } else {
        cacheMediaBuffer_->Init(totalBufferSize, CHUNK_SIZE);
        totalBufferSize_ = totalBufferSize;
        MEDIA_LOG_I("HTTP Success setted cache buffer size: " PUBLIC_LOG_D32, totalBufferSize);
    }
}

HttpMediaDownloader::HttpMediaDownloader(std::string url, uint32_t expectBufferDuration)
{
    if (static_cast<int32_t>(url.find(".flv")) != -1) {
        MEDIA_LOG_I("HTTP isflv.");
        isRingBuffer_ = true;
    }
    if (isRingBuffer_) {
        InitRingBuffer(expectBufferDuration);
    } else {
        InitCacheBuffer(expectBufferDuration);
    }
    isBuffering_ = true;
    downloader_ = std::make_shared<Downloader>("http");
    writeBitrateCaculator_ = std::make_shared<WriteBitrateCaculator>();
    steadyClock_.Reset();
    waterLineAbove_ = PLAY_WATER_LINE;
    recordData_ = std::make_shared<RecordData>();
}

HttpMediaDownloader::~HttpMediaDownloader()
{
    MEDIA_LOG_I("0x%{public}06" PRIXPTR " ~HttpMediaDownloader dtor", FAKE_POINTER(this));
    Close(false);
}

bool HttpMediaDownloader::Open(const std::string& url, const std::map<std::string, std::string>& httpHeader)
{
    MEDIA_LOG_I("HTTP Open download");
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
            && (totalBits_ / BYTES_TO_BIT) >= downloadRequest_->GetFileContentLengthNoWait()) {
            isDownloadFinish_ = true;
        }
        int64_t nowTime = steadyClock_.ElapsedMilliseconds();
        double downloadTime = (static_cast<double>(nowTime) - static_cast<double>(startDownloadTime_)) /
            SECOND_TO_MILLIONSECOND;
        if (downloadTime > ZERO_THRESHOLD) {
            avgDownloadSpeed_ = totalBits_ / downloadTime;
        }
        MEDIA_LOG_D("HTTP Download done, average download speed: " PUBLIC_LOG_D32 " bit/s",
            static_cast<int32_t>(avgDownloadSpeed_));
        MEDIA_LOG_I("HTTP Download done, data usage: " PUBLIC_LOG_U64 " bits in " PUBLIC_LOG_D64 "ms",
            totalBits_, static_cast<int64_t>(downloadTime * SECOND_TO_MILLIONSECOND));
        HandleBuffering();
    };
    RequestInfo requestInfo;
    requestInfo.url = url;
    requestInfo.httpHeader = httpHeader;
    downloadRequest_ = std::make_shared<DownloadRequest>(saveData, realStatusCallback, requestInfo);
    downloadRequest_->SetDownloadDoneCb(downloadDoneCallback);
    downloader_->Download(downloadRequest_, -1); // -1
    if (isRingBuffer_) {
        ringBuffer_->SetMediaOffset(0);
    }

    writeBitrateCaculator_->StartClock();
    downloader_->Start();
    return true;
}

void HttpMediaDownloader::Close(bool isAsync)
{
    if (isRingBuffer_) {
        ringBuffer_->SetActive(false);
    }
    isInterrupt_ = true;
    if (downloader_) {
        downloader_->Stop(isAsync);
    }
    cvReadWrite_.NotifyOne();
    if (!isDownloadFinish_) {
        int64_t nowTime = steadyClock_.ElapsedMilliseconds();
        double downloadTime = (static_cast<double>(nowTime) - static_cast<double>(startDownloadTime_)) /
            SECOND_TO_MILLIONSECOND;
        if (downloadTime > ZERO_THRESHOLD) {
            avgDownloadSpeed_ = totalBits_ / downloadTime;
        }
        MEDIA_LOG_D("HTTP Download close, average download speed: " PUBLIC_LOG_D32 " bit/s",
            static_cast<int32_t>(avgDownloadSpeed_));
        MEDIA_LOG_D("HTTP Download close, data usage: " PUBLIC_LOG_U64 " bits in " PUBLIC_LOG_D64 "ms",
            totalBits_, static_cast<int64_t>(downloadTime * SECOND_TO_MILLIONSECOND));
    }
}

void HttpMediaDownloader::Pause()
{
    if (isRingBuffer_) {
        bool cleanData = GetSeekable() != Seekable::SEEKABLE;
        ringBuffer_->SetActive(false, cleanData);
    }
    isInterrupt_ = true;
    downloader_->Pause();
    cvReadWrite_.NotifyOne();
}

void HttpMediaDownloader::Resume()
{
    if (isRingBuffer_) {
        ringBuffer_->SetActive(true);
    }
    isInterrupt_ = false;
    downloader_->Resume();
    cvReadWrite_.NotifyOne();
}

void HttpMediaDownloader::OnClientErrorEvent()
{
    if (callback_ != nullptr) {
        MEDIA_LOG_I("HTTP Read time out, OnEvent");
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
    if (downloadRequest_ != nullptr) {
        size_t fileContenLen = downloadRequest_->GetFileContentLength();
        if (fileContenLen > readOffset_) {
            fileRemain = fileContenLen - readOffset_;
            waterLineAbove_ = std::min(fileRemain, waterLineAbove_);
        }
    }

    UpdateWaterLineAbove();
    if (!canWrite_) {
        MEDIA_LOG_I("HTTP canWrite_ false");
        isBuffering_ = false;
    }
    if (GetCurrentBufferSize() >= waterLineAbove_) {
        MEDIA_LOG_I("HTTP Buffer is enough, bufferSize:" PUBLIC_LOG_ZU " waterLineAbove: " PUBLIC_LOG_ZU
            " avgDownloadSpeed: " PUBLIC_LOG_F, GetCurrentBufferSize(), waterLineAbove_, avgDownloadSpeed_);
        isBuffering_ = false;
    }
    if (HandleBreak()) {
        MEDIA_LOG_I("HTTP HandleBreak");
        isBuffering_ = false;
    }

    if (!isBuffering_ && isFirstFrameArrived_ && callback_ != nullptr) {
        MEDIA_LOG_I("HTTP CacheData onEvent BUFFERING_END, bufferSize: " PUBLIC_LOG_ZU ", waterLineAbove_: "
        PUBLIC_LOG_ZU ", isBuffering: " PUBLIC_LOG_D32 ", canWrite: " PUBLIC_LOG_D32,
            GetCurrentBufferSize(), waterLineAbove_, isBuffering_, canWrite_.load());
        UpdateCachedPercent(BufferingInfoType::BUFFERING_END);
        callback_->OnEvent({PluginEventType::BUFFERING_END, {BufferingInfoType::BUFFERING_END}, "end"});
        bufferingTime_ = 0;
    }
    MEDIA_LOG_D("HTTP HandleBuffering bufferSize: " PUBLIC_LOG_ZU ", waterLineAbove_: " PUBLIC_LOG_ZU
        ", isBuffering: " PUBLIC_LOG_D32 ", canWrite: " PUBLIC_LOG_D32,
        GetCurrentBufferSize(), waterLineAbove_, isBuffering_, canWrite_.load());
    return isBuffering_;
}

bool HttpMediaDownloader::StartBufferingCheck(unsigned int& wantReadLength)
{
    if (!isFirstFrameArrived_ || HandleBreak() || isBuffering_) {
        return false;
    }
    if (!isRingBuffer_ && cacheMediaBuffer_ != nullptr && cacheMediaBuffer_->IsReadSplit(readOffset_)) {
        MEDIA_LOG_D("HTTP StartBufferingCheck, change wantReadLength and return.");
        wantReadLength = std::min(static_cast<size_t>(wantReadLength), GetCurrentBufferSize());
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
    if (GetCurrentBufferSize() >= cacheWaterLine) {
        return false;
    }
    if (!canWrite_.load() && GetCurrentBufferSize() >= wantReadLength) {
        return false;
    }
    return true;
}

bool HttpMediaDownloader::StartBuffering(unsigned int& wantReadLength)
{
    if (!StartBufferingCheck(wantReadLength)) {
        return false;
    }
    if (!canWrite_.load()) { // Clear cacheBuffer when we can neither read nor write.
        ClearCacheBuffer();
        canWrite_ = true;
    }
    isBuffering_ = true;
    MEDIA_LOG_I("HTTP CacheData OnEvent BUFFERING_START, waterLineAbove: " PUBLIC_LOG_ZU " bufferSize "
        PUBLIC_LOG_ZU " readOffset: " PUBLIC_LOG_ZU " writeOffset: " PUBLIC_LOG_ZU, waterLineAbove_,
        GetCurrentBufferSize(), readOffset_, writeOffset_);
    UpdateCachedPercent(BufferingInfoType::BUFFERING_START);
    callback_->OnEvent({PluginEventType::BUFFERING_START, {BufferingInfoType::BUFFERING_START}, "start"});
    return true;
}

Status HttpMediaDownloader::ReadRingBuffer(unsigned char* buff, ReadDataInfo& readDataInfo)
{
    size_t fileContentLength = downloadRequest_->GetFileContentLength();
    uint64_t mediaOffset = ringBuffer_->GetMediaOffset();
    if (fileContentLength > mediaOffset) {
        uint64_t remain = fileContentLength - mediaOffset;
        readDataInfo.wantReadLength_ = remain < readDataInfo.wantReadLength_ ? remain :
            readDataInfo.wantReadLength_;
    }
    readDataInfo.realReadLength_ = 0;
    wantedReadLength_ = static_cast<size_t>(readDataInfo.wantReadLength_);
    while (ringBuffer_->GetSize() < readDataInfo.wantReadLength_ && !isInterruptNeeded_.load()) {
        if (downloadRequest_ != nullptr) {
            readDataInfo.isEos_ = downloadRequest_->IsEos();
        }
        if (readDataInfo.isEos_) {
            return CheckIsEosRingBuffer(buff, readDataInfo);
        }
        bool isClosed = downloadRequest_->IsClosed();
        if (isClosed && ringBuffer_->GetSize() == 0) {
            MEDIA_LOG_I("HttpMediaDownloader read return, isClosed: " PUBLIC_LOG_D32, isClosed);
            readDataInfo.realReadLength_ = 0;
            return Status::END_OF_STREAM;
        }
        Task::SleepInTask(FIVE_MICROSECOND);
    }
    if (isInterruptNeeded_.load()) {
        readDataInfo.realReadLength_ = 0;
        return Status::END_OF_STREAM;
    }
    readDataInfo.realReadLength_ = ringBuffer_->ReadBuffer(buff, readDataInfo.wantReadLength_, 2);  // wait 2 times
    MEDIA_LOG_D("HTTP ReadRingBuffer: wantReadLength " PUBLIC_LOG_D32 ", realReadLength " PUBLIC_LOG_D32 ", isEos "
                 PUBLIC_LOG_D32, readDataInfo.wantReadLength_, readDataInfo.realReadLength_, readDataInfo.isEos_);
    return Status::OK;
}

Status HttpMediaDownloader::ReadCacheBufferLoop(unsigned char* buff, ReadDataInfo& readDataInfo)
{
    FALSE_RETURN_V(downloadRequest_ != nullptr, Status::ERROR_UNKNOWN);
    if (downloadRequest_ != nullptr) {
        readDataInfo.isEos_ = downloadRequest_->IsEos();
    }
    if (readDataInfo.isEos_ || GetDownloadErrorState()) {
        return CheckIsEosCacheBuffer(buff, readDataInfo);
    }
    bool isClosed = downloadRequest_->IsClosed();
    if (isClosed && cacheMediaBuffer_->GetBufferSize(readOffset_) == 0) {
        MEDIA_LOG_I("HttpMediaDownloader read return, isClosed: " PUBLIC_LOG_D32, isClosed);
        readDataInfo.realReadLength_ = 0;
        return Status::END_OF_STREAM;
    }
    return Status::ERROR_UNKNOWN;
}

Status HttpMediaDownloader::ReadCacheBuffer(unsigned char* buff, ReadDataInfo& readDataInfo)
{
    size_t remain = cacheMediaBuffer_->GetBufferSize(readOffset_);
    MediaAVCodec::AVCodecTrace trace("HttpMediaDownloader::Read, readOffset: " + std::to_string(readOffset_) +
        ", expectedLen: " + std::to_string(readDataInfo.wantReadLength_) + ", bufferSize: " + std::to_string(remain));
    size_t hasReadSize = 0;
    wantedReadLength_ = static_cast<size_t>(readDataInfo.wantReadLength_);
    while (hasReadSize < readDataInfo.wantReadLength_ && !isInterruptNeeded_.load()) {
        Status tempStatus = ReadCacheBufferLoop(buff, readDataInfo);
        if (tempStatus != Status::ERROR_UNKNOWN) {
            return tempStatus;
        }
        auto size = cacheMediaBuffer_->Read(buff + hasReadSize, readOffset_ + hasReadSize,
                                            readDataInfo.wantReadLength_ - hasReadSize);
        if (size == 0) {
            Task::SleepInTask(FIVE_MICROSECOND); // 5
        } else {
            hasReadSize += size;
        }
    }
    if (isInterruptNeeded_.load()) {
        readDataInfo.realReadLength_ = hasReadSize;
        return Status::END_OF_STREAM;
    }
    readDataInfo.realReadLength_ = hasReadSize;
    readOffset_ += hasReadSize;
    isMinAndMaxOffsetUpdate_ = false;
    MEDIA_LOG_D("HTTP Read Success: wantReadLength " PUBLIC_LOG_D32 ", realReadLength " PUBLIC_LOG_D32 ", isEos "
        PUBLIC_LOG_D32 " readOffset_ " PUBLIC_LOG_ZU, readDataInfo.wantReadLength_,
        readDataInfo.realReadLength_, readDataInfo.isEos_, readOffset_);
    return Status::OK;
}

void HttpMediaDownloader::HandleDownloadWaterLine()
{
    if (downloader_ == nullptr || !isFirstFrameArrived_ || isBuffering_) {
        return;
    }
    uint64_t freeSize = cacheMediaBuffer_->GetFreeSize();
    size_t cachedDataSize = totalBufferSize_ > freeSize ? totalBufferSize_ - freeSize : 0;
    size_t downloadWaterLine = static_cast<size_t>(DOWNLOAD_WATER_LINE_RATIO *
        static_cast<float>(totalBufferSize_));
    if (canWrite_.load()) {
        if (cachedDataSize >= downloadWaterLine) {
            canWrite_ = false;
            MEDIA_LOG_D("HTTP downloadWaterLine above, stop write, downloadWaterLine: " PUBLIC_LOG_ZU,
                downloadWaterLine);
        }
    } else {
        if (cachedDataSize < downloadWaterLine) {
            canWrite_ = true;
            MEDIA_LOG_D("HTTP downloadWaterLine below, resume write, downloadWaterLine: " PUBLIC_LOG_ZU,
                downloadWaterLine);
        }
    }
}

void HttpMediaDownloader::CheckDownloadPos(unsigned int wantReadLength)
{
    size_t writeOffsetTmp = writeOffset_;
    size_t remain = GetCurrentBufferSize();
    if (cacheMediaBuffer_->IsReadSplit(readOffset_)) {
        return;
    }
    if (remain < wantReadLength && isServerAcceptRange_ &&
        (writeOffsetTmp < readOffset_ || writeOffsetTmp > readOffset_ + remain)) {
        MEDIA_LOG_I("HTTP CheckDownloadPos, change download pos.");
        ChangeDownloadPos(remain > 0);
    }
}

Status HttpMediaDownloader::ReadDelegate(unsigned char* buff, ReadDataInfo& readDataInfo)
{
    if (isRingBuffer_) {
        FALSE_RETURN_V_MSG(ringBuffer_ != nullptr, Status::END_OF_STREAM, "ringBuffer_ = nullptr");
        FALSE_RETURN_V_MSG(!isInterruptNeeded_.load(), Status::END_OF_STREAM, "isInterruptNeeded");
        FALSE_RETURN_V_MSG(readDataInfo.wantReadLength_ > 0, Status::END_OF_STREAM, "wantReadLength_ <= 0");
        if (isBuffering_ && !downloadRequest_->IsChunkedVod() && CheckBufferingOneSeconds()) {
            MEDIA_LOG_I("HTTP Return error again.");
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
            MEDIA_LOG_I("HTTP Return error again.");
            return Status::ERROR_AGAIN;
        }
        UpdateMinAndMaxReadOffset();
        HandleDownloadWaterLine();
        CheckDownloadPos(readDataInfo.wantReadLength_);
        ClearHasReadBuffer();
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
            MEDIA_LOG_D("HTTP Current read speed: " PUBLIC_LOG_D32 " Kbit/s,Current buffer size: " PUBLIC_LOG_U64
            " KByte", static_cast<int32_t>(readSpeed / 1024), static_cast<uint64_t>(curBufferSize / 1024));
            MediaAVCodec::AVCodecTrace trace("HttpMediaDownloader::Read, read speed: " +
                std::to_string(readSpeed) + " bit/s, bufferSize: " + std::to_string(curBufferSize) + " Byte");
            readTotalBytes_ = 0;
        }
        lastReadCheckTime_ = now;
        readRecordDuringTime_ = 0;
    }
    
    return ret;
}

Status HttpMediaDownloader::CheckIsEosRingBuffer(unsigned char* buff, ReadDataInfo& readDataInfo)
{
    if (ringBuffer_->GetSize() == 0) {
        MEDIA_LOG_I("HTTP read return, isEos: " PUBLIC_LOG_D32, readDataInfo.isEos_);
        return readDataInfo.realReadLength_ == 0 ? Status::END_OF_STREAM : Status::OK;
    } else {
        readDataInfo.realReadLength_ = ringBuffer_->ReadBuffer(buff, readDataInfo.wantReadLength_, 2); // 2
        return Status::OK;
    }
}

Status HttpMediaDownloader::CheckIsEosCacheBuffer(unsigned char* buff, ReadDataInfo& readDataInfo)
{
    if (cacheMediaBuffer_->GetBufferSize(readOffset_) == 0) {
        MEDIA_LOG_I("HTTP read return, isEos: " PUBLIC_LOG_D32, readDataInfo.isEos_);
        return readDataInfo.realReadLength_ == 0 ? Status::END_OF_STREAM : Status::OK;
    } else {
        readDataInfo.realReadLength_ = cacheMediaBuffer_->Read(buff, readOffset_, readDataInfo.wantReadLength_);
        readOffset_ += readDataInfo.realReadLength_;
        isMinAndMaxOffsetUpdate_ = false;
        MEDIA_LOG_D("HTTP read return, isEos: " PUBLIC_LOG_D32, readDataInfo.isEos_);
        return Status::OK;
    }
}

bool HttpMediaDownloader::SeekRingBuffer(int64_t offset)
{
    FALSE_RETURN_V(ringBuffer_ != nullptr, false);
    MEDIA_LOG_I("HTTP Seek in, buffer size " PUBLIC_LOG_ZU ", offset " PUBLIC_LOG_D64, ringBuffer_->GetSize(), offset);
    if (ringBuffer_->Seek(offset)) {
        MEDIA_LOG_I("HTTP ringBuffer_ seek success.");
        return true;
    }
    ringBuffer_->SetActive(false); // First clear buffer, avoid no available buffer then task pause never exit.
    downloader_->Pause();
    ringBuffer_->Clear();
    ringBuffer_->SetActive(true);
    bool result = downloader_->Seek(offset);
    if (result) {
        ringBuffer_->SetMediaOffset(offset);
    } else {
        MEDIA_LOG_D("HTTP Seek failed");
    }
    downloader_->Resume();
    return result;
}

void HttpMediaDownloader::UpdateMinAndMaxReadOffset()
{
    if (isMinAndMaxOffsetUpdate_) {
        return;
    }
    uint64_t readOffsetTmp = static_cast<uint64_t>(readOffset_);
    if (GetCurrentBufferSize() == 0 && readOffsetTmp < minReadOffset_) {
        maxReadOffset_ = readOffsetTmp;
        minReadOffset_ = readOffsetTmp;
        maxOffsetNotUpdateCount_ = 0;
        minOffsetNotUpdateCount_ = 0;
        ClearCacheBuffer();
        return;
    }

    if (maxReadOffset_ < readOffsetTmp) {
        maxReadOffset_ = readOffsetTmp;
        maxOffsetNotUpdateCount_ = 0;
    } else {
        maxOffsetNotUpdateCount_++;
    }
    if (maxOffsetNotUpdateCount_ > OFFSET_NOT_UPDATE_THRESHOLD) {
        maxReadOffset_ = readOffsetTmp;
        minReadOffset_ = readOffsetTmp;
        maxOffsetNotUpdateCount_ = 0;
    }
    if (readOffsetTmp < maxReadOffset_ && readOffsetTmp > minReadOffset_) {
        minReadOffset_ = readOffsetTmp;
        minOffsetNotUpdateCount_ = 0;
    } else {
        minOffsetNotUpdateCount_++;
    }
    if (minOffsetNotUpdateCount_ > OFFSET_NOT_UPDATE_THRESHOLD) {
        minReadOffset_ = readOffsetTmp;
        minOffsetNotUpdateCount_ = 0;
    }

    minReadOffset_ = std::min(minReadOffset_, static_cast<uint64_t>(readOffsetTmp));
    minReadOffset_ = std::min(minReadOffset_, maxReadOffset_);
    isMinAndMaxOffsetUpdate_ = true;

    MEDIA_LOG_D("HTTP UpdateMinAndMaxReadOffset, readOffset: " PUBLIC_LOG_U64 " minReadOffset_: "
        PUBLIC_LOG_U64 " maxReadOffset_: " PUBLIC_LOG_U64, readOffsetTmp, minReadOffset_, maxReadOffset_);
}

bool HttpMediaDownloader::ChangeDownloadPos(bool isSeekHit)
{
    MEDIA_LOG_D("HTTP ChangeDownloadPos in, offset: " PUBLIC_LOG_ZU, readOffset_);

    uint64_t seekOffset = readOffset_;
    if (isSeekHit) {
        isNeedDropData_ = true;
        downloader_->Pause();
        isNeedDropData_ = false;
        seekOffset += GetCurrentBufferSize();
        size_t fileContentLength = downloadRequest_->GetFileContentLength();
        if (seekOffset >= static_cast<uint64_t>(fileContentLength)) {
            MEDIA_LOG_W("HTTP seekOffset invalid, readOffset " PUBLIC_LOG_ZU " seekOffset " PUBLIC_LOG_U64
                " fileContentLength " PUBLIC_LOG_ZU, readOffset_, seekOffset, fileContentLength);
            return true;
        }
    } else {
        isNeedClean_ = true;
        downloader_->Pause();
        isNeedClean_ = false;
    }

    isHitSeeking_ = true;
    bool result = downloader_->Seek(seekOffset);
    isHitSeeking_ = false;
    if (result) {
        writeOffset_ = static_cast<size_t>(seekOffset);
    } else {
        MEDIA_LOG_E("HTTP Downloader seek fail.");
    }
    downloader_->Resume();
    MEDIA_LOG_D("HTTP ChangeDownloadPos out, seekOffset" PUBLIC_LOG_U64, seekOffset);
    return result;
}

bool HttpMediaDownloader::HandleSeekHit(int64_t offset)
{
    MEDIA_LOG_D("HTTP Seek hit.");
    if (cacheMediaBuffer_->IsReadSplit(offset)) {
        MEDIA_LOG_D("HTTP HandleSeekHit IsReadSplit, return.");
        return true;
    }
    size_t fileContentLength = downloadRequest_->GetFileContentLength();
    size_t downloadOffset = static_cast<size_t>(offset) + cacheMediaBuffer_->GetBufferSize(offset);
    if (downloadOffset >= fileContentLength) {
        MEDIA_LOG_W("HTTP downloadOffset invalid, offset " PUBLIC_LOG_D64 " downloadOffset " PUBLIC_LOG_ZU
            " fileContentLength " PUBLIC_LOG_ZU, offset, downloadOffset, fileContentLength);
        return true;
    }

    size_t changeDownloadPosThreshold = DEFAULT_WATER_LINE_ABOVE;
    if (offset < maxReadOffset_) {
        changeDownloadPosThreshold = AUDIO_WATER_LINE_ABOVE;
    }

    if (writeOffset_ != downloadOffset && cacheMediaBuffer_->GetBufferSize(offset) < changeDownloadPosThreshold) {
        return ChangeDownloadPos(true);
    } else {
        MEDIA_LOG_D("HTTP Seek hit, continue download.");
    }
    return true;
}

bool HttpMediaDownloader::SeekCacheBuffer(int64_t offset)
{
    readOffset_ = static_cast<size_t>(offset);
    cacheMediaBuffer_->Seek(offset); // Notify the cacheBuffer where to read.
    UpdateMinAndMaxReadOffset();

    if (!isServerAcceptRange_) {
        MEDIA_LOG_D("HTTP Don't support range, return true.");
        return true;
    }

    size_t remain = cacheMediaBuffer_->GetBufferSize(offset);
    MEDIA_LOG_I("HTTP Seek: buffer size " PUBLIC_LOG_ZU ", offset " PUBLIC_LOG_D64, remain, offset);
    if (remain > 0) {
        return HandleSeekHit(offset);
    }
    MEDIA_LOG_I("HTTP Seek miss.");

    uint64_t diff = offset > writeOffset_ ? offset - writeOffset_ : 0;
    if (diff > 0 && diff < ALLOW_SEEK_MIN_SIZE) {
        MEDIA_LOG_I("HTTP Seek miss, diff is too small so return and wait.");
        return true;
    }
    return ChangeDownloadPos(false);
}

bool HttpMediaDownloader::SeekToPos(int64_t offset)
{
    if (isRingBuffer_) {
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
    if (isRingBuffer_) {
        FALSE_RETURN(ringBuffer_ != nullptr);
        ringBuffer_->SetReadBlocking(isReadBlockingAllowed);
    }
}

bool HttpMediaDownloader::SaveRingBufferData(uint8_t* data, uint32_t len)
{
    FALSE_RETURN_V(ringBuffer_->WriteBuffer(data, len), false);
    cvReadWrite_.NotifyOne();
    size_t bufferSize = ringBuffer_->GetSize();
    double ratio = (static_cast<double>(bufferSize)) / RING_BUFFER_SIZE;
    if ((bufferSize >= WATER_LINE ||
        bufferSize >= downloadRequest_->GetFileContentLength() / 2) && !aboveWaterline_) { // 2
        aboveWaterline_ = true;
        MEDIA_LOG_I("HTTP Send http aboveWaterline event, ringbuffer ratio " PUBLIC_LOG_F, ratio);
        if (callback_ != nullptr) {
            callback_->OnEvent({PluginEventType::ABOVE_LOW_WATERLINE, {ratio}, "http"});
        }
        startedPlayStatus_ = true;
    } else if (bufferSize < WATER_LINE && aboveWaterline_) {
        aboveWaterline_ = false;
        MEDIA_LOG_I("HTTP Send http belowWaterline event, ringbuffer ratio " PUBLIC_LOG_F, ratio);
        if (callback_ != nullptr) {
            callback_->OnEvent({PluginEventType::BELOW_LOW_WATERLINE, {ratio}, "http"});
        }
    }
    return true;
}

bool HttpMediaDownloader::SaveData(uint8_t* data, uint32_t len)
{
    if (cacheMediaBuffer_ == nullptr && ringBuffer_ == nullptr) {
        return false;
    }
    OnWriteBuffer(len);
    bool ret = true;
    if (isRingBuffer_) {
        ret = SaveRingBufferData(data, len);
    } else {
        ret = SaveCacheBufferData(data, len);
    }
    HandleBuffering();
    return ret;
}

bool HttpMediaDownloader::SaveCacheBufferData(uint8_t* data, uint32_t len)
{
    if (isNeedClean_) {
        return true;
    }

    isServerAcceptRange_ = downloadRequest_->IsServerAcceptRange();

    size_t hasWriteSize = 0;
    while (hasWriteSize < len && !isInterruptNeeded_.load() && !isInterrupt_.load()) {
        if (isNeedClean_) {
            MEDIA_LOGI_LIMIT(SAVE_DATA_LOG_FREQUENCE, "isNeedClean true.");
            return true;
        }
        size_t res = cacheMediaBuffer_->Write(data + hasWriteSize, writeOffset_, len - hasWriteSize);
        writeOffset_ += res;
        hasWriteSize += res;
        writeBitrateCaculator_->UpdateWriteBytes(res);
        MEDIA_LOGI_LIMIT(SAVE_DATA_LOG_FREQUENCE, "writeOffset " PUBLIC_LOG_ZU " res "
                            PUBLIC_LOG_ZU, writeOffset_, res);
        if (canWrite_.load() && (res > 0 || hasWriteSize == len)) {
            HandleCachedDuration();
            writeBitrateCaculator_->StartClock();
            uint64_t writeTime  = writeBitrateCaculator_->GetWriteTime() / SECOND_TO_MILLIONSECOND;
            if (writeTime > ONE_SECONDS) {
                writeBitrateCaculator_->ResetClock();
            }
            continue;
        }
        MEDIA_LOG_W("HTTP CacheMediaBuffer full.");
        writeBitrateCaculator_->StopClock();
        canWrite_ = false;
        HandleBuffering();
        while (!isInterrupt_.load() && !isNeedClean_.load() && !canWrite_.load() && !isInterruptNeeded_.load()) {
            MEDIA_LOGI_LIMIT(SAVE_DATA_LOG_FREQUENCE, "HTTP CacheMediaBuffer full, waiting seek or read.");
            if (isHitSeeking_ || isNeedDropData_) {
                canWrite_ = true;
                return true;
            }
            OSAL::SleepFor(ONE_HUNDRED_MILLIONSECOND);
        }
        canWrite_ = true;
    }
    if (isInterruptNeeded_.load() || isInterrupt_.load()) {
        MEDIA_LOG_I("HTTP isInterruptNeeded true, return false.");
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
        MEDIA_LOG_D("HTTP Start play delay time: " PUBLIC_LOG_D64, playDelayTime_);
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
            MediaAVCodec::AVCodecTrace trace("HttpMediaDownloader::DownloadReport, download speed: " +
                std::to_string(downloadRate) + " bit/s, bufferSize: " + std::to_string(remainingBuffer) + " Byte");
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
    MEDIA_LOG_I("HTTP SetDemuxerState");
    isFirstFrameArrived_ = true;
}

void HttpMediaDownloader::SetDownloadErrorState()
{
    MEDIA_LOG_I("HTTP SetDownloadErrorState");
    downloadErrorState_ = true;
    if (callback_ != nullptr) {
        callback_->OnEvent({PluginEventType::CLIENT_ERROR, {NetworkClientErrorCode::ERROR_TIME_OUT}, "read"});
    }
    if (isRingBuffer_) {
        if (ringBuffer_ != nullptr && ringBuffer_->GetSize() == 0) {
            Close(true);
        }
    } else {
        if (cacheMediaBuffer_ != nullptr && cacheMediaBuffer_->GetBufferSize(readOffset_) == 0) {
            Close(true);
        }
    }
    Close(true);
}

void HttpMediaDownloader::SetInterruptState(bool isInterruptNeeded)
{
    MEDIA_LOG_I("SetInterruptState");
    isInterruptNeeded_ = isInterruptNeeded;
    if (ringBuffer_ != nullptr && isInterruptNeeded) {
        ringBuffer_->SetActive(false);
    }
    if (downloader_ != nullptr) {
        downloader_->SetInterruptState(isInterruptNeeded);
    }
}

size_t HttpMediaDownloader::GetBufferSize() const
{
    return GetCurrentBufferSize();
}

RingBuffer& HttpMediaDownloader::GetBuffer()
{
    return *ringBuffer_;
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
        MEDIA_LOG_E("recordSpeedCount is 0, can't get avgDownloadRate");
        return std::make_pair(0, static_cast<int32_t>(avgDownloadSpeed_));
    }
    auto rateAndSpeed = std::make_pair(avgSpeedSum_ / recordSpeedCount_, static_cast<int32_t>(avgDownloadSpeed_));
    return rateAndSpeed;
}

std::pair<int32_t, int32_t> HttpMediaDownloader::GetDownloadRateAndSpeed()
{
    MEDIA_LOG_I("HttpMediaDownloader::GetDownloadRateAndSpeed.");
    if (recordSpeedCount_ == 0) {
        MEDIA_LOG_E("recordSpeedCount is 0, can't get avgDownloadRate");
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
        MEDIA_LOG_I("HTTP CacheData over, downloadErrorState true.");
        return true;
    }
    if (downloadRequest_ == nullptr) {
        MEDIA_LOG_I("HTTP CacheData over, downloadRequest is nullptr.");
        return true;
    }
    if (callback_ == nullptr) {
        MEDIA_LOG_I("HTTP CacheData over, callback is nullptr.");
        return true;
    }
    if (downloadRequest_->IsEos()) {
        MEDIA_LOG_I("HTTP CacheData over, isEos");
        return true;
    }
    if (downloadRequest_->IsClosed()) {
        MEDIA_LOG_I("HTTP CacheData over, IsClosed");
        return true;
    }
    if (downloadRequest_->IsChunkedVod()) {
        MEDIA_LOG_I("HTTP CacheData over, IsChunkedVod");
        return true;
    }
    return false;
}

size_t HttpMediaDownloader::GetCurrentBufferSize() const
{
    size_t bufferSize = 0;
    if (isRingBuffer_) {
        if (ringBuffer_ != nullptr) {
            bufferSize = ringBuffer_->GetSize();
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
    MEDIA_LOG_I("HTTP SetCurrentBitRate: " PUBLIC_LOG_D32, bitRate);
    if (bitRate <= 0) {
        currentBitRate_ = DEFAULT_BIT_RATE;
    } else {
        currentBitRate_ = std::max(currentBitRate_, bitRate);
    }
    return Status::OK;
}

void HttpMediaDownloader::HandleCachedDuration()
{
    if (currentBitRate_ <= 0 || callback_ == nullptr) {
        return;
    }
    uint64_t cachedDuration = static_cast<uint64_t>((static_cast<int64_t>(GetCurrentBufferSize()) *
        BYTES_TO_BIT * SECOND_TO_MILLIONSECOND) / static_cast<int64_t>(currentBitRate_));
    // Subtraction of unsigned integers requires size comparison first.
    if ((cachedDuration > lastDurationReacord_ &&
        cachedDuration - lastDurationReacord_ > DURATION_CHANGE_AMOUT_MILLIONSECOND) ||
        (lastDurationReacord_ > cachedDuration &&
        lastDurationReacord_ - cachedDuration > DURATION_CHANGE_AMOUT_MILLIONSECOND)) {
        MEDIA_LOG_D("HTTP OnEvent cachedDuration: " PUBLIC_LOG_U64, cachedDuration);
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
    MEDIA_LOG_I("HTTP CheckBufferingOneSeconds in");
    int32_t sleepTime = 0;
    // return error again 1 time 1s, avoid ffmpeg error
    while (sleepTime < TWO_SECONDS && !isInterruptNeeded_.load()) {
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
    MEDIA_LOG_I("HTTP CheckBufferingOneSeconds out");
    return isBuffering_;
}

void HttpMediaDownloader::SetAppUid(int32_t appUid)
{
    if (downloader_) {
        downloader_->SetAppUid(appUid);
    }
}

float HttpMediaDownloader::GetCacheDuration(float ratio)
{
    if (ratio >= 1) {
        return CACHE_LEVEL_1;
    }
    return DEFAULT_CACHE_TIME;
}

void HttpMediaDownloader::UpdateWaterLineAbove()
{
    if (!isFirstFrameArrived_) {
        return;
    }
    size_t waterLineAbove = DEFAULT_WATER_LINE_ABOVE;
    if (currentBitRate_ > 0) {
        float cacheTime = 0;
        uint64_t writeBitrate = writeBitrateCaculator_->GetWriteBitrate();
        if (writeBitrate > 0) {
            float ratio = static_cast<float>(writeBitrate) / currentBitRate_;
            cacheTime = GetCacheDuration(ratio);
        } else {
            cacheTime = DEFAULT_CACHE_TIME;
        }
        waterLineAbove = static_cast<size_t>(cacheTime * currentBitRate_ / BYTES_TO_BIT);
        waterLineAbove = std::max(MIN_WATER_LINE_ABOVE, waterLineAbove);
    } else {
        MEDIA_LOG_D("UpdateWaterLineAbove default: " PUBLIC_LOG_ZU, waterLineAbove);
    }
    waterLineAbove_ = waterLineAbove;

    if (readOffset_ < maxReadOffset_) {
        waterLineAbove_ = DEFAULT_WATER_LINE_ABOVE;
    }

    size_t fileRemain = 0;
    if (downloadRequest_ != nullptr) {
        size_t fileContenLen = downloadRequest_->GetFileContentLength();
        if (fileContenLen > readOffset_) {
            fileRemain = fileContenLen - readOffset_;
            waterLineAbove_ = std::min(fileRemain, waterLineAbove_);
        }
    }

    waterLineAbove_ = std::min(waterLineAbove_, static_cast<size_t>(MAX_CACHE_BUFFER_SIZE *
        WATER_LINE_ABOVE_LIMIT_RATIO));
    MEDIA_LOG_D("UpdateWaterLineAbove: " PUBLIC_LOG_ZU " writeBitrate: " PUBLIC_LOG_U64 " readBitrate: "
        PUBLIC_LOG_D32, waterLineAbove_, writeBitrateCaculator_->GetWriteBitrate(), currentBitRate_);
}

bool HttpMediaDownloader::GetPlayable()
{
    if (!isFirstFrameArrived_) {
        return false;
    }
    size_t wantedLength = wantedReadLength_;
    size_t waterLine = wantedLength > 0 ? std::max(PLAY_WATER_LINE, wantedLength) : 0;
    return waterLine == 0 ? GetBufferSize() > waterLine : GetBufferSize() >= waterLine;
}

bool HttpMediaDownloader::GetBufferingTimeOut()
{
    if (bufferingTime_ == 0) {
        return false;
    } else {
        size_t now = static_cast<size_t>(steadyClock_.ElapsedMilliseconds());
        return now >= bufferingTime_ ? now - bufferingTime_ >= MAX_BUFFERING_TIME_OUT : false;
    }
}

Status HttpMediaDownloader::StopBufferring(bool isAppBackground)
{
    MEDIA_LOG_I("HttpMediaDownloader:StopBufferring enter");
    if (downloader_ == nullptr) {
        MEDIA_LOG_E("StopBufferring error.");
        return Status::ERROR_NULL_POINTER;
    }
    downloader_->SetAppState(isAppBackground);
    if (isAppBackground) {
        if (ringBuffer_ != nullptr) {
            ringBuffer_->SetActive(false, false);
        }
        if (cacheMediaBuffer_ != nullptr) {
            isInterrupt_ = true;
        }
    } else {
        if (ringBuffer_ != nullptr) {
            ringBuffer_->SetActive(true, false);
        }
        if (cacheMediaBuffer_ != nullptr) {
            isInterrupt_ = false;
        }
    }
    downloader_->StopBufferring();
    MEDIA_LOG_I("HttpMediaDownloader:StopBufferring out");
    return Status::OK;
}

bool HttpMediaDownloader::IsBuffering()
{
    return isBuffering_;
}

bool HttpMediaDownloader::ClearHasReadBuffer()
{
    if (!isFirstFrameArrived_ || cacheMediaBuffer_ == nullptr) {
        return false;
    }
    uint64_t minOffset = std::min(minReadOffset_, static_cast<uint64_t>(writeOffset_));
    uint64_t clearOffset = minOffset > CLEAR_SAVE_DATA_SIZE ? minOffset - CLEAR_SAVE_DATA_SIZE : 0;
    bool res = false;
    res = cacheMediaBuffer_->ClearFragmentBeforeOffset(clearOffset);
    res = cacheMediaBuffer_->ClearChunksOfFragment(clearOffset) || res;
    uint64_t minClearOffset = minOffset + CLEAR_SAVE_DATA_SIZE;
    uint64_t maxClearOffset = maxReadOffset_ > CLEAR_SAVE_DATA_SIZE ? maxReadOffset_ - CLEAR_SAVE_DATA_SIZE : 0;
    uint64_t diff = maxClearOffset > minClearOffset ? maxClearOffset - minClearOffset : 0;
    if (diff > ALLOW_CLEAR_MIDDLE_DATA_MIN_SIZE) {
        res = cacheMediaBuffer_->ClearMiddleReadFragment(minClearOffset, maxClearOffset) || res;
    }
    MEDIA_LOG_D("HTTP ClearHasReadBuffer, res: " PUBLIC_LOG_D32, res);
    return res;
}

void HttpMediaDownloader::ClearCacheBuffer()
{
    if (cacheMediaBuffer_ == nullptr || downloader_ == nullptr) {
        return;
    }
    isNeedDropData_ = true;
    downloader_->Pause();
    cacheMediaBuffer_->Clear();
    isNeedDropData_ = false;
    downloader_->Resume();
    uint64_t freeSize = cacheMediaBuffer_->GetFreeSize();
    MEDIA_LOG_I("HTTP ClearCacheBuffer, freeSize: " PUBLIC_LOG_U64, freeSize);
}
}
}
}
}