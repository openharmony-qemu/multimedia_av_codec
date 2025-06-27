/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "dash_mpd_parser_unittest.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
using namespace std;
using namespace testing;
using namespace testing::ext;
static const int32_t NUM_0 = 0;
static const int32_t NUM_1 = 1;

void DashMpdParserUnitTest::SetUpTestCase(void) {}

void DashMpdParserUnitTest::TearDownTestCase(void) {}

void DashMpdParserUnitTest::SetUp(void)
{
    dashMpdParser_ = std::make_shared<DashMpdParser>();
}

void DashMpdParserUnitTest::TearDown(void)
{
    dashMpdParser_ = nullptr;
}

/**
 * @tc.name  : Test ParsePeriod
 * @tc.number: ParsePeriod_001
 * @tc.desc  : Test periodNode == nullptr
 */
HWTEST_F(DashMpdParserUnitTest, ParsePeriod_001, TestSize.Level0)
{
    ASSERT_NE(dashMpdParser_, nullptr);
    dashMpdParser_->ParsePeriod(nullptr, nullptr);
    EXPECT_EQ(dashMpdParser_->dashMpdInfo_.periodInfoList_.size(), NUM_0);
}

/**
 * @tc.name  : Test GetPeriodAttr
 * @tc.number: GetPeriodAttr_001
 * @tc.desc  : Test tempStr == "true"
 */
HWTEST_F(DashMpdParserUnitTest, GetPeriodAttr_001, TestSize.Level0)
{
    ASSERT_NE(dashMpdParser_, nullptr);
    auto impl = std::make_shared<DashMpdNodeImpl>();
    auto info = std::make_shared<DashPeriodInfo>();
    dashMpdParser_->GetPeriodAttr(impl.get(), info.get());
    EXPECT_TRUE(info->bitstreamSwitching_);
}

/**
 * @tc.name  : Test GetPeriodElement
 * @tc.number: GetPeriodElement_001
 * @tc.desc  : Test childElement != nullptr && this->stopFlag_ == true
 */
HWTEST_F(DashMpdParserUnitTest, GetPeriodElement_001, TestSize.Level0)
{
    ASSERT_NE(dashMpdParser_, nullptr);
    auto xmlParser = std::make_shared<XmlParser>();
    auto rootElement = std::make_shared<XmlElement>();
    auto periodInfo = std::make_shared<DashPeriodInfo>();
    dashMpdParser_->stopFlag_ = NUM_1;
    rootElement->name_ = DashMpdParser::MPD_LABEL_SEGMENT_LIST;
    auto copy = periodInfo->periodSegList_;
    dashMpdParser_->GetPeriodElement(xmlParser, rootElement, periodInfo.get());
    EXPECT_EQ(periodInfo->periodSegList_, copy);
}

/**
 * @tc.name  : Test ParseAdaptationSet
 * @tc.number: ParseAdaptationSet_001
 * @tc.desc  : Test adptSetNode == nullptr
 */
HWTEST_F(DashMpdParserUnitTest, ParseAdaptationSet_001, TestSize.Level0)
{
    ASSERT_NE(dashMpdParser_, nullptr);
    auto xmlParser = std::make_shared<XmlParser>();
    auto rootElement = std::make_shared<XmlElement>();
    auto periodInfo = std::make_shared<DashPeriodInfo>();
    dashMpdParser_->ParseAdaptationSet(xmlParser, rootElement, periodInfo.get());
    EXPECT_EQ(periodInfo->adptSetList_.size(), NUM_0);
}

/**
 * @tc.name  : Test GetAdaptationSetAttr
 * @tc.number: GetAdaptationSetAttr_001
 * @tc.desc  : Test str == "true"
 *             Test str == "interlaced"
 *             Test str == "unknown"
 */
HWTEST_F(DashMpdParserUnitTest, GetAdaptationSetAttr_001, TestSize.Level0)
{
    ASSERT_NE(dashMpdParser_, nullptr);
    auto impl = std::make_shared<DashMpdNodeImpl>();
    auto adptSetInfo = std::make_shared<DashAdptSetInfo>();
    // Test str == "true"
    // Test str == "interlaced"
    dashMpdParser_->GetAdaptationSetAttr(impl.get(), adptSetInfo.get());
    EXPECT_EQ(adptSetInfo->commonAttrsAndElements_.codingDependency_, NUM_1);
    EXPECT_EQ(adptSetInfo->commonAttrsAndElements_.scanType_, VideoScanType::VIDEO_SCAN_INTERLACED);
    EXPECT_EQ(adptSetInfo->bitstreamSwitching_, true);
    // Test str == "unknown"
    auto impl2 = std::make_shared<DashMpdNodeImpl2>();
    dashMpdParser_->GetAdaptationSetAttr(impl2.get(), adptSetInfo.get());
    EXPECT_EQ(adptSetInfo->commonAttrsAndElements_.scanType_, VideoScanType::VIDEO_SCAN_UNKNOW);
}

