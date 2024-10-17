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
#define HST_LOG_TAG "M3U8"

#include <algorithm>
#include <utility>
#include <sstream>
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

const char DRM_PSSH_TITLE[] = "data:text/plain;";

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
        std::string::size_type pos = baseUrl.rfind('/');
        return baseUrl.substr(0, pos + 1) + uri;
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

M3U8Fragment::M3U8Fragment(std::string uri, double duration, int sequence, bool discont)
    : uri_(std::move(uri)), duration_(duration), sequence_(sequence), discont_(discont)
{
}

M3U8::M3U8(std::string uri, std::string name) : uri_(std::move(uri)), name_(std::move(name))
{
    InitTagUpdatersMap();
}

M3U8::~M3U8()
{
    if (downloader_) {
        downloader_ = nullptr;
    }
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
    }
    MEDIA_LOG_I("media playlist");
    auto tags = ParseEntries(playList);
    UpdateFromTags(tags);
    tags.clear();
    playList_ = playList;
    return true;
}

void M3U8::InitTagUpdaters()
{
    tagUpdatersMap_[HlsTag::EXTXPLAYLISTTYPE] = [this](std::shared_ptr<Tag> &tag, const M3U8Info &info) {
        bLive_ = !info.bVod && (std::static_pointer_cast<SingleValueTag>(tag)->GetValue().QuotedString() != "VOD");
    };
    tagUpdatersMap_[HlsTag::EXTXTARGETDURATION] = [this](std::shared_ptr<Tag> &tag, const M3U8Info &info) {
        std::ignore = info;
        targetDuration_ = std::static_pointer_cast<SingleValueTag>(tag)->GetValue().FloatingPoint();
    };
    tagUpdatersMap_[HlsTag::EXTXMEDIASEQUENCE] = [this](std::shared_ptr<Tag> &tag, const M3U8Info &info) {
        std::ignore = info;
        sequence_ = std::static_pointer_cast<SingleValueTag>(tag)->GetValue().Decimal();
    };
    tagUpdatersMap_[HlsTag::EXTXDISCONTINUITYSEQUENCE] = [this](std::shared_ptr<Tag> &tag, M3U8Info &info) {
        discontSequence_ = static_cast<int>(std::static_pointer_cast<SingleValueTag>(tag)->GetValue().Decimal());
        info.discontinuity = true;
    };
    tagUpdatersMap_[HlsTag::EXTINF] = [this](const std::shared_ptr<Tag> &tag, M3U8Info &info) {
        GetExtInf(tag, info.duration);
    };
    tagUpdatersMap_[HlsTag::URI] = [this](std::shared_ptr<Tag> &tag, M3U8Info &info) {
        info.uri = UriJoin(uri_, std::static_pointer_cast<SingleValueTag>(tag)->GetValue().QuotedString());
    };
    tagUpdatersMap_[HlsTag::EXTXBYTERANGE] = [](const std::shared_ptr<Tag> &tag, const M3U8Info &info) {
        std::ignore = tag;
        std::ignore = info;
        MEDIA_LOG_I("need to parse EXTXBYTERANGE");
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
        if (!isDecryptAble_ && !isDecryptKeyReady_) {
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
    };
    tagUpdatersMap_[HlsTag::EXTXMAP] = [](const std::shared_ptr<Tag> &tag, const M3U8Info &info) {
        std::ignore = tag;
        std::ignore = info;
        MEDIA_LOG_I("need to parse EXTXMAP");
    };
}

void M3U8::UpdateFromTags(std::list<std::shared_ptr<Tag>>& tags)
{
    M3U8Info info;
    info.bVod = !tags.empty() && tags.back()->GetType() == HlsTag::EXTXENDLIST;
    bLive_ = !info.bVod;
    for (auto& tag : tags) {
        HlsTag hlsTag = tag->GetType();
        auto iter = tagUpdatersMap_.find(hlsTag);
        if (iter != tagUpdatersMap_.end()) {
            auto updater = iter->second;
            updater(tag, info);
        }

        if (!info.uri.empty()) {
            if (!isFirstFragmentReady_ && !isDecryptAble_) {
                firstFragment_ = info;
                isFirstFragmentReady_ = true;
            }
            // add key_ and iv_ to M3U8Fragment(file)
            if (isDecryptAble_) {
                auto m3u8 = M3U8Fragment(info.uri, info.duration, sequence_++, info.discontinuity);
                auto fragment = std::make_shared<M3U8Fragment>(m3u8, key_, iv_);
                files_.emplace_back(fragment);
            } else {
                auto fragment = std::make_shared<M3U8Fragment>(info.uri, info.duration, sequence_++,
                    info.discontinuity);
                files_.emplace_back(fragment);
            }
            info.uri = "", info.duration = 0, info.discontinuity = false;
        }
    }
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

    return duration;
}

bool M3U8::IsLive() const
{
    return bLive_;
}

void M3U8::ParseKey(const std::shared_ptr<AttributesTag> &tag)
{
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

    downloader_ = std::make_shared<Downloader>("hlsSourceKey");
    dataSave_ = [this](uint8_t *&&data, uint32_t &&len) {
        return SaveData(std::forward<decltype(data)>(data), std::forward<decltype(len)>(len));
    };
    // this is default callback
    statusCallback_ = [this](DownloadStatus &&status, std::shared_ptr<Downloader> d,
        std::shared_ptr<DownloadRequest> &request) {
        OnDownloadStatus(std::forward<decltype(status)>(status), downloader_, std::forward<decltype(request)>(request));
    };
    auto realStatusCallback = [this](DownloadStatus &&status, std::shared_ptr<Downloader> &downloader,
        std::shared_ptr<DownloadRequest> &request) {
        statusCallback_(status, downloader_, std::forward<decltype(request)>(request));
    };
    std::string realKeyUrl = UriJoin(uri_, *keyUri_);
    downloadRequest_ = std::make_shared<DownloadRequest>(realKeyUrl, dataSave_, realStatusCallback, true);
    downloader_->Download(downloadRequest_, -1);
    downloader_->Start();
}

bool M3U8::SaveData(uint8_t *data, uint32_t len)
{
    // 16 is a magic number
    if (len <= MAX_LOOP && len != 0) {
        NZERO_RETURN_V(memcpy_s(key_, MAX_LOOP, data, len), false);
        keyLen_ = len;
        isDecryptKeyReady_ = true;
        MEDIA_LOG_I("DownloadKey hlsSourceKey end.\n");
        return true;
    }
    return false;
}

void M3U8::OnDownloadStatus(DownloadStatus status, std::shared_ptr<Downloader> &,
    std::shared_ptr<DownloadRequest> &request)
{
    // This should not be called normally
    if (request->GetClientError() != NetworkClientErrorCode::ERROR_OK || request->GetServerError() != 0) {
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
    uint8_t pssh[2048]; // 2048: pssh len
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
        static_cast<uint32_t>(psshString.length()), pssh, &psshSize);
    if (ret) {
        uint32_t uuidSize = 16; // 16: uuid len
        if (psshSize >= DRM_UUID_OFFSET + uuidSize) {
            uint8_t uuid[16]; // 16: uuid len
            NZERO_RETURN_V(memcpy_s(uuid, sizeof(uuid), pssh + DRM_UUID_OFFSET, uuidSize), false);
            std::stringstream ssConverter;
            std::string uuidString;
            for (uint32_t i = 0; i < uuidSize; i++) {
                ssConverter << std::hex << static_cast<int32_t>(uuid[i]);
                uuidString = ssConverter.str();
            }
            drmInfo.insert({ uuidString, std::vector<uint8_t>(pssh, pssh + psshSize) });
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

M3U8VariantStream::M3U8VariantStream(std::string name, std::string uri, std::shared_ptr<M3U8> m3u8)
    : name_(std::move(name)), uri_(std::move(uri)), m3u8_(std::move(m3u8))
{
}

M3U8MasterPlaylist::M3U8MasterPlaylist(const std::string& playList, const std::string& uri)
{
    playList_ = playList;
    uri_ = uri;
    if (!StrHasPrefix(playList_, "#EXTM3U")) {
        MEDIA_LOG_I("playlist doesn't start with #EXTM3U ");
    }
    if (playList_.find("\n#EXTINF:") != std::string::npos) {
        UpdateMediaPlaylist();
    } else {
        UpdateMasterPlaylist();
    }
}

void M3U8MasterPlaylist::UpdateMediaPlaylist()
{
    MEDIA_LOG_I("This is a simple media playlist, not a master playlist ");
    auto m3u8 = std::make_shared<M3U8>(uri_, "");
    auto stream = std::make_shared<M3U8VariantStream>(uri_, uri_, m3u8);
    variants_.emplace_back(stream);
    defaultVariant_ = stream;
    if (isDecryptAble_) {
        m3u8->isDecryptAble_ = isDecryptAble_;
        std::copy(std::begin(key_), std::end(key_), std::begin(m3u8->key_));
        m3u8->isDecryptKeyReady_ = isDecryptKeyReady_;
        std::copy(std::begin(iv_), std::end(iv_), std::begin(m3u8->iv_));
        m3u8->keyLen_ = keyLen_;
    }
    m3u8->Update(playList_, false);
    duration_ = m3u8->GetDuration();
    bLive_ = m3u8->IsLive();
    isSimple_ = true;
    MEDIA_LOG_D("UpdateMediaPlaylist called, duration_ = " PUBLIC_LOG_F, duration_);
}

void M3U8MasterPlaylist::DownloadSessionKey(std::shared_ptr<Tag>& tag)
{
    auto m3u8 = std::make_shared<M3U8>(uri_, "");
    m3u8->isDecryptAble_ = true;
    m3u8->isDecryptKeyReady_ = false;
    m3u8->ParseKey(std::static_pointer_cast<AttributesTag>(tag));
    m3u8->DownloadKey();
    uint32_t downloadTime = 0;
    while (!m3u8->isDecryptKeyReady_ && downloadTime < MAX_DOWNLOAD_TIME) {
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
    auto tags = ParseEntries(playList_);
    std::for_each(tags.begin(), tags.end(), [this] (std::shared_ptr<Tag>& tag) {
        switch (tag->GetType()) {
            case HlsTag::EXTXSESSIONKEY:
                DownloadSessionKey(tag);
                break;
            case HlsTag::EXTXSTREAMINF:
            case HlsTag::EXTXIFRAMESTREAMINF: {
                auto item = std::static_pointer_cast<AttributesTag>(tag);
                auto uriAttribute = item->GetAttributeByName("URI");
                if (uriAttribute) {
                    auto name = uriAttribute->QuotedString();
                    auto uri = UriJoin(uri_, name);
                    auto stream = std::make_shared<M3U8VariantStream>(name, uri, std::make_shared<M3U8>(uri, name));
                    FALSE_RETURN_MSG(stream != nullptr, "UpdateMasterPlaylist memory not enough");
                    if (tag->GetType() == HlsTag::EXTXIFRAMESTREAMINF) {
                        stream->iframe_ = true;
                    }
                    auto bandWidthAttribute = item->GetAttributeByName("BANDWIDTH");
                    if (bandWidthAttribute) {
                        stream->bandWidth_ = bandWidthAttribute->Decimal();
                    }
                    auto resolutionAttribute = item->GetAttributeByName("RESOLUTION");
                    if (resolutionAttribute) {
                        stream->width_ = resolutionAttribute->GetResolution().first;
                        stream->height_ = resolutionAttribute->GetResolution().second;
                    }
                    variants_.emplace_back(stream);
                    if (stream->bandWidth_ <= BAND_WIDTH_LIMIT) {
                        defaultVariant_ = stream; // play last stream
                    }
                }
                break;
            }
            default:
                break;
        }
    });
    if (defaultVariant_ == nullptr) {
        defaultVariant_ = variants_.front();
    }
    tags.clear();
}
}
}
}
}
