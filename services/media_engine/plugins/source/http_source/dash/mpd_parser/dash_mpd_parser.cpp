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
#define HST_LOG_TAG "DashMpdParser"

#include <ctime>
#include "common/log.h"
#include "securec.h"
#include "dash_mpd_parser.h"
#include "dash_mpd_util.h"
#include "utils/time_utils.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_STREAM_SOURCE, "HiStreamer" };
}

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
namespace {
    constexpr unsigned int DEFAULT_YEAR = 1900;
    constexpr unsigned int DEFAULT_YEAR_LEN = 70;
    constexpr unsigned int DEFAULT_DAY = 2;
}

DashMpdParser::~DashMpdParser()
{
    Clear();
}

time_t DashMpdParser::String2Time(const std::string szTime)
{
    if (szTime.length() == 0) {
        return 0;
    }

    struct tm tm1;
    memset_s(&tm1, sizeof(tm1), 0, sizeof(tm1));
    struct tm tmBase;
    memset_s(&tmBase, sizeof(tmBase), 0, sizeof(tmBase));
    time_t t1;
    time_t tBase;

    if (-1 == sscanf_s(szTime.c_str(), "%4d-%2d-%2dT%2d:%2d:%2d", &tm1.tm_year, &tm1.tm_mon, &tm1.tm_mday, &tm1.tm_hour,
                       &tm1.tm_min, &tm1.tm_sec)) {
        MEDIA_LOG_E("String2Time format error " PUBLIC_LOG_S, szTime.c_str());
    }

    tm1.tm_year -= DEFAULT_YEAR;
    tm1.tm_mon--;
    // day + 1 for as 1970/1/1 00:00:00.000 in windows(return -1) and mac(return -28800) return different
    tm1.tm_mday += 1;

    tm1.tm_isdst = -1;

    tmBase.tm_mday = DEFAULT_DAY;
    tmBase.tm_year = DEFAULT_YEAR_LEN;
    tmBase.tm_isdst = -1;

    t1 = mktime(&tm1);
    tBase = mktime(&tmBase);

    return t1 - tBase;
}

void DashMpdParser::ParsePeriod(std::shared_ptr<XmlParser> xmlParser, std::shared_ptr<XmlElement> rootElement)
{
    DashPeriodInfo *periodInfo = new DashPeriodInfo;
    if (periodInfo != nullptr) {
        IDashMpdNode *periodNode = IDashMpdNode::CreateNode("Period");
        if (periodNode == nullptr) {
            delete periodInfo;
            return;
        }

        periodNode->ParseNode(xmlParser, rootElement);

        GetPeriodAttr(periodNode, periodInfo);
        GetPeriodElement(xmlParser, rootElement, periodInfo);

        dashMpdInfo_.periodInfoList_.push_back(periodInfo);
        IDashMpdNode::DestroyNode(periodNode);
    } else {
        MEDIA_LOG_E("ParsePeriod periodInfo == nullptr");
    }
}

void DashMpdParser::GetPeriodAttr(IDashMpdNode *periodNode, DashPeriodInfo *periodInfo)
{
    std::string tempStr;
    periodNode->GetAttr("id", periodInfo->id_);

    periodNode->GetAttr("start", tempStr);
    DashStrToDuration(tempStr, periodInfo->start_);

    periodNode->GetAttr("duration", tempStr);
    DashStrToDuration(tempStr, periodInfo->duration_);

    periodNode->GetAttr("bitstreamSwitching", tempStr);
    if (tempStr == "true") {
        periodInfo->bitstreamSwitching_ = true;
    } else {
        periodInfo->bitstreamSwitching_ = false;
    }
}

void DashMpdParser::GetPeriodElement(std::shared_ptr<XmlParser> xmlParser, std::shared_ptr<XmlElement> rootElement,
                                     DashPeriodInfo *periodInfo)
{
    DashList<std::shared_ptr<XmlElement>> adptSetElementList;
    DashElementList dashElementList;
    std::shared_ptr<XmlElement> childElement = rootElement->GetChild();
    while (childElement != nullptr) {
        if (this->stopFlag_) {
            break;
        }

        ProcessPeriodElement(xmlParser, periodInfo, adptSetElementList, childElement, dashElementList);
        childElement = childElement->GetSiblingNext();
    }

    // parse element SegmentBase
    if (dashElementList.segBaseElement_ != nullptr) {
        ParseSegmentBase(xmlParser, dashElementList.segBaseElement_, &periodInfo->periodSegBase_);
    }

    // parse element SegmentList
    if (dashElementList.segListElement_ != nullptr) {
        ParseSegmentList(xmlParser, dashElementList.segListElement_, &periodInfo->periodSegList_);
    }

    // parse element SegmentTemplate
    if (dashElementList.segTmplElement_ != nullptr) {
        ParseSegmentTemplate(xmlParser, dashElementList.segTmplElement_, &periodInfo->periodSegTmplt_);
    }

    // parse AdaptationSet label last
    while (adptSetElementList.size() > 0) {
        if (this->stopFlag_) {
            break;
        }

        std::shared_ptr<XmlElement> adptSetElement = adptSetElementList.front();
        if (adptSetElement != nullptr) {
            ParseAdaptationSet(xmlParser, adptSetElement, periodInfo);
        }

        adptSetElementList.pop_front();
    }
}

void DashMpdParser::ProcessPeriodElement(const std::shared_ptr<XmlParser> &xmlParser, DashPeriodInfo *periodInfo,
                                         DashList<std::shared_ptr<XmlElement>> &adptSetElementList,
                                         std::shared_ptr<XmlElement> &childElement, DashElementList &dashElementList)
{
    std::string elementNameStr = childElement->GetName();
    if (!elementNameStr.empty()) {
        xmlParser->SkipElementNameSpace(elementNameStr);

        if (elementNameStr == MPD_LABEL_BASE_URL) {
            // parse element BaseURL
            std::string strValue = childElement->GetText();
            if (!strValue.empty()) {
                periodInfo->baseUrl_.push_back(strValue);
            }
        } else if (elementNameStr == MPD_LABEL_SEGMENT_BASE) {
            dashElementList.segBaseElement_ = childElement;
        } else if (elementNameStr == MPD_LABEL_SEGMENT_LIST) {
            dashElementList.segListElement_ = childElement;
        } else if (elementNameStr == MPD_LABEL_SEGMENT_TEMPLATE) {
            dashElementList.segTmplElement_ = childElement;
        } else if (elementNameStr == MPD_LABEL_ADAPTATIONSET) {
            // parse element AdaptationSet
            adptSetElementList.push_back(childElement);
        }
    }
}

