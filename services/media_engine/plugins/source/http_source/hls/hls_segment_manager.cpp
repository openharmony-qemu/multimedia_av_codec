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
#define HST_LOG_TAG "HlsSegmentManager"

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
constexpr int CACHE_BUFFER_FULL_LOOP_INTERVAL = 100;
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

std::map<HlsSegmentType, size_t> MIN_BUFFER_SIZE = {
    { HlsSegmentType::SEG_VIDEO, VIDEO_MIN_BUFFER_SIZE },
    { HlsSegmentType::SEG_AUDIO, 2 * 1024 * 1024 },
    { HlsSegmentType::SEG_SUBTITLE, 1 * 1024 * 1024 },
};

std::map<HlsSegmentType, size_t> MAX_CACHE_BUFFER_SIZE = {
    {HlsSegmentType::SEG_VIDEO, VIDEO_MAX_CACHE_BUFFER_SIZE},
    {HlsSegmentType::SEG_AUDIO, 5 * 1024 * 1024},
    {HlsSegmentType::SEG_SUBTITLE, 2 * 1024 * 1024},
};

uint64_t SpliceOffset(uint32_t tsIndex, uint32_t offset32)
{
    uint64_t offset64 = 0;
    offset64 = tsIndex;
    offset64 = (offset64 << 32); // 32
    offset64 |= offset32;
    return offset64;
}
}

//   hls manifest, m3u8 --- content get from m3u8 url, we get play list from the content
//   fragment --- one item in play list, download media data according to the fragment address.
HlsSegmentManager::HlsSegmentManager(int expectBufferDuration, bool userDefinedDuration,
    const std::map<std::string, std::string>& httpHeader, HlsSegmentType type,
    std::shared_ptr<MediaSourceLoaderCombinations> sourceLoader)
{
    SetType(type);
    if (userDefinedDuration) {
        userDefinedBufferDuration_ = userDefinedDuration;
        expectDuration_ = static_cast<uint64_t>(expectBufferDuration);
        expectDuration_ = std::min(expectDuration_, MAX_EXPECT_DURATION);
        totalBufferSize_ = expectDuration_ * CURRENT_BIT_RATE;
    } else {
        cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferHlsImpl>();
        cacheMediaBuffer_->Init(maxCacheBufferSize_, CHUNK_SIZE);
        isBuffering_ = true;
        bufferingTime_ = static_cast<size_t>(steadyClock_.ElapsedMilliseconds());
        totalBufferSize_ = maxCacheBufferSize_;
    }
    httpHeader_ = httpHeader;
    timeoutInterval_ = MAX_BUFFERING_TIME_OUT;
    if (sourceLoader != nullptr) {
        sourceLoader_ = sourceLoader;
        MEDIA_LOG_I("HLS app download, type: %{public}d", type_);
    }
    MEDIA_LOG_I("HLS constructor size: " PUBLIC_LOG_ZU " userDefinedDuration:" PUBLIC_LOG_D32 ", type: %{public}d",
        totalBufferSize_, userDefinedDuration, type_);
}

HlsSegmentManager::HlsSegmentManager(std::string mimeType, HlsSegmentType type,
    const std::map<std::string, std::string>& httpHeader)
{
    SetType(type);
    mimeType_ = mimeType;
    cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferHlsImpl>();
    httpHeader_ = httpHeader;
    timeoutInterval_ = MAX_BUFFERING_TIME_OUT;
    cacheMediaBuffer_->Init(maxCacheBufferSize_, CHUNK_SIZE);
    totalBufferSize_ = maxCacheBufferSize_;
    memorySize_ = maxCacheBufferSize_;
    MEDIA_LOG_I("HLS constructor size: " PUBLIC_LOG_ZU ", type: %{public}d", totalBufferSize_, type_);
}

HlsSegmentManager::HlsSegmentManager(const std::shared_ptr<HlsSegmentManager> &other,
    HlsSegmentType type)
{
    SetType(type);
    userDefinedBufferDuration_ = other->userDefinedBufferDuration_;

    if (userDefinedBufferDuration_) {
        expectDuration_ = other->expectDuration_;
        totalBufferSize_ = other->totalBufferSize_;
    } else {
        cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferHlsImpl>();
        cacheMediaBuffer_->Init(maxCacheBufferSize_, CHUNK_SIZE);
        isBuffering_.store(other->isBuffering_.load());
        bufferingTime_ = static_cast<size_t>(steadyClock_.ElapsedMilliseconds());
        totalBufferSize_ = maxCacheBufferSize_;
    }
    sourceLoader_ = other->sourceLoader_;
    mimeType_ = other->mimeType_;
    httpHeader_ = other->httpHeader_;
    timeoutInterval_ = MAX_BUFFERING_TIME_OUT;
    memorySize_ = maxCacheBufferSize_;
    MEDIA_LOG_I("HLS copy constructor size: " PUBLIC_LOG_ZU ", type: %{public}d", totalBufferSize_, type_);
}

void HlsSegmentManager::SetType(HlsSegmentType type)
{
    type_ = type;
    minBufferSize_ = MIN_BUFFER_SIZE[type_];
    maxCacheBufferSize_ = MAX_CACHE_BUFFER_SIZE[type_];
}

