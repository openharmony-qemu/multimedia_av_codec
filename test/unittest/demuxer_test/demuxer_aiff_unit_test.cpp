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

string g_aiffFileName = "pcmS16be_30s_44.1k.aiff";
string g_aifcFileName = "adpcmImaQt_20s_44.1k.aifc";

string g_aiffFilePath = TEST_FILE_PATH + g_aiffFileName;
string g_aiffUriPath = TEST_URI_PATH + g_aiffFileName;

string g_aifcFilePath = TEST_FILE_PATH + g_aifcFileName;
string g_aifcUriPath = TEST_URI_PATH + g_aifcFileName;

string g_aiffFilePath1 = TEST_FILE_PATH + string("pcm_s16be.aiff");
string g_aiffFilePath2 = TEST_FILE_PATH + string("pcm_s24be.aiff");
string g_aiffUriPath1 = TEST_URI_PATH + string("pcm_s16be.aiff");
string g_aiffUriPath2 = TEST_URI_PATH + string("pcm_s24be.aiff");

string g_aifcFilePath1 = TEST_FILE_PATH + string("pcm_mulaw.aifc");
string g_aifcFilePath2 = TEST_FILE_PATH + string("pcm_alaw.aifc");
string g_aifcFilePath3 = TEST_FILE_PATH + string("aifc_pcm_s16le.aifc");
string g_aifcFilePath4 = TEST_FILE_PATH + string("aifc_pcm_f64be.aifc");
string g_aifcFilePath5 = TEST_FILE_PATH + string("aifc_pcm_f32be.aifc");
string g_aifcFilePath6 = TEST_FILE_PATH + string("aifc_adpcm_ima_ws.aifc");
string g_aifcFilePath7 = TEST_FILE_PATH + string("aifc_adpcm_ima_qt.aifc");
string g_aifcUriPath1 = TEST_URI_PATH + string("pcm_mulaw.aifc");
string g_aifcUriPath2 = TEST_URI_PATH + string("pcm_alaw.aifc");
string g_aifcUriPath3 = TEST_URI_PATH + string("aifc_pcm_s16le.aifc");
string g_aifcUriPath4 = TEST_URI_PATH + string("aifc_pcm_f64be.aifc");
string g_aifcUriPath5 = TEST_URI_PATH + string("aifc_pcm_f32be.aifc");
string g_aifcUriPath6 = TEST_URI_PATH + string("aifc_adpcm_ima_ws.aifc");
string g_aifcUriPath7 = TEST_URI_PATH + string("aifc_adpcm_ima_qt.aifc");
} // namespace

