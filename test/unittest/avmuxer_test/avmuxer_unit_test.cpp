/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include "avmuxer_unit_test.h"
#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <fcntl.h>
#include "avmuxer.h"
#include "native_avbuffer.h"
#ifdef AVMUXER_UNITTEST_CAPI
#include "native_avmuxer.h"
#include "native_avformat.h"
#endif

using namespace testing::ext;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::Media;
namespace {
constexpr int32_t TEST_CHANNEL_COUNT = 2;
constexpr int32_t TEST_SAMPLE_RATE = 2;
constexpr int32_t TEST_WIDTH = 720;
constexpr int32_t TEST_HEIGHT = 480;
constexpr int32_t TEST_ROTATION = 90;
constexpr int32_t INVALID_FORMAT = -99;
const std::string TEST_FILE_PATH = "/data/test/media/";
const std::string INPUT_FILE_PATH = "/data/test/media/h264_720_480.dat";
const std::string HEVC_LIB_PATH = std::string(AV_CODEC_PATH) + "/libav_codec_hevc_parser.z.so";
constexpr uint32_t AVCODEC_BUFFER_FLAGS_DISPOSABLE_EXT_TEST = 1 << 6;
} // namespace

void AVMuxerUnitTest::SetUpTestCase() {}

void AVMuxerUnitTest::TearDownTestCase() {}

void AVMuxerUnitTest::SetUp()
{
    avmuxer_ = std::make_shared<AVMuxerSample>();
}

void AVMuxerUnitTest::TearDown()
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

int32_t AVMuxerUnitTest::WriteSample(int32_t trackId, std::shared_ptr<std::ifstream> file,
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

    OH_AVBuffer *buffer = OH_AVBuffer_Create(info.size);
    file->read(reinterpret_cast<char*>(OH_AVBuffer_GetAddr(buffer)), info.size);
    OH_AVBuffer_SetBufferAttr(buffer, &info);
    int32_t ret = avmuxer_->WriteSampleBuffer(trackId, buffer);
    OH_AVBuffer_Destroy(buffer);
    return ret;
}

