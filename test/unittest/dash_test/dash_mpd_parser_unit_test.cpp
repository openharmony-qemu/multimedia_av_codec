/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "dash_mpd_parser_unit_test.h"
#include <iostream>
#include "mpd_parser/dash_mpd_parser.h"
#include "mpd_parser/dash_mpd_manager.h"
#include "mpd_parser/dash_period_manager.h"
#include "mpd_parser/dash_adpt_set_manager.h"
#include "mpd_parser/i_dash_mpd_node.h"
#include "base64_utils.h"
#include "dash_segment_downloader.h"
#include "xml/xml_parser.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
namespace {
static const std::string BASE_MPD = "<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n"
    "<MPD xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
    "xmlns=\"urn:mpeg:DASH:schema:MPD:2011\" schemaLocation=\"urn:mpeg:DASH:schema:MPD:2011 "
    "http://standards.iso.org/ittf/PubliclyAvailableStandards/MPEG-DASH_schema_files/DASH-MPD.xsd\""
    " type=\"static\" availabilityStartTime=\"2022-10-05T19:38:39.263Z\" "
    "mediaPresentationDuration=\"PT0H2M32S\" minBufferTime=\"PT10S\" "
    "profiles=\"urn:mpeg:dash:profile:isoff-on-demand:2011\" >\n"
    "    <Period>\n"
    "        <AdaptationSet id=\"1\" group=\"1\" contentType=\"video\" par=\"4:3\" "
    "segmentAlignment=\"true\" subsegmentAlignment=\"true\" subsegmentStartsWithSAP=\"1\" startWithSAP=\"1\">\n"
    "            <Representation id=\"5\" bandwidth=\"7342976\" width=\"1920\" "
    "height=\"1080\" codecs=\"avc1.640028\">\n"
    "               <BaseURL>http://127.0.0.1:47777/test_dash/segment_base/2_video_1_1920X1080_6000_0_0.mp4</BaseURL>\n"
    "                <SegmentBase timescale=\"90000\" indexRangeExact=\"true\" "
    "indexRange=\"851-1166\">\n"
    "                    <Initialization range=\"0-850\"/>\n"
    "                </SegmentBase>\n"
    "            </Representation>\n"
    "        </AdaptationSet>\n"
    "        <AdaptationSet id=\"2\" contentType=\"audio\" lang=\"und\" group=\"2\" "
    "segmentAlignment=\"true\" subsegmentAlignment=\"true\" subsegmentStartsWithSAP=\"1\" "
    "mimeType=\"audio/mp4\">\n"
    "            <Representation id=\"8\" bandwidth=\"67135\" audioSamplingRate=\"44100\" "
    "codecs=\"mp4a.40.5\">\n"
    "                <AudioChannelConfiguration "
    "schemeIdUri=\"urn:dolby:dash:audio_channel_configuration:2011\" value=\"0\"/>\n"
    "                <BaseURL>http://127.0.0.1:47777/test_dash/segment_base/2_audio_6.mp4</BaseURL>\n"
    "                <SegmentBase timescale=\"44100\" indexRangeExact=\"true\" "
    "indexRange=\"756-1167\">\n"
    "                    <Initialization range=\"0-755\"/>\n"
    "                </SegmentBase>\n"
    "            </Representation>\n"
    "        </AdaptationSet>\n"
    "        <AdaptationSet id=\"3\" contentType=\"audio\" lang=\"und\" group=\"2\" "
    "segmentAlignment=\"true\" subsegmentAlignment=\"true\" subsegmentStartsWithSAP=\"1\" "
    "mimeType=\"audio/mp4\">\n"
    "        </AdaptationSet>\n"
    "    </Period>\n"
    "</MPD>";
}
using namespace testing::ext;

void DashMpdParserUnitTest::SetUpTestCase(void) {}

void DashMpdParserUnitTest::TearDownTestCase(void) {}

void DashMpdParserUnitTest::SetUp(void) {}

void DashMpdParserUnitTest::TearDown(void) {}

HWTEST_F(DashMpdParserUnitTest, Test_ParseSegmentBaseMpd_001, TestSize.Level1)
{
    std::string mpd = BASE_MPD;
    std::shared_ptr<DashMpdParser> mpdParser = std::make_shared<DashMpdParser>();
    mpdParser->ParseMPD(mpd.c_str(), mpd.length());
    DashMpdInfo *mpdInfo{nullptr};
    mpdParser->GetMPD(mpdInfo);
    if (mpdInfo != nullptr) {
        std::shared_ptr<DashMpdManager> mpdManager = std::make_shared<DashMpdManager>(mpdInfo, "http://1.0.0.1/1.mpd");
        mpdManager->GetMpdInfo();
        mpdManager->GetPeriods();
        std::list<std::string> baseUrlList;
        mpdManager->GetBaseUrlList(baseUrlList);
        std::string mpdUrlBase;
        std::string urlSchem;
        mpdManager->MakeBaseUrl(mpdUrlBase, urlSchem);
        DashPeriodInfo *first = mpdManager->GetFirstPeriod();
        if (first != nullptr) {
            DashPeriodInfo *next = mpdManager->GetNextPeriod(first);
            EXPECT_EQ(next, nullptr);
            std::shared_ptr<DashPeriodManager> periodMr = std::make_shared<DashPeriodManager>(first);
            DashList<DashAdptSetInfo*> adptSetList = first->adptSetList_;
            std::list<std::string> periodBaseUrlList;
            periodMr->GetBaseUrlList(periodBaseUrlList);
            EXPECT_EQ(periodBaseUrlList.empty(), true);
            int32_t flag;
            periodMr->GetInitSegment(flag);
            periodMr->GetPeriod();
            periodMr->GetPreviousPeriod();
            bool isEmpty = periodMr->Empty();
            EXPECT_EQ(isEmpty, false);
        }
    }
    EXPECT_NE(nullptr, mpdInfo);
}

HWTEST_F(DashMpdParserUnitTest, Test_ParseSegmentBaseMpd_002, TestSize.Level1)
{
    std::string mpd = BASE_MPD;
    std::shared_ptr<DashMpdParser> mpdParser = std::make_shared<DashMpdParser>();
    mpdParser->ParseMPD(mpd.c_str(), mpd.length());
    DashMpdInfo *mpdInfo{nullptr};
    mpdParser->GetMPD(mpdInfo);
    if (mpdInfo != nullptr) {
        std::shared_ptr<DashMpdManager> mpdManager = std::make_shared<DashMpdManager>(mpdInfo, "http://1.0.0.1/1.mpd");
        mpdManager->GetMpdInfo();
        mpdManager->GetPeriods();
        std::list<std::string> baseUrlList;
        mpdManager->GetBaseUrlList(baseUrlList);
        DashPeriodInfo *first = mpdManager->GetFirstPeriod();
        if (first != nullptr) {
            DashList<DashAdptSetInfo*> adptSetList = first->adptSetList_;
            std::shared_ptr<DashAdptSetManager> adpSetMr = std::make_shared<DashAdptSetManager>(adptSetList.front());
            bool isOnDemand = adpSetMr->IsOnDemand();
            EXPECT_EQ(isOnDemand, true);
            std::list<std::string> adpBaseUrlList;
            adpSetMr->GetBaseUrlList(adpBaseUrlList);
            EXPECT_EQ(adpBaseUrlList.empty(), true);
            std::vector<uint32_t> bandwidths;
            adpSetMr->GetBandwidths(bandwidths);
            EXPECT_NE(bandwidths.empty(), true);
            DashRepresentationInfo *reInfo = adpSetMr->GetRepresentationByBandwidth(7342976);
            EXPECT_NE(reInfo, nullptr);
            DashRepresentationInfo *highReInfo = adpSetMr->GetHighRepresentation();
            EXPECT_NE(highReInfo, nullptr);
            DashRepresentationInfo *lowReInfo = adpSetMr->GetLowRepresentation();
            EXPECT_NE(lowReInfo, nullptr);
            std::string mimeType = adpSetMr->GetMime();
            EXPECT_NE(mimeType.empty(), true);
        }
    }
    EXPECT_NE(nullptr, mpdInfo);
}

HWTEST_F(DashMpdParserUnitTest, Test_ParseSegmentBaseMpd_003, TestSize.Level1)
{
    DashMpdInfo *mpdInfo{nullptr};
    std::shared_ptr<DashMpdManager> mpdManager = std::make_shared<DashMpdManager>(mpdInfo, "http://1.0.0.1/1.mpd");

    auto periods = mpdManager->GetPeriods();
    EXPECT_TRUE(periods.empty());
}

