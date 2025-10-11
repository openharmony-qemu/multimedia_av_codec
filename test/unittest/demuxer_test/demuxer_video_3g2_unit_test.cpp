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
string g_3g2Path = TEST_FILE_PATH + string("3g2_mp4v_mp4a.3g2");
string g_3g2Uri = TEST_URI_PATH + string("3g2_mp4v_mp4a.3g2");
string g_3g2Path2 = TEST_FILE_PATH + string("3g2_h264_aac.3g2");
string g_3g2Uri2 = TEST_URI_PATH + string("3g2_h264_aac.3g2");

} //namespace

namespace {
/**
 * @tc.name: Demuxer_3G2_ReadSample_0001
 * @tc.desc: Read sample test for timed metadata track,local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_3G2_ReadSample_0001, TestSize.Level1)
{
    InitResource(g_3g2Path, LOCAL);
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
    ASSERT_EQ(frames_[0], 937);
    ASSERT_EQ(frames_[1], 1121);
    ASSERT_EQ(keyFrames_[0], 81);
    ASSERT_EQ(keyFrames_[1], 1121);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_3G2_ReadSample_0002
 * @tc.desc: Read sample test for timed metadata track,URI
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_3G2_ReadSample_0002, TestSize.Level1)
{
    InitResource(g_3g2Uri, URI);
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
    ASSERT_EQ(frames_[0], 937);
    ASSERT_EQ(frames_[1], 1121);
    ASSERT_EQ(keyFrames_[0], 81);
    ASSERT_EQ(keyFrames_[1], 1121);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_3G2_SeekToTime_0003
 * @tc.desc: seek to the specified time,Local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_3G2_SeekToTime_0003, TestSize.Level1)
{
    InitResource(g_3g2Path, LOCAL);
    ASSERT_TRUE(initStatus_);
    ASSERT_TRUE(SetInitValue());
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 2700}; // ms
    vector<int32_t> videoVals = {937, 937, 937, 878, 890, 890};
    vector<int32_t> audioVals = {1120, 1120, 1120, 1049, 1064, 1064};
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
 * @tc.name: Demuxer_3G2_SeekToTime_0004
 * @tc.desc: seek to the specified time,URI
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_3G2_SeekToTime_0004, TestSize.Level1)
{
    InitResource(g_3g2Uri, URI);
    ASSERT_TRUE(initStatus_);
    ASSERT_TRUE(SetInitValue());
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 2700}; // ms
    vector<int32_t> videoVals = {937, 937, 937, 878, 890, 890};
    vector<int32_t> audioVals = {1120, 1120, 1120, 1049, 1064, 1064};
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
 * @tc.name: Demuxer_3G2_ReadSample_0005
 * @tc.desc: Read sample test for 3G2 H264+AAC format, local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_3G2_ReadSample_0005, TestSize.Level1)
{
    InitResource(g_3g2Path2, LOCAL);
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
    ASSERT_EQ(frames_[1], 217);
    ASSERT_EQ(keyFrames_[0], 2);
    ASSERT_EQ(keyFrames_[1], 217);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_3G2_ReadSample_0006
 * @tc.desc: Read sample test for 3G2 H264+AAC format, URI
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_3G2_ReadSample_0006, TestSize.Level1)
{
    InitResource(g_3g2Uri2, URI);
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
    ASSERT_EQ(frames_[1], 217);
    ASSERT_EQ(keyFrames_[0], 2);
    ASSERT_EQ(keyFrames_[1], 217);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_3G2_SeekToTime_0007
 * @tc.desc: seek to the specified time,Local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_3G2_SeekToTime_0007, TestSize.Level1)
{
    InitResource(g_3g2Path2, LOCAL);
    ASSERT_TRUE(initStatus_);
    ASSERT_TRUE(SetInitValue());
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 9000}; // ms
    vector<int32_t> videoVals = {300, 300, 300, 50, 50};
    vector<int32_t> audioVals = {217, 217, 217, 38, 38};
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
 * @tc.name: Demuxer_3G2_SeekToTime_0008
 * @tc.desc: seek to the specified time, URI
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_3G2_SeekToTime_0008, TestSize.Level1)
{
    InitResource(g_3g2Uri2, URI);
    ASSERT_TRUE(initStatus_);
    ASSERT_TRUE(SetInitValue());
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 9000}; // ms
    vector<int32_t> videoVals = {300, 300, 300, 50, 50};
    vector<int32_t> audioVals = {217, 217, 217, 38, 38};
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