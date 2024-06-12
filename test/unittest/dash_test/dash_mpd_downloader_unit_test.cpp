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
#include "dash_mpd_downloader.h"
#include "utils/time_utils.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
namespace {
static const std::string MPD_SEGMENT_BASE =
    "http://poster-inland.hwcloudtest.cn/AiMaxEngine/DASH_LOCAL/DASH_SDR_H265_HEV1/DASH_SDR_H265_HEV1.mpd";
static const std::string MPD_SEGMENT_LIST =
    "http://poster-inland.hwcloudtest.cn/AiMaxEngine/DASH_LOCAL/DASH_SDR_H265_2K_segmentList/index_only720P.mpd";
static const std::string MPD_SEGMENT_TEMPLATE =
    "http://poster-inland.hwcloudtest.cn/AiMaxEngine/DASH_LOCAL/DASH_SDR_H265_2K_segmentTemplate/index_only720P.mpd";
}

using namespace testing::ext;

std::shared_ptr<DashMpdDownloader> g_mpdDownloader = nullptr;

void DashMpdDownloaderUnitTest::SetUpTestCase(void)
{
    g_mpdDownloader = std::make_shared<DashMpdDownloader>();
    g_mpdDownloader->Open(MPD_SEGMENT_BASE);
}

void DashMpdDownloaderUnitTest::TearDownTestCase(void)
{
    g_mpdDownloader = nullptr;
}

void DashMpdDownloaderUnitTest::SetUp(void) {}

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
    EXPECT_EQ(hdrBitrates.size(), 0);
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
    EXPECT_FALSE(stream->isHdr_);
}

}
}
}
}