HWTEST_F(DashMpdParserUnitTest, Test_ParseSegmentListMpd_001, TestSize.Level1)
{
    std::string mpd = "<?xml version=\"1.0\" ?>\n"
                      "<MPD xmlns=\"urn:mpeg:dash:schema:mpd:2011\" profiles=\"urn:mpeg:dash:profile:isoff-live:2011\""
                      " minBufferTime=\"PT2.00S\" mediaPresentationDuration=\"PT01M1.100S\" type=\"static\">\n"
                      "  <Period>\n"
                      "    <AdaptationSet mimeType=\"video/mp4\" segmentAlignment=\"true\" startWithSAP=\"1\" "
                      "minWidth=\"1280\" maxWidth=\"2560\" minHeight=\"720\" maxHeight=\"1440\">\n"
                      "      <Representation id=\"video/1\" codecs=\"hev1.2.4.H120.90\" width=\"1280\" "
                      "height=\"720\" scanType=\"progressive\" frameRate=\"30\" bandwidth=\"1955284\">\n"
                      "        <SegmentList timescale=\"1000\" duration=\"2000\">\n"
                      "          <Initialization sourceURL=\"video/1/init.mp4\"/>\n"
                      "          <SegmentTimeline>\n"
                      "              <S t=\"0\" d=\"2000\" />\n"
                      "              <S d=\"2000\" />\n"
                      "              <S d=\"2000\" />\n"
                      "              <S d=\"2000\" />\n"
                      "          </SegmentTimeline>\n"
                      "          <SegmentURL media=\"video/1/seg-1.m4s\" TI=\"6.84\" SI=\"35.15\"/>\n"
                      "          <SegmentURL media=\"video/1/seg-2.m4s\" TI=\"15.14\" SI=\"38.64\"/>\n"
                      "        </SegmentList>\n"
                      "      </Representation>\n"
                      "      <Representation id=\"video/2\" codecs=\"hev1.2.4.H120.90\" width=\"1920\" "
                      "height=\"1080\" scanType=\"progressive\" frameRate=\"30\" bandwidth=\"3282547\">\n"
                      "        <SegmentList timescale=\"1000\" duration=\"2000\">\n"
                      "          <Initialization sourceURL=\"video/2/init.mp4\"/>\n"
                      "          <SegmentURL media=\"video/2/seg-1.m4s\" TI=\"6.98\" SI=\"30.1\"/>\n"
                      "          <SegmentURL media=\"video/2/seg-2.m4s\" TI=\"15.09\" SI=\"33.79\"/>\n"
                      "        </SegmentList>\n"
                      "      </Representation>\n"
                      "    </AdaptationSet>\n"
                      "    <AdaptationSet mimeType=\"audio/mp4\" startWithSAP=\"1\" segmentAlignment=\"true\">\n"
                      "      <Representation id=\"audio/und/mp4a\" codecs=\"mp4a.40.2\" bandwidth=\"132195\" "
                      "audioSamplingRate=\"48000\">\n"
                      "        <AudioChannelConfiguration "
                      "schemeIdUri=\"urn:mpeg:dash:23003:3:audio_channel_configuration:2011\" value=\"2\"/>\n"
                      "        <SegmentList timescale=\"1000\" duration=\"2000\">\n"
                      "          <Initialization sourceURL=\"audio/und/mp4a/init.mp4\"/>\n"
                      "          <SegmentURL media=\"audio/und/mp4a/seg-1.m4s\"/>\n"
                      "          <SegmentURL media=\"audio/und/mp4a/seg-2.m4s\"/>\n"
                      "        </SegmentList>\n"
                      "      </Representation>\n"
                      "    </AdaptationSet>\n"
                      "  </Period>\n"
                      "</MPD>";
    std::shared_ptr<DashMpdParser> mpdParser = std::make_shared<DashMpdParser>();
    mpdParser->ParseMPD(mpd.c_str(), mpd.length());
    DashMpdInfo *mpdInfo{nullptr};
    mpdParser->GetMPD(mpdInfo);
    EXPECT_NE(nullptr, mpdInfo);
}

HWTEST_F(DashMpdParserUnitTest, Test_ParseSegmentTemplateMpd_001, TestSize.Level1)
{
    std::string mpd = "<?xml version=\"1.0\" ?>\n"
                      "<MPD xmlns=\"urn:mpeg:dash:schema:mpd:2011\" profiles=\"urn:mpeg:dash:profile:isoff-live:2011\""
                      " minBufferTime=\"PT2.00S\" mediaPresentationDuration=\"PT01M1.100S\" type=\"static\">\n"
                      "  <!-- Created with Bento4 mp4-dash.py, VERSION=1.7.0-613 -->\n"
                      "  <Period>\n"
                      "    <!-- Video -->\n"
                      "    <AdaptationSet mimeType=\"video/mp4\" segmentAlignment=\"true\" startWithSAP=\"1\" "
                      "minWidth=\"1280\" maxWidth=\"2560\" minHeight=\"720\" maxHeight=\"1440\">\n"
                      "      <SegmentTemplate timescale=\"1000\" duration=\"2000\" "
                      "initialization=\"$RepresentationID$/init.mp4\" media=\"$RepresentationID$/seg-$Number%04d$.m4s\""
                      " startNumber=\"1\">\n"
                      "          <SegmentTimeline>\n"
                      "              <S t=\"0\" d=\"2000\" />\n"
                      "              <S d=\"2000\" />\n"
                      "              <S d=\"2000\" />\n"
                      "              <S d=\"2000\" />\n"
                      "          </SegmentTimeline>"
                      "      </SegmentTemplate>\n"
                      "      <Representation id=\"video/1\" codecs=\"hev1.2.4.H120.90\" width=\"1280\" height=\"720\" "
                      "scanType=\"progressive\" frameRate=\"30\" bandwidth=\"1955284\"/>\n"
                      "      <Representation id=\"video/2\" codecs=\"hev1.2.4.H120.90\" width=\"1920\" height=\"1080\""
                      " scanType=\"progressive\" frameRate=\"30\" bandwidth=\"3282547\"/>\n"
                      "      <Representation id=\"video/3\" codecs=\"hev1.2.4.H150.90\" width=\"2560\" height=\"1440\""
                      " scanType=\"progressive\" frameRate=\"30\" bandwidth=\"4640480\"/>\n"
                      "    </AdaptationSet>\n"
                      "    <!-- Audio -->\n"
                      "    <AdaptationSet mimeType=\"audio/mp4\" startWithSAP=\"1\" segmentAlignment=\"true\">\n"
                      "      <SegmentTemplate timescale=\"1000\" duration=\"2000\" "
                      "initialization=\"$RepresentationID$/init.mp4\" media=\"$RepresentationID$/seg-$Number%04d$.m4s\""
                      " startNumber=\"1\"/>\n"
                      "      <Representation id=\"audio/und/mp4a\" codecs=\"mp4a.40.2\" bandwidth=\"132195\" "
                      "audioSamplingRate=\"48000\">\n"
                      "        <AudioChannelConfiguration "
                      "schemeIdUri=\"urn:mpeg:dash:23003:3:audio_channel_configuration:2011\" value=\"2\"/>\n"
                      "      </Representation>\n"
                      "    </AdaptationSet>\n"
                      "  </Period>\n"
                      "</MPD>";
    std::shared_ptr<DashMpdParser> mpdParser = std::make_shared<DashMpdParser>();
    mpdParser->ParseMPD(mpd.c_str(), mpd.length());
    DashMpdInfo *mpdInfo{nullptr};
    mpdParser->GetMPD(mpdInfo);
    EXPECT_NE(nullptr, mpdInfo);
}

HWTEST_F(DashMpdParserUnitTest, Test_ParserNode_001, TestSize.Level1)
{
    IDashMpdNode *node = IDashMpdNode::CreateNode("MPD");
    if (node != nullptr) {
        uint32_t uiAttrVal;
        node->GetAttr("id", uiAttrVal);
        node->GetAttr("test", uiAttrVal);
        EXPECT_EQ(uiAttrVal, 0);
        int32_t iAttrVal;
        node->GetAttr("id", iAttrVal);
        node->GetAttr("test", iAttrVal);
        EXPECT_EQ(iAttrVal, 0);
        uint64_t ullAttrVal;
        node->GetAttr("id", ullAttrVal);
        node->GetAttr("test", ullAttrVal);
        EXPECT_EQ(ullAttrVal, 0);
        double dAttrVal;
        node->GetAttr("id", dAttrVal);
        node->GetAttr("test", dAttrVal);
        EXPECT_EQ(dAttrVal, 0.0);
    }
}

HWTEST_F(DashMpdParserUnitTest, Test_ParserNode_002, TestSize.Level1)
{
    IDashMpdNode *node = IDashMpdNode::CreateNode("Period");
    if (node != nullptr) {
        uint32_t uiAttrVal;
        node->GetAttr("id", uiAttrVal);
        node->GetAttr("test", uiAttrVal);
        EXPECT_EQ(uiAttrVal, 0);
        int32_t iAttrVal;
        node->GetAttr("id", iAttrVal);
        node->GetAttr("test", iAttrVal);
        EXPECT_EQ(iAttrVal, 0);
        uint64_t ullAttrVal;
        node->GetAttr("id", ullAttrVal);
        node->GetAttr("test", ullAttrVal);
        EXPECT_EQ(ullAttrVal, 0);
        double dAttrVal;
        node->GetAttr("id", dAttrVal);
        node->GetAttr("test", dAttrVal);
        EXPECT_EQ(dAttrVal, 0.0);
    }
}

