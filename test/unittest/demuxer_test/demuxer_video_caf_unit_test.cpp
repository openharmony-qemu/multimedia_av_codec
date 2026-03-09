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

string g_cafFilePath1 = TEST_FILE_PATH + string("ac3.caf");
string g_cafFilePath2 = TEST_FILE_PATH + string("adpcm_ima_wav.caf");
string g_cafFilePath3 = TEST_FILE_PATH + string("adpcm_ms.caf");
string g_cafFilePath4 = TEST_FILE_PATH + string("alac.caf");
string g_cafFilePath5 = TEST_FILE_PATH + string("amr_nb.caf");
string g_cafFilePath6 = TEST_FILE_PATH + string("flac.caf");
string g_cafFilePath7 = TEST_FILE_PATH + string("gsm.caf");
string g_cafFilePath8 = TEST_FILE_PATH + string("gsm_ms.caf");
string g_cafFilePath9 = TEST_FILE_PATH + string("ilbc.caf");
string g_cafFilePath10 = TEST_FILE_PATH + string("mp2.caf");
string g_cafFilePath11 = TEST_FILE_PATH + string("mp3.caf");
string g_cafFilePath12 = TEST_FILE_PATH + string("opus.caf");
string g_cafFilePath13 = TEST_FILE_PATH + string("pcm_alaw.caf");
string g_cafFilePath14 = TEST_FILE_PATH + string("pcm_mulaw.caf");
string g_cafFilePath15 = TEST_FILE_PATH + string("pcm_s16be.caf");
string g_cafFilePath16 = TEST_FILE_PATH + string("caf_adpcm_ima_qt.caf");
string g_cafFilePath17 = TEST_FILE_PATH + string("caf_pcm_f32be.caf");
string g_cafFilePath18 = TEST_FILE_PATH + string("caf_pcm_f32le.caf");
string g_cafFilePath19 = TEST_FILE_PATH + string("caf_pcm_f64be.caf");
string g_cafFilePath20 = TEST_FILE_PATH + string("caf_pcm_f64le.caf");
string g_cafFilePath21 = TEST_FILE_PATH + string("caf_pcm_s8.caf");
string g_cafFilePath23 = TEST_FILE_PATH + string("caf_pcm_s24be.caf");
string g_cafFilePath24 = TEST_FILE_PATH + string("caf_pcm_s24le.caf");
string g_cafFilePath25 = TEST_FILE_PATH + string("caf_pcm_s32be.caf");
string g_cafFilePath26 = TEST_FILE_PATH + string("caf_pcm_s32le.caf");

string g_cafUriPath1 = TEST_URI_PATH + string("ac3.caf");
string g_cafUriPath2 = TEST_URI_PATH + string("adpcm_ima_wav.caf");
string g_cafUriPath3 = TEST_URI_PATH + string("adpcm_ms.caf");
string g_cafUriPath4 = TEST_URI_PATH + string("alac.caf");
string g_cafUriPath5 = TEST_URI_PATH + string("amr_nb.caf");
string g_cafUriPath6 = TEST_URI_PATH + string("flac.caf");
string g_cafUriPath7 = TEST_URI_PATH + string("gsm.caf");
string g_cafUriPath8 = TEST_URI_PATH + string("gsm_ms.caf");
string g_cafUriPath9 = TEST_URI_PATH + string("ilbc.caf");
string g_cafUriPath10 = TEST_URI_PATH + string("mp2.caf");
string g_cafUriPath11 = TEST_URI_PATH + string("mp3.caf");
string g_cafUriPath12 = TEST_URI_PATH + string("opus.caf");
string g_cafUriPath13 = TEST_URI_PATH + string("pcm_alaw.caf");
string g_cafUriPath14 = TEST_URI_PATH + string("pcm_mulaw.caf");
string g_cafUriPath15 = TEST_URI_PATH + string("pcm_s16be.caf");
string g_cafUriPath16 = TEST_URI_PATH + string("caf_adpcm_ima_qt.caf");
string g_cafUriPath17 = TEST_URI_PATH + string("caf_pcm_f32be.caf");
string g_cafUriPath18 = TEST_URI_PATH + string("caf_pcm_f32le.caf");
string g_cafUriPath19 = TEST_URI_PATH + string("caf_pcm_f64be.caf");
string g_cafUriPath20 = TEST_URI_PATH + string("caf_pcm_f64le.caf");
string g_cafUriPath21 = TEST_URI_PATH + string("caf_pcm_s8.caf");
string g_cafUriPath23 = TEST_URI_PATH + string("caf_pcm_s24be.caf");
string g_cafUriPath24 = TEST_URI_PATH + string("caf_pcm_s24le.caf");
string g_cafUriPath25 = TEST_URI_PATH + string("caf_pcm_s32be.caf");
string g_cafUriPath26 = TEST_URI_PATH + string("caf_pcm_s32le.caf");

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
            
            ASSERT_EQ(frames_[0], audioVals[numbers_]);
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
            
            ASSERT_EQ(frames_[0], audioVals[numbers_]);
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
            
            ASSERT_EQ(frames_[0], audioVals[numbers_]);
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
            
            ASSERT_EQ(frames_[0], audioVals[numbers_]);
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
        ASSERT_TRUE(initStatus_) << "Repeat read init resource failed, times=" << i + 1;
        
        ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK) << "Repeat read select track failed, times=" << i + 1;
        sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
        ASSERT_NE(sharedMem_, nullptr) << "Repeat read create AVMemory failed, times=" << i + 1;
        ASSERT_TRUE(SetInitValue()) << "Repeat read set init value failed, times=" << i + 1;

        while (!isEOS(eosFlag_)) {
            for (auto idx : selectedTrackIds_) {
                ASSERT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK)
                    << "Repeat read sample failed, times=" << i + 1 << ", track ID=" << idx;
                CountFrames(idx);
            }
        }

        printf("CAF repeat read times=%d | frames_[0]=%d | keyFrames_[0]=%d\n", i + 1, frames_[0], keyFrames_[0]);
        ASSERT_EQ(frames_[0], 501);
        ASSERT_EQ(keyFrames_[0], 501);

        RemoveValue();
    }
}

