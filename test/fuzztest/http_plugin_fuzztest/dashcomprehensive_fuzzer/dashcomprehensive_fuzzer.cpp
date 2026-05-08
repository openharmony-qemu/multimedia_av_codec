/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#include <atomic>
#include <iostream>
#include <fstream>
#include <queue>
#include <string>
#include <thread>
#include <vector>
#include <list>
#include "dash_adpt_set_manager.h"
#include "dash_mpd_manager.h"
#include "dash_period_manager.h"
#include "dash_mpd_parser.h"
#include "dash_segment_downloader.h"
#include "i_dash_mpd_node.h"
#include "base64_utils.h"
#include "dash_media_downloader.h"

#define FUZZ_PROJECT_NAME "dash_comprehensive_fuzzer"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
namespace {
// 测试用的MPD数据 - SegmentTemplate
static const std::string MPD_SEGMENT_TEMPLATE = R"(<?xml version="1.0" ?>
<MPD xmlns="urn:mpeg:dash:schema:mpd:2011" profiles="urn:mpeg:dash:profile:isoff-on-demand:2011" minBufferTime="PT2.00S" mediaPresentationDuration="PT0H1M1S" type="static">
  <!-- Created with Bento4 mp4-dash.py, VERSION=1.7.0-613 -->
  <Period>
    <SegmentBase timescale="1000"/>
    <!-- Video -->
    <AdaptationSet maxPlayoutRate="1.0" segmentAlignment="true" startWithSAP="1" minWidth="854" maxWidth="2560" minHeight="480" maxHeight="1440">
      <ContentComponent id="0" contentType="video" myContentComponentAttriubute="contentcomponent">
        <Accessibility>
          <NestedDescriptorElement>### Nested DescriptorTestText ###</NestedDescriptorElement>
        </Accessibility>
        <Role schemeIdUri="urn:mpeg:dash:role:2011" value="main"></Role>
        <Rating></Rating>
        <Viewpoint></Viewpoint>
        <ContentComponentElement>### ContentComponentTestText ###</ContentComponentElement>
      </ContentComponent>
      <ContentComponent id="1" contentType="audio" myContentComponentAttriubute="contentcomponent">
        <Accessibility>
          <NestedDescriptorElement>### Nested DescriptorTestText ###</NestedDescriptorElement>
        </Accessibility>
        <Role schemeIdUri="urn:mpeg:dash:role:2011" value="main"></Role>
        <Rating></Rating>
        <Viewpoint></Viewpoint>
        <ContentComponentElement>### ContentComponentTestText ###</ContentComponentElement>
      </ContentComponent>
      <ContentComponent id="2" contentType="text" myContentComponentAttriubute="contentcomponent">
        <Accessibility>
          <NestedDescriptorElement>### Nested DescriptorTestText ###</NestedDescriptorElement>
        </Accessibility>
        <Role schemeIdUri="urn:mpeg:dash:role:2011" value="main"></Role>
        <Rating></Rating>
        <Viewpoint></Viewpoint>
        <ContentComponentElement>### ContentComponentTestText ###</ContentComponentElement>
      </ContentComponent>
      <ContentProtection value="cbcs" schemeIdUri="urn:mpeg:dash:mp4protection:2011" cenc:default_KID="5544524d-5f54-4553-545f-4349445f3052"/>
        <ContentProtection schemeIdUri="urn:uuid:3d5e6d35-9b9a-41e8-b843-dd3c6e72c42c">
        <cenc:pssh>AAAAX3Bzc2gAAAAAPV5tNZuaQei4Q908bnLELAAAAD97InZlcnNpb24iOiJWMS4wIiwiY29udGVudElEIjoiTWpZMExXTmlZM009IiwiZW5zY2hlbWEiOiJjYmNzIn0=</cenc:pssh>
      </ContentProtection>
      <ContentProtection schemeIdUri="urn:uuid:1077efec-c0b2-4d02-ace3-3c1e52e2fb4b1">
        <cenc:pssh></cenc:pssh>
      </ContentProtection>
      <ContentProtection schemeIdUri="urn:uuid:1077efec-c0b2-4d02-ace3-3c1e52e2fb4b2">
        <cenc:pssh>A</cenc:pssh>
      </ContentProtection>
      <ContentProtection schemeIdUri="urn:uuid:1077efec-c0b2-4d02-ace3-3c1e52e2fb4b3">
        <cenc:pssh>AAAA3</cenc:pssh>
      </ContentProtection>
      <Representation id="video-1" codecs="hev1.1.6.L90.90" width="854" height="480" scanType="progressive" frameRate="30" bandwidth="535988">
        <BaseURL>media-video-1.mp4</BaseURL>
        <SegmentBase indexRange="3312-3715">
          <Initialization range="0-3311"/>
        </SegmentBase>
      </Representation>
      <Representation id="video-2" codecs="hev1.1.6.L93.90" width="1280" height="720" scanType="progressive" frameRate="30" bandwidth="797947">
        <BaseURL>media-video-2.mp4</BaseURL>
        <SegmentBase indexRange="3273-3676">
          <Initialization range="0-3272"/>
        </SegmentBase>
      </Representation>
    </AdaptationSet>
    <AdaptationSet mimeType="video/mp4" segmentAlignment="true" startWithSAP="1" minWidth="854" maxWidth="2560" minHeight="480" maxHeight="1440">
      <ContentProtection value="cbcs" schemeIdUri="urn:mpeg:dash:mp4protection:2011" cenc:default_KID="5544524d-5f54-4553-545f-4349445f3052"/>
        <ContentProtection schemeIdUri="urn:uuid:3d5e6d35-9b9a-41e8-b843-dd3c6e72c42c">
        <cenc:pssh>AAAAX3Bzc2gAAAAAPV5tNZuaQei4Q908bnLELAAAAD97InZlcnNpb24iOiJWMS4wIiwiY29udGVudElEIjoiTWpZMExXTmlZM009IiwiZW5zY2hlbWEiOiJjYmNzIn0=</cenc:pssh>
      </ContentProtection>
      <Representation id="video-1" codecs="hev1.1.6.L90.90" width="854" height="480" scanType="progressive" frameRate="30" bandwidth="535988" cuvvVersion="cuvv.110">
        <BaseURL>media-video-1.mp4</BaseURL>
        <SegmentBase indexRange="3312-3715">
          <Initialization range="0-3311"/>
        </SegmentBase>
      </Representation>
      <Representation id="video-2" codecs="hev1.1.6.L93.90" width="1280" height="720" scanType="progressive" frameRate="30" bandwidth="797947" cuvvVersion="cuvv.110">
        <BaseURL>media-video-2.mp4</BaseURL>
        <SegmentBase indexRange="3273-3676">
          <Initialization range="0-3272"/>
        </SegmentBase>
      </Representation>
      <EssentialProperty schemeIdUri="urn:mpeg:mpegB:cicp:ColourPrimaries" value="9"/>
      <EssentialProperty schemeIdUri="urn:mpeg:mpegB:cicp:MatrixCoefficients" value="9"/>
      <EssentialProperty schemeIdUri="urn:mpeg:mpegB:cicp:TransferCharacteristics" value="16"/>
    </AdaptationSet>
    <!-- Audio -->
    <AdaptationSet startWithSAP="1" segmentAlignment="true">
      <Representation mimeType="audio/mp4" id="audio-und-mp4a-1" codecs="mp4a.40.5" bandwidth="294135" audioSamplingRate="48000" volumeAdjust="original">
        <AudioChannelConfiguration schemeIdUri="urn:mpeg:dash:23003:3:audio_channel_configuration:2011" value="2"/>
        <BaseURL>media-audio-und-mp4a.mp4</BaseURL>
        <SegmentBase indexRange="662-1065">
          <Initialization range="0-661"/>
        </SegmentBase>
      </Representation>
      <Representation mimeType="audio/mp4" id="audio-und-mp4a-2" codecs="mp4a.40.5" bandwidth="294135" audioSamplingRate="48000" volumeAdjust="up">
        <AudioChannelConfiguration schemeIdUri="urn:mpeg:dash:23003:3:audio_channel_configuration:2011" value="2"/>
        <BaseURL>media-audio-und-mp4a.mp4</BaseURL>
        <SegmentBase indexRange="662-1065">
          <Initialization range="0-661"/>
        </SegmentBase>
      </Representation>
    </AdaptationSet>
  </Period>
</MPD>
)";

