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

#include "i_dash_mpd_node.h"
#include "dash_generic_node.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {

static const std::vector<std::string> COMMON_ATTRS = {
    "profiles", "width", "height", "sar", "frameRate", "audioSamplingRate",
    "mimeType", "segmentProfiles", "codecs", "maximumSAPPeriod",
    "startWithSAP", "maxPlayoutRate", "codingDependency", "scanType"
};

struct NodeConfig {
    std::vector<std::string> attrs;
    std::vector<std::string> commonAttrs;
    std::vector<std::pair<std::string, std::string>> childNodes;
};

const std::unordered_map<std::string, NodeConfig>& GetNodeConfigMap()
{
    static const std::unordered_map<std::string, NodeConfig> configs = {
        {"MPD", {{"id", "profiles", "type", "availabilityStartTime", "availabilityEndTime",
                  "mediaPresentationDuration", "minimumUpdatePeriod", "minBufferTime",
                  "timeShiftBufferDepth", "suggestedPresentationDelay", "maxSegmentDuration",
                  "maxSubsegmentDuration", "mediaType", "hwTotalViewNumber", "hwDefaultViewIndex"}, {}, {}}},
        {"Period", {{"id", "start", "duration", "bitstreamSwitching"}, {}, {}}},
        {"AdaptationSet", {{"id", "group", "lang", "contentType", "par", "minBandwidth", "maxBandwidth",
                            "minWidth", "maxWidth", "minHeight", "maxHeight", "minFrameRate", "maxFrameRate",
                            "segmentAlignment", "bitstreamSwitching", "subsegmentAlignment",
                            "subsegmentStartsWithSAP", "cameraIndex", "videoType"},
                           COMMON_ATTRS, {}}},
        {"ContentComponent", {{"id", "lang", "contentType", "par"}, {}, {}}},
        {"Representation", {{"id", "bandwidth", "qualityRanking", "dependencyId", "mediaStreamStructureId",
                             "volumeAdjust_", "cuvvVersion", "channels"},
                            COMMON_ATTRS, {}}},
        {"SegmentBase", {{"timescale", "presentationTimeOffset", "indexRange", "indexRangeExact"}, {}, {}}},
        {"MultipleSegmentBase", {{"duration", "startNumber"}, {},
                                 {{"inherit", "SegmentBase"}}}},
        {"SegmentList", {{"media", "mediaRange", "index", "indexRange"}, {},
                         {{"inherit", "MultipleSegmentBase"}}}},
        {"SegmentTemplate", {{"media", "index", "initialization", "bitstreamSwitching"}, {},
                             {{"inherit", "MultipleSegmentBase"}}}},
        {"SegmentTimeline", {{"t", "d", "r"}, {}, {}}},
        {"SegmentURL", {{"media", "mediaRange", "index", "indexRange"}, {}, {}}},
        {"EventStream", {{"timescale", "value", "schemeIdUri", "presentationTimeOffset"}, {}, {}}},
        {"Event", {{"id", "duration", "presentationTime", "value"}, {}, {}}},
        {"Initialization", {{"sourceURL", "range"}, {}, {}}},
        {"RepresentationIndex", {{"sourceURL", "range"}, {}, {}}},
        {"BitstreamSwitching", {{"sourceURL", "range"}, {}, {}}},
        {"ContentProtection", {{"schemeIdUri", "value", "default_KID"}, {}, {}}},
        {"Role", {{"schemeIdUri", "value"}, {}, {}}},
        {"EssentialProperty", {{"schemeIdUri", "value"}, {}, {}}},
        {"AudioChannelConfiguration", {{"schemeIdUri", "value"}, {}, {}}},
        {"ad", {{"messageData"}, {}, {}}},
    };
    return configs;
}

IDashMpdNode *IDashMpdNode::CreateNode(const std::string &nodeName)
{
    const auto& configMap = GetNodeConfigMap();
    auto configIt = configMap.find(nodeName);
    if (configIt != configMap.end()) {
        if (!configIt->second.childNodes.empty()) {
            return new DashGenericNode(configIt->second.attrs, configIt->second.commonAttrs,
                                       configIt->second.childNodes);
        }
        return new DashGenericNode(configIt->second.attrs, configIt->second.commonAttrs);
    }
    return nullptr;
}

void IDashMpdNode::DestroyNode(IDashMpdNode *node)
{
    if (node != nullptr) {
        delete node;
    }
}

} // namespace HttpPlugin
} // namespace Plugin
} // namespace Media
} // namespace OHOS
