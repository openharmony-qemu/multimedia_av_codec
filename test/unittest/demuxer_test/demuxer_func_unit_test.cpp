/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
string g_aviAvcMp3Path = TEST_FILE_PATH + string("h264_mp3.avi");
string g_aviAvcMp3Uri = TEST_URI_PATH + string("h264_mp3.avi");
string g_avi263AacPath = TEST_FILE_PATH + string("test_263_aac_B_Gop25_4sec_cover.avi");
string g_avi263AacUri = TEST_URI_PATH + string("test_263_aac_B_Gop25_4sec_cover.avi");
string g_aviMpeg2Mp2Path = TEST_FILE_PATH + string("test_mpeg2_mp2_B_Gop25_4sec_cover.avi");
string g_aviMpeg2Mp2Uri = TEST_URI_PATH + string("test_mpeg2_mp2_B_Gop25_4sec_cover.avi");
string g_aviMpeg4Mp3Path = TEST_FILE_PATH + string("test_mpeg4_mp3_B_Gop25_4sec_cover.avi");
string g_aviMpeg4Mp3Uri = TEST_URI_PATH + string("test_mpeg4_mp3_B_Gop25_4sec_cover.avi");
string g_aviMpeg4PcmPath = TEST_FILE_PATH + string("mpeg4_pcm.avi");
string g_aviMpeg4PcmUri = TEST_URI_PATH + string("mpeg4_pcm.avi");
} // namespace