/**
 * @tc.name  : Test GetAdaptationSetElement
 * @tc.number: GetAdaptationSetElement_001
 * @tc.desc  : Test childElement != nullptr && this->stopFlag_ == false
 */
HWTEST_F(DashMpdParserUnitTest, GetAdaptationSetElement_001, TestSize.Level0)
{
    ASSERT_NE(dashMpdParser_, nullptr);
    auto xmlParser = std::make_shared<XmlParser>();
    auto rootElement = std::make_shared<XmlElement>();
    auto periodInfo = std::make_shared<DashPeriodInfo>();
    auto adptSetInfo = std::make_shared<DashAdptSetInfo>();
    rootElement->name_ = DashMpdParser::MPD_LABEL_SEGMENT_LIST;
    auto copy = adptSetInfo.get();
    dashMpdParser_->GetAdaptationSetElement(xmlParser, rootElement, periodInfo.get(), adptSetInfo.get());
    EXPECT_EQ(adptSetInfo.get(), copy);
}

/**
 * @tc.name  : Test GetAdaptationSetElement
 * @tc.number: GetAdaptationSetElement_002
 * @tc.desc  : Test dashElementList.segBaseElement_ != nullptr
 */
HWTEST_F(DashMpdParserUnitTest, GetAdaptationSetElement_002, TestSize.Level0)
{
    ASSERT_NE(dashMpdParser_, nullptr);
    auto xmlParser = std::make_shared<XmlParser>();
    auto rootElement = std::make_shared<XmlElement>();
    auto periodInfo = std::make_shared<DashPeriodInfo>();
    auto adptSetInfo = std::make_shared<DashAdptSetInfo>();
    rootElement->name_ = DashMpdParser::MPD_LABEL_SEGMENT_BASE;
    auto copy = adptSetInfo.get();
    dashMpdParser_->GetAdaptationSetElement(xmlParser, rootElement, periodInfo.get(), adptSetInfo.get());
    EXPECT_EQ(adptSetInfo.get(), copy);
}

/**
 * @tc.name  : Test GetAdaptationSetElement
 * @tc.number: GetAdaptationSetElement_003
 * @tc.desc  : Test adptSetInfo->adptSetSegBase_ != nullptr && periodInfo->periodSegBase_ != nullptr
 */
HWTEST_F(DashMpdParserUnitTest, GetAdaptationSetElement_003, TestSize.Level0)
{
    ASSERT_NE(dashMpdParser_, nullptr);
    auto xmlParser = std::make_shared<XmlParser>();
    auto rootElement = std::make_shared<XmlElement>();
    auto periodInfo = std::make_shared<DashPeriodInfo>();
    auto adptSetInfo = std::make_shared<DashAdptSetInfo>();
    dashMpdParser_->stopFlag_ = NUM_1;
    adptSetInfo->adptSetSegBase_ = new DashSegBaseInfo();
    adptSetInfo->adptSetSegBase_->timeScale_ = NUM_0;
    periodInfo->periodSegBase_ = new DashSegBaseInfo();
    periodInfo->periodSegBase_->timeScale_ = NUM_1;
    dashMpdParser_->GetAdaptationSetElement(xmlParser, rootElement, periodInfo.get(), adptSetInfo.get());
    EXPECT_EQ(adptSetInfo->adptSetSegBase_->timeScale_, periodInfo->periodSegBase_->timeScale_);
    delete adptSetInfo->adptSetSegBase_;
    delete periodInfo->periodSegBase_;
}

/**
 * @tc.name  : Test ProcessAdpSetElement
 * @tc.number: ProcessAdpSetElement_001
 * @tc.desc  : Test elementNameStr == MPD_LABEL_SEGMENT_BASE
 */
HWTEST_F(DashMpdParserUnitTest, ProcessAdpSetElement_001, TestSize.Level0)
{
    ASSERT_NE(dashMpdParser_, nullptr);
    auto xmlParser = std::make_shared<XmlParser>();
    auto adptSetInfo = std::make_shared<DashAdptSetInfo>();
    DashList<std::shared_ptr<XmlElement>> representationElementList;
    auto childElement = std::make_shared<XmlElement>();
    childElement->name_ = DashMpdParser::MPD_LABEL_SEGMENT_BASE;
    DashElementList dashElementList;
    dashMpdParser_->ProcessAdpSetElement(xmlParser, adptSetInfo.get(), representationElementList, childElement,
        dashElementList);
    EXPECT_EQ(dashElementList.segBaseElement_, childElement);
}

/**
 * @tc.name  : Test ParseSegmentUrl
 * @tc.number: ParseSegmentUrl_001
 * @tc.desc  : Test segUrlNode == nullptr
 */