void DashMpdParser::ParseAdaptationSet(std::shared_ptr<XmlParser> xmlParser, std::shared_ptr<XmlElement> rootElement,
                                       DashPeriodInfo *periodInfo)
{
    DashAdptSetInfo *adptSetInfo = new DashAdptSetInfo;
    if (adptSetInfo != nullptr) {
        IDashMpdNode *adptSetNode = IDashMpdNode::CreateNode(MPD_LABEL_ADAPTATIONSET);
        if (adptSetNode == nullptr) {
            delete adptSetInfo;
            return;
        }

        adptSetNode->ParseNode(xmlParser, rootElement);
        GetAdaptationSetAttr(adptSetNode, adptSetInfo);
        GetAdaptationSetElement(xmlParser, rootElement, periodInfo, adptSetInfo);

        periodInfo->adptSetList_.push_back(adptSetInfo);
        IDashMpdNode::DestroyNode(adptSetNode);
    } else {
        MEDIA_LOG_E("ParseAdaptationSet adptSetInfo == nullptr");
    }
}

void DashMpdParser::GetAdaptationSetAttr(IDashMpdNode *adptSetNode, DashAdptSetInfo *adptSetInfo)
{
    GetAdptSetCommonAttr(adptSetNode, adptSetInfo);

    double dValue;
    adptSetNode->GetAttr("maxPlayoutRate", dValue);
    const double eps = 1e-8; // Limiting the Error Range
    if (fabs(dValue) > eps) {
        adptSetInfo->commonAttrsAndElements_.maxPlayoutRate_ = dValue;
    }

    std::string str;
    adptSetNode->GetAttr("codingDependency", str);
    if (str == "true") {
        adptSetInfo->commonAttrsAndElements_.codingDependency_ = 1;
    }

    adptSetNode->GetAttr("scanType", str);
    if (str == "interlaced") {
        adptSetInfo->commonAttrsAndElements_.scanType_ = VideoScanType::VIDEO_SCAN_INTERLACED;
    } else if (str == "unknown") {
        adptSetInfo->commonAttrsAndElements_.scanType_ = VideoScanType::VIDEO_SCAN_UNKNOW;
    }
    adptSetNode->GetAttr("mimeType", adptSetInfo->mimeType_);
    adptSetNode->GetAttr("lang", adptSetInfo->lang_);
    adptSetNode->GetAttr("contentType", adptSetInfo->contentType_);
    adptSetNode->GetAttr("par", adptSetInfo->par_);
    adptSetNode->GetAttr("minFrameRate", adptSetInfo->minFrameRate_);
    adptSetNode->GetAttr("maxFrameRate", adptSetInfo->maxFrameRate_);
    adptSetNode->GetAttr("videoType", adptSetInfo->videoType_);

    adptSetNode->GetAttr("id", adptSetInfo->id_);
    adptSetNode->GetAttr("group", adptSetInfo->group_);
    adptSetNode->GetAttr("minBandwidth", adptSetInfo->minBandwidth_);
    adptSetNode->GetAttr("maxBandwidth", adptSetInfo->maxBandwidth_);
    adptSetNode->GetAttr("minWidth", adptSetInfo->minWidth_);
    adptSetNode->GetAttr("maxWidth", adptSetInfo->maxWidth_);
    adptSetNode->GetAttr("minHeight", adptSetInfo->minHeight_);
    adptSetNode->GetAttr("maxHeight", adptSetInfo->maxHeight_);
    adptSetNode->GetAttr("cameraIndex", adptSetInfo->cameraIndex_);

    adptSetNode->GetAttr("bitstreamSwitching", str);
    if (str == "true") {
        adptSetInfo->bitstreamSwitching_ = true;
    }

    adptSetNode->GetAttr("segmentAlignment", str);
    if (str == "true") {
        adptSetInfo->segmentAlignment_ = true;
    }

    adptSetNode->GetAttr("subsegmentAlignment", str);
    if (str == "true") {
        adptSetInfo->subSegmentAlignment_ = true;
    }
}

void DashMpdParser::GetAdptSetCommonAttr(IDashMpdNode *adptSetNode, DashAdptSetInfo *adptSetInfo) const
{
    adptSetNode->GetAttr("width", adptSetInfo->commonAttrsAndElements_.width_);
    adptSetNode->GetAttr("height", adptSetInfo->commonAttrsAndElements_.height_);
    adptSetNode->GetAttr("startWithSAP", adptSetInfo->commonAttrsAndElements_.startWithSAP_);
    adptSetNode->GetAttr("sar", adptSetInfo->commonAttrsAndElements_.sar_);
    adptSetNode->GetAttr("mimeType", adptSetInfo->commonAttrsAndElements_.mimeType_);
    adptSetNode->GetAttr("codecs", adptSetInfo->commonAttrsAndElements_.codecs_);
    adptSetNode->GetAttr("audioSamplingRate", adptSetInfo->commonAttrsAndElements_.audioSamplingRate_);
    adptSetNode->GetAttr("frameRate", adptSetInfo->commonAttrsAndElements_.frameRate_);
    adptSetNode->GetAttr("profiles", adptSetInfo->commonAttrsAndElements_.profiles_);
}

void DashMpdParser::GetAdaptationSetElement(std::shared_ptr<XmlParser> xmlParser,
                                            std::shared_ptr<XmlElement> rootElement, const DashPeriodInfo *periodInfo,
                                            DashAdptSetInfo *adptSetInfo)
{
    // parse element in AdaptationSet
    DashList<std::shared_ptr<XmlElement>> representationElementList;
    DashElementList dashElementList;
    std::shared_ptr<XmlElement> childElement = rootElement->GetChild();
    while (childElement != nullptr) {
        if (this->stopFlag_) {
            break;
        }

        ProcessAdpSetElement(xmlParser, adptSetInfo, representationElementList, childElement, dashElementList);
        childElement = childElement->GetSiblingNext();
    }

    // parse element SegmentBase
    if (dashElementList.segBaseElement_ != nullptr) {
        ParseSegmentBase(xmlParser, dashElementList.segBaseElement_, &adptSetInfo->adptSetSegBase_);
    }

    // parse element SegmentList
    if (dashElementList.segListElement_ != nullptr) {
        ParseSegmentList(xmlParser, dashElementList.segListElement_, &adptSetInfo->adptSetSegList_);
    }

    // parse element SegmentTemplate
    if (dashElementList.segTmplElement_ != nullptr) {
        ParseSegmentTemplate(xmlParser, dashElementList.segTmplElement_, &adptSetInfo->adptSetSegTmplt_);
    }

    // (segmentTemplate/segmentList/segmentBase) in adaptationset inherit from period
    if (adptSetInfo->adptSetSegTmplt_ != nullptr && periodInfo->periodSegTmplt_ != nullptr) {
        InheritMultSegBase(&adptSetInfo->adptSetSegTmplt_->multSegBaseInfo_,
                           &periodInfo->periodSegTmplt_->multSegBaseInfo_);
    }

    if (adptSetInfo->adptSetSegList_ != nullptr && periodInfo->periodSegList_ != nullptr) {
        InheritMultSegBase(&adptSetInfo->adptSetSegList_->multSegBaseInfo_,
                           &periodInfo->periodSegList_->multSegBaseInfo_);
    }

    if (adptSetInfo->adptSetSegBase_ != nullptr && periodInfo->periodSegBase_ != nullptr) {
        InheritSegBase(adptSetInfo->adptSetSegBase_, periodInfo->periodSegBase_);
    }

    // parse representation last
    while (representationElementList.size() > 0) {
        if (this->stopFlag_) {
            break;
        }

        std::shared_ptr<XmlElement> representationElement = representationElementList.front();
        if (representationElement != nullptr) {
            ParseRepresentation(xmlParser, representationElement, periodInfo, adptSetInfo);
        }

        representationElementList.pop_front();
    }
}

