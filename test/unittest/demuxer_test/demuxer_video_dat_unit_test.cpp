/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
#include "demuxer_unit_test.h"

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

list<SeekMode> seekModes = {SeekMode::SEEK_NEXT_SYNC, SeekMode::SEEK_PREVIOUS_SYNC,
    SeekMode::SEEK_CLOSEST_SYNC};

string g_datPath1 = TEST_FILE_PATH + string("dat_mpeg4_aac.dat");
string g_datUri1 = TEST_URI_PATH + string("dat_mpeg4_aac.dat");
string g_datPath2 = TEST_FILE_PATH + string("dat_h264_mp3.dat");
string g_datUri2 = TEST_URI_PATH + string("dat_h264_mp3.dat");
string g_datPath3 = TEST_FILE_PATH + string("dat_h265_aac.dat");
string g_datUri3 = TEST_URI_PATH + string("dat_h265_aac.dat");

} //namespace

namespace {
/**
 * @tc.name: Demuxer_DAT_ReadSample_0001
 * @tc.desc: Read sample test for DAT(mpeg4+aac) format, local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_DAT_ReadSample_0001, TestSize.Level1)
{
    InitResource(g_datPath1, LOCAL);
    ASSERT_TRUE(initStatus_);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    ASSERT_TRUE(SetInitValue());
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            ASSERT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    printf("frames_[1]=%d | kFrames[1]=%d\n", frames_[1], keyFrames_[1]);
    ASSERT_EQ(frames_[0], 250);
    ASSERT_EQ(frames_[1], 441);
    ASSERT_EQ(keyFrames_[0], 25);
    ASSERT_EQ(keyFrames_[1], 441);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_DAT_ReadSample_0002
 * @tc.desc: Read sample test for DAT(mpeg4+aac) format, URI
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_DAT_ReadSample_0002, TestSize.Level1)
{
    InitResource(g_datUri1, URI);
    ASSERT_TRUE(initStatus_);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    ASSERT_TRUE(SetInitValue());
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            ASSERT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    printf("frames_[1]=%d | kFrames[1]=%d\n", frames_[1], keyFrames_[1]);
    ASSERT_EQ(frames_[0], 250);
    ASSERT_EQ(frames_[1], 441);
    ASSERT_EQ(keyFrames_[0], 25);
    ASSERT_EQ(keyFrames_[1], 441);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_DAT_SeekToTime_0003
 * @tc.desc: seek to the specified time for DAT(mpeg4+aac) format, Local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_DAT_SeekToTime_0003, TestSize.Level1)
{
    InitResource(g_datPath1, LOCAL);
    ASSERT_TRUE(initStatus_);
    ASSERT_TRUE(SetInitValue());
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 2000};
    vector<int32_t> videoVals = {75, 75, 75, 25, 25, 25};
    vector<int32_t> audioVals = {441, 441, 441, 309, 309, 309};
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
            printf("time = %" PRId64 " | frames_[0]=%d\n", *toPts, frames_[0]);
            printf("time = %" PRId64 " | frames_[1]=%d\n", *toPts, frames_[1]);
            ASSERT_EQ(frames_[0], videoVals[numbers_]);
            ASSERT_EQ(frames_[1], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: Demuxer_DAT_SeekToTime_0004
 * @tc.desc: seek to the specified time for DAT(mpeg4+aac) format, URI
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_DAT_SeekToTime_0004, TestSize.Level1)
{
    InitResource(g_datUri1, URI);
    ASSERT_TRUE(initStatus_);
    ASSERT_TRUE(SetInitValue());
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 2000};
    vector<int32_t> videoVals = {75, 75, 75, 25, 25, 25};
    vector<int32_t> audioVals = {441, 441, 441, 309, 309, 309};
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
            printf("time = %" PRId64 " | frames_[0]=%d\n", *toPts, frames_[0]);
            printf("time = %" PRId64 " | frames_[1]=%d\n", *toPts, frames_[1]);
            ASSERT_EQ(frames_[0], videoVals[numbers_]);
            ASSERT_EQ(frames_[1], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: Demuxer_DAT_ReadSample_0005
 * @tc.desc: Read sample test for DAT(h264+mp3) format, local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_DAT_ReadSample_0005, TestSize.Level1)
{
    InitResource(g_datPath2, LOCAL);
    ASSERT_TRUE(initStatus_);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    ASSERT_TRUE(SetInitValue());
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            ASSERT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    printf("frames_[1]=%d | kFrames[1]=%d\n", frames_[1], keyFrames_[1]);
    ASSERT_EQ(frames_[0], 300);
    ASSERT_EQ(frames_[1], 469);
    ASSERT_EQ(keyFrames_[0], 10);
    ASSERT_EQ(keyFrames_[1], 469);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_DAT_ReadSample_0006
 * @tc.desc: Read sample test for DAT(h264+mp3) format, URI
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_DAT_ReadSample_0006, TestSize.Level1)
{
    InitResource(g_datUri2, URI);
    ASSERT_TRUE(initStatus_);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    ASSERT_TRUE(SetInitValue());
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            ASSERT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    printf("frames_[1]=%d | kFrames[1]=%d\n", frames_[1], keyFrames_[1]);
    ASSERT_EQ(frames_[0], 300);
    ASSERT_EQ(frames_[1], 469);
    ASSERT_EQ(keyFrames_[0], 10);
    ASSERT_EQ(keyFrames_[1], 469);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_DAT_SeekToTime_0007
 * @tc.desc: seek to the specified time for DAT(h264+mp3) format, Local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_DAT_SeekToTime_0007, TestSize.Level1)
{
    InitResource(g_datPath2, LOCAL);
    ASSERT_TRUE(initStatus_);
    ASSERT_TRUE(SetInitValue());
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 2000};
    vector<int32_t> videoVals = {90, 90, 90, 30, 30, 30};
    vector<int32_t> audioVals = {469, 469, 469, 234, 234, 234};
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
            printf("time = %" PRId64 " | frames_[0]=%d\n", *toPts, frames_[0]);
            printf("time = %" PRId64 " | frames_[1]=%d\n", *toPts, frames_[1]);
            ASSERT_EQ(frames_[0], videoVals[numbers_]);
            ASSERT_EQ(frames_[1], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: Demuxer_DAT_SeekToTime_0008
 * @tc.desc: seek to the specified time for DAT(h264+mp3) format, URI
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_DAT_SeekToTime_0008, TestSize.Level1)
{
    InitResource(g_datUri2, URI);
    ASSERT_TRUE(initStatus_);
    ASSERT_TRUE(SetInitValue());
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 2000};
    vector<int32_t> videoVals = {90, 90, 90, 30, 30, 30};
    vector<int32_t> audioVals = {469, 469, 469, 234, 234, 234};
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
            printf("time = %" PRId64 " | frames_[0]=%d\n", *toPts, frames_[0]);
            printf("time = %" PRId64 " | frames_[1]=%d\n", *toPts, frames_[1]);
            ASSERT_EQ(frames_[0], videoVals[numbers_]);
            ASSERT_EQ(frames_[1], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: Demuxer_DAT_ReadSample_0009
 * @tc.desc: Read sample test for DAT(h265+aac) format, local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_DAT_ReadSample_0009, TestSize.Level1)
{
    InitResource(g_datPath3, LOCAL);
    ASSERT_TRUE(initStatus_);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    ASSERT_TRUE(SetInitValue());
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            ASSERT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    printf("frames_[1]=%d | kFrames[1]=%d\n", frames_[1], keyFrames_[1]);
    ASSERT_EQ(frames_[0], 600);
    ASSERT_EQ(frames_[1], 938);
    ASSERT_EQ(keyFrames_[0], 10);
    ASSERT_EQ(keyFrames_[1], 938);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_DAT_ReadSample_0010
 * @tc.desc: Read sample test for DAT(h265+aac) format, URI
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_DAT_ReadSample_0010, TestSize.Level1)
{
    InitResource(g_datUri3, URI);
    ASSERT_TRUE(initStatus_);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    ASSERT_TRUE(SetInitValue());
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            ASSERT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    printf("frames_[1]=%d | kFrames[1]=%d\n", frames_[1], keyFrames_[1]);
    ASSERT_EQ(frames_[0], 600);
    ASSERT_EQ(frames_[1], 938);
    ASSERT_EQ(keyFrames_[0], 10);
    ASSERT_EQ(keyFrames_[1], 938);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_DAT_SeekToTime_0011
 * @tc.desc: seek to the specified time for DAT(h265+aac) format, Local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_DAT_SeekToTime_0011, TestSize.Level1)
{
    InitResource(g_datPath3, LOCAL);
    ASSERT_TRUE(initStatus_);
    ASSERT_TRUE(SetInitValue());
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 2000};
    vector<int32_t> videoVals = {90, 90, 90, 30, 30, 30};
    vector<int32_t> audioVals = {938, 938, 938, 281, 281, 281};
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
            printf("time = %" PRId64 " | frames_[0]=%d\n", *toPts, frames_[0]);
            printf("time = %" PRId64 " | frames_[1]=%d\n", *toPts, frames_[1]);
            ASSERT_EQ(frames_[0], videoVals[numbers_]);
            ASSERT_EQ(frames_[1], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: Demuxer_DAT_SeekToTime_0012
 * @tc.desc: seek to the specified time for DAT(h265+aac) format, URI
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_DAT_SeekToTime_0012, TestSize.Level1)
{
    InitResource(g_datUri3, URI);
    ASSERT_TRUE(initStatus_);
    ASSERT_TRUE(SetInitValue());
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 2000};
    vector<int32_t> videoVals = {90, 90, 90, 30, 30, 30};
    vector<int32_t> audioVals = {938, 938, 938, 281, 281, 281};
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
            printf("time = %" PRId64 " | frames_[0]=%d\n", *toPts, frames_[0]);
            printf("time = %" PRId64 " | frames_[1]=%d\n", *toPts, frames_[1]);
            ASSERT_EQ(frames_[0], videoVals[numbers_]);
            ASSERT_EQ(frames_[1], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

} //namespace