HWTEST_F(DashMpdParserUnitTest, Test_ParserNode_003, TestSize.Level1)
{
    IDashMpdNode *node = IDashMpdNode::CreateNode("AdaptationSet");
    if (node != nullptr) {
        uint32_t uiAttrVal;
        node->GetAttr("id", uiAttrVal);
        node->GetAttr("test", uiAttrVal);
        EXPECT_EQ(uiAttrVal, 0);
        int32_t iAttrVal;
        node->GetAttr("id", iAttrVal);
        node->GetAttr("test", iAttrVal);
        EXPECT_EQ(iAttrVal, 0);
        uint64_t ullAttrVal;
        node->GetAttr("id", ullAttrVal);
        node->GetAttr("test", ullAttrVal);
        EXPECT_EQ(ullAttrVal, 0);
        double dAttrVal;
        node->GetAttr("id", dAttrVal);
        node->GetAttr("test", dAttrVal);
        EXPECT_EQ(dAttrVal, 0.0);
    }
}

HWTEST_F(DashMpdParserUnitTest, Test_ParserNode_004, TestSize.Level1)
{
    IDashMpdNode *node = IDashMpdNode::CreateNode("ContentComponent");
    if (node != nullptr) {
        uint32_t uiAttrVal;
        node->GetAttr("id", uiAttrVal);
        node->GetAttr("test", uiAttrVal);
        EXPECT_EQ(uiAttrVal, 0);
        int32_t iAttrVal;
        node->GetAttr("id", iAttrVal);
        node->GetAttr("test", iAttrVal);
        EXPECT_EQ(iAttrVal, 0);
        uint64_t ullAttrVal;
        node->GetAttr("id", ullAttrVal);
        node->GetAttr("test", ullAttrVal);
        EXPECT_EQ(ullAttrVal, 0);
        double dAttrVal;
        node->GetAttr("id", dAttrVal);
        node->GetAttr("test", dAttrVal);
        EXPECT_EQ(dAttrVal, 0.0);
    }
}

HWTEST_F(DashMpdParserUnitTest, Test_ParserNode_005, TestSize.Level1)
{
    IDashMpdNode *node = IDashMpdNode::CreateNode("Representation");
    if (node != nullptr) {
        uint32_t uiAttrVal;
        node->GetAttr("id", uiAttrVal);
        node->GetAttr("test", uiAttrVal);
        EXPECT_EQ(uiAttrVal, 0);
        int32_t iAttrVal;
        node->GetAttr("id", iAttrVal);
        node->GetAttr("test", iAttrVal);
        EXPECT_EQ(iAttrVal, 0);
        uint64_t ullAttrVal;
        node->GetAttr("id", ullAttrVal);
        node->GetAttr("test", ullAttrVal);
        EXPECT_EQ(ullAttrVal, 0);
        double dAttrVal;
        node->GetAttr("id", dAttrVal);
        node->GetAttr("test", dAttrVal);
        EXPECT_EQ(dAttrVal, 0.0);
    }
}

HWTEST_F(DashMpdParserUnitTest, Test_ParserNode_006, TestSize.Level1)
{
    IDashMpdNode *node = IDashMpdNode::CreateNode("SegmentBase");
    if (node != nullptr) {
        uint32_t uiAttrVal;
        node->GetAttr("timescale", uiAttrVal);
        node->GetAttr("test", uiAttrVal);
        EXPECT_EQ(uiAttrVal, 0);
        int32_t iAttrVal;
        node->GetAttr("timescale", iAttrVal);
        node->GetAttr("test", iAttrVal);
        EXPECT_EQ(iAttrVal, 0);
        uint64_t ullAttrVal;
        node->GetAttr("timescale", ullAttrVal);
        node->GetAttr("test", ullAttrVal);
        EXPECT_EQ(ullAttrVal, 0);
        double dAttrVal;
        node->GetAttr("timescale", dAttrVal);
        node->GetAttr("test", dAttrVal);
        EXPECT_EQ(dAttrVal, 0.0);
    }
}

HWTEST_F(DashMpdParserUnitTest, Test_ParserNode_007, TestSize.Level1)
{
    IDashMpdNode *node = IDashMpdNode::CreateNode("MultipleSegmentBase");
    if (node != nullptr) {
        uint32_t uiAttrVal;
        node->GetAttr("duration", uiAttrVal);
        node->GetAttr("test", uiAttrVal);
        EXPECT_EQ(uiAttrVal, 0);
        int32_t iAttrVal;
        node->GetAttr("duration", iAttrVal);
        node->GetAttr("test", iAttrVal);
        EXPECT_EQ(iAttrVal, 0);
        uint64_t ullAttrVal;
        node->GetAttr("duration", ullAttrVal);
        node->GetAttr("test", ullAttrVal);
        EXPECT_EQ(ullAttrVal, 0);
        double dAttrVal;
        node->GetAttr("duration", dAttrVal);
        node->GetAttr("test", dAttrVal);
        EXPECT_EQ(dAttrVal, 0.0);
    }
}

HWTEST_F(DashMpdParserUnitTest, Test_ParserNode_008, TestSize.Level1)
{
    IDashMpdNode *node = IDashMpdNode::CreateNode("SegmentList");
    if (node != nullptr) {
        uint32_t uiAttrVal;
        node->GetAttr("media", uiAttrVal);
        node->GetAttr("test", uiAttrVal);
        EXPECT_EQ(uiAttrVal, 0);
        int32_t iAttrVal;
        node->GetAttr("media", iAttrVal);
        node->GetAttr("test", iAttrVal);
        EXPECT_EQ(iAttrVal, 0);
        uint64_t ullAttrVal;
        node->GetAttr("media", ullAttrVal);
        node->GetAttr("test", ullAttrVal);
        EXPECT_EQ(ullAttrVal, 0);
        double dAttrVal;
        node->GetAttr("media", dAttrVal);
        node->GetAttr("test", dAttrVal);
        EXPECT_EQ(dAttrVal, 0.0);
    }
}

HWTEST_F(DashMpdParserUnitTest, Test_ParserNode_009, TestSize.Level1)
{
    IDashMpdNode *node = IDashMpdNode::CreateNode("SegmentTemplate");
    if (node != nullptr) {
        uint32_t uiAttrVal;
        node->GetAttr("media", uiAttrVal);
        node->GetAttr("test", uiAttrVal);
        EXPECT_EQ(uiAttrVal, 0);
        int32_t iAttrVal;
        node->GetAttr("media", iAttrVal);
        node->GetAttr("test", iAttrVal);
        EXPECT_EQ(iAttrVal, 0);
        uint64_t ullAttrVal;
        node->GetAttr("media", ullAttrVal);
        node->GetAttr("test", ullAttrVal);
        EXPECT_EQ(ullAttrVal, 0);
        double dAttrVal;
        node->GetAttr("media", dAttrVal);
        node->GetAttr("test", dAttrVal);
        EXPECT_EQ(dAttrVal, 0.0);
    }
}

HWTEST_F(DashMpdParserUnitTest, Test_ParserNode_010, TestSize.Level1)
{
    IDashMpdNode *node = IDashMpdNode::CreateNode("Initialization");
    if (node != nullptr) {
        uint32_t uiAttrVal;
        node->GetAttr("sourceURL", uiAttrVal);
        node->GetAttr("test", uiAttrVal);
        EXPECT_EQ(uiAttrVal, 0);
        int32_t iAttrVal;
        node->GetAttr("sourceURL", iAttrVal);
        node->GetAttr("test", iAttrVal);
        EXPECT_EQ(iAttrVal, 0);
        uint64_t ullAttrVal;
        node->GetAttr("sourceURL", ullAttrVal);
        node->GetAttr("test", ullAttrVal);
        EXPECT_EQ(ullAttrVal, 0);
        double dAttrVal;
        node->GetAttr("sourceURL", dAttrVal);
        node->GetAttr("test", dAttrVal);
        EXPECT_EQ(dAttrVal, 0.0);
    }
}

HWTEST_F(DashMpdParserUnitTest, Test_ParserNode_011, TestSize.Level1)
{
    IDashMpdNode *node = IDashMpdNode::CreateNode("SegmentTimeline");
    if (node != nullptr) {
        uint32_t uiAttrVal;
        node->GetAttr("t", uiAttrVal);
        node->GetAttr("test", uiAttrVal);
        EXPECT_EQ(uiAttrVal, 0);
        int32_t iAttrVal;
        node->GetAttr("t", iAttrVal);
        node->GetAttr("test", iAttrVal);
        EXPECT_EQ(iAttrVal, 0);
        uint64_t ullAttrVal;
        node->GetAttr("t", ullAttrVal);
        node->GetAttr("test", ullAttrVal);
        EXPECT_EQ(ullAttrVal, 0);
        double dAttrVal;
        node->GetAttr("t", dAttrVal);
        node->GetAttr("test", dAttrVal);
        EXPECT_EQ(dAttrVal, 0.0);
    }
}

