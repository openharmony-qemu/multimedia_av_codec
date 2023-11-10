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

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <fcntl.h>
#include "avmuxer.h"
#include "avcodec_info.h"
#include "avmuxer_unit_test.h"
#ifdef AVMUXER_UNITTEST_CAPI
#include "native_avmuxer.h"
#include "native_avformat.h"
#endif

using namespace testing::ext;
using namespace OHOS::MediaAVCodec;
namespace {
constexpr int32_t TEST_CHANNEL_COUNT = 2;
constexpr int32_t TEST_SAMPLE_RATE = 2;
constexpr int32_t TEST_WIDTH = 720;
constexpr int32_t TEST_HEIGHT = 480;
constexpr int32_t TEST_ROTATION = 90;
constexpr int32_t INVALID_FORMAT = -99;
const std::string TEST_FILE_PATH = "/data/test/media/";
const std::string HEVC_LIB_PATH = std::string(AV_CODEC_PLUGIN_PATH) + "/libav_codec_plugin_HevcParser.z.so";
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

    if (avmuxer_ != nullptr) {
        avmuxer_->Destroy();
        avmuxer_ = nullptr;
    }
}

/**
 * @tc.name: Muxer_Create_001
 * @tc.desc: Muxer Create
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_Create_001, TestSize.Level0)
{
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_Create.mp4");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    OutputFormat outputFormat = OUTPUT_FORMAT_MPEG_4;
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
    OutputFormat outputFormat = OUTPUT_FORMAT_MPEG_4;
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
    OutputFormat outputFormat = OUTPUT_FORMAT_MPEG_4;
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
    OutputFormat outputFormat = OUTPUT_FORMAT_MPEG_4;
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

    OutputFormat outputFormat = OUTPUT_FORMAT_MPEG_4;
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    avmuxer_->Destroy();
    outputFormat = OUTPUT_FORMAT_M4A;
    isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    avmuxer_->Destroy();
    outputFormat = OUTPUT_FORMAT_DEFAULT;
    isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    avmuxer_->Destroy();
    isCreated = avmuxer_->CreateMuxer(fd_, static_cast<OutputFormat>(INVALID_FORMAT));
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
    OutputFormat outputFormat = OUTPUT_FORMAT_MPEG_4;
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::AUDIO_MPEG);
    audioParams->PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, TEST_SAMPLE_RATE);
    audioParams->PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, TEST_CHANNEL_COUNT);
    ret = avmuxer_->AddTrack(audioTrackId, audioParams);
    EXPECT_EQ(ret, AV_ERR_OK);
    EXPECT_GE(audioTrackId, 0);

    audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, TEST_SAMPLE_RATE);
    audioParams->PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, TEST_CHANNEL_COUNT);
    ret = avmuxer_->AddTrack(audioTrackId, audioParams);
    EXPECT_NE(ret, AV_ERR_OK);

    audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::AUDIO_MPEG);
    audioParams->PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, TEST_CHANNEL_COUNT);
    ret = avmuxer_->AddTrack(audioTrackId, audioParams);
    EXPECT_NE(ret, AV_ERR_OK);

    audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::AUDIO_MPEG);
    audioParams->PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, TEST_SAMPLE_RATE);
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
    OutputFormat outputFormat = OUTPUT_FORMAT_MPEG_4;
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> videoParams = FormatMockFactory::CreateFormat();
    videoParams->PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_MPEG4);
    videoParams->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, TEST_WIDTH);
    videoParams->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, TEST_HEIGHT);
    ret = avmuxer_->AddTrack(videoTrackId, videoParams);
    EXPECT_EQ(ret, AV_ERR_OK);
    EXPECT_GE(videoTrackId, 0);

    videoParams = FormatMockFactory::CreateFormat();
    videoParams->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, TEST_WIDTH);
    videoParams->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, TEST_HEIGHT);
    ret = avmuxer_->AddTrack(videoTrackId, videoParams);
    EXPECT_NE(ret, AV_ERR_OK);

    videoParams = FormatMockFactory::CreateFormat();
    videoParams->PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_MPEG4);
    videoParams->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, TEST_HEIGHT);
    ret = avmuxer_->AddTrack(videoTrackId, videoParams);
    EXPECT_NE(ret, AV_ERR_OK);

    videoParams = FormatMockFactory::CreateFormat();
    videoParams->PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_MPEG4);
    videoParams->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, TEST_WIDTH);
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
    OutputFormat outputFormat = OUTPUT_FORMAT_MPEG_4;
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> videoParams =
        FormatMockFactory::CreateVideoFormat(CodecMimeType::VIDEO_MPEG4, TEST_WIDTH, TEST_HEIGHT);
    videoParams->PutBuffer(MediaDescriptionKey::MD_KEY_CODEC_CONFIG, buffer_, sizeof(buffer_));
    ret = avmuxer_->AddTrack(videoTrackId, videoParams);
    EXPECT_EQ(ret, AV_ERR_OK);
    EXPECT_GE(videoTrackId, 0);

    ASSERT_EQ(avmuxer_->Start(), 0);
    std::shared_ptr<FormatMock> audioParams =
        FormatMockFactory::CreateAudioFormat(CodecMimeType::AUDIO_MPEG, TEST_SAMPLE_RATE, TEST_CHANNEL_COUNT);
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
        CodecMimeType::AUDIO_MPEG,
        // CodecMimeType::AUDIO_FLAC,
        // CodecMimeType::AUDIO_RAW,
        CodecMimeType::AUDIO_AAC,
        // CodecMimeType::AUDIO_VORBIS,
        // CodecMimeType::AUDIO_OPUS,
        // CodecMimeType::AUDIO_AMR_NB,
        // CodecMimeType::AUDIO_AMR_WB,
        CodecMimeType::VIDEO_AVC,
        CodecMimeType::VIDEO_MPEG4,
        CodecMimeType::IMAGE_JPG,
        CodecMimeType::IMAGE_PNG,
        CodecMimeType::IMAGE_BMP,
    };

    const std::vector<std::string_view> testM4aMimeTypeList =
    {
        CodecMimeType::AUDIO_AAC,
        CodecMimeType::VIDEO_AVC,
        CodecMimeType::VIDEO_MPEG4,
        CodecMimeType::IMAGE_JPG,
        CodecMimeType::IMAGE_PNG,
        CodecMimeType::IMAGE_BMP,
    };

    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_AddTrack.mp4");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OutputFormat outputFormat = OUTPUT_FORMAT_MPEG_4;

    std::shared_ptr<FormatMock> avParam = FormatMockFactory::CreateFormat();
    avParam->PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, TEST_SAMPLE_RATE);
    avParam->PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, TEST_CHANNEL_COUNT);
    avParam->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, TEST_WIDTH);
    avParam->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, TEST_HEIGHT);

    for (uint32_t i = 0; i < testMp4MimeTypeList.size(); ++i) {
        bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
        ASSERT_TRUE(isCreated);
        avParam->PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, testMp4MimeTypeList[i]);
        ret = avmuxer_->AddTrack(trackId, avParam);
        EXPECT_EQ(ret, AV_ERR_OK) << "AddTrack failed: i:" << i << " mimeType:" << testMp4MimeTypeList[i];
        EXPECT_EQ(trackId, 0) << "i:" << i << " TrackId:" << trackId << " mimeType:" << testMp4MimeTypeList[i];
    }

    // need to change libohosffmpeg.z.so, muxer build config add ipod
    avmuxer_->Destroy();
    outputFormat = OUTPUT_FORMAT_M4A;
    for (uint32_t i = 0; i < testM4aMimeTypeList.size(); ++i) {
        bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
        ASSERT_TRUE(isCreated);
        avParam->PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, testM4aMimeTypeList[i]);
        ret = avmuxer_->AddTrack(trackId, avParam);
        EXPECT_EQ(ret, AV_ERR_OK) << "AddTrack failed: i:" << i << " mimeType:" << testM4aMimeTypeList[i];
        EXPECT_EQ(trackId, 0) << "i:" << i << " TrackId:" << trackId << " mimeType:" << testM4aMimeTypeList[i];
    }
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
    bool isCreated = avmuxer_->CreateMuxer(fd_, static_cast<OutputFormat>(INVALID_FORMAT));
    ASSERT_FALSE(isCreated);

    std::shared_ptr<FormatMock> avParam = FormatMockFactory::CreateFormat();
    avParam->PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::AUDIO_MPEG);
    avParam->PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, TEST_SAMPLE_RATE);
    avParam->PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, TEST_CHANNEL_COUNT);
    avParam->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, TEST_WIDTH);
    avParam->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, TEST_HEIGHT);

    int32_t ret = avmuxer_->AddTrack(trackId, avParam);
    ASSERT_NE(ret, 0);

    avParam->PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_MPEG4);
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
    OutputFormat outputFormat = OUTPUT_FORMAT_MPEG_4;
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParam = FormatMockFactory::CreateFormat();
    audioParam->PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::AUDIO_MPEG);
    audioParam->PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, TEST_SAMPLE_RATE);
    audioParam->PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, -1);
    int32_t ret = avmuxer_->AddTrack(trackId, audioParam);
    ASSERT_NE(ret, 0);

    audioParam->PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, -1);
    audioParam->PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, TEST_CHANNEL_COUNT);
    ret = avmuxer_->AddTrack(trackId, audioParam);
    ASSERT_NE(ret, 0);

    // test add video track
    std::shared_ptr<FormatMock> videoParam = FormatMockFactory::CreateFormat();
    videoParam->PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_MPEG4);
    videoParam->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, TEST_WIDTH);
    videoParam->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, -1);
    ret = avmuxer_->AddTrack(trackId, videoParam);
    ASSERT_NE(ret, 0);

    videoParam->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, -1);
    videoParam->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, TEST_HEIGHT);
    ret = avmuxer_->AddTrack(trackId, videoParam);
    ASSERT_NE(ret, 0);

    videoParam->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, TEST_WIDTH);
    videoParam->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 0xFFFF + 1);
    ret = avmuxer_->AddTrack(trackId, videoParam);
    ASSERT_NE(ret, 0);

    videoParam->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 0xFFFF + 1);
    videoParam->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, TEST_HEIGHT);
    ret = avmuxer_->AddTrack(trackId, videoParam);
    ASSERT_NE(ret, 0);

    videoParam->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 0xFFFF);
    videoParam->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 0xFFFF);
    ret = avmuxer_->AddTrack(trackId, videoParam);
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
    OutputFormat outputFormat = OUTPUT_FORMAT_MPEG_4;
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> videoParams =
        FormatMockFactory::CreateVideoFormat(CodecMimeType::VIDEO_MPEG4, TEST_WIDTH, TEST_HEIGHT);
    videoParams->PutBuffer(MediaDescriptionKey::MD_KEY_CODEC_CONFIG, buffer_, sizeof(buffer_));

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
    OutputFormat outputFormat = OUTPUT_FORMAT_MPEG_4;
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> videoParams =
        FormatMockFactory::CreateVideoFormat(CodecMimeType::VIDEO_MPEG4, TEST_WIDTH, TEST_HEIGHT);
    videoParams->PutBuffer(MediaDescriptionKey::MD_KEY_CODEC_CONFIG, buffer_, sizeof(buffer_));

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
    OutputFormat outputFormat = OUTPUT_FORMAT_MPEG_4;
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
    OutputFormat outputFormat = OUTPUT_FORMAT_MPEG_4;
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> videoParams =
        FormatMockFactory::CreateVideoFormat(CodecMimeType::VIDEO_MPEG4, TEST_WIDTH, TEST_HEIGHT);
    videoParams->PutBuffer(MediaDescriptionKey::MD_KEY_CODEC_CONFIG, buffer_, sizeof(buffer_));

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
    bool isCreated = avmuxer_->CreateMuxer(fd_, static_cast<OutputFormat>(INVALID_FORMAT));
    ASSERT_FALSE(isCreated);

    std::shared_ptr<FormatMock> videoParams =
        FormatMockFactory::CreateVideoFormat(CodecMimeType::VIDEO_MPEG4, TEST_WIDTH, TEST_HEIGHT);
    videoParams->PutBuffer(MediaDescriptionKey::MD_KEY_CODEC_CONFIG, buffer_, sizeof(buffer_));

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
    OutputFormat outputFormat = OUTPUT_FORMAT_MPEG_4;
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> videoParams =
        FormatMockFactory::CreateVideoFormat(CodecMimeType::VIDEO_MPEG4, TEST_WIDTH, TEST_HEIGHT);
    videoParams->PutBuffer(MediaDescriptionKey::MD_KEY_CODEC_CONFIG, buffer_, sizeof(buffer_));
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
    OutputFormat outputFormat = OUTPUT_FORMAT_MPEG_4;
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
    OutputFormat outputFormat = OUTPUT_FORMAT_MPEG_4;
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> videoParams =
        FormatMockFactory::CreateVideoFormat(CodecMimeType::VIDEO_MPEG4, TEST_WIDTH, TEST_HEIGHT);
    videoParams->PutBuffer(MediaDescriptionKey::MD_KEY_CODEC_CONFIG, buffer_, sizeof(buffer_));
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
    OutputFormat outputFormat = OUTPUT_FORMAT_MPEG_4;
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> videoParams =
        FormatMockFactory::CreateVideoFormat(CodecMimeType::VIDEO_MPEG4, TEST_WIDTH, TEST_HEIGHT);
    videoParams->PutBuffer(MediaDescriptionKey::MD_KEY_CODEC_CONFIG, buffer_, sizeof(buffer_));
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
    OutputFormat outputFormat = OUTPUT_FORMAT_MPEG_4;
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> videoParams =
        FormatMockFactory::CreateVideoFormat(CodecMimeType::VIDEO_MPEG4, TEST_WIDTH, TEST_HEIGHT);
    videoParams->PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_MPEG4);
    videoParams->PutBuffer(MediaDescriptionKey::MD_KEY_CODEC_CONFIG, buffer_, sizeof(buffer_));
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
    bool isCreated = avmuxer_->CreateMuxer(fd_, static_cast<OutputFormat>(INVALID_FORMAT));
    ASSERT_FALSE(isCreated);

    std::shared_ptr<FormatMock> videoParams =
        FormatMockFactory::CreateVideoFormat(CodecMimeType::VIDEO_MPEG4, TEST_WIDTH, TEST_HEIGHT);
    videoParams->PutBuffer(MediaDescriptionKey::MD_KEY_CODEC_CONFIG, buffer_, sizeof(buffer_));
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
    OutputFormat outputFormat = OUTPUT_FORMAT_MPEG_4;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> vParam =
        FormatMockFactory::CreateVideoFormat(CodecMimeType::VIDEO_AVC, TEST_WIDTH, TEST_HEIGHT);

    int32_t ret = avmuxer_->AddTrack(trackId, vParam);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);
    ASSERT_EQ(avmuxer_->Start(), 0);

    AVCodecBufferAttrMock info;
    info.pts = 0;
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
    bool isCreated = avmuxer_->CreateMuxer(fd_, static_cast<OutputFormat>(INVALID_FORMAT));
    ASSERT_FALSE(isCreated);

    std::shared_ptr<FormatMock> vParam =
        FormatMockFactory::CreateVideoFormat(CodecMimeType::VIDEO_AVC, TEST_WIDTH, TEST_HEIGHT);

    int32_t ret = avmuxer_->AddTrack(trackId, vParam);
    ASSERT_NE(ret, 0);
    ASSERT_LT(trackId, 0);
    ASSERT_NE(avmuxer_->Start(), 0);

    AVCodecBufferAttrMock info;
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
    OutputFormat outputFormat = OUTPUT_FORMAT_MPEG_4;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> vParam =
        FormatMockFactory::CreateVideoFormat(CodecMimeType::VIDEO_AVC, TEST_WIDTH, TEST_HEIGHT);

    int32_t ret = avmuxer_->AddTrack(trackId, vParam);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);

    AVCodecBufferAttrMock info;
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
    OutputFormat outputFormat = OUTPUT_FORMAT_MPEG_4;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> vParam =
        FormatMockFactory::CreateVideoFormat(CodecMimeType::VIDEO_AVC, TEST_WIDTH, TEST_HEIGHT);

    int32_t ret = avmuxer_->AddTrack(trackId, vParam);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);
    ASSERT_EQ(avmuxer_->Start(), 0);

    AVCodecBufferAttrMock info;
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
    OutputFormat outputFormat = OUTPUT_FORMAT_MPEG_4;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> vParam =
        FormatMockFactory::CreateVideoFormat(CodecMimeType::VIDEO_AVC, TEST_WIDTH, TEST_HEIGHT);

    int32_t ret = avmuxer_->AddTrack(trackId, vParam);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);
    ASSERT_EQ(avmuxer_->Start(), 0);
    ASSERT_EQ(avmuxer_->Stop(), 0);

    AVCodecBufferAttrMock info;
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
    OutputFormat outputFormat = OUTPUT_FORMAT_MPEG_4;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> vParam =
        FormatMockFactory::CreateVideoFormat(CodecMimeType::VIDEO_AVC, TEST_WIDTH, TEST_HEIGHT);
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
    OutputFormat outputFormat = OUTPUT_FORMAT_MPEG_4;

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
    OutputFormat outputFormat = OUTPUT_FORMAT_MPEG_4;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> vParam =
        FormatMockFactory::CreateVideoFormat(CodecMimeType::VIDEO_AVC, TEST_WIDTH, TEST_HEIGHT);
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
    OutputFormat outputFormat = OUTPUT_FORMAT_MPEG_4;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> vParam =
        FormatMockFactory::CreateVideoFormat(CodecMimeType::VIDEO_AVC, TEST_WIDTH, TEST_HEIGHT);
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
    OutputFormat outputFormat = OUTPUT_FORMAT_MPEG_4;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> vParam =
        FormatMockFactory::CreateVideoFormat(CodecMimeType::VIDEO_AVC, TEST_WIDTH, TEST_HEIGHT);

    int32_t ret = avmuxer_->AddTrack(trackId, vParam);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);
    ASSERT_EQ(avmuxer_->Start(), 0);

    AVCodecBufferAttrMock info;
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
    bool isCreated = avmuxer_->CreateMuxer(fd_, static_cast<OutputFormat>(INVALID_FORMAT));
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
    OutputFormat outputFormat = OUTPUT_FORMAT_MPEG_4;

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
    OutputFormat outputFormat = OUTPUT_FORMAT_MPEG_4;

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
        return;
    }

    constexpr int32_t validVideoDelay = 1;
    constexpr int32_t invalidVideoDelay = -1;
    constexpr int32_t validFrameRate = 30;
    constexpr int32_t invalidFrameRate = -1;

    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_H265.mp4");
    OutputFormat outputFormat = OUTPUT_FORMAT_MPEG_4;
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> videoParams =
        FormatMockFactory::CreateVideoFormat(CodecMimeType::VIDEO_HEVC, TEST_WIDTH, TEST_HEIGHT);

    videoParams->PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_DELAY, validVideoDelay);
    int32_t ret = avmuxer_->AddTrack(trackId, videoParams);
    ASSERT_NE(ret, 0);

    videoParams->PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_DELAY, invalidVideoDelay);
    videoParams->PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, validFrameRate);
    ret = avmuxer_->AddTrack(trackId, videoParams);
    ASSERT_NE(ret, 0);

    videoParams->PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_DELAY, validVideoDelay);
    videoParams->PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, invalidFrameRate);
    ret = avmuxer_->AddTrack(trackId, videoParams);
    ASSERT_NE(ret, 0);

    videoParams->PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_DELAY, 0xFF);
    videoParams->PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, validFrameRate);
    ret = avmuxer_->AddTrack(trackId, videoParams);
    ASSERT_NE(ret, 0);

    videoParams->PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_DELAY, validVideoDelay);
    videoParams->PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, validFrameRate);
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
    OutputFormat outputFormat = OUTPUT_FORMAT_MPEG_4;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> videoParams =
        FormatMockFactory::CreateVideoFormat(CodecMimeType::VIDEO_HEVC, TEST_WIDTH, TEST_HEIGHT);

    int32_t ret = avmuxer_->AddTrack(trackId, videoParams);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);
    ASSERT_EQ(avmuxer_->Start(), 0);

    AVCodecBufferAttrMock info;
    info.pts = 0;
    info.size = sizeof(buffer_);

    ret = avmuxer_->WriteSample(trackId + 1, buffer_, info);
    ASSERT_NE(ret, 0);

    ret = avmuxer_->WriteSample(-1, buffer_, info);
    ASSERT_NE(ret, 0);

    ret = avmuxer_->WriteSample(invalidTrackId, buffer_, info);
    ASSERT_NE(ret, 0);

    ret = avmuxer_->WriteSample(trackId, buffer_, info);
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
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_Destro.mp4");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OutputFormat outputFormat = OUTPUT_FORMAT_MPEG_4;
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> videoParams =
        FormatMockFactory::CreateVideoFormat(CodecMimeType::VIDEO_MPEG4, TEST_WIDTH, TEST_HEIGHT);
    videoParams->PutBuffer(MediaDescriptionKey::MD_KEY_CODEC_CONFIG, buffer_, sizeof(buffer_));
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