/**
 * @tc.name: Demuxer_CAF_ReadSample_And_SeekToTime_0001
 * @tc.desc: Read sample test for timed metadata track and seek to the specified time,local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CAF_ReadSample_And_SeekToTime_0001, TestSize.Level1)
{
    InitResource(g_cafFilePath1, LOCAL);
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
    EXPECT_EQ(frames_[0], 158);
    EXPECT_EQ(keyFrames_[0], 158);

    for (auto idx : selectedTrackIds_) {
        EXPECT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 200, 600}; // ms
    vector<int32_t> audioVals = {158, 158, 158, 151, 152, 152, 139, 140, 140};
    numbers_ = 0;
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
 * @tc.name: Demuxer_CAF_ReadSample_And_SeekToTime_0002
 * @tc.desc: Read sample test for timed metadata track and seek to the specified time,uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CAF_ReadSample_And_SeekToTime_0002, TestSize.Level1)
{
    InitResource(g_cafUriPath1, URI);
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
    EXPECT_EQ(frames_[0], 158);
    EXPECT_EQ(keyFrames_[0], 158);

    for (auto idx : selectedTrackIds_) {
        EXPECT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 200, 600}; // ms
    vector<int32_t> audioVals = {158, 158, 158, 151, 152, 152, 139, 140, 140};
    numbers_ = 0;
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
 * @tc.name: Demuxer_CAF_ReadSample_And_SeekToTime_0003
 * @tc.desc: Read sample test for timed metadata track and seek to the specified time,local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CAF_ReadSample_And_SeekToTime_0003, TestSize.Level1)
{
    InitResource(g_cafFilePath2, LOCAL);
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
    EXPECT_EQ(frames_[0], 119);
    EXPECT_EQ(keyFrames_[0], 119);

    for (auto idx : selectedTrackIds_) {
        EXPECT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 200, 600}; // ms
    vector<int32_t> audioVals = {119, 119, 119, 114, 115, 115, 104, 105, 105};
    numbers_ = 0;
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
 * @tc.name: Demuxer_CAF_ReadSample_And_SeekToTime_0004
 * @tc.desc: Read sample test for timed metadata track and seek to the specified time,uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CAF_ReadSample_And_SeekToTime_0004, TestSize.Level1)
{
    InitResource(g_cafUriPath2, URI);
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
    EXPECT_EQ(frames_[0], 119);
    EXPECT_EQ(keyFrames_[0], 119);

    for (auto idx : selectedTrackIds_) {
        EXPECT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 200, 600}; // ms
    vector<int32_t> audioVals = {119, 119, 119, 114, 115, 115, 104, 105, 105};
    numbers_ = 0;
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
 * @tc.name: Demuxer_CAF_ReadSample_And_SeekToTime_0005
 * @tc.desc: Read sample test for timed metadata track and seek to the specified time,local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CAF_ReadSample_And_SeekToTime_0005, TestSize.Level1)
{
    InitResource(g_cafFilePath3, LOCAL);
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
    EXPECT_EQ(frames_[0], 119);
    EXPECT_EQ(keyFrames_[0], 119);

    for (auto idx : selectedTrackIds_) {
        EXPECT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 200, 600}; // ms
    vector<int32_t> audioVals = {119, 119, 119, 114, 115, 115, 104, 105, 105};
    numbers_ = 0;
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
 * @tc.name: Demuxer_CAF_ReadSample_And_SeekToTime_0006
 * @tc.desc: Read sample test for timed metadata track and seek to the specified time,uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CAF_ReadSample_And_SeekToTime_0006, TestSize.Level1)
{
    InitResource(g_cafUriPath3, URI);
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
    EXPECT_EQ(frames_[0], 119);
    EXPECT_EQ(keyFrames_[0], 119);

    for (auto idx : selectedTrackIds_) {
        EXPECT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 200, 600}; // ms
    vector<int32_t> audioVals = {119, 119, 119, 114, 115, 115, 104, 105, 105};
    numbers_ = 0;
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
 * @tc.name: Demuxer_CAF_ReadSample_And_SeekToTime_0007
 * @tc.desc: Read sample test for timed metadata track and seek to the specified time,local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CAF_ReadSample_And_SeekToTime_0007, TestSize.Level1)
{
    InitResource(g_cafFilePath4, LOCAL);
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
    EXPECT_EQ(frames_[0], 60);
    EXPECT_EQ(keyFrames_[0], 60);

    for (auto idx : selectedTrackIds_) {
        EXPECT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 200, 600}; // ms
    vector<int32_t> audioVals = {60, 60, 60, 57, 58, 58, 52, 53, 53};
    numbers_ = 0;
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
 * @tc.name: Demuxer_CAF_ReadSample_And_SeekToTime_0008
 * @tc.desc: Read sample test for timed metadata track and seek to the specified time,uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CAF_ReadSample_And_SeekToTime_0008, TestSize.Level1)
{
    InitResource(g_cafUriPath4, URI);
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
    EXPECT_EQ(frames_[0], 60);
    EXPECT_EQ(keyFrames_[0], 60);

    for (auto idx : selectedTrackIds_) {
        EXPECT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 200, 600}; // ms
    vector<int32_t> audioVals = {60, 60, 60, 57, 58, 58, 52, 53, 53};
    numbers_ = 0;
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
 * @tc.name: Demuxer_CAF_ReadSample_And_SeekToTime_0009
 * @tc.desc: Read sample test for timed metadata track and seek to the specified time,local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CAF_ReadSample_And_SeekToTime_0009, TestSize.Level1)
{
    InitResource(g_cafFilePath6, LOCAL);
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
    EXPECT_EQ(frames_[0], 53);
    EXPECT_EQ(keyFrames_[0], 53);

    for (auto idx : selectedTrackIds_) {
        EXPECT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 200, 600}; // ms
    vector<int32_t> audioVals = {53, 53, 53, 50, 51, 51, 46, 47, 47};
    numbers_ = 0;
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
 * @tc.name: Demuxer_CAF_ReadSample_And_SeekToTime_0010
 * @tc.desc: Read sample test for timed metadata track and seek to the specified time,uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CAF_ReadSample_And_SeekToTime_0010, TestSize.Level1)
{
    InitResource(g_cafUriPath6, URI);
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
    EXPECT_EQ(frames_[0], 53);
    EXPECT_EQ(keyFrames_[0], 53);

    for (auto idx : selectedTrackIds_) {
        EXPECT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 200, 600}; // ms
    vector<int32_t> audioVals = {53, 53, 53, 50, 51, 51, 46, 47, 47};
    numbers_ = 0;
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
 * @tc.name: Demuxer_CAF_ReadSample_And_SeekToTime_0011
 * @tc.desc: Read sample test for timed metadata track and seek to the specified time,local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CAF_ReadSample_And_SeekToTime_0011, TestSize.Level1)
{
    InitResource(g_cafFilePath7, LOCAL);
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
    EXPECT_EQ(frames_[0], 252);
    EXPECT_EQ(keyFrames_[0], 252);

    for (auto idx : selectedTrackIds_) {
        EXPECT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 200, 600}; // ms
    vector<int32_t> audioVals = {252, 252, 252, 242, 242, 242, 222, 222, 222};
    numbers_ = 0;
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
 * @tc.name: Demuxer_CAF_ReadSample_And_SeekToTime_0012
 * @tc.desc: Read sample test for timed metadata track and seek to the specified time,uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CAF_ReadSample_And_SeekToTime_0012, TestSize.Level1)
{
    InitResource(g_cafUriPath7, URI);
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
    EXPECT_EQ(frames_[0], 252);
    EXPECT_EQ(keyFrames_[0], 252);

    for (auto idx : selectedTrackIds_) {
        EXPECT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 200, 600}; // ms
    vector<int32_t> audioVals = {252, 252, 252, 242, 242, 242, 222, 222, 222};
    numbers_ = 0;
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
 * @tc.name: Demuxer_CAF_ReadSample_And_SeekToTime_0013
 * @tc.desc: Read sample test for timed metadata track and seek to the specified time,local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CAF_ReadSample_And_SeekToTime_0013, TestSize.Level1)
{
    InitResource(g_cafFilePath8, LOCAL);
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
    EXPECT_EQ(frames_[0], 126);
    EXPECT_EQ(keyFrames_[0], 126);

    for (auto idx : selectedTrackIds_) {
        EXPECT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 200, 600}; // ms
    vector<int32_t> audioVals = {126, 126, 126, 121, 121, 121, 111, 111, 111};
    numbers_ = 0;
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
 * @tc.name: Demuxer_CAF_ReadSample_And_SeekToTime_0014
 * @tc.desc: Read sample test for timed metadata track and seek to the specified time,uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CAF_ReadSample_And_SeekToTime_0014, TestSize.Level1)
{
    InitResource(g_cafUriPath8, URI);
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
    EXPECT_EQ(frames_[0], 126);
    EXPECT_EQ(keyFrames_[0], 126);

    for (auto idx : selectedTrackIds_) {
        EXPECT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 200, 600}; // ms
    vector<int32_t> audioVals = {126, 126, 126, 121, 121, 121, 111, 111, 111};
    numbers_ = 0;
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
 * @tc.name: Demuxer_CAF_ReadSample_And_SeekToTime_0015
 * @tc.desc: Read sample test for timed metadata track and seek to the specified time,local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CAF_ReadSample_And_SeekToTime_0015, TestSize.Level1)
{
    InitResource(g_cafFilePath9, LOCAL);
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
    EXPECT_EQ(frames_[0], 252);
    EXPECT_EQ(keyFrames_[0], 252);

    for (auto idx : selectedTrackIds_) {
        EXPECT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 200, 600}; // ms
    vector<int32_t> audioVals = {252, 252, 252, 242, 242, 242, 222, 222, 222};
    numbers_ = 0;
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
 * @tc.name: Demuxer_CAF_ReadSample_And_SeekToTime_0016
 * @tc.desc: Read sample test for timed metadata track and seek to the specified time,uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CAF_ReadSample_And_SeekToTime_0016, TestSize.Level1)
{
    InitResource(g_cafUriPath9, URI);
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
    EXPECT_EQ(frames_[0], 252);
    EXPECT_EQ(keyFrames_[0], 252);

    for (auto idx : selectedTrackIds_) {
        EXPECT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 200, 600}; // ms
    vector<int32_t> audioVals = {252, 252, 252, 242, 242, 242, 222, 222, 222};
    numbers_ = 0;
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
 * @tc.name: Demuxer_CAF_ReadSample_And_SeekToTime_0017
 * @tc.desc: Read sample test for timed metadata track and seek to the specified time,local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CAF_ReadSample_And_SeekToTime_0017, TestSize.Level1)
{
    InitResource(g_cafFilePath10, LOCAL);
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
    EXPECT_EQ(frames_[0], 210);
    EXPECT_EQ(keyFrames_[0], 210);

    for (auto idx : selectedTrackIds_) {
        EXPECT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 200, 600}; // ms
    vector<int32_t> audioVals = {210, 210, 210, 201, 202, 202, 185, 185, 185};
    numbers_ = 0;
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
 * @tc.name: Demuxer_CAF_ReadSample_And_SeekToTime_0018
 * @tc.desc: Read sample test for timed metadata track and seek to the specified time,uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CAF_ReadSample_And_SeekToTime_0018, TestSize.Level1)
{
    InitResource(g_cafUriPath10, URI);
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
    EXPECT_EQ(frames_[0], 210);
    EXPECT_EQ(keyFrames_[0], 210);

    for (auto idx : selectedTrackIds_) {
        EXPECT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 200, 600}; // ms
    vector<int32_t> audioVals = {210, 210, 210, 201, 202, 202, 185, 185, 185};
    numbers_ = 0;
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
 * @tc.name: Demuxer_CAF_ReadSample_And_SeekToTime_0019
 * @tc.desc: Read sample test for timed metadata track and seek to the specified time,local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CAF_ReadSample_And_SeekToTime_0019, TestSize.Level1)
{
    InitResource(g_cafFilePath11, LOCAL);
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
    EXPECT_EQ(frames_[0], 210);
    EXPECT_EQ(keyFrames_[0], 210);

    for (auto idx : selectedTrackIds_) {
        EXPECT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 200, 600}; // ms
    vector<int32_t> audioVals = {210, 210, 210, 201, 202, 202, 185, 185, 185};
    numbers_ = 0;
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
 * @tc.name: Demuxer_CAF_ReadSample_And_SeekToTime_0020
 * @tc.desc: Read sample test for timed metadata track and seek to the specified time,uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CAF_ReadSample_And_SeekToTime_0020, TestSize.Level1)
{
    InitResource(g_cafUriPath11, URI);
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
    EXPECT_EQ(frames_[0], 210);
    EXPECT_EQ(keyFrames_[0], 210);

    for (auto idx : selectedTrackIds_) {
        EXPECT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 200, 600}; // ms
    vector<int32_t> audioVals = {210, 210, 210, 201, 202, 202, 185, 185, 185};
    numbers_ = 0;
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
 * @tc.name: Demuxer_CAF_ReadSample_And_SeekToTime_0021
 * @tc.desc: Read sample test for timed metadata track and seek to the specified time,local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CAF_ReadSample_And_SeekToTime_0021, TestSize.Level1)
{
    InitResource(g_cafFilePath12, LOCAL);
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
    EXPECT_EQ(frames_[0], 253);
    EXPECT_EQ(keyFrames_[0], 253);

    for (auto idx : selectedTrackIds_) {
        EXPECT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 200, 600}; // ms
    vector<int32_t> audioVals = {253, 253, 253, 243, 243, 243, 223, 223, 223};
    numbers_ = 0;
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
 * @tc.name: Demuxer_CAF_ReadSample_And_SeekToTime_0022
 * @tc.desc: Read sample test for timed metadata track and seek to the specified time,uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CAF_ReadSample_And_SeekToTime_0022, TestSize.Level1)
{
    InitResource(g_cafUriPath12, URI);
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
    EXPECT_EQ(frames_[0], 253);
    EXPECT_EQ(keyFrames_[0], 253);

    for (auto idx : selectedTrackIds_) {
        EXPECT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 200, 600}; // ms
    vector<int32_t> audioVals = {253, 253, 253, 243, 243, 243, 223, 223, 223};
    numbers_ = 0;
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
 * @tc.name: Demuxer_CAF_ReadSample_And_SeekToTime_0023
 * @tc.desc: Read sample test for timed metadata track and seek to the specified time,local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CAF_ReadSample_And_SeekToTime_0023, TestSize.Level1)
{
    InitResource(g_cafFilePath13, LOCAL);
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
    EXPECT_EQ(frames_[0], 60);
    EXPECT_EQ(keyFrames_[0], 60);

    for (auto idx : selectedTrackIds_) {
        EXPECT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 200, 600}; // ms
    vector<int32_t> audioVals = {60, 60, 60, 57, 57, 57, 53, 53, 53};
    numbers_ = 0;
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
 * @tc.name: Demuxer_CAF_ReadSample_And_SeekToTime_0024
 * @tc.desc: Read sample test for timed metadata track and seek to the specified time,uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CAF_ReadSample_And_SeekToTime_0024, TestSize.Level1)
{
    InitResource(g_cafUriPath13, URI);
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
    EXPECT_EQ(frames_[0], 60);
    EXPECT_EQ(keyFrames_[0], 60);

    for (auto idx : selectedTrackIds_) {
        EXPECT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 200, 600}; // ms
    vector<int32_t> audioVals = {60, 60, 60, 57, 57, 57, 53, 53, 53};
    numbers_ = 0;
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
 * @tc.name: Demuxer_CAF_ReadSample_And_SeekToTime_0025
 * @tc.desc: Read sample test for timed metadata track and seek to the specified time,local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CAF_ReadSample_And_SeekToTime_0025, TestSize.Level1)
{
    InitResource(g_cafFilePath14, LOCAL);
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
    EXPECT_EQ(frames_[0], 10);
    EXPECT_EQ(keyFrames_[0], 10);

    for (auto idx : selectedTrackIds_) {
        EXPECT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 200, 600}; // ms
    vector<int32_t> audioVals = {10, 10, 10, 10, 10, 10, 9, 9, 9};
    numbers_ = 0;
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
 * @tc.name: Demuxer_CAF_ReadSample_And_SeekToTime_0026
 * @tc.desc: Read sample test for timed metadata track and seek to the specified time,uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CAF_ReadSample_And_SeekToTime_0026, TestSize.Level1)
{
    InitResource(g_cafUriPath14, URI);
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
    EXPECT_EQ(frames_[0], 10);
    EXPECT_EQ(keyFrames_[0], 10);

    for (auto idx : selectedTrackIds_) {
        EXPECT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 200, 600}; // ms
    vector<int32_t> audioVals = {10, 10, 10, 10, 10, 10, 9, 9, 9};
    numbers_ = 0;
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
 * @tc.name: Demuxer_CAF_ReadSample_And_SeekToTime_0027
 * @tc.desc: Read sample test for timed metadata track and seek to the specified time,local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CAF_ReadSample_And_SeekToTime_0027, TestSize.Level1)
{
    InitResource(g_cafFilePath15, LOCAL);
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
    EXPECT_EQ(frames_[0], 119);
    EXPECT_EQ(keyFrames_[0], 119);

    for (auto idx : selectedTrackIds_) {
        EXPECT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 200, 600}; // ms
    vector<int32_t> audioVals = {119, 119, 119, 114, 114, 114, 105, 105, 105};
    numbers_ = 0;
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
 * @tc.name: Demuxer_CAF_ReadSample_And_SeekToTime_0028
 * @tc.desc: Read sample test for timed metadata track and seek to the specified time,uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CAF_ReadSample_And_SeekToTime_0028, TestSize.Level1)
{
    InitResource(g_cafUriPath15, URI);
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
    EXPECT_EQ(frames_[0], 119);
    EXPECT_EQ(keyFrames_[0], 119);

    for (auto idx : selectedTrackIds_) {
        EXPECT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 200, 600}; // ms
    vector<int32_t> audioVals = {119, 119, 119, 114, 114, 114, 105, 105, 105};
    numbers_ = 0;
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
 * @tc.name: Demuxer_CAF_ReadSample_And_SeekToTime_0029
 * @tc.desc: Read sample test for timed metadata track and seek to the specified time,local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CAF_ReadSample_And_SeekToTime_0029, TestSize.Level1)
{
    InitResource(g_cafFilePath5, LOCAL);
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
    EXPECT_EQ(frames_[0], 253);
    EXPECT_EQ(keyFrames_[0], 253);

    for (auto idx : selectedTrackIds_) {
        EXPECT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 200, 600}; // ms
    vector<int32_t> audioVals = {253, 253, 253, 243, 243, 243, 223, 223, 223};
    numbers_ = 0;
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
 * @tc.name: Demuxer_CAF_ReadSample_And_SeekToTime_0030
 * @tc.desc: Read sample test for timed metadata track and seek to the specified time,uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CAF_ReadSample_And_SeekToTime_0030, TestSize.Level1)
{
    InitResource(g_cafUriPath5, URI);
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
    EXPECT_EQ(frames_[0], 253);
    EXPECT_EQ(keyFrames_[0], 253);

    for (auto idx : selectedTrackIds_) {
        EXPECT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 200, 600}; // ms
    vector<int32_t> audioVals = {253, 253, 253, 243, 243, 243, 223, 223, 223};
    numbers_ = 0;
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
 * @tc.name: Demuxer_CAF_ReadSample_And_SeekToTime_0031
 * @tc.desc: Read sample test for timed metadata track and seek to the specified time,local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CAF_ReadSample_And_SeekToTime_0031, TestSize.Level1)
{
    InitResource(g_cafFilePath16, LOCAL);
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
    list<int64_t> toPtsList = {0, 200, 600}; // ms
    vector<int32_t> audioVals = {3750, 3750, 3750, 3600, 3600, 3600, 3300, 3300, 3300};
    numbers_ = 0;
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
 * @tc.name: Demuxer_CAF_ReadSample_And_SeekToTime_0032
 * @tc.desc: Read sample test for timed metadata track and seek to the specified time,uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CAF_ReadSample_And_SeekToTime_0032, TestSize.Level1)
{
    InitResource(g_cafUriPath16, URI);
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
    list<int64_t> toPtsList = {0, 200, 600}; // ms
    vector<int32_t> audioVals = {3750, 3750, 3750, 3600, 3600, 3600, 3300, 3300, 3300};
    numbers_ = 0;
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
 * @tc.name: Demuxer_CAF_ReadSample_And_SeekToTime_0033
 * @tc.desc: Read sample test for timed metadata track and seek to the specified time,local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CAF_ReadSample_And_SeekToTime_0033, TestSize.Level1)
{
    InitResource(g_cafFilePath17, LOCAL);
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
    list<int64_t> toPtsList = {0, 200, 600}; // ms
    vector<int32_t> audioVals = {235, 235, 235, 225, 225, 225, 207, 207, 207};
    numbers_ = 0;
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
 * @tc.name: Demuxer_CAF_ReadSample_And_SeekToTime_0034
 * @tc.desc: Read sample test for timed metadata track and seek to the specified time,uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CAF_ReadSample_And_SeekToTime_0034, TestSize.Level1)
{
    InitResource(g_cafUriPath17, URI);
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
    list<int64_t> toPtsList = {0, 200, 600}; // ms
    vector<int32_t> audioVals = {235, 235, 235, 225, 225, 225, 207, 207, 207};
    numbers_ = 0;
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
 * @tc.name: Demuxer_CAF_ReadSample_And_SeekToTime_0035
 * @tc.desc: Read sample test for timed metadata track and seek to the specified time,local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CAF_ReadSample_And_SeekToTime_0035, TestSize.Level1)
{
    InitResource(g_cafFilePath18, LOCAL);
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
    list<int64_t> toPtsList = {0, 200, 600}; // ms
    vector<int32_t> audioVals = {235, 235, 235, 225, 225, 225, 207, 207, 207};
    numbers_ = 0;
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
 * @tc.name: Demuxer_CAF_ReadSample_And_SeekToTime_0036
 * @tc.desc: Read sample test for timed metadata track and seek to the specified time,uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CAF_ReadSample_And_SeekToTime_0036, TestSize.Level1)
{
    InitResource(g_cafUriPath18, URI);
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
    list<int64_t> toPtsList = {0, 200, 600}; // ms
    vector<int32_t> audioVals = {235, 235, 235, 225, 225, 225, 207, 207, 207};
    numbers_ = 0;
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
 * @tc.name: Demuxer_CAF_ReadSample_And_SeekToTime_0037
 * @tc.desc: Read sample test for timed metadata track and seek to the specified time,local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CAF_ReadSample_And_SeekToTime_0037, TestSize.Level1)
{
    InitResource(g_cafFilePath19, LOCAL);
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
    list<int64_t> toPtsList = {0, 200, 600}; // ms
    vector<int32_t> audioVals = {469, 469, 469, 450, 450, 450, 413, 413, 413};
    numbers_ = 0;
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
 * @tc.name: Demuxer_CAF_ReadSample_And_SeekToTime_0038
 * @tc.desc: Read sample test for timed metadata track and seek to the specified time,uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CAF_ReadSample_And_SeekToTime_0038, TestSize.Level1)
{
    InitResource(g_cafUriPath19, URI);
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
    list<int64_t> toPtsList = {0, 200, 600}; // ms
    vector<int32_t> audioVals = {469, 469, 469, 450, 450, 450, 413, 413, 413};
    numbers_ = 0;
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
 * @tc.name: Demuxer_CAF_ReadSample_And_SeekToTime_0039
 * @tc.desc: Read sample test for timed metadata track and seek to the specified time,local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CAF_ReadSample_And_SeekToTime_0039, TestSize.Level1)
{
    InitResource(g_cafFilePath20, LOCAL);
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
    list<int64_t> toPtsList = {0, 200, 600}; // ms
    vector<int32_t> audioVals = {469, 469, 469, 450, 450, 450, 413, 413, 413};
    numbers_ = 0;
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
 * @tc.name: Demuxer_CAF_ReadSample_And_SeekToTime_0040
 * @tc.desc: Read sample test for timed metadata track and seek to the specified time,uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CAF_ReadSample_And_SeekToTime_0040, TestSize.Level1)
{
    InitResource(g_cafUriPath20, URI);
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
    list<int64_t> toPtsList = {0, 200, 600}; // ms
    vector<int32_t> audioVals = {469, 469, 469, 450, 450, 450, 413, 413, 413};
    numbers_ = 0;
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
 * @tc.name: Demuxer_CAF_ReadSample_And_SeekToTime_0041
 * @tc.desc: Read sample test for timed metadata track and seek to the specified time,local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CAF_ReadSample_And_SeekToTime_0041, TestSize.Level1)
{
    InitResource(g_cafFilePath21, LOCAL);
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
    list<int64_t> toPtsList = {0, 200, 600}; // ms
    vector<int32_t> audioVals = {59, 59, 59, 57, 57, 57, 52, 52, 52};
    numbers_ = 0;
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
 * @tc.name: Demuxer_CAF_ReadSample_And_SeekToTime_0042
 * @tc.desc: Read sample test for timed metadata track and seek to the specified time,uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CAF_ReadSample_And_SeekToTime_0042, TestSize.Level1)
{
    InitResource(g_cafUriPath21, URI);
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
    list<int64_t> toPtsList = {0, 200, 600}; // ms
    vector<int32_t> audioVals = {59, 59, 59, 57, 57, 57, 52, 52, 52};
    numbers_ = 0;
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
 * @tc.name: Demuxer_CAF_ReadSample_And_SeekToTime_0045
 * @tc.desc: Read sample test for timed metadata track and seek to the specified time,local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CAF_ReadSample_And_SeekToTime_0045, TestSize.Level1)
{
    InitResource(g_cafFilePath23, LOCAL);
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
    list<int64_t> toPtsList = {0, 200, 600}; // ms
    vector<int32_t> audioVals = {176, 176, 176, 169, 169, 169, 155, 155, 155};
    numbers_ = 0;
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
 * @tc.name: Demuxer_CAF_ReadSample_And_SeekToTime_0046
 * @tc.desc: Read sample test for timed metadata track and seek to the specified time,uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CAF_ReadSample_And_SeekToTime_0046, TestSize.Level1)
{
    InitResource(g_cafUriPath23, URI);
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
    list<int64_t> toPtsList = {0, 200, 600}; // ms
    vector<int32_t> audioVals = {176, 176, 176, 169, 169, 169, 155, 155, 155};
    numbers_ = 0;
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
 * @tc.name: Demuxer_CAF_ReadSample_And_SeekToTime_0047
 * @tc.desc: Read sample test for timed metadata track and seek to the specified time,local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CAF_ReadSample_And_SeekToTime_0047, TestSize.Level1)
{
    InitResource(g_cafFilePath24, LOCAL);
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
    list<int64_t> toPtsList = {0, 200, 600}; // ms
    vector<int32_t> audioVals = {176, 176, 176, 169, 169, 169, 155, 155, 155};
    numbers_ = 0;
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
 * @tc.name: Demuxer_CAF_ReadSample_And_SeekToTime_0048
 * @tc.desc: Read sample test for timed metadata track and seek to the specified time,uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CAF_ReadSample_And_SeekToTime_0048, TestSize.Level1)
{
    InitResource(g_cafUriPath24, URI);
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
    list<int64_t> toPtsList = {0, 200, 600}; // ms
    vector<int32_t> audioVals = {176, 176, 176, 169, 169, 169, 155, 155, 155};
    numbers_ = 0;
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
 * @tc.name: Demuxer_CAF_ReadSample_And_SeekToTime_0049
 * @tc.desc: Read sample test for timed metadata track and seek to the specified time,local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CAF_ReadSample_And_SeekToTime_0049, TestSize.Level1)
{
    InitResource(g_cafFilePath25, LOCAL);
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
    list<int64_t> toPtsList = {0, 200, 600}; // ms
    vector<int32_t> audioVals = {235, 235, 235, 225, 225, 225, 207, 207, 207};
    numbers_ = 0;
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
 * @tc.name: Demuxer_CAF_ReadSample_And_SeekToTime_0050
 * @tc.desc: Read sample test for timed metadata track and seek to the specified time,uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CAF_ReadSample_And_SeekToTime_0050, TestSize.Level1)
{
    InitResource(g_cafUriPath25, URI);
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
    list<int64_t> toPtsList = {0, 200, 600}; // ms
    vector<int32_t> audioVals = {235, 235, 235, 225, 225, 225, 207, 207, 207};
    numbers_ = 0;
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
 * @tc.name: Demuxer_CAF_ReadSample_And_SeekToTime_0051
 * @tc.desc: Read sample test for timed metadata track and seek to the specified time,local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CAF_ReadSample_And_SeekToTime_0051, TestSize.Level1)
{
    InitResource(g_cafFilePath26, LOCAL);
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
    list<int64_t> toPtsList = {0, 200, 600}; // ms
    vector<int32_t> audioVals = {235, 235, 235, 225, 225, 225, 207, 207, 207};
    numbers_ = 0;
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
 * @tc.name: Demuxer_CAF_ReadSample_And_SeekToTime_0052
 * @tc.desc: Read sample test for timed metadata track and seek to the specified time,uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CAF_ReadSample_And_SeekToTime_0052, TestSize.Level1)
{
    InitResource(g_cafUriPath26, URI);
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
    list<int64_t> toPtsList = {0, 200, 600}; // ms
    vector<int32_t> audioVals = {235, 235, 235, 225, 225, 225, 207, 207, 207};
    numbers_ = 0;
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

} //namespace