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
    MEDIA_LOG_I("~HlsPlayListDownloader out");
}

void HlsPlayListDownloader::UpdateManifest()
{
    if (currentVariant_ && currentVariant_->m3u8_ && !currentVariant_->m3u8_->uri_.empty()) {
        isNotifyPlayListFinished_ = false;
        UpdateStreamInfo();
        OnMasterReady(needAudioManager_, needSubTitleManager_);
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

void HlsPlayListDownloader::OnMasterReady(bool needAudioManager, bool needSubTitleManager)
{
    auto callback = callbackWeak_.lock();
    if (currentVariant_ == nullptr || callback == nullptr) {
        return;
    }
    if (currentVariant_->m3u8_ == nullptr) {
        return;
    }
    callback->OnMasterReady(needAudioManager, needSubTitleManager);
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

void HlsPlayListDownloader::KeyChange(void)
{
    auto callback = callbackWeak_.lock();
    if (currentVariant_ == nullptr || callback == nullptr) {
        return;
    }

    if (currentVariant_->m3u8_->isDecryptAble_) {
        int32_t times = 0;
        while (!currentVariant_->m3u8_->isDecryptKeyReady_ && !isInterruptNeeded_) {
            Task::SleepInTask(5); // sleep 5ms
            if ((times++) >= RETRY_TIMES) {
                MEDIA_LOG_E("Download decrypkey failed.");
                break;
            }
        }
        callback->OnSourceKeyChange(currentVariant_->m3u8_->key_, currentVariant_->m3u8_->keyLen_,
            currentVariant_->m3u8_->iv_);
    } else {
        MEDIA_LOG_E("Decrypkey is not needed.");
        if (master_ != nullptr) {
            callback->OnSourceKeyChange(master_->key_, master_->keyLen_, master_->iv_);
        } else {
            callback->OnSourceKeyChange(nullptr, 0, nullptr);
        }
    }
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
    auto files = currentVariant_->m3u8_->files_;
    {
        std::lock_guard<std::mutex> lock(audioMutex_);
        if (currentAudio_ != nullptr && currentAudio_->m3u8_ != nullptr) {
            files = currentAudio_->m3u8_->files_;
        }
    }
    auto playList = std::vector<PlayInfo>();

    KeyChange();

    FALSE_RETURN_MSG(!isInterruptNeeded_, "HLS Seek return, isInterruptNeeded_.");
    playList.reserve(files.size());
    for (const auto &file: files) {
        if (isInterruptNeeded_.load()) {
            MEDIA_LOG_I("HLS OnPlayListChanged isInterruptNeeded.");
            break;
        }
        PlayInfo palyInfo;
        CopyFragmentInfo(palyInfo, file);
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
            updateTask_->Start();
        }
    }
}

void HlsPlayListDownloader::CopyFragmentInfo(PlayInfo& playInfo, std::shared_ptr<M3U8Fragment> file)
{
    playInfo.duration_ = file->duration_;
    playInfo.offset_ = file->offset_;
    playInfo.length_ = file->length_;
    if (master_ && currentVariant_ && (master_->isFmp4_.load() || master_->isPureByteRange_.load())) {
        playInfo.url_ = std::to_string(playInfo.offset_) + "_" + std::to_string(playInfo.length_) + "_"
                        + "_" + file->uri_;
        playInfo.rangeUrl_ = file->uri_;
        std::lock_guard<std::mutex> lock(audioMutex_);
        playInfo.streamId_ = currentAudio_ != nullptr ? currentAudio_->streamId_ : currentVariant_->streamId_;
    } else {
        playInfo.url_ = file->uri_;
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
    if (!master_->isParseSuccess_ && eventCallback_ != nullptr) {
        MEDIA_LOG_E("ParseManifest parse failed.");
        eventCallback_->OnEvent({PluginEventType::CLIENT_ERROR,
                                {NetworkClientErrorCode::ERROR_TIME_OUT}, "parse m3u8"});
    }
}

void HlsPlayListDownloader::UpdateMasterAndNotifyList(bool isPreParse)
{
    bool ret = false;
    if (!master_->isSimple_) {
        std::lock_guard<std::mutex> lock(audioMutex_);
        if (currentAudio_ == nullptr) {
            currentVariant_ = master_->defaultVariant_;
            if (currentVariant_ && currentVariant_->m3u8_) {
                ret = currentVariant_->m3u8_->Update(playList_, true);
            }
        } else {
            if (currentAudio_->m3u8_ != nullptr) {
                ret = currentAudio_->m3u8_->Update(playList_, true);
            }
        }
    }
    if (currentVariant_ && currentVariant_->m3u8_) {
        currentVariant_->m3u8_->httpHeader_ = httpHeader_;
    }
    {
        std::lock_guard<std::mutex> lock(audioMutex_);
        if (currentAudio_ && currentAudio_->m3u8_) {
            currentAudio_->m3u8_->httpHeader_ = httpHeader_;
        }
    }
    if (master_->isSimple_) {
        std::lock_guard<std::mutex> lock(audioMutex_);
        if (currentAudio_ && currentAudio_->m3u8_) {
            ret = currentAudio_->m3u8_->Update(playList_, isParseFinished_);
            master_->isParseSuccess_ = ret;
        } else if (currentVariant_ && currentVariant_->m3u8_) {
            ret = currentVariant_->m3u8_->Update(playList_, isParseFinished_);
            master_->isParseSuccess_ = ret;
        }
    }
    if (ret) {
        UpdateMasterInfo(isPreParse);
        if (!master_->isSimple_) {
            master_->isSimple_ = true;
        }
        NotifyListChange();
    }
}

void HlsPlayListDownloader::UpdateMasterInfo(bool isPreParse)
{
    std::lock_guard<std::mutex> lock(audioMutex_);
    auto m3u8 = currentAudio_ == nullptr ? currentVariant_->m3u8_ : currentAudio_->m3u8_;
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

bool HlsPlayListDownloader::IsAudioSame(uint32_t streamId)
{
    std::lock_guard<std::mutex> lock(audioMutex_);
    FALSE_RETURN_V(currentAudio_ != nullptr, true);
    return streamId == currentAudio_->streamId_;
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

bool HlsPlayListDownloader::ReadFmp4Header(uint8_t* buffer, uint32_t wantLen, uint32_t& readLen, uint32_t streamId)
{
    if (master_ == nullptr || buffer == nullptr) {
        return false;
    }
    for (const auto &media : master_->mediaList_) {
        if (media == nullptr || media->streamId_ != streamId || !media->m3u8_->isHeaderReady_) {
            continue;
        }
        errno_t err = memcpy_s(buffer, wantLen, media->m3u8_->fmp4Header_,
            media->m3u8_->downloadHeaderLen_);
        if (err == 0) {
            readLen = media->m3u8_->downloadHeaderLen_;
            return true;
        } else {
            MEDIA_LOG_E("ReadFmp4Header audio, error.");
            return false;
        }
    }
    for (const auto &stream : master_->variants_) {
        if (stream == nullptr || stream->streamId_ != streamId || !stream->m3u8_->isHeaderReady_) {
            continue;
        }
        errno_t err = memcpy_s(buffer, wantLen, stream->m3u8_->fmp4Header_,
            stream->m3u8_->downloadHeaderLen_);
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
    if (!master_->isFmp4_) {
        return;
    }

    for (const auto &audioStream : master_->mediaList_) {
        if (audioStream == nullptr) {
            continue;
        }
        StreamInfo streamInfo;
        streamInfo.streamId = static_cast<int32_t>(audioStream->streamId_);
        streamInfo.type = StreamType::AUDIO;
        streamInfo.lang = audioStream->lang_;
        if (currentVariant_->defaultMedia_ != nullptr &&
            audioStream->streamId_ == currentVariant_->defaultMedia_->streamId_) {
            streams.insert(streams.begin(), streamInfo);
        } else {
            streams.push_back(streamInfo);
        }
    }
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

int32_t HlsPlayListDownloader::GetDefaultAudioStreamId()
{
    if (currentVariant_ == nullptr || currentVariant_->defaultMedia_ == nullptr || master_ == nullptr) {
        return -1;
    }
    auto streamId = currentVariant_->defaultMedia_->streamId_;
    return streamId;
}

void HlsPlayListDownloader::SetDefaultAudio()
{
    if (master_ == nullptr || master_->defaultVariant_ == nullptr ||
        master_->defaultVariant_->defaultMedia_ == nullptr) {
        return;
    }
    std::lock_guard<std::mutex> lock(audioMutex_);
    currentAudio_ = master_->defaultVariant_->defaultMedia_;
}

void HlsPlayListDownloader::SelectAudio(int32_t streamId)
{
    if (currentVariant_ == nullptr) {
        MEDIA_LOG_W("SelectAudio currentVariant_ null");
        return;
    }
    if (currentVariant_->media_.empty()) {
        MEDIA_LOG_W("SelectAudio media_ null");
        return;
    }
    for (const auto &audio : currentVariant_->media_) {
        if (audio == nullptr) {
            continue;
        }
        if (static_cast<uint32_t>(streamId) == audio->streamId_) {
            std::lock_guard<std::mutex> lock(audioMutex_);
            MEDIA_LOG_W("SelectAudio stream id: %{public}u", streamId);
            currentAudio_ = audio;
            return;
        }
    }
    MEDIA_LOG_W("SelectAudio no found");
    return;
}


void HlsPlayListDownloader::UpdateAudio()
{
    std::string nextUrl = "";
    {
        std::lock_guard<std::mutex> lock(audioMutex_);
        if (currentAudio_ != nullptr && currentVariant_ != nullptr && currentVariant_->m3u8_ != nullptr) {
            nextUrl = currentAudio_->uri_;
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
    for (const auto &stream: master_->variants_) {
        if (stream == nullptr) {
            continue;
        }
        MEDIA_LOG_I("UpdateStreamInfo stream: %{public}u", stream->streamId_);
        videoStreamIds_.insert(stream->streamId_);
        for (const auto &media: stream->media_) {
            if (media == nullptr) {
                continue;
            }
            MEDIA_LOG_I("UpdateStreamInfo stream: %{public}u, media: %{public}u", stream->streamId_, media->streamId_);
            audioStreamIds_.insert(media->streamId_);
            needAudioManager_ = true;
        }
    }
}

HlsSegmentType HlsPlayListDownloader::GetSegType(uint32_t streamId)
{
    std::lock_guard<std::mutex> lock(streamIdMutex);
    if (audioStreamIds_.find(streamId) != audioStreamIds_.end()) {
        return HlsSegmentType::SEG_AUDIO;
    }
    return HlsSegmentType::SEG_VIDEO;
}

}
}
}
}