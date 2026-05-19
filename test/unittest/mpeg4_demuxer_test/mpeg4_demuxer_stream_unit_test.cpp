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
#include <map>
#include <cmath>
#include "gtest/gtest.h"
#include "avcodec_errors.h"
#include "media_description.h"
#include "file_server_demo.h"
#include "mpeg4_demuxer_unit_test.h"

#define LOCAL true
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

const std::string HEVC_LIB_PATH = std::string(AV_CODEC_PATH) + "/libav_codec_hevc_parser.z.so";
const int64_t SOURCE_OFFSET = 0;
list<SeekMode> seekModes = {SeekMode::SEEK_NEXT_SYNC, SeekMode::SEEK_PREVIOUS_SYNC,
    SeekMode::SEEK_CLOSEST_SYNC};

string g_mp4Path = TEST_FILE_PATH + string("test_264_B_Gop25_4sec_cover.mp4");
string g_mp4Uri = TEST_URI_PATH + string("test_264_B_Gop25_4sec_cover.mp4");
string g_m4aPath = TEST_FILE_PATH + string("audio/h264_fmp4.m4a");
string g_m4aUri = TEST_URI_PATH + string("audio/h264_fmp4.m4a");
} // namespace

void Mpeg4DemuxerUnitTest::InitResource(const std::string &path, bool local)
{
    printf("---- %s ------\n", path.c_str());
    if (local) {
        fd_ = OpenFile(path);
        int64_t size = GetFileSize(path);
        source_ = Mpeg4SourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size);
        ASSERT_NE(source_, nullptr);
    } else {
        source_ = Mpeg4SourceMockFactory::CreateSourceWithURI(const_cast<char*>(path.data()));
        ASSERT_NE(source_, nullptr);
    }
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, nbStreams_));
    demuxer_ = Mpeg4DemuxerMockFactory::CreateDemuxer(source_);
    ASSERT_NE(demuxer_, nullptr);
    initStatus_ = true;
}

void Mpeg4DemuxerUnitTest::ReadSample(const std::string &path, bool local, bool checkBufferInfo)
{
    InitResource(path, local);
    ASSERT_TRUE(initStatus_);
    EXPECT_TRUE(SetInitValue());
    for (auto idx : selectedTrackIds_) {
        EXPECT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);

    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            EXPECT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_, checkBufferInfo), AV_ERR_OK);
            CountFrames(idx);
        }
    }
}

