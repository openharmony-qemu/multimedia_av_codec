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

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
namespace {
constexpr uint32_t DECRYPT_COPY_LEN = 128;
constexpr int32_t TIME_OUT = 3 * 1000;
constexpr int MIN_WITDH = 480;
constexpr int SECOND_WITDH = 720;
constexpr int THIRD_WITDH = 1080;
constexpr uint64_t MAX_BUFFER_SIZE = 20 * 1024 * 1024;
constexpr int RECORD_TIME_INTERVAL = 3000;
constexpr uint32_t SAMPLE_INTERVAL = 6000;
constexpr int MAX_RECORD_COUNT = 10;
}

//   hls manifest, m3u8 --- content get from m3u8 url, we get play list from the content
//   fragment --- one item in play list, download media data according to the fragment address.
HlsMediaDownloader::HlsMediaDownloader() noexcept
{
    buffer_ = std::make_shared<RingBuffer>(RING_BUFFER_SIZE);
    buffer_->Init();
    totalRingBufferSize_ = RING_BUFFER_SIZE;
    downloader_ = std::make_shared<Downloader>("hlsMedia");
    playList_ = std::make_shared<BlockingQueue<PlayInfo>>("PlayList", 5000); // 5000 to prevent blocking download

    dataSave_ =  [this] (uint8_t*&& data, uint32_t&& len) {
        return SaveData(std::forward<decltype(data)>(data), std::forward<decltype(len)>(len));
    };
    playListDownloader_ = std::make_shared<HlsPlayListDownloader>();
    playListDownloader_->SetPlayListCallback(this);
    steadyClock_.Reset();
}