HWTEST_F(DashMpdParserUnitTest, ParseSegmentUrl_001, TestSize.Level0)
{
    ASSERT_NE(dashMpdParser_, nullptr);
    DashSegUrl *segUrl = new (std::nothrow) DashSegUrl;
    ASSERT_NE(segUrl, nullptr);
    auto xmlParser = std::make_shared<XmlParser>();
    auto rootElement = std::make_shared<XmlElement>();
    DashList<DashSegUrl *> segUrlList;
    dashMpdParser_->ParseSegmentUrl(xmlParser, rootElement, segUrlList);
    EXPECT_EQ(segUrlList.size(), NUM_0);
    delete segUrl;
}

/**
 * @tc.name  : Test ParseContentComponent
 * @tc.number: ParseContentComponent_001
 * @tc.desc  : Test contentCompNode == nullptr
 */
HWTEST_F(DashMpdParserUnitTest, ParseContentComponent_001, TestSize.Level0)
{
    ASSERT_NE(dashMpdParser_, nullptr);
    DashContentCompInfo *contentCompInfo = new (std::nothrow) DashContentCompInfo;
    ASSERT_NE(contentCompInfo, nullptr);
    auto xmlParser = std::make_shared<XmlParser>();
    auto rootElement = std::make_shared<XmlElement>();
    DashList<DashContentCompInfo *> contentCompInfoList;
    dashMpdParser_->ParseContentComponent(xmlParser, rootElement, contentCompInfoList);
    EXPECT_EQ(contentCompInfoList.size(), NUM_0);
    delete contentCompInfo;
}

/**
 * @tc.name  : Test ParseSegmentBase
 * @tc.number: ParseSegmentBase_001
 * @tc.desc  : Test segBaseInfo == nullptr
 *             Test segBaseNode == nullptr
 */
HWTEST_F(DashMpdParserUnitTest, ParseSegmentBase_001, TestSize.Level0)
{
    ASSERT_NE(dashMpdParser_, nullptr);
    auto xmlParser = std::make_shared<XmlParser>();
    auto rootElement = std::make_shared<XmlElement>();
    DashSegBaseInfo *segBaseInfo = nullptr;
    // Test segBaseInfo == nullptr
    dashMpdParser_->ParseSegmentBase(xmlParser, rootElement, nullptr);
    // Test segBaseNode == nullptr
    dashMpdParser_->ParseSegmentBase(xmlParser, rootElement, &segBaseInfo);
    EXPECT_EQ(segBaseInfo, nullptr);
}

/**
 * @tc.name  : Test ParseElementUrlType
 * @tc.number: ParseElementUrlType_001
 * @tc.desc  : Test elementNameStr == MPD_LABEL_REPRESENTATION_INDEX
 */
HWTEST_F(DashMpdParserUnitTest, ParseElementUrlType_001, TestSize.Level0)
{
    ASSERT_NE(dashMpdParser_, nullptr);
    auto xmlParser = std::make_shared<XmlParser>();
    auto segBase = std::make_shared<DashSegBaseInfo>();
    auto childElement = std::make_shared<XmlElement>();
    childElement->name_ = DashMpdParser::MPD_LABEL_REPRESENTATION_INDEX;
    dashMpdParser_->ParseElementUrlType(xmlParser, segBase.get(), childElement);
    EXPECT_NE(segBase->representationIndex_, nullptr);
}

/**
 * @tc.name  : Test ParseSegmentList
 * @tc.number: ParseSegmentList_001
 * @tc.desc  : Test segListInfo == nullptr
 *             Test segListNode == nullptr
 */
HWTEST_F(DashMpdParserUnitTest, ParseSegmentList_001, TestSize.Level0)
{
    ASSERT_NE(dashMpdParser_, nullptr);
    auto xmlParser = std::make_shared<XmlParser>();
    auto rootElement = std::make_shared<XmlElement>();
    DashSegListInfo *segListInfo = nullptr;
    // Test segListInfo == nullptr
    dashMpdParser_->ParseSegmentList(xmlParser, rootElement, nullptr);
    // Test segListNode == nullptr
    rootElement->name_ = DashMpdParser::MPD_LABEL_REPRESENTATION_INDEX;
    dashMpdParser_->ParseSegmentList(xmlParser, rootElement, &segListInfo);
    EXPECT_EQ(segListInfo, nullptr);
}

/**
 * @tc.name  : Test ParseSegmentListElement
 * @tc.number: ParseSegmentListElement_001
 * @tc.desc  : Test elementNameStr == MPD_LABEL_BITSTREAM_SWITCHING
 *             Test elementNameStr == MPD_LABEL_REPRESENTATION_INDEX
 */
HWTEST_F(DashMpdParserUnitTest, ParseSegmentListElement_001, TestSize.Level0)
{
    ASSERT_NE(dashMpdParser_, nullptr);
    auto xmlParser = std::make_shared<XmlParser>();
    auto segList = std::make_shared<DashSegListInfo>();
    auto childElement = std::make_shared<XmlElement>();
    // Test elementNameStr == MPD_LABEL_BITSTREAM_SWITCHING
    childElement->name_ = DashMpdParser::MPD_LABEL_BITSTREAM_SWITCHING;
    dashMpdParser_->ParseSegmentListElement(xmlParser, segList.get(), childElement);
    EXPECT_NE(segList->multSegBaseInfo_.bitstreamSwitching_, nullptr);
    // Test elementNameStr == MPD_LABEL_REPRESENTATION_INDEX
    childElement->name_ = DashMpdParser::MPD_LABEL_REPRESENTATION_INDEX;
    dashMpdParser_->ParseSegmentListElement(xmlParser, segList.get(), childElement);
    EXPECT_NE(segList->multSegBaseInfo_.segBaseInfo_.representationIndex_, nullptr);
}

