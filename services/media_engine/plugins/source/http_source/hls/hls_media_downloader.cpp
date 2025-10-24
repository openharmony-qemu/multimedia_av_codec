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
#define HST_LOG_TAG "HlsMediaDownloader"

#include "hls_media_downloader.h"
#include "media_downloader.h"
#include "hls_playlist_downloader.h"
#include "securec.h"
#include <algorithm>
#include "plugin/plugin_time.h"
#include "openssl/aes.h"
#include "osal/task/task.h"
#include "network/network_typs.h"
#include "common/media_core.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <regex>
#include "avcodec_trace.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
namespace {
constexpr uint32_t DECRYPT_COPY_LEN = 128;
constexpr int MIN_WIDTH = 480;
constexpr int SECOND_WIDTH = 720;
constexpr int THIRD_WIDTH = 1080;
constexpr uint64_t MAX_BUFFER_SIZE = 19 * 1024 * 1024;
constexpr uint32_t SAMPLE_INTERVAL = 1000; // Sampling time interval: ms
constexpr int MAX_RECORD_COUNT = 10;
constexpr int START_PLAY_WATER_LINE = 512 * 1024;
constexpr int DATA_USAGE_INTERVAL = 300 * 1000;
constexpr double ZERO_THRESHOLD = 1e-9;
constexpr size_t PLAY_WATER_LINE = 5 * 1024;
constexpr int IS_DOWNLOAD_MIN_BIT = 100; // Determine whether it is downloading
constexpr size_t DEFAULT_WATER_LINE_ABOVE = 512 * 1024;
constexpr uint32_t DURATION_CHANGE_AMOUT_MILLISECONDS = 500;
constexpr int UPDATE_CACHE_STEP = 5 * 1024;
constexpr int SEEK_STATUS_RETRY_TIMES = 100;
constexpr int SEEK_STATUS_SLEEP_TIME = 50;
constexpr uint64_t CURRENT_BIT_RATE = 1 * 1024 * 1024; // bps
constexpr int32_t ONE_SECONDS = 1000;
constexpr int32_t TEN_MILLISECONDS = 10;
constexpr size_t MIN_WATER_LINE_ABOVE = 300 * 1024;
constexpr float WATER_LINE_ABOVE_LIMIT_RATIO = 0.6;
constexpr float CACHE_LEVEL_1 = 0.5;
constexpr float DEFAULT_CACHE_TIME = 5;
constexpr int TRANSFER_SIZE_RATE_2 = 2;
constexpr int TRANSFER_SIZE_RATE_3 = 3;
constexpr int TRANSFER_SIZE_RATE_4 = 4;
constexpr int SLEEP_TIME_100 = 100;
constexpr size_t MAX_BUFFERING_TIME_OUT = 30 * 1000;
constexpr size_t MAX_BUFFERING_TIME_OUT_DELAY = 60 * 1000;
constexpr int32_t HUNDRED_PERCENTS = 100;
constexpr int32_t HALF_DIVIDE = 2;
constexpr uint64_t READ_BACK_SAVE_SIZE = 1 * 1024 * 1024;
constexpr int32_t SAVE_DATA_LOG_FREQUENCY = 50;
constexpr uint32_t KILO = 1024;
constexpr uint64_t RESUME_FREE_SIZE_THRESHOLD = 2 * 1024 * 1024;
constexpr size_t STORP_WRITE_BUFFER_REDUNDANCY = 1 * 1024 * 1024;
constexpr int MAX_RETRY = 10;
constexpr uint32_t MAX_LOOP_TIMES = 100;
constexpr uint64_t MAX_EXPECT_DURATION = 19;
constexpr uint32_t POP_TIME_OUT_MS = 1;
}

//   hls manifest, m3u8 --- content get from m3u8 url, we get play list from the content
//   fragment --- one item in play list, download media data according to the fragment address.
HlsMediaDownloader::HlsMediaDownloader(int expectBufferDuration, bool userDefinedDuration,
    const std::map<std::string, std::string>& httpHeader,
    std::shared_ptr<MediaSourceLoaderCombinations> sourceLoader)
{
    if (userDefinedDuration) {
        userDefinedBufferDuration_ = userDefinedDuration;
        expectDuration_ = static_cast<uint64_t>(expectBufferDuration);
        expectDuration_ = std::min(expectDuration_, MAX_EXPECT_DURATION);
        totalBufferSize_ = expectDuration_ * CURRENT_BIT_RATE;
    } else {
        cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferHlsImpl>();
        cacheMediaBuffer_->Init(MAX_CACHE_BUFFER_SIZE, CHUNK_SIZE);
        isBuffering_ = true;
        bufferingTime_ = static_cast<size_t>(steadyClock_.ElapsedMilliseconds());
        totalBufferSize_ = MAX_CACHE_BUFFER_SIZE;
    }
    httpHeader_ = httpHeader;
    timeoutInterval_ = MAX_BUFFERING_TIME_OUT;
    if (sourceLoader != nullptr) {
        sourceLoader_ = sourceLoader;
        MEDIA_LOG_I("HLS app download.");
    }
    MEDIA_LOG_I("HLS setting buffer size: " PUBLIC_LOG_ZU " userDefinedDuration:" PUBLIC_LOG_D32,
        totalBufferSize_, userDefinedDuration);
}

HlsMediaDownloader::HlsMediaDownloader(std::string mimeType,
    const std::map<std::string, std::string>& httpHeader)
{
    mimeType_ = mimeType;
    cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferHlsImpl>();
    httpHeader_ = httpHeader;
    timeoutInterval_ = MAX_BUFFERING_TIME_OUT;
    cacheMediaBuffer_->Init(MAX_CACHE_BUFFER_SIZE, CHUNK_SIZE);
    totalBufferSize_ = MAX_CACHE_BUFFER_SIZE;
    memorySize_ = MAX_CACHE_BUFFER_SIZE;
    MEDIA_LOG_I("HLS setting buffer size: " PUBLIC_LOG_ZU, totalBufferSize_);
}

void HlsMediaDownloader::Init()
{
    if (sourceLoader_ != nullptr) {
        downloader_ = std::make_shared<Downloader>("hlsMedia", sourceLoader_);
        MEDIA_LOG_I("HLS app download.");
    } else {
        downloader_ = std::make_shared<Downloader>("hlsMedia");
    }
    downloader_->Init();
    playList_ = std::make_shared<BlockingQueue<PlayInfo>>("PlayList");
    auto weakDownloader = weak_from_this();
    dataSave_ =  [weakDownloader] (uint8_t*&& data, uint32_t&& len, bool&& notBlock) -> uint32_t {
        auto shareDownloader = weakDownloader.lock();
        FALSE_RETURN_V_MSG(shareDownloader != nullptr, 0u, "dataSave_, Hls Media Downloader already destructed.");
        return shareDownloader->SaveData(std::forward<decltype(data)>(data), std::forward<decltype(len)>(len),
            std::forward<decltype(notBlock)>(notBlock));
    };
    if (sourceLoader_ != nullptr) {
        playlistDownloader_ = std::make_shared<HlsPlayListDownloader>(httpHeader_, sourceLoader_);
    } else {
        playlistDownloader_ = std::make_shared<HlsPlayListDownloader>(httpHeader_, nullptr);
    }
    playlistDownloader_->Init();
    writeBitrateCaculator_ = std::make_shared<WriteBitrateCaculator>();
    waterLineAbove_ = PLAY_WATER_LINE;
    steadyClock_.Reset();
    loopInterruptClock_.Reset();
    aesKey_.rounds = 0;
    for (size_t i = 0; i < sizeof(aesKey_.rd_key) / sizeof(aesKey_.rd_key[0]); ++i) {
        aesKey_.rd_key[i] = 0;
    }
}

HlsMediaDownloader::~HlsMediaDownloader()
{
    MEDIA_LOG_I("0x%{public}06" PRIXPTR " ~HlsMediaDownloader dtor in", FAKE_POINTER(this));
    if (downloader_ != nullptr) {
        downloader_->Stop(false);
    }
    cacheMediaBuffer_ = nullptr;
    if (playlistDownloader_ != nullptr) {
        playlistDownloader_ = nullptr;
    }
    MEDIA_LOG_I("0x%{public}06" PRIXPTR " ~HlsMediaDownloader dtor out", FAKE_POINTER(this));
}

uint64_t SpliceOffset(uint32_t tsIndex, uint32_t offset32)
{
    uint64_t offset64 = 0;
    offset64 = tsIndex;
    offset64 = (offset64 << 32); // 32
    offset64 |= offset32;
    return offset64;
}

std::string HlsMediaDownloader::GetContentType()
{
    FALSE_RETURN_V(downloader_ != nullptr, "");
    MEDIA_LOG_I("In");
    return downloader_->GetContentType();
}

void HlsMediaDownloader::SetDownloaderRequestCb(
    StatusCallbackFunc& statusCallback, DownloadDoneCbFunc& downloadDoneCallback)
{
    auto weakDownloader = weak_from_this();
    statusCallback = [weakDownloader] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                                        std::shared_ptr<DownloadRequest>& request) {
        auto shareDownloader = weakDownloader.lock();
        FALSE_RETURN_MSG(shareDownloader != nullptr, "statusCallback, Hls Media Downloader already destructed.");
        shareDownloader->statusCallback_(status, shareDownloader->downloader_,
            std::forward<decltype(request)>(request));
    };
    downloadDoneCallback = [weakDownloader] (const std::string &url, const std::string& location) {
        auto shareDownloader = weakDownloader.lock();
        FALSE_RETURN_MSG(shareDownloader != nullptr, "downloadDoneCb, Hls Media Downloader already destructed.");
        shareDownloader->UpdateDownloadFinished(url, location);
    };
}

void HlsMediaDownloader::PutRequestIntoDownloader(const PlayInfo& playInfo)
{
    if (fragmentDownloadStart[playInfo.url_]) {
        writeTsIndex_ > 0 ? writeTsIndex_-- : 0;
        return;
    }
    StatusCallbackFunc realStatusCallback = nullptr;
    DownloadDoneCbFunc downloadDoneCallback = nullptr;
    SetDownloaderRequestCb(realStatusCallback, downloadDoneCallback);
    RequestInfo requestInfo;
    requestInfo.url = playInfo.rangeUrl_.empty() ? playInfo.url_ : playInfo.rangeUrl_;
    requestInfo.httpHeader = httpHeader_;
    bool isRequestWholeFile = playInfo.rangeUrl_.empty() ? true : playInfo.length_ <= 0;
    downloadRequest_ = std::make_shared<DownloadRequest>(playInfo.duration_, dataSave_,
                                                         realStatusCallback, requestInfo, isRequestWholeFile);
    fragmentDownloadStart[playInfo.url_] = true;
    int64_t startTimePos = playInfo.startTimePos_;
    curUrl_ = playInfo.rangeUrl_.empty() ? playInfo.url_ : playInfo.rangeUrl_;
    if (writeTsIndex_ == 0) {
        readOffset_ = SpliceOffset(writeTsIndex_, 0);
        MEDIA_LOG_I("HLS PutRequestIntoDownloader init readOffset." PUBLIC_LOG_U64, readOffset_);
        readTsIndex_ = writeTsIndex_;
        uint32_t readTsIndexTempValue = readTsIndex_.load();
        MEDIA_LOG_I("readTsIndex_, PutRequestIntoDownloader init readTsIndex_." PUBLIC_LOG_U32, readTsIndexTempValue);
    }
    writeOffset_ = SpliceOffset(writeTsIndex_, 0);
    MEDIA_LOG_I("HLS PutRequestIntoDwonloader update writeOffset_: " PUBLIC_LOG_U64 " writeTsIndex_: " PUBLIC_LOG_U32,
        writeOffset_, writeTsIndex_);
    {
        AutoLock lock(tsStorageInfoMutex_);
        if (tsStorageInfo_.find(writeTsIndex_) == tsStorageInfo_.end()) {
            tsStorageInfo_[writeTsIndex_] = std::make_pair(0, false);
        }
    }
    if (!playInfo.rangeUrl_.empty()) {
        tsStreamIdInfo_[writeTsIndex_] = playInfo.streamId_;
        if (!isRequestWholeFile) {
            downloadRequest_->SetRangePos(playInfo.offset_, playInfo.offset_ + playInfo.length_ - 1); // 1
        }
    }
    downloadRequest_->SetRequestProtocolType(RequestProtocolType::HLS);
    downloadRequest_->SetDownloadDoneCb(downloadDoneCallback);
    downloadRequest_->SetStartTimePos(startTimePos);
    downloader_->Download(downloadRequest_, -1); // -1
    downloader_->Start();
}

void HlsMediaDownloader::SaveHttpHeader(const std::map<std::string, std::string>& httpHeader)
{
    httpHeader_ = httpHeader;
}

