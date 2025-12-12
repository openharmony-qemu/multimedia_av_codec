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
#include <atomic>
#include <iostream>
#include <fstream>
#include <queue>
#include <string>
#include <thread>
#include "dash_media_downloader.h"
#include "dash_descriptor_node.h"
#include "dash_adpt_set_manager.h"
#include "dash_mpd_manager.h"
#include "dash_mpd_node.h"
#include "dash_period_node.h"
#include "dash_representation_node.h"
#include "dash_seg_base_node.h"
#include "dash_url_type_node.h"
#include <fuzzer/FuzzedDataProvider.h>
#include "http_server_demo.h"
#include "http_server_mock.h"
#include "test_template.h"
#define FUZZ_PROJECT_NAME "dashmediadownseektotime_fuzzer"
using namespace std;
using namespace OHOS::Media;
using namespace OHOS::Media::Plugins::HttpPlugin;
using namespace OHOS;
namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {

namespace {
static const std::string MPD_MULTI_AUDIO_SUB = "http://127.0.0.1:46666/test_dash/segment_base/index_audio_subtitle.mpd";
constexpr int32_t WAIT_FOR_SIDX_TIME = 1000 * 1000;
}

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

void RunMpdDownloaderTests(DashMpdInfo *mpdInfo)
{
    std::shared_ptr<DashMpdDownloader> mpdMpddownload = std::make_shared<DashMpdDownloader>();
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
        int32_t flag = GetData<int32_t>();
        periodMr->GetInitSegment(flag);
        periodMr->GetPeriod();
        periodMr->GetPreviousPeriod();
    }
    int streamId = GetData<int>();
    std::shared_ptr<DashMpdBitrateParam> paramNext = std::make_shared<DashMpdBitrateParam>();
    mpdMpddownload->GetNextVideoStream(*paramNext, streamId);
    mpdMpddownload->GetContentType();
    mpdMpddownload->GetUrl();
    bool isInterruptNeeded = GetData<bool>();
    mpdMpddownload->SetInterruptState(isInterruptNeeded);
    unsigned int width = GetData<unsigned int>();
    unsigned int height = GetData<unsigned int>();
    mpdMpddownload->SetInitResolution(width, height);
    bool isHdrStart = GetData<bool>();
    mpdMpddownload->SetHdrStart(isHdrStart);
    std::shared_ptr<DashStreamDescription> streamDesc = std::make_shared<DashStreamDescription>();
    uint32_t nextSegTime = GetData<uint32_t>();
    mpdMpddownload->UpdateCurrentNumberSeqByTime(streamDesc, nextSegTime);
    mpdMpddownload->GetInitSegmentByStreamId(streamId);
    mpdMpddownload->GetUsingStreamByType(MediaAVCodec::MediaType::MEDIA_TYPE_AUD);
    mpdMpddownload->GetUsingStreamByType(MediaAVCodec::MediaType::MEDIA_TYPE_VID);
    mpdMpddownload->GetUsingStreamByType(MediaAVCodec::MediaType::MEDIA_TYPE_SUBTITLE);
    mpdMpddownload->GetUsingStreamByType(MediaAVCodec::MediaType::MEDIA_TYPE_TIMED_METADATA);
    mpdMpddownload->GetStreamByStreamId(streamId);
    mpdMpddownload->GetInUseVideoStreamId();
    int64_t seekTime = GetData<int64_t>();
    std::shared_ptr<DashSegment> seg = nullptr;
    mpdMpddownload->SeekToTs(streamId, seekTime, seg);
    usleep(WAIT_FOR_SIDX_TIME);
    mpdMpddownload->Close(false);
    mpdMpddownload = nullptr;
}

bool DashMediaFuzzerTest(const uint8_t *data, size_t size)
{
    std::string mpd = BASE_MPD;
    std::shared_ptr<DashMpdParser> mpdParser = std::make_shared<DashMpdParser>();
    mpdParser->ParseMPD(mpd.c_str(), mpd.length());

    DashMpdInfo *mpdInfo{nullptr};
    mpdParser->GetMPD(mpdInfo);

    if (mpdInfo != nullptr) {
        RunMpdDownloaderTests(mpdInfo);
    }

    usleep(WAIT_FOR_SIDX_TIME);
    return true;
}

