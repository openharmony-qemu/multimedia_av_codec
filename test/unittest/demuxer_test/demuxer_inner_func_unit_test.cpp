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
#include "demuxer_unit_test.h"

#define LOCAL true

using namespace OHOS;
using namespace OHOS::Media;
using namespace OHOS::MediaAVCodec;
using namespace testing::ext;
using namespace std;

namespace {
const string TEST_FILE_PATH = "/data/test/media/";
// video
string g_mp4Path = TEST_FILE_PATH + string("test_265_B_Gop25_4sec.mp4");
string g_mp4TimeMetaPath = TEST_FILE_PATH + string("timed_metadata_track.mp4");
string g_movPath = TEST_FILE_PATH + string("h264_mp3.mov");
string g_flvPath = TEST_FILE_PATH + string("h264.flv");
string g_aviPath = TEST_FILE_PATH + string("h264_aac.avi");
string g_m4vPath = TEST_FILE_PATH + string("h264_fmp4.m4v");
string g_mkvPath = TEST_FILE_PATH + string("h264_mp3_4sec.mkv");
string g_tsPath = TEST_FILE_PATH + string("hevc_aac_1920x1080_g30_30fps.ts");
string g_mpgPath = TEST_FILE_PATH + string("mpeg_mpeg2_mp3.mpeg");
// audio
string g_m4aPath = TEST_FILE_PATH + string("audio/h264_fmp4.m4a");
string g_apePath = TEST_FILE_PATH + string("ape_test.ape");
string g_wavPath = TEST_FILE_PATH + string("wav_audio_test_202406290859.wav");
string g_oggPath = TEST_FILE_PATH + string("audio/ogg_48000_1.ogg");
string g_aacPath = TEST_FILE_PATH + string("audio/aac_44100_1.aac");
string g_amrPath = TEST_FILE_PATH + string("audio/amr_nb_8000_1.amr");
string g_flacPath = TEST_FILE_PATH + string("audio/flac_48000_1_cover.flac");
// subtitle
string g_vttPath = TEST_FILE_PATH + string("webvtt_test.vtt");

// cache check
const double CACHE_LIMIT_RANGE_MAX = 1.1; // +10%
const double CACHE_LIMIT_RANGE_MIN = 0.9; // -10%
std::vector<std::vector<int32_t>> g_singleTrackCheckSteps = {
    {0, 1,  0, 0, true}, // read track 0 1 time, check track 0
    {0, 10, 0, 0, true}, // read track 0 10 times, check track 0
    {1, 20, 0, 0, false}, // read track 0 20 times, check track 0
    {1, -1, 0, 0, false}, // read to end
};
} // namespace

bool TrackIsValid(uint32_t trackId, std::vector<uint32_t> selectedTracks)
{
    return std::any_of(selectedTracks.begin(), selectedTracks.end(), [trackId](uint32_t id) { return id == trackId; });
}