void HlsSegmentManager::Init()
{
    if (sourceLoader_ != nullptr) {
        downloader_ = std::make_shared<Downloader>("hlsMedia", sourceLoader_);
        MEDIA_LOG_I("HLS app download, type: %{public}d", type_);
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

HlsSegmentManager::~HlsSegmentManager()
{
    MEDIA_LOG_I("0x%{public}06" PRIXPTR " ~HlsSegmentManager dtor in, type: %{public}d", FAKE_POINTER(this), type_);
    if (downloader_ != nullptr) {
        downloader_->Stop(false);
    }
    cacheMediaBuffer_ = nullptr;
    if (playlistDownloader_ != nullptr) {
        playlistDownloader_ = nullptr;
    }
    NZERO_LOG(memset_s(iv_, 1, 0, DECRYPT_UNIT_LEN));
    NZERO_LOG(memset_s(initIv_, 1, 0, DECRYPT_UNIT_LEN));
    NZERO_LOG(memset_s(key_, 1, 0, DECRYPT_UNIT_LEN));
    MEDIA_LOG_I("0x%{public}06" PRIXPTR " ~HlsSegmentManager dtor out, type: %{public}d", FAKE_POINTER(this), type_);
}

std::string HlsSegmentManager::GetContentType()
{
    FALSE_RETURN_V(downloader_ != nullptr, "");
    MEDIA_LOG_I("GetContentType type: %{public}d", type_);
    return downloader_->GetContentType();
}

void HlsSegmentManager::SetDownloaderRequestCb(
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

void HlsSegmentManager::PutRequestIntoDownloader(const PlayInfo& playInfo)
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
        MEDIA_LOG_I("HLS PutRequestIntoDownloader init readOffset." PUBLIC_LOG_U64 ", type: %{public}d", readOffset_,
            type_);
        readTsIndex_ = writeTsIndex_;
        uint32_t readTsIndexTempValue = readTsIndex_.load();
        MEDIA_LOG_I("readTsIndex_, PutRequestIntoDownloader init readTsIndex_." PUBLIC_LOG_U32 ", type: %{public}d",
            readTsIndexTempValue, type_);
    }
    writeOffset_ = SpliceOffset(writeTsIndex_, 0);
    MEDIA_LOG_I("HLS PutRequestIntoDwonloader update writeOffset_: " PUBLIC_LOG_U64 " writeTsIndex_: " PUBLIC_LOG_U32
        ", type: %{public}d", writeOffset_, writeTsIndex_, type_);
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

void HlsSegmentManager::SaveHttpHeader(const std::map<std::string, std::string>& httpHeader)
{
    httpHeader_ = httpHeader;
}

bool HlsSegmentManager::Open(const std::string& url, const std::map<std::string, std::string>& httpHeader)
{
    FALSE_RETURN_V_MSG(playlistDownloader_ != nullptr, false, "Open no playlistDownloader_");
    isDownloadFinish_ = false;
    SaveHttpHeader(httpHeader);
    writeBitrateCaculator_->StartClock();
    playlistDownloader_->SetPlayListCallback(std::weak_ptr<PlayListChangeCallback>(weak_from_this()));
    playlistDownloader_->SetMimeType(mimeType_);
    playlistDownloader_->Open(url, httpHeader);
    steadyClock_.Reset();
    openTime_ = steadyClock_.ElapsedMilliseconds();
    InitCacheWithDuration();
    return true;
}

void HlsSegmentManager::InitCacheWithDuration()
{
    if (!userDefinedBufferDuration_) {
        return;
    }
    MEDIA_LOG_I("HLS setting buffer size: %{public}zu, min: %{public}zu, max: %{public}zu, type: %{public}d",
        totalBufferSize_, minBufferSize_, maxCacheBufferSize_, type_);
    size_t expectBufferSize = expectDuration_ * CURRENT_BIT_RATE;
    totalBufferSize_ = std::max(std::min(expectBufferSize, maxCacheBufferSize_), minBufferSize_);
    cacheMediaBuffer_ = std::make_shared<CacheMediaChunkBufferHlsImpl>();
    cacheMediaBuffer_->Init(totalBufferSize_, CHUNK_SIZE);
    memorySize_ = totalBufferSize_;
}

void HlsSegmentManager::Close(bool isAsync)
{
    MEDIA_LOG_I("0x%{public}06" PRIXPTR " HLS Close enter, type: %{public}d", FAKE_POINTER(this), type_);
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
        MEDIA_LOG_D("HLS Download close, average download speed: " PUBLIC_LOG_D32 " bit/s, type: %{public}d",
            avgDownloadSpeed_, type_);
        int64_t nowTime = steadyClock_.ElapsedMilliseconds();
        auto downloadTime = nowTime - startDownloadTime_;
        MEDIA_LOG_D("HLS Download close, Data usage: " PUBLIC_LOG_U64 " bits in " PUBLIC_LOG_D64 "ms, type: %{public}d",
            totalBits_, downloadTime, type_);
    }
}

void HlsSegmentManager::Pause()
{
    FALSE_RETURN_MSG(playlistDownloader_ != nullptr, "Pause no playlistDownloader_");
    MEDIA_LOG_I("HLS Pause enter, type: %{public}d", type_);
    playlistDownloader_->Pause();
}

void HlsSegmentManager::Resume()
{
    FALSE_RETURN_MSG(playlistDownloader_ != nullptr, "Resume no playlistDownloader_");
    MEDIA_LOG_I("HLS Resume enter, type: %{public}d", type_);
    playlistDownloader_->Resume();
}

bool HlsSegmentManager::CheckReadStatus()
{
    // eos:palylist is empty, request is finished, hls is vod, do not select bitrate
    FALSE_RETURN_V(playlistDownloader_ != nullptr, false);
    FALSE_RETURN_V(downloadRequest_ != nullptr, false);
    isEos_ = playList_->Empty() && (downloadRequest_ != nullptr) && downloadRequest_->IsEos() &&
        playlistDownloader_ != nullptr && (playlistDownloader_->GetDuration() > 0) &&
        playlistDownloader_->IsParseAndNotifyFinished();
    if (isEos_) {
        return true;
    }
    if (playlistDownloader_->GetDuration() > 0 && playlistDownloader_->IsParseAndNotifyFinished() &&
        seekTime_ >= static_cast<uint64_t>(playlistDownloader_->GetDuration())) {
        MEDIA_LOG_I("HLS seek to tail, type: %{public}d", type_);
        return true;
    }
    return false;
}

bool HlsSegmentManager::CheckBreakCondition()
{
    if (downloadErrorState_) {
        MEDIA_LOG_I("HLS downloadErrorState break, type: %{public}d", type_);
        return true;
    }
    if (CheckReadStatus()) {
        MEDIA_LOG_I("HLS download complete break, type: %{public}d", type_);
        return true;
    }
    return false;
}

bool HlsSegmentManager::HandleBuffering()
{
    if (HandleTimeout()) {
        return false;
    }
    if (IsNeedBufferForPlaying() || !isBuffering_.load()) {
        return false;
    }
    if (isFirstFrameArrived_) {
        UpdateWaterLineAbove();
        UpdateCachedPercent(BufferingInfoType::BUFFERING_PERCENT);
    }
    HandleBufferingState();
    HandleBufferingEnd();
    return isBuffering_.load();
}

bool HlsSegmentManager::HandleTimeout()
{
    if (!isBuffering_ || !GetBufferingTimeOut() || !segEventCb_) {
        return false;
    }
    MEDIA_LOG_I("HTTP cachebuffer buffering time out, type: %{public}d", type_);
    HlsSegEvent event {.segType = type_, .type = PluginEventType::CLIENT_ERROR,
        .networkError = NetworkClientErrorCode::ERROR_TIME_OUT, .str = "buffering"};
    segEventCb_(event);
    isTimeoutErrorNotified_.store(true);
    isBuffering_ = false;
    return true;
}

void HlsSegmentManager::HandleBufferingState()
{
    {
        AutoLock lk(bufferingEndMutex_);
        if (!canWrite_) {
            MEDIA_LOG_I("HLS canWrite false, type: %{public}d", type_);
        }
        {
            AutoLock lock(tsStorageInfoMutex_);
            if (tsStorageInfo_.find(readTsIndex_) != tsStorageInfo_.end() &&
                tsStorageInfo_[readTsIndex_].second &&
                !backPlayList_.empty() && readTsIndex_ >= backPlayList_.size() - 1) {
                MEDIA_LOG_I("HLS readTS download complete, type: %{public}d", type_);
                isBuffering_ = false;
            }
        }
        HandleWaterLine();
    }
}

void HlsSegmentManager::HandleBufferingEnd()
{
    if (!isBuffering_ && !isFirstFrameArrived_) {
        bufferingTime_ = 0;
    }
    if (!isBuffering_ && isFirstFrameArrived_ && bufferingCbFunc_) {
        MEDIA_LOG_I("HLS CacheData onEvent BUFFERING_END, waterLineAbove: " PUBLIC_LOG_ZU " readOffset: "
            PUBLIC_LOG_U64 " writeOffset: " PUBLIC_LOG_U64 " writeTsIndex: " PUBLIC_LOG_U32 " bufferSize: "
            PUBLIC_LOG_U64 ", type: %{public}d", waterLineAbove_, readOffset_, writeOffset_, writeTsIndex_,
            GetCrossTsBuffersize(), type_);
        UpdateCachedPercent(BufferingInfoType::BUFFERING_END);
        bufferingCbFunc_(type_, BufferingInfoType::BUFFERING_END);
        bufferingTime_ = 0;
    }
}

void HlsSegmentManager::HandleWaterLine()
{
    size_t currentWaterLine = waterLineAbove_;
    size_t currentOffset = readOffset_;
    {
        AutoLock lock(initCacheMutex_);
        if (initCacheSize_.load() != -1) {
            currentWaterLine = static_cast<size_t>(initCacheSize_.load());
            currentOffset = static_cast<size_t>(expectOffset_.load());
            MEDIA_LOG_I("currentOffset:" PUBLIC_LOG_ZU ", type: %{public}d", currentOffset, type_);
        }
        if (GetCrossTsBuffersize() >= currentWaterLine || CheckBreakCondition() ||
            (tsStorageInfo_.find(readTsIndex_ + 1) != tsStorageInfo_.end() &&
                tsStorageInfo_[readTsIndex_ + 1].second)) {
            MEDIA_LOG_I("HLS CheckBreakCondition true, waterLineAbove: " PUBLIC_LOG_ZU " bufferSize: " PUBLIC_LOG_U64
                ", type: %{public}d", waterLineAbove_, GetCrossTsBuffersize(), type_);
            if (initCacheSize_.load() != -1) {
                initCacheSize_.store(-1);
                expectOffset_.store(-1);
                if (bufferingCbFunc_) {
                    bufferingCbFunc_(type_, BufferingInfoType::BUFFERING_END);
                }
            }
            isBuffering_ = false;
        }
    }
    if (!isBuffering_) {
        MEDIA_LOG_I("HandleBuffering bufferingEndCond NotifyAll, type: %{public}d", type_);
        bufferingEndCond_.NotifyAll();
    }
}

bool HlsSegmentManager::HandleCache()
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
    if (!backPlayList_.empty() && readTsIndex_ + 1 >= backPlayList_.size()) {
        isNextTsReady = tsStorageInfo_[readTsIndex_].second;
    } else {
        isNextTsReady = tsStorageInfo_[readTsIndex_ + 1].second;
    }
    if (isBuffering_ || !bufferingCbFunc_ || !canWrite_ || isAboveLine || isNextTsReady) {
        return false;
    }
    isBuffering_ = true;
    bufferingTime_ = static_cast<size_t>(steadyClock_.ElapsedMilliseconds());
    if (isFirstFrameArrived_) {
        bufferingCbFunc_(type_, BufferingInfoType::BUFFERING_START);
        UpdateCachedPercent(BufferingInfoType::BUFFERING_START);
        MEDIA_LOG_I("HLS CacheData onEvent BUFFERING_START, waterLineAbove: " PUBLIC_LOG_ZU " readOffset: "
            PUBLIC_LOG_U64 " writeOffset: " PUBLIC_LOG_U64 " writeTsIndex: " PUBLIC_LOG_U32 " bufferSize: "
            PUBLIC_LOG_U64 ", type: %{public}d", waterLineAbove_, readOffset_, writeOffset_, writeTsIndex_,
            GetCrossTsBuffersize(), type_);
    }
    return true;
}

void HlsSegmentManager::HandleFfmpegReadback(uint64_t ffmpegOffset)
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
    MEDIA_LOG_D("HLS Read back, ffmpegOffset: " PUBLIC_LOG_U64 " ffmpegOffset: " PUBLIC_LOG_U64 ", type: %{public}d",
        ffmpegOffset_, ffmpegOffset, type_);
    uint64_t readBack = ffmpegOffset_ - ffmpegOffset;
    uint64_t curTsHaveRead = readOffset_ > SpliceOffset(readTsIndex_, 0) ?
        readOffset_ - SpliceOffset(readTsIndex_, 0) : 0;
    AutoLock lock(tsStorageInfoMutex_);
    if (curTsHaveRead >= readBack) {
        readOffset_ -= readBack;
        MEDIA_LOG_D("HLS Read back, current ts, update readOffset: " PUBLIC_LOG_U64 ", type: %{public}d", readOffset_,
            type_);
    } else {
        if (readTsIndex_ == 0) {
            readOffset_ = 0; // Cross ts readback, but this is the first ts, so reset readOffset.
            MEDIA_LOG_W("HLS Read back, this is the first ts: " PUBLIC_LOG_U64 ", type: %{public}d", readOffset_,
                type_);
            return;
        }
        if (tsStorageInfo_.find(readTsIndex_ - 1) == tsStorageInfo_.end()) {
            readOffset_ = readOffset_ > curTsHaveRead ? readOffset_ - curTsHaveRead : 0;
            MEDIA_LOG_W("HLS Read back, last ts is not ready, update readOffset to readTsIndex head: "
                PUBLIC_LOG_U64 ", type: %{public}d", readOffset_, type_);
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
                " update readOffset: " PUBLIC_LOG_U64 ", type: %{public}d", readTsIndexTempValue, readOffset_, type_);
        }
    }
}

bool HlsSegmentManager::CheckDataIntegrity()
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