HWTEST_F(DashMpdParserUnitTest, Test_ParserNode_012, TestSize.Level1)
{
    IDashMpdNode *node = IDashMpdNode::CreateNode("ContentProtection");
    if (node != nullptr) {
        uint32_t uiAttrVal;
        node->GetAttr("schemeIdUri", uiAttrVal);
        node->GetAttr("test", uiAttrVal);
        EXPECT_EQ(uiAttrVal, 0);
        int32_t iAttrVal;
        node->GetAttr("schemeIdUri", iAttrVal);
        node->GetAttr("test", iAttrVal);
        EXPECT_EQ(iAttrVal, 0);
        uint64_t ullAttrVal;
        node->GetAttr("schemeIdUri", ullAttrVal);
        node->GetAttr("test", ullAttrVal);
        EXPECT_EQ(ullAttrVal, 0);
        double dAttrVal;
        node->GetAttr("schemeIdUri", dAttrVal);
        node->GetAttr("test", dAttrVal);
        EXPECT_EQ(dAttrVal, 0.0);
    }
}

HWTEST_F(DashMpdParserUnitTest, Test_ParserNode_013, TestSize.Level1)
{
    IDashMpdNode *node = IDashMpdNode::CreateNode("SegmentURL");
    if (node != nullptr) {
        uint32_t uiAttrVal;
        node->GetAttr("media", uiAttrVal);
        node->GetAttr("test", uiAttrVal);
        EXPECT_EQ(uiAttrVal, 0);
        int32_t iAttrVal;
        node->GetAttr("media", iAttrVal);
        node->GetAttr("test", iAttrVal);
        EXPECT_EQ(iAttrVal, 0);
        uint64_t ullAttrVal;
        node->GetAttr("media", ullAttrVal);
        node->GetAttr("test", ullAttrVal);
        EXPECT_EQ(ullAttrVal, 0);
        double dAttrVal;
        node->GetAttr("media", dAttrVal);
        node->GetAttr("test", dAttrVal);
        EXPECT_EQ(dAttrVal, 0.0);
    }
}

HWTEST_F(DashMpdParserUnitTest, Test_Base64, TestSize.Level1)
{
    uint8_t src[2048];
    uint32_t srcSize = 0;
    uint8_t dest[2048]; // 2048: pssh len
    uint32_t destSize = 2048; // 2048: pssh len
    bool ret = Base64Utils::Base64Decode(src, srcSize, dest, &destSize);
    EXPECT_FALSE(ret);
    uint8_t *src1 = nullptr; // 2048: pssh len
    ret = Base64Utils::Base64Decode(src1, srcSize, dest, &destSize);
    EXPECT_FALSE(ret);
    uint8_t *dest1 = nullptr;
    ret = Base64Utils::Base64Decode(src, srcSize, dest1, &destSize);
    EXPECT_FALSE(ret);
    uint32_t *destSize1 = nullptr;
    ret = Base64Utils::Base64Decode(src, srcSize, dest, destSize1);
    EXPECT_FALSE(ret);
    srcSize = 2049;
    ret = Base64Utils::Base64Decode(src, srcSize, dest, &destSize);
    EXPECT_FALSE(ret);
}

HWTEST_F(DashMpdParserUnitTest, Test_Base64_001, TestSize.Level1)
{
    uint8_t src[5] = {0}; // 5: srcSize
    uint8_t dest[20]; // 20:destSize
    uint32_t destSize = sizeof(dest);
    bool ret = Base64Utils::Base64Decode(src, 5, dest, &destSize); // 5: srcSize
    EXPECT_FALSE(ret);
}

HWTEST_F(DashMpdParserUnitTest, Test_Base64_002, TestSize.Level1)
{
    const uint8_t src[] = "dGVzdCBzdHJpbmc="; // base64 编码的 "test string"
    uint8_t dest[20];                         // 20:预期解码输出的大小
    uint32_t destSize = sizeof(dest);

    bool ret = Base64Utils::Base64Decode(src, sizeof(src) - 1, dest, &destSize);
    ASSERT_TRUE(ret);
    ASSERT_EQ(destSize, 11u); // 11:"test string" 的长度
    ASSERT_EQ(std::string(reinterpret_cast<char *>(dest), destSize), "test string");
}

HWTEST_F(DashMpdParserUnitTest, Test_Base64_003, TestSize.Level1)
{
    const uint8_t src[] = "€€€€";
    uint8_t dest[20]; // 20:destSize
    uint32_t destSize = sizeof(dest);

    bool ret = Base64Utils::Base64Decode(src, sizeof(src) - 1, dest, &destSize);
    EXPECT_FALSE(ret);
}

/**
 * @tc.name: Test_Base64_004
 * @tc.desc: test Base64Decode with no padding (src length is multiple of 3, no '=' at end)
 * @tc.type: FUNC
 */
HWTEST_F(DashMpdParserUnitTest, Test_Base64_004, TestSize.Level1)
{
    // "abc" base64 encoded without padding
    const uint8_t src[] = "YWJj";
    uint8_t dest[20]; // 20:destSize
    uint32_t destSize = sizeof(dest);

    bool ret = Base64Utils::Base64Decode(src, sizeof(src) - 1, dest, &destSize);
    ASSERT_TRUE(ret);
    ASSERT_EQ(destSize, 3u); // 3: length of "abc"
    ASSERT_EQ(std::string(reinterpret_cast<char *>(dest), destSize), "abc");
}

/**
 * @tc.name: Test_Base64_005
 * @tc.desc: test Base64Decode with double padding (src length % 3 == 1, "==" at end)
 * @tc.type: FUNC
 */
HWTEST_F(DashMpdParserUnitTest, Test_Base64_005, TestSize.Level1)
{
    // "a" base64 encoded with double padding
    const uint8_t src[] = "YQ==";
    uint8_t dest[20]; // 20:destSize
    uint32_t destSize = sizeof(dest);

    bool ret = Base64Utils::Base64Decode(src, sizeof(src) - 1, dest, &destSize);
    ASSERT_TRUE(ret);
    ASSERT_EQ(destSize, 1u); // 1: length of "a"
    ASSERT_EQ(dest[0], static_cast<uint8_t>('a'));
}

HWTEST_F(DashMpdParserUnitTest, Test_ParseEventStream_001, TestSize.Level1)
{
    std::string mpd = "<?xml version=\"1.0\" ?>\n"
        "<MPD xmlns=\"urn:mpeg:dash:schema:mpd:2011\" type=\"static\">\n"
        "  <Period>\n"
        "    <EventStream schemeIdUri=\"urn:test:event\" value=\"test\" "
        "timescale=\"1000\" presentationTimeOffset=\"0\">\n"
        "      <Event id=\"1\" duration=\"5000\" presentationTime=\"1000\" value=\"event1\"/>\n"
        "      <Event id=\"2\" duration=\"10000\" presentationTime=\"2000\" value=\"event2\"/>\n"
        "    </EventStream>\n"
        "  </Period>\n"
        "</MPD>";
    auto mpdParser = std::make_shared<DashMpdParser>();
    mpdParser->ParseMPD(mpd.c_str(), mpd.length());
    DashMpdInfo *mpdInfo{nullptr};
    mpdParser->GetMPD(mpdInfo);
    ASSERT_NE(mpdInfo, nullptr);
    ASSERT_FALSE(mpdInfo->periodInfoList_.empty());
    auto periodInfo = mpdInfo->periodInfoList_.front();
    ASSERT_NE(periodInfo, nullptr);
    ASSERT_FALSE(periodInfo->eventStreamList_.empty());
    auto eventStreamInfo = periodInfo->eventStreamList_.front();
    ASSERT_NE(eventStreamInfo, nullptr);
    EXPECT_EQ(eventStreamInfo->schemeIdUri_, "urn:test:event");
    EXPECT_EQ(eventStreamInfo->value_, "test");
    EXPECT_EQ(eventStreamInfo->timescale_, 1000u);
    EXPECT_EQ(eventStreamInfo->presentationTimeOffset_, 0u);
    ASSERT_FALSE(eventStreamInfo->eventList_.empty());
    auto eventInfo = eventStreamInfo->eventList_.front();
    ASSERT_NE(eventInfo, nullptr);
    EXPECT_EQ(eventInfo->id_, "1");
}

HWTEST_F(DashMpdParserUnitTest, Test_ParseEventStream_002, TestSize.Level1)
{
    std::string mpd = "<?xml version=\"1.0\" ?>\n"
        "<MPD xmlns=\"urn:mpeg:dash:schema:mpd:2011\" type=\"static\">\n"
        "  <Period>\n"
        "    <EventStream schemeIdUri=\"urn:test:event\" timescale=\"1000\">\n"
        "      <BaseURL>http://test.com/base</BaseURL>\n"
        "      <Event id=\"1\" presentationTime=\"0\"/>\n"
        "    </EventStream>\n"
        "  </Period>\n"
        "</MPD>";
    auto mpdParser = std::make_shared<DashMpdParser>();
    mpdParser->ParseMPD(mpd.c_str(), mpd.length());
    DashMpdInfo *mpdInfo{nullptr};
    mpdParser->GetMPD(mpdInfo);
    ASSERT_NE(mpdInfo, nullptr);
    auto periodInfo = mpdInfo->periodInfoList_.front();
    ASSERT_NE(periodInfo, nullptr);
    auto eventStreamInfo = periodInfo->eventStreamList_.front();
    ASSERT_NE(eventStreamInfo, nullptr);
    ASSERT_FALSE(eventStreamInfo->baseUrl_.empty());
    EXPECT_EQ(eventStreamInfo->baseUrl_.front(), "http://test.com/base");
}