/**
 * @tc.name  : Test InheritMultSegBase
 * @tc.number: InheritMultSegBase_001
 * @tc.desc  : Test lowerMultSegBaseInfo->startNumber_.length() == 0
 *                  && higherMultSegBaseInfo->startNumber_.length() > 0
 */
HWTEST_F(DashMpdParserUnitTest, InheritMultSegBase_001, TestSize.Level0)
{
    ASSERT_NE(dashMpdParser_, nullptr);
    auto lowerMultSegBaseInfo = std::make_shared<DashMultSegBaseInfo>();
    auto higherMultSegBaseInfo = std::make_shared<DashMultSegBaseInfo>();
    lowerMultSegBaseInfo->startNumber_ = "";
    higherMultSegBaseInfo->startNumber_ = "1";
    dashMpdParser_->InheritMultSegBase(lowerMultSegBaseInfo.get(), higherMultSegBaseInfo.get());
    EXPECT_EQ(lowerMultSegBaseInfo->startNumber_, higherMultSegBaseInfo->startNumber_);
}

/**
 * @tc.name  : Test ParseSegmentTemplate
 * @tc.number: ParseSegmentTemplate_001
 * @tc.desc  : Test segTmpltInfo == nullptr
 *             Test this->stopFlag_ == true
 */
HWTEST_F(DashMpdParserUnitTest, ParseSegmentTemplate_001, TestSize.Level0)
{
    ASSERT_NE(dashMpdParser_, nullptr);
    auto xmlParser = std::make_shared<XmlParser>();
    auto rootElement = std::make_shared<XmlElement>();
    DashSegTmpltInfo *segTmpltInfo = nullptr;
    // Test segTmpltInfo == nullptr
    dashMpdParser_->ParseSegmentTemplate(xmlParser, rootElement, nullptr);
    // Test this->stopFlag_ == true
    dashMpdParser_->ParseSegmentTemplate(xmlParser, rootElement, &segTmpltInfo);
    EXPECT_NE(segTmpltInfo, nullptr);
}

/**
 * @tc.name  : Test ParseElement
 * @tc.number: ParseElement_001
 * @tc.desc  : Test elementNameStr == MPD_LABEL_BITSTREAM_SWITCHING
 *             Test elementNameStr == MPD_LABEL_INITIALIZATION
 *             Test elementNameStr == MPD_LABEL_REPRESENTATION_INDEX
 */
HWTEST_F(DashMpdParserUnitTest, ParseElement_001, TestSize.Level0)
{
    ASSERT_NE(dashMpdParser_, nullptr);
    auto xmlParser = std::make_shared<XmlParser>();
    auto segTmpltInfo = std::make_shared<DashSegTmpltInfo>();
    auto childElement = std::make_shared<XmlElement>();

    // Test elementNameStr == MPD_LABEL_BITSTREAM_SWITCHING
    childElement->name_ = DashMpdParser::MPD_LABEL_BITSTREAM_SWITCHING;
    segTmpltInfo->multSegBaseInfo_.bitstreamSwitching_ = nullptr;
    dashMpdParser_->ParseElement(xmlParser, segTmpltInfo.get(), childElement);
    EXPECT_NE(segTmpltInfo->multSegBaseInfo_.bitstreamSwitching_, nullptr);

    // Test elementNameStr == MPD_LABEL_INITIALIZATION
    childElement->name_ = DashMpdParser::MPD_LABEL_INITIALIZATION;
    segTmpltInfo->multSegBaseInfo_.segBaseInfo_.initialization_ = nullptr;
    dashMpdParser_->ParseElement(xmlParser, segTmpltInfo.get(), childElement);
    EXPECT_NE(segTmpltInfo->multSegBaseInfo_.segBaseInfo_.initialization_, nullptr);

    // Test elementNameStr == MPD_LABEL_REPRESENTATION_INDEX
    childElement->name_ = DashMpdParser::MPD_LABEL_REPRESENTATION_INDEX;
    segTmpltInfo->multSegBaseInfo_.segBaseInfo_.representationIndex_ = nullptr;
    dashMpdParser_->ParseElement(xmlParser, segTmpltInfo.get(), childElement);
    EXPECT_NE(segTmpltInfo->multSegBaseInfo_.segBaseInfo_.representationIndex_, nullptr);
}

/**
 * @tc.name  : Test ParseRepresentation
 * @tc.number: ParseRepresentation_001
 * @tc.desc  : Test representationNode == nullptr
 */
