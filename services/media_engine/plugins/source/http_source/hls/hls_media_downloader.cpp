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
}

//   hls manifest, m3u8 --- content get from m3u8 url, we get play list from the content
//   fragment --- one item in play list, download media data according to the fragment address.
HlsMediaDownloader::HlsMediaDownloader() noexcept
{
    buffer_ = std::make_shared<RingBuffer>(RING_BUFFER_SIZE);
    buffer_->Init();

    downloader_ = std::make_shared<Downloader>("hlsMedia");
    playList_ = std::make_shared<BlockingQueue<PlayInfo>>("PlayList", 5000); // 5000 to prevent blocking download

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
    auto downloadDoneCallback = [this] (std::string url) {
        UpdateDownloadFinished(url);
    };
    // TO DO: If the fragment file is too large, should not requestWholeFile.
    downloadRequest_ = std::make_shared<DownloadRequest>(playInfo.url_, playInfo.duration_, dataSave_,
                                                         realStatusCallback, true);
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

bool HlsMediaDownloader::Open(const std::string& url)
{
    playListDownloader_->Open(url);
    return true;
}

void HlsMediaDownloader::Close(bool isAsync)
{
    buffer_->SetActive(false);
    playList_->SetActive(false);
    playListDownloader_->Cancel();
    playListDownloader_->Close();
    downloader_->Cancel();
}

void HlsMediaDownloader::Pause()
{
    bool cleanData = GetSeekable() != Seekable::SEEKABLE;
    buffer_->SetActive(false, cleanData);
    playList_->SetActive(false, cleanData);
    playListDownloader_->Pause();
    downloader_->Pause();
}

void HlsMediaDownloader::Resume()
{
    buffer_->SetActive(true);
    playList_->SetActive(true);
    playListDownloader_->Resume();
    downloader_->Resume();
}

bool HlsMediaDownloader::Read(unsigned char* buff, unsigned int wantReadLength,
                              unsigned int& realReadLength, bool& isEos)
{
    FALSE_RETURN_V(buffer_ != nullptr, false);
    if (playList_->Empty() && (downloadRequest_ != nullptr) &&
         downloadRequest_->IsEos() && (playListDownloader_->GetDuration() > 0)) {
        isEos = true;
        realReadLength = 0;
        MEDIA_LOG_I("HLS read Eos.");
        return false;
    }
    realReadLength = buffer_->ReadBuffer(buff, wantReadLength, 2); // wait 2 times
    MEDIA_LOG_D("Read: wantReadLength " PUBLIC_LOG_D32 ", realReadLength " PUBLIC_LOG_D32 ", isEos "
                PUBLIC_LOG_D32, wantReadLength, realReadLength, isEos);
    return true;
}

bool HlsMediaDownloader::SeekToTime(int64_t seekTime)
{
    FALSE_RETURN_V(buffer_ != nullptr, false);
    MEDIA_LOG_I("Seek: buffer size " PUBLIC_LOG_ZU ", seekTime " PUBLIC_LOG_D64, buffer_->GetSize(), seekTime);
    buffer_->Clear(); // First clear buffer, avoid no available buffer then task pause never exit.
    downloader_->Cancel();
    buffer_->Clear();
    downloader_->Start();
    SeekToTs(seekTime);
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
    for (uint32_t i = 0; i < playList.size(); i++) {
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
}

bool HlsMediaDownloader::GetStartedStatus()
{
    return playListDownloader_->GetPlayListDownloadStatus() && startedPlayStatus_;
}

bool HlsMediaDownloader::SaveData(uint8_t* data, uint32_t len)
{
    startedPlayStatus_ = true;
    uint32_t writeLen = 0;
    uint8_t *writeDataPoint = data;
    uint32_t waitLen = len;

    if (keyLen_ == 0) {
        return buffer_->WriteBuffer(data, len);
    }

    if ((len + afterAlignRemainedLength_) < DECRYPT_UNIT_LEN) {
        memcpy_s(afterAlignRemainedBuffer_ + afterAlignRemainedLength_, DECRYPT_UNIT_LEN - afterAlignRemainedLength_,
            data, len);
        afterAlignRemainedLength_ += len;
        return true;
    }

    writeLen =
        ((waitLen + afterAlignRemainedLength_) / DECRYPT_UNIT_LEN) * DECRYPT_UNIT_LEN - afterAlignRemainedLength_;

    memcpy_s(decryptBuffer_, afterAlignRemainedLength_, afterAlignRemainedBuffer_, afterAlignRemainedLength_);
    uint32_t minWriteLen = (RING_BUFFER_SIZE - afterAlignRemainedLength_) > writeLen ?
                           writeLen : RING_BUFFER_SIZE - afterAlignRemainedLength_;
    memcpy_s(decryptBuffer_ + afterAlignRemainedLength_, minWriteLen, writeDataPoint, minWriteLen);

    // decry buffer data
    uint32_t realLen = writeLen + afterAlignRemainedLength_;
    AES_cbc_encrypt(decryptBuffer_, decryptCache_, realLen, &aesKey_, iv_, AES_DECRYPT);
    totalLen_ += realLen;
    bool ret = buffer_->WriteBuffer(decryptCache_, realLen);
    memset_s(decryptCache_, realLen, 0x00, realLen);
    if (!ret) {
        return false;
    }
    afterAlignRemainedLength_ = 0;
    memset_s(afterAlignRemainedBuffer_, DECRYPT_UNIT_LEN, 0x00, DECRYPT_UNIT_LEN);
    writeDataPoint += writeLen;
    waitLen -= writeLen;
    if (waitLen > 0) {
        afterAlignRemainedLength_ = waitLen;
        memcpy_s(afterAlignRemainedBuffer_, DECRYPT_UNIT_LEN, writeDataPoint, waitLen);
    }
    return true;
}

void HlsMediaDownloader::OnSourceKeyChange(const uint8_t *key, size_t keyLen, const uint8_t *iv)
{
    keyLen_ = keyLen;
    if (keyLen == 0) {
        return;
    }
    memcpy_s(iv_, DECRYPT_UNIT_LEN, iv, DECRYPT_UNIT_LEN);
    memcpy_s(key_, DECRYPT_UNIT_LEN, key, keyLen);
    AES_set_decrypt_key(key_, DECRYPT_COPY_LEN, &aesKey_);
}

void HlsMediaDownloader::OnDrmInfoChanged(const std::multimap<std::string, std::vector<uint8_t>> drmInfos)
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
    return 1;
}

void HlsMediaDownloader::SeekToTs(int64_t seekTime)
{
    havePlayedTsNum_ = 0;
    int64_t totalDuration = 0;
    isDownloadStarted_ = false;
    playList_->Clear();
    for (const auto &item : backPlayList_) {
        int64_t hstTime;
        Sec2HstTime(item.duration_, hstTime);
        totalDuration += HstTime2Ns(hstTime);
        if (seekTime >= totalDuration) {
            havePlayedTsNum_++;
            continue;
        }
        PlayInfo playInfo;
        playInfo.url_ = item.url_;
        playInfo.duration_ = item.duration_;
        int64_t startTimePos = 0;
        int64_t lastTotalDuration = totalDuration - hstTime;
        if (lastTotalDuration < seekTime) {
            startTimePos = seekTime - lastTotalDuration;
            if (startTimePos > hstTime / 2) { // 2
                havePlayedTsNum_++;
                continue;
            }
            startTimePos = 0;
        }
        playInfo.startTimePos_ = startTimePos;
        if (!isDownloadStarted_) {
            isDownloadStarted_ = true;
            PutRequestIntoDownloader(playInfo);
        } else {
            playList_->Push(playInfo);
        }
    }
}

void HlsMediaDownloader::UpdateDownloadFinished(std::string url)
{
    uint32_t bitRate = downloadRequest_->GetBitRate();
    if (!playList_->Empty()) {
        auto playInfo = playList_->Pop();
        PutRequestIntoDownloader(playInfo);
    } else {
        isDownloadStarted_ = false;
    }
    if ((bitRate > 0) && !isSelectingBitrate_ && isAutoSelectBitrate_) {
        MEDIA_LOG_I("SelectBitRate(auto) not support.");
    }
}

void HlsMediaDownloader::SetIsTriggerAutoMode(bool isAuto)
{
    isAutoSelectBitrate_ = isAuto;
}
}
}
}
}