bool DashAdptRun(const uint8_t *data, size_t size)
{
    if (size <= sizeof(int64_t)) {
        return false;
    }
    std::shared_ptr<DashAdptSetManager> mpdMpddownload = std::make_shared<DashAdptSetManager>();
    std::list<std::string> baseUrlList;
    mpdMpddownload->GetBaseUrlList(baseUrlList);
    mpdMpddownload->Reset();
    int32_t flag = *reinterpret_cast<const int32_t *>(data);
    mpdMpddownload->GetInitSegment(flag);
    uint32_t bandwidth = *reinterpret_cast<const uint32_t *>(data);
    mpdMpddownload->GetRepresentationByBandwidth(bandwidth);
    mpdMpddownload->GetHighRepresentation();
    mpdMpddownload->GetLowRepresentation();
    mpdMpddownload->GetMime();
    mpdMpddownload->IsOnDemand();
    return true;
}

bool DashDescriptorNodeRun(const uint8_t *data, size_t size)
{
    if (size <= sizeof(int64_t)) {
        return false;
    }
    std::shared_ptr<DashDescriptorNode> mpdMpddownload = std::make_shared<DashDescriptorNode>();
    std::string addr = "hev1.1.6.L93.90";
    uint32_t uiAttrVal = *reinterpret_cast<const uint32_t *>(data);
    int32_t iAttrVal = *reinterpret_cast<const int32_t *>(data);
    uint64_t ullAttrVal = *reinterpret_cast<const uint64_t *>(data);
    double dAttrVal = *reinterpret_cast<const double *>(data);
    mpdMpddownload->GetAttr(addr, uiAttrVal);
    mpdMpddownload->GetAttr(addr, iAttrVal);
    mpdMpddownload->GetAttr(addr, ullAttrVal);
    mpdMpddownload->GetAttr(addr, dAttrVal);
    return true;
}

bool MpdMangerRun(const uint8_t *data, size_t size)
{
    if (size <= sizeof(int64_t)) {
        return false;
    }
    std::shared_ptr<DashMpdManager> mpdMpddownload = std::make_shared<DashMpdManager>();
    mpdMpddownload->Reset();
    DashMpdInfo *mangerInfo = new DashMpdInfo;
    DashPeriodInfo *periodInfo = nullptr;
    mangerInfo->periodInfoList_.push_back(periodInfo);
    std::string mpdUrl = MPD_MULTI_AUDIO_SUB;
    mpdMpddownload->SetMpdInfo(mangerInfo, mpdUrl);
    mpdMpddownload->GetNextPeriod(periodInfo);
    mpdMpddownload->GetMpdInfo();
    mpdMpddownload->GetFirstPeriod();
    mpdMpddownload->GetBaseUrl();
    return true;
}

bool DashMpdNodeRun(const uint8_t *data, size_t size)
{
    if (size <= sizeof(int64_t)) {
        return false;
    }
    std::shared_ptr<DashMpdNode> mpdMpddownload = std::make_shared<DashMpdNode>();
    std::string addr = "hev1.1.6.L93.90";
    int32_t iAttrVal = *reinterpret_cast<const int32_t *>(data);
    uint64_t ullAttrVal = *reinterpret_cast<const uint64_t *>(data);
    double dAttrVal = *reinterpret_cast<const double *>(data);
    mpdMpddownload->GetAttr(addr, iAttrVal);
    mpdMpddownload->GetAttr(addr, ullAttrVal);
    mpdMpddownload->GetAttr(addr, dAttrVal);
    std::shared_ptr<DashPeriodManager> dashPeriodManager = std::make_shared<DashPeriodManager>();
    dashPeriodManager->Empty();
    return true;
}

bool DashPeriodNodeRun(const uint8_t *data, size_t size)
{
    if (size <= sizeof(int64_t)) {
        return false;
    }
    std::shared_ptr<DashPeriodNode> mpdMpddownload = std::make_shared<DashPeriodNode>();
    std::string addr = "hev1.1.6.L93.90";
    uint32_t uiAttrVal = *reinterpret_cast<const uint32_t *>(data);
    int32_t iAttrVal = *reinterpret_cast<const int32_t *>(data);
    uint64_t ullAttrVal = *reinterpret_cast<const uint64_t *>(data);
    double dAttrVal = *reinterpret_cast<const double *>(data);
    mpdMpddownload->GetAttr(addr, uiAttrVal);
    mpdMpddownload->GetAttr(addr, iAttrVal);
    mpdMpddownload->GetAttr(addr, ullAttrVal);
    mpdMpddownload->GetAttr(addr, dAttrVal);
    return true;
}