HWTEST_F(DashMpdParserUnitTest, ParseRepresentation_001, TestSize.Level0)
{
    ASSERT_NE(dashMpdParser_, nullptr);
    auto xmlParser = std::make_shared<XmlParser>();
    auto rootElement = std::make_shared<XmlElement>();
    auto periodInfo = std::make_shared<DashPeriodInfo>();
    auto adptSetInfo = std::make_shared<DashAdptSetInfo>();
    dashMpdParser_->ParseRepresentation(xmlParser, rootElement, periodInfo.get(), adptSetInfo.get());
    EXPECT_TRUE(adptSetInfo->representationList_.empty());
}

/**
 * @tc.name  : Test GetRepresentationElement
 * @tc.number: GetRepresentationElement_001
 * @tc.desc  : Test this->stopFlag_ == true
 *             Test dashElementList.segTmplElement_ != nullptr
 */
HWTEST_F(DashMpdParserUnitTest, GetRepresentationElement_001, TestSize.Level0)
{
    ASSERT_NE(dashMpdParser_, nullptr);
    auto xmlParser = std::make_shared<XmlParser>();
    auto rootElement = std::make_shared<XmlElement>();
    auto periodInfo = std::make_shared<DashPeriodInfo>();
    auto adptSetInfo = std::make_shared<DashAdptSetInfo>();
    auto representationInfo = std::make_shared<DashRepresentationInfo>();

    // Test this->stopFlag_ == true
    dashMpdParser_->stopFlag_ = NUM_1;
    dashMpdParser_->GetRepresentationElement(xmlParser, rootElement, periodInfo.get(), adptSetInfo.get(),
        representationInfo.get());

    // Test dashElementList.segTmplElement_ != nullptr
    dashMpdParser_->stopFlag_ = NUM_0;
    rootElement->name_ = DashMpdParser::MPD_LABEL_SEGMENT_TEMPLATE;
    dashMpdParser_->GetRepresentationElement(xmlParser, rootElement, periodInfo.get(), adptSetInfo.get(),
        representationInfo.get());
    EXPECT_NE(representationInfo->representationSegTmplt_, nullptr);
}

/**
 * @tc.name  : Test GetRepresentationElement
 * @tc.number: GetRepresentationElement_002
 * @tc.desc  : Test representationInfo->representationSegTmplt_ != nullptr && adptSetInfo->adptSetSegTmplt_ != nullptr
 */
HWTEST_F(DashMpdParserUnitTest, GetRepresentationElement_002, TestSize.Level0)
{
    ASSERT_NE(dashMpdParser_, nullptr);
    auto xmlParser = std::make_shared<XmlParser>();
    auto rootElement = std::make_shared<XmlElement>();
    auto periodInfo = std::make_shared<DashPeriodInfo>();
    auto adptSetInfo = std::make_shared<DashAdptSetInfo>();
    auto representationInfo = std::make_shared<DashRepresentationInfo>();
    dashMpdParser_->stopFlag_ = NUM_1;

    representationInfo->representationSegTmplt_ = new DashSegTmpltInfo();
    adptSetInfo->adptSetSegTmplt_ = new DashSegTmpltInfo();
    adptSetInfo->adptSetSegTmplt_->multSegBaseInfo_.duration_ = NUM_1;
    dashMpdParser_->GetRepresentationElement(xmlParser, rootElement, periodInfo.get(), adptSetInfo.get(),
        representationInfo.get());
    EXPECT_EQ(representationInfo->representationSegTmplt_->multSegBaseInfo_.duration_,
        adptSetInfo->adptSetSegTmplt_->multSegBaseInfo_.duration_);
    delete representationInfo->representationSegTmplt_;
    delete adptSetInfo->adptSetSegTmplt_;
}

/**
 * @tc.name  : Test GetRepresentationElement
 * @tc.number: GetRepresentationElement_003
 * @tc.desc  : Test representationInfo->representationSegTmplt_ != nullptr && periodInfo->periodSegTmplt_ != nullptr
 */
HWTEST_F(DashMpdParserUnitTest, GetRepresentationElement_003, TestSize.Level0)
{
    ASSERT_NE(dashMpdParser_, nullptr);
    auto xmlParser = std::make_shared<XmlParser>();
    auto rootElement = std::make_shared<XmlElement>();
    auto periodInfo = std::make_shared<DashPeriodInfo>();
    auto adptSetInfo = std::make_shared<DashAdptSetInfo>();
    auto representationInfo = std::make_shared<DashRepresentationInfo>();
    dashMpdParser_->stopFlag_ = NUM_1;

    representationInfo->representationSegTmplt_ = new DashSegTmpltInfo();
    periodInfo->periodSegTmplt_ = new DashSegTmpltInfo();
    periodInfo->periodSegTmplt_->multSegBaseInfo_.duration_ = NUM_1;
    dashMpdParser_->GetRepresentationElement(xmlParser, rootElement, periodInfo.get(), adptSetInfo.get(),
        representationInfo.get());
    EXPECT_EQ(representationInfo->representationSegTmplt_->multSegBaseInfo_.duration_,
        periodInfo->periodSegTmplt_->multSegBaseInfo_.duration_);
    delete representationInfo->representationSegTmplt_;
    delete periodInfo->periodSegTmplt_;
}