void DashMpdParser::ProcessAdpSetElement(std::shared_ptr<XmlParser> &xmlParser, DashAdptSetInfo *adptSetInfo,
                                         DashList<std::shared_ptr<XmlElement>> &representationElementList,
                                         std::shared_ptr<XmlElement> &childElement, DashElementList &dashElementList)
{
    std::string elementNameStr = childElement->GetName();
    if (!elementNameStr.empty()) {
        xmlParser->SkipElementNameSpace(elementNameStr);

        if (elementNameStr == MPD_LABEL_BASE_URL) {
            // parse element BaseURL
            std::string strValue = childElement->GetText();
            if (!strValue.empty()) {
                adptSetInfo->baseUrl_.push_back(strValue);
            }
        } else if (elementNameStr == MPD_LABEL_CONTENT_COMPONENT) {
            // parse element ContentComponent
            ParseContentComponent(xmlParser, childElement, adptSetInfo->contentCompList_);
        } else if (elementNameStr == MPD_LABEL_SEGMENT_BASE) {
            dashElementList.segBaseElement_ = childElement;
        } else if (elementNameStr == MPD_LABEL_SEGMENT_LIST) {
            dashElementList.segListElement_ = childElement;
        } else if (elementNameStr == MPD_LABEL_SEGMENT_TEMPLATE) {
            dashElementList.segTmplElement_ = childElement;
        } else if (elementNameStr == MPD_LABEL_REPRESENTATION) {
            // get element Representation
            representationElementList.push_back(childElement);
        } else if (elementNameStr == MPD_LABEL_CONTENT_PROTECTION) {
            // parse element ContentProtection
            ParseContentProtection(xmlParser, childElement,
                                   adptSetInfo->commonAttrsAndElements_.contentProtectionList_);
        } else if (elementNameStr == MPD_LABEL_ESSENTIAL_PROPERTY) {
            ParseEssentialProperty(xmlParser, childElement,
                                   adptSetInfo->commonAttrsAndElements_.essentialPropertyList_);
        }
    }
}

void DashMpdParser::ParseSegmentUrl(std::shared_ptr<XmlParser> xmlParser, std::shared_ptr<XmlElement> rootElement,
                                    DashList<DashSegUrl *> &segUrlList)
{
    DashSegUrl *segUrl = new (std::nothrow) DashSegUrl;
    if (segUrl == nullptr) {
        return;
    }

    IDashMpdNode *segUrlNode = IDashMpdNode::CreateNode(MPD_LABEL_SEGMENT_URL);
    if (segUrlNode == nullptr) {
        delete segUrl;
        return;
    }

    segUrlNode->ParseNode(xmlParser, rootElement);
    segUrlNode->GetAttr("media", segUrl->media_);
    segUrlNode->GetAttr("mediaRange", segUrl->mediaRange_);
    segUrlNode->GetAttr("index", segUrl->index_);
    segUrlNode->GetAttr("indexRange", segUrl->indexRange_);

    segUrlList.push_back(segUrl);
    IDashMpdNode::DestroyNode(segUrlNode);
}

void DashMpdParser::ParseContentComponent(std::shared_ptr<XmlParser> xmlParser, std::shared_ptr<XmlElement> rootElement,
                                          DashList<DashContentCompInfo *> &contentCompInfoList)
{
    DashContentCompInfo *contentCompInfo = new (std::nothrow) DashContentCompInfo;
    if (contentCompInfo == nullptr) {
        return;
    }

    IDashMpdNode *contentCompNode = IDashMpdNode::CreateNode(MPD_LABEL_CONTENT_COMPONENT);
    if (contentCompNode != nullptr) {
        contentCompNode->ParseNode(xmlParser, rootElement);
        contentCompNode->GetAttr("id", contentCompInfo->id_);
        contentCompNode->GetAttr("par", contentCompInfo->par_);
        contentCompNode->GetAttr("lang", contentCompInfo->lang_);
        contentCompNode->GetAttr("contentType", contentCompInfo->contentType_);

        contentCompInfoList.push_back(contentCompInfo);
        IDashMpdNode::DestroyNode(contentCompNode);
    } else {
        MEDIA_LOG_E("ParseContentComponent contentCompNode == nullptr");
        delete contentCompInfo;
    }
}

void DashMpdParser::ParseSegmentBase(std::shared_ptr<XmlParser> xmlParser, std::shared_ptr<XmlElement> rootElement,
                                     DashSegBaseInfo **segBaseInfo)
{
    if (segBaseInfo == nullptr) {
        MEDIA_LOG_E("ParseSegmentBase segBaseInfo == nullptr");
        return;
    }

    DashSegBaseInfo *segBase = new (std::nothrow) DashSegBaseInfo;
    if (segBase == nullptr) {
        return;
    }

    IDashMpdNode *segBaseNode = IDashMpdNode::CreateNode(MPD_LABEL_SEGMENT_BASE);
    if (segBaseNode != nullptr) {
        segBaseNode->ParseNode(xmlParser, rootElement);
        segBaseNode->GetAttr("timescale", segBase->timeScale_);
        segBaseNode->GetAttr("presentationTimeOffset", segBase->presentationTimeOffset_);
        segBaseNode->GetAttr("indexRange", segBase->indexRange_);
        std::string tmp;
        segBaseNode->GetAttr("indexRangeExact", tmp);
        if (tmp == "true") {
            segBase->indexRangeExact_ = true;
        } else {
            segBase->indexRangeExact_ = false;
        }

        // parse element SegmentBase
        std::shared_ptr<XmlElement> childElement = rootElement->GetChild();
        while (childElement != nullptr) {
            if (this->stopFlag_) {
                break;
            }

            ParseElementUrlType(xmlParser, segBase, childElement);
            childElement = childElement->GetSiblingNext();
        }

        *segBaseInfo = segBase;
        IDashMpdNode::DestroyNode(segBaseNode);
    } else {
        MEDIA_LOG_E("ParseSegmentBase segBaseNode == nullptr");
        delete segBase;
    }
}

void DashMpdParser::ParseElementUrlType(std::shared_ptr<XmlParser> &xmlParser, DashSegBaseInfo *segBase,
                                        std::shared_ptr<XmlElement> &childElement)
{
    std::string elementNameStr = childElement->GetName();
    if (!elementNameStr.empty()) {
        xmlParser->SkipElementNameSpace(elementNameStr);

        if (elementNameStr == MPD_LABEL_INITIALIZATION) {
            // parse element Initialization
            ParseUrlType(xmlParser, childElement, elementNameStr, &segBase->initialization_);
        } else if (elementNameStr == MPD_LABEL_REPRESENTATION_INDEX) {
            // parse element RepresentationIndex
            ParseUrlType(xmlParser, childElement, elementNameStr, &segBase->representationIndex_);
        }
    }
}