bool HlsMediaDownloader::Open(const std::string& url, const std::map<std::string, std::string>& httpHeader)
{
    isDownloadFinish_ = false;
    SaveHttpHeader(httpHeader);
    writeBitrateCaculator_->StartClock();
    playlistDownloader_->SetPlayListCallback(std::weak_ptr<PlayListChangeCallback>(weak_from_this()));
    playlistDownloader_->SetMimeType(mimeType_);
    playlistDownloader_->Open(url, httpHeader);
    steadyClock_.Reset();
    openTime_ = steadyClock_.ElapsedMilliseconds();
    if (userDefinedBufferDuration_) {
        MEDIA_LOG_I("HLS User setting buffer duration playListDownloader_ opened.");
        totalBufferSize_ = expectDuration_ * CURRENT_BIT_RATE;
        if (totalBufferSize_ < MIN_BUFFER_SIZE) {
            MEDIA_LOG_I("HLS Failed setting buffer size: " PUBLIC_LOG_ZU ". already lower than the min buffer size: "
            PUBLIC_LOG_ZU ", setting buffer size: " PUBLIC_LOG_ZU ". ",
            totalBufferSize_, MIN_BUFFER_SIZE, MIN_BUFFER_SIZE);
            cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferHlsImpl>();
            cacheMediaBuffer_->Init(MIN_BUFFER_SIZE, CHUNK_SIZE);
            totalBufferSize_ = MIN_BUFFER_SIZE;
            memorySize_ = MIN_BUFFER_SIZE;
        } else if (totalBufferSize_ > MAX_CACHE_BUFFER_SIZE) {
            MEDIA_LOG_I("HLS Failed setting buffer size: " PUBLIC_LOG_ZU ". already exceed the max buffer size: "
            PUBLIC_LOG_U64 ", setting buffer size: " PUBLIC_LOG_U64 ". ",
            totalBufferSize_, MAX_CACHE_BUFFER_SIZE, MAX_CACHE_BUFFER_SIZE);
            cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferHlsImpl>();
            cacheMediaBuffer_->Init(MAX_CACHE_BUFFER_SIZE, CHUNK_SIZE);
            totalBufferSize_ = MAX_CACHE_BUFFER_SIZE;
            memorySize_ = MAX_CACHE_BUFFER_SIZE;
        } else {
            cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferHlsImpl>();
            cacheMediaBuffer_->Init(totalBufferSize_, CHUNK_SIZE);
            memorySize_ = totalBufferSize_;
            MEDIA_LOG_I("HLS Success setted buffer size: " PUBLIC_LOG_ZU ". ", totalBufferSize_);
        }
    }
    return true;
}

void HlsMediaDownloader::Close(bool isAsync)
{
    MEDIA_LOG_I("0x%{public}06" PRIXPTR " HLS Close enter", FAKE_POINTER(this));
    isInterrupt_ = true;
    if (playList_) {
        playList_->SetActive(false);
    }
    if (playlistDownloader_) {
        playlistDownloader_->Close();
    }
    if (downloader_) {
        downloader_->Stop(isAsync);
    }
    isStopped = true;
    if (!isDownloadFinish_) {
        MEDIA_LOG_D("HLS Download close, average download speed: " PUBLIC_LOG_D32 " bit/s", avgDownloadSpeed_);
        int64_t nowTime = steadyClock_.ElapsedMilliseconds();
        auto downloadTime = nowTime - startDownloadTime_;
        MEDIA_LOG_D("HLS Download close, Data usage: " PUBLIC_LOG_U64 " bits in " PUBLIC_LOG_D64 "ms",
            totalBits_, downloadTime);
    }
}

void HlsMediaDownloader::Pause()
{
    MEDIA_LOG_I("HLS Pause enter");
    playlistDownloader_->Pause();
}

void HlsMediaDownloader::Resume()
{
    MEDIA_LOG_I("HLS Resume enter");
    playlistDownloader_->Resume();
}

bool HlsMediaDownloader::CheckReadStatus()
{
    // eos:palylist is empty, request is finished, hls is vod, do not select bitrate
    FALSE_RETURN_V(playlistDownloader_ != nullptr, false);
    FALSE_RETURN_V(downloadRequest_ != nullptr, false);
    isEos_ = playList_->Empty() && (downloadRequest_ != nullptr) &&
                 downloadRequest_->IsEos() && playlistDownloader_ != nullptr &&
                 (playlistDownloader_->GetDuration() > 0) &&
                 playlistDownloader_->IsParseAndNotifyFinished();
    if (isEos_) {
        return true;
    }
    if (playlistDownloader_->GetDuration() > 0 && playlistDownloader_->IsParseAndNotifyFinished() &&
        static_cast<int64_t>(seekTime_) >= playlistDownloader_->GetDuration()) {
        MEDIA_LOG_I("HLS seek to tail.");
        return true;
    }
    return false;
}

bool HlsMediaDownloader::CheckBreakCondition()
{
    if (downloadErrorState_) {
        MEDIA_LOG_I("HLS downloadErrorState break");
        return true;
    }
    if (CheckReadStatus()) {
        MEDIA_LOG_I("HLS download complete break");
        return true;
    }
    return false;
}

bool HlsMediaDownloader::HandleBuffering()
{
    if (isBuffering_ && GetBufferingTimeOut() && callback_) {
        MEDIA_LOG_I("HTTP cachebuffer buffering time out.");
        callback_->OnEvent({PluginEventType::CLIENT_ERROR, {NetworkClientErrorCode::ERROR_TIME_OUT}, "buffering"});
        isTimeoutErrorNotified_.store(true);
        isBuffering_ = false;
        return false;
    }
    if (IsNeedBufferForPlaying() || !isBuffering_.load()) {
        return false;
    }
    if (isFirstFrameArrived_) {
        UpdateWaterLineAbove();
        UpdateCachedPercent(BufferingInfoType::BUFFERING_PERCENT);
    }
    {
        AutoLock lk(bufferingEndMutex_);
        if (!canWrite_) {
            MEDIA_LOG_I("HLS canWrite false");
        }
        {
            AutoLock lock(tsStorageInfoMutex_);
            if (tsStorageInfo_.find(readTsIndex_) != tsStorageInfo_.end() &&
                tsStorageInfo_[readTsIndex_].second &&
                !backPlayList_.empty() && readTsIndex_ >= backPlayList_.size() - 1) {
                MEDIA_LOG_I("HLS readTS download complete.");
                isBuffering_ = false;
            }
        }
        HandleWaterLine();
    }
    if (!isBuffering_ && !isFirstFrameArrived_) {
        bufferingTime_ = 0;
    }
    if (!isBuffering_ && isFirstFrameArrived_ && callback_ != nullptr) {
        MEDIA_LOG_I("HLS CacheData onEvent BUFFERING_END, waterLineAbove: " PUBLIC_LOG_ZU " readOffset: "
        PUBLIC_LOG_U64 " writeOffset: " PUBLIC_LOG_U64 " writeTsIndex: " PUBLIC_LOG_U32 " bufferSize: "
        PUBLIC_LOG_U64, waterLineAbove_, readOffset_, writeOffset_, writeTsIndex_, GetCrossTsBuffersize());
        UpdateCachedPercent(BufferingInfoType::BUFFERING_END);
        callback_->OnEvent({PluginEventType::BUFFERING_END, {BufferingInfoType::BUFFERING_END}, "end"});
        bufferingTime_ = 0;
    }
    return isBuffering_.load();
}

void HlsMediaDownloader::HandleWaterLine()
{
    size_t currentWaterLine = waterLineAbove_;
    size_t currentOffset = readOffset_;
    {
        AutoLock lock(initCacheMutex_);
        if (initCacheSize_.load() != -1) {
            currentWaterLine = static_cast<size_t>(initCacheSize_.load());
            currentOffset = static_cast<size_t>(expectOffset_.load());
            MEDIA_LOG_I("currentOffset:" PUBLIC_LOG_ZU, currentOffset);
        }
        if (GetCrossTsBuffersize() >= currentWaterLine || CheckBreakCondition() ||
            (tsStorageInfo_.find(readTsIndex_ + 1) != tsStorageInfo_.end() &&
                tsStorageInfo_[readTsIndex_ + 1].second)) {
            MEDIA_LOG_I("HLS CheckBreakCondition true, waterLineAbove: " PUBLIC_LOG_ZU " bufferSize: " PUBLIC_LOG_U64,
                waterLineAbove_, GetCrossTsBuffersize());
            if (initCacheSize_.load() != -1) {
                initCacheSize_.store(-1);
                expectOffset_.store(-1);
                callback_->OnEvent({PluginEventType::INITIAL_BUFFER_SUCCESS,
                                    {BufferingInfoType::BUFFERING_END}, "end"});
            }
            isBuffering_ = false;
        }
    }
    if (!isBuffering_) {
        MEDIA_LOG_I("HandleBuffering bufferingEndCond NotifyAll.");
        bufferingEndCond_.NotifyAll();
    }
}

bool HlsMediaDownloader::HandleCache()
{
    size_t waterLine = 0;
    if (isFirstFrameArrived_) {
        waterLine = wantedReadLength_ > 0 ?
            std::max(PLAY_WATER_LINE, static_cast<size_t>(wantedReadLength_)) : 0;
    } else {
        waterLine = wantedReadLength_;
        waterLineAbove_ = waterLine;
    }
    bool isAboveLine = GetCrossTsBuffersize() >= waterLine;
    bool isNextTsReady = true;
    if (!backPlayList_.empty() && readTsIndex_ >= backPlayList_.size() - 1) {
        isNextTsReady = tsStorageInfo_[readTsIndex_].second;
    } else {
        isNextTsReady = tsStorageInfo_[readTsIndex_ + 1].second;
    }
    if (isBuffering_ || callback_ == nullptr || !canWrite_ || isAboveLine || isNextTsReady) {
        return false;
    }
    isBuffering_ = true;
    bufferingTime_ = static_cast<size_t>(steadyClock_.ElapsedMilliseconds());
    if (isFirstFrameArrived_) {
        callback_->OnEvent({PluginEventType::BUFFERING_START, {BufferingInfoType::BUFFERING_START}, "start"});
        UpdateCachedPercent(BufferingInfoType::BUFFERING_START);
        MEDIA_LOG_I("HLS CacheData onEvent BUFFERING_START, waterLineAbove: " PUBLIC_LOG_ZU " readOffset: "
            PUBLIC_LOG_U64 " writeOffset: " PUBLIC_LOG_U64 " writeTsIndex: " PUBLIC_LOG_U32 " bufferSize: "
            PUBLIC_LOG_U64, waterLineAbove_, readOffset_, writeOffset_, writeTsIndex_, GetCrossTsBuffersize());
    }
    return true;
}

void HlsMediaDownloader::HandleFfmpegReadback(uint64_t ffmpegOffset)
{
    if (notNeedReadBack_.load()) {
        ffmpegOffset_ = ffmpegOffset;
        return;
    }
    if (curStreamId_ > 0 && isNeedResetOffset_.load()) {
        ffmpegOffset_ = ffmpegOffset;
        return;
    }
    if (ffmpegOffset_ <= ffmpegOffset) {
        return;
    }
    MEDIA_LOG_D("HLS Read back, ffmpegOffset: " PUBLIC_LOG_U64 " ffmpegOffset: " PUBLIC_LOG_U64,
        ffmpegOffset_, ffmpegOffset);
    uint64_t readBack = ffmpegOffset_ - ffmpegOffset;
    uint64_t curTsHaveRead = readOffset_ > SpliceOffset(readTsIndex_, 0) ?
        readOffset_ - SpliceOffset(readTsIndex_, 0) : 0;
    AutoLock lock(tsStorageInfoMutex_);
    if (curTsHaveRead >= readBack) {
        readOffset_ -= readBack;
        MEDIA_LOG_D("HLS Read back, current ts, update readOffset: " PUBLIC_LOG_U64, readOffset_);
    } else {
        if (readTsIndex_ == 0) {
            readOffset_ = 0; // Cross ts readback, but this is the first ts, so reset readOffset.
            MEDIA_LOG_W("HLS Read back, this is the first ts: " PUBLIC_LOG_U64, readOffset_);
            return;
        }
        if (tsStorageInfo_.find(readTsIndex_ - 1) == tsStorageInfo_.end()) {
            readOffset_ = readOffset_ > curTsHaveRead ? readOffset_ - curTsHaveRead : 0;
            MEDIA_LOG_W("HLS Read back, last ts is not ready, update readOffset to readTsIndex head: "
                PUBLIC_LOG_U64, readOffset_);
            return;
        }
        readTsIndex_--;
        uint64_t lastTsReadBack = readBack - curTsHaveRead;
        uint32_t readTsIndexTempValue = readTsIndex_.load();
        uint32_t offset32 = 0;
        if (tsStorageInfo_.find(readTsIndex_) != tsStorageInfo_.end()) {
            offset32 = tsStorageInfo_[readTsIndex_].first >= lastTsReadBack ?
                tsStorageInfo_[readTsIndex_].first - lastTsReadBack : 0;
            readOffset_ = SpliceOffset(readTsIndex_, offset32);
            MEDIA_LOG_I("HLS Read back, last ts, update readTsIndex: " PUBLIC_LOG_U32
                " update readOffset: " PUBLIC_LOG_U64, readTsIndexTempValue, readOffset_);
        }
    }
}

bool HlsMediaDownloader::CheckDataIntegrity()
{
    AutoLock lock(tsStorageInfoMutex_);
    if (tsStorageInfo_.find(readTsIndex_) != tsStorageInfo_.end() &&
        !tsStorageInfo_[readTsIndex_].second) {
        return readTsIndex_ == writeTsIndex_;
    } else {
        uint64_t head = SpliceOffset(readTsIndex_, 0);
        uint64_t hasRead = readOffset_ >= head ? readOffset_ - head : 0;
        uint64_t bufferSize = tsStorageInfo_[readTsIndex_].first >= hasRead ?
            tsStorageInfo_[readTsIndex_].first - hasRead : 0;
        return bufferSize == GetBufferSize();
    }
}

