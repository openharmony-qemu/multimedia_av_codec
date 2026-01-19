/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
#include "mpd_parser/dash_mpd_parser.h"
#include "mpd_parser/dash_mpd_manager.h"
#include "mpd_parser/dash_period_manager.h"
#include "mpd_parser/dash_adpt_set_manager.h"
#include "mpd_parser/i_dash_mpd_node.h"
#include "base64_utils.h"
#include "dash_segment_downloader.h"
#define FUZZ_PROJECT_NAME "dashmpddownloader_fuzzer"

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
using namespace std;
using namespace OHOS::Media;

bool DashMpdDownloaderFuzzerTest(const uint8_t *data, size_t size)
{
    if (data == nullptr && size < sizeof(int32_t)) {
        return false;
    }
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
            std::shared_ptr<DashPeriodManager> periodMr = std::make_shared<DashPeriodManager>(first);
            DashList<DashAdptSetInfo*> adptSetList = first->adptSetList_;
            std::list<std::string> periodBaseUrlList;
            periodMr->GetBaseUrlList(periodBaseUrlList);
            int32_t flag = *reinterpret_cast<const int32_t *>(data);
            periodMr->GetInitSegment(flag);
            periodMr->GetPeriod();
            periodMr->GetPreviousPeriod();
        }
        return true;
    }
    return false;
}

}
}
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::Media::Plugins::HttpPlugin::DashMpdDownloaderFuzzerTest(data, size);
    return 0;
}