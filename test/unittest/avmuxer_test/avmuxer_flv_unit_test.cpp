/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <fcntl.h>
#include <fstream>
#include "avmuxer_sample.h"
#include "buffer/avbuffer.h"
#include "buffer/avbuffer_queue_producer.h"
#include "nocopyable.h"
#include "avmuxer.h"
#include "native_avbuffer.h"
#include "native_audio_channel_layout.h"
#ifdef AVMUXER_UNITTEST_CAPI
#include "native_avmuxer.h"
#include "native_avformat.h"
#endif

using namespace testing::ext;
using namespace OHOS::Media;
namespace OHOS {
namespace MediaAVCodec {
namespace {
const std::string TEST_FILE_PATH = "/data/test/media/";
constexpr int32_t TEST_CHANNEL_COUNT = 2;
constexpr int32_t TEST_SAMPLE_RATE = 44100;
constexpr int32_t TEST_WIDTH = 720;
constexpr int32_t TEST_HEIGHT = 480;
constexpr int32_t TEST_BIT_RATE = 128000;
constexpr int32_t TEST_FRAME_RATE = 30;
constexpr int32_t INVALID_FORMAT = -99;
const std::string INPUT_AUDIO_FILE_PATH = "/data/test/media/aac_2c_44100hz_199k_muxer.dat";
const std::string INPUT_VIDEO_FILE_PATH = "/data/test/media/h264_720_480.dat";
const std::string HEVC_LIB_PATH = std::string(AV_CODEC_PATH) + "/libav_codec_hevc_parser.z.so";
} // namespace

class AVMuxerFlvUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);
    int32_t WriteSample(int32_t trackId, std::shared_ptr<std::ifstream> file, bool &eosFlag, uint32_t flag);
    void TrackWriteSample(std::string inputFilePath, int32_t trackId);

protected:
    std::shared_ptr<AVMuxerSample> avmuxer_ {nullptr};
    std::shared_ptr<std::ifstream> inputFile_ = nullptr;
    int32_t fd_ {-1};
    uint8_t buffer_[26] = {
        'a', 'b', 'c', 'd', 'e', 'f', 'g',
        'h', 'i', 'j', 'k', 'l', 'm', 'n',
        'o', 'p', 'q', 'r', 's', 't',
        'u', 'v', 'w', 'x', 'y', 'z'
    };
};

void AVMuxerFlvUnitTest::SetUpTestCase()
{
}

void AVMuxerFlvUnitTest::TearDownTestCase()
{
}

void AVMuxerFlvUnitTest::SetUp()
{
    avmuxer_ = std::make_shared<AVMuxerSample>();
}

void AVMuxerFlvUnitTest::TearDown()
{
    if (fd_ >= 0) {
        close(fd_);
        fd_ = -1;
    }

    if (inputFile_ != nullptr) {
        if (inputFile_->is_open()) {
            inputFile_->close();
        }
    }

    if (avmuxer_ != nullptr) {
        avmuxer_->Destroy();
        avmuxer_ = nullptr;
    }
}

int32_t AVMuxerFlvUnitTest::WriteSample(int32_t trackId, std::shared_ptr<std::ifstream> file,
                                        bool &eosFlag, uint32_t flag)
{
    OH_AVCodecBufferAttr info;

    if (file->eof()) {
        eosFlag = true;
        return 0;
    }
    file->read(reinterpret_cast<char*>(&info.pts), sizeof(info.pts));

    if (file->eof()) {
        eosFlag = true;
        return 0;
    }
    file->read(reinterpret_cast<char*>(&info.flags), sizeof(info.flags));

    if (file->eof()) {
        eosFlag = true;
        return 0;
    }
    file->read(reinterpret_cast<char*>(&info.size), sizeof(info.size));

    if (file->eof()) {
        eosFlag = true;
        return 0;
    }

    if (info.flags & AVCODEC_BUFFER_FLAGS_SYNC_FRAME) {
        info.flags |= flag;
    }

    OH_AVBuffer *buffer = nullptr;
    if (info.size == 0) {
        buffer = OH_AVBuffer_Create(1);
    } else {
        buffer = OH_AVBuffer_Create(info.size);
        file->read(reinterpret_cast<char*>(OH_AVBuffer_GetAddr(buffer)), info.size);
    }
    OH_AVBuffer_SetBufferAttr(buffer, &info);
    int32_t ret = avmuxer_->WriteSampleBuffer(trackId, buffer);
    OH_AVBuffer_Destroy(buffer);
    return ret;
}

