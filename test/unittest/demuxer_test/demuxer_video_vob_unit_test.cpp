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
string g_mpegPath = TEST_FILE_PATH + string("mov_mpeg2_mp2.vob");
string g_mpegURL = TEST_URI_PATH + string("mov_mpeg2_mp2.vob");

} //namespace

namespace {
/**
 * @tc.name: Demuxer_ReadSample_4280
 * @tc.desc: Read sample test for timed metadata track,local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_4280, TestSize.Level1)
{
    InitResource(g_mpegPath, LOCAL);
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
    ASSERT_EQ(frames_[0], 4389);
    ASSERT_EQ(frames_[1], 7631);
    ASSERT_EQ(keyFrames_[0], 397);
    ASSERT_EQ(keyFrames_[1], 7631);
    RemoveValue();
}


/**
 * @tc.name: Demuxer_ReadSample_4281
 * @tc.desc: Read sample test for timed metadata track,local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_4281, TestSize.Level1)
{
    InitResource(g_mpegURL, URI);
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
    ASSERT_EQ(frames_[0], 4389);
    ASSERT_EQ(frames_[1], 7631);
    ASSERT_EQ(keyFrames_[0], 397);
    ASSERT_EQ(keyFrames_[1], 7631);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_SeekToTime_4282
 * @tc.desc: seek to the specified time
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_4282, TestSize.Level1)
{
    InitResource(g_mpegPath, LOCAL);
    ASSERT_TRUE(initStatus_);
    ASSERT_TRUE(SetInitValue());
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0,  12000}; // ms
    vector<int32_t> videoVals = {4389, 4389, 4389, 3789, 3791, 3789, 3789, 3428, 3430, 3430, 3669, 3670, 3670};
    vector<int32_t> audioVals = {7629, 7629, 7629, 6614, 6619, 6619, 6614, 5989, 5990, 5990, 6406, 6407, 6407};
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
 * @tc.name: Demuxer_SeekToTime_4283
 * @tc.desc: seek to the specified time
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_4283, TestSize.Level1)
{
    InitResource(g_mpegURL, URI);
    ASSERT_TRUE(initStatus_);
    ASSERT_TRUE(SetInitValue());
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 12000}; // ms
    vector<int32_t> videoVals = {4389, 4389, 4389, 3789, 3791, 3789, 3789, 3428, 3430, 3430, 3669, 3670, 3670};
    vector<int32_t> audioVals = {7629, 7629, 7629, 6614, 6619, 6619, 6614, 5989, 5990, 5990, 6406, 6407, 6407};
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