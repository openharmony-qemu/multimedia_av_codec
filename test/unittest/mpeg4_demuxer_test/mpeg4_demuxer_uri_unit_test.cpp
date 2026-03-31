/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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
#include "gtest/gtest.h"
#include "avcodec_errors.h"
#include "media_description.h"
#include "file_server_demo.h"
#include "mpeg4_demuxer_unit_test.h"

#define URI false

using namespace OHOS;
using namespace OHOS::Media;
using namespace OHOS::MediaAVCodec;
using namespace testing::ext;
using namespace std;

namespace {
unique_ptr<FileServerDemo> server = nullptr;
static const string TEST_FILE_PATH = "/data/test/media/";
static const string TEST_URI_PATH = "http://127.0.0.1:46666/";
static const string TEST_URI_PATH2 = "http://192.168.3.11:8080/share/";

list<SeekMode> seekModes = {SeekMode::SEEK_NEXT_SYNC, SeekMode::SEEK_PREVIOUS_SYNC,
    SeekMode::SEEK_CLOSEST_SYNC};

string g_mp4SniffFreeUri = TEST_URI_PATH + string("only_free_in_first_2kb.mp4");
string g_mp4SniffMoovUri = TEST_URI_PATH + string("only_moov_in_first_2kb.mp4");
string g_mp4SniffSkipUri = TEST_URI_PATH + string("only_skip_in_first_2kb.mp4");
string g_mp4SniffMdatUri = TEST_URI_PATH + string("only_mdat_in_first_2kb.mp4");
string g_mp4SniffJp2Uri = TEST_URI_PATH + string("ftyp_jp2_in_first_2kb.mp4");
string g_mp4MPEGPSUri = TEST_URI_PATH + string("mpeg_ps_in_mov.mp4");
string g_mp4AbnormalBrandUri = TEST_URI_PATH + string("abnormal_brand.mp4");
string g_mp4MoovOverSizeUri = TEST_URI_PATH + string("moov_oversize.mp4");
string g_mp4DelayUri = TEST_URI_PATH + string("delay_v1s.mp4");
string g_fmp4MeetingUri = TEST_URI_PATH + string("meeting_fmp4.mp4");
string g_mp4WvttUri = TEST_URI_PATH + string("wvtt.mp4");
string g_mp4G711muUri = TEST_URI_PATH + string("mpeg4_g711mu.mov");
string g_fmp4SidxUri = TEST_URI_PATH + string("sidx.mp4");
string g_fmp4DoubleTopSidxUri = TEST_URI_PATH + string("double_sidx_at_top.mp4");
string g_fmp4MultiSidxUri = TEST_URI_PATH + string("multi_sidx.mp4");
string g_fmp4MoreSidxUri = TEST_URI_PATH + string("more_sidx.mp4");
string g_mp4ShortAudioUri = TEST_URI_PATH + string("short_audio.mp4");
string g_mp4ShortVideoUri = TEST_URI_PATH + string("short_video.mp4");
string g_mp4H263AmrwbUri = TEST_URI_PATH + string("h263_amr_wb.3gp");
string g_mp4Mpeg4OpusUri = TEST_URI_PATH + string("mpeg4_opus.mp4");
string g_mp4Mpeg4AmrnbUri = TEST_URI_PATH + string("mpeg4_amr_nb.3gp");
string g_mp4Mpeg2AlacUri = TEST_URI_PATH + string("mpeg2_alac.mp4");
string g_mp4Mpeg2DtsUri = TEST_URI_PATH + string("mpeg2_dts.mp4");


/**
 * @tc.name: mp4Demuxer_CreateDemuxer_1001
 * @tc.desc: create demuxer. The first 2048 bytes of this MP4 resource
 * contain 'ftyp' box and the brand is jp2.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_CreateDemuxer_1001, TestSize.Level1)
{
    InitResource(g_mp4SniffJp2Uri, URI);
    ASSERT_TRUE(initStatus_);
}

/**
 * @tc.name: mp4Demuxer_CreateDemuxer_1002
 * @tc.desc: create demuxer. The first 2048 bytes of this MP4 resource contain only a single 'mdat' box.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_CreateDemuxer_1002, TestSize.Level1)
{
    InitResource(g_mp4SniffMdatUri, URI);
    ASSERT_TRUE(initStatus_);
}

/**
 * @tc.name: mp4Demuxer_CreateDemuxer_1003
 * @tc.desc: create demuxer. The first 2048 bytes of this MP4 resource contain only a single 'moov' box.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_CreateDemuxer_1003, TestSize.Level1)
{
    InitResource(g_mp4SniffMoovUri, URI);
    ASSERT_TRUE(initStatus_);
}

/**
 * @tc.name: mp4Demuxer_CreateDemuxer_1004
 * @tc.desc: create demuxer. The first 2048 bytes of this MP4 resource contain only a single 'free' box.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_CreateDemuxer_1004, TestSize.Level1)
{
    InitResource(g_mp4SniffFreeUri, URI);
    ASSERT_TRUE(initStatus_);
}

/**
 * @tc.name: mp4Demuxer_CreateDemuxer_1005
 * @tc.desc: create demuxer. The first 2048 bytes of this MP4 resource contain only a single 'skip' box.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_CreateDemuxer_1005, TestSize.Level1)
{
    source_ = Mpeg4SourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4SniffSkipUri.data()));
    ASSERT_EQ(source_, nullptr);
}

/**
 * @tc.name: mp4Demuxer_CreateDemuxer_1006
 * @tc.desc: create demuxer. MPEG-PS in mp4.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_CreateDemuxer_1006, TestSize.Level1)
{
    source_ = Mpeg4SourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4MPEGPSUri.data()));
    ASSERT_EQ(source_, nullptr);
}

/**
 * @tc.name: mp4Demuxer_CreateDemuxer_1007
 * @tc.desc: create demuxer. Abnormal brand in 'ftyp' box.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_CreateDemuxer_1007, TestSize.Level1)
{
    source_ = Mpeg4SourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4AbnormalBrandUri.data()));
    ASSERT_NE(source_, nullptr);
}

/**
 * @tc.name: mp4Demuxer_CreateDemuxer_1008
 * @tc.desc: Demuxer creation succeeds, but initialization fails due to excessively large moov box size
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_CreateDemuxer_1008, TestSize.Level1)
{
    source_ = Mpeg4SourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4MoovOverSizeUri.data()));
    ASSERT_EQ(source_, nullptr);
}

/**
 * @tc.name: mp4Demuxer_UnselectTrackByID_1001
 * @tc.desc: select and remove track by ID
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_UnselectTrackByID_1001, TestSize.Level1)
{
    InitResource(g_mp4DelayUri, URI);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    EXPECT_NE(demuxer_->SelectTrackByID(2), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_NE(demuxer_->SelectTrackByID(-1), AV_ERR_OK);
    EXPECT_EQ(demuxer_->UnselectTrackByID(1), AV_ERR_OK);
    EXPECT_EQ(demuxer_->UnselectTrackByID(2), AV_ERR_OK);
    EXPECT_EQ(demuxer_->UnselectTrackByID(-1), AV_ERR_OK);
}

/**
 * @tc.name: mp4Demuxer_UnselectTrackByID_1002
 * @tc.desc: select and remove track by ID
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_UnselectTrackByID_1002, TestSize.Level1)
{
    InitResource(g_fmp4MeetingUri, URI);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    EXPECT_NE(demuxer_->SelectTrackByID(2), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_NE(demuxer_->SelectTrackByID(-1), AV_ERR_OK);
    EXPECT_EQ(demuxer_->UnselectTrackByID(1), AV_ERR_OK);
    EXPECT_EQ(demuxer_->UnselectTrackByID(2), AV_ERR_OK);
    EXPECT_EQ(demuxer_->UnselectTrackByID(-1), AV_ERR_OK);
}

/**
 * @tc.name: mp4Demuxer_ReadSample_1001
 * @tc.desc: copy current sample to buffer, check read in PTS order
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_ReadSample_1001, TestSize.Level1)
{
    InitResource(g_mp4DelayUri, URI);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    EXPECT_TRUE(SetInitValue());
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            EXPECT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    printf("frames_[1]=%d | kFrames[1]=%d\n", frames_[1], keyFrames_[1]);
    EXPECT_EQ(frames_[0], 602);
    EXPECT_EQ(frames_[1], 434);
    EXPECT_EQ(keyFrames_[0], 51);
    EXPECT_EQ(keyFrames_[1], 434);
    RemoveValue();
}

/**
 * @tc.name: mp4Demuxer_ReadSample_1002
 * @tc.desc: copy current sample to buffer, check read in PTS order
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_ReadSample_1002, TestSize.Level1)
{
    InitResource(g_fmp4MeetingUri, URI);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    EXPECT_TRUE(SetInitValue());
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            EXPECT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    printf("frames_[1]=%d | kFrames[1]=%d\n", frames_[1], keyFrames_[1]);
    EXPECT_EQ(frames_[0], 178);
    EXPECT_EQ(frames_[1], 548);
    EXPECT_EQ(keyFrames_[0], 12);
    EXPECT_EQ(keyFrames_[1], 548);
    RemoveValue();
}

/**
 * @tc.name: mp4Demuxer_ReadSample_1003
 * @tc.desc: copy current sample to buffer, check read in PTS order
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_ReadSample_1003, TestSize.Level1)
{
    InitResource(g_mp4WvttUri, URI);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(2), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    EXPECT_TRUE(SetInitValue());
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            EXPECT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    printf("frames_[1]=%d | kFrames[1]=%d\n", frames_[1], keyFrames_[1]);
    printf("frames_[1]=%d | kFrames[1]=%d\n", frames_[2], keyFrames_[2]);
    EXPECT_EQ(frames_[0], 604);
    EXPECT_EQ(frames_[1], 434);
    EXPECT_EQ(frames_[2], 8);
    EXPECT_EQ(keyFrames_[0], 61);
    EXPECT_EQ(keyFrames_[1], 434);
    EXPECT_EQ(keyFrames_[2], 8);
    RemoveValue();
}

/**
 * @tc.name: mp4Demuxer_ReadSample_1004
 * @tc.desc: copy current sample to buffer, check read in PTS order
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_ReadSample_1004, TestSize.Level1)
{
    InitResource(g_mp4G711muUri, URI);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    EXPECT_TRUE(SetInitValue());
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            EXPECT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    printf("frames_[1]=%d | kFrames[1]=%d\n", frames_[1], keyFrames_[1]);
    EXPECT_EQ(frames_[0], 45);
    EXPECT_EQ(frames_[1], 45);
    EXPECT_EQ(keyFrames_[0], 5);
    EXPECT_EQ(keyFrames_[1], 45);
    RemoveValue();
}

/**
 * @tc.name: mp4Demuxer_ReadSample_1005
 * @tc.desc: copy current sample to buffer, check read in PTS order
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_ReadSample_1005, TestSize.Level1)
{
    InitResource(g_fmp4SidxUri, URI);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    EXPECT_TRUE(SetInitValue());
    uint32_t index = 0;
    uint32_t listIndex = 0;
    vector<int64_t> sampleIndexList = {26, 42, 53, 136, 247};
    vector<int32_t> videoPts = {450000, 716666, 966666, 2300000, 4116666};
    vector<int32_t> audioPts = {697573, 1115532, 1402879, 3571043, 6470634};
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            EXPECT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            if (index == sampleIndexList[listIndex] && idx == 0) {
                EXPECT_EQ(info_.presentationTimeUs, videoPts[listIndex]);
            } else if (index == sampleIndexList[listIndex] && idx == 1) {
                EXPECT_EQ(info_.presentationTimeUs, audioPts[listIndex]);
            }
            CountFrames(idx);
        }
        if (index == sampleIndexList[listIndex] && listIndex < sampleIndexList.size()) {
            listIndex++;
        }
        index++;
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    printf("frames_[1]=%d | kFrames[1]=%d\n", frames_[1], keyFrames_[1]);
    EXPECT_EQ(frames_[0], 604);
    EXPECT_EQ(frames_[1], 387);
    EXPECT_EQ(keyFrames_[0], 21);
    EXPECT_EQ(keyFrames_[1], 387);
    RemoveValue();
}

/**
 * @tc.name: mp4Demuxer_ReadSample_1006
 * @tc.desc: copy current sample to buffer, check read in PTS order
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_ReadSample_1006, TestSize.Level1)
{
    InitResource(g_fmp4DoubleTopSidxUri, URI);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    EXPECT_TRUE(SetInitValue());
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            EXPECT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    printf("frames_[1]=%d | kFrames[1]=%d\n", frames_[1], keyFrames_[1]);
    EXPECT_EQ(frames_[0], 604);
    EXPECT_EQ(frames_[1], 387);
    EXPECT_EQ(keyFrames_[0], 21);
    EXPECT_EQ(keyFrames_[1], 387);
    RemoveValue();
}

/**
 * @tc.name: mp4Demuxer_ReadSample_1007
 * @tc.desc: copy current sample to buffer, check read in PTS order
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_ReadSample_1007, TestSize.Level1)
{
    InitResource(g_fmp4MultiSidxUri, URI);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    EXPECT_TRUE(SetInitValue());
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            EXPECT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    EXPECT_EQ(frames_[0], 324);
    EXPECT_EQ(keyFrames_[0], 33);
    RemoveValue();
}

/**
 * @tc.name: mp4Demuxer_ReadSample_1008
 * @tc.desc: copy current sample to buffer, check read in PTS order
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_ReadSample_1008, TestSize.Level1)
{
    InitResource(g_fmp4MoreSidxUri, URI);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    EXPECT_TRUE(SetInitValue());
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            EXPECT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    printf("frames_[1]=%d | kFrames[1]=%d\n", frames_[1], keyFrames_[1]);
    EXPECT_EQ(frames_[0], 420);
    EXPECT_EQ(frames_[1], 268);
    EXPECT_EQ(keyFrames_[0], 14);
    EXPECT_EQ(keyFrames_[1], 268);
    RemoveValue();
}

/**
 * @tc.name: mp4Demuxer_ReadSample_1009
 * @tc.desc: copy current sample to buffer, check read in PTS order
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_ReadSample_1009, TestSize.Level1)
{
    InitResource(g_mp4ShortAudioUri, URI);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    EXPECT_TRUE(SetInitValue());
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            EXPECT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    printf("frames_[1]=%d | kFrames[1]=%d\n", frames_[1], keyFrames_[1]);
    EXPECT_EQ(frames_[0], 120);
    EXPECT_EQ(frames_[1], 85);
    EXPECT_EQ(keyFrames_[0], 12);
    EXPECT_EQ(keyFrames_[1], 85);
    RemoveValue();
}

/**
 * @tc.name: mp4Demuxer_ReadSample_1010
 * @tc.desc: copy current sample to buffer, check read in PTS order
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_ReadSample_1010, TestSize.Level1)
{
    InitResource(g_mp4ShortVideoUri, URI);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    EXPECT_TRUE(SetInitValue());
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            EXPECT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    printf("frames_[1]=%d | kFrames[1]=%d\n", frames_[1], keyFrames_[1]);
    EXPECT_EQ(frames_[0], 150);
    EXPECT_EQ(frames_[1], 470);
    EXPECT_EQ(keyFrames_[0], 15);
    EXPECT_EQ(keyFrames_[1], 470);
    RemoveValue();
}

/**
 * @tc.name: mp4Demuxer_ReadSample_1011
 * @tc.desc: copy current sample to buffer, check read in PTS order
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_ReadSample_1011, TestSize.Level1)
{
    InitResource(g_mp4H263AmrwbUri, URI);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    EXPECT_TRUE(SetInitValue());
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            EXPECT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    printf("frames_[1]=%d | kFrames[1]=%d\n", frames_[1], keyFrames_[1]);
    EXPECT_EQ(frames_[0], 99);
    EXPECT_EQ(frames_[1], 165);
    EXPECT_EQ(keyFrames_[0], 9);
    EXPECT_EQ(keyFrames_[1], 165);
    RemoveValue();
}

/**
 * @tc.name: mp4Demuxer_ReadSample_1012
 * @tc.desc: copy current sample to buffer, check read in PTS order
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_ReadSample_1012, TestSize.Level1)
{
    InitResource(g_mp4Mpeg4OpusUri, URI);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    EXPECT_TRUE(SetInitValue());
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            EXPECT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    printf("frames_[1]=%d | kFrames[1]=%d\n", frames_[1], keyFrames_[1]);
    EXPECT_EQ(frames_[0], 95);
    EXPECT_EQ(frames_[1], 165);
    EXPECT_EQ(keyFrames_[0], 8);
    EXPECT_EQ(keyFrames_[1], 165);
    RemoveValue();
}

/**
 * @tc.name: mp4Demuxer_ReadSample_1013
 * @tc.desc: copy current sample to buffer, check read in PTS order
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_ReadSample_1013, TestSize.Level1)
{
    InitResource(g_mp4Mpeg4AmrnbUri, URI);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    EXPECT_TRUE(SetInitValue());
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            EXPECT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    printf("frames_[1]=%d | kFrames[1]=%d\n", frames_[1], keyFrames_[1]);
    EXPECT_EQ(frames_[0], 95);
    EXPECT_EQ(frames_[1], 165);
    EXPECT_EQ(keyFrames_[0], 8);
    EXPECT_EQ(keyFrames_[1], 165);
    RemoveValue();
}

/**
 * @tc.name: mp4Demuxer_ReadSample_1014
 * @tc.desc: copy current sample to buffer, check read in PTS order
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_ReadSample_1014, TestSize.Level1)
{
    InitResource(g_mp4Mpeg2AlacUri, URI);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    EXPECT_TRUE(SetInitValue());
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            EXPECT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    printf("frames_[1]=%d | kFrames[1]=%d\n", frames_[1], keyFrames_[1]);
    EXPECT_EQ(frames_[0], 95);
    EXPECT_EQ(frames_[1], 39);
    EXPECT_EQ(keyFrames_[0], 10);
    EXPECT_EQ(keyFrames_[1], 39);
    RemoveValue();
}

/**
 * @tc.name: mp4Demuxer_ReadSample_1015
 * @tc.desc: copy current sample to buffer, check read in PTS order
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_ReadSample_1015, TestSize.Level1)
{
    InitResource(g_mp4Mpeg2DtsUri, URI);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    EXPECT_TRUE(SetInitValue());
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            EXPECT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    printf("frames_[1]=%d | kFrames[1]=%d\n", frames_[1], keyFrames_[1]);
    EXPECT_EQ(frames_[0], 95);
    EXPECT_EQ(frames_[1], 308);
    EXPECT_EQ(keyFrames_[0], 10);
    EXPECT_EQ(keyFrames_[1], 308);
    RemoveValue();
}

#ifndef MPEG4_DEMUXER_CAPI_BUFFER_UNIT_TEST
/**
 * @tc.name: mp4Demuxer_ReadSample_1016
 * @tc.desc: copy current sample to buffer, samll bufferSize, check total read size
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_ReadSample_1016, TestSize.Level1)
{
    bufferSize_ = 1000;
    InitResource(g_mp4G711muUri, URI);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    EXPECT_TRUE(SetInitValue());
    vector<uint32_t> sizes = {0, 0};
    vector<uint32_t> expectSizes = {582040, 12000};
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            demuxer_->ReadSample(idx, sharedMem_, &info_, flag_);
            sizes[idx] += info_.size;
            CountFrames(idx);
        }
    }
    printf("sizes[0]=%d | expectSizes[0]=%d\n", sizes[0], expectSizes[0]);
    printf("sizes[1]=%d | expectSizes[1]=%d\n", sizes[1], expectSizes[1]);
    EXPECT_EQ(sizes[0], expectSizes[0]);
    EXPECT_EQ(sizes[1], expectSizes[1]);
    RemoveValue();
}
#endif

/**
 * @tc.name: mp4Demuxer_SeekToTime_1001
 * @tc.desc: seek to the specified time
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_SeekToTime_1001, TestSize.Level1)
{
    InitResource(g_mp4DelayUri, URI);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    // 边界情况
    EXPECT_EQ(demuxer_->SeekToTime(0, SeekMode::SEEK_PREVIOUS_SYNC), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SeekToTime(800, SeekMode::SEEK_PREVIOUS_SYNC), AV_ERR_OK);
    EXPECT_NE(demuxer_->SeekToTime(-10, SeekMode::SEEK_PREVIOUS_SYNC), AV_ERR_OK);
    EXPECT_NE(demuxer_->SeekToTime(12000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    // 普通时刻
    list<int64_t> toPtsList = {1000, 2000, 2550, 3400, 9050};
    vector<int32_t> videoVals = {602, 602, 602, 542, 542, 542, 506, 518, 506, 458, 458, 458, 110, 122, 122};
    vector<int32_t> audioVals = {390, 391, 390, 347, 348, 347, 321, 331, 321, 287, 288, 287,  37,  46,  46};
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            ret_ = demuxer_->SeekToTime(*toPts, *mode);
            if (ret_ != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret_);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames_[0]=%d | kFrames[0]=%d\n", *toPts, frames_[0], keyFrames_[0]);
            printf("time = %" PRId64 " | frames_[1]=%d | kFrames[1]=%d\n", *toPts, frames_[1], keyFrames_[1]);
            EXPECT_EQ(frames_[0], videoVals[numbers_]);
            EXPECT_EQ(frames_[1], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: mp4Demuxer_SeekToTime_1002
 * @tc.desc: seek to the specified time
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_SeekToTime_1002, TestSize.Level1)
{
    InitResource(g_fmp4MeetingUri, URI);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    // 边界情况
    EXPECT_NE(demuxer_->SeekToTime(-10, SeekMode::SEEK_PREVIOUS_SYNC), AV_ERR_OK);
    EXPECT_NE(demuxer_->SeekToTime(15000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    // 普通时刻
    list<int64_t> toPtsList = {0, 800, 2500, 3400, 9500};
    vector<int32_t> videoVals = {178, 178, 178, 163, 178, 163, 148, 148, 148, 133, 133, 133,  43,  43,  43};
    vector<int32_t> audioVals = {548, 548, 548, 505, 548, 505, 461, 461, 461, 417, 417, 417, 156, 156, 156};
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            ret_ = demuxer_->SeekToTime(*toPts, *mode);
            if (ret_ != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret_);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames_[0]=%d | kFrames[0]=%d\n", *toPts, frames_[0], keyFrames_[0]);
            printf("time = %" PRId64 " | frames_[1]=%d | kFrames[1]=%d\n", *toPts, frames_[1], keyFrames_[1]);
            EXPECT_EQ(frames_[0], videoVals[numbers_]);
            EXPECT_EQ(frames_[1], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: mp4Demuxer_SeekToTime_1003
 * @tc.desc: seek to the specified time
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_SeekToTime_1003, TestSize.Level1)
{
    InitResource(g_mp4WvttUri, URI);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(2), AV_ERR_OK);
    // 边界情况
    EXPECT_NE(demuxer_->SeekToTime(-10, SeekMode::SEEK_PREVIOUS_SYNC), AV_ERR_OK);
    EXPECT_NE(demuxer_->SeekToTime(15000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    // 普通时刻
    list<int64_t> toPtsList = {500, 800, 2500, 3400, 9050};
    vector<int32_t> videoVals = {574, 574, 574, 554, 564, 554, 454, 454, 454, 394, 404, 404, 54, 64, 64};
    vector<int32_t> audioVals = {411, 412, 412, 397, 405, 398, 325, 326, 326, 282, 290, 290, 38, 46, 46};
    vector<int32_t> subtitleVals = { 8, 8, 8, 8, 8, 8, 5, 5, 5, 4, 4, 4, 0, 0, 0};
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            ret_ = demuxer_->SeekToTime(*toPts, *mode);
            if (ret_ != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret_);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames_[0]=%d | kFrames[0]=%d\n", *toPts, frames_[0], keyFrames_[0]);
            printf("time = %" PRId64 " | frames_[1]=%d | kFrames[1]=%d\n", *toPts, frames_[1], keyFrames_[1]);
            printf("time = %" PRId64 " | frames_[2]=%d | kFrames[2]=%d\n", *toPts, frames_[2], keyFrames_[2]);
            EXPECT_EQ(frames_[0], videoVals[numbers_]);
            EXPECT_EQ(frames_[1], audioVals[numbers_]);
            EXPECT_EQ(frames_[2], subtitleVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: mp4Demuxer_SeekToTime_1004
 * @tc.desc: seek to the specified time
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_SeekToTime_1004, TestSize.Level1)
{
    InitResource(g_mp4G711muUri, URI);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    // 边界情况
    EXPECT_EQ(demuxer_->SeekToTime(0, SeekMode::SEEK_PREVIOUS_SYNC), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SeekToTime(800, SeekMode::SEEK_PREVIOUS_SYNC), AV_ERR_OK);
    EXPECT_NE(demuxer_->SeekToTime(-10, SeekMode::SEEK_PREVIOUS_SYNC), AV_ERR_OK);
    EXPECT_NE(demuxer_->SeekToTime(2000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    // 普通时刻
    list<int64_t> toPtsList = {500, 800, 1000, 1200, 1333};
    vector<int32_t> videoVals = {25, 35, 35, 15, 25, 25, 15, 15, 15, 5, 15, 5, 5, 15, 5};
    vector<int32_t> audioVals = {25, 36, 36, 15, 26, 25, 15, 16, 15, 5, 16, 5, 5, 16, 5};
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            ret_ = demuxer_->SeekToTime(*toPts, *mode);
            if (ret_ != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret_);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames_[0]=%d | kFrames[0]=%d\n", *toPts, frames_[0], keyFrames_[0]);
            printf("time = %" PRId64 " | frames_[1]=%d | kFrames[1]=%d\n", *toPts, frames_[1], keyFrames_[1]);
            EXPECT_EQ(frames_[0], videoVals[numbers_]);
            EXPECT_EQ(frames_[1], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: mp4Demuxer_SeekToTime_1005
 * @tc.desc: seek to the specified time
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_SeekToTime_1005, TestSize.Level1)
{
    InitResource(g_fmp4SidxUri, URI);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    // 边界情况
    EXPECT_NE(demuxer_->SeekToTime(-10, SeekMode::SEEK_PREVIOUS_SYNC), AV_ERR_OK);
    EXPECT_NE(demuxer_->SeekToTime(15000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    // 普通时刻
    list<int64_t> toPtsList = {0, 800, 2500, 3400, 9500};
    vector<int32_t> videoVals = {604, 604, 604, 574, 574, 574, 454, 454, 454, 424, 424, 424, 34, 34, 34};
    vector<int32_t> audioVals = {387, 387, 387, 368, 368, 368, 292, 292, 292, 272, 272, 272, 24, 24, 24};
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            ret_ = demuxer_->SeekToTime(*toPts, *mode);
            if (ret_ != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret_);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames_[0]=%d | kFrames[0]=%d\n", *toPts, frames_[0], keyFrames_[0]);
            printf("time = %" PRId64 " | frames_[1]=%d | kFrames[1]=%d\n", *toPts, frames_[1], keyFrames_[1]);
            EXPECT_EQ(frames_[0], videoVals[numbers_]);
            EXPECT_EQ(frames_[1], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: mp4Demuxer_SeekToTime_1006
 * @tc.desc: seek to the specified time
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_SeekToTime_1006, TestSize.Level1)
{
    InitResource(g_fmp4DoubleTopSidxUri, URI);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    // 边界情况
    EXPECT_NE(demuxer_->SeekToTime(-10, SeekMode::SEEK_PREVIOUS_SYNC), AV_ERR_OK);
    EXPECT_NE(demuxer_->SeekToTime(15000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    // 普通时刻
    list<int64_t> toPtsList = {0, 800, 2500, 3400, 9500};
    vector<int32_t> videoVals = {604, 604, 604, 574, 574, 574, 454, 454, 454, 424, 424, 424, 34, 34, 34};
    vector<int32_t> audioVals = {387, 387, 387, 368, 368, 368, 292, 292, 292, 272, 272, 272, 24, 24, 24};
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            ret_ = demuxer_->SeekToTime(*toPts, *mode);
            if (ret_ != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret_);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames_[0]=%d | kFrames[0]=%d\n", *toPts, frames_[0], keyFrames_[0]);
            printf("time = %" PRId64 " | frames_[1]=%d | kFrames[1]=%d\n", *toPts, frames_[1], keyFrames_[1]);
            EXPECT_EQ(frames_[0], videoVals[numbers_]);
            EXPECT_EQ(frames_[1], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: mp4Demuxer_SeekToTime_1007
 * @tc.desc: seek to the specified time
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_SeekToTime_1007, TestSize.Level1)
{
    InitResource(g_fmp4MultiSidxUri, URI);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    // 边界情况
    EXPECT_NE(demuxer_->SeekToTime(-10, SeekMode::SEEK_PREVIOUS_SYNC), AV_ERR_OK);
    EXPECT_NE(demuxer_->SeekToTime(15000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    // 普通时刻
    list<int64_t> toPtsList = {0, 800, 2500, 3400, 9500};
    vector<int32_t> videoVals = {324, 324, 324, 324, 324, 324, 264, 264, 264, 234, 234, 234, 54, 54, 54};
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            ret_ = demuxer_->SeekToTime(*toPts, *mode);
            if (ret_ != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret_);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames_[0]=%d | kFrames[0]=%d\n", *toPts, frames_[0], keyFrames_[0]);
            EXPECT_EQ(frames_[0], videoVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: mp4Demuxer_SeekToTime_1008
 * @tc.desc: seek to the specified time
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_SeekToTime_1008, TestSize.Level1)
{
    InitResource(g_fmp4MoreSidxUri, URI);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    // 边界情况
    EXPECT_NE(demuxer_->SeekToTime(-10, SeekMode::SEEK_PREVIOUS_SYNC), AV_ERR_OK);
    EXPECT_NE(demuxer_->SeekToTime(8000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    // 普通时刻
    list<int64_t> toPtsList = {0, 800, 2500, 3400, 6500};
    vector<int32_t> videoVals = {420, 420, 420, 390, 390, 390, 270, 270, 270, 240, 240, 240, 30, 30, 30};
    vector<int32_t> audioVals = {268, 268, 268, 249, 249, 249, 173, 173, 173, 153, 153, 153, 19, 19, 19};
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            ret_ = demuxer_->SeekToTime(*toPts, *mode);
            if (ret_ != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret_);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames_[0]=%d | kFrames[0]=%d\n", *toPts, frames_[0], keyFrames_[0]);
            printf("time = %" PRId64 " | frames_[1]=%d | kFrames[1]=%d\n", *toPts, frames_[1], keyFrames_[1]);
            EXPECT_EQ(frames_[0], videoVals[numbers_]);
            EXPECT_EQ(frames_[1], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: mp4Demuxer_SeekToTime_1009
 * @tc.desc: seek to the specified time
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_SeekToTime_1009, TestSize.Level1)
{
    InitResource(g_mp4ShortAudioUri, URI);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    // 边界情况
    EXPECT_EQ(demuxer_->SeekToTime(0, SeekMode::SEEK_PREVIOUS_SYNC), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SeekToTime(800, SeekMode::SEEK_PREVIOUS_SYNC), AV_ERR_OK);
    EXPECT_NE(demuxer_->SeekToTime(-10, SeekMode::SEEK_PREVIOUS_SYNC), AV_ERR_OK);
    EXPECT_NE(demuxer_->SeekToTime(12000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    // 普通时刻
    list<int64_t> toPtsList = {500, 1500, 2550, 3000, 3600};
    vector<int32_t> videoVals = {100, 110, 110, 70, 80, 80, 40, 50, 40, 30, 30, 30, 10, 20, 10};
    vector<int32_t> audioVals = { 85,  85,  85, 58, 73, 72, 15, 30, 15,  0,  0,  0,  0,  0,  0};
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            ret_ = demuxer_->SeekToTime(*toPts, *mode);
            if (ret_ != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret_);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames_[0]=%d | kFrames[0]=%d\n", *toPts, frames_[0], keyFrames_[0]);
            printf("time = %" PRId64 " | frames_[1]=%d | kFrames[1]=%d\n", *toPts, frames_[1], keyFrames_[1]);
            EXPECT_EQ(frames_[0], videoVals[numbers_]);
            EXPECT_EQ(frames_[1], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: mp4Demuxer_SeekToTime_1010
 * @tc.desc: seek to the specified time
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_SeekToTime_1010, TestSize.Level1)
{
    InitResource(g_mp4ShortVideoUri, URI);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    // 边界情况
    EXPECT_EQ(demuxer_->SeekToTime(0, SeekMode::SEEK_PREVIOUS_SYNC), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SeekToTime(800, SeekMode::SEEK_PREVIOUS_SYNC), AV_ERR_OK);
    EXPECT_NE(demuxer_->SeekToTime(-10, SeekMode::SEEK_PREVIOUS_SYNC), AV_ERR_OK);
    EXPECT_NE(demuxer_->SeekToTime(12000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    // 普通时刻
    list<int64_t> toPtsList = {500, 2066, 2550, 7033, 8500};
    vector<int32_t> videoVals = {150, 150, 150, 150, 150, 150, 130, 140, 140,   0,  10,  10,  0,  0,  0};
    vector<int32_t> audioVals = {445, 446, 446, 372, 373, 372, 340, 357, 357, 139, 154, 153, 70, 71, 71};
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            ret_ = demuxer_->SeekToTime(*toPts, *mode);
            if (ret_ != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret_);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames_[0]=%d | kFrames[0]=%d\n", *toPts, frames_[0], keyFrames_[0]);
            printf("time = %" PRId64 " | frames_[1]=%d | kFrames[1]=%d\n", *toPts, frames_[1], keyFrames_[1]);
            EXPECT_EQ(frames_[0], videoVals[numbers_]);
            EXPECT_EQ(frames_[1], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: mp4Demuxer_SeekToTime_1011
 * @tc.desc: seek to the specified time
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_SeekToTime_1011, TestSize.Level1)
{
    InitResource(g_mp4H263AmrwbUri, URI);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    // 边界情况
    EXPECT_EQ(demuxer_->SeekToTime(0, SeekMode::SEEK_PREVIOUS_SYNC), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SeekToTime(800, SeekMode::SEEK_PREVIOUS_SYNC), AV_ERR_OK);
    EXPECT_NE(demuxer_->SeekToTime(-10, SeekMode::SEEK_PREVIOUS_SYNC), AV_ERR_OK);
    EXPECT_NE(demuxer_->SeekToTime(5000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    // 普通时刻
    list<int64_t> toPtsList = {500, 800, 2100, 2633, 3166};
    vector<int32_t> videoVals = { 75,  87,  87,  75,  75,  75, 27, 39, 39, 15, 27, 15, 3, 15, 3};
    vector<int32_t> audioVals = {124, 145, 145, 124, 125, 125, 44, 65, 65, 24, 45, 25, 4, 25, 5};
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            ret_ = demuxer_->SeekToTime(*toPts, *mode);
            if (ret_ != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret_);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames_[0]=%d | kFrames[0]=%d\n", *toPts, frames_[0], keyFrames_[0]);
            printf("time = %" PRId64 " | frames_[1]=%d | kFrames[1]=%d\n", *toPts, frames_[1], keyFrames_[1]);
            EXPECT_EQ(frames_[0], videoVals[numbers_]);
            EXPECT_EQ(frames_[1], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: mp4Demuxer_SeekToTime_1012
 * @tc.desc: seek to the specified time
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_SeekToTime_1012, TestSize.Level1)
{
    InitResource(g_mp4Mpeg4OpusUri, URI);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    // 边界情况
    EXPECT_EQ(demuxer_->SeekToTime(0, SeekMode::SEEK_PREVIOUS_SYNC), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SeekToTime(800, SeekMode::SEEK_PREVIOUS_SYNC), AV_ERR_OK);
    EXPECT_NE(demuxer_->SeekToTime(-10, SeekMode::SEEK_PREVIOUS_SYNC), AV_ERR_OK);
    EXPECT_NE(demuxer_->SeekToTime(5000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    // 普通时刻
    list<int64_t> toPtsList = {500, 800, 1500, 2100, 2633};
    vector<int32_t> videoVals = { 83,  95,  83,  71,  83,  71, 47, 59, 59, 35, 47, 35, 11, 23, 23};
    vector<int32_t> audioVals = {138, 165, 138, 118, 139, 118, 78, 99, 98, 58, 79, 58, 18, 39, 38};
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            ret_ = demuxer_->SeekToTime(*toPts, *mode);
            if (ret_ != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret_);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames_[0]=%d | kFrames[0]=%d\n", *toPts, frames_[0], keyFrames_[0]);
            printf("time = %" PRId64 " | frames_[1]=%d | kFrames[1]=%d\n", *toPts, frames_[1], keyFrames_[1]);
            EXPECT_EQ(frames_[0], videoVals[numbers_]);
            EXPECT_EQ(frames_[1], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: mp4Demuxer_SeekToTime_1013
 * @tc.desc: seek to the specified time
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_SeekToTime_1013, TestSize.Level1)
{
    InitResource(g_mp4Mpeg4AmrnbUri, URI);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    // 边界情况
    EXPECT_EQ(demuxer_->SeekToTime(0, SeekMode::SEEK_PREVIOUS_SYNC), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SeekToTime(800, SeekMode::SEEK_PREVIOUS_SYNC), AV_ERR_OK);
    EXPECT_NE(demuxer_->SeekToTime(-10, SeekMode::SEEK_PREVIOUS_SYNC), AV_ERR_OK);
    EXPECT_NE(demuxer_->SeekToTime(5000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    // 普通时刻
    list<int64_t> toPtsList = {500, 800, 2100, 2633, 3166};
    vector<int32_t> videoVals = { 83,  95,  83,  71,  83,  71, 35, 47, 35, 11, 23, 23, 0, 11, 11};
    vector<int32_t> audioVals = {138, 165, 138, 118, 139, 118, 58, 79, 58, 18, 39, 38, 1, 19, 18};
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            ret_ = demuxer_->SeekToTime(*toPts, *mode);
            if (ret_ != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret_);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames_[0]=%d | kFrames[0]=%d\n", *toPts, frames_[0], keyFrames_[0]);
            printf("time = %" PRId64 " | frames_[1]=%d | kFrames[1]=%d\n", *toPts, frames_[1], keyFrames_[1]);
            EXPECT_EQ(frames_[0], videoVals[numbers_]);
            EXPECT_EQ(frames_[1], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: mp4Demuxer_SeekToTime_1014
 * @tc.desc: seek to the specified time
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_SeekToTime_1014, TestSize.Level1)
{
    InitResource(g_mp4Mpeg2AlacUri, URI);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    // 边界情况
    EXPECT_EQ(demuxer_->SeekToTime(0, SeekMode::SEEK_PREVIOUS_SYNC), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SeekToTime(800, SeekMode::SEEK_PREVIOUS_SYNC), AV_ERR_OK);
    EXPECT_NE(demuxer_->SeekToTime(-10, SeekMode::SEEK_PREVIOUS_SYNC), AV_ERR_OK);
    EXPECT_NE(demuxer_->SeekToTime(5000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    // 普通时刻
    list<int64_t> toPtsList = {500, 800, 1500, 2100, 2633};
    vector<int32_t> videoVals = { 75, 85, 85, 75, 75, 75, 45, 55, 55, 35, 45, 35, 15, 25, 25};
    vector<int32_t> audioVals = { 29, 34, 34, 29, 30, 30, 17, 22, 22, 14, 18, 14,  6, 11, 10};
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            ret_ = demuxer_->SeekToTime(*toPts, *mode);
            if (ret_ != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret_);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames_[0]=%d | kFrames[0]=%d\n", *toPts, frames_[0], keyFrames_[0]);
            printf("time = %" PRId64 " | frames_[1]=%d | kFrames[1]=%d\n", *toPts, frames_[1], keyFrames_[1]);
            EXPECT_EQ(frames_[0], videoVals[numbers_]);
            EXPECT_EQ(frames_[1], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: mp4Demuxer_SeekToTime_1015
 * @tc.desc: seek to the specified time
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_SeekToTime_1015, TestSize.Level1)
{
    InitResource(g_mp4Mpeg2DtsUri, URI);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    // 边界情况
    EXPECT_EQ(demuxer_->SeekToTime(0, SeekMode::SEEK_PREVIOUS_SYNC), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SeekToTime(800, SeekMode::SEEK_PREVIOUS_SYNC), AV_ERR_OK);
    EXPECT_NE(demuxer_->SeekToTime(-10, SeekMode::SEEK_PREVIOUS_SYNC), AV_ERR_OK);
    EXPECT_NE(demuxer_->SeekToTime(5000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    // 普通时刻
    list<int64_t> toPtsList = {500, 800, 2100, 2633};
    vector<int32_t> videoVals = { 75,  85,  85,  75,  75,  75, 35, 45, 35, 15, 25, 25};
    vector<int32_t> audioVals = {232, 265, 264, 232, 233, 233, 108, 140, 108, 45, 77, 77};
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            ret_ = demuxer_->SeekToTime(*toPts, *mode);
            if (ret_ != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret_);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames_[0]=%d | kFrames[0]=%d\n", *toPts, frames_[0], keyFrames_[0]);
            printf("time = %" PRId64 " | frames_[1]=%d | kFrames[1]=%d\n", *toPts, frames_[1], keyFrames_[1]);
            EXPECT_EQ(frames_[0], videoVals[numbers_]);
            EXPECT_EQ(frames_[1], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}
} // namespace