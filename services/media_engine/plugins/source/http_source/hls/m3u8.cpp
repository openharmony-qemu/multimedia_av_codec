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
#define HST_LOG_TAG "M3U8"

#include <algorithm>
#include <utility>
#include <sstream>
#include <iomanip>
#include <unordered_set>
#include <unordered_map>
#include <cstdint>
#include "m3u8.h"
#include "base64_utils.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
namespace {
constexpr uint32_t MAX_LOOP = 16;
constexpr uint32_t DRM_UUID_OFFSET = 12;
constexpr uint32_t DRM_PSSH_TITLE_LEN = 16;
constexpr uint32_t WAIT_KEY_SLEEP_TIME = 10;
constexpr uint32_t MAX_DOWNLOAD_TIME = 500;
constexpr uint64_t BAND_WIDTH_LIMIT = 3 * 1024 * 1024;
constexpr uint32_t DEFAULT_HEADER_SIZE = 1 * 1024 * 1024;
constexpr uint32_t DEFAULT_MAX_SIZE = 100 * 1024 * 1024;
constexpr double SECOND_TO_MICROSECOND = 1000.0 * 1000.0;
constexpr uint64_t MAX_UINT64_NUM = UINT64_MAX;
const char DRM_PSSH_TITLE[] = "data:text/plain;";
constexpr char EXT_X_DEFINE_TAG[] = "#EXT-X-DEFINE";
static const std::unordered_map<std::string, M3U8MediaType> kTypeMap = {
    {"AUDIO", M3U8MediaType::M3U8_MEDIA_TYPE_AUDIO},
    {"VIDEO", M3U8MediaType::M3U8_MEDIA_TYPE_VIDEO},
    {"SUBTITLES", M3U8MediaType::M3U8_MEDIA_TYPE_SUBTITLES},
    {"CLOSED-CAPTIONS", M3U8MediaType::M3U8_MEDIA_TYPE_CLOSED_CAPTIONS}
};
static const std::unordered_set<std::string> VIDEO_CODECS = {
    "mp4v", "avc1", "hev1", "svc1", "mvc1", "mvc2", "sevc", "s263"
};

bool StrHasPrefix(const std::string &str, const std::string &prefix)
{
    return str.find(prefix) == 0;
}

std::string UriJoin(std::string& baseUrl, const std::string& uri)
{
    if ((uri.find("http://") != std::string::npos) || (uri.find("https://") != std::string::npos)) {
        return uri;
    } else if (uri.find("//") == 0) { // start with "//"
        return baseUrl.substr(0, baseUrl.find('/')) + uri;
    } else if (uri.find('/') == 0) {
        auto pos = baseUrl.find('/', strlen("https://"));
        return baseUrl.substr(0, pos) + uri;
    } else {
        std::string::size_type pos = baseUrl.find('?');
        std::string uriMain = (pos != std::string::npos) ? baseUrl.substr(0, pos) : baseUrl;
        pos = uriMain.rfind('/');
        if (pos == std::string::npos) {
            return uri;
        }
        return uriMain.substr(0, pos + 1) + uri;
    }
}
}

M3U8Fragment::M3U8Fragment(const M3U8Fragment& m3u8, const uint8_t *key, const uint8_t *iv)
    : uri_(std::move(m3u8.uri_)),
      duration_(m3u8.duration_),
      sequence_(m3u8.sequence_),
      discont_(m3u8.discont_)
{
    if (iv == nullptr || key == nullptr) {
        return;
    }

    for (int i = 0; i < static_cast<int>(MAX_LOOP); i++) {
        iv_[i] = iv[i];
        key_[i] = key[i];
    }
}

M3U8Fragment::M3U8Fragment(const std::string &uri, double duration, int sequence, bool discont)
    : uri_(std::move(uri)), duration_(duration), sequence_(sequence), discont_(discont)
{
}

M3U8Fragment::~M3U8Fragment()
{
    NZERO_LOG(memset_s(key_, sizeof(key_), 0, sizeof(key_)));
    NZERO_LOG(memset_s(iv_, sizeof(iv_), 0, sizeof(iv_)));
}

M3U8::M3U8(const std::string &uri, const std::string &name,
    const std::unordered_map<std::string, std::string> tagMasterMap,
    StatusCallbackFunc statusCallback,
    std::shared_ptr<MediaSourceLoaderCombinations> sourceLoader)
    : uri_(std::move(uri)), name_(std::move(name))
{
    sourceLoader_ = sourceLoader;
    tagMasterMap_ = tagMasterMap;
    monitorStatusCallback_ = statusCallback;
    InitTagUpdatersMap();
}

M3U8::~M3U8()
{
    if (downloader_) {
        downloader_ = nullptr;
    }
    if (downloaderHeader_) {
        downloaderHeader_ = nullptr;
    }
    delete[] fmp4Header_;
    fmp4Header_ = nullptr;
    NZERO_LOG(memset_s(key_, sizeof(key_), 0, sizeof(key_)));
    NZERO_LOG(memset_s(iv_, sizeof(iv_), 0, sizeof(iv_)));
}

void M3U8::SetDownloadCallback(const std::shared_ptr<DownloadMetricsInfo> &callback)
{
    downloadCallback_ = callback;
}

bool M3U8::Update(const std::string& playList, bool isNeedCleanFiles)
{
    if (playList_ == playList) {
        MEDIA_LOG_I("playlist does not change ");
        return true;
    }
    if (!StrHasPrefix(playList, "#EXTM3U")) {
        MEDIA_LOG_I("playlist doesn't start with #EXTM3U");
        return false;
    }
    if (playList.find("\n#EXT-X-STREAM-INF:") != std::string::npos) {
        MEDIA_LOG_I("Not a media playlist, but a master playlist!");
        return false;
    }
    if (isNeedCleanFiles) {
        files_.clear();
        segmentOffsets_.clear();
    }
    MEDIA_LOG_I("media playlist");
    std::pair<std::list<std::shared_ptr<Tag>>, std::unordered_map<std::string, std::string>> ret;
    if (playList.find(EXT_X_DEFINE_TAG) != std::string::npos) {
        std::unordered_map<std::string, std::string> tagMasterMap;
        std::unordered_map<std::string, std::string> tagUriMap;
        if (playList.find("IMPORT") != std::string::npos && !tagMasterMap_.empty()) {
            tagMasterMap = tagMasterMap_;
        }
        if (!uri_.empty() && uri_.find("?") != std::string::npos) {
            tagUriMap = ParseUriQuery(uri_);
        }
        ret = ParseEntries(playList, tagMasterMap, tagUriMap);
    } else {
        ret = ParseEntries(playList);
    }
    std::list<std::shared_ptr<Tag>> tags = ret.first;
    UpdateFromTags(tags);
    MEDIA_LOG_I("UpdateFromTags done, isInterruptNeed " PUBLIC_LOG_D32, isInterruptNeeded_.load());
    tags.clear();
    playList_ = playList;
    return true;
}