static const std::string MPD_SEGMENT_BASE = R"(<?xml version="1.0" ?>
<MPD xmlns="urn:mpeg:dash:schema:mpd:2011" profiles="urn:mpeg:dash:profile:isoff-live:2011" minBufferTime="PT2.00S" mediaPresentationDuration="PT8S" type="static">
    <!-- Created with Bento4 mp4-dash.py, VERSION=1.7.0-613 -->
    <Period>
        <SegmentList timescale="1000" duration="2000"/>
        <!-- Video -->
        <AdaptationSet mimeType="video/mp4" maxPlayoutRate="1.0" segmentAlignment="true" startWithSAP="1" minWidth="1280" maxWidth="2560" minHeight="720" maxHeight="1440">
            <BaseURL>video/</BaseURL>
            <Representation id="video/1" codecs="hev1.2.4.H120.90" width="1280" height="720" scanType="progressive" frameRate="30" bandwidth="1955284">
                <SegmentList timescale="0" duration="0">
                    <Initialization sourceURL="1/init.mp4"/>
                    <SegmentURL media="1/seg-1.m4s" TI="6.84" SI="35.15"/>
                    <SegmentURL media="1/seg-2.m4s" TI="15.14" SI="38.64"/>
                    <SegmentURL media="1/seg-3.m4s" TI="17.85" SI="44.73"/>
                    <SegmentURL media="1/seg-4.m4s" TI="12.88" SI="52.5"/>
                </SegmentList>
            </Representation>
        </AdaptationSet>
        <!-- Audio -->
        <AdaptationSet mimeType="audio/mp4" startWithSAP="1" segmentAlignment="true">
            <BaseURL>audio/</BaseURL>
            <SegmentList timescale="0" duration="0">
                <Initialization sourceURL="und/mp4a/init.mp4"/>
                <SegmentURL media="und/mp4a/seg-1.m4s"/>
                <SegmentURL media="und/mp4a/seg-2.m4s"/>
                <SegmentURL media="und/mp4a/seg-3.m4s"/>
                <SegmentURL media="und/mp4a/seg-4.m4s"/>
            </SegmentList>
            <Representation id="und/mp4a" codecs="mp4a.40.2" bandwidth="132195" audioSamplingRate="48000">
                <AudioChannelConfiguration schemeIdUri="urn:mpeg:dash:23003:3:audio_channel_configuration:2011" value="2"/>
            </Representation>
        </AdaptationSet>
    </Period>