HWTEST_F(DashMpdParserUnitTest, Test_ParseSpliceInfoSection_001, TestSize.Level1)
{
    std::string mpd = "<?xml version=\"1.0\" ?>\n"
        "<MPD xmlns=\"urn:mpeg:dash:schema:mpd:2011\" type=\"static\">\n"
        "  <Period>\n"
        "    <EventStream schemeIdUri=\"urn:scte:scte35:2013\" timescale=\"1000\">\n"
        "      <Event id=\"1\" presentationTime=\"1000\">\n"
        "        <SpliceInfoSection xmlns=\"urn:scte:scte35:2013\">\n"
        "          <SpliceInsert outOfNetworkIndicator=\"true\" spliceImmediateFlag=\"true\">\n"
        "            <BreakDuration duration=\"1000\" autoReturn=\"false\"/>\n"
        "          </SpliceInsert>\n"
        "        </SpliceInfoSection>\n"
        "      </Event>\n"
        "    </EventStream>\n"
        "  </Period>\n"
        "</MPD>";
    auto mpdParser = std::make_shared<DashMpdParser>();
    mpdParser->ParseMPD(mpd.c_str(), mpd.length());
    DashMpdInfo *mpdInfo{nullptr};
    mpdParser->GetMPD(mpdInfo);
    ASSERT_NE(mpdInfo, nullptr);
    auto periodInfo = mpdInfo->periodInfoList_.front();
    ASSERT_NE(periodInfo, nullptr);
    auto eventStreamInfo = periodInfo->eventStreamList_.front();
    ASSERT_NE(eventStreamInfo, nullptr);
    auto eventInfo = eventStreamInfo->eventList_.front();
    ASSERT_NE(eventInfo, nullptr);
    ASSERT_FALSE(eventInfo->contents_.empty());
}

HWTEST_F(DashMpdParserUnitTest, Test_ParseSpliceInsert_001, TestSize.Level1)
{
    std::string mpd = "<?xml version=\"1.0\" ?>\n"
        "<MPD xmlns=\"urn:mpeg:dash:schema:mpd:2011\" type=\"static\">\n"
        "  <Period>\n"
        "    <EventStream schemeIdUri=\"urn:scte:scte35:2013\" timescale=\"1000\">\n"
        "      <Event id=\"1\">\n"
        "        <SpliceInfoSection xmlns=\"urn:scte\">\n"
        "          <SpliceInsert outOfNetworkIndicator=\"1\" spliceImmediateFlag=\"false\">\n"
        "            <BreakDuration duration=\"5000\" autoReturn=\"true\"/>\n"
        "          </SpliceInsert>\n"
        "        </SpliceInfoSection>\n"
        "      </Event>\n"
        "    </EventStream>\n"
        "  </Period>\n"
        "</MPD>";
    auto mpdParser = std::make_shared<DashMpdParser>();
    mpdParser->ParseMPD(mpd.c_str(), mpd.length());
    DashMpdInfo *mpdInfo{nullptr};
    mpdParser->GetMPD(mpdInfo);
    ASSERT_NE(mpdInfo, nullptr);
    auto periodInfo = mpdInfo->periodInfoList_.front();
    auto eventStreamInfo = periodInfo->eventStreamList_.front();
    auto eventInfo = eventStreamInfo->eventList_.front();
    ASSERT_NE(eventInfo, nullptr);
    EXPECT_TRUE(eventInfo->contents_.find("SpliceInsert") != eventInfo->contents_.end() ||
                eventInfo->contents_.find("SpliceInfoSection") != eventInfo->contents_.end());
}

HWTEST_F(DashMpdParserUnitTest, Test_ParseBreakDuration_001, TestSize.Level1)
{
    std::string mpd = "<?xml version=\"1.0\" ?>\n"
        "<MPD xmlns=\"urn:mpeg:dash:schema:mpd:2011\" type=\"static\">\n"
        "  <Period>\n"
        "    <EventStream schemeIdUri=\"urn:scte:scte35:2013\" timescale=\"1000\">\n"
        "      <Event id=\"1\">\n"
        "        <SpliceInfoSection>\n"
        "          <SpliceInsert>\n"
        "            <BreakDuration duration=\"10000\" autoReturn=\"false\"/>\n"
        "          </SpliceInsert>\n"
        "        </SpliceInfoSection>\n"
        "      </Event>\n"
        "    </EventStream>\n"
        "  </Period>\n"
        "</MPD>";
    auto mpdParser = std::make_shared<DashMpdParser>();
    mpdParser->ParseMPD(mpd.c_str(), mpd.length());
    DashMpdInfo *mpdInfo{nullptr};
    mpdParser->GetMPD(mpdInfo);
    ASSERT_NE(mpdInfo, nullptr);
    auto periodInfo = mpdInfo->periodInfoList_.front();
    auto eventStreamInfo = periodInfo->eventStreamList_.front();
    auto eventInfo = eventStreamInfo->eventList_.front();
    ASSERT_NE(eventInfo, nullptr);
    EXPECT_TRUE(eventInfo->contents_.find("BreakDuration") != eventInfo->contents_.end());
}

HWTEST_F(DashMpdParserUnitTest, Test_ParseEvent_001, TestSize.Level1)
{
    std::string mpd = "<?xml version=\"1.0\" ?>\n"
        "<MPD xmlns=\"urn:mpeg:dash:schema:mpd:2011\" type=\"static\">\n"
        "  <Period>\n"
        "    <EventStream schemeIdUri=\"urn:test:event\" timescale=\"100\" presentationTimeOffset=\"10\">\n"
        "      <Event id=\"evt1\" duration=\"1000\" presentationTime=\"100\" value=\"data\"/>\n"
        "    </EventStream>\n"
        "  </Period>\n"
        "</MPD>";
    auto mpdParser = std::make_shared<DashMpdParser>();
    mpdParser->ParseMPD(mpd.c_str(), mpd.length());
    DashMpdInfo *mpdInfo{nullptr};
    mpdParser->GetMPD(mpdInfo);
    ASSERT_NE(mpdInfo, nullptr);
    auto periodInfo = mpdInfo->periodInfoList_.front();
    auto eventStreamInfo = periodInfo->eventStreamList_.front();
    auto eventInfo = eventStreamInfo->eventList_.front();
    ASSERT_NE(eventInfo, nullptr);
    EXPECT_EQ(eventInfo->id_, "evt1");
    EXPECT_EQ(eventInfo->value_, "data");
    EXPECT_EQ(eventInfo->classify_, "urn:test:event");
}

HWTEST_F(DashMpdParserUnitTest, Test_ParseEventStream_EmptyEventStream_001, TestSize.Level1)
{
    std::string mpd = "<?xml version=\"1.0\" ?>\n"
        "<MPD xmlns=\"urn:mpeg:dash:schema:mpd:2011\" type=\"static\">\n"
        "  <Period>\n"
        "    <EventStream schemeIdUri=\"urn:test:event\"/>\n"
        "  </Period>\n"
        "</MPD>";
    auto mpdParser = std::make_shared<DashMpdParser>();
    mpdParser->ParseMPD(mpd.c_str(), mpd.length());
    DashMpdInfo *mpdInfo{nullptr};
    mpdParser->GetMPD(mpdInfo);
    ASSERT_NE(mpdInfo, nullptr);
    auto periodInfo = mpdInfo->periodInfoList_.front();
    ASSERT_NE(periodInfo, nullptr);
    ASSERT_FALSE(periodInfo->eventStreamList_.empty());
    auto eventStreamInfo = periodInfo->eventStreamList_.front();
    ASSERT_NE(eventStreamInfo, nullptr);
    EXPECT_TRUE(eventStreamInfo->eventList_.empty());
}

HWTEST_F(DashMpdParserUnitTest, Test_ParseEventStream_MultipleEvents_001, TestSize.Level1)
{
    std::string mpd = "<?xml version=\"1.0\" ?>\n"
        "<MPD xmlns=\"urn:mpeg:dash:schema:mpd:2011\" type=\"static\">\n"
        "  <Period>\n"
        "    <EventStream schemeIdUri=\"urn:test\" timescale=\"1000\">\n"
        "      <Event id=\"1\" presentationTime=\"100\"/>\n"
        "      <Event id=\"2\" presentationTime=\"200\"/>\n"
        "      <Event id=\"3\" presentationTime=\"300\"/>\n"
        "    </EventStream>\n"
        "  </Period>\n"
        "</MPD>";
    auto mpdParser = std::make_shared<DashMpdParser>();
    mpdParser->ParseMPD(mpd.c_str(), mpd.length());
    DashMpdInfo *mpdInfo{nullptr};
    mpdParser->GetMPD(mpdInfo);
    ASSERT_NE(mpdInfo, nullptr);
    auto periodInfo = mpdInfo->periodInfoList_.front();
    auto eventStreamInfo = periodInfo->eventStreamList_.front();
    ASSERT_EQ(eventStreamInfo->eventList_.size(), 3u);
}