bool DashRepresenRun(const uint8_t *data, size_t size)
{
    if (size <= sizeof(int64_t)) {
        return false;
    }
    std::shared_ptr<DashRepresentationManager> mpdMpddownload = std::make_shared<DashRepresentationManager>();
    int32_t flag = *reinterpret_cast<const int32_t *>(data);
    mpdMpddownload->GetInitSegment(flag);
    mpdMpddownload->Reset();
    mpdMpddownload->GetRepresentationInfo();
    mpdMpddownload->GetPreviousRepresentationInfo();
    return true;
}

bool DashRepresentationNodeRun(const uint8_t *data, size_t size)
{
    if (size <= sizeof(int64_t)) {
        return false;
    }
    std::shared_ptr<DashRepresentationNode> mpdMpddownload = std::make_shared<DashRepresentationNode>();
    std::string addr = "hev1.1.6.L93.90";
    uint32_t uiAttrVal = *reinterpret_cast<const uint32_t *>(data);
    int32_t iAttrVal = *reinterpret_cast<const int32_t *>(data);
    uint64_t ullAttrVal = *reinterpret_cast<const uint64_t *>(data);
    double dAttrVal = *reinterpret_cast<const double *>(data);
    mpdMpddownload->GetAttr(addr, uiAttrVal);
    mpdMpddownload->GetAttr(addr, iAttrVal);
    mpdMpddownload->GetAttr(addr, ullAttrVal);
    mpdMpddownload->GetAttr(addr, dAttrVal);
    return true;
}

bool DashSegBaseNodeRun(const uint8_t *data, size_t size)
{
    if (size <= sizeof(int64_t)) {
        return false;
    }
    std::shared_ptr<DashSegBaseNode> mpdMpddownload = std::make_shared<DashSegBaseNode>();
    std::string addr = "hev1.1.6.L93.90";
    uint32_t uiAttrVal = *reinterpret_cast<const uint32_t *>(data);
    int32_t iAttrVal = *reinterpret_cast<const int32_t *>(data);
    uint64_t ullAttrVal = *reinterpret_cast<const uint64_t *>(data);
    double dAttrVal = *reinterpret_cast<const double *>(data);
    mpdMpddownload->GetAttr(addr, uiAttrVal);
    mpdMpddownload->GetAttr(addr, iAttrVal);
    mpdMpddownload->GetAttr(addr, ullAttrVal);
    mpdMpddownload->GetAttr(addr, dAttrVal);
    return true;
}

bool DashUrlTypeNodeRun(const uint8_t *data, size_t size)
{
    if (size <= sizeof(int64_t)) {
        return false;
    }
    std::shared_ptr<DashUrlTypeNode> mpdMpddownload = std::make_shared<DashUrlTypeNode>();
    std::string addr = "hev1.1.6.L93.90";
    uint32_t uiAttrVal = *reinterpret_cast<const uint32_t *>(data);
    int32_t iAttrVal = *reinterpret_cast<const int32_t *>(data);
    uint64_t ullAttrVal = *reinterpret_cast<const uint64_t *>(data);
    double dAttrVal = *reinterpret_cast<const double *>(data);
    mpdMpddownload->GetAttr(addr, uiAttrVal);
    mpdMpddownload->GetAttr(addr, iAttrVal);
    mpdMpddownload->GetAttr(addr, ullAttrVal);
    mpdMpddownload->GetAttr(addr, dAttrVal);
    return true;
}
}
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    if (!InitServer()) {
        cout << "Init server error" << endl;
        return -1;
    }
    OHOS::Media::Plugins::DashMediaFuzzerTest(data, size);
    OHOS::Media::Plugins::DashAdptRun(data, size);
    OHOS::Media::Plugins::DashDescriptorNodeRun(data, size);
    OHOS::Media::Plugins::MpdMangerRun(data, size);
    OHOS::Media::Plugins::DashMpdNodeRun(data, size);
    OHOS::Media::Plugins::DashPeriodNodeRun(data, size);
    OHOS::Media::Plugins::DashRepresenRun(data, size);
    OHOS::Media::Plugins::DashRepresentationNodeRun(data, size);
    if (!CloseServer()) {
        cout << "Close server error" << endl;
        return -1;
    }
    return 0;
}