Status HlsMediaDownloader::CheckPlaylist(unsigned char* buff, ReadDataInfo& readDataInfo)
{
    bool isFinishedPlay = CheckReadStatus() || isStopped;
    if (downloadRequest_ != nullptr) {
        readDataInfo.isEos_ = downloadRequest_->IsEos();
    }
    if (isFinishedPlay && readTsIndex_ + 1 < backPlayList_.size()) {
        return Status::ERROR_UNKNOWN;
    }
    if (isFinishedPlay && GetBufferSize() > 0) {
        if (ReadHeaderData(buff, readDataInfo)) {
            return Status::OK;
        }
        size_t readLen = std::min(GetBufferSize(), static_cast<uint64_t>(readDataInfo.wantReadLength_));
        readDataInfo.realReadLength_ = cacheMediaBuffer_->Read(buff, readOffset_, readLen);
        readOffset_ += readDataInfo.realReadLength_;
        ffmpegOffset_ = readDataInfo.ffmpegOffset + readDataInfo.realReadLength_;
        canWrite_ = true;
        OnReadBuffer(readDataInfo.realReadLength_);
        uint32_t readTsIndexTempValue = readTsIndex_.load();
        timeoutInterval_ = MAX_BUFFERING_TIME_OUT;
        MEDIA_LOG_D("HLS Read Success: wantReadLength " PUBLIC_LOG_D32 ", realReadLength " PUBLIC_LOG_D32 ", isEos "
            PUBLIC_LOG_D32 " readOffset_ " PUBLIC_LOG_U64 " readTsIndex_ " PUBLIC_LOG_U32, readDataInfo.wantReadLength_,
            readDataInfo.realReadLength_, readDataInfo.isEos_, readOffset_, readTsIndexTempValue);
        return Status::OK;
    }
    if (isFinishedPlay && GetBufferSize() == 0 && GetSeekable() == Seekable::SEEKABLE &&
        tsStorageInfo_.find(writeTsIndex_) != tsStorageInfo_.end() &&
        tsStorageInfo_[writeTsIndex_].second) {
        readDataInfo.realReadLength_ = 0;
        MEDIA_LOG_I("HLS CheckPlaylist, eos.");
        return Status::END_OF_STREAM;
    }
    return Status::ERROR_UNKNOWN;
}

Status HlsMediaDownloader::ReadDelegate(unsigned char* buff, ReadDataInfo& readDataInfo)
{
    FALSE_RETURN_V_MSG(cacheMediaBuffer_ != nullptr, Status::END_OF_STREAM, "eos, cacheMediaBuffer_ is nullptr");
    FALSE_RETURN_V_MSG(!isInterruptNeeded_.load(), Status::END_OF_STREAM, "eos, isInterruptNeeded");
    MediaAVCodec::AVCodecTrace trace("HLS ReadDelegate, expectedLen: " +
        std::to_string(readDataInfo.wantReadLength_) + ", bufferSize: " + std::to_string(GetBufferSize()));
    MEDIA_LOG_D("HLS Read in: wantReadLength " PUBLIC_LOG_D32 ", readOffset: " PUBLIC_LOG_U64 " readTsIndex: "
        PUBLIC_LOG_U32 " bufferSize: " PUBLIC_LOG_U64, readDataInfo.wantReadLength_, readOffset_,
        readTsIndex_.load(), GetCrossTsBuffersize());
    if (isTsEnd_.load()) {
        MEDIA_LOG_I("HLS READ TS END");
        isTsEnd_.store(false);
        return Status::END_OF_STREAM;
    }
    readDataInfo.isEos_ = CheckReadStatus();
    if (readDataInfo.isEos_ && GetBufferSize() == 0 && readTsIndex_ + 1 == backPlayList_.size() &&
        tsStorageInfo_.find(readTsIndex_) != tsStorageInfo_.end() &&
        tsStorageInfo_[readTsIndex_].second) {
        readDataInfo.realReadLength_ = 0;
        MEDIA_LOG_I("HLS buffer is empty, eos.");
        return Status::END_OF_STREAM;
    }
    if (isBuffering_ && GetBufferingTimeOut() && callback_ && !isReportedErrorCode_) {
        callback_->OnEvent({PluginEventType::CLIENT_ERROR, {NetworkClientErrorCode::ERROR_TIME_OUT}, "read"});
        isTimeoutErrorNotified_.store(true);
        MEDIA_LOG_D("HLS return END_OF_STREAM.");
        return Status::END_OF_STREAM;
    }
    if (isBuffering_ && CheckBufferingOneSeconds()) {
        MEDIA_LOG_I("HLS read return error again.");
        return Status::ERROR_AGAIN;
    }
    wantedReadLength_ = static_cast<size_t>(readDataInfo.wantReadLength_);
    if (!CheckBreakCondition() && HandleCache()) {
        MEDIA_LOG_D("HLS read return error again.");
        return Status::ERROR_AGAIN;
    }
    Status tmpRes = CheckPlaylist(buff, readDataInfo);
    if (tmpRes != Status::ERROR_UNKNOWN) {
        MEDIA_LOG_D("HLS read return tmpRes.");
        return tmpRes;
    }

    FALSE_RETURN_V_MSG(readDataInfo.wantReadLength_ > 0, Status::END_OF_STREAM, "eos, wantReadLength_ <= 0");
    ReadCacheBuffer(buff, readDataInfo);
    MEDIA_LOG_D("HLS Read success: wantReadLength " PUBLIC_LOG_D32 " realReadLen: " PUBLIC_LOG_D32 " readOffset: "
        PUBLIC_LOG_U64 " readTsIndex: " PUBLIC_LOG_U32 " bufferSize: " PUBLIC_LOG_U64, readDataInfo.wantReadLength_,
        readDataInfo.realReadLength_, readOffset_, readTsIndex_.load(), GetCrossTsBuffersize());
    return Status::OK;
}

bool HlsMediaDownloader::ReadHeaderData(unsigned char* buff, ReadDataInfo& readDataInfo)
{
    if (playlistDownloader_ == nullptr || (playlistDownloader_ && !playlistDownloader_->IsHlsFmp4())) {
        return false;
    }
    if (curStreamId_ <= 0 && readDataInfo.streamId_ > 0) {
        curStreamId_ = static_cast<uint32_t>(readDataInfo.streamId_);
        isNeedReadHeader_.store(true);
        MEDIA_LOG_D("HLS read curStreamId_ " PUBLIC_LOG_U32, curStreamId_);
    } else if (readDataInfo.streamId_ > 0 && readDataInfo.streamId_ != static_cast<int32_t>(curStreamId_)) {
        readDataInfo.nextStreamId_ = static_cast<int32_t>(curStreamId_);
        isNeedReadHeader_.store(true);
        MEDIA_LOG_I("HLS read curStreamId_ " PUBLIC_LOG_U32 " curStreamId_ " PUBLIC_LOG_U32,
                    curStreamId_, readDataInfo.streamId_);
        return true;
    }
    if (readDataInfo.streamId_ > 0 && curStreamId_ == static_cast<uint32_t>(readDataInfo.streamId_) &&
        isNeedReadHeader_.load()) {
        playlistDownloader_->ReadFmp4Header(buff, readDataInfo.realReadLength_, readDataInfo.streamId_);
        isNeedReadHeader_.store(false);
        MEDIA_LOG_I("HLS read fmp4 header.");
        return true;
    }
    return false;
}

void HlsMediaDownloader::ReadCacheBuffer(unsigned char* buff, ReadDataInfo& readDataInfo)
{
    if (ReadHeaderData(buff, readDataInfo)) {
        return;
    }
    readDataInfo.realReadLength_ = cacheMediaBuffer_->Read(buff, readOffset_, readDataInfo.wantReadLength_);
    readOffset_ += readDataInfo.realReadLength_;
    ffmpegOffset_ = readDataInfo.ffmpegOffset + readDataInfo.realReadLength_;
    if ((IsHlsFmp4() && readDataInfo.streamId_ > 0) || IsPureByteRange()) {
        uint64_t remain = cacheMediaBuffer_->GetBufferSize(readOffset_);
        if (remain > 0 && remain < DECRYPT_UNIT_LEN && keyLen_ > 0) {
            size_t readRemain = cacheMediaBuffer_->Read(buff, readOffset_, readDataInfo.wantReadLength_);
            readOffset_ += readRemain;
            ffmpegOffset_ += readRemain;
            readDataInfo.realReadLength_ += readRemain;
        }
    }
    if (tsStorageInfo_.find(readTsIndex_) != tsStorageInfo_.end() &&
        tsStorageInfo_[readTsIndex_].second == true) {
        uint64_t tsEndOffset = SpliceOffset(readTsIndex_, tsStorageInfo_[readTsIndex_].first);
        if (readOffset_ >= tsEndOffset) {
            isTsEnd_.store(true);
            notNeedReadBack_.store(true);
            if (playlistDownloader_->IsHlsFmp4()) {
                isNeedReadHeader_.store(true);
            }
            RemoveFmp4PaddingData(buff, readDataInfo);
            readTsIndex_++;
            cacheMediaBuffer_->ClearFragmentBeforeOffset(SpliceOffset(readTsIndex_, 0));
            readOffset_ = SpliceOffset(readTsIndex_, 0);
            if (playlistDownloader_->IsHlsFmp4() && tsStreamIdInfo_.find(readTsIndex_) != tsStreamIdInfo_.end() &&
                readDataInfo.streamId_ > 0 &&
                readDataInfo.streamId_ != static_cast<int32_t>(tsStreamIdInfo_[readTsIndex_])) {
                curStreamId_ = tsStreamIdInfo_[readTsIndex_];
                isNeedResetOffset_.store(true);
                isTsEnd_.store(false);
                MEDIA_LOG_D("HLS read readTsIndex_ " PUBLIC_LOG_U32, readTsIndex_.load());
                return;
            }
        }
    }
    canWrite_ = true;
}

void HlsMediaDownloader::RemoveFmp4PaddingData(unsigned char* buff, ReadDataInfo& readDataInfo)
{
    if (keyLen_ <= 0 || readDataInfo.realReadLength_ < 1) {
        return;
    }
    if ((IsHlsFmp4() && readDataInfo.streamId_ > 0) || IsPureByteRange()) {
        size_t endValue = buff[readDataInfo.realReadLength_ - 1];
        size_t paddingStart = readDataInfo.realReadLength_ > endValue ?
                              readDataInfo.realReadLength_ - endValue : 0;
        if (buff[paddingStart] == endValue) {
            readOffset_ -= endValue;
            ffmpegOffset_ -= endValue;
            readDataInfo.realReadLength_ -= endValue;
        }
    }
}

Status HlsMediaDownloader::Read(unsigned char* buff, ReadDataInfo& readDataInfo)
{
    FALSE_RETURN_V(cacheMediaBuffer_ != nullptr, Status::ERROR_AGAIN);
    uint64_t now = static_cast<uint64_t>(steadyClock_.ElapsedMilliseconds());
    readTime_ = now;
    if (isBuffering_ && !canWrite_) {
        MEDIA_LOG_I("HLS can not write and buffering.");
        SeekToTsForRead(readTsIndex_);
        return Status::ERROR_AGAIN;
    }
    if (!CheckDataIntegrity()) {
        MEDIA_LOG_W("HLS Read in, fix download.");
        SeekToTsForRead(readTsIndex_);
        return Status::ERROR_AGAIN;
    }

    HandleFfmpegReadback(readDataInfo.ffmpegOffset);

    auto ret = ReadDelegate(buff, readDataInfo);

    uint64_t freeSize = cacheMediaBuffer_->GetFreeSize();
    if (freeSize > RESUME_FREE_SIZE_THRESHOLD && isNeedResume_.load()) {
        downloader_->Resume();
        isNeedResume_.store(false);
        MEDIA_LOG_D("HLS downloader resume.");
    }

    readTotalBytes_ += readDataInfo.realReadLength_;
    if (now > lastReadCheckTime_ && now - lastReadCheckTime_ > SAMPLE_INTERVAL) {
        readRecordDuringTime_ = now - lastReadCheckTime_;
        double readDuration = static_cast<double>(readRecordDuringTime_) / SECOND_TO_MILLISECONDS;
        if (readDuration > ZERO_THRESHOLD) {
            double readSpeed = readTotalBytes_ * BYTES_TO_BIT / readDuration;    // bps
            int32_t temp = static_cast<int32_t>(readSpeed);
            readBitrate_ = temp > 0 ? static_cast<uint64_t>(temp) : readBitrate_;
            MEDIA_LOG_D("Current read speed: " PUBLIC_LOG_D32 " Kbit/s,Current buffer size: " PUBLIC_LOG_U64
            " KByte", static_cast<int32_t>(readSpeed / KILO), static_cast<uint64_t>(GetBufferSize() / KILO));
            MediaAVCodec::AVCodecTrace trace("HlsMediaDownloader::Read, read speed: " +
                std::to_string(readSpeed) + " bit/s, bufferSize: " + std::to_string(GetBufferSize()) + " Byte");
            readTotalBytes_ = 0;
        }
        lastReadCheckTime_ = now;
        readRecordDuringTime_ = 0;
    }
    return ret;
}