/**
 * @tc.name  : Test GetRepresentationElement
 * @tc.number: GetRepresentationElement_004
 * @tc.desc  : Test representationInfo->representationSegList_ != nullptr && adptSetInfo->adptSetSegList_ != nullptr
 */
HWTEST_F(DashMpdParserUnitTest, GetRepresentationElement_004, TestSize.Level0)
{
    ASSERT_NE(dashMpdParser_, nullptr);
    auto xmlParser = std::make_shared<XmlParser>();
    auto rootElement = std::make_shared<XmlElement>();
    auto periodInfo = std::make_shared<DashPeriodInfo>();
    auto adptSetInfo = std::make_shared<DashAdptSetInfo>();
    auto representationInfo = std::make_shared<DashRepresentationInfo>();
    dashMpdParser_->stopFlag_ = NUM_1;

    representationInfo->representationSegList_ = new DashSegListInfo();
    adptSetInfo->adptSetSegList_ = new DashSegListInfo();
    adptSetInfo->adptSetSegList_->multSegBaseInfo_.duration_ = NUM_1;
    dashMpdParser_->GetRepresentationElement(xmlParser, rootElement, periodInfo.get(), adptSetInfo.get(),
        representationInfo.get());
    EXPECT_EQ(representationInfo->representationSegList_->multSegBaseInfo_.duration_,
        adptSetInfo->adptSetSegList_->multSegBaseInfo_.duration_);
    delete representationInfo->representationSegList_;
    delete adptSetInfo->adptSetSegList_;
}

/**
 * @tc.name  : Test GetRepresentationElement
 * @tc.number: GetRepresentationElement_005
 * @tc.desc  : Test representationInfo->representationSegBase_ != nullptr && adptSetInfo->adptSetSegBase_ != nullptr
 */
HWTEST_F(DashMpdParserUnitTest, GetRepresentationElement_005, TestSize.Level0)
{
    ASSERT_NE(dashMpdParser_, nullptr);
    auto xmlParser = std::make_shared<XmlParser>();
    auto rootElement = std::make_shared<XmlElement>();
    auto periodInfo = std::make_shared<DashPeriodInfo>();
    auto adptSetInfo = std::make_shared<DashAdptSetInfo>();
    auto representationInfo = std::make_shared<DashRepresentationInfo>();
    dashMpdParser_->stopFlag_ = NUM_1;

    representationInfo->representationSegBase_ = new DashSegBaseInfo();
    adptSetInfo->adptSetSegBase_ = new DashSegBaseInfo();
    adptSetInfo->adptSetSegBase_->timeScale_ = NUM_1;
    dashMpdParser_->GetRepresentationElement(xmlParser, rootElement, periodInfo.get(), adptSetInfo.get(),
        representationInfo.get());
    EXPECT_EQ(representationInfo->representationSegBase_->timeScale_,
        adptSetInfo->adptSetSegBase_->timeScale_);
    delete representationInfo->representationSegBase_;
    delete adptSetInfo->adptSetSegBase_;
}

/**
 * @tc.name  : Test ProcessRepresentationElement
 * @tc.number: ProcessRepresentationElement_001
 * @tc.desc  : Test elementNameStr == MPD_LABEL_CONTENT_PROTECTIO
 *             Test elementNameStr == MPD_LABEL_ESSENTIAL_PROPERTY
 *             Test elementNameStr == MPD_LABEL_SEGMENT_TEMPLATE
 */
HWTEST_F(DashMpdParserUnitTest, ProcessRepresentationElement_001, TestSize.Level0)
{
    ASSERT_NE(dashMpdParser_, nullptr);
    auto xmlParser = std::make_shared<XmlParser>();
    auto representationInfo = std::make_shared<DashRepresentationInfo>();
    auto childElement = std::make_shared<XmlElement>();
    DashElementList dashElementList;

    // Test elementNameStr == MPD_LABEL_CONTENT_PROTECTION
    childElement->name_ = DashMpdParser::MPD_LABEL_CONTENT_PROTECTION;
    dashMpdParser_->ProcessRepresentationElement(xmlParser, representationInfo.get(), childElement, dashElementList);

    // Test elementNameStr == MPD_LABEL_ESSENTIAL_PROPERTY
    childElement->name_ = DashMpdParser::MPD_LABEL_ESSENTIAL_PROPERTY;
    dashMpdParser_->ProcessRepresentationElement(xmlParser, representationInfo.get(), childElement, dashElementList);
    
    // Test elementNameStr == MPD_LABEL_SEGMENT_TEMPLATE
    childElement->name_ = DashMpdParser::MPD_LABEL_SEGMENT_TEMPLATE;
    dashMpdParser_->ProcessRepresentationElement(xmlParser, representationInfo.get(), childElement, dashElementList);
    EXPECT_NE(dashElementList.segTmplElement_, nullptr);
}

