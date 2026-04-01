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
} // namespace

void Mpeg4DemuxerUnitTest::SetUpTestCase(void)
{
    OHOS::system::SetParameter("Mpeg4DemuxerPlugin.enable", "1");
    server = make_unique<FileServerDemo>();
    server->StartServer();
    cout << "start" << endl;
}

void Mpeg4DemuxerUnitTest::TearDownTestCase(void)
{
    server->StopServer();
    OHOS::system::SetParameter("Mpeg4DemuxerPlugin.enable", "0");
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
    ASSERT_EQ(source_, nullptr);
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

/**
 * @tc.name: mp4Demuxer_SeekToTime_0001
 * @tc.desc: seek to the specified time
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_SeekToTime_0001, TestSize.Level1)
{
    InitResource(g_mp4DelayPath, LOCAL);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    // 边界情况
    EXPECT_EQ(demuxer_->SeekToTime(0, SeekMode::SEEK_PREVIOUS_SYNC), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SeekToTime(800, SeekMode::SEEK_PREVIOUS_SYNC), AV_ERR_OK);
    EXPECT_NE(demuxer_->SeekToTime(-10, SeekMode::SEEK_PREVIOUS_SYNC), AV_ERR_OK);
    EXPECT_NE(demuxer_->SeekToTime(12000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    // 普通时刻
    list<int64_t> toPtsList = {1000, 2000, 2550, 3400, 9050};
    vector<int32_t> videoVals = {602, 602, 602, 542, 542, 542, 506, 518, 506, 458, 458, 458, 110, 122, 122};
    vector<int32_t> audioVals = {390, 391, 390, 347, 348, 347, 321, 331, 321, 287, 288, 287,  37,  46,  46};
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
            EXPECT_EQ(frames_[0], videoVals[numbers_]);
            EXPECT_EQ(frames_[1], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: mp4Demuxer_SeekToTime_0002
 * @tc.desc: seek to the specified time
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_SeekToTime_0002, TestSize.Level1)
{
    InitResource(g_fmp4MeetingPath, LOCAL);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    // 边界情况
    EXPECT_NE(demuxer_->SeekToTime(-10, SeekMode::SEEK_PREVIOUS_SYNC), AV_ERR_OK);
    EXPECT_NE(demuxer_->SeekToTime(15000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    // 普通时刻
    list<int64_t> toPtsList = {0, 800, 2500, 3400, 9500};
    vector<int32_t> videoVals = {178, 178, 178, 163, 178, 163, 148, 148, 148, 133, 133, 133,  43,  43,  43};
    vector<int32_t> audioVals = {548, 548, 548, 505, 548, 505, 461, 461, 461, 417, 417, 417, 156, 156, 156};
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
            EXPECT_EQ(frames_[0], videoVals[numbers_]);
            EXPECT_EQ(frames_[1], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: mp4Demuxer_SeekToTime_0003
 * @tc.desc: seek to the specified time
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_SeekToTime_0003, TestSize.Level1)
{
    InitResource(g_mp4WvttPath, LOCAL);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(2), AV_ERR_OK);
    // 边界情况
    EXPECT_NE(demuxer_->SeekToTime(-10, SeekMode::SEEK_PREVIOUS_SYNC), AV_ERR_OK);
    EXPECT_NE(demuxer_->SeekToTime(15000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    // 普通时刻
    list<int64_t> toPtsList = {500, 800, 2500, 3400, 9050};
    vector<int32_t> videoVals = {574, 574, 574, 554, 564, 554, 454, 454, 454, 394, 404, 404, 54, 64, 64};
    vector<int32_t> audioVals = {411, 412, 412, 397, 405, 398, 325, 326, 326, 282, 290, 290, 38, 46, 46};
    vector<int32_t> subtitleVals = { 8, 8, 8, 8, 8, 8, 5, 5, 5, 4, 4, 4, 0, 0, 0};
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
            EXPECT_EQ(frames_[0], videoVals[numbers_]);
            EXPECT_EQ(frames_[1], audioVals[numbers_]);
            EXPECT_EQ(frames_[2], subtitleVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: mp4Demuxer_SeekToTime_0004
 * @tc.desc: seek to the specified time
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_SeekToTime_0004, TestSize.Level1)
{
    InitResource(g_mp4G711muPath, LOCAL);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    // 边界情况
    EXPECT_EQ(demuxer_->SeekToTime(0, SeekMode::SEEK_PREVIOUS_SYNC), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SeekToTime(800, SeekMode::SEEK_PREVIOUS_SYNC), AV_ERR_OK);
    EXPECT_NE(demuxer_->SeekToTime(-10, SeekMode::SEEK_PREVIOUS_SYNC), AV_ERR_OK);
    EXPECT_NE(demuxer_->SeekToTime(2000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    // 普通时刻
    list<int64_t> toPtsList = {500, 800, 1000, 1200, 1333};
    vector<int32_t> videoVals = {25, 35, 35, 15, 25, 25, 15, 15, 15, 5, 15, 5, 5, 15, 5};
    vector<int32_t> audioVals = {25, 36, 36, 15, 26, 25, 15, 16, 15, 5, 16, 5, 5, 16, 5};
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
            EXPECT_EQ(frames_[0], videoVals[numbers_]);
            EXPECT_EQ(frames_[1], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: mp4Demuxer_SeekToTime_0005
 * @tc.desc: seek to the specified time
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_SeekToTime_0005, TestSize.Level1)
{
    InitResource(g_fmp4SidxPath, LOCAL);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    // 边界情况
    EXPECT_NE(demuxer_->SeekToTime(-10, SeekMode::SEEK_PREVIOUS_SYNC), AV_ERR_OK);
    EXPECT_NE(demuxer_->SeekToTime(15000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    // 普通时刻
    list<int64_t> toPtsList = {0, 800, 2500, 3400, 9500};
    vector<int32_t> videoVals = {604, 604, 604, 574, 574, 574, 454, 454, 454, 424, 424, 424, 34, 34, 34};
    vector<int32_t> audioVals = {387, 387, 387, 368, 368, 368, 292, 292, 292, 272, 272, 272, 24, 24, 24};
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
            EXPECT_EQ(frames_[0], videoVals[numbers_]);
            EXPECT_EQ(frames_[1], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: mp4Demuxer_SeekToTime_0006
 * @tc.desc: seek to the specified time
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_SeekToTime_0006, TestSize.Level1)
{
    InitResource(g_fmp4DoubleTopSidxPath, LOCAL);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    // 边界情况
    EXPECT_NE(demuxer_->SeekToTime(-10, SeekMode::SEEK_PREVIOUS_SYNC), AV_ERR_OK);
    EXPECT_NE(demuxer_->SeekToTime(15000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    // 普通时刻
    list<int64_t> toPtsList = {0, 800, 2500, 3400, 9500};
    vector<int32_t> videoVals = {604, 604, 604, 574, 574, 574, 454, 454, 454, 424, 424, 424, 34, 34, 34};
    vector<int32_t> audioVals = {387, 387, 387, 368, 368, 368, 292, 292, 292, 272, 272, 272, 24, 24, 24};
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
            EXPECT_EQ(frames_[0], videoVals[numbers_]);
            EXPECT_EQ(frames_[1], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: mp4Demuxer_SeekToTime_0007
 * @tc.desc: seek to the specified time
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_SeekToTime_0007, TestSize.Level1)
{
    InitResource(g_fmp4MultiSidxPath, LOCAL);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    // 边界情况
    EXPECT_NE(demuxer_->SeekToTime(-10, SeekMode::SEEK_PREVIOUS_SYNC), AV_ERR_OK);
    EXPECT_NE(demuxer_->SeekToTime(15000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    // 普通时刻
    list<int64_t> toPtsList = {0, 800, 2500, 3400, 9500};
    vector<int32_t> videoVals = {324, 324, 324, 324, 324, 324, 264, 264, 264, 234, 234, 234, 54, 54, 54};
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
            EXPECT_EQ(frames_[0], videoVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: mp4Demuxer_SeekToTime_0008
 * @tc.desc: seek to the specified time
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_SeekToTime_0008, TestSize.Level1)
{
    InitResource(g_fmp4MoreSidxPath, LOCAL);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    //边界情况
    EXPECT_NE(demuxer_->SeekToTime(-10, SeekMode::SEEK_PREVIOUS_SYNC), AV_ERR_OK);
    EXPECT_NE(demuxer_->SeekToTime(8000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    //普通时刻
    list<int64_t> toPtsList = {0, 800, 2500, 3400, 6500};
    vector<int32_t> videoVals = {420, 420, 420, 390, 390, 390, 270, 270, 270, 240, 240, 240, 30, 30, 30};
    vector<int32_t> audioVals = {268, 268, 268, 249, 249, 249, 173, 173, 173, 153, 153, 153, 19, 19, 19};
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
            EXPECT_EQ(frames_[0], videoVals[numbers_]);
            EXPECT_EQ(frames_[1], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: mp4Demuxer_SeekToTime_0009
 * @tc.desc: seek to the specified time
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_SeekToTime_0009, TestSize.Level1)
{
    InitResource(g_mp4ShortAudioPath, LOCAL);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    // 边界情况
    EXPECT_EQ(demuxer_->SeekToTime(0, SeekMode::SEEK_PREVIOUS_SYNC), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SeekToTime(800, SeekMode::SEEK_PREVIOUS_SYNC), AV_ERR_OK);
    EXPECT_NE(demuxer_->SeekToTime(-10, SeekMode::SEEK_PREVIOUS_SYNC), AV_ERR_OK);
    EXPECT_NE(demuxer_->SeekToTime(12000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    // 普通时刻
    list<int64_t> toPtsList = {500, 1500, 2550, 3000, 3600};
    vector<int32_t> videoVals = {100, 110, 110, 70, 80, 80, 40, 50, 40, 30, 30, 30, 10, 20, 10};
    vector<int32_t> audioVals = { 85,  85,  85, 58, 73, 72, 15, 30, 15,  0,  0,  0,  0,  0,  0};
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
            EXPECT_EQ(frames_[0], videoVals[numbers_]);
            EXPECT_EQ(frames_[1], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: mp4Demuxer_SeekToTime_0010
 * @tc.desc: seek to the specified time
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_SeekToTime_0010, TestSize.Level1)
{
    InitResource(g_mp4ShortVideoPath, LOCAL);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    // 边界情况
    EXPECT_EQ(demuxer_->SeekToTime(0, SeekMode::SEEK_PREVIOUS_SYNC), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SeekToTime(800, SeekMode::SEEK_PREVIOUS_SYNC), AV_ERR_OK);
    EXPECT_NE(demuxer_->SeekToTime(-10, SeekMode::SEEK_PREVIOUS_SYNC), AV_ERR_OK);
    EXPECT_NE(demuxer_->SeekToTime(12000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    // 普通时刻
    list<int64_t> toPtsList = {500, 2066, 2550, 7033, 8500};
    vector<int32_t> videoVals = {150, 150, 150, 150, 150, 150, 130, 140, 140,   0,  10,  10,  0,  0,  0};
    vector<int32_t> audioVals = {445, 446, 446, 372, 373, 372, 340, 357, 357, 139, 154, 153, 70, 71, 71};
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
            EXPECT_EQ(frames_[0], videoVals[numbers_]);
            EXPECT_EQ(frames_[1], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: mp4Demuxer_SeekToTime_0011
 * @tc.desc: seek to the specified time
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_SeekToTime_0011, TestSize.Level1)
{
    InitResource(g_mp4H263AmrwbPath, LOCAL);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    // 边界情况
    EXPECT_EQ(demuxer_->SeekToTime(0, SeekMode::SEEK_PREVIOUS_SYNC), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SeekToTime(800, SeekMode::SEEK_PREVIOUS_SYNC), AV_ERR_OK);
    EXPECT_NE(demuxer_->SeekToTime(-10, SeekMode::SEEK_PREVIOUS_SYNC), AV_ERR_OK);
    EXPECT_NE(demuxer_->SeekToTime(5000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    // 普通时刻
    list<int64_t> toPtsList = {500, 800, 2100, 2633, 3166};
    vector<int32_t> videoVals = { 75,  87,  87,  75,  75,  75, 27, 39, 39, 15, 27, 15, 3, 15, 3};
    vector<int32_t> audioVals = {124, 145, 145, 124, 125, 125, 44, 65, 65, 24, 45, 25, 4, 25, 5};
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
            EXPECT_EQ(frames_[0], videoVals[numbers_]);
            EXPECT_EQ(frames_[1], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: mp4Demuxer_SeekToTime_0012
 * @tc.desc: seek to the specified time
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_SeekToTime_0012, TestSize.Level1)
{
    InitResource(g_mp4Mpeg4OpusPath, LOCAL);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    // 边界情况
    EXPECT_EQ(demuxer_->SeekToTime(0, SeekMode::SEEK_PREVIOUS_SYNC), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SeekToTime(800, SeekMode::SEEK_PREVIOUS_SYNC), AV_ERR_OK);
    EXPECT_NE(demuxer_->SeekToTime(-10, SeekMode::SEEK_PREVIOUS_SYNC), AV_ERR_OK);
    EXPECT_NE(demuxer_->SeekToTime(5000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    // 普通时刻
    list<int64_t> toPtsList = {500, 800, 1500, 2100, 2633};
    vector<int32_t> videoVals = { 83,  95,  83,  71,  83,  71, 47, 59, 59, 35, 47, 35, 11, 23, 23};
    vector<int32_t> audioVals = {138, 165, 138, 118, 139, 118, 78, 99, 98, 58, 79, 58, 18, 39, 38};
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
            EXPECT_EQ(frames_[0], videoVals[numbers_]);
            EXPECT_EQ(frames_[1], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: mp4Demuxer_SeekToTime_0013
 * @tc.desc: seek to the specified time
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_SeekToTime_0013, TestSize.Level1)
{
    InitResource(g_mp4Mpeg4AmrnbPath, LOCAL);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    // 边界情况
    EXPECT_EQ(demuxer_->SeekToTime(0, SeekMode::SEEK_PREVIOUS_SYNC), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SeekToTime(800, SeekMode::SEEK_PREVIOUS_SYNC), AV_ERR_OK);
    EXPECT_NE(demuxer_->SeekToTime(-10, SeekMode::SEEK_PREVIOUS_SYNC), AV_ERR_OK);
    EXPECT_NE(demuxer_->SeekToTime(5000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    // 普通时刻
    list<int64_t> toPtsList = {500, 800, 2100, 2633, 3166};
    vector<int32_t> videoVals = { 83,  95,  83,  71,  83,  71, 35, 47, 35, 11, 23, 23, 0, 11, 11};
    vector<int32_t> audioVals = {138, 165, 138, 118, 139, 118, 58, 79, 58, 18, 39, 38, 1, 19, 18};
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
            EXPECT_EQ(frames_[0], videoVals[numbers_]);
            EXPECT_EQ(frames_[1], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: mp4Demuxer_SeekToTime_0014
 * @tc.desc: seek to the specified time
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_SeekToTime_0014, TestSize.Level1)
{
    InitResource(g_mp4Mpeg2AlacPath, LOCAL);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    // 边界情况
    EXPECT_EQ(demuxer_->SeekToTime(0, SeekMode::SEEK_PREVIOUS_SYNC), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SeekToTime(800, SeekMode::SEEK_PREVIOUS_SYNC), AV_ERR_OK);
    EXPECT_NE(demuxer_->SeekToTime(-10, SeekMode::SEEK_PREVIOUS_SYNC), AV_ERR_OK);
    EXPECT_NE(demuxer_->SeekToTime(5000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    // 普通时刻
    list<int64_t> toPtsList = {500, 800, 1500, 2100, 2633};
    vector<int32_t> videoVals = { 75, 85, 85, 75, 75, 75, 45, 55, 55, 35, 45, 35, 15, 25, 25};
    vector<int32_t> audioVals = { 29, 34, 34, 29, 30, 30, 17, 22, 22, 14, 18, 14,  6, 11, 10};
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
            EXPECT_EQ(frames_[0], videoVals[numbers_]);
            EXPECT_EQ(frames_[1], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: mp4Demuxer_SeekToTime_0015
 * @tc.desc: seek to the specified time
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerUnitTest, mp4Demuxer_SeekToTime_0015, TestSize.Level1)
{
    InitResource(g_mp4Mpeg2DtsPath, LOCAL);
    ASSERT_TRUE(initStatus_);
    EXPECT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    // 边界情况
    EXPECT_EQ(demuxer_->SeekToTime(0, SeekMode::SEEK_PREVIOUS_SYNC), AV_ERR_OK);
    EXPECT_EQ(demuxer_->SeekToTime(800, SeekMode::SEEK_PREVIOUS_SYNC), AV_ERR_OK);
    EXPECT_NE(demuxer_->SeekToTime(-10, SeekMode::SEEK_PREVIOUS_SYNC), AV_ERR_OK);
    EXPECT_NE(demuxer_->SeekToTime(5000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    // 普通时刻
    list<int64_t> toPtsList = {500, 800, 2100, 2633};
    vector<int32_t> videoVals = { 75,  85,  85,  75,  75,  75, 35, 45, 35, 15, 25, 25};
    vector<int32_t> audioVals = {232, 265, 264, 232, 233, 233, 108, 140, 108, 45, 77, 77};
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
            EXPECT_EQ(frames_[0], videoVals[numbers_]);
            EXPECT_EQ(frames_[1], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}
} // namespace