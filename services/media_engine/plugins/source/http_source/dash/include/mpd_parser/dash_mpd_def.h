/*
 * Copyright (c) 2024-2024 Huawei Device Co., Ltd.
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

#ifndef DASH_MPD_DEF_H
#define DASH_MPD_DEF_H

#include <map>
#include "mpd_parser_def.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
static const char *const VIDEO_MIME_TYPE = "video";
static const char *const AUDIO_MIME_TYPE = "audio";
static const char *const TEXT_MIME_TYPE = "text";
static const char *const APPLICATION_MIME_TYPE = "application";

enum class VideoScanType { VIDEO_SCAN_PROGRESSIVE, VIDEO_SCAN_INTERLACED, VIDEO_SCAN_UNKNOW };

enum class DashType { DASH_TYPE_STATIC, DASH_TYPE_DYNAMIC };

struct DashSegUrl {
    std::string media_;
    std::string mediaRange_;
    std::string index_;
    std::string indexRange_;
};

struct DashDescriptor {
    std::string schemeIdUrl_;
    std::string value_;
    std::string defaultKid_;
    std::map<std::string, std::string> elementMap_;
};

struct DashSegTimeline {
    uint64_t t_{0};
    uint64_t d_{0};
    int32_t r_{0};
};

struct DashUrlType {
    std::string sourceUrl_;
    std::string range_;
};

struct DashSegBaseInfo {
    uint32_t timeScale_{0};
    uint32_t presentationTimeOffset_{0};
    std::string indexRange_;
    bool indexRangeExact_{false};

    DashUrlType *initialization_{nullptr};
    DashUrlType *representationIndex_{nullptr};
};

struct DashMultSegBaseInfo {
    uint32_t duration_{0};
    std::string startNumber_; // if not present, the value set to 1.

    DashUrlType *bitstreamSwitching_{nullptr};
    DashList<DashSegTimeline *> segTimeline_;
    DashSegBaseInfo segBaseInfo_;
};

// SegmentTemplate Element Infomation
struct DashSegTmpltInfo {
    std::string segTmpltMedia_;
    std::string segTmpltIndex_;
    std::string segTmpltInitialization_;
    std::string segTmpltBitstreamSwitching_;
    DashMultSegBaseInfo multSegBaseInfo_;
};

// SegmentList Element Infomation. <href> <actuate> are not support
struct DashSegListInfo {
    DashList<DashSegUrl *> segmentUrl_;
    DashMultSegBaseInfo multSegBaseInfo_;
};

// <segmentProfiles> <codecs> <maximumSAPPeriod> <startWithSAP> <maxPlayoutRate> <codingDependency> <FramePacking>
// <AudioChannelConfiguration> are not support
struct DashCommonAttrsAndElements {
    bool codingDependency_{false};
    uint32_t width_{0};
    uint32_t height_{0};
    uint32_t startWithSAP_{0};
    double maxPlayoutRate_{1.0};
    std::string profiles_;
    std::string sar_;
    std::string frameRate_;
    std::string audioSamplingRate_;
    DashList<DashDescriptor *> audioChannelConfigurationList_;
    std::string mimeType_;
    std::string codecs_;
    std::string cuvvVersion_; // private element
    VideoScanType scanType_{VideoScanType::VIDEO_SCAN_PROGRESSIVE};
    DashList<DashDescriptor *> contentProtectionList_;
    DashList<DashDescriptor *> essentialPropertyList_;
};

// Representation Element Infomation. <dependencyId> <mediaStreamStructureId> <SubRepresentation> are not support
struct DashRepresentationInfo {
    std::string id_;
    uint32_t bandwidth_{0};
    uint32_t qualityRanking_{0};
    std::list<std::string> baseUrl_;
    std::string volumeAdjust_;

    DashSegBaseInfo *representationSegBase_{nullptr};
    DashSegListInfo *representationSegList_{nullptr};
    DashSegTmpltInfo *representationSegTmplt_{nullptr};

    DashCommonAttrsAndElements commonAttrsAndElements_;
};

struct DashContentCompInfo {
    uint32_t id_{0};
    std::string lang_;
    std::string contentType_;
    std::string par_;
    DashList<DashDescriptor *> roleList_;
};

struct DashAdptSetInfo {
    bool segmentAlignment_{false};
    bool subSegmentAlignment_{false};
    bool bitstreamSwitching_{false};
    uint32_t id_{0};
    uint32_t group_{0};
    uint32_t minBandwidth_{0};
    uint32_t maxBandwidth_{0};
    uint32_t minWidth_{0};
    uint32_t maxWidth_{0};
    uint32_t minHeight_{0};
    uint32_t maxHeight_{0};
    uint32_t cameraIndex_{0}; // only for hw multi view
    int32_t subSegmentStartsWithSAP_{0};
    std::string lang_;
    std::string contentType_;
    std::string par_;
    std::string minFrameRate_;
    std::string maxFrameRate_;
    std::string mimeType_;
    std::string videoType_; // only for hw multi view

    DashSegBaseInfo *adptSetSegBase_{nullptr};
    DashSegListInfo *adptSetSegList_{nullptr};
    DashSegTmpltInfo *adptSetSegTmplt_{nullptr};

    std::list<std::string> baseUrl_;
    DashList<DashContentCompInfo *> contentCompList_;
    DashList<DashRepresentationInfo *> representationList_;
    DashList<DashDescriptor *> roleList_;
    DashCommonAttrsAndElements commonAttrsAndElements_;
};

// Period Element Infomation. <href> <actuate> <subset> are not support
struct DashPeriodInfo {
    uint32_t start_{0};
    uint32_t duration_{0};
    bool bitstreamSwitching_{false};
    std::string id_;
    std::list<std::string> baseUrl_;

    DashSegBaseInfo *periodSegBase_{nullptr};
    DashSegListInfo *periodSegList_{nullptr};
    DashSegTmpltInfo *periodSegTmplt_{nullptr};
    DashList<DashAdptSetInfo *> adptSetList_;
};

// MPD Element Infomation. ProgramInfomation Location Metrics are not support
struct DashMpdInfo {
    DashType type_{DashType::DASH_TYPE_STATIC};
    uint32_t mediaPresentationDuration_{0};
    uint32_t minimumUpdatePeriod_{0};
    uint32_t minBufferTime_{0};
    uint32_t timeShiftBufferDepth_{0};
    uint32_t suggestedPresentationDelay_{0};
    uint32_t maxSegmentDuration_{0};
    uint32_t maxSubSegmentDuration_{0};
    uint32_t hwTotalViewNumber_{0};
    uint32_t hwDefaultViewIndex_{0};
    int64_t availabilityStartTime_{0};
    int64_t availabilityEndTime_{0};
    std::string mediaType_;
    std::string profile_;
    std::list<std::string> baseUrl_;
    DashList<DashPeriodInfo *> periodInfoList_;
};
} // namespace HttpPluginLite
} // namespace Plugin
} // namespace Media
} // namespace OHOS
#endif // DASH_MPD_DEF_H