/**
 * @tc.name  : Test ParseContentProtection
 * @tc.number: ParseContentProtection_001
 * @tc.desc  : Test contentProtectionNode == nullptr
 */
HWTEST_F(DashMpdParserUnitTest, ParseContentProtection_001, TestSize.Level0)
{
    ASSERT_NE(dashMpdParser_, nullptr);
    auto xmlParser = std::make_shared<XmlParser>();
    auto rootElement = std::make_shared<XmlElement>();
    DashList<DashDescriptor *> contentProtectionList;
    dashMpdParser_->ParseContentProtection(xmlParser, rootElement, contentProtectionList);
    EXPECT_TRUE(contentProtectionList.empty());
}

/**
 * @tc.name  : Test ParseEssentialProperty
 * @tc.number: ParseEssentialProperty_001
 * @tc.desc  : Test essentialPropertyNode == nullptr
 */
HWTEST_F(DashMpdParserUnitTest, ParseEssentialProperty_001, TestSize.Level0)
{
    ASSERT_NE(dashMpdParser_, nullptr);
    auto xmlParser = std::make_shared<XmlParser>();
    auto rootElement = std::make_shared<XmlElement>();
    DashList<DashDescriptor *> essentialPropertyList;
    dashMpdParser_->ParseContentProtection(xmlParser, rootElement, essentialPropertyList);
    EXPECT_TRUE(essentialPropertyList.empty());
}

/**
 * @tc.name  : Test ParseAudioChannelConfiguration
 * @tc.number: ParseAudioChannelConfiguration_001
 * @tc.desc  : Test node == nullptr
 */
HWTEST_F(DashMpdParserUnitTest, ParseAudioChannelConfiguration_001, TestSize.Level0)
{
    ASSERT_NE(dashMpdParser_, nullptr);
    auto xmlParser = std::make_shared<XmlParser>();
    auto rootElement = std::make_shared<XmlElement>();
    DashList<DashDescriptor *> propertyList;
    dashMpdParser_->ParseAudioChannelConfiguration(xmlParser, rootElement, propertyList);
    EXPECT_TRUE(propertyList.empty());
}

/**
 * @tc.name  : Test ParseSegmentTimeline
 * @tc.number: ParseSegmentTimeline_001
 * @tc.desc  : Test this->stopFlag_ == true
 *             Test node == nullptr
 */
HWTEST_F(DashMpdParserUnitTest, ParseSegmentTimeline_001, TestSize.Level0)
{
    ASSERT_NE(dashMpdParser_, nullptr);
    auto xmlParser = std::make_shared<XmlParser>();
    auto rootElement = std::make_shared<XmlElement>();
    DashList<DashSegTimeline *> segTmlineList;
    rootElement->name_ = DashMpdParser::MPD_LABEL_SEGMENT_TIMELINE_S;
    // Test this->stopFlag_ == true
    dashMpdParser_->stopFlag_ = NUM_1;
    dashMpdParser_->ParseSegmentTimeline(xmlParser, rootElement, segTmlineList);
    // Test node == nullptr
    dashMpdParser_->ParseSegmentTimeline(xmlParser, rootElement, segTmlineList);
    EXPECT_TRUE(segTmlineList.empty());
}

/**
 * @tc.name  : Test ParseUrlType
 * @tc.number: ParseUrlType_001
 * @tc.desc  : Test urlTypeInfo == nullptr
 *             Test *urlTypeInfo != nullptr
 */
HWTEST_F(DashMpdParserUnitTest, ParseUrlType_001, TestSize.Level0)
{
    ASSERT_NE(dashMpdParser_, nullptr);
    auto xmlParser = std::make_shared<XmlParser>();
    auto rootElement = std::make_shared<XmlElement>();
    const std::string urlTypeName = "test";
    // Test urlTypeInfo == nullptr
    dashMpdParser_->ParseUrlType(xmlParser, rootElement, urlTypeName, nullptr);
    // Test *urlTypeInfo != nullptr
    DashUrlType *urlTypeInfo = new DashUrlType();
    DashUrlType *copy = urlTypeInfo;
    dashMpdParser_->ParseUrlType(xmlParser, rootElement, urlTypeName, &urlTypeInfo);
    EXPECT_EQ(urlTypeInfo, copy);
    delete urlTypeInfo;
}

/**
 * @tc.name  : Test GetMpdAttr
 * @tc.number: GetMpdAttr_001
 * @tc.desc  : Test type == "dynamic"
 */
HWTEST_F(DashMpdParserUnitTest, GetMpdAttr_001, TestSize.Level0)
{
    ASSERT_NE(dashMpdParser_, nullptr);
    auto impl = std::make_shared<DashMpdNodeImpl>();
    dashMpdParser_->GetMpdAttr(impl.get());
    EXPECT_EQ(dashMpdParser_->dashMpdInfo_.type_, DashType::DASH_TYPE_DYNAMIC);
}