void DashMpdParser::ParseSegmentList(std::shared_ptr<XmlParser> xmlParser, std::shared_ptr<XmlElement> rootElement,
                                     DashSegListInfo **segListInfo)
{
    if (segListInfo == nullptr) {
        MEDIA_LOG_E("ParseSegmentList segListInfo == nullptr");
        return;
    }

    DashSegListInfo *segList = new (std::nothrow) DashSegListInfo;
    if (segList == nullptr) {
        return;
    }

    IDashMpdNode *segListNode = IDashMpdNode::CreateNode(MPD_LABEL_SEGMENT_LIST);
    if (segListNode != nullptr) {
        segListNode->ParseNode(xmlParser, rootElement);
        segListNode->GetAttr("duration", segList->multSegBaseInfo_.duration_);
        segListNode->GetAttr("startNumber", segList->multSegBaseInfo_.startNumber_);
        segListNode->GetAttr("timescale", segList->multSegBaseInfo_.segBaseInfo_.timeScale_);
        segListNode->GetAttr("presentationTimeOffset",
                             segList->multSegBaseInfo_.segBaseInfo_.presentationTimeOffset_);
        // parse element SegmentList
        std::shared_ptr<XmlElement> childElement = rootElement->GetChild();
        while (childElement != nullptr) {
            if (this->stopFlag_) {
                break;
            }

            ParseSegmentListElement(xmlParser, segList, childElement);
            childElement = childElement->GetSiblingNext();
        }

        *segListInfo = segList;
        IDashMpdNode::DestroyNode(segListNode);
    } else {
        MEDIA_LOG_E("ParseSegmentList segListNode == nullptr");
        delete segList;
    }
}

void DashMpdParser::ParseSegmentListElement(std::shared_ptr<XmlParser> &xmlParser, DashSegListInfo *segList,
                                            std::shared_ptr<XmlElement> &childElement)
{
    std::string elementNameStr = childElement->GetName();
    if (!elementNameStr.empty()) {
        xmlParser->SkipElementNameSpace(elementNameStr);

        if (elementNameStr == MPD_LABEL_SEGMENT_TIMELINE) {
            // parse element SegmentTimeline
            ParseSegmentTimeline(xmlParser, childElement, segList->multSegBaseInfo_.segTimeline_);
        } else if (elementNameStr == MPD_LABEL_BITSTREAM_SWITCHING) {
            // parse element BitstreamSwitching
            ParseUrlType(xmlParser, childElement, elementNameStr,
                         &segList->multSegBaseInfo_.bitstreamSwitching_);
        } else if (elementNameStr == MPD_LABEL_INITIALIZATION) {
            // parse element Initialization
            ParseUrlType(xmlParser, childElement, elementNameStr,
                         &segList->multSegBaseInfo_.segBaseInfo_.initialization_);
        } else if (elementNameStr == MPD_LABEL_REPRESENTATION_INDEX) {
            // parse element RepresentationIndex
            ParseUrlType(xmlParser, childElement, elementNameStr,
                         &segList->multSegBaseInfo_.segBaseInfo_.representationIndex_);
        } else if (elementNameStr == MPD_LABEL_SEGMENT_URL) {
            // parse element SegmentURL
            ParseSegmentUrl(xmlParser, childElement, segList->segmentUrl_);
        }
    }
}

void DashMpdParser::InheritMultSegBase(DashMultSegBaseInfo *lowerMultSegBaseInfo,
                                       const DashMultSegBaseInfo *higherMultSegBaseInfo)
{
    if (lowerMultSegBaseInfo->duration_ == 0 && higherMultSegBaseInfo->duration_ > 0) {
        lowerMultSegBaseInfo->duration_ = higherMultSegBaseInfo->duration_;
    }

    if (lowerMultSegBaseInfo->segBaseInfo_.timeScale_ == 0 && higherMultSegBaseInfo->segBaseInfo_.timeScale_ > 0) {
        lowerMultSegBaseInfo->segBaseInfo_.timeScale_ = higherMultSegBaseInfo->segBaseInfo_.timeScale_;
    }

    if (lowerMultSegBaseInfo->startNumber_.length() == 0 && higherMultSegBaseInfo->startNumber_.length() > 0) {
        lowerMultSegBaseInfo->startNumber_ = higherMultSegBaseInfo->startNumber_;
    }
}

void DashMpdParser::InheritSegBase(DashSegBaseInfo *lowerSegBaseInfo, const DashSegBaseInfo *higherSegBaseInfo) const
{
    if (lowerSegBaseInfo->timeScale_ == 0 && higherSegBaseInfo->timeScale_ > 0) {
        lowerSegBaseInfo->timeScale_ = higherSegBaseInfo->timeScale_;
    }
}

void DashMpdParser::ParseSegmentTemplate(std::shared_ptr<XmlParser> xmlParser, std::shared_ptr<XmlElement> rootElement,
                                         DashSegTmpltInfo **segTmpltInfo)
{
    if (segTmpltInfo == nullptr) {
        MEDIA_LOG_E("ParseSegmentTemplate segTmpltInfo == nullptr");
        return;
    }

    DashSegTmpltInfo *segTmplt = new DashSegTmpltInfo;
    if (segTmplt != nullptr) {
        IDashMpdNode *segTmpltNode = IDashMpdNode::CreateNode(MPD_LABEL_SEGMENT_TEMPLATE);
        if (segTmpltNode == nullptr) {
            delete segTmplt;
            return;
        }

        segTmpltNode->ParseNode(xmlParser, rootElement);
        segTmpltNode->GetAttr("timescale", segTmplt->multSegBaseInfo_.segBaseInfo_.timeScale_);
        segTmpltNode->GetAttr("presentationTimeOffset",
                              segTmplt->multSegBaseInfo_.segBaseInfo_.presentationTimeOffset_);
        segTmpltNode->GetAttr("duration", segTmplt->multSegBaseInfo_.duration_);
        segTmpltNode->GetAttr("startNumber", segTmplt->multSegBaseInfo_.startNumber_);
        segTmpltNode->GetAttr("media", segTmplt->segTmpltMedia_);
        segTmpltNode->GetAttr("index", segTmplt->segTmpltIndex_);
        segTmpltNode->GetAttr("initialization", segTmplt->segTmpltInitialization_);
        segTmpltNode->GetAttr("bitstreamSwitching", segTmplt->segTmpltBitstreamSwitching_);

        // parse element SegmentTemplate
        std::shared_ptr<XmlElement> childElement = rootElement->GetChild();
        while (childElement != nullptr) {
            if (this->stopFlag_) {
                break;
            }

            ParseElement(xmlParser, segTmplt, childElement);
            childElement = childElement->GetSiblingNext();
        }

        *segTmpltInfo = segTmplt;
        IDashMpdNode::DestroyNode(segTmpltNode);
    }
}