Status HlsSegmentManager::CheckPlaylist(unsigned char* buff, ReadDataInfo& readDataInfo)
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
            PUBLIC_LOG_D32 " readOffset_ " PUBLIC_LOG_U64 " readTsIndex_ " PUBLIC_LOG_U32 ", type: %{public}d",
            readDataInfo.wantReadLength_, readDataInfo.realReadLength_, readDataInfo.isEos_, readOffset_,
            readTsIndexTempValue, type_);
        return Status::OK;
    }
    if (isFinishedPlay && GetBufferSize() == 0 && GetSeekable() == Seekable::SEEKABLE &&
        tsStorageInfo_.find(writeTsIndex_) != tsStorageInfo_.end() &&
        tsStorageInfo_[writeTsIndex_].second) {
        readDataInfo.realReadLength_ = 0;
        MEDIA_LOG_I("HLS CheckPlaylist, eos, type: %{public}d", type_);
        return Status::END_OF_STREAM;
    }
    return Status::ERROR_UNKNOWN;
}

Status HlsSegmentManager::ReadDelegate(unsigned char* buff, ReadDataInfo& readDataInfo)
{
    FALSE_RETURN_V_MSG(cacheMediaBuffer_ != nullptr, Status::END_OF_STREAM, "eos, cacheMediaBuffer_ is nullptr");
    FALSE_RETURN_V_MSG(!isInterruptNeeded_.load(), Status::END_OF_STREAM, "eos, isInterruptNeeded");
    MediaAVCodec::AVCodecTrace trace("HLS ReadDelegate, expectedLen: " +
        std::to_string(readDataInfo.wantReadLength_) + ", bufferSize: " + std::to_string(GetBufferSize()));
    MEDIA_LOG_D("HLS Read in: wantReadLength " PUBLIC_LOG_D32 ", readOffset: " PUBLIC_LOG_U64 " readTsIndex: "
        PUBLIC_LOG_U32 " bufferSize: " PUBLIC_LOG_U64 ", type: %{public}d", readDataInfo.wantReadLength_, readOffset_,
        readTsIndex_.load(), GetCrossTsBuffersize(), type_);
    if (CheckTsEndOrEos(readDataInfo)) {
        MEDIA_LOG_D("HLS CheckTsEndOrEos return END_OF_STREAM, type: %{public}d", type_);
        return Status::END_OF_STREAM;
    }
    if (isBuffering_ && GetBufferingTimeOut() && segEventCb_ && !isReportedErrorCode_) {
        HlsSegEvent event {.segType = type_, .type = PluginEventType::CLIENT_ERROR,
            .networkError = NetworkClientErrorCode::ERROR_TIME_OUT, .str = "read"};
        segEventCb_(event);
        isTimeoutErrorNotified_.store(true);
        MEDIA_LOG_D("HLS CheckBufferingTimeOut return END_OF_STREAM, type: %{public}d", type_);
        return Status::END_OF_STREAM;
    }
    if (isBuffering_ && CheckBufferingOneSeconds()) {
        MEDIA_LOG_I("HLS read CheckBuffering return error again, type: %{public}d", type_);
        return Status::ERROR_AGAIN;
    }
    wantedReadLength_ = static_cast<size_t>(readDataInfo.wantReadLength_);
    if (!CheckBreakCondition() && HandleCache()) {
        MEDIA_LOG_D("HLS read CheckBreak or HandleCache return error again, type: %{public}d", type_);
        return Status::ERROR_AGAIN;
    }
    Status tmpRes = CheckPlaylist(buff, readDataInfo);
    if (tmpRes != Status::ERROR_UNKNOWN) {
        MEDIA_LOG_D("HLS read return tmpRes, type: %{public}d", type_);
        return tmpRes;
    }
    FALSE_RETURN_V_MSG(readDataInfo.wantReadLength_ > 0, Status::END_OF_STREAM, "eos, wantReadLength_ <= 0");
    ReadCacheBuffer(buff, readDataInfo);
    MEDIA_LOG_D("HLS Read success: wantReadLength " PUBLIC_LOG_D32 " realReadLen: " PUBLIC_LOG_D32 " readOffset: "
        PUBLIC_LOG_U64 " readTsIndex: " PUBLIC_LOG_U32 " bufferSize: " PUBLIC_LOG_U64 ", type: %{public}d",
        readDataInfo.wantReadLength_, readDataInfo.realReadLength_, readOffset_, readTsIndex_.load(),
        GetCrossTsBuffersize(), type_);
    return Status::OK;
}

bool HlsSegmentManager::CheckTsEndOrEos(ReadDataInfo& readDataInfo)
{
    if (isTsEnd_.load()) {
        MEDIA_LOG_I("HLS READ TS END, type: %{public}d", type_);
        isTsEnd_.store(false);
        return true;
    }
    readDataInfo.isEos_ = CheckReadStatus();
    if (readDataInfo.isEos_ && GetBufferSize() == 0 && readTsIndex_ + 1 == backPlayList_.size() &&
        tsStorageInfo_.find(readTsIndex_) != tsStorageInfo_.end() &&
        tsStorageInfo_[readTsIndex_].second) {
        readDataInfo.realReadLength_ = 0;
        MEDIA_LOG_I("HLS buffer is empty, eos, type: %{public}d", type_);
        return true;
    }
    return false;
}

bool HlsSegmentManager::ReadHeaderData(unsigned char* buff, ReadDataInfo& readDataInfo)
{
    if (playlistDownloader_ == nullptr || !playlistDownloader_->IsHlsFmp4()) {
        return false;
    }
    if (curStreamId_ <= 0 && readDataInfo.streamId_ > 0) {
        curStreamId_ = static_cast<uint32_t>(readDataInfo.streamId_);
        isNeedReadHeader_.store(true);
        MEDIA_LOG_D("HLS read curStreamId_ " PUBLIC_LOG_U32 ", type: %{public}d", curStreamId_, type_);
    } else if (readDataInfo.streamId_ > 0 && readDataInfo.streamId_ != static_cast<int32_t>(curStreamId_)) {
        readDataInfo.nextStreamId_ = static_cast<int32_t>(curStreamId_);
        isNeedReadHeader_.store(true);
        MEDIA_LOG_I("HLS read curStreamId_ " PUBLIC_LOG_U32 " curStreamId_ " PUBLIC_LOG_U32 ", type: %{public}d",
                    curStreamId_, readDataInfo.streamId_, type_);
        return true;
    }
    if (readDataInfo.streamId_ > 0 && curStreamId_ == static_cast<uint32_t>(readDataInfo.streamId_) &&
        isNeedReadHeader_.load()) {
        auto headerRet = playlistDownloader_->ReadFmp4Header(buff, readDataInfo.wantReadLength_,
            readDataInfo.realReadLength_, readDataInfo.streamId_);
        FALSE_RETURN_V_MSG(headerRet, true, "HLS read fmp4 header failed, type: %{public}d", type_);
        isNeedReadHeader_.store(false);
        MEDIA_LOG_I("HLS read fmp4 header, type: %{public}d", type_);
        return true;
    }
    return false;
}

void HlsSegmentManager::ReadCacheBuffer(unsigned char* buff, ReadDataInfo& readDataInfo)
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
                MEDIA_LOG_D("HLS read readTsIndex_ " PUBLIC_LOG_U32 ", type: %{public}d", readTsIndex_.load(), type_);
                return;
            }
        }
    }
    canWrite_ = true;
}

void HlsSegmentManager::RemoveFmp4PaddingData(unsigned char* buff, ReadDataInfo& readDataInfo)
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

Status HlsSegmentManager::Read(unsigned char* buff, ReadDataInfo& readDataInfo)
{
    FALSE_RETURN_V(cacheMediaBuffer_ != nullptr, Status::ERROR_AGAIN);
    uint64_t now = static_cast<uint64_t>(steadyClock_.ElapsedMilliseconds());
    readTime_ = now;
    if (isBuffering_ && !canWrite_) {
        MEDIA_LOG_I("HLS can not write and buffering, type: %{public}d", type_);
        SeekToTsForRead(readTsIndex_);
        return Status::ERROR_AGAIN;
    }
    if (!CheckDataIntegrity()) {
        MEDIA_LOG_W("HLS Read in, fix download, type: %{public}d", type_);
        SeekToTsForRead(readTsIndex_);
        return Status::ERROR_AGAIN;
    }

    HandleFfmpegReadback(readDataInfo.ffmpegOffset);

    auto ret = ReadDelegate(buff, readDataInfo);

    uint64_t freeSize = cacheMediaBuffer_->GetFreeSize();
    if (freeSize > RESUME_FREE_SIZE_THRESHOLD && isNeedResume_.load()) {
        downloader_->Resume();
        isNeedResume_.store(false);
        MEDIA_LOG_D("HLS downloader resume, type: %{public}d", type_);
    }

    readTotalBytes_ += readDataInfo.realReadLength_;
    if (now > lastReadCheckTime_ && now - lastReadCheckTime_ > SAMPLE_INTERVAL) {
        readRecordDuringTime_ = now - lastReadCheckTime_;
        double readDuration = static_cast<double>(readRecordDuringTime_) / SECOND_TO_MILLISECONDS;
        if (readDuration > ZERO_THRESHOLD) {
            double readSpeed = readTotalBytes_ * BYTES_TO_BIT / readDuration;    // bps
            readBitrate_ = readSpeed > 0 ? static_cast<uint64_t>(readSpeed) : readBitrate_;
            MEDIA_LOG_D("Current read speed: " PUBLIC_LOG_D32 " Kbit/s,Current buffer size: " PUBLIC_LOG_U64 " KByte"
                ", type: %{public}d", static_cast<int32_t>(readSpeed / KILO),
                static_cast<uint64_t>(GetBufferSize() / KILO), type_);
            MediaAVCodec::AVCodecTrace trace("HlsSegmentManager::Read, read speed: " +
                std::to_string(readSpeed) + " bit/s, bufferSize: " + std::to_string(GetBufferSize()) + " Byte");
            readTotalBytes_ = 0;
        }
        lastReadCheckTime_ = now;
        readRecordDuringTime_ = 0;
    }
    return ret;
}