/**
 * @tc.name  : Test GetMpdElement
 * @tc.number: GetMpdElement_001
 * @tc.desc  : Test this->stopFlag_ == true
 */
HWTEST_F(DashMpdParserUnitTest, GetMpdElement_001, TestSize.Level0)
{
    ASSERT_NE(dashMpdParser_, nullptr);
    auto xmlParser = std::make_shared<XmlParser>();
    auto rootElement = std::make_shared<XmlElement>();
    dashMpdParser_->stopFlag_ = NUM_1;
    rootElement->name_ = DashMpdParser::MPD_LABEL_BASE_URL;
    dashMpdParser_->GetMpdElement(xmlParser, rootElement);
    EXPECT_TRUE(dashMpdParser_->dashMpdInfo_.baseUrl_.empty());
}

/**
 * @tc.name  : Test ProcessMpdElement
 * @tc.number: ProcessMpdElement_001
 * @tc.desc  : Test elementNameStr == MPD_LABEL_BASE_URL
 *             Test !strValue.empty() == true
 */
HWTEST_F(DashMpdParserUnitTest, ProcessMpdElement_001, TestSize.Level0)
{
    ASSERT_NE(dashMpdParser_, nullptr);
    auto xmlParser = std::make_shared<XmlParser>();
    DashList<std::shared_ptr<XmlElement>> periodElementList;
    auto childElement = std::make_shared<XmlElement>();
    // Test elementNameStr == MPD_LABEL_BASE_URL
    childElement->name_ = DashMpdParser::MPD_LABEL_BASE_URL;
    // Test !strValue.empty() == true
    dashMpdParser_->ProcessMpdElement(xmlParser, periodElementList, childElement);
    EXPECT_FALSE(dashMpdParser_->dashMpdInfo_.baseUrl_.empty());
}

/**
 * @tc.name  : Test ParsePeriodElement
 * @tc.number: ParsePeriodElement_001
 * @tc.desc  : Test this->stopFlag_ == true
 */
HWTEST_F(DashMpdParserUnitTest, ParsePeriodElement_001, TestSize.Level0)
{
    ASSERT_NE(dashMpdParser_, nullptr);
    auto xmlParser = std::make_shared<XmlParser>();
    DashList<std::shared_ptr<XmlElement>> periodElementList;
    std::shared_ptr<XmlElement> element = nullptr;
    periodElementList.push_back(element);
    dashMpdParser_->stopFlag_ = NUM_1;
    dashMpdParser_->ParsePeriodElement(xmlParser, periodElementList);
    EXPECT_FALSE(periodElementList.empty());
}

/**
 * @tc.name  : Test ClearSegmentBase
 * @tc.number: ClearSegmentBase_001
 * @tc.desc  : Test segBaseInfo->representationIndex_ != nullptr
 */
HWTEST_F(DashMpdParserUnitTest, ClearSegmentBase_001, TestSize.Level0)
{
    ASSERT_NE(dashMpdParser_, nullptr);
    auto segBaseInfo = new DashSegBaseInfo();
    segBaseInfo->representationIndex_ = new DashUrlType();
    dashMpdParser_->ClearSegmentBase(segBaseInfo);
    EXPECT_EQ(segBaseInfo->representationIndex_, nullptr);
    delete segBaseInfo;
}

/**
 * @tc.name  : Test ClearMultSegBase
 * @tc.number: ClearMultSegBase_001
 * @tc.desc  : Test multSegBaseInfo->bitstreamSwitching_ != nullptr
 */
HWTEST_F(DashMpdParserUnitTest, ClearMultSegBase_001, TestSize.Level0)
{
    ASSERT_NE(dashMpdParser_, nullptr);
    auto multSegBaseInfo = new DashMultSegBaseInfo();
    multSegBaseInfo->bitstreamSwitching_ = new DashUrlType();
    dashMpdParser_->ClearMultSegBase(multSegBaseInfo);
    EXPECT_EQ(multSegBaseInfo->bitstreamSwitching_, nullptr);
    delete multSegBaseInfo;
}

/**
 * @tc.name  : Test ClearRoleList
 * @tc.number: ClearRoleList_001
 * @tc.desc  : Test roleList.size() > 0
 *             Test role != nullptr
 */
HWTEST_F(DashMpdParserUnitTest, ClearRoleList_001, TestSize.Level0)
{
    ASSERT_NE(dashMpdParser_, nullptr);
    DashList<DashDescriptor *> roleList;
    auto role = new DashDescriptor();
    roleList.push_back(role);
    dashMpdParser_->ClearRoleList(roleList);
    EXPECT_EQ(roleList.size(), NUM_0);
}

} // namespace HttpPluginLite
} // namespace Plugin
} // namespace Media
} // namespace OHOS