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

#include <string>
#include <malloc.h>
#include <sys/stat.h>
#include <cinttypes>
#include <fcntl.h>
#include <list>
#include <cmath>
#include <sys/types.h>
#include <unistd.h>
#include <fstream>
#include <memory>
#include <vector>
#include "gtest/gtest.h"
#include "avcodec_errors.h"
#include "pts_and_index_conversion_unit_test.h"
#include "pts_and_index_conversion_test_utils.h"

#define LOCAL true
namespace OHOS::Media {

using namespace std;
using namespace testing::ext;
using MediaSource = OHOS::Media::Plugins::MediaSource;

string g_flvPath            = string("/data/test/media/h264.flv");
string g_ptsConversionPath  = string("/data/test/media/camera_info_parser.mp4");
string g_ptsdoublevideoPath = string("/data/test/media/h264_double_video_audio.mp4");

using namespace PtsAndIndexConversionTestUtils;

void PtsAndIndexConversionTest::SetUpTestCase(void)
{
}

void PtsAndIndexConversionTest::TearDownTestCase(void)
{
}

void PtsAndIndexConversionTest::SetUp()
{
}

void PtsAndIndexConversionTest::TearDown()
{
    if (fd_ > 0) {
        close(fd_);
        fd_ = -1;
    }

    if (index_ != 0) {
        index_ = 0;
    }

    if (trackIndex_ != 0) {
        trackIndex_ = 0;
    }

    if (relativePresentationTimeUs_ != 0) {
        relativePresentationTimeUs_ = 0;
    }

    initStatus_ = false;
}

void PtsAndIndexConversionTest::InitResource(const std::string &srtpath, Status code)
{
    int64_t fileSize = 0;
    if (!srtpath.empty()) {
        struct stat fileStatus {};
        if (stat(srtpath.c_str(), &fileStatus) == 0) {
            fileSize = static_cast<int64_t>(fileStatus.st_size);
        }
    }
    fd_ = open(srtpath.c_str(), O_RDONLY);
    if (fd_ < 0) {
        return;
    }
    std::string uri = "fd://" + std::to_string(fd_) + "?offset=0&size=" + std::to_string(fileSize);
    TimeAndIndexConversions_ = std::make_shared<TimeAndIndexConversion>();
    std::shared_ptr<MediaSource> mediaSource = std::make_shared<MediaSource>(uri);
    ASSERT_EQ(TimeAndIndexConversions_->SetDataSource(mediaSource), code);
    initStatus_ = true;
}

/**
 * @tc.name: Demuxer_GetIndexByRelativePresentationTimeUs_1000
 * @tc.desc: Get index by pts(audio track)
 * @tc.type: FUNC
 */
HWTEST_F(PtsAndIndexConversionTest, Demuxer_GetIndexByRelativePresentationTimeUs_1000, TestSize.Level1)
{
    InitResource(g_ptsConversionPath, Status::OK);
    ASSERT_TRUE(initStatus_);
    relativePresentationTimeUs_ = 69659;
    ASSERT_EQ(TimeAndIndexConversions_->GetIndexByRelativePresentationTimeUs(
        trackIndex_, relativePresentationTimeUs_, index_), Status::OK);
    ASSERT_EQ(index_, 3);
}

/**
 * @tc.name: Demuxer_GetIndexByRelativePresentationTimeUs_1001
 * @tc.desc: Get index by pts(video track)
 * @tc.type: FUNC
 */
HWTEST_F(PtsAndIndexConversionTest, Demuxer_GetIndexByRelativePresentationTimeUs_1001, TestSize.Level1)
{
    InitResource(g_ptsConversionPath, Status::OK);
    ASSERT_TRUE(initStatus_);
    trackIndex_ = 1;
    relativePresentationTimeUs_ = 66666;
    ASSERT_EQ(TimeAndIndexConversions_->GetIndexByRelativePresentationTimeUs(
        trackIndex_, relativePresentationTimeUs_, index_), Status::OK);
    ASSERT_EQ(index_, 4);
}

/**
 * @tc.name: Demuxer_GetIndexByRelativePresentationTimeUs_1002
 * @tc.desc: Get index by pts(not MP4)
 * @tc.type: FUNC
 */
HWTEST_F(PtsAndIndexConversionTest, Demuxer_GetIndexByRelativePresentationTimeUs_1002, TestSize.Level1)
{
    InitResource(g_flvPath, Status::ERROR_UNSUPPORTED_FORMAT);
    ASSERT_TRUE(initStatus_);
    relativePresentationTimeUs_ = 69659;
    ASSERT_NE(TimeAndIndexConversions_->GetIndexByRelativePresentationTimeUs(
        trackIndex_, relativePresentationTimeUs_, index_), Status::OK);
    ASSERT_EQ(index_, 0);
}

/**
 * @tc.name: Demuxer_GetIndexByRelativePresentationTimeUs_1003
 * @tc.desc: Get index by pts(non-standard pts & different track)
 * @tc.type: FUNC
 */
HWTEST_F(PtsAndIndexConversionTest, Demuxer_GetIndexByRelativePresentationTimeUs_1003, TestSize.Level1)
{
    InitResource(g_ptsConversionPath, Status::OK);
    ASSERT_TRUE(initStatus_);
    trackIndex_ = 1;
    relativePresentationTimeUs_ = 166666;
    ASSERT_EQ(TimeAndIndexConversions_->GetIndexByRelativePresentationTimeUs(
        trackIndex_, relativePresentationTimeUs_, index_), Status::OK);
    ASSERT_EQ(index_, 10);
}

/**
 * @tc.name: Demuxer_GetRelativePresentationTimeUsByIndex_1000
 * @tc.desc: get pts by frameIndex(audio track)
 * @tc.type: FUNC
 */
HWTEST_F(PtsAndIndexConversionTest, Demuxer_GetRelativePresentationTimeUsByIndex_1000, TestSize.Level1)
{
    InitResource(g_ptsConversionPath, Status::OK);
    ASSERT_TRUE(initStatus_);
    index_ = 2;
    ASSERT_EQ(TimeAndIndexConversions_->GetRelativePresentationTimeUsByIndex(
        trackIndex_, index_, relativePresentationTimeUs_), Status::OK);
    ASSERT_EQ(relativePresentationTimeUs_, 46439);
}

/**
 * @tc.name: Demuxer_GetRelativePresentationTimeUsByIndex_1001
 * @tc.desc: get pts by frameIndex(video track)
 * @tc.type: FUNC
 */
HWTEST_F(PtsAndIndexConversionTest, Demuxer_GetRelativePresentationTimeUsByIndex_1001, TestSize.Level1)
{
    InitResource(g_ptsConversionPath, Status::OK);
    ASSERT_TRUE(initStatus_);
    index_ = 2;
    trackIndex_ = 1;
    ASSERT_EQ(TimeAndIndexConversions_->GetRelativePresentationTimeUsByIndex(
        trackIndex_, index_, relativePresentationTimeUs_), Status::OK);
    ASSERT_EQ(relativePresentationTimeUs_, 33333);
}

/**
 * @tc.name: Demuxer_GetRelativePresentationTimeUsByIndex_1002
 * @tc.desc: get pts by frameIndex(not MP4)
 * @tc.type: FUNC
 */
HWTEST_F(PtsAndIndexConversionTest, Demuxer_GetRelativePresentationTimeUsByIndex_1002, TestSize.Level1)
{
    InitResource(g_flvPath, Status::ERROR_UNSUPPORTED_FORMAT);
    ASSERT_TRUE(initStatus_);
    index_ = 10;
    ASSERT_NE(TimeAndIndexConversions_->GetRelativePresentationTimeUsByIndex(
        trackIndex_, index_, relativePresentationTimeUs_), Status::OK);
    ASSERT_EQ(relativePresentationTimeUs_, 0);
}

/**
 * @tc.name: Demuxer_PtsAndFrameIndexConversion_1000
 * @tc.desc: pts and frameIndex convertion test(pts -> frameIndex -> pts)
 * @tc.type: FUNC
 */
HWTEST_F(PtsAndIndexConversionTest, Demuxer_PtsAndFrameIndexConversion_1000, TestSize.Level1)
{
    InitResource(g_ptsConversionPath, Status::OK);
    ASSERT_TRUE(initStatus_);
    relativePresentationTimeUs_ = 92879;
    ASSERT_EQ(TimeAndIndexConversions_->GetIndexByRelativePresentationTimeUs(
        trackIndex_, relativePresentationTimeUs_, index_), Status::OK);
    ASSERT_EQ(index_, 4);

    relativePresentationTimeUs_ = 0;
    ASSERT_EQ(TimeAndIndexConversions_->GetRelativePresentationTimeUsByIndex(
        trackIndex_, index_, relativePresentationTimeUs_), Status::OK);
    ASSERT_EQ(relativePresentationTimeUs_, 92879);
}

/**
 * @tc.name: Demuxer_PtsAndFrameIndexConversion_1001
 * @tc.desc: pts and frameIndex convertion test(frameIndex -> pts -> frameIndex)
 * @tc.type: FUNC
 */
HWTEST_F(PtsAndIndexConversionTest, Demuxer_PtsAndFrameIndexConversion_1001, TestSize.Level1)
{
    InitResource(g_ptsConversionPath, Status::OK);
    ASSERT_TRUE(initStatus_);
    index_ = 4;
    ASSERT_EQ(TimeAndIndexConversions_->GetRelativePresentationTimeUsByIndex(
        trackIndex_, index_, relativePresentationTimeUs_), Status::OK);
    ASSERT_EQ(relativePresentationTimeUs_, 92879);

    index_ = 0;
    ASSERT_EQ(TimeAndIndexConversions_->GetIndexByRelativePresentationTimeUs(
        trackIndex_, relativePresentationTimeUs_, index_), Status::OK);
    ASSERT_EQ(index_, 4);
}

/**
 * @tc.name: Demuxer_PTSOutOfRange_1000
 * @tc.desc: pts out of range
 * @tc.type: FUNC
 */
HWTEST_F(PtsAndIndexConversionTest, Demuxer_PTSOutOfRange_1000, TestSize.Level1)
{
    InitResource(g_ptsConversionPath, Status::OK);
    ASSERT_TRUE(initStatus_);
    relativePresentationTimeUs_ = 999999999;
    ASSERT_NE(TimeAndIndexConversions_->GetIndexByRelativePresentationTimeUs(
        trackIndex_, relativePresentationTimeUs_, index_), Status::OK);
    ASSERT_EQ(index_, 0);
}

/**
 * @tc.name: Demuxer_IndexOutOfRange_1000
 * @tc.desc: Index out of range
 * @tc.type: FUNC
 */
HWTEST_F(PtsAndIndexConversionTest, Demuxer_IndexOutOfRange_1000, TestSize.Level1)
{
    InitResource(g_ptsConversionPath, Status::OK);
    ASSERT_TRUE(initStatus_);
    index_ = 9999999;
    ASSERT_NE(TimeAndIndexConversions_->GetRelativePresentationTimeUsByIndex(
        trackIndex_, index_, relativePresentationTimeUs_), Status::OK);
    ASSERT_EQ(relativePresentationTimeUs_, 0);
}

/**
 * @tc.name: Demuxer_TrackOutOfRange_1000
 * @tc.desc: Track out of range
 * @tc.type: FUNC
 */
HWTEST_F(PtsAndIndexConversionTest, Demuxer_TrackOutOfRange_1000, TestSize.Level1)
{
    InitResource(g_ptsConversionPath, Status::OK);
    ASSERT_TRUE(initStatus_);
    trackIndex_ = 99;
    ASSERT_NE(TimeAndIndexConversions_->GetRelativePresentationTimeUsByIndex(
        trackIndex_, index_, relativePresentationTimeUs_), Status::OK);
    ASSERT_EQ(relativePresentationTimeUs_, 0);
    ASSERT_EQ(index_, 0);
}

/**
 * @tc.name: Demuxer_GetVideoTrackIndex_1000
 * @tc.desc: Get video track index
 * @tc.type: FUNC
 */
HWTEST_F(PtsAndIndexConversionTest, Demuxer_GetVideoTrackIndex_1000, TestSize.Level1)
{
    InitResource(g_ptsConversionPath, Status::OK);
    ASSERT_TRUE(initStatus_);
    trackIndex_ = -1;
    ASSERT_EQ(TimeAndIndexConversions_->GetFirstVideoTrackIndex(trackIndex_), Status::OK);
    ASSERT_EQ(trackIndex_, 1);
}

/**
 * @tc.name: Demuxer_GetVideoTrackIndex_1001
 * @tc.desc: Get video track index(double video track index)
 * @tc.type: FUNC
 */
HWTEST_F(PtsAndIndexConversionTest, Demuxer_GetVideoTrackIndex_1001, TestSize.Level1)
{
    InitResource(g_ptsdoublevideoPath, Status::OK);
    ASSERT_TRUE(initStatus_);
    trackIndex_ = -1;
    ASSERT_EQ(TimeAndIndexConversions_->GetFirstVideoTrackIndex(trackIndex_), Status::OK);
    ASSERT_EQ(trackIndex_, 0);
}

/**
 * @tc.name: Demuxer_GetVideoTrackIndex_1002
 * @tc.desc: Get video track index(not MP4)
 * @tc.type: FUNC
 */
HWTEST_F(PtsAndIndexConversionTest, Demuxer_GetVideoTrackIndex_1002, TestSize.Level1)
{
    InitResource(g_flvPath, Status::ERROR_UNSUPPORTED_FORMAT);
    ASSERT_TRUE(initStatus_);
    trackIndex_ = -1;
    ASSERT_NE(TimeAndIndexConversions_->GetFirstVideoTrackIndex(trackIndex_), Status::OK);
    ASSERT_EQ(trackIndex_, -1);
}

/**
 * @tc.name: LargeSizeCtts
 * @tc.number: LargeSizeCtts_001
 * @tc.desc: A large CTTS payload must not make the parser allocate the whole box.
 */
HWTEST_F(PtsAndIndexConversionTest, LargeSizeCtts, TestSize.Level1)
{
    EXPECT_TRUE(RunLargeSizeReproduction(BOX_TYPE_CTTS));
}

/**
 * @tc.name: LargeSizeStts
 * @tc.number: LargeSizeStts_001
 * @tc.desc: A large STTS payload must not make the parser allocate the whole box.
 */
HWTEST_F(PtsAndIndexConversionTest, LargeSizeStts, TestSize.Level1)
{
    EXPECT_TRUE(RunLargeSizeReproduction(BOX_TYPE_STTS));
}

/**
 * @tc.name: LargeSizeHdlr
 * @tc.number: LargeSizeHdlr_001
 * @tc.desc: A large HDLR payload must not make the parser allocate the whole box.
 */
HWTEST_F(PtsAndIndexConversionTest, LargeSizeHdlr, TestSize.Level1)
{
    EXPECT_TRUE(RunLargeSizeReproduction(BOX_TYPE_HDLR));
}

/**
 * @tc.name: LargeSizeMdhd
 * @tc.number: LargeSizeMdhd_001
 * @tc.desc: A large MDHD payload must not make the parser allocate the whole box.
 */
HWTEST_F(PtsAndIndexConversionTest, LargeSizeMdhd, TestSize.Level1)
{
    EXPECT_TRUE(RunLargeSizeReproduction(BOX_TYPE_MDHD));
}

/**
 * @tc.name: DeepContainerStackExhaustion
 * @tc.number: DeepContainerStackExhaustion_001
 * @tc.desc: Excessively nested recognized containers must not exhaust the parser stack.
 */
HWTEST_F(PtsAndIndexConversionTest, DeepContainerStackExhaustion, TestSize.Level1)
{
    int childStatus = 0;
    EXPECT_TRUE(RunDeepContainerReproduction(childStatus)) << "child process status=" << childStatus;
}

/**
 * @tc.name: DeepContainerDepthBoundary
 * @tc.number: DeepContainerDepthBoundary_001
 * @tc.desc: Containers at the depth limit are parsed, while deeper subtrees are skipped safely.
 */
HWTEST_F(PtsAndIndexConversionTest, DeepContainerDepthBoundary, TestSize.Level1)
{
    TimeAndIndexConversion allowedConversion;
    ASSERT_TRUE(ParseDeepContainerMedia(MAX_CONTAINER_DEPTH, allowedConversion));
    ASSERT_EQ(allowedConversion.trakInfoVec_.size(), 1);
    EXPECT_EQ(allowedConversion.trakInfoVec_[0].trakType, TimeAndIndexConversion::TrakType::TRAK_VIDIO);

    TimeAndIndexConversion skippedConversion;
    ASSERT_TRUE(ParseDeepContainerMedia(MAX_CONTAINER_DEPTH + 1, skippedConversion));
    ASSERT_EQ(skippedConversion.trakInfoVec_.size(), 1);
    EXPECT_EQ(skippedConversion.trakInfoVec_[0].trakType, TimeAndIndexConversion::TrakType::TRAK_OTHER);
}

/**
 * @tc.name: ValidCttsEntries
 * @tc.number: ValidCttsEntries_001
 * @tc.desc: A normal CTTS box keeps its entry data after bounded reading.
 */
HWTEST_F(PtsAndIndexConversionTest, ValidCttsEntries, TestSize.Level0)
{
    TimeAndIndexConversion conversion;
    ASSERT_TRUE(ParseSmallSizeMedia(BOX_TYPE_CTTS, MakeTimeEntryPayload(1, 3), conversion));
    ASSERT_EQ(conversion.trakInfoVec_.size(), 1);
    ASSERT_EQ(conversion.trakInfoVec_[0].cttsEntries.size(), 1);
    EXPECT_EQ(conversion.trakInfoVec_[0].cttsEntries[0].sampleCount, 2);
    EXPECT_EQ(conversion.trakInfoVec_[0].cttsEntries[0].sampleOffset, 3);
}

/**
 * @tc.name: ValidSttsEntries
 * @tc.number: ValidSttsEntries_001
 * @tc.desc: A normal STTS box keeps its entry data after bounded reading.
 */
HWTEST_F(PtsAndIndexConversionTest, ValidSttsEntries, TestSize.Level0)
{
    TimeAndIndexConversion conversion;
    ASSERT_TRUE(ParseSmallSizeMedia(BOX_TYPE_STTS, MakeTimeEntryPayload(1, 100), conversion));
    ASSERT_EQ(conversion.trakInfoVec_.size(), 1);
    ASSERT_EQ(conversion.trakInfoVec_[0].sttsEntries.size(), 1);
    EXPECT_EQ(conversion.trakInfoVec_[0].sttsEntries[0].sampleCount, 2);
    EXPECT_EQ(conversion.trakInfoVec_[0].sttsEntries[0].sampleDelta, 100);
}

/**
 * @tc.name: ValidHdlr
 * @tc.number: ValidHdlr_001
 * @tc.desc: A normal HDLR box preserves the video handler classification.
 */
HWTEST_F(PtsAndIndexConversionTest, ValidHdlr, TestSize.Level0)
{
    TimeAndIndexConversion conversion;
    ASSERT_TRUE(ParseSmallSizeMedia(BOX_TYPE_HDLR, MakeHdlrPayload(), conversion));
    ASSERT_EQ(conversion.trakInfoVec_.size(), 1);
    EXPECT_EQ(conversion.trakInfoVec_[0].trakType, TimeAndIndexConversion::TrakType::TRAK_VIDIO);
}

/**
 * @tc.name: ValidMdhdVersions
 * @tc.number: ValidMdhdVersions_001
 * @tc.desc: Both MDHD versions retain their time scale with bounded reading.
 */
HWTEST_F(PtsAndIndexConversionTest, ValidMdhdVersions, TestSize.Level0)
{
    TimeAndIndexConversion conversionV0;
    ASSERT_TRUE(ParseSmallSizeMedia(BOX_TYPE_MDHD, MakeMdhdPayload(0), conversionV0));
    ASSERT_EQ(conversionV0.trakInfoVec_.size(), 1);
    EXPECT_EQ(conversionV0.trakInfoVec_[0].timeScale, 1000);

    TimeAndIndexConversion conversionV1;
    ASSERT_TRUE(ParseSmallSizeMedia(BOX_TYPE_MDHD, MakeMdhdPayload(1), conversionV1));
    ASSERT_EQ(conversionV1.trakInfoVec_.size(), 1);
    EXPECT_EQ(conversionV1.trakInfoVec_[0].timeScale, 1000);
}

/**
 * @tc.name: RejectsExcessiveTimeEntryCount
 * @tc.number: RejectsExcessiveTimeEntryCount_001
 * @tc.desc: CTTS/STTS entry counts over the parser limit are rejected without large allocation.
 */
HWTEST_F(PtsAndIndexConversionTest, RejectsExcessiveTimeEntryCount, TestSize.Level0)
{
    TimeAndIndexConversion cttsConversion;
    ASSERT_TRUE(ParseSmallSizeMedia(BOX_TYPE_CTTS, MakeTimeEntryPayload(EXCESSIVE_ENTRY_COUNT, 0), cttsConversion));
    ASSERT_EQ(cttsConversion.trakInfoVec_.size(), 1);
    EXPECT_TRUE(cttsConversion.trakInfoVec_[0].cttsEntries.empty());

    TimeAndIndexConversion sttsConversion;
    ASSERT_TRUE(ParseSmallSizeMedia(BOX_TYPE_STTS, MakeTimeEntryPayload(EXCESSIVE_ENTRY_COUNT, 0),
        sttsConversion));
    ASSERT_EQ(sttsConversion.trakInfoVec_.size(), 1);
    EXPECT_TRUE(sttsConversion.trakInfoVec_[0].sttsEntries.empty());
}

/**
 * @tc.name: RejectsTooSmallMetadataBoxes
 * @tc.number: RejectsTooSmallMetadataBoxes_001
 * @tc.desc: HDLR and MDHD boxes smaller than their required fields are ignored safely.
 */
HWTEST_F(PtsAndIndexConversionTest, RejectsTooSmallMetadataBoxes, TestSize.Level0)
{
    TimeAndIndexConversion hdlrConversion;
    ASSERT_TRUE(ParseSmallSizeMedia(BOX_TYPE_HDLR, std::vector<uint8_t>(sizeof(uint32_t) * 2), hdlrConversion));
    ASSERT_EQ(hdlrConversion.trakInfoVec_.size(), 1);
    EXPECT_EQ(hdlrConversion.trakInfoVec_[0].trakType, TimeAndIndexConversion::TrakType::TRAK_OTHER);

    TimeAndIndexConversion mdhdConversion;
    ASSERT_TRUE(ParseSmallSizeMedia(BOX_TYPE_MDHD, std::vector<uint8_t>(sizeof(uint32_t)), mdhdConversion));
    ASSERT_EQ(mdhdConversion.trakInfoVec_.size(), 1);
    EXPECT_EQ(mdhdConversion.trakInfoVec_[0].timeScale, 0);
}
}