void HlsSegmentManager::PrepareToSeek()
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
            MEDIA_LOG_I("HLS Seek may be failed, type: %{public}d", type_);
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
    memset_s(decryptCache_, minBufferSize_, 0x00, minBufferSize_);
    memset_s(decryptBuffer_, minBufferSize_, 0x00, minBufferSize_);
    auto ret = memcpy_s(iv_, DECRYPT_UNIT_LEN, initIv_, DECRYPT_UNIT_LEN);
    if (ret != 0) {
        MEDIA_LOG_E("iv copy error, type: %{public}d", type_);
    }
    afterAlignRemainedLength_ = 0;
    isLastDecryptWriteError_ = false;
}

bool HlsSegmentManager::SeekToTime(int64_t seekTime, SeekMode mode)
{
    MEDIA_LOG_I("HLS Seek: buffer size " PUBLIC_LOG_U64 ", seekTime " PUBLIC_LOG_D64 " mode: " PUBLIC_LOG_D32
        ", type: %{public}d", GetBufferSize(), seekTime, mode, type_);
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
    auto totalDuration = playlistDownloader_->GetDuration();
    FALSE_RETURN_V(totalDuration != -1, false);
    if (seekTime_ < static_cast<uint64_t>(totalDuration)) {
        if (playlistDownloader_->IsHlsFmp4()) {
            isNeedReadHeader_.store(true);
        }
        SeekToTs(seekTime, mode);
    } else {
        readTsIndex_ = !backPlayList_.empty() ? backPlayList_.size() - 1 : 0; // 0
        tsStorageInfo_[readTsIndex_].second = true;
    }
    HandleSeekReady(curStreamId_, CheckBreakCondition());
    MEDIA_LOG_I("HLS SeekToTime end, type: %{public}d", type_);
    isSeekingFlag = false;
    notNeedReadBack_.store(false);
    return true;
}

size_t HlsSegmentManager::GetContentLength() const
{
    return 0;
}

int64_t HlsSegmentManager::GetDuration() const
{
    FALSE_RETURN_V(playlistDownloader_ != nullptr, 0);
    MEDIA_LOG_I("HLS GetDuration " PUBLIC_LOG_D64 ", type: %{public}d", playlistDownloader_->GetDuration(), type_);
    return playlistDownloader_->GetDuration();
}

Seekable HlsSegmentManager::GetSeekable() const
{
    FALSE_RETURN_V(playlistDownloader_ != nullptr, Seekable::INVALID);
    return playlistDownloader_->GetSeekable();
}

void HlsSegmentManager::SetCallback(Callback* cb)
{
    FALSE_RETURN(playlistDownloader_ != nullptr);
    callback_ = cb;
    playlistDownloader_->SetCallback(cb);
}

void HlsSegmentManager::ResetPlaylistCapacity(size_t size)
{
    size_t remainCapacity = playList_->Capacity() - playList_->Size();
    if (remainCapacity >= size) {
        return;
    }
    size_t newCapacity = playList_->Size() + size;
    playList_->ResetCapacity(newCapacity);
}

void HlsSegmentManager::PlaylistBackup(const PlayInfo& fragment)
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