void HlsMediaDownloader::PrepareToSeek()
{
    isTsEnd_.store(false);
    int32_t retry {0};
    int64_t loopStartTime = loopInterruptClock_.ElapsedSeconds();
    do {
        if (CheckLoopTimeout(loopStartTime)) {
            break;
        }
        retry++;
        FALSE_RETURN_MSG(!isInterruptNeeded_, "HLS Seek return, isInterruptNeeded_.");
        if (retry >= SEEK_STATUS_RETRY_TIMES) { // 100 means retry times
            MEDIA_LOG_I("HLS Seek may be failed");
            break;
        }
        OSAL::SleepFor(SEEK_STATUS_SLEEP_TIME); // 50 means sleep time pre retry
    } while (!playlistDownloader_->IsParseAndNotifyFinished());

    playList_->Clear();
    downloader_->Cancel();

    if (cacheMediaBuffer_ == nullptr) {
        cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferHlsImpl>();
    } else {
        cacheMediaBuffer_->Reset();
    }
    cacheMediaBuffer_->Init(totalBufferSize_, CHUNK_SIZE);
    memorySize_ = totalBufferSize_;
    canWrite_ = true;

    AutoLock lock(tsStorageInfoMutex_);
    tsStorageInfo_.clear();
    if (playlistDownloader_->IsHlsFmp4()) {
        tsStreamIdInfo_.clear();
    }
    
    memset_s(afterAlignRemainedBuffer_, DECRYPT_UNIT_LEN, 0x00, DECRYPT_UNIT_LEN);
    memset_s(decryptCache_, MIN_BUFFER_SIZE, 0x00, MIN_BUFFER_SIZE);
    memset_s(decryptBuffer_, MIN_BUFFER_SIZE, 0x00, MIN_BUFFER_SIZE);
    auto ret = memcpy_s(iv_, DECRYPT_UNIT_LEN, initIv_, DECRYPT_UNIT_LEN);
    if (ret != 0) {
        MEDIA_LOG_E("iv copy error.");
    }
    afterAlignRemainedLength_ = 0;
    isLastDecryptWriteError_ = false;
}

bool HlsMediaDownloader::SeekToTime(int64_t seekTime, SeekMode mode)
{
    MEDIA_LOG_I("HLS Seek: buffer size " PUBLIC_LOG_U64 ", seekTime " PUBLIC_LOG_D64 " mode: " PUBLIC_LOG_D32,
        GetBufferSize(), seekTime, mode);
    AutoLock lock(switchMutex_);
    FALSE_RETURN_V(playlistDownloader_ != nullptr, false);
    FALSE_RETURN_V(cacheMediaBuffer_ != nullptr, false);
    if (GetBufferSize() == 0 && seekTime == 0) {
        NotifyInitSuccess();
    }
    isSeekingFlag = true;
    seekTime_ = static_cast<uint64_t>(seekTime);
    bufferingTime_ = 0;
    PrepareToSeek();
    FALSE_RETURN_V_MSG(!isInterruptNeeded_, true, "HLS Seek return, isInterruptNeeded_.");
    if (seekTime_ < static_cast<uint64_t>(playlistDownloader_->GetDuration())) {
        if (playlistDownloader_->IsHlsFmp4()) {
            isNeedReadHeader_.store(true);
        }
        SeekToTs(seekTime, mode);
    } else {
        readTsIndex_ = !backPlayList_.empty() ? backPlayList_.size() - 1 : 0; // 0
        tsStorageInfo_[readTsIndex_].second = true;
    }
    HandleSeekReady(MediaAVCodec::MediaType::MEDIA_TYPE_VID, curStreamId_, CheckBreakCondition());
    MEDIA_LOG_I("HLS SeekToTime end\n");
    isSeekingFlag = false;
    notNeedReadBack_.store(false);
    return true;
}

size_t HlsMediaDownloader::GetContentLength() const
{
    return 0;
}

int64_t HlsMediaDownloader::GetDuration() const
{
    FALSE_RETURN_V(playlistDownloader_ != nullptr, 0);
    MEDIA_LOG_I("HLS GetDuration " PUBLIC_LOG_D64, playlistDownloader_->GetDuration());
    return playlistDownloader_->GetDuration();
}

Seekable HlsMediaDownloader::GetSeekable() const
{
    FALSE_RETURN_V(playlistDownloader_ != nullptr, Seekable::INVALID);
    return playlistDownloader_->GetSeekable();
}

void HlsMediaDownloader::SetCallback(Callback* cb)
{
    FALSE_RETURN(playlistDownloader_ != nullptr);
    callback_ = cb;
    playlistDownloader_->SetCallback(cb);
}

void HlsMediaDownloader::ResetPlaylistCapacity(size_t size)
{
    size_t remainCapacity = playList_->Capacity() - playList_->Size();
    if (remainCapacity >= size) {
        return;
    }
    size_t newCapacity = playList_->Size() + size;
    playList_->ResetCapacity(newCapacity);
}

void HlsMediaDownloader::PlaylistBackup(const PlayInfo& fragment)
{
    FALSE_RETURN(playlistDownloader_ != nullptr);
    if (playlistDownloader_->IsParseFinished() && (GetSeekable() == Seekable::UNSEEKABLE)) {
        if (backPlayList_.size() > 0) {
            backPlayList_.clear();
        }
        return;
    }
    if (playlistDownloader_->IsParseFinished()) {
        backPlayList_.push_back(fragment);
    }
}

void HlsMediaDownloader::OnPlayListChanged(const std::vector<PlayInfo>& playList)
{
    ResetPlaylistCapacity(static_cast<size_t>(playList.size()));
    int64_t loopStartTime = loopInterruptClock_.ElapsedSeconds();
    for (uint32_t i = 0; i < static_cast<uint32_t>(playList.size()); i++) {
        if (CheckLoopTimeout(loopStartTime)) {
            break;
        }
        if (isInterruptNeeded_.load()) {
            MEDIA_LOG_I("HLS OnPlayListChanged isInterruptNeeded.");
            break;
        }
        auto fragment = playList[i];
        PlaylistBackup(fragment);
        if (isSelectingBitrate_ && (GetSeekable() == Seekable::SEEKABLE)) {
            if (writeTsIndex_ == i) {
                MEDIA_LOG_I("HLS Switch bitrate");
                isSelectingBitrate_ = false;
                fragmentDownloadStart[fragment.url_] = true;
            } else {
                fragmentDownloadStart[fragment.url_] = true;
                continue;
            }
        }
        if (!fragmentDownloadStart[fragment.url_] && !fragmentPushed[fragment.url_]) {
            playList_->Push(fragment);
            fragmentPushed[fragment.url_] = true;
        }
    }
    if (!isDownloadStarted_ && !playList_->Empty() && !isInterruptNeeded_.load()) {
        auto playInfo = playList_->Pop();
        std::string url = playInfo.url_;
        isDownloadStarted_ = true;
        if (tsStorageInfo_.find(0) != tsStorageInfo_.end() && tsStorageInfo_[0].second) {
            writeTsIndex_++;
        } else {
            writeTsIndex_ > 0 ? writeTsIndex_++ : 0;
        }
        PutRequestIntoDownloader(playInfo);
    }
}

bool HlsMediaDownloader::GetStartedStatus()
{
    FALSE_RETURN_V(playlistDownloader_ != nullptr, false);
    return playlistDownloader_->GetPlayListDownloadStatus() && startedPlayStatus_;
}

bool HlsMediaDownloader::CacheBufferFullLoop()
{
    {
        AutoLock lock(initCacheMutex_);
        if (initCacheSize_.load() != -1) {
            if (IsCachedInitSizeReady(initCacheSize_.load())) {
                callback_->OnEvent({PluginEventType::INITIAL_BUFFER_SUCCESS,
                                        {BufferingInfoType::BUFFERING_END}, "end"});
                initCacheSize_.store(-1);
                expectOffset_.store(-1);
            }
        }
    }

    MEDIA_LOGI_LIMIT(SAVE_DATA_LOG_FREQUENCY, "HLS CacheMediaBuffer full, waiting seek or read");
    if (isSeekingFlag.load()) {
        MEDIA_LOG_I("HLS CacheMediaBuffer full, isSeeking, return true.");
        return true;
    }
    OSAL::SleepFor(SLEEP_TIME_100);
    return false;
}

uint32_t HlsMediaDownloader::SaveCacheBufferDataNotblock(uint8_t* data, uint32_t len)
{
    uint64_t freeSize = cacheMediaBuffer_->GetFreeSize();
    if (freeSize <= (len + STORP_WRITE_BUFFER_REDUNDANCY) && !isNeedResume_.load()) {
        isNeedResume_.store(true);
        MEDIA_LOG_I("HLS stop write, freeSize: " PUBLIC_LOG_U64 " len: " PUBLIC_LOG_U32, freeSize, len);
        return 0;
    }

    size_t res = cacheMediaBuffer_->Write(data, writeOffset_, len);
    writeOffset_ += res;
    MEDIA_LOGI_LIMIT(SAVE_DATA_LOG_FREQUENCY, "SaveCacheBufferDataNotblock writeOffset " PUBLIC_LOG_U64 " res "
        PUBLIC_LOG_ZU " len: " PUBLIC_LOG_U32, writeOffset_, res, len);
    {
        AutoLock lock(tsStorageInfoMutex_);
        if (tsStorageInfo_.find(writeTsIndex_) != tsStorageInfo_.end()) {
            tsStorageInfo_[writeTsIndex_].first += res;
        }
    }

    writeBitrateCaculator_->UpdateWriteBytes(res);
    ClearChunksOfFragment();
    HandleCachedDuration();
    writeBitrateCaculator_->StartClock();
    uint64_t writeTime  = writeBitrateCaculator_->GetWriteTime() / SECOND_TO_MILLISECONDS;
    if (writeTime > ONE_SECONDS) {
        writeBitrateCaculator_->ResetClock();
    }
    return res;
}

void HlsMediaDownloader::HandleSaveDataLoopContinue()
{
    HandleCachedDuration();
    writeBitrateCaculator_->StartClock();
    uint64_t writeTime = writeBitrateCaculator_->GetWriteTime() / SECOND_TO_MILLISECONDS;
    if (writeTime > ONE_SECONDS) {
        writeBitrateCaculator_->ResetClock();
    }
}

uint32_t HlsMediaDownloader::SaveCacheBufferData(uint8_t* data, uint32_t len, bool notBlock)
{
    if (notBlock) {
        return SaveCacheBufferDataNotblock(data, len);
    }
    size_t hasWriteSize = 0;
    while (hasWriteSize < len && !isInterruptNeeded_.load() && !isInterrupt_) {
        size_t res = cacheMediaBuffer_->Write(data + hasWriteSize, writeOffset_, len - hasWriteSize);
        writeOffset_ += res;
        hasWriteSize += res;
        {
            AutoLock lock(tsStorageInfoMutex_);
            if (tsStorageInfo_.find(writeTsIndex_) != tsStorageInfo_.end()) {
                tsStorageInfo_[writeTsIndex_].first += res;
            }
        }
        writeBitrateCaculator_->UpdateWriteBytes(res);
        MEDIA_LOGI_LIMIT(SAVE_DATA_LOG_FREQUENCY, "writeOffset " PUBLIC_LOG_U64 " res "
            PUBLIC_LOG_ZU, writeOffset_, res);
        ClearChunksOfFragment();
        if (res > 0 || hasWriteSize == len) {
            HandleSaveDataLoopContinue();
            continue;
        }
        writeBitrateCaculator_->StopClock();
        MEDIA_LOG_W("HLS CacheMediaBuffer full.");
        canWrite_ = false;
        HandleBuffering();
        while (!isInterrupt_ && !canWrite_.load() && !isInterruptNeeded_.load()) {
            if (CacheBufferFullLoop()) {
                return len;
            }
        }
        canWrite_ = true;
    }
    if (isInterruptNeeded_.load() || isInterrupt_) {
        MEDIA_LOG_I("HLS isInterruptNeeded true, return false.");
        return 0;
    }
    return hasWriteSize;
}

uint32_t HlsMediaDownloader::SaveData(uint8_t* data, uint32_t len, bool notBlock)
{
    if (cacheMediaBuffer_ == nullptr) {
        return 0;
    }
    OnWriteCacheBuffer(len);
    if (isSeekingFlag.load()) {
        return len;
    }
    startedPlayStatus_ = true;
    uint32_t res = 0;
    if (keyLen_ == 0) {
        res = SaveCacheBufferData(data, len, notBlock);
    } else {
        res = SaveEncryptData(data, len, notBlock);
    }
    HandleBuffering();

    uint64_t freeSize = cacheMediaBuffer_->GetFreeSize();
    MEDIA_LOG_D("HLS free size: " PUBLIC_LOG_U64, freeSize);
    return res;
}

uint32_t HlsMediaDownloader::GetDecrptyRealLen(uint8_t* writeDataPoint, uint32_t waitLen, uint32_t writeLen)
{
    uint32_t realLen;
    errno_t err {0};
    if (afterAlignRemainedLength_ > 0) {
        err = memcpy_s(decryptBuffer_, afterAlignRemainedLength_,
                       afterAlignRemainedBuffer_, afterAlignRemainedLength_);
        if (err != 0) {
            MEDIA_LOG_D("afterAlignRemainedLength_: " PUBLIC_LOG_D64, afterAlignRemainedLength_);
        }
    }
    uint32_t minWriteLen = (MIN_BUFFER_SIZE - afterAlignRemainedLength_) > writeLen ?
                            writeLen : MIN_BUFFER_SIZE - afterAlignRemainedLength_;
    if (minWriteLen > MIN_BUFFER_SIZE) {
        return 0;
    }
    err = memcpy_s(decryptBuffer_ + afterAlignRemainedLength_,
                   minWriteLen, writeDataPoint, minWriteLen);
    if (err != 0) {
        MEDIA_LOG_D("minWriteLen: " PUBLIC_LOG_D32, minWriteLen);
    }
    realLen = minWriteLen + afterAlignRemainedLength_;
    AES_cbc_encrypt(decryptBuffer_, decryptCache_, realLen, &aesKey_, iv_, AES_DECRYPT);
    return realLen;
}