void M3U8::InitTagUpdaters()
{
    tagUpdatersMap_[HlsTag::EXTXPLAYLISTTYPE] = [this](std::shared_ptr<Tag> &tag, const M3U8Info &info) {
        FALSE_RETURN(tag != nullptr);
        bLive_ = std::static_pointer_cast<SingleValueTag>(tag)->GetValue().QuotedString() != "VOD";
        if (sourceLoader_ != nullptr && sourceLoader_->GetenableOfflineCache()) {
            sourceLoader_->Close(-1);
        }
    };
    tagUpdatersMap_[HlsTag::EXTXTARGETDURATION] = [this](std::shared_ptr<Tag> &tag, const M3U8Info &info) {
        FALSE_RETURN(tag != nullptr);
        std::ignore = info;
        targetDuration_ = std::static_pointer_cast<SingleValueTag>(tag)->GetValue().FloatingPoint();
    };
    tagUpdatersMap_[HlsTag::EXTXMEDIASEQUENCE] = [this](std::shared_ptr<Tag> &tag, const M3U8Info &info) {
        FALSE_RETURN(tag != nullptr);
        std::ignore = info;
        sequence_ = std::static_pointer_cast<SingleValueTag>(tag)->GetValue().Decimal();
        initSequence_ = sequence_;
    };
    tagUpdatersMap_[HlsTag::EXTXDISCONTINUITYSEQUENCE] = [this](std::shared_ptr<Tag> &tag, M3U8Info &info) {
        FALSE_RETURN(tag != nullptr);
        discontSequence_ = static_cast<int>(std::static_pointer_cast<SingleValueTag>(tag)->GetValue().Decimal());
        info.discontinuity = true;
    };
    tagUpdatersMap_[HlsTag::EXTINF] = [this](const std::shared_ptr<Tag> &tag, M3U8Info &info) {
        FALSE_RETURN(tag != nullptr);
        GetExtInf(tag, info.duration);
    };
    tagUpdatersMap_[HlsTag::URI] = [this](std::shared_ptr<Tag> &tag, M3U8Info &info) {
        FALSE_RETURN(tag != nullptr);
        info.uri = UriJoin(uri_, std::static_pointer_cast<SingleValueTag>(tag)->GetValue().QuotedString());
    };
    tagUpdatersMap_[HlsTag::EXTXBYTERANGE] = [this](const std::shared_ptr<Tag> &tag, const M3U8Info &info) {
        FALSE_RETURN(tag != nullptr);
        std::ignore = info;
        std::string value = std::static_pointer_cast<SingleValueTag>(tag)->GetValue().QuotedString();
        std::shared_ptr<Attribute> attr = std::make_shared<Attribute>("BYTERANGE", value);
        offset_ = attr->GetByteRange().first + 1; // 1
        length_ = attr->GetByteRange().second;
        if (!isHeaderReady_.load()) {
            offset_ -= 1; // 1
            isPureByteRange_.store(true);
        }
    };
}

void M3U8::InitTagUpdatersMap()
{
    InitTagUpdaters();
    tagUpdatersMap_[HlsTag::EXTXDISCONTINUITY] = [this](const std::shared_ptr<Tag> &tag, M3U8Info &info) {
        std::ignore = tag;
        discontSequence_++;
        info.discontinuity = true;
    };
    tagUpdatersMap_[HlsTag::EXTXKEY] = [this](std::shared_ptr<Tag> &tag, const M3U8Info &info) {
        PrepareDecrptionKeys(tag);
    };
    tagUpdatersMap_[HlsTag::EXTXMAP] = [this](const std::shared_ptr<Tag> &tag, const M3U8Info &info) {
        FALSE_RETURN(tag != nullptr);
        std::ignore = info;
        ParseMap(std::static_pointer_cast<AttributesTag>(tag));
    };
    tagUpdatersMap_[HlsTag::EXTXSKIP] = [this](const std::shared_ptr<Tag> &tag, const M3U8Info &info) {
        FALSE_RETURN(tag != nullptr);
        std::ignore = info;
        auto item = std::static_pointer_cast<AttributesTag>(tag);
        auto skipSegmentsNum = item->GetAttributeByName("SKIPPED-SEGMENTS");
        if (skipSegmentsNum) {
            skipSegmentsNum_ = skipSegmentsNum->Decimal();
        }
    };
    tagUpdatersMap_[HlsTag::EXTXSTART] = [this](const std::shared_ptr<Tag> &tag, const M3U8Info &info) {
        FALSE_RETURN(tag != nullptr);
        std::ignore = info;
        auto item = std::static_pointer_cast<AttributesTag>(tag);
        auto precise = item->GetAttributeByName("PRECISE");
        if (precise) {
            precise_ = (precise->QuotedString() == "YES") ? true : false;
        }
        auto timeOffset = item->GetAttributeByName("TIME-OFFSET");
        if (timeOffset) {
            timeOffset_ = timeOffset->FloatingPoint();
        }
        isStart_ = true;
    };
}

void M3U8::PrepareDecrptionKeys(std::shared_ptr<Tag>& tag)
{
    FALSE_RETURN(tag != nullptr);
    if (isDecryptAble_ || isDecryptKeyReady_) {
        return;
    }
    if (sourceLoader_ != nullptr && sourceLoader_->GetenableOfflineCache()) {
        sourceLoader_->Close(-1);
    }
    isDecryptAble_ = true;
    isDecryptKeyReady_ = false;
    ParseKey(std::static_pointer_cast<AttributesTag>(tag));
    if ((keyUri_ != nullptr) && (keyUri_->length() > DRM_PSSH_TITLE_LEN) &&
        (keyUri_->substr(0, DRM_PSSH_TITLE_LEN) == DRM_PSSH_TITLE)) {
        ProcessDrmInfos();
    } else {
        DownloadKey();
    }
}