void AVMuxerFlvUnitTest::TrackWriteSample(std::string inputFilePath, int32_t trackId)
{
    inputFile_ = std::make_shared<std::ifstream>(inputFilePath, std::ios::binary);

    int32_t extSize = 0;
    inputFile_->read(reinterpret_cast<char*>(&extSize), sizeof(extSize));
    if (extSize > 0) {
        std::vector<uint8_t> buffer(extSize);
        inputFile_->read(reinterpret_cast<char*>(buffer.data()), extSize);
    }

    bool eosFlag = false;
    uint32_t flag = AVCODEC_BUFFER_FLAGS_SYNC_FRAME;
    int32_t ret = WriteSample(trackId, inputFile_, eosFlag, flag);
    while (!eosFlag && (ret == 0)) {
        ret = WriteSample(trackId, inputFile_, eosFlag, flag);
    }
    ASSERT_EQ(ret, 0);
}

namespace {
/**
 * @tc.name: Muxer_FLV_Create_001
 * @tc.desc: Create FLV muxer with valid fd and format
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerFlvUnitTest, Muxer_FLV_Create_001, TestSize.Level0)
{
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_FLV_Create.flv");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_FLV;
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);
}

/**
 * @tc.name: Muxer_FLV_Create_002
 * @tc.desc: Create FLV muxer with write-only fd
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerFlvUnitTest, Muxer_FLV_Create_002, TestSize.Level0)
{
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_FLV_Create_wo.flv");
    fd_ = open(outputFile.c_str(), O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_FLV;
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);
}

/**
 * @tc.name: Muxer_FLV_Create_003
 * @tc.desc: Create FLV muxer with read-only fd (should fail)
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerFlvUnitTest, Muxer_FLV_Create_003, TestSize.Level0)
{
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_FLV_Create_ro.flv");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDONLY, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_FLV;
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_FALSE(isCreated);
}

/**
 * @tc.name: Muxer_FLV_Create_004
 * @tc.desc: Create FLV muxer with invalid fd (should fail)
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerFlvUnitTest, Muxer_FLV_Create_004, TestSize.Level0)
{
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_FLV;
    bool isCreated = avmuxer_->CreateMuxer(-1, outputFormat);
    ASSERT_FALSE(isCreated);

    avmuxer_->Destroy();
    isCreated = avmuxer_->CreateMuxer(999999, outputFormat);
    ASSERT_FALSE(isCreated);
}

/**
 * @tc.name: Muxer_FLV_Create_005
 * @tc.desc: Create FLV muxer with unexpected format
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerFlvUnitTest, Muxer_FLV_Create_005, TestSize.Level0)
{
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_FLV_Create_fmt.flv");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, static_cast<OH_AVOutputFormat>(INVALID_FORMAT));
    ASSERT_FALSE(isCreated);
}

/**
 * @tc.name: Muxer_FLV_AddTrack_001
 * @tc.desc: Add audio track (AAC) to FLV muxer
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerFlvUnitTest, Muxer_FLV_AddTrack_001, TestSize.Level0)
{
    int32_t audioTrackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_FLV_AddTrack_AAC.flv");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_FLV;
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_AAC);
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, TEST_SAMPLE_RATE);
    audioParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, TEST_CHANNEL_COUNT);
    audioParams->PutIntValue(OH_MD_KEY_PROFILE, AAC_PROFILE_LC);
    int32_t ret = avmuxer_->AddTrack(audioTrackId, audioParams);
    EXPECT_EQ(ret, AV_ERR_OK);
    EXPECT_GE(audioTrackId, 0);
}

/**
 * @tc.name: Muxer_FLV_AddTrack_002
 * @tc.desc: Add audio track (MP3) to FLV muxer (unsupported)
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerFlvUnitTest, Muxer_FLV_AddTrack_002, TestSize.Level0)
{
    int32_t audioTrackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_FLV_AddTrack_MP3.flv");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_FLV;
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_MPEG);
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, TEST_SAMPLE_RATE);
    audioParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, TEST_CHANNEL_COUNT);
    int32_t ret = avmuxer_->AddTrack(audioTrackId, audioParams);
    EXPECT_NE(ret, AV_ERR_OK);
}

/**
 * @tc.name: Muxer_FLV_AddTrack_003
 * @tc.desc: Add video track (H.264/AVC) to FLV muxer
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerFlvUnitTest, Muxer_FLV_AddTrack_003, TestSize.Level0)
{
    int32_t videoTrackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_FLV_AddTrack_AVC.flv");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_FLV;
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> videoParams = FormatMockFactory::CreateFormat();
    videoParams->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    videoParams->PutIntValue(OH_MD_KEY_WIDTH, TEST_WIDTH);
    videoParams->PutIntValue(OH_MD_KEY_HEIGHT, TEST_HEIGHT);
    int32_t ret = avmuxer_->AddTrack(videoTrackId, videoParams);
    EXPECT_EQ(ret, AV_ERR_OK);
    EXPECT_GE(videoTrackId, 0);
}

/**
 * @tc.name: Muxer_FLV_AddTrack_004
 * @tc.desc: Add video track (HEVC/H.265) to FLV muxer
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerFlvUnitTest, Muxer_FLV_AddTrack_004, TestSize.Level0)
{
    int32_t videoTrackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_FLV_AddTrack_HEVC.flv");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_FLV;
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> videoParams = FormatMockFactory::CreateFormat();
    videoParams->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_VIDEO_HEVC);
    videoParams->PutIntValue(OH_MD_KEY_WIDTH, TEST_WIDTH);
    videoParams->PutIntValue(OH_MD_KEY_HEIGHT, TEST_HEIGHT);
    int32_t ret = avmuxer_->AddTrack(videoTrackId, videoParams);
    if (ret == AV_ERR_OK) {
        EXPECT_GE(videoTrackId, 0);
    } else {
        std::cout<<"not support muxer hecv"<<std::endl;
    }
}

/**
 * @tc.name: Muxer_FLV_AddTrack_005
 * @tc.desc: Add audio track missing mimeType (should fail)
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerFlvUnitTest, Muxer_FLV_AddTrack_005, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_FLV_AddTrack_no_mime.flv");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_FLV;
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, TEST_SAMPLE_RATE);
    audioParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, TEST_CHANNEL_COUNT);
    int32_t ret = avmuxer_->AddTrack(trackId, audioParams);
    EXPECT_NE(ret, AV_ERR_OK);
}

/**
 * @tc.name: Muxer_FLV_AddTrack_006
 * @tc.desc: Add audio track missing sample rate (should fail)
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerFlvUnitTest, Muxer_FLV_AddTrack_006, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_FLV_AddTrack_no_sr.flv");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_FLV;
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_AAC);
    audioParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, TEST_CHANNEL_COUNT);
    int32_t ret = avmuxer_->AddTrack(trackId, audioParams);
    EXPECT_NE(ret, AV_ERR_OK);
}

/**
 * @tc.name: Muxer_FLV_AddTrack_007
 * @tc.desc: Add audio track missing channel count (should fail)
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerFlvUnitTest, Muxer_FLV_AddTrack_007, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_FLV_AddTrack_no_ch.flv");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_FLV;
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_AAC);
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, TEST_SAMPLE_RATE);
    int32_t ret = avmuxer_->AddTrack(trackId, audioParams);
    EXPECT_NE(ret, AV_ERR_OK);
}

/**
 * @tc.name: Muxer_FLV_AddTrack_008
 * @tc.desc: Add video track missing width (should fail)
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerFlvUnitTest, Muxer_FLV_AddTrack_008, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_FLV_AddTrack_no_w.flv");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_FLV;
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> videoParams = FormatMockFactory::CreateFormat();
    videoParams->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    videoParams->PutIntValue(OH_MD_KEY_HEIGHT, TEST_HEIGHT);
    int32_t ret = avmuxer_->AddTrack(trackId, videoParams);
    EXPECT_NE(ret, AV_ERR_OK);
}

/**
 * @tc.name: Muxer_FLV_AddTrack_009
 * @tc.desc: Add video track missing height (should fail)
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerFlvUnitTest, Muxer_FLV_AddTrack_009, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_FLV_AddTrack_no_h.flv");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_FLV;
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> videoParams = FormatMockFactory::CreateFormat();
    videoParams->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    videoParams->PutIntValue(OH_MD_KEY_WIDTH, TEST_WIDTH);
    int32_t ret = avmuxer_->AddTrack(trackId, videoParams);
    EXPECT_NE(ret, AV_ERR_OK);
}

/**
 * @tc.name: Muxer_FLV_AddTrack_010
 * @tc.desc: Add track after Start (should fail)
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerFlvUnitTest, Muxer_FLV_AddTrack_010, TestSize.Level0)
{
    int32_t audioTrackId = -1;
    int32_t videoTrackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_FLV_AddTrack_after_start.flv");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_FLV;
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> videoParams = FormatMockFactory::CreateFormat();
    videoParams->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    videoParams->PutIntValue(OH_MD_KEY_WIDTH, TEST_WIDTH);
    videoParams->PutIntValue(OH_MD_KEY_HEIGHT, TEST_HEIGHT);
    int32_t ret = avmuxer_->AddTrack(videoTrackId, videoParams);
    EXPECT_EQ(ret, AV_ERR_OK);

    ASSERT_EQ(avmuxer_->Start(), 0);
    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_AAC);
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, TEST_SAMPLE_RATE);
    audioParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, TEST_CHANNEL_COUNT);
    ret = avmuxer_->AddTrack(audioTrackId, audioParams);
    EXPECT_NE(ret, AV_ERR_OK);
}

/**
 * @tc.name: Muxer_FLV_AddTrack_011
 * @tc.desc: Add video track with invalid width (exceed max)
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerFlvUnitTest, Muxer_FLV_AddTrack_011, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_FLV_AddTrack_inv_w.flv");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_FLV;
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> videoParams = FormatMockFactory::CreateFormat();
    videoParams->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    videoParams->PutIntValue(OH_MD_KEY_WIDTH, 70000); // exceeds 65535
    videoParams->PutIntValue(OH_MD_KEY_HEIGHT, TEST_HEIGHT);
    int32_t ret = avmuxer_->AddTrack(trackId, videoParams);
    EXPECT_NE(ret, AV_ERR_OK);
}

/**
 * @tc.name: Muxer_FLV_AddTrack_012
 * @tc.desc: Add video track with invalid height (0)
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerFlvUnitTest, Muxer_FLV_AddTrack_012, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_FLV_AddTrack_inv_h.flv");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_FLV;
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> videoParams = FormatMockFactory::CreateFormat();
    videoParams->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    videoParams->PutIntValue(OH_MD_KEY_WIDTH, TEST_WIDTH);
    videoParams->PutIntValue(OH_MD_KEY_HEIGHT, 0);
    int32_t ret = avmuxer_->AddTrack(trackId, videoParams);
    EXPECT_NE(ret, AV_ERR_OK);
}

/**
 * @tc.name: Muxer_FLV_StartStop_001
 * @tc.desc: Start and Stop FLV muxer normally with audio track
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerFlvUnitTest, Muxer_FLV_StartStop_001, TestSize.Level0)
{
    int32_t audioTrackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_FLV_StartStop.flv");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_FLV;
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_AAC);
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, TEST_SAMPLE_RATE);
    audioParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, TEST_CHANNEL_COUNT);
    audioParams->PutIntValue(OH_MD_KEY_PROFILE, AAC_PROFILE_LC);
    int32_t ret = avmuxer_->AddTrack(audioTrackId, audioParams);
    EXPECT_EQ(ret, AV_ERR_OK);

    ASSERT_EQ(avmuxer_->Start(), 0);
    ASSERT_EQ(avmuxer_->Stop(), 0);
}

/**
 * @tc.name: Muxer_FLV_StartStop_002
 * @tc.desc: Start without adding any track
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerFlvUnitTest, Muxer_FLV_StartStop_002, TestSize.Level0)
{
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_FLV_Start_no_track.flv");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_FLV;
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    ASSERT_NE(avmuxer_->Start(), 0);
    ASSERT_NE(avmuxer_->Stop(), 0);
}

/**
 * @tc.name: Muxer_FLV_StartStop_003
 * @tc.desc: Stop without Start (should fail)
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerFlvUnitTest, Muxer_FLV_StartStop_003, TestSize.Level0)
{
    int32_t audioTrackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_FLV_Stop_no_start.flv");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_FLV;
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_AAC);
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, TEST_SAMPLE_RATE);
    audioParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, TEST_CHANNEL_COUNT);
    int32_t ret = avmuxer_->AddTrack(audioTrackId, audioParams);
    EXPECT_EQ(ret, AV_ERR_OK);

    int32_t stopRet = avmuxer_->Stop();
    EXPECT_NE(stopRet, 0);
}

/**
 * @tc.name: Muxer_FLV_StartStop_004
 * @tc.desc: Start twice (second should fail)
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerFlvUnitTest, Muxer_FLV_StartStop_004, TestSize.Level0)
{
    int32_t audioTrackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_FLV_Start_twice.flv");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_FLV;
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_AAC);
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, TEST_SAMPLE_RATE);
    audioParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, TEST_CHANNEL_COUNT);
    int32_t ret = avmuxer_->AddTrack(audioTrackId, audioParams);
    EXPECT_EQ(ret, AV_ERR_OK);

    ASSERT_EQ(avmuxer_->Start(), 0);
    int32_t secondStart = avmuxer_->Start();
    EXPECT_NE(secondStart, 0);
}

/**
 * @tc.name: Muxer_FLV_WriteSample_001
 * @tc.desc: Write audio samples to FLV muxer
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerFlvUnitTest, Muxer_FLV_WriteSample_001, TestSize.Level0)
{
    int32_t audioTrackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_FLV_WriteSample_audio.flv");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_FLV;
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_AAC);
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, TEST_SAMPLE_RATE);
    audioParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, TEST_CHANNEL_COUNT);
    audioParams->PutIntValue(OH_MD_KEY_PROFILE, AAC_PROFILE_LC);
    int32_t ret = avmuxer_->AddTrack(audioTrackId, audioParams);
    EXPECT_EQ(ret, AV_ERR_OK);
    ASSERT_EQ(avmuxer_->Start(), 0);

    inputFile_ = std::make_shared<std::ifstream>(INPUT_AUDIO_FILE_PATH, std::ios::binary);
    if (inputFile_->is_open()) {
        int32_t extSize = 0;
        inputFile_->read(reinterpret_cast<char*>(&extSize), sizeof(extSize));
        if (extSize > 0) {
            std::vector<uint8_t> buf(extSize);
            inputFile_->read(reinterpret_cast<char*>(buf.data()), extSize);
        }

        bool eosFlag = false;
        uint32_t flag = AVCODEC_BUFFER_FLAGS_SYNC_FRAME;
        ret = WriteSample(audioTrackId, inputFile_, eosFlag, flag);
        while (!eosFlag && (ret == 0)) {
            ret = WriteSample(audioTrackId, inputFile_, eosFlag, flag);
        }
        EXPECT_EQ(ret, 0);
    }
    ASSERT_EQ(avmuxer_->Stop(), 0);
}

/**
 * @tc.name: Muxer_FLV_WriteSample_002
 * @tc.desc: Write video samples to FLV muxer
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerFlvUnitTest, Muxer_FLV_WriteSample_002, TestSize.Level0)
{
    int32_t videoTrackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_FLV_WriteSample_video.flv");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_FLV;
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> videoParams = FormatMockFactory::CreateFormat();
    videoParams->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    videoParams->PutIntValue(OH_MD_KEY_WIDTH, TEST_WIDTH);
    videoParams->PutIntValue(OH_MD_KEY_HEIGHT, TEST_HEIGHT);
    videoParams->PutBuffer(OH_MD_KEY_CODEC_CONFIG, buffer_, sizeof(buffer_));
    int32_t ret = avmuxer_->AddTrack(videoTrackId, videoParams);
    EXPECT_EQ(ret, AV_ERR_OK);
    ASSERT_EQ(avmuxer_->Start(), 0);

    TrackWriteSample(INPUT_VIDEO_FILE_PATH, videoTrackId);
    ASSERT_EQ(avmuxer_->Stop(), 0);
}

/**
 * @tc.name: Muxer_FLV_WriteSample_003
 * @tc.desc: Write sample before Start (should fail)
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerFlvUnitTest, Muxer_FLV_WriteSample_003, TestSize.Level0)
{
    int32_t audioTrackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_FLV_Write_before_start.flv");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_FLV;
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_AAC);
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, TEST_SAMPLE_RATE);
    audioParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, TEST_CHANNEL_COUNT);
    int32_t ret = avmuxer_->AddTrack(audioTrackId, audioParams);
    EXPECT_EQ(ret, AV_ERR_OK);

    OH_AVCodecBufferAttr info;
    info.pts = 0;
    info.size = sizeof(buffer_);
    info.offset = 0;
    info.flags = AVCODEC_BUFFER_FLAGS_SYNC_FRAME;
    OH_AVBuffer *buffer = OH_AVBuffer_Create(info.size);
    memcpy_s(OH_AVBuffer_GetAddr(buffer), info.size, buffer_, info.size);
    OH_AVBuffer_SetBufferAttr(buffer, &info);
    ret = avmuxer_->WriteSampleBuffer(audioTrackId, buffer);
    OH_AVBuffer_Destroy(buffer);
    EXPECT_NE(ret, AV_ERR_OK);
}

/**
 * @tc.name: Muxer_FLV_MuxAudioVideo_001
 * @tc.desc: Mux both audio and video tracks to FLV
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerFlvUnitTest, Muxer_FLV_MuxAudioVideo_001, TestSize.Level0)
{
    int32_t audioTrackId = -1;
    int32_t videoTrackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_FLV_AudioVideo.flv");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_FLV;
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> videoParams = FormatMockFactory::CreateFormat();
    videoParams->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    videoParams->PutIntValue(OH_MD_KEY_WIDTH, TEST_WIDTH);
    videoParams->PutIntValue(OH_MD_KEY_HEIGHT, TEST_HEIGHT);
    videoParams->PutBuffer(OH_MD_KEY_CODEC_CONFIG, buffer_, sizeof(buffer_));
    int32_t ret = avmuxer_->AddTrack(videoTrackId, videoParams);
    EXPECT_EQ(ret, AV_ERR_OK);
    EXPECT_GE(videoTrackId, 0);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_AAC);
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, TEST_SAMPLE_RATE);
    audioParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, TEST_CHANNEL_COUNT);
    audioParams->PutIntValue(OH_MD_KEY_PROFILE, AAC_PROFILE_LC);
    ret = avmuxer_->AddTrack(audioTrackId, audioParams);
    EXPECT_EQ(ret, AV_ERR_OK);
    EXPECT_GE(audioTrackId, 0);

    ASSERT_EQ(avmuxer_->Start(), 0);

    // Write video samples
    TrackWriteSample(INPUT_VIDEO_FILE_PATH, videoTrackId);

    // Write audio samples
    std::shared_ptr<std::ifstream> audioFile =
        std::make_shared<std::ifstream>(INPUT_AUDIO_FILE_PATH, std::ios::binary);
    if (audioFile->is_open()) {
        int32_t extSize = 0;
        audioFile->read(reinterpret_cast<char*>(&extSize), sizeof(extSize));
        if (extSize > 0) {
            std::vector<uint8_t> buf(extSize);
            audioFile->read(reinterpret_cast<char*>(buf.data()), extSize);
        }

        bool eosFlag = false;
        uint32_t flag = AVCODEC_BUFFER_FLAGS_SYNC_FRAME;
        ret = WriteSample(audioTrackId, audioFile, eosFlag, flag);
        while (!eosFlag && (ret == 0)) {
            ret = WriteSample(audioTrackId, audioFile, eosFlag, flag);
        }
        EXPECT_EQ(ret, 0);
        audioFile->close();
    }

    ASSERT_EQ(avmuxer_->Stop(), 0);
}

/**
 * @tc.name: Muxer_FLV_VideoTrackWithCodecConfig_001
 * @tc.desc: Add video track with codec config (extradata)
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerFlvUnitTest, Muxer_FLV_VideoTrackWithCodecConfig_001, TestSize.Level0)
{
    int32_t videoTrackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_FLV_CodecConfig.flv");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_FLV;
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> videoParams = FormatMockFactory::CreateFormat();
    videoParams->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    videoParams->PutIntValue(OH_MD_KEY_WIDTH, TEST_WIDTH);
    videoParams->PutIntValue(OH_MD_KEY_HEIGHT, TEST_HEIGHT);
    videoParams->PutBuffer(OH_MD_KEY_CODEC_CONFIG, buffer_, sizeof(buffer_));
    int32_t ret = avmuxer_->AddTrack(videoTrackId, videoParams);
    EXPECT_EQ(ret, AV_ERR_OK);
    EXPECT_GE(videoTrackId, 0);

    ASSERT_EQ(avmuxer_->Start(), 0);
    ASSERT_EQ(avmuxer_->Stop(), 0);
}

/**
 * @tc.name: Muxer_FLV_AudioTrackWithChannelLayout_001
 * @tc.desc: Add audio track with channel layout
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerFlvUnitTest, Muxer_FLV_AudioTrackWithChannelLayout_001, TestSize.Level0)
{
    int32_t audioTrackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_FLV_ChannelLayout.flv");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_FLV;
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_AAC);
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, TEST_SAMPLE_RATE);
    audioParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, TEST_CHANNEL_COUNT);
    audioParams->PutLongValue(OH_MD_KEY_CHANNEL_LAYOUT, CH_LAYOUT_STEREO);
    int32_t ret = avmuxer_->AddTrack(audioTrackId, audioParams);
    EXPECT_EQ(ret, AV_ERR_OK);
    EXPECT_GE(audioTrackId, 0);
}

/**
 * @tc.name: Muxer_FLV_VideoTrackWithFrameRate_001
 * @tc.desc: Add video track with frame rate
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerFlvUnitTest, Muxer_FLV_VideoTrackWithFrameRate_001, TestSize.Level0)
{
    int32_t videoTrackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_FLV_FrameRate.flv");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_FLV;
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> videoParams = FormatMockFactory::CreateFormat();
    videoParams->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    videoParams->PutIntValue(OH_MD_KEY_WIDTH, TEST_WIDTH);
    videoParams->PutIntValue(OH_MD_KEY_HEIGHT, TEST_HEIGHT);
    videoParams->PutDoubleValue(OH_MD_KEY_FRAME_RATE, TEST_FRAME_RATE);
    int32_t ret = avmuxer_->AddTrack(videoTrackId, videoParams);
    EXPECT_EQ(ret, AV_ERR_OK);
    EXPECT_GE(videoTrackId, 0);
}

/**
 * @tc.name: Muxer_FLV_AudioTrackWithBitRate_001
 * @tc.desc: Add audio track with bit rate
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerFlvUnitTest, Muxer_FLV_AudioTrackWithBitRate_001, TestSize.Level0)
{
    int32_t audioTrackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_FLV_BitRate.flv");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_FLV;
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_AAC);
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, TEST_SAMPLE_RATE);
    audioParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, TEST_CHANNEL_COUNT);
    audioParams->PutLongValue(OH_MD_KEY_BITRATE, TEST_BIT_RATE);
    int32_t ret = avmuxer_->AddTrack(audioTrackId, audioParams);
    EXPECT_EQ(ret, AV_ERR_OK);
    EXPECT_GE(audioTrackId, 0);
}

/**
 * @tc.name: Muxer_FLV_Destroy_001
 * @tc.desc: Destroy muxer normally
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerFlvUnitTest, Muxer_FLV_Destroy_001, TestSize.Level0)
{
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_FLV_Destroy.flv");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_FLV;
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    int32_t ret = avmuxer_->Destroy();
    EXPECT_EQ(ret, AV_ERR_OK);
    avmuxer_ = nullptr;
}

/**
 * @tc.name: Muxer_FLV_Destroy_002
 * @tc.desc: Destroy muxer after Start/Stop lifecycle
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerFlvUnitTest, Muxer_FLV_Destroy_002, TestSize.Level0)
{
    int32_t audioTrackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_FLV_DestroyAfterStop.flv");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_FLV;
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_AAC);
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, TEST_SAMPLE_RATE);
    audioParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, TEST_CHANNEL_COUNT);
    avmuxer_->AddTrack(audioTrackId, audioParams);
    ASSERT_EQ(avmuxer_->Start(), 0);
    ASSERT_EQ(avmuxer_->Stop(), 0);

    int32_t ret = avmuxer_->Destroy();
    EXPECT_EQ(ret, AV_ERR_OK);
    avmuxer_ = nullptr;
}

/**
 * @tc.name: Muxer_FLV_WriteSample_InvalidTrackIndex_001
 * @tc.desc: Write sample with invalid track index
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerFlvUnitTest, Muxer_FLV_WriteSample_InvalidTrackIndex_001, TestSize.Level0)
{
    int32_t audioTrackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_FLV_InvalidTrack.flv");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_FLV;
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_AAC);
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, TEST_SAMPLE_RATE);
    audioParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, TEST_CHANNEL_COUNT);
    int32_t ret = avmuxer_->AddTrack(audioTrackId, audioParams);
    EXPECT_EQ(ret, AV_ERR_OK);
    ASSERT_EQ(avmuxer_->Start(), 0);

    OH_AVCodecBufferAttr info;
    info.pts = 0;
    info.size = sizeof(buffer_);
    info.offset = 0;
    info.flags = AVCODEC_BUFFER_FLAGS_SYNC_FRAME;
    OH_AVBuffer *buffer = OH_AVBuffer_Create(info.size);
    memcpy_s(OH_AVBuffer_GetAddr(buffer), info.size, buffer_, info.size);
    OH_AVBuffer_SetBufferAttr(buffer, &info);
    ret = avmuxer_->WriteSampleBuffer(999, buffer);
    OH_AVBuffer_Destroy(buffer);
    EXPECT_NE(ret, AV_ERR_OK);
}
} // namespace
} // namespace MediaAVCodec
} // namespace OHOS
