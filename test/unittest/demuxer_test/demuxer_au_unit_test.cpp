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

using namespace OHOS;
using namespace OHOS::Media;
using namespace OHOS::MediaAVCodec;
using namespace testing::ext;
using namespace std;
/*add au format test*/
namespace {
unique_ptr<FileServerDemo> server = nullptr;
static const string TEST_FILE_PATH = "/data/test/media/";

list<SeekMode> seekModes = {SeekMode::SEEK_NEXT_SYNC, SeekMode::SEEK_PREVIOUS_SYNC,
    SeekMode::SEEK_CLOSEST_SYNC};

string g_pcmAlawSunAuPath = TEST_FILE_PATH + string("au_pcm_alaw.au");
string g_pcmMulawSunAuPath = TEST_FILE_PATH + string("au_pcm_mulaw.au");
string g_pcmS16beSunAuPath = TEST_FILE_PATH + string("au_pcm_s16be.au");
string g_pcmF64beSunAuPath = TEST_FILE_PATH + string("au_pcm_f64be.au");
string g_adpcmG722SunAuPath = TEST_FILE_PATH + string("au_adpcm_g722.au");
string g_adpcmG726leSunAuPath = TEST_FILE_PATH + string("au_adpcm_g726le.au");
string g_pcmF32beSunAuPath = TEST_FILE_PATH + string("au_pcm_f32be.au");
string g_pcmS8SunAuPath = TEST_FILE_PATH + string("au_pcm_s8.au");
string g_pcmS24beSunAuPath = TEST_FILE_PATH + string("au_pcm_s24be.au");
string g_pcmS32beSunAuPath = TEST_FILE_PATH + string("au_pcm_s32be.au");
} // namespace

