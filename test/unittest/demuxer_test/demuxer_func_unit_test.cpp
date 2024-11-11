/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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
string g_tsMpeg4Path = TEST_FILE_PATH + string("test_mpeg4_Gop25_4sec.ts");
string g_tsMpeg4Uri = TEST_URI_PATH + string("test_mpeg4_Gop25_4sec.ts");
} // namespace

/**********************************demuxer fd**************************************/
namespace {
/**
 * @tc.name: Demuxer_ReadSample_1075
 * @tc.desc: copy current sample to buffer
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_1075, TestSize.Level1)
{
    InitResource(g_tsMpeg4Path, LOCAL);
    ASSERT_TRUE(initStatus_);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    SetInitValue();
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            ASSERT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    printf("frames_[1]=%d | kFrames[1]=%d\n", frames_[1], keyFrames_[1]);
    ASSERT_EQ(frames_[0], 103);
    ASSERT_EQ(frames_[1], 155);
    ASSERT_EQ(keyFrames_[0], 9);
    ASSERT_EQ(keyFrames_[1], 155);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_2075
 * @tc.desc: copy current sample to buffer
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_2075, TestSize.Level1)
{
    InitResource(g_tsMpeg4Uri, URI);
    ASSERT_TRUE(initStatus_);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    SetInitValue();
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            ASSERT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    printf("frames_[1]=%d | kFrames[1]=%d\n", frames_[1], keyFrames_[1]);
    ASSERT_EQ(frames_[0], 103);
    ASSERT_EQ(frames_[1], 155);
    ASSERT_EQ(keyFrames_[0], 9);
    ASSERT_EQ(keyFrames_[1], 155);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_SeekToTime_1065
 * @tc.desc: seek to the specified time
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_1065, TestSize.Level1)
{
    InitResource(g_tsMpeg4Path, LOCAL);
    ASSERT_TRUE(initStatus_);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    list<int64_t> toPtsList = {0, 3480, 3640, 3350, 3000, 3100, 4120}; // ms
    vector<int32_t> videoVals = {103, 103, 103, 15, 15, 15, 11, 11, 11, 19, 20, 19, 27, 27, 27, 24, 26, 24, 1, 1, 1};
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
            ASSERT_EQ(frames_[0], videoVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: Demuxer_SeekToTime_2065
 * @tc.desc: seek to the specified time
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_2065, TestSize.Level1)
{
    InitResource(g_tsMpeg4Uri, URI);
    ASSERT_TRUE(initStatus_);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    list<int64_t> toPtsList = {0, 3480, 3640, 3350, 3000, 3100, 4120}; // ms
    vector<int32_t> videoVals = {103, 103, 103, 15, 15, 15, 11, 11, 11, 19, 20, 19, 27, 27, 27, 24, 26, 24, 1, 1, 1};
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
            ASSERT_EQ(frames_[0], videoVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}
}