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

#include "dash_mpd_downloader_unit_test.h"
#include <iostream>
#include "dash_mpd_downloader.h"
#include "utils/time_utils.h"
#include "http_server_demo.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
namespace {
constexpr int32_t SERVERPORT = 47777;
static const std::string MPD_SEGMENT_BASE = "http://127.0.0.1:47777/test_dash/segment_base/index.mpd";
static const std::string MPD_SEGMENT_LIST = "http://127.0.0.1:47777/test_dash/segment_list/index.mpd";
static const std::string MPD_SEGMENT_TEMPLATE = "http://127.0.0.1:47777/test_dash/segment_template/index.mpd";
static const std::string MPD_SEGMENT_LIST_TIMELINE = "http://127.0.0.1:47777/test_dash/segment_list/index_timeline.mpd";
static const std::string MPD_SEGMENT_TEMPLATE_ADPT = "http://127.0.0.1:47777/test_dash/segment_template/index_adpt.mpd";
static const std::string URL_TEMPLATE_TIMELINE = "http://127.0.0.1:47777/test_dash/segment_template/index_timeline.mpd";
static const std::string URL_SEGMENT_BASE_IN_PERIOD = "http://127.0.0.1:47777/test_dash/segment_base/index_period.mpd";
constexpr unsigned int INIT_WIDTH = 1280;
constexpr unsigned int INIT_HEIGHT = 720;
}

using namespace testing::ext;

std::unique_ptr<MediaAVCodec::HttpServerDemo> g_server = nullptr;
std::shared_ptr<DashMpdDownloader> g_mpdDownloader = nullptr;

void DashMpdDownloaderUnitTest::SetUpTestCase(void)
{
    g_server = std::make_unique<MediaAVCodec::HttpServerDemo>();
    g_server->StartServer(SERVERPORT);
    std::cout << "start" << std::endl;

    g_mpdDownloader = std::make_shared<DashMpdDownloader>();
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                              std::shared_ptr<DownloadRequest>& request) {};
    g_mpdDownloader->SetStatusCallback(statusCallback);
    g_mpdDownloader->SetInitResolution(INIT_WIDTH, INIT_HEIGHT);
    g_mpdDownloader->Open(MPD_SEGMENT_BASE);
}

void DashMpdDownloaderUnitTest::TearDownTestCase(void)
{
    g_mpdDownloader->SetInterruptState(true);
    g_mpdDownloader->Close(true);
    g_mpdDownloader = nullptr;
    g_server->StopServer();
    g_server = nullptr;
}

void DashMpdDownloaderUnitTest::SetUp(void)
{
    if (g_server == nullptr) {
        g_server = std::make_unique<MediaAVCodec::HttpServerDemo>();
        g_server->StartServer(SERVERPORT);
        std::cout << "start server" << std::endl;
    }
}

void DashMpdDownloaderUnitTest::TearDown(void) {}

HWTEST_F(DashMpdDownloaderUnitTest, TEST_OPEN, TestSize.Level1)
{
    std::string testUrl = MPD_SEGMENT_BASE;
    EXPECT_EQ(testUrl, g_mpdDownloader->GetUrl());
}

HWTEST_F(DashMpdDownloaderUnitTest, TEST_GET_SEEKABLE, TestSize.Level1)
{
    Seekable value = g_mpdDownloader->GetSeekable();
    EXPECT_EQ(Seekable::SEEKABLE, value);
}

HWTEST_F(DashMpdDownloaderUnitTest, TEST_GET_DURATION, TestSize.Level1)
{
    EXPECT_GE(g_mpdDownloader->GetDuration(), 0);
}

HWTEST_F(DashMpdDownloaderUnitTest, TEST_GET_BITRATES, TestSize.Level1)
{
    std::vector<uint32_t> bitrates = g_mpdDownloader->GetBitRates();

    EXPECT_GE(bitrates.size(), 0);
}

HWTEST_F(DashMpdDownloaderUnitTest, TEST_GET_BITRATES_BY_HDR, TestSize.Level1)
{
    std::vector<uint32_t> sdrBitrates = g_mpdDownloader->GetBitRatesByHdr(false);
    EXPECT_GE(sdrBitrates.size(), 0);

    std::vector<uint32_t> hdrBitrates = g_mpdDownloader->GetBitRatesByHdr(true);
    EXPECT_GE(hdrBitrates.size(), 0);
}

