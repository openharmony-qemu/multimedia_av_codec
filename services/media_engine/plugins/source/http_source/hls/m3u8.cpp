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
#include "common/log.h"
#include "m3u8.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
namespace {
constexpr uint32_t MAX_LOOP = 16;
constexpr uint32_t DRM_UUID_OFFSET = 12;
constexpr uint32_t DRM_INFO_BASE64_DATA_MULTIPLE = 4;
constexpr uint32_t DRM_INFO_BASE64_BASE_UNIT_OF_CONVERSION = 3;
constexpr uint32_t DRM_PSSH_TITLE_LEN = 16;

const char DRM_PSSH_TITLE[] = "data:text/plain;";

/**
 * base64 decoding table
 */
static const uint8_t BASE64_DECODE_TABLE[] = {
    // 16 per row
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 1 - 16
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 17 - 32
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 62, 0, 0, 0, 63,  // 33 - 48
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 0, 0, 0, 0, 0, 0,  // 49 - 64
    0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,  // 65 - 80
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 0, 0, 0, 0, 0,  // 81 - 96
    0, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,  // 97 - 112
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 0, 0, 0, 0, 0  // 113 - 128
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
    } else {
        std::string::size_type pos = baseUrl.rfind('/');
        return baseUrl.substr(0, pos + 1) + uri;
    }
}
}

M3U8Fragment::M3U8Fragment(const M3U8Fragment& m3u8, const uint8_t *key, const uint8_t *iv)
    : uri_(std::move(m3u8.uri_)),
      title_(std::move(m3u8.title_)),
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

M3U8Fragment::M3U8Fragment(std::string uri, std::string title, double duration, int sequence, bool discont)
    : uri_(std::move(uri)), title_(std::move(title)), duration_(duration), sequence_(sequence), discont_(discont)
{
}

M3U8::M3U8(std::string uri, std::string name) : uri_(std::move(uri)), name_(std::move(name))
{
    InitTagUpdatersMap();
}

M3U8::~M3U8()
{
    if (downloader_) {
        downloader_->Stop();
    }
}

bool M3U8::Update(const std::string& playList)
{
    if (playList_ == playList) {
        MEDIA_LOG_I("playlist does not change ");
        return true;
    }
    if (!StrHasPrefix(playList, "#EXTM3U")) {
        MEDIA_LOG_I("playlist doesn't start with #EXTM3U " PUBLIC_LOG_S, playList.c_str());
        return false;
    }
    if (playList.find("\n#EXT-X-STREAM-INF:") != std::string::npos) {
        MEDIA_LOG_I("Not a media playlist, but a master playlist! " PUBLIC_LOG_S, playList.c_str());
        return false;
    }
    files_.clear();
    MEDIA_LOG_I("media playlist " PUBLIC_LOG_S, playList.c_str());
    auto tags = ParseEntries(playList);
    UpdateFromTags(tags);
    tags.clear();
    playList_ = playList;
    return true;
}

void M3U8::InitTagUpdatersMap()
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
        GetExtInf(tag, info.duration, info.title);
    };

    tagUpdatersMap_[HlsTag::URI] = [this](std::shared_ptr<Tag> &tag, M3U8Info &info) {
        info.uri = UriJoin(uri_, std::static_pointer_cast<SingleValueTag>(tag)->GetValue().QuotedString());
    };

    tagUpdatersMap_[HlsTag::EXTXBYTERANGE] = [](const std::shared_ptr<Tag> &tag, const M3U8Info &info) {
        std::ignore = tag;
        std::ignore = info;
        MEDIA_LOG_I("need to parse EXTXBYTERANGE");
    };

    tagUpdatersMap_[HlsTag::EXTXDISCONTINUITY] = [this](const std::shared_ptr<Tag> &tag, M3U8Info &info) {
        std::ignore = tag;
        discontSequence_++;
        info.discontinuity = true;
    };

    tagUpdatersMap_[HlsTag::EXTXKEY] = [this](std::shared_ptr<Tag> &tag, const M3U8Info &info) {
        isDecryptAble_ = true;
        isDecryptKeyReady_ = false;
        ParseKey(std::static_pointer_cast<AttributesTag>(tag));
        if ((keyUri_ != nullptr) && (keyUri_->length() > DRM_PSSH_TITLE_LEN) &&
            (keyUri_->substr(0, DRM_PSSH_TITLE_LEN) == DRM_PSSH_TITLE)) {
            ProcessDrmInfos();
        } else {
            DownloadKey();
        }
        // wait for key downloaded
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
            if (!isFirstFragmentReady_) {
                firstFragment_ = info;
                isFirstFragmentReady_ = true;
            }
            // add key_ and iv_ to M3U8Fragment(file)
            if (isDecryptAble_) {
                auto m3u8 = M3U8Fragment(info.uri, info.title, info.duration, sequence_++, info.discontinuity);
                auto fragment = std::make_shared<M3U8Fragment>(m3u8, key_, iv_);
                files_.emplace_back(fragment);
            } else {
                auto fragment = std::make_shared<M3U8Fragment>(info.uri, info.title, info.duration, sequence_++,
                    info.discontinuity);
                files_.emplace_back(fragment);
            }
            info.uri = "", info.title = "", info.duration = 0, info.discontinuity = false;
        }
    }
}