HWTEST_F(DashMpdParserUnitTest, Test_ParseEventStream_WithEmptyElementName_001, TestSize.Level1)
{
    std::string mpd = "<?xml version=\"1.0\" ?>\n"
        "<MPD xmlns=\"urn:mpeg:dash:schema:mpd:2011\" type=\"static\">\n"
        "  <Period>\n"
        "    <EventStream schemeIdUri=\"urn:test:event\" value=\"test\" timescale=\"1000\">\n"
        "      <Event id=\"1\" duration=\"5000\" presentationTime=\"1000\" value=\"event1\"/>\n"
        "    </EventStream>\n"
        "  </Period>\n"
        "</MPD>";
    auto mpdParser = std::make_shared<DashMpdParser>();
    mpdParser->ParseMPD(mpd.c_str(), mpd.length());
    DashMpdInfo *mpdInfo{nullptr};
    mpdParser->GetMPD(mpdInfo);
    ASSERT_NE(mpdInfo, nullptr);
    ASSERT_FALSE(mpdInfo->periodInfoList_.empty());
    auto periodInfo = mpdInfo->periodInfoList_.front();
    ASSERT_NE(periodInfo, nullptr);
    ASSERT_FALSE(periodInfo->eventStreamList_.empty());
    auto eventStreamInfo = periodInfo->eventStreamList_.front();
    ASSERT_NE(eventStreamInfo, nullptr);
    EXPECT_EQ(eventStreamInfo->eventList_.size(), 1u);
}

HWTEST_F(DashMpdParserUnitTest, Test_ParseEventStream_WithEmptyEventId_001, TestSize.Level1)
{
    std::string mpd = "<?xml version=\"1.0\" ?>\n"
        "<MPD xmlns=\"urn:mpeg:dash:schema:mpd:2011\" type=\"static\">\n"
        "  <Period>\n"
        "    <EventStream schemeIdUri=\"urn:test:event\" value=\"test\" timescale=\"1000\">\n"
        "      <Event duration=\"5000\" presentationTime=\"1000\" value=\"event1\"/>\n"
        "    </EventStream>\n"
        "  </Period>\n"
        "</MPD>";
    auto mpdParser = std::make_shared<DashMpdParser>();
    mpdParser->ParseMPD(mpd.c_str(), mpd.length());
    DashMpdInfo *mpdInfo{nullptr};
    mpdParser->GetMPD(mpdInfo);
    ASSERT_NE(mpdInfo, nullptr);
    ASSERT_FALSE(mpdInfo->periodInfoList_.empty());
    auto periodInfo = mpdInfo->periodInfoList_.front();
    ASSERT_NE(periodInfo, nullptr);
    ASSERT_FALSE(periodInfo->eventStreamList_.empty());
    auto eventStreamInfo = periodInfo->eventStreamList_.front();
    ASSERT_NE(eventStreamInfo, nullptr);
    EXPECT_EQ(eventStreamInfo->eventList_.size(), 1u);
    auto eventInfo = eventStreamInfo->eventList_.front();
    EXPECT_EQ(eventInfo->id_, "");
}

HWTEST_F(DashMpdParserUnitTest, Test_ParseEventStream_NullEventStreamInfo_001, TestSize.Level1)
{
    std::string mpd = "<?xml version=\"1.0\" ?>\n"
        "<MPD xmlns=\"urn:mpeg:dash:schema:mpd:2011\" type=\"static\">\n"
        "  <Period>\n"
        "    <EventStream schemeIdUri=\"urn:test:event\" value=\"test\" timescale=\"1000\">\n"
        "      <Event id=\"1\" duration=\"5000\" presentationTime=\"1000\" value=\"event1\">\n"
        "        <SpliceInfoSection xmlns=\"test\">\n"
        "          <SpliceInsert/>\n"
        "        </SpliceInfoSection>\n"
        "      </Event>\n"
        "    </EventStream>\n"
        "  </Period>\n"
        "</MPD>";
    auto mpdParser = std::make_shared<DashMpdParser>();
    mpdParser->ParseMPD(mpd.c_str(), mpd.length());
    DashMpdInfo *mpdInfo{nullptr};
    mpdParser->GetMPD(mpdInfo);
    ASSERT_NE(mpdInfo, nullptr);
    auto periodInfo = mpdInfo->periodInfoList_.front();
    ASSERT_NE(periodInfo, nullptr);
    ASSERT_FALSE(periodInfo->eventStreamList_.empty());
    auto eventStreamInfo = periodInfo->eventStreamList_.front();
    ASSERT_NE(eventStreamInfo, nullptr);
    ASSERT_FALSE(eventStreamInfo->eventList_.empty());
    auto eventInfo = eventStreamInfo->eventList_.front();
    ASSERT_NE(eventInfo, nullptr);
    EXPECT_FALSE(eventInfo->contents_.empty());
}

HWTEST_F(DashMpdParserUnitTest, Test_ParseSpliceInfoSection_WithNullNode_001, TestSize.Level1)
{
    std::string mpd = "<?xml version=\"1.0\" ?>\n"
        "<MPD xmlns=\"urn:mpeg:dash:schema:mpd:2011\" type=\"static\">\n"
        "  <Period>\n"
        "    <EventStream schemeIdUri=\"urn:scte:scte35:2013\" timescale=\"1000\">\n"
        "      <Event id=\"1\" presentationTime=\"1000\">\n"
        "      </Event>\n"
        "    </EventStream>\n"
        "  </Period>\n"
        "</MPD>";
    auto mpdParser = std::make_shared<DashMpdParser>();
    mpdParser->ParseMPD(mpd.c_str(), mpd.length());
    DashMpdInfo *mpdInfo{nullptr};
    mpdParser->GetMPD(mpdInfo);
    ASSERT_NE(mpdInfo, nullptr);
    auto periodInfo = mpdInfo->periodInfoList_.front();
    ASSERT_NE(periodInfo, nullptr);
    ASSERT_FALSE(periodInfo->eventStreamList_.empty());
    auto eventStreamInfo = periodInfo->eventStreamList_.front();
    ASSERT_NE(eventStreamInfo, nullptr);
    auto eventInfo = eventStreamInfo->eventList_.front();
    ASSERT_NE(eventInfo, nullptr);
    EXPECT_TRUE(eventInfo->contents_.empty());
}

HWTEST_F(DashMpdParserUnitTest, Test_ParseSpliceInfoSectionChildren_NoSpliceInsert_001, TestSize.Level1)
{
    std::string mpd = "<?xml version=\"1.0\" ?>\n"
        "<MPD xmlns=\"urn:mpeg:dash:schema:mpd:2011\" type=\"static\">\n"
        "  <Period>\n"
        "    <EventStream schemeIdUri=\"urn:scte:scte35:2013\" timescale=\"1000\">\n"
        "      <Event id=\"1\" presentationTime=\"1000\">\n"
        "        <SpliceInfoSection xmlns=\"urn:scte\">\n"
        "          <OtherElement/>\n"
        "        </SpliceInfoSection>\n"
        "      </Event>\n"
        "    </EventStream>\n"
        "  </Period>\n"
        "</MPD>";
    auto mpdParser = std::make_shared<DashMpdParser>();
    mpdParser->ParseMPD(mpd.c_str(), mpd.length());
    DashMpdInfo *mpdInfo{nullptr};
    mpdParser->GetMPD(mpdInfo);
    ASSERT_NE(mpdInfo, nullptr);
    auto periodInfo = mpdInfo->periodInfoList_.front();
    auto eventStreamInfo = periodInfo->eventStreamList_.front();
    auto eventInfo = eventStreamInfo->eventList_.front();
    ASSERT_NE(eventInfo, nullptr);
    EXPECT_TRUE(eventInfo->contents_.find("SpliceInfoSection") != eventInfo->contents_.end());
}

HWTEST_F(DashMpdParserUnitTest, Test_ParseSpliceInsertChildren_NoBreakDuration_001, TestSize.Level1)
{
    std::string mpd = "<?xml version=\"1.0\" ?>\n"
        "<MPD xmlns=\"urn:mpeg:dash:schema:mpd:2011\" type=\"static\">\n"
        "  <Period>\n"
        "    <EventStream schemeIdUri=\"urn:scte:scte35:2013\" timescale=\"1000\">\n"
        "      <Event id=\"1\" presentationTime=\"1000\">\n"
        "        <SpliceInfoSection xmlns=\"urn:scte\">\n"
        "          <SpliceInsert outOfNetworkIndicator=\"true\" spliceImmediateFlag=\"false\">\n"
        "            <OtherElement/>\n"
        "          </SpliceInsert>\n"
        "        </SpliceInfoSection>\n"
        "      </Event>\n"
        "    </EventStream>\n"
        "  </Period>\n"
        "</MPD>";
    auto mpdParser = std::make_shared<DashMpdParser>();
    mpdParser->ParseMPD(mpd.c_str(), mpd.length());
    DashMpdInfo *mpdInfo{nullptr};
    mpdParser->GetMPD(mpdInfo);
    ASSERT_NE(mpdInfo, nullptr);
    auto periodInfo = mpdInfo->periodInfoList_.front();
    auto eventStreamInfo = periodInfo->eventStreamList_.front();
    auto eventInfo = eventStreamInfo->eventList_.front();
    ASSERT_NE(eventInfo, nullptr);
    EXPECT_TRUE(eventInfo->contents_.find("SpliceInsert") != eventInfo->contents_.end());
}