uint32_t HlsMediaDownloader::SaveEncryptData(uint8_t* data, uint32_t len, bool notBlock)
{
    uint32_t writeLen = 0;
    uint8_t *writeDataPoint = data;
    uint32_t waitLen = len;
    errno_t err {0};
    uint32_t realLen;
    if ((waitLen + afterAlignRemainedLength_) < DECRYPT_UNIT_LEN && waitLen <= len) {
        err = memcpy_s(afterAlignRemainedBuffer_ + afterAlignRemainedLength_,
                       DECRYPT_UNIT_LEN - afterAlignRemainedLength_,
                       writeDataPoint, waitLen);
        if (err != 0) {
            MEDIA_LOG_E("afterAlignRemainedLength_: " PUBLIC_LOG_D64,
                        DECRYPT_UNIT_LEN - afterAlignRemainedLength_);
        }
        afterAlignRemainedLength_ += waitLen;
        return len;
    }
    writeLen = ((waitLen + afterAlignRemainedLength_) / DECRYPT_UNIT_LEN) *
                DECRYPT_UNIT_LEN - afterAlignRemainedLength_;
    realLen = GetDecrptyRealLen(writeDataPoint, waitLen, writeLen);
    totalLen_ += realLen;

    uint32_t writeSuccessLen = SaveCacheBufferData(decryptCache_, realLen, notBlock);

    err = memset_s(decryptCache_, realLen, 0x00, realLen);
    if (err != 0) {
        MEDIA_LOG_E("realLen: " PUBLIC_LOG_D32, realLen);
    }
    if (writeSuccessLen == realLen) {
        afterAlignRemainedLength_ = 0;
        err = memset_s(afterAlignRemainedBuffer_, DECRYPT_UNIT_LEN, 0x00, DECRYPT_UNIT_LEN);
        if (err != 0) {
            MEDIA_LOG_E("DECRYPT_UNIT_LEN: " PUBLIC_LOG_D64, DECRYPT_UNIT_LEN);
        }
    }
    if (writeLen > len) {
        MEDIA_LOG_D("writeLen: " PUBLIC_LOG_D32, writeLen);
    }
    writeDataPoint += writeLen;
    waitLen -= writeLen;
    if (waitLen > 0 && writeSuccessLen == realLen) {
        afterAlignRemainedLength_ = waitLen;
        err = memcpy_s(afterAlignRemainedBuffer_, DECRYPT_UNIT_LEN, writeDataPoint, waitLen);
        if (err != 0) {
            MEDIA_LOG_D("waitLen: " PUBLIC_LOG_D32, waitLen);
        }
    }
    MEDIA_LOG_D("SaveEncryptData, return len " PUBLIC_LOG_D32, len);
    return writeSuccessLen == realLen ? len : writeSuccessLen;
}

void HlsMediaDownloader::DownloadRecordHistory(int64_t nowTime)
{
    if ((static_cast<uint64_t>(nowTime) - lastWriteTime_) >= SAMPLE_INTERVAL) {
        MEDIA_LOG_I("HLS OnWriteRingBuffer nowTime: " PUBLIC_LOG_D64
        " lastWriteTime:" PUBLIC_LOG_D64 ".\n", nowTime, lastWriteTime_);
        BufferDownRecord* record = new BufferDownRecord();
        record->dataBits = lastWriteBit_;
        record->timeoff = static_cast<uint64_t>(nowTime) - lastWriteTime_;
        record->next = bufferDownRecord_;
        bufferDownRecord_ = record;
        lastWriteBit_ = 0;
        lastWriteTime_ = static_cast<uint64_t>(nowTime);
        BufferDownRecord* tmpRecord = bufferDownRecord_;
        int64_t loopStartTime = loopInterruptClock_.ElapsedSeconds();
        for (int i = 0; i < MAX_RECORD_COUNT; i++) {
            if (CheckLoopTimeout(loopStartTime)) {
                break;
            }
            if (tmpRecord->next) {
                tmpRecord = tmpRecord->next;
            } else {
                break;
            }
        }
        BufferDownRecord* next = tmpRecord->next;
        tmpRecord->next = nullptr;
        tmpRecord = next;
        loopStartTime = loopInterruptClock_.ElapsedSeconds();
        while (tmpRecord) {
            if (CheckLoopTimeout(loopStartTime)) {
                break;
            }
            next = tmpRecord->next;
            delete tmpRecord;
            tmpRecord = next;
        }
        if (autoBufferSize_ && !userDefinedBufferDuration_) {
            if (CheckRiseBufferSize()) {
                RiseBufferSize();
            } else if (CheckPulldownBufferSize()) {
                DownBufferSize();
            }
        }
    }
}

void HlsMediaDownloader::OnWriteCacheBuffer(uint32_t len)
{
    {
        AutoLock lock(initCacheMutex_);
        if (initCacheSize_.load() != -1) {
            if (IsCachedInitSizeReady(initCacheSize_.load())) {
                callback_->OnEvent({PluginEventType::INITIAL_BUFFER_SUCCESS,
                                       {BufferingInfoType::BUFFERING_END}, "end"});
                initCacheSize_.store(-1);
                expectOffset_.store(-1);
            }
        }
    }
    int64_t nowTime = steadyClock_.ElapsedMilliseconds();
    if (startDownloadTime_ == 0) {
        startDownloadTime_ = nowTime;
        lastReportUsageTime_ = nowTime;
    }
    uint32_t writeBits = len * BYTES_TO_BIT;   // bit
    bufferedDuration_ += writeBits;
    totalBits_ += writeBits;
    lastWriteBit_ += writeBits;
    dataUsage_ += writeBits;
    if ((totalBits_ > START_PLAY_WATER_LINE) && (playDelayTime_ == 0)) {
        auto startPlayTime = steadyClock_.ElapsedMilliseconds();
        playDelayTime_ = startPlayTime - openTime_;
        MEDIA_LOG_D("Start play delay time: " PUBLIC_LOG_D64, playDelayTime_);
    }
    DownloadRecordHistory(nowTime);
    DownloadReport();
}

double HlsMediaDownloader::CalculateCurrentDownloadSpeed()
{
    double downloadRate = 0;
    double tmpNumerator = static_cast<double>(downloadBits_);
    double tmpDenominator = static_cast<double>(downloadDuringTime_) / SECOND_TO_MILLISECONDS;
    if (tmpDenominator > ZERO_THRESHOLD) {
        downloadRate = tmpNumerator / tmpDenominator;
        avgDownloadSpeed_ = downloadRate;
        downloadDuringTime_ = 0;
        downloadBits_ = 0;
    }
    
    return downloadRate;
}

void HlsMediaDownloader::DownloadReport()
{
    uint64_t now = static_cast<uint64_t>(steadyClock_.ElapsedMilliseconds());
    if (now > lastCheckTime_ && now - lastCheckTime_ > SAMPLE_INTERVAL) {
        uint64_t curDownloadBits = totalBits_ - lastBits_;
        if (curDownloadBits >= IS_DOWNLOAD_MIN_BIT) {
            downloadDuringTime_ = now - lastCheckTime_;
            downloadBits_ = curDownloadBits;
            totalDownloadDuringTime_ += downloadDuringTime_;

            std::shared_ptr<RecordData> recordBuff = std::make_shared<RecordData>();
            double downloadRate = CalculateCurrentDownloadSpeed();
            recordBuff->downloadRate = downloadRate;
            uint64_t remainingBuffer = GetBufferSize();
            MEDIA_LOG_D("Current download speed : " PUBLIC_LOG_D32 " Kbit/s,Current buffer size : " PUBLIC_LOG_U64
                " KByte", static_cast<int32_t>(downloadRate / KILO), static_cast<uint64_t>(remainingBuffer / KILO));
            MediaAVCodec::AVCodecTrace trace("HlsMediaDownloader::DownloadReport, download speed: " +
                std::to_string(downloadRate) + " bit/s, bufferSize: " + std::to_string(remainingBuffer) + " Byte");
            // Remaining playable time: s
            uint64_t bufferDuration = 0;
            if (readBitrate_ > 0) {
                bufferDuration = bufferedDuration_ / static_cast<uint64_t>(readBitrate_);
            } else {
                bufferDuration = bufferedDuration_ / CURRENT_BIT_RATE;
            }
            recordBuff->bufferDuring = bufferDuration;
            recordData_ = recordBuff;
            recordCount_++;
        }
        // Total amount of downloaded data
        lastBits_ = totalBits_;
        lastCheckTime_ = now;
    }
    if (!isDownloadFinish_ && (static_cast<int64_t>(now) - lastReportUsageTime_) > DATA_USAGE_INTERVAL) {
        MEDIA_LOG_D("Data usage: " PUBLIC_LOG_U64 " bits in " PUBLIC_LOG_D32 "ms", dataUsage_, DATA_USAGE_INTERVAL);
        dataUsage_ = 0;
        lastReportUsageTime_ = static_cast<int64_t>(now);
    }
}

void HlsMediaDownloader::OnSourceKeyChange(const uint8_t *key, size_t keyLen, const uint8_t *iv)
{
    keyLen_ = keyLen;
    if (keyLen <= 0) {
        return;
    }
    NZERO_LOG(memcpy_s(iv_, DECRYPT_UNIT_LEN, iv, DECRYPT_UNIT_LEN));
    NZERO_LOG(memcpy_s(initIv_, DECRYPT_UNIT_LEN, iv, DECRYPT_UNIT_LEN));
    NZERO_LOG(memcpy_s(key_, DECRYPT_UNIT_LEN, key, keyLen > DECRYPT_UNIT_LEN ? DECRYPT_UNIT_LEN : keyLen));
    AES_set_decrypt_key(key_, DECRYPT_COPY_LEN, &aesKey_);
}

void HlsMediaDownloader::OnDrmInfoChanged(const std::multimap<std::string, std::vector<uint8_t>>& drmInfos)
{
    if (callback_ != nullptr) {
        callback_->OnEvent({PluginEventType::SOURCE_DRM_INFO_UPDATE, {drmInfos}, "drm_info_update"});
    }
}

void HlsMediaDownloader::SetStatusCallback(StatusCallbackFunc cb)
{
    FALSE_RETURN(playlistDownloader_ != nullptr);
    statusCallback_ = cb;
    playlistDownloader_->SetStatusCallback(cb);
}

std::vector<uint32_t> HlsMediaDownloader::GetBitRates()
{
    FALSE_RETURN_V(playlistDownloader_ != nullptr, std::vector<uint32_t>());
    return playlistDownloader_->GetBitRates();
}

bool HlsMediaDownloader::SelectBitRate(uint32_t bitRate)
{
    AutoLock lock(switchMutex_);
    FALSE_RETURN_V(playlistDownloader_ != nullptr, false);
    FALSE_RETURN_V(playList_ != nullptr, false);
    if (playlistDownloader_->IsBitrateSame(bitRate)) {
        return true;
    }
    if (playlistDownloader_->IsHlsFmp4() && callback_ && isAutoSelectBitrate_ && !CheckReadStatus()) {
        bool switchFlag = true;
        switchFlag = callback_->CanAutoSelectBitRate();
        if (!switchFlag) {
            return true;
        }
        callback_->SetSelectBitRateFlag(true, bitRate);
    }
    if (isEos_) {
        MEDIA_LOG_I("HLS download done.");
    }
    // report change bitrate start
    ReportBitrateStart(bitRate);

    playlistDownloader_->Cancel();

    // clear request queue
    playList_->SetActive(false, true);
    playList_->SetActive(true);
    fragmentDownloadStart.clear();
    fragmentPushed.clear();
    backPlayList_.clear();
    
    // switch to des bitrate
    playlistDownloader_->SelectBitRate(bitRate);
    playlistDownloader_->Start();
    isSelectingBitrate_ = true;
    playlistDownloader_->UpdateManifest();
    return true;
}

void HlsMediaDownloader::SeekToTs(uint64_t seekTime, SeekMode mode)
{
    MEDIA_LOG_I("SeekToTs: in.");
    writeTsIndex_ = 0;
    double totalDuration = 0;
    isDownloadStarted_ = false;
    playList_->Clear();
    int64_t loopStartTime = loopInterruptClock_.ElapsedSeconds();
    for (const auto &item : backPlayList_) {
        if (CheckLoopTimeout(loopStartTime)) {
            break;
        }
        double hstTime = item.duration_ * HST_SECOND;
        totalDuration += hstTime / HST_NSECOND;
        if (seekTime >=  static_cast<uint64_t>(totalDuration)) {
            writeTsIndex_++;
            continue;
        }
        if (RequestNewTs(seekTime, mode, totalDuration, hstTime, item) == -1) {
            seekFailedCount_ ++;
            MEDIA_LOG_D("Seek failed count: " PUBLIC_LOG_D32, seekFailedCount_);
            continue;
        }
    }
}

