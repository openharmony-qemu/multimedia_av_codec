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
const std::string HEVC_LIB_PATH = std::string(AV_CODEC_PATH) + "/libav_codec_hevc_parser.z.so";
list<SeekMode> seekModes = {SeekMode::SEEK_NEXT_SYNC, SeekMode::SEEK_PREVIOUS_SYNC,
    SeekMode::SEEK_CLOSEST_SYNC};

string g_mtsPath1 = TEST_FILE_PATH + string("mpeg2.mts");
string g_mtsUri1 = TEST_URI_PATH + string("mpeg2.mts");
string g_mtsPath2 = TEST_FILE_PATH + string("mpeg4.mts");
string g_mtsUri2 = TEST_URI_PATH + string("mpeg4.mts");
string g_mtsPath3 = TEST_FILE_PATH + string("h264_mp3.mts");
string g_mtsUri3 = TEST_URI_PATH + string("h264_mp3.mts");
string g_mtsPath4 = TEST_FILE_PATH + string("hevc_aac.mts");
string g_mtsUri4 = TEST_URI_PATH + string("hevc_aac.mts");
string g_m2tsPath1 = TEST_FILE_PATH + string("mpeg2.m2ts");
string g_m2tsUri1 = TEST_URI_PATH + string("mpeg2.m2ts");
string g_m2tsPath2 = TEST_FILE_PATH + string("h264_mp3.m2ts");
string g_m2tsUri2 = TEST_URI_PATH + string("h264_mp3.m2ts");
string g_m2tsPath3 = TEST_FILE_PATH + string("hevc_aac.m2ts");
string g_m2tsUri3 = TEST_URI_PATH + string("hevc_aac.m2ts");
string g_mtsPath5 = TEST_FILE_PATH + string("h264_aac.mts");
string g_trpPath1 = TEST_FILE_PATH + string("mpeg2.trp");
string g_trpUri1 = TEST_URI_PATH + string("mpeg2.trp");
string g_trpPath2 = TEST_FILE_PATH + string("mpeg4.trp");
string g_trpUri2 = TEST_URI_PATH + string("mpeg4.trp");
string g_trpPath3 = TEST_FILE_PATH + string("h264_mp3.trp");
string g_trpUri3 = TEST_URI_PATH + string("h264_mp3.trp");
string g_trpPath4 = TEST_FILE_PATH + string("hevc_aac.trp");
string g_trpUri4 = TEST_URI_PATH + string("hevc_aac.trp");
} // namespace

/**********************************demuxer fd**************************************/