</MPD>
)";

static const std::string MPD_SEGMENT_LIST = R"(<?xml version="1.0" ?>
<MPD xmlns="urn:mpeg:dash:schema:mpd:2011" profiles="urn:mpeg:dash:profile:isoff-live:2011" minBufferTime="PT2.00S" mediaPresentationDuration="PT8S" type="static">
    <!-- Created with Bento4 mp4-dash.py, VERSION=1.7.0-613 -->
    <Period>
        <SegmentTemplate timescale="1000" duration="2000">
            <SegmentTimeline>
                <S t="0" d="2000" />
                <S d="2000" />
                <S d="2000" />
                <S d="2000" />
            </SegmentTimeline>
        </SegmentTemplate>
        <!-- Video -->
        <AdaptationSet mimeType="video/mp4" maxPlayoutRate="1.0" segmentAlignment="true" startWithSAP="1" minWidth="1280" maxWidth="2560" minHeight="720" maxHeight="1440">
            <SegmentTemplate timescale="0" duration="0" initialization="$RepresentationID$/init.mp4" media="$RepresentationID$/seg-$Number%04d$.m4s" startNumber="1"/>
            <Representation id="video/1" codecs="hev1.2.4.H120.90" width="1280" height="720" scanType="progressive" frameRate="30" bandwidth="1955284"/>
        </AdaptationSet>
        <!-- Audio -->
        <AdaptationSet mimeType="audio/mp4" startWithSAP="1" segmentAlignment="true">
            <SegmentTemplate timescale="0" duration="0" initialization="$RepresentationID$/init.mp4" media="$RepresentationID$/seg-$Number%04d$.m4s" startNumber="1"/>
            <Representation id="audio/und/mp4a" codecs="mp4a.40.2" bandwidth="132195" audioSamplingRate="48000">
                <AudioChannelConfiguration schemeIdUri="urn:mpeg:dash:23003:3:audio_channel_configuration:2011" value="2"/>
            </Representation>
        </AdaptationSet>
    </Period>