int64_t HlsMediaDownloader::RequestNewTs(uint64_t seekTime, SeekMode mode, double totalDuration,
    double hstTime, const PlayInfo& item)
{
    PlayInfo playInfo;
    playInfo.url_ = item.url_;
    playInfo.rangeUrl_ = item.rangeUrl_;
    playInfo.streamId_ = item.streamId_;
    playInfo.duration_ = item.duration_;
    playInfo.offset_ = item.offset_;
    playInfo.length_ = item.length_;
    if (mode == SeekMode::SEEK_PREVIOUS_SYNC) {
        double lastTotalDuration = totalDuration - hstTime;
        if (static_cast<uint64_t>(lastTotalDuration) <= seekTime) {
            seekStartTimePos_ = lastTotalDuration;
        }
        playInfo.startTimePos_ = 0;
    } else {
        int64_t startTimePos = 0;
        double lastTotalDuration = totalDuration - hstTime;
        if (static_cast<uint64_t>(lastTotalDuration) <= seekTime) {
            startTimePos = static_cast<int64_t>(seekTime) - static_cast<int64_t>(lastTotalDuration);
            seekStartTimePos_ = lastTotalDuration;
            if (startTimePos > (int64_t)(hstTime / HALF_DIVIDE) && (&item != &backPlayList_.back())) { // 2
                writeTsIndex_++;
                seekStartTimePos_ = totalDuration;
                MEDIA_LOG_I("writeTsIndex, RequestNewTs update writeTsIndex " PUBLIC_LOG_U32, writeTsIndex_);
                return -1;
            }
            startTimePos = 0;
        }
        playInfo.startTimePos_ = startTimePos;
    }
    PushPlayInfo(playInfo);
    return 0;
}
void HlsMediaDownloader::PushPlayInfo(PlayInfo playInfo)
{
    fragmentDownloadStart[playInfo.url_] = false;
    if (playlistDownloader_ && playlistDownloader_->IsHlsFmp4()) {
        curStreamId_ = playInfo.streamId_;
    }
    if (!isDownloadStarted_) {
        isDownloadStarted_ = true;
        // To avoid downloader potentially stopped by curl error caused by break readbuffer blocking in seeking
        OSAL::SleepFor(6); // sleep 6ms
        readOffset_ = SpliceOffset(writeTsIndex_, 0);
        writeOffset_ = readOffset_;
        readTsIndex_ = writeTsIndex_;
        {
            AutoLock lock(tsStorageInfoMutex_);
            if (tsStorageInfo_.find(writeTsIndex_) != tsStorageInfo_.end()) {
                tsStorageInfo_[writeTsIndex_] = std::make_pair(0, false);
            } else {
                tsStorageInfo_[writeTsIndex_].first = 0;
                tsStorageInfo_[writeTsIndex_].second = false;
            }
        }
        PutRequestIntoDownloader(playInfo);
    } else {
        playList_->Push(playInfo);
    }
}

void HlsMediaDownloader::SeekToTsForRead(uint32_t currentTsIndex)
{
    MEDIA_LOG_I("SeekToTimeForRead: currentTsIndex " PUBLIC_LOG_U32, currentTsIndex);
    FALSE_RETURN_MSG(cacheMediaBuffer_ != nullptr, "cacheMediaBuffer_ is nullptr");
    AutoLock lock(switchMutex_);
    isSeekingFlag = true;
    PrepareToSeek();

    writeTsIndex_ = 0;
    isDownloadStarted_ = false;
    playList_->Clear();
    int64_t loopStartTime = loopInterruptClock_.ElapsedSeconds();
    for (const auto &item : backPlayList_) {
        if (CheckLoopTimeout(loopStartTime)) {
            break;
        }
        if (writeTsIndex_ < currentTsIndex) {
            writeTsIndex_++;
            continue;
        }
        if (RequestNewTsForRead(item) == -1) {
            seekFailedCount_++;
            MEDIA_LOG_D("Seek failed count: " PUBLIC_LOG_U32, seekFailedCount_);
            continue;
        }
    }
    MEDIA_LOG_I("SeekToTimeForRead end");
    isSeekingFlag = false;
}

int64_t HlsMediaDownloader::RequestNewTsForRead(const PlayInfo& item)
{
    PlayInfo playInfo;
    playInfo.url_ = item.url_;
    playInfo.rangeUrl_ = item.rangeUrl_;
    playInfo.streamId_ = item.streamId_;
    playInfo.duration_ = item.duration_;
    playInfo.offset_ = item.offset_;
    playInfo.length_ = item.length_;
    playInfo.startTimePos_ = 0;
    PushPlayInfo(playInfo);
    return 0;
}

void HlsMediaDownloader::UpdateDownloadFinished(const std::string &url, const std::string& location)
{
    uint32_t bitRate = downloadRequest_->GetBitRate();
    {
        AutoLock lock(tsStorageInfoMutex_);
        tsStorageInfo_[writeTsIndex_].second = true;
    }
    if (keyLen_ > 0) {
        NZERO_LOG(memcpy_s(iv_, DECRYPT_UNIT_LEN, initIv_, DECRYPT_UNIT_LEN));
    }
    auto playInfo = playList_->Pop(POP_TIME_OUT_MS);
    if (!playInfo.url_.empty()) {
        writeTsIndex_++;
        size_t fragmentSize = downloadRequest_->GetFileContentLength();
        double duration = downloadRequest_->GetDuration();
        CalculateBitRate(fragmentSize, duration);
        PutRequestIntoDownloader(playInfo);
    } else {
        isDownloadStarted_ = false;
        if (isSeekingFlag) {
            return;
        }
        isDownloadFinish_ = true;
        MEDIA_LOG_D("Download done, average download speed : " PUBLIC_LOG_D32 " bit/s", avgDownloadSpeed_);
        int64_t nowTime = steadyClock_.ElapsedMilliseconds();
        auto downloadTime = (nowTime - startDownloadTime_) / 1000;
        MEDIA_LOG_D("Download done, data usage: " PUBLIC_LOG_U64 " bits in " PUBLIC_LOG_D64 "ms",
            totalBits_, downloadTime * 1000);
        HandleBuffering();
    }

    // bitrate above 0, user is not selecting, auto seliect is not going, playlist is done, is not seeking
    if ((bitRate > 0) && !isSelectingBitrate_ && isAutoSelectBitrate_ &&
        playlistDownloader_ != nullptr && playlistDownloader_->IsParseAndNotifyFinished() && !isSeekingFlag) {
        AutoSelectBitrate(bitRate);
    }
}

void HlsMediaDownloader::SetReadBlockingFlag(bool isReadBlockingAllowed)
{
    MEDIA_LOG_D("SetReadBlockingFlag entered");
}

void HlsMediaDownloader::SetIsTriggerAutoMode(bool isAuto)
{
    isAutoSelectBitrate_ = isAuto;
}

void HlsMediaDownloader::ReportVideoSizeChange()
{
    if (callback_ == nullptr) {
        MEDIA_LOG_I("HLS callback == nullptr dont report video size change");
        return;
    }
    FALSE_RETURN(playlistDownloader_ != nullptr);
    int32_t videoWidth = playlistDownloader_->GetVideoWidth();
    int32_t videoHeight = playlistDownloader_->GetVideoHeight();
    MEDIA_LOG_I("HLS ReportVideoSizeChange videoWidth : " PUBLIC_LOG_D32 "videoHeight: "
        PUBLIC_LOG_D32, videoWidth, videoHeight);
    changeBitRateCount_++;
    MEDIA_LOG_I("HLS Change bit rate count : " PUBLIC_LOG_U32, changeBitRateCount_);
    std::pair<int32_t, int32_t> videoSize {videoWidth, videoHeight};
    callback_->OnEvent({PluginEventType::VIDEO_SIZE_CHANGE, {videoSize}, "video_size_change"});
}

void HlsMediaDownloader::SetDemuxerState(int32_t streamId)
{
    MEDIA_LOG_I("HLS SetDemuxerState");
    isReadFrame_ = true;
    isFirstFrameArrived_ = true;
}

void HlsMediaDownloader::SetDownloadErrorState()
{
    MEDIA_LOG_I("HLS SetDownloadErrorState");
    downloadErrorState_ = true;
    if (callback_ != nullptr && !isReportedErrorCode_) {
        callback_->OnEvent({PluginEventType::CLIENT_ERROR, {NetworkClientErrorCode::ERROR_TIME_OUT}, "download"});
        isTimeoutErrorNotified_.store(true);
    }
    Close(true);
}

void HlsMediaDownloader::AutoSelectBitrate(uint32_t bitRate)
{
    MEDIA_LOG_I("HLS AutoSelectBitrate download bitrate " PUBLIC_LOG_D32, bitRate);
    FALSE_RETURN(playlistDownloader_ != nullptr);
    std::vector<uint32_t> bitRates = playlistDownloader_->GetBitRates();
    if (bitRates.size() == 0) {
        return;
    }
    sort(bitRates.begin(), bitRates.end());
    uint32_t desBitRate = bitRates[0];
    int64_t loopStartTime = loopInterruptClock_.ElapsedSeconds();
    for (const auto &item : bitRates) {
        if (CheckLoopTimeout(loopStartTime)) {
            break;
        }
        if (item < bitRate * 0.8) { // 0.8
            desBitRate = item;
        } else {
            break;
        }
    }
    uint32_t curBitrate = playlistDownloader_->GetCurBitrate();
    if (desBitRate == curBitrate) {
        return;
    }
    uint32_t bufferLowSize = static_cast<uint32_t>(static_cast<double>(bitRate) / 8.0 * 0.3);

    // switch to high bitrate,if buffersize less than lowsize, do not switch
    if (curBitrate < desBitRate && GetBufferSize() < bufferLowSize) {
        MEDIA_LOG_I("AutoSelectBitrate curBitrate " PUBLIC_LOG_D32 ", desBitRate " PUBLIC_LOG_D32
                    ", bufferLowSize " PUBLIC_LOG_D32, curBitrate, desBitRate, bufferLowSize);
        return;
    }
    uint32_t bufferHighSize = MIN_BUFFER_SIZE * 0.8; // high size: buffersize * 0.8

    // switch to low bitrate, if buffersize more than highsize, do not switch
    if (curBitrate > desBitRate && GetBufferSize() > bufferHighSize) {
        MEDIA_LOG_I("HLS AutoSelectBitrate curBitrate " PUBLIC_LOG_D32 ", desBitRate " PUBLIC_LOG_D32
                     ", bufferHighSize " PUBLIC_LOG_D32, curBitrate, desBitRate, bufferHighSize);
        return;
    }
    MEDIA_LOG_I("HLS AutoSelectBitrate " PUBLIC_LOG_D32 " switch to " PUBLIC_LOG_D32, curBitrate, desBitRate);
    SelectBitRate(desBitRate);
}

bool HlsMediaDownloader::CheckRiseBufferSize()
{
    if (recordData_ == nullptr) {
        return false;
    }
    FALSE_RETURN_V(playlistDownloader_ != nullptr, false);
    bool isHistoryLow = false;
    std::shared_ptr<RecordData> search = recordData_;
    uint64_t playingBitrate = playlistDownloader_->GetCurrentBitRate();
    if (playingBitrate == 0) {
        playingBitrate = TransferSizeToBitRate(playlistDownloader_->GetVideoWidth());
    }
    if (search->downloadRate > playingBitrate) {
        MEDIA_LOG_I("HLS downloadRate: " PUBLIC_LOG_D64 "current bit rate: "
        PUBLIC_LOG_D64 ", increasing buffer size.", static_cast<uint64_t>(search->downloadRate), playingBitrate);
        isHistoryLow = true;
    }
    return isHistoryLow;
}

bool HlsMediaDownloader::CheckPulldownBufferSize()
{
    FALSE_RETURN_V(recordData_ != nullptr, false);
    FALSE_RETURN_V(playlistDownloader_ != nullptr, false);
    bool isPullDown = false;
    uint64_t playingBitrate = playlistDownloader_ -> GetCurrentBitRate();
    if (playingBitrate == 0) {
        playingBitrate = TransferSizeToBitRate(playlistDownloader_->GetVideoWidth());
    }
    std::shared_ptr<RecordData> search = recordData_;
    if (search->downloadRate < playingBitrate) {
        MEDIA_LOG_I("HLS downloadRate: " PUBLIC_LOG_D64 "current bit rate: "
        PUBLIC_LOG_D64 ", reducing buffer size.", static_cast<uint64_t>(search->downloadRate), playingBitrate);
        isPullDown = true;
    }
    return isPullDown;
}

void HlsMediaDownloader::RiseBufferSize()
{
    if (totalBufferSize_ >= MAX_BUFFER_SIZE) {
        MEDIA_LOGI_LIMIT(SAVE_DATA_LOG_FREQUENCY, "HLS increasing buffer size failed, already reach the max buffer "
        "size: " PUBLIC_LOG_D64 ", current buffer size: " PUBLIC_LOG_ZU, MAX_BUFFER_SIZE, totalBufferSize_);
        return;
    }
    size_t tmpBufferSize = totalBufferSize_ + 1 * 1024 * 1024;
    totalBufferSize_ = tmpBufferSize;
    MEDIA_LOG_I("HLS increasing buffer size: " PUBLIC_LOG_ZU, totalBufferSize_);
}

void HlsMediaDownloader::DownBufferSize()
{
    if (totalBufferSize_ <= MIN_BUFFER_SIZE) {
        MEDIA_LOG_I("HLS reducing buffer size failed, already reach the min buffer size: "
        PUBLIC_LOG_ZU ", current buffer size: " PUBLIC_LOG_ZU, MIN_BUFFER_SIZE, totalBufferSize_);
        return;
    }
    size_t tmpBufferSize = totalBufferSize_ - 1 * 1024 * 1024;
    totalBufferSize_ = tmpBufferSize;
    MEDIA_LOG_I("HLS reducing buffer size: " PUBLIC_LOG_ZU, totalBufferSize_);
}

