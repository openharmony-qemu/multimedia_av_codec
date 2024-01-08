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
#include "common/log.h"
#include "m3u8.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
namespace {
constexpr int MAX_LOOP = 16;
bool StrHasPrefix(std::string &str, const std::string &prefix)
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

M3U8Fragment::M3U8Fragment(M3U8Fragment m3u8, uint8_t *key, uint8_t *iv)
    : uri_(std::move(m3u8.uri_)),
      title_(std::move(m3u8.title_)),
      duration_(m3u8.duration_),
      sequence_(m3u8.sequence_),
      discont_(m3u8.discont_)
{
    if (iv == nullptr || key == nullptr) {
        return;
    }

    for (int i = 0; i < MAX_LOOP; i++) {
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
    downloader_->Stop();
}

bool M3U8::Update(std::string& playList)
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
    tagUpdatersMap_[HlsTag::EXTXPLAYLISTTYPE] = [this](std::shared_ptr<Tag> &tag, M3U8Info &info) {
        bLive_ = !info.bVod && (std::static_pointer_cast<SingleValueTag>(tag)->GetValue().QuotedString() != "VOD");
    };

    tagUpdatersMap_[HlsTag::EXTXTARGETDURATION] = [this](std::shared_ptr<Tag> &tag, M3U8Info &info) {
        std::ignore = info;
        targetDuration_ = std::static_pointer_cast<SingleValueTag>(tag)->GetValue().FloatingPoint();
    };

    tagUpdatersMap_[HlsTag::EXTXMEDIASEQUENCE] = [this](std::shared_ptr<Tag> &tag, M3U8Info &info) {
        std::ignore = info;
        sequence_ = std::static_pointer_cast<SingleValueTag>(tag)->GetValue().Decimal();
    };

    tagUpdatersMap_[HlsTag::EXTXDISCONTINUITYSEQUENCE] = [this](std::shared_ptr<Tag> &tag, M3U8Info &info) {
        discontSequence_ = static_cast<int>(std::static_pointer_cast<SingleValueTag>(tag)->GetValue().Decimal());
        info.discontinuity = true;
    };

    tagUpdatersMap_[HlsTag::EXTINF] = [this](std::shared_ptr<Tag> &tag, M3U8Info &info) {
        GetExtInf(tag, info.duration, info.title);
    };

    tagUpdatersMap_[HlsTag::URI] = [this](std::shared_ptr<Tag> &tag, M3U8Info &info) {
        info.uri = UriJoin(uri_, std::static_pointer_cast<SingleValueTag>(tag)->GetValue().QuotedString());
    };

    tagUpdatersMap_[HlsTag::EXTXBYTERANGE] = [](std::shared_ptr<Tag> &tag, M3U8Info &info) {
        std::ignore = tag;
        std::ignore = info;
        MEDIA_LOG_I("need to parse EXTXBYTERANGE");
    };

    tagUpdatersMap_[HlsTag::EXTXDISCONTINUITY] = [this](std::shared_ptr<Tag> &tag, M3U8Info &info) {
        std::ignore = tag;
        discontSequence_++;
        info.discontinuity = true;
    };

    tagUpdatersMap_[HlsTag::EXTXKEY] = [this](std::shared_ptr<Tag> &tag, M3U8Info &info) {
        isDecryptAble_ = true;
        isDecryptKeyReady_ = false;
        ParseKey(std::static_pointer_cast<AttributesTag>(tag));
        DownloadKey();
        // wait for key downloaded
    };

    tagUpdatersMap_[HlsTag::EXTXMAP] = [](std::shared_ptr<Tag> &tag, M3U8Info &info) {
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
        int size = iv_buff.size() > MAX_LOOP ? MAX_LOOP : iv_buff.size();
        for (int i = 0; i < size; i++) {
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
        memcpy_s(key_, MAX_LOOP, data, len);
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

M3U8VariantStream::M3U8VariantStream(std::string name, std::string uri, std::shared_ptr<M3U8> m3u8)
    : name_(std::move(name)), uri_(std::move(uri)), m3u8_(std::move(m3u8))
{
}

M3U8MasterPlaylist::M3U8MasterPlaylist(std::string& playList, const std::string& uri)
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