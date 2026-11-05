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
#include "test_http_port.h"

#define URI false

using namespace OHOS;
using namespace OHOS::Media;
using namespace OHOS::MediaAVCodec;
using namespace testing::ext;
using namespace std;

namespace {
unique_ptr<FileServerDemo> server = nullptr;
static const string TEST_FILE_PATH = "/data/test/media/";
static const string TEST_URI_PATH = "http://127.0.0.1:" AVCODEC_TEST_HTTP_PORT_STR "/";
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
    ASSERT_NE(source_, nullptr);
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
    for (int32_t i = 0; i < 3; i++) {
        printf("frames_[%d]=%d | kFrames[%d]=%d\n", i, frames_[i], i, keyFrames_[i]);
    }
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

} // namespace