void HlsSegmentManager::OnPlayListChanged(const std::vector<PlayInfo>& playList)
{
    ResetPlaylistCapacity(static_cast<size_t>(playList.size()));
    int64_t loopStartTime = loopInterruptClock_.ElapsedSeconds();
    for (uint32_t i = 0; i < static_cast<uint32_t>(playList.size()); i++) {
        if (CheckLoopTimeout(loopStartTime)) {
            break;
        }
        if (isInterruptNeeded_.load()) {
            MEDIA_LOG_I("HLS OnPlayListChanged isInterruptNeeded, type: %{public}d", type_);
            break;
        }
        auto fragment = playList[i];
        PlaylistBackup(fragment);
        if (isSelectingBitrate_ && (GetSeekable() == Seekable::SEEKABLE)) {
            fragmentDownloadStart[fragment.url_] = true;
            if (writeTsIndex_ == i) {
                MEDIA_LOG_I("HLS Switch bitrate, type: %{public}d", type_);
                isSelectingBitrate_ = false;
            } else {
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
    if (!isDownloadStarted_ && playList_->Empty()) {
        MEDIA_LOG_I("HLS OnPlayListChanged no new playinfo, type: %{public}d", type_);
        HandleBuffering();
    }
}

bool HlsSegmentManager::GetStartedStatus()
{
    FALSE_RETURN_V(playlistDownloader_ != nullptr, false);
    return playlistDownloader_->GetPlayListDownloadStatus() && startedPlayStatus_;
}

bool HlsSegmentManager::CacheBufferFullLoop()
{
    {
        AutoLock lock(initCacheMutex_);
        if (initCacheSize_.load() != -1) {
            if (IsCachedInitSizeReady(initCacheSize_.load()) && bufferingCbFunc_) {
                bufferingCbFunc_(type_, BufferingInfoType::BUFFERING_END);
                initCacheSize_.store(-1);
                expectOffset_.store(-1);
            }
        }
    }

    MEDIA_LOGI_LIMIT(SAVE_DATA_LOG_FREQUENCY, "HLS CacheMediaBuffer full, waiting seek or read");
    if (isSeekingFlag.load()) {
        MEDIA_LOG_I("HLS CacheMediaBuffer full, isSeeking, return true, type: %{public}d", type_);
        return true;
    }
    OSAL::SleepFor(CACHE_BUFFER_FULL_LOOP_INTERVAL);
    return false;
}

uint32_t HlsSegmentManager::SaveCacheBufferDataNotblock(uint8_t* data, uint32_t len)
{
    uint64_t freeSize = cacheMediaBuffer_->GetFreeSize();
    if (freeSize <= (len + STORP_WRITE_BUFFER_REDUNDANCY) && !isNeedResume_.load()) {
        isNeedResume_.store(true);
        MEDIA_LOG_I("HLS stop write, freeSize: " PUBLIC_LOG_U64 " len: " PUBLIC_LOG_U32 ", type: %{public}d",
            freeSize, len, type_);
        return 0;
    }

    size_t res = cacheMediaBuffer_->Write(data, writeOffset_, len);
    writeOffset_ += res;
    MEDIA_LOGI_LIMIT(SAVE_DATA_LOG_FREQUENCY, "SaveCacheBufferDataNotblock writeOffset " PUBLIC_LOG_U64 " res "
        PUBLIC_LOG_ZU " len: " PUBLIC_LOG_U32 ", type: %{public}d", writeOffset_, res, len, type_);
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

void HlsSegmentManager::HandleSaveDataLoopContinue()
{
    HandleCachedDuration();
    writeBitrateCaculator_->StartClock();
    uint64_t writeTime = writeBitrateCaculator_->GetWriteTime() / SECOND_TO_MILLISECONDS;
    if (writeTime > ONE_SECONDS) {
        writeBitrateCaculator_->ResetClock();
    }
}

uint32_t HlsSegmentManager::SaveCacheBufferData(uint8_t* data, uint32_t len, bool notBlock)
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
        MEDIA_LOGI_LIMIT(SAVE_DATA_LOG_FREQUENCY, "writeOffset " PUBLIC_LOG_U64 " res " PUBLIC_LOG_ZU
            ", type: %{public}d", writeOffset_, res, type_);
        ClearChunksOfFragment();
        if (res > 0 || hasWriteSize == len) {
            HandleSaveDataLoopContinue();
            continue;
        }
        writeBitrateCaculator_->StopClock();
        MEDIA_LOG_W("HLS CacheMediaBuffer full, type: %{public}d", type_);
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
        MEDIA_LOG_I("HLS isInterruptNeeded true, return false, type: %{public}d", type_);
        return 0;
    }
    return hasWriteSize;
}

uint32_t HlsSegmentManager::SaveData(uint8_t* data, uint32_t len, bool notBlock)
{
    if (cacheMediaBuffer_ == nullptr || data == nullptr) {
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
    MEDIA_LOG_D("HLS free size: " PUBLIC_LOG_U64 ", type: %{public}d", freeSize, type_);
    return res;
}

uint32_t HlsSegmentManager::GetDecrptyRealLen(uint8_t* writeDataPoint, uint32_t waitLen, uint32_t writeLen)
{
    uint32_t realLen;
    errno_t err {0};
    if (afterAlignRemainedLength_ > 0) {
        err = memcpy_s(decryptBuffer_, afterAlignRemainedLength_,
                       afterAlignRemainedBuffer_, afterAlignRemainedLength_);
        if (err != 0) {
            MEDIA_LOG_D("afterAlignRemainedLength_: " PUBLIC_LOG_D64 ", type: %{public}d", afterAlignRemainedLength_,
                type_);
        }
    }
    uint32_t minWriteLen = (minBufferSize_ - afterAlignRemainedLength_) > writeLen ?
                            writeLen : minBufferSize_ - afterAlignRemainedLength_;
    if (minWriteLen > minBufferSize_) {
        return 0;
    }
    err = memcpy_s(decryptBuffer_ + afterAlignRemainedLength_,
                   minWriteLen, writeDataPoint, minWriteLen);
    if (err != 0) {
        MEDIA_LOG_D("minWriteLen: " PUBLIC_LOG_D32 ", type: %{public}d", minWriteLen, type_);
    }
    realLen = minWriteLen + afterAlignRemainedLength_;
    AES_cbc_encrypt(decryptBuffer_, decryptCache_, realLen, &aesKey_, iv_, AES_DECRYPT);
    return realLen;
}

uint32_t HlsSegmentManager::SaveEncryptData(uint8_t* data, uint32_t len, bool notBlock)
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
            MEDIA_LOG_E("afterAlignRemainedLength_: " PUBLIC_LOG_D64 ", type: %{public}d",
                DECRYPT_UNIT_LEN - afterAlignRemainedLength_, type_);
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
        MEDIA_LOG_E("realLen: " PUBLIC_LOG_D32 ", type: %{public}d", realLen, type_);
    }
    if (writeSuccessLen == realLen) {
        afterAlignRemainedLength_ = 0;
        err = memset_s(afterAlignRemainedBuffer_, DECRYPT_UNIT_LEN, 0x00, DECRYPT_UNIT_LEN);
        if (err != 0) {
            MEDIA_LOG_E("DECRYPT_UNIT_LEN: " PUBLIC_LOG_D64 ", type: %{public}d", DECRYPT_UNIT_LEN, type_);
        }
    }
    if (writeLen > len) {
        MEDIA_LOG_D("writeLen: " PUBLIC_LOG_D32 ", type: %{public}d", writeLen, type_);
    }
    writeDataPoint += writeLen;
    waitLen -= writeLen;
    if (waitLen > 0 && writeSuccessLen == realLen) {
        afterAlignRemainedLength_ = waitLen;
        err = memcpy_s(afterAlignRemainedBuffer_, DECRYPT_UNIT_LEN, writeDataPoint, waitLen);
        if (err != 0) {
            MEDIA_LOG_D("waitLen: " PUBLIC_LOG_D32 ", type: %{public}d", waitLen, type_);
        }
    }
    MEDIA_LOG_D("SaveEncryptData, return len " PUBLIC_LOG_D32 ", type: %{public}d", len, type_);
    return writeSuccessLen == realLen ? len : writeSuccessLen;
}

void HlsSegmentManager::DownloadRecordHistory(int64_t nowTime)
{
    if ((static_cast<uint64_t>(nowTime) - lastWriteTime_) < SAMPLE_INTERVAL) {
        return;
    }
    MEDIA_LOG_I("HLS OnWriteRingBuffer nowTime: " PUBLIC_LOG_D64
        " lastWriteTime:" PUBLIC_LOG_D64  ", type: %{public}d", nowTime, lastWriteTime_, type_);
    lastWriteBit_ = 0;
    lastWriteTime_ = static_cast<uint64_t>(nowTime);
    if (!autoBufferSize_ || userDefinedBufferDuration_) {
        return;
    }
    if (CheckRiseBufferSize()) {
        RiseBufferSize();
    } else if (CheckPulldownBufferSize()) {
        DownBufferSize();
    } else {
        MEDIA_LOG_D("DownloadRecordHistory, no rise, no down, type: %{public}d", type_);
    }
}

void HlsSegmentManager::OnWriteCacheBuffer(uint32_t len)
{
    {
        AutoLock lock(initCacheMutex_);
        if (initCacheSize_.load() != -1) {
            if (IsCachedInitSizeReady(initCacheSize_.load()) && bufferingCbFunc_) {
                bufferingCbFunc_(type_, BufferingInfoType::BUFFERING_END);
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
        MEDIA_LOG_D("Start play delay time: " PUBLIC_LOG_D64 ", type: %{public}d", playDelayTime_, type_);
    }
    DownloadRecordHistory(nowTime);
    DownloadReport();
}

double HlsSegmentManager::CalculateCurrentDownloadSpeed()
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

void HlsSegmentManager::DownloadReport()
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
                " KByte, type: %{public}d", static_cast<int32_t>(downloadRate / KILO),
                static_cast<uint64_t>(remainingBuffer / KILO), type_);
            MediaAVCodec::AVCodecTrace trace("HlsSegmentManager::DownloadReport, download speed: " +
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
        MEDIA_LOG_D("Data usage: " PUBLIC_LOG_U64 " bits in " PUBLIC_LOG_D32 "ms, type: %{public}d", dataUsage_,
            DATA_USAGE_INTERVAL, type_);
        dataUsage_ = 0;
        lastReportUsageTime_ = static_cast<int64_t>(now);
    }
}

void HlsSegmentManager::OnSourceKeyChange(const uint8_t *key, size_t keyLen, const uint8_t *iv)
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

void HlsSegmentManager::OnDrmInfoChanged(const std::multimap<std::string, std::vector<uint8_t>>& drmInfos)
{
    if (segEventCb_) {
        HlsSegEvent event {.segType = type_, .type = PluginEventType::SOURCE_DRM_INFO_UPDATE, .drmInfos = drmInfos,
            .str = "drm_info_update"};
        segEventCb_(event);
    }
}

void HlsSegmentManager::SetStatusCallback(StatusCallbackFunc cb)
{
    FALSE_RETURN(playlistDownloader_ != nullptr);
    statusCallback_ = cb;
    playlistDownloader_->SetStatusCallback(cb);
}

std::vector<uint32_t> HlsSegmentManager::GetBitRates()
{
    FALSE_RETURN_V(playlistDownloader_ != nullptr, std::vector<uint32_t>());
    return playlistDownloader_->GetBitRates();
}

bool HlsSegmentManager::SelectBitRate(uint32_t bitRate)
{
    AutoLock lock(switchMutex_);
    FALSE_RETURN_V(playlistDownloader_ != nullptr, false);
    FALSE_RETURN_V(playList_ != nullptr, false);
    if (type_ == HlsSegmentType::SEG_AUDIO || playlistDownloader_->IsBitrateSame(bitRate)) {
        // audio do not auto select bitrate
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
        MEDIA_LOG_I("HLS download done, type: %{public}d", type_);
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

void HlsSegmentManager::SeekToTs(uint64_t seekTime, SeekMode mode)
{
    MEDIA_LOG_I("SeekToTs: in, type: %{public}d", type_);
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
            MEDIA_LOG_D("Seek failed count: " PUBLIC_LOG_D32 ", type: %{public}d", seekFailedCount_, type_);
            continue;
        }
    }
}

int64_t HlsSegmentManager::RequestNewTs(uint64_t seekTime, SeekMode mode, double totalDuration,
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
                MEDIA_LOG_I("writeTsIndex, RequestNewTs update writeTsIndex " PUBLIC_LOG_U32 ", type: %{public}d",
                    writeTsIndex_, type_);
                return -1;
            }
            startTimePos = 0;
        }
        playInfo.startTimePos_ = startTimePos;
    }
    PushPlayInfo(playInfo);
    return 0;
}
void HlsSegmentManager::PushPlayInfo(PlayInfo playInfo)
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

void HlsSegmentManager::SeekToTsForRead(uint32_t currentTsIndex)
{
    MEDIA_LOG_I("SeekToTimeForRead: currentTsIndex " PUBLIC_LOG_U32 ", type: %{public}d", currentTsIndex, type_);
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
            MEDIA_LOG_D("Seek failed count: " PUBLIC_LOG_U32 ", type: %{public}d", seekFailedCount_, type_);
            continue;
        }
    }
    MEDIA_LOG_I("SeekToTimeForRead end, type: %{public}d", type_);
    isSeekingFlag = false;
}

int64_t HlsSegmentManager::RequestNewTsForRead(const PlayInfo& item)
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

void HlsSegmentManager::UpdateDownloadFinished(const std::string &url, const std::string& location)
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
        MEDIA_LOG_D("Download done, average download speed : " PUBLIC_LOG_D32 " bit/s, type: %{public}d",
            avgDownloadSpeed_, type_);
        int64_t nowTime = steadyClock_.ElapsedMilliseconds();
        auto downloadTime = (nowTime - startDownloadTime_) / 1000;
        MEDIA_LOG_D("Download done, data usage: " PUBLIC_LOG_U64 " bits in " PUBLIC_LOG_D64 "ms, type: %{public}d",
            totalBits_, downloadTime * 1000, type_);
        HandleBuffering();
    }

    // bitrate above 0, user is not selecting, auto seliect is not going, playlist is done, is not seeking
    if ((bitRate > 0) && !isSelectingBitrate_ && isAutoSelectBitrate_ &&
        playlistDownloader_ != nullptr && playlistDownloader_->IsParseAndNotifyFinished() && !isSeekingFlag) {
        AutoSelectBitrate(bitRate);
    }
}


void HlsSegmentManager::SetReadBlockingFlag(bool isReadBlockingAllowed)
{
    MEDIA_LOG_D("SetReadBlockingFlag entered, type: %{public}d", type_);
}

void HlsSegmentManager::SetIsTriggerAutoMode(bool isAuto)
{
    isAutoSelectBitrate_ = isAuto;
}

void HlsSegmentManager::ReportVideoSizeChange()
{
    FALSE_RETURN(segEventCb_);
    FALSE_RETURN(playlistDownloader_ != nullptr);
    int32_t videoWidth = playlistDownloader_->GetVideoWidth();
    int32_t videoHeight = playlistDownloader_->GetVideoHeight();
    MEDIA_LOG_I("HLS ReportVideoSizeChange videoWidth : " PUBLIC_LOG_D32 "videoHeight: "
        PUBLIC_LOG_D32 ", type: %{public}d", videoWidth, videoHeight, type_);
    changeBitRateCount_++;
    MEDIA_LOG_I("HLS Change bit rate count : " PUBLIC_LOG_U32 ", type: %{public}d", changeBitRateCount_, type_);
    std::pair<int32_t, int32_t> videoSize {videoWidth, videoHeight};
    HlsSegEvent event {.segType = type_, .type = PluginEventType::VIDEO_SIZE_CHANGE, .videoSize = videoSize,
        .str = "video_size_change"};
    segEventCb_(event);
}