void DashMpdParser::ParseElement(std::shared_ptr<XmlParser> &xmlParser, DashSegTmpltInfo *segTmplt,
                                 std::shared_ptr<XmlElement> &childElement)
{
    std::string elementNameStr = childElement->GetName();
    if (!elementNameStr.empty()) {
        xmlParser->SkipElementNameSpace(elementNameStr);

        if (elementNameStr == MPD_LABEL_SEGMENT_TIMELINE) {
            // parse element SegmentTimeline
            ParseSegmentTimeline(xmlParser, childElement, segTmplt->multSegBaseInfo_.segTimeline_);
        } else if (elementNameStr == MPD_LABEL_BITSTREAM_SWITCHING) {
            // parse element BitstreamSwitching
            ParseUrlType(xmlParser, childElement, elementNameStr,
                         &segTmplt->multSegBaseInfo_.bitstreamSwitching_);
        } else if (elementNameStr == MPD_LABEL_INITIALIZATION) {
            // parse element Initialization
            ParseUrlType(xmlParser, childElement, elementNameStr,
                         &segTmplt->multSegBaseInfo_.segBaseInfo_.initialization_);
        } else if (elementNameStr == MPD_LABEL_REPRESENTATION_INDEX) {
            // parse element RepresentationIndex
            ParseUrlType(xmlParser, childElement, elementNameStr,
                         &segTmplt->multSegBaseInfo_.segBaseInfo_.representationIndex_);
        }
    }
}

void DashMpdParser::ParseRepresentation(std::shared_ptr<XmlParser> xmlParser, std::shared_ptr<XmlElement> rootElement,
                                        const DashPeriodInfo *periodInfo,
                                        DashAdptSetInfo *adptSetInfo)
{
    DashRepresentationInfo *representationInfo = new (std::nothrow) DashRepresentationInfo;
    if (representationInfo == nullptr) {
        return;
    }

    IDashMpdNode *representationNode = IDashMpdNode::CreateNode(MPD_LABEL_REPRESENTATION);
    if (representationNode != nullptr) {
        representationNode->ParseNode(xmlParser, rootElement);

        GetRepresentationAttr(representationNode, representationInfo);
        GetRepresentationElement(xmlParser, rootElement, periodInfo, adptSetInfo, representationInfo);

        adptSetInfo->representationList_.push_back(representationInfo);
        IDashMpdNode::DestroyNode(representationNode);
    } else {
        MEDIA_LOG_E("ParseRepresentation representationNode == nullptr");
        delete representationInfo;
    }
}

void DashMpdParser::GetRepresentationAttr(IDashMpdNode *representationNode, DashRepresentationInfo *representationInfo)
{
    representationNode->GetAttr("id", representationInfo->id_);
    representationNode->GetAttr("volumeAdjust_", representationInfo->volumeAdjust_);
    representationNode->GetAttr("bandwidth", representationInfo->bandwidth_);
    representationNode->GetAttr("qualityRanking", representationInfo->qualityRanking_);
    representationNode->GetAttr("width", representationInfo->commonAttrsAndElements_.width_);
    representationNode->GetAttr("height", representationInfo->commonAttrsAndElements_.height_);
    representationNode->GetAttr("frameRate", representationInfo->commonAttrsAndElements_.frameRate_);
    representationNode->GetAttr("codecs", representationInfo->commonAttrsAndElements_.codecs_);
    representationNode->GetAttr("mimeType", representationInfo->commonAttrsAndElements_.mimeType_);
    representationNode->GetAttr("startWithSAP", representationInfo->commonAttrsAndElements_.startWithSAP_);
    representationNode->GetAttr("cuvvVersion", representationInfo->commonAttrsAndElements_.cuvvVersion_);
}

void DashMpdParser::GetRepresentationElement(std::shared_ptr<XmlParser> xmlParser,
                                             std::shared_ptr<XmlElement> rootElement,
                                             const DashPeriodInfo *periodInfo, const DashAdptSetInfo *adptSetInfo,
                                             DashRepresentationInfo *representationInfo)
{
    // parse element Representation
    DashElementList dashElementList;
    std::shared_ptr<XmlElement> childElement = rootElement->GetChild();
    while (childElement != nullptr) {
        if (this->stopFlag_) {
            break;
        }

        ProcessRepresentationElement(xmlParser, representationInfo, childElement, dashElementList);
        childElement = childElement->GetSiblingNext();
    }

    // parse element SegmentBase
    if (dashElementList.segBaseElement_ != nullptr) {
        ParseSegmentBase(xmlParser, dashElementList.segBaseElement_, &representationInfo->representationSegBase_);
    }

    // parse element SegmentList
    if (dashElementList.segListElement_ != nullptr) {
        ParseSegmentList(xmlParser, dashElementList.segListElement_, &representationInfo->representationSegList_);
    }

    // parse element SegmentTemplate
    if (dashElementList.segTmplElement_ != nullptr) {
        ParseSegmentTemplate(xmlParser, dashElementList.segTmplElement_, &representationInfo->representationSegTmplt_);
    }

    // (segmentTemplate/segmentList/segmentBase) in representation inherit from adaptationset or period
    if (representationInfo->representationSegTmplt_ != nullptr && adptSetInfo->adptSetSegTmplt_ != nullptr) {
        InheritMultSegBase(&representationInfo->representationSegTmplt_->multSegBaseInfo_,
                           &adptSetInfo->adptSetSegTmplt_->multSegBaseInfo_);
    } else if (representationInfo->representationSegTmplt_ != nullptr && periodInfo->periodSegTmplt_ != nullptr) {
        InheritMultSegBase(&representationInfo->representationSegTmplt_->multSegBaseInfo_,
                           &periodInfo->periodSegTmplt_->multSegBaseInfo_);
    }

    if (representationInfo->representationSegList_ != nullptr && adptSetInfo->adptSetSegList_ != nullptr) {
        InheritMultSegBase(&representationInfo->representationSegList_->multSegBaseInfo_,
                           &adptSetInfo->adptSetSegList_->multSegBaseInfo_);
    } else if (representationInfo->representationSegList_ != nullptr && periodInfo->periodSegList_ != nullptr) {
        InheritMultSegBase(&representationInfo->representationSegList_->multSegBaseInfo_,
                           &periodInfo->periodSegList_->multSegBaseInfo_);
    }

    if (representationInfo->representationSegBase_ != nullptr && adptSetInfo->adptSetSegBase_ != nullptr) {
        InheritSegBase(representationInfo->representationSegBase_, adptSetInfo->adptSetSegBase_);
    } else if (representationInfo->representationSegBase_ != nullptr && periodInfo->periodSegBase_ != nullptr) {
        InheritSegBase(representationInfo->representationSegBase_, periodInfo->periodSegBase_);
    }
}