bool DemuxerUnitTest::CheckCache(
    uint32_t readTrackId, uint32_t times, uint32_t checkTrackId, uint32_t expect, bool exist)
{
    printf("read %d %d times, check %d, expect %d, exist %d\n", readTrackId, times, checkTrackId, expect, exist);
    if (demuxer_ == nullptr) {
        printf("demuxer is nullptr\n");
        return false;
    }
    int step = 0;
    int32_t ret = AV_ERR_OK;
    while (TrackIsValid(readTrackId, selectedTrackIds_) && (times == -1 || step < times)) {
        ret = demuxer_->ReadSample(readTrackId, sharedMem_, &info_, flag_);
        if (ret != AV_ERR_OK) {
            printf("read failed\n");
            return false;
        }
        if (flag_ & AVCodecBufferFlag::AVCODEC_BUFFER_FLAG_EOS) {
            break;
        }
        step++;
    }
    uint32_t cacheSize = 0;
    ret = demuxer_->GetCurrentCacheSize(checkTrackId, cacheSize);
    if (ret != static_cast<int32_t>(Status::OK)) {
        printf("No track\n");
        return !exist;
    }
    if (cacheSize != expect) {
        if (static_cast<double>(cacheSize) < CACHE_LIMIT_RANGE_MIN * static_cast<double>(expect) ||
            static_cast<double>(cacheSize) > CACHE_LIMIT_RANGE_MAX * static_cast<double>(expect)) {
            printf("cacheSize error\n");
            return false;
        } else {
            printf("not equal, in range limit\n");
        }
    }
    printf("Check pass\n");
    return true;
}
/**********************************demuxer inner func**************************************/
namespace {
#ifdef DEMUXER_INNER_UNIT_TEST
/**
 * @tc.name: Demuxer_GetCurrentCacheSize_0001
 * @tc.desc: Clear unselect track cache, push when initing
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_GetCurrentCacheSize_0001, TestSize.Level1)
{
    InitResource(g_movPath, LOCAL);
    ASSERT_TRUE(initStatus_);
    ASSERT_TRUE(SetInitValue());
    selectedTrackIds_.erase(selectedTrackIds_.begin());
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    int32_t ret = demuxer_->ReadSample(1, sharedMem_, &info_, flag_);
    ASSERT_EQ(ret, AV_ERR_OK);
    uint32_t cacheSize = 0;
    ret = demuxer_->GetCurrentCacheSize(0, cacheSize);
    ASSERT_EQ(ret, AV_ERR_OK);
    ASSERT_EQ(cacheSize, 0);
    ret = demuxer_->GetCurrentCacheSize(1, cacheSize);
    ASSERT_EQ(ret, AV_ERR_OK);
    ASSERT_EQ(cacheSize, 0);
}

/**
 * @tc.name: Demuxer_GetCurrentCacheSize_1000
 * @tc.desc: GetCurrentCacheSize when reading
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_GetCurrentCacheSize_1000, TestSize.Level1)
{
    InitResource(g_mp4Path, LOCAL);
    ASSERT_TRUE(initStatus_);
    ASSERT_TRUE(SetInitValue());
    uint32_t cacheSize = 0;
    ret_ = demuxer_->GetCurrentCacheSize(1, cacheSize);
    ASSERT_EQ(ret_, AVCS_ERR_INVALID_OPERATION);
    ret_ = demuxer_->SelectTrackByID(0);
    ASSERT_EQ(ret_, AV_ERR_OK);
    ret_ = demuxer_->GetCurrentCacheSize(1, cacheSize);
    ASSERT_EQ(ret_, AVCS_ERR_INVALID_VAL);
}

/**
 * @tc.name: Demuxer_GetCurrentCacheSize_1001
 * @tc.desc: GetCurrentCacheSize when reading, mp4
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_GetCurrentCacheSize_1001, TestSize.Level1)
{
    InitResource(g_mp4Path, LOCAL);
    ASSERT_TRUE(initStatus_);
    ASSERT_TRUE(SetInitValue());
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {1, 1,  0, 156165}, // read track 1 1 time, check track 0
        {1, 10, 0, 186845}, // read track 1 10 times, check track 0
        {0, 10, 1, 1534}, // read track 0 10 times, check track 1
        {0, -1, 0, 0}, // read to end
        {1, -1, 1, 0}, // read to end
    };
    for (auto step : cacheCheckSteps) {
        ASSERT_TRUE(CheckCache(step[0], step[1], step[2], step[3]));
    }
}

/**
 * @tc.name: Demuxer_GetCurrentCacheSize_1002
 * @tc.desc: GetCurrentCacheSize when reading, mp4+timedmeta
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_GetCurrentCacheSize_1002, TestSize.Level1)
{
    InitResource(g_mp4TimeMetaPath, LOCAL);
    ASSERT_TRUE(initStatus_);
    ASSERT_TRUE(SetInitValue());
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {1, 1,  0, 138037}, // read track 1 1 time, check track 0
        {1, 10, 0, 138037}, // read track 1 10 times, check track 0
        {0, 10, 1, 0}, // read track 0 10 times, check track 1
        {0, -1, 0, 0}, // read to end
        {1, -1, 1, 0}, // read to end
    };
    for (auto step : cacheCheckSteps) {
        ASSERT_TRUE(CheckCache(step[0], step[1], step[2], step[3]));
    }
}

/**
 * @tc.name: Demuxer_GetCurrentCacheSize_1003
 * @tc.desc: GetCurrentCacheSize when reading, mov
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_GetCurrentCacheSize_1003, TestSize.Level1)
{
    InitResource(g_movPath, LOCAL);
    ASSERT_TRUE(initStatus_);
    ASSERT_TRUE(SetInitValue());
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {1, 1,  0, 45886}, // read track 1 1 time, check track 0
        {1, 10, 0, 59158}, // read track 1 10 times, check track 0
        {0, 10, 1, 0}, // read track 0 10 times, check track 1
        {0, -1, 0, 0}, // read to end
        {1, -1, 1, 0}, // read to end
    };
    for (auto step : cacheCheckSteps) {
        ASSERT_TRUE(CheckCache(step[0], step[1], step[2], step[3]));
    }
}

/**
 * @tc.name: Demuxer_GetCurrentCacheSize_1004
 * @tc.desc: GetCurrentCacheSize when reading, flv
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_GetCurrentCacheSize_1004, TestSize.Level1)
{
    InitResource(g_flvPath, LOCAL);
    ASSERT_TRUE(initStatus_);
    ASSERT_TRUE(SetInitValue());
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {1, 1,  0, 339258}, // read track 1 1 time, check track 0
        {1, 10, 0, 539571}, // read track 1 10 times, check track 0
        {0, 10, 1, 2688}, // read track 0 10 times, check track 1
        {0, -1, 0, 0}, // read to end
        {1, -1, 1, 0}, // read to end
    };
    for (auto step : cacheCheckSteps) {
        ASSERT_TRUE(CheckCache(step[0], step[1], step[2], step[3]));
    }
}

/**
 * @tc.name: Demuxer_GetCurrentCacheSize_1005
 * @tc.desc: GetCurrentCacheSize when reading, avi
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_GetCurrentCacheSize_1005, TestSize.Level1)
{
    InitResource(g_aviPath, LOCAL);
    ASSERT_TRUE(initStatus_);
    ASSERT_TRUE(SetInitValue());
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {1, 1,  0, 46073}, // read track 1 1 time, check track 0
        {1, 10, 0, 48066}, // read track 1 10 times, check track 0
        {0, 10, 1, 7906}, // read track 0 10 times, check track 1
        {0, -1, 0, 0}, // read to end
        {1, -1, 1, 0}, // read to end
    };
    for (auto step : cacheCheckSteps) {
        ASSERT_TRUE(CheckCache(step[0], step[1], step[2], step[3]));
    }
}

/**
 * @tc.name: Demuxer_GetCurrentCacheSize_1006
 * @tc.desc: GetCurrentCacheSize when reading, m4v, single track
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_GetCurrentCacheSize_1006, TestSize.Level1)
{
    InitResource(g_m4vPath, LOCAL);
    ASSERT_TRUE(initStatus_);
    ASSERT_TRUE(SetInitValue());
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {0, 1,  1, 0, false},
        {0, 10, 1, 0, false},
        {0, -1, 0, 0, true}, // read to end
    };
    for (auto step : cacheCheckSteps) {
        ASSERT_TRUE(CheckCache(step[0], step[1], step[2], step[3], step[4]));
    }
}

/**
 * @tc.name: Demuxer_GetCurrentCacheSize_1007
 * @tc.desc: GetCurrentCacheSize when reading, mkv
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_GetCurrentCacheSize_1007, TestSize.Level1)
{
    InitResource(g_mkvPath, LOCAL);
    ASSERT_TRUE(initStatus_);
    ASSERT_TRUE(SetInitValue());
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {1, 1,  0, 20464}, // read track 1 1 time, check track 0
        {1, 10, 0, 171744}, // read track 1 10 times, check track 0
        {0, 10, 1, 0}, // read track 0 10 times, check track 1
        {0, -1, 0, 0}, // read to end
        {1, -1, 1, 0}, // read to end
    };
    for (auto step : cacheCheckSteps) {
        ASSERT_TRUE(CheckCache(step[0], step[1], step[2], step[3]));
    }
}

/**
 * @tc.name: Demuxer_GetCurrentCacheSize_1008
 * @tc.desc: GetCurrentCacheSize when reading, ts
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_GetCurrentCacheSize_1008, TestSize.Level1)
{
    InitResource(g_tsPath, LOCAL);
    ASSERT_TRUE(initStatus_);
    ASSERT_TRUE(SetInitValue());
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {1, 1,  0, 57407}, // read track 1 1 time, check track 0
        {1, 10, 0, 57407}, // read track 1 10 times, check track 0
        {0, 10, 1, 804}, // read track 0 10 times, check track 1
        {0, -1, 0, 0}, // read to end
        {1, -1, 1, 0}, // read to end
    };
    for (auto step : cacheCheckSteps) {
        ASSERT_TRUE(CheckCache(step[0], step[1], step[2], step[3]));
    }
}

/**
 * @tc.name: Demuxer_GetCurrentCacheSize_1009
 * @tc.desc: GetCurrentCacheSize when reading, mpg
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_GetCurrentCacheSize_1009, TestSize.Level1)
{
    InitResource(g_mpgPath, LOCAL);
    ASSERT_TRUE(initStatus_);
    ASSERT_TRUE(SetInitValue());
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {1, 1,  0, 0}, // read track 1 1 time, check track 0
        {1, 10, 0, 225849}, // read track 1 10 times, check track 0
        {0, 10, 1, 0}, // read track 0 10 times, check track 1
        {0, -1, 0, 0}, // read to end
        {1, -1, 1, 0}, // read to end
    };
    for (auto step : cacheCheckSteps) {
        ASSERT_TRUE(CheckCache(step[0], step[1], step[2], step[3]));
    }
}

/**
 * @tc.name: Demuxer_GetCurrentCacheSize_2001
 * @tc.desc: GetCurrentCacheSize when reading, m4a
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_GetCurrentCacheSize_2001, TestSize.Level1)
{
    InitResource(g_m4aPath, LOCAL);
    ASSERT_TRUE(initStatus_);
    ASSERT_TRUE(SetInitValue());
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    for (auto step : g_singleTrackCheckSteps) {
        ASSERT_TRUE(CheckCache(step[0], step[1], step[2], step[3], step[4]));
    }
}

/**
 * @tc.name: Demuxer_GetCurrentCacheSize_2002
 * @tc.desc: GetCurrentCacheSize when reading, ape
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_GetCurrentCacheSize_2002, TestSize.Level1)
{
    InitResource(g_apePath, LOCAL);
    ASSERT_TRUE(initStatus_);
    ASSERT_TRUE(SetInitValue());
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    for (auto step : g_singleTrackCheckSteps) {
        ASSERT_TRUE(CheckCache(step[0], step[1], step[2], step[3], step[4]));
    }
}

/**
 * @tc.name: Demuxer_GetCurrentCacheSize_2003
 * @tc.desc: GetCurrentCacheSize when reading, wav
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_GetCurrentCacheSize_2003, TestSize.Level1)
{
    InitResource(g_wavPath, LOCAL);
    ASSERT_TRUE(initStatus_);
    ASSERT_TRUE(SetInitValue());
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    for (auto step : g_singleTrackCheckSteps) {
        ASSERT_TRUE(CheckCache(step[0], step[1], step[2], step[3], step[4]));
    }
}

/**
 * @tc.name: Demuxer_GetCurrentCacheSize_2004
 * @tc.desc: GetCurrentCacheSize when reading, ogg
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_GetCurrentCacheSize_2004, TestSize.Level1)
{
    InitResource(g_oggPath, LOCAL);
    ASSERT_TRUE(initStatus_);
    ASSERT_TRUE(SetInitValue());
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    for (auto step : g_singleTrackCheckSteps) {
        ASSERT_TRUE(CheckCache(step[0], step[1], step[2], step[3], step[4]));
    }
}

/**
 * @tc.name: Demuxer_GetCurrentCacheSize_2005
 * @tc.desc: GetCurrentCacheSize when reading, aac
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_GetCurrentCacheSize_2005, TestSize.Level1)
{
    InitResource(g_aacPath, LOCAL);
    ASSERT_TRUE(initStatus_);
    ASSERT_TRUE(SetInitValue());
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    for (auto step : g_singleTrackCheckSteps) {
        ASSERT_TRUE(CheckCache(step[0], step[1], step[2], step[3], step[4]));
    }
}

/**
 * @tc.name: Demuxer_GetCurrentCacheSize_2006
 * @tc.desc: GetCurrentCacheSize when reading, amr
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_GetCurrentCacheSize_2006, TestSize.Level1)
{
    InitResource(g_amrPath, LOCAL);
    ASSERT_TRUE(initStatus_);
    ASSERT_TRUE(SetInitValue());
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    for (auto step : g_singleTrackCheckSteps) {
        ASSERT_TRUE(CheckCache(step[0], step[1], step[2], step[3], step[4]));
    }
}

/**
 * @tc.name: Demuxer_GetCurrentCacheSize_2007
 * @tc.desc: GetCurrentCacheSize when reading, flac
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_GetCurrentCacheSize_2007, TestSize.Level1)
{
    InitResource(g_flacPath, LOCAL);
    ASSERT_TRUE(initStatus_);
    ASSERT_TRUE(SetInitValue());
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    for (auto step : g_singleTrackCheckSteps) {
        ASSERT_TRUE(CheckCache(step[0], step[1], step[2], step[3], step[4]));
    }
}

/**
 * @tc.name: Demuxer_GetCurrentCacheSize_3001
 * @tc.desc: GetCurrentCacheSize when reading, vtt
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_GetCurrentCacheSize_3001, TestSize.Level1)
{
    InitResource(g_vttPath, LOCAL);
    ASSERT_TRUE(initStatus_);
    ASSERT_TRUE(SetInitValue());
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    for (auto step : g_singleTrackCheckSteps) {
        ASSERT_TRUE(CheckCache(step[0], step[1], step[2], step[3], step[4]));
    }
}
#endif
} // namespace