namespace {
/**
 * @tc.name: mp4Demuxer_CreateDemuxer_2001
 * @tc.desc: create demuxer. Normal mp4, local
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_CreateDemuxer_2001, TestSize.Level1)
{
    InitResource(g_mp4Path, LOCAL);
    ASSERT_TRUE(initStatus_);
}

/**
 * @tc.name: mp4Demuxer_CreateDemuxer_3001
 * @tc.desc: create demuxer. Normal mp4, uri
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_CreateDemuxer_3001, TestSize.Level1)
{
    InitResource(g_mp4Uri, URI);
    ASSERT_TRUE(initStatus_);
}

/**
 * @tc.name: mp4Demuxer_UnselectTrackByID_2001
 * @tc.desc: select and remove track by ID
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_UnselectTrackByID_2001, TestSize.Level1)
{
    InitResource(g_mp4Path, LOCAL);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    EXPECT_NE(demuxer_->SelectTrackByID(2), AV_ERR_OK);
    EXPECT_NE(demuxer_->SelectTrackByID(3), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_NE(demuxer_->SelectTrackByID(-1), AV_ERR_OK);
    EXPECT_EQ(demuxer_->UnselectTrackByID(1), AV_ERR_OK);
    EXPECT_EQ(demuxer_->UnselectTrackByID(2), AV_ERR_OK);
    EXPECT_EQ(demuxer_->UnselectTrackByID(3), AV_ERR_OK);
    EXPECT_EQ(demuxer_->UnselectTrackByID(-1), AV_ERR_OK);
}

/**
 * @tc.name: mp4Demuxer_UnselectTrackByID_3001
 * @tc.desc: select and remove track by ID
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_UnselectTrackByID_3001, TestSize.Level1)
{
    InitResource(g_mp4Uri, URI);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    EXPECT_NE(demuxer_->SelectTrackByID(2), AV_ERR_OK);
    EXPECT_NE(demuxer_->SelectTrackByID(3), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_NE(demuxer_->SelectTrackByID(-1), AV_ERR_OK);
    EXPECT_EQ(demuxer_->UnselectTrackByID(1), AV_ERR_OK);
    EXPECT_EQ(demuxer_->UnselectTrackByID(2), AV_ERR_OK);
    EXPECT_EQ(demuxer_->UnselectTrackByID(3), AV_ERR_OK);
    EXPECT_EQ(demuxer_->UnselectTrackByID(-1), AV_ERR_OK);
}

/**
 * @tc.name: mp4Demuxer_ReadSample_2001
 * @tc.desc: copy current sample to buffer, check read in PTS order
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_ReadSample_2001, TestSize.Level1)
{
    InitResource(g_mp4Path, LOCAL);
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
    EXPECT_EQ(frames_[0], 103);
    EXPECT_EQ(frames_[1], 174);
    EXPECT_EQ(keyFrames_[0], 5);
    EXPECT_EQ(keyFrames_[1], 174);
    RemoveValue();
}

/**
 * @tc.name: mp4Demuxer_ReadSample_3001
 * @tc.desc: copy current sample to buffer, check read in PTS order
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_ReadSample_3001, TestSize.Level1)
{
    InitResource(g_mp4Uri, URI);
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
    EXPECT_EQ(frames_[0], 103);
    EXPECT_EQ(frames_[1], 174);
    EXPECT_EQ(keyFrames_[0], 5);
    EXPECT_EQ(keyFrames_[1], 174);
    RemoveValue();
}

/**
 * @tc.name: mp4Demuxer_SeekToTime_2002
 * @tc.desc: seek to the specified time, fd
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_SeekToTime_2002, TestSize.Level1)
{
    InitResource(g_m4aPath, LOCAL);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    // 边界情况
    EXPECT_EQ(demuxer_->SeekToTime(0, SeekMode::SEEK_PREVIOUS_SYNC), AV_ERR_OK);
    EXPECT_NE(demuxer_->SeekToTime(-10, SeekMode::SEEK_PREVIOUS_SYNC), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SeekToTime(10041, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    EXPECT_NE(demuxer_->SeekToTime(11000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    // 普通时刻
    list<int64_t> toPtsList = {0, 4500, 7000, 2000, 6000};
    vector<int32_t> audioVals = {433, 433, 433, 239, 240, 240, 131, 132, 132, 347, 348, 347, 175, 176, 175};
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
            EXPECT_EQ(frames_[0], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: mp4Demuxer_SeekToTime_3002
 * @tc.desc: seek to the specified time
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_SeekToTime_3002, TestSize.Level1)
{
    InitResource(g_m4aUri, URI);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    // 边界情况
    EXPECT_EQ(demuxer_->SeekToTime(0, SeekMode::SEEK_PREVIOUS_SYNC), AV_ERR_OK);
    EXPECT_NE(demuxer_->SeekToTime(-10, SeekMode::SEEK_PREVIOUS_SYNC), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SeekToTime(10041, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    EXPECT_NE(demuxer_->SeekToTime(11000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    // 普通时刻
    list<int64_t> toPtsList = {0, 4500, 7000, 2000, 6000};
    vector<int32_t> audioVals = {433, 433, 433, 239, 240, 240, 131, 132, 132, 347, 348, 347, 175, 176, 175};
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
            EXPECT_EQ(frames_[0], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

} // namespace