namespace {
/**
 * @tc.name: Muxer_Create_001
 * @tc.desc: Muxer Create
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_Create_001, TestSize.Level0)
{
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_Create.mp4");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);
}

/**
 * @tc.name: Muxer_Create_002
 * @tc.desc: Muxer Create write only
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_Create_002, TestSize.Level0)
{
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_Create.mp4");
    fd_ = open(outputFile.c_str(), O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_FALSE(isCreated);
}

/**
 * @tc.name: Muxer_Create_003
 * @tc.desc: Muxer Create read only
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_Create_003, TestSize.Level0)
{
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_Create.mp4");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDONLY, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_FALSE(isCreated);
}

/**
 * @tc.name: Muxer_Create_004
 * @tc.desc: Muxer Create rand fd
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_Create_004, TestSize.Level0)
{
    constexpr int32_t invalidFd = 999999;
    constexpr int32_t negativeFd = -999;
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;
    bool isCreated = avmuxer_->CreateMuxer(invalidFd, outputFormat);
    ASSERT_FALSE(isCreated);

    avmuxer_->Destroy();
    isCreated = avmuxer_->CreateMuxer(0xfffffff, outputFormat);
    ASSERT_FALSE(isCreated);

    avmuxer_->Destroy();
    isCreated = avmuxer_->CreateMuxer(-1, outputFormat);
    ASSERT_FALSE(isCreated);

    avmuxer_->Destroy();
    isCreated = avmuxer_->CreateMuxer(negativeFd, outputFormat);
    ASSERT_FALSE(isCreated);
}

/**
 * @tc.name: Muxer_Create_005
 * @tc.desc: Muxer Create different outputFormat
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_Create_005, TestSize.Level0)
{
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_Create.mp4");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);

    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    avmuxer_->Destroy();
    outputFormat = AV_OUTPUT_FORMAT_M4A;
    isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    avmuxer_->Destroy();
    outputFormat = AV_OUTPUT_FORMAT_AMR;
    isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    avmuxer_->Destroy();
    outputFormat = AV_OUTPUT_FORMAT_DEFAULT;
    isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    avmuxer_->Destroy();
    outputFormat = AV_OUTPUT_FORMAT_MP3;
    isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    avmuxer_->Destroy();
    isCreated = avmuxer_->CreateMuxer(fd_, static_cast<OH_AVOutputFormat>(INVALID_FORMAT));
    ASSERT_FALSE(isCreated);
}

/**
 * @tc.name: Muxer_AddTrack_001
 * @tc.desc: Muxer AddTrack add audio track
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_AddTrack_001, TestSize.Level0)
{
    int32_t audioTrackId = -1;
    int32_t ret = 0;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_AddTrack.mp4");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_MPEG);
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, TEST_SAMPLE_RATE);
    audioParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, TEST_CHANNEL_COUNT);
    ret = avmuxer_->AddTrack(audioTrackId, audioParams);
    EXPECT_EQ(ret, AV_ERR_OK);
    EXPECT_GE(audioTrackId, 0);

    audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, TEST_SAMPLE_RATE);
    audioParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, TEST_CHANNEL_COUNT);
    ret = avmuxer_->AddTrack(audioTrackId, audioParams);
    EXPECT_NE(ret, AV_ERR_OK);

    audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_MPEG);
    audioParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, TEST_CHANNEL_COUNT);
    ret = avmuxer_->AddTrack(audioTrackId, audioParams);
    EXPECT_NE(ret, AV_ERR_OK);

    audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_MPEG);
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, TEST_SAMPLE_RATE);
    ret = avmuxer_->AddTrack(audioTrackId, audioParams);
    EXPECT_NE(ret, AV_ERR_OK);
}

/**
 * @tc.name: Muxer_AddTrack_002
 * @tc.desc: Muxer AddTrack add video track
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_AddTrack_002, TestSize.Level0)
{
    int32_t videoTrackId = -1;
    int32_t ret = AV_ERR_INVALID_VAL;
    std::string outputFile = TEST_FILE_PATH + std::string("avmuxer_AddTrack_002.mp4");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> videoParams = FormatMockFactory::CreateFormat();
    videoParams->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    videoParams->PutIntValue(OH_MD_KEY_WIDTH, TEST_WIDTH);
    videoParams->PutIntValue(OH_MD_KEY_HEIGHT, TEST_HEIGHT);
    ret = avmuxer_->AddTrack(videoTrackId, videoParams);
    EXPECT_EQ(ret, AV_ERR_OK);
    EXPECT_GE(videoTrackId, 0);

    videoParams = FormatMockFactory::CreateFormat();
    videoParams->PutIntValue(OH_MD_KEY_WIDTH, TEST_WIDTH);
    videoParams->PutIntValue(OH_MD_KEY_HEIGHT, TEST_HEIGHT);
    ret = avmuxer_->AddTrack(videoTrackId, videoParams);
    EXPECT_NE(ret, AV_ERR_OK);

    videoParams = FormatMockFactory::CreateFormat();
    videoParams->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    videoParams->PutIntValue(OH_MD_KEY_HEIGHT, TEST_HEIGHT);
    ret = avmuxer_->AddTrack(videoTrackId, videoParams);
    EXPECT_NE(ret, AV_ERR_OK);

    videoParams = FormatMockFactory::CreateFormat();
    videoParams->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    videoParams->PutIntValue(OH_MD_KEY_WIDTH, TEST_WIDTH);
    ret = avmuxer_->AddTrack(videoTrackId, videoParams);
    EXPECT_NE(ret, AV_ERR_OK);
}

/**
 * @tc.name: Muxer_AddTrack_003
 * @tc.desc: Muxer AddTrack after Start()
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_AddTrack_003, TestSize.Level0)
{
    int32_t audioTrackId = -1;
    int32_t videoTrackId = -1;
    int32_t ret = AV_ERR_INVALID_VAL;
    std::string outputFile = TEST_FILE_PATH + std::string("avmuxer_AddTrack_003.mp4");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> videoParams =
        FormatMockFactory::CreateVideoFormat(OH_AVCODEC_MIMETYPE_VIDEO_AVC, TEST_WIDTH, TEST_HEIGHT);
    videoParams->PutBuffer(OH_MD_KEY_CODEC_CONFIG, buffer_, sizeof(buffer_));
    ret = avmuxer_->AddTrack(videoTrackId, videoParams);
    EXPECT_EQ(ret, AV_ERR_OK);
    EXPECT_GE(videoTrackId, 0);

    ASSERT_EQ(avmuxer_->Start(), 0);
    std::shared_ptr<FormatMock> audioParams =
        FormatMockFactory::CreateAudioFormat(OH_AVCODEC_MIMETYPE_AUDIO_MPEG, TEST_SAMPLE_RATE, TEST_CHANNEL_COUNT);
    ret = avmuxer_->AddTrack(audioTrackId, audioParams);
    EXPECT_NE(ret, AV_ERR_OK);
}

/**
 * @tc.name: Muxer_AddTrack_004
 * @tc.desc: Muxer AddTrack mimeType test
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_AddTrack_004, TestSize.Level0)
{
    int32_t trackId = -1;
    int32_t ret = 0;
    const std::vector<std::string_view> testMp4MimeTypeList =
    {
        OH_AVCODEC_MIMETYPE_AUDIO_MPEG,
        // OH_AVCODEC_MIMETYPE_AUDIO_FLAC,
        // OH_AVCODEC_MIMETYPE_AUDIO_RAW,
        OH_AVCODEC_MIMETYPE_AUDIO_AAC,
        // OH_AVCODEC_MIMETYPE_AUDIO_VORBIS,
        // OH_AVCODEC_MIMETYPE_AUDIO_OPUS,
        // OH_AVCODEC_MIMETYPE_AUDIO_AMR_NB,
        // OH_AVCODEC_MIMETYPE_AUDIO_AMR_WB,
        OH_AVCODEC_MIMETYPE_VIDEO_AVC,
        OH_AVCODEC_MIMETYPE_VIDEO_MPEG4,
        OH_AVCODEC_MIMETYPE_IMAGE_JPG,
        OH_AVCODEC_MIMETYPE_IMAGE_PNG,
        OH_AVCODEC_MIMETYPE_IMAGE_BMP,
    };

    const std::vector<std::string_view> testM4aMimeTypeList =
    {
        OH_AVCODEC_MIMETYPE_AUDIO_AAC,
        // OH_AVCODEC_MIMETYPE_VIDEO_AVC,
        // OH_AVCODEC_MIMETYPE_VIDEO_MPEG4,
        OH_AVCODEC_MIMETYPE_IMAGE_JPG,
        OH_AVCODEC_MIMETYPE_IMAGE_PNG,
        OH_AVCODEC_MIMETYPE_IMAGE_BMP,
    };

    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_AddTrack.mp4");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;

    std::shared_ptr<FormatMock> avParam = FormatMockFactory::CreateFormat();
    avParam->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, TEST_SAMPLE_RATE);
    avParam->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, TEST_CHANNEL_COUNT);
    avParam->PutIntValue(OH_MD_KEY_WIDTH, TEST_WIDTH);
    avParam->PutIntValue(OH_MD_KEY_HEIGHT, TEST_HEIGHT);
    avParam->PutIntValue(OH_MD_KEY_PROFILE, 0);

    for (uint32_t i = 0; i < testMp4MimeTypeList.size(); ++i) {
        bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
        ASSERT_TRUE(isCreated);
        avParam->PutStringValue(OH_MD_KEY_CODEC_MIME, testMp4MimeTypeList[i]);
        ret = avmuxer_->AddTrack(trackId, avParam);
        EXPECT_EQ(ret, AV_ERR_OK) << "AddTrack failed: i:" << i << " mimeType:" << testMp4MimeTypeList[i];
        EXPECT_EQ(trackId, 0) << "i:" << i << " TrackId:" << trackId << " mimeType:" << testMp4MimeTypeList[i];
    }

    // need to change libohosffmpeg.z.so, muxer build config add ipod
    avmuxer_->Destroy();
    outputFormat = AV_OUTPUT_FORMAT_M4A;
    avParam->PutBuffer(OH_MD_KEY_CODEC_CONFIG, buffer_, sizeof(buffer_));
    for (uint32_t i = 0; i < testM4aMimeTypeList.size(); ++i) {
        bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
        ASSERT_TRUE(isCreated);
        avParam->PutStringValue(OH_MD_KEY_CODEC_MIME, testM4aMimeTypeList[i]);
        ret = avmuxer_->AddTrack(trackId, avParam);
        EXPECT_EQ(ret, AV_ERR_OK) << "AddTrack failed: i:" << i << " mimeType:" << testM4aMimeTypeList[i];
        EXPECT_EQ(trackId, 0) << "i:" << i << " TrackId:" << trackId << " mimeType:" << testM4aMimeTypeList[i];
    }
}

/**
 * @tc.name: Muxer_AddTrack_0091
 * @tc.desc: Muxer AddTrack while create by unexpected outputFormat
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_AddTrack_0091, TestSize.Level0)
{
    int32_t audioTrackId = -1; // -1 track
    int32_t ret = 0;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_AddTrack.mp3");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MP3;
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_MPEG);
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, TEST_SAMPLE_RATE);
    audioParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, TEST_CHANNEL_COUNT);
    ret = avmuxer_->AddTrack(audioTrackId, audioParams);
    EXPECT_EQ(ret, AV_ERR_OK);
    EXPECT_GE(audioTrackId, 0);
}

/**
 * @tc.name: Muxer_AddTrack_005
 * @tc.desc: Muxer AddTrack while create by unexpected outputFormat
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_AddTrack_005, TestSize.Level0)
{
    int32_t trackId = -2;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_AddTrack.mp4");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, static_cast<OH_AVOutputFormat>(INVALID_FORMAT));
    ASSERT_FALSE(isCreated);

    std::shared_ptr<FormatMock> avParam = FormatMockFactory::CreateFormat();
    avParam->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_MPEG);
    avParam->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, TEST_SAMPLE_RATE);
    avParam->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, TEST_CHANNEL_COUNT);
    avParam->PutIntValue(OH_MD_KEY_WIDTH, TEST_WIDTH);
    avParam->PutIntValue(OH_MD_KEY_HEIGHT, TEST_HEIGHT);

    int32_t ret = avmuxer_->AddTrack(trackId, avParam);
    ASSERT_NE(ret, 0);

    avParam->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ret = avmuxer_->AddTrack(trackId, avParam);
    ASSERT_NE(ret, 0);
}

/**
 * @tc.name: Muxer_AddTrack_006
 * @tc.desc: Muxer AddTrack while create by unexpected value
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_AddTrack_006, TestSize.Level0)
{
    int32_t trackId = -2;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_AddTrack.mp4");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParam = FormatMockFactory::CreateFormat();
    audioParam->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_MPEG);
    audioParam->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, TEST_SAMPLE_RATE);
    audioParam->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, -1);
    int32_t ret = avmuxer_->AddTrack(trackId, audioParam);
    ASSERT_NE(ret, 0);

    audioParam->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, -1);
    audioParam->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, TEST_CHANNEL_COUNT);
    ret = avmuxer_->AddTrack(trackId, audioParam);
    ASSERT_NE(ret, 0);

    // test add video track
    std::shared_ptr<FormatMock> videoParam = FormatMockFactory::CreateFormat();
    videoParam->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    videoParam->PutIntValue(OH_MD_KEY_WIDTH, TEST_WIDTH);
    videoParam->PutIntValue(OH_MD_KEY_HEIGHT, -1);
    ret = avmuxer_->AddTrack(trackId, videoParam);
    ASSERT_NE(ret, 0);

    videoParam->PutIntValue(OH_MD_KEY_WIDTH, -1);
    videoParam->PutIntValue(OH_MD_KEY_HEIGHT, TEST_HEIGHT);
    ret = avmuxer_->AddTrack(trackId, videoParam);
    ASSERT_NE(ret, 0);

    videoParam->PutIntValue(OH_MD_KEY_WIDTH, TEST_WIDTH);
    videoParam->PutIntValue(OH_MD_KEY_HEIGHT, 0xFFFF + 1);
    ret = avmuxer_->AddTrack(trackId, videoParam);
    ASSERT_NE(ret, 0);

    videoParam->PutIntValue(OH_MD_KEY_WIDTH, 0xFFFF + 1);
    videoParam->PutIntValue(OH_MD_KEY_HEIGHT, TEST_HEIGHT);
    ret = avmuxer_->AddTrack(trackId, videoParam);
    ASSERT_NE(ret, 0);

    videoParam->PutIntValue(OH_MD_KEY_WIDTH, 0xFFFF);
    videoParam->PutIntValue(OH_MD_KEY_HEIGHT, 0xFFFF);
    ret = avmuxer_->AddTrack(trackId, videoParam);
    ASSERT_EQ(ret, 0);
}

/**
 * @tc.name: Muxer_AddTrack_007
 * @tc.desc: Create amr-nb Muxer AddTrack
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_AddTrack_007, TestSize.Level0)
{
    int32_t trackId = -2; // -2: Initialize to an invalid ID
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_AddTrack.amr");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, static_cast<OH_AVOutputFormat>(AV_OUTPUT_FORMAT_AMR));
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> avParam = FormatMockFactory::CreateFormat();
    avParam->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_AMR_NB);
    avParam->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, 8000); // 8000: 8khz sample rate
    avParam->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, 1); // 1: 1 audio channel,mono

    int32_t ret = avmuxer_->AddTrack(trackId, avParam);
    ASSERT_EQ(ret, 0);
}

/**
 * @tc.name: Muxer_AddTrack_008
 * @tc.desc: Create amr-wb Muxer AddTrack
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_AddTrack_008, TestSize.Level0)
{
    int32_t trackId = -2; // -2: Initialize to an invalid ID
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_AddTrack.amr");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, static_cast<OH_AVOutputFormat>(AV_OUTPUT_FORMAT_AMR));
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> avParam = FormatMockFactory::CreateFormat();
    avParam->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_AMR_WB);
    avParam->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, 16000); // 16000: 16khz sample rate
    avParam->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, 1); // 1: 1 audio channel, mono

    int32_t ret = avmuxer_->AddTrack(trackId, avParam);
    ASSERT_EQ(ret, 0);
}

/**
 * @tc.name: Muxer_Start_001
 * @tc.desc: Muxer Start normal
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_Start_001, TestSize.Level0)
{
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_Start.mp4");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> videoParams =
        FormatMockFactory::CreateVideoFormat(OH_AVCODEC_MIMETYPE_VIDEO_AVC, TEST_WIDTH, TEST_HEIGHT);
    videoParams->PutBuffer(OH_MD_KEY_CODEC_CONFIG, buffer_, sizeof(buffer_));

    int32_t videoTrackId = -1;
    int32_t ret = avmuxer_->AddTrack(videoTrackId, videoParams);
    ASSERT_EQ(ret, AV_ERR_OK);
    ASSERT_GE(videoTrackId, 0);
    EXPECT_EQ(avmuxer_->Start(), 0);
}

/**
 * @tc.name: Muxer_Start_002
 * @tc.desc: Muxer Start twice
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_Start_002, TestSize.Level0)
{
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_Start.mp4");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> videoParams =
        FormatMockFactory::CreateVideoFormat(OH_AVCODEC_MIMETYPE_VIDEO_AVC, TEST_WIDTH, TEST_HEIGHT);
    videoParams->PutBuffer(OH_MD_KEY_CODEC_CONFIG, buffer_, sizeof(buffer_));

    int32_t videoTrackId = -1;
    int32_t ret = avmuxer_->AddTrack(videoTrackId, videoParams);
    ASSERT_EQ(ret, AV_ERR_OK);
    ASSERT_GE(videoTrackId, 0);
    ASSERT_EQ(avmuxer_->Start(), 0);
    EXPECT_NE(avmuxer_->Start(), 0);
}

/**
 * @tc.name: Muxer_Start_003
 * @tc.desc: Muxer Start after Create()
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_Start_003, TestSize.Level0)
{
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_Start.mp4");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);
    EXPECT_NE(avmuxer_->Start(), 0);
}

/**
 * @tc.name: Muxer_Start_004
 * @tc.desc: Muxer Start after Stop()
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_Start_004, TestSize.Level0)
{
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_Start.mp4");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> videoParams =
        FormatMockFactory::CreateVideoFormat(OH_AVCODEC_MIMETYPE_VIDEO_AVC, TEST_WIDTH, TEST_HEIGHT);
    videoParams->PutBuffer(OH_MD_KEY_CODEC_CONFIG, buffer_, sizeof(buffer_));

    int32_t videoTrackId = -1;
    int32_t ret = avmuxer_->AddTrack(videoTrackId, videoParams);
    ASSERT_EQ(ret, AV_ERR_OK);
    ASSERT_GE(videoTrackId, 0);
    ASSERT_EQ(avmuxer_->Start(), 0);
    ASSERT_EQ(avmuxer_->Stop(), 0);

    EXPECT_NE(avmuxer_->Start(), 0);
}

/**
 * @tc.name: Muxer_Start_005
 * @tc.desc: Muxer Start while create by unexpected outputFormat
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_Start_005, TestSize.Level0)
{
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_Start.mp4");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, static_cast<OH_AVOutputFormat>(INVALID_FORMAT));
    ASSERT_FALSE(isCreated);

    std::shared_ptr<FormatMock> videoParams =
        FormatMockFactory::CreateVideoFormat(OH_AVCODEC_MIMETYPE_VIDEO_AVC, TEST_WIDTH, TEST_HEIGHT);
    videoParams->PutBuffer(OH_MD_KEY_CODEC_CONFIG, buffer_, sizeof(buffer_));

    int32_t videoTrackId = -1;
    int32_t ret = avmuxer_->AddTrack(videoTrackId, videoParams);
    ASSERT_NE(ret, AV_ERR_OK);
    ASSERT_LT(videoTrackId, 0);
    ASSERT_NE(avmuxer_->Start(), 0);
}

/**
 * @tc.name: Muxer_Stop_001
 * @tc.desc: Muxer Stop() normal
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_Stop_001, TestSize.Level0)
{
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_Stop.mp4");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> videoParams =
        FormatMockFactory::CreateVideoFormat(OH_AVCODEC_MIMETYPE_VIDEO_AVC, TEST_WIDTH, TEST_HEIGHT);
    videoParams->PutBuffer(OH_MD_KEY_CODEC_CONFIG, buffer_, sizeof(buffer_));
    int32_t videoTrackId = -1;
    int32_t ret = avmuxer_->AddTrack(videoTrackId, videoParams);
    ASSERT_EQ(ret, AV_ERR_OK);
    ASSERT_GE(videoTrackId, 0);
    ASSERT_EQ(avmuxer_->Start(), 0);
    EXPECT_EQ(avmuxer_->Stop(), 0);
}

/**
 * @tc.name: Muxer_Stop_002
 * @tc.desc: Muxer Stop() after Create()
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_Stop_002, TestSize.Level0)
{
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_Stop.mp4");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);
    EXPECT_NE(avmuxer_->Stop(), 0);
}

/**
 * @tc.name: Muxer_Stop_003
 * @tc.desc: Muxer Stop() after AddTrack()
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_Stop_003, TestSize.Level0)
{
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_Stop.mp4");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> videoParams =
        FormatMockFactory::CreateVideoFormat(OH_AVCODEC_MIMETYPE_VIDEO_AVC, TEST_WIDTH, TEST_HEIGHT);
    videoParams->PutBuffer(OH_MD_KEY_CODEC_CONFIG, buffer_, sizeof(buffer_));
    int32_t videoTrackId = -1;
    int32_t ret = avmuxer_->AddTrack(videoTrackId, videoParams);
    ASSERT_EQ(ret, AV_ERR_OK);
    ASSERT_GE(videoTrackId, 0);
    EXPECT_NE(avmuxer_->Stop(), 0);
}

/**
 * @tc.name: Muxer_Stop_004
 * @tc.desc: Muxer Stop() multiple times
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_Stop_004, TestSize.Level0)
{
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_Stop.mp4");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> videoParams =
        FormatMockFactory::CreateVideoFormat(OH_AVCODEC_MIMETYPE_VIDEO_AVC, TEST_WIDTH, TEST_HEIGHT);
    videoParams->PutBuffer(OH_MD_KEY_CODEC_CONFIG, buffer_, sizeof(buffer_));
    int32_t videoTrackId = -1;
    int32_t ret = avmuxer_->AddTrack(videoTrackId, videoParams);
    ASSERT_EQ(ret, AV_ERR_OK);
    ASSERT_GE(videoTrackId, 0);
    ASSERT_EQ(avmuxer_->Start(), 0);
    ASSERT_EQ(avmuxer_->Stop(), 0);

    ASSERT_NE(avmuxer_->Stop(), 0);
    ASSERT_NE(avmuxer_->Stop(), 0);
    ASSERT_NE(avmuxer_->Stop(), 0);
}

/**
 * @tc.name: Muxer_Stop_005
 * @tc.desc: Muxer Stop() before Start
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_Stop_005, TestSize.Level0)
{
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_Stop.mp4");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> videoParams =
        FormatMockFactory::CreateVideoFormat(OH_AVCODEC_MIMETYPE_VIDEO_AVC, TEST_WIDTH, TEST_HEIGHT);
    videoParams->PutBuffer(OH_MD_KEY_CODEC_CONFIG, buffer_, sizeof(buffer_));
    int32_t videoTrackId = -1;
    int32_t ret = avmuxer_->AddTrack(videoTrackId, videoParams);
    ASSERT_EQ(ret, AV_ERR_OK);
    EXPECT_GE(videoTrackId, 0);
    EXPECT_NE(avmuxer_->Stop(), 0);
    EXPECT_EQ(avmuxer_->Start(), 0);
    EXPECT_EQ(avmuxer_->Stop(), 0);
}

/**
 * @tc.name: Muxer_Stop_006
 * @tc.desc: Muxer Stop() while create by unexpected outputFormat
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_Stop_006, TestSize.Level0)
{
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_Stop.mp4");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, static_cast<OH_AVOutputFormat>(INVALID_FORMAT));
    ASSERT_FALSE(isCreated);

    std::shared_ptr<FormatMock> videoParams =
        FormatMockFactory::CreateVideoFormat(OH_AVCODEC_MIMETYPE_VIDEO_AVC, TEST_WIDTH, TEST_HEIGHT);
    videoParams->PutBuffer(OH_MD_KEY_CODEC_CONFIG, buffer_, sizeof(buffer_));
    int32_t videoTrackId = -1;
    int32_t ret = avmuxer_->AddTrack(videoTrackId, videoParams);
    ASSERT_NE(ret, AV_ERR_OK);
    ASSERT_LT(videoTrackId, 0);
    ASSERT_NE(avmuxer_->Start(), 0);
    ASSERT_NE(avmuxer_->Stop(), 0);
}

/**
 * @tc.name: Muxer_writeSample_001
 * @tc.desc: Muxer Write Sample normal
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_writeSample_001, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_WriteSample.mp4");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> vParam =
        FormatMockFactory::CreateVideoFormat(OH_AVCODEC_MIMETYPE_VIDEO_AVC, TEST_WIDTH, TEST_HEIGHT);

    int32_t ret = avmuxer_->AddTrack(trackId, vParam);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);
    ASSERT_EQ(avmuxer_->Start(), 0);

    OH_AVCodecBufferAttr info;
    info.pts = 0;
    info.offset = 0;
    info.size = sizeof(buffer_);
    ret = avmuxer_->WriteSample(trackId, buffer_, info);
    ASSERT_EQ(ret, 0);
}

/**
 * @tc.name: Muxer_writeSample_002
 * @tc.desc: Muxer Write Sample while create by unexpected outputFormat
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_writeSample_002, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_writeSample.mp4");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, static_cast<OH_AVOutputFormat>(INVALID_FORMAT));
    ASSERT_FALSE(isCreated);

    std::shared_ptr<FormatMock> vParam =
        FormatMockFactory::CreateVideoFormat(OH_AVCODEC_MIMETYPE_VIDEO_AVC, TEST_WIDTH, TEST_HEIGHT);

    int32_t ret = avmuxer_->AddTrack(trackId, vParam);
    ASSERT_NE(ret, 0);
    ASSERT_LT(trackId, 0);
    ASSERT_NE(avmuxer_->Start(), 0);

    OH_AVCodecBufferAttr info;
    info.pts = 0;
    info.size = sizeof(buffer_);
    ret = avmuxer_->WriteSample(trackId, buffer_, info);
    ASSERT_NE(ret, 0);
}

/**
 * @tc.name: Muxer_writeSample_003
 * @tc.desc: Muxer Write Sample without Start()
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_writeSample_003, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_WriteSample.mp4");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> vParam =
        FormatMockFactory::CreateVideoFormat(OH_AVCODEC_MIMETYPE_VIDEO_AVC, TEST_WIDTH, TEST_HEIGHT);

    int32_t ret = avmuxer_->AddTrack(trackId, vParam);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);

    OH_AVCodecBufferAttr info;
    info.pts = 0;
    info.size = sizeof(buffer_);
    ret = avmuxer_->WriteSample(trackId, buffer_, info);
    ASSERT_NE(ret, 0);
}

/**
 * @tc.name: Muxer_writeSample_004
 * @tc.desc: Muxer Write Sample unexisting track
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_writeSample_004, TestSize.Level0)
{
    constexpr int32_t invalidTrackId = 99999;
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_WriteSample.mp4");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> vParam =
        FormatMockFactory::CreateVideoFormat(OH_AVCODEC_MIMETYPE_VIDEO_AVC, TEST_WIDTH, TEST_HEIGHT);

    int32_t ret = avmuxer_->AddTrack(trackId, vParam);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);
    ASSERT_EQ(avmuxer_->Start(), 0);

    OH_AVCodecBufferAttr info;
    info.pts = 0;
    info.size = sizeof(buffer_);
    ret = avmuxer_->WriteSample(trackId + 1, buffer_, info);
    ASSERT_NE(ret, 0);

    ret = avmuxer_->WriteSample(-1, buffer_, info);
    ASSERT_NE(ret, 0);

    ret = avmuxer_->WriteSample(invalidTrackId, buffer_, info);
    ASSERT_NE(ret, 0);
}

/**
 * @tc.name: Muxer_writeSample_005
 * @tc.desc: Muxer Write Sample after Stop()
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_writeSample_005, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_WriteSample.mp4");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> vParam =
        FormatMockFactory::CreateVideoFormat(OH_AVCODEC_MIMETYPE_VIDEO_AVC, TEST_WIDTH, TEST_HEIGHT);

    int32_t ret = avmuxer_->AddTrack(trackId, vParam);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);
    ASSERT_EQ(avmuxer_->Start(), 0);
    ASSERT_EQ(avmuxer_->Stop(), 0);

    OH_AVCodecBufferAttr info;
    info.pts = 0;
    info.size = sizeof(buffer_);
    ret = avmuxer_->WriteSample(trackId, buffer_, info);
    ASSERT_NE(ret, 0);
}

/**
 * @tc.name: Muxer_SetRotation_001
 * @tc.desc: Muxer SetRotation after Create
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_SetRotation_001, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_SetRotation.mp4");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> vParam =
        FormatMockFactory::CreateVideoFormat(OH_AVCODEC_MIMETYPE_VIDEO_AVC, TEST_WIDTH, TEST_HEIGHT);
    int32_t ret = avmuxer_->AddTrack(trackId, vParam);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);

    ret = avmuxer_->SetRotation(TEST_ROTATION);
    EXPECT_EQ(ret, 0);
}


/**
 * @tc.name: Muxer_SetRotation_002
 * @tc.desc: Muxer SetRotation after Create
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_SetRotation_002, TestSize.Level0)
{
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_SetRotation.mp4");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    int32_t ret = avmuxer_->SetRotation(TEST_ROTATION);
    EXPECT_EQ(ret, 0);
}

/**
 * @tc.name: Muxer_SetRotation_003
 * @tc.desc: Muxer SetRotation after Start
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_SetRotation_003, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_SetRotation.mp4");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> vParam =
        FormatMockFactory::CreateVideoFormat(OH_AVCODEC_MIMETYPE_VIDEO_AVC, TEST_WIDTH, TEST_HEIGHT);
    int32_t ret = avmuxer_->AddTrack(trackId, vParam);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);
    ASSERT_EQ(avmuxer_->Start(), 0);

    ret = avmuxer_->SetRotation(TEST_ROTATION);
    EXPECT_NE(ret, 0);
}

/**
 * @tc.name: Muxer_SetRotation_004
 * @tc.desc: Muxer SetRotation after Stop
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_SetRotation_004, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_SetRotation.mp4");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> vParam =
        FormatMockFactory::CreateVideoFormat(OH_AVCODEC_MIMETYPE_VIDEO_AVC, TEST_WIDTH, TEST_HEIGHT);
    int32_t ret = avmuxer_->AddTrack(trackId, vParam);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);
    ASSERT_EQ(avmuxer_->Start(), 0);
    ASSERT_EQ(avmuxer_->Stop(), 0);

    ret = avmuxer_->SetRotation(TEST_ROTATION);
    EXPECT_NE(ret, 0);
}

/**
 * @tc.name: Muxer_SetRotation_005
 * @tc.desc: Muxer SetRotation after WriteSample
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_SetRotation_005, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_SetRotation.mp4");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> vParam =
        FormatMockFactory::CreateVideoFormat(OH_AVCODEC_MIMETYPE_VIDEO_AVC, TEST_WIDTH, TEST_HEIGHT);

    int32_t ret = avmuxer_->AddTrack(trackId, vParam);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);
    ASSERT_EQ(avmuxer_->Start(), 0);

    OH_AVCodecBufferAttr info;
    info.pts = 0;
    info.size = sizeof(buffer_);
    ret = avmuxer_->WriteSample(trackId, buffer_, info);
    EXPECT_EQ(ret, 0);

    ret = avmuxer_->SetRotation(TEST_ROTATION);
    EXPECT_NE(ret, 0);
}

/**
 * @tc.name: Muxer_SetRotation_006
 * @tc.desc: Muxer SetRotation while Create failed!
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_SetRotation_006, TestSize.Level0)
{
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_SetRotation.mp4");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, static_cast<OH_AVOutputFormat>(INVALID_FORMAT));
    ASSERT_FALSE(isCreated);

    int32_t ret = avmuxer_->SetRotation(TEST_ROTATION);
    ASSERT_NE(ret, 0);
}

/**
 * @tc.name: Muxer_SetRotation_007
 * @tc.desc: Muxer SetRotation expected value
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_SetRotation_007, TestSize.Level0)
{
    constexpr int32_t testRotation180 = 180;
    constexpr int32_t testRotation270 = 270;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_SetRotation.mp4");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    int32_t ret = avmuxer_->SetRotation(0);
    EXPECT_EQ(ret, 0);

    ret = avmuxer_->SetRotation(TEST_ROTATION);
    EXPECT_EQ(ret, 0);

    ret = avmuxer_->SetRotation(testRotation180);
    EXPECT_EQ(ret, 0);

    ret = avmuxer_->SetRotation(testRotation270);
    EXPECT_EQ(ret, 0);
}

/**
 * @tc.name: Muxer_SetRotation_008
 * @tc.desc: Muxer SetRotation unexpected value
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_SetRotation_008, TestSize.Level0)
{
    constexpr int32_t testRotation360 = 360;
    constexpr int32_t testRotationNeg90 = -90;
    constexpr int32_t testRotationNeg180 = -180;
    constexpr int32_t testRotationNeg270 = -270;
    constexpr int32_t testRotationNeg360 = -360;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_SetRotation.mp4");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    int32_t ret = avmuxer_->SetRotation(1);
    EXPECT_NE(ret, 0);

    ret = avmuxer_->SetRotation(TEST_ROTATION + 1);
    EXPECT_NE(ret, 0);

    ret = avmuxer_->SetRotation(testRotation360);
    EXPECT_NE(ret, 0);

    ret = avmuxer_->SetRotation(-1);
    EXPECT_NE(ret, 0);

    ret = avmuxer_->SetRotation(testRotationNeg90);
    EXPECT_NE(ret, 0);

    ret = avmuxer_->SetRotation(testRotationNeg180);
    EXPECT_NE(ret, 0);

    ret = avmuxer_->SetRotation(testRotationNeg270);
    EXPECT_NE(ret, 0);

    ret = avmuxer_->SetRotation(testRotationNeg360);
    EXPECT_NE(ret, 0);
}

/**
 * @tc.name: Muxer_Hevc_AddTrack_001
 * @tc.desc: Muxer Hevc AddTrack while create by unexpected value
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_Hevc_AddTrack_001, TestSize.Level0)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        std::cout << "the hevc of mimetype is not supported" << std::endl;
        return;
    }

    constexpr int32_t validVideoDelay = 1;
    constexpr int32_t invalidVideoDelay = -1;
    constexpr int32_t validFrameRate = 30;
    constexpr int32_t invalidFrameRate = -1;

    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_H265.mp4");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> videoParams =
        FormatMockFactory::CreateVideoFormat(OH_AVCODEC_MIMETYPE_VIDEO_HEVC, TEST_WIDTH, TEST_HEIGHT);

    videoParams->PutIntValue("video_delay", validVideoDelay);
    int32_t ret = avmuxer_->AddTrack(trackId, videoParams);
    ASSERT_NE(ret, 0);

    videoParams->PutIntValue("video_delay", invalidVideoDelay);
    videoParams->PutDoubleValue(OH_MD_KEY_FRAME_RATE, validFrameRate);
    ret = avmuxer_->AddTrack(trackId, videoParams);
    ASSERT_NE(ret, 0);

    videoParams->PutIntValue("video_delay", validVideoDelay);
    videoParams->PutDoubleValue(OH_MD_KEY_FRAME_RATE, invalidFrameRate);
    ret = avmuxer_->AddTrack(trackId, videoParams);
    ASSERT_NE(ret, 0);

    videoParams->PutIntValue("video_delay", 0xFF);
    videoParams->PutDoubleValue(OH_MD_KEY_FRAME_RATE, validFrameRate);
    ret = avmuxer_->AddTrack(trackId, videoParams);
    ASSERT_NE(ret, 0);

    videoParams->PutIntValue("video_delay", validVideoDelay);
    videoParams->PutDoubleValue(OH_MD_KEY_FRAME_RATE, validFrameRate);
    ret = avmuxer_->AddTrack(trackId, videoParams);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);
}

/**
 * @tc.name: Muxer_Hevc_WriteSample_001
 * @tc.desc: Muxer Hevc Write Sample normal
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_Hevc_WriteSample_001, TestSize.Level0)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }

    constexpr int32_t invalidTrackId = 99999;
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_H265.mp4");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> videoParams =
        FormatMockFactory::CreateVideoFormat(OH_AVCODEC_MIMETYPE_VIDEO_HEVC, TEST_WIDTH, TEST_HEIGHT);

    int32_t ret = avmuxer_->AddTrack(trackId, videoParams);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);
    ASSERT_EQ(avmuxer_->Start(), 0);

    OH_AVCodecBufferAttr info;
    info.pts = 0;
    info.size = sizeof(buffer_);
    OH_AVBuffer *buffer = OH_AVBuffer_Create(info.size);
    ASSERT_EQ(memcpy_s(OH_AVBuffer_GetAddr(buffer), info.size, buffer_, sizeof(buffer_)), 0);
    OH_AVBuffer_SetBufferAttr(buffer, &info);

    ret = avmuxer_->WriteSampleBuffer(trackId + 1, buffer);
    ASSERT_NE(ret, 0);

    ret = avmuxer_->WriteSampleBuffer(-1, buffer);
    ASSERT_NE(ret, 0);

    ret = avmuxer_->WriteSampleBuffer(invalidTrackId, buffer);
    ASSERT_NE(ret, 0);

    ret = avmuxer_->WriteSampleBuffer(trackId, buffer);
    ASSERT_EQ(ret, 0);

    OH_AVBuffer_Destroy(buffer);
}

/**
 * @tc.name: Muxer_Hevc_WriteSample_002
 * @tc.desc: Muxer Hevc Write Sample flags AVCODEC_BUFFER_FLAGS_CODEC_DATA
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_Hevc_WriteSample_002, TestSize.Level0)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }

    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_H265.mp4");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> videoParams =
        FormatMockFactory::CreateVideoFormat(OH_AVCODEC_MIMETYPE_VIDEO_HEVC, TEST_WIDTH, TEST_HEIGHT);

    int32_t ret = avmuxer_->AddTrack(trackId, videoParams);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);
    ASSERT_EQ(avmuxer_->Start(), 0);

    OH_AVCodecBufferAttr info;
    info.pts = 0;
    info.size = sizeof(annexBuffer_);
    OH_AVBuffer *buffer = OH_AVBuffer_Create(info.size);
    ASSERT_EQ(memcpy_s(OH_AVBuffer_GetAddr(buffer), info.size, annexBuffer_, sizeof(annexBuffer_)), 0);

    info.flags = AVCODEC_BUFFER_FLAGS_CODEC_DATA;
    OH_AVBuffer_SetBufferAttr(buffer, &info);
    ret = avmuxer_->WriteSampleBuffer(trackId, buffer);
    ASSERT_EQ(ret, 0);

    info.flags = AVCODEC_BUFFER_FLAGS_SYNC_FRAME;
    OH_AVBuffer_SetBufferAttr(buffer, &info);
    ret = avmuxer_->WriteSampleBuffer(trackId, buffer);
    ASSERT_EQ(ret, 0);

    OH_AVBuffer_Destroy(buffer);
}

/**
 * @tc.name: Muxer_Hevc_WriteSample_003
 * @tc.desc: Muxer Hevc Write Sample flags AVCODEC_BUFFER_FLAGS_CODEC_DATA | AVCODEC_BUFFER_FLAGS_SYNC_FRAME
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_Hevc_WriteSample_003, TestSize.Level0)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }

    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_H265.mp4");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> videoParams =
        FormatMockFactory::CreateVideoFormat(OH_AVCODEC_MIMETYPE_VIDEO_HEVC, TEST_WIDTH, TEST_HEIGHT);

    int32_t ret = avmuxer_->AddTrack(trackId, videoParams);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);
    ASSERT_EQ(avmuxer_->Start(), 0);

    OH_AVCodecBufferAttr info;
    info.pts = 0;
    info.size = sizeof(annexBuffer_);
    OH_AVBuffer *buffer = OH_AVBuffer_Create(info.size);
    ASSERT_EQ(memcpy_s(OH_AVBuffer_GetAddr(buffer), info.size, annexBuffer_, sizeof(annexBuffer_)), 0);

    info.flags = AVCODEC_BUFFER_FLAGS_CODEC_DATA | AVCODEC_BUFFER_FLAGS_SYNC_FRAME;
    OH_AVBuffer_SetBufferAttr(buffer, &info);
    ret = avmuxer_->WriteSampleBuffer(trackId, buffer);
    ASSERT_EQ(ret, 0);

    info.flags = AVCODEC_BUFFER_FLAGS_NONE;
    OH_AVBuffer_SetBufferAttr(buffer, &info);
    ret = avmuxer_->WriteSampleBuffer(trackId, buffer);
    ASSERT_EQ(ret, 0);

    OH_AVBuffer_Destroy(buffer);
}

/**
 * @tc.name: Muxer_Hevc_WriteSample_004
 * @tc.desc: Muxer Hevc Write Sample flags AVCODEC_BUFFER_FLAGS_SYNC_FRAME
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_Hevc_WriteSample_004, TestSize.Level0)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }

    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_H265.mp4");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> videoParams =
        FormatMockFactory::CreateVideoFormat(OH_AVCODEC_MIMETYPE_VIDEO_HEVC, TEST_WIDTH, TEST_HEIGHT);

    int32_t ret = avmuxer_->AddTrack(trackId, videoParams);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);
    ASSERT_EQ(avmuxer_->Start(), 0);

    OH_AVCodecBufferAttr info;
    info.pts = 0;
    info.size = sizeof(annexBuffer_);
    OH_AVBuffer *buffer = OH_AVBuffer_Create(info.size);
    ASSERT_EQ(memcpy_s(OH_AVBuffer_GetAddr(buffer), info.size, annexBuffer_, sizeof(annexBuffer_)), 0);

    info.flags = AVCODEC_BUFFER_FLAGS_SYNC_FRAME;
    OH_AVBuffer_SetBufferAttr(buffer, &info);
    ret = avmuxer_->WriteSampleBuffer(trackId, buffer);
    ASSERT_EQ(ret, 0);

    info.flags = AVCODEC_BUFFER_FLAGS_NONE;
    OH_AVBuffer_SetBufferAttr(buffer, &info);
    ret = avmuxer_->WriteSampleBuffer(trackId, buffer);
    ASSERT_EQ(ret, 0);

    OH_AVBuffer_Destroy(buffer);
}

/**
 * @tc.name: Muxer_SetFlag_001
 * @tc.desc: Muxer Write Sample flags AVCODEC_BUFFER_FLAGS_DISPOSABLE
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_SetFlag_001, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_Disposable_flag.mp4");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> videoParams =
        FormatMockFactory::CreateVideoFormat(OH_AVCODEC_MIMETYPE_VIDEO_AVC, TEST_WIDTH, TEST_HEIGHT);

    int32_t ret = avmuxer_->AddTrack(trackId, videoParams);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);
    ASSERT_EQ(avmuxer_->Start(), 0);

    inputFile_ = std::make_shared<std::ifstream>(INPUT_FILE_PATH, std::ios::binary);

    int32_t extSize = 0;
    inputFile_->read(reinterpret_cast<char*>(&extSize), sizeof(extSize));
    if (extSize > 0) {
        std::vector<uint8_t> buffer(extSize);
        inputFile_->read(reinterpret_cast<char*>(buffer.data()), extSize);
    }

    bool eosFlag = false;
    uint32_t flag = AVCODEC_BUFFER_FLAGS_DISPOSABLE;
    ret = WriteSample(trackId, inputFile_, eosFlag, flag);
    while (!eosFlag && (ret == 0)) {
        ret = WriteSample(trackId, inputFile_, eosFlag, flag);
    }
    ASSERT_EQ(ret, 0);
}

/**
 * @tc.name: Muxer_SetFlag_001
 * @tc.desc: Muxer Write Sample flags AVCODEC_BUFFER_FLAGS_DISPOSABLE_EXT
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_SetFlag_002, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_DisposableExt_flag.mp4");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> videoParams =
        FormatMockFactory::CreateVideoFormat(OH_AVCODEC_MIMETYPE_VIDEO_AVC, TEST_WIDTH, TEST_HEIGHT);

    int32_t ret = avmuxer_->AddTrack(trackId, videoParams);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);
    ASSERT_EQ(avmuxer_->Start(), 0);

    inputFile_ = std::make_shared<std::ifstream>(INPUT_FILE_PATH, std::ios::binary);

    int32_t extSize = 0;
    inputFile_->read(reinterpret_cast<char*>(&extSize), sizeof(extSize));
    if (extSize > 0) {
        std::vector<uint8_t> buffer(extSize);
        inputFile_->read(reinterpret_cast<char*>(buffer.data()), extSize);
    }

    bool eosFlag = false;
    uint32_t flag = AVCODEC_BUFFER_FLAGS_DISPOSABLE_EXT_TEST;
    ret = WriteSample(trackId, inputFile_, eosFlag, flag);
    while (!eosFlag && (ret == 0)) {
        ret = WriteSample(trackId, inputFile_, eosFlag, flag);
    }
    ASSERT_EQ(ret, 0);
}

/**
 * @tc.name: Muxer_SetFlag_001
 * @tc.desc: Muxer Write Sample flags AVCODEC_BUFFER_FLAGS_DISPOSABLE & AVCODEC_BUFFER_FLAGS_DISPOSABLE_EXT
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_SetFlag_003, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_Disposable_DisposableExt_flag.mp4");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> videoParams =
        FormatMockFactory::CreateVideoFormat(OH_AVCODEC_MIMETYPE_VIDEO_AVC, TEST_WIDTH, TEST_HEIGHT);

    int32_t ret = avmuxer_->AddTrack(trackId, videoParams);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);
    ASSERT_EQ(avmuxer_->Start(), 0);

    inputFile_ = std::make_shared<std::ifstream>(INPUT_FILE_PATH, std::ios::binary);

    int32_t extSize = 0;
    inputFile_->read(reinterpret_cast<char*>(&extSize), sizeof(extSize));
    if (extSize > 0) {
        std::vector<uint8_t> buffer(extSize);
        inputFile_->read(reinterpret_cast<char*>(buffer.data()), extSize);
    }

    bool eosFlag = false;
    uint32_t flag = AVCODEC_BUFFER_FLAGS_DISPOSABLE;
    flag |= AVCODEC_BUFFER_FLAGS_DISPOSABLE_EXT_TEST;
    ret = WriteSample(trackId, inputFile_, eosFlag, flag);
    while (!eosFlag && (ret == 0)) {
        ret = WriteSample(trackId, inputFile_, eosFlag, flag);
    }
    ASSERT_EQ(ret, 0);
}

#ifdef AVMUXER_UNITTEST_CAPI
/**
 * @tc.name: Muxer_Destroy_001
 * @tc.desc: Muxer Destroy normal
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_Destroy_001, TestSize.Level0)
{
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_Destroy.mp4");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> videoParams =
        FormatMockFactory::CreateVideoFormat(OH_AVCODEC_MIMETYPE_VIDEO_AVC, TEST_WIDTH, TEST_HEIGHT);
    videoParams->PutBuffer(OH_MD_KEY_CODEC_CONFIG, buffer_, sizeof(buffer_));
    int32_t videoTrackId = -1;
    int32_t ret = avmuxer_->AddTrack(videoTrackId, videoParams);
    ASSERT_EQ(ret, AV_ERR_OK);
    ASSERT_GE(videoTrackId, 0);
    ASSERT_EQ(avmuxer_->Start(), 0);
    ASSERT_EQ(avmuxer_->Stop(), 0);
    ASSERT_EQ(avmuxer_->Destroy(), 0);
}

/**
 * @tc.name: Muxer_Destroy_002
 * @tc.desc: Muxer Destroy normal
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_Destroy_002, TestSize.Level0)
{
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_Destroy.mp4");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;
    OH_AVMuxer *muxer = OH_AVMuxer_Create(fd_, outputFormat);
    int32_t ret = OH_AVMuxer_Destroy(muxer);
    ASSERT_EQ(ret, 0);
    muxer = nullptr;
}

/**
 * @tc.name: Muxer_Destroy_003
 * @tc.desc: Muxer Destroy nullptr
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_Destroy_003, TestSize.Level0)
{
    int32_t ret = OH_AVMuxer_Destroy(nullptr);
    ASSERT_EQ(ret, AV_ERR_INVALID_VAL);
}

/**
 * @tc.name: Muxer_Destroy_004
 * @tc.desc: Muxer Destroy other class
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_Destroy_004, TestSize.Level0)
{
    OH_AVFormat *format = OH_AVFormat_Create();
    int32_t ret = OH_AVMuxer_Destroy(reinterpret_cast<OH_AVMuxer*>(format));
    ASSERT_EQ(ret, AV_ERR_INVALID_VAL);
    OH_AVFormat_Destroy(format);
}
#endif // AVMUXER_UNITTEST_CAPI

#ifdef AVMUXER_UNITTEST_INNER_API
/**
 * @tc.name: Muxer_SetParameter_001
 * @tc.desc: Muxer SetParameter after Create
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_SetParameter_001, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_SetParameter.mp4");
    Plugins::OutputFormat outputFormat = Plugins::OutputFormat::MPEG_4;

    int32_t fd = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    std::shared_ptr<AVMuxer> avmuxer = AVMuxerFactory::CreateAVMuxer(fd, outputFormat);
    ASSERT_NE(avmuxer, nullptr);

    std::shared_ptr<Meta> videoParams = std::make_shared<Meta>();
    videoParams->Set<Tag::MIME_TYPE>(Plugins::MimeType::VIDEO_AVC);
    videoParams->Set<Tag::VIDEO_WIDTH>(TEST_WIDTH);
    videoParams->Set<Tag::VIDEO_HEIGHT>(TEST_HEIGHT);

    int32_t ret = avmuxer->AddTrack(trackId, videoParams);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);

    std::shared_ptr<Meta> param = std::make_shared<Meta>();
    param->Set<Tag::VIDEO_ROTATION>(Plugins::VideoRotation::VIDEO_ROTATION_0);
    param->Set<Tag::MEDIA_CREATION_TIME>("2023-12-19T03:16:00.000Z");
    param->Set<Tag::MEDIA_LATITUDE>(22.67f); // 22.67f test latitude
    param->Set<Tag::MEDIA_LONGITUDE>(114.06f); // 114.06f test longitude
    param->Set<Tag::MEDIA_TITLE>("ohos muxer");
    param->Set<Tag::MEDIA_ARTIST>("ohos muxer");
    param->Set<Tag::MEDIA_COMPOSER>("ohos muxer");
    param->Set<Tag::MEDIA_DATE>("2023-12-19");
    param->Set<Tag::MEDIA_ALBUM>("ohos muxer");
    param->Set<Tag::MEDIA_ALBUM_ARTIST>("ohos muxer");
    param->Set<Tag::MEDIA_COPYRIGHT>("ohos muxer");
    param->Set<Tag::MEDIA_GENRE>("{marketing-name:\"HW P60\"}");
    ret = avmuxer->SetParameter(param);
    EXPECT_EQ(ret, 0);

    close(fd);
}

/**
 * @tc.name: Muxer_SetParameter_002
 * @tc.desc: Muxer SetParameter after Create
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_SetParameter_002, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_SetParameter.mp4");
    Plugins::OutputFormat outputFormat = Plugins::OutputFormat::MPEG_4;

    int32_t fd = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    std::shared_ptr<AVMuxer> avmuxer = AVMuxerFactory::CreateAVMuxer(fd, outputFormat);
    ASSERT_NE(avmuxer, nullptr);

    std::shared_ptr<Meta> videoParams = std::make_shared<Meta>();
    videoParams->Set<Tag::MIME_TYPE>(Plugins::MimeType::VIDEO_AVC);
    videoParams->Set<Tag::VIDEO_WIDTH>(TEST_WIDTH);
    videoParams->Set<Tag::VIDEO_HEIGHT>(TEST_HEIGHT);

    int32_t ret = avmuxer->AddTrack(trackId, videoParams);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);

    std::shared_ptr<Meta> param = std::make_shared<Meta>();
    param->Set<Tag::MEDIA_LONGITUDE>(114.06f); // 114.06f test longitude
    ret = avmuxer->SetParameter(param);
    EXPECT_NE(ret, 0);

    param->Set<Tag::MEDIA_LATITUDE>(-90.0f); // -90.0f test latitude
    ret = avmuxer->SetParameter(param);
    EXPECT_EQ(ret, 0);

    param->Set<Tag::MEDIA_LATITUDE>(90.0f); // 90.0f test latitude
    ret = avmuxer->SetParameter(param);
    EXPECT_EQ(ret, 0);

    param->Set<Tag::MEDIA_LATITUDE>(-90.1f); // -90.1f test latitude
    ret = avmuxer->SetParameter(param);
    EXPECT_NE(ret, 0);

    param->Set<Tag::MEDIA_LATITUDE>(90.1f); // 90.1f test latitude
    ret = avmuxer->SetParameter(param);
    EXPECT_NE(ret, 0);

    close(fd);
}

/**
 * @tc.name: Muxer_SetParameter_003
 * @tc.desc: Muxer SetParameter after Create
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_SetParameter_003, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_SetParameter.mp4");
    Plugins::OutputFormat outputFormat = Plugins::OutputFormat::MPEG_4;

    int32_t fd = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    std::shared_ptr<AVMuxer> avmuxer = AVMuxerFactory::CreateAVMuxer(fd, outputFormat);
    ASSERT_NE(avmuxer, nullptr);

    std::shared_ptr<Meta> videoParams = std::make_shared<Meta>();
    videoParams->Set<Tag::MIME_TYPE>(Plugins::MimeType::VIDEO_AVC);
    videoParams->Set<Tag::VIDEO_WIDTH>(TEST_WIDTH);
    videoParams->Set<Tag::VIDEO_HEIGHT>(TEST_HEIGHT);

    int32_t ret = avmuxer->AddTrack(trackId, videoParams);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);

    std::shared_ptr<Meta> param = std::make_shared<Meta>();
    param->Set<Tag::MEDIA_LATITUDE>(22.67f); // 22.67f test latitude
    ret = avmuxer->SetParameter(param);
    EXPECT_NE(ret, 0);

    param->Set<Tag::MEDIA_LONGITUDE>(-180.0f); // -180.0f test longitude
    ret = avmuxer->SetParameter(param);
    EXPECT_EQ(ret, 0);

    param->Set<Tag::MEDIA_LONGITUDE>(180.0f); // 180.0f test longitude
    ret = avmuxer->SetParameter(param);
    EXPECT_EQ(ret, 0);

    param->Set<Tag::MEDIA_LONGITUDE>(-180.1f); // -180.1f test longitude
    ret = avmuxer->SetParameter(param);
    EXPECT_NE(ret, 0);

    param->Set<Tag::MEDIA_LONGITUDE>(180.1f); // 180.1f test longitude
    ret = avmuxer->SetParameter(param);
    EXPECT_NE(ret, 0);

    close(fd);
}

/**
 * @tc.name: Muxer_SetUserMeta_001
 * @tc.desc: Muxer SetUserMeta after Create
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_SetUserMeta_001, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_SetUserMeta.mp4");
    Plugins::OutputFormat outputFormat = Plugins::OutputFormat::MPEG_4;

    int32_t fd = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    std::shared_ptr<AVMuxer> avmuxer = AVMuxerFactory::CreateAVMuxer(fd, outputFormat);
    ASSERT_NE(avmuxer, nullptr);

    std::shared_ptr<Meta> videoParams = std::make_shared<Meta>();
    videoParams->Set<Tag::MIME_TYPE>(Plugins::MimeType::VIDEO_AVC);
    videoParams->Set<Tag::VIDEO_WIDTH>(TEST_WIDTH);
    videoParams->Set<Tag::VIDEO_HEIGHT>(TEST_HEIGHT);

    int32_t ret = avmuxer->AddTrack(trackId, videoParams);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);

    std::shared_ptr<Meta> param = std::make_shared<Meta>();
    param->Set<Tag::VIDEO_ROTATION>(Plugins::VideoRotation::VIDEO_ROTATION_0);
    param->Set<Tag::MEDIA_CREATION_TIME>("2023-12-19T03:16:00.000Z");
    param->Set<Tag::MEDIA_LATITUDE>(22.67f); // 22.67f test latitude
    param->Set<Tag::MEDIA_LONGITUDE>(114.06f); // 114.06f test longitude
    param->Set<Tag::MEDIA_TITLE>("ohos muxer");
    param->Set<Tag::MEDIA_ARTIST>("ohos muxer");
    param->Set<Tag::MEDIA_COMPOSER>("ohos muxer");
    param->Set<Tag::MEDIA_DATE>("2023-12-19");
    param->Set<Tag::MEDIA_ALBUM>("ohos muxer");
    param->Set<Tag::MEDIA_ALBUM_ARTIST>("ohos muxer");
    param->Set<Tag::MEDIA_COPYRIGHT>("ohos muxer");
    param->Set<Tag::MEDIA_GENRE>("{marketing-name:\"HW P60\"}");
    ret = avmuxer->SetParameter(param);
    EXPECT_EQ(ret, 0);

    std::shared_ptr<Meta> userMeta = std::make_shared<Meta>();
    userMeta->SetData("com.os.version", 5); // 5 test version
    userMeta->SetData("com.os.model", "LNA-AL00");
    userMeta->SetData("com.os.manufacturer", "HW");
    userMeta->SetData("com.os.marketing_name", "HW P60");
    userMeta->SetData("com.os.capture.fps", 30.00f); // 30.00f test capture fps
    ret = avmuxer->SetUserMeta(userMeta);
    EXPECT_EQ(ret, 0);

    close(fd);
}
#endif
} // namespace