void DashMpdParser::ProcessRepresentationElement(std::shared_ptr<XmlParser> &xmlParser,
                                                 DashRepresentationInfo *representationInfo,
                                                 std::shared_ptr<XmlElement> &childElement,
                                                 DashElementList &dashElementList)
{
    std::string elementNameStr = childElement->GetName();
    if (!elementNameStr.empty()) {
        xmlParser->SkipElementNameSpace(elementNameStr);

        if (elementNameStr == MPD_LABEL_BASE_URL) {
            // parse element BaseURL
            std::string strValue = childElement->GetText();
            if (!strValue.empty()) {
                representationInfo->baseUrl_.push_back(strValue);
            }
        } else if (elementNameStr == MPD_LABEL_SEGMENT_BASE) {
            dashElementList.segBaseElement_ = childElement;
        } else if (elementNameStr == MPD_LABEL_SEGMENT_LIST) {
            dashElementList.segListElement_ = childElement;
        } else if (elementNameStr == MPD_LABEL_SEGMENT_TEMPLATE) {
            dashElementList.segTmplElement_ = childElement;
        } else if (elementNameStr == MPD_LABEL_CONTENT_PROTECTION) {
            // parse element ContentProtection
            ParseContentProtection(xmlParser, childElement,
                                   representationInfo->commonAttrsAndElements_.contentProtectionList_);
        } else if (elementNameStr == MPD_LABEL_ESSENTIAL_PROPERTY) {
            ParseEssentialProperty(xmlParser, childElement,
                                   representationInfo->commonAttrsAndElements_.essentialPropertyList_);
        } else if (elementNameStr == MPD_LABEL_AUDIO_CHANNEL_CONFIGURATION) {
            ParseAudioChannelConfiguration(
                xmlParser, childElement,
                representationInfo->commonAttrsAndElements_.audioChannelConfigurationList_);
        }
    }
}

void DashMpdParser::ParseContentProtection(std::shared_ptr<XmlParser> xmlParser,
                                           std::shared_ptr<XmlElement> rootElement,
                                           DashList<DashDescriptor *> &contentProtectionList)
{
    DashDescriptor *contentProtection = new (std::nothrow) DashDescriptor;
    if (contentProtection == nullptr) {
        return;
    }

    IDashMpdNode *contentProtectionNode = IDashMpdNode::CreateNode(MPD_LABEL_CONTENT_PROTECTION);
    if (contentProtectionNode != nullptr) {
        contentProtectionNode->ParseNode(xmlParser, rootElement);
        contentProtectionNode->GetAttr("schemeIdUri", contentProtection->schemeIdUrl_);
        contentProtectionNode->GetAttr("value", contentProtection->value_);
        contentProtectionNode->GetAttr(MPD_LABEL_DEFAULT_KID, contentProtection->defaultKid_);

        std::shared_ptr<XmlElement> childElement = rootElement->GetChild();
        while (childElement != nullptr) {
            if (this->stopFlag_) {
                break;
            }

            GetContentProtection(xmlParser, contentProtection, childElement);
            childElement = childElement->GetSiblingNext();
        }

        contentProtectionList.push_back(contentProtection);
        IDashMpdNode::DestroyNode(contentProtectionNode);
    } else {
        MEDIA_LOG_E("contentProtectionNode == nullptr");
        delete contentProtection;
    }
}

void DashMpdParser::GetContentProtection(const std::shared_ptr<XmlParser> &xmlParser, DashDescriptor *contentProtection,
                                         std::shared_ptr<XmlElement> &childElement) const
{
    std::string elementNameStr = childElement->GetName();
    if (!elementNameStr.empty()) {
        xmlParser->SkipElementNameSpace(elementNameStr);

        if (elementNameStr == MPD_LABEL_PSSH && contentProtection->elementMap_.size() == 0) {
            // only support one pssh in ContentProtection
            std::string psshValue = childElement->GetText();
            if (!psshValue.empty()) {
                std::string pssh(MPD_LABEL_PSSH);
                contentProtection->elementMap_.insert(
                    std::map<std::string, std::string>::value_type(pssh, psshValue));
            }
        }
    }
}

void DashMpdParser::ParseEssentialProperty(std::shared_ptr<XmlParser> xmlParser,
                                           std::shared_ptr<XmlElement> rootElement,
                                           DashList<DashDescriptor *> &essentialPropertyList)
{
    DashDescriptor *essentialProperty = new (std::nothrow) DashDescriptor;
    if (essentialProperty == nullptr) {
        return;
    }

    IDashMpdNode *essentialPropertyNode = IDashMpdNode::CreateNode(MPD_LABEL_ESSENTIAL_PROPERTY);
    if (essentialPropertyNode != nullptr) {
        essentialPropertyNode->ParseNode(xmlParser, rootElement);
        essentialPropertyNode->GetAttr("schemeIdUri", essentialProperty->schemeIdUrl_);
        essentialPropertyNode->GetAttr("value", essentialProperty->value_);

        essentialPropertyList.push_back(essentialProperty);
        IDashMpdNode::DestroyNode(essentialPropertyNode);
    } else {
        MEDIA_LOG_E("essentialPropertyNode == nullptr");
        delete essentialProperty;
    }
}

void DashMpdParser::ParseAudioChannelConfiguration(std::shared_ptr<XmlParser> xmlParser,
                                                   std::shared_ptr<XmlElement> rootElement,
                                                   DashList<DashDescriptor *> &propertyList)
{
    DashDescriptor *channelProperty = new (std::nothrow) DashDescriptor;
    if (channelProperty == nullptr) {
        return;
    }

    IDashMpdNode *node = IDashMpdNode::CreateNode(MPD_LABEL_AUDIO_CHANNEL_CONFIGURATION);
    if (node != nullptr) {
        node->ParseNode(xmlParser, rootElement);
        node->GetAttr("schemeIdUri", channelProperty->schemeIdUrl_);
        node->GetAttr("value", channelProperty->value_);
        propertyList.push_back(channelProperty);

        IDashMpdNode::DestroyNode(node);
    } else {
        MEDIA_LOG_E("audioChannelConfigurationNode=null");
        delete channelProperty;
    }
}

void DashMpdParser::ParseSegmentTimeline(std::shared_ptr<XmlParser> xmlParser, std::shared_ptr<XmlElement> rootElement,
                                         DashList<DashSegTimeline *> &segTmlineList)
{
    // parse element SegmentTimeliness
    std::shared_ptr<XmlElement> childElement = rootElement->GetChild();
    while (childElement != nullptr) {
        if (this->stopFlag_) {
            break;
        }

        std::string elementNameStr = childElement->GetName();
        if (!elementNameStr.empty()) {
            xmlParser->SkipElementNameSpace(elementNameStr);
            if (elementNameStr != MPD_LABEL_SEGMENT_TIMELINE_S) {
                childElement = childElement->GetSiblingNext();
                continue;
            }

            DashSegTimeline *segTimeLine = new (std::nothrow) DashSegTimeline;
            if (segTimeLine == nullptr) {
                MEDIA_LOG_E("segTimeLine == nullptr");
                break;
            }

            IDashMpdNode *node = IDashMpdNode::CreateNode("SegmentTimeline");
            if (node != nullptr) {
                node->ParseNode(xmlParser, childElement);
                node->GetAttr("t", segTimeLine->t_);
                node->GetAttr("d", segTimeLine->d_);
                node->GetAttr("r", segTimeLine->r_);

                segTmlineList.push_back(segTimeLine);
                IDashMpdNode::DestroyNode(node);
            } else {
                MEDIA_LOG_E("SegmentTimeline node == nullptr");
                delete segTimeLine;
                break;
            }
        }

        childElement = childElement->GetSiblingNext();
    }
}