HlsMediaDownloader::HlsMediaDownloader(int expectBufferDuration)
{
    expectDuration_ = static_cast<uint64_t>(expectBufferDuration);
    userDefinedBufferDuration_ = true;
    totalRingBufferSize_ = expectDuration_ * currentBitrate_;
    MEDIA_LOG_I("user define buffer duration.");
    downloader_ = std::make_shared<Downloader>("hlsMedia");
    playList_ = std::make_shared<BlockingQueue<PlayInfo>>("PlayList", 5000); // 5000 to prevent blocking download
    steadyClock_.Reset();
    dataSave_ =  [this] (uint8_t*&& data, uint32_t&& len) {
        return SaveData(std::forward<decltype(data)>(data), std::forward<decltype(len)>(len));
    };

    playListDownloader_ = std::make_shared<HlsPlayListDownloader>();
    playListDownloader_->SetPlayListCallback(this);
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
    MEDIA_LOG_I("Open enter");
    SaveHttpHeader(httpHeader);
    playListDownloader_->Open(url, httpHeader);
    steadyClock_.Reset();
    if (userDefinedBufferDuration_) {
        MEDIA_LOG_I("User seeting buffer duration playListDownloader_ opened.");
        totalRingBufferSize_ = expectDuration_ * currentBitrate_;
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
    buffer_->SetActive(false);
    playList_->SetActive(false);
    playListDownloader_->Cancel();
    playListDownloader_->Close();
    downloader_->Cancel();
    downloader_->Stop(isAsync);
    isStopped = true;
}

void HlsMediaDownloader::Pause()
{
    MEDIA_LOG_I("Pause enter");
    bool cleanData = GetSeekable() != Seekable::SEEKABLE;
    buffer_->SetActive(false, cleanData);
    playList_->SetActive(false, cleanData);
    playListDownloader_->Pause();
    downloader_->Pause();
}

void HlsMediaDownloader::Resume()
{
    MEDIA_LOG_I("Resume enter");
    buffer_->SetActive(true);
    playList_->SetActive(true);
    playListDownloader_->Resume();
    downloader_->Resume();
}

bool HlsMediaDownloader::CheckReadStatus()
{
    if (buffer_->GetSize() == 0 && playList_->Empty() && (downloadRequest_ != nullptr) &&
        downloadRequest_->IsEos() && (playListDownloader_->GetDuration() > 0)) {
        MEDIA_LOG_I("HLS read Eos.");
        return true;
    }
    if (playListDownloader_->GetDuration() > 0 && seekTime_ >= playListDownloader_->GetDuration()) {
        MEDIA_LOG_I("HLS read Eos.");
        return true;
    }
    return false;
}

bool HlsMediaDownloader::CheckReadTimeOut()
{
    if (readTime_ >= TIME_OUT || downloadErrorState_ || isTimeOut_) {
        isTimeOut_ = true;
        if (downloader_ != nullptr) {
            downloader_->Pause();
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

bool HlsMediaDownloader::Read(unsigned char* buff, unsigned int wantReadLength,
                              unsigned int& realReadLength, bool& isEos)
{
    FALSE_RETURN_V(buffer_ != nullptr, false);
    if (CheckReadStatus()) {
        isEos = true;
        realReadLength = 0;
        return false;
    }
    readTime_ = 0;
    while (buffer_->GetSize() < wantReadLength && !isInterruptNeeded_.load()) {
        bool isFinishedPlay = (playList_->Empty() && (downloadRequest_ != nullptr) &&
                               downloadRequest_->IsEos()) || isStopped;
        if (downloadRequest_ != nullptr) {
            isEos = downloadRequest_->IsEos();
        }
        if (isFinishedPlay && buffer_->GetSize() > 0) {
            realReadLength = buffer_->ReadBuffer(buff, buffer_->GetSize(), 2);  // wait 2 times
            return true;
        }
        if (isFinishedPlay && buffer_->GetSize() == 0) {
            realReadLength = 0;
            return false;
        }
        if (CheckReadTimeOut()) {
            realReadLength = 0;
            return false;
        }
        OSAL::SleepFor(5);  // 5
        readTime_ += 5;
    }
    realReadLength = buffer_->ReadBuffer(buff, wantReadLength, 2);  // wait 2 times
    MEDIA_LOG_D("Read: wantReadLength " PUBLIC_LOG_D32 ", realReadLength " PUBLIC_LOG_D32 ", isEos "
                PUBLIC_LOG_D32, wantReadLength, realReadLength, isEos);
    return true;
}

bool HlsMediaDownloader::SeekToTime(int64_t seekTime, SeekMode mode)
{
    FALSE_RETURN_V(buffer_ != nullptr, false);
    MEDIA_LOG_I("Seek: buffer size " PUBLIC_LOG_ZU ", seekTime " PUBLIC_LOG_D64, buffer_->GetSize(), seekTime);
    seekTime_ = static_cast<int64_t>(seekTime);
    buffer_->SetActive(false);
    downloader_->Cancel();
    buffer_->Clear();
    buffer_->SetActive(true);
    SeekToTs(seekTime, mode);
    MEDIA_LOG_I("SeekToTime end\n");
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

void HlsMediaDownloader::OnPlayListChanged(const std::vector<PlayInfo>& playList)
{
    AutoLock lock(firstTsMutex_);
    for (int i = 0; i < static_cast<int>(playList.size()); i++) {
        auto fragment = playList[i];
        auto ret = std::find_if(backPlayList_.begin(), backPlayList_.end(), [&](PlayInfo playInfo) {
                   return playInfo.url_ == fragment.url_;
        });
        if (ret == backPlayList_.end()) {
            backPlayList_.push_back(fragment);
        }
        if (isSelectingBitrate_ && (GetSeekable() == Seekable::SEEKABLE)) {
            bool isFileIndexSame = (havePlayedTsNum_ - i) == 1 ? true : false; // 1
            if (isFileIndexSame) {
                MEDIA_LOG_I("Switch bitrate, start ts file is: " PUBLIC_LOG_S, fragment.url_.c_str());
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
    isNeedStopPlayListTask_ = true;
}

bool HlsMediaDownloader::GetStartedStatus()
{
    return playListDownloader_->GetPlayListDownloadStatus() && startedPlayStatus_;
}

bool HlsMediaDownloader::SaveData(uint8_t* data, uint32_t len)
{
    if (autoBufferSize_ && !userDefinedBufferDuration_) {
        OnWriteRingBuffer(len);
    }
    startedPlayStatus_ = true;
    if (keyLen_ == 0) {
        return buffer_->WriteBuffer(data, len);
    } else {
        return SaveEncryptData(data, len);
    }
}

bool HlsMediaDownloader::SaveEncryptData(uint8_t* data, uint32_t len)
{
    uint32_t writeLen = 0;
    uint8_t *writeDataPoint = data;
    uint32_t waitLen = len;
    errno_t err {0};
    if ((len + afterAlignRemainedLength_) < DECRYPT_UNIT_LEN) {
        err = memcpy_s(afterAlignRemainedBuffer_ + afterAlignRemainedLength_, DECRYPT_UNIT_LEN -
            afterAlignRemainedLength_, data, len);
        if (err!=0) {
            return false;
        }
        afterAlignRemainedLength_ += len;
        return true;
    }
    writeLen =
        ((waitLen + afterAlignRemainedLength_) / DECRYPT_UNIT_LEN) * DECRYPT_UNIT_LEN - afterAlignRemainedLength_;
    err = memcpy_s(decryptBuffer_, afterAlignRemainedLength_, afterAlignRemainedBuffer_, afterAlignRemainedLength_);
    if (err!=0) {
        return false;
    }
    uint32_t minWriteLen = (RING_BUFFER_SIZE - afterAlignRemainedLength_) > writeLen ?
                            writeLen : RING_BUFFER_SIZE - afterAlignRemainedLength_;
    err = memcpy_s(decryptBuffer_ + afterAlignRemainedLength_, minWriteLen, writeDataPoint, minWriteLen);
    if (err!=0) {
        return false;
    }
    uint32_t realLen = writeLen + afterAlignRemainedLength_;
    AES_cbc_encrypt(decryptBuffer_, decryptCache_, realLen, &aesKey_, iv_, AES_DECRYPT);
    totalLen_ += realLen;
    buffer_->WriteBuffer(decryptCache_, len);
    err = memset_s(decryptCache_, realLen, 0x00, realLen);
    if (err!=0) {
        return false;
    }
    afterAlignRemainedLength_ = 0;
    err = memset_s(afterAlignRemainedBuffer_, DECRYPT_UNIT_LEN, 0x00, DECRYPT_UNIT_LEN);
    if (err!=0) {
        return false;
    }
    writeDataPoint += writeLen;
    waitLen -= writeLen;
    if (waitLen > 0) {
        afterAlignRemainedLength_ = waitLen;
        err = memcpy_s(afterAlignRemainedBuffer_, DECRYPT_UNIT_LEN, writeDataPoint, waitLen);
        if (err!=0) {
            return false;
        }
    }
    return true;
}

void HlsMediaDownloader::OnWriteRingBuffer(uint32_t len)
{
    int64_t nowTime = steadyClock_.ElapsedMilliseconds();
    uint32_t writeBits = len * 8;
    bufferedDuration_ += writeBits;
    totalBits_ += writeBits;
    lastWriteBit_ += writeBits;
    if ((nowTime - lastWriteTime_) >= RECORD_TIME_INTERVAL) {
        MEDIA_LOG_I("OnWriteRingBuffer nowTime: " PUBLIC_LOG_D64
        " lastWriteTime:" PUBLIC_LOG_D64 ".\n", nowTime, lastWriteTime_);
        BufferDownRecord* record = new BufferDownRecord();
        record->dataBits = lastWriteBit_;
        record->timeoff = nowTime - lastWriteTime_;
        record->next = bufferDownRecord_;
        bufferDownRecord_ = record;
        lastWriteBit_ = 0;
        lastWriteTime_ = nowTime;

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
        if (CheckRiseBufferSize()) {
            RiseBufferSize();
        } else if (CheckPulldownBufferSize()) {
            DownBufferSize();
        }
    }
    DownloadReportLoop();
}

constexpr int IS_DOWNLOAD_MIN_BIT = 1000;     // 判断下载是否在进行的阈值 bit
uint64_t lastCheckTime_ {0};
uint32_t record_count_ {0};
int64_t lastRecordTime_ {0};
void HlsMediaDownloader::DownloadReportLoop()
{
    int64_t now = static_cast<uint64_t>(steadyClock_.ElapsedMilliseconds());
    if ((now - lastCheckTime_) > RECORD_TIME_INTERVAL) {
        uint64_t curDownloadBits = totalBits_ - lastBits_;
        if (curDownloadBits >= IS_DOWNLOAD_MIN_BIT) {
            // 周期下载量达阈值，统计有效下载时长
            downloadDuringTime_ += (now - lastCheckTime_)<0? 0 : static_cast<uint64_t>(now - lastCheckTime_);
            // 有效下载数据量
            downloadBits_ += curDownloadBits;
        }
        // 下载总数据量
        lastBits_ = totalBits_;
        lastCheckTime_ = now;
    }

    if ((now - lastRecordTime_) > SAMPLE_INTERVAL) {
        std::shared_ptr<RecordData> recordBuff = std::make_shared<RecordData>();
        if (downloadDuringTime_ > 0) {
            double tmpNumerator = static_cast<double>(downloadBits_);
            double tmpDenominator = static_cast<double>(downloadDuringTime_) / 1000;
            double downloadRate = tmpNumerator / tmpDenominator;
            recordBuff->downloadRate = downloadRate;
        } else {
            recordBuff->downloadRate = 0;
        }
        // 缓冲区剩余时长
        uint64_t bufferDuration = bufferedDuration_ / currentBitrate_;
        recordBuff->bufferDuring = bufferDuration;
        recordBuff->next = recordData_;
        recordData_ = recordBuff;
        record_count_++;
        downloadDuringTime_ = 0;
        downloadBits_ = 0;
        lastRecordTime_ = now;
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
    if (playListDownloader_->IsBitrateSame(bitRate)) {
        return 1;
    }
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

    // report video size change
    ReportVideoSizeChange();
    return 1;
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
        if (seekTime >= (int64_t)totalDuration) {
            havePlayedTsNum_++;
            continue;
        }
        if (RequestNewTs(seekTime, mode, totalDuration, hstTime, item) == -1) {
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
        if (static_cast<int64_t>(lastTotalDuration) < seekTime) {
            startTimePos = seekTime - static_cast<int64_t>(lastTotalDuration);
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
    if (isNeedStopPlayListTask_ && GetSeekable() == Seekable::SEEKABLE) {
        MEDIA_LOG_I("Stop playlist task enter.");
        playListDownloader_->Cancel();
        playListDownloader_->Close();
        isNeedStopPlayListTask_ = false;
        MEDIA_LOG_I("Stop playlist task exit.");
    }
    uint32_t bitRate = downloadRequest_->GetBitRate();
    if (!playList_->Empty()) {
        auto playInfo = playList_->Pop();
        PutRequestIntoDownloader(playInfo);
    } else {
        isDownloadStarted_ = false;
    }
    if ((bitRate > 0) && !isSelectingBitrate_ && isAutoSelectBitrate_) {
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

void HlsMediaDownloader::SetDemuxerState()
{
    MEDIA_LOG_I("SetDemuxerState");
    isReadFrame_ = true;
}

void HlsMediaDownloader::SetDownloadErrorState()
{
    MEDIA_LOG_I("SetDownloadErrorState");
    downloadErrorState_ = true;
}
void HlsMediaDownloader::ReportVideoSizeChange()
{
    if (callback_ == nullptr) {
        MEDIA_LOG_I("callback_ == nullptr dont report video size change");
        return;
    }
    int32_t videoWidth = playListDownloader_->GetVideoWidth();
    int32_t videoHeight = playListDownloader_->GetVideoHeight();
    MEDIA_LOG_I("ReportVideoSizeChange videoWidth : " PUBLIC_LOG_D32 "videoHeight: "
        PUBLIC_LOG_D32, videoWidth, videoHeight);
    std::pair<int32_t, int32_t> videoSize {videoWidth, videoHeight};
    callback_->OnEvent({PluginEventType::VIDEO_SIZE_CHANGE, {videoSize}, "video_size_change"});
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
    int64_t nowTime = steadyClock_.ElapsedMilliseconds();
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
    if ((nowTime - lastReadTime_) >= RECORD_TIME_INTERVAL || bufferedDuration_ == 0) {
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

void HlsMediaDownloader::OnFirstTsReady(const std::string& url, const double& duration)
{
    AutoLock lock(firstTsMutex_);
    if (isDownloadStarted_) {
        return;
    }
    PlayInfo playInfo;
    playInfo.url_ = url;
    playInfo.duration_ = duration;
    fragmentDownloadStart[playInfo.url_] = true;
    fragmentPushed[playInfo.url_] = true;
    isDownloadStarted_ = true;
    PutRequestIntoDownloader(playInfo);
}

void HlsMediaDownloader::SetInterruptState(bool isInterruptNeeded)
{
    isInterruptNeeded_ = isInterruptNeeded;
    if (playListDownloader_ != nullptr) {
        playListDownloader_->SetInterruptState(isInterruptNeeded);
    }
}
}
}
}
}