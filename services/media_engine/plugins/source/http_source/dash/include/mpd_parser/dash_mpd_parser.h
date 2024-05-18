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

#ifndef DASH_MPD_PARSER_H
#define DASH_MPD_PARSER_H

#include <cstdint>
#include "dash_mpd_def.h"
#include "i_dash_mpd_node.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
struct DashElementList {
    DashElementList()
    {
        segBaseElement_ = nullptr;
        segListElement_ = nullptr;
        segTmplElement_ = nullptr;
    }

    std::shared_ptr<XmlElement> segBaseElement_;
    std::shared_ptr<XmlElement> segListElement_;
    std::shared_ptr<XmlElement> segTmplElement_;
};

class DashMpdParser {
public:
    DashMpdParser() = default;
    ~DashMpdParser();

    void ParseMPD(const char *mpdData, uint32_t length);
    void GetMPD(DashMpdInfo *&mpdInfo);
    void Clear();
    void StopParseMpd();

private:
    void ParsePeriod(std::shared_ptr<XmlParser> xmlParser, std::shared_ptr<XmlElement> rootElement);
    void ParseAdaptationSet(std::shared_ptr<XmlParser> xmlParser, std::shared_ptr<XmlElement> rootElement,
                            DashPeriodInfo *periodInfo);
    void ParseRepresentation(std::shared_ptr<XmlParser> xmlParser, std::shared_ptr<XmlElement> rootElement,
                             const DashPeriodInfo *periodInfo,
                             DashAdptSetInfo *adptSetInfo);
    void ParseSegmentBase(std::shared_ptr<XmlParser> xmlParser, std::shared_ptr<XmlElement> rootElement,
                          DashSegBaseInfo **segBaseInfo);
    void ParseSegmentList(std::shared_ptr<XmlParser> xmlParser, std::shared_ptr<XmlElement> rootElement,
                          DashSegListInfo **segListInfo);
    void ParseSegmentListElement(std::shared_ptr<XmlParser> &xmlParser, DashSegListInfo *segList,
                                 std::shared_ptr<XmlElement> &childElement);
    void ParseSegmentTemplate(std::shared_ptr<XmlParser> xmlParser, std::shared_ptr<XmlElement> rootElement,
                              DashSegTmpltInfo **segTmpltInfo);
    void ParseSegmentTimeline(std::shared_ptr<XmlParser> xmlParser, std::shared_ptr<XmlElement> rootElement,
                              DashList<DashSegTimeline *> &segTmlineList);
    void ParseUrlType(std::shared_ptr<XmlParser> xmlParser, std::shared_ptr<XmlElement> rootElement,
                      const std::string urlTypeName,
                      DashUrlType **urlTypeInfo);
    void ParseContentProtection(std::shared_ptr<XmlParser> xmlParser, std::shared_ptr<XmlElement> rootElement,
                                DashList<DashDescriptor *> &contentProtectionList);
    void ParseEssentialProperty(std::shared_ptr<XmlParser> xmlParser, std::shared_ptr<XmlElement> rootElement,
                                DashList<DashDescriptor *> &essentialPropertyList);
    void ParseAudioChannelConfiguration(std::shared_ptr<XmlParser> xmlParser, std::shared_ptr<XmlElement> rootElement,
                                        DashList<DashDescriptor *> &propertyList);
    void ParseSegmentUrl(std::shared_ptr<XmlParser> xmlParser, std::shared_ptr<XmlElement> rootElement,
                         DashList<DashSegUrl *> &segUrlList);
    void ParseContentComponent(std::shared_ptr<XmlParser> xmlParser, std::shared_ptr<XmlElement> rootElement,
                               DashList<DashContentCompInfo *> &contentCompInfoList);
    void ParseProgramInfo(std::list<std::string> &programStrList) const;
    void ParseBaseUrl(std::list<std::string> &baseUrlStrList) const;
    void ParseLocation(std::list<std::string> &locationStrlist) const;
    void ParseMetrics(std::list<std::string> &metricsStrList) const;

    void FreeSegmentBase(DashSegBaseInfo *segBaseInfo) const;
    void FreeSegmentList(DashSegListInfo *segListInfo);
    void FreeSegmentTemplate(DashSegTmpltInfo *segTmpltInfo);
    void ClearAdaptationSet(DashList<DashAdptSetInfo *> &adptSetInfoList);
    void ClearContentComp(DashList<DashContentCompInfo *> &contentCompList);
    void ClearRepresentation(DashList<DashRepresentationInfo *> &representationList);
    void ClearSegmentBase(DashSegBaseInfo *segBaseInfo) const;
    void ClearMultSegBase(DashMultSegBaseInfo *multSegBaseInfo);
    void ClearCommonAttrsAndElements(DashCommonAttrsAndElements *commonAttrsAndElements);
    void ClearRoleList(DashList<DashDescriptor *> &roleList);