void M3U8::ParseMap(const std::shared_ptr<AttributesTag>& tag)
{
    FALSE_RETURN(tag != nullptr);
    std::string uri;
    auto uriAttribute = tag->GetAttributeByName("URI");
    if (uriAttribute) {
        if (uriAttribute->QuotedString().empty()) {
            return;
        }
        uri = UriJoin(uri_, uriAttribute->QuotedString());
    }
    auto byteRangeAttribute = tag->GetAttributeByName("BYTERANGE");
    size_t offset = 0;
    size_t length = 0;
    if (byteRangeAttribute) {
        offset = byteRangeAttribute->GetByteRange().first;
        length = byteRangeAttribute->GetByteRange().second;
    }
    if (isHeaderReady_.load()) {
        return;
    }
    if (length > 0 && length < DEFAULT_MAX_SIZE) {
        fmp4Header_ = new uint8_t[length + 1]; // 1
        fmp4HeaderLen = length + 1;
    }
    DownloadMap(uri, offset, length);
    AutoLock lock(sleepMutex_);
    sleepCond_.WaitFor(lock, WAIT_KEY_SLEEP_TIME * MAX_DOWNLOAD_TIME, [this]() {
        return isInterruptNeeded_.load() || isHeaderReady_.load();
    });
    MEDIA_LOG_I("x-map download %{public}u, is ready: %{public}d", downloadHeaderLen_, isHeaderReady_.load());
}
void M3U8::InitDownloadHeader()
{
    if (sourceLoader_ != nullptr) {
        downloaderHeader_ = std::make_shared<Downloader>("HlsSourceMap", sourceLoader_);
    } else {
        downloaderHeader_ = std::make_shared<Downloader>("HlsSourceMap");
    }
}

void M3U8::DownloadMap(const std::string& uri, size_t offset, size_t length)
{
    if (uri.empty()) {
        return;
    }
    InitDownloadHeader();
    if (downloaderHeader_ != nullptr && downloadCallback_ != nullptr) {
        downloaderHeader_->SetDownloadCallback(downloadCallback_);
    }
    downloaderHeader_->Init();
    auto weakDownloader = weak_from_this();
    dataSaveHeader_ = [weakDownloader](uint8_t *&&data, uint32_t &&len, bool &&notBlock) -> uint32_t {
        auto shareDownloader = weakDownloader.lock();
        FALSE_RETURN_V_MSG(shareDownloader != nullptr, 0u, "dataSaveHeader, M3U8 map downloader already destructed.");
        return shareDownloader->SaveMapData(std::forward<decltype(data)>(data), std::forward<decltype(len)>(len),
            notBlock);
    };
    statusCallback_ = [weakDownloader](DownloadStatus &&status, std::shared_ptr<Downloader> d,
        std::shared_ptr<DownloadRequest> &request) {
        auto shareDownloader = weakDownloader.lock();
        FALSE_RETURN_MSG(shareDownloader != nullptr, "statusCb, M3U8 map downloader already destructed.");
        shareDownloader->OnDownloadStatus(std::forward<decltype(status)>(status), shareDownloader->downloaderHeader_,
            std::forward<decltype(request)>(request));
    };
    auto downloadDoneCallback = [weakDownloader] (const std::string &url, const std::string& location) {
        auto shareDownloader = weakDownloader.lock();
        FALSE_RETURN_MSG(shareDownloader != nullptr, "downloadDoneCb, M3U8 map downloader already destructed.");
        shareDownloader->UpdateDownloadFinished(url, location);
    };
    auto realStatusCallback = [weakDownloader](DownloadStatus &&status, std::shared_ptr<Downloader> &downloader,
        std::shared_ptr<DownloadRequest> &request) {
        auto shareDownloader = weakDownloader.lock();
        FALSE_RETURN_MSG(shareDownloader != nullptr, "realStatusCb, M3U8 map downloader already destructed.");
        shareDownloader->monitorStatusCallback_(status, shareDownloader->downloaderHeader_,
            std::forward<decltype(request)>(request));
        shareDownloader->statusCallback_(status, shareDownloader->downloaderHeader_,
            std::forward<decltype(request)>(request));
    };
    RequestInfo requestInfo;
    requestInfo.url = uri;
    requestInfo.httpHeader = httpHeader_;
    downloadHeaderRequest_ = std::make_shared<DownloadRequest>(dataSaveHeader_, realStatusCallback,
                                                        requestInfo, length > 0 ? false : true);
    downloadHeaderRequest_->SetDownloadDoneCb(downloadDoneCallback);
    if (length > 0) {
        downloadHeaderRequest_->SetRangePos(offset, offset + length);
    }
    downloadHeaderRequest_->SetIsAuthRequest(true);
    downloaderHeader_->Download(downloadHeaderRequest_, -1);
    downloaderHeader_->Start();
}

void M3U8::UpdateDownloadFinished(const std::string& url, const std::string& location)
{
    isHeaderReady_.store(true);
    sleepCond_.NotifyOne();
}

uint32_t M3U8::SaveMapData(uint8_t* data, uint32_t len, bool notBlock)
{
    FALSE_RETURN_V(data != nullptr, 0);
    if (fmp4Header_ == nullptr && downloadHeaderRequest_) {
        uint32_t headerLen = downloadHeaderRequest_->GetFileContentLengthNoWait();
        headerLen = headerLen > 0 ? headerLen : DEFAULT_HEADER_SIZE; // 1MB
        headerLen = std::min(headerLen, DEFAULT_MAX_SIZE);
        fmp4Header_ = new(std::nothrow) uint8_t[headerLen];
        FALSE_RETURN_V_MSG(fmp4Header_ != nullptr, 0u, "SaveMapData error, no memory.");
        fmp4HeaderLen = headerLen;
    }
    NZERO_RETURN_V(memcpy_s(fmp4Header_ + downloadHeaderLen_, fmp4HeaderLen - downloadHeaderLen_, data, len), 0);
    downloadHeaderLen_ += std::min(len, fmp4HeaderLen - downloadHeaderLen_);
    return len;
}

uint64_t M3U8::ExtractNumber(const std::string& uri)
{
    if (uri.empty()) {
        return MAX_UINT64_NUM;
    }
    uint64_t dotPos = 0;
    uint64_t queryPos = (uri.find('?') == std::string::npos) ? 0 : static_cast<uint64_t>(uri.find('?'));
    if (queryPos == 0) {
        bool isExitDotPos = uri.find_last_of('.') == std::string::npos;
        dotPos = isExitDotPos ? MAX_UINT64_NUM : static_cast<uint64_t>(uri.find_last_of('.'));
    } else {
        bool isExitQueryPos = uri.rfind('.', queryPos - 1) == std::string::npos;
        dotPos = isExitQueryPos ? MAX_UINT64_NUM : static_cast<uint64_t>(uri.rfind('.', queryPos - 1));
    }
    uint64_t start = dotPos;
    while (start > 0 && isdigit(static_cast<unsigned char>(uri[start - 1]))) {
        start--;
    }
    if (start == dotPos) {
        return MAX_UINT64_NUM;
    }
    uint64_t num = 0;
    for (uint64_t i = start; i < dotPos; ++i) {
        num = num * 10 + (uri[i] - '0'); // 10
    }
    return num;
}

