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
const std::string HEVC_LIB_PATH = std::string(AV_CODEC_PATH) + "/libav_codec_hevc_parser.z.so";
list<SeekMode> seekModes = {SeekMode::SEEK_NEXT_SYNC, SeekMode::SEEK_PREVIOUS_SYNC,
    SeekMode::SEEK_CLOSEST_SYNC};
string g_tsMpeg4Path = TEST_FILE_PATH + string("test_mpeg4_Gop25_4sec.ts");
string g_tsMpeg4Uri = TEST_URI_PATH + string("test_mpeg4_Gop25_4sec.ts");
string g_h264aacPath = TEST_FILE_PATH + string("h264_aac.mov");
string g_h264mp3Path = TEST_FILE_PATH + string("h264_mp3.mov");
string g_h264vorPath = TEST_FILE_PATH + string("h264_vorbis.mov");
string g_mpg4mp2Path = TEST_FILE_PATH + string("MPEG4_mp2.mov");
string g_h264aacUri = TEST_URI_PATH + string("h264_aac.mov");
string g_h264mp3Uri = TEST_URI_PATH + string("h264_mp3.mov");
string g_h264vorUri = TEST_URI_PATH + string("h264_vorbis.mov");
string g_mpg4mp2Uri = TEST_URI_PATH + string("MPEG4_mp2.mov");
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
string g_mpg4mp4Path = TEST_FILE_PATH + string("MPEG4.mp4");
string g_mpg4mp4Uri = TEST_URI_PATH + string("MPEG4.mp4");
string g_mp4AvcAacAuxlPath = TEST_FILE_PATH + string("muxer_auxl_265_264_aac.mp4");
string g_mp4AvcAacAuxlUri = TEST_URI_PATH + string("muxer_auxl_265_264_aac.mp4");
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
/**
 * @tc.name: Demuxer_ReadSample_2280
 * @tc.desc: copy current sample to buffer (h264-aac) local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_2280, TestSize.Level1)
{
    InitResource(g_h264aacPath, LOCAL);
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
    ASSERT_EQ(frames_[1], 434);
    ASSERT_EQ(keyFrames_[0], 3);
    ASSERT_EQ(keyFrames_[1], 434);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_2281
 * @tc.desc: copy current sample to buffer (h264-aac) uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_2281, TestSize.Level1)
{
    InitResource(g_h264aacUri, URI);
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
    ASSERT_EQ(frames_[1], 434);
    ASSERT_EQ(keyFrames_[0], 3);
    ASSERT_EQ(keyFrames_[1], 434);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_2282
 * @tc.desc: copy current sample to buffer (h264-mp3) local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_2282, TestSize.Level1)
{
    InitResource(g_h264mp3Path, LOCAL);
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
 * @tc.name: Demuxer_ReadSample_2283
 * @tc.desc: copy current sample to buffer (h264-mp3) uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_2283, TestSize.Level1)
{
    InitResource(g_h264mp3Uri, URI);
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
 * @tc.name: Demuxer_ReadSample_2284
 * @tc.desc: copy current sample to buffer (h264-vorbis) local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_2284, TestSize.Level1)
{
    InitResource(g_h264vorPath, LOCAL);
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
    ASSERT_EQ(frames_[1], 609);
    ASSERT_EQ(keyFrames_[0], 3);
    ASSERT_EQ(keyFrames_[1], 609);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_2285
 * @tc.desc: copy current sample to buffer (h264-vorbis) uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_2285, TestSize.Level1)
{
    InitResource(g_h264vorUri, URI);
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
    ASSERT_EQ(frames_[1], 609);
    ASSERT_EQ(keyFrames_[0], 3);
    ASSERT_EQ(keyFrames_[1], 609);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_2286
 * @tc.desc: copy current sample to buffer (MPEG4-mp2) local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_2286, TestSize.Level1)
{
    InitResource(g_mpg4mp2Path, LOCAL);
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
    ASSERT_EQ(frames_[1], 385);
    ASSERT_EQ(keyFrames_[0], 51);
    ASSERT_EQ(keyFrames_[1], 385);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_2287
 * @tc.desc: copy current sample to buffer (MPEG4-mp2) uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_2287, TestSize.Level1)
{
    InitResource(g_mpg4mp2Uri, URI);
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
    ASSERT_EQ(frames_[1], 385);
    ASSERT_EQ(keyFrames_[0], 51);
    ASSERT_EQ(keyFrames_[1], 385);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_2314
 * @tc.desc: copy current sample to buffer, local (MPEG4 aac)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_2314, TestSize.Level1)
{
    InitResource(g_mpg4mp4Path, LOCAL);
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
    ASSERT_EQ(frames_[1], 434);
    ASSERT_EQ(keyFrames_[0], 51);
    ASSERT_EQ(keyFrames_[1], 434);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_2315
 * @tc.desc: copy current sample to buffer, uri (MPEG4 aac)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_2315, TestSize.Level1)
{
    InitResource(g_mpg4mp4Uri, URI);
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
    ASSERT_EQ(frames_[1], 434);
    ASSERT_EQ(keyFrames_[0], 51);
    ASSERT_EQ(keyFrames_[1], 434);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_SeekToTime_2288
 * @tc.desc: seek to the specified time (h264-aac) local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_2281, TestSize.Level1)
{
    InitResource(g_h264aacPath, LOCAL);
    ASSERT_TRUE(initStatus_);
    SetInitValue();
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 4500, 7000, 2000}; // ms
    vector<int32_t> videoVals = {602, 602, 602, 102, 352, 352, 102, 352, 102, 352, 602, 602};
    vector<int32_t> audioVals = {434, 434, 434, 74, 255, 255, 74, 255, 74, 254, 434, 434};
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
 * @tc.name: Demuxer_SeekToTime_2289
 * @tc.desc: seek to the specified time (h264-aac) uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_2289, TestSize.Level1)
{
    InitResource(g_h264aacUri, URI);
    ASSERT_TRUE(initStatus_);
    SetInitValue();
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 4500, 7000, 2000}; // ms
    vector<int32_t> videoVals = {602, 602, 602, 102, 352, 352, 102, 352, 102, 352, 602, 602};
    vector<int32_t> audioVals = {434, 434, 434, 74, 255, 255, 74, 255, 74, 254, 434, 434};
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
 * @tc.name: Demuxer_SeekToTime_2290
 * @tc.desc: seek to the specified time (h264-mp3) local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_2290, TestSize.Level1)
{
    InitResource(g_h264mp3Path, LOCAL);
    ASSERT_TRUE(initStatus_);
    SetInitValue();
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 4500, 7000, 2000}; // ms
    vector<int32_t> videoVals = {602, 602, 602, 102, 352, 352, 102, 352, 102, 352, 602, 602};
    vector<int32_t> audioVals = {386, 386, 386, 66, 226, 226, 66, 226, 66, 225, 386, 386};
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
 * @tc.name: Demuxer_SeekToTime_2291
 * @tc.desc: seek to the specified time (h264-mp3) uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_2291, TestSize.Level1)
{
    InitResource(g_h264mp3Uri, URI);
    ASSERT_TRUE(initStatus_);
    SetInitValue();
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 4500, 7000, 2000}; // ms
    vector<int32_t> videoVals = {602, 602, 602, 102, 352, 352, 102, 352, 102, 352, 602, 602};
    vector<int32_t> audioVals = {386, 386, 386, 66, 226, 226, 66, 226, 66, 225, 386, 386};
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
 * @tc.name: Demuxer_SeekToTime_2292
 * @tc.desc: seek to the specified time (h264-vorbis) local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_2292, TestSize.Level1)
{
    InitResource(g_h264vorPath, LOCAL);
    ASSERT_TRUE(initStatus_);
    SetInitValue();
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 4500, 7000, 2000}; // ms
    vector<int32_t> videoVals = {602, 602, 602, 102, 352, 352, 102, 352, 102, 352, 602, 602};
    vector<int32_t> audioVals = {609, 609, 609, 106, 364, 364, 106, 364, 106, 363, 609, 609};
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
 * @tc.name: Demuxer_SeekToTime_2293
 * @tc.desc: seek to the specified time (h264-vorbis) uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_2293, TestSize.Level1)
{
    InitResource(g_h264vorUri, URI);
    ASSERT_TRUE(initStatus_);
    SetInitValue();
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 4500, 7000, 2000}; // ms
    vector<int32_t> videoVals = {602, 602, 602, 102, 352, 352, 102, 352, 102, 352, 602, 602};
    vector<int32_t> audioVals = {609, 609, 609, 106, 364, 364, 106, 364, 106, 363, 609, 609};
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
 * @tc.name: Demuxer_SeekToTime_2294
 * @tc.desc: seek to the specified time (mpeg4-mp2) local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_2294, TestSize.Level1)
{
    InitResource(g_mpg4mp2Path, LOCAL);
    ASSERT_TRUE(initStatus_);
    SetInitValue();
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 4500, 7000, 2000}; // ms
    vector<int32_t> videoVals = {602, 602, 602, 326, 338, 338, 182, 182, 182, 482, 482, 482};
    vector<int32_t> audioVals = {385, 385, 385, 208, 217, 217, 116, 117, 116, 308, 309, 308};
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
 * @tc.name: Demuxer_SeekToTime_2295
 * @tc.desc: seek to the specified time (mpeg4-mp2) uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_2295, TestSize.Level1)
{
    InitResource(g_mpg4mp2Uri, URI);
    ASSERT_TRUE(initStatus_);
    SetInitValue();
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 4500, 7000, 2000}; // ms
    vector<int32_t> videoVals = {602, 602, 602, 326, 338, 338, 182, 182, 182, 482, 482, 482};
    vector<int32_t> audioVals = {385, 385, 385, 208, 217, 217, 116, 117, 116, 308, 309, 308};
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
 * @tc.name: Demuxer_SeekToTime_2316
 * @tc.desc: seek to the specified time, local (MPEG4 aac)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_2316, TestSize.Level1)
{
    InitResource(g_mpg4mp4Path, LOCAL);
    ASSERT_TRUE(initStatus_);
    SetInitValue();
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 4500, 7000, 2000}; // ms
    vector<int32_t> videoVals = {602, 602, 602, 326, 338, 338, 182, 182, 182, 482, 482, 482};
    vector<int32_t> audioVals = {433, 433, 433, 233, 243, 243, 130, 131, 130, 345, 346, 345};
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
 * @tc.name: Demuxer_SeekToTime_2317
 * @tc.desc: seek to the specified time uri (MPEG4 aac)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_2317, TestSize.Level1)
{
    InitResource(g_mpg4mp4Uri, URI);
    ASSERT_TRUE(initStatus_);
    SetInitValue();
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 4500, 7000, 2000}; // ms
    vector<int32_t> videoVals = {602, 602, 602, 326, 338, 338, 182, 182, 182, 482, 482, 482};
    vector<int32_t> audioVals = {433, 433, 433, 233, 243, 243, 130, 131, 130, 345, 346, 345};
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

std::map<std::string, std::map<std::string, std::vector<int32_t>>> infoMap = {
    {"muxer264Aac",   {{"frames", {430, 601, 430, 601, 601}}, {"kFrames", {430, 3, 430, 3, 3}}}},
};

/**
 * @tc.name: Demuxer_ReadSample_Auxl_0003
 * @tc.desc: copy current sample to buffer, local(mp4 264 aac auxl local)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_Auxl_0003, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    ReadSample(g_mp4AvcAacAuxlPath, LOCAL);
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(frames_[idx], infoMap["muxer264Aac"]["frames"][idx]);
        ASSERT_EQ(keyFrames_[idx], infoMap["muxer264Aac"]["kFrames"][idx]);
    }
    RemoveValue();
    selectedTrackIds_.clear();
}

/**
 * @tc.name: Demuxer_ReadSample_Auxl_0004
 * @tc.desc: copy current sample to buffer, uri(mp4 264 aac auxl uri)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_Auxl_0004, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    ReadSample(g_mp4AvcAacAuxlUri, URI);
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(frames_[idx], infoMap["muxer264Aac"]["frames"][idx]);
        ASSERT_EQ(keyFrames_[idx], infoMap["muxer264Aac"]["kFrames"][idx]);
    }
    RemoveValue();
    selectedTrackIds_.clear();
}

/**
 * @tc.name: Demuxer_SeekToTime_Auxl_0003
 * @tc.desc: seek to the specified time(mp4 264 aac auxl local)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_Auxl_0003, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    InitResource(g_mp4AvcAacAuxlPath, LOCAL);
    ASSERT_TRUE(initStatus_);
    SetInitValue();
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 5000, 9000, 10000}; // ms
    vector<int32_t> videoVals = {601, 601, 601, 101, 351, 351, 101, 101, 101, 101};
    vector<int32_t> audioVals = {430, 430, 430, 71, 251, 251, 72, 72, 72, 72};
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
            for (int32_t i = 0; i < 5; i++) {
                printf("time = %" PRId64 " | frames_[%d]=%d\n", *toPts, i, frames_[i]);
            }
            ASSERT_EQ(frames_[0], audioVals[numbers_]);
            ASSERT_EQ(frames_[1], videoVals[numbers_]);
            ASSERT_EQ(frames_[2], audioVals[numbers_]);
            ASSERT_EQ(frames_[3], videoVals[numbers_]);
            ASSERT_EQ(frames_[4], videoVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: Demuxer_SeekToTime_Auxl_0004
 * @tc.desc: seek to the specified time(mp4 264 aac auxl uri)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_Auxl_0004, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    InitResource(g_mp4AvcAacAuxlUri, URI);
    ASSERT_TRUE(initStatus_);
    SetInitValue();
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 5000, 9000, 10000}; // ms
    vector<int32_t> videoVals = {601, 601, 601, 101, 351, 351, 0, 101, 101, 0, 101, 101};
    vector<int32_t> audioVals = {601, 601, 601, 101, 351, 351, 0, 101, 101, 0, 101, 101};
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
            for (int32_t i = 0; i < 5; i++) {
                printf("time = %" PRId64 " | frames_[%d]=%d\n", *toPts, i, frames_[i]);
            }
            ASSERT_EQ(frames_[0], audioVals[numbers_]);
            ASSERT_EQ(frames_[1], videoVals[numbers_]);
            ASSERT_EQ(frames_[2], audioVals[numbers_]);
            ASSERT_EQ(frames_[3], videoVals[numbers_]);
            ASSERT_EQ(frames_[4], videoVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}
} // namespace