</MPD>
)";

// 测试DashAdptSetManager
bool TestDashAdptSetManagerUncovered(const std::string &mpdContent)
{
    std::shared_ptr<DashMpdParser> mpdParser = std::make_shared<DashMpdParser>();
    mpdParser->ParseMPD(mpdContent.c_str(), mpdContent.length());

    DashMpdInfo *mpdInfo = nullptr;
    mpdParser->GetMPD(mpdInfo);
    if (mpdInfo == nullptr || mpdInfo->periodInfoList_.empty()) {
        return false;
    }
    DashPeriodInfo *periodInfo = mpdInfo->periodInfoList_.front();
    if (periodInfo == nullptr || periodInfo->adptSetList_.empty()) {
        return false;
    }
    DashAdptSetInfo *adptSetInfo = periodInfo->adptSetList_.front();
    if (adptSetInfo == nullptr) {
        return false;
    }

    DashAdptSetManager *adptSetManager = new DashAdptSetManager(adptSetInfo);
    if (adptSetManager == nullptr) {
        return false;
    }
    std::list<std::string> baseUrlList;
    adptSetManager->GetBaseUrlList(baseUrlList);

    DashVector<uint32_t> bandwidths;
    adptSetManager->GetBandwidths(bandwidths);
    int32_t flag = 0;
    adptSetManager->GetInitSegment(flag);
    adptSetManager->GetAdptSetInfo();
    adptSetManager->GetPreviousAdptSetInfo();

    if (!bandwidths.empty()) {
        uint32_t bandwidth = bandwidths[0];
        adptSetManager->GetRepresentationByBandwidth(bandwidth);
    }
    adptSetManager->GetHighRepresentation();
    adptSetManager->GetLowRepresentation();
    adptSetManager->GetMime();
    adptSetManager->IsOnDemand();
    adptSetManager->Reset();

    delete adptSetManager;
    return true;
}

bool TestDashMpdManagerUncovered(const std::string &mpdContent)
{
    std::shared_ptr<DashMpdParser> mpdParser = std::make_shared<DashMpdParser>();
    mpdParser->ParseMPD(mpdContent.c_str(), mpdContent.length());

    DashMpdInfo *mpdInfo = nullptr;
    mpdParser->GetMPD(mpdInfo);
    if (mpdInfo == nullptr) {
        return false;
    }
    DashMpdManager *mpdManager = new DashMpdManager(mpdInfo, "http://test.mpd");
    if (mpdManager == nullptr) {
        return false;
    }

    DashPeriodInfo *firstPeriod = mpdManager->GetFirstPeriod();
    if (firstPeriod != nullptr) {
        mpdManager->GetNextPeriod(firstPeriod);
    }

    DashMpdInfo *newMpdInfo = mpdInfo;
    mpdManager->Update(newMpdInfo);

    mpdInfo->mediaPresentationDuration_ = 0; // 设置为0以测试从Period计算
    uint32_t duration = 0;
    mpdManager->GetDuration(&duration);
    if (!mpdInfo->baseUrl_.empty()) {
        mpdInfo->baseUrl_.front() = "/relative/path";
        std::string baseUrl = mpdManager->GetBaseUrl();
    }
    mpdManager->Reset();

    delete mpdManager;
    return true;
}