HWTEST_F(DashMpdDownloaderUnitTest, TEST_GET_STREAM_INFO, TestSize.Level1)
{
    std::vector<StreamInfo> streams;
    Status status = g_mpdDownloader->GetStreamInfo(streams);

    EXPECT_GE(streams.size(), 0);
    EXPECT_EQ(status, Status::OK);
}

HWTEST_F(DashMpdDownloaderUnitTest, TEST_IS_BITRATE_SAME, TestSize.Level1)
{
    int usingStreamId = g_mpdDownloader->GetInUseVideoStreamId();
    std::shared_ptr<DashStreamDescription> stream = g_mpdDownloader->GetStreamByStreamId(usingStreamId);
    
    EXPECT_NE(stream, nullptr);
    EXPECT_TRUE(g_mpdDownloader->IsBitrateSame(stream->bandwidth_));
}

HWTEST_F(DashMpdDownloaderUnitTest, TEST_GET_BREAK_POINT_SEGMENT, TestSize.Level1)
{
    int usingStreamId = g_mpdDownloader->GetInUseVideoStreamId();
    std::shared_ptr<DashSegment> seg = nullptr;
    int64_t breakPoint = 30000;
    DashMpdGetRet getRet = g_mpdDownloader->GetBreakPointSegment(usingStreamId, breakPoint, seg);

    std::shared_ptr<DashSegment> segError = nullptr;
    int64_t breakPointError = 300000;
    g_mpdDownloader->GetBreakPointSegment(usingStreamId, breakPointError, segError);
    
    EXPECT_NE(getRet, DASH_MPD_GET_ERROR);
    EXPECT_EQ(segError, nullptr);
}

HWTEST_F(DashMpdDownloaderUnitTest, TEST_SEEK_TO_TS, TestSize.Level1)
{
    int usingStreamId = g_mpdDownloader->GetInUseVideoStreamId();
    int64_t duration = g_mpdDownloader->GetDuration();
    int64_t seekTime = duration / 2;
    std::shared_ptr<DashSegment> seg = nullptr;
    g_mpdDownloader->SeekToTs(usingStreamId, seekTime / MS_2_NS, seg);

    EXPECT_NE(seg, nullptr);
}

HWTEST_F(DashMpdDownloaderUnitTest, TEST_GET_NEXT_SEGMENT, TestSize.Level1)
{
    int usingStreamId = g_mpdDownloader->GetInUseVideoStreamId();
    std::shared_ptr<DashSegment> seg = nullptr;
    g_mpdDownloader->GetNextSegmentByStreamId(usingStreamId, seg);
    EXPECT_NE(seg, nullptr);
}

HWTEST_F(DashMpdDownloaderUnitTest, TEST_GET_INIT_SEGMENT, TestSize.Level1)
{
    int usingStreamId = g_mpdDownloader->GetInUseVideoStreamId();
    std::shared_ptr<DashInitSegment> initSeg = g_mpdDownloader->GetInitSegmentByStreamId(usingStreamId);
    EXPECT_NE(initSeg, nullptr);
}

HWTEST_F(DashMpdDownloaderUnitTest, TEST_SET_CURRENT_NUMBER_SEQ, TestSize.Level1)
{
    int usingStreamId = g_mpdDownloader->GetInUseVideoStreamId();
    g_mpdDownloader->SetCurrentNumberSeqByStreamId(usingStreamId, 10);
    std::shared_ptr<DashStreamDescription> stream = g_mpdDownloader->GetStreamByStreamId(usingStreamId);
    EXPECT_NE(stream, nullptr);
    EXPECT_EQ(stream->currentNumberSeq_, 10);
}