void M3U8::UpdateFromTags(std::list<std::shared_ptr<Tag>>& tags)
{
    M3U8Info info;
    bLive_ = !info.bVod;
    size_t segmentTimeOffset = 0;
    size_t duration = 0;
    minFragDuration_.store(0);
    for (const auto &frag : files_) {
        duration += static_cast<size_t>(frag->duration_ * SECOND_TO_MICROSECOND);
    }
    for (auto& tag : tags) {
        if (isInterruptNeeded_.load()) {
            break;
        }
        HlsTag hlsTag = tag->GetType();
        if (hlsTag == HlsTag::EXTXENDLIST) {
            info.bVod = true;
            bLive_ = !info.bVod;
            MEDIA_LOG_I("UpdateFromTags not live with EXTXENDLIST.");
        }
        if (hlsTag == HlsTag::EXTXDISCONTINUITY) {
            segmentTimeOffset = duration;
            hasDiscontinuity_ = true;
            MEDIA_LOG_I("segmentTimeOffset here is: " PUBLIC_LOG_ZU, segmentTimeOffset);
            continue;
        }
        auto iter = tagUpdatersMap_.find(hlsTag);
        if (iter != tagUpdatersMap_.end() && iter->second) {
            auto updater = iter->second;
            updater(tag, info);
        }
        if (!info.uri.empty()) {
            if (ShouldSkipSegment(info.uri, skipSegmentsNum_, initSequence_)) {
                info.uri = "", info.duration = 0, info.discontinuity = false;
                continue;
            }
            ProcessInfo(info, duration);
        }
    }
}

void M3U8::ProcessInfo(M3U8Info& info, size_t& duration)
{
    if (!isFirstFragmentReady_ && !isDecryptAble_) {
        firstFragment_ = info;
        isFirstFragmentReady_ = true;
    }
    auto fragment = std::make_shared<M3U8Fragment>(info.uri, info.duration, sequence_, info.discontinuity);
    if (isDecryptAble_) {
        auto m3u8 = M3U8Fragment(info.uri, info.duration, sequence_, info.discontinuity);
        fragment = std::make_shared<M3U8Fragment>(m3u8, key_, iv_);
    }
    sequence_++;
    AddFile(fragment, duration);
    duration += static_cast<size_t>(info.duration * SECOND_TO_MICROSECOND);
    info.uri = "";
    info.duration = 0;
    info.discontinuity = false;
}

bool M3U8::ShouldSkipSegment(const std::string& uri, uint64_t skippedSegments, uint64_t initSequence)
{
    if (skippedSegments == 0) {
        return false;
    }
    uint64_t uriSequence = ExtractNumber(uri);
    return (uriSequence >= initSequence && uriSequence < initSequence + skippedSegments);
}

void M3U8::AddFile(std::shared_ptr<M3U8Fragment> fragment, size_t duration)
{
    if (minFragDuration_.load() <= 0 || minFragDuration_.load() > duration) {
        minFragDuration_.store(duration);
    }
    segmentOffsets_.emplace_back(duration);
    fragment->offset_ = offset_;
    fragment->length_ = length_;
    files_.emplace_back(fragment);
}

size_t M3U8::GetLiveUpdateGap() const
{
    if (minFragDuration_.load() <= 0) {
        return 0;
    }
    return (minFragDuration_.load() - 1) / 2; // 2
}

void M3U8::GetExtInf(const std::shared_ptr<Tag>& tag, double& duration) const
{
    auto item = std::static_pointer_cast<ValuesListTag>(tag);
    if (item == nullptr) {
        return;
    }
    if (item->GetAttributeByName("DURATION")) {
        duration = item->GetAttributeByName("DURATION")->FloatingPoint();
    }
}

double M3U8::GetDuration() const
{
    double duration = 0;
    for (auto file : files_) {
        duration += file->duration_;
    }

    MEDIA_LOG_I("duration = " PUBLIC_LOG_F ", files num " PUBLIC_LOG_ZU, duration, files_.size());
    return duration;
}

bool M3U8::IsLive() const
{
    return bLive_;
}

void M3U8::ParseKey(const std::shared_ptr<AttributesTag> &tag)
{
    FALSE_RETURN(tag != nullptr);
    auto methodAttribute = tag->GetAttributeByName("METHOD");
    if (methodAttribute) {
        method_ = std::make_shared<std::string>(methodAttribute->QuotedString());
    }
    auto uriAttribute = tag->GetAttributeByName("URI");
    if (uriAttribute) {
        keyUri_ = std::make_shared<std::string>(uriAttribute->QuotedString());
    }
    auto ivAttribute = tag->GetAttributeByName("IV");
    if (ivAttribute) {
        std::vector<uint8_t> iv_buff = ivAttribute->HexSequence();
        uint32_t size = iv_buff.size() > MAX_LOOP ? MAX_LOOP : iv_buff.size();
        for (uint32_t i = 0; i < size; i++) {
            iv_[i] = iv_buff[i];
        }
    }
}

