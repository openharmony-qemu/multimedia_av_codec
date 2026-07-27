/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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
#include "mpeg4_demuxer_unit_test.h"
#include "syspara/parameters.h"

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
const int64_t SOURCE_OFFSET = 0;
const int32_t WIDTH = 3840;
const int32_t HEIGHT = 2160;
list<SeekMode> seekModes = {SeekMode::SEEK_NEXT_SYNC, SeekMode::SEEK_PREVIOUS_SYNC,
    SeekMode::SEEK_CLOSEST_SYNC};

string g_mp4SniffFreePath = TEST_FILE_PATH + string("only_free_in_first_2kb.mp4");
string g_mp4SniffMoovPath = TEST_FILE_PATH + string("only_moov_in_first_2kb.mp4");
string g_mp4SniffSkipPath = TEST_FILE_PATH + string("only_skip_in_first_2kb.mp4");
string g_mp4SniffMdatPath = TEST_FILE_PATH + string("only_mdat_in_first_2kb.mp4");
string g_mp4SniffJp2Path = TEST_FILE_PATH + string("ftyp_jp2_in_first_2kb.mp4");
string g_mp4MPEGPSPath = TEST_FILE_PATH + string("mpeg_ps_in_mov.mp4");
string g_mp4AbnormalBrandPath = TEST_FILE_PATH + string("abnormal_brand.mp4");
string g_mp4MoovOverSizePath = TEST_FILE_PATH + string("moov_oversize.mp4");
string g_mp4DelayPath = TEST_FILE_PATH + string("delay_v1s.mp4");
string g_fmp4MeetingPath = TEST_FILE_PATH + string("meeting_fmp4.mp4");
string g_mp4WvttPath = TEST_FILE_PATH + string("wvtt.mp4");
string g_mp4G711muPath = TEST_FILE_PATH + string("mpeg4_g711mu.mov");
string g_fmp4SidxPath = TEST_FILE_PATH + string("sidx.mp4");
string g_fmp4DoubleTopSidxPath = TEST_FILE_PATH + string("double_sidx_at_top.mp4");
string g_fmp4MultiSidxPath = TEST_FILE_PATH + string("multi_sidx.mp4");
string g_fmp4MoreSidxPath = TEST_FILE_PATH + string("more_sidx.mp4");
string g_mp4ShortAudioPath = TEST_FILE_PATH + string("short_audio.mp4");
string g_mp4ShortVideoPath = TEST_FILE_PATH + string("short_video.mp4");
string g_mp4H263AmrwbPath = TEST_FILE_PATH + string("h263_amr_wb.3gp");
string g_mp4Mpeg4OpusPath = TEST_FILE_PATH + string("mpeg4_opus.mp4");
string g_mp4Mpeg4AmrnbPath = TEST_FILE_PATH + string("mpeg4_amr_nb.3gp");
string g_mp4Mpeg2AlacPath = TEST_FILE_PATH + string("mpeg2_alac.mp4");
string g_mp4Mpeg2DtsPath = TEST_FILE_PATH + string("mpeg2_dts.mp4");
string g_ptsConversionPath = TEST_FILE_PATH + string("camera_info_parser.mp4");
string g_flvPath = TEST_FILE_PATH + string("h264.flv");
} // namespace

void Mpeg4DemuxerUnitTest::SetUpTestCase(void)
{
    server = make_unique<FileServerDemo>();
    server->StartServer();
    cout << "start" << endl;
}

void Mpeg4DemuxerUnitTest::TearDownTestCase(void)
{
    server->StopServer();
}

void Mpeg4DemuxerUnitTest::SetUp(void)
{
    bufferSize_ = WIDTH * HEIGHT;
}

void Mpeg4DemuxerUnitTest::TearDown(void)
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
        sharedMem_->Destroy();
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
    initStatus_ = false;
    selectedTrackIds_.clear();
}

int64_t Mpeg4DemuxerUnitTest::GetFileSize(const string &fileName)
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

int32_t Mpeg4DemuxerUnitTest::OpenFile(const string &fileName)
{
    int32_t fd = open(fileName.c_str(), O_RDONLY);
    return fd;
}

bool Mpeg4DemuxerUnitTest::isEOS(map<uint32_t, bool>& countFlag)
{
    for (auto iter = countFlag.begin(); iter != countFlag.end(); ++iter) {
        if (!iter->second) {
            return false;
        }
    }
    return true;
}