void DashMpdParser::ParseUrlType(std::shared_ptr<XmlParser> xmlParser, std::shared_ptr<XmlElement> rootElement,
                                 const std::string urlTypeName,
                                 DashUrlType **urlTypeInfo)
{
    if (urlTypeInfo == nullptr) {
        MEDIA_LOG_E("ParseUrlType urlTypeInfo == nullptr");
        return;
    }

    if (*urlTypeInfo != nullptr) {
        MEDIA_LOG_W("ParseUrlType " PUBLIC_LOG_S ", the value is not nullptr", urlTypeName.c_str());
        return;
    }

    DashUrlType *urlType = new (std::nothrow) DashUrlType;
    if (urlType == nullptr) {
        return;
    }

    IDashMpdNode *urlTypeNode = IDashMpdNode::CreateNode(urlTypeName);
    if (urlTypeNode != nullptr) {
        urlTypeNode->ParseNode(xmlParser, rootElement);
        urlTypeNode->GetAttr("sourceURL", urlType->sourceUrl_);
        urlTypeNode->GetAttr("range", urlType->range_);

        IDashMpdNode::DestroyNode(urlTypeNode);
    }

    *urlTypeInfo = urlType;
}

void DashMpdParser::ParseProgramInfo(std::list<std::string> &programStrList) const {}

void DashMpdParser::ParseBaseUrl(std::list<std::string> &baseUrlStrList) const {}

void DashMpdParser::ParseLocation(std::list<std::string> &locationStrlist) const {}

void DashMpdParser::ParseMetrics(std::list<std::string> &metricsStrList) const {}

void DashMpdParser::GetMpdAttr(IDashMpdNode *mpdNode)
{
    mpdNode->GetAttr("profiles", dashMpdInfo_.profile_);
    mpdNode->GetAttr("mediaType", dashMpdInfo_.mediaType_);
    mpdNode->GetAttr("hwDefaultViewIndex", dashMpdInfo_.hwDefaultViewIndex_);
    mpdNode->GetAttr("hwTotalViewNumber", dashMpdInfo_.hwTotalViewNumber_);

    std::string type;
    mpdNode->GetAttr("type", type);

    if (type == "dynamic") {
        dashMpdInfo_.type_ = DashType::DASH_TYPE_DYNAMIC;
    } else {
        dashMpdInfo_.type_ = DashType::DASH_TYPE_STATIC;
    }

    std::string time;
    mpdNode->GetAttr("mediaPresentationDuration", time);
    DashStrToDuration(time, dashMpdInfo_.mediaPresentationDuration_);

    mpdNode->GetAttr("minimumUpdatePeriod", time);
    DashStrToDuration(time, dashMpdInfo_.minimumUpdatePeriod_);

    mpdNode->GetAttr("minBufferTime", time);
    DashStrToDuration(time, dashMpdInfo_.minBufferTime_);

    mpdNode->GetAttr("timeShiftBufferDepth", time);
    DashStrToDuration(time, dashMpdInfo_.timeShiftBufferDepth_);

    mpdNode->GetAttr("suggestedPresentationDelay", time);
    DashStrToDuration(time, dashMpdInfo_.suggestedPresentationDelay_);

    mpdNode->GetAttr("maxSegmentDuration", time);
    DashStrToDuration(time, dashMpdInfo_.maxSegmentDuration_);

    mpdNode->GetAttr("maxSubsegmentDuration", time);
    DashStrToDuration(time, dashMpdInfo_.maxSubSegmentDuration_);

    std::string startTime;
    mpdNode->GetAttr("availabilityStartTime", startTime);
    dashMpdInfo_.availabilityStartTime_ = (int64_t)String2Time(startTime) * S_2_MS;
}

void DashMpdParser::GetMpdElement(std::shared_ptr<XmlParser> xmlParser, std::shared_ptr<XmlElement> rootElement)
{
    DashList<std::shared_ptr<XmlElement>> periodElementList;
    std::shared_ptr<XmlElement> childElement = rootElement->GetChild();
    while (childElement != nullptr) {
        if (this->stopFlag_) {
            break;
        }

        ProcessMpdElement(xmlParser, periodElementList, childElement);
        childElement = childElement->GetSiblingNext();
    }
    ParsePeriodElement(xmlParser, periodElementList);
}

void DashMpdParser::ProcessMpdElement(const std::shared_ptr<XmlParser> &xmlParser,
                                      DashList <std::shared_ptr<XmlElement>> &periodElementList,
                                      std::shared_ptr<XmlElement> &childElement)
{
    std::string elementNameStr = childElement->GetName();
    if (!elementNameStr.empty()) {
        xmlParser->SkipElementNameSpace(elementNameStr);

        if (elementNameStr == MPD_LABEL_BASE_URL) {
            std::string strValue = childElement->GetText();
            if (!strValue.empty()) {
                dashMpdInfo_.baseUrl_.push_back(strValue);
            }
        } else if (elementNameStr == MPD_LABEL_PERIOD) {
            periodElementList.push_back(childElement);
        }
    }
}

void DashMpdParser::ParsePeriodElement(std::shared_ptr<XmlParser> &xmlParser,
                                       DashList<std::shared_ptr<XmlElement>> &periodElementList)
{
    // parse Period label last
    while (periodElementList.size() > 0) {
        if (this->stopFlag_) {
            break;
        }

        std::shared_ptr<XmlElement> periodElement = periodElementList.front();
        if (periodElement != nullptr) {
            ParsePeriod(xmlParser, periodElement);
        }

        periodElementList.pop_front();
    }
}

void DashMpdParser::ParseMPD(const char *mpdData, uint32_t length)
{
    if (this->stopFlag_) {
        return;
    }

    std::shared_ptr<XmlParser> xmlParser = std::make_shared<XmlParser>();
    int32_t ret = xmlParser->ParseFromBuffer(mpdData, length);
    std::shared_ptr<XmlElement> rootElement = xmlParser->GetRootElement();
    if (ret != static_cast<int32_t>(XmlBaseRtnValue::XML_BASE_OK) || this->stopFlag_ || rootElement == nullptr) {
        MEDIA_LOG_E("Parse error or stop " PUBLIC_LOG_D32 ", ret=" PUBLIC_LOG_D32, this->stopFlag_, ret);
        return;
    }

    IDashMpdNode *mpdNode = IDashMpdNode::CreateNode(MPD_LABEL_MPD);
    if (mpdNode == nullptr) {
        return;
    }

    // parse attribute in MPD label
    mpdNode->ParseNode(xmlParser, rootElement);
    GetMpdAttr(mpdNode);

    // parse element in MPD label
    GetMpdElement(xmlParser, rootElement);

    IDashMpdNode::DestroyNode(mpdNode);
}

void DashMpdParser::GetMPD(DashMpdInfo *&mpdInfo)
{
    mpdInfo = &dashMpdInfo_;
}