void M3U8::DownloadKey()
{
    if (keyUri_ == nullptr) {
        return;
    }

    if (sourceLoader_ != nullptr) {
        downloader_ = std::make_shared<Downloader>("hlsSourceKey", sourceLoader_);
    } else {
        downloader_ = std::make_shared<Downloader>("hlsSourceKey");
    }

    if (downloader_ != nullptr && downloadCallback_ != nullptr) {
        downloader_->SetDownloadCallback(downloadCallback_);
    }
    downloader_->Init();
    auto weakDownloader = weak_from_this();
    dataSave_ = [weakDownloader](uint8_t *&&data, uint32_t &&len, bool &&notBlock) -> uint32_t {
        auto shareDownloader = weakDownloader.lock();
        FALSE_RETURN_V_MSG(shareDownloader != nullptr, 0u, "dataSave, M3U8 key downloader already destructed.");
        return shareDownloader->SaveData(std::forward<decltype(data)>(data), std::forward<decltype(len)>(len),
            notBlock);
    };
    // this is default callback
    statusCallback_ = [weakDownloader](DownloadStatus &&status, std::shared_ptr<Downloader> d,
        std::shared_ptr<DownloadRequest> &request) {
        auto shareDownloader = weakDownloader.lock();
        FALSE_RETURN_MSG(shareDownloader != nullptr, "statusCb, M3U8 key downloader already destructed.");
        shareDownloader->OnDownloadStatus(std::forward<decltype(status)>(status), shareDownloader->downloader_,
            std::forward<decltype(request)>(request));
    };
    auto realStatusCallback = [weakDownloader](DownloadStatus &&status, std::shared_ptr<Downloader> &downloader,
        std::shared_ptr<DownloadRequest> &request) {
        auto shareDownloader = weakDownloader.lock();
        FALSE_RETURN_MSG(shareDownloader != nullptr, "realStatusCb, M3U8 key downloader already destructed.");
        shareDownloader->monitorStatusCallback_(status, shareDownloader->downloader_,
            std::forward<decltype(request)>(request));
        shareDownloader->statusCallback_(status, shareDownloader->downloader_,
            std::forward<decltype(request)>(request));
    };
    std::string realKeyUrl = UriJoin(uri_, *keyUri_);
    RequestInfo requestInfo;
    requestInfo.url = realKeyUrl;
    requestInfo.httpHeader = httpHeader_;
    downloadRequest_ = std::make_shared<DownloadRequest>(dataSave_, realStatusCallback, requestInfo, true);
    downloadRequest_->SetIsAuthRequest(true);
    downloader_->Download(downloadRequest_, -1);
    downloader_->Start();
}

uint32_t M3U8::SaveData(uint8_t *data, uint32_t len, bool notBlock)
{
    FALSE_RETURN_V(data != nullptr, 0);
    // 16 is a magic number
    if (len <= MAX_LOOP && len != 0) {
        NZERO_RETURN_V(memcpy_s(key_, MAX_LOOP, data, len), 0);
        keyLen_ = len;
        isDecryptKeyReady_ = true;
        MEDIA_LOG_I("DownloadKey hlsSourceKey end.\n");
        return len;
    }
    return 0;
}

void M3U8::OnDownloadStatus(DownloadStatus status, std::shared_ptr<Downloader> &,
    std::shared_ptr<DownloadRequest> &request)
{
    FALSE_RETURN(request != nullptr);
    // This should not be called normally
    if (request->GetClientError() != 0 || request->GetServerError() != 0) {
        MEDIA_LOG_E("OnDownloadStatus " PUBLIC_LOG_D32, status);
    }
}

/**
 * @brief Parse the data in the URI and obtain pssh data and uuid from it.
 * @param drmInfo Map data of uuid and pssh.
 * @return bool true: success false: invalid parameter
 */
bool M3U8::SetDrmInfo(std::multimap<std::string, std::vector<uint8_t>>& drmInfo)
{
    std::string::size_type n;
    std::string psshString;
    std::vector<uint8_t> pssh; // 2048: pssh len
    uint32_t psshSize = 2048; // 2048: pssh len
    if (keyUri_ == nullptr) {
        return false;
    }
    n = keyUri_->find("base64,");
    if (n != std::string::npos) {
        psshString = keyUri_->substr(n + 7); // 7: len of "base64,"
    }
    if (psshString.length() == 0) {
        return false;
    }
    bool ret = Base64Utils::Base64Decode(reinterpret_cast<const uint8_t *>(psshString.c_str()),
        static_cast<uint32_t>(psshString.length()), pssh.data(), &psshSize);
    if (ret) {
        uint32_t uuidSize = 16; // 16: uuid len
        if (psshSize >= DRM_UUID_OFFSET + uuidSize) {
            uint8_t uuid[16]; // 16: uuid len
            NZERO_RETURN_V(memcpy_s(uuid, sizeof(uuid), pssh.data() + DRM_UUID_OFFSET, uuidSize), false);
            std::stringstream ssConverter;
            std::string uuidString;
            for (uint32_t i = 0; i < uuidSize; i++) {
                ssConverter << std::hex << std::setfill('0') << std::setw(2) << static_cast<int32_t>(uuid[i]); // 2:w
                uuidString = ssConverter.str();
            }
            drmInfo.insert({ uuidString, std::vector<uint8_t>(pssh.data(), pssh.data() + psshSize) });
            return true;
        }
    }
    return false;
}

/**
 * @brief Store uuid and pssh data.
 * @param drmInfo Map data of uuid and pssh.
 */
void M3U8::StoreDrmInfos(const std::multimap<std::string, std::vector<uint8_t>>& drmInfo)
{
    MEDIA_LOG_I("StoreDrmInfos");
    for (auto &newItem : drmInfo) {
        auto pos = localDrmInfos_.equal_range(newItem.first);
        if (pos.first == pos.second && pos.first == localDrmInfos_.end()) {
            MEDIA_LOG_D("this uuid not exists and update");
            localDrmInfos_.insert(newItem);
            continue;
        }
        MEDIA_LOG_D("this uuid exists many times");
        bool isSame = false;
        for (; pos.first != pos.second; ++pos.first) {
            if (newItem.second == pos.first->second) {
                MEDIA_LOG_D("this uuid exists and same pssh");
                isSame = true;
                break;
            }
        }
        if (!isSame) {
            MEDIA_LOG_D("this uuid exists but pssh not same");
            localDrmInfos_.insert(newItem);
        }
    }
    return;
}

/**
 * @brief Process uuid and pssh data.
 */
void M3U8::ProcessDrmInfos(void)
{
    isDecryptAble_ = false;
    std::multimap<std::string, std::vector<uint8_t>> drmInfo;
    bool ret = SetDrmInfo(drmInfo);
    if (ret) {
        StoreDrmInfos(drmInfo);
    }
}

M3U8VariantStream::M3U8VariantStream(const std::string &name, const std::string &uri, std::shared_ptr<M3U8> m3u8)
    : name_(std::move(name)), uri_(std::move(uri)), m3u8_(std::move(m3u8))
{
}

M3U8MasterPlaylist::M3U8MasterPlaylist(const std::string& playList, const std::string& uri, uint32_t initResolution,
    const std::map<std::string, std::string>& httpHeader, StatusCallbackFunc statusCallback)
{
    playList_ = playList;
    uri_ = uri;
    httpHeader_ = httpHeader;
    initResolution_ = initResolution;
    monitorStatusCallback_ = statusCallback;
}

M3U8MasterPlaylist::~M3U8MasterPlaylist()
{
    NZERO_LOG(memset_s(key_, sizeof(key_), 0, sizeof(key_)));
    NZERO_LOG(memset_s(iv_, sizeof(iv_), 0, sizeof(iv_)));
}

