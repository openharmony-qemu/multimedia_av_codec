/*
 * Copyright (c) 2022-2022 Huawei Device Co., Ltd.
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
#include "common/media_core.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <regex>

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
namespace {
constexpr uint32_t DECRYPT_COPY_LEN = 128;
constexpr int MIN_WITDH = 480;
constexpr int SECOND_WITDH = 720;
constexpr int THIRD_WITDH = 1080;
constexpr uint64_t MAX_BUFFER_SIZE = 19 * 1024 * 1024;
constexpr uint32_t SAMPLE_INTERVAL = 1000; // Sampling time interval: ms
constexpr int MAX_RECORD_COUNT = 10;
constexpr int START_PLAY_WATER_LINE = 512 * 1024;
constexpr int DATA_USAGE_NTERVAL = 300 * 1000;
constexpr double ZERO_THRESHOLD = 1e-9;
constexpr size_t PLAY_WATER_LINE = 5 * 1024;
constexpr uint32_t READ_SLEEP_INTERVAL = 5;
constexpr uint32_t READ_SLEEP_TIME_OUT = 30 * 1000;
constexpr int IS_DOWNLOAD_MIN_BIT = 100; // Determine whether it is downloading
constexpr int32_t BYTES_TO_BIT = 8;
constexpr int32_t DEFAULT_WATER_LINE_ABOVE = 512 * 1024;
constexpr float DEFAULT_CACHE_TIME = 0.3;
constexpr uint32_t DURATION_CHANGE_AMOUT_MILLIONSECOND = 500;
constexpr int64_t SECOND_TO_MILLIONSECOND = 1000;
constexpr int UPDATE_CACHE_STEP = 5 * 1024;
constexpr int SEEK_STATUS_RETRY_TIMES = 100;
constexpr int SEEK_STATUS_SLEEP_TIME = 50;
constexpr uint64_t CURRENT_BIT_RATE = 1 * 1024 * 1024; // bps
constexpr int32_t ONE_SECONDS = 1000;
constexpr int32_t TEN_MILLISECONDS = 10;
}

//   hls manifest, m3u8 --- content get from m3u8 url, we get play list from the content
//   fragment --- one item in play list, download media data according to the fragment address.
HlsMediaDownloader::HlsMediaDownloader() noexcept
{
    buffer_ = std::make_shared<RingBuffer>(RING_BUFFER_SIZE);
    buffer_->Init();
    isBuffering_ = true;
    totalRingBufferSize_ = RING_BUFFER_SIZE;
    downloader_ = std::make_shared<Downloader>("hlsMedia");
    playList_ = std::make_shared<BlockingQueue<PlayInfo>>("PlayList");

    dataSave_ =  [this] (uint8_t*&& data, uint32_t&& len) {
        return SaveData(std::forward<decltype(data)>(data), std::forward<decltype(len)>(len));
    };

    playListDownloader_ = std::make_shared<HlsPlayListDownloader>();
    playListDownloader_->SetPlayListCallback(this);
    waterLineAbove_ = PLAY_WATER_LINE;
    aesKey_.rounds = 0;
    for (size_t i = 0; i < sizeof(aesKey_.rd_key) / sizeof(aesKey_.rd_key[0]); ++i) {
        aesKey_.rd_key[i] = 0;
    }
}

HlsMediaDownloader::HlsMediaDownloader(int expectBufferDuration)
{
    expectDuration_ = static_cast<uint64_t>(expectBufferDuration);
    userDefinedBufferDuration_ = true;
    totalRingBufferSize_ = expectDuration_ * CURRENT_BIT_RATE;
    MEDIA_LOG_I("user define buffer duration.");
    downloader_ = std::make_shared<Downloader>("hlsMedia");
    playList_ = std::make_shared<BlockingQueue<PlayInfo>>("PlayList");
    dataSave_ =  [this] (uint8_t*&& data, uint32_t&& len) {
        return SaveData(std::forward<decltype(data)>(data), std::forward<decltype(len)>(len));
    };

    playListDownloader_ = std::make_shared<HlsPlayListDownloader>();
    playListDownloader_->SetPlayListCallback(this);
    aesKey_.rounds = 0;
    for (size_t i = 0; i < sizeof(aesKey_.rd_key) / sizeof(aesKey_.rd_key[0]); ++i) {
        aesKey_.rd_key[i] = 0;
    }
}

HlsMediaDownloader::HlsMediaDownloader(std::string mimeType)
{
    mimeType_ = mimeType;
    buffer_ = std::make_shared<RingBuffer>(RING_BUFFER_SIZE);
    buffer_->Init();
    totalRingBufferSize_ = RING_BUFFER_SIZE;
    downloader_ = std::make_shared<Downloader>("hlsMedia");
    playList_ = std::make_shared<BlockingQueue<PlayInfo>>("PlayList");

    dataSave_ =  [this] (uint8_t*&& data, uint32_t&& len) {
        return SaveData(std::forward<decltype(data)>(data), std::forward<decltype(len)>(len));
    };
    playListDownloader_ = std::make_shared<HlsPlayListDownloader>();
    playListDownloader_->SetPlayListCallback(this);
    steadyClock_.Reset();
    aesKey_.rounds = 0;
    for (size_t i = 0; i < sizeof(aesKey_.rd_key) / sizeof(aesKey_.rd_key[0]); ++i) {
        aesKey_.rd_key[i] = 0;
    }
}

HlsMediaDownloader::~HlsMediaDownloader()
{
    MEDIA_LOG_I("~HlsMediaDownloader in");
    if (playListDownloader_ != nullptr) {
        playListDownloader_ = nullptr;
    }
    if (downloader_ != nullptr) {
        downloader_ = nullptr;
    }
    MEDIA_LOG_I("~HlsMediaDownloader out");
}

void HlsMediaDownloader::PutRequestIntoDownloader(const PlayInfo& playInfo)
{
    auto realStatusCallback = [this] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                                        std::shared_ptr<DownloadRequest>& request) {
        statusCallback_(status, downloader_, std::forward<decltype(request)>(request));
    };
    auto downloadDoneCallback = [this] (const std::string &url, const std::string& location) {
        UpdateDownloadFinished(url, location);
    };

    MediaSouce mediaSouce;
    mediaSouce.url = playInfo.url_;
    mediaSouce.httpHeader = httpHeader_;
    // TO DO: If the fragment file is too large, should not requestWholeFile.
    downloadRequest_ = std::make_shared<DownloadRequest>(playInfo.duration_, dataSave_,
                                                         realStatusCallback, mediaSouce, true);
    // push request to back queue for seek
    fragmentDownloadStart[playInfo.url_] = true;
    int64_t startTimePos = playInfo.startTimePos_;
    curUrl_ = playInfo.url_;
    havePlayedTsNum_++;
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
    playListDownloader_->SetMimeType(mimeType_);
    playListDownloader_->Open(url, httpHeader);
    steadyClock_.Reset();
    openTime_ = steadyClock_.ElapsedMilliseconds();
    if (userDefinedBufferDuration_) {
        MEDIA_LOG_I("User setting buffer duration playListDownloader_ opened.");
        totalRingBufferSize_ = expectDuration_ * CURRENT_BIT_RATE;
        if (totalRingBufferSize_ < RING_BUFFER_SIZE) {
            MEDIA_LOG_I("Failed setting buffer size: " PUBLIC_LOG_ZU ". already lower than the min buffer size: "
            PUBLIC_LOG_D64 ", setting buffer size: " PUBLIC_LOG_D64 ". ",
            totalRingBufferSize_, RING_BUFFER_SIZE, RING_BUFFER_SIZE);
            buffer_ = std::make_shared<RingBuffer>(RING_BUFFER_SIZE);
            totalRingBufferSize_ = RING_BUFFER_SIZE;
        } else if (totalRingBufferSize_ > MAX_BUFFER_SIZE) {
            MEDIA_LOG_I("Failed setting buffer size: " PUBLIC_LOG_ZU ". already exceed the max buffer size: "
            PUBLIC_LOG_D64 ", setting buffer size: " PUBLIC_LOG_D64 ". ",
            totalRingBufferSize_, MAX_BUFFER_SIZE, MAX_BUFFER_SIZE);
            buffer_ = std::make_shared<RingBuffer>(MAX_BUFFER_SIZE);
            totalRingBufferSize_ = MAX_BUFFER_SIZE;
        } else {
            buffer_ = std::make_shared<RingBuffer>(totalRingBufferSize_);
            MEDIA_LOG_I("Success setted buffer size: " PUBLIC_LOG_ZU ". ", totalRingBufferSize_);
        }
        buffer_->Init();
    }
    return true;
}

void HlsMediaDownloader::Close(bool isAsync)
{
    MEDIA_LOG_I("Close enter");
    isInterrupt_ = true;
    buffer_->SetActive(false);
    playList_->SetActive(false);
    playListDownloader_->Close();
    downloader_->Stop(isAsync);
    isStopped = true;
    if (!isDownloadFinish_) {
        MEDIA_LOG_D("Download close, average download speed: " PUBLIC_LOG_D32 " bit/s", avgDownloadSpeed_);
        int64_t nowTime = steadyClock_.ElapsedMilliseconds();
        auto downloadTime = nowTime - startDownloadTime_;
        MEDIA_LOG_D("Download close, Data usage: " PUBLIC_LOG_U64 " bits in " PUBLIC_LOG_D64 "ms",
            totalBits_, downloadTime);
    }
}

void HlsMediaDownloader::Pause()
{
    MEDIA_LOG_I("Pause enter");
    playListDownloader_->Pause();
}

void HlsMediaDownloader::Resume()
{
    MEDIA_LOG_I("Resume enter");
    playListDownloader_->Resume();
}

bool HlsMediaDownloader::CheckReadStatus()
{
    // eos:palylist is empty, request is finished, hls is vod, do not select bitrate
    bool isEos = playList_->Empty() && (downloadRequest_ != nullptr) &&
                 downloadRequest_->IsEos() && playListDownloader_ != nullptr &&
                 (playListDownloader_->GetDuration() > 0) &&
                 playListDownloader_->IsParseAndNotifyFinished();
    if (isEos) {
        MEDIA_LOG_I("HLS download done.");
        return true;
    }
    if (playListDownloader_->GetDuration() > 0 && playListDownloader_->IsParseAndNotifyFinished() &&
        static_cast<int64_t>(seekTime_) >= playListDownloader_->GetDuration()) {
        MEDIA_LOG_I("HLS seek to tail.");
        return true;
    }
    return false;
}

bool HlsMediaDownloader::CheckReadTimeOut()
{
    if (readTime_ >= READ_SLEEP_TIME_OUT || downloadErrorState_ || isTimeOut_) {
        isTimeOut_ = true;
        if (downloader_ != nullptr) {
            // avoid deadlock caused by ringbuffer write stall
            buffer_->SetActive(false);
            // the downloader is unavailable after this
            downloader_->Pause(true);
        }
        if (downloader_ != nullptr && downloadRequest_ != nullptr && !downloadRequest_->IsClosed()) {
            downloadRequest_->Close();
        }
        if (callback_ != nullptr) {
            MEDIA_LOG_I("Read time out, OnEvent");
            callback_->OnEvent({PluginEventType::CLIENT_ERROR, {NetworkClientErrorCode::ERROR_TIME_OUT}, "read"});
        }
        return true;
    }
    return false;
}

bool HlsMediaDownloader::CheckBreakCondition()
{
    if (downloadErrorState_) {
        MEDIA_LOG_I("downloadErrorState break");
        return true;
    }
    if (CheckReadStatus()) {
        MEDIA_LOG_I("download complete break");
        return true;
    }
    return false;
}

bool HlsMediaDownloader::HandleBuffering()
{
    if (!isBuffering_) {
        return false;
    }
    UpdateCachedPercent(BufferingInfoType::BUFFERING_PERCENT);
    if (buffer_->GetSize() >= waterLineAbove_ || CheckBreakCondition()) {
        isBuffering_ = false;
    }

    if (!isBuffering_ && isFirstFrameArrived_ && callback_ != nullptr) {
        MEDIA_LOG_I("CacheData onEvent BUFFERING_END");
        UpdateCachedPercent(BufferingInfoType::BUFFERING_END);
        callback_->OnEvent({PluginEventType::BUFFERING_END, {BufferingInfoType::BUFFERING_END}, "end"});
    }
    return isBuffering_;
}

bool HlsMediaDownloader::HandleCache()
{
    waterLineAbove_ = static_cast<size_t>(GetWaterLineAbove());
    if (!isBuffering_ && callback_ != nullptr) {
        MEDIA_LOG_I("DownloadTask start.");
        isBuffering_ = true;
        UpdateCachedPercent(BufferingInfoType::BUFFERING_START);
        callback_->OnEvent({PluginEventType::BUFFERING_START, {BufferingInfoType::BUFFERING_START}, "start"});
        return true;
    }
    return false;
}

Status HlsMediaDownloader::CheckPlaylist(unsigned char* buff, ReadDataInfo& readDataInfo)
{
    bool isFinishedPlay = CheckReadStatus() || isStopped;
    if (downloadRequest_ != nullptr) {
        readDataInfo.isEos_ = downloadRequest_->IsEos();
    }
    if (isFinishedPlay && buffer_->GetSize() > 0) {
        readDataInfo.realReadLength_ = buffer_->ReadBuffer(buff, buffer_->GetSize(), 2);  // wait 2 times
        return Status::OK;
    }
    if (isFinishedPlay && buffer_->GetSize() == 0 && GetSeekable() == Seekable::SEEKABLE) {
        readDataInfo.realReadLength_ = 0;
        MEDIA_LOG_I("HlsMediaDownloader: CheckPlaylist, eos.");
        return Status::END_OF_STREAM;
    }
    return Status::ERROR_UNKNOWN;
}

Status HlsMediaDownloader::ReadDelegate(unsigned char* buff, ReadDataInfo& readDataInfo)
{
    FALSE_RETURN_V(buffer_ != nullptr, Status::END_OF_STREAM);
    FALSE_RETURN_V_MSG(!isInterruptNeeded_.load(), Status::END_OF_STREAM, "isInterruptNeeded");
    readDataInfo.isEos_ = CheckReadStatus();
    if (readDataInfo.isEos_ && buffer_->GetSize() == 0) {
        readDataInfo.realReadLength_ = 0;
        MEDIA_LOG_I("HlsMediaDownloader: buffer is empty, eos.");
        return Status::END_OF_STREAM;
    }

    if (isBuffering_ && CheckBufferingOneSeconds()) {
        MEDIA_LOG_I("Read return error again.");
        return Status::ERROR_AGAIN;
    }
    if (isFirstFrameArrived_ && buffer_->GetSize() < PLAY_WATER_LINE && !CheckBreakCondition()) {
        if (HandleCache()) {
            return Status::ERROR_AGAIN;
        }
    }
    FALSE_RETURN_V_MSG(readDataInfo.wantReadLength_ > 0, Status::END_OF_STREAM, "wantReadLength_ <= 0");
    readTime_ = 0;
    int64_t startTime = steadyClock_.ElapsedMilliseconds();
    while (buffer_->GetSize() < readDataInfo.wantReadLength_ && !isInterruptNeeded_.load()) {
        Status tmpRes = CheckPlaylist(buff, readDataInfo);
        if (tmpRes != Status::ERROR_UNKNOWN) {
            return tmpRes;
        }
        if (CheckReadTimeOut()) {
            readDataInfo.realReadLength_ = 0;
            MEDIA_LOG_I("HlsMediaDownloader: read time out, eos");
            return Status::END_OF_STREAM;
        }
        OSAL::SleepFor(READ_SLEEP_INTERVAL);  // 5
        int64_t endTime = steadyClock_.ElapsedMilliseconds();
        readTime_ = static_cast<uint64_t>(endTime - startTime);
    }
    if (isInterruptNeeded_.load()) {
        readDataInfo.realReadLength_ = 0;
        MEDIA_LOG_I("HlsMediaDownloader: InterruptNeeded, eos");
        return Status::END_OF_STREAM;
    }
    readDataInfo.realReadLength_ = buffer_->ReadBuffer(buff, readDataInfo.wantReadLength_, 2);  // wait 2 times
    OnReadRingBuffer(readDataInfo.realReadLength_);
    MEDIA_LOG_D("Read: wantReadLength " PUBLIC_LOG_D32 ", realReadLength " PUBLIC_LOG_D32 ", isEos "
                PUBLIC_LOG_D32, readDataInfo.wantReadLength_, readDataInfo.realReadLength_, readDataInfo.isEos_);
    return Status::OK;
}

Status HlsMediaDownloader::Read(unsigned char* buff, ReadDataInfo& readDataInfo)
{
    uint64_t now = static_cast<uint64_t>(steadyClock_.ElapsedMilliseconds());
    auto ret = ReadDelegate(buff, readDataInfo);
    readTotalBytes_ += readDataInfo.realReadLength_;
    if (now > lastReadCheckTime_ && now - lastReadCheckTime_ > SAMPLE_INTERVAL) {
        readRecordDuringTime_ = now - lastReadCheckTime_;
        double readDuration = static_cast<double>(readRecordDuringTime_) / SECOND_TO_MILLIONSECOND;
        if (readDuration > ZERO_THRESHOLD) {
            double readSpeed = static_cast<double>(readTotalBytes_ * BYTES_TO_BIT) / readDuration;    // bps
            currentBitrate_ = static_cast<uint64_t>(readSpeed);     // bps
            size_t curBufferSize = 0;
            if (buffer_ != nullptr) {
                curBufferSize = buffer_->GetSize();
            }
            MEDIA_LOG_D("Current read speed: " PUBLIC_LOG_D32 " Kbit/s,Current buffer size: " PUBLIC_LOG_U64
            " KByte", static_cast<int32_t>(readSpeed / 1024), static_cast<uint64_t>(curBufferSize / 1024));
            readTotalBytes_ = 0;
        }
        lastReadCheckTime_ = now;
        readRecordDuringTime_ = 0;
    }
    
    return ret;
}

bool HlsMediaDownloader::SeekToTime(int64_t seekTime, SeekMode mode)
{
    FALSE_RETURN_V(buffer_ != nullptr, false);
    AutoLock lock(switchMutex_);
    MEDIA_LOG_I("Seek: buffer size " PUBLIC_LOG_ZU ", seekTime " PUBLIC_LOG_D64, buffer_->GetSize(), seekTime);
    isSeekingFlag = true;
    seekTime_ = static_cast<uint64_t>(seekTime);
    int32_t retry {0};
    do {
        retry++;
        if (retry >= SEEK_STATUS_RETRY_TIMES) { // 100 means retry times
            MEDIA_LOG_I("Seek may be failed");
            break;
        }
        OSAL::SleepFor(SEEK_STATUS_SLEEP_TIME); // 50 means sleep time pre retry
    } while (!playListDownloader_->IsParseAndNotifyFinished());
    memset_s(afterAlignRemainedBuffer_, DECRYPT_UNIT_LEN, 0x00, DECRYPT_UNIT_LEN);
    memset_s(decryptCache_, RING_BUFFER_SIZE, 0x00, RING_BUFFER_SIZE);
    memset_s(decryptBuffer_, RING_BUFFER_SIZE, 0x00, RING_BUFFER_SIZE);
    afterAlignRemainedLength_ = 0;
    isLastDecryptWriteError_ = false;
    buffer_->SetActive(false);
    downloader_->Cancel();
    buffer_->Clear();
    buffer_->SetActive(true);
    SeekToTs(seekTime, mode);
    MEDIA_LOG_I("SeekToTime end\n");
    isSeekingFlag = false;
    return true;
}

size_t HlsMediaDownloader::GetContentLength() const
{
    return 0;
}

int64_t HlsMediaDownloader::GetDuration() const
{
    MEDIA_LOG_I("GetDuration " PUBLIC_LOG_D64, playListDownloader_->GetDuration());
    return playListDownloader_->GetDuration();
}

Seekable HlsMediaDownloader::GetSeekable() const
{
    return playListDownloader_->GetSeekable();
}

void HlsMediaDownloader::SetCallback(Callback* cb)
{
    callback_ = cb;
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
    if (playListDownloader_ != nullptr && playListDownloader_->IsParseFinished() &&
        (GetSeekable() == Seekable::UNSEEKABLE)) {
        if (backPlayList_.size() > 0) {
            backPlayList_.clear();
        }
        return;
    }
    if (playListDownloader_ != nullptr && playListDownloader_->IsParseFinished()) {
        backPlayList_.push_back(fragment);
    }
}

void HlsMediaDownloader::OnPlayListChanged(const std::vector<PlayInfo>& playList)
{
    ResetPlaylistCapacity(static_cast<size_t>(playList.size()));
    for (int i = 0; i < static_cast<int>(playList.size()); i++) {
        auto fragment = playList[i];
        PlaylistBackup(fragment);
        if (isSelectingBitrate_ && (GetSeekable() == Seekable::SEEKABLE)) {
            bool isFileIndexSame = (havePlayedTsNum_ - i) == 1 ? true : false; // 1
            if (isFileIndexSame) {
                MEDIA_LOG_I("Switch bitrate");
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
    if (!isDownloadStarted_ && !playList_->Empty()) {
        auto playInfo = playList_->Pop();
        std::string url = playInfo.url_;
        isDownloadStarted_ = true;
        PutRequestIntoDownloader(playInfo);
    }
}

bool HlsMediaDownloader::GetStartedStatus()
{
    return playListDownloader_->GetPlayListDownloadStatus() && startedPlayStatus_;
}

bool HlsMediaDownloader::SaveData(uint8_t* data, uint32_t len)
{
    OnWriteRingBuffer(len);
    startedPlayStatus_ = true;
    bool ret = false;
    if (keyLen_ == 0) {
        ret = buffer_->WriteBuffer(data, len);
    } else {
        ret = SaveEncryptData(data, len);
    }
    MEDIA_LOG_D("SaveData ret: " PUBLIC_LOG_D32 ", bufferSize: " PUBLIC_LOG_ZU ", len: " PUBLIC_LOG_U32,
        ret, buffer_->GetSize(), len);
    HandleCachedDuration();
    HandleBuffering();
    return ret;
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
    uint32_t minWriteLen = (RING_BUFFER_SIZE - afterAlignRemainedLength_) > writeLen ?
                            writeLen : RING_BUFFER_SIZE - afterAlignRemainedLength_;
    err = memcpy_s(decryptBuffer_ + afterAlignRemainedLength_,
                   minWriteLen, writeDataPoint, minWriteLen);
    if (err != 0) {
        MEDIA_LOG_D("minWriteLen: " PUBLIC_LOG_D32, minWriteLen);
    }
    realLen = minWriteLen + afterAlignRemainedLength_;
    AES_cbc_encrypt(decryptBuffer_, decryptCache_, realLen, &aesKey_, iv_, AES_DECRYPT);
    return realLen;
}

bool HlsMediaDownloader::SaveEncryptData(uint8_t* data, uint32_t len)
{
    uint32_t writeLen = 0;
    uint8_t *writeDataPoint = data;
    uint32_t waitLen = len;
    errno_t err {0};
    uint32_t realLen;
    if ((waitLen + afterAlignRemainedLength_) < DECRYPT_UNIT_LEN) {
        err = memcpy_s(afterAlignRemainedBuffer_ + afterAlignRemainedLength_,
                       DECRYPT_UNIT_LEN - afterAlignRemainedLength_,
                       writeDataPoint, waitLen);
        if (err != 0) {
            MEDIA_LOG_D("afterAlignRemainedLength_: " PUBLIC_LOG_D64,
                        DECRYPT_UNIT_LEN - afterAlignRemainedLength_);
        }
        afterAlignRemainedLength_ += waitLen;
        return true;
    }
    writeLen = ((waitLen + afterAlignRemainedLength_) / DECRYPT_UNIT_LEN) *
                DECRYPT_UNIT_LEN - afterAlignRemainedLength_;
    realLen = GetDecrptyRealLen(writeDataPoint, waitLen, writeLen);
    totalLen_ += realLen;
    bool isWriteRingBufferSuccess = buffer_->WriteBuffer(decryptCache_, realLen);
    err = memset_s(decryptCache_, realLen, 0x00, realLen);
    if (err != 0) {
        MEDIA_LOG_D("realLen: " PUBLIC_LOG_D32, realLen);
    }
    if (isWriteRingBufferSuccess) {
        afterAlignRemainedLength_ = 0;
        err = memset_s(afterAlignRemainedBuffer_, DECRYPT_UNIT_LEN, 0x00, DECRYPT_UNIT_LEN);
        if (err != 0) {
            MEDIA_LOG_D("DECRYPT_UNIT_LEN: " PUBLIC_LOG_D64, DECRYPT_UNIT_LEN);
        }
    }
    writeDataPoint += writeLen;
    waitLen -= writeLen;
    if (waitLen > 0 && isWriteRingBufferSuccess) {
        afterAlignRemainedLength_ = waitLen;
        err = memcpy_s(afterAlignRemainedBuffer_, DECRYPT_UNIT_LEN, writeDataPoint, waitLen);
        if (err != 0) {
            MEDIA_LOG_D("waitLen: " PUBLIC_LOG_D32, waitLen);
        }
    }
    return isWriteRingBufferSuccess;
}

void HlsMediaDownloader::DownloadRecordHistory(int64_t nowTime)
{
    if ((static_cast<uint64_t>(nowTime) - lastWriteTime_) >= SAMPLE_INTERVAL) {
        MEDIA_LOG_I("OnWriteRingBuffer nowTime: " PUBLIC_LOG_D64
        " lastWriteTime:" PUBLIC_LOG_D64 ".\n", nowTime, lastWriteTime_);
        BufferDownRecord* record = new BufferDownRecord();
        record->dataBits = lastWriteBit_;
        record->timeoff = static_cast<uint64_t>(nowTime) - lastWriteTime_;
        record->next = bufferDownRecord_;
        bufferDownRecord_ = record;
        lastWriteBit_ = 0;
        lastWriteTime_ = static_cast<uint64_t>(nowTime);
        BufferDownRecord* tmpRecord = bufferDownRecord_;
        for (int i = 0; i < MAX_RECORD_COUNT; i++) {
            if (tmpRecord->next) {
                tmpRecord = tmpRecord->next;
            } else {
                break;
            }
        }
        BufferDownRecord* next = tmpRecord->next;
        tmpRecord->next = nullptr;
        tmpRecord = next;
        while (tmpRecord) {
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

void HlsMediaDownloader::OnWriteRingBuffer(uint32_t len)
{
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
    double tmpDenominator = static_cast<double>(downloadDuringTime_) / SECOND_TO_MILLIONSECOND;
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
            size_t remainingBuffer = 0;
            if (buffer_ != nullptr) {
                remainingBuffer = buffer_->GetSize();
            }
            MEDIA_LOG_D("Current download speed : " PUBLIC_LOG_D32 " Kbit/s,Current buffer size : " PUBLIC_LOG_U64
                " KByte", static_cast<int32_t>(downloadRate / 1024), static_cast<uint64_t>(remainingBuffer / 1024));
            // Remaining playable time: s
            uint64_t bufferDuration = 0;
            if (currentBitrate_ > 0) {
                bufferDuration = bufferedDuration_ / currentBitrate_;
            } else {
                bufferDuration = bufferedDuration_ / CURRENT_BIT_RATE;
            }
            recordBuff->bufferDuring = bufferDuration;
            recordBuff->next = recordData_;
            recordData_ = recordBuff;
            recordCount_++;
        }
        // Total amount of downloaded data
        lastBits_ = totalBits_;
        lastCheckTime_ = now;
    }
    if (!isDownloadFinish_ && (static_cast<int64_t>(now) - lastReportUsageTime_) > DATA_USAGE_NTERVAL) {
        MEDIA_LOG_D("Data usage: " PUBLIC_LOG_U64 " bits in " PUBLIC_LOG_D32 "ms", dataUsage_, DATA_USAGE_NTERVAL);
        dataUsage_ = 0;
        lastReportUsageTime_ = static_cast<int64_t>(now);
    }
}

void HlsMediaDownloader::OnSourceKeyChange(const uint8_t *key, size_t keyLen, const uint8_t *iv)
{
    keyLen_ = keyLen;
    if (keyLen == 0) {
        return;
    }
    NZERO_LOG(memcpy_s(iv_, DECRYPT_UNIT_LEN, iv, DECRYPT_UNIT_LEN));
    NZERO_LOG(memcpy_s(key_, DECRYPT_UNIT_LEN, key, keyLen));
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
    statusCallback_ = cb;
    playListDownloader_->SetStatusCallback(cb);
}

std::vector<uint32_t> HlsMediaDownloader::GetBitRates()
{
    return playListDownloader_->GetBitRates();
}

bool HlsMediaDownloader::SelectBitRate(uint32_t bitRate)
{
    AutoLock lock(switchMutex_);
    if (playListDownloader_->IsBitrateSame(bitRate)) {
        return true;
    }
    // report change bitrate start
    ReportBitrateStart(bitRate);

    playListDownloader_->Cancel();

    // clear request queue
    playList_->SetActive(false, true);
    playList_->SetActive(true);
    fragmentDownloadStart.clear();
    fragmentPushed.clear();
    backPlayList_.clear();
    
    // switch to des bitrate
    playListDownloader_->SelectBitRate(bitRate);
    playListDownloader_->Start();
    isSelectingBitrate_ = true;
    playListDownloader_->UpdateManifest();
    return true;
}

void HlsMediaDownloader::SeekToTs(uint64_t seekTime, SeekMode mode)
{
    havePlayedTsNum_ = 0;
    double totalDuration = 0;
    isDownloadStarted_ = false;
    playList_->Clear();
    for (const auto &item : backPlayList_) {
        double hstTime = item.duration_ * HST_SECOND;
        totalDuration += hstTime / HST_NSECOND;
        if (seekTime >=  static_cast<uint64_t>(totalDuration)) {
            havePlayedTsNum_++;
            continue;
        }
        if (RequestNewTs(seekTime, mode, totalDuration, hstTime, item) == -1) {
            seekFailedCount_ ++;
            MEDIA_LOG_D("Seek failed count: " PUBLIC_LOG_D32, seekFailedCount_);
            continue;
        }
    }
}

uint64_t HlsMediaDownloader::RequestNewTs(uint64_t seekTime, SeekMode mode, double totalDuration,
    double hstTime, const PlayInfo& item)
{
    PlayInfo playInfo;
    playInfo.url_ = item.url_;
    playInfo.duration_ = item.duration_;
    if (mode == SeekMode::SEEK_PREVIOUS_SYNC) {
        playInfo.startTimePos_ = 0;
    } else {
        int64_t startTimePos = 0;
        double lastTotalDuration = totalDuration - hstTime;
        if (static_cast<uint64_t>(lastTotalDuration) < seekTime) {
            startTimePos = static_cast<int64_t>(seekTime) - static_cast<int64_t>(lastTotalDuration);
            if (startTimePos > (int64_t)(hstTime / 2) && (&item != &backPlayList_.back())) { // 2
                havePlayedTsNum_++;
                return -1;
            }
            startTimePos = 0;
        }
        playInfo.startTimePos_ = startTimePos;
    }
    if (!isDownloadStarted_) {
        isDownloadStarted_ = true;
        // To avoid downloader potentially stopped by curl error caused by break readbuffer blocking in seeking
        OSAL::SleepFor(6); // sleep 6ms
        PutRequestIntoDownloader(playInfo);
    } else {
        playList_->Push(playInfo);
    }
    return 0;
}

void HlsMediaDownloader::UpdateDownloadFinished(const std::string &url, const std::string& location)
{
    uint32_t bitRate = downloadRequest_->GetBitRate();
    if (!playList_->Empty()) {
        size_t fragmentSize = downloadRequest_->GetFileContentLength();
        double duration = downloadRequest_->GetDuration();
        CaculateBitRate(fragmentSize, duration);
        auto playInfo = playList_->Pop();
        PutRequestIntoDownloader(playInfo);
    } else {
        isDownloadStarted_ = false;
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
        playListDownloader_ != nullptr && playListDownloader_->IsParseAndNotifyFinished() && !isSeekingFlag) {
        AutoSelectBitrate(bitRate);
    }
}

void HlsMediaDownloader::SetReadBlockingFlag(bool isReadBlockingAllowed)
{
    MEDIA_LOG_D("SetReadBlockingFlag entered, IsReadBlockingAllowed %{public}d", isReadBlockingAllowed);
    FALSE_RETURN(buffer_ != nullptr);
    buffer_->SetReadBlocking(isReadBlockingAllowed);
}

void HlsMediaDownloader::SetIsTriggerAutoMode(bool isAuto)
{
    isAutoSelectBitrate_ = isAuto;
}

void HlsMediaDownloader::ReportVideoSizeChange()
{
    if (callback_ == nullptr) {
        MEDIA_LOG_I("callback_ == nullptr dont report video size change");
        return;
    }
    int32_t videoWidth = playListDownloader_->GetVedioWidth();
    int32_t videoHeight = playListDownloader_->GetVedioHeight();
    MEDIA_LOG_I("ReportVideoSizeChange videoWidth : " PUBLIC_LOG_D32 "videoHeight: "
        PUBLIC_LOG_D32, videoWidth, videoHeight);
    changeBitRateCount_++;
    MEDIA_LOG_I("Change bit rate count : " PUBLIC_LOG_U32, changeBitRateCount_);
    std::pair<int32_t, int32_t> videoSize {videoWidth, videoHeight};
    callback_->OnEvent({PluginEventType::VIDEO_SIZE_CHANGE, {videoSize}, "video_size_change"});
}

void HlsMediaDownloader::SetDemuxerState(int32_t streamId)
{
    MEDIA_LOG_I("SetDemuxerState");
    isReadFrame_ = true;
    isFirstFrameArrived_ = true;
}

void HlsMediaDownloader::SetDownloadErrorState()
{
    MEDIA_LOG_I("SetDownloadErrorState");
    downloadErrorState_ = true;
}

void HlsMediaDownloader::AutoSelectBitrate(uint32_t bitRate)
{
    MEDIA_LOG_I("AutoSelectBitrate download bitrate " PUBLIC_LOG_D32, bitRate);
    std::vector<uint32_t> bitRates = playListDownloader_->GetBitRates();
    if (bitRates.size() == 0) {
        return;
    }
    sort(bitRates.begin(), bitRates.end());
    uint32_t desBitRate = bitRates[0];
    for (const auto &item : bitRates) {
        if (item < bitRate * 0.8) { // 0.8
            desBitRate = item;
        } else {
            break;
        }
    }
    uint32_t curBitrate = playListDownloader_->GetCurBitrate();
    if (desBitRate == curBitrate) {
        return;
    }
    uint32_t bufferLowSize = static_cast<uint32_t>(static_cast<double>(bitRate) / 8.0 * 0.3);

    // switch to high bitrate,if buffersize less than lowsize, do not switch
    if (curBitrate < desBitRate && buffer_->GetSize() < bufferLowSize) {
        MEDIA_LOG_I("AutoSelectBitrate curBitrate " PUBLIC_LOG_D32 ", desBitRate " PUBLIC_LOG_D32
                    ", bufferLowSize " PUBLIC_LOG_D32, curBitrate, desBitRate, bufferLowSize);
        return;
    }
    uint32_t bufferHighSize = RING_BUFFER_SIZE * 0.8; // high size: buffersize * 0.8

    // switch to low bitrate, if buffersize more than highsize, do not switch
    if (curBitrate > desBitRate && buffer_->GetSize() > bufferHighSize) {
        MEDIA_LOG_I("AutoSelectBitrate curBitrate " PUBLIC_LOG_D32 ", desBitRate " PUBLIC_LOG_D32
                     ", bufferHighSize " PUBLIC_LOG_D32, curBitrate, desBitRate, bufferHighSize);
        return;
    }
    MEDIA_LOG_I("AutoSelectBitrate " PUBLIC_LOG_D32 " switch to " PUBLIC_LOG_D32, curBitrate, desBitRate);
    SelectBitRate(desBitRate);
}

bool HlsMediaDownloader::CheckRiseBufferSize()
{
    if (recordData_ == nullptr) {
        return false;
    }
    bool isHistoryLow = false;
    std::shared_ptr<RecordData> search = recordData_;
    uint64_t playingBitrate = playListDownloader_ -> GetCurrentBitRate();
    if (playingBitrate == 0) {
        playingBitrate = TransferSizeToBitRate(playListDownloader_->GetVedioWidth());
    }
    if (search->downloadRate > playingBitrate) {
        MEDIA_LOG_I("downloadRate: " PUBLIC_LOG_D64 "current bit rate: "
        PUBLIC_LOG_D64 ", increasing buffer size.", static_cast<uint64_t>(search->downloadRate), playingBitrate);
        isHistoryLow = true;
    }
    return isHistoryLow;
}

bool HlsMediaDownloader::CheckPulldownBufferSize()
{
    if (recordData_ == nullptr) {
        return false;
    }
    bool isPullDown = false;
    uint64_t playingBitrate = playListDownloader_ -> GetCurrentBitRate();
    if (playingBitrate == 0) {
        playingBitrate = TransferSizeToBitRate(playListDownloader_->GetVedioWidth());
    }
    std::shared_ptr<RecordData> search = recordData_;
    if (search->downloadRate < playingBitrate) {
        MEDIA_LOG_I("downloadRate: " PUBLIC_LOG_D64 "current bit rate: "
        PUBLIC_LOG_D64 ", reducing buffer size.", static_cast<uint64_t>(search->downloadRate), playingBitrate);
        isPullDown = true;
    }
    return isPullDown;
}

void HlsMediaDownloader::RiseBufferSize()
{
    if (totalRingBufferSize_ >= MAX_BUFFER_SIZE) {
        MEDIA_LOG_I("increasing buffer size failed, already reach the max buffer size: "
        PUBLIC_LOG_D64 ", current buffer size: " PUBLIC_LOG_ZU, MAX_BUFFER_SIZE, totalRingBufferSize_);
        return;
    }
    size_t tmpBufferSize = totalRingBufferSize_ + 1 * 1024 * 1024;
    totalRingBufferSize_ = tmpBufferSize;
    MEDIA_LOG_I("increasing buffer size: " PUBLIC_LOG_ZU, totalRingBufferSize_);
}

void HlsMediaDownloader::DownBufferSize()
{
    if (totalRingBufferSize_ <= RING_BUFFER_SIZE) {
        MEDIA_LOG_I("reducing buffer size failed, already reach the min buffer size: "
        PUBLIC_LOG_D64 ", current buffer size: " PUBLIC_LOG_ZU, RING_BUFFER_SIZE, totalRingBufferSize_);
        return;
    }
    size_t tmpBufferSize = totalRingBufferSize_ - 1 * 1024 * 1024;
    totalRingBufferSize_ = tmpBufferSize;
    MEDIA_LOG_I("reducing buffer size: " PUBLIC_LOG_ZU, totalRingBufferSize_);
}

void HlsMediaDownloader::OnReadRingBuffer(uint32_t len)
{
    static uint32_t minDuration = 0;
    uint64_t nowTime = static_cast<uint64_t>(steadyClock_.ElapsedMilliseconds());
    // len是字节 转换成bit
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
        for (int i = 0; i < MAX_RECORD_COUNT; i++) {
            if (tmpRecord->next) {
                tmpRecord = tmpRecord->next;
            } else {
                break;
            }
        }
        BufferLeastRecord* next = tmpRecord->next;
        tmpRecord->next = nullptr;
        tmpRecord = next;
        while (tmpRecord) {
            next = tmpRecord->next;
            delete tmpRecord;
            tmpRecord = next;
        }
    }
}

void HlsMediaDownloader::ActiveAutoBufferSize()
{
    if (userDefinedBufferDuration_) {
        MEDIA_LOG_I("User has already setted a buffersize, can not switch auto buffer size");
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
    if (width <= MIN_WITDH) {
        return RING_BUFFER_SIZE;
    } else if (width >= MIN_WITDH && width < SECOND_WITDH) {
        return RING_BUFFER_SIZE + RING_BUFFER_SIZE;
    } else if (width >= SECOND_WITDH && width < THIRD_WITDH) {
        return RING_BUFFER_SIZE + RING_BUFFER_SIZE + RING_BUFFER_SIZE;
    } else {
        return RING_BUFFER_SIZE + RING_BUFFER_SIZE + RING_BUFFER_SIZE + RING_BUFFER_SIZE;
    }
}

size_t HlsMediaDownloader::GetTotalBufferSize()
{
    return totalRingBufferSize_;
}

size_t HlsMediaDownloader::GetRingBufferSize()
{
    return buffer_->GetSize();
}

void HlsMediaDownloader::SetInterruptState(bool isInterruptNeeded)
{
    isInterruptNeeded_ = isInterruptNeeded;
    if (playListDownloader_ != nullptr) {
        playListDownloader_->SetInterruptState(isInterruptNeeded);
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
    double tmpDownloadTime = static_cast<double>(totalDownloadDuringTime_) / SECOND_TO_MILLIONSECOND;
    if (tmpDownloadTime > ZERO_THRESHOLD) {
        playbackInfo.averageDownloadRate = static_cast<int64_t>(totalBits_ / tmpDownloadTime);
    } else {
        playbackInfo.averageDownloadRate = 0;
    }
    playbackInfo.isDownloading = isDownloadFinish_ ? false : true;
    if (recordData_ != nullptr) {
        playbackInfo.downloadRate = static_cast<int64_t>(recordData_->downloadRate);
        size_t remainingBuffer = 0;
        if (buffer_ != nullptr) {
            remainingBuffer = buffer_->GetSize();
        }
        uint64_t bufferDuration = 0;
        if (currentBitrate_ > 0) {
            bufferDuration = static_cast<uint64_t>(remainingBuffer) / currentBitrate_;
        } else {
            bufferDuration = static_cast<uint64_t>(remainingBuffer) / CURRENT_BIT_RATE;
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
        MEDIA_LOG_I("callback_ == nullptr dont report bitrate start");
        return;
    }
    MEDIA_LOG_I("ReportBitrateStart bitRate : " PUBLIC_LOG_U32, bitRate);
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
    MEDIA_LOG_I("SetCurrentBitRate: " PUBLIC_LOG_D32, bitRate);
    if (bitRate <= 0) {
        currentBitRate_ = -1; // -1
    } else {
        currentBitRate_ = bitRate;
    }
    return Status::OK;
}

void HlsMediaDownloader::CaculateBitRate(size_t fragmentSize, double duration)
{
    if (fragmentSize == 0 || duration == 0) {
        return;
    }
    currentBitRate_ = static_cast<int32_t>(static_cast<int32_t>(fragmentSize * BYTES_TO_BIT) / duration);
    MEDIA_LOG_I("CaculateBitRate: " PUBLIC_LOG_D32, currentBitRate_);
}

int32_t HlsMediaDownloader::GetWaterLineAbove()
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

void HlsMediaDownloader::HandleCachedDuration()
{
    if (currentBitRate_ <= 0 || callback_ == nullptr) {
        return;
    }

    uint64_t cachedDuration = static_cast<uint64_t>((static_cast<int64_t>(buffer_->GetSize()) *
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

void HlsMediaDownloader::UpdateCachedPercent(BufferingInfoType infoType)
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
    int32_t bufferSize = static_cast<int32_t>(buffer_->GetSize());
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

bool HlsMediaDownloader::CheckBufferingOneSeconds()
{
    MEDIA_LOG_I("CheckBufferingOneSeconds in");
    int32_t sleepTime = 0;
    // return error again 1 time 1s, avoid ffmpeg error
    while (sleepTime < ONE_SECONDS && !isInterruptNeeded_.load()) {
        if (!isBuffering_) {
            break;
        }
        if (CheckBreakCondition()) {
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