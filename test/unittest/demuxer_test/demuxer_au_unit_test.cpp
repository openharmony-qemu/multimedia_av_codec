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
/*add au format test*/
namespace {
unique_ptr<FileServerDemo> server = nullptr;
static const string TEST_FILE_PATH = "/data/test/media/";
static const string TEST_URI_PATH = "http://127.0.0.1:46666/";

list<SeekMode> seekModes = {SeekMode::SEEK_NEXT_SYNC, SeekMode::SEEK_PREVIOUS_SYNC,
    SeekMode::SEEK_CLOSEST_SYNC};


string g_auFileName = "au_48000_1_cover.au";

string g_auFilePath = TEST_FILE_PATH + g_auFileName;
string g_auUriPath = TEST_URI_PATH + g_auFileName;
} // namespace

namespace {
/**
 * @tc.name: Demuxer_AU_ReadSample_0001
 * @tc.desc: Read sample test for timed metadata track,local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_AU_ReadSample_0001, TestSize.Level1)
{
    InitResource(g_auFilePath, LOCAL);
    ASSERT_TRUE(initStatus_);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    ASSERT_TRUE(SetInitValue());
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            ASSERT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }

    printf("frames_[0]=%d | kFrames_[0]=%d\n", frames_[0], keyFrames_[0]);
    ASSERT_EQ(frames_[0], 1407);
    ASSERT_EQ(keyFrames_[0], 1407);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_AU_ReadSample_0002
 * @tc.desc: Read sample test for timed metadata track,URI
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_AU_ReadSample_0002, TestSize.Level1)
{
    InitResource(g_auUriPath, URI);
    ASSERT_TRUE(initStatus_);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    ASSERT_TRUE(SetInitValue());
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            ASSERT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }

    printf("frames_[0]=%d | kFrames_[0]=%d\n", frames_[0], keyFrames_[0]);
    ASSERT_EQ(frames_[0], 1407);
    ASSERT_EQ(keyFrames_[0], 1407);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_AU_SeekToTime_0003
 * @tc.desc: seek to the specified time,Local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_AU_SeekToTime_0003, TestSize.Level1)
{
    InitResource(g_auFilePath, LOCAL);
    ASSERT_TRUE(initStatus_);
    ASSERT_TRUE(SetInitValue());
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 10000, 15000}; // ms
    vector<int32_t> audioVals = {1407, 1407, 1407, 938, 938, 938, 704, 704, 704};
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
            ASSERT_EQ(frames_[0], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: Demuxer_AU_SeekToTime_0004
 * @tc.desc: seek to the specified time,URI
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_AU_SeekToTime_0004, TestSize.Level1)
{
    InitResource(g_auUriPath, URI);
    ASSERT_TRUE(initStatus_);
    ASSERT_TRUE(SetInitValue());
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 10000, 15000}; // ms
    vector<int32_t> audioVals = {1407, 1407, 1407, 938, 938, 938, 704, 704, 704};
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
            ASSERT_EQ(frames_[0], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

}