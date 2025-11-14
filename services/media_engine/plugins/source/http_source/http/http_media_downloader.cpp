/*
 * Copyright (c) 2024-2024 Huawei Device Co., Ltd.
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
#include <climits>
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
constexpr size_t FLV_PLAY_WATER_LINE = 20 * 1024;
constexpr size_t DEFAULT_WATER_LINE_ABOVE = 48 * 10 * 1024;
constexpr int FIVE_MICROSECOND = 5;
constexpr int32_t ONE_HUNDRED_MILLIONSECOND = 100;
constexpr int32_t SAVE_DATA_LOG_FREQUENCE = 50;
constexpr int IS_DOWNLOAD_MIN_BIT = 100; // Determine whether it is downloading
constexpr uint32_t DURATION_CHANGE_AMOUT_MILLIONSECOND = 500;
constexpr int32_t DEFAULT_BIT_RATE = 1638400;
constexpr int UPDATE_CACHE_STEP = 5 * 1024;
constexpr size_t MIN_WATER_LINE_ABOVE = 100 * 1024;
constexpr int32_t ONE_SECONDS = 1000;
constexpr int32_t TWO_SECONDS = 2000;
constexpr int32_t TEN_MILLISECONDS = 10;
constexpr float WATER_LINE_ABOVE_LIMIT_RATIO = 0.6;
constexpr float CACHE_LEVEL_1 = 0.5;
constexpr float DEFAULT_CACHE_TIME = 5;
constexpr size_t MAX_BUFFERING_TIME_OUT = 30 * 1000;
constexpr size_t MAX_BUFFERING_TIME_OUT_DELAY = 60 * 1000;
constexpr size_t MAX_WATER_LINE_ABOVE = 8 * 1024 * 1024;
constexpr uint32_t OFFSET_NOT_UPDATE_THRESHOLD = 8;
constexpr float DOWNLOAD_WATER_LINE_RATIO = 0.90;
constexpr uint32_t ALLOW_SEEK_MIN_SIZE = 1 * 1024 * 1024;
constexpr uint64_t ALLOW_CLEAR_MIDDLE_DATA_MIN_SIZE = 2 * 1024 * 1024;
constexpr size_t AUDIO_WATER_LINE_ABOVE = 16 * 1024;
constexpr uint32_t CLEAR_SAVE_DATA_SIZE = 1 * 1024 * 1024;
constexpr size_t LARGE_OFFSET_SPAN_THRESHOLD = 10 * 1024 * 1024;
constexpr int32_t STATE_CHANGE_THRESHOLD = 2;
constexpr size_t LARGE_VIDEO_THRESHOLD = 18 * 1024 * 1024;
constexpr size_t BUFFER_REDUNDANCY = 4 * 1024 * 1024;
constexpr size_t IGNORE_BUFFERING_WITH_START_TIME_MS = 5000;
constexpr size_t IGNORE_BUFFERING_EXTRA_CACHE_BEYOND_MS = 300;
constexpr float FLV_AUTO_SELECT_SMOOTH_FACTOR = 0.8;
constexpr size_t FLV_AUTO_SELECT_TIME_GAP = 3;
constexpr uint32_t CHUNK_SIZE = 16 * 1024;
constexpr uint64_t MAX_CACHE_BUFFER_SIZE = 19 * 1024 * 1024;

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
    FALSE_RETURN_MSG(cacheMediaBuffer_ != nullptr, "HTTP CacheBuffer create failed.");
    if (totalBufferSize < RING_BUFFER_SIZE) {
        MEDIA_LOG_I("HTTP Failed setting cache buffer size: " PUBLIC_LOG_D32
                    ". already lower than the min buffer size: " PUBLIC_LOG_D32
                    ", setting buffer size: " PUBLIC_LOG_D32 ". ", totalBufferSize,
                    RING_BUFFER_SIZE, RING_BUFFER_SIZE);
        isCacheBufferInited_ = cacheMediaBuffer_->Init(RING_BUFFER_SIZE, CHUNK_SIZE);
        totalBufferSize_ = RING_BUFFER_SIZE;
    } else if (totalBufferSize > MAX_BUFFER_SIZE) {
        MEDIA_LOG_I("HTTP Failed setting cache buffer size: " PUBLIC_LOG_D32 ". already exceed the max buffer size: "
        PUBLIC_LOG_D32 ", setting buffer size: " PUBLIC_LOG_D32 ". ",
        totalBufferSize, MAX_BUFFER_SIZE, MAX_BUFFER_SIZE);
        isCacheBufferInited_ = cacheMediaBuffer_->Init(MAX_BUFFER_SIZE, CHUNK_SIZE);
        totalBufferSize_ = MAX_BUFFER_SIZE;
    } else {
        isCacheBufferInited_ = cacheMediaBuffer_->Init(totalBufferSize, CHUNK_SIZE);
        totalBufferSize_ = totalBufferSize;
        MEDIA_LOG_I("HTTP Success setted cache buffer size: " PUBLIC_LOG_D32, totalBufferSize);
    }
    FALSE_RETURN_MSG(isCacheBufferInited_, "HTTP CacheBufferInit error");
}

HttpMediaDownloader::HttpMediaDownloader(std::string url, uint32_t expectBufferDuration,
    std::shared_ptr<MediaSourceLoaderCombinations> sourceLoader)
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
    bufferingTime_ = static_cast<size_t>(steadyClock_.ElapsedMilliseconds());

    if (sourceLoader != nullptr) {
        sourceLoader_ = sourceLoader;
        isLargeOffsetSpan_ = true;
        downloader_ = std::make_shared<Downloader>("http", sourceLoader);
        MEDIA_LOG_I("HTTP app download.");
    } else {
        downloader_ = std::make_shared<Downloader>("http");
    }
    downloader_->Init();

    timeoutInterval_ = MAX_BUFFERING_TIME_OUT;
    writeBitrateCaculator_ = std::make_shared<WriteBitrateCaculator>();
    steadyClock_.Reset();
    loopInterruptClock_.Reset();
    waterLineAbove_ = PLAY_WATER_LINE;
    recordData_ = std::make_shared<RecordData>();
}

HttpMediaDownloader::~HttpMediaDownloader()
{
    MEDIA_LOG_I("0x%{public}06" PRIXPTR " ~HttpMediaDownloader dtor", FAKE_POINTER(this));
    Close(false);
}

void HttpMediaDownloader::Init()
{
    MEDIA_LOG_D("0x%{public}06" PRIXPTR " Init", FAKE_POINTER(this));
}

std::string HttpMediaDownloader::GetContentType()
{
    FALSE_RETURN_V(downloader_ != nullptr, "");
    MEDIA_LOG_I("In");
    return downloader_->GetContentType();
}

bool HttpMediaDownloader::Open(const std::string& url, const std::map<std::string, std::string>& httpHeader)
{
    MEDIA_LOG_I("HTTP Open download in");
    isDownloadFinish_ = false;
    openTime_ = steadyClock_.ElapsedMilliseconds();
    auto weakDownloader = weak_from_this();
    auto saveData = [weakDownloader] (uint8_t*&& data, uint32_t&& len, bool&& notBlock) -> uint32_t {
        auto shareDownloader = weakDownloader.lock();
        FALSE_RETURN_V_MSG(shareDownloader != nullptr, 0u, "saveData, Http Media Downloader already destructed.");
        return shareDownloader->SaveData(std::forward<decltype(data)>(data), std::forward<decltype(len)>(len),
            std::forward<decltype(notBlock)>(notBlock));
    };
    FALSE_RETURN_V(statusCallback_ != nullptr, false);
    auto realStatusCallback = [weakDownloader] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {
        auto shareDownloader = weakDownloader.lock();
        FALSE_RETURN_MSG(shareDownloader != nullptr, "realStatusCb, Http Media Downloader already destructed.");
        shareDownloader->statusCallback_(status, shareDownloader->downloader_,
            std::forward<decltype(request)>(request));
    };
    auto downloadDoneCallback = [weakDownloader] (const std::string &url, const std::string& location) {
        auto shareDownloader = weakDownloader.lock();
        FALSE_RETURN_MSG(shareDownloader != nullptr, "downloadDoneCb, Http Media Downloader already destructed.");
        if (shareDownloader->downloadRequest_ != nullptr && shareDownloader->downloadRequest_->IsEos()
            && (shareDownloader->totalBits_ / BYTES_TO_BIT) >=
            shareDownloader->downloadRequest_->GetFileContentLengthNoWait()) {
            shareDownloader->isDownloadFinish_ = true;
        }
        int64_t nowTime = shareDownloader->steadyClock_.ElapsedMilliseconds();
        double downloadTime = (nowTime - shareDownloader->startDownloadTime_) * 1.0 / SECOND_TO_MILLISECONDS;
        if (downloadTime > ZERO_THRESHOLD) {
            shareDownloader->avgDownloadSpeed_ = shareDownloader->totalBits_ / downloadTime;
        }
        MEDIA_LOG_I("HTTP Download done, data usage: " PUBLIC_LOG_U64 " bits in %{public}.0lfms, "
            "average download speed: %{public}.0lf bits/s", shareDownloader->totalBits_,
            downloadTime * SECOND_TO_MILLISECONDS, shareDownloader->avgDownloadSpeed_);
        shareDownloader->HandleBuffering();
    };
    RequestInfo requestInfo;
    requestInfo.url = defaultStream_ != nullptr ? defaultStream_->url : url;
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
            SECOND_TO_MILLISECONDS;
        if (downloadTime > ZERO_THRESHOLD) {
            avgDownloadSpeed_ = totalBits_ / downloadTime;
        }
        MEDIA_LOG_D("HTTP Download close, average download speed: " PUBLIC_LOG_D32 " bit/s",
            static_cast<int32_t>(avgDownloadSpeed_));
        MEDIA_LOG_D("HTTP Download close, data usage: " PUBLIC_LOG_U64 " bits in " PUBLIC_LOG_D64 "ms",
            totalBits_, static_cast<int64_t>(downloadTime * SECOND_TO_MILLISECONDS));
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
        isTimeoutErrorNotified_.store(true);
    }
}

bool HttpMediaDownloader::HandleBuffering()
{
    if (isBuffering_ && GetBufferingTimeOut() && callback_) {
        MEDIA_LOG_I("HTTP cachebuffer buffering time out.");
        callback_->OnEvent({PluginEventType::CLIENT_ERROR, {NetworkClientErrorCode::ERROR_TIME_OUT}, "buffering"});
        isTimeoutErrorNotified_.store(true);
        isBuffering_ = false;
        return false;
    }
    if (IsNeedBufferForPlaying()) {
        return false;
    }
    if (!isBuffering_ || downloadRequest_ == nullptr || downloadRequest_->IsChunkedVod()) {
        bufferingTime_ = 0;
        return false;
    }
    size_t fileContenLen = downloadRequest_->GetFileContentLength();
    if (fileContenLen > readOffset_) {
        size_t fileRemain = fileContenLen - readOffset_;
        waterLineAbove_ = std::min(fileRemain, waterLineAbove_);
    }

    UpdateWaterLineAbove();
    UpdateCachedPercent(BufferingInfoType::BUFFERING_PERCENT);
    HandleWaterline();
    if (!isBuffering_ && !isFirstFrameArrived_) {
        bufferingTime_ = 0;
    }
    if (!isBuffering_ && isFirstFrameArrived_ && callback_ != nullptr) {
        MEDIA_LOG_I("HTTP CacheData onEvent BUFFERING_END, bufferSize: " PUBLIC_LOG_U64 ", waterLineAbove_: "
        PUBLIC_LOG_ZU ", isBuffering: " PUBLIC_LOG_D32 ", canWrite: " PUBLIC_LOG_D32 " readOffset: "
        PUBLIC_LOG_ZU " writeOffset: " PUBLIC_LOG_ZU, GetCurrentBufferSize(), waterLineAbove_, isBuffering_.load(),
        canWrite_.load(), readOffset_, writeOffset_);
        UpdateCachedPercent(BufferingInfoType::BUFFERING_END);
        callback_->OnEvent({PluginEventType::BUFFERING_END, {BufferingInfoType::BUFFERING_END}, "end"});
        bufferingTime_ = 0;
    }
    return isBuffering_.load();
}

void HttpMediaDownloader::HandleWaterline()
{
    AutoLock lk(bufferingEndMutex_);
    if (!canWrite_) {
        MEDIA_LOG_I("HTTP canWrite_ false");
        isBuffering_ = false;
    }
    size_t currentWaterLine = waterLineAbove_;
    size_t currentOffset = readOffset_;
    {
        AutoLock lock(initCacheMutex_);
        if (initCacheSize_.load() != -1) {
            currentWaterLine = static_cast<size_t>(initCacheSize_.load());
            currentOffset = static_cast<size_t>(expectOffset_.load());
            MEDIA_LOG_I("currentOffset:" PUBLIC_LOG_ZU, currentOffset);
        }
        if (GetCurrentBufferSize() >= currentWaterLine || HandleBreak()) {
            MEDIA_LOG_I("HTTP Buffer is enough, bufferSize:" PUBLIC_LOG_U64 " waterLineAbove: " PUBLIC_LOG_ZU
                " avgDownloadSpeed: " PUBLIC_LOG_F, GetCurrentBufferSize(), currentWaterLine, avgDownloadSpeed_);
            MEDIA_LOG_I("initCacheSize_: " PUBLIC_LOG_D32, static_cast<int32_t>(initCacheSize_.load()));
            isBuffering_ = false;
            if (initCacheSize_.load() != -1) {
                callback_->OnEvent({PluginEventType::INITIAL_BUFFER_SUCCESS,
                                        {BufferingInfoType::BUFFERING_END}, "end"});
                MEDIA_LOG_I("initCacheSize_: " PUBLIC_LOG_D32, static_cast<int32_t>(initCacheSize_.load()));
                initCacheSize_.store(-1);
                expectOffset_.store(-1);
            }
        }
    }
    if (!isBuffering_) {
        MEDIA_LOG_I("HandleBuffering bufferingEndCond NotifyAll.");
        bufferingEndCond_.NotifyAll();
    }
}

bool HttpMediaDownloader::IsStartDurationOfFlvMultiStream()
{
    return isRingBuffer_ && playMediaStreams_.size() > 0 &&
        (steadyClock_.ElapsedMilliseconds() - openTime_) < IGNORE_BUFFERING_WITH_START_TIME_MS;
}

bool HttpMediaDownloader::StartBufferingCheck(unsigned int& wantReadLength)
{
    AutoLock lk(savedataMutex_);
    if (!isFirstFrameArrived_ || IsStartDurationOfFlvMultiStream()) {
        if (GetCurrentBufferSize() >= wantReadLength || HandleBreak()) {
            return false;
        } else {
            waterLineAbove_ = wantReadLength;
            return true;
        }
    }
    if (HandleBreak() || isBuffering_) {
        return false;
    }
    if (!isRingBuffer_ && cacheMediaBuffer_ != nullptr && !isLargeOffsetSpan_ &&
        cacheMediaBuffer_->IsReadSplit(readOffset_) && isNeedClearHasRead_) {
        MEDIA_LOG_I("HTTP IsReadSplit, StartBuffering return, readOffset_ " PUBLIC_LOG_ZU, readOffset_);
        wantReadLength = std::min(static_cast<uint64_t>(wantReadLength), GetCurrentBufferSize());
        return false;
    }
    size_t cacheWaterLine = 0;
    size_t fileRemain = 0;
    size_t fileContenLen = downloadRequest_->GetFileContentLength();
    cacheWaterLine = std::max(static_cast<size_t>(wantReadLength), PLAY_WATER_LINE);
    if (isRingBuffer_) {
        cacheWaterLine = std::max(static_cast<size_t>(wantReadLength), FLV_PLAY_WATER_LINE);
    }
    if (fileContenLen > readOffset_) {
        fileRemain = fileContenLen - readOffset_;
        cacheWaterLine = std::min(fileRemain, cacheWaterLine);
    }
    if (isRingBuffer_ && extraCache_ >= IGNORE_BUFFERING_EXTRA_CACHE_BEYOND_MS) {
        return false;
    }
    if (GetCurrentBufferSize() >= cacheWaterLine) {
        return false;
    }
    if (GetCurrentBufferSize() >= wantReadLength) {
        return false;
    }
    return true;
}

bool HttpMediaDownloader::StartBuffering(unsigned int& wantReadLength)
{
    MEDIA_LOG_D("HTTP StartBuffering in.");
    if (!StartBufferingCheck(wantReadLength)) {
        return false;
    }

    if (isNeedResume_.load()) {
        isNeedResume_.store(false);
        totalConsumeSize_ = 0;
        downloader_->Resume();
        if (!StartBufferingCheck(wantReadLength)) {
            return false;
        }
    }

    if (!canWrite_.load()) { // Clear cacheBuffer when we can neither read nor write.
        MEDIA_LOG_I("HTTP ClearCacheBuffer.");
        ClearCacheBuffer();
        canWrite_ = true;
    }
    if (IsStartDurationOfFlvMultiStream()) {
        return false;
    }
    isBuffering_ = true;
    bufferingTime_ = static_cast<size_t>(steadyClock_.ElapsedMilliseconds());
    if (!isFirstFrameArrived_) {
        return true;
    }
    if (callback_ != nullptr) {
        MEDIA_LOG_I("HTTP CacheData OnEvent BUFFERING_START, waterLineAbove: " PUBLIC_LOG_ZU " bufferSize "
            PUBLIC_LOG_U64 " readOffset: " PUBLIC_LOG_ZU " writeOffset: " PUBLIC_LOG_ZU, waterLineAbove_,
            GetCurrentBufferSize(), readOffset_, writeOffset_);
        callback_->OnEvent({PluginEventType::BUFFERING_START, {BufferingInfoType::BUFFERING_START}, "start"});
    }
    UpdateCachedPercent(BufferingInfoType::BUFFERING_START);
    bufferingTime_ = static_cast<size_t>(steadyClock_.ElapsedMilliseconds());
    return true;
}

Status HttpMediaDownloader::ReadRingBuffer(unsigned char* buff, ReadDataInfo& readDataInfo)
{
    FALSE_RETURN_V(downloadRequest_ != nullptr, Status::ERROR_UNKNOWN);
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
        readDataInfo.isEos_ = downloadRequest_->IsEos();
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
        MEDIA_LOG_I("Http read return, isClosed: " PUBLIC_LOG_D32, isClosed);
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
    MEDIA_LOG_D("HTTP Read in: wantReadLength " PUBLIC_LOG_D32 " readOffset_ " PUBLIC_LOG_ZU " bufferSize: "
        PUBLIC_LOG_ZU, readDataInfo.wantReadLength_, readOffset_, remain);
    size_t hasReadSize = 0;
    wantedReadLength_ = static_cast<size_t>(readDataInfo.wantReadLength_);
    int64_t loopStartTime = loopInterruptClock_.ElapsedSeconds();
    while (hasReadSize < readDataInfo.wantReadLength_ && !isInterruptNeeded_.load()) {
        if (CheckLoopTimeout(loopStartTime)) {
            break;
        }
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
    if (hasReadSize > 0 || (!isLargeOffsetSpan_ && !isNeedClearHasRead_)) {
        canWrite_ = true;
    }
    if (isInterruptNeeded_.load()) {
        readDataInfo.realReadLength_ = hasReadSize;
        return Status::END_OF_STREAM;
    }
    readDataInfo.realReadLength_ = hasReadSize;
    readOffset_ += hasReadSize;
    isMinAndMaxOffsetUpdate_ = false;
    isSeekWait_ = false;
    timeoutInterval_ = MAX_BUFFERING_TIME_OUT;
    MEDIA_LOG_D("HTTP Read Success: wantReadLength " PUBLIC_LOG_D32 ", realReadLength " PUBLIC_LOG_D32 ", isEos "
        PUBLIC_LOG_D32 " readOffset_ " PUBLIC_LOG_ZU, readDataInfo.wantReadLength_,
        readDataInfo.realReadLength_, readDataInfo.isEos_, readOffset_);
    return Status::OK;
}

void HttpMediaDownloader::HandleDownloadWaterLine()
{
    if (downloader_ == nullptr || !isFirstFrameArrived_ || isBuffering_ || isLargeOffsetSpan_) {
        return;
    }
    if (!isNeedClearHasRead_ || sourceLoader_ != nullptr) {
        return;
    }
    uint64_t freeSize = cacheMediaBuffer_->GetFreeSize();
    size_t cachedDataSize = static_cast<size_t>(totalBufferSize_) > freeSize ?
                                static_cast<size_t>(totalBufferSize_) - freeSize : 0;
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
    uint64_t remain = GetCurrentBufferSize();
    if ((!isLargeOffsetSpan_ && cacheMediaBuffer_->IsReadSplit(readOffset_) && isNeedClearHasRead_) ||
        (isSeekWait_ && canWrite_)) {
        MEDIA_LOG_I("HTTP CheckDownloadPos return, IsReadSplit.");
        return;
    }
    if (remain < wantReadLength && isServerAcceptRange_ &&
        (writeOffsetTmp < readOffset_ || writeOffsetTmp > readOffset_ + remain)) {
        MEDIA_LOG_I("HTTP CheckDownloadPos, change download pos.");
        ChangeDownloadPos(remain > 0);
    }
}

Status HttpMediaDownloader::HandleRingBuffer(unsigned char* buff, ReadDataInfo& readDataInfo)
{
    if (isBuffering_ && GetBufferingTimeOut() && callback_ && !isReportedErrorCode_) {
        MEDIA_LOG_I("HTTP ringbuffer read time out.");
        callback_->OnEvent({PluginEventType::CLIENT_ERROR, {NetworkClientErrorCode::ERROR_TIME_OUT}, "read"});
        isTimeoutErrorNotified_.store(true);
        return Status::END_OF_STREAM;
    }
    bool isNeedErrorAgain = GetCurrentBufferSize() <= 0;
    if (isBuffering_ && CheckBufferingOneSeconds() && !downloadRequest_->IsChunkedVod() && isNeedErrorAgain) {
        MEDIA_LOG_I("HTTP Return error again.");
        return Status::ERROR_AGAIN;
    }
    if (StartBuffering(readDataInfo.wantReadLength_) && isNeedErrorAgain) {
        return Status::ERROR_AGAIN;
    }
    return ReadRingBuffer(buff, readDataInfo);
}

Status HttpMediaDownloader::HandleCacheBuffer(unsigned char* buff, ReadDataInfo& readDataInfo)
{
    if (isBuffering_ && GetBufferingTimeOut() && callback_ && !isReportedErrorCode_) {
        MEDIA_LOG_I("HTTP cachebuffer read time out.");
        callback_->OnEvent({PluginEventType::CLIENT_ERROR, {NetworkClientErrorCode::ERROR_TIME_OUT}, "read"});
        isTimeoutErrorNotified_.store(true);
        return Status::END_OF_STREAM;
    }
    if (isBuffering_ && canWrite_ && CheckBufferingOneSeconds() && !downloadRequest_->IsChunkedVod()) {
        MEDIA_LOG_I("HTTP Return error again.");
        return Status::ERROR_AGAIN;
    }
    UpdateMinAndMaxReadOffset();
    CheckDownloadPos(readDataInfo.wantReadLength_);
    ClearHasReadBuffer();
    if (StartBuffering(readDataInfo.wantReadLength_)) {
        return Status::ERROR_AGAIN;
    }
    Status res = ReadCacheBuffer(buff, readDataInfo);

    if (isNeedResume_.load() && readDataInfo.realReadLength_ > 0) {
        totalConsumeSize_ += readDataInfo.realReadLength_;
        if (totalConsumeSize_ > DEFAULT_WATER_LINE_ABOVE) {
            isNeedResume_.store(false);
            totalConsumeSize_ = 0;
            downloader_->Resume();
            MEDIA_LOG_D("HTTP downloader resume.");
        }
    }

    HandleDownloadWaterLine();
    return res;
}

void HttpMediaDownloader::WaitCacheBufferInit()
{
    if (cacheMediaBuffer_ == nullptr || !isCacheBufferInited_) {
        AutoLock lock(sleepMutex_);
        if (cacheMediaBuffer_ == nullptr || !isCacheBufferInited_) {
            MEDIA_LOG_I("HTTP wait for CacheBufferInit begin " PUBLIC_LOG_D32, isCacheBufferInited_);
            sleepCond_.WaitFor(lock, MAX_BUFFERING_TIME_OUT, [this]() {
                return isInterruptNeeded_.load() || isCacheBufferInited_;
            });
            MEDIA_LOG_I("HTTP wait for CacheBufferInit end " PUBLIC_LOG_D32, isCacheBufferInited_);
        }
    }
}

Status HttpMediaDownloader::ReadDelegate(unsigned char* buff, ReadDataInfo& readDataInfo)
{
    if (isRingBuffer_) {
        FALSE_RETURN_V_MSG(ringBuffer_ != nullptr, Status::END_OF_STREAM, "ringBuffer_ = nullptr");
        FALSE_RETURN_V_MSG(!isInterruptNeeded_.load(), Status::END_OF_STREAM, "isInterruptNeeded");
        FALSE_RETURN_V_MSG(readDataInfo.wantReadLength_ > 0, Status::END_OF_STREAM, "wantReadLength_ <= 0");
        return HandleRingBuffer(buff, readDataInfo);
    } else {
        WaitCacheBufferInit();
        FALSE_RETURN_V_MSG(!isInterruptNeeded_.load(), Status::END_OF_STREAM, "isInterruptNeeded");
        FALSE_RETURN_V_MSG(isCacheBufferInited_, Status::END_OF_STREAM, "CacheBufferInit fail");
        FALSE_RETURN_V_MSG(readDataInfo.wantReadLength_ > 0, Status::END_OF_STREAM, "wantReadLength_ <= 0");
        return HandleCacheBuffer(buff, readDataInfo);
    }
}

Status HttpMediaDownloader::Read(unsigned char* buff, ReadDataInfo& readDataInfo)
{
    uint64_t now = static_cast<uint64_t>(steadyClock_.ElapsedMilliseconds());
    readTime_ = now;
    auto ret = ReadDelegate(buff, readDataInfo);
    readTotalBytes_ += readDataInfo.realReadLength_;
    if (now > lastReadCheckTime_ && now - lastReadCheckTime_ > SAMPLE_INTERVAL) {
        readRecordDuringTime_ = now - lastReadCheckTime_;   // ms
        float readDuration = static_cast<float>(readRecordDuringTime_) / SECOND_TO_MILLISECONDS; // s
        if (readDuration > ZERO_THRESHOLD) {
            float readSpeed = static_cast<float>(readTotalBytes_ * BYTES_TO_BIT) / readDuration; // bps
            readBitrate_ = static_cast<uint64_t>(readSpeed);     // bps
            currentBitRate_ = readSpeed > 0 ? static_cast<int32_t>(readSpeed) : currentBitRate_;     // bps
            uint64_t curBufferSize = GetCurrentBufferSize();
            MEDIA_LOG_D("HTTP Current read speed: " PUBLIC_LOG_D32 " Kbit/s,Current buffer size: " PUBLIC_LOG_U64
            " KByte", static_cast<int32_t>(readSpeed / 1024), curBufferSize / 1024);
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
        if (readDataInfo.realReadLength_ > 0 && isLargeOffsetSpan_) {
            canWrite_ = true;
        }
        return readDataInfo.realReadLength_ == 0 ? Status::END_OF_STREAM : Status::OK;
    } else {
        readDataInfo.realReadLength_ = cacheMediaBuffer_->Read(buff, readOffset_, readDataInfo.wantReadLength_);
        readOffset_ += readDataInfo.realReadLength_;
        if (isLargeOffsetSpan_) {
            canWrite_ = true;
        }
        isMinAndMaxOffsetUpdate_ = false;
        isSeekWait_ = false;
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
    if ((!isLargeOffsetSpan_ && isMinAndMaxOffsetUpdate_) || !isNeedClearHasRead_ || sourceLoader_ != nullptr) {
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

    uint64_t span = maxReadOffset_ > minReadOffset_ ? maxReadOffset_ - minReadOffset_ : 0;
    if (span > LARGE_OFFSET_SPAN_THRESHOLD) {
        isLargeOffsetSpan_ = true;
        cacheMediaBuffer_->SetIsLargeOffsetSpan(true);
        canWrite_ = true;
    } else if (stateChangeCount_ <= STATE_CHANGE_THRESHOLD) {
        isLargeOffsetSpan_ = false;
        cacheMediaBuffer_->SetIsLargeOffsetSpan(false);
        stateChangeCount_++;
    }
    MEDIA_LOG_D("HTTP UpdateMinAndMaxReadOffset, readOffset: " PUBLIC_LOG_U64 " minReadOffset_: "
        PUBLIC_LOG_U64 " maxReadOffset_: " PUBLIC_LOG_U64, readOffsetTmp, minReadOffset_, maxReadOffset_);
}

bool HttpMediaDownloader::ChangeDownloadPos(bool isSeekHit)
{
    MEDIA_LOG_D("HTTP ChangeDownloadPos in, offset: " PUBLIC_LOG_ZU, readOffset_);

    uint64_t seekOffset = readOffset_;
    if (isSeekHit) {
        isNeedDropData_ = true;
        OSAL::SleepFor(TEN_MILLISECONDS);
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
        OSAL::SleepFor(TEN_MILLISECONDS);
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
    if (!isLargeOffsetSpan_ && cacheMediaBuffer_->IsReadSplit(offset) && isNeedClearHasRead_) {
        MEDIA_LOG_D("HTTP seek hit return, because IsReadSplit.");
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
    if (!isLargeOffsetSpan_ && static_cast<size_t>(offset) < maxReadOffset_) {
        changeDownloadPosThreshold = AUDIO_WATER_LINE_ABOVE;
    }

    if (writeOffset_ != downloadOffset && cacheMediaBuffer_->GetBufferSize(offset) < changeDownloadPosThreshold) {
        MEDIA_LOG_I("HTTP HandleSeekHit ChangeDownloadPos, writeOffset_: " PUBLIC_LOG_ZU " downloadOffset: "
            PUBLIC_LOG_ZU " bufferSize: " PUBLIC_LOG_U64, writeOffset_, downloadOffset,
            cacheMediaBuffer_->GetBufferSize(offset));
        return ChangeDownloadPos(true);
    } else {
        MEDIA_LOG_D("HTTP Seek hit, continue download.");
    }
    return true;
}

bool HttpMediaDownloader::SeekCacheBuffer(int64_t offset, bool& isSeekHit)
{
    readOffset_ = static_cast<size_t>(offset);
    if (cacheMediaBuffer_->GetBufferSize(offset) == 0 && offset < 500) { // 500
        NotifyInitSuccess();
    }
    cacheMediaBuffer_->Seek(offset); // Notify the cacheBuffer where to read.
    UpdateMinAndMaxReadOffset();

    if (!isServerAcceptRange_) {
        MEDIA_LOG_D("HTTP Don't support range, return true.");
        return true;
    }

    size_t remain = cacheMediaBuffer_->GetBufferSize(offset);
    MEDIA_LOG_I("HTTP Seek: buffer size " PUBLIC_LOG_ZU ", offset " PUBLIC_LOG_D64, remain, offset);
    if (remain > 0) {
        isSeekHit = true;
        return HandleSeekHit(offset);
    }
    isSeekHit = false;
    MEDIA_LOG_I("HTTP Seek miss.");

    size_t fileContentLength = downloadRequest_->GetFileContentLength();
    isNeedClearHasRead_ = fileContentLength > LARGE_VIDEO_THRESHOLD ? true : false;

    uint64_t diff = static_cast<size_t>(offset) > writeOffset_ ?
        static_cast<size_t>(offset) - writeOffset_ : 0;
    if (diff > 0 && diff < ALLOW_SEEK_MIN_SIZE && sourceLoader_ == nullptr) {
        isSeekWait_ = true;
        MEDIA_LOG_I("HTTP Seek miss, diff is too small so return and wait.");
        return true;
    }
    return ChangeDownloadPos(false);
}

bool HttpMediaDownloader::SeekToPos(int64_t offset, bool& isSeekHit)
{
    if (isRingBuffer_) {
        return SeekRingBuffer(offset);
    } else {
        return SeekCacheBuffer(offset, isSeekHit);
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

uint32_t HttpMediaDownloader::SaveRingBufferData(uint8_t* data, uint32_t len, bool notBlock)
{
    FALSE_RETURN_V(ringBuffer_->WriteBuffer(data, len), 0);
    cvReadWrite_.NotifyOne();
    size_t bufferSize = ringBuffer_->GetSize();
    double ratio = (static_cast<double>(bufferSize)) / RING_BUFFER_SIZE;
    if ((bufferSize >= WATER_LINE ||
        bufferSize >= downloadRequest_->GetFileContentLength() / 2) && !aboveWaterline_) { // 2
        aboveWaterline_ = true;
        MEDIA_LOG_D("HTTP Send http aboveWaterline event, ringbuffer ratio " PUBLIC_LOG_F, ratio);
        if (callback_ != nullptr) {
            callback_->OnEvent({PluginEventType::ABOVE_LOW_WATERLINE, {ratio}, "http"});
        }
        startedPlayStatus_ = true;
    } else if (bufferSize < WATER_LINE && aboveWaterline_) {
        aboveWaterline_ = false;
        MEDIA_LOG_D("HTTP Send http belowWaterline event, ringbuffer ratio " PUBLIC_LOG_F, ratio);
        if (callback_ != nullptr) {
            callback_->OnEvent({PluginEventType::BELOW_LOW_WATERLINE, {ratio}, "http"});
        }
    }
    if (writeBitrateCaculator_ != nullptr) {
        writeBitrateCaculator_->UpdateWriteBytes(len);
        writeBitrateCaculator_->StartClock();
        uint64_t writeBitrate = writeBitrateCaculator_->GetWriteBitrate();
        uint64_t writeTime  = writeBitrateCaculator_->GetWriteTime();
        if (writeTime > ONE_SECONDS) {
            MEDIA_LOG_I("waterLineAbove: " PUBLIC_LOG_ZU " writeBitrate: " PUBLIC_LOG_U64 " readBitrate: "
                PUBLIC_LOG_D32, waterLineAbove_, writeBitrate, currentBitRate_);
            writeBitrateCaculator_->ResetClock();
            if (downloadSpeeds_.size() >= FLV_AUTO_SELECT_TIME_GAP) { // 3s
                downloadSpeeds_.pop_front();
                downloadSpeeds_.push_back(writeBitrate);
                CheckAutoSelectBitrate();
            } else {
                downloadSpeeds_.push_back(writeBitrate);
            }
        }
    }
    HandleCachedDuration();
    return len;
}

uint32_t HttpMediaDownloader::SaveData(uint8_t* data, uint32_t len, bool notBlock)
{
    if (!isRingBuffer_ && (cacheMediaBuffer_ == nullptr || !isCacheBufferInited_)) {
        FALSE_RETURN_V_MSG(downloadRequest_ != nullptr, 0, "downloadRequest_ nullptr");
        if (cacheMediaBuffer_ == nullptr) {
            cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferImpl>();
        }
        if (cacheMediaBuffer_ == nullptr) {
            {
                AutoLock lock(sleepMutex_);
                isCacheBufferInited_ = false;
            }
            sleepCond_.NotifyAll();
            MEDIA_LOG_I("HTTP CacheBuffer create failed.");
            return false;
        }
        size_t fileContenLen = downloadRequest_->GetFileContentLength();
        if (fileContenLen > 0) {
            uint64_t bufferSize = static_cast<uint64_t>(fileContenLen + BUFFER_REDUNDANCY);
            totalBufferSize_ = std::min(MAX_CACHE_BUFFER_SIZE, bufferSize);
        } else {
            totalBufferSize_ = MAX_CACHE_BUFFER_SIZE;
        }
        MEDIA_LOG_I("HTTP setting buffer size: " PUBLIC_LOG_D32 " fileContenLen: " PUBLIC_LOG_ZU,
            totalBufferSize_, fileContenLen);
        {
            AutoLock lock(sleepMutex_);
            isCacheBufferInited_ = cacheMediaBuffer_->Init(totalBufferSize_, CHUNK_SIZE);
        }
        sleepCond_.NotifyAll();
        FALSE_RETURN_V_MSG(isCacheBufferInited_, 0, "HTTP CacheBufferInit failed");
    }

    if (cacheMediaBuffer_ == nullptr && ringBuffer_ == nullptr) {
        return 0;
    }
    OnWriteBuffer(len);
    uint32_t ret = 0;
    if (isRingBuffer_) {
        ret = SaveRingBufferData(data, len, notBlock);
    } else {
        ret = SaveCacheBufferData(data, len, notBlock);
        HandleDownloadWaterLine();
    }
    HandleBuffering();
    return ret;
}

bool HttpMediaDownloader::CacheBufferFullLoop()
{
    {
        AutoLock lock(initCacheMutex_);
        if (initCacheSize_.load() != -1 &&
            GetBufferSize() >= static_cast<size_t>(initCacheSize_.load())) {
            callback_->OnEvent({PluginEventType::INITIAL_BUFFER_SUCCESS,
                                    {BufferingInfoType::BUFFERING_END}, "end"});
            initCacheSize_.store(-1);
            expectOffset_.store(-1);
        }
    }
    MEDIA_LOGI_LIMIT(SAVE_DATA_LOG_FREQUENCE, "HTTP CacheMediaBuffer full, waiting seek or read.");
    if (isHitSeeking_ || isNeedDropData_) {
        canWrite_ = true;
        return true;
    }
    OSAL::SleepFor(ONE_HUNDRED_MILLIONSECOND);
    return false;
}

uint32_t HttpMediaDownloader::SaveCacheBufferDataNotblock(uint8_t* data, uint32_t len)
{
    AutoLock lk(savedataMutex_);
    isServerAcceptRange_ = downloadRequest_->IsServerAcceptRange();

    size_t res = cacheMediaBuffer_->Write(data, writeOffset_, len);
    writeOffset_ += res;

    MEDIA_LOGI_LIMIT(SAVE_DATA_LOG_FREQUENCE, "SaveCacheBufferDataNotblock writeOffset " PUBLIC_LOG_ZU " res "
        PUBLIC_LOG_ZU, writeOffset_, res);

    writeBitrateCaculator_->UpdateWriteBytes(res);
    uint64_t freeSize = cacheMediaBuffer_->GetFreeSize();
    MEDIA_LOGI_LIMIT(SAVE_DATA_LOG_FREQUENCE, "SaveCacheBufferDataNotblock, freeSize: " PUBLIC_LOG_U64, freeSize);
    HandleCachedDuration();
    writeBitrateCaculator_->StartClock();
    uint64_t writeTime  = writeBitrateCaculator_->GetWriteTime() / SECOND_TO_MILLISECONDS;
    if (writeTime > ONE_SECONDS) {
        writeBitrateCaculator_->ResetClock();
    }

    if (res < len) {
        cacheMediaBuffer_->Dump(0);
        isNeedResume_.store(true);
    }

    return res;
}

uint32_t HttpMediaDownloader::SaveCacheBufferData(uint8_t* data, uint32_t len, bool notBlock)
{
    if (notBlock) {
        return SaveCacheBufferDataNotblock(data, len);
    }

    if (isNeedClean_) {
        return len;
    }

    isServerAcceptRange_ = downloadRequest_->IsServerAcceptRange();

    size_t hasWriteSize = 0;
    while (hasWriteSize < len && !isInterruptNeeded_.load() && !isInterrupt_.load()) {
        if (isNeedClean_) {
            MEDIA_LOGI_LIMIT(SAVE_DATA_LOG_FREQUENCE, "isNeedClean true.");
            return len;
        }
        size_t res = cacheMediaBuffer_->Write(data + hasWriteSize, writeOffset_, len - hasWriteSize);
        writeOffset_ += res;
        hasWriteSize += res;
        writeBitrateCaculator_->UpdateWriteBytes(res);
        MEDIA_LOGI_LIMIT(SAVE_DATA_LOG_FREQUENCE, "HTTP writeOffset " PUBLIC_LOG_ZU " res "
            PUBLIC_LOG_ZU, writeOffset_, res);
        if ((isLargeOffsetSpan_ || canWrite_.load()) && (res > 0 || hasWriteSize == len)) {
            HandleCachedDuration();
            writeBitrateCaculator_->StartClock();
            uint64_t writeTime  = writeBitrateCaculator_->GetWriteTime() / SECOND_TO_MILLISECONDS;
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
            if (CacheBufferFullLoop()) {
                return len;
            }
        }
        canWrite_ = true;
    }
    if (isInterruptNeeded_.load() || isInterrupt_.load()) {
        MEDIA_LOG_I("HTTP isInterruptNeeded true, return false.");
        return 0;
    }
    return len;
}

void HttpMediaDownloader::OnWriteBuffer(uint32_t len)
{
    {
        AutoLock lock(initCacheMutex_);
        if (initCacheSize_.load() != -1 &&
            GetCurrentBufferSize() >= static_cast<uint64_t>(initCacheSize_.load())) {
            callback_->OnEvent({ PluginEventType::INITIAL_BUFFER_SUCCESS, {BufferingInfoType::BUFFERING_END}, "end"});
            initCacheSize_.store(-1);
            expectOffset_.store(-1);
        }
    }
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
    double tmpDenominator = static_cast<double>(downloadDuringTime_) / SECOND_TO_MILLISECONDS;
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
            uint64_t remainingBuffer = GetCurrentBufferSize();    // Byte
            MEDIA_LOG_D("Current download speed : " PUBLIC_LOG_D32 " Kbit/s,Current buffer size : " PUBLIC_LOG_U64
                " KByte", static_cast<int32_t>(downloadRate / 1024), remainingBuffer / 1024);
            MediaAVCodec::AVCodecTrace trace("HttpMediaDownloader::DownloadReport, download speed: " +
                std::to_string(downloadRate) + " bit/s, bufferSize: " + std::to_string(remainingBuffer) + " Byte");
            // Remaining playable time: s
            uint64_t bufferDuration = 0;
            if (readBitrate_ > 0) {
                bufferDuration = remainingBuffer * BYTES_TO_BIT / readBitrate_;
            } else {
                bufferDuration = remainingBuffer * BYTES_TO_BIT / CURRENT_BIT_RATE;
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
    if (callback_ != nullptr && !isReportedErrorCode_) {
        callback_->OnEvent({PluginEventType::CLIENT_ERROR, {NetworkClientErrorCode::ERROR_NOT_RETRY}, "read"});
        isTimeoutErrorNotified_.store(true);
    }
    Close(true);
}

void HttpMediaDownloader::SetInterruptState(bool isInterruptNeeded)
{
    MEDIA_LOG_I("SetInterruptState: " PUBLIC_LOG_D32, isInterruptNeeded);
    {
        AutoLock lk(bufferingEndMutex_);
        isInterruptNeeded_ = isInterruptNeeded;
        if (isInterruptNeeded_) {
            MEDIA_LOG_I("SetInterruptState, bufferingEndCond NotifyAll.");
            bufferingEndCond_.NotifyAll();
            sleepCond_.NotifyAll();
        }
    }
    if (ringBuffer_ != nullptr && isInterruptNeeded) {
        ringBuffer_->SetActive(false);
    }
    if (downloader_ != nullptr) {
        downloader_->SetInterruptState(isInterruptNeeded);
    }
}

uint64_t HttpMediaDownloader::GetBufferSize() const
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
    double tmpDownloadTime = static_cast<double>(totalDownloadDuringTime_) / SECOND_TO_MILLISECONDS;
    if (tmpDownloadTime > ZERO_THRESHOLD) {
        playbackInfo.averageDownloadRate = static_cast<int64_t>(totalBits_ / tmpDownloadTime);
    } else {
        playbackInfo.averageDownloadRate = 0;
    }
    playbackInfo.isDownloading = isDownloadFinish_ ? false : true;
    if (recordData_ != nullptr) {
        playbackInfo.downloadRate = static_cast<int64_t>(recordData_->downloadRate);
        uint64_t remainingBuffer = GetCurrentBufferSize();
        uint64_t bufferDuration = 0;
        if (readBitrate_ > 0) {
            bufferDuration = remainingBuffer * BYTES_TO_BIT / readBitrate_;
        } else {
            bufferDuration = remainingBuffer * BYTES_TO_BIT / CURRENT_BIT_RATE;
        }
        playbackInfo.bufferDuration = static_cast<int64_t>(bufferDuration);
    } else {
        playbackInfo.downloadRate = 0;
        playbackInfo.bufferDuration = 0;
    }
}

bool HttpMediaDownloader::HandleBreak()
{
    MEDIA_LOG_D("HTTP HandleBreak");
    if (downloadErrorState_) {
        MEDIA_LOG_I("downloadErrorState true.");
        return true;
    }
    if (downloadRequest_ == nullptr) {
        MEDIA_LOG_I("downloadRequest is nullptr.");
        return true;
    }
    if (downloadRequest_->IsEos()) {
        MEDIA_LOG_I("isEos true");
        return true;
    }
    if (downloadRequest_->IsClosed()) {
        MEDIA_LOG_I("IsClosed true");
        return true;
    }
    if (downloadRequest_->IsChunkedVod()) {
        MEDIA_LOG_I("IsChunkedVod true");
        return true;
    }
    return false;
}

uint64_t HttpMediaDownloader::GetCurrentBufferSize() const
{
    uint64_t bufferSize = 0;
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
        videoBitrate_ = std::max(videoBitrate_, static_cast<uint32_t>(DEFAULT_BIT_RATE));
    } else {
        videoBitrate_ = std::max(videoBitrate_, static_cast<uint32_t>(bitRate));
    }
    currentBitRate_ = static_cast<int32_t>(videoBitrate_);
    if (downloadRequest_) {
        downloadRequest_->SetBitRateToRequestSize(currentBitRate_);
    }
    return Status::OK;
}

void HttpMediaDownloader::HandleCachedDuration()
{
    int32_t tmpBitRate = currentBitRate_;
    if (tmpBitRate <= 0 || callback_ == nullptr) {
        return;
    }
    cachedDuration_ = GetCurrentBufferSize() *
        BYTES_TO_BIT * SECOND_TO_MILLISECONDS / static_cast<uint64_t>(tmpBitRate);
    // Subtraction of unsigned integers requires size comparison first.
    if ((cachedDuration_ > lastDurationReacord_ &&
        cachedDuration_ - lastDurationReacord_ > DURATION_CHANGE_AMOUT_MILLIONSECOND) ||
        (lastDurationReacord_ > cachedDuration_ &&
        lastDurationReacord_ - cachedDuration_ > DURATION_CHANGE_AMOUT_MILLIONSECOND)) {
        MEDIA_LOG_D("HTTP OnEvent cachedDuration: " PUBLIC_LOG_U64, cachedDuration_);
        callback_->OnEvent({PluginEventType::CACHED_DURATION, {cachedDuration_}, "buffering_duration"});
        lastDurationReacord_ = cachedDuration_;
    }
}

void HttpMediaDownloader::UpdateCachedPercent(BufferingInfoType infoType)
{
    if (waterLineAbove_ == 0 || callback_ == nullptr) {
        MEDIA_LOG_E("UpdateCachedPercent: ERROR");
        return;
    }
    if (infoType == BufferingInfoType::BUFFERING_START) {
        lastCachedSize_ = 0;
        isBufferingStart_ = true;
        return;
    }
    if (infoType == BufferingInfoType::BUFFERING_END) {
        lastCachedSize_ = 0;
        isBufferingStart_ = false;
        return;
    }
    if (infoType != BufferingInfoType::BUFFERING_PERCENT || !isBufferingStart_) {
        return;
    }
    uint64_t bufferSize = GetCurrentBufferSize();
    if (bufferSize < lastCachedSize_) {
        return;
    }
    uint64_t deltaSize = bufferSize - lastCachedSize_;
    if (deltaSize >= UPDATE_CACHE_STEP) {
        uint64_t percentValue = (bufferSize >= waterLineAbove_) ?
                        100 : bufferSize * 100 / waterLineAbove_; // 100
        int percent = static_cast<int>(percentValue);
        callback_->OnEvent({PluginEventType::EVENT_BUFFER_PROGRESS, {percent}, "buffer percent"});
        lastCachedSize_ = bufferSize;
    }
}

bool HttpMediaDownloader::CheckBufferingOneSeconds()
{
    MEDIA_LOG_I("HTTP CheckBufferingOneSeconds in");
    int32_t sleepTime = 0;
    // return error again 1 time 1s, avoid ffmpeg error
    while (sleepTime < (isFirstFrameArrived_ ? TWO_SECONDS : ONE_HUNDRED_MILLIONSECOND) &&
           !isInterruptNeeded_.load()) {
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
    return isBuffering_.load();
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
    currentBitRate_ = std::max(currentBitRate_, static_cast<int32_t>(videoBitrate_));
    if (currentBitRate_ > 0) {
        float cacheTime = 0;
        uint64_t writeBitrate = writeBitrateCaculator_->GetWriteBitrate();
        if (writeBitrate > 0) {
            float ratio = static_cast<float>(writeBitrate) / currentBitRate_;
            cacheTime = GetCacheDuration(ratio);
        } else {
            cacheTime = DEFAULT_CACHE_TIME;
        }
        if (isRingBuffer_) {
            cacheTime -= static_cast<float>(static_cast<double>(extraCache_) / static_cast<double>(ONE_SECONDS));
            cacheTime = std::max(cacheTime, 0.0f);
        }
        waterLineAbove = static_cast<size_t>(cacheTime * currentBitRate_ / BYTES_TO_BIT);
        if (!isRingBuffer_) {
            waterLineAbove = std::max(MIN_WATER_LINE_ABOVE, waterLineAbove);
        }
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
    waterLineAbove_ = std::min(waterLineAbove_, MAX_WATER_LINE_ABOVE);
    MEDIA_LOG_D("UpdateWaterLineAbove: " PUBLIC_LOG_ZU " writeBitrate: " PUBLIC_LOG_U64 " readBitrate: "
        PUBLIC_LOG_D32, waterLineAbove_, writeBitrateCaculator_->GetWriteBitrate(), currentBitRate_);
}

bool HttpMediaDownloader::GetPlayable()
{
    if (isBuffering_) {
        return false;
    }
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
        return now >= bufferingTime_ ? now - bufferingTime_ >= timeoutInterval_ : false;
    }
}

bool HttpMediaDownloader::GetReadTimeOut(bool isDelay)
{
    size_t now = static_cast<size_t>(steadyClock_.ElapsedMilliseconds());
    if (isDelay) {
        timeoutInterval_ = MAX_BUFFERING_TIME_OUT_DELAY;
    }
    return (now >= readTime_) ? (now - readTime_ >= timeoutInterval_) : false;
}

Status HttpMediaDownloader::StopBufferring(bool isAppBackground)
{
    MEDIA_LOG_I("HttpMediaDownloader:StopBufferring enter");
    if (downloader_ == nullptr) {
        MEDIA_LOG_E("StopBufferring error.");
        return Status::ERROR_NULL_POINTER;
    }
    isAppBackground_ = isAppBackground;
    downloader_->SetAppState(isAppBackground);
    if (isAppBackground) {
        if (ringBuffer_ != nullptr) {
            //flv will relink, unactive buffer to interupt download and clean data.
            ringBuffer_->SetActive(false, true);
        }
        if (cacheMediaBuffer_ != nullptr) {
            isInterrupt_ = true;
        }
    } else {
        if (ringBuffer_ != nullptr) {
            ringBuffer_->SetActive(true, true);
        }
        if (cacheMediaBuffer_ != nullptr) {
            isInterrupt_ = false;
        }
    }
    bufferingTime_ = static_cast<size_t>(steadyClock_.ElapsedMilliseconds());
    downloader_->StopBufferring();
    MEDIA_LOG_I("HttpMediaDownloader:StopBufferring out");
    return Status::OK;
}

void HttpMediaDownloader::WaitForBufferingEnd()
{
    AutoLock lk(bufferingEndMutex_);
    FALSE_RETURN_MSG(isBuffering_.load(), "isBuffering false.");
    MEDIA_LOG_I("WaitForBufferingEnd");
    bufferingEndCond_.Wait(lk, [this]() {
        MEDIA_LOG_I("Wait in, isBuffering: " PUBLIC_LOG_D32 " isInterruptNeeded: " PUBLIC_LOG_D32,
            isBuffering_.load(), isInterruptNeeded_.load());
        return !isBuffering_.load() || isInterruptNeeded_.load();
    });
}

bool HttpMediaDownloader::ClearHasReadBuffer()
{
    if (!isFirstFrameArrived_ || cacheMediaBuffer_ == nullptr || isLargeOffsetSpan_) {
        return false;
    }
    if (!isNeedClearHasRead_) {
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
    MEDIA_LOG_D("HTTP ClearHasReadBuffer, res: " PUBLIC_LOG_D32 " clearOffset: " PUBLIC_LOG_U64 " minClearOffset: "
        PUBLIC_LOG_U64 " maxClearOffset: " PUBLIC_LOG_U64, res, clearOffset, minClearOffset, maxClearOffset);
    return res;
}

void HttpMediaDownloader::ClearCacheBuffer()
{
    if (cacheMediaBuffer_ == nullptr || downloader_ == nullptr) {
        return;
    }
    MEDIA_LOG_I("HTTP ClearCacheBuffer begin.");
    isNeedDropData_ = true;
    downloader_->Pause();
    cacheMediaBuffer_->Clear();
    isNeedDropData_ = false;
    downloader_->Seek(readOffset_);
    writeOffset_ = readOffset_;
    downloader_->Resume();
    uint64_t freeSize = cacheMediaBuffer_->GetFreeSize();
    MEDIA_LOG_I("HTTP ClearCacheBuffer end, freeSize: " PUBLIC_LOG_U64, freeSize);
}

void HttpMediaDownloader::SetIsReportedErrorCode()
{
    isReportedErrorCode_ = true;
}

bool HttpMediaDownloader::SetInitialBufferSize(int32_t offset, int32_t size)
{
    AutoLock lock(initCacheMutex_);
    bool isInitBufferSizeOk = GetCurrentBufferSize() >= static_cast<uint64_t>(size) || HandleBreak();
    if (isInitBufferSizeOk || !downloader_ || !downloadRequest_ || isTimeoutErrorNotified_.load()) {
        MEDIA_LOG_I("HTTP SetInitialBufferSize initCacheSize ok.");
        return false;
    }
    if (downloadRequest_->IsChunkedVod()) {
        MEDIA_LOG_I("ChunkedVod, fail to SetInitBufferSize.");
        return false;
    }
    MEDIA_LOG_I("HTTP SetInitialBufferSize initCacheSize " PUBLIC_LOG_U32, size);
    if (!isBuffering_.load()) {
        isBuffering_.store(true);
    }
    bufferingTime_ = static_cast<size_t>(steadyClock_.ElapsedMilliseconds());
    expectOffset_.store(offset);
    initCacheSize_.store(size);
    return true;
}

void HttpMediaDownloader::SetPlayStrategy(const std::shared_ptr<PlayStrategy>& playStrategy)
{
    if (playStrategy == nullptr) {
        MEDIA_LOG_E("HTTP SetPlayStrategy error.");
        return;
    }
    if (playStrategy->bufferDurationForPlaying > 0) {
        bufferDurationForPlaying_ = playStrategy->bufferDurationForPlaying;
        waterlineForPlaying_ = static_cast<uint64_t>(static_cast<double>(CURRENT_BIT_RATE) /
            static_cast<double>(BYTES_TO_BIT) * bufferDurationForPlaying_);
        MEDIA_LOG_I("HTTP buffer duration for playing : " PUBLIC_LOG ".3f", bufferDurationForPlaying_);
    }
    if (playStrategy->width > 0 && playStrategy->width < USHRT_MAX
        && playStrategy->height > 0 && playStrategy->height < USHRT_MAX) {
        initResolution_ = playStrategy->width * playStrategy->height;
        ChooseStreamByResolution();
    }
}

bool HttpMediaDownloader::IsNeedBufferForPlaying()
{
    if (bufferDurationForPlaying_ <= 0 || !isDemuxerInitSuccess_.load() || !isBuffering_.load()) {
        return false;
    }
    if (GetBufferingTimeOut()) {
        if (callback_) {
            callback_->OnEvent({PluginEventType::CLIENT_ERROR, {NetworkClientErrorCode::ERROR_TIME_OUT},
                "buffer for playing"});
        }
        isTimeoutErrorNotified_.store(true);
        AutoLock lk(bufferingEndMutex_);
        isBuffering_.store(false);
        bufferingEndCond_.NotifyAll();
        isDemuxerInitSuccess_.store(false);
        bufferingTime_ = 0;
        return false;
    }
    if (GetCurrentBufferSize() >= waterlineForPlaying_ || HandleBreak()) {
        MEDIA_LOG_I("HTTP buffer duration for playing is enough, buffersize: " PUBLIC_LOG_U64 " waterLineAbove: "
                    PUBLIC_LOG_U64, GetCurrentBufferSize(), waterlineForPlaying_);
        AutoLock lk(bufferingEndMutex_);
        isBuffering_.store(false);
        bufferingEndCond_.NotifyAll();
        isDemuxerInitSuccess_.store(false);
        bufferingTime_ = 0;
        if (isRingBuffer_ && callback_) {
            callback_->OnEvent({PluginEventType::BUFFERING_END, {BufferingInfoType::BUFFERING_END}, "end"});
        }
        return false;
    }
    return true;
}

void HttpMediaDownloader::NotifyInitSuccess()
{
    MEDIA_LOG_I("HTTP NotifyInitSuccess in");
    isDemuxerInitSuccess_.store(true);
    if (bufferDurationForPlaying_ <= 0 || currentBitRate_ <= 0) {
        return;
    }
    if (currentBitRate_ > 0) {
        waterlineForPlaying_ = static_cast<uint64_t>(static_cast<double>(currentBitRate_) /
            static_cast<double>(BYTES_TO_BIT) * bufferDurationForPlaying_);
    }
    isBuffering_.store(true);
    bufferingTime_ = static_cast<size_t>(steadyClock_.ElapsedMilliseconds());
}

uint64_t HttpMediaDownloader::GetCachedDuration()
{
    MEDIA_LOG_I("HTTP GetCachedDuration: " PUBLIC_LOG_U64, cachedDuration_);
    return cachedDuration_;
}

void HttpMediaDownloader::RestartAndClearBuffer()
{
    FALSE_RETURN_MSG(downloader_ != nullptr, "downloader_ is nullptr");
    FALSE_RETURN_MSG(ringBuffer_ != nullptr || cacheMediaBuffer_ != nullptr, "buffer is nullptr");
    MEDIA_LOG_I("HTTP RestartAndClearBuffer in.");
    {
        AutoLock lk(bufferingEndMutex_);
        isBuffering_.store(false);
        bufferingEndCond_.NotifyAll();
    }
    isAllowResume_.store(true);
    if (isRingBuffer_) {
        ringBuffer_->SetActive(false);
        downloader_->Pause();
        ringBuffer_->SetActive(true);
        MEDIA_LOG_I("HTTP clear ringbuffer done.");
    } else {
        downloader_->Pause();
        cacheMediaBuffer_->Clear();
        MEDIA_LOG_I("HTTP clear cachebuffer done.");
    }
    downloader_->Resume();
    isAllowResume_.store(true);
    MEDIA_LOG_I("HTTP RestartAndClearBuffer out.");
}

bool HttpMediaDownloader::IsFlvLive()
{
    FALSE_RETURN_V_MSG_E(downloader_ != nullptr, false, "downloader_ is nullptr");
    FALSE_RETURN_V_MSG_E(downloadRequest_ != nullptr, false, "downloadRequest_ is nullptr");
    size_t fileContenLen = downloadRequest_->GetFileContentLength();
    return fileContenLen == 0 && isRingBuffer_;
}

void HttpMediaDownloader::SetStartPts(int64_t startPts)
{
    flvStartPts_ = startPts;
}

void HttpMediaDownloader::SetExtraCache(uint64_t cacheDuration)
{
    extraCache_ = cacheDuration;
    MEDIA_LOG_D("SetExtraCache extraCache_=" PUBLIC_LOG_U64, extraCache_);
}

bool HttpMediaDownloader::SelectBitRate(uint32_t bitRate)
{
    if (playMediaStreams_.size() <= 1 || !isRingBuffer_) {
        MEDIA_LOG_E("HTTP SelectBitRate error.");
        return true;
    }
    if (ringBuffer_ == nullptr || downloader_ == nullptr || downloadRequest_ == nullptr) {
        return true;
    }
    MEDIA_LOG_I("HTTP SelectBitRate " PUBLIC_LOG_U32, bitRate);
    defaultStream_ = playMediaStreams_.front();
    uint32_t preBitrate = 0;
    for (const auto &stream : playMediaStreams_) {
        if (preBitrate == 0 && bitRate <= stream->bitrate) {
            break;
        } else if (bitRate > preBitrate && bitRate <= stream->bitrate) {
            uint32_t deltaA = bitRate - preBitrate;
            uint32_t deltaB = stream->bitrate - bitRate;
            deltaB < deltaA ? defaultStream_ = stream : 0;
            break;
        } else if (preBitrate == stream->bitrate) {
            continue;
        } else {
            defaultStream_ = stream;
            preBitrate = stream->bitrate;
        }
    }
    openTime_ = steadyClock_.ElapsedMilliseconds();
    isSelectingBitrate_.store(true);
    ringBuffer_->SetActive(false, true);
    downloadSpeeds_.clear();
    downloader_->Pause(false);
    ringBuffer_->SetActive(true, true);
    isSelectingBitrate_.store(false);
    std::string url = defaultStream_->url;
    AddParamForUrl(url, "startPts", std::to_string(flvStartPts_));
    MEDIA_LOG_I("flvStartPts_ " PUBLIC_LOG_D64, flvStartPts_);
    currentBitRate_ = static_cast<int32_t>(defaultStream_->bitrate);
    downloadRequest_->SetUrl(url);
    downloadRequest_->SetRangePos(0, -1);
    downloader_->Resume();
    return true;
}

void HttpMediaDownloader::AddParamForUrl(std::string& url, const std::string& key, const std::string& value)
{
    std::string param;
    if (url.find("?") == std::string::npos) {
        param = "?" + key + "=" + value;
    } else {
        param = "&" + key + "=" + value;
    }
    url += param;
}

void HttpMediaDownloader::SetMediaStreams(const MediaStreamList& mediaStreams)
{
    MEDIA_LOG_I("HTTP MediaStreams size is " PUBLIC_LOG_ZU, static_cast<size_t>(mediaStreams.size()));
    playMediaStreams_ = mediaStreams;
    defaultStream_ = playMediaStreams_.empty() ? nullptr : playMediaStreams_.front();
}

void HttpMediaDownloader::ChooseStreamByResolution()
{
    if (initResolution_ == 0 || defaultStream_ == nullptr) {
        return;
    }
    for (const auto &stream : playMediaStreams_) {
        if (stream == nullptr) {
            continue;
        }
        if (IsNearToInitResolution(defaultStream_, stream)) {
            defaultStream_ = stream;
        }
    }
    MEDIA_LOG_I("resolution, width:" PUBLIC_LOG_U32 ", height:" PUBLIC_LOG_U32,
                defaultStream_->width, defaultStream_->height);
}

bool HttpMediaDownloader::IsNearToInitResolution(const std::shared_ptr<PlayMediaStream> &choosedStream,
    const std::shared_ptr<PlayMediaStream> &currentStream)
{
    if (choosedStream == nullptr || currentStream == nullptr || initResolution_ == 0) {
        return false;
    }
    uint32_t choosedDelta = GetResolutionDelta(choosedStream->width, choosedStream->height);
    uint32_t currentDelta = GetResolutionDelta(currentStream->width, currentStream->height);
    return (currentDelta < choosedDelta)
           || (currentDelta == choosedDelta && currentStream->bitrate < choosedStream->bitrate);
}

uint32_t HttpMediaDownloader::GetResolutionDelta(uint32_t width, uint32_t height)
{
    if (width >= USHRT_MAX || height >= USHRT_MAX) {
        return 0;
    }
    
    uint32_t resolution = width * height;
    if (resolution > initResolution_) {
        return resolution - initResolution_;
    } else {
        return initResolution_ - resolution;
    }
}

bool HttpMediaDownloader::CheckLoopTimeout(int64_t startLoopTime)
{
    int64_t now = loopInterruptClock_.ElapsedSeconds();
    int64_t loopDuration = now > startLoopTime ? now - startLoopTime : 0;
    bool isLoopTimeout = loopDuration > LOOP_TIMEOUT ? true : false;
    if (isLoopTimeout) {
        SetDownloadErrorState();
        MEDIA_LOG_E("loop timeout");
    }
    return isLoopTimeout;
}

void HttpMediaDownloader::SetIsTriggerAutoMode(bool isAuto)
{
    isAutoSelectBitrate_.store(isAuto);
}

bool HttpMediaDownloader::AutoSelectBitRate(uint32_t bitRate)
{
    SelectBitRate(bitRate);
    return true;
}

bool HttpMediaDownloader::CheckAutoSelectBitrate()
{
    if (!IsAutoSelectConditionOk()) {
        return false;
    }
    uint64_t sumSpeed = 0;
    for (const auto &speed : downloadSpeeds_) {
        sumSpeed += speed;
    }
    uint64_t aveSpeed = sumSpeed / downloadSpeeds_.size();
    uint32_t desBitRate = 0;
    for (const auto &item : playMediaStreams_) {
        if (defaultStream_->bitrate == item->bitrate) {
            continue;
        }
        if (item->bitrate > 0 && item->bitrate == desBitRate) {
            break;
        }
        uint32_t smoothSpeed = static_cast<uint32_t>(aveSpeed * FLV_AUTO_SELECT_SMOOTH_FACTOR);
        if (defaultStream_->bitrate < item->bitrate && item->bitrate <= smoothSpeed) {
            desBitRate = item->bitrate;
        }
        if (defaultStream_->bitrate > item->bitrate && aveSpeed < defaultStream_->bitrate &&
            item->bitrate <= smoothSpeed) {
            desBitRate = item->bitrate;
        }
    }
    if (desBitRate <= 0 || desBitRate == defaultStream_->bitrate) {
        return false;
    }
    MEDIA_LOG_I("HTTP CheckAutoSelectBitrate aveRate " PUBLIC_LOG_U64 " desRate " PUBLIC_LOG_U32, aveSpeed, desBitRate);
    if (isAutoSelectBitrate_.load() && callback_) {
        callback_->OnEvent({PluginEventType::FLV_AUTO_SELECT_BITRATE, {desBitRate},
            "auto select bitrate"});
    }
    return true;
}

bool HttpMediaDownloader::IsAutoSelectConditionOk()
{
    if (!isAutoSelectBitrate_.load()) {
        return false;
    }
    if (!isRingBuffer_) {
        return false;
    }
    if (playMediaStreams_.size() <= 1) {
        return false;
    }
    if (defaultStream_ == nullptr) {
        return false;
    }
    if (isSelectingBitrate_.load()) {
        return false;
    }
    if (downloadSpeeds_.size() <= 0) {
        return false;
    }
    return true;
}

void HttpMediaDownloader::ClearBuffer()
{
    FALSE_RETURN_MSG(downloader_ != nullptr, "downloader_ is nullptr, fail to ClearBuffer");
    FALSE_RETURN_MSG(ringBuffer_ != nullptr || cacheMediaBuffer_ != nullptr, "buffer is nullptr.");
    if (isRingBuffer_) {
        size_t sizeBefore = ringBuffer_->GetFreeSize();
        ringBuffer_->SetActive(false);
        ringBuffer_->SetActive(true);
        size_t sizeAfter = ringBuffer_->GetFreeSize();
        MEDIA_LOG_I("HTTP ClearBuffer done, sizeBefore: " PUBLIC_LOG_ZU " sizeAfter: " PUBLIC_LOG_ZU,
            sizeBefore, sizeAfter);
    } else {
        size_t sizeBefore = cacheMediaBuffer_->GetFreeSize();
        cacheMediaBuffer_->Clear();
        size_t sizeAfter = cacheMediaBuffer_->GetFreeSize();
        MEDIA_LOG_I("HTTP ClearBuffer done, sizeBefore: " PUBLIC_LOG_ZU " sizeAfter: " PUBLIC_LOG_ZU,
            sizeBefore, sizeAfter);
    }
}

uint64_t HttpMediaDownloader::GetMemorySize()
{
    if (totalBufferSize_ <= 0) {
        return 0;
    }
    return static_cast<uint64_t>(totalBufferSize_);
}

std::string HttpMediaDownloader::GetCurUrl()
{
    FALSE_RETURN_V_MSG_E(downloadRequest_ != nullptr, "", "currentRequest_ is nullptr");
    GetContentLength();
    if (!downloadRequest_->haveRedirectRetry_.load()) {
        return "";
    }
    return downloadRequest_->GetUrl();
}
}
}
}
}