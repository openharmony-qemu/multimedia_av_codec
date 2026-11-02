/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "http_source_plugin_stream_resource_parser.h"
#include "hls/m3u8.h"
#include "dash/dash_mpd_downloader.h"
#include "common/log.h"
#include "track_selection_filter_util.h"
#include <set>
#include <cstdlib>
#include <cerrno>

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {

namespace {
struct HlsFilteredStreams {
    std::vector<StreamResourceInfo> video;
    std::vector<StreamResourceInfo> audio;
    std::vector<StreamResourceInfo> subtitles;
};

void DeduplicateResources(std::vector<StreamResourceInfo>& resources)
{
    std::set<std::string> seen;
    resources.erase(
        std::remove_if(resources.begin(), resources.end(),
            [&seen](const StreamResourceInfo& res) {
                return !seen.insert(res.url).second;
            }),
        resources.end());
}

const char* DashTypeLabel(Plugins::StreamType type)
{
    if (type == Plugins::VIDEO) {
        return "DASH-video";
    }
    if (type == Plugins::AUDIO) {
        return "DASH-audio";
    }
    return "DASH-subtitle";
}

void LogMatchedStream(const char* source, const std::string& url,
    const Plugins::StreamInfo& si, std::set<std::string>& loggedUrls)
{
    if (!loggedUrls.insert(url).second) {
        return;
    }
    MEDIA_LOG_I("FilterMatched [%{public}s]: url=%{private}s, type=%{public}d, "
        "bitrate=%{public}u, res=%{public}dx%{public}d, fps=%{public}u, "
        "channels=%{public}u, lang=%{public}s, codecs=%{public}s, "
        "mimeType=%{public}s",
        source, url.c_str(), static_cast<int>(si.type), si.bitRate,
        si.videoWidth, si.videoHeight, si.frameRate, si.channels,
        si.lang.c_str(), si.codecs.c_str(), si.mimeType.c_str());
}

void LogFallbackStream(const char* source, const std::string& url,
    std::set<std::string>& loggedUrls)
{
    if (!loggedUrls.insert(url).second) {
        return;
    }
    MEDIA_LOG_I("Fallback [%{public}s]: url=%{private}s", source, url.c_str());
}

void CollectHlsResources(std::vector<StreamResourceInfo>& resources,
    const std::vector<StreamResourceInfo>& filtered,
    const std::vector<StreamResourceInfo>& all, const char* typeLabel,
    std::set<std::string>& loggedUrls)
{
    if (filtered.empty()) {
        MEDIA_LOG_I("ParseHls: %{public}s fallback to all", typeLabel);
        for (const auto& res : all) {
            LogFallbackStream(typeLabel, res.url, loggedUrls);
            resources.push_back(res);
        }
    } else {
        for (const auto& res : filtered) {
            resources.push_back(res);
        }
    }
}

void CollectFilteredHlsResources(std::vector<StreamResourceInfo>& resources,
    const M3U8MasterPlaylist& masterPlaylist, const HlsFilteredStreams& filtered,
    std::set<std::string>& loggedUrls)
{
    std::vector<StreamResourceInfo> allVideo;
    std::vector<StreamResourceInfo> allAudio;
    std::vector<StreamResourceInfo> allSubtitles;
    for (const auto& variant : masterPlaylist.variants_) {
        if (!variant->uri_.empty()) {
            allVideo.push_back({variant->uri_, true, StreamProtocolType::HLS});
        }
    }
    for (const auto& audioMedia : masterPlaylist.audioList_) {
        if (!audioMedia->uri_.empty()) {
            allAudio.push_back({audioMedia->uri_, true, StreamProtocolType::HLS});
        }
    }
    for (const auto& subtitleMedia : masterPlaylist.subtitlesList_) {
        if (!subtitleMedia->uri_.empty()) {
            allSubtitles.push_back({subtitleMedia->uri_, true, StreamProtocolType::HLS});
        }
    }
    CollectHlsResources(resources, filtered.video, allVideo, "HLS-video", loggedUrls);
    CollectHlsResources(resources, filtered.audio, allAudio, "HLS-audio", loggedUrls);
    CollectHlsResources(resources, filtered.subtitles, allSubtitles, "HLS-subtitle", loggedUrls);
}

void CollectSimplePlaylistResources(std::vector<StreamResourceInfo>& resources,
    const M3U8MasterPlaylist& masterPlaylist)
{
    auto variant = masterPlaylist.variants_.front();
    if (!variant->m3u8_->fmp4HeaderUri_.empty()) {
        resources.push_back({variant->m3u8_->fmp4HeaderUri_, false, StreamProtocolType::HLS});
    }
    for (const auto& file : variant->m3u8_->files_) {
        if (!file.uri_.empty()) {
            resources.push_back({file.uri_, false, StreamProtocolType::HLS});
        }
    }
}

void FilterVariantAudio(const std::shared_ptr<M3U8VariantStream>& variant,
    const std::map<Plugins::StreamType, std::vector<Plugins::StreamFilterPredicate>>& predicateMap,
    std::vector<StreamResourceInfo>& filteredAudio,
    std::set<std::string>& loggedUrls)
{
    for (const auto& audioMedia : variant->audioMedia_) {
        if (audioMedia->uri_.empty()) {
            continue;
        }
        Plugins::StreamInfo ai;
        ai.type = Plugins::AUDIO;
        ai.lang = audioMedia->lang_;
        ai.originCodecs = audioMedia->originCodec_;
        if (!audioMedia->channels_.empty()) {
            char* endptr = nullptr;
            errno = 0;
            unsigned long val = std::strtoul(audioMedia->channels_.c_str(), &endptr, 10);
            if (errno == 0 && *endptr == '\0') {
                ai.channels = static_cast<uint32_t>(val);
            }
        }
        bool audioMatched = Plugins::IsStreamMatched(ai, predicateMap);
        MEDIA_LOG_D("ParseHls audio(variant): uri=%{private}s, lang=%{public}s, "
            "matched=%{public}d",
            audioMedia->uri_.c_str(), ai.lang.c_str(),
            static_cast<int>(audioMatched));
        if (audioMatched) {
            filteredAudio.push_back({audioMedia->uri_, true, StreamProtocolType::HLS});
            LogMatchedStream("HLS-audio", audioMedia->uri_, ai, loggedUrls);
        }
    }
}

void FilterVariantSubtitles(const std::shared_ptr<M3U8VariantStream>& variant,
    const std::map<Plugins::StreamType, std::vector<Plugins::StreamFilterPredicate>>& predicateMap,
    std::vector<StreamResourceInfo>& filteredSubtitles,
    std::set<std::string>& loggedUrls)
{
    for (const auto& subtitleMedia : variant->subtitlesMedia_) {
        if (subtitleMedia->uri_.empty()) {
            continue;
        }
        Plugins::StreamInfo subSi;
        subSi.type = Plugins::SUBTITLE;
        subSi.lang = subtitleMedia->lang_;
        subSi.originCodecs = subtitleMedia->originCodec_;
        bool subMatched = Plugins::IsStreamMatched(subSi, predicateMap);
        MEDIA_LOG_D("ParseHls subtitle(variant): uri=%{private}s, lang=%{public}s, "
            "matched=%{public}d",
            subtitleMedia->uri_.c_str(), subSi.lang.c_str(),
            static_cast<int>(subMatched));
        if (subMatched) {
            filteredSubtitles.push_back(
                {subtitleMedia->uri_, true, StreamProtocolType::HLS});
            LogMatchedStream("HLS-subtitle", subtitleMedia->uri_, subSi, loggedUrls);
        }
    }
}

void FilterHlsVariants(const M3U8MasterPlaylist& masterPlaylist,
    const std::map<Plugins::StreamType, std::vector<Plugins::StreamFilterPredicate>>& predicateMap,
    HlsFilteredStreams& filtered, std::set<std::string>& loggedUrls)
{
    for (const auto& variant : masterPlaylist.variants_) {
        if (variant->uri_.empty()) {
            continue;
        }
        Plugins::StreamInfo si;
        si.type = variant->isVideo_ ? Plugins::VIDEO : Plugins::MIXED;
        si.bitRate = static_cast<uint32_t>(variant->bandWidth_);
        si.frameRate = static_cast<uint32_t>(variant->frameRate_);
        si.videoWidth = variant->width_;
        si.videoHeight = variant->height_;
        si.codecs = variant->codecs_;
        si.originCodecs = variant->codecs_;
        bool matched = Plugins::IsStreamMatched(si, predicateMap);
        MEDIA_LOG_D("ParseHls variant: uri=%{private}s, bw=%{public}u, "
            "res=%{public}dx%{public}d, matched=%{public}d",
            variant->uri_.c_str(), si.bitRate, si.videoWidth, si.videoHeight,
            static_cast<int>(matched));
        if (matched) {
            filtered.video.push_back({variant->uri_, true, StreamProtocolType::HLS});
            LogMatchedStream("HLS-video", variant->uri_, si, loggedUrls);
        }
        FilterVariantAudio(variant, predicateMap, filtered.audio, loggedUrls);
        FilterVariantSubtitles(variant, predicateMap, filtered.subtitles, loggedUrls);
    }
}

void FilterHlsGlobalMedia(const M3U8MasterPlaylist& masterPlaylist,
    const std::map<Plugins::StreamType, std::vector<Plugins::StreamFilterPredicate>>& predicateMap,
    HlsFilteredStreams& filtered, std::set<std::string>& loggedUrls)
{
    for (const auto& audioMedia : masterPlaylist.audioList_) {
        if (audioMedia->uri_.empty()) {
            continue;
        }
        Plugins::StreamInfo ai;
        ai.type = Plugins::AUDIO;
        ai.lang = audioMedia->lang_;
        ai.originCodecs = audioMedia->originCodec_;
        if (!audioMedia->channels_.empty()) {
            char* endptr = nullptr;
            errno = 0;
            unsigned long val = std::strtoul(audioMedia->channels_.c_str(), &endptr, 10);
            if (errno == 0 && *endptr == '\0') {
                ai.channels = static_cast<uint32_t>(val);
            }
        }
        bool audioMatched = Plugins::IsStreamMatched(ai, predicateMap);
        MEDIA_LOG_D("ParseHls audio(global): uri=%{private}s, lang=%{public}s, "
            "matched=%{public}d",
            audioMedia->uri_.c_str(), ai.lang.c_str(),
            static_cast<int>(audioMatched));
        if (audioMatched) {
            filtered.audio.push_back({audioMedia->uri_, true, StreamProtocolType::HLS});
            LogMatchedStream("HLS-audio", audioMedia->uri_, ai, loggedUrls);
        }
    }
    for (const auto& subtitleMedia : masterPlaylist.subtitlesList_) {
        if (subtitleMedia->uri_.empty()) {
            continue;
        }
        Plugins::StreamInfo subSi;
        subSi.type = Plugins::SUBTITLE;
        subSi.lang = subtitleMedia->lang_;
        subSi.originCodecs = subtitleMedia->originCodec_;
        bool subMatched = Plugins::IsStreamMatched(subSi, predicateMap);
        MEDIA_LOG_D("ParseHls subtitle(global): uri=%{private}s, lang=%{public}s, "
            "matched=%{public}d",
            subtitleMedia->uri_.c_str(), subSi.lang.c_str(),
            static_cast<int>(subMatched));
        if (subMatched) {
            filtered.subtitles.push_back(
                {subtitleMedia->uri_, true, StreamProtocolType::HLS});
            LogMatchedStream("HLS-subtitle", subtitleMedia->uri_, subSi, loggedUrls);
        }
    }
}

struct DashScanResult {
    std::map<Plugins::StreamType, size_t> matchedCount;
    std::map<Plugins::StreamType, size_t> allStreamCount;
    std::map<Plugins::StreamType, std::vector<std::string>> filteredByType;
    std::map<Plugins::StreamType, std::vector<std::string>> allByType;
};

void ScanDashStreams(const std::vector<MpdStreamResource>& streamResources,
    const std::map<Plugins::StreamType, std::vector<Plugins::StreamFilterPredicate>>& predicateMap,
    DashScanResult& result, std::set<std::string>& loggedUrls)
{
    for (const auto& res : streamResources) {
        auto type = res.info.type;
        if (type == Plugins::MIXED) {
            type = Plugins::VIDEO;
        }
        result.allStreamCount[type]++;
        for (const auto& u : res.urls) {
            result.allByType[type].push_back(u);
        }
        bool matched = Plugins::IsStreamMatched(res.info, predicateMap);
        MEDIA_LOG_D("ParseDash stream: type=%{public}d, bw=%{public}u, "
            "res=%{public}dx%{public}d, lang=%{public}s, urls=%{public}zu, matched=%{public}d",
            static_cast<int>(type), res.info.bitRate, res.info.videoWidth, res.info.videoHeight,
            res.info.lang.c_str(), res.urls.size(), static_cast<int>(matched));
        if (matched) {
            result.matchedCount[type]++;
            std::string firstUrl = res.urls.empty() ? "" : res.urls.front();
            LogMatchedStream(DashTypeLabel(type), firstUrl, res.info, loggedUrls);
            for (const auto& u : res.urls) {
                result.filteredByType[type].push_back(u);
            }
        }
    }
}

void LogDashFallbackStreams(const std::vector<MpdStreamResource>& streamResources,
    const std::map<Plugins::StreamType, size_t>& matchedCount,
    std::set<std::string>& loggedUrls)
{
    for (const auto& res : streamResources) {
        auto type = res.info.type;
        if (type == Plugins::MIXED) {
            type = Plugins::VIDEO;
        }
        if (matchedCount.count(type) == 0 || matchedCount.at(type) == 0) {
            std::string firstUrl = res.urls.empty() ? "" : res.urls.front();
            LogFallbackStream(DashTypeLabel(type), firstUrl, loggedUrls);
        }
    }
}

void FilterDashStreams(const std::vector<MpdStreamResource>& streamResources,
    const std::map<Plugins::StreamType, std::vector<Plugins::StreamFilterPredicate>>& predicateMap,
    std::vector<StreamResourceInfo>& resources,
    std::set<std::string>& loggedUrls)
{
    DashScanResult result;
    ScanDashStreams(streamResources, predicateMap, result, loggedUrls);

    MEDIA_LOG_I("ParseDash result: videoMatched=%{public}zu, audioMatched=%{public}zu, "
        "subtitleMatched=%{public}zu, videoAll=%{public}zu, audioAll=%{public}zu, subtitleAll=%{public}zu",
        result.matchedCount[Plugins::VIDEO], result.matchedCount[Plugins::AUDIO],
        result.matchedCount[Plugins::SUBTITLE],
        result.allStreamCount[Plugins::VIDEO], result.allStreamCount[Plugins::AUDIO],
        result.allStreamCount[Plugins::SUBTITLE]);

    for (auto& [type, urls] : result.allByType) {
        if (result.matchedCount[type] == 0) {
            MEDIA_LOG_I("ParseDash: type=%{public}d fallback to all (%{public}zu streams)",
                static_cast<int>(type), result.allStreamCount[type]);
            for (const auto& u : urls) {
                resources.push_back({u, false, StreamProtocolType::DASH});
            }
        } else {
            MEDIA_LOG_I("ParseDash: type=%{public}d filtered (%{public}zu streams)",
                static_cast<int>(type), result.matchedCount[type]);
            for (const auto& u : result.filteredByType[type]) {
                resources.push_back({u, false, StreamProtocolType::DASH});
            }
        }
    }
    LogDashFallbackStreams(streamResources, result.matchedCount, loggedUrls);
}
} // anonymous namespace

StreamResourceParser* StreamResourceParser::Parse(
    const uint8_t* data, size_t size, StreamProtocolType protocol, const std::string& rootUrl,
    const TrackSelectionFilter& filter)
{
    auto parser = new StreamResourceParser();
    parser->protocol_ = protocol;
    parser->rootUrl_ = rootUrl;
    parser->filter_ = filter;
    MEDIA_LOG_D("StreamResourceParser::Parse: protocol=%{public}d, filter set: maxVideoBitrate=%{public}d, "
        "maxRes={%{public}d,%{public}d}, preferredAudioLanguages=%{public}zu",
        static_cast<int32_t>(protocol), filter.maxVideoBitrate,
        filter.maxVideoResolution.first, filter.maxVideoResolution.second,
        filter.preferredAudioLanguages.size());

    if (data == nullptr || size == 0) {
        return parser;
    }

    switch (protocol) {
        case StreamProtocolType::HLS:
            parser->ParseHls(data, size);
            break;
        case StreamProtocolType::DASH:
            parser->ParseDash(data, size);
            break;
        case StreamProtocolType::HTTP:
        default:
            parser->ParseHttp(data, size);
            break;
    }
    return parser;
}

bool StreamResourceParser::HasNext() const
{
    return currentIndex_ < resources_.size();
}

StreamResourceInfo StreamResourceParser::Next()
{
    if (currentIndex_ < resources_.size()) {
        return resources_[currentIndex_++];
    }
    return {"", false, StreamProtocolType::HTTP};
}

std::vector<StreamResourceInfo> StreamResourceParser::NextBatch(size_t batchSize)
{
    std::vector<StreamResourceInfo> batch;
    for (size_t i = 0; i < batchSize && HasNext(); ++i) {
        batch.push_back(Next());
    }
    return batch;
}

std::vector<StreamResourceInfo> StreamResourceParser::GetAll()
{
    std::vector<StreamResourceInfo> remaining(resources_.begin() + currentIndex_, resources_.end());
    currentIndex_ = resources_.size();
    return remaining;
}

size_t StreamResourceParser::GetTotalCount() const
{
    return resources_.size();
}

size_t StreamResourceParser::GetProcessedCount() const
{
    return currentIndex_;
}

void StreamResourceParser::ParseHls(const uint8_t* data, size_t size)
{
    std::string content(reinterpret_cast<const char*>(data), size);
    M3U8MasterPlaylist masterPlaylist(content, rootUrl_);
    masterPlaylist.StartParsing();

    auto videoPredicates = Plugins::GetVideoFilterPredicates(filter_);
    auto audioPredicates = Plugins::GetAudioFilterPredicates(filter_);
    auto subtitlePredicates = Plugins::GetSubtitleFilterPredicates(filter_);
    std::map<Plugins::StreamType, std::vector<Plugins::StreamFilterPredicate>> predicateMap;
    predicateMap[Plugins::VIDEO] = videoPredicates;
    predicateMap[Plugins::AUDIO] = audioPredicates;
    predicateMap[Plugins::SUBTITLE] = subtitlePredicates;

    MEDIA_LOG_I("ParseHls: isSimple=%{public}d, variants=%{public}zu, videoPredicates=%{public}zu, "
        "audioPredicates=%{public}zu, subtitlePredicates=%{public}zu",
        static_cast<int>(masterPlaylist.isSimple_.load()), masterPlaylist.variants_.size(),
        videoPredicates.size(), audioPredicates.size(), subtitlePredicates.size());

    std::set<std::string> loggedUrls;
    if (!masterPlaylist.isSimple_) {
        HlsFilteredStreams filtered;
        FilterHlsVariants(masterPlaylist, predicateMap, filtered, loggedUrls);
        FilterHlsGlobalMedia(masterPlaylist, predicateMap, filtered, loggedUrls);
        MEDIA_LOG_I("ParseHls result: filteredVideo=%{public}zu, filteredAudio=%{public}zu, "
            "filteredSubtitles=%{public}zu", filtered.video.size(), filtered.audio.size(),
            filtered.subtitles.size());
        CollectFilteredHlsResources(resources_, masterPlaylist, filtered, loggedUrls);
    } else {
        MEDIA_LOG_D("ParseHls: simple playlist (media playlist), no variant filtering");
        CollectSimplePlaylistResources(resources_, masterPlaylist);
    }
    DeduplicateResources(resources_);
    MEDIA_LOG_I("ParseHls done: total %{public}zu unique resources", resources_.size());
}

void StreamResourceParser::ParseDash(const uint8_t* data, size_t size)
{
    auto streamResources = DashMpdDownloader::ParseMpdResources(data, size, rootUrl_);

    auto videoPredicates = Plugins::GetVideoFilterPredicates(filter_);
    auto audioPredicates = Plugins::GetAudioFilterPredicates(filter_);
    auto subtitlePredicates = Plugins::GetSubtitleFilterPredicates(filter_);
    std::map<Plugins::StreamType, std::vector<Plugins::StreamFilterPredicate>> predicateMap;
    predicateMap[Plugins::VIDEO] = videoPredicates;
    predicateMap[Plugins::AUDIO] = audioPredicates;
    predicateMap[Plugins::SUBTITLE] = subtitlePredicates;

    MEDIA_LOG_I("ParseDash: streamResources=%{public}zu, videoPredicates=%{public}zu, "
        "audioPredicates=%{public}zu, subtitlePredicates=%{public}zu",
        streamResources.size(), videoPredicates.size(), audioPredicates.size(),
        subtitlePredicates.size());

    std::set<std::string> loggedUrls;
    FilterDashStreams(streamResources, predicateMap, resources_, loggedUrls);
    DeduplicateResources(resources_);
    MEDIA_LOG_I("ParseDash done: total %{public}zu unique resources", resources_.size());
}

void StreamResourceParser::ParseHttp(const uint8_t* data, size_t size)
{
    (void)data;
    (void)size;
    resources_.push_back({"", false, StreamProtocolType::HTTP});
}

} // namespace HttpPlugin
} // namespace Plugins
} // namespace Media
} // namespace OHOS

extern "C" {
OHOS::Media::Plugins::HttpPlugin::StreamResourceParser* GetStreamResourceParser(
    const uint8_t* data, size_t size, OHOS::Media::Plugins::HttpPlugin::StreamProtocolType protocol,
    const std::string& rootUrl, const OHOS::Media::Plugins::TrackSelectionFilter& filter)
{
    return OHOS::Media::Plugins::HttpPlugin::StreamResourceParser::Parse(data, size, protocol, rootUrl, filter);
}
}
