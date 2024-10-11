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

using namespace OHOS;
using namespace OHOS::Media;
using namespace OHOS::MediaAVCodec;
using namespace testing::ext;
using namespace std;

namespace {
unique_ptr<FileServerDemo> server = nullptr;
static const string TEST_FILE_PATH = "/data/test/media/";
static const string TEST_URI_PATH = "http://127.0.0.1:46666/";
int32_t g_width = 3840;
int32_t g_height = 2160;
list<SeekMode> seekModes = {SeekMode::SEEK_NEXT_SYNC, SeekMode::SEEK_PREVIOUS_SYNC,
    SeekMode::SEEK_CLOSEST_SYNC};

string g_mp4Path = TEST_FILE_PATH + string("test_264_B_Gop25_4sec_cover.mp4");
string g_mp4Path2 = TEST_FILE_PATH + string("test_mpeg2_B_Gop25_4sec.mp4");
string g_mp4Path4 = TEST_FILE_PATH + string("zero_track.mp4");
string g_mp4Path5 = TEST_FILE_PATH + string("timed_metadata_track.mp4");
string g_mkvPath2 = TEST_FILE_PATH + string("h264_opus_4sec.mkv");
string g_tsPath = TEST_FILE_PATH + string("test_mpeg2_Gop25_4sec.ts");
string g_aacPath = TEST_FILE_PATH + string("audio/aac_44100_1.aac");
string g_flacPath = TEST_FILE_PATH + string("audio/flac_48000_1_cover.flac");
string g_m4aPath = TEST_FILE_PATH + string("audio/m4a_48000_1.m4a");
string g_mp3Path = TEST_FILE_PATH + string("audio/mp3_48000_1_cover.mp3");
string g_oggPath = TEST_FILE_PATH + string("audio/ogg_48000_1.ogg");
string g_wavPath = TEST_FILE_PATH + string("audio/wav_48000_1.wav");
string g_amrPath = TEST_FILE_PATH + string("audio/amr_nb_8000_1.amr");
string g_amrPath2 = TEST_FILE_PATH + string("audio/amr_wb_16000_1.amr");
string g_audioVividPath = TEST_FILE_PATH + string("2obj_44100Hz_16bit_32k.mp4");
string g_audioVividPath2 = TEST_FILE_PATH + string("2obj_44100Hz_16bit_32k.ts");
string g_multiSoundTrackMp4Path = TEST_FILE_PATH + string("avcc_aac_mp3.mp4");
string g_flvPath = TEST_FILE_PATH + string("h264.flv");
string g_apePath = TEST_FILE_PATH + string("ape_test.ape");
string g_fmp4AvcPath = TEST_FILE_PATH + string("h264_fmp4.mp4");
string g_fmp4m4vPath = TEST_FILE_PATH + string("h264_fmp4.m4v");
string g_fmp4m4aPath = TEST_FILE_PATH + string("audio/h264_fmp4.m4a");
string g_srt = TEST_FILE_PATH + string("subtitle.srt");
string g_drmSm4cPath = TEST_FILE_PATH + string("drm/sm4c.ts");
string g_vttPath = TEST_FILE_PATH + string("webvtt_test.vtt");
string g_vttPath2 = TEST_FILE_PATH + string("webvtt_test2.vtt");
string g_ptsConversionPath = TEST_FILE_PATH + string("camera_info_parser.mp4");
string g_mp4VvcPath = TEST_FILE_PATH + string("vvc.mp4");
} // namespace

void DemuxerUnitTest::SetUpTestCase(void)
{
    server = make_unique<FileServerDemo>();
    server->StartServer();
    cout << "start" << endl;
}

void DemuxerUnitTest::TearDownTestCase(void)
{
    server->StopServer();
}

void DemuxerUnitTest::SetUp(void)
{
    bufferSize_ = g_width * g_height;
}

void DemuxerUnitTest::TearDown(void)
{
    if (source_ != nullptr) {
        source_->Destroy();
        source_ = nullptr;
    }
    if (demuxer_ != nullptr) {
        demuxer_->Destroy();
        demuxer_ = nullptr;
    }
    if (format_ != nullptr) {
        format_->Destroy();
        format_ = nullptr;
    }
    if (sharedMem_ != nullptr) {
        sharedMem_->Destory();
        sharedMem_ = nullptr;
    }
    if (fd_ > 0) {
        close(fd_);
        fd_ = -1;
    }
    bufferSize_ = 0;
    nbStreams_ = 0;
    numbers_ = 0;
    ret_ = AV_ERR_OK;
    info_.presentationTimeUs = 0;
    info_.offset = 0;
    info_.size = 0;
    selectedTrackIds_.clear();
}

int64_t DemuxerUnitTest::GetFileSize(const string &fileName)
{
    int64_t fileSize = 0;
    if (!fileName.empty()) {
        struct stat fileStatus {};
        if (stat(fileName.c_str(), &fileStatus) == 0) {
            fileSize = static_cast<int64_t>(fileStatus.st_size);
        }
    }
    return fileSize;
}

int32_t DemuxerUnitTest::OpenFile(const string &fileName)
{
    int32_t fd = open(fileName.c_str(), O_RDONLY);
    return fd;
}

bool DemuxerUnitTest::isEOS(map<uint32_t, bool>& countFlag)
{
    for (auto iter = countFlag.begin(); iter != countFlag.end(); ++iter) {
        if (!iter->second) {
            return false;
        }
    }
    return true;
}

void DemuxerUnitTest::SetInitValue()
{
    for (int i = 0; i < nbStreams_; i++) {
        string codecMime = "";
        int32_t trackType = -1;
        if (format_ != nullptr) {
            format_->Destroy();
            format_ = nullptr;
        }
        format_ = source_->GetTrackFormat(i);
        format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, codecMime);
        format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, trackType);
        if (codecMime.find("image/") != std::string::npos) {
            continue;
        }
        if (trackType == MediaType::MEDIA_TYPE_VID || trackType == MediaType::MEDIA_TYPE_AUD ||
            trackType == MediaType::MEDIA_TYPE_SUBTITLE || trackType == MediaType::MEDIA_TYPE_TIMED_METADATA) {
            selectedTrackIds_.push_back(static_cast<uint32_t>(i));
            frames_[i] = 0;
            keyFrames_[i] = 0;
            eosFlag_[i] = false;
        }
    }
}

void DemuxerUnitTest::RemoveValue()
{
    if (!frames_.empty()) {
        frames_.clear();
    }
    if (!keyFrames_.empty()) {
        keyFrames_.clear();
    }
    if (!eosFlag_.empty()) {
        eosFlag_.clear();
    }
}

void DemuxerUnitTest::SetEosValue()
{
    for (int i = 0; i < nbStreams_; i++) {
        eosFlag_[i] = true;
    }
}

void DemuxerUnitTest::CountFrames(uint32_t index)
{
    if (flag_ & AVCodecBufferFlag::AVCODEC_BUFFER_FLAG_EOS) {
        eosFlag_[index] = true;
        return;
    }

    if (flag_ & AVCodecBufferFlag::AVCODEC_BUFFER_FLAG_SYNC_FRAME) {
        keyFrames_[index]++;
        frames_[index]++;
    } else if ((flag_ & AVCodecBufferFlag::AVCODEC_BUFFER_FLAG_NONE) == AVCodecBufferFlag::AVCODEC_BUFFER_FLAG_NONE) {
        frames_[index]++;
    } else {
        SetEosValue();
        printf("flag is unknown, read sample break");
    }
}

