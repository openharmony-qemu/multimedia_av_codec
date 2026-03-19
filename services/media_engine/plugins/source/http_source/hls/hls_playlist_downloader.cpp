/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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
#include <unistd.h>
#include <utility>
#include <algorithm>
#include "plugin/plugin_time.h"
#include "hls_playlist_downloader.h"
#include "common/media_source.h"

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
const std::string M3U8_X_MAP_TAG = "#EXT-X-MAP";
constexpr unsigned int MAX_LIVE_TS_NUM = 3;
constexpr uint64_t DEFAULT_SUBTITLE_SNIFFSIZE = 128;
}
// StateMachine thread: call plugin SetSource -> call Open
// StateMachine thread: call plugin GetSeekable -> call GetSeekable
// PlayListDownload thread: call ParseManifest
// First call Open, then start PlayListDownload thread, it seems no lock is required.
// [In future] StateMachine thread: call plugin GetDuration -> call GetDuration
void HlsPlayListDownloader::Open(const std::string& url, const std::map<std::string, std::string>& httpHeader)
{
    urlOrigin_ = url;
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
        downloader_->Stop(false);
    }
    aesDecryptorManager_ = nullptr;
    MEDIA_LOG_I("~HlsPlayListDownloader out");
    FALSE_RETURN_MSG(reportInfo_ != nullptr, "reportInfo_ is nullptr");
    FALSE_RETURN_MSG(currentVariant_ != nullptr, "currentVariant_ is nullptr");
    auto m3u8 = currentVariant_->m3u8_;
    FALSE_RETURN_MSG(m3u8 != nullptr, "m3u8 is nullptr");
    if (m3u8->IsLive()) {
        if (isFmp4_.load()) {
            reportInfo_->sourceType_ = static_cast<int8_t>(MediaAVCodec::DfxSourceType::FMP4LIVE);
        } else {
            reportInfo_->sourceType_ = static_cast<int8_t>(MediaAVCodec::DfxSourceType::HLSLIVE);
        }
    } else {
        if (isFmp4_.load()) {
            reportInfo_->sourceType_ = static_cast<int8_t>(MediaAVCodec::DfxSourceType::FMP4VOD);
        } else {
            reportInfo_->sourceType_ = static_cast<int8_t>(MediaAVCodec::DfxSourceType::HLSVOD);
        }
    }
}

void HlsPlayListDownloader::SetSourceStatisticsDfx(
    std::shared_ptr<OHOS::MediaAVCodec::SourceStatisticsReportInfo> rpInfoPtr, bool isFmp4)
{
    reportInfo_ = rpInfoPtr;
    isFmp4_.store(isFmp4);
}

void HlsPlayListDownloader::UpdateManifest()
{
    if (currentVariant_ && currentVariant_->m3u8_ && !currentVariant_->m3u8_->uri_.empty()) {
        isNotifyPlayListFinished_ = false;
        UpdateStreamInfo();
        OnMasterReady(needAudioManager_, needSubtitlesManager_);
        DoOpen(currentVariant_->m3u8_->uri_);
    } else {
        MEDIA_LOG_E("UpdateManifest currentVariant_ not ready.");
    }
}

void HlsPlayListDownloader::Clone(std::shared_ptr<PlayListDownloader> ptr)
{
    FALSE_RETURN_MSG(ptr != nullptr, "HlsPlayListDownloader Clone, other is nullptr.");
    auto other = std::static_pointer_cast<HlsPlayListDownloader>(ptr);

    // clone all members
    // httpHeader_ set in Open
    httpHeader_ = other->httpHeader_;

    // statusCallback_ set after Init with callback in DownloadMonitor, decide whether to retry
    statusCallback_ = other->statusCallback_;
    
    startedDownloadStatus_ = other->startedDownloadStatus_;
    fd_ = dup(other->fd_);
    offset_ = other->offset_;
    size_ = other->size_;
    fileSize_ = other->fileSize_;
    seekable_ = other->seekable_;
    position_ = other->position_;
    eventCallback_ = other->eventCallback_;
    isInterruptNeeded_.store(other->isInterruptNeeded_.load());
    isAppBackground_.store(other->isAppBackground_.load());
    url_ = other->url_;
    urlOrigin_ = other->urlOrigin_;
    mimeType_ = other->mimeType_;
    isParseFinished_.store(other->isParseFinished_.load());
    initResolution_ = other->initResolution_;
    currentVariant_ = other->currentVariant_;
    playList_ = other->playList_;
    ParseManifest("", false);
    // playList_、retryStartTime_ will clear after parse manifest completed, no need clone
}