namespace {
/**
 * @tc.name: Demuxer_ReadSample_0001
 * @tc.desc: copy current sample to buffer(pcm_alaw.au)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_0001, TestSize.Level1)
{
    InitResource(g_pcmAlawSunAuPath, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(format_, nullptr);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_NE(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_NE(demuxer_->SelectTrackByID(-1), AV_ERR_OK);
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
    ASSERT_EQ(frames_[0], 33);
    ASSERT_EQ(keyFrames_[0], 33);
    RemoveValue();
    ASSERT_EQ(demuxer_->UnselectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(1), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(3), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(-1), AV_ERR_OK);
    ASSERT_EQ(demuxer_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: Demuxer_SeekToTime_0002
 * @tc.desc: seek to the specified time(pcm_alaw.au)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_0002, TestSize.Level1)
{
    InitResource(g_pcmAlawSunAuPath, LOCAL);
    ASSERT_TRUE(SetInitValue());
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 1000, 1500, 2000, 2900}; // ms
    vector<int32_t> audioVals = {33, 33, 33, 22, 22, 22, 17, 17, 17, 11, 11, 11, 2, 2, 2};
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
    ASSERT_NE(demuxer_->SeekToTime(12000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    ASSERT_NE(demuxer_->SeekToTime(-1000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
}

/**
 * @tc.name: Demuxer_ReadSample_0003
 * @tc.desc: copy current sample to buffer(pcm_mulaw.au)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_0003, TestSize.Level1)
{
    InitResource(g_pcmMulawSunAuPath, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(format_, nullptr);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_NE(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_NE(demuxer_->SelectTrackByID(-1), AV_ERR_OK);
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
    ASSERT_EQ(frames_[0], 33);
    ASSERT_EQ(keyFrames_[0], 33);
    RemoveValue();
    ASSERT_EQ(demuxer_->UnselectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(1), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(3), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(-1), AV_ERR_OK);
    ASSERT_EQ(demuxer_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: Demuxer_SeekToTime_0004
 * @tc.desc: seek to the specified time(pcm_mulaw.au)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_0004, TestSize.Level1)
{
    InitResource(g_pcmMulawSunAuPath, LOCAL);
    ASSERT_TRUE(SetInitValue());
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 1000, 1500, 2000, 2900}; // ms
    vector<int32_t> audioVals = {33, 33, 33, 22, 22, 22, 17, 17, 17, 11, 11, 11, 2, 2, 2};
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
    ASSERT_NE(demuxer_->SeekToTime(12000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    ASSERT_NE(demuxer_->SeekToTime(-1000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
}

/**
 * @tc.name: Demuxer_ReadSample_0005
 * @tc.desc: copy current sample to buffer(pcm_s16be.au)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_0005, TestSize.Level1)
{
    InitResource(g_pcmS16beSunAuPath, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(format_, nullptr);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_NE(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_NE(demuxer_->SelectTrackByID(-1), AV_ERR_OK);
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
    ASSERT_EQ(frames_[0], 33);
    ASSERT_EQ(keyFrames_[0], 33);
    RemoveValue();
    ASSERT_EQ(demuxer_->UnselectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(1), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(3), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(-1), AV_ERR_OK);
    ASSERT_EQ(demuxer_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: Demuxer_SeekToTime_0006
 * @tc.desc: seek to the specified time(pcm_s16be.au)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_0006, TestSize.Level1)
{
    InitResource(g_pcmS16beSunAuPath, LOCAL);
    ASSERT_TRUE(SetInitValue());
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 1000, 1500, 2000, 2900}; // ms
    vector<int32_t> audioVals = {33, 33, 33, 22, 22, 22, 17, 17, 17, 11, 11, 11, 2, 2, 2};
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
    ASSERT_NE(demuxer_->SeekToTime(12000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    ASSERT_NE(demuxer_->SeekToTime(-1000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
}

/**
 * @tc.name: Demuxer_ReadSample_0007
 * @tc.desc: copy current sample to buffer(au_pcm_f64be.au)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_0007, TestSize.Level1)
{
    InitResource(g_pcmF64beSunAuPath, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(format_, nullptr);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_NE(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_NE(demuxer_->SelectTrackByID(-1), AV_ERR_OK);
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
    ASSERT_EQ(frames_[0], 59);
    ASSERT_EQ(keyFrames_[0], 59);
    RemoveValue();
    ASSERT_EQ(demuxer_->UnselectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(1), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(3), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(-1), AV_ERR_OK);
    ASSERT_EQ(demuxer_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: Demuxer_SeekToTime_0008
 * @tc.desc: seek to the specified time(au_pcm_f64be.au)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_0008, TestSize.Level1)
{
    InitResource(g_pcmF64beSunAuPath, LOCAL);
    ASSERT_TRUE(SetInitValue());
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 1000, 1500, 2000, 4900}; // ms
    vector<int32_t> audioVals = {59, 59, 59, 47, 47, 47, 42, 42, 42, 36, 36, 36, 2, 2, 2};
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
    ASSERT_NE(demuxer_->SeekToTime(12000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    ASSERT_NE(demuxer_->SeekToTime(-1000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
}

/**
 * @tc.name: Demuxer_ReadSample_0009
 * @tc.desc: copy current sample to buffer(au_adpcm_g722.au)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_0009, TestSize.Level1)
{
    InitResource(g_adpcmG722SunAuPath, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(format_, nullptr);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_NE(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_NE(demuxer_->SelectTrackByID(-1), AV_ERR_OK);
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
    ASSERT_EQ(frames_[0], 59);
    ASSERT_EQ(keyFrames_[0], 59);
    RemoveValue();
    ASSERT_EQ(demuxer_->UnselectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(1), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(3), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(-1), AV_ERR_OK);
    ASSERT_EQ(demuxer_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: Demuxer_SeekToTime_0010
 * @tc.desc: seek to the specified time(au_adpcm_g722.au)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_0010, TestSize.Level1)
{
    InitResource(g_adpcmG722SunAuPath, LOCAL);
    ASSERT_TRUE(SetInitValue());
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 1000, 1500, 2000, 4900}; // ms
    vector<int32_t> audioVals = {59, 59, 59, 47, 47, 47, 42, 42, 42, 36, 36, 36, 2, 2, 2};
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
    ASSERT_NE(demuxer_->SeekToTime(12000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    ASSERT_NE(demuxer_->SeekToTime(-1000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
}

/**
 * @tc.name: Demuxer_ReadSample_0011
 * @tc.desc: copy current sample to buffer(au_adpcm_g726le.au)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_0011, TestSize.Level1)
{
    InitResource(g_adpcmG726leSunAuPath, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(format_, nullptr);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_NE(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_NE(demuxer_->SelectTrackByID(-1), AV_ERR_OK);
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
    ASSERT_EQ(frames_[0], 79);
    ASSERT_EQ(keyFrames_[0], 79);
    RemoveValue();
    ASSERT_EQ(demuxer_->UnselectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(1), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(3), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(-1), AV_ERR_OK);
    ASSERT_EQ(demuxer_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: Demuxer_SeekToTime_0012
 * @tc.desc: seek to the specified time(au_adpcm_g726le.au)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_0012, TestSize.Level1)
{
    InitResource(g_adpcmG726leSunAuPath, LOCAL);
    ASSERT_TRUE(SetInitValue());
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 1000, 1500, 2000, 4900}; // ms
    vector<int32_t> audioVals = {79, 79, 79, 63, 63, 63, 55, 55, 55, 47, 47, 47, 2, 2, 2};
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
    ASSERT_NE(demuxer_->SeekToTime(12000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    ASSERT_NE(demuxer_->SeekToTime(-1000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
}

/**
 * @tc.name: Demuxer_ReadSample_0013
 * @tc.desc: copy current sample to buffer(au_pcm_f32be.au)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_0013, TestSize.Level1)
{
    InitResource(g_pcmF32beSunAuPath, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(format_, nullptr);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_NE(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_NE(demuxer_->SelectTrackByID(-1), AV_ERR_OK);
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
    ASSERT_EQ(frames_[0], 59);
    ASSERT_EQ(keyFrames_[0], 59);
    RemoveValue();
    ASSERT_EQ(demuxer_->UnselectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(1), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(3), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(-1), AV_ERR_OK);
    ASSERT_EQ(demuxer_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: Demuxer_SeekToTime_0014
 * @tc.desc: seek to the specified time(au_pcm_f32be.au)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_0014, TestSize.Level1)
{
    InitResource(g_pcmF32beSunAuPath, LOCAL);
    ASSERT_TRUE(SetInitValue());
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 1000, 1500, 2000, 4900}; // ms
    vector<int32_t> audioVals = {59, 59, 59, 47, 47, 47, 42, 42, 42, 36, 36, 36, 2, 2, 2};
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
    ASSERT_NE(demuxer_->SeekToTime(12000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    ASSERT_NE(demuxer_->SeekToTime(-1000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
}

/**
 * @tc.name: Demuxer_ReadSample_0015
 * @tc.desc: copy current sample to buffer(au_pcm_s8.au)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_0015, TestSize.Level1)
{
    InitResource(g_pcmS8SunAuPath, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(format_, nullptr);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_NE(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_NE(demuxer_->SelectTrackByID(-1), AV_ERR_OK);
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
    ASSERT_EQ(frames_[0], 59);
    ASSERT_EQ(keyFrames_[0], 59);
    RemoveValue();
    ASSERT_EQ(demuxer_->UnselectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(1), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(3), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(-1), AV_ERR_OK);
    ASSERT_EQ(demuxer_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: Demuxer_SeekToTime_0016
 * @tc.desc: seek to the specified time(au_pcm_s8.au)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_0016, TestSize.Level1)
{
    InitResource(g_pcmS8SunAuPath, LOCAL);
    ASSERT_TRUE(SetInitValue());
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 1000, 1500, 2000, 4900}; // ms
    vector<int32_t> audioVals = {59, 59, 59, 47, 47, 47, 42, 42, 42, 36, 36, 36, 2, 2, 2};
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
    ASSERT_NE(demuxer_->SeekToTime(12000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    ASSERT_NE(demuxer_->SeekToTime(-1000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
}

/**
 * @tc.name: Demuxer_ReadSample_0017
 * @tc.desc: copy current sample to buffer(au_pcm_s24be.au)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_0017, TestSize.Level1)
{
    InitResource(g_pcmS24beSunAuPath, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(format_, nullptr);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_NE(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_NE(demuxer_->SelectTrackByID(-1), AV_ERR_OK);
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
    ASSERT_EQ(frames_[0], 59);
    ASSERT_EQ(keyFrames_[0], 59);
    RemoveValue();
    ASSERT_EQ(demuxer_->UnselectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(1), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(3), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(-1), AV_ERR_OK);
    ASSERT_EQ(demuxer_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: Demuxer_SeekToTime_0018
 * @tc.desc: seek to the specified time(au_pcm_s24be.au)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_0018, TestSize.Level1)
{
    InitResource(g_pcmS24beSunAuPath, LOCAL);
    ASSERT_TRUE(SetInitValue());
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 1000, 1500, 2000, 4900}; // ms
    vector<int32_t> audioVals = {59, 59, 59, 47, 47, 47, 42, 42, 42, 36, 36, 36, 2, 2, 2};
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
    ASSERT_NE(demuxer_->SeekToTime(12000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    ASSERT_NE(demuxer_->SeekToTime(-1000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
}

/**
 * @tc.name: Demuxer_ReadSample_0019
 * @tc.desc: copy current sample to buffer(au_pcm_s32be.au)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_0019, TestSize.Level1)
{
    InitResource(g_pcmS32beSunAuPath, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(format_, nullptr);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_NE(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_NE(demuxer_->SelectTrackByID(-1), AV_ERR_OK);
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
    ASSERT_EQ(frames_[0], 59);
    ASSERT_EQ(keyFrames_[0], 59);
    RemoveValue();
    ASSERT_EQ(demuxer_->UnselectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(1), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(3), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(-1), AV_ERR_OK);
    ASSERT_EQ(demuxer_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: Demuxer_SeekToTime_0020
 * @tc.desc: seek to the specified time(au_pcm_s32be.au)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_0020, TestSize.Level1)
{
    InitResource(g_pcmS32beSunAuPath, LOCAL);
    ASSERT_TRUE(SetInitValue());
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 1000, 1500, 2000, 4900}; // ms
    vector<int32_t> audioVals = {59, 59, 59, 47, 47, 47, 42, 42, 42, 36, 36, 36, 2, 2, 2};
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
    ASSERT_NE(demuxer_->SeekToTime(12000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    ASSERT_NE(demuxer_->SeekToTime(-1000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
}
}