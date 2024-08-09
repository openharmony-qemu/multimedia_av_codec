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
#include "dash_segment_downloader.h"

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
    "segmentAlignment=\"true\" subsegmentAlignment=\"true\" subsegmentStartsWithSAP=\"1\" "
    "mimeType=\"video/mp4\" startWithSAP=\"1\">\n"
    "            <Representation id=\"5\" bandwidth=\"7342976\" width=\"1920\" "
    "height=\"1080\" codecs=\"avc1.640028\">\n"
    "                <BaseURL>2_video_1_1920X1080_6000_0_0.mp4</BaseURL>\n"
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
    "                <BaseURL>2_audio_6.mp4</BaseURL>\n"
    "                <SegmentBase timescale=\"44100\" indexRangeExact=\"true\" "
    "indexRange=\"756-1167\">\n"
    "                    <Initialization range=\"0-755\"/>\n"
    "                </SegmentBase>\n"
    "            </Representation>\n"
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
                      " startNumber=\"1\"/>\n"
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

}
}
}
}