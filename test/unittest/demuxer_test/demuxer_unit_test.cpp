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

using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace testing::ext;
using namespace std;

namespace {
unique_ptr<FileServerDemo> server = nullptr;
static const string TEST_FILE_PATH = "/data/test/media/";
static const string TEST_URI_PATH = "http://127.0.0.1:46666/";
const int64_t SOURCE_OFFSET = 0;
int32_t bufferSize = 0;
int32_t width = 3840;
int32_t height = 2160;
int32_t nb_streams = 0;
int32_t number = 0;
int8_t modeNum = 0;
list<AVSeekMode> seekModes = {AVSeekMode::SEEK_MODE_NEXT_SYNC, AVSeekMode::SEEK_MODE_PREVIOUS_SYNC,
    AVSeekMode::SEEK_MODE_CLOSEST_SYNC};
map<uint32_t, int32_t> frames;
map<uint32_t, int32_t> keyFrames;
map<uint32_t, bool> eosFlag;
int32_t ret = AV_ERR_OK;

string mp4Path = TEST_FILE_PATH + string("test_264_B_Gop25_4sec_cover.mp4");
string mp4Path2 = TEST_FILE_PATH + string("test_mpeg2_B_Gop25_4sec.mp4");
string mp4Path4 = TEST_FILE_PATH + string("zero_track.mp4");
string mkvPath2 = TEST_FILE_PATH + string("h264_opus_4sec.mkv");
string tsPath = TEST_FILE_PATH + string("test_mpeg2_Gop25_4sec.ts");
string aacPath = TEST_FILE_PATH + string("audio/aac_44100_1.aac");
string flacPath = TEST_FILE_PATH + string("audio/flac_48000_1_cover.flac");
string m4aPath = TEST_FILE_PATH + string("audio/m4a_48000_1.m4a");
string mp3Path = TEST_FILE_PATH + string("audio/mp3_48000_1_cover.mp3");
string oggPath = TEST_FILE_PATH + string("audio/ogg_48000_1.ogg");
string wavPath = TEST_FILE_PATH + string("audio/wav_48000_1.wav");
string amrPath = TEST_FILE_PATH + string("audio/amr_nb_8000_1.amr");
string amrPath2 = TEST_FILE_PATH + string("audio/amr_wb_16000_1.amr");

string mp4Uri = TEST_URI_PATH + string("test_264_B_Gop25_4sec_cover.mp4");
string mp4Uri2 = TEST_URI_PATH + string("test_mpeg2_B_Gop25_4sec.mp4");
string mp4Uri4 = TEST_URI_PATH + string("zero_track.mp4");
string mkvUri2 = TEST_URI_PATH + string("h264_opus_4sec.mkv");
string tsUri = TEST_URI_PATH + string("test_mpeg2_Gop25_4sec.ts");
string aacUri = TEST_URI_PATH + string("audio/aac_44100_1.aac");
string flacUri = TEST_URI_PATH + string("audio/flac_48000_1_cover.flac");
string m4aUri = TEST_URI_PATH + string("audio/m4a_48000_1.m4a");
string mp3Uri = TEST_URI_PATH + string("audio/mp3_48000_1_cover.mp3");
string oggUri = TEST_URI_PATH + string("audio/ogg_48000_1.ogg");
string wavUri = TEST_URI_PATH + string("audio/wav_48000_1.wav");
string amrUri = TEST_URI_PATH + string("audio/amr_nb_8000_1.amr");
string amrUri2 = TEST_URI_PATH + string("audio/amr_wb_16000_1.amr");
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
    bufferSize = width * height;
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
    bufferSize = 0;
    nb_streams = 0;
    number = 0;
    info_.presentationTimeUs = 0;
    info_.offset = 0;
    info_.size = 0;
    modeNum = 0;
    selectedTrackIds_.clear();
}

int64_t DemuxerUnitTest::GetFileSize(const string fileName)
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

int32_t DemuxerUnitTest::OpenFile(const string fileName)
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
    string codec_mime = "";
    for (int i = 0; i < nb_streams; i++) {
        format_ = source_->GetTrackFormat(i);
        format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, codec_mime);
        if (codec_mime.find("image/") != std::string::npos) {
            continue;
        }
        selectedTrackIds_.push_back(static_cast<uint32_t>(i));
        frames[i] = 0;
        keyFrames[i] = 0;
        eosFlag[i] = false;
    }
}

static void RemoveValue()
{
    if (!frames.empty()) {
        frames.clear();
    }
    if (!keyFrames.empty()) {
        keyFrames.clear();
    }
    if (!eosFlag.empty()) {
        eosFlag.clear();
    }
}

static void SetEosValue()
{
    for (int i = 0; i < nb_streams; i++) {
        eosFlag[i] = true;
    }
}

static void CountFrames(uint32_t index, AVCodecBufferFlag flag)
{
    switch (flag) {
        case AVCodecBufferFlag::AVCODEC_BUFFER_FLAG_SYNC_FRAME:
            keyFrames[index]++;
            frames[index]++;
            break;
        case AVCodecBufferFlag::AVCODEC_BUFFER_FLAG_NONE:
            frames[index]++;
            break;
        case AVCodecBufferFlag::AVCODEC_BUFFER_FLAG_EOS:
            eosFlag[index] = true;
            break;
        default:
            SetEosValue();
            printf("flag is unknown, read sample break");
            break;
    }
}

void DemuxerUnitTest::ReadData()
{
    SetInitValue();
    while (!isEOS(eosFlag)) {
        for (auto idx : selectedTrackIds_) {
            demuxer_->ReadSample(idx, sharedMem_, &info_, flag_);
            if (ret != AV_ERR_OK) {
                break;
            }
            CountFrames(idx, flag_);
        }
        if (ret != AV_ERR_OK) {
            break;
        }
    }
}

void DemuxerUnitTest::ReadData(int readNum, int64_t &seek_time)
{
    int num = 0;
    SetInitValue();
    while (!isEOS(eosFlag)) {
        for (auto idx : selectedTrackIds_) {
            ret = demuxer_->ReadSample(idx, sharedMem_, &info_, flag_);
            if (ret != AV_ERR_OK) {
                break;
            }
            CountFrames(idx, flag_);
        }
        if (ret != AV_ERR_OK) {
            break;
        }
        if (num == readNum) {
            seek_time = info_.presentationTimeUs;
            break;
        }
    }
}