void M3U8MasterPlaylist::SetSourceloader(std::shared_ptr<MediaSourceLoaderCombinations> sourceLoader)
{
    sourceLoader_ = sourceLoader;
}
void M3U8MasterPlaylist::SetDownloadCallback(const std::shared_ptr<DownloadMetricsInfo> &callback)
{
    downloadCallback_ = callback;
}

void M3U8MasterPlaylist::StartParsing()
{
    if (!StrHasPrefix(playList_, "#EXTM3U")) {
        MEDIA_LOG_I("playlist doesn't start with #EXTM3U ");
        isParseSuccess_ = false;
    }
    if (playList_.find("\n#EXTINF:") != std::string::npos) {
        UpdateMediaPlaylist();
    } else {
        UpdateMasterPlaylist();
    }
}

void M3U8MasterPlaylist::UpdateMediaPlaylist()
{
    MEDIA_LOG_I("This is a simple media playlist, not a master playlist");
    auto m3u8 = std::make_shared<M3U8>(uri_, "", tagMasterMap_, monitorStatusCallback_, sourceLoader_);
    FALSE_RETURN_NOLOG(m3u8 != nullptr);
    if (downloadCallback_ != nullptr) {
        m3u8->SetDownloadCallback(downloadCallback_);
    }
    auto stream = std::make_shared<M3U8VariantStream>(uri_, uri_, m3u8);
    FALSE_RETURN_NOLOG(stream != nullptr);
    stream->streamId_ = ++defaultStreamId_;
    variants_.emplace_back(stream);
    defaultVariant_ = stream;
    if (isDecryptAble_) {
        m3u8->isDecryptAble_ = isDecryptAble_;
        std::copy(std::begin(key_), std::end(key_), std::begin(m3u8->key_));
        m3u8->isDecryptKeyReady_ = isDecryptKeyReady_;
        std::copy(std::begin(iv_), std::end(iv_), std::begin(m3u8->iv_));
        m3u8->keyLen_ = keyLen_;
    }
    m3u8->httpHeader_ = httpHeader_;
    isParseSuccess_ = m3u8->Update(playList_, false);
    hasDiscontinuity_ = m3u8->hasDiscontinuity_;
    segmentOffsets_ = m3u8->segmentOffsets_;
    duration_ = m3u8->GetDuration();
    bLive_ = m3u8->IsLive();
    isFmp4_ = m3u8->isHeaderReady_.load();
    isPureByteRange_.store(m3u8->isPureByteRange_.load());
    isSimple_ = true;
    MEDIA_LOG_I("UpdateMediaPlaylist called, duration_ = " PUBLIC_LOG_F, duration_);
}

void M3U8MasterPlaylist::DownloadSessionKey(std::shared_ptr<Tag>& tag)
{
    auto m3u8 = std::make_shared<M3U8>(uri_, "", tagMasterMap_, monitorStatusCallback_, sourceLoader_);
    FALSE_RETURN_NOLOG(m3u8 != nullptr);
    if (downloadCallback_ != nullptr) {
        m3u8->SetDownloadCallback(downloadCallback_);
    }
    m3u8->isDecryptAble_ = true;
    m3u8->isDecryptKeyReady_ = false;
    m3u8->ParseKey(std::static_pointer_cast<AttributesTag>(tag));
    m3u8->DownloadKey();
    uint32_t downloadTime = 0;
    while (!m3u8->isDecryptKeyReady_ && downloadTime < MAX_DOWNLOAD_TIME && !isInterruptNeeded_) {
        Task::SleepInTask(WAIT_KEY_SLEEP_TIME);
        downloadTime++;
    }
    std::copy(std::begin(m3u8->key_), std::end(m3u8->key_), std::begin(key_));
    isDecryptKeyReady_ = m3u8->isDecryptKeyReady_;
    std::copy(std::begin(m3u8->iv_), std::end(m3u8->iv_), std::begin(iv_));
    keyLen_ = m3u8->keyLen_;
}

void M3U8MasterPlaylist::UpdateMasterPlaylist()
{
    MEDIA_LOG_I("master playlist");
    std::pair<std::list<std::shared_ptr<Tag>>, std::unordered_map<std::string, std::string>> ret;
    if (playList_.find(EXT_X_DEFINE_TAG) != std::string::npos) {
        std::unordered_map<std::string, std::string> tagMasterMap;
        std::unordered_map<std::string, std::string> tagUriMap;
        if (!uri_.empty() && uri_.find("?") != std::string::npos) {
            tagUriMap = ParseUriQuery(uri_);
        }
        ret = ParseEntries(playList_, tagMasterMap, tagUriMap);
    } else {
        ret = ParseEntries(playList_);
    }
    std::list<std::shared_ptr<Tag>> tags = ret.first;
    if (!ret.second.empty()) {
        tagMasterMap_ = ret.second;
    }
    ProcessAllTags(tags);
    BindVideoMedia("audio");
    BindVideoMedia("subtitles");
    if (defaultVariant_ == nullptr && !variants_.empty()) {
        if (firstVideoStream_ != nullptr) {
            defaultVariant_ = firstVideoStream_;
        } else {
            defaultVariant_ = variants_.back();
        }
    }
    tags.clear();
    ChooseStreamByResolution();
}

void M3U8MasterPlaylist::ProcessAllTags(std::list<std::shared_ptr<Tag>>& tags)
{
    std::for_each(tags.begin(), tags.end(), [this] (std::shared_ptr<Tag>& tag) {
        FALSE_RETURN(tag != nullptr);
        switch (tag->GetType()) {
            case HlsTag::EXTXSESSIONKEY:
                DownloadSessionKey(tag);
                break;
            case HlsTag::EXTXIFRAMESTREAMINF:
                break;
            case HlsTag::EXTXSTREAMINF: {
                CreateVariantStream(tag);
                break;
            }
            case HlsTag::EXTXMEDIA: {
                ParseMediaStreamInfo(tag);
                break;
            }
            case HlsTag::EXTXSTART: {
                ParseStart(tag);
                break;
            }
            default:
                break;
        }
    });
}