namespace {
/**
 * @tc.name: Demuxer_AIFF_ReadSample_0001
 * @tc.desc: Read sample test for timed metadata track,local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_AIFF_ReadSample_0001, TestSize.Level1)
{
    InitResource(g_aiffFilePath, LOCAL);
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
    ASSERT_EQ(frames_[0], 1292);
    ASSERT_EQ(keyFrames_[0], 1292);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_AIFF_ReadSample_0002
 * @tc.desc: Read sample test for timed metadata track,URI
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_AIFF_ReadSample_0002, TestSize.Level1)
{
    InitResource(g_aiffUriPath, URI);
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
    ASSERT_EQ(frames_[0], 1292);
    ASSERT_EQ(keyFrames_[0], 1292);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_AIFF_SeekToTime_0003
 * @tc.desc: seek to the specified time,Local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_AIFF_SeekToTime_0003, TestSize.Level1)
{
    InitResource(g_aiffFilePath, LOCAL);
    ASSERT_TRUE(initStatus_);
    ASSERT_TRUE(SetInitValue());
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 10000, 7000, 15000}; // ms
    vector<int32_t> audioVals = {1292, 1292, 1292, 862, 862, 862, 991, 991, 991, 646, 646, 646};
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
 * @tc.name: Demuxer_AIFF_SeekToTime_0004
 * @tc.desc: seek to the specified time,URI
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_AIFF_SeekToTime_0004, TestSize.Level1)
{
    InitResource(g_aiffUriPath, URI);
    ASSERT_TRUE(initStatus_);
    ASSERT_TRUE(SetInitValue());
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 10000, 7000, 15000}; // ms
    vector<int32_t> audioVals = {1292, 1292, 1292, 862, 862, 862, 991, 991, 991, 646, 646, 646};
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
 * @tc.name: Demuxer_AIFC_ReadSample_0001
 * @tc.desc: Read sample test for timed metadata track,local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_AIFC_ReadSample_0001, TestSize.Level1)
{
    InitResource(g_aifcFilePath, LOCAL);
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
    ASSERT_EQ(frames_[0], 13782);
    ASSERT_EQ(keyFrames_[0], 13782);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_AIFC_ReadSample_0002
 * @tc.desc: Read sample test for timed metadata track,URI
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_AIFC_ReadSample_0002, TestSize.Level1)
{
    InitResource(g_aifcUriPath, URI);
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
    ASSERT_EQ(frames_[0], 13782);
    ASSERT_EQ(keyFrames_[0], 13782);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_AIFC_SeekToTime_0003
 * @tc.desc: seek to the specified time,Local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_AIFC_SeekToTime_0003, TestSize.Level1)
{
    InitResource(g_aifcFilePath, LOCAL);
    ASSERT_TRUE(initStatus_);
    ASSERT_TRUE(SetInitValue());
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 10000, 7000, 15000}; // ms
    vector<int32_t> audioVals = {13782, 13782, 13782, 6891, 6892, 6892, 8958, 8959, 8959, 3446, 3447, 3447};
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
 * @tc.name: Demuxer_AIFC_SeekToTime_0004
 * @tc.desc: seek to the specified time,URI
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_AIFC_SeekToTime_0004, TestSize.Level1)
{
    InitResource(g_aifcUriPath, URI);
    ASSERT_TRUE(initStatus_);
    ASSERT_TRUE(SetInitValue());
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 10000, 7000, 15000}; // ms
    vector<int32_t> audioVals = {13782, 13782, 13782, 6891, 6892, 6892, 8958, 8959, 8959, 3446, 3447, 3447};
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
 * @tc.name: Demuxer_AIFF_ReadSample_And_SeekToTime_0001
 * @tc.desc: Read sample test for timed metadata track and seek to the specified time,local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_AIFF_ReadSample_And_SeekToTime_0001, TestSize.Level1)
{
    InitResource(g_aiffFilePath1, LOCAL);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    ASSERT_TRUE(SetInitValue());
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            EXPECT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    printf("frames_[0]=%d | kFrames_[0]=%d\n", frames_[0], keyFrames_[0]);
    EXPECT_EQ(frames_[0], 118);
    EXPECT_EQ(keyFrames_[0], 118);

    for (auto idx : selectedTrackIds_) {
        EXPECT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 1000, 700, 1500}; // ms
    vector<int32_t> audioVals = {118, 118, 118, 94, 94, 94, 101, 101, 101, 83, 83, 83};
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            ret_ = demuxer_->SeekToTime(*toPts, *mode);
            if (ret_ != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret_);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames_[0]=%d\n", *toPts, frames_[0]);
            EXPECT_EQ(frames_[0], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: Demuxer_AIFF_ReadSample_And_SeekToTime_0002
 * @tc.desc: Read sample test for timed metadata track and seek to the specified time,uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_AIFF_ReadSample_And_SeekToTime_0002, TestSize.Level1)
{
    InitResource(g_aiffUriPath1, URI);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    ASSERT_TRUE(SetInitValue());
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            EXPECT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    printf("frames_[0]=%d | kFrames_[0]=%d\n", frames_[0], keyFrames_[0]);
    EXPECT_EQ(frames_[0], 118);
    EXPECT_EQ(keyFrames_[0], 118);

    for (auto idx : selectedTrackIds_) {
        EXPECT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 1000, 700, 1500}; // ms
    vector<int32_t> audioVals = {118, 118, 118, 94, 94, 94, 101, 101, 101, 83, 83, 83};
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            ret_ = demuxer_->SeekToTime(*toPts, *mode);
            if (ret_ != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret_);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames_[0]=%d\n", *toPts, frames_[0]);
            EXPECT_EQ(frames_[0], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: Demuxer_AIFF_ReadSample_And_SeekToTime_0003
 * @tc.desc: Read sample test for timed metadata track and seek to the specified time,local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_AIFF_ReadSample_And_SeekToTime_0003, TestSize.Level1)
{
    InitResource(g_aiffFilePath2, LOCAL);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    ASSERT_TRUE(SetInitValue());
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            EXPECT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    printf("frames_[0]=%d | kFrames_[0]=%d\n", frames_[0], keyFrames_[0]);
    EXPECT_EQ(frames_[0], 176);
    EXPECT_EQ(keyFrames_[0], 176);

    for (auto idx : selectedTrackIds_) {
        EXPECT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 1000, 700, 1500}; // ms
    vector<int32_t> audioVals = {176, 176, 176, 141, 141, 141, 152, 152, 152, 124, 124, 124};
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            ret_ = demuxer_->SeekToTime(*toPts, *mode);
            if (ret_ != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret_);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames_[0]=%d\n", *toPts, frames_[0]);
            EXPECT_EQ(frames_[0], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: Demuxer_AIFF_ReadSample_And_SeekToTime_0004
 * @tc.desc: Read sample test for timed metadata track and seek to the specified time,uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_AIFF_ReadSample_And_SeekToTime_0004, TestSize.Level1)
{
    InitResource(g_aiffUriPath2, URI);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    ASSERT_TRUE(SetInitValue());
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            EXPECT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    printf("frames_[0]=%d | kFrames_[0]=%d\n", frames_[0], keyFrames_[0]);
    EXPECT_EQ(frames_[0], 176);
    EXPECT_EQ(keyFrames_[0], 176);

    for (auto idx : selectedTrackIds_) {
        EXPECT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 1000, 700, 1500}; // ms
    vector<int32_t> audioVals = {176, 176, 176, 141, 141, 141, 152, 152, 152, 124, 124, 124};
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            ret_ = demuxer_->SeekToTime(*toPts, *mode);
            if (ret_ != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret_);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames_[0]=%d\n", *toPts, frames_[0]);
            EXPECT_EQ(frames_[0], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: Demuxer_AIFC_ReadSample_And_SeekToTime_0001
 * @tc.desc: Read sample test for timed metadata track and seek to the specified time,local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_AIFC_ReadSample_And_SeekToTime_0001, TestSize.Level1)
{
    InitResource(g_aifcFilePath1, LOCAL);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    ASSERT_TRUE(SetInitValue());
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            EXPECT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    printf("frames_[0]=%d | kFrames_[0]=%d\n", frames_[0], keyFrames_[0]);
    EXPECT_EQ(frames_[0], 59);
    EXPECT_EQ(keyFrames_[0], 59);

    for (auto idx : selectedTrackIds_) {
        EXPECT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 1000, 700, 1500}; // ms
    vector<int32_t> audioVals = {59, 59, 59, 47, 47, 47, 51, 51, 51, 42, 42, 42};
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            ret_ = demuxer_->SeekToTime(*toPts, *mode);
            if (ret_ != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret_);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames_[0]=%d\n", *toPts, frames_[0]);
            EXPECT_EQ(frames_[0], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: Demuxer_AIFC_ReadSample_And_SeekToTime_0002
 * @tc.desc: Read sample test for timed metadata track and seek to the specified time,uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_AIFC_ReadSample_And_SeekToTime_0002, TestSize.Level1)
{
    InitResource(g_aifcUriPath1, URI);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    ASSERT_TRUE(SetInitValue());
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            EXPECT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    printf("frames_[0]=%d | kFrames_[0]=%d\n", frames_[0], keyFrames_[0]);
    EXPECT_EQ(frames_[0], 59);
    EXPECT_EQ(keyFrames_[0], 59);

    for (auto idx : selectedTrackIds_) {
        EXPECT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 1000, 700, 1500}; // ms
    vector<int32_t> audioVals = {59, 59, 59, 47, 47, 47, 51, 51, 51, 42, 42, 42};
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            ret_ = demuxer_->SeekToTime(*toPts, *mode);
            if (ret_ != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret_);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames_[0]=%d\n", *toPts, frames_[0]);
            EXPECT_EQ(frames_[0], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: Demuxer_AIFC_ReadSample_And_SeekToTime_0003
 * @tc.desc: Read sample test for timed metadata track and seek to the specified time,local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_AIFC_ReadSample_And_SeekToTime_0003, TestSize.Level1)
{
    InitResource(g_aifcFilePath2, LOCAL);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    ASSERT_TRUE(SetInitValue());
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            EXPECT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    printf("frames_[0]=%d | kFrames_[0]=%d\n", frames_[0], keyFrames_[0]);
    EXPECT_EQ(frames_[0], 59);
    EXPECT_EQ(keyFrames_[0], 59);

    for (auto idx : selectedTrackIds_) {
        EXPECT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 1000, 700, 1500}; // ms
    vector<int32_t> audioVals = {59, 59, 59, 47, 47, 47, 51, 51, 51, 42, 42, 42};
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            ret_ = demuxer_->SeekToTime(*toPts, *mode);
            if (ret_ != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret_);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames_[0]=%d\n", *toPts, frames_[0]);
            EXPECT_EQ(frames_[0], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: Demuxer_AIFC_ReadSample_And_SeekToTime_0004
 * @tc.desc: Read sample test for timed metadata track and seek to the specified time,uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_AIFC_ReadSample_And_SeekToTime_0004, TestSize.Level1)
{
    InitResource(g_aifcUriPath2, URI);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    ASSERT_TRUE(SetInitValue());
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            EXPECT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    printf("frames_[0]=%d | kFrames_[0]=%d\n", frames_[0], keyFrames_[0]);
    EXPECT_EQ(frames_[0], 59);
    EXPECT_EQ(keyFrames_[0], 59);

    for (auto idx : selectedTrackIds_) {
        EXPECT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 1000, 700, 1500}; // ms
    vector<int32_t> audioVals = {59, 59, 59, 47, 47, 47, 51, 51, 51, 42, 42, 42};
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            ret_ = demuxer_->SeekToTime(*toPts, *mode);
            if (ret_ != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret_);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames_[0]=%d\n", *toPts, frames_[0]);
            EXPECT_EQ(frames_[0], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: Demuxer_AIFC_ReadSample_And_SeekToTime_0005
 * @tc.desc: Read sample test for timed metadata track and seek to the specified time,local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_AIFC_ReadSample_And_SeekToTime_0005, TestSize.Level1)
{
    InitResource(g_aifcFilePath3, LOCAL);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    ASSERT_TRUE(SetInitValue());
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            EXPECT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    printf("frames_[0]=%d | kFrames_[0]=%d\n", frames_[0], keyFrames_[0]);
    EXPECT_EQ(frames_[0], 118);
    EXPECT_EQ(keyFrames_[0], 118);

    for (auto idx : selectedTrackIds_) {
        EXPECT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 1000, 700, 1500}; // ms
    vector<int32_t> audioVals = {118, 118, 118, 94, 94, 94, 101, 101, 101, 83, 83, 83};
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            ret_ = demuxer_->SeekToTime(*toPts, *mode);
            if (ret_ != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret_);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames_[0]=%d\n", *toPts, frames_[0]);
            EXPECT_EQ(frames_[0], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: Demuxer_AIFC_ReadSample_And_SeekToTime_0006
 * @tc.desc: Read sample test for timed metadata track and seek to the specified time,uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_AIFC_ReadSample_And_SeekToTime_0006, TestSize.Level1)
{
    InitResource(g_aifcUriPath3, URI);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    ASSERT_TRUE(SetInitValue());
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            EXPECT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    printf("frames_[0]=%d | kFrames_[0]=%d\n", frames_[0], keyFrames_[0]);
    EXPECT_EQ(frames_[0], 118);
    EXPECT_EQ(keyFrames_[0], 118);

    for (auto idx : selectedTrackIds_) {
        EXPECT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 1000, 700, 1500}; // ms
    vector<int32_t> audioVals = {118, 118, 118, 94, 94, 94, 101, 101, 101, 83, 83, 83};
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            ret_ = demuxer_->SeekToTime(*toPts, *mode);
            if (ret_ != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret_);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames_[0]=%d\n", *toPts, frames_[0]);
            EXPECT_EQ(frames_[0], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: Demuxer_AIFC_ReadSample_And_SeekToTime_0007
 * @tc.desc: Read sample test for timed metadata track and seek to the specified time,local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_AIFC_ReadSample_And_SeekToTime_0007, TestSize.Level1)
{
    InitResource(g_aifcFilePath4, LOCAL);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    ASSERT_TRUE(SetInitValue());
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            EXPECT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    printf("frames_[0]=%d | kFrames_[0]=%d\n", frames_[0], keyFrames_[0]);
    EXPECT_EQ(frames_[0], 469);
    EXPECT_EQ(keyFrames_[0], 469);

    for (auto idx : selectedTrackIds_) {
        EXPECT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 1000, 700, 1500}; // ms
    vector<int32_t> audioVals = {469, 469, 469, 375, 375, 375, 404, 404, 404, 329, 329, 329};
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            ret_ = demuxer_->SeekToTime(*toPts, *mode);
            if (ret_ != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret_);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames_[0]=%d\n", *toPts, frames_[0]);
            EXPECT_EQ(frames_[0], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: Demuxer_AIFC_ReadSample_And_SeekToTime_0008
 * @tc.desc: Read sample test for timed metadata track and seek to the specified time,uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_AIFC_ReadSample_And_SeekToTime_0008, TestSize.Level1)
{
    InitResource(g_aifcUriPath4, URI);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    ASSERT_TRUE(SetInitValue());
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            EXPECT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    printf("frames_[0]=%d | kFrames_[0]=%d\n", frames_[0], keyFrames_[0]);
    EXPECT_EQ(frames_[0], 469);
    EXPECT_EQ(keyFrames_[0], 469);

    for (auto idx : selectedTrackIds_) {
        EXPECT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 1000, 700, 1500}; // ms
    vector<int32_t> audioVals = {469, 469, 469, 375, 375, 375, 404, 404, 404, 329, 329, 329};
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            ret_ = demuxer_->SeekToTime(*toPts, *mode);
            if (ret_ != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret_);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames_[0]=%d\n", *toPts, frames_[0]);
            EXPECT_EQ(frames_[0], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: Demuxer_AIFC_ReadSample_And_SeekToTime_0009
 * @tc.desc: Read sample test for timed metadata track and seek to the specified time,local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_AIFC_ReadSample_And_SeekToTime_0009, TestSize.Level1)
{
    InitResource(g_aifcFilePath5, LOCAL);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    ASSERT_TRUE(SetInitValue());
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            EXPECT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    printf("frames_[0]=%d | kFrames_[0]=%d\n", frames_[0], keyFrames_[0]);
    EXPECT_EQ(frames_[0], 235);
    EXPECT_EQ(keyFrames_[0], 235);

    for (auto idx : selectedTrackIds_) {
        EXPECT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 1000, 700, 1500}; // ms
    vector<int32_t> audioVals = {235, 235, 235, 188, 188, 188, 202, 202, 202, 165, 165, 165};
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            ret_ = demuxer_->SeekToTime(*toPts, *mode);
            if (ret_ != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret_);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames_[0]=%d\n", *toPts, frames_[0]);
            EXPECT_EQ(frames_[0], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: Demuxer_AIFC_ReadSample_And_SeekToTime_0010
 * @tc.desc: Read sample test for timed metadata track and seek to the specified time,uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_AIFC_ReadSample_And_SeekToTime_0010, TestSize.Level1)
{
    InitResource(g_aifcUriPath5, URI);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    ASSERT_TRUE(SetInitValue());
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            EXPECT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    printf("frames_[0]=%d | kFrames_[0]=%d\n", frames_[0], keyFrames_[0]);
    EXPECT_EQ(frames_[0], 235);
    EXPECT_EQ(keyFrames_[0], 235);

    for (auto idx : selectedTrackIds_) {
        EXPECT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 1000, 700, 1500}; // ms
    vector<int32_t> audioVals = {235, 235, 235, 188, 188, 188, 202, 202, 202, 165, 165, 165};
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            ret_ = demuxer_->SeekToTime(*toPts, *mode);
            if (ret_ != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret_);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames_[0]=%d\n", *toPts, frames_[0]);
            EXPECT_EQ(frames_[0], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: Demuxer_AIFC_ReadSample_And_SeekToTime_0011
 * @tc.desc: Read sample test for timed metadata track and seek to the specified time,local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_AIFC_ReadSample_And_SeekToTime_0011, TestSize.Level1)
{
    InitResource(g_aifcFilePath6, LOCAL);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    ASSERT_TRUE(SetInitValue());
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            EXPECT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    printf("frames_[0]=%d | kFrames_[0]=%d\n", frames_[0], keyFrames_[0]);
    EXPECT_EQ(frames_[0], 30);
    EXPECT_EQ(keyFrames_[0], 30);

    for (auto idx : selectedTrackIds_) {
        EXPECT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 10, 7, 15}; // ms
    vector<int32_t> audioVals = {30, 30, 30, 94, 94, 94, 101, 101, 101, 83, 83, 83};
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            ret_ = demuxer_->SeekToTime(*toPts, *mode);
            if (ret_ != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret_);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames_[0]=%d\n", *toPts, frames_[0]);
            EXPECT_EQ(frames_[0], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: Demuxer_AIFC_ReadSample_And_SeekToTime_0012
 * @tc.desc: Read sample test for timed metadata track and seek to the specified time,uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_AIFC_ReadSample_And_SeekToTime_0012, TestSize.Level1)
{
    InitResource(g_aifcUriPath6, URI);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    ASSERT_TRUE(SetInitValue());
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            EXPECT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    printf("frames_[0]=%d | kFrames_[0]=%d\n", frames_[0], keyFrames_[0]);
    EXPECT_EQ(frames_[0], 30);
    EXPECT_EQ(keyFrames_[0], 30);

    for (auto idx : selectedTrackIds_) {
        EXPECT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 10, 7, 15}; // ms
    vector<int32_t> audioVals = {30, 30, 30, 94, 94, 94, 101, 101, 101, 83, 83, 83};
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            ret_ = demuxer_->SeekToTime(*toPts, *mode);
            if (ret_ != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret_);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames_[0]=%d\n", *toPts, frames_[0]);
            EXPECT_EQ(frames_[0], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: Demuxer_AIFC_ReadSample_And_SeekToTime_0013
 * @tc.desc: Read sample test for timed metadata track and seek to the specified time,local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_AIFC_ReadSample_And_SeekToTime_0013, TestSize.Level1)
{
    InitResource(g_aifcFilePath7, LOCAL);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    ASSERT_TRUE(SetInitValue());
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            EXPECT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    printf("frames_[0]=%d | kFrames_[0]=%d\n", frames_[0], keyFrames_[0]);
    EXPECT_EQ(frames_[0], 3750);
    EXPECT_EQ(keyFrames_[0], 3750);

    for (auto idx : selectedTrackIds_) {
        EXPECT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 1000, 700, 1500}; // ms
    vector<int32_t> audioVals = {3750, 3750, 3750, 3000, 3000, 3000, 3225, 3225, 3225, 2625, 2625, 2625};
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            ret_ = demuxer_->SeekToTime(*toPts, *mode);
            if (ret_ != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret_);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames_[0]=%d\n", *toPts, frames_[0]);
            EXPECT_EQ(frames_[0], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: Demuxer_AIFC_ReadSample_And_SeekToTime_0014
 * @tc.desc: Read sample test for timed metadata track and seek to the specified time,uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_AIFC_ReadSample_And_SeekToTime_0014, TestSize.Level1)
{
    InitResource(g_aifcUriPath7, URI);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    ASSERT_TRUE(SetInitValue());
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            EXPECT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    printf("frames_[0]=%d | kFrames_[0]=%d\n", frames_[0], keyFrames_[0]);
    EXPECT_EQ(frames_[0], 3750);
    EXPECT_EQ(keyFrames_[0], 3750);

    for (auto idx : selectedTrackIds_) {
        EXPECT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 1000, 700, 1500}; // ms
    vector<int32_t> audioVals = {3750, 3750, 3750, 3000, 3000, 3000, 3225, 3225, 3225, 2625, 2625, 2625};
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            ret_ = demuxer_->SeekToTime(*toPts, *mode);
            if (ret_ != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret_);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames_[0]=%d\n", *toPts, frames_[0]);
            EXPECT_EQ(frames_[0], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}
}