/**********************************demuxer**************************************/
/**
 * @tc.name: Demuxer_CreateDemuxer_1000
 * @tc.desc: create demuxer
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CreateDemuxer_1000, TestSize.Level1)
{
    fd_ = OpenFile(mp4Path);
    int64_t size = GetFileSize(mp4Path);
    printf("---- %s ------\n", mp4Path.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size);
    ASSERT_NE(source_, nullptr);
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
    ASSERT_NE(demuxer_, nullptr);
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
    fd_ = OpenFile(mp4Path);
    int64_t size = GetFileSize(mp4Path);
    printf("---- %s ------\n", mp4Path.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size);
    ASSERT_NE(source_, nullptr);
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
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
    fd_ = OpenFile(tsPath);
    int64_t size = GetFileSize(tsPath);
    printf("---- %s ------\n", tsPath.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size);
    ASSERT_NE(source_, nullptr);
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
    ASSERT_NE(demuxer_, nullptr);
}

/**
 * @tc.name: Demuxer_CreateDemuxer_1040
 * @tc.desc: create demuxer
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CreateDemuxer_1040, TestSize.Level1)
{
    fd_ = OpenFile(mp4Path4);
    int64_t size = GetFileSize(mp4Path4);
    printf("---- %s ------\n", mp4Path4.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size);
    ASSERT_NE(source_, nullptr);
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
    ASSERT_NE(demuxer_, nullptr);
}

/**
 * @tc.name: Demuxer_UnselectTrackByID_1000
 * @tc.desc: select and remove track by ID
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_UnselectTrackByID_1000, TestSize.Level1)
{
    fd_ = OpenFile(mp4Path);
    int64_t size = GetFileSize(mp4Path);
    printf("---- %s ------\n", mp4Path.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size);
    ASSERT_NE(source_, nullptr);
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
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
 * @tc.name: Demuxer_UnselectTrackByID_1010
 * @tc.desc: select and remove track by ID
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_UnselectTrackByID_1010, TestSize.Level1)
{
    fd_ = OpenFile(mp4Path4);
    int64_t size = GetFileSize(mp4Path4);
    printf("---- %s ------\n", mp4Path4.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size);
    ASSERT_NE(source_, nullptr);
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_NE(demuxer_->SelectTrackByID(0), AV_ERR_OK);
}

/**
 * @tc.name: Demuxer_UnselectTrackByID_1020
 * @tc.desc: select and remove track by ID
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_UnselectTrackByID_1020, TestSize.Level1)
{
    fd_ = OpenFile(tsPath);
    int64_t size = GetFileSize(tsPath);
    printf("---- %s ------\n", tsPath.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size);
    ASSERT_NE(source_, nullptr);
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
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
 * @tc.name: Demuxer_UnselectTrackByID_1030
 * @tc.desc: select and remove track by ID
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_UnselectTrackByID_1030, TestSize.Level1)
{
    fd_ = OpenFile(aacPath);
    int64_t size = GetFileSize(aacPath);
    printf("---- %s ------\n", aacPath.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size);
    ASSERT_NE(source_, nullptr);
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
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
 * @tc.name: Demuxer_UnselectTrackByID_1040
 * @tc.desc: select and remove track by ID
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_UnselectTrackByID_1040, TestSize.Level1)
{
    fd_ = OpenFile(flacPath);
    int64_t size = GetFileSize(flacPath);
    printf("---- %s ------\n", flacPath.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size);
    ASSERT_NE(source_, nullptr);
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
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
 * @tc.name: Demuxer_UnselectTrackByID_1050
 * @tc.desc: select and remove track by ID
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_UnselectTrackByID_1050, TestSize.Level1)
{
    fd_ = OpenFile(m4aPath);
    int64_t size = GetFileSize(m4aPath);
    printf("---- %s ------\n", m4aPath.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size);
    ASSERT_NE(source_, nullptr);
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
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
    fd_ = OpenFile(mp3Path);
    int64_t size = GetFileSize(mp3Path);
    printf("---- %s ------\n", mp3Path.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size);
    ASSERT_NE(source_, nullptr);
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
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
    fd_ = OpenFile(oggPath);
    int64_t size = GetFileSize(oggPath);
    printf("---- %s ------\n", oggPath.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size);
    ASSERT_NE(source_, nullptr);
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
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
    fd_ = OpenFile(wavPath);
    int64_t size = GetFileSize(wavPath);
    printf("---- %s ------\n", wavPath.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size);
    ASSERT_NE(source_, nullptr);
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
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
    fd_ = OpenFile(mkvPath2);
    int64_t size = GetFileSize(mkvPath2);
    printf("---- %s ------\n", mkvPath2.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size);
    ASSERT_NE(source_, nullptr);
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
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
    fd_ = OpenFile(amrPath);
    int64_t size = GetFileSize(amrPath);
    printf("---- %s ------\n", amrPath.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size);
    ASSERT_NE(source_, nullptr);
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
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
    fd_ = OpenFile(mp4Path);
    int64_t size = GetFileSize(mp4Path);
    printf("---- %s ------\n", mp4Path.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, nb_streams));
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize);
    ASSERT_NE(sharedMem_, nullptr);
    SetInitValue();
    while (!isEOS(eosFlag)) {
        for (auto idx : selectedTrackIds_) {
            ASSERT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx, flag_);
        }
    }
    printf("frames[0]=%d | kFrames[0]=%d\n", frames[0], keyFrames[0]);
    printf("frames[1]=%d | kFrames[1]=%d\n", frames[1], keyFrames[1]);
    ASSERT_EQ(frames[0], 103);
    ASSERT_EQ(frames[1], 174);
    ASSERT_EQ(keyFrames[0], 5);
    ASSERT_EQ(keyFrames[1], 174);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_1010
 * @tc.desc: copy current sample to buffer
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_1010, TestSize.Level1)
{
    fd_ = OpenFile(mp4Path);
    int64_t size = GetFileSize(mp4Path);
    printf("---- %s ------\n", mp4Path.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size);
    ASSERT_NE(source_, nullptr);
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize);
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
    fd_ = OpenFile(mp4Path);
    int64_t size = GetFileSize(mp4Path);
    printf("---- %s ------\n", mp4Path.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size);
    ASSERT_NE(source_, nullptr);
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize);
    ASSERT_NE(sharedMem_, nullptr);
    uint32_t idx = -1;
    ASSERT_NE(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
}

/**
 * @tc.name: Demuxer_ReadSample_1030
 * @tc.desc: copy current sample to buffer
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_1030, TestSize.Level1)
{
    fd_ = OpenFile(mp4Path);
    int64_t size = GetFileSize(mp4Path);
    printf("---- %s ------\n", mp4Path.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, nb_streams));
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize);
    ASSERT_NE(sharedMem_, nullptr);
    int32_t vkeyFrames = 0;
    int32_t vframes = 0;
    flag_ = AVCodecBufferFlag::AVCODEC_BUFFER_FLAG_NONE;
    while (flag_ != AVCodecBufferFlag::AVCODEC_BUFFER_FLAG_EOS) {
        ASSERT_EQ(demuxer_->ReadSample(0, sharedMem_, &info_, flag_), AV_ERR_OK);
        switch (flag_) {
            case AVCodecBufferFlag::AVCODEC_BUFFER_FLAG_SYNC_FRAME:
                vkeyFrames++;
                vframes++;
                break;
            case AVCodecBufferFlag::AVCODEC_BUFFER_FLAG_NONE:
                vframes++;
                break;
            case AVCodecBufferFlag::AVCODEC_BUFFER_FLAG_EOS:
                break;
            default:
                printf("flag is unknown, read sample break");
                break;
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
    fd_ = OpenFile(mp4Path2);
    int64_t size = GetFileSize(mp4Path2);
    printf("---- %s ------\n", mp4Path2.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, nb_streams));
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize);
    ASSERT_NE(sharedMem_, nullptr);
    SetInitValue();
    while (!isEOS(eosFlag)) {
        for (auto idx : selectedTrackIds_) {
            ASSERT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx, flag_);
        }
    }
    printf("frames[0]=%d | kFrames[0]=%d\n", frames[0], keyFrames[0]);
    printf("frames[1]=%d | kFrames[1]=%d\n", frames[1], keyFrames[1]);
    ASSERT_EQ(frames[0], 103);
    ASSERT_EQ(frames[1], 174);
    ASSERT_EQ(keyFrames[0], 5);
    ASSERT_EQ(keyFrames[1], 174);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_1070
 * @tc.desc: copy current sample to buffer
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_1070, TestSize.Level1)
{
    fd_ = OpenFile(mkvPath2);
    int64_t size = GetFileSize(mkvPath2);
    printf("---- %s ------\n", mkvPath2.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, nb_streams));
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize);
    ASSERT_NE(sharedMem_, nullptr);
    SetInitValue();
    while (!isEOS(eosFlag)) {
        for (auto idx : selectedTrackIds_) {
            ASSERT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx, flag_);
        }
    }
    printf("frames[0]=%d | kFrames[0]=%d\n", frames[0], keyFrames[0]);
    printf("frames[1]=%d | kFrames[1]=%d\n", frames[1], keyFrames[1]);
    ASSERT_EQ(frames[0], 240);
    ASSERT_EQ(frames[1], 199);
    ASSERT_EQ(keyFrames[0], 4);
    ASSERT_EQ(keyFrames[1], 199);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_1090
 * @tc.desc: copy current sample to buffer
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_1090, TestSize.Level1)
{
    fd_ = OpenFile(tsPath);
    int64_t size = GetFileSize(tsPath);
    printf("---- %s ------\n", tsPath.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, nb_streams));
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize);
    ASSERT_NE(sharedMem_, nullptr);
    SetInitValue();
    while (!isEOS(eosFlag)) {
        for (auto idx : selectedTrackIds_) {
            ASSERT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx, flag_);
        }
    }
    printf("frames[0]=%d | kFrames[0]=%d\n", frames[0], keyFrames[0]);
    printf("frames[1]=%d | kFrames[1]=%d\n", frames[1], keyFrames[1]);
    ASSERT_EQ(frames[0], 103);
    ASSERT_EQ(frames[1], 174);
    ASSERT_EQ(keyFrames[0], 5);
    ASSERT_EQ(keyFrames[1], 174);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_1100
 * @tc.desc: copy current sample to buffer
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_1100, TestSize.Level1)
{
    fd_ = OpenFile(aacPath);
    int64_t size = GetFileSize(aacPath);
    printf("---- %s ------\n", aacPath.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, nb_streams));
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize);
    ASSERT_NE(sharedMem_, nullptr);
    SetInitValue();
    uint32_t idx = 0;
    while (!isEOS(eosFlag)) {
        ASSERT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
        CountFrames(idx, flag_);
    }
    printf("frames[0]=%d | kFrames[0]=%d\n", frames[0], keyFrames[0]);
    ASSERT_EQ(frames[0], 1293);
    ASSERT_EQ(keyFrames[0], 1293);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_1110
 * @tc.desc: copy current sample to buffer
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_1110, TestSize.Level1)
{
    fd_ = OpenFile(flacPath);
    int64_t size = GetFileSize(flacPath);
    printf("---- %s ------\n", flacPath.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, nb_streams));
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize);
    ASSERT_NE(sharedMem_, nullptr);
    SetInitValue();
    uint32_t idx = 0;
    while (!isEOS(eosFlag)) {
        ASSERT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
        CountFrames(idx, flag_);
    }
    printf("frames[0]=%d | kFrames[0]=%d\n", frames[0], keyFrames[0]);
    ASSERT_EQ(frames[0], 313);
    ASSERT_EQ(keyFrames[0], 313);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_1120
 * @tc.desc: copy current sample to buffer
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_1120, TestSize.Level1)
{
    fd_ = OpenFile(m4aPath);
    int64_t size = GetFileSize(m4aPath);
    printf("---- %s ------\n", m4aPath.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, nb_streams));
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize);
    ASSERT_NE(sharedMem_, nullptr);
    SetInitValue();
    uint32_t idx = 0;
    while (!isEOS(eosFlag)) {
        ASSERT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
        CountFrames(idx, flag_);
    }
    printf("frames[0]=%d | kFrames[0]=%d\n", frames[0], keyFrames[0]);
    ASSERT_EQ(frames[0], 1408);
    ASSERT_EQ(keyFrames[0], 1408);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_1130
 * @tc.desc: copy current sample to buffer
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_1130, TestSize.Level1)
{
    fd_ = OpenFile(mp3Path);
    int64_t size = GetFileSize(mp3Path);
    printf("---- %s ------\n", mp3Path.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, nb_streams));
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize);
    ASSERT_NE(sharedMem_, nullptr);
    SetInitValue();
    uint32_t idx = 0;
    while (!isEOS(eosFlag)) {
        ASSERT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
        CountFrames(idx, flag_);
    }
    printf("frames[0]=%d | kFrames[0]=%d\n", frames[0], keyFrames[0]);
    ASSERT_EQ(frames[0], 1251);
    ASSERT_EQ(keyFrames[0], 1251);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_1140
 * @tc.desc: copy current sample to buffer
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_1140, TestSize.Level1)
{
    fd_ = OpenFile(oggPath);
    int64_t size = GetFileSize(oggPath);
    printf("---- %s ------\n", oggPath.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, nb_streams));
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize);
    ASSERT_NE(sharedMem_, nullptr);
    SetInitValue();
    uint32_t idx = 0;
    while (!isEOS(eosFlag)) {
        ASSERT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
        CountFrames(idx, flag_);
    }
    printf("frames[0]=%d | kFrames[0]=%d\n", frames[0], keyFrames[0]);
    ASSERT_EQ(frames[0], 1598);
    ASSERT_EQ(keyFrames[0], 1598);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_1150
 * @tc.desc: copy current sample to buffer
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_1150, TestSize.Level1)
{
    fd_ = OpenFile(wavPath);
    int64_t size = GetFileSize(wavPath);
    printf("---- %s ------\n", wavPath.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, nb_streams));
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize);
    ASSERT_NE(sharedMem_, nullptr);
    SetInitValue();
    uint32_t idx = 0;
    while (!isEOS(eosFlag)) {
        ASSERT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
        CountFrames(idx, flag_);
    }
    printf("frames[0]=%d | kFrames[0]=%d\n", frames[0], keyFrames[0]);
    ASSERT_EQ(frames[0], 704);
    ASSERT_EQ(keyFrames[0], 704);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_1160
 * @tc.desc: copy current sample to buffer
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_1160, TestSize.Level1)
{
    fd_ = OpenFile(amrPath);
    int64_t size = GetFileSize(amrPath);
    printf("---- %s ------\n", amrPath.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, nb_streams));
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize);
    ASSERT_NE(sharedMem_, nullptr);
    SetInitValue();
    uint32_t idx = 0;
    while (!isEOS(eosFlag)) {
        ASSERT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
        CountFrames(idx, flag_);
    }
    printf("frames[0]=%d | kFrames[0]=%d\n", frames[0], keyFrames[0]);
    ASSERT_EQ(frames[0], 1501);
    ASSERT_EQ(keyFrames[0], 1501);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_1170
 * @tc.desc: copy current sample to buffer
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_1170, TestSize.Level1)
{
    fd_ = OpenFile(amrPath2);
    int64_t size = GetFileSize(amrPath2);
    printf("---- %s ------\n", amrPath2.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, nb_streams));
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize);
    ASSERT_NE(sharedMem_, nullptr);
    SetInitValue();
    uint32_t idx = 0;
    while (!isEOS(eosFlag)) {
        ASSERT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
        CountFrames(idx, flag_);
    }
    printf("frames[0]=%d | kFrames[0]=%d\n", frames[0], keyFrames[0]);
    ASSERT_EQ(frames[0], 1500);
    ASSERT_EQ(keyFrames[0], 1500);
    RemoveValue();
}


/**
 * @tc.name: Demuxer_SeekToTime_1000
 * @tc.desc: seek to the specified time
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_1000, TestSize.Level1)
{
    fd_ = OpenFile(mp4Path);
    int64_t size = GetFileSize(mp4Path);
    printf("---- %s ------\n", mp4Path.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, nb_streams));
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    list<int64_t> toPtsList = {0, 2000, 1920, 2160, 2200, 2440, 2600, 2700, 4080, 4100}; // ms
    vector<int32_t> audioVals = {174, 174, 174, 90, 91, 91, 90, 134, 90, 47, 91, 91, 47, 91, 91, 47, 91, 47, 47, 91, 47,
        47, 91, 47, 5, 5, 5, 5};
    vector<int32_t> videoVals = {103, 103, 103, 53, 53, 53, 53, 78, 53, 28, 53, 53, 28, 53, 53, 28, 53, 28, 28, 53, 28,
        28, 53, 28, 3, 3, 3, 3};
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize);
            ASSERT_NE(sharedMem_, nullptr);
            ret = demuxer_->SeekToTime(*toPts, *mode);
            if (ret != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames[0]=%d | kFrames[0]=%d\n", *toPts, frames[0], keyFrames[0]);
            printf("time = %" PRId64 " | frames[1]=%d | kFrames[1]=%d\n", *toPts, frames[1], keyFrames[1]);
            ASSERT_EQ(frames[0], videoVals[number]);
            ASSERT_EQ(frames[1], audioVals[number]);
            number += 1;
            RemoveValue();
            selectedTrackIds_.clear();
            if (sharedMem_ != nullptr) {
                sharedMem_->Destory();
                sharedMem_ = nullptr;
            }
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
    fd_ = OpenFile(mp4Path);
    int64_t size = GetFileSize(mp4Path);
    printf("---- %s ------\n", mp4Path.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, nb_streams));
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    list<int64_t> toPtsList = {-100, 1000000}; // ms
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize);
        ASSERT_NE(sharedMem_, nullptr);
        ret = demuxer_->SeekToTime(*toPts, AVSeekMode::SEEK_MODE_NEXT_SYNC);
        ASSERT_NE(ret, AV_ERR_OK);
        ret = demuxer_->SeekToTime(*toPts, AVSeekMode::SEEK_MODE_PREVIOUS_SYNC);
        ASSERT_NE(ret, AV_ERR_OK);
        ret = demuxer_->SeekToTime(*toPts, AVSeekMode::SEEK_MODE_CLOSEST_SYNC);
        ASSERT_NE(ret, AV_ERR_OK);
        if (sharedMem_ != nullptr) {
            sharedMem_->Destory();
            sharedMem_ = nullptr;
        }
    }
}

/**
 * @tc.name: Demuxer_SeekToTime_1002
 * @tc.desc: seek to the specified time
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_1002, TestSize.Level1)
{
    fd_ = OpenFile(mp4Path);
    int64_t size = GetFileSize(mp4Path);
    printf("---- %s ------\n", mp4Path.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, nb_streams));
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize);
    ASSERT_NE(sharedMem_, nullptr);
    int readNum = 121;
    int64_t seek_time = 0;
    ReadData(readNum, seek_time);
    seek_time = (seek_time / 1000) + 500;
    ASSERT_EQ(demuxer_->SeekToTime(seek_time, AVSeekMode::SEEK_MODE_NEXT_SYNC), AV_ERR_OK);
    ASSERT_EQ(demuxer_->ReadSample(0, sharedMem_, &info_, flag_), AV_ERR_OK);
    printf("time = %" PRId64 " | pts = %" PRId64 "\n", seek_time, info_.presentationTimeUs);
}

/**
 * @tc.name: Demuxer_SeekToTime_1010
 * @tc.desc: seek to the specified time
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_1010, TestSize.Level1)
{
    fd_ = OpenFile(mp4Path2);
    int64_t size = GetFileSize(mp4Path2);
    printf("---- %s ------\n", mp4Path2.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, nb_streams));
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    list<int64_t> toPtsList = {0, 3000, 2040, 1880, 1960, 2400, 2720, 2830, 4040, 4100}; // ms
    vector<int32_t> audioVals = {174, 174, 174, 7, 49, 49, 48, 91, 91, 90, 132, 90, 90, 91, 91, 48, 91, 91, 48, 91, 48,
        48, 91, 48, 8, 8, 8, 8};
    vector<int32_t> videoVals = {103, 103, 103, 6, 30, 30, 30, 54, 54, 54, 78, 54, 54, 54, 54, 30, 54, 54, 30, 54, 30,
        30, 54, 30, 6, 6, 6, 6};
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize);
            ASSERT_NE(sharedMem_, nullptr);
            ret = demuxer_->SeekToTime(*toPts, *mode);
            if (ret != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames[0]=%d | kFrames[0]=%d\n", *toPts, frames[0], keyFrames[0]);
            printf("time = %" PRId64 " | frames[1]=%d | kFrames[1]=%d\n", *toPts, frames[1], keyFrames[1]);
            ASSERT_EQ(frames[0], videoVals[number]);
            ASSERT_EQ(frames[1], audioVals[number]);
            number += 1;
            RemoveValue();
            selectedTrackIds_.clear();
            if (sharedMem_ != nullptr) {
                sharedMem_->Destory();
                sharedMem_ = nullptr;
            }
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
    fd_ = OpenFile(mkvPath2);
    int64_t size = GetFileSize(mkvPath2);
    printf("---- %s ------\n", mkvPath2.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, nb_streams));
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    list<int64_t> toPtsList = {0, 1000, 1017, 1500, 1700, 1940, 3983, 1983, 3990}; // ms
    vector<int32_t> audioVals = {199, 199, 199, 149, 149, 149, 99, 149, 149, 99, 149, 149, 99, 149, 99, 99, 149, 99,
        49, 49, 99, 149, 99, 49, 49};
    vector<int32_t> videoVals = {240, 240, 240, 180, 180, 180, 120, 180, 180, 120, 180, 180, 120, 180, 120, 120, 180,
        120, 60, 60, 120, 180, 120, 60, 60};
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize);
            ASSERT_NE(sharedMem_, nullptr);
            ret = demuxer_->SeekToTime(*toPts, *mode);
            if (ret != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames[0]=%d | kFrames[0]=%d\n", *toPts, frames[0], keyFrames[0]);
            printf("time = %" PRId64 " | frames[1]=%d | kFrames[1]=%d\n", *toPts, frames[1], keyFrames[1]);
            ASSERT_EQ(frames[0], videoVals[number]);
            ASSERT_EQ(frames[1], audioVals[number]);
            number += 1;
            RemoveValue();
            selectedTrackIds_.clear();
            if (sharedMem_ != nullptr) {
                sharedMem_->Destory();
                sharedMem_ = nullptr;
            }
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
    fd_ = OpenFile(tsPath);
    int64_t size = GetFileSize(tsPath);
    printf("---- %s ------\n", tsPath.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, nb_streams));
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    list<int64_t> toPtsList = {0, 3480, 3640, 3320, 3000, 3100, 4120, 5520}; // ms
    vector<int32_t> videoVals = {102, 102, 102, 15, 15, 15, 11, 11, 11, 19, 19, 19, 27, 27, 27, 24, 25, 25, 1, 1, 1};
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize);
            ASSERT_NE(sharedMem_, nullptr);
            ret = demuxer_->SeekToTime(*toPts, *mode);
            if (ret != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames[0]=%d | kFrames[0]=%d\n", *toPts, frames[0], keyFrames[0]);
            ASSERT_EQ(frames[0], videoVals[number]);
            number += 1;
            RemoveValue();
            selectedTrackIds_.clear();
            if (sharedMem_ != nullptr) {
                sharedMem_->Destory();
                sharedMem_ = nullptr;
            }
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
    fd_ = OpenFile(aacPath);
    int64_t size = GetFileSize(aacPath);
    printf("---- %s ------\n", aacPath.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, nb_streams));
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    list<int64_t> toPtsList = {0, 10240, 10230, 10220, 30000, 30010}; // ms
    vector<int32_t> audioVals = {1293, 1293, 1293, 852, 852, 852, 853, 853, 853, 853, 853, 853, 2, 2, 2, 1, 1, 1};
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize);
            ASSERT_NE(sharedMem_, nullptr);
            ret = demuxer_->SeekToTime(*toPts, *mode);
            if (ret != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames[0]=%d | kFrames[0]=%d\n", *toPts, frames[0], keyFrames[0]);
            ASSERT_EQ(frames[0], audioVals[number]);
            number += 1;
            RemoveValue();
            selectedTrackIds_.clear();
            if (sharedMem_ != nullptr) {
                sharedMem_->Destory();
                sharedMem_ = nullptr;
            }
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
    fd_ = OpenFile(flacPath);
    int64_t size = GetFileSize(flacPath);
    printf("---- %s ------\n", flacPath.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, nb_streams));
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    list<int64_t> toPtsList = {0, 3072, 4031, 4035, 29952, 29953}; // ms
    vector<int32_t> audioVals = {313, 313, 313, 281, 281, 281, 272, 272, 272, 271, 271, 271, 1, 1, 1, 2, 2, 2};
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize);
            ASSERT_NE(sharedMem_, nullptr);
            ret = demuxer_->SeekToTime(*toPts, *mode);
            if (ret != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames[0]=%d | kFrames[0]=%d\n", *toPts, frames[0], keyFrames[0]);
            ASSERT_EQ(frames[0], audioVals[number]);
            number += 1;
            RemoveValue();
            selectedTrackIds_.clear();
            if (sharedMem_ != nullptr) {
                sharedMem_->Destory();
                sharedMem_ = nullptr;
            }
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
    fd_ = OpenFile(m4aPath);
    int64_t size = GetFileSize(m4aPath);
    printf("---- %s ------\n", m4aPath.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, nb_streams));
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    list<int64_t> toPtsList = {0, 14592, 15297, 15290, 29994, 29998}; // ms
    vector<int32_t> audioVals = {1407, 1407, 1407, 723, 723, 723, 690, 690, 690, 691, 691, 691, 2, 2, 2, 1, 1, 1};
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize);
            ASSERT_NE(sharedMem_, nullptr);
            ret = demuxer_->SeekToTime(*toPts, *mode);
            if (ret != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames[0]=%d | kFrames[0]=%d\n", *toPts, frames[0], keyFrames[0]);
            ASSERT_EQ(frames[0], audioVals[number]);
            number += 1;
            RemoveValue();
            selectedTrackIds_.clear();
            if (sharedMem_ != nullptr) {
                sharedMem_->Destory();
                sharedMem_ = nullptr;
            }
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
    fd_ = OpenFile(mp3Path);
    int64_t size = GetFileSize(mp3Path);
    printf("---- %s ------\n", mp3Path.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, nb_streams));
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    list<int64_t> toPtsList = {0, 4128, 11980, 11990, 30000, 30010}; // ms
    vector<int32_t> audioVals = {1251, 1251, 1251, 1079, 1079, 1079, 752, 752, 752, 752, 752, 752, 1, 1, 1, 1, 1, 1};
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize);
            ASSERT_NE(sharedMem_, nullptr);
            ret = demuxer_->SeekToTime(*toPts, *mode);
            if (ret != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames[0]=%d | kFrames[0]=%d\n", *toPts, frames[0], keyFrames[0]);
            ASSERT_EQ(frames[0], audioVals[number]);
            number += 1;
            RemoveValue();
            selectedTrackIds_.clear();
            if (sharedMem_ != nullptr) {
                sharedMem_->Destory();
                sharedMem_ = nullptr;
            }
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
    fd_ = OpenFile(oggPath);
    int64_t size = GetFileSize(oggPath);
    printf("---- %s ------\n", oggPath.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, nb_streams));
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    list<int64_t> toPtsList = {0, 5868, 5548, 5292, 29992, 29999}; // ms
    vector<int32_t> audioVals = {1598, 1598, 1598, 1357, 1357, 1357, 1357, 1357, 1357, 1357, 1357, 1357, 46, 46,
        46, 46, 46, 46};
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize);
            ASSERT_NE(sharedMem_, nullptr);
            ret = demuxer_->SeekToTime(*toPts, *mode);
            if (ret != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames[0]=%d | kFrames[0]=%d\n", *toPts, frames[0], keyFrames[0]);
            ASSERT_EQ(frames[0], audioVals[number]);
            number += 1;
            RemoveValue();
            selectedTrackIds_.clear();
            if (sharedMem_ != nullptr) {
                sharedMem_->Destory();
                sharedMem_ = nullptr;
            }
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
    fd_ = OpenFile(wavPath);
    int64_t size = GetFileSize(wavPath);
    printf("---- %s ------\n", wavPath.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, nb_streams));
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    list<int64_t> toPtsList = {0, 8576, 8566, 8578, 29994, 30000}; // ms
    vector<int32_t> audioVals = {704, 704, 704, 503, 503, 503, 504, 504, 504, 503, 503, 503, 2, 2, 2, 1, 1, 1};
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize);
            ASSERT_NE(sharedMem_, nullptr);
            ret = demuxer_->SeekToTime(*toPts, *mode);
            if (ret != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames[0]=%d | kFrames[0]=%d\n", *toPts, frames[0], keyFrames[0]);
            ASSERT_EQ(frames[0], audioVals[number]);
            number += 1;
            RemoveValue();
            selectedTrackIds_.clear();
            if (sharedMem_ != nullptr) {
                sharedMem_->Destory();
                sharedMem_ = nullptr;
            }
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
    fd_ = OpenFile(amrPath);
    int64_t size = GetFileSize(amrPath);
    printf("---- %s ------\n", amrPath.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, nb_streams));
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    list<int64_t> toPtsList = {0, 8560, 8550, 8570, 30000, 30900}; // ms
    vector<int32_t> audioVals = {1501, 1501, 1501, 1073, 1073, 1073, 1074, 1074, 1074, 1073, 1073, 1073,
        1, 1, 1, 1, 1, 1};
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize);
            ASSERT_NE(sharedMem_, nullptr);
            ret = demuxer_->SeekToTime(*toPts, *mode);
            if (ret != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames[0]=%d | kFrames[0]=%d\n", *toPts, frames[0], keyFrames[0]);
            ASSERT_EQ(frames[0], audioVals[number]);
            number += 1;
            RemoveValue();
            selectedTrackIds_.clear();
            if (sharedMem_ != nullptr) {
                sharedMem_->Destory();
                sharedMem_ = nullptr;
            }
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
    fd_ = OpenFile(amrPath2);
    int64_t size = GetFileSize(amrPath2);
    printf("---- %s ------\n", amrPath2.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, nb_streams));
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    list<int64_t> toPtsList = {0, 11920, 11910, 11930, 29980, 30800}; // ms
    vector<int32_t> audioVals = {1500, 1500, 1500, 904, 904, 904, 905, 905, 905, 904, 904, 904,
        1, 1, 1, 1, 1, 1};
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize);
            ASSERT_NE(sharedMem_, nullptr);
            ret = demuxer_->SeekToTime(*toPts, *mode);
            if (ret != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames[0]=%d | kFrames[0]=%d\n", *toPts, frames[0], keyFrames[0]);
            ASSERT_EQ(frames[0], audioVals[number]);
            number += 1;
            RemoveValue();
            selectedTrackIds_.clear();
            if (sharedMem_ != nullptr) {
                sharedMem_->Destory();
                sharedMem_ = nullptr;
            }
        }
    }
}

// ######################################demuxer uri###############################################
/**
 * @tc.name: Demuxer_CreateDemuxer_2000
 * @tc.desc: create demuxer(URI)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CreateDemuxer_2000, TestSize.Level1)
{
    printf("---- %s ------\n", mp4Uri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(mp4Uri.data()));
    ASSERT_NE(source_, nullptr);
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
    ASSERT_NE(demuxer_, nullptr);
}

/**
 * @tc.name: Demuxer_CreateDemuxer_2010
 * @tc.desc: repeatedly create demuxer
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CreateDemuxer_2010, TestSize.Level1)
{
    printf("---- %s ------\n", mp4Uri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(mp4Uri.data()));
    ASSERT_NE(source_, nullptr);
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
    ASSERT_NE(demuxer_, nullptr);
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
    ASSERT_NE(demuxer_, nullptr);
}

/**
 * @tc.name: Demuxer_CreateDemuxer_2020
 * @tc.desc: create demuxer
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_CreateDemuxer_2300, TestSize.Level1)
{
    printf("---- %s ------\n", mp4Uri4.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(mp4Uri4.data()));
    ASSERT_NE(source_, nullptr);
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
    ASSERT_NE(demuxer_, nullptr);
}

/**
 * @tc.name: Demuxer_UnselectTrackByID_2000
 * @tc.desc: select and remove track by ID
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_UnselectTrackByID_2000, TestSize.Level1)
{
    printf("---- %s ------\n", mp4Uri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(mp4Uri.data()));
    ASSERT_NE(source_, nullptr);
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
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
 * @tc.name: Demuxer_ReadSample_2000
 * @tc.desc: copy current sample to buffer
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_2000, TestSize.Level1)
{
    printf("---- %s ------\n", mp4Uri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(mp4Uri.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, nb_streams));
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize);
    ASSERT_NE(sharedMem_, nullptr);
    SetInitValue();
    while (!isEOS(eosFlag)) {
        for (auto idx : selectedTrackIds_) {
            ASSERT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx, flag_);
        }
    }
    printf("frames[0]=%d | kFrames[0]=%d\n", frames[0], keyFrames[0]);
    printf("frames[1]=%d | kFrames[1]=%d\n", frames[1], keyFrames[1]);
    ASSERT_EQ(frames[0], 103);
    ASSERT_EQ(frames[1], 174);
    ASSERT_EQ(keyFrames[0], 5);
    ASSERT_EQ(keyFrames[1], 174);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_2010
 * @tc.desc: copy current sample to buffer
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_2010, TestSize.Level1)
{
    printf("---- %s ------\n", mp4Uri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(mp4Uri.data()));
    ASSERT_NE(source_, nullptr);
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize);
    ASSERT_NE(sharedMem_, nullptr);
    uint32_t idx = 4;
    ASSERT_NE(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
}

/**
 * @tc.name: Demuxer_ReadSample_2020
 * @tc.desc: copy current sample to buffer
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_2020, TestSize.Level1)
{
    printf("---- %s ------\n", mp4Uri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(mp4Uri.data()));
    ASSERT_NE(source_, nullptr);
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize);
    ASSERT_NE(sharedMem_, nullptr);
    uint32_t idx = -1;
    ASSERT_NE(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
}

/**
 * @tc.name: Demuxer_ReadSample_2030
 * @tc.desc: copy current sample to buffer
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_2030, TestSize.Level1)
{
    printf("---- %s ------\n", mp4Uri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(mp4Uri.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, nb_streams));
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize);
    ASSERT_NE(sharedMem_, nullptr);
    int32_t vkeyFrames = 0;
    int32_t vframes = 0;
    flag_ = AVCodecBufferFlag::AVCODEC_BUFFER_FLAG_NONE;
    while (flag_ != AVCodecBufferFlag::AVCODEC_BUFFER_FLAG_EOS) {
        ASSERT_EQ(demuxer_->ReadSample(0, sharedMem_, &info_, flag_), AV_ERR_OK);
        switch (flag_) {
            case AVCodecBufferFlag::AVCODEC_BUFFER_FLAG_SYNC_FRAME:
                vkeyFrames++;
                vframes++;
                break;
            case AVCodecBufferFlag::AVCODEC_BUFFER_FLAG_NONE:
                vframes++;
                break;
            case AVCodecBufferFlag::AVCODEC_BUFFER_FLAG_EOS:
                break;
            default:
                printf("flag is unknown, read sample break");
                break;
        }
    }
    printf("vframes=%d | vkFrames=%d\n", vframes, vkeyFrames);
    ASSERT_EQ(vframes, 103);
    ASSERT_EQ(vkeyFrames, 5);
}

/**
 * @tc.name: Demuxer_ReadSample_2040
 * @tc.desc: copy current sample to buffer
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_2040, TestSize.Level1)
{
    printf("---- %s ------\n", mp4Uri2.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(mp4Uri2.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, nb_streams));
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize);
    ASSERT_NE(sharedMem_, nullptr);
    SetInitValue();
    while (!isEOS(eosFlag)) {
        for (auto idx : selectedTrackIds_) {
            ASSERT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx, flag_);
        }
    }
    printf("frames[0]=%d | kFrames[0]=%d\n", frames[0], keyFrames[0]);
    printf("frames[1]=%d | kFrames[1]=%d\n", frames[1], keyFrames[1]);
    ASSERT_EQ(frames[0], 103);
    ASSERT_EQ(frames[1], 174);
    ASSERT_EQ(keyFrames[0], 5);
    ASSERT_EQ(keyFrames[1], 174);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_2060
 * @tc.desc: copy current sample to buffer
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_2060, TestSize.Level1)
{
    printf("---- %s ------\n", mkvUri2.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(mkvUri2.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, nb_streams));
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize);
    ASSERT_NE(sharedMem_, nullptr);
    SetInitValue();
    while (!isEOS(eosFlag)) {
        for (auto idx : selectedTrackIds_) {
            ASSERT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx, flag_);
        }
    }
    printf("frames[0]=%d | kFrames[0]=%d\n", frames[0], keyFrames[0]);
    printf("frames[1]=%d | kFrames[1]=%d\n", frames[1], keyFrames[1]);
    ASSERT_EQ(frames[0], 240);
    ASSERT_EQ(frames[1], 199);
    ASSERT_EQ(keyFrames[0], 4);
    ASSERT_EQ(keyFrames[1], 199);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_2070
 * @tc.desc: copy current sample to buffer
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_2070, TestSize.Level1)
{
    printf("---- %s ------\n", tsUri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(tsUri.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, nb_streams));
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize);
    ASSERT_NE(sharedMem_, nullptr);
    SetInitValue();
    while (!isEOS(eosFlag)) {
        for (auto idx : selectedTrackIds_) {
            ASSERT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx, flag_);
        }
    }
    printf("frames[0]=%d | kFrames[0]=%d\n", frames[0], keyFrames[0]);
    printf("frames[1]=%d | kFrames[1]=%d\n", frames[1], keyFrames[1]);
    ASSERT_EQ(frames[0], 103);
    ASSERT_EQ(frames[1], 174);
    ASSERT_EQ(keyFrames[0], 5);
    ASSERT_EQ(keyFrames[1], 174);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_2080
 * @tc.desc: copy current sample to buffer
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_2080, TestSize.Level1)
{
    printf("---- %s ------\n", aacUri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(aacUri.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, nb_streams));
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize);
    ASSERT_NE(sharedMem_, nullptr);
    SetInitValue();
    uint32_t idx = 0;
    while (!isEOS(eosFlag)) {
        ASSERT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
        CountFrames(idx, flag_);
    }
    printf("frames[0]=%d | kFrames[0]=%d\n", frames[0], keyFrames[0]);
    ASSERT_EQ(frames[0], 1293);
    ASSERT_EQ(keyFrames[0], 1293);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_2090
 * @tc.desc: copy current sample to buffer
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_2090, TestSize.Level1)
{
    printf("---- %s ------\n", flacUri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(flacUri.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, nb_streams));
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize);
    ASSERT_NE(sharedMem_, nullptr);
    SetInitValue();
    uint32_t idx = 0;
    while (!isEOS(eosFlag)) {
        ASSERT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
        CountFrames(idx, flag_);
    }
    printf("frames[0]=%d | kFrames[0]=%d\n", frames[0], keyFrames[0]);
    ASSERT_EQ(frames[0], 313);
    ASSERT_EQ(keyFrames[0], 313);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_2100
 * @tc.desc: copy current sample to buffer
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_2100, TestSize.Level1)
{
    printf("---- %s ------\n", m4aUri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(m4aUri.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, nb_streams));
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize);
    ASSERT_NE(sharedMem_, nullptr);
    SetInitValue();
    uint32_t idx = 0;
    while (!isEOS(eosFlag)) {
        ASSERT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
        CountFrames(idx, flag_);
    }
    printf("frames[0]=%d | kFrames[0]=%d\n", frames[0], keyFrames[0]);
    ASSERT_EQ(frames[0], 1408);
    ASSERT_EQ(keyFrames[0], 1408);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_2110
 * @tc.desc: copy current sample to buffer
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_2110, TestSize.Level1)
{
    printf("---- %s ------\n", mp3Uri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(mp3Uri.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, nb_streams));
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize);
    ASSERT_NE(sharedMem_, nullptr);
    SetInitValue();
    uint32_t idx = 0;
    while (!isEOS(eosFlag)) {
        ASSERT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
        CountFrames(idx, flag_);
    }
    printf("frames[0]=%d | kFrames[0]=%d\n", frames[0], keyFrames[0]);
    ASSERT_EQ(frames[0], 1251);
    ASSERT_EQ(keyFrames[0], 1251);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_2120
 * @tc.desc: copy current sample to buffer
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_2120, TestSize.Level1)
{
    printf("---- %s ------\n", oggUri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(oggUri.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, nb_streams));
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize);
    ASSERT_NE(sharedMem_, nullptr);
    SetInitValue();
    uint32_t idx = 0;
    while (!isEOS(eosFlag)) {
        ASSERT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
        CountFrames(idx, flag_);
    }
    printf("frames[0]=%d | kFrames[0]=%d\n", frames[0], keyFrames[0]);
    ASSERT_EQ(frames[0], 1598);
    ASSERT_EQ(keyFrames[0], 1598);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_2130
 * @tc.desc: copy current sample to buffer
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_2130, TestSize.Level1)
{
    printf("---- %s ------\n", wavUri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(wavUri.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, nb_streams));
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize);
    ASSERT_NE(sharedMem_, nullptr);
    SetInitValue();
    uint32_t idx = 0;
    while (!isEOS(eosFlag)) {
        ASSERT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
        CountFrames(idx, flag_);
    }
    printf("frames[0]=%d | kFrames[0]=%d\n", frames[0], keyFrames[0]);
    ASSERT_EQ(frames[0], 704);
    ASSERT_EQ(keyFrames[0], 704);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_2140
 * @tc.desc: copy current sample to buffer
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_2140, TestSize.Level1)
{
    printf("---- %s ------\n", amrUri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(amrUri.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, nb_streams));
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize);
    ASSERT_NE(sharedMem_, nullptr);
    SetInitValue();
    uint32_t idx = 0;
    while (!isEOS(eosFlag)) {
        ASSERT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
        CountFrames(idx, flag_);
    }
    printf("frames[0]=%d | kFrames[0]=%d\n", frames[0], keyFrames[0]);
    ASSERT_EQ(frames[0], 1501);
    ASSERT_EQ(keyFrames[0], 1501);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_SeekToTime_2000
 * @tc.desc: seek to the specified time
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_2000, TestSize.Level1)
{
    printf("---- %s ------\n", mp4Uri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(mp4Uri.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, nb_streams));
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    list<int64_t> toPtsList = {0, 2000, 1920, 2160, 2200, 2440, 2600, 2700, 4080, 4100}; // ms
    vector<int32_t> audioVals = {174, 174, 174, 90, 91, 91, 90, 134, 90, 47, 91, 91, 47, 91, 91, 47, 91, 47, 47, 91, 47,
        47, 91, 47, 5, 5, 5, 5};
    vector<int32_t> videoVals = {103, 103, 103, 53, 53, 53, 53, 78, 53, 28, 53, 53, 28, 53, 53, 28, 53, 28, 28, 53, 28,
        28, 53, 28, 3, 3, 3, 3};
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize);
            ASSERT_NE(sharedMem_, nullptr);
            ret = demuxer_->SeekToTime(*toPts, *mode);
            if (ret != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames[0]=%d | kFrames[0]=%d\n", *toPts, frames[0], keyFrames[0]);
            printf("time = %" PRId64 " | frames[1]=%d | kFrames[1]=%d\n", *toPts, frames[1], keyFrames[1]);
            ASSERT_EQ(frames[0], videoVals[number]);
            ASSERT_EQ(frames[1], audioVals[number]);
            number += 1;
            RemoveValue();
            selectedTrackIds_.clear();
            if (sharedMem_ != nullptr) {
                sharedMem_->Destory();
                sharedMem_ = nullptr;
            }
        }
    }
}

/**
 * @tc.name: Demuxer_SeekToTime_2001
 * @tc.desc: seek to the specified time
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_2001, TestSize.Level1)
{
    printf("---- %s ------\n", mp4Uri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(mp4Uri.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, nb_streams));
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    list<int64_t> toPtsList = {-100, 1000000}; // ms
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize);
        ASSERT_NE(sharedMem_, nullptr);
        ret = demuxer_->SeekToTime(*toPts, AVSeekMode::SEEK_MODE_NEXT_SYNC);
        ASSERT_NE(ret, AV_ERR_OK);
        ret = demuxer_->SeekToTime(*toPts, AVSeekMode::SEEK_MODE_PREVIOUS_SYNC);
        ASSERT_NE(ret, AV_ERR_OK);
        ret = demuxer_->SeekToTime(*toPts, AVSeekMode::SEEK_MODE_CLOSEST_SYNC);
        ASSERT_NE(ret, AV_ERR_OK);
        if (sharedMem_ != nullptr) {
            sharedMem_->Destory();
            sharedMem_ = nullptr;
        }
    }
}

/**
 * @tc.name: Demuxer_SeekToTime_2002
 * @tc.desc: seek to the specified time
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_2002, TestSize.Level1)
{
    printf("---- %s ------\n", mp4Uri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(mp4Uri.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, nb_streams));
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize);
    ASSERT_NE(sharedMem_, nullptr);
    int readNum = 121;
    int64_t seek_time = 0;
    ReadData(readNum, seek_time);
    seek_time = (seek_time / 1000) + 500;
    ASSERT_EQ(demuxer_->SeekToTime(seek_time, AVSeekMode::SEEK_MODE_NEXT_SYNC), AV_ERR_OK);
    ASSERT_EQ(demuxer_->ReadSample(0, sharedMem_, &info_, flag_), AV_ERR_OK);
    printf("time = %" PRId64 " | pts = %" PRId64 "\n", seek_time, info_.presentationTimeUs);
}

/**
 * @tc.name: Demuxer_SeekToTime_2010
 * @tc.desc: seek to the specified time
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_2010, TestSize.Level1)
{
    printf("---- %s ------\n", mp4Uri2.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(mp4Uri2.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, nb_streams));
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    list<int64_t> toPtsList = {0, 3000, 2040, 1880, 1960, 2400, 2720, 2830, 4040, 4100}; // ms
    vector<int32_t> audioVals = {174, 174, 174, 7, 49, 49, 48, 91, 91, 90, 132, 90, 90, 91, 91, 48, 91, 91, 48, 91, 48,
        48, 91, 48, 8, 8, 8, 8};
    vector<int32_t> videoVals = {103, 103, 103, 6, 30, 30, 30, 54, 54, 54, 78, 54, 54, 54, 54, 30, 54, 54, 30, 54, 30,
        30, 54, 30, 6, 6, 6, 6};
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize);
            ASSERT_NE(sharedMem_, nullptr);
            ret = demuxer_->SeekToTime(*toPts, *mode);
            if (ret != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames[0]=%d | kFrames[0]=%d\n", *toPts, frames[0], keyFrames[0]);
            printf("time = %" PRId64 " | frames[1]=%d | kFrames[1]=%d\n", *toPts, frames[1], keyFrames[1]);
            ASSERT_EQ(frames[0], videoVals[number]);
            ASSERT_EQ(frames[1], audioVals[number]);
            number += 1;
            RemoveValue();
            selectedTrackIds_.clear();
            if (sharedMem_ != nullptr) {
                sharedMem_->Destory();
                sharedMem_ = nullptr;
            }
        }
    }
}

/**
 * @tc.name: Demuxer_SeekToTime_2040
 * @tc.desc: seek to the specified time
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_2040, TestSize.Level1)
{
    printf("---- %s ------\n", mkvUri2.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(mkvUri2.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, nb_streams));
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    list<int64_t> toPtsList = {0, 1000, 1017, 1500, 1700, 1940, 3983, 1983, 3990}; // ms
    vector<int32_t> audioVals = {199, 199, 199, 149, 149, 149, 99, 149, 149, 99, 149, 149, 99, 149, 99, 99, 149, 99,
        49, 49, 99, 149, 99, 49, 49};
    vector<int32_t> videoVals = {240, 240, 240, 180, 180, 180, 120, 180, 180, 120, 180, 180, 120, 180, 120, 120, 180,
        120, 60, 60, 120, 180, 120, 60, 60};
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize);
            ASSERT_NE(sharedMem_, nullptr);
            ret = demuxer_->SeekToTime(*toPts, *mode);
            if (ret != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames[0]=%d | kFrames[0]=%d\n", *toPts, frames[0], keyFrames[0]);
            printf("time = %" PRId64 " | frames[1]=%d | kFrames[1]=%d\n", *toPts, frames[1], keyFrames[1]);
            ASSERT_EQ(frames[0], videoVals[number]);
            ASSERT_EQ(frames[1], audioVals[number]);
            number += 1;
            RemoveValue();
            selectedTrackIds_.clear();
            if (sharedMem_ != nullptr) {
                sharedMem_->Destory();
                sharedMem_ = nullptr;
            }
        }
    }
}

/**
 * @tc.name: Demuxer_SeekToTime_2060
 * @tc.desc: seek to the specified time
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_2060, TestSize.Level1)
{
    printf("---- %s ------\n", tsUri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(tsUri.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, nb_streams));
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    list<int64_t> toPtsList = {0, 3480, 3640, 3320, 3000, 3100, 4120, 5520}; // ms
    vector<int32_t> videoVals = {102, 102, 102, 15, 15, 15, 11, 11, 11, 19, 19, 19, 27, 27, 27, 24, 25, 25, 1, 1, 1};
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize);
            ASSERT_NE(sharedMem_, nullptr);
            ret = demuxer_->SeekToTime(*toPts, *mode);
            if (ret != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames[0]=%d | kFrames[0]=%d\n", *toPts, frames[0], keyFrames[0]);
            ASSERT_EQ(frames[0], videoVals[number]);
            number += 1;
            RemoveValue();
            selectedTrackIds_.clear();
            if (sharedMem_ != nullptr) {
                sharedMem_->Destory();
                sharedMem_ = nullptr;
            }
        }
    }
}

/**
 * @tc.name: Demuxer_SeekToTime_2070
 * @tc.desc: seek to the specified time
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_2070, TestSize.Level1)
{
    printf("---- %s ------\n", aacUri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(aacUri.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, nb_streams));
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    list<int64_t> toPtsList = {0, 10240, 10230, 10220, 30000, 30010}; // ms
    vector<int32_t> audioVals = {1293, 1293, 1293, 852, 852, 852, 853, 853, 853, 853, 853, 853, 2, 2, 2, 1, 1, 1};
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize);
            ASSERT_NE(sharedMem_, nullptr);
            ret = demuxer_->SeekToTime(*toPts, *mode);
            if (ret != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames[0]=%d | kFrames[0]=%d\n", *toPts, frames[0], keyFrames[0]);
            ASSERT_EQ(frames[0], audioVals[number]);
            number += 1;
            RemoveValue();
            selectedTrackIds_.clear();
            if (sharedMem_ != nullptr) {
                sharedMem_->Destory();
                sharedMem_ = nullptr;
            }
        }
    }
}

/**
 * @tc.name: Demuxer_SeekToTime_2080
 * @tc.desc: seek to the specified time
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_2080, TestSize.Level1)
{
    printf("---- %s ------\n", flacUri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(flacUri.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, nb_streams));
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    list<int64_t> toPtsList = {0, 3072, 4031, 4035, 29952, 29953}; // ms
    vector<int32_t> audioVals = {313, 313, 313, 281, 281, 281, 272, 272, 272, 271, 271, 271, 1, 1, 1, 2, 2, 2};
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize);
            ASSERT_NE(sharedMem_, nullptr);
            ret = demuxer_->SeekToTime(*toPts, *mode);
            if (ret != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames[0]=%d | kFrames[0]=%d\n", *toPts, frames[0], keyFrames[0]);
            ASSERT_EQ(frames[0], audioVals[number]);
            number += 1;
            RemoveValue();
            selectedTrackIds_.clear();
            if (sharedMem_ != nullptr) {
                sharedMem_->Destory();
                sharedMem_ = nullptr;
            }
        }
    }
}


/**
 * @tc.name: Demuxer_SeekToTime_2090
 * @tc.desc: seek to the specified time
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_2090, TestSize.Level1)
{
    printf("---- %s ------\n", m4aUri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(m4aUri.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, nb_streams));
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    list<int64_t> toPtsList = {0, 14592, 15297, 15290, 29994, 29998}; // ms
    vector<int32_t> audioVals = {1407, 1407, 1407, 723, 723, 723, 690, 690, 690, 691, 691, 691, 2, 2, 2, 1, 1, 1};
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize);
            ASSERT_NE(sharedMem_, nullptr);
            ret = demuxer_->SeekToTime(*toPts, *mode);
            if (ret != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames[0]=%d | kFrames[0]=%d\n", *toPts, frames[0], keyFrames[0]);
            ASSERT_EQ(frames[0], audioVals[number]);
            number += 1;
            RemoveValue();
            selectedTrackIds_.clear();
            if (sharedMem_ != nullptr) {
                sharedMem_->Destory();
                sharedMem_ = nullptr;
            }
        }
    }
}

/**
 * @tc.name: Demuxer_SeekToTime_2100
 * @tc.desc: seek to the specified time
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_2100, TestSize.Level1)
{
    printf("---- %s ------\n", mp3Uri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(mp3Uri.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, nb_streams));
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    list<int64_t> toPtsList = {0, 4128, 11980, 11990, 30000, 30010}; // ms
    vector<int32_t> audioVals = {1251, 1251, 1251, 1079, 1079, 1079, 752, 752, 752, 752, 752, 752, 1, 1, 1, 1, 1, 1};
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize);
            ASSERT_NE(sharedMem_, nullptr);
            ret = demuxer_->SeekToTime(*toPts, *mode);
            if (ret != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames[0]=%d | kFrames[0]=%d\n", *toPts, frames[0], keyFrames[0]);
            ASSERT_EQ(frames[0], audioVals[number]);
            number += 1;
            RemoveValue();
            selectedTrackIds_.clear();
            if (sharedMem_ != nullptr) {
                sharedMem_->Destory();
                sharedMem_ = nullptr;
            }
        }
    }
}

/**
 * @tc.name: Demuxer_SeekToTime_2110
 * @tc.desc: seek to the specified time
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_2110, TestSize.Level1)
{
    printf("---- %s ------\n", oggUri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(oggUri.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, nb_streams));
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    list<int64_t> toPtsList = {0, 5868, 5548, 5292, 29992, 29999}; // ms
    vector<int32_t> audioVals = {1598, 1598, 1598, 1357, 1357, 1357, 1357, 1357, 1357, 1357, 1357, 1357, 46, 46,
        46, 46, 46, 46};
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize);
            ASSERT_NE(sharedMem_, nullptr);
            ret = demuxer_->SeekToTime(*toPts, *mode);
            if (ret != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames[0]=%d | kFrames[0]=%d\n", *toPts, frames[0], keyFrames[0]);
            ASSERT_EQ(frames[0], audioVals[number]);
            number += 1;
            RemoveValue();
            selectedTrackIds_.clear();
            if (sharedMem_ != nullptr) {
                sharedMem_->Destory();
                sharedMem_ = nullptr;
            }
        }
    }
}

/**
 * @tc.name: Demuxer_SeekToTime_2120
 * @tc.desc: seek to the specified time
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_2120, TestSize.Level1)
{
    printf("---- %s ------\n", wavUri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(wavUri.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, nb_streams));
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    list<int64_t> toPtsList = {0, 8576, 8566, 8578, 29994, 30000}; // ms
    vector<int32_t> audioVals = {704, 704, 704, 503, 503, 503, 504, 504, 504, 503, 503, 503, 2, 2, 2, 1, 1, 1};
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize);
            ASSERT_NE(sharedMem_, nullptr);
            ret = demuxer_->SeekToTime(*toPts, *mode);
            if (ret != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames[0]=%d | kFrames[0]=%d\n", *toPts, frames[0], keyFrames[0]);
            ASSERT_EQ(frames[0], audioVals[number]);
            number += 1;
            RemoveValue();
            selectedTrackIds_.clear();
            if (sharedMem_ != nullptr) {
                sharedMem_->Destory();
                sharedMem_ = nullptr;
            }
        }
    }
}

/**
 * @tc.name: Demuxer_SeekToTime_2130
 * @tc.desc: seek to the specified time
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_2130, TestSize.Level1)
{
    printf("---- %s ------\n", amrUri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(amrUri.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, nb_streams));
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    list<int64_t> toPtsList = {0, 8560, 8550, 8570, 30000, 30900}; // ms
    vector<int32_t> audioVals = {1501, 1501, 1501, 1073, 1073, 1073, 1074, 1074, 1074, 1073, 1073, 1073,
        1, 1, 1, 1, 1, 1};
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize);
            ASSERT_NE(sharedMem_, nullptr);
            ret = demuxer_->SeekToTime(*toPts, *mode);
            if (ret != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames[0]=%d | kFrames[0]=%d\n", *toPts, frames[0], keyFrames[0]);
            ASSERT_EQ(frames[0], audioVals[number]);
            number += 1;
            RemoveValue();
            selectedTrackIds_.clear();
            if (sharedMem_ != nullptr) {
                sharedMem_->Destory();
                sharedMem_ = nullptr;
            }
        }
    }
}

/**
 * @tc.name: Demuxer_SeekToTime_2140
 * @tc.desc: seek to the specified time
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_2140, TestSize.Level1)
{
    printf("---- %s ------\n", amrUri2.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(amrUri2.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, nb_streams));
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    list<int64_t> toPtsList = {0, 11920, 11910, 11930, 29980, 30800}; // ms
    vector<int32_t> audioVals = {1500, 1500, 1500, 904, 904, 904, 905, 905, 905, 904, 904, 904,
        1, 1, 1, 1, 1, 1};
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize);
            ASSERT_NE(sharedMem_, nullptr);
            ret = demuxer_->SeekToTime(*toPts, *mode);
            if (ret != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames[0]=%d | kFrames[0]=%d\n", *toPts, frames[0], keyFrames[0]);
            ASSERT_EQ(frames[0], audioVals[number]);
            number += 1;
            RemoveValue();
            selectedTrackIds_.clear();
            if (sharedMem_ != nullptr) {
                sharedMem_->Destory();
                sharedMem_ = nullptr;
            }
        }
    }
}