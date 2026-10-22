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

#ifndef TRACK_SELECTION_FILTER_UTIL_H
#define TRACK_SELECTION_FILTER_UTIL_H

#include <algorithm>
#include <functional>
#include <map>
#include <string>
#include <vector>
#include "plugin/source_plugin.h"
#include "media_source.h"

namespace OHOS {
namespace Media {
namespace Plugins {

using StreamFilterPredicate = std::function<bool(StreamInfo)>;

[[maybe_unused]] static bool MatchPreferredMimeTypes(const std::vector<std::string> &preferredMimeTypes,
    const StreamInfo &streamInfo)
{
    if (!streamInfo.mimeType.empty() &&
        std::find(preferredMimeTypes.begin(), preferredMimeTypes.end(), streamInfo.mimeType) !=
        preferredMimeTypes.end()) {
        return true;
    }
    if (streamInfo.originCodecs.empty()) {
        return false;
    }

    const auto &codecs = streamInfo.originCodecs;
    for (const auto &mimeType : preferredMimeTypes) {
        if (mimeType.empty() || codecs.size() < mimeType.size()) {
            continue;
        }

        const size_t mimeTypeDotPos = mimeType.find('.');
        if (mimeTypeDotPos == std::string::npos) {
            if (codecs.compare(0, mimeType.size(), mimeType, 0, mimeType.size()) == 0 &&
                (codecs.size() == mimeType.size() || codecs[mimeType.size()] == '.')) {
                return true;
            }
            continue;
        }

        size_t codecsDotPos = codecs.find('.');
        if (codecsDotPos == std::string::npos) {
            codecsDotPos = codecs.size();
        }
        if (codecsDotPos != mimeTypeDotPos) {
            continue;
        }

        if (codecs.compare(0, mimeType.size(), mimeType, 0, mimeType.size()) == 0) {
            return true;
        }
    }
    return false;
}

[[maybe_unused]] static void GetVideoFilterPredicatesExt(const TrackSelectionFilter &filter,
    std::vector<StreamFilterPredicate> &predicates)
{
    if (filter.minVideoFrameRate > 0) {
        predicates.emplace_back(
            [minFrameRate = filter.minVideoFrameRate](StreamInfo streamInfo) {
                return streamInfo.frameRate != 0 && streamInfo.frameRate >= static_cast<uint32_t>(minFrameRate);
            }
        );
    }
    if (filter.maxVideoResolution.first > 0 && filter.maxVideoResolution.second > 0) {
        predicates.emplace_back(
            [filter = filter](StreamInfo streamInfo) {
                return streamInfo.GetResolution() != 0 && filter.GetMaxVideoResolution() >= streamInfo.GetResolution();
            }
        );
    }
    if (filter.minVideoResolution.first > 0 && filter.minVideoResolution.second > 0) {
        predicates.emplace_back(
            [filter = filter](StreamInfo streamInfo) {
                return streamInfo.GetResolution() != 0 && filter.GetMinVideoResolution() <= streamInfo.GetResolution();
            }
        );
    }
    if (!filter.preferredVideoMimeTypes.empty()) {
        predicates.emplace_back(
            [preferredMimeTypes = filter.preferredVideoMimeTypes](StreamInfo streamInfo) {
                return MatchPreferredMimeTypes(preferredMimeTypes, streamInfo);
            }
        );
    }
}

[[maybe_unused]] static std::vector<StreamFilterPredicate> GetVideoFilterPredicates(
    const TrackSelectionFilter &filter)
{
    std::vector<StreamFilterPredicate> predicates;
    if (filter.maxVideoBitrate > 0) {
        predicates.emplace_back(
            [maxBitrate = filter.maxVideoBitrate](const StreamInfo &streamInfo) {
                return streamInfo.bitRate != 0 && streamInfo.bitRate <= static_cast<uint32_t>(maxBitrate);
            }
        );
    }
    if (filter.minVideoBitrate > 0) {
        predicates.emplace_back(
            [minBitrate = filter.minVideoBitrate](const StreamInfo &streamInfo) {
                return streamInfo.bitRate != 0 && streamInfo.bitRate >= static_cast<uint32_t>(minBitrate);
            }
        );
    }
    if (filter.maxVideoFrameRate > 0) {
        predicates.emplace_back(
            [maxFrameRate = filter.maxVideoFrameRate](const StreamInfo &streamInfo) {
                return streamInfo.frameRate != 0 && streamInfo.frameRate <= static_cast<uint32_t>(maxFrameRate);
            }
        );
    }
    GetVideoFilterPredicatesExt(filter, predicates);
    return predicates;
}

[[maybe_unused]] static void GetAudioFilterPredicatesExt(const TrackSelectionFilter &filter,
    std::vector<StreamFilterPredicate> &predicates)
{
    if (!filter.preferredAudioMimeTypes.empty()) {
        predicates.emplace_back(
            [preferredMimeTypes = filter.preferredAudioMimeTypes](const StreamInfo &streamInfo) {
                return MatchPreferredMimeTypes(preferredMimeTypes, streamInfo);
            }
        );
    }
    if (!filter.preferredAudioLanguages.empty()) {
        predicates.emplace_back(
            [preferredLanguages = filter.preferredAudioLanguages](const StreamInfo &streamInfo) {
                return !streamInfo.lang.empty() && std::find(preferredLanguages.begin(), preferredLanguages.end(),
                    streamInfo.lang) != preferredLanguages.end();
            }
        );
    }
}

[[maybe_unused]] static std::vector<StreamFilterPredicate> GetAudioFilterPredicates(
    const TrackSelectionFilter &filter)
{
    std::vector<StreamFilterPredicate> predicates;
    if (filter.maxAudioBitrate > 0) {
        predicates.emplace_back(
            [maxBitrate = filter.maxAudioBitrate](const StreamInfo &streamInfo) {
                return streamInfo.bitRate != 0 && streamInfo.bitRate <= static_cast<uint32_t>(maxBitrate);
            }
        );
    }
    if (filter.minAudioBitrate > 0) {
        predicates.emplace_back(
            [minBitrate = filter.minAudioBitrate](const StreamInfo &streamInfo) {
                return streamInfo.bitRate != 0 && streamInfo.bitRate >= static_cast<uint32_t>(minBitrate);
            }
        );
    }
    if (filter.maxAudioChannels > 0) {
        predicates.emplace_back(
            [maxChannels = filter.maxAudioChannels](const StreamInfo &streamInfo) {
                return streamInfo.channels != 0 && streamInfo.channels <= static_cast<uint32_t>(maxChannels);
            }
        );
    }
    GetAudioFilterPredicatesExt(filter, predicates);
    return predicates;
}

[[maybe_unused]] static std::vector<StreamFilterPredicate> GetSubtitleFilterPredicates(
    const TrackSelectionFilter &filter)
{
    std::vector<StreamFilterPredicate> predicates;
    if (!filter.preferredSubtitleLanguages.empty()) {
        predicates.emplace_back(
            [preferredLanguages = filter.preferredSubtitleLanguages](const StreamInfo &streamInfo) {
                return !streamInfo.lang.empty() && std::find(preferredLanguages.begin(), preferredLanguages.end(),
                    streamInfo.lang) != preferredLanguages.end();
            }
        );
    }
    return predicates;
}

[[maybe_unused]] static bool IsStreamMatched(const StreamInfo &stream,
    const std::map<StreamType, std::vector<StreamFilterPredicate>> &filteredStreamPredicates)
{
    auto type = stream.type;
    if (type == MIXED) {
        type = VIDEO;
    }
    auto it = filteredStreamPredicates.find(type);
    if (it == filteredStreamPredicates.end() || it->second.empty()) {
        return false;
    }
    for (const auto &predicate : it->second) {
        if (!predicate(stream)) {
            return false;
        }
    }
    return true;
}

} // namespace Plugins
} // namespace Media
} // namespace OHOS

#endif // TRACK_SELECTION_FILTER_UTIL_H