void DashMpdParser::Clear()
{
    while (dashMpdInfo_.periodInfoList_.size() > 0) {
        DashPeriodInfo *periodInfo = dashMpdInfo_.periodInfoList_.front();
        if (periodInfo != nullptr) {
            ClearAdaptationSet(periodInfo->adptSetList_);
            FreeSegmentBase(periodInfo->periodSegBase_);
            FreeSegmentList(periodInfo->periodSegList_);
            FreeSegmentTemplate(periodInfo->periodSegTmplt_);
            delete periodInfo;
        }
        dashMpdInfo_.periodInfoList_.pop_front();
    }

    while (dashMpdInfo_.baseUrl_.size() > 0) {
        dashMpdInfo_.baseUrl_.pop_front();
    }

    dashMpdInfo_.type_ = DashType::DASH_TYPE_STATIC;
    dashMpdInfo_.mediaPresentationDuration_ = 0;
    dashMpdInfo_.minimumUpdatePeriod_ = 0;
    dashMpdInfo_.minBufferTime_ = 0;
    dashMpdInfo_.timeShiftBufferDepth_ = 0;
    dashMpdInfo_.suggestedPresentationDelay_ = 0;
    dashMpdInfo_.maxSegmentDuration_ = 0;
    dashMpdInfo_.maxSubSegmentDuration_ = 0;
    dashMpdInfo_.availabilityStartTime_ = 0;
    dashMpdInfo_.availabilityEndTime_ = 0;
    dashMpdInfo_.hwDefaultViewIndex_ = 0;
    dashMpdInfo_.hwTotalViewNumber_ = 0;
    dashMpdInfo_.profile_ = "";
    dashMpdInfo_.mediaType_ = "";
    stopFlag_ = 0;
}

void DashMpdParser::StopParseMpd()
{
    this->stopFlag_ = 1;
}

void DashMpdParser::FreeSegmentBase(DashSegBaseInfo *segBaseInfo) const
{
    if (segBaseInfo != nullptr) {
        ClearSegmentBase(segBaseInfo);

        delete segBaseInfo;
    }
}

void DashMpdParser::FreeSegmentList(DashSegListInfo *segListInfo)
{
    if (segListInfo != nullptr) {
        ClearMultSegBase(&segListInfo->multSegBaseInfo_);

        while (segListInfo->segmentUrl_.size() > 0) {
            DashSegUrl *segUrl = segListInfo->segmentUrl_.front();
            if (segUrl != nullptr) {
                delete segUrl;
            }

            segListInfo->segmentUrl_.pop_front();
        }

        delete segListInfo;
    }
}

void DashMpdParser::FreeSegmentTemplate(DashSegTmpltInfo *segTmpltInfo)
{
    if (segTmpltInfo != nullptr) {
        ClearMultSegBase(&segTmpltInfo->multSegBaseInfo_);

        delete segTmpltInfo;
    }
}

void DashMpdParser::ClearAdaptationSet(DashList<DashAdptSetInfo *> &adptSetInfoList)
{
    while (adptSetInfoList.size() > 0) {
        DashAdptSetInfo *adptSetInfo = adptSetInfoList.front();
        if (adptSetInfo != nullptr) {
            ClearContentComp(adptSetInfo->contentCompList_);
            ClearRepresentation(adptSetInfo->representationList_);
            ClearCommonAttrsAndElements(&adptSetInfo->commonAttrsAndElements_);
            ClearRoleList(adptSetInfo->roleList_);

            FreeSegmentBase(adptSetInfo->adptSetSegBase_);
            FreeSegmentList(adptSetInfo->adptSetSegList_);
            FreeSegmentTemplate(adptSetInfo->adptSetSegTmplt_);

            delete adptSetInfo;
        }

        adptSetInfoList.pop_front();
    }
}

void DashMpdParser::ClearContentComp(DashList<DashContentCompInfo *> &contentCompList)
{
    while (contentCompList.size() > 0) {
        DashContentCompInfo *contentCompInfo = contentCompList.front();
        if (contentCompInfo != nullptr) {
            ClearRoleList(contentCompInfo->roleList_);

            delete contentCompInfo;
        }

        contentCompList.pop_front();
    }
}

void DashMpdParser::ClearRepresentation(DashList<DashRepresentationInfo *> &representationList)
{
    while (representationList.size() > 0) {
        DashRepresentationInfo *representationInfo = representationList.front();
        if (representationInfo != nullptr) {
            ClearCommonAttrsAndElements(&representationInfo->commonAttrsAndElements_);

            FreeSegmentBase(representationInfo->representationSegBase_);
            FreeSegmentList(representationInfo->representationSegList_);
            FreeSegmentTemplate(representationInfo->representationSegTmplt_);

            delete representationInfo;
        }

        representationList.pop_front();
    }
}

void DashMpdParser::ClearSegmentBase(DashSegBaseInfo *segBaseInfo) const
{
    if (segBaseInfo != nullptr) {
        if (segBaseInfo->initialization_ != nullptr) {
            delete segBaseInfo->initialization_;
            segBaseInfo->initialization_ = nullptr;
        }

        if (segBaseInfo->representationIndex_ != nullptr) {
            delete segBaseInfo->representationIndex_;
            segBaseInfo->representationIndex_ = nullptr;
        }
    }
}

void DashMpdParser::ClearMultSegBase(DashMultSegBaseInfo *multSegBaseInfo)
{
    if (multSegBaseInfo != nullptr) {
        if (multSegBaseInfo->bitstreamSwitching_ != nullptr) {
            delete multSegBaseInfo->bitstreamSwitching_;
            multSegBaseInfo->bitstreamSwitching_ = nullptr;
        }

        while (multSegBaseInfo->segTimeline_.size() > 0) {
            DashSegTimeline *segTimeline = multSegBaseInfo->segTimeline_.front();
            if (segTimeline != nullptr) {
                delete segTimeline;
            }

            multSegBaseInfo->segTimeline_.pop_front();
        }

        ClearSegmentBase(&multSegBaseInfo->segBaseInfo_);
    }
}

static void ClearPropertyList(DashList<DashDescriptor *> &propertyList)
{
    while (propertyList.size() > 0) {
        DashDescriptor *property = propertyList.front();
        if (property != nullptr) {
            delete property;
        }
        propertyList.pop_front();
    }
}

void DashMpdParser::ClearCommonAttrsAndElements(DashCommonAttrsAndElements *commonAttrsAndElements)
{
    if (commonAttrsAndElements == nullptr) {
        return;
    }
    ClearPropertyList(commonAttrsAndElements->audioChannelConfigurationList_);
    ClearPropertyList(commonAttrsAndElements->contentProtectionList_);
    ClearPropertyList(commonAttrsAndElements->essentialPropertyList_);
}

void DashMpdParser::ClearRoleList(DashList<DashDescriptor *> &roleList)
{
    while (roleList.size() > 0) {
        DashDescriptor *role = roleList.front();
        if (role != nullptr) {
            delete role;
        }

        roleList.pop_front();
    }
}
} // namespace HttpPluginLite
} // namespace Plugin
} // namespace Media
} // namespace OHOS