void HlsMediaDownloader::OnReadBuffer(uint32_t len)
{
    static uint32_t minDuration = 0;
    uint64_t nowTime = static_cast<uint64_t>(steadyClock_.ElapsedMilliseconds());
    // Bytes to bit
    uint32_t duration = len * 8;
    if (duration >= bufferedDuration_) {
        bufferedDuration_ = 0;
    } else {
        bufferedDuration_ -= duration;
    }

    if (minDuration == 0 || bufferedDuration_ < minDuration) {
        minDuration = bufferedDuration_;
    }
    if ((nowTime - lastReadTime_) >= SAMPLE_INTERVAL || bufferedDuration_ == 0) {
        BufferLeastRecord* record = new BufferLeastRecord();
        record->minDuration = minDuration;
        record->next = bufferLeastRecord_;
        bufferLeastRecord_ = record;
        lastReadTime_ = nowTime;
        minDuration = 0;
        // delete all after bufferLeastRecord_[MAX_RECORD_CT]
        BufferLeastRecord* tmpRecord = bufferLeastRecord_;
        int64_t loopStartTime = loopInterruptClock_.ElapsedSeconds();
        for (int i = 0; i < MAX_RECORD_COUNT; i++) {
            if (CheckLoopTimeout(loopStartTime)) {
                break;
            }
            if (tmpRecord->next) {
                tmpRecord = tmpRecord->next;
            } else {
                break;
            }
        }
        BufferLeastRecord* next = tmpRecord->next;
        tmpRecord->next = nullptr;
        tmpRecord = next;
        loopStartTime = loopInterruptClock_.ElapsedSeconds();
        while (tmpRecord) {
            if (CheckLoopTimeout(loopStartTime)) {
                break;
            }
            next = tmpRecord->next;
            delete tmpRecord;
            tmpRecord = next;
        }
    }
}

void HlsMediaDownloader::ActiveAutoBufferSize()
{
    if (userDefinedBufferDuration_) {
        MEDIA_LOG_I("HLS User has already setted a buffersize, can not switch auto buffer size");
        return;
    }
    autoBufferSize_ = true;
}

void HlsMediaDownloader::InActiveAutoBufferSize()
{
    autoBufferSize_ = false;
}

uint64_t HlsMediaDownloader::TransferSizeToBitRate(int width)
{
    if (width <= MIN_WIDTH) {
        return MIN_BUFFER_SIZE;
    } else if (width >= MIN_WIDTH && width < SECOND_WIDTH) {
        return MIN_BUFFER_SIZE * TRANSFER_SIZE_RATE_2;
    } else if (width >= SECOND_WIDTH && width < THIRD_WIDTH) {
        return MIN_BUFFER_SIZE * TRANSFER_SIZE_RATE_3;
    } else {
        return MIN_BUFFER_SIZE * TRANSFER_SIZE_RATE_4;
    }
}

size_t HlsMediaDownloader::GetTotalBufferSize()
{
    return totalBufferSize_;
}

void HlsMediaDownloader::SetInterruptState(bool isInterruptNeeded)
{
    MEDIA_LOG_I("SetInterruptState: " PUBLIC_LOG_D32, isInterruptNeeded);
    {
        AutoLock lk(bufferingEndMutex_);
        isInterruptNeeded_ = isInterruptNeeded;
        if (isInterruptNeeded_) {
            MEDIA_LOG_I("SetInterruptState bufferingEndCond NotifyAll.");
            bufferingEndCond_.NotifyAll();
        }
    }
    if (playlistDownloader_ != nullptr) {
        playlistDownloader_->SetInterruptState(isInterruptNeeded);
    }
    if (downloader_ != nullptr) {
        downloader_->SetInterruptState(isInterruptNeeded);
    }
}

void HlsMediaDownloader::GetDownloadInfo(DownloadInfo& downloadInfo)
{
    if (recordSpeedCount_ == 0) {
        MEDIA_LOG_E("HlsMediaDownloader is 0, can't get avgDownloadRate");
        downloadInfo.avgDownloadRate = 0;
    } else {
        downloadInfo.avgDownloadRate = avgSpeedSum_ / recordSpeedCount_;
    }
    downloadInfo.avgDownloadSpeed = avgDownloadSpeed_;
    downloadInfo.totalDownLoadBits = totalBits_;
    downloadInfo.isTimeOut = isTimeOut_;
}

void HlsMediaDownloader::GetPlaybackInfo(PlaybackInfo& playbackInfo)
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
        uint64_t remainingBuffer = GetBufferSize();
        uint64_t bufferDuration = 0;
        if (readBitrate_ > 0) {
            bufferDuration = remainingBuffer / readBitrate_;
        } else {
            bufferDuration = remainingBuffer / CURRENT_BIT_RATE;
        }
        playbackInfo.bufferDuration = static_cast<int64_t>(bufferDuration);
    } else {
        playbackInfo.downloadRate = 0;
        playbackInfo.bufferDuration = 0;
    }
}

void HlsMediaDownloader::ReportBitrateStart(uint32_t bitRate)
{
    if (callback_ == nullptr) {
        MEDIA_LOG_I("HLS callback_ == nullptr dont report bitrate start");
        return;
    }
    MEDIA_LOG_I("HLS ReportBitrateStart bitRate : " PUBLIC_LOG_U32, bitRate);
    callback_->OnEvent({PluginEventType::SOURCE_BITRATE_START, {bitRate}, "source_bitrate_start"});
}

std::pair<int32_t, int32_t> HlsMediaDownloader::GetDownloadInfo()
{
    MEDIA_LOG_I("HlsMediaDownloader::GetDownloadInfo.");
    if (recordSpeedCount_ == 0) {
        MEDIA_LOG_E("recordSpeedCount_ is 0, can't get avgDownloadRate");
        return std::make_pair(0, static_cast<int32_t>(avgDownloadSpeed_));
    }
    auto rateAndSpeed = std::make_pair(avgSpeedSum_ / recordSpeedCount_, static_cast<int32_t>(avgDownloadSpeed_));
    return rateAndSpeed;
}

std::pair<int32_t, int32_t> HlsMediaDownloader::GetDownloadRateAndSpeed()
{
    MEDIA_LOG_I("HlsMediaDownloader::GetDownloadRateAndSpeed.");
    if (recordSpeedCount_ == 0) {
        MEDIA_LOG_E("recordSpeedCount_ is 0, can't get avgDownloadRate");
        return std::make_pair(0, static_cast<int32_t>(avgDownloadSpeed_));
    }
    auto rateAndSpeed = std::make_pair(avgSpeedSum_ / recordSpeedCount_, static_cast<int32_t>(avgDownloadSpeed_));
    return rateAndSpeed;
}

Status HlsMediaDownloader::SetCurrentBitRate(int32_t bitRate, int32_t streamID)
{
    MEDIA_LOG_I("HLS SetCurrentBitRate: " PUBLIC_LOG_D32, bitRate);
    if (bitRate <= 0 && currentBitRate_ == 0) {
        currentBitRate_ = -1; // -1
    } else {
        int32_t playlistBitrate = static_cast<int32_t>(playlistDownloader_->GetCurBitrate());
        currentBitRate_ = std::max(playlistBitrate, static_cast<int32_t>(bitRate));
        MEDIA_LOG_I("HLS playlistBitrate: " PUBLIC_LOG_D32 " currentBitRate: " PUBLIC_LOG_D32,
            playlistBitrate, currentBitRate_);
    }
    if (bufferDurationForPlaying_ > 0 && currentBitRate_ > 0) {
        waterlineForPlaying_ = static_cast<uint64_t>(static_cast<double>(currentBitRate_) /
            static_cast<double>(BYTES_TO_BIT) * bufferDurationForPlaying_);
    }
    if (downloadRequest_) {
        downloadRequest_->SetBitRateToRequestSize(currentBitRate_);
    }
    return Status::OK;
}

void HlsMediaDownloader::CalculateBitRate(size_t fragmentSize, double duration)
{
    if (fragmentSize == 0 || duration < ZERO_THRESHOLD) {
        return;
    }
    double divisorFragmentSize = (static_cast<double>(fragmentSize) / static_cast<double>(ONE_SECONDS))
                                    * static_cast<double>(BYTES_TO_BIT);
    double dividendDuration = static_cast<double>(duration) / static_cast<double>(ONE_SECONDS);
    uint32_t calculateBitRate = static_cast<uint32_t>(divisorFragmentSize / dividendDuration);
    uint32_t tmpBitRate = static_cast<uint32_t>(currentBitRate_);
    tmpBitRate = (calculateBitRate >> 1) + (tmpBitRate >> 1) + ((calculateBitRate | tmpBitRate) & 1);
    currentBitRate_ = static_cast<int32_t>(tmpBitRate);
    MEDIA_LOG_I("HLS Calculate avgBitRate: " PUBLIC_LOG_D32, currentBitRate_);
}

void HlsMediaDownloader::UpdateWaterLineAbove()
{
    if (!isFirstFrameArrived_) {
        return;
    }
    if (currentBitRate_ == 0) {
        currentBitRate_ = static_cast<int32_t>(playlistDownloader_->GetCurBitrate());
        MEDIA_LOG_I("HLS use playlist bitrate: " PUBLIC_LOG_D32, currentBitRate_);
    }
    size_t waterLineAbove = DEFAULT_WATER_LINE_ABOVE;
    if (currentBitRate_ > 0) {
        float cacheTime = 0;
        uint64_t writeBitrate = writeBitrateCaculator_->GetWriteBitrate();
        if (writeBitrate > 0) {
            float ratio = static_cast<float>(writeBitrate) /
                          static_cast<float>(currentBitRate_);
            cacheTime = GetCacheDuration(ratio);
        } else {
            cacheTime = DEFAULT_CACHE_TIME;
        }
        waterLineAbove = static_cast<size_t>(cacheTime * currentBitRate_ / BYTES_TO_BIT);
        waterLineAbove = std::max(MIN_WATER_LINE_ABOVE, waterLineAbove);
    } else {
        MEDIA_LOG_D("HLS UpdateWaterLineAbove default: " PUBLIC_LOG_ZU, waterLineAbove);
    }
    waterLineAbove_ = std::min(waterLineAbove, static_cast<size_t>(totalBufferSize_ *
        WATER_LINE_ABOVE_LIMIT_RATIO));
    MEDIA_LOG_D("HLS UpdateWaterLineAbove: " PUBLIC_LOG_ZU " writeBitrate: " PUBLIC_LOG_U64 " avgDownloadSpeed: "
        PUBLIC_LOG_D32 " currentBitRate: " PUBLIC_LOG_D32,
        waterLineAbove_, writeBitrateCaculator_->GetWriteBitrate(), avgDownloadSpeed_, currentBitRate_);
}

void HlsMediaDownloader::HandleCachedDuration()
{
    auto tmpBitRate = currentBitRate_;
    if (tmpBitRate <= 0 || callback_ == nullptr) {
        return;
    }
    cachedDuration_ = (GetTotalTsBuffersize() *
        BYTES_TO_BIT * SECOND_TO_MILLISECONDS) / static_cast<uint64_t>(tmpBitRate);
    if ((cachedDuration_ > lastDurationReacord_ &&
        cachedDuration_ - lastDurationReacord_ > DURATION_CHANGE_AMOUT_MILLISECONDS) ||
        (lastDurationReacord_ > cachedDuration_ &&
        lastDurationReacord_ - cachedDuration_ > DURATION_CHANGE_AMOUT_MILLISECONDS)) {
        MEDIA_LOG_D("HLS OnEvent cachedDuration: " PUBLIC_LOG_U64, cachedDuration_);
        callback_->OnEvent({PluginEventType::CACHED_DURATION, {cachedDuration_}, "buffering_duration"});
        lastDurationReacord_ = cachedDuration_;
    }
}

void HlsMediaDownloader::UpdateCachedPercent(BufferingInfoType infoType)
{
    if (waterLineAbove_ == 0 || callback_ == nullptr) {
        MEDIA_LOG_E("HLS UpdateCachedPercent: ERROR");
        return;
    }
    if (infoType == BufferingInfoType::BUFFERING_START) {
        lastCachedSize_ = 0;
        isBufferingStart_ = true;
        return;
    }
    if (infoType == BufferingInfoType::BUFFERING_END) {
        bufferingTime_ = 0;
        lastCachedSize_ = 0;
        isBufferingStart_ = false;
        return;
    }
    if (infoType != BufferingInfoType::BUFFERING_PERCENT || !isBufferingStart_) {
        return;
    }
    uint64_t bufferSize = GetBufferSize();
    if (bufferSize < lastCachedSize_) {
        return;
    }
    uint64_t deltaSize = bufferSize - lastCachedSize_;
    if (deltaSize >= UPDATE_CACHE_STEP) {
        uint64_t percentValue = (bufferSize >= waterLineAbove_) ?
                        HUNDRED_PERCENTS : bufferSize * HUNDRED_PERCENTS / waterLineAbove_;
        int percent = static_cast<int>(percentValue);
        callback_->OnEvent({PluginEventType::EVENT_BUFFER_PROGRESS, {percent}, "buffer percent"});
        lastCachedSize_ = bufferSize;
    }
}

bool HlsMediaDownloader::CheckBufferingOneSeconds()
{
    MEDIA_LOG_I("HLS CheckBufferingOneSeconds in");
    int32_t sleepTime = 0;
    // return error again 1 time 1s, avoid ffmpeg error
    int64_t loopStartTime = loopInterruptClock_.ElapsedSeconds();
    while (sleepTime < ONE_SECONDS && !isInterruptNeeded_.load()) {
        if (CheckLoopTimeout(loopStartTime)) {
            break;
        }
        if (!isBuffering_) {
            break;
        }
        if (CheckBreakCondition()) {
            break;
        }
        OSAL::SleepFor(TEN_MILLISECONDS);
        sleepTime += TEN_MILLISECONDS;
    }
    MEDIA_LOG_I("HLS CheckBufferingOneSeconds out");
    return isBuffering_.load();
}