HWTEST_F(DashMpdParserUnitTest, Test_ParseBreakDuration_DurationAndAutoReturn_001, TestSize.Level1)
{
    std::string mpd = "<?xml version=\"1.0\" ?>\n"
        "<MPD xmlns=\"urn:mpeg:dash:schema:mpd:2011\" type=\"static\">\n"
        "  <Period>\n"
        "    <EventStream schemeIdUri=\"urn:scte:scte35:2013\" timescale=\"1000\">\n"
        "      <Event id=\"1\">\n"
        "        <SpliceInfoSection xmlns=\"urn:scte\">\n"
        "          <SpliceInsert outOfNetworkIndicator=\"1\">\n"
        "            <BreakDuration duration=\"9999\" autoReturn=\"true\"/>\n"
        "          </SpliceInsert>\n"
        "        </SpliceInfoSection>\n"
        "      </Event>\n"
        "    </EventStream>\n"
        "  </Period>\n"
        "</MPD>";
    auto mpdParser = std::make_shared<DashMpdParser>();
    mpdParser->ParseMPD(mpd.c_str(), mpd.length());
    DashMpdInfo *mpdInfo{nullptr};
    mpdParser->GetMPD(mpdInfo);
    ASSERT_NE(mpdInfo, nullptr);
    auto periodInfo = mpdInfo->periodInfoList_.front();
    auto eventStreamInfo = periodInfo->eventStreamList_.front();
    auto eventInfo = eventStreamInfo->eventList_.front();
    ASSERT_NE(eventInfo, nullptr);
    auto it = eventInfo->contents_.find("BreakDuration");
    ASSERT_NE(it, eventInfo->contents_.end());
    EXPECT_TRUE(it->second.find("duration=9999") != std::string::npos);
    EXPECT_TRUE(it->second.find("autoReturn=true") != std::string::npos);
}

HWTEST_F(DashMpdParserUnitTest, Test_ParseEventChildren_EmptyElementName_001, TestSize.Level1)
{
    std::string mpd = "<?xml version=\"1.0\" ?>\n"
        "<MPD xmlns=\"urn:mpeg:dash:schema:mpd:2011\" type=\"static\">\n"
        "  <Period>\n"
        "    <EventStream schemeIdUri=\"urn:scte:scte35:2013\" timescale=\"1000\">\n"
        "      <Event id=\"1\" presentationTime=\"1000\">\n"
        "        <SpliceInfoSection xmlns=\"test\">\n"
        "          <SpliceInsert/>\n"
        "        </SpliceInfoSection>\n"
        "      </Event>\n"
        "    </EventStream>\n"
        "  </Period>\n"
        "</MPD>";
    auto mpdParser = std::make_shared<DashMpdParser>();
    mpdParser->ParseMPD(mpd.c_str(), mpd.length());
    DashMpdInfo *mpdInfo{nullptr};
    mpdParser->GetMPD(mpdInfo);
    ASSERT_NE(mpdInfo, nullptr);
    auto periodInfo = mpdInfo->periodInfoList_.front();
    auto eventStreamInfo = periodInfo->eventStreamList_.front();
    auto eventInfo = eventStreamInfo->eventList_.front();
    ASSERT_NE(eventInfo, nullptr);
    ASSERT_FALSE(eventInfo->contents_.empty());
}

HWTEST_F(DashMpdParserUnitTest, Test_FillEventMetadata_ZeroTimeValues_001, TestSize.Level1)
{
    std::string mpd = "<?xml version=\"1.0\" ?>\n"
        "<MPD xmlns=\"urn:mpeg:dash:schema:mpd:2011\" type=\"static\">\n"
        "  <Period>\n"
        "    <EventStream schemeIdUri=\"urn:test:event\" value=\"test\" timescale=\"1000\">\n"
        "      <Event id=\"1\" duration=\"0\" presentationTime=\"0\" value=\"event1\"/>\n"
        "    </EventStream>\n"
        "  </Period>\n"
        "</MPD>";
    auto mpdParser = std::make_shared<DashMpdParser>();
    mpdParser->ParseMPD(mpd.c_str(), mpd.length());
    DashMpdInfo *mpdInfo{nullptr};
    mpdParser->GetMPD(mpdInfo);
    ASSERT_NE(mpdInfo, nullptr);
    auto periodInfo = mpdInfo->periodInfoList_.front();
    ASSERT_NE(periodInfo, nullptr);
    ASSERT_FALSE(periodInfo->eventStreamList_.empty());
    auto eventStreamInfo = periodInfo->eventStreamList_.front();
    ASSERT_NE(eventStreamInfo, nullptr);
    ASSERT_FALSE(eventStreamInfo->eventList_.empty());
    auto eventInfo = eventStreamInfo->eventList_.front();
    ASSERT_NE(eventInfo, nullptr);
    EXPECT_EQ(eventInfo->start_, 0);
    EXPECT_EQ(eventInfo->duration_, 0);
}

HWTEST_F(DashMpdParserUnitTest, Test_FillEventMetadata_TimescaleZero_001, TestSize.Level1)
{
    std::string mpd = "<?xml version=\"1.0\" ?>\n"
        "<MPD xmlns=\"urn:mpeg:dash:schema:mpd:2011\" type=\"static\">\n"
        "  <Period>\n"
        "    <EventStream schemeIdUri=\"urn:test:event\" value=\"test\" timescale=\"0\"\n"
        "                   presentationTimeOffset=\"0\">\n"
        "      <Event id=\"1\" duration=\"5000\" presentationTime=\"1000\" value=\"event1\"/>\n"
        "    </EventStream>\n"
        "  </Period>\n"
        "</MPD>";
    auto mpdParser = std::make_shared<DashMpdParser>();
    mpdParser->ParseMPD(mpd.c_str(), mpd.length());
    DashMpdInfo *mpdInfo{nullptr};
    mpdParser->GetMPD(mpdInfo);
    ASSERT_NE(mpdInfo, nullptr);
    auto periodInfo = mpdInfo->periodInfoList_.front();
    ASSERT_NE(periodInfo, nullptr);
    ASSERT_FALSE(periodInfo->eventStreamList_.empty());
    auto eventStreamInfo = periodInfo->eventStreamList_.front();
    ASSERT_NE(eventStreamInfo, nullptr);
    ASSERT_FALSE(eventStreamInfo->eventList_.empty());
    auto eventInfo = eventStreamInfo->eventList_.front();
    ASSERT_NE(eventInfo, nullptr);
    EXPECT_EQ(eventInfo->start_, 0);
    EXPECT_EQ(eventInfo->duration_, 0);
}

HWTEST_F(DashMpdParserUnitTest, Test_FillEventMetadata_TimescaleGreaterThanZero_001, TestSize.Level1)
{
    std::string mpd = "<?xml version=\"1.0\" ?>\n"
        "<MPD xmlns=\"urn:mpeg:dash:schema:mpd:2011\" type=\"static\">\n"
        "  <Period>\n"
        "    <EventStream schemeIdUri=\"urn:test:event\" value=\"val\" timescale=\"1000\"\n"
        "                   presentationTimeOffset=\"100\">\n"
        "      <Event id=\"1\" duration=\"2000\" presentationTime=\"1000\" value=\"event1\"/>\n"
        "    </EventStream>\n"
        "  </Period>\n"
        "</MPD>";
    auto mpdParser = std::make_shared<DashMpdParser>();
    mpdParser->ParseMPD(mpd.c_str(), mpd.length());
    DashMpdInfo *mpdInfo{nullptr};
    mpdParser->GetMPD(mpdInfo);
    ASSERT_NE(mpdInfo, nullptr);
    auto periodInfo = mpdInfo->periodInfoList_.front();
    ASSERT_NE(periodInfo, nullptr);
    ASSERT_FALSE(periodInfo->eventStreamList_.empty());
    auto eventStreamInfo = periodInfo->eventStreamList_.front();
    ASSERT_NE(eventStreamInfo, nullptr);
    ASSERT_FALSE(eventStreamInfo->eventList_.empty());
    auto eventInfo = eventStreamInfo->eventList_.front();
    ASSERT_NE(eventInfo, nullptr);
    EXPECT_GE(eventInfo->start_, 0);
    EXPECT_GT(eventInfo->duration_, 0);
    EXPECT_EQ(eventInfo->classify_, "urn:test:eventval");
}