void HlsSegmentManager::SetDemuxerState(int32_t streamId)
{
    MEDIA_LOG_I("HLS SetDemuxerState, type: %{public}d", type_);
    isReadFrame_ = true;
    isFirstFrameArrived_ = true;
}

void HlsSegmentManager::SetDownloadErrorState()
{
    MEDIA_LOG_I("HLS SetDownloadErrorState, type: %{public}d", type_);
    downloadErrorState_ = true;
    if (segEventCb_ && !isReportedErrorCode_) {
        HlsSegEvent event {.segType = type_, .type = PluginEventType::CLIENT_ERROR,
            .networkError = NetworkClientErrorCode::ERROR_TIME_OUT, .str = "download"};
        segEventCb_(event);
        isTimeoutErrorNotified_.store(true);
    }
    Close(true);
}

void HlsSegmentManager::AutoSelectBitrate(uint32_t bitRate)
{
    MEDIA_LOG_I("HLS AutoSelectBitrate download bitrate " PUBLIC_LOG_D32 ", type: %{public}d", bitRate, type_);
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
            ", bufferLowSize " PUBLIC_LOG_D32 ", type: %{public}d", curBitrate, desBitRate, bufferLowSize, type_);
        return;
    }
    uint32_t bufferHighSize = minBufferSize_ * 0.8; // high size: buffersize * 0.8

    // switch to low bitrate, if buffersize more than highsize, do not switch
    if (curBitrate > desBitRate && GetBufferSize() > bufferHighSize) {
        MEDIA_LOG_I("HLS AutoSelectBitrate curBitrate " PUBLIC_LOG_D32 ", desBitRate " PUBLIC_LOG_D32
            ", bufferHighSize " PUBLIC_LOG_D32 ", type: %{public}d", curBitrate, desBitRate, bufferHighSize, type_);
        return;
    }
    MEDIA_LOG_I("HLS AutoSelectBitrate " PUBLIC_LOG_D32 " switch to " PUBLIC_LOG_D32 ", type: %{public}d", curBitrate,
        desBitRate, type_);
    SelectBitRate(desBitRate);
}

bool HlsSegmentManager::CheckRiseBufferSize()
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
        MEDIA_LOG_I("HLS downloadRate: " PUBLIC_LOG_D64 "current bit rate: " PUBLIC_LOG_D64 ", increasing buffer size"
            ", type: %{public}d", static_cast<uint64_t>(search->downloadRate), playingBitrate, type_);
        isHistoryLow = true;
    }
    return isHistoryLow;
}

bool HlsSegmentManager::CheckPulldownBufferSize()
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
        MEDIA_LOG_I("HLS downloadRate: " PUBLIC_LOG_D64 "current bit rate: " PUBLIC_LOG_D64 ", reducing buffer size"
            ", type: %{public}d", static_cast<uint64_t>(search->downloadRate), playingBitrate, type_);
        isPullDown = true;
    }
    return isPullDown;
}

void HlsSegmentManager::RiseBufferSize()
{
    if (totalBufferSize_ >= MAX_BUFFER_SIZE) {
        MEDIA_LOGI_LIMIT(SAVE_DATA_LOG_FREQUENCY, "HLS increasing buffer size failed, already reach the max buffer "
            "size: " PUBLIC_LOG_D64 ", current buffer size: " PUBLIC_LOG_ZU ", type: %{public}d", MAX_BUFFER_SIZE,
            totalBufferSize_, type_);
        return;
    }
    size_t tmpBufferSize = totalBufferSize_ + 1 * 1024 * 1024;
    totalBufferSize_ = tmpBufferSize;
    MEDIA_LOG_I("HLS increasing buffer size: " PUBLIC_LOG_ZU ", type: %{public}d", totalBufferSize_, type_);
}

void HlsSegmentManager::DownBufferSize()
{
    if (totalBufferSize_ <= minBufferSize_) {
        MEDIA_LOG_I("HLS reducing buffer size failed, already reach the min buffer size: "
            PUBLIC_LOG_ZU ", current buffer size: " PUBLIC_LOG_ZU ", type: %{public}d", minBufferSize_,
            totalBufferSize_, type_);
        return;
    }
    size_t tmpBufferSize = totalBufferSize_ - 1 * 1024 * 1024;
    totalBufferSize_ = tmpBufferSize;
    MEDIA_LOG_I("HLS reducing buffer size: " PUBLIC_LOG_ZU ", type: %{public}d", totalBufferSize_, type_);
}

void HlsSegmentManager::OnReadBuffer(uint32_t len)
{
    // Bytes to bit
    uint32_t duration = len * 8;
    if (duration >= bufferedDuration_) {
        bufferedDuration_ = 0;
    } else {
        bufferedDuration_ -= duration;
    }
}

void HlsSegmentManager::ActiveAutoBufferSize()
{
    if (userDefinedBufferDuration_) {
        MEDIA_LOG_I("HLS User has already setted a buffersize, not switch auto buffer size, type: %{public}d", type_);
        return;
    }
    autoBufferSize_ = true;
}

void HlsSegmentManager::InActiveAutoBufferSize()
{
    autoBufferSize_ = false;
}

uint64_t HlsSegmentManager::TransferSizeToBitRate(int width)
{
    if (width <= MIN_WIDTH) {
        return minBufferSize_;
    } else if (width >= MIN_WIDTH && width < SECOND_WIDTH) {
        return minBufferSize_ * TRANSFER_SIZE_RATE_2;
    } else if (width >= SECOND_WIDTH && width < THIRD_WIDTH) {
        return minBufferSize_ * TRANSFER_SIZE_RATE_3;
    } else {
        return minBufferSize_ * TRANSFER_SIZE_RATE_4;
    }
}

size_t HlsSegmentManager::GetTotalBufferSize()
{
    return totalBufferSize_;
}

