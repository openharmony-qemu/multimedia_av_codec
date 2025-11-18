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

string g_cafPcmPath = TEST_FILE_PATH + string("caf_pcm_s16le.caf");
string g_cafPcmUri = TEST_URI_PATH + string("caf_pcm_s16le.caf");
string g_cafAlacPath = TEST_FILE_PATH + string("caf_alac.caf");
string g_cafAlacUri = TEST_URI_PATH + string("caf_alac.caf");
string g_cafOpusPath = TEST_FILE_PATH + string("caf_opus.caf");
string g_cafOpusUri = TEST_URI_PATH + string("caf_opus.caf");

} //namespace

namespace {
/**
 * @tc.name: Demuxer_CAF_ReadSample_0001
 * @tc.desc: Read sample test for CAF(PCM_S16LE) format, local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CAF_ReadSample_0001, TestSize.Level1)
{
    InitResource(g_cafPcmPath, LOCAL);
    ASSERT_TRUE(initStatus_) << "Init CAF(PCM) local resource failed";
    
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK) << "Select CAF(PCM) track 0 failed";
    
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr) << "Create AVMemory for CAF(PCM) failed";
    ASSERT_TRUE(SetInitValue()) << "Set init value failed";

    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            ASSERT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK) 
                << "Read CAF(PCM) sample failed, track ID=" << idx;
            CountFrames(idx);
        }
    }

    printf("CAF(PCM) frames_[0]=%d | keyFrames_[0]=%d\n", frames_[0], keyFrames_[0]);
    ASSERT_EQ(frames_[0], 431);
    ASSERT_EQ(keyFrames_[0], 431);

    RemoveValue();
}

/**
 * @tc.name: Demuxer_CAF_ReadSample_0002
 * @tc.desc: Read sample test for CAF(PCM_S16LE) format, URI
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CAF_ReadSample_0002, TestSize.Level1)
{
    InitResource(g_cafPcmUri, URI);
    ASSERT_TRUE(initStatus_) << "Init CAF(PCM) URI resource failed";
    
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK) << "Select CAF(PCM) track 0 failed";
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr) << "Create AVMemory for CAF(PCM) failed";
    ASSERT_TRUE(SetInitValue()) << "Set init value failed";

    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            ASSERT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK)
                << "Read CAF(PCM) URI sample failed, track ID=" << idx;
            CountFrames(idx);
        }
    }

    printf("CAF(PCM) URI frames_[0]=%d | keyFrames_[0]=%d\n", frames_[0], keyFrames_[0]);
    ASSERT_EQ(frames_[0], 431);
    ASSERT_EQ(keyFrames_[0], 431);

    RemoveValue();
}

/**
 * @tc.name: Demuxer_CAF_ReadSample_0003
 * @tc.desc: Read sample test for CAF(ALAC) format, local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CAF_ReadSample_0003, TestSize.Level1)
{
    InitResource(g_cafAlacPath, LOCAL);
    ASSERT_TRUE(initStatus_) << "Init CAF(ALAC) local resource failed";
    
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK) << "Select CAF(ALAC) track 0 failed";
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr) << "Create AVMemory for CAF(ALAC) failed";
    ASSERT_TRUE(SetInitValue()) << "Set init value failed";

    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            ASSERT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK)
                << "Read CAF(ALAC) sample failed, track ID=" << idx;
            CountFrames(idx);
        }
    }

    printf("CAF(ALAC) frames_[0]=%d | keyFrames_[0]=%d\n", frames_[0], keyFrames_[0]);
    ASSERT_EQ(frames_[0], 108);
    ASSERT_EQ(keyFrames_[0], 108);

    RemoveValue();
}

/**
 * @tc.name: Demuxer_CAF_ReadSample_0004
 * @tc.desc: Read sample test for CAF(ALAC) format, URI
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CAF_ReadSample_0004, TestSize.Level1)
{
    InitResource(g_cafAlacUri, URI);
    ASSERT_TRUE(initStatus_) << "Init CAF(ALAC) URI resource failed";
    
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK) << "Select CAF(ALAC) track 0 failed";
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr) << "Create AVMemory for CAF(ALAC) failed";
    ASSERT_TRUE(SetInitValue()) << "Set init value failed";

    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            ASSERT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK)
                << "Read CAF(ALAC) URI sample failed, track ID=" << idx;
            CountFrames(idx);
        }
    }

    printf("CAF(ALAC) URI frames_[0]=%d | keyFrames_[0]=%d\n", frames_[0], keyFrames_[0]);
    ASSERT_EQ(frames_[0], 108);
    ASSERT_EQ(keyFrames_[0], 108);

    RemoveValue();
}

/**
 * @tc.name: Demuxer_CAF_ReadSample_0005
 * @tc.desc: Read sample test for CAF(Opus) format, local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CAF_ReadSample_0005, TestSize.Level1)
{
    InitResource(g_cafOpusPath, LOCAL);
    ASSERT_TRUE(initStatus_) << "Init CAF(Opus) local resource failed";
    
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK) << "Select CAF(Opus) track 0 failed";
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr) << "Create AVMemory for CAF(Opus) failed";
    ASSERT_TRUE(SetInitValue()) << "Set init value failed";

    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            ASSERT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK)
                << "Read CAF(Opus) sample failed, track ID=" << idx;
            CountFrames(idx);
        }
    }

    printf("CAF(Opus) frames_[0]=%d | keyFrames_[0]=%d\n", frames_[0], keyFrames_[0]);
    ASSERT_EQ(frames_[0], 501);
    ASSERT_EQ(keyFrames_[0], 501);

    RemoveValue();
}

/**
 * @tc.name: Demuxer_CAF_ReadSample_0006
 * @tc.desc: Read sample test for CAF(Opus) format, URI
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CAF_ReadSample_0006, TestSize.Level1)
{
    InitResource(g_cafOpusUri, URI);
    ASSERT_TRUE(initStatus_) << "Init CAF(Opus) URI resource failed";
    
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK) << "Select CAF(Opus) track 0 failed";
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr) << "Create AVMemory for CAF(Opus) failed";
    ASSERT_TRUE(SetInitValue()) << "Set init value failed";

    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            ASSERT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK)
                << "Read CAF(Opus) URI sample failed, track ID=" << idx;
            CountFrames(idx);
        }
    }

    printf("CAF(Opus) URI frames_[0]=%d | keyFrames_[0]=%d\n", frames_[0], keyFrames_[0]);
    ASSERT_EQ(frames_[0], 501);
    ASSERT_EQ(keyFrames_[0], 501);

    RemoveValue();
}

/**
 * @tc.name: Demuxer_CAF_SeekToTime_0007
 * @tc.desc: Seek to specified time for CAF(PCM_S16LE) format, local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CAF_SeekToTime_0007, TestSize.Level1)
{
    InitResource(g_cafPcmPath, LOCAL);
    ASSERT_TRUE(initStatus_) << "Init CAF(PCM) seek local resource failed";
    ASSERT_TRUE(SetInitValue()) << "Set init value failed";
    
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK) << "Select track " << idx << " failed";
    }

    list<int64_t> toPtsList = {0, 3000, 7000};
    vector<int32_t> audioVals = {431, 302, 130};
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr) << "Create AVMemory for CAF(PCM) seek failed";

    numbers_ = 0;
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            ret_ = demuxer_->SeekToTime(*toPts, *mode);
            if (ret_ != AV_ERR_OK) {
                printf("CAF(PCM) seek failed, time = %" PRId64 "ms | mode = %d | ret = %d\n", 
                       *toPts, static_cast<int>(*mode), ret_);
                continue;
            }
            ReadData();
            printf("CAF(PCM) seek time = %" PRId64 "ms | mode = %d | remaining frames = %d\n",
                   *toPts, static_cast<int>(*mode), frames_[0]);
            
            ASSERT_EQ(frames_[0], audioVals[numbers_]) << "PCM seek remaining frames not match (expected " << audioVals[numbers_] << ")";
        }
        numbers_ += 1;
        RemoveValue();
        selectedTrackIds_.clear();
    }
}

/**
 * @tc.name: Demuxer_CAF_SeekToTime_0008
 * @tc.desc: Seek to specified time for CAF(PCM_S16LE) format, URI
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CAF_SeekToTime_0008, TestSize.Level1)
{
    InitResource(g_cafPcmUri, URI);
    ASSERT_TRUE(initStatus_) << "Init CAF(PCM) seek URI resource failed";
    ASSERT_TRUE(SetInitValue()) << "Set init value failed";
    
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK) << "Select track " << idx << " failed";
    }

    list<int64_t> toPtsList = {0, 3000, 7000};
    vector<int32_t> audioVals = {431, 302, 130};
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr) << "Create AVMemory for CAF(PCM) URI seek failed";

    numbers_ = 0;
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            ret_ = demuxer_->SeekToTime(*toPts, *mode);
            if (ret_ != AV_ERR_OK) {
                printf("CAF(PCM) URI seek failed, time = %" PRId64 "ms | mode = %d | ret = %d\n", 
                       *toPts, static_cast<int>(*mode), ret_);
                continue;
            }
            ReadData();
            printf("CAF(PCM) URI seek time = %" PRId64 "ms | mode = %d | remaining frames = %d\n",
                   *toPts, static_cast<int>(*mode), frames_[0]);
            
            ASSERT_EQ(frames_[0], audioVals[numbers_]) << "PCM URI seek remaining frames not match (expected " << audioVals[numbers_] << ")";
        }
        numbers_ += 1;
        RemoveValue();
        selectedTrackIds_.clear();
    }
}

/**
 * @tc.name: Demuxer_CAF_SeekToTime_0009
 * @tc.desc: Seek to specified time for CAF(Opus) format, local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CAF_SeekToTime_0009, TestSize.Level1)
{
    InitResource(g_cafOpusPath, LOCAL);
    ASSERT_TRUE(initStatus_) << "Init CAF(Opus) seek local resource failed";
    ASSERT_TRUE(SetInitValue()) << "Set init value failed";
    
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK) << "Select track " << idx << " failed";
    }

    list<int64_t> toPtsList = {0, 2000, 6000};
    vector<int32_t> audioVals = {501, 401, 201};
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr) << "Create AVMemory for CAF(Opus) seek failed";

    numbers_ = 0;
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            ret_ = demuxer_->SeekToTime(*toPts, *mode);
            if (ret_ != AV_ERR_OK) {
                printf("CAF(Opus) seek failed, time = %" PRId64 "ms | mode = %d | ret = %d\n", 
                       *toPts, static_cast<int>(*mode), ret_);
                continue;
            }
            ReadData();
            printf("CAF(Opus) seek time = %" PRId64 "ms | mode = %d | remaining frames = %d\n",
                   *toPts, static_cast<int>(*mode), frames_[0]);
            
            ASSERT_EQ(frames_[0], audioVals[numbers_]) << "Opus seek remaining frames not match (expected " << audioVals[numbers_] << ")";
        }
        numbers_ += 1;
        RemoveValue();
        selectedTrackIds_.clear();
    }
}

/**
 * @tc.name: Demuxer_CAF_SeekToTime_0010
 * @tc.desc: Seek to specified time for CAF(Opus) format, URI
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CAF_SeekToTime_0010, TestSize.Level1)
{
    InitResource(g_cafOpusUri, URI);
    ASSERT_TRUE(initStatus_) << "Init CAF(Opus) seek URI resource failed";
    ASSERT_TRUE(SetInitValue()) << "Set init value failed";
    
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK) << "Select track " << idx << " failed";
    }

    list<int64_t> toPtsList = {0, 2000, 6000};
    vector<int32_t> audioVals = {501, 401, 201};
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr) << "Create AVMemory for CAF(Opus) URI seek failed";

    numbers_ = 0;
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            ret_ = demuxer_->SeekToTime(*toPts, *mode);
            if (ret_ != AV_ERR_OK) {
                printf("CAF(Opus) URI seek failed, time = %" PRId64 "ms | mode = %d | ret = %d\n", 
                       *toPts, static_cast<int>(*mode), ret_);
                continue;
            }
            ReadData();
            printf("CAF(Opus) URI seek time = %" PRId64 "ms | mode = %d | remaining frames = %d\n",
                   *toPts, static_cast<int>(*mode), frames_[0]);
            
            ASSERT_EQ(frames_[0], audioVals[numbers_]) << "Opus URI seek remaining frames not match (expected " << audioVals[numbers_] << ")";
        }
        numbers_ += 1;
        RemoveValue();
        selectedTrackIds_.clear();
    }
}

/**
 * @tc.name: Demuxer_CAF_Seek_InvalidTime_0011
 * @tc.desc: Seek to invalid time for CAF format, local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CAF_Seek_InvalidTime_0011, TestSize.Level1)
{
    InitResource(g_cafPcmPath, LOCAL);
    ASSERT_TRUE(initStatus_) << "Init CAF invalid seek resource failed";
    ASSERT_TRUE(SetInitValue()) << "Set init value failed";
    
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK) << "Select track " << idx << " failed";
    }

    list<int64_t> invalidPtsList = {-1000, 15000};
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr) << "Create AVMemory for invalid seek failed";

    for (auto toPts = invalidPtsList.begin(); toPts != invalidPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            ret_ = demuxer_->SeekToTime(*toPts, *mode);
            ASSERT_NE(ret_, AV_ERR_OK) << "Seek to invalid time " << *toPts << "ms should return non-OK";
            printf("CAF invalid seek time = %" PRId64 "ms | mode = %d | ret = %d (expected non-OK)\n",
                   *toPts, static_cast<int>(*mode), ret_);
        }
    }

    RemoveValue();
}

/**
 * @tc.name: Demuxer_CAF_RepeatRead_0012
 * @tc.desc: Repeat read sample test for CAF format, local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CAF_RepeatRead_0012, TestSize.Level1)
{
    const int repeatCount = 3;
    for (int i = 0; i < repeatCount; i++) {
        InitResource(g_cafOpusPath, LOCAL);
        ASSERT_TRUE(initStatus_) << "Repeat read init resource failed, times=" << i+1;
        
        ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK) << "Repeat read select track failed, times=" << i+1;
        sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
        ASSERT_NE(sharedMem_, nullptr) << "Repeat read create AVMemory failed, times=" << i+1;
        ASSERT_TRUE(SetInitValue()) << "Repeat read set init value failed, times=" << i+1;

        while (!isEOS(eosFlag_)) {
            for (auto idx : selectedTrackIds_) {
                ASSERT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK)
                    << "Repeat read sample failed, times=" << i+1 << ", track ID=" << idx;
                CountFrames(idx);
            }
        }

        printf("CAF repeat read times=%d | frames_[0]=%d | keyFrames_[0]=%d\n", i+1, frames_[0], keyFrames_[0]);
        ASSERT_EQ(frames_[0], 501);
        ASSERT_EQ(keyFrames_[0], 501);

        RemoveValue();
    }
}

} //namespace