HWTEST_F(DashMpdParserUnitTest, Test_ParseEventStream_WithPresentationTimeOffset_001, TestSize.Level1)
{
    std::string mpd = "<?xml version=\"1.0\" ?>\n"
        "<MPD xmlns=\"urn:mpeg:dash:schema:mpd:2011\" type=\"static\">\n"
        "  <Period>\n"
        "    <EventStream schemeIdUri=\"urn:test:event\" value=\"test\" timescale=\"1000\"\n"
        "                   presentationTimeOffset=\"100\">\n"
        "      <Event id=\"1\" duration=\"5000\" presentationTime=\"2000\" value=\"event1\"/>\n"
        "    </EventStream>\n"
        "  </Period>\n"
        "</MPD>";
    auto mpdParser = std::make_shared<DashMpdParser>();
    mpdParser->ParseMPD(mpd.c_str(), mpd.length());
    DashMpdInfo *mpdInfo{nullptr};
    mpdParser->GetMPD(mpdInfo);
    ASSERT_NE(mpdInfo, nullptr);
    auto periodInfo = mpdInfo->periodInfoList_.front();
    ASSERT_NE(periodInfo, nullptr);
    auto eventStreamInfo = periodInfo->eventStreamList_.front();
    ASSERT_NE(eventStreamInfo, nullptr);
    auto eventInfo = eventStreamInfo->eventList_.front();
    ASSERT_NE(eventInfo, nullptr);
    EXPECT_EQ(eventStreamInfo->presentationTimeOffset_, 100u);
}

HWTEST_F(DashMpdParserUnitTest, Test_ParseEventStream_BaseURLOnly_001, TestSize.Level1)
{
    std::string mpd = "<?xml version=\"1.0\" ?>\n"
        "<MPD xmlns=\"urn:mpeg:dash:schema:mpd:2011\" type=\"static\">\n"
        "  <Period>\n"
        "    <EventStream schemeIdUri=\"urn:test:event\" value=\"test\" timescale=\"1000\">\n"
        "      <BaseURL>http://example.com/event</BaseURL>\n"
        "    </EventStream>\n"
        "  </Period>\n"
        "</MPD>";
    auto mpdParser = std::make_shared<DashMpdParser>();
    mpdParser->ParseMPD(mpd.c_str(), mpd.length());
    DashMpdInfo *mpdInfo{nullptr};
    mpdParser->GetMPD(mpdInfo);
    ASSERT_NE(mpdInfo, nullptr);
    auto periodInfo = mpdInfo->periodInfoList_.front();
    ASSERT_NE(periodInfo, nullptr);
    ASSERT_FALSE(periodInfo->eventStreamList_.empty());
    auto eventStreamInfo = periodInfo->eventStreamList_.front();
    ASSERT_NE(eventStreamInfo, nullptr);
    EXPECT_FALSE(eventStreamInfo->baseUrl_.empty());
    EXPECT_EQ(eventStreamInfo->baseUrl_.front(), "http://example.com/event");
    EXPECT_TRUE(eventStreamInfo->eventList_.empty());
}

HWTEST_F(DashMpdParserUnitTest, Test_ParseSpliceInfoSection_WithEmptyChildren_001, TestSize.Level1)
{
    std::string mpd = "<?xml version=\"1.0\" ?>\n"
        "<MPD xmlns=\"urn:mpeg:dash:schema:mpd:2011\" type=\"static\">\n"
        "  <Period>\n"
        "    <EventStream schemeIdUri=\"urn:scte:scte35:2013\" timescale=\"1000\">\n"
        "      <Event id=\"1\" presentationTime=\"1000\">\n"
        "        <SpliceInfoSection xmlns=\"urn:scte\">\n"
        "        </SpliceInfoSection>\n"
        "      </Event>\n"
        "    </EventStream>\n"
        "  </Period>\n"
        "</MPD>";
    auto mpdParser = std::make_shared<DashMpdParser>();
    mpdParser->ParseMPD(mpd.c_str(), mpd.length());
    DashMpdInfo *mpdInfo{nullptr};
    mpdParser->GetMPD(mpdInfo);
    ASSERT_NE(mpdInfo, nullptr);
    auto periodInfo = mpdInfo->periodInfoList_.front();
    auto eventStreamInfo = periodInfo->eventStreamList_.front();
    auto eventInfo = eventStreamInfo->eventList_.front();
    ASSERT_NE(eventInfo, nullptr);
    EXPECT_TRUE(eventInfo->contents_.find("SpliceInfoSection") != eventInfo->contents_.end());
}

HWTEST_F(DashMpdParserUnitTest, Test_ParseSpliceInsert_OutOfNetworkIndicator_001, TestSize.Level1)
{
    std::string mpd = "<?xml version=\"1.0\" ?>\n"
        "<MPD xmlns=\"urn:mpeg:dash:schema:mpd:2011\" type=\"static\">\n"
        "  <Period>\n"
        "    <EventStream schemeIdUri=\"urn:scte:scte35:2013\" timescale=\"1000\">\n"
        "      <Event id=\"1\">\n"
        "        <SpliceInfoSection xmlns=\"urn:scte\">\n"
        "          <SpliceInsert outOfNetworkIndicator=\"false\" spliceImmediateFlag=\"true\">\n"
        "            <BreakDuration duration=\"3000\" autoReturn=\"true\"/>\n"
        "          </SpliceInsert>\n"
        "        </SpliceInfoSection>\n"
        "      </Event>\n"
        "    </EventStream>\n"
        "  </Period>\n"
        "</MPD>";
    auto mpdParser = std::make_shared<DashMpdParser>();
    mpdParser->ParseMPD(mpd.c_str(), mpd.length());
    DashMpdInfo *mpdInfo{nullptr};
    mpdParser->GetMPD(mpdInfo);
    ASSERT_NE(mpdInfo, nullptr);
    auto periodInfo = mpdInfo->periodInfoList_.front();
    auto eventStreamInfo = periodInfo->eventStreamList_.front();
    auto eventInfo = eventStreamInfo->eventList_.front();
    ASSERT_NE(eventInfo, nullptr);
    auto it = eventInfo->contents_.find("SpliceInsert");
    ASSERT_NE(it, eventInfo->contents_.end());
    EXPECT_TRUE(it->second.find("outOfNetworkIndicator=false") != std::string::npos);
    EXPECT_TRUE(it->second.find("spliceImmediateFlag=true") != std::string::npos);
}

HWTEST_F(DashMpdParserUnitTest, Test_ParseBreakDuration_EmptyChildElement_001, TestSize.Level1)
{
    std::string mpd = "<?xml version=\"1.0\" ?>\n"
        "<MPD xmlns=\"urn:mpeg:dash:schema:mpd:2011\" type=\"static\">\n"
        "  <Period>\n"
        "    <EventStream schemeIdUri=\"urn:scte:scte35:2013\" timescale=\"1000\">\n"
        "      <Event id=\"1\">\n"
        "        <SpliceInfoSection xmlns=\"urn:scte\">\n"
        "          <SpliceInsert outOfNetworkIndicator=\"1\">\n"
        "            <BreakDuration duration=\"0\" autoReturn=\"false\">\n"
        "              <ChildElement/>\n"
        "            </BreakDuration>\n"
        "          </SpliceInsert>\n"
        "        </SpliceInfoSection>\n"
        "      </Event>\n"
        "    </EventStream>\n"
        "  </Period>\n"
        "</MPD>";
    auto mpdParser = std::make_shared<DashMpdParser>();
    mpdParser->ParseMPD(mpd.c_str(), mpd.length());
    DashMpdInfo *mpdInfo{nullptr};
    mpdParser->GetMPD(mpdInfo);
    ASSERT_NE(mpdInfo, nullptr);
    auto periodInfo = mpdInfo->periodInfoList_.front();
    auto eventStreamInfo = periodInfo->eventStreamList_.front();
    auto eventInfo = eventStreamInfo->eventList_.front();
    ASSERT_NE(eventInfo, nullptr);
    auto it = eventInfo->contents_.find("BreakDuration");
    ASSERT_NE(it, eventInfo->contents_.end());
    EXPECT_TRUE(it->second.find("duration=0") != std::string::npos);
    EXPECT_TRUE(it->second.find("autoReturn=false") != std::string::npos);
}

HWTEST_F(DashMpdParserUnitTest, Test_ParseEvent_WithoutDuration_001, TestSize.Level1)
{
    std::string mpd = "<?xml version=\"1.0\" ?>\n"
        "<MPD xmlns=\"urn:mpeg:dash:schema:mpd:2011\" type=\"static\">\n"
        "  <Period>\n"
        "    <EventStream schemeIdUri=\"urn:test:event\" timescale=\"1000\">\n"
        "      <Event id=\"evt1\" presentationTime=\"100\" value=\"data\"/>\n"
        "    </EventStream>\n"
        "  </Period>\n"
        "</MPD>";
    auto mpdParser = std::make_shared<DashMpdParser>();
    mpdParser->ParseMPD(mpd.c_str(), mpd.length());
    DashMpdInfo *mpdInfo{nullptr};
    mpdParser->GetMPD(mpdInfo);
    ASSERT_NE(mpdInfo, nullptr);
    auto periodInfo = mpdInfo->periodInfoList_.front();
    auto eventStreamInfo = periodInfo->eventStreamList_.front();
    auto eventInfo = eventStreamInfo->eventList_.front();
    ASSERT_NE(eventInfo, nullptr);
    EXPECT_EQ(eventInfo->id_, "evt1");
    EXPECT_EQ(eventInfo->value_, "data");
}
}
}
}
}