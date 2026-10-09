/*
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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
#include <algorithm>
#include <limits>
#include <mutex>
#include <unistd.h>
#include <utility>
#include <algorithm>
#include "plugin/plugin_time.h"
#include "hls_playlist_downloader.h"
#include "common/media_source.h"
#include "utils/string_utils.h"

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

int64_t GetFragmentDuration(const std::shared_ptr<M3U8Fragment>& file)
{
    FALSE_RETURN_V(file != nullptr, 0);
    int64_t duration = static_cast<int64_t>(file->duration_ * static_cast<double>(HST_SECOND));
    return std::max<int64_t>(duration, 0);
}

int64_t GetAverageDuration(const std::vector<int64_t>& lastSegDurations,
    const std::list<std::shared_ptr<M3U8Fragment>>& files)
{
    if (!lastSegDurations.empty()) {
        int64_t sum = 0;
        for (auto duration : lastSegDurations) {
            sum += duration;
        }
        return sum / static_cast<int64_t>(lastSegDurations.size());
    }

    int64_t currentSum = 0;
    int64_t currentCount = 0;
    for (const auto &file : files) {
        FALSE_CONTINUE_NOLOG(file != nullptr);
        currentSum += GetFragmentDuration(file);
        currentCount++;
    }
    return currentCount > 0 ? (currentSum / currentCount) : 0;
}

int64_t GetRemovedDuration(int64_t removedCount, const std::vector<int64_t>& lastSegDurations,
    int64_t avgDuration)
{
    int64_t removedDuration = 0;
    int64_t useCount = std::min<int64_t>(removedCount, static_cast<int64_t>(lastSegDurations.size()));
    for (int64_t i = 0; i < useCount; ++i) {
        removedDuration += lastSegDurations[i];
    }
    int64_t missing = removedCount - useCount;
    if (missing > 0 && avgDuration > 0) {
        removedDuration += avgDuration * missing;
    }
    return removedDuration;
}

void UpdateLastSegmentDurations(const std::list<std::shared_ptr<M3U8Fragment>>& files,
    std::vector<int64_t>& lastSegDurations)
{
    lastSegDurations.clear();
    for (const auto &file : files) {
        FALSE_CONTINUE_NOLOG(file != nullptr);
        lastSegDurations.push_back(GetFragmentDuration(file));
    }
}
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
    SetThreadSafeMaster(nullptr);
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
    auto currentVariant = GetThreadSafeCurrentVariant();
    FALSE_RETURN_MSG(currentVariant != nullptr, "currentVariant_ is nullptr");
    auto m3u8 = currentVariant->m3u8_;
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
    FALSE_RETURN_MSG(downloader_ != nullptr, "downloader_ is nullptr");
    reportInfo_->ca_ = downloader_->GetCertIssuerNames();
}

void HlsPlayListDownloader::SetSourceStatisticsDfx(
    std::shared_ptr<OHOS::MediaAVCodec::SourceStatisticsReportInfo> rpInfoPtr, bool isFmp4)
{
    reportInfo_ = rpInfoPtr;
    isFmp4_.store(isFmp4);
}

void HlsPlayListDownloader::UpdateManifest()
{
    auto currentVariant = GetThreadSafeCurrentVariant();
    if (currentVariant && currentVariant->m3u8_ && !currentVariant->m3u8_->uri_.empty()) {
        isNotifyPlayListFinished_ = false;
        UpdateStreamInfo();
        OnMasterReady(needAudioManager_, needSubtitlesManager_);
        DoOpen(currentVariant->m3u8_->uri_);
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
    SetThreadSafeCurrentVariant(other->currentVariant_);
    playList_ = other->playList_;
    audioStreamId_.store(other->audioStreamId_.load());
    subtitleStreamId_.store(other->subtitleStreamId_.load());
    isClone_.store(true);
    ParseManifest("", false);
    // playList_、retryStartTime_ will clear after parse manifest completed, no need clone
}

void HlsPlayListDownloader::OnMasterReady(bool needAudioManager, bool needSubtitlesManager)
{
    auto callback = callbackWeak_.lock();
    auto currentVariant = GetThreadSafeCurrentVariant();
    if (currentVariant == nullptr || callback == nullptr) {
        return;
    }
    if (currentVariant->m3u8_ == nullptr) {
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

bool HlsPlayListDownloader::IsFragmentInCurrentPlayList(const std::string& url, const std::string& rangeUrl,
    uint32_t offset, uint32_t length)
{
    auto currentVariant = GetThreadSafeCurrentVariant();
    FALSE_RETURN_V(currentVariant != nullptr && currentVariant->m3u8_ != nullptr, false);
    auto m3u8 = currentVariant->m3u8_;
    {
        std::lock_guard<std::mutex> lock(mediaMutex_);
        if (currentSubtitles_ && currentSubtitles_->m3u8_) {
            m3u8 = currentSubtitles_->m3u8_;
        }
        if (currentAudio_ && currentAudio_->m3u8_) {
            m3u8 = currentAudio_->m3u8_;
        }
    }
    FALSE_RETURN_V(m3u8 != nullptr, false);
    for (const auto &file : m3u8->files_) {
        FALSE_CONTINUE_NOLOG(file != nullptr);
        if (!rangeUrl.empty()) {
            if (file->uri_ == rangeUrl && file->offset_ == offset && file->length_ == length) {
                return true;
            }
            continue;
        }
        if (file->uri_ == url) {
            return true;
        }
    }
    return false;
}

int64_t HlsPlayListDownloader::GetDuration()
{
    auto master = GetThreadSafeMaster();
    FALSE_RETURN_V(master != nullptr, 0);
    if (master->bLive_) {
        // For live stream, HiPlayerImpl needs a negative duration to indicate it's live.
        // But source needs a playwindow duration for obtaining seekable range.
        // So here returns -1 * duration.
        if (IsLiveTimeShiftSeekable()) {
            return -1 * static_cast<int64_t>(master->duration_ * static_cast<double>(HST_SECOND));
        }
        return -1;
    }
    return static_cast<int64_t>(master->duration_ * static_cast<double>(HST_SECOND));
}

std::pair<int64_t, bool> HlsPlayListDownloader::GetStartInfo()
{
    auto master = GetThreadSafeMaster();
    if (!master) {
        return std::make_pair(0, false);
    }
    int64_t duration = GetDuration();
    std::pair<int64_t, bool> startInfo{0, false};
    if (master->isStart_) {
        int64_t timeOffset = static_cast<int64_t>(static_cast<double>(master->timeOffset_) *
            static_cast<double>(HST_SECOND / HST_MSECOND));
        if (duration != -1) {
            startInfo.first = timeOffset < 0 ? std::max(timeOffset + duration, static_cast<int64_t>(0)) :
                (timeOffset > duration ? duration : timeOffset);
            startInfo.second = master->precise_;
        }
    } else if (master->defaultVariant_ && master->defaultVariant_->m3u8_ &&
            master->defaultVariant_->m3u8_->isStart_) {
        int64_t timeOffset = static_cast<int64_t>(static_cast<double>(master->defaultVariant_->m3u8_->timeOffset_)
            * static_cast<double>(HST_SECOND / HST_MSECOND));
        if (duration != -1) {
            startInfo.first = timeOffset < 0 ? std::max(timeOffset + duration, static_cast<int64_t>(0)) :
                (timeOffset > duration ? duration : timeOffset);
            startInfo.second = master->defaultVariant_->m3u8_->precise_;
        }
    } else {
        MEDIA_LOG_W("EXT-X-START does no exist.");
    }
    return startInfo;
}

Seekable HlsPlayListDownloader::GetSeekable()
{
    // need wait master_ not null
    size_t times = 0;
    auto master = GetThreadSafeMaster();
    while (times < RETRY_TIMES && !isInterruptNeeded_) {
        if (master && master->isSimple_ && isParseFinished_) {
            break;
        }
        OSAL::SleepFor(SLEEP_TIME); // 1 ms
        times++;
    }
    if (times >= RETRY_TIMES || isInterruptNeeded_) {
        return Seekable::INVALID;
    }
    if (master->bLive_) {
        return IsLiveTimeShiftSeekable() ? Seekable::SEEKABLE : Seekable::UNSEEKABLE;
    }
    return Seekable::SEEKABLE;
}

std::vector<SeekRange> HlsPlayListDownloader::GetSeekableRanges()
{
    auto master = GetThreadSafeMaster();
    FALSE_RETURN_V(master != nullptr, {});
    if (master->bLive_ && !IsLiveTimeShiftSeekable()) {
        return {};
    }
    std::lock_guard<std::mutex> lock(seekRangeMutex_);
    return seekRanges_;
}

bool HlsPlayListDownloader::IsLiveTimeShiftSeekable()
{
    auto master = GetThreadSafeMaster();
    FALSE_RETURN_V(master != nullptr && master->bLive_, false);
    FALSE_RETURN_V(master->isSimple_ && isParseFinished_, false);
    FALSE_RETURN_V(master->segmentOffsets_.size() >= MIN_PRE_PARSE_NUM, false);
    return master->duration_ > 0;
}

void HlsPlayListDownloader::UpdateSeekRanges(const std::list<std::shared_ptr<M3U8Fragment>>& files)
{
    std::lock_guard<std::mutex> lock(seekRangeMutex_);
    CheckAllFragmentsSeekable(files);
    int64_t pos = liveBaseOffset_;
    int64_t rangeStart = 0;
    int64_t rangeEnd = 0;
    bool hasRange = false;

    for (const auto &file : files) {
        FALSE_CONTINUE_NOLOG(file != nullptr);
        if (!hasRange) {
            rangeStart = pos;
            rangeEnd = pos;
            hasRange = true;
        }
        if (file->discont_ && rangeEnd > rangeStart) {
            seekRanges_.push_back({rangeStart, rangeEnd});
            rangeStart = pos;
        }
        int64_t fragDuration = static_cast<int64_t>(file->duration_ * static_cast<double>(HST_SECOND));
        if (fragDuration < 0) {
            fragDuration = 0;
        }
        pos += fragDuration;
        rangeEnd = pos;
    }
    if (hasRange && rangeEnd > rangeStart) {
        auto master = GetThreadSafeMaster();
        FALSE_RETURN_NOLOG(master != nullptr);
        if (allFragmentsSeekable_ || !master->bLive_) {
            seekRanges_.push_back({rangeStart, rangeEnd});
            return;
        }
        rangeStart = rangeEnd;
        auto it = files.rbegin();
        for (auto i = 0; i != MAX_LIVE_TS_NUM && it != files.rend(); i++) {
            int64_t fragDuration = static_cast<int64_t>((*it)->duration_ * static_cast<double>(HST_SECOND));
            if (fragDuration < 0) {
                fragDuration = 0;
            }
            rangeStart -= fragDuration;
            ++it;
        }
        seekRanges_.push_back({rangeStart, rangeEnd});
    }
}

void HlsPlayListDownloader::CheckAllFragmentsSeekable(const std::list<std::shared_ptr<M3U8Fragment>>& files)
{
    auto master = GetThreadSafeMaster();
    if (master && master->bLive_) {
        UpdateLiveBaseOffset(files);
    } else {
        liveBaseOffset_ = 0;
        lastFirstSeq_ = -1;
        lastSegDurations_.clear();
    }
    seekRanges_.clear();
    FALSE_RETURN_NOLOG(!files.empty());
    if (!isFragmentsUriSet_) {
        copiedFileSequences_.clear();
        allFragmentsSeekable_ = false;
        for (const auto &file : files) {
            FALSE_CONTINUE_NOLOG(file != nullptr);
            copiedFileSequences_.push_back(file->sequence_);
        }
        isFragmentsUriSet_ = true;
    }
    if ((*files.begin()) == nullptr) {
        return;
    }
    auto firstFileSequence = (*files.begin())->sequence_;
    auto fileSequence = copiedFileSequences_.rbegin();
    for (auto i = 0; i < std::min(MAX_LIVE_TS_NUM, static_cast<unsigned int>(copiedFileSequences_.size())); i++) {
        if (firstFileSequence == *fileSequence) {
            allFragmentsSeekable_ = true;
            break;
        }
        ++fileSequence;
    }

    FALSE_RETURN_NOLOG(!allFragmentsSeekable_);

    allFragmentsSeekable_ = true;
    for (auto it = copiedFileSequences_.begin(); it != copiedFileSequences_.end(); ++it) {
        if (*it == firstFileSequence) {
            allFragmentsSeekable_ = false;
            break;
        }
    }
    return;
}

void HlsPlayListDownloader::UpdateLiveBaseOffset(const std::list<std::shared_ptr<M3U8Fragment>>& files)
{
    FALSE_RETURN(!files.empty());
    auto firstFile = files.front();
    FALSE_RETURN(firstFile != nullptr);
    int64_t firstSeq = firstFile->sequence_;
    {
        std::lock_guard<std::mutex> lock(lastSegDurationsMutex_);
        if (lastFirstSeq_ < 0) {
            int64_t avgDuration = GetAverageDuration(lastSegDurations_, files);
            int64_t normalizedSeq = std::max<int64_t>(firstSeq - 1, 0);
            if (avgDuration > 0 && normalizedSeq > 0 &&
                normalizedSeq <= std::numeric_limits<int64_t>::max() / avgDuration) {
                liveBaseOffset_ = normalizedSeq * avgDuration;
            }
        } else if (firstSeq < lastFirstSeq_) {
            MEDIA_LOG_W("Sequence number decreased. from: %{public}" PRId64
                        "to: %{public}" PRId64, lastFirstSeq_, firstSeq);
        } else if (lastFirstSeq_ >= 0 && firstSeq > lastFirstSeq_) {
            int64_t removedCount = firstSeq - lastFirstSeq_;
            int64_t avgDuration = GetAverageDuration(lastSegDurations_, files);
            liveBaseOffset_ += GetRemovedDuration(removedCount, lastSegDurations_, avgDuration);
        }
        lastFirstSeq_ = firstSeq;
        UpdateLastSegmentDurations(files, lastSegDurations_);
    }
}

uint64_t HlsPlayListDownloader::KeyChange(std::list<std::shared_ptr<M3U8Fragment>>& files)
{
    auto currentVariant = GetThreadSafeCurrentVariant();
    files = currentVariant->m3u8_->files_;
    uint64_t sessionKeyIndex = currentVariant->sessionKeyIndex_;
    std::unordered_map<uint64_t, KeyInfo> keyInfos;
    CollectAllKeyInfos(keyInfos, files, sessionKeyIndex);

    if (aesDecryptorManager_ == nullptr) {
        aesDecryptorManager_ = std::make_shared<AesDecryptorManager>();
    }

    aesDecryptorManager_->CreateAesDecryptorByKeyInfos(keyInfos);

    auto master = GetThreadSafeMaster();
    if (sessionKeyIndex > 0 && master != nullptr) {
        KeyInfo sessionKeyInfo;
        sessionKeyInfo.index_ = UINT64_MAX;
        std::copy(std::begin(master->key_), std::end(master->key_), std::begin(sessionKeyInfo.key_));
        std::copy(std::begin(master->iv_), std::end(master->iv_), std::begin(sessionKeyInfo.iv_));
        sessionKeyInfo.keyLen_ = master->keyLen_;
        aesDecryptorManager_->CreateOneAesDecryptor(sessionKeyInfo);
    }

    StartPendingKeyDownload(keyInfos, currentVariant->m3u8_->uri_);

    return sessionKeyIndex;
}

void HlsPlayListDownloader::CollectAllKeyInfos(std::unordered_map<uint64_t, KeyInfo>& keyInfos,
    std::list<std::shared_ptr<M3U8Fragment>>& files, uint64_t& sessionKeyIndex)
{
    auto currentVariant = GetThreadSafeCurrentVariant();
    currentVariant->m3u8_->GetKeyInfos(keyInfos);
    {
        std::lock_guard<std::mutex> lock(mediaMutex_);
        if (currentSubtitles_ && currentSubtitles_->m3u8_) {
            files = currentSubtitles_->m3u8_->files_;
            sessionKeyIndex = currentSubtitles_->sessionKeyIndex_;
            std::unordered_map<uint64_t, KeyInfo> subKeyInfos;
            currentSubtitles_->m3u8_->GetKeyInfos(subKeyInfos);
            keyInfos.insert(subKeyInfos.begin(), subKeyInfos.end());
        }
        if (currentAudio_ && currentAudio_->m3u8_) {
            files = currentAudio_->m3u8_->files_;
            sessionKeyIndex = currentAudio_->sessionKeyIndex_;
            std::unordered_map<uint64_t, KeyInfo> audioKeyInfos;
            currentAudio_->m3u8_->GetKeyInfos(audioKeyInfos);
            keyInfos.insert(audioKeyInfos.begin(), audioKeyInfos.end());
        }
    }
}

void HlsPlayListDownloader::StartPendingKeyDownload(const std::unordered_map<uint64_t, KeyInfo>& keyInfos,
    const std::string& baseUri)
{
    std::vector<KeyInfo> pendingKeys;
    for (const auto& [index, keyInfo] : keyInfos) {
        if (keyInfo.keyLen_ == 0 && !keyInfo.uri_.empty() &&
            aesDecryptorManager_->GetAesDecryptorByKeyIndex(index) == nullptr) {
            pendingKeys.push_back(keyInfo);
        }
    }
    if (!pendingKeys.empty()) {
        if (keyDownloader_ == nullptr) {
            keyDownloader_ = std::make_shared<KeyDownloader>(
                aesDecryptorManager_, httpHeader_, baseUri, GetSourceLoader());
        }
        keyDownloader_->StartSequentialDownload(std::move(pendingKeys));
    }
}

bool HlsPlayListDownloader::WaitForKeyReady(uint64_t keyIndex, uint32_t timeoutMs)
{
    if (keyDownloader_ != nullptr) {
        return keyDownloader_->WaitForKey(keyIndex, timeoutMs);
    }
    return GetAesDecryptor(keyIndex) != nullptr;
}

std::shared_ptr<AesDecryptor> HlsPlayListDownloader::GetAesDecryptor(uint64_t keyIndex)
{
    if (aesDecryptorManager_ != nullptr) {
        return aesDecryptorManager_->GetAesDecryptorByKeyIndex(keyIndex);
    }
    return nullptr;
}

bool HlsPlayListDownloader::HasExplicitIv(uint64_t keyIndex)
{
    if (aesDecryptorManager_ != nullptr) {
        return aesDecryptorManager_->HasExplicitIv(keyIndex);
    }
    return false;
}

void HlsPlayListDownloader::NotifyListChange()
{
    auto callback = callbackWeak_.lock();
    auto currentVariant = GetThreadSafeCurrentVariant();
    FALSE_RETURN_MSG(callback && currentVariant && currentVariant->m3u8_, "invalid state");
    std::list<std::shared_ptr<M3U8Fragment>> files;
    auto sessionKeyIndex = KeyChange(files);
    std::vector<PlayInfo> playList;
    playList.reserve(files.size());
    FALSE_RETURN_MSG(!isInterruptNeeded_, "HLS Seek return, isInterruptNeeded_.");
    for (const auto &file : files) {
        if (isInterruptNeeded_.load()) break;
        PlayInfo playInfo;
        CopyFragmentInfo(playInfo, file, sessionKeyIndex);
        playList.push_back(playInfo);
    }
    if (!currentVariant->m3u8_->localDrmInfos_.empty()) {
        callback->OnDrmInfoChanged(currentVariant->m3u8_->localDrmInfos_);
    }
    NotifyDateRanges(currentVariant->m3u8_->dateRanges_, callback);
    currentVariant->m3u8_->dateRanges_.clear();
    auto master = GetThreadSafeMaster();
    if (playList.size() > MAX_LIVE_TS_NUM && isParseFinished_ && master && master->bLive_) {
        playList.erase(playList.begin(), playList.end() - MAX_LIVE_TS_NUM);
    }
    callback->OnPlayListChanged(playList);
    if (isParseFinished_) {
        isNotifyPlayListFinished_ = true;
        StartLiveUpdateIfNeeded(master);
    }
}

void HlsPlayListDownloader::NotifyDateRanges(
    const std::list<std::shared_ptr<DateRange>>& dateRanges,
    const std::shared_ptr<PlayListChangeCallback>& callback)
{
    for (const auto &dateRange : dateRanges) {
        if (dateRange == nullptr) continue;
        auto metaData = std::make_shared<MediaAVCodec::AVTimedMetaData>();
        metaData->id = dateRange->id;
        metaData->classify = dateRange->classify;
        metaData->duration = dateRange->duration;
        metaData->start = dateRange->start;
        metaData->contents = dateRange->contents;
        callback->OnTimedMetaDataChanged(metaData);
        MEDIA_LOG_I("NotifyDateRanges: id=" PUBLIC_LOG_S ", classify=" PUBLIC_LOG_S
            ", duration=" PUBLIC_LOG_D64, metaData->id.c_str(), metaData->classify.c_str(), metaData->duration);
    }
}

void HlsPlayListDownloader::StartLiveUpdateIfNeeded(const std::shared_ptr<M3U8MasterPlaylist>& master)
{
    if (master && master->bLive_ && !updateTask_->IsTaskRunning() && !isLiveUpdateTaskStarted_) {
        isLiveUpdateTaskStarted_ = true;
        isPreParseFinished_.store(true);
        updateTask_->Start();
    }
}

void HlsPlayListDownloader::CopyFragmentInfo(PlayInfo& playInfo, std::shared_ptr<M3U8Fragment> file,
    uint64_t sessionKeyIndex)
{
    FALSE_RETURN_MSG(file != nullptr, "file is nullptr");
    playInfo.duration_ = file->duration_;
    playInfo.offset_ = file->offset_;
    playInfo.length_ = file->length_;
    playInfo.sequence_ = file->sequence_;
    auto master = GetThreadSafeMaster();
    auto currentVariant = GetThreadSafeCurrentVariant();
    if (master && currentVariant && (master->isFmp4_.load() || master->isPureByteRange_.load())) {
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
        playInfo.keyIndex_ = UINT64_MAX;
    } else {
        playInfo.keyIndex_ = 0;
    }
}

void HlsPlayListDownloader::ParseManifest(const std::string& location, bool isPreParse)
{
    if (!location.empty()) {
        url_ = location;
    }
    auto beforeMaster = GetThreadSafeMaster();
    if (!beforeMaster) {
        std::shared_ptr<MediaSourceLoaderCombinations> sourceLoader = GetSourceLoader();
        SetThreadSafeMaster(std::make_shared<M3U8MasterPlaylist>(playList_, url_, initResolution_, httpHeader_,
            statusCallback_));
        auto master = GetThreadSafeMaster();
        FALSE_RETURN_NOLOG(master != nullptr);
        master->SetSourceloader(sourceLoader);
        if (downloadCallback_ != nullptr) {
            master->SetDownloadCallback(downloadCallback_);
        }
        master->StartParsing();
        isPreParse_ = isPreParse;
        auto callback = eventCallback_.lock();
        if (callback != nullptr && !isClone_.load()) {
            std::string type = "hls";
            callback->OnEvent({PluginEventType::STREAM_UPDATE, {type}, "Stream update."});
        } else {
            UpdateManifestAndNotifyListChange(isPreParse);
            isClone_.store(false);
        }
    } else {
        UpdateMasterAndNotifyList(isPreParse);
    }
    auto afterMaster = GetThreadSafeMaster();
    if (!afterMaster->isParseSuccess_) {
        auto callback = eventCallback_.lock();
        if (callback) {
            MEDIA_LOG_E("ParseManifest parse failed.");
            callback->OnEvent({PluginEventType::CLIENT_ERROR, {NetworkClientErrorCode::ERROR_TIME_OUT},
                "errorMessage:Parse Invalid M3U8 file"});
        }
    }
}

void HlsPlayListDownloader::UpdateManifestAndNotifyListChange(bool isPreParse)
{
    auto master = GetThreadSafeMaster();
    if (!master) {
        return;
    }
    SetThreadSafeCurrentVariant(master->defaultVariant_);
    auto currentVariant = GetThreadSafeCurrentVariant();
    if (currentVariant && currentVariant->m3u8_) {
        currentVariant->m3u8_->httpHeader_ = httpHeader_;
    }
    if (!master->isSimple_) {
        UpdateManifest();
    } else {
        // need notify , avoid delay 5s
        isParseFinished_ = isPreParse ? false : true;
        UpdateMasterInfo(isPreParse);
        NotifyListChange();
    }
}

void HlsPlayListDownloader::SetDefaultStreamId(
    int32_t &videoStreamId, int32_t &audioStreamId, int32_t &subTitleStreamId)
{
    auto master = GetThreadSafeMaster();
    if (!master || videoStreamId == -1) {
        return;
    }
    audioStreamId_.store(audioStreamId);
    subtitleStreamId_.store(subTitleStreamId);
    master->ChooseStreamByStream(videoStreamId);
    UpdateManifestAndNotifyListChange(isPreParse_);
    auto currentVariant = GetThreadSafeCurrentVariant();
    if (audioStreamId == -1 && currentVariant != nullptr && currentVariant->defaultAudio_ != nullptr) {
        audioStreamId = static_cast<int32_t>(currentVariant->defaultAudio_->streamId_);
    }
    if (subTitleStreamId == -1 && currentVariant != nullptr && currentVariant->defaultSubtitles_ != nullptr) {
        subTitleStreamId = static_cast<int32_t>(currentVariant->defaultSubtitles_->streamId_);
    }
    auto callback = eventCallback_.lock();
    if (!master->isParseSuccess_ && callback != nullptr) {
        MEDIA_LOG_E("ParseManifest parse failed.");
        callback->OnEvent({PluginEventType::CLIENT_ERROR, {NetworkClientErrorCode::ERROR_TIME_OUT},
            "errorMessage:Invalid M3U8 file"});
    }
}

void HlsPlayListDownloader::UpdateMasterAndNotifyList(bool isPreParse)
{
    auto master = GetThreadSafeMaster();
    bool ret = false;
    if (!master->isSimple_) {
        ret = UpdatePlaylists(false);
    }
    auto currentVariant = GetThreadSafeCurrentVariant();
    if (currentVariant && currentVariant->m3u8_) {
        currentVariant->m3u8_->httpHeader_ = httpHeader_;
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
    if (master->isSimple_) {
        ret = UpdatePlaylists(true);
    }
    if (ret) {
        UpdateMasterInfo(isPreParse);
        if (!master->isSimple_) {
            master->isSimple_ = true;
        }
        NotifyListChange();
    }
}

bool HlsPlayListDownloader::UpdatePlaylists(bool isSimple)
{
    std::lock_guard<std::mutex> lock(mediaMutex_);
    bool ret = false;
    auto master = GetThreadSafeMaster();
    auto currentVariant = GetThreadSafeCurrentVariant();
    if (isSimple) {
        bool isNeedClean = isParseFinished_ || playList_.find(M3U8_END_TAG) != std::string::npos;
        if (currentAudio_ && currentAudio_->m3u8_) {
            ret = currentAudio_->m3u8_->Update(playList_, isNeedClean, master->totalKeyIndex_);
            master->isParseSuccess_ = ret;
            master->totalKeyIndex_ += currentAudio_->m3u8_->keyIndex_;
            currentAudio_->m3u8_->keyIndex_ = 0;
        } else if (currentSubtitles_ && currentSubtitles_->m3u8_) {
            ret = currentSubtitles_->m3u8_->Update(playList_, isNeedClean, master->totalKeyIndex_);
            master->isParseSuccess_ = ret;
            master->totalKeyIndex_ += currentSubtitles_->m3u8_->keyIndex_;
            currentSubtitles_->m3u8_->keyIndex_ = 0;
        } else if (currentVariant && currentVariant->m3u8_) {
            ret = currentVariant->m3u8_->Update(playList_, isNeedClean, master->totalKeyIndex_);
            master->isParseSuccess_ = ret;
            master->totalKeyIndex_ += currentVariant->m3u8_->keyIndex_;
            currentVariant->m3u8_->keyIndex_ = 0;
        } else {}
    } else {
        if (currentAudio_ && currentAudio_->m3u8_) {
            ret = currentAudio_->m3u8_->Update(playList_, true, master->totalKeyIndex_);
            master->totalKeyIndex_ += currentAudio_->m3u8_->keyIndex_;
            currentAudio_->m3u8_->keyIndex_ = 0;
        } else if (currentSubtitles_ && currentSubtitles_->m3u8_) {
            ret = currentSubtitles_->m3u8_->Update(playList_, true, master->totalKeyIndex_);
            master->totalKeyIndex_ += currentSubtitles_->m3u8_->keyIndex_;
            currentSubtitles_->m3u8_->keyIndex_ = 0;
        } else {
            currentVariant = master->defaultVariant_;
            if (currentVariant && currentVariant->m3u8_) {
                ret = currentVariant->m3u8_->Update(playList_, true, master->totalKeyIndex_);
                master->totalKeyIndex_ += currentVariant->m3u8_->keyIndex_;
                currentVariant->m3u8_->keyIndex_ = 0;
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
    auto currentVariant = GetThreadSafeCurrentVariant();
    if (currentVariant == nullptr) {
        return;
    }
    auto m3u8 = currentVariant->m3u8_;
    if (currentAudio_ && currentAudio_->m3u8_) {
        m3u8 = currentAudio_->m3u8_;
    }
    if (currentSubtitles_ && currentSubtitles_->m3u8_) {
        m3u8 = currentSubtitles_->m3u8_;
    }

    auto master = GetThreadSafeMaster();
    FALSE_RETURN_MSG(master != nullptr, "master_ is nullptr");
    FALSE_RETURN_MSG(m3u8 != nullptr, "m3u8 is nullptr");

    if (isPreParseFinished_.load() && master->bLive_ && !m3u8->IsLive()) {
        MEDIA_LOG_I("Live stream ended and transitioning to Vod");
        updateTask_->Stop();
        isLiveEnd_ = true;
    }
    master->bLive_ = m3u8->IsLive();
    master->isFmp4_ = m3u8->isHeaderReady_.load();
    master->duration_ = m3u8->GetDuration();
    master->segmentOffsets_ = m3u8->segmentOffsets_;
    master->hasDiscontinuity_ = m3u8->hasDiscontinuity_;
    master->isPureByteRange_.store(m3u8->isPureByteRange_.load());
    UpdateSeekRanges(m3u8->files_);
    isParseFinished_ = isPreParse ? false : true;
}

void HlsPlayListDownloader::PreParseManifest(const std::string& location)
{
    auto master = GetThreadSafeMaster();
    if (master && master->isSimple_) {
        return;
    }
    if (playList_.find(M3U8_START_TAG) == std::string::npos ||
        playList_.find(M3U8_END_TAG) != std::string::npos ||
        playList_.find(M3U8_X_MAP_TAG) != std::string::npos) {
        return;
    }
    int tsNum = 0;
    std::string::size_type tsIndex = 0;
    std::string::size_type firstTsTagIndex = 0;
    std::string::size_type lastTsTagIndex = 0;
    std::string tsTag = M3U8_TS_TAG;
    std::string::size_type tsTagSize = tsTag.size();
    while (true) {
        tsIndex = playList_.find(tsTag, tsIndex);
        if (tsIndex == std::string::npos || tsIndex >= playList_.length()) {
            break;
        }
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
    playList_ = backUpPlayList;
}

void HlsPlayListDownloader::SelectBitRate(uint32_t bitRate)
{
    if (newVariant_ == nullptr) {
        return;
    }
    SetThreadSafeCurrentVariant(newVariant_);
    auto currentVariant = GetThreadSafeCurrentVariant();
    isFragmentsUriSet_ = false;
    MEDIA_LOG_I("SelectBitRate currentVariant_ " PUBLIC_LOG_U64, currentVariant->bandWidth_);
}

bool HlsPlayListDownloader::IsBitrateSame(uint32_t bitRate)
{
    auto master = GetThreadSafeMaster();
    FALSE_RETURN_V(master != nullptr, false);
    uint32_t maxGap = 0;
    bool isFirstSelect = true;
    for (const auto &item : master->variants_) {
        if (item == nullptr) {
            continue;
        }
        uint32_t tempGap = (item->bandWidth_ > bitRate) ? (item->bandWidth_ - bitRate) : (bitRate - item->bandWidth_);
        if (isFirstSelect || (tempGap < maxGap)) {
            if (master->firstVideoStream_ == nullptr) {
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
    auto currentVariant = GetThreadSafeCurrentVariant();
    if (currentVariant != nullptr && newVariant_->bandWidth_ == currentVariant->bandWidth_) {
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
        return currentSubtitles_ == nullptr || streamId == currentSubtitles_->streamId_;
    } else {
        MEDIA_LOG_W("Audio and Subtitle both Null.");
    }
    return true;
}

std::vector<uint32_t> HlsPlayListDownloader::GetBitRates()
{
    auto master = GetThreadSafeMaster();
    std::vector<uint32_t> bitRates;
    FALSE_RETURN_V(master != nullptr, bitRates);

    for (const auto &item : master->variants_) {
        if (item) {
            bitRates.push_back(item->bandWidth_);
        }
    }
    return bitRates;
}

uint32_t HlsPlayListDownloader::GetCurBitrate()
{
    auto currentVariant = GetThreadSafeCurrentVariant();
    if (currentVariant == nullptr) {
        return 0;
    }
    return currentVariant->bandWidth_;
}

uint64_t HlsPlayListDownloader::GetCurrentBitRate()
{
    auto currentVariant = GetThreadSafeCurrentVariant();
    if (currentVariant == nullptr) {
        return 0;
    }
    MEDIA_LOG_I("HlsPlayListDownloader currentBitrate: " PUBLIC_LOG_D64, currentVariant->bandWidth_);
    return currentVariant->bandWidth_;
}

int HlsPlayListDownloader::GetVideoWidth()
{
    auto currentVariant = GetThreadSafeCurrentVariant();
    if (currentVariant == nullptr) {
        return 0;
    }
    MEDIA_LOG_I("HlsPlayListDownloader currentWidth: " PUBLIC_LOG_D64, currentVariant->bandWidth_);
    return currentVariant->width_;
}

int HlsPlayListDownloader::GetVideoHeight()
{
    auto currentVariant = GetThreadSafeCurrentVariant();
    if (currentVariant == nullptr) {
        return 0;
    }
    MEDIA_LOG_I("HlsPlayListDownloader currentHeight: " PUBLIC_LOG_D64, currentVariant->bandWidth_);
    return currentVariant->height_;
}

bool HlsPlayListDownloader::IsLive()
{
    MEDIA_LOG_I("HlsPlayListDownloader IsLive enter.");
    auto master = GetThreadSafeMaster();
    if (!master) {
        return false;
    }
    return master->bLive_;
}

std::string HlsPlayListDownloader::GetUrl()
{
    return url_;
}

std::shared_ptr<M3U8MasterPlaylist> HlsPlayListDownloader::GetMaster()
{
    return GetThreadSafeMaster();
}

std::shared_ptr<M3U8VariantStream> HlsPlayListDownloader::GetCurrentVariant()
{
    return GetThreadSafeCurrentVariant();
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
    auto master = GetThreadSafeMaster();
    if (master && master->segmentOffsets_.size() > tsIndex) {
        return master->segmentOffsets_[tsIndex];
    }
    return 0;
}

bool HlsPlayListDownloader::GetHLSDiscontinuity()
{
    auto master = GetThreadSafeMaster();
    if (master) {
        return master->hasDiscontinuity_;
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

size_t HlsPlayListDownloader::GetLiveUpdateGap()
{
    auto currentVariant = GetThreadSafeCurrentVariant();
    if (currentVariant && currentVariant->m3u8_) {
        return currentVariant->m3u8_->GetLiveUpdateGap();
    }
    return 0;
}

void HlsPlayListDownloader::InterruptM3U8Parse(bool isInterruptNeeded)
{
    auto master = GetThreadSafeMaster();
    if (master) {
        master->SetInterruptState(isInterruptNeeded);
    }
    auto currentVariant = GetThreadSafeCurrentVariant();
    if (currentVariant && currentVariant->m3u8_) {
        currentVariant->m3u8_->isInterruptNeeded_.store(isInterruptNeeded);
    }
}

bool HlsPlayListDownloader::ReadFmp4Header(uint8_t* buffer, uint32_t wantLen,
    uint32_t& readLen, uint32_t streamId)
{
    auto master = GetThreadSafeMaster();
    if (master == nullptr || buffer == nullptr) {
        return false;
    }
    return ReadMediaHeader(master->subtitlesList_, buffer, wantLen, readLen, streamId) ||
        ReadMediaHeader(master->audioList_, buffer, wantLen, readLen, streamId) ||
        ReadStreamHeader(master->variants_, buffer, wantLen, readLen, streamId);
}

bool HlsPlayListDownloader::ReadMediaHeader(const std::list<std::shared_ptr<M3U8Media>>& mediaList,
    uint8_t* buffer, uint32_t wantLen, uint32_t& readLen, uint32_t streamId)
{
    for (const auto& stream : mediaList) {
        if (stream == nullptr || stream->m3u8_ == nullptr || stream->streamId_ != streamId ||
            !stream->m3u8_->isHeaderReady_.load()) {
            continue;
        }
        AutoLock lock(stream->m3u8_->headerMutex_);
        if (stream->m3u8_->downloadHeaderLen_ > wantLen) {
            MEDIA_LOG_W("ReadMediaHeader downloadHeaderLen_ " PUBLIC_LOG_U32 " > wantLen " PUBLIC_LOG_U32,
                stream->m3u8_->downloadHeaderLen_, wantLen);
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
            !stream->m3u8_->isHeaderReady_.load()) {
            continue;
        }
        AutoLock lock(stream->m3u8_->headerMutex_);
        if (stream->m3u8_->downloadHeaderLen_ > wantLen) {
            MEDIA_LOG_W("ReadStreamHeader downloadHeaderLen_ " PUBLIC_LOG_U32 " > wantLen " PUBLIC_LOG_U32,
                stream->m3u8_->downloadHeaderLen_, wantLen);
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

void HlsPlayListDownloader::GetStreamInfo(std::vector<StreamInfo>& streams, bool isUpdate)
{
    auto master = GetThreadSafeMaster();
    auto currentVariant = GetThreadSafeCurrentVariant();
    if ((!isUpdate && currentVariant == nullptr) || master == nullptr) {
        return;
    }
    GetMediaStreams(StreamType::AUDIO, streams);
    GetMediaStreams(StreamType::SUBTITLE, streams);
    for (const auto &stream : master->variants_) {
        if (stream == nullptr) {
            continue;
        }
        StreamInfo streamInfo;
        streamInfo.streamId = static_cast<int32_t>(stream->streamId_);
        streamInfo.type = StreamType::VIDEO;
        streamInfo.bitRate = stream->bandWidth_;
        streamInfo.frameRate = stream->frameRate_;
        streamInfo.mimeType = master->GetMimeType(stream->codecs_);
        streamInfo.codecs = master->GetMimeType(stream->codecs_);
        streamInfo.originCodecs = master->GetCodec(StreamType::VIDEO, stream->codecs_);
        streamInfo.isSimple = stream->isSimple_;
        streamInfo.videoWidth = stream->width_;
        streamInfo.videoHeight = stream->height_;
        if (!isUpdate && stream->streamId_ == currentVariant->streamId_) {
            streams.insert(streams.begin(), streamInfo);
        } else {
            streams.push_back(streamInfo);
        }
    }
}

bool HlsPlayListDownloader::ContainsNonDigit(const std::string& str)
{
    for (char c : str) {
        if (!std::isdigit(c)) {
            return true;
        }
    }
    return false;
}

void HlsPlayListDownloader::GetMediaStreams(StreamType streamType, std::vector<StreamInfo>& streams)
{
    std::list<std::shared_ptr<M3U8Media>> mediaList;
    auto master = GetThreadSafeMaster();
    FALSE_RETURN_NOLOG(master != nullptr);
    mediaList = streamType == StreamType::AUDIO ? master->audioList_ : master->subtitlesList_;
    auto currentVariant = GetThreadSafeCurrentVariant();
    for (const auto& media : mediaList) {
        if (media == nullptr) continue;
        
        StreamInfo streamInfo;
        streamInfo.streamId = static_cast<int32_t>(media->streamId_);
        streamInfo.type = streamType;
        streamInfo.lang = media->lang_;
        if (streamType == StreamType::AUDIO) {
            streamInfo.originCodecs = master->GetCodec(StreamType::AUDIO, media->originCodec_);
            int32_t tempChannel = 0;
            StringUtil::SafeStoInt32(media->channels_, tempChannel);
            streamInfo.channels = tempChannel > 0 ? static_cast<uint32_t>(tempChannel) : 0;
        }
        bool isDefault = false;
        if (streamType == StreamType::AUDIO && audioStreamId_.load() != -1) {
            isDefault = media->streamId_ == static_cast<uint32_t>(audioStreamId_.load());
        } else if (streamType == StreamType::AUDIO && currentVariant != nullptr) {
            isDefault = currentVariant->defaultAudio_ != nullptr &&
                media->streamId_ == currentVariant->defaultAudio_->streamId_;
        }
        if (streamType == StreamType::SUBTITLE && subtitleStreamId_.load() != -1) {
            isDefault = media->streamId_ == static_cast<uint32_t>(subtitleStreamId_.load());
            streamInfo.sniffSize = DEFAULT_SUBTITLE_SNIFFSIZE;
        } else if (streamType == StreamType::SUBTITLE && currentVariant != nullptr) {
            isDefault = currentVariant->defaultSubtitles_ != nullptr &&
                media->streamId_ == currentVariant->defaultSubtitles_->streamId_;
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
    auto master = GetThreadSafeMaster();
    if (master && master->isFmp4_) {
        return true;
    }
    return false;
}

bool HlsPlayListDownloader::IsPureByteRange()
{
    auto master = GetThreadSafeMaster();
    if (master && master->isPureByteRange_.load()) {
        return true;
    }
    return false;
}

void HlsPlayListDownloader::ReOpen(void)
{
    // 关闭直播目录更新，清空一二级目录对象
    Pause(true);
    SetThreadSafeMaster(nullptr);
    SetThreadSafeCurrentVariant(nullptr);
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
    auto master = GetThreadSafeMaster();
    auto currentVariant = GetThreadSafeCurrentVariant();
    if (currentVariant == nullptr || master == nullptr) {
        return nullptr;
    }
    for (const auto &variant : master->variants_) {
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
    auto master = GetThreadSafeMaster();
    auto currentVariant = GetThreadSafeCurrentVariant();
    if (currentVariant == nullptr || master == nullptr) {
        return -1;
    }
    if (mediaType == HlsSegmentType::SEG_AUDIO && audioStreamId_.load() != -1) {
        return audioStreamId_.load();
    } else if (mediaType == HlsSegmentType::SEG_SUBTITLE && subtitleStreamId_.load() != -1) {
        return subtitleStreamId_.load();
    } else {
        MEDIA_LOG_I("no default streamId.");
    }
    if (mediaType == HlsSegmentType::SEG_AUDIO && currentVariant->defaultAudio_ != nullptr) {
        return static_cast<int32_t>(currentVariant->defaultAudio_->streamId_);
    } else if (currentVariant->defaultSubtitles_ != nullptr) {
        return static_cast<int32_t>(currentVariant->defaultSubtitles_->streamId_);
    } else {
        MEDIA_LOG_I("no DefaultMediaStreamId.");
    }
    return -1;
}

void HlsPlayListDownloader::SetDefaultMedia(HlsSegmentType mediaType)
{
    auto master = GetThreadSafeMaster();
    if (master == nullptr || master->defaultVariant_ == nullptr) {
        return;
    }
    std::shared_ptr<M3U8Media> defaultMediaType;
    if (mediaType == HlsSegmentType::SEG_AUDIO && master->defaultVariant_->defaultAudio_ != nullptr) {
        defaultMediaType = master->defaultVariant_->defaultAudio_;
    } else if (master->defaultVariant_->defaultSubtitles_ != nullptr) {
        defaultMediaType = master->defaultVariant_->defaultSubtitles_;
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
    auto currentVariant = GetThreadSafeCurrentVariant();
    if (currentVariant == nullptr) {
        MEDIA_LOG_W("SelectMedia currentVariant_ null");
        return;
    }
    std::list<std::shared_ptr<M3U8Media>> videoStreamMedia;
    if (mediaType == HlsSegmentType::SEG_AUDIO && !currentVariant->audioMedia_.empty()) {
        videoStreamMedia = currentVariant->audioMedia_;
    } else if (!currentVariant->subtitlesMedia_.empty()) {
        videoStreamMedia = currentVariant->subtitlesMedia_;
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
        auto currentVariant = GetThreadSafeCurrentVariant();
        if (currentVariant && currentVariant->m3u8_) {
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
    std::lock_guard<std::mutex> lock(streamIdMutex_);
    videoStreamIds_.clear();
    audioStreamIds_.clear();
    subtitlesStreamIds_.clear();
    auto master = GetThreadSafeMaster();
    FALSE_RETURN_MSG(master != nullptr, "master_ is nullptr");
    for (const auto &stream: master->variants_) {
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
    std::lock_guard<std::mutex> lock(streamIdMutex_);
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
    auto currentVariant = GetThreadSafeCurrentVariant();
    FALSE_RETURN_V_MSG(currentVariant != nullptr, 0, "GetCurStreamId no currentVariant found!");
    std::lock_guard<std::mutex> lock(mediaMutex_);
    if (currentAudio_ != nullptr) {
        return currentAudio_->streamId_;
    }
    if (currentSubtitles_ != nullptr) {
        return currentSubtitles_->streamId_;
    }
    return currentVariant->streamId_;
}

/**
 * master_变量写函数
 */
void HlsPlayListDownloader::SetThreadSafeMaster(std::shared_ptr<M3U8MasterPlaylist> master)
{
    std::unique_lock<std::shared_mutex> lock(masterMutex_);
    master_ = std::move(master);
}

/**
 * master_变量读函数
 */
std::shared_ptr<M3U8MasterPlaylist> HlsPlayListDownloader::GetThreadSafeMaster()
{
    std::shared_lock<std::shared_mutex> lock(masterMutex_);
    return master_;
}

/**
 * currentVariant_变量写函数
 */
void HlsPlayListDownloader::SetThreadSafeCurrentVariant(std::shared_ptr<M3U8VariantStream> currentVariant)
{
    std::unique_lock<std::shared_mutex> lock(currentVariantMutex_);
    currentVariant_ = std::move(currentVariant);
}

/**
 * currentVariant_变量读函数
 */
std::shared_ptr<M3U8VariantStream> HlsPlayListDownloader::GetThreadSafeCurrentVariant()
{
    std::shared_lock<std::shared_mutex> lock(currentVariantMutex_);
    return currentVariant_;
}
}
}
}
}