void M3U8::GetExtInf(const std::shared_ptr<Tag>& tag, double& duration, std::string& title) const
{
    auto item = std::static_pointer_cast<ValuesListTag>(tag);
    duration =  item ->GetAttributeByName("DURATION")->FloatingPoint();
    title = item ->GetAttributeByName("TITLE")->QuotedString();
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
        MEDIA_LOG_E("OnDownloadStatus " PUBLIC_LOG_D32 ", url : " PUBLIC_LOG_S, status, request->GetUrl().c_str());
    }
}

/**
 * @brief Base64Decode base64 decoding
 * @param src data to be decoded
 * @param srcSize The size of the data to be decoded
 * @param dest decoded output data
 * @param destSize The size of the output data after decoding
 * @return bool true: success false: invalid parameter
 * Note: The size of the decoded data must be greater than 4 and be a multiple of 4
 */
bool M3U8::Base64Decode(const uint8_t *src, uint32_t srcSize, uint8_t *dest, uint32_t *destSize)
{
    if ((src == nullptr) || (srcSize == 0) || (dest == nullptr) || (destSize == nullptr) || (srcSize > *destSize)) {
        MEDIA_LOG_E("parameter is err");
        return false;
    }
    if ((srcSize < DRM_INFO_BASE64_DATA_MULTIPLE) || (srcSize % DRM_INFO_BASE64_DATA_MULTIPLE != 0)) {
        MEDIA_LOG_E("srcSize is err");
        return false;
    }

    uint32_t i;
    uint32_t j;
    // Calculate decoded string length
    uint32_t len = srcSize / DRM_INFO_BASE64_DATA_MULTIPLE * DRM_INFO_BASE64_BASE_UNIT_OF_CONVERSION;
    if (src[srcSize - 1] == '=') { // 1:last one
        len--;
    }
    if (src[srcSize - 2] == '=') { // 2:second to last
        len--;
    }

    for (i = 0, j = 0; i < srcSize;
        i += DRM_INFO_BASE64_DATA_MULTIPLE, j += DRM_INFO_BASE64_BASE_UNIT_OF_CONVERSION) {
        dest[j] = (BASE64_DECODE_TABLE[src[i]] << 2) | (BASE64_DECODE_TABLE[src[i + 1]] >> 4); // 2&4bits move
        dest[j + 1] = (BASE64_DECODE_TABLE[src[i + 1]] << 4) | // 4:4bits moved
            (BASE64_DECODE_TABLE[src[i + 2]] >> 2); // 2:index 2:2bits moved
        dest[j + 2] = (BASE64_DECODE_TABLE[src[i + 2]] << 6) | // 2:index 6:6bits moved
            (BASE64_DECODE_TABLE[src[i + 3]]); // 3:index
    }
    *destSize = len;
    return true;
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
    bool ret = Base64Decode(reinterpret_cast<const uint8_t *>(psshString.c_str()),
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
        MEDIA_LOG_I("playlist doesn't start with #EXTM3U " PUBLIC_LOG_S, uri.c_str());
    }
    if (playList_.find("\n#EXTINF:") != std::string::npos) {
        UpdateMediaPlaylist();
    } else {
        UpdateMasterPlaylist();
    }
}

void M3U8MasterPlaylist::UpdateMediaPlaylist()
{
    MEDIA_LOG_I("This is a simple media playlist, not a master playlist " PUBLIC_LOG_S, uri_.c_str());
    auto m3u8 = std::make_shared<M3U8>(uri_, "");
    auto stream = std::make_shared<M3U8VariantStream>(uri_, uri_, m3u8);
    variants_.emplace_back(stream);
    defaultVariant_ = stream;
    m3u8->Update(playList_);
    duration_ = m3u8->GetDuration();
    bLive_ = m3u8->IsLive();
    isSimple_ = true;
    MEDIA_LOG_D("UpdateMediaPlaylist called, duration_ = " PUBLIC_LOG_F, duration_);
}

void M3U8MasterPlaylist::UpdateMasterPlaylist()
{
    MEDIA_LOG_I("master playlist " PUBLIC_LOG_S, playList_.c_str());
    auto tags = ParseEntries(playList_);
    std::for_each(tags.begin(), tags.end(), [this] (std::shared_ptr<Tag>& tag) {
        switch (tag->GetType()) {
            case HlsTag::EXTXSTREAMINF:
            case HlsTag::EXTXIFRAMESTREAMINF: {
                auto item = std::static_pointer_cast<AttributesTag>(tag);
                auto uriAttribute = item->GetAttributeByName("URI");
                if (uriAttribute) {
                    auto name = uriAttribute->QuotedString();
                    auto uri = UriJoin(uri_, name);
                    auto stream = std::make_shared<M3U8VariantStream>(name, uri, std::make_shared<M3U8>(uri, name));
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
                    defaultVariant_ = stream; // play last stream
                }
                break;
            }
            default:
                break;
        }
    });
    tags.clear();
}
}
}
}
}