void M3U8MasterPlaylist::CreateVariantStream(const std::shared_ptr<Tag>& tag)
{
    FALSE_RETURN(tag != nullptr);
    auto item = std::static_pointer_cast<AttributesTag>(tag);
    auto uriAttribute = item->GetAttributeByName("URI");
    if (uriAttribute) {
        auto name = uriAttribute->QuotedString();
        auto uri = UriJoin(uri_, name);
        auto m3u8 = std::make_shared<M3U8>(uri, name, tagMasterMap_, monitorStatusCallback_, sourceLoader_);
        if (m3u8 != nullptr && downloadCallback_ != nullptr) {
            m3u8->SetDownloadCallback(downloadCallback_);
        }
        auto stream = std::make_shared<M3U8VariantStream>(name, uri, m3u8);
        FALSE_RETURN_MSG(stream != nullptr, "UpdateMasterPlaylist memory not enough");
        stream->streamId_ = ++defaultStreamId_;
        if (tag->GetType() == HlsTag::EXTXIFRAMESTREAMINF) {
            stream->iframe_ = true;
        }
        ParseAttributes(item, stream);
        variants_.emplace_back(stream);
        auto codecs = item->GetAttributeByName("CODECS");
        if (codecs && IsVideoStream(codecs->QuotedString())) {
            stream->isVideo_ = true;
            if (firstVideoStream_ == nullptr) {
                firstVideoStream_= stream;
            }
            if (stream->bandWidth_ <= BAND_WIDTH_LIMIT) {
                defaultVariant_ = stream; // play last stream
            }
        }
    }
}

void M3U8MasterPlaylist::ParseAttributes(const std::shared_ptr<AttributesTag>& item,
    std::shared_ptr<M3U8VariantStream>& stream)
{
    auto bandWidthAttribute = item->GetAttributeByName("BANDWIDTH");
    if (bandWidthAttribute) {
        stream->bandWidth_ = bandWidthAttribute->Decimal();
    }
    auto resolutionAttribute = item->GetAttributeByName("RESOLUTION");
    if (resolutionAttribute) {
        stream->width_ = resolutionAttribute->GetResolution().first;
        stream->height_ = resolutionAttribute->GetResolution().second;
    }
    auto codecsAttribute = item->GetAttributeByName("CODECS");
    if (codecsAttribute) {
        stream->codecs_ = codecsAttribute->QuotedString();
    }
    auto subtitlesAttribute = item->GetAttributeByName("SUBTITLES");
    if (subtitlesAttribute) {
        stream->subtitles_ = subtitlesAttribute->QuotedString();
    }
    auto audioAttribute = item->GetAttributeByName("AUDIO");
    if (audioAttribute) {
        stream->audio_ = audioAttribute->QuotedString();
    }
}

void M3U8MasterPlaylist::ParseMediaStreamInfo(std::shared_ptr<Tag> &tag)
{
    FALSE_RETURN(tag != nullptr);
    auto item = std::static_pointer_cast<AttributesTag>(tag);
    auto uriAttribute = item->GetAttributeByName("URI");
    if (uriAttribute) {
        auto curUriValue = uriAttribute->QuotedString();
        auto uri = UriJoin(uri_, curUriValue);
        auto m3u8 = std::make_shared<M3U8>(uri, curUriValue, tagMasterMap_, monitorStatusCallback_, sourceLoader_);
        if (m3u8 != nullptr && downloadCallback_ != nullptr) {
            m3u8->SetDownloadCallback(downloadCallback_);
        }
        auto media = std::make_shared<M3U8Media>(curUriValue, uri, m3u8);
        FALSE_RETURN_MSG(media != nullptr, "UpdateMasterPlaylist memory not enough");
        media->streamId_ = ++defaultStreamId_;
        auto typeAttr = item->GetAttributeByName("TYPE");
        if (typeAttr) {
            std::string type = typeAttr->QuotedString();
            auto it = kTypeMap.find(type);
            media->type_ = (it != kTypeMap.end()) ? it->second : M3U8MediaType::M3U8_N_MEDIA_TYPES;
        }
        auto setStringAttr = [&item](const std::string& name, std::string& target) {
            if (auto attr = item->GetAttributeByName(name.c_str())) {
                target = attr->QuotedString();
            }
        };

        auto setBoolAttr = [&item](const std::string& name, bool& target) {
            if (auto attr = item->GetAttributeByName(name.c_str())) {
                target = (attr->QuotedString() == "YES");
            }
        };

        setStringAttr("GROUP-ID", media->groupID_);
        setStringAttr("NAME", media->name_);
        setStringAttr("CHANNELS", media->channels_);
        setStringAttr("LANGUAGE", media->lang_);
        setStringAttr("INSTREAM-ID", media->instreamId_);
        setStringAttr("CHARACTERISTICS", media->characteristics_);

        setBoolAttr("DEFAULT", media->isDefault_);
        setBoolAttr("AUTOSELECT", media->autoSelect_);
        setBoolAttr("FORCED", media->forced_);

        if (media->type_ == M3U8MediaType::M3U8_MEDIA_TYPE_AUDIO) {
            audioList_.emplace_back(media);
        }
        if (media->type_ == M3U8MediaType::M3U8_MEDIA_TYPE_SUBTITLES) {
            subtitlesList_.emplace_back(media);
        }
    }
}

void M3U8MasterPlaylist::ParseStart(std::shared_ptr<Tag> &tag)
{
    FALSE_RETURN(tag != nullptr);
    auto item = std::static_pointer_cast<AttributesTag>(tag);
    auto timeOffset = item->GetAttributeByName("TIME-OFFSET");
    if (timeOffset) {
        timeOffset_ = timeOffset->Decimal();
    }
    auto precise = item->GetAttributeByName("PRECISE");
    if (precise) {
        precise_ = (precise->QuotedString() == "YES") ? true : false;
    }
    isStart_ = true;
}

void M3U8MasterPlaylist::BindVideoMedia(std::string mediaType)
{
    std::list<std::shared_ptr<M3U8Media>> mediaList;
    if (mediaType == "audio" && !audioList_.empty()) {
        mediaList = audioList_;
    } else if (!subtitlesList_.empty()) {
        mediaList = subtitlesList_;
    } else {
        MEDIA_LOG_I("BindVideoMedia, no.");
        return;
    }
    for (auto &videoStream : variants_) {
        if (videoStream == nullptr) {
            continue;
        }
        auto& videoStreamId = (mediaType == "audio") ? videoStream->audio_ : videoStream->subtitles_;
        auto& videoStreamMedia = (mediaType == "audio") ? videoStream->audioMedia_ : videoStream->subtitlesMedia_;
        if (videoStreamId.empty()) {
            videoStreamMedia.insert(videoStreamMedia.end(), mediaList.begin(), mediaList.end());
            GetDefaultMedieStream(videoStream, mediaType);
            continue;
        }
        for (const auto &mediaStream : mediaList) {
            if (mediaStream == nullptr) {
                continue;
            }
            if (videoStreamId != mediaStream->groupID_) {
                MEDIA_LOG_I("not compare: video %{public}s, %{public}s %s", videoStreamId.c_str(),
                    mediaType.c_str(), mediaStream->groupID_.c_str());
                continue;
            }
            videoStreamMedia.push_back(mediaStream);
        }
        if (videoStreamMedia.empty()) {
            videoStreamMedia.push_back(mediaList.back());
        }
        GetDefaultMedieStream(videoStream, mediaType);
    }
}