void HlsSegmentManager::SetInterruptState(bool isInterruptNeeded)
{
    MEDIA_LOG_I("SetInterruptState: " PUBLIC_LOG_D32 ", type: %{public}d", isInterruptNeeded, type_);
    {
        AutoLock lk(bufferingEndMutex_);
        isInterruptNeeded_ = isInterruptNeeded;
        if (isInterruptNeeded_) {
            MEDIA_LOG_I("SetInterruptState bufferingEndCond NotifyAll, type: %{public}d", type_);
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

void HlsSegmentManager::GetDownloadInfo(DownloadInfo& downloadInfo)
{
    if (recordSpeedCount_ == 0) {
        MEDIA_LOG_E("HlsSegmentManager is 0, can't get avgDownloadRate, type: %{public}d", type_);
        downloadInfo.avgDownloadRate = 0;
    } else {
        downloadInfo.avgDownloadRate = avgSpeedSum_ / recordSpeedCount_;
    }
    downloadInfo.avgDownloadSpeed = avgDownloadSpeed_;
    downloadInfo.totalDownLoadBits = totalBits_;
    downloadInfo.isTimeOut = isTimeOut_;
}

void HlsSegmentManager::GetPlaybackInfo(PlaybackInfo& playbackInfo)
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

void HlsSegmentManager::ReportBitrateStart(uint32_t bitRate)
{
    FALSE_RETURN_MSG(segEventCb_, "HLS ReportBitrateStart no callback, type: %{public}d", type_);
    MEDIA_LOG_I("HLS ReportBitrateStart bitRate : " PUBLIC_LOG_U32, bitRate);
    HlsSegEvent event {.segType = type_, .type = PluginEventType::SOURCE_BITRATE_START, .bitRate = bitRate,
        .str = "source_bitrate_start"};
    segEventCb_(event);
}

std::pair<int32_t, int32_t> HlsSegmentManager::GetDownloadInfo()
{
    MEDIA_LOG_I("HlsSegmentManager::GetDownloadInfo, type: %{public}d", type_);
    if (recordSpeedCount_ == 0) {
        MEDIA_LOG_E("recordSpeedCount_ is 0, can't get avgDownloadRate, type: %{public}d", type_);
        return std::make_pair(0, static_cast<int32_t>(avgDownloadSpeed_));
    }
    auto rateAndSpeed = std::make_pair(avgSpeedSum_ / recordSpeedCount_, static_cast<int32_t>(avgDownloadSpeed_));
    return rateAndSpeed;
}

std::pair<int32_t, int32_t> HlsSegmentManager::GetDownloadRateAndSpeed()
{
    MEDIA_LOG_I("HlsSegmentManager::GetDownloadRateAndSpeed, type: %{public}d", type_);
    if (recordSpeedCount_ == 0) {
        MEDIA_LOG_E("recordSpeedCount_ is 0, can't get avgDownloadRate, type: %{public}d", type_);
        return std::make_pair(0, static_cast<int32_t>(avgDownloadSpeed_));
    }
    auto rateAndSpeed = std::make_pair(avgSpeedSum_ / recordSpeedCount_, static_cast<int32_t>(avgDownloadSpeed_));
    return rateAndSpeed;
}

Status HlsSegmentManager::SetCurrentBitRate(int32_t bitRate, int32_t streamID)
{
    MEDIA_LOG_I("HLS SetCurrentBitRate: " PUBLIC_LOG_D32 ", type: %{public}d", bitRate, type_);
    if ((bitRate <= 0 && currentBitRate_ == 0) || playlistDownloader_ == nullptr) {
        currentBitRate_ = -1; // -1
    } else {
        int32_t playlistBitrate = static_cast<int32_t>(playlistDownloader_->GetCurBitrate());
        currentBitRate_ = std::max(playlistBitrate, static_cast<int32_t>(bitRate));
        MEDIA_LOG_I("HLS playlistBitrate: " PUBLIC_LOG_D32 " currentBitRate: " PUBLIC_LOG_D32 ", type: %{public}d",
            playlistBitrate, currentBitRate_, type_);
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

void HlsSegmentManager::CalculateBitRate(size_t fragmentSize, double duration)
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
    MEDIA_LOG_I("HLS Calculate avgBitRate: " PUBLIC_LOG_D32 ", type: %{public}d", currentBitRate_, type_);
}

void HlsSegmentManager::UpdateWaterLineAbove()
{
    if (!isFirstFrameArrived_) {
        return;
    }
    if (currentBitRate_ == 0 && playlistDownloader_ != nullptr) {
        currentBitRate_ = static_cast<int32_t>(playlistDownloader_->GetCurBitrate());
        MEDIA_LOG_I("HLS use playlist bitrate: " PUBLIC_LOG_D32 ", type: %{public}d", currentBitRate_, type_);
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
        MEDIA_LOG_D("HLS UpdateWaterLineAbove default: " PUBLIC_LOG_ZU ", type: %{public}d", waterLineAbove, type_);
    }
    waterLineAbove_ = std::min(waterLineAbove, static_cast<size_t>(totalBufferSize_ *
        WATER_LINE_ABOVE_LIMIT_RATIO));
    MEDIA_LOG_D("HLS UpdateWaterLineAbove: " PUBLIC_LOG_ZU " writeBitrate: " PUBLIC_LOG_U64 " avgDownloadSpeed: "
        PUBLIC_LOG_D32 " currentBitRate: " PUBLIC_LOG_D32 ", type: %{public}d",
        waterLineAbove_, writeBitrateCaculator_->GetWriteBitrate(), avgDownloadSpeed_, currentBitRate_, type_);
}

void HlsSegmentManager::HandleCachedDuration()
{
    auto tmpBitRate = currentBitRate_;
    if (tmpBitRate <= 0 || !segEventCb_) {
        return;
    }
    cachedDuration_ = (GetTotalTsBuffersize() *
        BYTES_TO_BIT * SECOND_TO_MILLISECONDS) / static_cast<uint64_t>(tmpBitRate);
    if ((cachedDuration_ > lastDurationReacord_ &&
        cachedDuration_ - lastDurationReacord_ > DURATION_CHANGE_AMOUT_MILLISECONDS) ||
        (lastDurationReacord_ > cachedDuration_ &&
        lastDurationReacord_ - cachedDuration_ > DURATION_CHANGE_AMOUT_MILLISECONDS)) {
        MEDIA_LOG_D("HLS OnEvent cachedDuration: " PUBLIC_LOG_U64 ", type: %{public}d", cachedDuration_, type_);
        HlsSegEvent event {.segType = type_, .type = PluginEventType::CACHED_DURATION,
            .cachedDuration = cachedDuration_, .str = "buffering_duration"};
        segEventCb_(event);
        lastDurationReacord_ = cachedDuration_;
    }
}

void HlsSegmentManager::UpdateCachedPercent(BufferingInfoType infoType)
{
    if (waterLineAbove_ == 0 || !segEventCb_) {
        MEDIA_LOG_E("HLS UpdateCachedPercent: ERROR, type: %{public}d", type_);
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
        HlsSegEvent event {.segType = type_, .type = PluginEventType::EVENT_BUFFER_PROGRESS, .percent = percent,
            .str = "buffer percent"};
        segEventCb_(event);
        lastCachedSize_ = bufferSize;
    }
}

bool HlsSegmentManager::CheckBufferingOneSeconds()
{
    MEDIA_LOG_I("HLS CheckBufferingOneSeconds in, type: %{public}d", type_);
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
    MEDIA_LOG_I("HLS CheckBufferingOneSeconds out, type: %{public}d", type_);
    return isBuffering_.load();
}

void HlsSegmentManager::SetAppUid(int32_t appUid)
{
    if (downloader_) {
        downloader_->SetAppUid(appUid);
    }
    if (playlistDownloader_) {
        playlistDownloader_->SetAppUid(appUid);
    }
}

float HlsSegmentManager::GetCacheDuration(float ratio)
{
    if (ratio >= 1) {
        return CACHE_LEVEL_1;
    }
    return DEFAULT_CACHE_TIME;
}

uint64_t HlsSegmentManager::GetBufferSize() const
{
    uint64_t bufferSize = 0;
    if (cacheMediaBuffer_ != nullptr) {
        bufferSize = cacheMediaBuffer_->GetBufferSize(readOffset_);
    }
    return bufferSize;
}

uint64_t HlsSegmentManager::GetCrossTsBuffersize()
{
    uint64_t bufferSize = 0;
    if (cacheMediaBuffer_ == nullptr) {
        return bufferSize;
    }
    bufferSize = cacheMediaBuffer_->GetBufferSize(readOffset_);
    if (!backPlayList_.empty() && readTsIndex_ + 1 < backPlayList_.size()) {
        uint64_t nextTsOffset = SpliceOffset(readTsIndex_ + 1, 0);
        uint64_t nextTsBuffersize = cacheMediaBuffer_->GetBufferSize(nextTsOffset);
        bufferSize += nextTsBuffersize;
    }
    return bufferSize;
}

bool HlsSegmentManager::IsCachedInitSizeReady(int32_t wantInitSize)
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

bool HlsSegmentManager::GetPlayable()
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

bool HlsSegmentManager::GetBufferingTimeOut()
{
    if (bufferingTime_ == 0) {
        return false;
    } else {
        size_t now = static_cast<size_t>(steadyClock_.ElapsedMilliseconds());
        return now >= bufferingTime_ ? now - bufferingTime_ >= timeoutInterval_ : false;
    }
}

bool HlsSegmentManager::GetReadTimeOut(bool isDelay)
{
    size_t now = static_cast<size_t>(steadyClock_.ElapsedMilliseconds());
    if (isDelay) { // NOT_READY
        timeoutInterval_ = MAX_BUFFERING_TIME_OUT_DELAY;
    }
    return (now >= readTime_) ? (now - readTime_ >= timeoutInterval_) : false;
}

size_t HlsSegmentManager::GetSegmentOffset()
{
    if (playlistDownloader_) {
        return playlistDownloader_->GetSegmentOffset(readTsIndex_);
    }
    return 0;
}

bool HlsSegmentManager::GetHLSDiscontinuity()
{
    if (playlistDownloader_) {
        return playlistDownloader_->GetHLSDiscontinuity();
    }
    return false;
}

Status HlsSegmentManager::StopBufferring(bool isAppBackground)
{
    MEDIA_LOG_I("HlsSegmentManager:StopBufferring enter, type: %{public}d", type_);
    if (cacheMediaBuffer_ == nullptr || downloader_ == nullptr || playlistDownloader_ == nullptr) {
        MEDIA_LOG_E("StopBufferring error, type: %{public}d", type_);
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
    MEDIA_LOG_I("HlsSegmentManager:StopBufferring out, type: %{public}d", type_);
    return Status::OK;
}

bool HlsSegmentManager::ClearChunksOfFragment()
{
    bool res = false;
    uint64_t offsetBegin = SpliceOffset(readTsIndex_, 0);
    uint64_t diff = readOffset_ > offsetBegin ? readOffset_ - offsetBegin : 0;
    if (diff > READ_BACK_SAVE_SIZE) {
        MEDIA_LOG_D("HLS ClearChunksOfFragment: " PUBLIC_LOG_U64 ", type: %{public}d", readOffset_, type_);
        res = cacheMediaBuffer_->ClearChunksOfFragment(readOffset_ - READ_BACK_SAVE_SIZE);
    }
    return res;
}

void HlsSegmentManager::WaitForBufferingEnd()
{
    AutoLock lk(bufferingEndMutex_);
    FALSE_RETURN_MSG(isBuffering_.load(), "isBuffering false.");
    MEDIA_LOG_I("WaitForBufferingEnd");
    bufferingEndCond_.Wait(lk, [this]() {
        MEDIA_LOG_I("Wait in, isBuffering: " PUBLIC_LOG_D32 " isInterruptNeeded: " PUBLIC_LOG_D32 ", type: %{public}d",
            isBuffering_.load(), isInterruptNeeded_.load(), type_);
        return !isBuffering_.load() || isInterruptNeeded_.load();
    });
}

void HlsSegmentManager::SetIsReportedErrorCode()
{
    isReportedErrorCode_ = true;
}

bool HlsSegmentManager::SetInitialBufferSize(int32_t offset, int32_t size)
{
    AutoLock lock(initCacheMutex_);
    bool isInitBufferSizeOk = IsCachedInitSizeReady(size) || CheckBreakCondition();
    if (isInitBufferSizeOk || !downloader_ || !downloadRequest_ || isTimeoutErrorNotified_.load()) {
        MEDIA_LOG_I("HLS SetInitialBufferSize initCacheSize ok, type: %{public}d", type_);
        return false;
    }
    MEDIA_LOG_I("HLS SetInitialBufferSize initCacheSize " PUBLIC_LOG_U32 ", type: %{public}d", size, type_);
    if (!isBuffering_.load()) {
        isBuffering_.store(true);
    }
    bufferingTime_ = static_cast<size_t>(steadyClock_.ElapsedMilliseconds());
    expectOffset_.store(offset);
    initCacheSize_.store(size);
    return true;
}

void HlsSegmentManager::SetPlayStrategy(const std::shared_ptr<PlayStrategy>& playStrategy)
{
    if (playlistDownloader_ == nullptr || playStrategy == nullptr) {
        MEDIA_LOG_E("SetPlayStrategy error, type: %{public}d", type_);
        return;
    }
    if (playStrategy->width > 0 && playStrategy->height > 0) {
        playlistDownloader_->SetInitResolution(playStrategy->width, playStrategy->height);
    }
    if (playStrategy->bufferDurationForPlaying > 0) {
        bufferDurationForPlaying_ = playStrategy->bufferDurationForPlaying;
        waterlineForPlaying_ = static_cast<uint64_t>(static_cast<double>(CURRENT_BIT_RATE) /
            static_cast<double>(BYTES_TO_BIT) * bufferDurationForPlaying_);
        MEDIA_LOG_I("HLS buffer duration for playing : " PUBLIC_LOG ".3f, type: %{public}d", bufferDurationForPlaying_,
            type_);
    }
}

bool HlsSegmentManager::IsNeedBufferForPlaying()
{
    if (bufferDurationForPlaying_ <= 0 || !isDemuxerInitSuccess_.load() || !isBuffering_.load()) {
        return false;
    }
    if (GetBufferingTimeOut() && segEventCb_) {
        HlsSegEvent event {.segType = type_, .type = PluginEventType::CLIENT_ERROR,
            .networkError = NetworkClientErrorCode::ERROR_TIME_OUT, .str = "buffer for playing"};
        segEventCb_(event);
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
            PUBLIC_LOG_U64 ", type: %{public}d", GetCrossTsBuffersize(), waterlineForPlaying_, type_);
        isBuffering_.store(false);
        isDemuxerInitSuccess_.store(false);
        bufferingTime_ = 0;
        return false;
    }
    return true;
}

void HlsSegmentManager::NotifyInitSuccess()
{
    MEDIA_LOG_I("HLS NotifyInitSuccess in, type: %{public}d", type_);
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

uint64_t HlsSegmentManager::GetCachedDuration()
{
    MEDIA_LOG_I("HLS GetCachedDuration: " PUBLIC_LOG_U64 ", type: %{public}d", cachedDuration_, type_);
    return cachedDuration_;
}

bool HlsSegmentManager::CheckLoopTimeout(int64_t loopStartTime)
{
    int64_t now = loopInterruptClock_.ElapsedSeconds();
    int64_t loopDuration = now > loopStartTime ? now - loopStartTime : 0;
    bool isLoopTimeout = loopDuration > LOOP_TIMEOUT;
    if (isLoopTimeout) {
        SetDownloadErrorState();
        MEDIA_LOG_E("loop timeout, type: %{public}d", type_);
    }
    return isLoopTimeout;
}

Status HlsSegmentManager::GetStreamInfo(std::vector<StreamInfo>& streams)
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
        MEDIA_LOG_I("HLS GetStreamInfo " PUBLIC_LOG_ZU ", type: %{public}d", streams.size(), type_);
    }
    return Status::OK;
}

bool HlsSegmentManager::IsHlsFmp4()
{
    if (playlistDownloader_) {
        return playlistDownloader_->IsHlsFmp4();
    }
    return false;
}

bool HlsSegmentManager::IsPureByteRange()
{
    if (playlistDownloader_) {
        return playlistDownloader_->IsPureByteRange();
    }
    return false;
}

void HlsSegmentManager::HandleSeekReady(int32_t streamId, int32_t isEos)
{
    Format seekReadyInfo {};
    auto type = type_ == HlsSegmentType::SEG_VIDEO ?
        MediaAVCodec::MediaType::MEDIA_TYPE_VID : MediaAVCodec::MediaType::MEDIA_TYPE_AUD;
    seekReadyInfo.PutIntValue("currentStreamType", type);
    seekReadyInfo.PutIntValue("currentStreamId", streamId);
    seekReadyInfo.PutIntValue("isEOS", isEos);
    seekReadyInfo.PutLongValue("seekStartTimePos", seekStartTimePos_);
    MEDIA_LOG_D("StreamType: " PUBLIC_LOG_D32 " StreamId: " PUBLIC_LOG_D32 " isEOS: " PUBLIC_LOG_D32
        " seekStartTimePos: " PUBLIC_LOG_D64 ", type: %{public}d", type, streamId, isEos, seekStartTimePos_, type_);
    if (segEventCb_) {
        HlsSegEvent event {.segType = type_, .type = PluginEventType::HLS_SEEK_READY, .seekReadyInfo = seekReadyInfo,
            .str = "hls_seek_ready"};
        segEventCb_(event);
    }
}

uint64_t HlsSegmentManager::GetTotalTsBuffersize()
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
        PUBLIC_LOG_U32 " offset: " PUBLIC_LOG_U64 ", type: %{public}d", readTsIndex_.load(), tsIndex, offset, type_);
    return totalBufferSize;
}

uint64_t HlsSegmentManager::GetMemorySize()
{
    return memorySize_;
}

bool HlsSegmentManager::IsHlsEnd()
{
    if (CheckReadStatus() && GetBufferSize() == 0 && readTsIndex_ + 1 == backPlayList_.size() &&
        tsStorageInfo_.find(readTsIndex_) != tsStorageInfo_.end() &&
        tsStorageInfo_[readTsIndex_].second) {
            if (isEos_) {
                MEDIA_LOG_I("HLS download done, type: %{public}d", type_);
            }
            return true;
        }
    return false;
}

void HlsSegmentManager::OnMasterReady(bool needAudioManager, bool needSubTitleManager)
{
    if (type_ == HlsSegmentType::SEG_AUDIO) {
        return;
    }
    if (masterReadyCallback_) {
        masterReadyCallback_(needAudioManager, needSubTitleManager);
    }
}

void HlsSegmentManager::Clone(const std::shared_ptr<HlsSegmentManager> &other)
{
    totalBufferSize_ = other->totalBufferSize_;
    callback_ = other->callback_;
    statusCallback_ = other->statusCallback_;
    OnSourceKeyChange(other->key_, other->keyLen_, other->iv_);
    isAutoSelectBitrate_ = other->isAutoSelectBitrate_;
    seekTime_ = other->seekTime_;
    downloadErrorState_ = other->downloadErrorState_;
    autoBufferSize_ = other->autoBufferSize_;
    isInterruptNeeded_.store(other->isInterruptNeeded_.load());
    httpHeader_ = other->httpHeader_;
    isStopped.store(other->isStopped.load());
    isInterrupt_ = other->isInterrupt_;
    isFirstFrameArrived_ = other->isFirstFrameArrived_;
    isReportedErrorCode_ = other->isReportedErrorCode_;
    bufferDurationForPlaying_ = other->bufferDurationForPlaying_;
    waterlineForPlaying_ = other->waterlineForPlaying_;
    isDemuxerInitSuccess_.store(other->isDemuxerInitSuccess_.load());
    isTimeoutErrorNotified_.store(other->isTimeoutErrorNotified_.load());
    timeoutInterval_ = other->timeoutInterval_;
    bufferingCbFunc_ = other->bufferingCbFunc_;
    segEventCb_ = other->segEventCb_;
    InitCacheWithDuration();
    FALSE_RETURN_MSG(playlistDownloader_ != nullptr, "Clone no playlistDownloader_");
    playlistDownloader_->SetPlayListCallback(std::weak_ptr<PlayListChangeCallback>(weak_from_this()));
    FALSE_RETURN_MSG(other->playlistDownloader_ != nullptr, "Clone no other playlistDownloader_");
    playlistDownloader_->Clone(other->playlistDownloader_);
}

void HlsSegmentManager::SetMasterReadyCallback(std::function<void(bool, bool)> cb)
{
    masterReadyCallback_ = cb;
}

bool HlsSegmentManager::SelectAudio(int32_t streamId)
{
    AutoLock lock(switchMutex_);
    FALSE_RETURN_V(playlistDownloader_ != nullptr, false);
    FALSE_RETURN_V(playList_ != nullptr, false);
    MEDIA_LOG_W("HLS SelectAudi: %{public}d, type: %{public}d", streamId, type_);
    auto isAudioSame = playlistDownloader_->IsAudioSame(streamId);
    if (type_ == HlsSegmentType::SEG_VIDEO || isAudioSame) {
        // video do not auto select bitrate
        MEDIA_LOG_W("HLS SelectAudio skip, type: %{public}d, isAudioSame: %{public}d", type_, isAudioSame);
        return true;
    }

    playlistDownloader_->Cancel();

    // clear request queue
    playList_->SetActive(false, true);
    playList_->SetActive(true);
    fragmentDownloadStart.clear();
    fragmentPushed.clear();
    backPlayList_.clear();
    
    // switch to des bitrate
    playlistDownloader_->SelectAudio(streamId);
    isSelectingBitrate_ = true;
    playlistDownloader_->Start();
    playlistDownloader_->UpdateAudio();
    return true;
}

bool HlsSegmentManager::StartAudioDownload(int32_t streamId)
{
    FALSE_RETURN_V_MSG(playlistDownloader_ != nullptr, false, "StartAudioDownload no playlistDownloader_");
    playlistDownloader_->SelectAudio(streamId);
    playlistDownloader_->UpdateAudio();
    return true;
}

std::shared_ptr<StreamInfo> HlsSegmentManager::GetStreamInfoById(int32_t streamId)
{
    if (playlistDownloader_) {
        auto streamInfo = playlistDownloader_->GetStreamInfoById(streamId);
        return streamInfo;
    }
    return nullptr;
}

int32_t HlsSegmentManager::GetDefaultAudioStreamId()
{
    if (playlistDownloader_) {
        auto streamId = playlistDownloader_->GetDefaultAudioStreamId();
        return streamId;
    }
    return 0;
}

HlsSegmentType HlsSegmentManager::GetSegType(uint32_t streamId)
{
    if (playlistDownloader_) {
        auto type = playlistDownloader_->GetSegType(streamId);
        return type;
    }
    return HlsSegmentType::SEG_VIDEO;
}

void HlsSegmentManager::SetSegmentBufferingCallback(HlsSegmentBufferingCbFunc bufferingCbFunc)
{
    bufferingCbFunc_ = bufferingCbFunc;
}

void HlsSegmentManager::SetSegmentAllCallback(HlsSegmentEventCbFunc segEventCallback)
{
    segEventCb_ = segEventCallback;
}

}
}
}
}