void HlsMediaDownloader::SetAppUid(int32_t appUid)
{
    if (downloader_) {
        downloader_->SetAppUid(appUid);
    }
    if (playlistDownloader_) {
        playlistDownloader_->SetAppUid(appUid);
    }
}

float HlsMediaDownloader::GetCacheDuration(float ratio)
{
    if (ratio >= 1) {
        return CACHE_LEVEL_1;
    }
    return DEFAULT_CACHE_TIME;
}

uint64_t HlsMediaDownloader::GetBufferSize() const
{
    uint64_t bufferSize = 0;
    if (cacheMediaBuffer_ != nullptr) {
        bufferSize = cacheMediaBuffer_->GetBufferSize(readOffset_);
    }
    return bufferSize;
}

uint64_t HlsMediaDownloader::GetCrossTsBuffersize()
{
    uint64_t bufferSize = 0;
    if (cacheMediaBuffer_ == nullptr) {
        return bufferSize;
    }
    bufferSize = cacheMediaBuffer_->GetBufferSize(readOffset_);
    if (!backPlayList_.empty() && readTsIndex_ < backPlayList_.size() - 1) {
        uint64_t nextTsOffset = SpliceOffset(readTsIndex_ + 1, 0);
        uint64_t nextTsBuffersize = cacheMediaBuffer_->GetBufferSize(nextTsOffset);
        bufferSize += nextTsBuffersize;
    }
    return bufferSize;
}

bool HlsMediaDownloader::IsCachedInitSizeReady(int32_t wantInitSize)
{
    if (wantInitSize < 0) {
        return false;
    }
    uint64_t bufferSize = 0;
    if (cacheMediaBuffer_ == nullptr) {
        return false;
    }
    size_t listSize = backPlayList_.size();
    int64_t loopStartTime = loopInterruptClock_.ElapsedSeconds();
    for (size_t i = 0; i < listSize; i++) {
        if (CheckLoopTimeout(loopStartTime)) {
            break;
        }
        if (i + 1 == listSize) {
            return true;
        }
        if (tsStorageInfo_.find(i) == tsStorageInfo_.end()) {
            break;
        } else {
            uint64_t currentTsStart = SpliceOffset(i, 0);
            bufferSize += cacheMediaBuffer_->GetBufferSize(currentTsStart);
            if (bufferSize >= static_cast<uint64_t>(wantInitSize)) {
                return true;
            }
            if (!tsStorageInfo_[i].second) {
                break;
            }
        }
    }
    return false;
}

bool HlsMediaDownloader::GetPlayable()
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

bool HlsMediaDownloader::GetBufferingTimeOut()
{
    if (bufferingTime_ == 0) {
        return false;
    } else {
        size_t now = static_cast<size_t>(steadyClock_.ElapsedMilliseconds());
        return now >= bufferingTime_ ? now - bufferingTime_ >= timeoutInterval_ : false;
    }
}

bool HlsMediaDownloader::GetReadTimeOut(bool isDelay)
{
    size_t now = static_cast<size_t>(steadyClock_.ElapsedMilliseconds());
    if (isDelay) { // NOT_READY
        timeoutInterval_ = MAX_BUFFERING_TIME_OUT_DELAY;
    }
    return (now >= readTime_) ? (now - readTime_ >= timeoutInterval_) : false;
}

size_t HlsMediaDownloader::GetSegmentOffset()
{
    if (playlistDownloader_) {
        return playlistDownloader_->GetSegmentOffset(readTsIndex_);
    }
    return 0;
}

bool HlsMediaDownloader::GetHLSDiscontinuity()
{
    if (playlistDownloader_) {
        return playlistDownloader_->GetHLSDiscontinuity();
    }
    return false;
}

Status HlsMediaDownloader::StopBufferring(bool isAppBackground)
{
    MEDIA_LOG_I("HlsMediaDownloader:StopBufferring enter");
    if (cacheMediaBuffer_ == nullptr || downloader_ == nullptr || playlistDownloader_ == nullptr) {
        MEDIA_LOG_E("StopBufferring error.");
        return Status::ERROR_NULL_POINTER;
    }
    downloader_->SetAppState(isAppBackground);
    if (isAppBackground) {
        isInterrupt_ = true;
    } else {
        isInterrupt_ = false;
    }
    bufferingTime_ = static_cast<size_t>(steadyClock_.ElapsedMilliseconds());
    downloader_->StopBufferring();
    playlistDownloader_->StopBufferring(isAppBackground);
    MEDIA_LOG_I("HlsMediaDownloader:StopBufferring out");
    return Status::OK;
}

bool HlsMediaDownloader::ClearChunksOfFragment()
{
    bool res = false;
    uint64_t offsetBegin = SpliceOffset(readTsIndex_, 0);
    uint64_t diff = readOffset_ > offsetBegin ? readOffset_ - offsetBegin : 0;
    if (diff > READ_BACK_SAVE_SIZE) {
        MEDIA_LOG_D("HLS ClearChunksOfFragment: " PUBLIC_LOG_U64, readOffset_);
        res = cacheMediaBuffer_->ClearChunksOfFragment(readOffset_ - READ_BACK_SAVE_SIZE);
    }
    return res;
}

void HlsMediaDownloader::WaitForBufferingEnd()
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

void HlsMediaDownloader::SetIsReportedErrorCode()
{
    isReportedErrorCode_ = true;
}

bool HlsMediaDownloader::SetInitialBufferSize(int32_t offset, int32_t size)
{
    AutoLock lock(initCacheMutex_);
    bool isInitBufferSizeOk = IsCachedInitSizeReady(size) || CheckBreakCondition();
    if (isInitBufferSizeOk || !downloader_ || !downloadRequest_ || isTimeoutErrorNotified_.load()) {
        MEDIA_LOG_I("HLS SetInitialBufferSize initCacheSize ok.");
        return false;
    }
    MEDIA_LOG_I("HLS SetInitialBufferSize initCacheSize " PUBLIC_LOG_U32, size);
    if (!isBuffering_.load()) {
        isBuffering_.store(true);
    }
    bufferingTime_ = static_cast<size_t>(steadyClock_.ElapsedMilliseconds());
    expectOffset_.store(offset);
    initCacheSize_.store(size);
    return true;
}

void HlsMediaDownloader::SetPlayStrategy(const std::shared_ptr<PlayStrategy>& playStrategy)
{
    if (playlistDownloader_ == nullptr || playStrategy == nullptr) {
        MEDIA_LOG_E("SetPlayStrategy error.");
        return;
    }
    if (playStrategy->width > 0 && playStrategy->height > 0) {
        playlistDownloader_->SetInitResolution(playStrategy->width, playStrategy->height);
    }
    if (playStrategy->bufferDurationForPlaying > 0) {
        bufferDurationForPlaying_ = playStrategy->bufferDurationForPlaying;
        waterlineForPlaying_ = static_cast<uint64_t>(static_cast<double>(CURRENT_BIT_RATE) /
            static_cast<double>(BYTES_TO_BIT) * bufferDurationForPlaying_);
        MEDIA_LOG_I("HLS buffer duration for playing : " PUBLIC_LOG ".3f", bufferDurationForPlaying_);
    }
}

bool HlsMediaDownloader::IsNeedBufferForPlaying()
{
    if (bufferDurationForPlaying_ <= 0 || !isDemuxerInitSuccess_.load() || !isBuffering_.load()) {
        return false;
    }
    if (GetBufferingTimeOut()) {
        callback_->OnEvent({PluginEventType::CLIENT_ERROR, {NetworkClientErrorCode::ERROR_TIME_OUT},
                            "buffer for playing"});
        isTimeoutErrorNotified_.store(true);
        isBuffering_.store(false);
        isDemuxerInitSuccess_.store(false);
        bufferingTime_ = 0;
        return false;
    }
    if (GetCrossTsBuffersize() >= waterlineForPlaying_ || CheckBreakCondition() ||
        (tsStorageInfo_.find(readTsIndex_ + 1) != tsStorageInfo_.end() &&
        tsStorageInfo_[readTsIndex_ + 1].second)) {
        MEDIA_LOG_I("HLS buffer duration for playing is enough, buffersize: " PUBLIC_LOG_U64 " waterLineAbove: "
                    PUBLIC_LOG_U64, GetCrossTsBuffersize(), waterlineForPlaying_);
        isBuffering_.store(false);
        isDemuxerInitSuccess_.store(false);
        bufferingTime_ = 0;
        return false;
    }
    return true;
}

void HlsMediaDownloader::NotifyInitSuccess()
{
    MEDIA_LOG_I("HLS NotifyInitSuccess in");
    isDemuxerInitSuccess_.store(true);
    if (bufferDurationForPlaying_ <= 0) {
        return;
    }
    uint32_t playlistBitrate = 0;
    if (playlistDownloader_ != nullptr) {
        playlistBitrate = static_cast<uint32_t>(playlistDownloader_->GetCurBitrate());
    }
    if (playlistBitrate > 0) {
        waterlineForPlaying_ = static_cast<uint64_t>(static_cast<double>(playlistBitrate) /
            static_cast<double>(BYTES_TO_BIT) * bufferDurationForPlaying_);
    }
    isBuffering_.store(true);
    bufferingTime_ = static_cast<size_t>(steadyClock_.ElapsedMilliseconds());
}

uint64_t HlsMediaDownloader::GetCachedDuration()
{
    MEDIA_LOG_I("HLS GetCachedDuration: " PUBLIC_LOG_U64, cachedDuration_);
    return cachedDuration_;
}

bool HlsMediaDownloader::CheckLoopTimeout(int64_t loopStartTime)
{
    int64_t now = loopInterruptClock_.ElapsedSeconds();
    int64_t loopDuration = now > loopStartTime ? now - loopStartTime : 0;
    bool isLoopTimeout = loopDuration > LOOP_TIMEOUT;
    if (isLoopTimeout) {
        SetDownloadErrorState();
        MEDIA_LOG_E("loop timeout.");
    }
    return isLoopTimeout;
}

Status HlsMediaDownloader::GetStreamInfo(std::vector<StreamInfo>& streams)
{
    Seekable seekable = Seekable::INVALID;
    int retry = 0;
    do {
        seekable = GetSeekable();
        retry++;
        if (seekable == Seekable::INVALID) {
            if (retry >= MAX_RETRY) {
                break;
            }
        }
    } while (seekable == Seekable::INVALID && !isInterruptNeeded_.load());
    
    if (playlistDownloader_) {
        playlistDownloader_->GetStreamInfo(streams);
        MEDIA_LOG_I("HLS GetStreamInfo " PUBLIC_LOG_ZU, streams.size());
    }
    return Status::OK;
}

bool HlsMediaDownloader::IsHlsFmp4()
{
    if (playlistDownloader_) {
        return playlistDownloader_->IsHlsFmp4();
    }
    return false;
}

bool HlsMediaDownloader::IsPureByteRange()
{
    if (playlistDownloader_) {
        return playlistDownloader_->IsPureByteRange();
    }
    return false;
}

void HlsMediaDownloader::HandleSeekReady(int32_t streamType, int32_t streamId, int32_t isEos)
{
    Format seekReadyInfo {};
    seekReadyInfo.PutIntValue("currentStreamType", streamType);
    seekReadyInfo.PutIntValue("currentStreamId", streamId);
    seekReadyInfo.PutIntValue("isEOS", isEos);
    seekReadyInfo.PutLongValue("seekStartTimePos", seekStartTimePos_);
    MEDIA_LOG_D("StreamType: " PUBLIC_LOG_D32 " StreamId: " PUBLIC_LOG_D32 " isEOS: "
        PUBLIC_LOG_D32 " seekStartTimePos: " PUBLIC_LOG_D64,
        streamType, streamId, isEos, seekStartTimePos_);
    if (callback_ != nullptr) {
        callback_->OnEvent({PluginEventType::HLS_SEEK_READY, seekReadyInfo, "hls_seek_ready"});
    }
}

uint64_t HlsMediaDownloader::GetTotalTsBuffersize()
{
    FALSE_RETURN_V_MSG(cacheMediaBuffer_ != nullptr, 0, "cacheMediaBuffer_ is nullptr.");
    uint64_t totalBufferSize = 0;
    uint32_t tsIndex = readTsIndex_;
    uint64_t offset = readOffset_;
    while (tsIndex < writeTsIndex_) {
        uint64_t bufferSize = cacheMediaBuffer_->GetBufferSize(offset);
        uint32_t loopTimes = tsIndex > readOffset_ ? tsIndex - readOffset_ : 0;
        if (bufferSize == 0 || loopTimes > MAX_LOOP_TIMES) {
            break;
        }
        totalBufferSize += bufferSize;
        tsIndex++;
        offset = SpliceOffset(tsIndex, 0);
    }
    MEDIA_LOG_D("GetTotalTsBuffersize  readTsIndex_: " PUBLIC_LOG_U32 " tsIndex: "
        PUBLIC_LOG_U32 " offset: " PUBLIC_LOG_U64, readTsIndex_.load(), tsIndex, offset);
    return totalBufferSize;
}

uint64_t HlsMediaDownloader::GetMemorySize()
{
    return memorySize_;
}

bool HlsMediaDownloader::IsHlsEnd()
{
    if (CheckReadStatus() && GetBufferSize() == 0 && readTsIndex_ + 1 == backPlayList_.size() &&
        tsStorageInfo_.find(readTsIndex_) != tsStorageInfo_.end() &&
        tsStorageInfo_[readTsIndex_].second) {
            if (isEos_) {
                MEDIA_LOG_I("HLS download done.");
            }
            return true;
        }
    return false;
}
}
}
}
}