bool TestDashPeriodManagerUncovered(const std::string &mpdContent)
{
    std::shared_ptr<DashMpdParser> mpdParser = std::make_shared<DashMpdParser>();
    mpdParser->ParseMPD(mpdContent.c_str(), mpdContent.length());

    DashMpdInfo *mpdInfo = nullptr;
    mpdParser->GetMPD(mpdInfo);
    if (mpdInfo == nullptr || mpdInfo->periodInfoList_.empty()) {
        return false;
    }

    DashPeriodInfo *periodInfo = mpdInfo->periodInfoList_.front();
    if (periodInfo == nullptr) {
        return false;
    }

    DashPeriodManager *periodManager = new DashPeriodManager(periodInfo);
    if (periodManager == nullptr) {
        return false;
    }

    periodManager->Empty();

    std::list<std::string> baseUrlList;
    periodManager->GetBaseUrlList(baseUrlList);

    int32_t segTmpltFlag = 0;
    periodManager->GetInitSegment(segTmpltFlag);
    periodManager->Reset();

    delete periodManager;
    return true;
}

bool TestDashSegmentDownloaderUncovered(const std::string &mpdUrl, const std::string &mpdContent)
{
    std::shared_ptr<DashMpdParser> mpdParser = std::make_shared<DashMpdParser>();
    mpdParser->ParseMPD(mpdContent.c_str(), mpdContent.length());

    DashMpdInfo *mpdInfo = nullptr;
    mpdParser->GetMPD(mpdInfo);
    if (mpdInfo == nullptr) {
        return false;
    }

    return true;
}

bool DashComprehensiveFuzzTest(const uint8_t *data, size_t size)
{
    if (data == nullptr || size < sizeof(int32_t)) {
        return false;
    }

    int32_t testType = *reinterpret_cast<const int32_t *>(data);
    testType = abs(testType) % 10; // 0-9 用于选择不同的测试
    TestDashMpdManagerUncovered(MPD_SEGMENT_TEMPLATE);
    bool result = false;

    switch (testType) {
        case 0:
            result = TestDashAdptSetManagerUncovered(MPD_SEGMENT_TEMPLATE);
            break;
        case 1:
            result = TestDashAdptSetManagerUncovered(MPD_SEGMENT_BASE);
            break;
        case 2:
            result = TestDashAdptSetManagerUncovered(MPD_SEGMENT_LIST);
            break;
        case 3:
            result = TestDashMpdManagerUncovered(MPD_SEGMENT_TEMPLATE);
            break;
        case 4:
            result = TestDashMpdManagerUncovered(MPD_SEGMENT_TEMPLATE);
            break;
        case 5:
            result = TestDashPeriodManagerUncovered(MPD_SEGMENT_TEMPLATE);
            break;
        case 6:
            result = TestDashPeriodManagerUncovered(MPD_SEGMENT_TEMPLATE);
            break;
        case 7:
            result = TestDashSegmentDownloaderUncovered("http://127.0.0.1:46666/test_dash/segment_template/index.mpd", MPD_SEGMENT_TEMPLATE);
            break;
        case 8:
            result = TestDashSegmentDownloaderUncovered("http://127.0.0.1:46666/test_dash/segment_base/index.mpd", MPD_SEGMENT_BASE);
            break;
        case 9:
            result = TestDashSegmentDownloaderUncovered("http://127.0.0.1:46666/test_dash/segment_list/index.mpd", MPD_SEGMENT_LIST);
            break;
        default:
            result = TestDashAdptSetManagerUncovered(MPD_SEGMENT_TEMPLATE) ||
                     TestDashMpdManagerUncovered(MPD_SEGMENT_LIST);
            break;
    }

    usleep(10 * 1000 * 1000);
    return result;
}
}
}
}
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::Media::Plugins::HttpPlugin::DashComprehensiveFuzzTest(data, size);
    return 0;
}