namespace {
/**
 * @tc.name: Demuxer_ReadSample_2400
 * @tc.desc: copy current sample to buffer when file is mts(mpeg2), Uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_2400, TestSize.Level1)
{
    InitResource(g_mtsUri1, URI);
    EXPECT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_NE(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    EXPECT_NE(sharedMem_, nullptr);
    EXPECT_TRUE(SetInitValue());
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            EXPECT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    EXPECT_EQ(frames_[0], 103);
    EXPECT_EQ(keyFrames_[0], 9);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_2401
 * @tc.desc: copy current sample to buffer when file is mts(mpeg2), Fd
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_2401, TestSize.Level1)
{
    InitResource(g_mtsPath1, LOCAL);
    EXPECT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_NE(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    EXPECT_NE(sharedMem_, nullptr);
    EXPECT_TRUE(SetInitValue());
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            EXPECT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    EXPECT_EQ(frames_[0], 103);
    EXPECT_EQ(keyFrames_[0], 9);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_2402
 * @tc.desc: copy current sample to buffer when file is mts(mpeg4), Uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_2402, TestSize.Level1)
{
    InitResource(g_mtsUri2, URI);
    EXPECT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_NE(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    EXPECT_NE(sharedMem_, nullptr);
    EXPECT_TRUE(SetInitValue());
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            EXPECT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    EXPECT_EQ(frames_[0], 103);
    EXPECT_EQ(keyFrames_[0], 9);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_2403
 * @tc.desc: copy current sample to buffer when file is mts(mpeg4), Fd
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_2403, TestSize.Level1)
{
    InitResource(g_mtsPath2, LOCAL);
    EXPECT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_NE(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    EXPECT_NE(sharedMem_, nullptr);
    EXPECT_TRUE(SetInitValue());
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            EXPECT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    EXPECT_EQ(frames_[0], 103);
    EXPECT_EQ(keyFrames_[0], 9);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_2404
 * @tc.desc: copy current sample to buffer when file is mts(h264, mp3), Uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_2404, TestSize.Level1)
{
    InitResource(g_mtsUri3, URI);
    EXPECT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    EXPECT_NE(sharedMem_, nullptr);
    EXPECT_TRUE(SetInitValue());
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            EXPECT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    printf("frames_[1]=%d | kFrames[1]=%d\n", frames_[1], keyFrames_[1]);
    EXPECT_EQ(frames_[0], 602);
    EXPECT_EQ(frames_[1], 420);
    EXPECT_EQ(keyFrames_[0], 3);
    EXPECT_EQ(keyFrames_[1], 420);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_2405
 * @tc.desc: copy current sample to buffer when file is mts(h264, mp3), Fd
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_2405, TestSize.Level1)
{
    InitResource(g_mtsPath3, LOCAL);
    EXPECT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    EXPECT_NE(sharedMem_, nullptr);
    EXPECT_TRUE(SetInitValue());
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            EXPECT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    printf("frames_[1]=%d | kFrames[1]=%d\n", frames_[1], keyFrames_[1]);
    EXPECT_EQ(frames_[0], 602);
    EXPECT_EQ(frames_[1], 420);
    EXPECT_EQ(keyFrames_[0], 3);
    EXPECT_EQ(keyFrames_[1], 420);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_2406
 * @tc.desc: copy current sample to buffer when file is mts(hevc, aac), Uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_2406, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) == 0) {
        InitResource(g_mtsUri4, URI);
        EXPECT_TRUE(initStatus_);
        EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
        EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
        sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
        EXPECT_NE(sharedMem_, nullptr);
        EXPECT_TRUE(SetInitValue());
        while (!isEOS(eosFlag_)) {
            for (auto idx : selectedTrackIds_) {
                EXPECT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
                CountFrames(idx);
            }
        }
        printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
        printf("frames_[1]=%d | kFrames[1]=%d\n", frames_[1], keyFrames_[1]);
        EXPECT_EQ(frames_[0], 303);
        EXPECT_EQ(frames_[1], 434);
        EXPECT_EQ(keyFrames_[0], 2);
        EXPECT_EQ(keyFrames_[1], 434);
        RemoveValue();
    }
}

/**
 * @tc.name: Demuxer_ReadSample_2407
 * @tc.desc: copy current sample to buffer when file is mts(hevc, aac), Fd
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_2407, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) == 0) {
        InitResource(g_mtsPath4, LOCAL);
        EXPECT_TRUE(initStatus_);
        EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
        EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
        sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
        EXPECT_NE(sharedMem_, nullptr);
        EXPECT_TRUE(SetInitValue());
        while (!isEOS(eosFlag_)) {
            for (auto idx : selectedTrackIds_) {
                EXPECT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
                CountFrames(idx);
            }
        }
        printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
        printf("frames_[1]=%d | kFrames[1]=%d\n", frames_[1], keyFrames_[1]);
        EXPECT_EQ(frames_[0], 303);
        EXPECT_EQ(frames_[1], 434);
        EXPECT_EQ(keyFrames_[0], 2);
        EXPECT_EQ(keyFrames_[1], 434);
        RemoveValue();
    }
}

/**
 * @tc.name: Demuxer_ReadSample_2408
 * @tc.desc: copy current sample to buffer when file is m2ts(mpeg2), Uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_2408, TestSize.Level1)
{
    InitResource(g_m2tsUri1, URI);
    EXPECT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_NE(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    EXPECT_NE(sharedMem_, nullptr);
    EXPECT_TRUE(SetInitValue());
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            EXPECT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    EXPECT_EQ(frames_[0], 103);
    EXPECT_EQ(keyFrames_[0], 9);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_2409
 * @tc.desc: copy current sample to buffer when file is m2ts(mpeg2), Fd
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_2409, TestSize.Level1)
{
    InitResource(g_m2tsPath1, LOCAL);
    EXPECT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_NE(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    EXPECT_NE(sharedMem_, nullptr);
    EXPECT_TRUE(SetInitValue());
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            EXPECT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    EXPECT_EQ(frames_[0], 103);
    EXPECT_EQ(keyFrames_[0], 9);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_2410
 * @tc.desc: copy current sample to buffer when file is m2ts(h264, mp3), Uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_2410, TestSize.Level1)
{
    InitResource(g_m2tsUri2, URI);
    EXPECT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    EXPECT_NE(sharedMem_, nullptr);
    EXPECT_TRUE(SetInitValue());
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            EXPECT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    printf("frames_[1]=%d | kFrames[1]=%d\n", frames_[1], keyFrames_[1]);
    EXPECT_EQ(frames_[0], 602);
    EXPECT_EQ(frames_[1], 420);
    EXPECT_EQ(keyFrames_[0], 3);
    EXPECT_EQ(keyFrames_[1], 420);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_2411
 * @tc.desc: copy current sample to buffer when file is m2ts(h264, mp3), Fd
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_2411, TestSize.Level1)
{
    InitResource(g_m2tsPath2, LOCAL);
    EXPECT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    EXPECT_NE(sharedMem_, nullptr);
    EXPECT_TRUE(SetInitValue());
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            EXPECT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    printf("frames_[1]=%d | kFrames[1]=%d\n", frames_[1], keyFrames_[1]);
    EXPECT_EQ(frames_[0], 602);
    EXPECT_EQ(frames_[1], 420);
    EXPECT_EQ(keyFrames_[0], 3);
    EXPECT_EQ(keyFrames_[1], 420);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_2412
 * @tc.desc: copy current sample to buffer when file is m2ts(hevc, aac), Uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_2412, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) == 0) {
        InitResource(g_m2tsUri3, URI);
        EXPECT_TRUE(initStatus_);
        EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
        EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
        sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
        EXPECT_NE(sharedMem_, nullptr);
        EXPECT_TRUE(SetInitValue());
        while (!isEOS(eosFlag_)) {
            for (auto idx : selectedTrackIds_) {
                EXPECT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
                CountFrames(idx);
            }
        }
        printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
        printf("frames_[1]=%d | kFrames[1]=%d\n", frames_[1], keyFrames_[1]);
        EXPECT_EQ(frames_[0], 303);
        EXPECT_EQ(frames_[1], 434);
        EXPECT_EQ(keyFrames_[0], 2);
        EXPECT_EQ(keyFrames_[1], 434);
        RemoveValue();
    }
}

/**
 * @tc.name: Demuxer_ReadSample_2413
 * @tc.desc: copy current sample to buffer when file is m2ts(hevc, aac), Fd
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_2413, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) == 0) {
        InitResource(g_m2tsPath3, LOCAL);
        EXPECT_TRUE(initStatus_);
        EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
        EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
        sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
        EXPECT_NE(sharedMem_, nullptr);
        EXPECT_TRUE(SetInitValue());
        while (!isEOS(eosFlag_)) {
            for (auto idx : selectedTrackIds_) {
                EXPECT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
                CountFrames(idx);
            }
        }
        printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
        printf("frames_[1]=%d | kFrames[1]=%d\n", frames_[1], keyFrames_[1]);
        EXPECT_EQ(frames_[0], 303);
        EXPECT_EQ(frames_[1], 434);
        EXPECT_EQ(keyFrames_[0], 2);
        EXPECT_EQ(keyFrames_[1], 434);
        RemoveValue();
    }
}

/**
 * @tc.name: Demuxer_ReadSample_2414
 * @tc.desc: copy current sample to buffer when file is trp(mpeg2), Uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_2414, TestSize.Level1)
{
    InitResource(g_trpUri1, URI);
    EXPECT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_NE(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    EXPECT_NE(sharedMem_, nullptr);
    EXPECT_TRUE(SetInitValue());
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            EXPECT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    EXPECT_EQ(frames_[0], 103);
    EXPECT_EQ(keyFrames_[0], 9);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_2415
 * @tc.desc: copy current sample to buffer when file is trp(mpeg2), Fd
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_2415, TestSize.Level1)
{
    InitResource(g_trpPath1, LOCAL);
    EXPECT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_NE(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    EXPECT_NE(sharedMem_, nullptr);
    EXPECT_TRUE(SetInitValue());
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            EXPECT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    EXPECT_EQ(frames_[0], 103);
    EXPECT_EQ(keyFrames_[0], 9);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_2416
 * @tc.desc: copy current sample to buffer when file is trp(mpeg4), Uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_2416, TestSize.Level1)
{
    InitResource(g_trpUri2, URI);
    EXPECT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_NE(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    EXPECT_NE(sharedMem_, nullptr);
    EXPECT_TRUE(SetInitValue());
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            EXPECT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    EXPECT_EQ(frames_[0], 103);
    EXPECT_EQ(keyFrames_[0], 9);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_2417
 * @tc.desc: copy current sample to buffer when file is trp(mpeg4), Fd
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_2417, TestSize.Level1)
{
    InitResource(g_trpPath2, LOCAL);
    EXPECT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_NE(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    EXPECT_NE(sharedMem_, nullptr);
    EXPECT_TRUE(SetInitValue());
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            EXPECT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    EXPECT_EQ(frames_[0], 103);
    EXPECT_EQ(keyFrames_[0], 9);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_2418
 * @tc.desc: copy current sample to buffer when file is trp(h264, mp3), Uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_2418, TestSize.Level1)
{
    InitResource(g_trpUri3, URI);
    EXPECT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    EXPECT_NE(sharedMem_, nullptr);
    EXPECT_TRUE(SetInitValue());
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            EXPECT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    printf("frames_[1]=%d | kFrames[1]=%d\n", frames_[1], keyFrames_[1]);
    EXPECT_EQ(frames_[0], 602);
    EXPECT_EQ(frames_[1], 420);
    EXPECT_EQ(keyFrames_[0], 3);
    EXPECT_EQ(keyFrames_[1], 420);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_2419
 * @tc.desc: copy current sample to buffer when file is trp(h264, mp3), Fd
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_2419, TestSize.Level1)
{
    InitResource(g_trpPath3, LOCAL);
    EXPECT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    EXPECT_NE(sharedMem_, nullptr);
    EXPECT_TRUE(SetInitValue());
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            EXPECT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    printf("frames_[1]=%d | kFrames[1]=%d\n", frames_[1], keyFrames_[1]);
    EXPECT_EQ(frames_[0], 602);
    EXPECT_EQ(frames_[1], 420);
    EXPECT_EQ(keyFrames_[0], 3);
    EXPECT_EQ(keyFrames_[1], 420);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_2420
 * @tc.desc: copy current sample to buffer when file is trp(hevc, aac), Uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_2420, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) == 0) {
        InitResource(g_trpUri4, URI);
        EXPECT_TRUE(initStatus_);
        EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
        EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
        sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
        EXPECT_NE(sharedMem_, nullptr);
        EXPECT_TRUE(SetInitValue());
        while (!isEOS(eosFlag_)) {
            for (auto idx : selectedTrackIds_) {
                EXPECT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
                CountFrames(idx);
            }
        }
        printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
        printf("frames_[1]=%d | kFrames[1]=%d\n", frames_[1], keyFrames_[1]);
        EXPECT_EQ(frames_[0], 303);
        EXPECT_EQ(frames_[1], 434);
        EXPECT_EQ(keyFrames_[0], 2);
        EXPECT_EQ(keyFrames_[1], 434);
        RemoveValue();
    }
}

/**
 * @tc.name: Demuxer_ReadSample_2421
 * @tc.desc: copy current sample to buffer when file is trp(hevc, aac), Fd
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_2421, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) == 0) {
        InitResource(g_trpPath4, LOCAL);
        EXPECT_TRUE(initStatus_);
        EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
        EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
        sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
        EXPECT_NE(sharedMem_, nullptr);
        EXPECT_TRUE(SetInitValue());
        while (!isEOS(eosFlag_)) {
            for (auto idx : selectedTrackIds_) {
                EXPECT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
                CountFrames(idx);
            }
        }
        printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
        printf("frames_[1]=%d | kFrames[1]=%d\n", frames_[1], keyFrames_[1]);
        EXPECT_EQ(frames_[0], 303);
        EXPECT_EQ(frames_[1], 434);
        EXPECT_EQ(keyFrames_[0], 2);
        EXPECT_EQ(keyFrames_[1], 434);
        RemoveValue();
    }
}

/**
 * @tc.name: Demuxer_SeekToTime_2400
 * @tc.desc: seek to the specified time when file is mts(mpeg2), Uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_2400, TestSize.Level1)
{
    InitResource(g_mtsUri1, URI);
    EXPECT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    list<int64_t> toPtsList = {0, 3480, 3640, 3320, 3000, 3110, 4120, 5520}; // ms
    vector<int32_t> videoVals = {103, 103, 103, 15, 15, 15, 11, 11, 11, 19, 19, 19, 27, 27, 27, 24, 25, 24, 1, 1, 1};
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    EXPECT_NE(sharedMem_, nullptr);
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            ret_ = demuxer_->SeekToTime(*toPts, *mode);
            if (ret_ != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret_);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames_[0]=%d | kFrames[0]=%d\n", *toPts, frames_[0], keyFrames_[0]);
            EXPECT_EQ(frames_[0], videoVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: Demuxer_SeekToTime_2401
 * @tc.desc: seek to the specified time when file is mts(mpeg2), Fd
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_2401, TestSize.Level1)
{
    InitResource(g_mtsPath1, LOCAL);
    EXPECT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    list<int64_t> toPtsList = {0, 3480, 3640, 3320, 3000, 3110, 4120, 5520}; // ms
    vector<int32_t> videoVals = {103, 103, 103, 15, 15, 15, 11, 11, 11, 19, 19, 19, 27, 27, 27, 24, 25, 24, 1, 1, 1};
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    EXPECT_NE(sharedMem_, nullptr);
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            ret_ = demuxer_->SeekToTime(*toPts, *mode);
            if (ret_ != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret_);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames_[0]=%d | kFrames[0]=%d\n", *toPts, frames_[0], keyFrames_[0]);
            EXPECT_EQ(frames_[0], videoVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: Demuxer_SeekToTime_2402
 * @tc.desc: seek to the specified time when file is mts(mpeg4), Uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_2402, TestSize.Level1)
{
    InitResource(g_mtsUri2, URI);
    EXPECT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    list<int64_t> toPtsList = {0, 3480, 3640, 3350, 3000, 3110, 4120}; // ms
    vector<int32_t> videoVals = {103, 103, 103, 15, 15, 15, 11, 11, 11, 19, 20, 19, 27, 27, 27, 24, 25, 23, 1, 1, 1};
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    EXPECT_NE(sharedMem_, nullptr);
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            ret_ = demuxer_->SeekToTime(*toPts, *mode);
            if (ret_ != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret_);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames_[0]=%d | kFrames[0]=%d\n", *toPts, frames_[0], keyFrames_[0]);
            EXPECT_EQ(frames_[0], videoVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: Demuxer_SeekToTime_2403
 * @tc.desc: seek to the specified time when file is mts(mpeg4), Fd
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_2403, TestSize.Level1)
{
    InitResource(g_mtsPath2, LOCAL);
    EXPECT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    list<int64_t> toPtsList = {0, 3480, 3640, 3350, 3000, 3110, 4120}; // ms
    vector<int32_t> videoVals = {103, 103, 103, 15, 15, 15, 11, 11, 11, 19, 20, 19, 27, 27, 27, 24, 25, 23, 1, 1, 1};
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    EXPECT_NE(sharedMem_, nullptr);
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            ret_ = demuxer_->SeekToTime(*toPts, *mode);
            if (ret_ != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret_);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames_[0]=%d | kFrames[0]=%d\n", *toPts, frames_[0], keyFrames_[0]);
            EXPECT_EQ(frames_[0], videoVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: Demuxer_SeekToTime_2404
 * @tc.desc: seek to the specified time when file is mts(h264, mp3), Uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_2404, TestSize.Level1)
{
    InitResource(g_mtsUri3, URI);
    EXPECT_TRUE(SetInitValue());
    for (auto idx : selectedTrackIds_) {
        EXPECT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 4500, 7000, 2000, 6000}; // ms
    vector<int32_t> videoVals = {602, 602, 602, 330, 330, 330, 180, 180, 180, 480, 480, 480, 240, 240, 240};
    vector<int32_t> audioVals = {420, 420, 420, 238, 238, 238, 133, 133, 133, 336, 336, 336, 175, 175, 175};
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    EXPECT_NE(sharedMem_, nullptr);
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
            EXPECT_EQ(frames_[0], videoVals[numbers_]);
            EXPECT_EQ(frames_[1], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
    EXPECT_NE(demuxer_->SeekToTime(12000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    EXPECT_NE(demuxer_->SeekToTime(-1000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
}

/**
 * @tc.name: Demuxer_SeekToTime_2405
 * @tc.desc: seek to the specified time when file is mts(h264, mp3), Fd
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_2405, TestSize.Level1)
{
    InitResource(g_mtsPath3, LOCAL);
    EXPECT_TRUE(SetInitValue());
    for (auto idx : selectedTrackIds_) {
        EXPECT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 4500, 7000, 2000, 6000}; // ms
    vector<int32_t> videoVals = {602, 602, 602, 330, 330, 330, 180, 180, 180, 480, 480, 480, 240, 240, 240};
    vector<int32_t> audioVals = {420, 420, 420, 238, 238, 238, 133, 133, 133, 336, 336, 336, 175, 175, 175};
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    EXPECT_NE(sharedMem_, nullptr);
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
            EXPECT_EQ(frames_[0], videoVals[numbers_]);
            EXPECT_EQ(frames_[1], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
    EXPECT_NE(demuxer_->SeekToTime(12000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    EXPECT_NE(demuxer_->SeekToTime(-1000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
}

/**
 * @tc.name: Demuxer_SeekToTime_2406
 * @tc.desc: seek to the specified time when file is mts(hevc, aac), Uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_2406, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) == 0) {
        InitResource(g_mtsUri4, URI);
        EXPECT_TRUE(initStatus_);
        EXPECT_TRUE(SetInitValue());
        for (auto idx : selectedTrackIds_) {
            EXPECT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
        }
        list<int64_t> toPtsList = {0, 1500, 10000, 4600}; // ms
        vector<int32_t> videoVals = {303, 303, 303, 256, 256, 256, 1, 1, 1, 163, 163, 163};
        vector<int32_t> audioVals = {434, 434, 434, 370, 370, 370, 2, 2, 2, 236, 236, 236};
        sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
        EXPECT_NE(sharedMem_, nullptr);
        SeekTest(toPtsList, seekModes, {videoVals, audioVals});
        EXPECT_TRUE(seekTestFlag_);
        EXPECT_NE(demuxer_->SeekToTime(11000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
        EXPECT_NE(demuxer_->SeekToTime(-1000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    }
}

/**
 * @tc.name: Demuxer_SeekToTime_2407
 * @tc.desc: seek to the specified time when file is mts(hevc, aac), Fd
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_2407, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) == 0) {
        InitResource(g_mtsPath4, LOCAL);
        EXPECT_TRUE(initStatus_);
        EXPECT_TRUE(SetInitValue());
        for (auto idx : selectedTrackIds_) {
            EXPECT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
        }
        list<int64_t> toPtsList = {0, 1500, 10000, 4600}; // ms
        vector<int32_t> videoVals = {303, 303, 303, 256, 256, 256, 1, 1, 1, 163, 163, 163};
        vector<int32_t> audioVals = {434, 434, 434, 370, 370, 370, 2, 2, 2, 236, 236, 236};
        sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
        EXPECT_NE(sharedMem_, nullptr);
        SeekTest(toPtsList, seekModes, {videoVals, audioVals});
        EXPECT_TRUE(seekTestFlag_);
        EXPECT_NE(demuxer_->SeekToTime(11000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
        EXPECT_NE(demuxer_->SeekToTime(-1000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    }
}

/**
 * @tc.name: Demuxer_SeekToTime_2408
 * @tc.desc: seek to the specified time when file is m2ts(mpeg2), Uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_2408, TestSize.Level1)
{
    InitResource(g_m2tsUri1, URI);
    EXPECT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    list<int64_t> toPtsList = {0, 3480, 3640, 3320, 3000, 3110, 4120, 5520}; // ms
    vector<int32_t> videoVals = {103, 103, 103, 15, 15, 15, 10, 10, 10, 19, 19, 19, 27, 27, 27, 24, 25, 24, 1, 1, 1};
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    EXPECT_NE(sharedMem_, nullptr);
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            ret_ = demuxer_->SeekToTime(*toPts, *mode);
            if (ret_ != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret_);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames_[0]=%d | kFrames[0]=%d\n", *toPts, frames_[0], keyFrames_[0]);
            EXPECT_EQ(frames_[0], videoVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: Demuxer_SeekToTime_2409
 * @tc.desc: seek to the specified time when file is m2ts(mpeg2), Fd
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_2409, TestSize.Level1)
{
    InitResource(g_m2tsPath1, LOCAL);
    EXPECT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    list<int64_t> toPtsList = {0, 3480, 3640, 3320, 3000, 3110, 4120, 5520}; // ms
    vector<int32_t> videoVals = {103, 103, 103, 15, 15, 15, 10, 10, 10, 19, 19, 19, 27, 27, 27, 24, 25, 24, 1, 1, 1};
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    EXPECT_NE(sharedMem_, nullptr);
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            ret_ = demuxer_->SeekToTime(*toPts, *mode);
            if (ret_ != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret_);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames_[0]=%d | kFrames[0]=%d\n", *toPts, frames_[0], keyFrames_[0]);
            EXPECT_EQ(frames_[0], videoVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: Demuxer_SeekToTime_2410
 * @tc.desc: seek to the specified time when file is m2ts(h264, mp3), Uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_2410, TestSize.Level1)
{
    InitResource(g_m2tsUri2, URI);
    EXPECT_TRUE(SetInitValue());
    for (auto idx : selectedTrackIds_) {
        EXPECT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 4500, 7000, 2000, 6000}; // ms
    vector<int32_t> videoVals = {602, 602, 602, 330, 330, 330, 180, 180, 180, 480, 480, 480, 240, 240, 240};
    vector<int32_t> audioVals = {420, 420, 420, 238, 238, 238, 133, 133, 133, 336, 336, 336, 175, 175, 175};
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    EXPECT_NE(sharedMem_, nullptr);
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
            EXPECT_EQ(frames_[0], videoVals[numbers_]);
            EXPECT_EQ(frames_[1], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
    EXPECT_NE(demuxer_->SeekToTime(12000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    EXPECT_NE(demuxer_->SeekToTime(-1000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
}

/**
 * @tc.name: Demuxer_SeekToTime_2411
 * @tc.desc: seek to the specified time when file is m2ts(h264, mp3), Fd
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_2411, TestSize.Level1)
{
    InitResource(g_m2tsPath2, LOCAL);
    EXPECT_TRUE(SetInitValue());
    for (auto idx : selectedTrackIds_) {
        EXPECT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 4500, 7000, 2000, 6000}; // ms
    vector<int32_t> videoVals = {602, 602, 602, 330, 330, 330, 180, 180, 180, 480, 480, 480, 240, 240, 240};
    vector<int32_t> audioVals = {420, 420, 420, 238, 238, 238, 133, 133, 133, 336, 336, 336, 175, 175, 175};
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    EXPECT_NE(sharedMem_, nullptr);
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
            EXPECT_EQ(frames_[0], videoVals[numbers_]);
            EXPECT_EQ(frames_[1], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
    EXPECT_NE(demuxer_->SeekToTime(12000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    EXPECT_NE(demuxer_->SeekToTime(-1000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
}

/**
 * @tc.name: Demuxer_SeekToTime_2412
 * @tc.desc: seek to the specified time when file is m2ts(hevc, aac), Uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_2412, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) == 0) {
        InitResource(g_m2tsUri3, URI);
        EXPECT_TRUE(initStatus_);
        EXPECT_TRUE(SetInitValue());
        for (auto idx : selectedTrackIds_) {
            EXPECT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
        }
        list<int64_t> toPtsList = {0, 1500, 10000, 4600}; // ms
        vector<int32_t> videoVals = {303, 303, 303, 256, 256, 256, 1, 1, 1, 163, 163, 163};
        vector<int32_t> audioVals = {434, 434, 434, 370, 370, 370, 2, 2, 2, 236, 236, 236};
        sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
        EXPECT_NE(sharedMem_, nullptr);
        SeekTest(toPtsList, seekModes, {videoVals, audioVals});
        EXPECT_TRUE(seekTestFlag_);
        EXPECT_NE(demuxer_->SeekToTime(11000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
        EXPECT_NE(demuxer_->SeekToTime(-1000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    }
}

/**
 * @tc.name: Demuxer_SeekToTime_2413
 * @tc.desc: seek to the specified time when file is m2ts(hevc, aac), Fd
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_2413, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) == 0) {
        InitResource(g_m2tsPath3, LOCAL);
        EXPECT_TRUE(initStatus_);
        EXPECT_TRUE(SetInitValue());
        for (auto idx : selectedTrackIds_) {
            EXPECT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
        }
        list<int64_t> toPtsList = {0, 1500, 10000, 4600}; // ms
        vector<int32_t> videoVals = {303, 303, 303, 256, 256, 256, 1, 1, 1, 163, 163, 163};
        vector<int32_t> audioVals = {434, 434, 434, 370, 370, 370, 2, 2, 2, 236, 236, 236};
        sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
        EXPECT_NE(sharedMem_, nullptr);
        SeekTest(toPtsList, seekModes, {videoVals, audioVals});
        EXPECT_TRUE(seekTestFlag_);
        EXPECT_NE(demuxer_->SeekToTime(11000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
        EXPECT_NE(demuxer_->SeekToTime(-1000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    }
}

/**
 * @tc.name: Demuxer_SeekToTime_2414
 * @tc.desc: seek to the specified time when file is trp(mpeg2), Uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_2414, TestSize.Level1)
{
    InitResource(g_trpUri1, URI);
    EXPECT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    list<int64_t> toPtsList = {0, 3480, 3640, 3320, 3000, 3110, 4120, 5520}; // ms
    vector<int32_t> videoVals = {103, 103, 103, 15, 15, 15, 11, 11, 11, 19, 19, 19, 27, 27, 27, 24, 25, 24, 1, 1, 1};
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    EXPECT_NE(sharedMem_, nullptr);
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            ret_ = demuxer_->SeekToTime(*toPts, *mode);
            if (ret_ != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret_);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames_[0]=%d | kFrames[0]=%d\n", *toPts, frames_[0], keyFrames_[0]);
            EXPECT_EQ(frames_[0], videoVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: Demuxer_SeekToTime_2415
 * @tc.desc: seek to the specified time when file is trp(mpeg2), Fd
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_2415, TestSize.Level1)
{
    InitResource(g_trpPath1, LOCAL);
    EXPECT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    list<int64_t> toPtsList = {0, 3480, 3640, 3320, 3000, 3110, 4120, 5520}; // ms
    vector<int32_t> videoVals = {103, 103, 103, 15, 15, 15, 11, 11, 11, 19, 19, 19, 27, 27, 27, 24, 25, 24, 1, 1, 1};
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    EXPECT_NE(sharedMem_, nullptr);
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            ret_ = demuxer_->SeekToTime(*toPts, *mode);
            if (ret_ != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret_);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames_[0]=%d | kFrames[0]=%d\n", *toPts, frames_[0], keyFrames_[0]);
            EXPECT_EQ(frames_[0], videoVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: Demuxer_SeekToTime_2416
 * @tc.desc: seek to the specified time when file is trp(mpeg4), Uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_2416, TestSize.Level1)
{
    InitResource(g_trpUri2, URI);
    EXPECT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    list<int64_t> toPtsList = {0, 3480, 3640, 3350, 3000, 3110, 4120}; // ms
    vector<int32_t> videoVals = {103, 103, 103, 15, 15, 15, 11, 11, 11, 19, 20, 19, 27, 27, 27, 24, 25, 23, 1, 1, 1};
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    EXPECT_NE(sharedMem_, nullptr);
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            ret_ = demuxer_->SeekToTime(*toPts, *mode);
            if (ret_ != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret_);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames_[0]=%d | kFrames[0]=%d\n", *toPts, frames_[0], keyFrames_[0]);
            EXPECT_EQ(frames_[0], videoVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: Demuxer_SeekToTime_2417
 * @tc.desc: seek to the specified time when file is trp(mpeg4), Fd
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_2417, TestSize.Level1)
{
    InitResource(g_trpPath2, LOCAL);
    EXPECT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    list<int64_t> toPtsList = {0, 3480, 3640, 3350, 3000, 3110, 4120}; // ms
    vector<int32_t> videoVals = {103, 103, 103, 15, 15, 15, 11, 11, 11, 19, 20, 19, 27, 27, 27, 24, 25, 23, 1, 1, 1};
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    EXPECT_NE(sharedMem_, nullptr);
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            ret_ = demuxer_->SeekToTime(*toPts, *mode);
            if (ret_ != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret_);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames_[0]=%d | kFrames[0]=%d\n", *toPts, frames_[0], keyFrames_[0]);
            EXPECT_EQ(frames_[0], videoVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: Demuxer_SeekToTime_2418
 * @tc.desc: seek to the specified time when file is trp(h264, mp3), Uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_2418, TestSize.Level1)
{
    InitResource(g_trpUri3, URI);
    EXPECT_TRUE(SetInitValue());
    for (auto idx : selectedTrackIds_) {
        EXPECT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 4500, 7000, 2000, 6000}; // ms
    vector<int32_t> videoVals = {602, 602, 602, 330, 330, 330, 180, 180, 180, 480, 480, 480, 240, 240, 240};
    vector<int32_t> audioVals = {420, 420, 420, 238, 238, 238, 133, 133, 133, 336, 336, 336, 175, 175, 175};
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    EXPECT_NE(sharedMem_, nullptr);
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
            EXPECT_EQ(frames_[0], videoVals[numbers_]);
            EXPECT_EQ(frames_[1], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
    EXPECT_NE(demuxer_->SeekToTime(12000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    EXPECT_NE(demuxer_->SeekToTime(-1000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
}

/**
 * @tc.name: Demuxer_SeekToTime_2419
 * @tc.desc: seek to the specified time when file is trp(h264, mp3), Fd
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_2419, TestSize.Level1)
{
    InitResource(g_trpPath3, LOCAL);
    EXPECT_TRUE(SetInitValue());
    for (auto idx : selectedTrackIds_) {
        EXPECT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 4500, 7000, 2000, 6000}; // ms
    vector<int32_t> videoVals = {602, 602, 602, 330, 330, 330, 180, 180, 180, 480, 480, 480, 240, 240, 240};
    vector<int32_t> audioVals = {420, 420, 420, 238, 238, 238, 133, 133, 133, 336, 336, 336, 175, 175, 175};
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    EXPECT_NE(sharedMem_, nullptr);
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
            EXPECT_EQ(frames_[0], videoVals[numbers_]);
            EXPECT_EQ(frames_[1], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
    EXPECT_NE(demuxer_->SeekToTime(12000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    EXPECT_NE(demuxer_->SeekToTime(-1000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
}

/**
 * @tc.name: Demuxer_SeekToTime_2420
 * @tc.desc: seek to the specified time when file is trp(hevc, aac), Uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_2420, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) == 0) {
        InitResource(g_trpUri4, URI);
        EXPECT_TRUE(initStatus_);
        EXPECT_TRUE(SetInitValue());
        for (auto idx : selectedTrackIds_) {
            EXPECT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
        }
        list<int64_t> toPtsList = {0, 1500, 10000, 4600}; // ms
        vector<int32_t> videoVals = {303, 303, 303, 256, 256, 256, 1, 1, 1, 163, 163, 163};
        vector<int32_t> audioVals = {434, 434, 434, 370, 370, 370, 2, 2, 2, 236, 236, 236};
        sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
        EXPECT_NE(sharedMem_, nullptr);
        SeekTest(toPtsList, seekModes, {videoVals, audioVals});
        EXPECT_TRUE(seekTestFlag_);
        EXPECT_NE(demuxer_->SeekToTime(11000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
        EXPECT_NE(demuxer_->SeekToTime(-1000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    }
}

/**
 * @tc.name: Demuxer_SeekToTime_2421
 * @tc.desc: seek to the specified time when file is trp(hevc, aac), Fd
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_2421, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) == 0) {
        InitResource(g_trpPath4, LOCAL);
        EXPECT_TRUE(initStatus_);
        EXPECT_TRUE(SetInitValue());
        for (auto idx : selectedTrackIds_) {
            EXPECT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
        }
        list<int64_t> toPtsList = {0, 1500, 10000, 4600}; // ms
        vector<int32_t> videoVals = {303, 303, 303, 256, 256, 256, 1, 1, 1, 163, 163, 163};
        vector<int32_t> audioVals = {434, 434, 434, 370, 370, 370, 2, 2, 2, 236, 236, 236};
        sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
        EXPECT_NE(sharedMem_, nullptr);
        SeekTest(toPtsList, seekModes, {videoVals, audioVals});
        EXPECT_TRUE(seekTestFlag_);
        EXPECT_NE(demuxer_->SeekToTime(11000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
        EXPECT_NE(demuxer_->SeekToTime(-1000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    }
}
} // namespace