bool Mpeg4DemuxerUnitTest::SetInitValue()
{
    for (int i = 0; i < nbStreams_; i++) {
        string codecMime = "";
        int32_t trackType = -1;
        if (format_ != nullptr) {
            format_->Destroy();
            format_ = nullptr;
        }
        format_ = source_->GetTrackFormat(i);
        if (format_ == nullptr) {
            return false;
        }
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
    return true;
}

void Mpeg4DemuxerUnitTest::RemoveValue()
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

void Mpeg4DemuxerUnitTest::SetEosValue()
{
    for (int i = 0; i < nbStreams_; i++) {
        eosFlag_[i] = true;
    }
}

void Mpeg4DemuxerUnitTest::CountFrames(uint32_t index)
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

bool Mpeg4DemuxerUnitTest::CheckKeyFrameIndex(std::vector<uint32_t> keyFrameIndexList,
    const uint32_t frameIndex, const bool isKeyFrame)
{
    bool contaionIndex = (std::count(keyFrameIndexList.begin(), keyFrameIndexList.end(), frameIndex) > 0);
    return isKeyFrame ? contaionIndex : !contaionIndex;
}

void Mpeg4DemuxerUnitTest::ReadData()
{
    EXPECT_TRUE(SetInitValue());
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
    }
}