void DemuxerUnitTest::ReadData()
{
    SetInitValue();
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            demuxer_->ReadSample(idx, sharedMem_, &info_, flag_);
            if (ret_ != AV_ERR_OK) {
                break;
            }
            CountFrames(idx);
        }
        if (ret_ != AV_ERR_OK) {
            break;
        }
    }
}

void DemuxerUnitTest::ReadData(int readNum, int64_t &seekTime)
{
    int num = 0;
    SetInitValue();
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            ret_ = demuxer_->ReadSample(idx, sharedMem_, &info_, flag_);
            if (ret_ != AV_ERR_OK) {
                break;
            }
            CountFrames(idx);
        }
        if (ret_ != AV_ERR_OK) {
            break;
        }
        if (num == readNum) {
            seekTime = info_.presentationTimeUs;
            break;
        }
    }
}

/**********************************demuxer fd**************************************/
namespace {
/**
 * @tc.name: Demuxer_CreateDemuxer_1000
 * @tc.desc: create demuxer
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CreateDemuxer_1000, TestSize.Level1)
{
    InitResource(g_mp4Path, LOCAL);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    ASSERT_NE(demuxer_->SelectTrackByID(3), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_NE(demuxer_->SelectTrackByID(-1), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(1), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(3), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(-1), AV_ERR_OK);
}

/**
 * @tc.name: Demuxer_CreateDemuxer_1010
 * @tc.desc: create demuxer
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CreateDemuxer_1010, TestSize.Level1)
{
    source_ = nullptr;
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
    ASSERT_EQ(demuxer_, nullptr);
    demuxer_ = nullptr;
}

/**
 * @tc.name: Demuxer_CreateDemuxer_1020
 * @tc.desc: repeatedly create demuxer
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CreateDemuxer_1020, TestSize.Level1)
{
    InitResource(g_mp4Path, LOCAL);
    ASSERT_NE(demuxer_, nullptr);
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
    ASSERT_NE(demuxer_, nullptr);
}

/**
 * @tc.name: Demuxer_CreateDemuxer_1030
 * @tc.desc: create demuxer
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CreateDemuxer_1030, TestSize.Level1)
{
    InitResource(g_tsPath, LOCAL);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    ASSERT_NE(demuxer_->SelectTrackByID(3), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_NE(demuxer_->SelectTrackByID(-1), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(1), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(3), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(-1), AV_ERR_OK);
}

/**
 * @tc.name: Demuxer_CreateDemuxer_1040
 * @tc.desc: create demuxer
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CreateDemuxer_1040, TestSize.Level1)
{
    InitResource(g_mp4Path4, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(demuxer_, nullptr);
}

/**
 * @tc.name: Demuxer_UnselectTrackByID_1000
 * @tc.desc: select and remove track by ID
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_UnselectTrackByID_1000, TestSize.Level1)
{
    InitResource(g_mp4Path4, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_NE(demuxer_->SelectTrackByID(0), AV_ERR_OK);
}

/**
 * @tc.name: Demuxer_UnselectTrackByID_1010
 * @tc.desc: select and remove track by ID
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_UnselectTrackByID_1010, TestSize.Level1)
{
    InitResource(g_aacPath, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_NE(demuxer_->SelectTrackByID(3), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_NE(demuxer_->SelectTrackByID(-1), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(1), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(3), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(-1), AV_ERR_OK);
}

/**
 * @tc.name: Demuxer_UnselectTrackByID_1020
 * @tc.desc: select and remove track by ID
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_UnselectTrackByID_1020, TestSize.Level1)
{
    InitResource(g_flacPath, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_NE(demuxer_->SelectTrackByID(3), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_NE(demuxer_->SelectTrackByID(-1), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(1), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(3), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(-1), AV_ERR_OK);
}

/**
 * @tc.name: Demuxer_UnselectTrackByID_1030
 * @tc.desc: select and remove track by ID
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_UnselectTrackByID_1030, TestSize.Level1)
{
    InitResource(g_m4aPath, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_NE(demuxer_->SelectTrackByID(3), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_NE(demuxer_->SelectTrackByID(-1), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(1), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(3), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(-1), AV_ERR_OK);
}

/**
 * @tc.name: Demuxer_UnselectTrackByID_1060
 * @tc.desc: select and remove track by ID
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_UnselectTrackByID_1060, TestSize.Level1)
{
    InitResource(g_mp3Path, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_NE(demuxer_->SelectTrackByID(3), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_NE(demuxer_->SelectTrackByID(-1), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(1), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(3), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(-1), AV_ERR_OK);
}

/**
 * @tc.name: Demuxer_UnselectTrackByID_1070
 * @tc.desc: select and remove track by ID
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_UnselectTrackByID_1070, TestSize.Level1)
{
    InitResource(g_oggPath, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_NE(demuxer_->SelectTrackByID(3), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_NE(demuxer_->SelectTrackByID(-1), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(1), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(3), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(-1), AV_ERR_OK);
}

/**
 * @tc.name: Demuxer_UnselectTrackByID_1080
 * @tc.desc: select and remove track by ID
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_UnselectTrackByID_1080, TestSize.Level1)
{
    InitResource(g_wavPath, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_NE(demuxer_->SelectTrackByID(3), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_NE(demuxer_->SelectTrackByID(-1), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(1), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(3), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(-1), AV_ERR_OK);
}

/**
 * @tc.name: Demuxer_UnselectTrackByID_1090
 * @tc.desc: select and remove track by ID
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_UnselectTrackByID_1090, TestSize.Level1)
{
    InitResource(g_mkvPath2, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_NE(demuxer_->SelectTrackByID(3), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_NE(demuxer_->SelectTrackByID(-1), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(1), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(3), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(-1), AV_ERR_OK);
}

/**
 * @tc.name: Demuxer_UnselectTrackByID_1100
 * @tc.desc: select and remove track by ID
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_UnselectTrackByID_1100, TestSize.Level1)
{
    InitResource(g_amrPath, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_NE(demuxer_->SelectTrackByID(3), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_NE(demuxer_->SelectTrackByID(-1), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(1), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(3), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(-1), AV_ERR_OK);
}

/**
 * @tc.name: Demuxer_ReadSample_1000
 * @tc.desc: copy current sample to buffer
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_1000, TestSize.Level1)
{
    InitResource(g_mp4Path, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(format_, nullptr);
    ASSERT_NE(demuxer_, nullptr);
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
    ASSERT_EQ(keyFrames_[0], 5);
    ASSERT_EQ(keyFrames_[1], 174);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_1050
 * @tc.desc: Read sample test for timed metadata track
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_1050, TestSize.Level1)
{
    InitResource(g_mp4Path5, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(format_, nullptr);
    ASSERT_NE(demuxer_, nullptr);
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
    ASSERT_EQ(frames_[0], 601);
    ASSERT_EQ(frames_[1], 601);
    ASSERT_EQ(keyFrames_[0], 3);
    ASSERT_EQ(keyFrames_[1], 601);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_1010
 * @tc.desc: copy current sample to buffer
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_1010, TestSize.Level1)
{
    InitResource(g_mp4Path, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    uint32_t idx = 4;
    ASSERT_NE(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
}

/**
 * @tc.name: Demuxer_ReadSample_1020
 * @tc.desc: copy current sample to buffer
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_1020, TestSize.Level1)
{
    InitResource(g_mp4Path, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    uint32_t idx = -1;
    ASSERT_NE(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
}

/**
 * @tc.name: Demuxer_ReadSample_1030
 * @tc.desc: copy current sample to buffer(only video track)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_1030, TestSize.Level1)
{
    InitResource(g_mp4Path, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    int32_t vkeyFrames = 0;
    int32_t vframes = 0;
    flag_ = AVCodecBufferFlag::AVCODEC_BUFFER_FLAG_NONE;
    while (!(flag_ & AVCodecBufferFlag::AVCODEC_BUFFER_FLAG_EOS)) {
        ASSERT_EQ(demuxer_->ReadSample(0, sharedMem_, &info_, flag_), AV_ERR_OK);
        if (flag_ & AVCodecBufferFlag::AVCODEC_BUFFER_FLAG_EOS) {
            break;
        }
        if (flag_ & AVCodecBufferFlag::AVCODEC_BUFFER_FLAG_SYNC_FRAME) {
            vkeyFrames++;
            vframes++;
        } else if ((flag_ & AVCodecBufferFlag::AVCODEC_BUFFER_FLAG_NONE) == 0) {
            vframes++;
        }
    }
    printf("vframes=%d | vkFrames=%d\n", vframes, vkeyFrames);
    ASSERT_EQ(vframes, 103);
    ASSERT_EQ(vkeyFrames, 5);
}

/**
 * @tc.name: Demuxer_ReadSample_1040
 * @tc.desc: copy current sample to buffer
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_1040, TestSize.Level1)
{
    InitResource(g_mp4Path2, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(format_, nullptr);
    ASSERT_NE(demuxer_, nullptr);
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
    ASSERT_EQ(keyFrames_[0], 5);
    ASSERT_EQ(keyFrames_[1], 174);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_1070
 * @tc.desc: copy current sample to buffer
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_1070, TestSize.Level1)
{
    InitResource(g_mkvPath2, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(format_, nullptr);
    ASSERT_NE(demuxer_, nullptr);
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
    ASSERT_EQ(frames_[0], 240);
    ASSERT_EQ(frames_[1], 199);
    ASSERT_EQ(keyFrames_[0], 4);
    ASSERT_EQ(keyFrames_[1], 199);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_1090
 * @tc.desc: copy current sample to buffer
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_1090, TestSize.Level1)
{
    InitResource(g_tsPath, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(format_, nullptr);
    ASSERT_NE(demuxer_, nullptr);
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
    ASSERT_EQ(keyFrames_[0], 5);
    ASSERT_EQ(keyFrames_[1], 174);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_1100
 * @tc.desc: copy current sample to buffer
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_1100, TestSize.Level1)
{
    InitResource(g_aacPath, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(format_, nullptr);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    SetInitValue();
    uint32_t idx = 0;
    while (!isEOS(eosFlag_)) {
        ASSERT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
        CountFrames(idx);
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    ASSERT_EQ(frames_[0], 1293);
    ASSERT_EQ(keyFrames_[0], 1293);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_1110
 * @tc.desc: copy current sample to buffer
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_1110, TestSize.Level1)
{
    InitResource(g_flacPath, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(format_, nullptr);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    SetInitValue();
    uint32_t idx = 0;
    while (!isEOS(eosFlag_)) {
        ASSERT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
        CountFrames(idx);
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    ASSERT_EQ(frames_[0], 313);
    ASSERT_EQ(keyFrames_[0], 313);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_1120
 * @tc.desc: copy current sample to buffer
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_1120, TestSize.Level1)
{
    InitResource(g_m4aPath, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(format_, nullptr);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    SetInitValue();
    uint32_t idx = 0;
    while (!isEOS(eosFlag_)) {
        ASSERT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
        CountFrames(idx);
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    ASSERT_EQ(frames_[0], 1408);
    ASSERT_EQ(keyFrames_[0], 1408);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_1130
 * @tc.desc: copy current sample to buffer
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_1130, TestSize.Level1)
{
    InitResource(g_mp3Path, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(format_, nullptr);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    SetInitValue();
    uint32_t idx = 0;
    while (!isEOS(eosFlag_)) {
        ASSERT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
        CountFrames(idx);
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    ASSERT_EQ(frames_[0], 1251);
    ASSERT_EQ(keyFrames_[0], 1251);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_1140
 * @tc.desc: copy current sample to buffer
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_1140, TestSize.Level1)
{
    InitResource(g_oggPath, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(format_, nullptr);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    SetInitValue();
    uint32_t idx = 0;
    while (!isEOS(eosFlag_)) {
        ASSERT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
        CountFrames(idx);
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    ASSERT_EQ(frames_[0], 1598);
    ASSERT_EQ(keyFrames_[0], 1598);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_1150
 * @tc.desc: copy current sample to buffer
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_1150, TestSize.Level1)
{
    InitResource(g_wavPath, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(format_, nullptr);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    SetInitValue();
    uint32_t idx = 0;
    while (!isEOS(eosFlag_)) {
        ASSERT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
        CountFrames(idx);
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    ASSERT_EQ(frames_[0], 704);
    ASSERT_EQ(keyFrames_[0], 704);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_1160
 * @tc.desc: copy current sample to buffer
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_1160, TestSize.Level1)
{
    InitResource(g_amrPath, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(format_, nullptr);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    SetInitValue();
    uint32_t idx = 0;
    while (!isEOS(eosFlag_)) {
        ASSERT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
        CountFrames(idx);
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    ASSERT_EQ(frames_[0], 1501);
    ASSERT_EQ(keyFrames_[0], 1501);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_1170
 * @tc.desc: copy current sample to buffer
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_1170, TestSize.Level1)
{
    InitResource(g_amrPath2, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(format_, nullptr);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    SetInitValue();
    uint32_t idx = 0;
    while (!isEOS(eosFlag_)) {
        ASSERT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
        CountFrames(idx);
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    ASSERT_EQ(frames_[0], 1500);
    ASSERT_EQ(keyFrames_[0], 1500);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_1180
 * @tc.desc: copy current sample to buffer(av3a mp4)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_1180, TestSize.Level1)
{
    InitResource(g_audioVividPath, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(format_, nullptr);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    SetInitValue();
    uint32_t idx = 0;
    while (!isEOS(eosFlag_)) {
        ASSERT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
        CountFrames(idx);
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    ASSERT_EQ(frames_[0], 1380);
    ASSERT_EQ(keyFrames_[0], 1380);
    RemoveValue();
}


/**
 * @tc.name: Demuxer_SeekToTime_1000
 * @tc.desc: seek to the specified time
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_1000, TestSize.Level1)
{
    InitResource(g_mp4Path, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(format_, nullptr);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    list<int64_t> toPtsList = {0, 2000, 1920, 2160, 2200, 2440, 2600, 2700, 4080, 4100}; // ms
    vector<int32_t> audioVals = {174, 174, 174, 90, 91, 91, 90, 134, 90, 47, 91, 91, 47, 91, 91, 47, 91, 47, 47, 91, 47,
        47, 91, 47, 5, 5, 5, 5};
    vector<int32_t> videoVals = {103, 103, 103, 53, 53, 53, 53, 78, 53, 28, 53, 53, 28, 53, 53, 28, 53, 28, 28, 53, 28,
        28, 53, 28, 3, 3, 3, 3};
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
 * @tc.name: Demuxer_SeekToTime_1001
 * @tc.desc: seek to the specified time
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_1001, TestSize.Level1)
{
    InitResource(g_mp4Path, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(format_, nullptr);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    list<int64_t> toPtsList = {-100, 1000000}; // ms
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        ret_ = demuxer_->SeekToTime(*toPts, SeekMode::SEEK_NEXT_SYNC);
        ASSERT_NE(ret_, AV_ERR_OK);
        ret_ = demuxer_->SeekToTime(*toPts, SeekMode::SEEK_PREVIOUS_SYNC);
        ASSERT_NE(ret_, AV_ERR_OK);
        ret_ = demuxer_->SeekToTime(*toPts, SeekMode::SEEK_CLOSEST_SYNC);
        ASSERT_NE(ret_, AV_ERR_OK);
    }
}

/**
 * @tc.name: Demuxer_SeekToTime_1002
 * @tc.desc: seek to the specified time
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_1002, TestSize.Level1)
{
    InitResource(g_mp4Path, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(format_, nullptr);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    int readNum = 121;
    int64_t seekTime = 0;
    ReadData(readNum, seekTime);
    seekTime = (seekTime / 1000) + 500;
    ASSERT_EQ(demuxer_->SeekToTime(seekTime, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    ASSERT_EQ(demuxer_->ReadSample(0, sharedMem_, &info_, flag_), AV_ERR_OK);
    printf("time = %" PRId64 " | pts = %" PRId64 "\n", seekTime, info_.presentationTimeUs);
}

/**
 * @tc.name: Demuxer_SeekToTime_1010
 * @tc.desc: seek to the specified time
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_1010, TestSize.Level1)
{
    InitResource(g_mp4Path2, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(format_, nullptr);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    list<int64_t> toPtsList = {0, 3000, 2040, 1880, 1960, 2400, 2720, 2830, 4040, 4100}; // ms
    vector<int32_t> audioVals = {174, 174, 174, 7, 49, 49, 48, 91, 91, 90, 132, 90, 90, 91, 91, 48, 91, 91, 48, 91, 48,
        48, 91, 48, 8, 8, 8, 8};
    vector<int32_t> videoVals = {103, 103, 103, 6, 30, 30, 30, 54, 54, 54, 78, 54, 54, 54, 54, 30, 54, 54, 30, 54, 30,
        30, 54, 30, 6, 6, 6, 6};
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
 * @tc.name: Demuxer_SeekToTime_1040
 * @tc.desc: seek to the specified time
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_1040, TestSize.Level1)
{
    InitResource(g_mkvPath2, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(format_, nullptr);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    list<int64_t> toPtsList = {0, 1000, 1017, 1500, 1700, 1940, 3983, 1983, 3990}; // ms
    vector<int32_t> audioVals = {199, 199, 199, 149, 149, 149, 99, 149, 149, 99, 149, 149, 99, 149, 99, 99, 149, 99,
        49, 49, 99, 149, 99, 49, 49};
    vector<int32_t> videoVals = {240, 240, 240, 180, 180, 180, 120, 180, 180, 120, 180, 180, 120, 180, 120, 120, 180,
        120, 60, 60, 120, 180, 120, 60, 60};
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
 * @tc.name: Demuxer_SeekToTime_1060
 * @tc.desc: seek to the specified time
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_1060, TestSize.Level1)
{
    InitResource(g_tsPath, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(format_, nullptr);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    list<int64_t> toPtsList = {0, 3480, 3640, 3320, 3000, 3100, 4120, 5520}; // ms
    vector<int32_t> videoVals = {102, 102, 102, 15, 15, 15, 11, 11, 11, 19, 19, 19, 27, 27, 27, 24, 25, 25, 1, 1, 1};
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
 * @tc.name: Demuxer_SeekToTime_1070
 * @tc.desc: seek to the specified time
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_1070, TestSize.Level1)
{
    InitResource(g_aacPath, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(format_, nullptr);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    list<int64_t> toPtsList = {0, 10240, 10230, 10220, 30000, 30010}; // ms
    vector<int32_t> audioVals = {1293, 1293, 1293, 852, 852, 852, 853, 853, 853, 853, 853, 853, 2, 2, 2, 1, 1, 1};
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
            ASSERT_EQ(frames_[0], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: Demuxer_SeekToTime_1080
 * @tc.desc: seek to the specified time
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_1080, TestSize.Level1)
{
    InitResource(g_flacPath, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(format_, nullptr);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    list<int64_t> toPtsList = {0, 3072, 4031, 4035, 29952, 29953}; // ms
    vector<int32_t> audioVals = {313, 313, 313, 281, 281, 281, 272, 272, 272, 271, 271, 271, 1, 1, 1, 2, 2, 2};
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
            ASSERT_EQ(frames_[0], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: Demuxer_SeekToTime_1090
 * @tc.desc: seek to the specified time
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_1090, TestSize.Level1)
{
    InitResource(g_m4aPath, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(format_, nullptr);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    list<int64_t> toPtsList = {0, 14592, 15297, 15290, 29994, 29998}; // ms
    vector<int32_t> audioVals = {1407, 1407, 1407, 723, 723, 723, 690, 690, 690, 691, 691, 691, 2, 2, 2, 1, 1, 1};
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
            ASSERT_EQ(frames_[0], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: Demuxer_SeekToTime_1100
 * @tc.desc: seek to the specified time
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_1100, TestSize.Level1)
{
    InitResource(g_mp3Path, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(format_, nullptr);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    list<int64_t> toPtsList = {0, 4128, 11980, 11990, 30000, 30010}; // ms
    vector<int32_t> audioVals = {1251, 1251, 1251, 1079, 1079, 1079, 752, 752, 752, 752, 752, 752, 1, 1, 1, 1, 1, 1};
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
            ASSERT_EQ(frames_[0], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: Demuxer_SeekToTime_1110
 * @tc.desc: seek to the specified time
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_1110, TestSize.Level1)
{
    InitResource(g_oggPath, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(format_, nullptr);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    list<int64_t> toPtsList = {0, 5868, 5548, 5292, 29992, 29999}; // ms
    vector<int32_t> audioVals = {1598, 1598, 1598, 1357, 1357, 1357, 1357, 1357, 1357, 1357, 1357, 1357, 46, 46,
        46, 46, 46, 46};
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
            ASSERT_EQ(frames_[0], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: Demuxer_SeekToTime_1120
 * @tc.desc: seek to the specified time
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_1120, TestSize.Level1)
{
    InitResource(g_wavPath, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(format_, nullptr);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    list<int64_t> toPtsList = {0, 8576, 8566, 8578, 29994, 30000}; // ms
    vector<int32_t> audioVals = {704, 704, 704, 503, 503, 503, 504, 504, 504, 503, 503, 503, 2, 2, 2, 1, 1, 1};
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
            ASSERT_EQ(frames_[0], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: Demuxer_SeekToTime_1130
 * @tc.desc: seek to the specified time
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_1130, TestSize.Level1)
{
    InitResource(g_amrPath, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(format_, nullptr);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    list<int64_t> toPtsList = {0, 8560, 8550, 8570, 30000, 30900}; // ms
    vector<int32_t> audioVals = {1501, 1501, 1501, 1073, 1073, 1073, 1074, 1074, 1074, 1073, 1073, 1073,
        1, 1, 1, 1, 1, 1};
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
            ASSERT_EQ(frames_[0], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: Demuxer_SeekToTime_1140
 * @tc.desc: seek to the specified time
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_1140, TestSize.Level1)
{
    InitResource(g_amrPath2, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(format_, nullptr);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    list<int64_t> toPtsList = {0, 11920, 11910, 11930, 29980, 30800}; // ms
    vector<int32_t> audioVals = {1500, 1500, 1500, 904, 904, 904, 905, 905, 905, 904, 904, 904,
        1, 1, 1, 1, 1, 1};
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
            ASSERT_EQ(frames_[0], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: Demuxer_SeekToTime_1150
 * @tc.desc: seek to the specified time(audioVivid mp4)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_1150, TestSize.Level1)
{
    InitResource(g_audioVividPath, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(format_, nullptr);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    list<int64_t> toPtsList = {0, 10000, 8000, 12300, 25000, 29000, 30800, 33000}; // ms
    vector<int32_t> audioVals = {1380, 1380, 1380, 950, 950, 950, 1036, 1036, 1036, 851, 851, 851, 304, 304, 304,
        132, 132, 132, 54, 54, 54};
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
            ASSERT_EQ(frames_[0], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
    ASSERT_NE(demuxer_->SelectTrackByID(3), AV_ERR_OK);
    ASSERT_NE(demuxer_->SelectTrackByID(-1), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(3), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(-1), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(0), AV_ERR_OK);
}

/**
 * @tc.name: Demuxer_SeekToTime_1160
 * @tc.desc: seek to the specified time(audioVivid ts)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_1160, TestSize.Level1)
{
    InitResource(g_audioVividPath2, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(format_, nullptr);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    list<int64_t> toPtsList = {0, 10000, 8000, 12300, 25000, 29000, 30800, 32000}; // ms
    vector<int32_t> audioVals = {92, 92, 92, 68, 68, 68, 74, 74, 74, 61, 61, 61, 25, 25, 25, 13, 13, 13, 8, 8, 8};
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
            ASSERT_EQ(frames_[0], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
    ASSERT_NE(demuxer_->SelectTrackByID(3), AV_ERR_OK);
    ASSERT_NE(demuxer_->SelectTrackByID(-1), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(3), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(-1), AV_ERR_OK);
    ASSERT_EQ(demuxer_->UnselectTrackByID(0), AV_ERR_OK);
}

/**
 * @tc.name: Demuxer_SeekToTime_1200
 * @tc.desc: read first and then seek
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_1200, TestSize.Level1)
{
    InitResource(g_mp4Path2, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(format_, nullptr);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    SetInitValue();
    for (int i = 0; i < 50; i++) {
        for (auto idx : selectedTrackIds_) {
            ASSERT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
        }
    }
    int64_t seekTime = info_.presentationTimeUs / 1000 + 2000;
    ASSERT_EQ(demuxer_->SeekToTime(seekTime, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    ASSERT_EQ(demuxer_->ReadSample(0, sharedMem_, &info_, flag_), AV_ERR_OK);
    ASSERT_EQ(info_.presentationTimeUs, 3960000);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_1219
 * @tc.desc: copy current sample to buffer(two sound track mp4)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_1219, TestSize.Level1)
{
    InitResource(g_multiSoundTrackMp4Path, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(format_, nullptr);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(2), AV_ERR_OK);
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
    printf("frames_[2]=%d | kFrames[2]=%d\n", frames_[2], keyFrames_[2]);
    ASSERT_EQ(frames_[0], 602);
    ASSERT_EQ(frames_[1], 433);
    ASSERT_EQ(frames_[2], 417);
    ASSERT_EQ(keyFrames_[0], 3);
    ASSERT_EQ(keyFrames_[1], 433);
    ASSERT_EQ(keyFrames_[2], 417);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_1228
 * @tc.desc: copy current sample to buffer, local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_1228, TestSize.Level1)
{
    InitResource(g_fmp4AvcPath, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(format_, nullptr);
    ASSERT_NE(demuxer_, nullptr);
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
    ASSERT_EQ(frames_[1], 433);
    ASSERT_EQ(keyFrames_[0], 3);
    ASSERT_EQ(keyFrames_[1], 433);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_1234
 * @tc.desc: copy current sample to buffer, local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_1234, TestSize.Level1)
{
    InitResource(g_fmp4m4vPath, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(format_, nullptr);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
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
    ASSERT_EQ(frames_[0], 602);
    ASSERT_EQ(keyFrames_[0], 3);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_1236
 * @tc.desc: copy current sample to buffer, local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_1236, TestSize.Level1)
{
    InitResource(g_fmp4m4aPath, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(format_, nullptr);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
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
    ASSERT_EQ(frames_[0], 433);
    ASSERT_EQ(keyFrames_[0], 433);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_SeekToTime_1194
 * @tc.desc: seek to the specified time(two sound track mp4 local)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_1194, TestSize.Level1)
{
    InitResource(g_multiSoundTrackMp4Path, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(format_, nullptr);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(2), AV_ERR_OK);
    list<int64_t> toPtsList = {0, 4500, 7000, 2000, 10000}; // ms
    vector<int32_t> videoVals = {602, 602, 602, 102, 352, 352, 102, 352, 102, 352, 602, 602, 102, 102};
    vector<int32_t> audioVals = {433, 433, 433, 74, 254, 254, 74, 254, 74, 253, 433, 433, 75, 75};
    vector<int32_t> audioVals2 = {417, 417, 417, 71, 245, 245, 71, 245, 71, 244, 417, 417, 72, 72};
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
            printf("time = %" PRId64 " | frames_[2]=%d | kFrames[2]=%d\n", *toPts, frames_[2], keyFrames_[2]);
            ASSERT_EQ(frames_[0], videoVals[numbers_]);
            ASSERT_EQ(frames_[1], audioVals[numbers_]);
            ASSERT_EQ(frames_[2], audioVals2[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
    ASSERT_NE(demuxer_->SeekToTime(11000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    ASSERT_NE(demuxer_->SeekToTime(-1000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
}

/**
 * @tc.name: Demuxer_ReadSample_1222
 * @tc.desc: copy current sample to buffer(ape)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_1222, TestSize.Level1)
{
    InitResource(g_apePath, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(format_, nullptr);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    SetInitValue();
    uint32_t idx = 0;
    while (!isEOS(eosFlag_)) {
        ASSERT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
        CountFrames(idx);
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    ASSERT_EQ(frames_[0], 7);
    ASSERT_EQ(keyFrames_[0], 7);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_SeekToTime_1224
 * @tc.desc: seek to the specified time(two sound track ape local)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_1224, TestSize.Level1)
{
    InitResource(g_apePath, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(format_, nullptr);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    list<int64_t> toPtsList = {0, 4500, 7000, 2000, 10000}; // ms
    vector<int32_t> audioVals = {7, 7, 7, 5, 5, 5, 3, 3, 3, 6, 6, 6, 2, 2, 2};

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
 * @tc.name: Demuxer_ReadSample_1400
 * @tc.desc: copy current sample to buffer(flv)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_1400, TestSize.Level1)
{
    InitResource(g_flvPath, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(format_, nullptr);
    ASSERT_NE(demuxer_, nullptr);
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
    ASSERT_EQ(frames_[0], 76);
    ASSERT_EQ(frames_[1], 113);
    ASSERT_EQ(keyFrames_[0], 1);
    ASSERT_EQ(keyFrames_[1], 113);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_SeekToTime_1206
 * @tc.desc: seek to the specified time(h264 flv local)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_1206, TestSize.Level1)
{
    InitResource(g_flvPath, LOCAL);
    SetInitValue();
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 1500, 1000, 1740, 1970, 2100}; // ms
    vector<int32_t> videoVals = {76, 76, 76, 0, 76, 0, 0, 76, 76, 0, 76, 0, 0, 76, 0, 0, 76, 0};
    vector<int32_t> audioVals = {107, 107, 107, 0, 107, 0, 0, 107, 107, 0, 107, 0, 0, 107, 0, 0, 107, 0};
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
 * @tc.name: Demuxer_SeekToTime_1229
 * @tc.desc: seek to the specified time(h264 fmp4 fd)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_1229, TestSize.Level1)
{
    InitResource(g_fmp4AvcPath, LOCAL);
    SetInitValue();
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 4500, 7000, 2000}; // ms
    vector<int32_t> videoVals = {602, 602, 602, 102, 352, 352, 102, 352, 102, 352, 602, 602};
    vector<int32_t> audioVals = {433, 433, 433, 74, 254, 254, 74, 254, 74, 253, 433, 433};
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
 * @tc.name: Demuxer_SeekToTime_1238
 * @tc.desc: seek to the specified time(fmp4 m4v fd)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_1238, TestSize.Level1)
{
    InitResource(g_fmp4m4vPath, LOCAL);
    SetInitValue();
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 4500, 7000, 2000}; // ms
    vector<int32_t> videoVals = {602, 602, 602, 102, 352, 352, 102, 352, 102, 352, 602, 602};
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
            ASSERT_EQ(frames_[0], videoVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
    ASSERT_NE(demuxer_->SeekToTime(11000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    ASSERT_NE(demuxer_->SeekToTime(-1000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
}

/**
 * @tc.name: Demuxer_SeekToTime_1239
 * @tc.desc: seek to the specified time(fmp4 m4a fd)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_1239, TestSize.Level1)
{
    InitResource(g_fmp4m4aPath, LOCAL);
    SetInitValue();
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 4500, 7000, 2000}; // ms
    vector<int32_t> audioVals = {433, 433, 433, 240, 240, 240, 132, 132, 132, 348, 348, 348};
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
    ASSERT_NE(demuxer_->SeekToTime(11000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    ASSERT_NE(demuxer_->SeekToTime(-1000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
}

/**
 * @tc.name: Demuxer_ReadSample_3000
 * @tc.desc: copy current sample to buffer(srt)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_3000, TestSize.Level1)
{
    InitResource(g_srt, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(format_, nullptr);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);

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
    ASSERT_EQ(frames_[0], 5);
    RemoveValue();
}

#ifndef DEMUXER_CAPI_BUFFER_UNIT_TEST
/**
 * @tc.name: Demuxer_ReadSample_3002
 * @tc.desc: copy current sample to buffer(vtt, en)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_3002, TestSize.Level1)
{
    InitResource(g_vttPath, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(format_, nullptr);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);

    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    SetInitValue();
    int32_t vttIndex = 0;
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            ASSERT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            char *vttStr = static_cast<char *>(malloc(info_.size * sizeof(char) + 1));
            memcpy_s(vttStr, info_.size, sharedMem_->GetAddr(), info_.size);
            *(vttStr + info_.size) = '\0';
            switch (vttIndex) {
                case 0:
                    ASSERT_EQ(strcmp(vttStr, "testA"), 0);
                    ASSERT_EQ(info_.presentationTimeUs, 500000);
                    ASSERT_EQ(info_.size, 5);
                    break;
                case 1:
                    ASSERT_EQ(strcmp(vttStr, "testBB"), 0);
                    ASSERT_EQ(info_.presentationTimeUs, 1001000);
                    ASSERT_EQ(info_.size, 6);
                    break;
                case 2:
                    ASSERT_EQ(strcmp(vttStr, "testCCC"), 0);
                    ASSERT_EQ(info_.presentationTimeUs, 2003000);
                    ASSERT_EQ(info_.size, 7);
                    break;
                case 3:
                    ASSERT_EQ(strcmp(vttStr, "testDDDD"), 0);
                    ASSERT_EQ(info_.presentationTimeUs, 2100000);
                    ASSERT_EQ(info_.size, 8);
                    break;
                default:
                    break;
            }
            CountFrames(idx);
            free(vttStr);
        }
        vttIndex++;
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    ASSERT_EQ(frames_[0], 4);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_3003
 * @tc.desc: copy current sample to buffer(vtt, zh)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_3003, TestSize.Level1)
{
    InitResource(g_vttPath2, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(format_, nullptr);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);

    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    SetInitValue();
    int32_t vttIndex = 0;
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            ASSERT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            char *vttStr = static_cast<char *>(malloc(info_.size * sizeof(char) + 1));
            memcpy_s(vttStr, info_.size, sharedMem_->GetAddr(), info_.size);
            *(vttStr + info_.size) = '\0';
            switch (vttIndex) {
                case 0:
                    ASSERT_EQ(strcmp(vttStr, "第1帧"), 0);
                    ASSERT_EQ(info_.presentationTimeUs, 30000);
                    ASSERT_EQ(info_.size, 7);
                    break;
                case 1:
                    ASSERT_EQ(strcmp(vttStr, "第2帧"), 0);
                    ASSERT_EQ(info_.presentationTimeUs, 2030000);
                    ASSERT_EQ(info_.size, 7);
                    break;
                case 2:
                    ASSERT_EQ(strcmp(vttStr, "第3帧"), 0);
                    ASSERT_EQ(info_.presentationTimeUs, 2900000);
                    ASSERT_EQ(info_.size, 7);
                    break;
                case 3:
                    ASSERT_EQ(strcmp(vttStr, "第4帧"), 0);
                    ASSERT_EQ(info_.presentationTimeUs, 3000000);
                    ASSERT_EQ(info_.size, 7);
                    break;
                default:
                    break;
            }
            CountFrames(idx);
            free(vttStr);
        }
        vttIndex++;
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);

    ASSERT_EQ(frames_[0], 10);
    RemoveValue();
}
#endif

/**
 * @tc.name: Demuxer_SeekToTime_3000
 * @tc.desc: seek to the specified time(srt)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_3000, TestSize.Level1)
{
    InitResource(g_srt, LOCAL);
    SetInitValue();
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 160, 2000, 4000, 7000}; // ms
    vector<int32_t> subVals = {5, 5, 5, 5, 5, 5, 5, 5, 5, 4, 4, 4, 2, 2, 2};
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
            ASSERT_EQ(frames_[0], subVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

#ifndef DEMUXER_CAPI_BUFFER_UNIT_TEST
/**
 * @tc.name: Demuxer_SeekToTime_3002
 * @tc.desc: seek to the specified time(vtt, en)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_3002, TestSize.Level1)
{
    InitResource(g_vttPath, LOCAL);
    SetInitValue();
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {500, 1000, 2000, 3000}; // ms
    vector<int32_t> subVals = {4, 4, 4, 4, 4, 4, 3, 3, 3, 1, 1, 1};
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
            ASSERT_EQ(frames_[0], subVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: Demuxer_SeekToTime_3003
 * @tc.desc: seek to the specified time(vtt, zh)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_3003, TestSize.Level1)
{
    InitResource(TEST_FILE_PATH + string("webvtt_test2.vtt"), LOCAL);
    SetInitValue();
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {100, 1000, 1200, 61000, 65000}; // ms
    vector<int32_t> subVals = {10, 10, 10, 10, 10, 10, 9, 9, 9, 1, 1, 1, 1, 1, 1};
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
            ASSERT_EQ(frames_[0], subVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}
#endif

/**
 * @tc.name: Demuxer_SetMediaKeySystemInfoCallback_4000
 * @tc.desc: set normal drm callback
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SetMediaKeySystemInfoCallback_4000, TestSize.Level1)
{
    InitResource(g_drmSm4cPath, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(demuxer_, nullptr);

    bool isNullCallback = false;
    int32_t ret = demuxer_->SetMediaKeySystemInfoCallback(isNullCallback);
    ASSERT_EQ(ret, AV_ERR_OK);
}

/**
 * @tc.name: Demuxer_SetMediaKeySystemInfoCallback_4001
 * @tc.desc: set null drm callback
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SetMediaKeySystemInfoCallback_4001, TestSize.Level1)
{
    InitResource(g_drmSm4cPath, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(demuxer_, nullptr);

    bool isNullCallback = true;
    int32_t ret = demuxer_->SetMediaKeySystemInfoCallback(isNullCallback);
    ASSERT_EQ(ret, AV_ERR_OK);
}

/**
 * @tc.name: Demuxer_SetDemuxerMediaKeySystemInfoCallback_4002
 * @tc.desc: set normal drm callback
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SetDemuxerMediaKeySystemInfoCallback_4002, TestSize.Level1)
{
    InitResource(g_drmSm4cPath, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(demuxer_, nullptr);
    bool isNullCallback = false;
    int32_t ret = demuxer_->SetDemuxerMediaKeySystemInfoCallback(isNullCallback);
    ASSERT_EQ(ret, AV_ERR_OK);
}

/**
 * @tc.name: Demuxer_SetDemuxerMediaKeySystemInfoCallback_4003
 * @tc.desc: set null drm callback
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SetDemuxerMediaKeySystemInfoCallback_4003, TestSize.Level1)
{
    InitResource(g_drmSm4cPath, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(demuxer_, nullptr);

    bool isNullCallback = true;
    int32_t ret = demuxer_->SetDemuxerMediaKeySystemInfoCallback(isNullCallback);
    ASSERT_EQ(ret, AV_ERR_OK);
}

/**
 * @tc.name: Demuxer_GetMediaKeySystemInfo_4004
 * @tc.desc: get drm info when play drm
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_GetMediaKeySystemInfo_4004, TestSize.Level1)
{
    InitResource(g_drmSm4cPath, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(demuxer_, nullptr);

    int32_t ret = demuxer_->GetMediaKeySystemInfo();
    ASSERT_EQ(ret, AV_ERR_OK);
}

#ifndef DEMUXER_CAPI_UNIT_TEST
/**
 * @tc.name: Demuxer_GetIndexByRelativePresentationTimeUs_1000
 * @tc.desc: Get index by pts(audio track)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_GetIndexByRelativePresentationTimeUs_1000, TestSize.Level1)
{
    InitResource(g_ptsConversionPath, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(demuxer_, nullptr);

    uint32_t trackIndex = 0;
    uint64_t relativePresentationTimeUs = 69659;
    uint32_t index = 0;

    int32_t ret = demuxer_->GetIndexByRelativePresentationTimeUs(trackIndex, relativePresentationTimeUs, index);
    ASSERT_EQ(ret, AV_ERR_OK);

    ASSERT_EQ(index, 3);
}

/**
 * @tc.name: Demuxer_GetIndexByRelativePresentationTimeUs_1001
 * @tc.desc: Get index by pts(video track)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_GetIndexByRelativePresentationTimeUs_1001, TestSize.Level1)
{
    InitResource(g_ptsConversionPath, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(demuxer_, nullptr);

    uint32_t trackIndex = 1;
    uint64_t relativePresentationTimeUs = 66666;
    uint32_t index = 0;

    int32_t ret = demuxer_->GetIndexByRelativePresentationTimeUs(trackIndex, relativePresentationTimeUs, index);
    ASSERT_EQ(ret, AV_ERR_OK);

    ASSERT_EQ(index, 4);
}

/**
 * @tc.name: Demuxer_GetIndexByRelativePresentationTimeUs_1002
 * @tc.desc: Get index by pts(not MP4)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_GetIndexByRelativePresentationTimeUs_1002, TestSize.Level1)
{
    InitResource(g_flvPath, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(demuxer_, nullptr);

    uint32_t trackIndex = 0;
    uint64_t relativePresentationTimeUs = 69659;
    uint32_t index = 0;

    int32_t ret = demuxer_->GetIndexByRelativePresentationTimeUs(trackIndex, relativePresentationTimeUs, index);
    ASSERT_NE(ret, AV_ERR_OK);

    ASSERT_EQ(index, 0);
}

/**
 * @tc.name: Demuxer_GetIndexByRelativePresentationTimeUs_1003
 * @tc.desc: Get index by pts(non-standard pts & different track)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_GetIndexByRelativePresentationTimeUs_1003, TestSize.Level1)
{
    InitResource(g_ptsConversionPath, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(demuxer_, nullptr);

    // standard pts & video track
    uint32_t trackIndex = 1;
    uint64_t relativePresentationTimeUs = 166666;
    uint32_t index = 0;
    int32_t ret = demuxer_->GetIndexByRelativePresentationTimeUs(trackIndex, relativePresentationTimeUs, index);
    ASSERT_EQ(ret, AV_ERR_OK);
    ASSERT_EQ(index, 10);
}

/**
 * @tc.name: Demuxer_GetRelativePresentationTimeUsByIndex_1000
 * @tc.desc: get pts by frameIndex(audio track)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_GetRelativePresentationTimeUsByIndex_1000, TestSize.Level1)
{
    InitResource(g_ptsConversionPath, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(demuxer_, nullptr);

    uint32_t trackIndex = 0;
    uint64_t relativePresentationTimeUs = 0;
    uint32_t index = 2;

    int32_t ret = demuxer_->GetRelativePresentationTimeUsByIndex(trackIndex, index, relativePresentationTimeUs);
    ASSERT_EQ(ret, AV_ERR_OK);

    ASSERT_EQ(relativePresentationTimeUs, 46439);
}

/**
 * @tc.name: Demuxer_GetRelativePresentationTimeUsByIndex_1001
 * @tc.desc: get pts by frameIndex(video track)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_GetRelativePresentationTimeUsByIndex_1001, TestSize.Level1)
{
    InitResource(g_ptsConversionPath, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(demuxer_, nullptr);

    uint32_t trackIndex = 1;
    uint64_t relativePresentationTimeUs = 0;
    uint32_t index = 2;

    int32_t ret = demuxer_->GetRelativePresentationTimeUsByIndex(trackIndex, index, relativePresentationTimeUs);
    ASSERT_EQ(ret, AV_ERR_OK);

    ASSERT_EQ(relativePresentationTimeUs, 33333);
}

/**
 * @tc.name: Demuxer_GetRelativePresentationTimeUsByIndex_1002
 * @tc.desc: get pts by frameIndex(not MP4)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_GetRelativePresentationTimeUsByIndex_1002, TestSize.Level1)
{
    InitResource(g_flvPath, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(demuxer_, nullptr);

    uint32_t trackIndex = 0;
    uint64_t relativePresentationTimeUs = 0;
    uint32_t index = 10;

    int32_t ret = demuxer_->GetRelativePresentationTimeUsByIndex(trackIndex, index, relativePresentationTimeUs);
    ASSERT_NE(ret, AV_ERR_OK);

    ASSERT_EQ(relativePresentationTimeUs, 0);
}

/**
 * @tc.name: Demuxer_PtsAndFrameIndexConversion_1000
 * @tc.desc: pts and frameIndex convertion test(pts -> frameIndex -> pts)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_PtsAndFrameIndexConversion_1000, TestSize.Level1)
{
    InitResource(g_ptsConversionPath, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(format_, nullptr);

    uint32_t trackIndex = 0;
    uint64_t relativePresentationTimeUs = 92879;
    uint32_t index = 0;

    int32_t ret = demuxer_->GetIndexByRelativePresentationTimeUs(trackIndex, relativePresentationTimeUs, index);
    ASSERT_EQ(ret, AV_ERR_OK);
    ASSERT_EQ(index, 4);

    uint64_t relativePresentationTimeUs1 = 0;

    ret = demuxer_->GetRelativePresentationTimeUsByIndex(trackIndex, index, relativePresentationTimeUs1);
    ASSERT_EQ(ret, AV_ERR_OK);
    ASSERT_EQ(relativePresentationTimeUs1, 92879);
}

/**
 * @tc.name: Demuxer_PtsAndFrameIndexConversion_1001
 * @tc.desc: pts and frameIndex convertion test(frameIndex -> pts -> frameIndex)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_PtsAndFrameIndexConversion_1001, TestSize.Level1)
{
    InitResource(g_ptsConversionPath, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(format_, nullptr);

    uint32_t trackIndex = 0;
    uint64_t relativePresentationTimeUs = 0;
    uint32_t index = 4;

    int32_t ret = demuxer_->GetRelativePresentationTimeUsByIndex(trackIndex, index, relativePresentationTimeUs);
    ASSERT_EQ(ret, AV_ERR_OK);
    ASSERT_EQ(relativePresentationTimeUs, 92879);

    uint32_t index1 = 0;

    ret = demuxer_->GetIndexByRelativePresentationTimeUs(trackIndex, relativePresentationTimeUs, index1);
    ASSERT_EQ(ret, AV_ERR_OK);
    ASSERT_EQ(index1, 4);
}

/**
 * @tc.name: Demuxer_PTSOutOfRange_1000
 * @tc.desc: pts out of range
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_PTSOutOfRange_1000, TestSize.Level1)
{
    InitResource(g_ptsConversionPath, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(format_, nullptr);

    uint32_t trackIndex = 0;
    uint64_t relativePresentationTimeUs = 999999999;
    uint32_t index = 0;
    int32_t ret = demuxer_->GetIndexByRelativePresentationTimeUs(trackIndex, relativePresentationTimeUs, index);
    ASSERT_NE(ret, AV_ERR_OK);
}

/**
 * @tc.name: Demuxer_IndexOutOfRange_1000
 * @tc.desc: Index out of range
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_IndexOutOfRange_1000, TestSize.Level1)
{
    InitResource(g_ptsConversionPath, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(format_, nullptr);

    uint32_t trackIndex = 0;
    uint64_t relativePresentationTimeUs = 0;
    uint32_t index = 9999999;
    int32_t ret = demuxer_->GetRelativePresentationTimeUsByIndex(trackIndex, index, relativePresentationTimeUs);
    ASSERT_NE(ret, AV_ERR_OK);
}

/**
 * @tc.name: Demuxer_TrackOutOfRange_1000
 * @tc.desc: Track out of range
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_TrackOutOfRange_1000, TestSize.Level1)
{
    InitResource(g_ptsConversionPath, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(format_, nullptr);

    uint32_t trackIndex = 99;
    uint64_t relativePresentationTimeUs = 0;
    uint32_t index = 0;
    int32_t ret = demuxer_->GetRelativePresentationTimeUsByIndex(trackIndex, index, relativePresentationTimeUs);
    ASSERT_NE(ret, AV_ERR_OK);
}

#endif

/**
 * @tc.name: Demuxer_ReadSample_1601
 * @tc.desc: copy current sample to buffer(mp4 vvc)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_1601, TestSize.Level1)
{
    if (access(g_mp4VvcPath.c_str(), F_OK) != 0) {
        return;
    }
    InitResource(g_mp4VvcPath, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(format_, nullptr);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    SetInitValue();
    uint32_t idx = 0;
    while (!isEOS(eosFlag_)) {
        ASSERT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
        CountFrames(idx);
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    ASSERT_EQ(frames_[0], 600);
    ASSERT_EQ(keyFrames_[0], 10);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_SeekToTime_1601
 * @tc.desc: seek to the specified time(mp4 vvc fd)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_1601, TestSize.Level1)
{
    if (access(g_mp4VvcPath.c_str(), F_OK) != 0) {
        return;
    }
    InitResource(g_mp4VvcPath, LOCAL);
    SetInitValue();
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 4500, 7000, 2000}; // ms
    vector<int32_t> audioVals = {433, 433, 433, 240, 240, 240, 132, 132, 132, 348, 348, 348};
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
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
    ASSERT_NE(demuxer_->SeekToTime(11000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    ASSERT_NE(demuxer_->SeekToTime(-1000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
}
} // namespace