/**********************************demuxer fd**************************************/
namespace {
/**
 * @tc.name: Demuxer_ReadSample_2222
 * @tc.desc: copy current sample to buffer, local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_2222, TestSize.Level1)
{
    InitResource(g_aviAvcMp3Path, LOCAL);
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
    ASSERT_EQ(frames_[0], 602);
    ASSERT_EQ(frames_[1], 386);
    ASSERT_EQ(keyFrames_[0], 3);
    ASSERT_EQ(keyFrames_[1], 386);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_2232
 * @tc.desc: copy current sample to buffer, uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_2232, TestSize.Level1)
{
    InitResource(g_aviAvcMp3Uri, URI);
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
    ASSERT_EQ(frames_[0], 602);
    ASSERT_EQ(frames_[1], 386);
    ASSERT_EQ(keyFrames_[0], 3);
    ASSERT_EQ(keyFrames_[1], 386);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_2226
 * @tc.desc: copy current sample to buffer, local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_2226, TestSize.Level1)
{
    InitResource(g_aviMpeg2Mp2Path, LOCAL);
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
 * @tc.name: Demuxer_ReadSample_2236
 * @tc.desc: copy current sample to buffer, uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_2236, TestSize.Level1)
{
    InitResource(g_aviMpeg2Mp2Uri, URI);
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
 * @tc.name: Demuxer_ReadSample_2227
 * @tc.desc: copy current sample to buffer, local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_2227, TestSize.Level1)
{
    InitResource(g_aviMpeg4Mp3Path, LOCAL);
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
    ASSERT_EQ(frames_[1], 156);
    ASSERT_EQ(keyFrames_[0], 9);
    ASSERT_EQ(keyFrames_[1], 156);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_2237
 * @tc.desc: copy current sample to buffer, uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_2237, TestSize.Level1)
{
    InitResource(g_aviMpeg4Mp3Uri, URI);
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
    ASSERT_EQ(frames_[1], 156);
    ASSERT_EQ(keyFrames_[0], 9);
    ASSERT_EQ(keyFrames_[1], 156);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_2229
 * @tc.desc: copy current sample to buffer, local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_2229, TestSize.Level1)
{
    InitResource(g_aviMpeg4PcmPath, LOCAL);
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
    ASSERT_EQ(frames_[0], 604);
    ASSERT_EQ(frames_[1], 433);
    ASSERT_EQ(keyFrames_[0], 51);
    ASSERT_EQ(keyFrames_[1], 433);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_2239
 * @tc.desc: copy current sample to buffer, uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_2239, TestSize.Level1)
{
    InitResource(g_aviMpeg4PcmUri, URI);
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
    ASSERT_EQ(frames_[0], 604);
    ASSERT_EQ(frames_[1], 433);
    ASSERT_EQ(keyFrames_[0], 51);
    ASSERT_EQ(keyFrames_[1], 433);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_2230
 * @tc.desc: copy current sample to buffer, local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_2230, TestSize.Level1)
{
    InitResource(g_avi263AacPath, LOCAL);
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
    ASSERT_EQ(frames_[1], 174);
    ASSERT_EQ(keyFrames_[0], 103);
    ASSERT_EQ(keyFrames_[1], 174);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_2240
 * @tc.desc: copy current sample to buffer, uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_2240, TestSize.Level1)
{
    InitResource(g_avi263AacUri, URI);
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
    ASSERT_EQ(frames_[1], 174);
    ASSERT_EQ(keyFrames_[0], 103);
    ASSERT_EQ(keyFrames_[1], 174);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_SeekToTime_2222
 * @tc.desc: seek to the specified time, local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_2222, TestSize.Level1)
{
    InitResource(g_aviAvcMp3Path, LOCAL);
    ASSERT_TRUE(initStatus_);
    SetInitValue();
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 4500, 7000, 2000}; // ms
    vector<int32_t> videoVals = {602, 602, 602, 102, 352, 352, 102, 352, 102, 352, 602, 602};
    vector<int32_t> audioVals = {386, 386, 386, 66, 225, 225, 66, 225, 66, 225, 386, 386};
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
    ASSERT_NE(demuxer_->SeekToTime(11000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    ASSERT_NE(demuxer_->SeekToTime(-1000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
}

/**
 * @tc.name: Demuxer_SeekToTime_2232
 * @tc.desc: seek to the specified time, uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_2232, TestSize.Level1)
{
    InitResource(g_aviAvcMp3Uri, URI);
    ASSERT_TRUE(initStatus_);
    SetInitValue();
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 4500, 7000, 2000}; // ms
    vector<int32_t> videoVals = {602, 602, 602, 102, 352, 352, 102, 352, 102, 352, 602, 602};
    vector<int32_t> audioVals = {386, 386, 386, 66, 225, 225, 66, 225, 66, 225, 386, 386};
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
    ASSERT_NE(demuxer_->SeekToTime(11000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    ASSERT_NE(demuxer_->SeekToTime(-1000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
}

/**
 * @tc.name: Demuxer_SeekToTime_2226
 * @tc.desc: seek to the specified time, local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_2226, TestSize.Level1)
{
    InitResource(g_aviMpeg2Mp2Path, LOCAL);
    ASSERT_TRUE(initStatus_);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    list<int64_t> toPtsList = {0, 2000, 1920, 2300, 2430, 2600, 3000, 4080}; // ms
    vector<int32_t> audioVals = {155, 155, 155, 62, 80, 80, 80, 98, 80, 62, 80, 62, 62, 62, 62, 43, 62, 62,
        25, 43, 43, 6, 6, 6};
    vector<int32_t> videoVals = {103, 103, 103, 43, 55, 55, 55, 67, 55, 43, 55, 43, 43, 43, 43, 31, 43, 43,
        19, 31, 31, 7, 7, 7};
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
            ASSERT_EQ(frames_[0], videoVals[numbers_]);
            ASSERT_EQ(frames_[1], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: Demuxer_SeekToTime_2236
 * @tc.desc: seek to the specified time, uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_2236, TestSize.Level1)
{
    InitResource(g_aviMpeg2Mp2Uri, URI);
    ASSERT_TRUE(initStatus_);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    list<int64_t> toPtsList = {0, 2000, 1920, 2300, 2430, 2600, 3000, 4080}; // ms
    vector<int32_t> audioVals = {155, 155, 155, 62, 80, 80, 80, 98, 80, 62, 80, 62, 62, 62, 62, 43, 62, 62,
        25, 43, 43, 6, 6, 6};
    vector<int32_t> videoVals = {103, 103, 103, 43, 55, 55, 55, 67, 55, 43, 55, 43, 43, 43, 43, 31, 43, 43,
        19, 31, 31, 7, 7, 7};
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
            ASSERT_EQ(frames_[0], videoVals[numbers_]);
            ASSERT_EQ(frames_[1], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
    ASSERT_NE(demuxer_->SeekToTime(11000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    ASSERT_NE(demuxer_->SeekToTime(-1000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
}

/**
 * @tc.name: Demuxer_SeekToTime_2227
 * @tc.desc: seek to the specified time, local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_2227, TestSize.Level1)
{
    InitResource(g_aviMpeg4Mp3Path, LOCAL);
    ASSERT_TRUE(initStatus_);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    list<int64_t> toPtsList = {0, 2000, 1620, 2300, 2430, 2600, 3000, 4080}; // ms
    vector<int32_t> audioVals = {156, 156, 156, 78, 96, 78, 78, 96, 96, 60, 78, 60, 60, 78, 60, 41, 60, 60,
        41, 41, 41, 4, 4, 4};
    vector<int32_t> videoVals = {103, 103, 103, 55, 67, 55, 55, 67, 67, 43, 55, 43, 43, 55, 43, 31, 43, 43,
        31, 31, 31, 7, 7, 7};
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
            ASSERT_EQ(frames_[0], videoVals[numbers_]);
            ASSERT_EQ(frames_[1], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: Demuxer_SeekToTime_2237
 * @tc.desc: seek to the specified time, uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_2237, TestSize.Level1)
{
    InitResource(g_aviMpeg4Mp3Uri, URI);
    ASSERT_TRUE(initStatus_);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    list<int64_t> toPtsList = {0, 2000, 1620, 2300, 2430, 2600, 3000, 4080}; // ms
    vector<int32_t> audioVals = {156, 156, 156, 78, 96, 78, 78, 96, 96, 60, 78, 60, 60, 78, 60, 41, 60, 60,
        41, 41, 41, 4, 4, 4};
    vector<int32_t> videoVals = {103, 103, 103, 55, 67, 55, 55, 67, 67, 43, 55, 43, 43, 55, 43, 31, 43, 43,
        31, 31, 31, 7, 7, 7};
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
            ASSERT_EQ(frames_[0], videoVals[numbers_]);
            ASSERT_EQ(frames_[1], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
    ASSERT_NE(demuxer_->SeekToTime(11000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    ASSERT_NE(demuxer_->SeekToTime(-1000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
}

/**
 * @tc.name: Demuxer_SeekToTime_2229
 * @tc.desc: seek to the specified time, local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_2229, TestSize.Level1)
{
    InitResource(g_aviMpeg4PcmPath, LOCAL);
    ASSERT_TRUE(initStatus_);
    SetInitValue();
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 4450, 7000, 2000}; // ms
    vector<int32_t> videoVals = {604, 604, 604, 328, 340, 340, 184, 184, 184, 484, 484, 484};
    vector<int32_t> audioVals = {433, 433, 433, 235, 244, 244, 132, 132, 132, 347, 347, 347};
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
    ASSERT_NE(demuxer_->SeekToTime(11000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    ASSERT_NE(demuxer_->SeekToTime(-1000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
}

/**
 * @tc.name: Demuxer_SeekToTime_2239
 * @tc.desc: seek to the specified time, uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_2239, TestSize.Level1)
{
    InitResource(g_aviMpeg4PcmUri, URI);
    ASSERT_TRUE(initStatus_);
    SetInitValue();
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 4450, 7000, 2000}; // ms
    vector<int32_t> videoVals = {604, 604, 604, 328, 340, 340, 184, 184, 184, 484, 484, 484};
    vector<int32_t> audioVals = {433, 433, 433, 235, 244, 244, 132, 132, 132, 347, 347, 347};
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
    ASSERT_NE(demuxer_->SeekToTime(11000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    ASSERT_NE(demuxer_->SeekToTime(-1000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
}

/**
 * @tc.name: Demuxer_SeekToTime_2230
 * @tc.desc: seek to the specified time, local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_2230, TestSize.Level1)
{
    InitResource(g_avi263AacPath, LOCAL);
    ASSERT_TRUE(initStatus_);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    list<int64_t> toPtsList = {0, 2000, 1920, 2300, 2430, 2600, 3000, 4080}; // ms
    vector<int32_t> audioVals = {174, 174, 174, 69, 90, 90, 90, 110, 90, 69, 90, 69, 69, 69, 69, 48, 69, 69,
        28, 48, 48, 7, 7, 7};
    vector<int32_t> videoVals = {103, 103, 103, 43, 55, 55, 55, 67, 55, 43, 55, 43, 43, 43, 43, 31, 43, 43,
        19, 31, 31, 7, 7, 7};
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
            ASSERT_EQ(frames_[0], videoVals[numbers_]);
            ASSERT_EQ(frames_[1], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
    ASSERT_NE(demuxer_->SeekToTime(11000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    ASSERT_NE(demuxer_->SeekToTime(-1000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
}

/**
 * @tc.name: Demuxer_SeekToTime_2240
 * @tc.desc: seek to the specified time, uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_2240, TestSize.Level1)
{
    InitResource(g_avi263AacUri, URI);
    ASSERT_TRUE(initStatus_);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    list<int64_t> toPtsList = {0, 2000, 1920, 2300, 2430, 2600, 3000, 4080}; // ms
    vector<int32_t> audioVals = {174, 174, 174, 69, 90, 90, 90, 110, 90, 69, 90, 69, 69, 69, 69, 48, 69, 69,
        28, 48, 48, 7, 7, 7};
    vector<int32_t> videoVals = {103, 103, 103, 43, 55, 55, 55, 67, 55, 43, 55, 43, 43, 43, 43, 31, 43, 43,
        19, 31, 31, 7, 7, 7};
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
            ASSERT_EQ(frames_[0], videoVals[numbers_]);
            ASSERT_EQ(frames_[1], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
    ASSERT_NE(demuxer_->SeekToTime(11000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    ASSERT_NE(demuxer_->SeekToTime(-1000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
}
} // namespace