void M3U8MasterPlaylist::GetDefaultMedieStream(std::shared_ptr<M3U8VariantStream>& videoStream, std::string mediaType)
{
    FALSE_RETURN(videoStream != nullptr);
    const auto &allMedia = mediaType == "audio" ? videoStream->audioMedia_ : videoStream->subtitlesMedia_;
    if (allMedia.empty()) {
        return;
    }
    auto &mediaStream = mediaType == "audio" ? videoStream->defaultAudio_ : videoStream->defaultSubtitles_;
    auto forcedItor = std::find_if(allMedia.rbegin(), allMedia.rend(),
        [] (const std::shared_ptr<M3U8Media>& mediaStream) {
            FALSE_RETURN_V(mediaStream != nullptr, false);
            return mediaStream->forced_;
        });
    if (forcedItor != allMedia.rend()) {
        mediaStream = *forcedItor;
        MEDIA_LOG_I("GetDefaultMediaStream type: %{public}s, streamId:" PUBLIC_LOG_U32, mediaType.c_str(),
            mediaStream->streamId_);
        return;
    }
    auto defaultItor = std::find_if(allMedia.rbegin(), allMedia.rend(),
        [] (const std::shared_ptr<M3U8Media>& mediaStream) {
            FALSE_RETURN_V(mediaStream != nullptr, false);
            return mediaStream->isDefault_ && mediaStream->characteristics_.empty();
        });
    if (defaultItor != allMedia.rend()) {
        mediaStream = *defaultItor;
        MEDIA_LOG_I("GetDefaultMediaStream type: %{public}s, streamId:" PUBLIC_LOG_U32, mediaType.c_str(),
            mediaStream->streamId_);
        return;
    }
    auto autoSelectItor = std::find_if(allMedia.rbegin(), allMedia.rend(),
        [] (const std::shared_ptr<M3U8Media>& mediaStream) {
            FALSE_RETURN_V(mediaStream != nullptr, false);
            return mediaStream->autoSelect_;
        });
    if (autoSelectItor != allMedia.rend()) {
        mediaStream = *autoSelectItor;
        MEDIA_LOG_I("GetDefaultMediaStream type: %{public}s, streamId:" PUBLIC_LOG_U32, mediaType.c_str(),
            mediaStream->streamId_);
        return;
    }
    mediaStream = allMedia.back();
    MEDIA_LOG_I("GetDefaultMediaStream type: %{public}s, streamId:" PUBLIC_LOG_U32, mediaType.c_str(),
            mediaStream->streamId_);
}

bool M3U8MasterPlaylist::IsVideoStream(const std::string& codecs)
{
    if (codecs.empty()) {
        return false;
    }
    std::string lowerCodecs = codecs;
    std::transform(lowerCodecs.begin(), lowerCodecs.end(), lowerCodecs.begin(), ::tolower);

    std::vector<std::string> parts;
    std::istringstream iss(lowerCodecs);
    std::string token;

    while (std::getline(iss, token, ',')) {
        auto start = token.find_first_not_of(" \t\r\n");
        auto end = token.find_last_not_of(" \t\r\n");
        if (start == std::string::npos) {
            continue;
        }
        token = token.substr(start, end - start + 1);

        std::istringstream subIss(token);
        std::string subToken;
        while (std::getline(subIss, subToken, '.')) {
            auto subStart = subToken.find_first_not_of(" \t\r\n");
            auto subEnd = subToken.find_last_not_of(" \t\r\n");
            if (subStart == std::string::npos) {
                continue;
            }
            std::string targetCodecs = subToken.substr(subStart, subEnd - subStart + 1);
            if (VIDEO_CODECS.find(targetCodecs) != VIDEO_CODECS.end()) {
                return true;
            }
        }
    }
    return false;
}

void M3U8MasterPlaylist::ChooseStreamByResolution()
{
    FALSE_RETURN_MSG(defaultVariant_ != nullptr, "defaultVariant is nullptr.");
    if (initResolution_ == 0) {
        return;
    }
    for (const auto &variant : variants_) {
        if (variant == nullptr) {
            continue;
        }
        if (IsNearToInitResolution(defaultVariant_, variant)) {
            defaultVariant_ = variant;
        }
    }
    MEDIA_LOG_I("resolution, width:" PUBLIC_LOG_U32 ", height:" PUBLIC_LOG_U32,
                defaultVariant_->width_, defaultVariant_->height_);
}

bool M3U8MasterPlaylist::IsNearToInitResolution(const std::shared_ptr<M3U8VariantStream> &choosedStream,
    const std::shared_ptr<M3U8VariantStream> &currentStream)
{
    if (choosedStream == nullptr || currentStream == nullptr || initResolution_ == 0) {
        return false;
    }

    uint32_t choosedDelta = GetResolutionDelta(choosedStream->width_, choosedStream->height_);
    uint32_t currentDelta = GetResolutionDelta(currentStream->width_, currentStream->height_);
    return (currentDelta < choosedDelta)
           || (currentDelta == choosedDelta && currentStream->bandWidth_ < choosedStream->bandWidth_);
}

uint32_t M3U8MasterPlaylist::GetResolutionDelta(uint32_t width, uint32_t height)
{
    uint32_t resolution = width * height;
    if (resolution > initResolution_) {
        return resolution - initResolution_;
    } else {
        return initResolution_ - resolution;
    }
}

void M3U8MasterPlaylist::SetInterruptState(bool isInterruptNeeded)
{
    isInterruptNeeded_ = isInterruptNeeded;
    if (defaultVariant_ && defaultVariant_->m3u8_) {
        defaultVariant_->m3u8_->isInterruptNeeded_ = isInterruptNeeded;
        defaultVariant_->m3u8_->sleepCond_.NotifyOne();
    }
    MEDIA_LOG_I("M3U8MasterPlaylist SetInterruptState %{public}d.", isInterruptNeeded);
}

M3U8Media::M3U8Media(const std::string &name, const std::string &uri, std::shared_ptr<M3U8> m3u8)
    : name_(std::move(name)), uri_(std::move(uri)), m3u8_(std::move(m3u8))
{
}

}
}
}
}