HWTEST_F(DashMpdDownloaderUnitTest, TEST_GET_NEXT_VIDEO_STREAM, TestSize.Level1)
{
    int usingStreamId = g_mpdDownloader->GetInUseVideoStreamId();
    std::shared_ptr<DashStreamDescription> stream = g_mpdDownloader->GetStreamByStreamId(usingStreamId);
    EXPECT_NE(stream, nullptr);

    uint32_t switchingBitrate = 0;
    std::vector<uint32_t> bitrates = g_mpdDownloader->GetBitRates();
    EXPECT_GT(bitrates.size(), 0);
    for (auto bitrate : bitrates) {
        if (bitrate != stream->bandwidth_) {
            switchingBitrate = bitrate;
            break;
        }
    }

    DashMpdGetRet ret = DASH_MPD_GET_ERROR;
    DashMpdBitrateParam bitrateParam;
    bitrateParam.bitrate_ = switchingBitrate;
    bitrateParam.type_ = DASH_MPD_SWITCH_TYPE_SMOOTH;
    ret = g_mpdDownloader->GetNextVideoStream(bitrateParam, bitrateParam.streamId_);

    EXPECT_NE(DASH_MPD_GET_ERROR, ret);
}

HWTEST_F(DashMpdDownloaderUnitTest, TEST_GET_NEXT_TRACK_STREAM, TestSize.Level1)
{
    std::shared_ptr<DashStreamDescription> audioStream = g_mpdDownloader->GetUsingStreamByType(
        MediaAVCodec::MediaType::MEDIA_TYPE_AUD);
    EXPECT_NE(audioStream, nullptr);

    DashMpdTrackParam trackParam;
    trackParam.isEnd_ = false;
    trackParam.type_ = MediaAVCodec::MediaType::MEDIA_TYPE_AUD;
    trackParam.streamId_ = audioStream->streamId_;
    trackParam.position_ = 2; // update by segment sequence, -1 means no segment downloading

    DashMpdGetRet ret = g_mpdDownloader->GetNextTrackStream(trackParam);
    EXPECT_NE(DASH_MPD_GET_ERROR, ret);
}

HWTEST_F(DashMpdDownloaderUnitTest, TEST_SET_HDR_START, TestSize.Level1)
{
    DashMpdDownloader downloader;
    std::string testUrl = MPD_SEGMENT_LIST;
    downloader.SetHdrStart(true);
    downloader.Open(testUrl);
    downloader.GetSeekable();
    int usingStreamId = downloader.GetInUseVideoStreamId();
    std::shared_ptr<DashStreamDescription> stream = downloader.GetStreamByStreamId(usingStreamId);
    EXPECT_NE(stream, nullptr);
    EXPECT_FALSE(stream->videoType_ == DASH_VIDEO_TYPE_HDR_VIVID);
    downloader.Close(true);
}

HWTEST_F(DashMpdDownloaderUnitTest, TEST_OPEN_TIMELINE_MPD, TestSize.Level1)
{
    DashMpdDownloader downloader;
    std::string testUrl = MPD_SEGMENT_LIST_TIMELINE;
    downloader.Open(testUrl);
    downloader.GetSeekable();
    int64_t duration = downloader.GetDuration();
    EXPECT_GE(duration, 0);
    downloader.Close(true);
}

HWTEST_F(DashMpdDownloaderUnitTest, TEST_OPEN_TEMPLATE_TIMELINE_MPD, TestSize.Level1)
{
    DashMpdDownloader downloader;
    std::string testUrl = URL_TEMPLATE_TIMELINE;
    downloader.Open(testUrl);
    downloader.GetSeekable();
    int64_t duration = downloader.GetDuration();
    EXPECT_GE(duration, 0);
    downloader.Close(true);
}

HWTEST_F(DashMpdDownloaderUnitTest, TEST_OPEN_ADPT_MPD, TestSize.Level1)
{
    DashMpdDownloader downloader;
    std::string testUrl = MPD_SEGMENT_TEMPLATE_ADPT;
    downloader.Open(testUrl);
    downloader.GetSeekable();
    int64_t duration = downloader.GetDuration();
    EXPECT_GE(duration, 0);
    downloader.Close(true);
}

HWTEST_F(DashMpdDownloaderUnitTest, TEST_OPEN_PERIOD_MPD, TestSize.Level1)
{
    DashMpdDownloader downloader;
    std::string testUrl = URL_SEGMENT_BASE_IN_PERIOD;
    downloader.Open(testUrl);
    downloader.GetSeekable();
    int64_t duration = downloader.GetDuration();
    EXPECT_GE(duration, 0);
    downloader.Close(true);
}

}
}
}
}