void Mpeg4DemuxerUnitTest::ReadData(int readNum, int64_t &seekTime)
{
    int num = 0;
    EXPECT_TRUE(SetInitValue());
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

namespace {
/**
 * @tc.name: mp4Demuxer_CreateDemuxer_0001
 * @tc.desc: create demuxer. The first 2048 bytes of this MP4 resource
 * contain 'ftyp' box and the brand is jp2.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_CreateDemuxer_0001, TestSize.Level1)
{
    InitResource(g_mp4SniffJp2Path, LOCAL);
    ASSERT_TRUE(initStatus_);
}

/**
 * @tc.name: mp4Demuxer_CreateDemuxer_0002
 * @tc.desc: create demuxer. The first 2048 bytes of this MP4 resource contain only a single 'mdat' box.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_CreateDemuxer_0002, TestSize.Level1)
{
    InitResource(g_mp4SniffMdatPath, LOCAL);
    ASSERT_TRUE(initStatus_);
}

/**
 * @tc.name: mp4Demuxer_CreateDemuxer_0003
 * @tc.desc: create demuxer. The first 2048 bytes of this MP4 resource contain only a single 'moov' box.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_CreateDemuxer_0003, TestSize.Level1)
{
    InitResource(g_mp4SniffMoovPath, LOCAL);
    ASSERT_TRUE(initStatus_);
}

/**
 * @tc.name: mp4Demuxer_CreateDemuxer_0004
 * @tc.desc: create demuxer. The first 2048 bytes of this MP4 resource contain only a single 'free' box.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_CreateDemuxer_0004, TestSize.Level1)
{
    InitResource(g_mp4SniffFreePath, LOCAL);
    ASSERT_TRUE(initStatus_);
}

/**
 * @tc.name: mp4Demuxer_CreateDemuxer_0005
 * @tc.desc: create demuxer. The first 2048 bytes of this MP4 resource contain only a single 'skip' box.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_CreateDemuxer_0005, TestSize.Level1)
{
    fd_ = OpenFile(g_mp4SniffSkipPath);
    int64_t size = GetFileSize(g_mp4SniffSkipPath);
    source_ = Mpeg4SourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size);
    ASSERT_EQ(source_, nullptr);
}

/**
 * @tc.name: mp4Demuxer_CreateDemuxer_0006
 * @tc.desc: create demuxer. MPEG-PS in mp4.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_CreateDemuxer_0006, TestSize.Level1)
{
    fd_ = OpenFile(g_mp4MPEGPSPath);
    int64_t size = GetFileSize(g_mp4MPEGPSPath);
    source_ = Mpeg4SourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size);
    ASSERT_NE(source_, nullptr);
}

/**
 * @tc.name: mp4Demuxer_CreateDemuxer_0007
 * @tc.desc: create demuxer. Abnormal brand in 'ftyp' box.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_CreateDemuxer_0007, TestSize.Level1)
{
    fd_ = OpenFile(g_mp4AbnormalBrandPath);
    int64_t size = GetFileSize(g_mp4AbnormalBrandPath);
    source_ = Mpeg4SourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size);
    ASSERT_NE(source_, nullptr);
}

/**
 * @tc.name: mp4Demuxer_CreateDemuxer_0008
 * @tc.desc: Demuxer creation succeeds, but initialization fails due to excessively large moov box size
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_CreateDemuxer_0008, TestSize.Level1)
{
    fd_ = OpenFile(g_mp4MoovOverSizePath);
    int64_t size = GetFileSize(g_mp4MoovOverSizePath);
    source_ = Mpeg4SourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size);
    ASSERT_EQ(source_, nullptr);
}

/**
 * @tc.name: mp4Demuxer_UnselectTrackByID_0001
 * @tc.desc: select and remove track by ID
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_UnselectTrackByID_0001, TestSize.Level1)
{
    InitResource(g_mp4DelayPath, LOCAL);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    EXPECT_NE(demuxer_->SelectTrackByID(2), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_NE(demuxer_->SelectTrackByID(-1), AV_ERR_OK);
    EXPECT_EQ(demuxer_->UnselectTrackByID(1), AV_ERR_OK);
    EXPECT_EQ(demuxer_->UnselectTrackByID(2), AV_ERR_OK);
    EXPECT_EQ(demuxer_->UnselectTrackByID(-1), AV_ERR_OK);
}

/**
 * @tc.name: mp4Demuxer_UnselectTrackByID_0002
 * @tc.desc: select and remove track by ID
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_UnselectTrackByID_0002, TestSize.Level1)
{
    InitResource(g_fmp4MeetingPath, LOCAL);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    EXPECT_NE(demuxer_->SelectTrackByID(2), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_NE(demuxer_->SelectTrackByID(-1), AV_ERR_OK);
    EXPECT_EQ(demuxer_->UnselectTrackByID(1), AV_ERR_OK);
    EXPECT_EQ(demuxer_->UnselectTrackByID(2), AV_ERR_OK);
    EXPECT_EQ(demuxer_->UnselectTrackByID(-1), AV_ERR_OK);
}

/**
 * @tc.name: mp4Demuxer_ReadSample_0001
 * @tc.desc: copy current sample to buffer, check read in PTS order
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_ReadSample_0001, TestSize.Level1)
{
    InitResource(g_mp4DelayPath, LOCAL);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
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
    EXPECT_EQ(frames_[1], 434);
    EXPECT_EQ(keyFrames_[0], 51);
    EXPECT_EQ(keyFrames_[1], 434);
    RemoveValue();
}

/**
 * @tc.name: mp4Demuxer_ReadSample_0002
 * @tc.desc: copy current sample to buffer, check read in PTS order
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_ReadSample_0002, TestSize.Level1)
{
    InitResource(g_fmp4MeetingPath, LOCAL);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    EXPECT_TRUE(SetInitValue());
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            EXPECT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    printf("frames_[1]=%d | kFrames[1]=%d\n", frames_[1], keyFrames_[1]);
    EXPECT_EQ(frames_[0], 178);
    EXPECT_EQ(frames_[1], 548);
    EXPECT_EQ(keyFrames_[0], 12);
    EXPECT_EQ(keyFrames_[1], 548);
    RemoveValue();
}

/**
 * @tc.name: mp4Demuxer_ReadSample_0003
 * @tc.desc: copy current sample to buffer, check read in PTS order
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_ReadSample_0003, TestSize.Level1)
{
    InitResource(g_mp4WvttPath, LOCAL);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(2), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    EXPECT_TRUE(SetInitValue());
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            EXPECT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    for (int32_t i = 0; i < 3; i++) {
        printf("frames_[%d]=%d | kFrames[%d]=%d\n", i, frames_[i], i, keyFrames_[i]);
    }
    EXPECT_EQ(frames_[0], 604);
    EXPECT_EQ(frames_[1], 434);
    EXPECT_EQ(frames_[2], 8);
    EXPECT_EQ(keyFrames_[0], 61);
    EXPECT_EQ(keyFrames_[1], 434);
    EXPECT_EQ(keyFrames_[2], 8);
    RemoveValue();
}

/**
 * @tc.name: mp4Demuxer_ReadSample_0004
 * @tc.desc: copy current sample to buffer, check read in PTS order
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_ReadSample_0004, TestSize.Level1)
{
    InitResource(g_mp4G711muPath, LOCAL);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    EXPECT_TRUE(SetInitValue());
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            EXPECT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    printf("frames_[1]=%d | kFrames[1]=%d\n", frames_[1], keyFrames_[1]);
    EXPECT_EQ(frames_[0], 45);
    EXPECT_EQ(frames_[1], 45);
    EXPECT_EQ(keyFrames_[0], 5);
    EXPECT_EQ(keyFrames_[1], 45);
    RemoveValue();
}

/**
 * @tc.name: mp4Demuxer_ReadSample_0005
 * @tc.desc: copy current sample to buffer, check read in PTS order
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_ReadSample_0005, TestSize.Level1)
{
    InitResource(g_fmp4SidxPath, LOCAL);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    EXPECT_TRUE(SetInitValue());
    uint32_t index = 0;
    uint32_t listIndex = 0;
    vector<int64_t> sampleIndexList = {26, 42, 53, 136, 247};
    vector<int32_t> videoPts = {450000, 716666, 966666, 2300000, 4116666};
    vector<int32_t> audioPts = {697573, 1115532, 1402879, 3571043, 6470634};
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            EXPECT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            if (index == sampleIndexList[listIndex] && idx == 0) {
                EXPECT_EQ(info_.presentationTimeUs, videoPts[listIndex]);
            } else if (index == sampleIndexList[listIndex] && idx == 1) {
                EXPECT_EQ(info_.presentationTimeUs, audioPts[listIndex]);
            }
            CountFrames(idx);
        }
        if (index == sampleIndexList[listIndex] && listIndex < sampleIndexList.size()) {
            listIndex++;
        }
        index++;
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    printf("frames_[1]=%d | kFrames[1]=%d\n", frames_[1], keyFrames_[1]);
    EXPECT_EQ(frames_[0], 604);
    EXPECT_EQ(frames_[1], 387);
    EXPECT_EQ(keyFrames_[0], 21);
    EXPECT_EQ(keyFrames_[1], 387);
    RemoveValue();
}

/**
 * @tc.name: mp4Demuxer_ReadSample_0006
 * @tc.desc: copy current sample to buffer, check read in PTS order
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_ReadSample_0006, TestSize.Level1)
{
    InitResource(g_fmp4DoubleTopSidxPath, LOCAL);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    EXPECT_TRUE(SetInitValue());
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            EXPECT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    printf("frames_[1]=%d | kFrames[1]=%d\n", frames_[1], keyFrames_[1]);
    EXPECT_EQ(frames_[0], 604);
    EXPECT_EQ(frames_[1], 387);
    EXPECT_EQ(keyFrames_[0], 21);
    EXPECT_EQ(keyFrames_[1], 387);
    RemoveValue();
}

/**
 * @tc.name: mp4Demuxer_ReadSample_0007
 * @tc.desc: copy current sample to buffer, check read in PTS order
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_ReadSample_0007, TestSize.Level1)
{
    InitResource(g_fmp4MultiSidxPath, LOCAL);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    EXPECT_TRUE(SetInitValue());
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            EXPECT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    EXPECT_EQ(frames_[0], 324);
    EXPECT_EQ(keyFrames_[0], 33);
    RemoveValue();
}

/**
 * @tc.name: mp4Demuxer_ReadSample_0008
 * @tc.desc: copy current sample to buffer, check read in PTS order
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_ReadSample_0008, TestSize.Level1)
{
    InitResource(g_fmp4MoreSidxPath, LOCAL);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    EXPECT_TRUE(SetInitValue());
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            EXPECT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    printf("frames_[1]=%d | kFrames[1]=%d\n", frames_[1], keyFrames_[1]);
    EXPECT_EQ(frames_[0], 420);
    EXPECT_EQ(frames_[1], 268);
    EXPECT_EQ(keyFrames_[0], 14);
    EXPECT_EQ(keyFrames_[1], 268);
    RemoveValue();
}

/**
 * @tc.name: mp4Demuxer_ReadSample_0009
 * @tc.desc: copy current sample to buffer, check read in PTS order
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_ReadSample_0009, TestSize.Level1)
{
    InitResource(g_mp4ShortAudioPath, LOCAL);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    EXPECT_TRUE(SetInitValue());
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            EXPECT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    printf("frames_[1]=%d | kFrames[1]=%d\n", frames_[1], keyFrames_[1]);
    EXPECT_EQ(frames_[0], 120);
    EXPECT_EQ(frames_[1], 85);
    EXPECT_EQ(keyFrames_[0], 12);
    EXPECT_EQ(keyFrames_[1], 85);
    RemoveValue();
}

/**
 * @tc.name: mp4Demuxer_ReadSample_0010
 * @tc.desc: copy current sample to buffer, check read in PTS order
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_ReadSample_0010, TestSize.Level1)
{
    InitResource(g_mp4ShortVideoPath, LOCAL);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    EXPECT_TRUE(SetInitValue());
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            EXPECT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    printf("frames_[1]=%d | kFrames[1]=%d\n", frames_[1], keyFrames_[1]);
    EXPECT_EQ(frames_[0], 150);
    EXPECT_EQ(frames_[1], 470);
    EXPECT_EQ(keyFrames_[0], 15);
    EXPECT_EQ(keyFrames_[1], 470);
    RemoveValue();
}

/**
 * @tc.name: mp4Demuxer_ReadSample_0011
 * @tc.desc: copy current sample to buffer, check read in PTS order
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_ReadSample_0011, TestSize.Level1)
{
    InitResource(g_mp4H263AmrwbPath, LOCAL);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    EXPECT_TRUE(SetInitValue());
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            EXPECT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    printf("frames_[1]=%d | kFrames[1]=%d\n", frames_[1], keyFrames_[1]);
    EXPECT_EQ(frames_[0], 99);
    EXPECT_EQ(frames_[1], 165);
    EXPECT_EQ(keyFrames_[0], 9);
    EXPECT_EQ(keyFrames_[1], 165);
    RemoveValue();
}

/**
 * @tc.name: mp4Demuxer_ReadSample_0012
 * @tc.desc: copy current sample to buffer, check read in PTS order
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_ReadSample_0012, TestSize.Level1)
{
    InitResource(g_mp4Mpeg4OpusPath, LOCAL);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    EXPECT_TRUE(SetInitValue());
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            EXPECT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    printf("frames_[1]=%d | kFrames[1]=%d\n", frames_[1], keyFrames_[1]);
    EXPECT_EQ(frames_[0], 95);
    EXPECT_EQ(frames_[1], 165);
    EXPECT_EQ(keyFrames_[0], 8);
    EXPECT_EQ(keyFrames_[1], 165);
    RemoveValue();
}

/**
 * @tc.name: mp4Demuxer_ReadSample_0013
 * @tc.desc: copy current sample to buffer, check read in PTS order
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_ReadSample_0013, TestSize.Level1)
{
    InitResource(g_mp4Mpeg4AmrnbPath, LOCAL);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    EXPECT_TRUE(SetInitValue());
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            EXPECT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    printf("frames_[1]=%d | kFrames[1]=%d\n", frames_[1], keyFrames_[1]);
    EXPECT_EQ(frames_[0], 95);
    EXPECT_EQ(frames_[1], 165);
    EXPECT_EQ(keyFrames_[0], 8);
    EXPECT_EQ(keyFrames_[1], 165);
    RemoveValue();
}

/**
 * @tc.name: mp4Demuxer_ReadSample_0014
 * @tc.desc: copy current sample to buffer, check read in PTS order
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_ReadSample_0014, TestSize.Level1)
{
    InitResource(g_mp4Mpeg2AlacPath, LOCAL);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    EXPECT_TRUE(SetInitValue());
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            EXPECT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    printf("frames_[1]=%d | kFrames[1]=%d\n", frames_[1], keyFrames_[1]);
    EXPECT_EQ(frames_[0], 95);
    EXPECT_EQ(frames_[1], 39);
    EXPECT_EQ(keyFrames_[0], 10);
    EXPECT_EQ(keyFrames_[1], 39);
    RemoveValue();
}

/**
 * @tc.name: mp4Demuxer_ReadSample_0015
 * @tc.desc: copy current sample to buffer, check read in PTS order
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_ReadSample_0015, TestSize.Level1)
{
    InitResource(g_mp4Mpeg2DtsPath, LOCAL);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    EXPECT_TRUE(SetInitValue());
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            EXPECT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    printf("frames_[1]=%d | kFrames[1]=%d\n", frames_[1], keyFrames_[1]);
    EXPECT_EQ(frames_[0], 95);
    EXPECT_EQ(frames_[1], 308);
    EXPECT_EQ(keyFrames_[0], 10);
    EXPECT_EQ(keyFrames_[1], 308);
    RemoveValue();
}

#ifndef MPEG4_DEMUXER_CAPI_BUFFER_UNIT_TEST
/**
 * @tc.name: mp4Demuxer_ReadSample_0016
 * @tc.desc: copy current sample to buffer, samll bufferSize, check total read size
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_ReadSample_0016, TestSize.Level1)
{
    bufferSize_ = 1000;
    InitResource(g_mp4G711muPath, LOCAL);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    EXPECT_TRUE(SetInitValue());
    vector<uint32_t> sizes = {0, 0};
    vector<uint32_t> expectSizes = {582040, 12000};
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            demuxer_->ReadSample(idx, sharedMem_, &info_, flag_);
            sizes[idx] += info_.size;
            CountFrames(idx);
        }
    }
    printf("sizes[0]=%d | expectSizes[0]=%d\n", sizes[0], expectSizes[0]);
    printf("sizes[1]=%d | expectSizes[1]=%d\n", sizes[1], expectSizes[1]);
    EXPECT_EQ(sizes[0], expectSizes[0]);
    EXPECT_EQ(sizes[1], expectSizes[1]);
    RemoveValue();
}
#endif

#ifndef MPEG4_DEMUXER_CAPI_UNIT_TEST
/**
 * @tc.name: Demuxer_GetIndexByRelativePresentationTimeUs_1000
 * @tc.desc: Get index by pts(audio track)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, Demuxer_GetIndexByRelativePresentationTimeUs_1000, TestSize.Level1)
{
    InitResource(g_ptsConversionPath, LOCAL);
    ASSERT_TRUE(initStatus_);

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
HWTEST_F(Mpeg4DemuxerUnitTest, Demuxer_GetIndexByRelativePresentationTimeUs_1001, TestSize.Level1)
{
    InitResource(g_ptsConversionPath, LOCAL);
    ASSERT_TRUE(initStatus_);

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
HWTEST_F(Mpeg4DemuxerUnitTest, Demuxer_GetIndexByRelativePresentationTimeUs_1002, TestSize.Level1)
{
    InitResource(g_flvPath, LOCAL);
    ASSERT_TRUE(initStatus_);

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
HWTEST_F(Mpeg4DemuxerUnitTest, Demuxer_GetIndexByRelativePresentationTimeUs_1003, TestSize.Level1)
{
    InitResource(g_ptsConversionPath, LOCAL);
    ASSERT_TRUE(initStatus_);

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
HWTEST_F(Mpeg4DemuxerUnitTest, Demuxer_GetRelativePresentationTimeUsByIndex_1000, TestSize.Level1)
{
    InitResource(g_ptsConversionPath, LOCAL);
    ASSERT_TRUE(initStatus_);

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
HWTEST_F(Mpeg4DemuxerUnitTest, Demuxer_GetRelativePresentationTimeUsByIndex_1001, TestSize.Level1)
{
    InitResource(g_ptsConversionPath, LOCAL);
    ASSERT_TRUE(initStatus_);

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
HWTEST_F(Mpeg4DemuxerUnitTest, Demuxer_GetRelativePresentationTimeUsByIndex_1002, TestSize.Level1)
{
    InitResource(g_flvPath, LOCAL);
    ASSERT_TRUE(initStatus_);

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
HWTEST_F(Mpeg4DemuxerUnitTest, Demuxer_PtsAndFrameIndexConversion_1000, TestSize.Level1)
{
    InitResource(g_ptsConversionPath, LOCAL);
    ASSERT_TRUE(initStatus_);

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
HWTEST_F(Mpeg4DemuxerUnitTest, Demuxer_PtsAndFrameIndexConversion_1001, TestSize.Level1)
{
    InitResource(g_ptsConversionPath, LOCAL);
    ASSERT_TRUE(initStatus_);

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
HWTEST_F(Mpeg4DemuxerUnitTest, Demuxer_PTSOutOfRange_1000, TestSize.Level1)
{
    InitResource(g_ptsConversionPath, LOCAL);
    ASSERT_TRUE(initStatus_);

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
HWTEST_F(Mpeg4DemuxerUnitTest, Demuxer_IndexOutOfRange_1000, TestSize.Level1)
{
    InitResource(g_ptsConversionPath, LOCAL);
    ASSERT_TRUE(initStatus_);

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
HWTEST_F(Mpeg4DemuxerUnitTest, Demuxer_TrackOutOfRange_1000, TestSize.Level1)
{
    InitResource(g_ptsConversionPath, LOCAL);
    ASSERT_TRUE(initStatus_);

    uint32_t trackIndex = 99;
    uint64_t relativePresentationTimeUs = 0;
    uint32_t index = 0;
    int32_t ret = demuxer_->GetRelativePresentationTimeUsByIndex(trackIndex, index, relativePresentationTimeUs);
    ASSERT_NE(ret, AV_ERR_OK);
}
#endif
} // namespace
