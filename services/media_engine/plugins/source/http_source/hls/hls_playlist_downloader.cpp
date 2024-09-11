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
#define HST_LOG_TAG "HlsPlayListDownloader"
#include <mutex>
#include "plugin/plugin_time.h"
#include "hls_playlist_downloader.h"
#include "common/media_source.h"
#include <unistd.h>

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
namespace {
constexpr unsigned int SLEEP_TIME = 1;
constexpr size_t RETRY_TIMES = 1000;
constexpr int MIN_PRE_PARSE_NUM = 2; // at least 2 ts frag
const std::string M3U8_START_TAG = "#EXTM3U";
const std::string M3U8_END_TAG = "#EXT-X-ENDLIST";
const std::string M3U8_TS_TAG = "#EXTINF";
}
// StateMachine thread: call plugin SetSource -> call Open
// StateMachine thread: call plugin GetSeekable -> call GetSeekable
// PlayListDownload thread: call ParseManifest
// First call Open, then start PlayListDownload thread, it seems no lock is required.
// [In future] StateMachine thread: call plugin GetDuration -> call GetDuration
void HlsPlayListDownloader::Open(const std::string& url, const std::map<std::string, std::string>& httpHeader)
{
    url_ = url;
    master_ = nullptr;
    SaveHttpHeader(httpHeader);
    if (mimeType_ == AVMimeTypes::APPLICATION_M3U8) {
        DoOpenNative(url_);
    } else {
        DoOpen(url_);
    }
}

HlsPlayListDownloader::~HlsPlayListDownloader()
{
    MEDIA_LOG_I("~HlsPlayListDownloader in");
    if (updateTask_ != nullptr) {
        updateTask_->Stop();
    }
    if (downloader_ != nullptr) {
        downloader_ = nullptr;
    }
    MEDIA_LOG_I("~HlsPlayListDownloader out");
}

void HlsPlayListDownloader::UpdateManifest()
{
    if (currentVariant_ && currentVariant_->m3u8_ && !currentVariant_->m3u8_->uri_.empty()) {
        isNotifyPlayListFinished_ = false;
        DoOpen(currentVariant_->m3u8_->uri_);
    } else {
        MEDIA_LOG_E("UpdateManifest currentVariant_ not ready.");
    }
}

void HlsPlayListDownloader::SetPlayListCallback(PlayListChangeCallback* callback)
{
    callback_ = callback;
}

bool HlsPlayListDownloader::IsParseAndNotifyFinished()
{
    return isParseFinished_ && isNotifyPlayListFinished_;
}

bool HlsPlayListDownloader::IsParseFinished()
{
    return isParseFinished_;
}

int64_t HlsPlayListDownloader::GetDuration() const
{
    if (!master_) {
        return 0;
    }
    return master_->bLive_ ? -1 : ((int64_t)(master_->duration_ * HST_SECOND) / HST_NSECOND); // -1
}

Seekable HlsPlayListDownloader::GetSeekable() const
{
    // need wait master_ not null
    size_t times = 0;
    while (times < RETRY_TIMES && !isInterruptNeeded_) {
        if (master_ && master_->isSimple_ && isParseFinished_) {
            break;
        }
        OSAL::SleepFor(SLEEP_TIME); // 1 ms
        times++;
    }
    if (times >= RETRY_TIMES || isInterruptNeeded_) {
        return Seekable::INVALID;
    }
    return master_->bLive_ ? Seekable::UNSEEKABLE : Seekable::SEEKABLE;
}

void HlsPlayListDownloader::NotifyListChange()
{
    if (currentVariant_ == nullptr || callback_ == nullptr) {
        return;
    }
    if (currentVariant_->m3u8_ == nullptr) {
        return;
    }
    auto files = currentVariant_->m3u8_->files_;
    auto playList = std::vector<PlayInfo>();
    if (currentVariant_->m3u8_->isDecryptAble_) {
        while (!currentVariant_->m3u8_->isDecryptKeyReady_) {
            Task::SleepInTask(10); // sleep 10ms
        }
        callback_->OnSourceKeyChange(currentVariant_->m3u8_->key_, currentVariant_->m3u8_->keyLen_,
            currentVariant_->m3u8_->iv_);
    } else {
        MEDIA_LOG_E("Decrypkey is not needed.");
        if (master_ != nullptr) {
            callback_->OnSourceKeyChange(master_->key_, master_->keyLen_, master_->iv_);
        } else {
            callback_->OnSourceKeyChange(nullptr, 0, nullptr);
        }
    }
    playList.reserve(files.size());
    for (const auto &file: files) {
        PlayInfo palyInfo;
        palyInfo.url_ = file->uri_;
        palyInfo.duration_ = file->duration_;
        playList.push_back(palyInfo);
    }
    if (!currentVariant_->m3u8_->localDrmInfos_.empty()) {
        callback_->OnDrmInfoChanged(currentVariant_->m3u8_->localDrmInfos_);
    }
    callback_->OnPlayListChanged(playList);
    if (isParseFinished_) {
        isNotifyPlayListFinished_ = true;
        if (master_->bLive_ && !updateTask_->IsTaskRunning() && !isLiveUpdateTaskStarted_) {
            isLiveUpdateTaskStarted_ = true;
            updateTask_->Start();
        }
    }
}