void HlsPlayListDownloader::OnMasterReady(bool needAudioManager, bool needSubtitlesManager)
{
    auto callback = callbackWeak_.lock();
    if (currentVariant_ == nullptr || callback == nullptr) {
        return;
    }
    if (currentVariant_->m3u8_ == nullptr) {
        return;
    }
    callback->OnMasterReady(needAudioManager, needSubtitlesManager);
}

void HlsPlayListDownloader::SetPlayListCallback(std::weak_ptr<PlayListChangeCallback> callback)
{
    callbackWeak_ = callback;
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

std::pair<int64_t, bool> HlsPlayListDownloader::GetStartInfo() const
{
    if (!master_) {
        return std::make_pair(0, false);
    }
    int64_t duration = GetDuration();
    std::pair<int64_t, bool> startInfo{0, false};
    if (master_->isStart_) {
        int64_t timeOffset = ((int64_t)(master_->timeOffset_ * HST_SECOND) / HST_MSECOND);
        if (duration != -1) {
            startInfo.first = timeOffset < 0 ? std::max(timeOffset + duration, static_cast<int64_t>(0)) :
                (timeOffset > duration ? duration : timeOffset);
            startInfo.second = master_->precise_;
        }
    } else if (master_->defaultVariant_ && master_->defaultVariant_->m3u8_ &&
            master_->defaultVariant_->m3u8_->isStart_) {
        int64_t timeOffset = ((int64_t)(master_->defaultVariant_->m3u8_->timeOffset_ * HST_SECOND) / HST_MSECOND);
        if (duration != -1) {
            startInfo.first = timeOffset < 0 ? std::max(timeOffset + duration, static_cast<int64_t>(0)) :
                (timeOffset > duration ? duration : timeOffset);
            startInfo.second = master_->defaultVariant_->m3u8_->precise_;
        }
    } else {
        MEDIA_LOG_W("EXT-X-START does no exist.");
    }
    return startInfo;
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

uint64_t HlsPlayListDownloader::KeyChange(std::list<std::shared_ptr<M3U8Fragment>>& files)
{
    files = currentVariant_->m3u8_->files_;
    uint64_t sessionKeyIndex = currentVariant_->sessionKeyIndex_;
    currentVariant_->m3u8_->WaitKeyDownload();
    std::vector<KeyInfo> keyInfos;
    currentVariant_->m3u8_->GetKeyInfos(keyInfos);
    {
        std::lock_guard<std::mutex> lock(mediaMutex_);
        if (currentSubtitles_ && currentSubtitles_->m3u8_) {
            files = currentSubtitles_->m3u8_->files_;
            sessionKeyIndex = currentSubtitles_->sessionKeyIndex_;
            currentSubtitles_->m3u8_->WaitKeyDownload();
            currentSubtitles_->m3u8_->GetKeyInfos(keyInfos);
        }
        if (currentAudio_ && currentAudio_->m3u8_) {
            files = currentAudio_->m3u8_->files_;
            sessionKeyIndex = currentAudio_->sessionKeyIndex_;
            currentAudio_->m3u8_->WaitKeyDownload();
            currentAudio_->m3u8_->GetKeyInfos(keyInfos);
        }
    }
    if (aesDecryptorManager_ == nullptr) {
        aesDecryptorManager_ = std::make_shared<AesDecryptorManager>();
    }
    if (sessionKeyIndex > 0 && master_ != nullptr) {
        KeyInfo sessionKeyInfo;
        sessionKeyInfo.index_ = sessionKeyIndex;
        std::copy(std::begin(master_->key_), std::end(master_->key_), std::begin(sessionKeyInfo.key_));
        std::copy(std::begin(master_->iv_), std::end(master_->iv_), std::begin(sessionKeyInfo.iv_));
        sessionKeyInfo.keyLen_ = master_->keyLen_;
        aesDecryptorManager_->GetOneAesDecryptor(sessionKeyInfo);
    }
    aesDecryptorManager_->CreateAesDecryptorByKeyInfos(keyInfos);
    return sessionKeyIndex;
}

std::shared_ptr<AesDecryptor> HlsPlayListDownloader::GetAesDecryptor(uint64_t keyIndex)
{
    if (aesDecryptorManager_ != nullptr) {
        return aesDecryptorManager_->GetAesDecryptor(keyIndex);
    }
    return nullptr;
}

void HlsPlayListDownloader::NotifyListChange()
{
    auto callback = callbackWeak_.lock();
    if (currentVariant_ == nullptr || callback == nullptr) {
        return;
    }
    if (currentVariant_->m3u8_ == nullptr) {
        return;
    }
    std::list<std::shared_ptr<M3U8Fragment>> files;
    auto sessionKeyIndex = KeyChange(files);
    auto playList = std::vector<PlayInfo>();
    FALSE_RETURN_MSG(!isInterruptNeeded_, "HLS Seek return, isInterruptNeeded_.");
    playList.reserve(files.size());
    for (const auto &file: files) {
        if (isInterruptNeeded_.load()) {
            MEDIA_LOG_I("HLS OnPlayListChanged isInterruptNeeded.");
            break;
        }
        PlayInfo palyInfo;
        CopyFragmentInfo(palyInfo, file, sessionKeyIndex);
        playList.push_back(palyInfo);
    }
    if (!currentVariant_->m3u8_->localDrmInfos_.empty()) {
        callback->OnDrmInfoChanged(currentVariant_->m3u8_->localDrmInfos_);
    }
    if (playList.size() > MAX_LIVE_TS_NUM && isParseFinished_ && master_->bLive_) {
        playList.erase(playList.begin(), playList.end() - MAX_LIVE_TS_NUM);
    }
    callback->OnPlayListChanged(playList);
    if (isParseFinished_) {
        isNotifyPlayListFinished_ = true;
        if (master_->bLive_ && !updateTask_->IsTaskRunning() && !isLiveUpdateTaskStarted_) {
            isLiveUpdateTaskStarted_ = true;
            isPreParseFinished_.store(true);
            updateTask_->Start();
        }
    }
}

void HlsPlayListDownloader::CopyFragmentInfo(PlayInfo& playInfo, std::shared_ptr<M3U8Fragment> file,
    uint64_t sessionKeyIndex)
{
    FALSE_RETURN_MSG(file != nullptr, "file is nullptr");
    playInfo.duration_ = file->duration_;
    playInfo.offset_ = file->offset_;
    playInfo.length_ = file->length_;
    if (master_ && currentVariant_ && (master_->isFmp4_.load() || master_->isPureByteRange_.load())) {
        playInfo.url_ = std::to_string(playInfo.offset_) + "_" + std::to_string(playInfo.length_) + "_"
                        + "_" + file->uri_;
        playInfo.rangeUrl_ = file->uri_;
    } else {
        playInfo.url_ = file->uri_;
    }
    playInfo.streamId_ = GetCurStreamId();
    if (file->keyIndex_ == UINT64_MAX) {
        playInfo.keyIndex_ = 0;
    } else if (file->keyIndex_ > 0) {
        playInfo.keyIndex_ = file->keyIndex_;
    } else if (sessionKeyIndex > 0) {
        playInfo.keyIndex_ = sessionKeyIndex;
    } else {
        playInfo.keyIndex_ = 0;
    }
}

void HlsPlayListDownloader::ParseManifest(const std::string& location, bool isPreParse)
{
    if (!location.empty()) {
        url_ = location;
    }
    if (!master_) {
        std::shared_ptr<MediaSourceLoaderCombinations> sourceLoader = GetSourceLoader();
        master_ = std::make_shared<M3U8MasterPlaylist>(playList_, url_, initResolution_, httpHeader_,
                                                       statusCallback_);
        master_->SetSourceloader(sourceLoader);
        FALSE_RETURN_NOLOG(master_ != nullptr);
        if (downloadCallback_ != nullptr) {
            master_->SetDownloadCallback(downloadCallback_);
        }
        master_->StartParsing();
        currentVariant_ = master_->defaultVariant_;
        if (currentVariant_ && currentVariant_->m3u8_) {
            currentVariant_->m3u8_->httpHeader_ = httpHeader_;
        }
        if (!master_->isSimple_) {
            UpdateManifest();
        } else {
            // need notify , avoid delay 5s
            isParseFinished_ = isPreParse ? false : true;
            NotifyListChange();
        }
    } else {
        UpdateMasterAndNotifyList(isPreParse);
    }
    if (!master_->isParseSuccess_) {
        auto callback = eventCallback_.lock();
        if (callback) {
            MEDIA_LOG_E("ParseManifest parse failed.");
            callback->OnEvent({PluginEventType::CLIENT_ERROR,
                              {NetworkClientErrorCode::ERROR_TIME_OUT}, "parse m3u8"});
        }
    }
}

void HlsPlayListDownloader::UpdateMasterAndNotifyList(bool isPreParse)
{
    bool ret = false;
    if (!master_->isSimple_) {
        ret = UpdatePlaylists(false);
    }
    if (currentVariant_ && currentVariant_->m3u8_) {
        currentVariant_->m3u8_->httpHeader_ = httpHeader_;
    }
    {
        std::lock_guard<std::mutex> lock(mediaMutex_);
        if (currentSubtitles_ && currentSubtitles_->m3u8_) {
            currentSubtitles_->m3u8_->httpHeader_ = httpHeader_;
        }
        if (currentAudio_ && currentAudio_->m3u8_) {
            currentAudio_->m3u8_->httpHeader_ = httpHeader_;
        }
    }
    if (master_->isSimple_) {
        ret = UpdatePlaylists(true);
    }
    if (ret) {
        UpdateMasterInfo(isPreParse);
        if (!master_->isSimple_) {
            master_->isSimple_ = true;
        }
        NotifyListChange();
    }
}

bool HlsPlayListDownloader::UpdatePlaylists(bool isSimple)
{
    std::lock_guard<std::mutex> lock(mediaMutex_);
    bool ret = false;
    if (isSimple) {
        if (currentAudio_ && currentAudio_->m3u8_) {
            ret = currentAudio_->m3u8_->Update(playList_, isParseFinished_);
            master_->isParseSuccess_ = ret;
        } else if (currentSubtitles_ && currentSubtitles_->m3u8_) {
            ret = currentSubtitles_->m3u8_->Update(playList_, isParseFinished_);
            master_->isParseSuccess_ = ret;
        } else if (currentVariant_ && currentVariant_->m3u8_) {
            ret = currentVariant_->m3u8_->Update(playList_, isParseFinished_);
            master_->isParseSuccess_ = ret;
        } else {}
    } else {
        if (currentAudio_ && currentAudio_->m3u8_) {
            ret = currentAudio_->m3u8_->Update(playList_, true);
        } else if (currentSubtitles_ && currentSubtitles_->m3u8_) {
            ret = currentSubtitles_->m3u8_->Update(playList_, true);
        } else {
            currentVariant_ = master_->defaultVariant_;
            if (currentVariant_ && currentVariant_->m3u8_) {
                ret = currentVariant_->m3u8_->Update(playList_, true);
            }
        }
    }
    return ret;
}

bool HlsPlayListDownloader::IsLiveEnd()
{
    return isLiveEnd_;
}

void HlsPlayListDownloader::UpdateMasterInfo(bool isPreParse)
{
    std::lock_guard<std::mutex> lock(mediaMutex_);
    if (currentVariant_ == nullptr) {
        return;
    }
    auto m3u8 = currentVariant_->m3u8_;
    if (currentAudio_ && currentAudio_->m3u8_) {
        m3u8 = currentAudio_->m3u8_;
    }
    if (currentSubtitles_ && currentSubtitles_->m3u8_) {
        m3u8 = currentSubtitles_->m3u8_;
    }

    FALSE_RETURN_MSG(master_ != nullptr, "master_ is nullptr");
    FALSE_RETURN_MSG(m3u8 != nullptr, "m3u8 is nullptr");

    if (isPreParseFinished_.load() && master_->bLive_ && !m3u8->IsLive()) {
        MEDIA_LOG_I("Live stream ended and transitioning to Vod");
        updateTask_->Stop();
        isLiveEnd_ = true;
    }
    master_->bLive_ = m3u8->IsLive();
    master_->isFmp4_ = m3u8->isHeaderReady_.load();
    master_->duration_ = m3u8->GetDuration();
    master_->segmentOffsets_ = m3u8->segmentOffsets_;
    master_->hasDiscontinuity_ = m3u8->hasDiscontinuity_;
    master_->isPureByteRange_.store(m3u8->isPureByteRange_.load());
    isParseFinished_ = isPreParse ? false : true;
}

void HlsPlayListDownloader::PreParseManifest(const std::string& location)
{
    if (master_ && master_->isSimple_) {
        return;
    }
    if (playList_.find(M3U8_START_TAG) == std::string::npos ||
        playList_.find(M3U8_END_TAG) != std::string::npos ||
        playList_.find(M3U8_X_MAP_TAG) != std::string::npos) {
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
    FALSE_RETURN_V(master_ != nullptr, false);
    uint32_t maxGap = 0;
    bool isFirstSelect = true;
    for (const auto &item : master_->variants_) {
        if (item == nullptr) {
            continue;
        }
        uint32_t tempGap = (item->bandWidth_ > bitRate) ? (item->bandWidth_ - bitRate) : (bitRate - item->bandWidth_);
        if (isFirstSelect || (tempGap < maxGap)) {
            if (master_->firstVideoStream_ == nullptr) {
                isFirstSelect = false;
                maxGap = tempGap;
                newVariant_ = item;
            }
            if (item->isVideo_) {
                isFirstSelect = false;
                maxGap = tempGap;
                newVariant_ = item;
            }
        }
    }
    if (currentVariant_ != nullptr && newVariant_->bandWidth_ == currentVariant_->bandWidth_) {
        return true;
    }
    return false;
}

bool HlsPlayListDownloader::IsMediaSame(uint32_t streamId, HlsSegmentType mediaType)
{
    std::lock_guard<std::mutex> lock(mediaMutex_);
    if (mediaType == HlsSegmentType::SEG_AUDIO) {
        return currentAudio_ == nullptr || streamId == currentAudio_->streamId_;
    } else if (mediaType == HlsSegmentType::SEG_SUBTITLE) {
        return streamId == currentSubtitles_->streamId_;
    } else {
        MEDIA_LOG_W("Audio and Subtitle both Null.");
    }
    return true;
}

std::vector<uint32_t> HlsPlayListDownloader::GetBitRates()
{
    std::vector<uint32_t> bitRates;
    FALSE_RETURN_V(master_ != nullptr, bitRates);

    for (const auto &item : master_->variants_) {
        if (item) {
            bitRates.push_back(item->bandWidth_);
        }
    }
    return bitRates;
}

uint32_t HlsPlayListDownloader::GetCurBitrate()
{
    if (currentVariant_ == nullptr) {
        return 0;
    }
    return currentVariant_->bandWidth_;
}

uint64_t HlsPlayListDownloader::GetCurrentBitRate()
{
    if (currentVariant_ == nullptr) {
        return 0;
    }
    MEDIA_LOG_I("HlsPlayListDownloader currentBitrate: " PUBLIC_LOG_D64, currentVariant_->bandWidth_);
    return currentVariant_->bandWidth_;
}

int HlsPlayListDownloader::GetVideoWidth()
{
    if (currentVariant_ == nullptr) {
        return 0;
    }
    MEDIA_LOG_I("HlsPlayListDownloader currentWidth: " PUBLIC_LOG_D64, currentVariant_->bandWidth_);
    return currentVariant_->width_;
}

int HlsPlayListDownloader::GetVideoHeight()
{
    if (currentVariant_ == nullptr) {
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

size_t HlsPlayListDownloader::GetSegmentOffset(uint32_t tsIndex)
{
    if (master_ && master_->segmentOffsets_.size() > tsIndex) {
        return master_->segmentOffsets_[tsIndex];
    }
    return 0;
}

bool HlsPlayListDownloader::GetHLSDiscontinuity()
{
    if (master_) {
        return master_->hasDiscontinuity_;
    }
    return false;
}

void HlsPlayListDownloader::SetInitResolution(uint32_t width, uint32_t height)
{
    MEDIA_LOG_I("SetInitResolution, width:" PUBLIC_LOG_U32 ", height:" PUBLIC_LOG_U32, width, height);
    if (width > 0 && height > 0) {
        initResolution_ = width * height;
    }
}

size_t HlsPlayListDownloader::GetLiveUpdateGap() const
{
    if (currentVariant_ && currentVariant_->m3u8_) {
        return currentVariant_->m3u8_->GetLiveUpdateGap();
    }
    return 0;
}

void HlsPlayListDownloader::InterruptM3U8Parse(bool isInterruptNeeded)
{
    if (master_) {
        master_->SetInterruptState(isInterruptNeeded);
    }
    if (currentVariant_ && currentVariant_->m3u8_) {
        currentVariant_->m3u8_->isInterruptNeeded_.store(isInterruptNeeded);
    }
}

bool HlsPlayListDownloader::ReadFmp4Header(uint8_t* buffer, uint32_t wantLen,
    uint32_t& readLen, uint32_t streamId)
{
    if (master_ == nullptr || buffer == nullptr) {
        return false;
    }
    return ReadMediaHeader(master_->subtitlesList_, buffer, wantLen, readLen, streamId) ||
        ReadMediaHeader(master_->audioList_, buffer, wantLen, readLen, streamId) ||
        ReadStreamHeader(master_->variants_, buffer, wantLen, readLen, streamId);
}

bool HlsPlayListDownloader::ReadMediaHeader(const std::list<std::shared_ptr<M3U8Media>>& mediaList,
    uint8_t* buffer, uint32_t wantLen, uint32_t& readLen, uint32_t streamId)
{
    for (const auto& stream : mediaList) {
        if (stream == nullptr || stream->m3u8_ == nullptr || stream->streamId_ != streamId ||
            !stream->m3u8_->isHeaderReady_) {
            continue;
        }
        errno_t err = memcpy_s(buffer, wantLen, stream->m3u8_->fmp4Header_, stream->m3u8_->downloadHeaderLen_);
        if (err == 0) {
            readLen = stream->m3u8_->downloadHeaderLen_;
            return true;
        } else {
            MEDIA_LOG_E("ReadFmp4Header media, error.");
            return false;
        }
    }
    return false;
}

bool HlsPlayListDownloader::ReadStreamHeader(const std::list<std::shared_ptr<M3U8VariantStream>>& streamList,
    uint8_t* buffer, uint32_t wantLen, uint32_t& readLen, uint32_t streamId)
{
    for (const auto &stream : streamList) {
        if (stream == nullptr || stream->m3u8_ == nullptr || stream->streamId_ != streamId ||
            !stream->m3u8_->isHeaderReady_) {
            continue;
        }
        errno_t err = memcpy_s(buffer, wantLen, stream->m3u8_->fmp4Header_, stream->m3u8_->downloadHeaderLen_);
        if (err == 0) {
            readLen = stream->m3u8_->downloadHeaderLen_;
            return true;
        } else {
            MEDIA_LOG_E("ReadFmp4Header video, error.");
        }
    }
    return false;
}

void HlsPlayListDownloader::GetStreamInfo(std::vector<StreamInfo>& streams)
{
    if (currentVariant_ == nullptr || master_ == nullptr) {
        return;
    }
    GetMediaStreams(StreamType::AUDIO, streams);
    GetMediaStreams(StreamType::SUBTITLE, streams);
    for (const auto &stream : master_->variants_) {
        if (stream == nullptr) {
            continue;
        }
        StreamInfo streamInfo;
        streamInfo.streamId = static_cast<int32_t>(stream->streamId_);
        streamInfo.type = StreamType::VIDEO;
        streamInfo.bitRate = stream->bandWidth_;
        streamInfo.videoWidth = stream->width_;
        streamInfo.videoHeight = stream->height_;
        if (stream->streamId_ == currentVariant_->streamId_) {
            streams.insert(streams.begin(), streamInfo);
        } else {
            streams.push_back(streamInfo);
        }
    }
}

void HlsPlayListDownloader::GetMediaStreams(StreamType streamType, std::vector<StreamInfo>& streams)
{
    std::list<std::shared_ptr<M3U8Media>> mediaList;
    mediaList = streamType == StreamType::AUDIO ? master_->audioList_ : master_->subtitlesList_;
    for (const auto& media : mediaList) {
        if (media == nullptr) continue;
        
        StreamInfo streamInfo;
        streamInfo.streamId = static_cast<int32_t>(media->streamId_);
        streamInfo.type = streamType;
        streamInfo.lang = media->lang_;
        
        bool isDefault = false;
        if (streamType == StreamType::AUDIO) {
            isDefault = currentVariant_->defaultAudio_ != nullptr &&
                media->streamId_ == currentVariant_->defaultAudio_->streamId_;
        }
        if (streamType == StreamType::SUBTITLE) {
            isDefault = currentVariant_->defaultSubtitles_ != nullptr &&
                media->streamId_ == currentVariant_->defaultSubtitles_->streamId_;
            streamInfo.sniffSize = DEFAULT_SUBTITLE_SNIFFSIZE;
        }
        
        if (isDefault) {
            streams.insert(streams.begin(), streamInfo);
        } else {
            streams.push_back(streamInfo);
        }
    }
}

bool HlsPlayListDownloader::IsHlsFmp4()
{
    if (master_ && master_->isFmp4_) {
        return true;
    }
    return false;
}

bool HlsPlayListDownloader::IsPureByteRange()
{
    if (master_ && master_->isPureByteRange_.load()) {
        return true;
    }
    return false;
}

void HlsPlayListDownloader::ReOpen(void)
{
    // 关闭直播目录更新，清空一二级目录对象
    Pause(true);
    master_ = nullptr;
    currentVariant_ = nullptr;
    isLiveUpdateTaskStarted_ = false;
    retryStartTime_ = 0; // 重置重试时间

    // 从异常流程过来，允许重新开始下发请求
    if (downloader_ != nullptr) {
        downloader_->ReStart();
    }

    // 重新启动一级目录连接流程
    DoOpen(urlOrigin_);
}

std::shared_ptr<StreamInfo> HlsPlayListDownloader::GetStreamInfoById(int32_t streamId)
{
    if (currentVariant_ == nullptr || master_ == nullptr) {
        return nullptr;
    }
    for (const auto &variant : master_->variants_) {
        if (variant == nullptr || variant->streamId_ != static_cast<uint32_t>(streamId)) {
            continue;
        }
        auto streamInfo = std::make_shared<StreamInfo>();
        streamInfo->streamId = static_cast<int32_t>(variant->streamId_);
        streamInfo->type = StreamType::VIDEO;
        streamInfo->bitRate = variant->bandWidth_;
        streamInfo->videoWidth = variant->width_;
        streamInfo->videoHeight = variant->height_;
        return streamInfo;
    }
    return nullptr;
}

int32_t HlsPlayListDownloader::GetDefaultMediaStreamId(HlsSegmentType mediaType)
{
    if (currentVariant_ == nullptr || master_ == nullptr) {
        return -1;
    }
    if (mediaType == HlsSegmentType::SEG_AUDIO && currentVariant_->defaultAudio_ != nullptr) {
        return currentVariant_->defaultAudio_->streamId_;
    } else if (currentVariant_->defaultSubtitles_ != nullptr) {
        return currentVariant_->defaultSubtitles_->streamId_;
    } else {
        MEDIA_LOG_I("no DefaultMediaStreamId.");
    }
    return -1;
}

void HlsPlayListDownloader::SetDefaultMedia(HlsSegmentType mediaType)
{
    if (master_ == nullptr || master_->defaultVariant_ == nullptr) {
        return;
    }
    std::shared_ptr<M3U8Media> defaultMediaType;
    if (mediaType == HlsSegmentType::SEG_AUDIO && master_->defaultVariant_->defaultAudio_ != nullptr) {
        defaultMediaType = master_->defaultVariant_->defaultAudio_;
    } else if (master_->defaultVariant_->defaultSubtitles_ != nullptr) {
        defaultMediaType = master_->defaultVariant_->defaultSubtitles_;
    } else {
        return;
    }
    std::lock_guard<std::mutex> lock(mediaMutex_);
    if (mediaType == HlsSegmentType::SEG_AUDIO) {
        currentAudio_ = defaultMediaType;
    } else {
        currentSubtitles_ = defaultMediaType;
    }
}

void HlsPlayListDownloader::SelectMedia(int32_t streamId, HlsSegmentType mediaType)
{
    if (currentVariant_ == nullptr) {
        MEDIA_LOG_W("SelectMedia currentVariant_ null");
        return;
    }
    std::list<std::shared_ptr<M3U8Media>> videoStreamMedia;
    if (mediaType == HlsSegmentType::SEG_AUDIO && !currentVariant_->audioMedia_.empty()) {
        videoStreamMedia = currentVariant_->audioMedia_;
    } else if (!currentVariant_->subtitlesMedia_.empty()) {
        videoStreamMedia = currentVariant_->subtitlesMedia_;
    } else {
        MEDIA_LOG_W("SelectMedia media_ null");
        return;
    }
    for (const auto &media : videoStreamMedia) {
        if (media == nullptr) {
            continue;
        }
        if (static_cast<uint32_t>(streamId) == media->streamId_) {
            std::lock_guard<std::mutex> lock(mediaMutex_);
            if (mediaType == HlsSegmentType::SEG_AUDIO) {
                currentAudio_ = media;
            } else {
                currentSubtitles_ = media;
            }
            MEDIA_LOG_I("SelectMedia stream id: %{public}u, mediaType : %{public}d", streamId, mediaType);
            return;
        }
    }
    MEDIA_LOG_W("SelectMedia no found");
    return;
}

void HlsPlayListDownloader::UpdateMedia(HlsSegmentType mediaType)
{
    std::string nextUrl = "";
    {
        std::lock_guard<std::mutex> lock(mediaMutex_);
        if (currentVariant_ && currentVariant_->m3u8_) {
            if (mediaType == HlsSegmentType::SEG_AUDIO && currentAudio_ != nullptr) {
                nextUrl = currentAudio_->uri_;
            } else if (currentSubtitles_ != nullptr) {
                nextUrl = currentSubtitles_->uri_;
            } else {
                return;
            }
        }
    }
    if (!nextUrl.empty()) {
        DoOpen(nextUrl);
    }
    return;
}

void HlsPlayListDownloader::UpdateStreamInfo()
{
    std::lock_guard<std::mutex> lock(streamIdMutex);
    videoStreamIds_.clear();
    audioStreamIds_.clear();
    subtitlesStreamIds_.clear();
    for (const auto &stream: master_->variants_) {
        if (stream == nullptr) {
            continue;
        }
        videoStreamIds_.insert(stream->streamId_);
        for (const auto &audioMedia: stream->audioMedia_) {
            if (audioMedia == nullptr) {
                continue;
            }
            MEDIA_LOG_I("UpdateStreamInfo stream: %{public}u, audioMedia: %{public}u", stream->streamId_,
                audioMedia->streamId_);
            audioStreamIds_.insert(audioMedia->streamId_);
            needAudioManager_ = true;
        }
        for (const auto &subtitlesMedia: stream->subtitlesMedia_) {
            if (subtitlesMedia == nullptr) {
                continue;
            }
            MEDIA_LOG_I("UpdateStreamInfo stream: %{public}u, subtitlesMedia: %{public}u", stream->streamId_,
                subtitlesMedia->streamId_);
            subtitlesStreamIds_.insert(subtitlesMedia->streamId_);
            needSubtitlesManager_ = true;
        }
    }
}

HlsSegmentType HlsPlayListDownloader::GetSegType(uint32_t streamId)
{
    std::lock_guard<std::mutex> lock(streamIdMutex);
    if (audioStreamIds_.find(streamId) != audioStreamIds_.end()) {
        return HlsSegmentType::SEG_AUDIO;
    }
    if (subtitlesStreamIds_.find(streamId) != subtitlesStreamIds_.end()) {
        return HlsSegmentType::SEG_SUBTITLE;
    }
    return HlsSegmentType::SEG_VIDEO;
}

uint32_t HlsPlayListDownloader::GetCurStreamId()
{
    FALSE_RETURN_V_MSG(currentVariant_ != nullptr, 0, "GetCurStreamId no currentVariant found!");
    std::lock_guard<std::mutex> lock(mediaMutex_);
    if (currentAudio_ != nullptr) {
        return currentAudio_->streamId_;
    }
    if (currentSubtitles_ != nullptr) {
        return currentSubtitles_->streamId_;
    }
    return currentVariant_->streamId_;
}
}
}
}
}