    void GetMpdAttr(IDashMpdNode *mpdNode);
    void GetMpdElement(std::shared_ptr<XmlParser> xmlParser, std::shared_ptr<XmlElement> rootElement);
    void ProcessMpdElement(const std::shared_ptr<XmlParser> &xmlParser,
                           DashList <std::shared_ptr<XmlElement>> &periodElementList,
                           std::shared_ptr<XmlElement> &childElement);
    void GetAdaptationSetAttr(IDashMpdNode *adptSetNode, DashAdptSetInfo *adptSetInfo);
    void GetAdaptationSetElement(std::shared_ptr<XmlParser> xmlParser, std::shared_ptr<XmlElement> rootElement,
                                 const DashPeriodInfo *periodInfo,
                                 DashAdptSetInfo *adptSetInfo);
    void ProcessAdpSetElement(std::shared_ptr<XmlParser> &xmlParser, DashAdptSetInfo *adptSetInfo,
                              DashList<std::shared_ptr<XmlElement>> &representationElementList,
                              std::shared_ptr<XmlElement> &childElement, DashElementList &dashElementList);
    void GetPeriodAttr(IDashMpdNode *periodNode, DashPeriodInfo *periodInfo);
    void GetPeriodElement(std::shared_ptr<XmlParser> xmlParser, std::shared_ptr<XmlElement> rootElement,
                          DashPeriodInfo *periodInfo);
    void ProcessPeriodElement(const std::shared_ptr<XmlParser> &xmlParser, DashPeriodInfo *periodInfo,
                              DashList <std::shared_ptr<XmlElement>> &adptSetElementList,
                              std::shared_ptr<XmlElement> &childElement, DashElementList &dashElementList);
    void GetRepresentationAttr(IDashMpdNode *representationNode, DashRepresentationInfo *representationInfo);
    void GetRepresentationElement(std::shared_ptr<XmlParser> xmlParser, std::shared_ptr<XmlElement> rootElement,
                                  const DashPeriodInfo *periodInfo,
                                  const DashAdptSetInfo *adptSetInfo, DashRepresentationInfo *representationInfo);
    void ProcessRepresentationElement(std::shared_ptr<XmlParser> &xmlParser, DashRepresentationInfo *representationInfo,
                                      std::shared_ptr<XmlElement> &childElement, DashElementList &dashElementList);

    void InheritMultSegBase(DashMultSegBaseInfo *lowerMultSegBaseInfo,
                            const DashMultSegBaseInfo *higherMultSegBaseInfo);
    void InheritSegBase(DashSegBaseInfo *lowerSegBaseInfo, const DashSegBaseInfo *higherSegBaseInfo) const;
    time_t String2Time(const std::string szTime);
    void ParseElement(std::shared_ptr<XmlParser> &xmlParser, DashSegTmpltInfo *segTmplt,
                      std::shared_ptr<XmlElement> &childElement);
    void ParsePeriodElement(std::shared_ptr<XmlParser> &xmlParser,
                            DashList<std::shared_ptr<XmlElement>> &periodElementList);
    void GetContentProtection(const std::shared_ptr<XmlParser> &xmlParser, DashDescriptor *contentProtection,
                              std::shared_ptr<XmlElement> &childElement) const;
    void ParseElementUrlType(std::shared_ptr<XmlParser> &xmlParser, DashSegBaseInfo *segBase,
                             std::shared_ptr<XmlElement> &childElement);
    void GetAdptSetCommonAttr(IDashMpdNode *adptSetNode, DashAdptSetInfo *adptSetInfo) const;

private:
    static constexpr const char *MPD_LABEL_MPD = "MPD";
    static constexpr const char *MPD_LABEL_BASE_URL = "BaseURL";
    static constexpr const char *MPD_LABEL_PERIOD = "Period";
    static constexpr const char *MPD_LABEL_SEGMENT_BASE = "SegmentBase";
    static constexpr const char *MPD_LABEL_SEGMENT_LIST = "SegmentList";
    static constexpr const char *MPD_LABEL_SEGMENT_TEMPLATE = "SegmentTemplate";
    static constexpr const char *MPD_LABEL_ADAPTATIONSET = "AdaptationSet";
    static constexpr const char *MPD_LABEL_REPRESENTATION = "Representation";
    static constexpr const char *MPD_LABEL_INITIALIZATION = "Initialization";
    static constexpr const char *MPD_LABEL_REPRESENTATION_INDEX = "RepresentationIndex";
    static constexpr const char *MPD_LABEL_BITSTREAM_SWITCHING = "BitstreamSwitching";
    static constexpr const char *MPD_LABEL_SEGMENT_TIMELINE = "SegmentTimeline";
    static constexpr const char *MPD_LABEL_SEGMENT_TIMELINE_S = "S";
    static constexpr const char *MPD_LABEL_SEGMENT_URL = "SegmentURL";
    static constexpr const char *MPD_LABEL_CONTENT_COMPONENT = "ContentComponent";
    static constexpr const char *MPD_LABEL_CONTENT_PROTECTION = "ContentProtection";
    static constexpr const char *MPD_LABEL_ESSENTIAL_PROPERTY = "EssentialProperty";
    static constexpr const char *MPD_LABEL_AUDIO_CHANNEL_CONFIGURATION = "AudioChannelConfiguration";

    DashMpdInfo dashMpdInfo_;
    int32_t stopFlag_{0};
};
} // namespace HttpPluginLite
} // namespace Plugin
} // namespace Media
} // namespace OHOS
#endif // DASH_MPD_PARSER_H