void HlsPlayListDownloader::ParseManifest(const std::string& location, bool isPreParse)
{
    if (!location.empty()) {
        url_ = location;
    }
    if (!master_) {
        master_ = std::make_shared<M3U8MasterPlaylist>(playList_, url_);
        currentVariant_ = master_->defaultVariant_;
        if (!master_->isSimple_) {
            UpdateManifest();
        } else {
            // need notify , avoid delay 5s
            isParseFinished_ = isPreParse ? false : true;
            NotifyListChange();
        }
    } else {
        if (master_->isSimple_) {
            bool ret = currentVariant_->m3u8_->Update(playList_, isParseFinished_);
            if (ret) {
                UpdateMasterInfo(isPreParse);
                NotifyListChange();
            }
        } else {
            currentVariant_ = master_->defaultVariant_;
            bool ret = currentVariant_->m3u8_->Update(playList_, true);
            if (ret) {
                UpdateMasterInfo(isPreParse);
                master_->isSimple_ = true;
                NotifyListChange();
            }
        }
    }
}

void HlsPlayListDownloader::UpdateMasterInfo(bool isPreParse)
{
    master_->bLive_ = currentVariant_->m3u8_->IsLive();
    master_->duration_ = currentVariant_->m3u8_->GetDuration();
    isParseFinished_ = isPreParse ? false : true;
}

void HlsPlayListDownloader::PreParseManifest(const std::string& location)
{
    if (master_ && master_->isSimple_) {
        return;
    }
    if (playList_.find(M3U8_START_TAG) == std::string::npos ||
        playList_.find(M3U8_END_TAG) != std::string::npos) {
        return;
    }
    int tsNum = 0;
    int tsIndex = 0;
    int firstTsTagIndex = 0;
    int lastTsTagIndex = 0;
    std::string tsTag = M3U8_TS_TAG;
    int tsTagSize = static_cast<int>(tsTag.size());
    while ((tsIndex = static_cast<int>(playList_.find(tsTag, tsIndex))) <
            static_cast<int>(playList_.length()) && tsIndex != -1) { // -1
        if (tsNum == 0) {
            firstTsTagIndex = tsIndex;
        }
        tsNum++;
        lastTsTagIndex = tsIndex;
        if (tsNum >= MIN_PRE_PARSE_NUM) {
            break;
        }
        tsIndex += tsTagSize;
    }
    if (tsNum < MIN_PRE_PARSE_NUM) {
        return;
    }
    std::string backUpPlayList = playList_;
    playList_ = playList_.substr(0, lastTsTagIndex).append(tsTag);
    isParseFinished_ = false;
    ParseManifest(location, true);
    playList_ = backUpPlayList.erase(firstTsTagIndex, lastTsTagIndex - firstTsTagIndex);
}

void HlsPlayListDownloader::SelectBitRate(uint32_t bitRate)
{
    if (newVariant_ == nullptr) {
        return;
    }
    currentVariant_ = newVariant_;
    MEDIA_LOG_I("SelectBitRate currentVariant_ " PUBLIC_LOG_U64, currentVariant_->bandWidth_);
}

bool HlsPlayListDownloader::IsBitrateSame(uint32_t bitRate)
{
    uint32_t maxGap = 0;
    bool isFirstSelect = true;
    for (const auto &item : master_->variants_) {
        uint32_t tempGap = (item->bandWidth_ > bitRate) ? (item->bandWidth_ - bitRate) : (bitRate - item->bandWidth_);
        if (isFirstSelect || (tempGap < maxGap)) {
            isFirstSelect = false;
            maxGap = tempGap;
            newVariant_ = item;
        }
    }
    if (currentVariant_ != nullptr && newVariant_->bandWidth_ == currentVariant_->bandWidth_) {
        return true;
    }
    return false;
}

std::vector<uint32_t> HlsPlayListDownloader::GetBitRates()
{
    std::vector<uint32_t> bitRates;
    for (const auto &item : master_->variants_) {
        if (item->bandWidth_) {
            bitRates.push_back(item->bandWidth_);
        }
    }
    return bitRates;
}

uint32_t HlsPlayListDownloader::GetCurBitrate()
{
    if (currentVariant_==nullptr) {
        return 0;
    }
    return currentVariant_->bandWidth_;
}

uint64_t HlsPlayListDownloader::GetCurrentBitRate()
{
    if (currentVariant_==nullptr) {
        return 0;
    }
    MEDIA_LOG_I("HlsPlayListDownloader currentBitrate: " PUBLIC_LOG_D64, currentVariant_->bandWidth_);
    return currentVariant_->bandWidth_;
}

int HlsPlayListDownloader::GetVedioWidth()
{
    if (currentVariant_==nullptr) {
        return 0;
    }
    MEDIA_LOG_I("HlsPlayListDownloader currentWidth: " PUBLIC_LOG_D64, currentVariant_->bandWidth_);
    return currentVariant_->width_;
}

int HlsPlayListDownloader::GetVedioHeight()
{
    if (currentVariant_==nullptr) {
        return 0;
    }
    MEDIA_LOG_I("HlsPlayListDownloader currentHeight: " PUBLIC_LOG_D64, currentVariant_->bandWidth_);
    return currentVariant_->height_;
}

bool HlsPlayListDownloader::IsLive() const
{
    MEDIA_LOG_I("HlsPlayListDownloader IsLive enter.");
    if (!master_) {
        return false;
    }
    return master_->bLive_;
}

std::string HlsPlayListDownloader::GetUrl()
{
    return url_;
}

std::shared_ptr<M3U8MasterPlaylist> HlsPlayListDownloader::GetMaster()
{
    return master_;
}

std::shared_ptr<M3U8VariantStream> HlsPlayListDownloader::GetCurrentVariant()
{
    return currentVariant_;
}

std::shared_ptr<M3U8VariantStream> HlsPlayListDownloader::GetNewVariant()
{
    return newVariant_;
}

void HlsPlayListDownloader::SetMimeType(const std::string& mimeType)
{
    mimeType_ = mimeType;
}
}
}
}
}