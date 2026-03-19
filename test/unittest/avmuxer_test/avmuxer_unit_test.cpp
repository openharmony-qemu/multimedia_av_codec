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
#include "native_audio_channel_layout.h"
#ifdef AVMUXER_UNITTEST_CAPI
#include "native_avmuxer.h"
#include "native_avformat.h"
#include "native_avsource.h"
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
#ifdef AVMUXER_UNITTEST_CAPI
constexpr int32_t ABNORMAL_COLOR_VALUE = 100;
constexpr int32_t ABNORMAL_COLOR_VALUE_1 = -1;
#endif
const std::string TEST_FILE_PATH = "/data/test/media/";
const std::string INPUT_FILE_PATH = "/data/test/media/h264_720_480.dat";
const std::string LOGINFO_INPUT_FILE_PATH = "/data/test/media/h265_logInfo.dat";
const std::string HEVC_LIB_PATH = std::string(AV_CODEC_PATH) + "/libav_codec_hevc_parser.z.so";
constexpr uint32_t AVCODEC_BUFFER_FLAGS_DISPOSABLE_EXT_TEST = 1 << 6;
const std::string TIMED_METADATA_TRACK_MIMETYPE = "meta/timed-metadata";
const std::string TIMED_METADATA_KEY = "com.openharmony.timed_metadata.test";
const std::string TRACK_REF_TYPE_DEPTH = "vdep";
const std::string TRACK_REF_TYPE_PREY = "auxl";
const std::string TRACK_REF_TYPE_AUDIO = "auxl";
const std::string AUXILIARY_DEPTH_TRACK_KEY = "com.openharmony.moviemode.depth";
const std::string AUXILIARY_PREY_TRACK_KEY = "com.openharmony.moviemode.prey";
const std::string AUXILIARY_AUDIO_TRACK_KEY = "com.openharmony.audiomode.auxiliary";
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

int32_t AVMuxerUnitTest::WriteSample(sptr<AVBufferQueueProducer> bqProducer,
    std::shared_ptr<std::ifstream> file, bool &eosFlag)
{
    if (file->eof()) {
        eosFlag = true;
        return 0;
    }
    int64_t pts = 0;
    file->read(reinterpret_cast<char*>(&pts), sizeof(pts));
    if (file->eof()) {
        eosFlag = true;
        return 0;
    }
    uint32_t flags = 0;
    file->read(reinterpret_cast<char*>(&flags), sizeof(flags));
    if (file->eof()) {
        eosFlag = true;
        return 0;
    }
    int32_t size = 0;
    file->read(reinterpret_cast<char*>(&size), sizeof(size));
    if (file->eof()) {
        eosFlag = true;
        return 0;
    }
    std::shared_ptr<AVBuffer> buffer = nullptr;
    AVBufferConfig avBufferConfig;
    avBufferConfig.size = size;
    avBufferConfig.memoryType = MemoryType::VIRTUAL_MEMORY;
    bqProducer->RequestBuffer(buffer, avBufferConfig, -1);
    buffer->pts_ = pts;
    buffer->flag_ = flags;
    file->read(reinterpret_cast<char*>(buffer->memory_->GetAddr()), size);
    buffer->memory_->SetSize(size);
    Status ret = bqProducer->PushBuffer(buffer, true);
    if (ret == Status::OK) {
        return 0;
    }
    return -1;
}

void AVMuxerUnitTest::TrackWriteSample(std::string inputFilePath, int32_t trackId)
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
    ASSERT_TRUE(isCreated);
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
    outputFormat = AV_OUTPUT_FORMAT_WAV;
    isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    avmuxer_->Destroy();
    outputFormat = AV_OUTPUT_FORMAT_AAC;
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
        OH_AVCODEC_MIMETYPE_AUDIO_AAC,
        OH_AVCODEC_MIMETYPE_VIDEO_AVC,
        OH_AVCODEC_MIMETYPE_IMAGE_JPG,
        OH_AVCODEC_MIMETYPE_IMAGE_PNG,
        OH_AVCODEC_MIMETYPE_IMAGE_BMP,
    };

    const std::vector<std::string_view> testM4aMimeTypeList =
    {
        OH_AVCODEC_MIMETYPE_AUDIO_AAC,
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
    avParam->PutIntValue(OH_MD_KEY_PROFILE, AAC_PROFILE_LC);
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
 * @tc.name: Muxer_AddTrack_009
 * @tc.desc: Create AAC Muxer AddTrack
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_AddTrack_009, TestSize.Level0)
{
    int32_t trackId = -2; // -2: Initialize to an invalid ID
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_AddTrack.aac");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, static_cast<OH_AVOutputFormat>(AV_OUTPUT_FORMAT_AAC));
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> avParam = FormatMockFactory::CreateFormat();
    avParam->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_AAC);
    avParam->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, 16000); // 16000: 16khz sample rate
    avParam->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, 1); // 1: 1 audio channel, mono
    avParam->PutIntValue(OH_MD_KEY_PROFILE, AAC_PROFILE_LC);
    avParam->PutIntValue(OH_MD_KEY_AAC_IS_ADTS, 0);

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
    EXPECT_EQ(avmuxer_->Start(), 0);
    EXPECT_EQ(avmuxer_->Stop(), 0);
}


/**
 * @tc.name: Muxer_SetRotation_002
 * @tc.desc: Muxer SetRotation after Create
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_SetRotation_002, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_SetRotation.mp4");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    int32_t ret = avmuxer_->SetRotation(180); // 180 rotation
    EXPECT_EQ(ret, 0);

    std::shared_ptr<FormatMock> vParam =
    FormatMockFactory::CreateVideoFormat(OH_AVCODEC_MIMETYPE_VIDEO_AVC, TEST_WIDTH, TEST_HEIGHT);
    ret = avmuxer_->AddTrack(trackId, vParam);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);

    EXPECT_EQ(avmuxer_->Start(), 0);
    EXPECT_EQ(avmuxer_->Stop(), 0);
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
    int32_t trackId = -1;
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

    std::shared_ptr<FormatMock> vParam =
        FormatMockFactory::CreateVideoFormat(OH_AVCODEC_MIMETYPE_VIDEO_AVC, TEST_WIDTH, TEST_HEIGHT);
    ret = avmuxer_->AddTrack(trackId, vParam);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);

    EXPECT_EQ(avmuxer_->Start(), 0);
    EXPECT_EQ(avmuxer_->Stop(), 0);
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
    int32_t ret = 0;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_H265.mp4");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> videoParams =
        FormatMockFactory::CreateVideoFormat(OH_AVCODEC_MIMETYPE_VIDEO_HEVC, TEST_WIDTH, TEST_HEIGHT);

    videoParams->PutIntValue("video_delay", validVideoDelay);
    ret = avmuxer_->AddTrack(trackId, videoParams);

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
 * @tc.name: Muxer_Hevc_WriteSample_005
 * @tc.desc: Muxer Hevc Write Sample include sei logInfo
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_Hevc_WriteSample_005, TestSize.Level0)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }

    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_H265_logInfo.mp4");
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

    inputFile_ = std::make_shared<std::ifstream>(LOGINFO_INPUT_FILE_PATH, std::ios::binary);

    int32_t extSize = 0;
    inputFile_->read(reinterpret_cast<char*>(&extSize), sizeof(extSize));
    if (extSize > 0) {
        std::vector<uint8_t> buffer(extSize);
        inputFile_->read(reinterpret_cast<char*>(buffer.data()), extSize);
    }

    bool eosFlag = false;
    uint32_t flag = AVCODEC_BUFFER_FLAGS_SYNC_FRAME;
    ret = WriteSample(trackId, inputFile_, eosFlag, flag);
    while (!eosFlag && (ret == 0)) {
        ret = WriteSample(trackId, inputFile_, eosFlag, flag);
    }
    ASSERT_EQ(ret, 0);
}

/**
 * @tc.name: Muxer_Hevc_Optimize_Filler_Data_001
 * @tc.desc: Muxer Hevc Optimize Filler Data
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_Hevc_Optimize_Filler_Data_001, TestSize.Level0)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) == 0) {
        int32_t trackId = -1;
        std::string outputFile = TEST_FILE_PATH + std::string("Muxer_Optimize_Filler_Data.mp4");
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

        inputFile_ = std::make_shared<std::ifstream>(LOGINFO_INPUT_FILE_PATH, std::ios::binary);

        int32_t extSize = 0;
        inputFile_->read(reinterpret_cast<char*>(&extSize), sizeof(extSize));
        if (extSize > 0) {
            std::vector<uint8_t> buffer(extSize);
            inputFile_->read(reinterpret_cast<char*>(buffer.data()), extSize);
        }

        bool eosFlag = false;
        uint32_t flag = AVCODEC_BUFFER_FLAGS_SYNC_FRAME;
        ret = WriteSample(trackId, inputFile_, eosFlag, flag);
        while (!eosFlag && (ret == 0)) {
            ret = WriteSample(trackId, inputFile_, eosFlag, flag);
        }
        ASSERT_EQ(ret, 0);
    }
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

    ASSERT_EQ(avmuxer_->SetRotation(180), 0); // 180 rotation

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
    ASSERT_EQ(avmuxer_->Stop(), 0);
}

/**
 * @tc.name: Muxer_WAV_001
 * @tc.desc: Muxer mux the wav by g711mu
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_WAV_001, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_G711MU_44100_2.wav");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_WAV;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_G711MU);
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, 44100); // 44100 sample rate
    audioParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, 2); // 2 channels
    audioParams->PutIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_U8);
    audioParams->PutLongValue(OH_MD_KEY_BITRATE, 705600); // 705600 bit rate
    audioParams->PutIntValue("audio_samples_per_frame", 2048); // 2048 frame size
    int32_t ret = avmuxer_->AddTrack(trackId, audioParams);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);
    ASSERT_EQ(avmuxer_->Start(), 0);

    inputFile_ = std::make_shared<std::ifstream>("/data/test/media/g711mu_44100_2.dat", std::ios::binary);

    int32_t extSize = 0;
    inputFile_->read(reinterpret_cast<char*>(&extSize), sizeof(extSize));
    if (extSize > 0) {
        std::vector<uint8_t> buffer(extSize);
        inputFile_->read(reinterpret_cast<char*>(buffer.data()), extSize);
    }

    bool eosFlag = false;
    uint32_t flag = AVCODEC_BUFFER_FLAGS_SYNC_FRAME;
    ret = WriteSample(trackId, inputFile_, eosFlag, flag);
    while (!eosFlag && (ret == 0)) {
        ret = WriteSample(trackId, inputFile_, eosFlag, flag);
    }
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(avmuxer_->Stop(), 0);
}

/**
 * @tc.name: Muxer_WAV_002
 * @tc.desc: Muxer mux the wav by pcm
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_WAV_002, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_PCM_44100_2_S16LE.wav");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_WAV;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, "audio/raw");
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, 44100); // 44100 sample rate
    audioParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, 2); // 2 channels
    audioParams->PutIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    audioParams->PutLongValue(OH_MD_KEY_BITRATE, 1411200); // 1411200 bit rate
    audioParams->PutIntValue("audio_samples_per_frame", 1024); // 1024 frame size
    audioParams->PutLongValue(OH_MD_KEY_CHANNEL_LAYOUT, CH_LAYOUT_STEREO);
    int32_t ret = avmuxer_->AddTrack(trackId, audioParams);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);
    ASSERT_EQ(avmuxer_->Start(), 0);

    inputFile_ = std::make_shared<std::ifstream>("/data/test/media/pcm_44100_2_s16le.dat", std::ios::binary);

    int32_t extSize = 0;
    inputFile_->read(reinterpret_cast<char*>(&extSize), sizeof(extSize));
    if (extSize > 0) {
        std::vector<uint8_t> buffer(extSize);
        inputFile_->read(reinterpret_cast<char*>(buffer.data()), extSize);
    }

    bool eosFlag = false;
    uint32_t flag = AVCODEC_BUFFER_FLAGS_SYNC_FRAME;
    ret = WriteSample(trackId, inputFile_, eosFlag, flag);
    while (!eosFlag && (ret == 0)) {
        ret = WriteSample(trackId, inputFile_, eosFlag, flag);
    }
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(avmuxer_->Stop(), 0);
}

/**
 * @tc.name: Muxer_WAV_003
 * @tc.desc: Muxer mux the wav by pcm, test OH_MD_KEY_AUDIO_SAMPLE_FORMAT
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_WAV_003, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_PCM_44100_2_XXX.wav");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_WAV;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, "audio/raw");
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, 44100); // 44100 sample rate
    audioParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, 2); // 2 channels
    audioParams->PutIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_U8P);
    audioParams->PutLongValue(OH_MD_KEY_BITRATE, 705600); // 705600 bit rate
    audioParams->PutIntValue("audio_samples_per_frame", 1024); // 1024 frame size
    audioParams->PutLongValue(OH_MD_KEY_CHANNEL_LAYOUT, CH_LAYOUT_STEREO);
    ASSERT_NE(avmuxer_->AddTrack(trackId, audioParams), 0);
    audioParams->PutIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16P);
    ASSERT_NE(avmuxer_->AddTrack(trackId, audioParams), 0);
    audioParams->PutIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S24P);
    ASSERT_NE(avmuxer_->AddTrack(trackId, audioParams), 0);
    audioParams->PutIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S32P);
    ASSERT_NE(avmuxer_->AddTrack(trackId, audioParams), 0);
    audioParams->PutIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_F32P);
    ASSERT_NE(avmuxer_->AddTrack(trackId, audioParams), 0);
}

/**
 * @tc.name: Muxer_WAV_004
 * @tc.desc: Muxer mux the wav by pcm, test OH_MD_KEY_CHANNEL_LAYOUT
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_WAV_004, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_PCM_44100_2_XXX.wav");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_WAV;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, "audio/raw");
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, 44100); // 44100 sample rate
    audioParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, 2); // 2 channels
    audioParams->PutIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    audioParams->PutLongValue(OH_MD_KEY_BITRATE, 1411200); // 1411200 bit rate
    audioParams->PutIntValue("audio_samples_per_frame", 1024); // 1024 frame size
    audioParams->PutLongValue(OH_MD_KEY_CHANNEL_LAYOUT, CH_LAYOUT_2POINT0POINT2);
    ASSERT_NE(avmuxer_->AddTrack(trackId, audioParams), 0);
    audioParams->PutLongValue(OH_MD_KEY_CHANNEL_LAYOUT, CH_LAYOUT_AMB_ORDER1_ACN_N3D);
    ASSERT_NE(avmuxer_->AddTrack(trackId, audioParams), 0);
    audioParams->PutLongValue(OH_MD_KEY_CHANNEL_LAYOUT, CH_LAYOUT_AMB_ORDER1_FUMA);
    ASSERT_NE(avmuxer_->AddTrack(trackId, audioParams), 0);
}

/**
 * @tc.name: Muxer_AAC_001
 * @tc.desc: Muxer mux the aac with adts header
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_AAC_001, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_AAC_44100_2.aac");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_AAC;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_AAC);
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, 44100); // 44100 sample rate
    audioParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, 2); // 2 channels
    audioParams->PutIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_U8);
    audioParams->PutLongValue(OH_MD_KEY_BITRATE, 705600); // 705600 bit rate
    audioParams->PutIntValue("audio_samples_per_frame", 2048); // 2048 frame size
    audioParams->PutIntValue(OH_MD_KEY_PROFILE, AAC_PROFILE_LC);
    audioParams->PutIntValue(OH_MD_KEY_AAC_IS_ADTS, 0);
    int32_t ret = avmuxer_->AddTrack(trackId, audioParams);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);
    ASSERT_EQ(avmuxer_->Start(), 0);

    inputFile_ = std::make_shared<std::ifstream>("/data/test/media/aac_2c_44100hz_199k_muxer.dat", std::ios::binary);

    int32_t extSize = 0;
    inputFile_->read(reinterpret_cast<char*>(&extSize), sizeof(extSize));
    if (extSize > 0) {
        std::vector<uint8_t> buffer(extSize);
        inputFile_->read(reinterpret_cast<char*>(buffer.data()), extSize);
    }

    bool eosFlag = false;
    uint32_t flag = AVCODEC_BUFFER_FLAGS_SYNC_FRAME;
    ret = WriteSample(trackId, inputFile_, eosFlag, flag);
    while (!eosFlag && (ret == 0)) {
        ret = WriteSample(trackId, inputFile_, eosFlag, flag);
    }
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(avmuxer_->Stop(), 0);
}

/**
 * @tc.name: Muxer_AAC_002
 * @tc.desc: Muxer mux aac, test OH_MD_KEY_AUDIO_SAMPLE_FORMAT
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_AAC_002, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_AAC_44100_2_XXX.aac");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_AAC;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_AAC);
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, 44100); // 44100 sample rate
    audioParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, 2); // 2 channels
    audioParams->PutIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_U8P);
    audioParams->PutLongValue(OH_MD_KEY_BITRATE, 705600); // 705600 bit rate
    audioParams->PutIntValue("audio_samples_per_frame", 1024); // 1024 frame size
    audioParams->PutIntValue(OH_MD_KEY_PROFILE, AAC_PROFILE_LC);
    audioParams->PutIntValue(OH_MD_KEY_AAC_IS_ADTS, 0);
    audioParams->PutLongValue(OH_MD_KEY_CHANNEL_LAYOUT, CH_LAYOUT_STEREO);
    ASSERT_EQ(avmuxer_->AddTrack(trackId, audioParams), 0);
}

/**
 * @tc.name: Muxer_AAC_003
 * @tc.desc: Muxer mux aac, test OH_MD_KEY_CHANNEL_LAYOUT
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_AAC_003, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_AAC_44100_2_XXX.aac");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_AAC;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_AAC);
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, 44100); // 44100 sample rate
    audioParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, 2); // 2 channels
    audioParams->PutIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    audioParams->PutLongValue(OH_MD_KEY_BITRATE, 1411200); // 1411200 bit rate
    audioParams->PutIntValue("audio_samples_per_frame", 1024); // 1024 frame size
    audioParams->PutLongValue(OH_MD_KEY_CHANNEL_LAYOUT, CH_LAYOUT_2POINT0POINT2);
    ASSERT_NE(avmuxer_->AddTrack(trackId, audioParams), 0);
    audioParams->PutLongValue(OH_MD_KEY_CHANNEL_LAYOUT, CH_LAYOUT_AMB_ORDER1_ACN_N3D);
    ASSERT_NE(avmuxer_->AddTrack(trackId, audioParams), 0);
    audioParams->PutLongValue(OH_MD_KEY_CHANNEL_LAYOUT, CH_LAYOUT_AMB_ORDER1_FUMA);
    ASSERT_NE(avmuxer_->AddTrack(trackId, audioParams), 0);
}

/**
 * @tc.name: Muxer_AAC_004
 * @tc.desc: Muxer check unsupported adts value
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_AAC_004, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_AAC_44100_2.aac");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_AAC;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_AAC);
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, 44100); // 44100 sample rate
    audioParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, 2); // 2 channels
    audioParams->PutIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_U8);
    audioParams->PutLongValue(OH_MD_KEY_BITRATE, 705600); // 705600 bit rate
    audioParams->PutIntValue("audio_samples_per_frame", 2048); // 2048 frame size
    audioParams->PutIntValue(OH_MD_KEY_PROFILE, AAC_PROFILE_LC);
    audioParams->PutIntValue(OH_MD_KEY_AAC_IS_ADTS, 3); // unsupported aac_is_adts value
    int32_t ret = avmuxer_->AddTrack(trackId, audioParams);
    ret = (ret == AV_ERR_OK) ? AV_ERR_OK : AV_ERR_INVALID_VAL;
    ASSERT_EQ(ret, AV_ERR_INVALID_VAL);
}

/**
 * @tc.name: Muxer_AAC_005
 * @tc.desc: Muxer check unsupported profile value
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_AAC_005, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_AAC_44100_2.aac");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_AAC;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_AAC);
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, 44100); // 44100 sample rate
    audioParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, 2); // 2 channels
    audioParams->PutIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_U8);
    audioParams->PutLongValue(OH_MD_KEY_BITRATE, 705600); // 705600 bit rate
    audioParams->PutIntValue("audio_samples_per_frame", 2048); // 2048 frame size
    audioParams->PutIntValue(OH_MD_KEY_PROFILE, 2); // unsupported profile value
    audioParams->PutIntValue(OH_MD_KEY_AAC_IS_ADTS, 0);
    int32_t ret = avmuxer_->AddTrack(trackId, audioParams);
    ret = (ret == AV_ERR_OK) ? AV_ERR_OK : AV_ERR_INVALID_VAL;
    ASSERT_EQ(ret, AV_ERR_INVALID_VAL);
}

/**
 * @tc.name: Muxer_AddTrack_Auxiliary_001
 * @tc.desc: Muxer AddTrack video Auxiliary track.
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_AddTrack_Auxiliary_001, TestSize.Level0) {
    int32_t trackId = -1;
    int32_t trackIdDepth = -1;
    std::vector<int32_t> vDepth = {0};
    int32_t *trackIdsDepth = vDepth.data();
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_AddTrack_Auxiliary.mp4");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> videoParams =
        FormatMockFactory::CreateVideoFormat(OH_AVCODEC_MIMETYPE_VIDEO_AVC, TEST_WIDTH, TEST_HEIGHT);

    int32_t ret = avmuxer_->AddTrack(trackId, videoParams);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);

    std::shared_ptr<FormatMock> metadataParamsDepth = FormatMockFactory::CreateFormat();
    metadataParamsDepth->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    metadataParamsDepth->PutIntValue(OH_MD_KEY_WIDTH, TEST_WIDTH);
    metadataParamsDepth->PutIntValue(OH_MD_KEY_HEIGHT, TEST_HEIGHT);
    metadataParamsDepth->PutIntValue(OH_MD_KEY_TRACK_TYPE, static_cast<int32_t>(OH_MediaType::MEDIA_TYPE_AUXILIARY));
    metadataParamsDepth->PutStringValue(OH_MD_KEY_TRACK_REFERENCE_TYPE, TRACK_REF_TYPE_DEPTH);
    metadataParamsDepth->PutStringValue(OH_MD_KEY_TRACK_DESCRIPTION, AUXILIARY_DEPTH_TRACK_KEY);
    metadataParamsDepth->PutIntBuffer(OH_MD_KEY_REFERENCE_TRACK_IDS, trackIdsDepth, vDepth.size());

    ret = avmuxer_->AddTrack(trackIdDepth, metadataParamsDepth);
    ASSERT_EQ(ret, AV_ERR_OK);
    ASSERT_GE(trackIdDepth, 1);
}

/**
 * @tc.name: Muxer_AddTrack_Auxiliary_002
 * @tc.desc: Muxer AddTrack video Auxiliary track(no parameter).
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_AddTrack_Auxiliary_002, TestSize.Level0) {
    int32_t trackId = -1;
    int32_t trackIdDepth = -1;
    std::vector<int32_t> vDepth = {0};
    int32_t *trackIdsDepth = vDepth.data();
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_AddTrack_Auxiliary.mp4");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> videoParams =
        FormatMockFactory::CreateVideoFormat(OH_AVCODEC_MIMETYPE_VIDEO_AVC, TEST_WIDTH, TEST_HEIGHT);

    int32_t ret = avmuxer_->AddTrack(trackId, videoParams);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);

    std::shared_ptr<FormatMock> metadataParamsDepth = FormatMockFactory::CreateFormat();
    metadataParamsDepth->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    metadataParamsDepth->PutIntValue(OH_MD_KEY_WIDTH, TEST_WIDTH);
    metadataParamsDepth->PutIntValue(OH_MD_KEY_HEIGHT, TEST_HEIGHT);

    metadataParamsDepth->PutIntValue(OH_MD_KEY_TRACK_TYPE, static_cast<int32_t>(OH_MediaType::MEDIA_TYPE_AUXILIARY));
    ret = avmuxer_->AddTrack(trackIdDepth, metadataParamsDepth);
    ASSERT_NE(ret, AV_ERR_OK);

    metadataParamsDepth->PutStringValue(OH_MD_KEY_TRACK_REFERENCE_TYPE, TRACK_REF_TYPE_DEPTH);
    ret = avmuxer_->AddTrack(trackIdDepth, metadataParamsDepth);
    ASSERT_NE(ret, AV_ERR_OK);

    metadataParamsDepth->PutStringValue(OH_MD_KEY_TRACK_DESCRIPTION, AUXILIARY_DEPTH_TRACK_KEY);
    ret = avmuxer_->AddTrack(trackIdDepth, metadataParamsDepth);
    ASSERT_NE(ret, AV_ERR_OK);

    metadataParamsDepth->PutIntBuffer(OH_MD_KEY_REFERENCE_TRACK_IDS, trackIdsDepth, vDepth.size());
    ret = avmuxer_->AddTrack(trackIdDepth, metadataParamsDepth);
    ASSERT_EQ(ret, AV_ERR_OK);
}

/**
 * @tc.name: Muxer_AddTrack_Auxiliary_003
 * @tc.desc: Muxer AddTrack video Auxiliary track(H265 video).
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_AddTrack_Auxiliary_003, TestSize.Level0) {
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    int32_t trackId = -1;
    int32_t trackIdDepth = -1;
    std::vector<int32_t> vDepth = {0};
    int32_t *trackIdsDepth = vDepth.data();
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_AddTrack_Auxiliary.mp4");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> videoParams =
        FormatMockFactory::CreateVideoFormat(OH_AVCODEC_MIMETYPE_VIDEO_HEVC, TEST_WIDTH, TEST_HEIGHT);

    int32_t ret = avmuxer_->AddTrack(trackId, videoParams);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);

    std::shared_ptr<FormatMock> metadataParamsDepth = FormatMockFactory::CreateFormat();
    metadataParamsDepth->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_VIDEO_HEVC);
    metadataParamsDepth->PutIntValue(OH_MD_KEY_WIDTH, TEST_WIDTH);
    metadataParamsDepth->PutIntValue(OH_MD_KEY_HEIGHT, TEST_HEIGHT);
    metadataParamsDepth->PutIntValue(OH_MD_KEY_TRACK_TYPE, static_cast<int32_t>(OH_MediaType::MEDIA_TYPE_AUXILIARY));
    metadataParamsDepth->PutStringValue(OH_MD_KEY_TRACK_REFERENCE_TYPE, TRACK_REF_TYPE_DEPTH);
    metadataParamsDepth->PutStringValue(OH_MD_KEY_TRACK_DESCRIPTION, AUXILIARY_DEPTH_TRACK_KEY);
    metadataParamsDepth->PutIntBuffer(OH_MD_KEY_REFERENCE_TRACK_IDS, trackIdsDepth, vDepth.size());

    ret = avmuxer_->AddTrack(trackIdDepth, metadataParamsDepth);
    ASSERT_EQ(ret, AV_ERR_OK);
    ASSERT_GE(trackIdDepth, 1);
}

/**
 * @tc.name: Muxer_AddTrack_Auxiliary_004
 * @tc.desc: Muxer AddTrack video Auxiliary track(invalid reference track id).
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_AddTrack_Auxiliary_004, TestSize.Level0) {
    int32_t trackId = -1;
    int32_t trackIdDepth = -1;
    std::vector<int32_t> vDepth = {10};    // invalid reference track id
    int32_t *trackIdsDepth = vDepth.data();
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_AddTrack_Auxiliary.mp4");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> videoParams =
        FormatMockFactory::CreateVideoFormat(OH_AVCODEC_MIMETYPE_VIDEO_AVC, TEST_WIDTH, TEST_HEIGHT);

    int32_t ret = avmuxer_->AddTrack(trackId, videoParams);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);

    std::shared_ptr<FormatMock> metadataParamsDepth = FormatMockFactory::CreateFormat();
    metadataParamsDepth->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    metadataParamsDepth->PutIntValue(OH_MD_KEY_WIDTH, TEST_WIDTH);
    metadataParamsDepth->PutIntValue(OH_MD_KEY_HEIGHT, TEST_HEIGHT);
    metadataParamsDepth->PutIntValue(OH_MD_KEY_TRACK_TYPE, static_cast<int32_t>(OH_MediaType::MEDIA_TYPE_AUXILIARY));
    metadataParamsDepth->PutStringValue(OH_MD_KEY_TRACK_REFERENCE_TYPE, TRACK_REF_TYPE_DEPTH);
    metadataParamsDepth->PutStringValue(OH_MD_KEY_TRACK_DESCRIPTION, AUXILIARY_DEPTH_TRACK_KEY);
    metadataParamsDepth->PutIntBuffer(OH_MD_KEY_REFERENCE_TRACK_IDS, trackIdsDepth, vDepth.size());

    ret = avmuxer_->AddTrack(trackIdDepth, metadataParamsDepth);
    ASSERT_NE(ret, AV_ERR_OK);
}

/**
 * @tc.name: Muxer_AddTrack_Auxiliary_005
 * @tc.desc: Muxer AddTrack video Auxiliary track(use PutBuffer).
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_AddTrack_Auxiliary_005, TestSize.Level0) {
    int32_t trackId = -1;
    int32_t trackIdDepth = -1;
    std::vector<int32_t> vDepth = {0};
    int32_t *trackIdsDepth = vDepth.data();
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_AddTrack_Auxiliary.mp4");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> videoParams =
        FormatMockFactory::CreateVideoFormat(OH_AVCODEC_MIMETYPE_VIDEO_AVC, TEST_WIDTH, TEST_HEIGHT);

    int32_t ret = avmuxer_->AddTrack(trackId, videoParams);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);

    std::shared_ptr<FormatMock> metadataParamsDepth = FormatMockFactory::CreateFormat();
    metadataParamsDepth->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    metadataParamsDepth->PutIntValue(OH_MD_KEY_WIDTH, TEST_WIDTH);
    metadataParamsDepth->PutIntValue(OH_MD_KEY_HEIGHT, TEST_HEIGHT);
    metadataParamsDepth->PutIntValue(OH_MD_KEY_TRACK_TYPE, static_cast<int32_t>(OH_MediaType::MEDIA_TYPE_AUXILIARY));
    metadataParamsDepth->PutStringValue(OH_MD_KEY_TRACK_REFERENCE_TYPE, TRACK_REF_TYPE_DEPTH);
    metadataParamsDepth->PutStringValue(OH_MD_KEY_TRACK_DESCRIPTION, AUXILIARY_DEPTH_TRACK_KEY);
    std::vector<int32_t> vPutBuffer = {0};
    int32_t *trackIdsPutBuffer = vPutBuffer.data();
    ret = metadataParamsDepth->PutBuffer(OH_MD_KEY_REFERENCE_TRACK_IDS, reinterpret_cast<uint8_t*>(trackIdsPutBuffer),
        sizeof(int32_t) * vPutBuffer.size());
    ASSERT_NE(ret, true);
    ret = metadataParamsDepth->PutIntBuffer(OH_MD_KEY_REFERENCE_TRACK_IDS, trackIdsDepth, vDepth.size());
    ASSERT_EQ(ret, true);

    ret = avmuxer_->AddTrack(trackIdDepth, metadataParamsDepth);
    ASSERT_EQ(ret, AV_ERR_OK);
}

/**
 * @tc.name: Muxer_Add_Video_Auxiliary
 * @tc.desc: Muxer add video Auxiliary.
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_Add_Video_Auxiliary, TestSize.Level0) {
    int32_t trackId = -1;
    int32_t trackIdDepth = -1;
    int32_t trackIdPrey = -1;
    std::vector<int32_t> vDepth = {0};
    int32_t *trackIdsDepth = vDepth.data();
    std::vector<int32_t> vPrey = {0, 1};
    int32_t *trackIdsPrey = vPrey.data();
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_Add_Video_Auxiliary.mp4");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> videoParams =
        FormatMockFactory::CreateVideoFormat(OH_AVCODEC_MIMETYPE_VIDEO_AVC, TEST_WIDTH, TEST_HEIGHT);

    int32_t ret = avmuxer_->AddTrack(trackId, videoParams);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);

    // create auxiliary format depth track
    std::shared_ptr<FormatMock> metadataParamsDepth = FormatMockFactory::CreateFormat();
    metadataParamsDepth->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    metadataParamsDepth->PutIntValue(OH_MD_KEY_WIDTH, TEST_WIDTH);
    metadataParamsDepth->PutIntValue(OH_MD_KEY_HEIGHT, TEST_HEIGHT);
    metadataParamsDepth->PutIntValue(OH_MD_KEY_TRACK_TYPE, static_cast<int32_t>(OH_MediaType::MEDIA_TYPE_AUXILIARY));
    metadataParamsDepth->PutStringValue(OH_MD_KEY_TRACK_REFERENCE_TYPE, TRACK_REF_TYPE_DEPTH);
    metadataParamsDepth->PutStringValue(OH_MD_KEY_TRACK_DESCRIPTION, AUXILIARY_DEPTH_TRACK_KEY);
    metadataParamsDepth->PutIntBuffer(OH_MD_KEY_REFERENCE_TRACK_IDS, trackIdsDepth, vDepth.size());

    ret = avmuxer_->AddTrack(trackIdDepth, metadataParamsDepth);
    ASSERT_EQ(ret, AV_ERR_OK);
    ASSERT_GE(trackIdDepth, 1);

    // create auxiliary format prey track
    std::shared_ptr<FormatMock> metadataParamsPrey = FormatMockFactory::CreateFormat();
    metadataParamsPrey->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    metadataParamsPrey->PutIntValue(OH_MD_KEY_WIDTH, TEST_WIDTH);
    metadataParamsPrey->PutIntValue(OH_MD_KEY_HEIGHT, TEST_HEIGHT);
    metadataParamsPrey->PutIntValue(OH_MD_KEY_TRACK_TYPE, static_cast<int32_t>(OH_MediaType::MEDIA_TYPE_AUXILIARY));
    metadataParamsPrey->PutStringValue(OH_MD_KEY_TRACK_REFERENCE_TYPE, TRACK_REF_TYPE_PREY);
    metadataParamsPrey->PutStringValue(OH_MD_KEY_TRACK_DESCRIPTION, AUXILIARY_PREY_TRACK_KEY);
    metadataParamsPrey->PutIntBuffer(OH_MD_KEY_REFERENCE_TRACK_IDS, trackIdsPrey, vPrey.size());

    ret = avmuxer_->AddTrack(trackIdPrey, metadataParamsPrey);
    ASSERT_EQ(ret, AV_ERR_OK);
    ASSERT_GE(trackIdPrey, 2);

    ASSERT_EQ(avmuxer_->Start(), 0);

    TrackWriteSample(INPUT_FILE_PATH, trackId);
    TrackWriteSample(INPUT_FILE_PATH, trackIdDepth);
    TrackWriteSample(INPUT_FILE_PATH, trackIdPrey);

    ASSERT_EQ(avmuxer_->Stop(), 0);
}

/**
 * @tc.name: Muxer_Add_Audio_Auxiliary
 * @tc.desc: Muxer add audio Auxiliary.
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_Add_Audio_Auxiliary, TestSize.Level0) {
    int32_t trackId = -1;
    int32_t trackIdAudio = -1;
    std::vector<int32_t> vAudio = {0};
    int32_t *trackIdsAudio = vAudio.data();
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_Add_Audio_Auxiliary.mp4");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_AAC);
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, 44100); // 44100 sample rate
    audioParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, 2); // 2 channels
    audioParams->PutIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    audioParams->PutLongValue(OH_MD_KEY_BITRATE, 199000); // 199000 bit rate
    audioParams->PutIntValue("audio_samples_per_frame", 1024); // 1024 frame size
    audioParams->PutIntValue(OH_MD_KEY_PROFILE, AAC_PROFILE_LC);
    audioParams->PutIntValue(OH_MD_KEY_AAC_IS_ADTS, 0);

    int32_t ret = avmuxer_->AddTrack(trackId, audioParams);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);

    std::shared_ptr<FormatMock> audioAuxiliaryParams = FormatMockFactory::CreateFormat();
    audioAuxiliaryParams->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_AAC);
    audioAuxiliaryParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, 44100); // 44100 sample rate
    audioAuxiliaryParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, 2); // 2 channels
    audioAuxiliaryParams->PutIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    audioAuxiliaryParams->PutLongValue(OH_MD_KEY_BITRATE, 199000); // 199000 bit rate
    audioAuxiliaryParams->PutIntValue("audio_samples_per_frame", 1024); // 1024 frame size
    audioAuxiliaryParams->PutIntValue(OH_MD_KEY_PROFILE, AAC_PROFILE_LC);
    audioAuxiliaryParams->PutIntValue(OH_MD_KEY_AAC_IS_ADTS, 0);
    audioAuxiliaryParams->PutIntValue(OH_MD_KEY_TRACK_TYPE, static_cast<int32_t>(OH_MediaType::MEDIA_TYPE_AUXILIARY));
    audioAuxiliaryParams->PutStringValue(OH_MD_KEY_TRACK_REFERENCE_TYPE, TRACK_REF_TYPE_AUDIO);
    audioAuxiliaryParams->PutStringValue(OH_MD_KEY_TRACK_DESCRIPTION, AUXILIARY_AUDIO_TRACK_KEY);
    audioAuxiliaryParams->PutIntBuffer(OH_MD_KEY_REFERENCE_TRACK_IDS, trackIdsAudio, vAudio.size());

    ret = avmuxer_->AddTrack(trackIdAudio, audioAuxiliaryParams);
    ASSERT_EQ(ret, AV_ERR_OK);
    ASSERT_GE(trackIdAudio, 1);

    ASSERT_EQ(avmuxer_->Start(), 0);

    std::string inputFilePath = "/data/test/media/aac_2c_44100hz_199k_muxer.dat";
    TrackWriteSample(inputFilePath, trackId);
    TrackWriteSample(inputFilePath, trackIdAudio);

    ASSERT_EQ(avmuxer_->Stop(), 0);
}
#ifdef AVMUXER_UNITTEST_CAPI

/**
 * @tc.name: Muxer_GENRE_001
 * @tc.desc: Muxer add genre data test
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_GENRE_001, TestSize.Level1)
{
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_GENRE_001.mp4");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;
    ASSERT_TRUE(avmuxer_->CreateMuxer(fd_, outputFormat));
    std::shared_ptr<FormatMock> fileFormat = FormatMockFactory::CreateFormat();
    fileFormat->PutStringValue(OH_MD_KEY_GENRE, "test_genre_str");
    avmuxer_->SetFormat(fileFormat);

    std::shared_ptr<FormatMock> videoParams =
        FormatMockFactory::CreateVideoFormat(OH_AVCODEC_MIMETYPE_VIDEO_AVC, TEST_WIDTH, TEST_HEIGHT);
    videoParams->PutBuffer(OH_MD_KEY_CODEC_CONFIG, buffer_, sizeof(buffer_));
    int32_t videoTrackId = -1;
    int32_t ret = avmuxer_->AddTrack(videoTrackId, videoParams);
    ASSERT_EQ(ret, AV_ERR_OK);
    ASSERT_EQ(avmuxer_->Start(), 0);
    ASSERT_EQ(avmuxer_->Stop(), 0);
    ASSERT_EQ(avmuxer_->Destroy(), 0);

    struct stat fileStatus {};
    ASSERT_EQ(stat(outputFile.c_str(), &fileStatus), 0);
    int64_t fileSize = static_cast<int64_t>(fileStatus.st_size);
    OH_AVSource *source = OH_AVSource_CreateWithFD(fd_, 0, fileSize);
    OH_AVFormat *sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    const char *genreGet;
    ASSERT_NE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_GENRE, &genreGet), false);
    ASSERT_EQ(memcmp(genreGet, "test_genre_str", strlen(genreGet)), 0);
    OH_AVSource_Destroy(source);
}

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

/**
 * @tc.name: Muxer_SetFormat_CreationTime_001
 * @tc.desc: Muxer set format with valid creation time
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_SetFormat_CreationTime_001, TestSize.Level0)
{
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_SetFormat.mp4");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;
    
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_MPEG);
    audioParams->PutStringValue(OH_MD_KEY_CREATION_TIME, "2023-12-19T03:16:00.000000Z");
    int32_t ret = avmuxer_->SetFormat(audioParams);
    ASSERT_EQ(ret, 0);
}

/**
 * @tc.name: Muxer_SetFormat_CreationTime_002
 * @tc.desc: Muxer set format with invalid length
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_SetFormat_CreationTime_002, TestSize.Level0)
{
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_SetFormat.mp4");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;
    
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_MPEG);
    audioParams->PutStringValue(OH_MD_KEY_CREATION_TIME, "2023-12-19T03:16:00.00000000Z");
    int32_t ret = avmuxer_->SetFormat(audioParams);
    ASSERT_EQ(ret, 3);
}

/**
 * @tc.name: Muxer_SetFormat_CreationTime_003
 * @tc.desc: Muxer set format and fill several bits with char which should have been int
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_SetFormat_CreationTime_003, TestSize.Level0)
{
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_SetFormat.mp4");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;
    
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_MPEG);
    audioParams->PutStringValue(OH_MD_KEY_CREATION_TIME, "202a-12-19T03:16:0b.0000c0Z");
    int32_t ret = avmuxer_->SetFormat(audioParams);
    ASSERT_EQ(ret, 3);
}

/**
 * @tc.name: Muxer_SetFormat_CreationTime_004
 * @tc.desc: Muxer set format without valid key in Meta
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_SetFormat_CreationTime_004, TestSize.Level0)
{
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_SetFormat.mp4");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;
    
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_MPEG);
    int32_t ret = avmuxer_->SetFormat(audioParams);
    ASSERT_EQ(ret, 0);
}

/**
 * @tc.name: Muxer_SetFormat_DefinedKey_001
 * @tc.desc: Muxer set format with defined key(include MEDIA_CREATION_TIME)
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_SetFormat_DefinedKey_001, TestSize.Level0)
{
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_SetFormat.mp4");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;
    
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutIntValue(Tag::AUDIO_CHANNEL_COUNT, 2);
    audioParams->PutIntValue(Tag::AUDIO_SAMPLE_RATE, 48000);
    audioParams->PutStringValue(Tag::MEDIA_CREATION_TIME, "2023-12-19T03:16:00.000000Z");
    audioParams->PutLongValue(Tag::MEDIA_BITRATE, 128000);
    audioParams->PutFloatValue(Tag::MEDIA_LATITUDE, 22.67f);
    audioParams->PutFloatValue(Tag::MEDIA_LONGITUDE, 45.67f);
    int32_t ret = avmuxer_->SetFormat(audioParams);
    ASSERT_EQ(ret, 0);
}

/**
 * @tc.name: Muxer_SetFormat_DefinedKey_002
 * @tc.desc: Muxer set format with defined key(no include MEDIA_CREATION_TIME)
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_SetFormat_DefinedKey_002, TestSize.Level0)
{
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_SetFormat.mp4");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;
    
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutIntValue(Tag::AUDIO_CHANNEL_COUNT, 2);
    audioParams->PutIntValue(Tag::AUDIO_SAMPLE_RATE, 48000);
    audioParams->PutLongValue(Tag::MEDIA_BITRATE, 128000);
    audioParams->PutFloatValue(Tag::MEDIA_LATITUDE, 22.67f);
    audioParams->PutFloatValue(Tag::MEDIA_LONGITUDE, 45.67f);
    int32_t ret = avmuxer_->SetFormat(audioParams);
    ASSERT_EQ(ret, 0);
}

/**
 * @tc.name: Muxer_SetFormat_UserKey_001
 * @tc.desc: Muxer set format with user key(true keys)
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_SetFormat_UserKey_001, TestSize.Level0)
{
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_SetFormat.mp4");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;
    
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutIntValue("com.openharmony.version", 5);
    audioParams->PutStringValue("com.openharmony.model", "LNA-AL00");
    audioParams->PutFloatValue("com.openharmony.capture.fps", 30.00f);
    int32_t ret = avmuxer_->SetFormat(audioParams);
    ASSERT_EQ(ret, 0);
}

/**
 * @tc.name: Muxer_SetFormat_UserKey_002
 * @tc.desc: Muxer set format with user key(include wrong keys)
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_SetFormat_UserKey_002, TestSize.Level0)
{
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_SetFormat.mp4");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;
    
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutIntValue("com.openharmony.version", 5);
    audioParams->PutStringValue("com.openharmony.model", "LNA-AL00");
    audioParams->PutIntValue("com.openopen.version", 1);
    audioParams->PutFloatValue("com.openharmony.capture.fps", 30.00f);
    audioParams->PutFloatValue("capture.fps", 30.00f);
    int32_t ret = avmuxer_->SetFormat(audioParams);
    ASSERT_EQ(ret, 0);
}

/**
 * @tc.name: Muxer_SetFormat_UserKey_003
 * @tc.desc: Muxer set format with user key(include wrong keys and 2 times)
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_SetFormat_UserKey_003, TestSize.Level0)
{
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_SetFormat.mp4");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;
    
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutIntValue("com.openharmony.version", 5);
    audioParams->PutStringValue("com.openharmony.model", "LNA-AL00");
    audioParams->PutIntValue("com.openopen.version", 1);
    audioParams->PutFloatValue("com.openharmony.capture.fps", 30.00f);
    audioParams->PutFloatValue("capture.fps", 30.00f);
    int32_t ret = avmuxer_->SetFormat(audioParams);
    ASSERT_EQ(ret, 0);

    audioParams->PutIntValue("com.openharmony.version", 5);
    audioParams->PutStringValue("com.openharmony.model", "LNA-AL00");
    audioParams->PutFloatValue("com.openharmony.capture.fps", 30.00f);
    ret = avmuxer_->SetFormat(audioParams);
    ASSERT_EQ(ret, 0);
}

/**
 * @tc.name: Muxer_SetFormat_UserKey_004
 * @tc.desc: Muxer set format with user key(string keys length more than 256 characters)
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_SetFormat_UserKey_004, TestSize.Level0)
{
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_SetFormat.mp4");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;
    
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    std::string str = "";
    for (uint32_t i = 0; i < 260; ++i) {
        char ch = '0' + i % 10;
        str += ch;
    }
    audioParams->PutStringValue("com.openharmony.model", str);
    int ret = avmuxer_->SetFormat(audioParams);
    ASSERT_EQ(ret, 3);  // 3
}

/**
 * @tc.name: Muxer_SetFormat_UserKey_005
 * @tc.desc: Muxer set format with user key(string keys length less than 256 characters)
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_SetFormat_UserKey_005, TestSize.Level0)
{
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_SetFormat.mp4");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;
    
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    std::string str = "";
    for (uint32_t i = 0; i < 255; ++i) {
        char ch = '0' + i % 10;
        str += ch;
    }
    audioParams->PutStringValue("com.openharmony.model", str);
    int32_t ret = avmuxer_->SetFormat(audioParams);
    ASSERT_EQ(ret, 0);
}

/**
 * @tc.name: Muxer_SetFormat_UserKey_006
 * @tc.desc: Muxer set format with user key(include buffer)
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_SetFormat_UserKey_006, TestSize.Level0)
{
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_SetFormat.mp4");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;

    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    uint8_t testData[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    ASSERT_EQ(sizeof(testData), 10);
    audioParams->PutBuffer("com.openharmony.test", testData, sizeof(testData));

    int32_t ret = avmuxer_->SetFormat(audioParams);
    ASSERT_EQ(ret, 0);
}

/**
 * @tc.name: Muxer_SetFormat_UserKey_007
 * @tc.desc: Muxer set format with user key(more than max size)
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_SetFormat_UserKey_007, TestSize.Level0)
{
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_SetFormat.mp4");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;

    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();

    // more than max size
    const size_t maxSize = 1024 * 1024;
    uint8_t* testData = new uint8_t[maxSize + 10];
    bool retBool = audioParams->PutBuffer("com.openharmony.test", testData, maxSize + 10);
    ASSERT_EQ(retBool, false);
    delete[] testData;

    // max size
    const size_t maxSize2 = 1024 * 1024;
    uint8_t* testData2 = new uint8_t[maxSize2];
    bool retBool2 = audioParams->PutBuffer("com.openharmony.test", testData2, maxSize2);
    ASSERT_EQ(retBool2, true);
    delete[] testData2;

    int32_t ret = avmuxer_->SetFormat(audioParams);
    ASSERT_EQ(ret, 0);
}

/**
 * @tc.name: Muxer_SetFormat_UserKey_008
 * @tc.desc: Muxer set format with user key(complete)
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_SetFormat_UserKey_008, TestSize.Level0)
{
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_SetFormat.mp4");
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

    ASSERT_EQ(avmuxer_->Start(), 0);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    uint8_t testData[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    ASSERT_EQ(sizeof(testData), 10);
    audioParams->PutBuffer("com.openharmony.test", testData, sizeof(testData));

    ret = avmuxer_->SetFormat(audioParams);
    ASSERT_EQ(ret, 0);

    ASSERT_EQ(avmuxer_->Stop(), 0);
    ASSERT_EQ(avmuxer_->Destroy(), 0);
}

/**
 * @tc.name: Muxer_SetFormat_DefinedKey_And_UserKey_001
 * @tc.desc: Muxer set format with user key(true keys)
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_SetFormat_DefinedKey_And_UserKey_001, TestSize.Level0)
{
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_SetFormat.mp4");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;
    
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutIntValue("com.openharmony.version", 5);
    audioParams->PutLongValue(Tag::MEDIA_BITRATE, 128000);
    audioParams->PutStringValue("com.openharmony.model", "LNA-AL00");
    audioParams->PutStringValue(Tag::MEDIA_CREATION_TIME, "2023-12-19T03:16:00.000000Z");
    audioParams->PutFloatValue("com.openharmony.capture.fps", 30.00f);
    int32_t ret = avmuxer_->SetFormat(audioParams);
    ASSERT_EQ(ret, 0);
}

/**
 * @tc.name: Muxer_SetFormat_DefinedKey_And_UserKey_002
 * @tc.desc: Muxer set format with user key(include wrong value)
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_SetFormat_DefinedKey_And_UserKey_002, TestSize.Level0)
{
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_SetFormat.mp4");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;
    
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutIntValue("com.openharmony.version", 5);
    audioParams->PutLongValue(Tag::MEDIA_BITRATE, 128000);
    audioParams->PutStringValue("com.openharmony.model", "LNA-AL00");
    audioParams->PutStringValue(Tag::MEDIA_CREATION_TIME, "202a-12-19T03:16:0b.0000c0Z");
    audioParams->PutFloatValue("com.openharmony.capture.fps", 30.00f);
    int32_t ret = avmuxer_->SetFormat(audioParams);
    ASSERT_NE(ret, 0);
}

/**
 * @tc.name: Muxer_SetFormat_DefinedKey_And_UserKey_003
 * @tc.desc: Muxer set format with user key(include wrong keys)
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_SetFormat_DefinedKey_And_UserKey_003, TestSize.Level0)
{
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_SetFormat.mp4");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;
    
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutIntValue("com.openharny.version", 5);
    audioParams->PutLongValue(Tag::MEDIA_BITRATE, 128000);
    audioParams->PutStringValue("c.openharmony.model", "LNA-AL00");
    audioParams->PutStringValue(Tag::MEDIA_CREATION_TIME, "2023-12-19T03:16:00.000000Z");
    audioParams->PutFloatValue("com.openhar.capture.fps", 30.00f);
    int32_t ret = avmuxer_->SetFormat(audioParams);
    ASSERT_EQ(ret, 0);
}

/**
 * @tc.name: Muxer_SetFormat_Comment_001
 * @tc.desc: Muxer set format with defined key(include MEDIA_COMMENT)
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_SetFormat_Comment_001, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_SetFormat.mp4");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;
    
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutIntValue(Tag::AUDIO_CHANNEL_COUNT, 2);
    audioParams->PutIntValue(Tag::AUDIO_SAMPLE_RATE, 48000);
    audioParams->PutLongValue(Tag::MEDIA_BITRATE, 128000);
    audioParams->PutStringValue(Tag::MEDIA_COMMENT, "comment_test_str");
    int32_t ret = avmuxer_->SetFormat(audioParams);
    ASSERT_EQ(ret, 0);
    audioParams->InitAudioTrackFormat(Plugins::MimeType::AUDIO_MPEG, 48000, 2);
    ret = avmuxer_->AddTrack(trackId, audioParams);
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
 * @tc.name: Muxer_SetFormat_Comment_002
 * @tc.desc: Muxer set invalid comment with length 0 (less than 1)
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_SetFormat_Comment_002, TestSize.Level0)
{
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_SetFormat.mp4");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;
    
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutIntValue(Tag::AUDIO_CHANNEL_COUNT, 2);
    audioParams->PutIntValue(Tag::AUDIO_SAMPLE_RATE, 48000);
    audioParams->PutLongValue(Tag::MEDIA_BITRATE, 128000);
    audioParams->PutStringValue(Tag::MEDIA_COMMENT, "");
    int32_t ret = avmuxer_->SetFormat(audioParams);
    ASSERT_NE(ret, 0);
}

/**
 * @tc.name: Muxer_SetFormat_Comment_003
 * @tc.desc: Muxer set invalid comment with length 0 (longer than 256)
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_SetFormat_Comment_003, TestSize.Level0)
{
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_SetFormat.mp4");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;
    
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutIntValue(Tag::AUDIO_CHANNEL_COUNT, 2);
    audioParams->PutIntValue(Tag::AUDIO_SAMPLE_RATE, 48000);
    audioParams->PutLongValue(Tag::MEDIA_BITRATE, 128000);
    audioParams->PutStringValue(Tag::MEDIA_COMMENT, "thisisaninvalidcommentstringwhichistoolongthisisaninvalidcomment"
        "stringwhichistoolongthisisaninvalidcommentstringwhichistoolongthisisaninvalidcommentstringwhichistoolongthis"
        "isaninvalidcommentstringwhichistoolongthisisaninvalidcommentstringwhichistoolongthisisan");
    int32_t ret = avmuxer_->SetFormat(audioParams);
    ASSERT_NE(ret, 0);
}

/**
 * @tc.name: Muxer_SetFormat_IsMoovFront_001
 * @tc.desc: do not set moov in front
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_SetFormat_IsMoovFront_001, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_SetFormat_IsMoovFront_001.mp4");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;
    
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutIntValue(Tag::MEDIA_ENABLE_MOOV_FRONT, 0);
    int32_t ret = avmuxer_->SetFormat(audioParams);
    ASSERT_EQ(ret, 0);
    audioParams->InitAudioTrackFormat(Plugins::MimeType::AUDIO_MPEG, 48000, 2);
    ret = avmuxer_->AddTrack(trackId, audioParams);
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
 * @tc.name: Muxer_SetFormat_IsMoovFront_002
 * @tc.desc: set moov in front
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_SetFormat_IsMoovFront_002, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_SetFormat_IsMoovFront_001.mp4");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;
    
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutIntValue(Tag::MEDIA_ENABLE_MOOV_FRONT, 1);
    int32_t ret = avmuxer_->SetFormat(audioParams);
    ASSERT_EQ(ret, 0);
    audioParams->InitAudioTrackFormat(Plugins::MimeType::AUDIO_MPEG, 48000, 2);
    ret = avmuxer_->AddTrack(trackId, audioParams);
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
 * @tc.name: Muxer_SetLocation_001
 * @tc.desc: Muxer mp4 set location(latitude、longitude、altitude)
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_SetLocation_001, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_SetLocation_001.mp4");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;
    
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> metaData = FormatMockFactory::CreateFormat();
    metaData->PutFloatValue(OH_MD_KEY_LATITUDE, 90.0f);
    metaData->PutFloatValue(OH_MD_KEY_LONGITUDE, 180.0f);
    metaData->PutFloatValue(OH_MD_KEY_ALTITUDE, 123.4f);

    EXPECT_EQ(avmuxer_->SetFormat(metaData), 0);
    metaData->InitAudioTrackFormat(Plugins::MimeType::AUDIO_MPEG, 48000, 2);
    ASSERT_EQ(avmuxer_->AddTrack(trackId, metaData), 0);
    
    ASSERT_EQ(avmuxer_->Start(), 0);
    ASSERT_EQ(avmuxer_->Stop(), 0);
}

/**
 * @tc.name: Muxer_SetColorBoxInfo_001
 * @tc.desc: Muxer mp4 colorBox info(h265)
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_SetColorBoxInfo_001, TestSize.Level0)
{
    int32_t videoTrackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_SetColorBoxInfo_001.mp4");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    // 添加视频轨
    std::shared_ptr<FormatMock> videoParams = FormatMockFactory::CreateFormat();
    videoParams->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_VIDEO_HEVC);
    videoParams->PutIntValue(OH_MD_KEY_WIDTH, TEST_WIDTH);
    videoParams->PutIntValue(OH_MD_KEY_HEIGHT, TEST_HEIGHT);

    videoParams->PutIntValue(OH_MD_KEY_COLOR_PRIMARIES, COLOR_PRIMARY_BT2020); // primary
    videoParams->PutIntValue(OH_MD_KEY_TRANSFER_CHARACTERISTICS, TRANSFER_CHARACTERISTIC_UNSPECIFIED); // transfer
    videoParams->PutIntValue(OH_MD_KEY_MATRIX_COEFFICIENTS, MATRIX_COEFFICIENT_BT2020_NCL); // matrix
    videoParams->PutIntValue(OH_MD_KEY_RANGE_FLAG, 0); // range
    videoParams->PutIntValue(OH_MD_KEY_VIDEO_IS_HDR_VIVID, 1);
    ASSERT_EQ(avmuxer_->AddTrack(videoTrackId, videoParams), 0);

    // 添加metadata
    std::shared_ptr<FormatMock> metaData = FormatMockFactory::CreateFormat();
    metaData->PutStringValue(Tag::MEDIA_CREATION_TIME, "2025-08-14T00:01:02.000000Z");
    EXPECT_EQ(avmuxer_->SetFormat(metaData), 0);

    // start
    ASSERT_EQ(avmuxer_->Start(), 0);

    // video write sample
    TrackWriteSample(LOGINFO_INPUT_FILE_PATH, videoTrackId);

    ASSERT_EQ(avmuxer_->Stop(), 0);
}

/**
 * @tc.name: Muxer_SetColorBoxInfo_002
 * @tc.desc: Muxer mp4 colorBox info(h264)
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_SetColorBoxInfo_002, TestSize.Level0)
{
    int32_t videoTrackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_SetColorBoxInfo_002.mp4");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    // 添加视频轨
    std::shared_ptr<FormatMock> videoParams = FormatMockFactory::CreateFormat();
    videoParams->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    videoParams->PutIntValue(OH_MD_KEY_WIDTH, TEST_WIDTH);
    videoParams->PutIntValue(OH_MD_KEY_HEIGHT, TEST_HEIGHT);

    videoParams->PutIntValue(OH_MD_KEY_COLOR_PRIMARIES, COLOR_PRIMARY_BT2020); // primary
    videoParams->PutIntValue(OH_MD_KEY_TRANSFER_CHARACTERISTICS, TRANSFER_CHARACTERISTIC_UNSPECIFIED); // transfer
    videoParams->PutIntValue(OH_MD_KEY_MATRIX_COEFFICIENTS, MATRIX_COEFFICIENT_BT2020_NCL); // matrix
    videoParams->PutIntValue(OH_MD_KEY_RANGE_FLAG, 0); // range
    videoParams->PutIntValue(OH_MD_KEY_VIDEO_IS_HDR_VIVID, 1);
    ASSERT_EQ(avmuxer_->AddTrack(videoTrackId, videoParams), 0);

    // 添加metadata
    std::shared_ptr<FormatMock> metaData = FormatMockFactory::CreateFormat();
    metaData->PutStringValue(Tag::MEDIA_CREATION_TIME, "2025-08-14T00:01:02.000000Z");
    EXPECT_EQ(avmuxer_->SetFormat(metaData), 0);

    // start
    ASSERT_EQ(avmuxer_->Start(), 0);

    // video write sample
    TrackWriteSample(INPUT_FILE_PATH, videoTrackId);

    ASSERT_EQ(avmuxer_->Stop(), 0);
}

/**
 * @tc.name: Muxer_SetColorBoxInfo_003
 * @tc.desc: Muxer mp4 colorBox info(error)
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_SetColorBoxInfo_003, TestSize.Level0)
{
    int32_t videoTrackId = -1;
    int32_t videoTrackId1 = -1;
    int32_t videoTrackId2 = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_SetColorBoxInfo_003.mp4");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    // 添加视频轨
    std::shared_ptr<FormatMock> videoParams = FormatMockFactory::CreateFormat();
    videoParams->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_VIDEO_HEVC);
    videoParams->PutIntValue(OH_MD_KEY_WIDTH, TEST_WIDTH);
    videoParams->PutIntValue(OH_MD_KEY_HEIGHT, TEST_HEIGHT);
    videoParams->PutIntValue(OH_MD_KEY_COLOR_PRIMARIES, COLOR_PRIMARY_BT2020); // primary
    videoParams->PutIntValue(OH_MD_KEY_TRANSFER_CHARACTERISTICS, ABNORMAL_COLOR_VALUE); // transfer
    videoParams->PutIntValue(OH_MD_KEY_MATRIX_COEFFICIENTS, MATRIX_COEFFICIENT_BT2020_NCL); // matrix
    videoParams->PutIntValue(OH_MD_KEY_RANGE_FLAG, ABNORMAL_COLOR_VALUE); // range
    videoParams->PutIntValue(OH_MD_KEY_VIDEO_IS_HDR_VIVID, 1);
    ASSERT_NE(avmuxer_->AddTrack(videoTrackId, videoParams), 0);

    videoParams->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_VIDEO_HEVC);
    videoParams->PutIntValue(OH_MD_KEY_WIDTH, TEST_WIDTH);
    videoParams->PutIntValue(OH_MD_KEY_HEIGHT, TEST_HEIGHT);
    videoParams->PutIntValue(OH_MD_KEY_COLOR_PRIMARIES, COLOR_PRIMARY_BT2020); // primary
    videoParams->PutIntValue(OH_MD_KEY_TRANSFER_CHARACTERISTICS, TRANSFER_CHARACTERISTIC_UNSPECIFIED); // transfer
    videoParams->PutIntValue(OH_MD_KEY_MATRIX_COEFFICIENTS, MATRIX_COEFFICIENT_BT2020_NCL); // matrix
    videoParams->PutIntValue(OH_MD_KEY_RANGE_FLAG, 1); // range
    videoParams->PutIntValue(OH_MD_KEY_VIDEO_IS_HDR_VIVID, 1);
    ASSERT_EQ(avmuxer_->AddTrack(videoTrackId1, videoParams), 0);

    videoParams->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_VIDEO_HEVC);
    videoParams->PutIntValue(OH_MD_KEY_WIDTH, TEST_WIDTH);
    videoParams->PutIntValue(OH_MD_KEY_HEIGHT, TEST_HEIGHT);
    videoParams->PutIntValue(OH_MD_KEY_COLOR_PRIMARIES, ABNORMAL_COLOR_VALUE); // primary
    videoParams->PutIntValue(OH_MD_KEY_TRANSFER_CHARACTERISTICS, ABNORMAL_COLOR_VALUE); // transfer
    videoParams->PutIntValue(OH_MD_KEY_MATRIX_COEFFICIENTS, ABNORMAL_COLOR_VALUE); // matrix
    videoParams->PutIntValue(OH_MD_KEY_RANGE_FLAG, ABNORMAL_COLOR_VALUE); // range
    videoParams->PutIntValue(OH_MD_KEY_VIDEO_IS_HDR_VIVID, 1);
    ASSERT_NE(avmuxer_->AddTrack(videoTrackId2, videoParams), 0);

    // start
    ASSERT_EQ(avmuxer_->Start(), 0);

    // video write sample
    TrackWriteSample(LOGINFO_INPUT_FILE_PATH, videoTrackId1);

    ASSERT_EQ(avmuxer_->Stop(), 0);
}

/**
 * @tc.name: Muxer_SetColorBoxInfo_004
 * @tc.desc: Muxer mp4 colorBox info(error value)
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_SetColorBoxInfo_004, TestSize.Level0)
{
    int32_t videoTrackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_SetColorBoxInfo_004.mp4");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    // 添加视频轨
    std::shared_ptr<FormatMock> videoParams = FormatMockFactory::CreateFormat();
    videoParams->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_VIDEO_HEVC);
    videoParams->PutIntValue(OH_MD_KEY_WIDTH, TEST_WIDTH);
    videoParams->PutIntValue(OH_MD_KEY_HEIGHT, TEST_HEIGHT);

    EXPECT_EQ(videoParams->PutIntValue(OH_MD_KEY_COLOR_PRIMARIES, ABNORMAL_COLOR_VALUE), true); // primary
    EXPECT_NE(avmuxer_->AddTrack(videoTrackId, videoParams), 0);
    EXPECT_EQ(videoParams->PutIntValue(OH_MD_KEY_COLOR_PRIMARIES, ABNORMAL_COLOR_VALUE_1), true); // primary
    EXPECT_NE(avmuxer_->AddTrack(videoTrackId, videoParams), 0);
    EXPECT_EQ(videoParams->PutIntValue(OH_MD_KEY_COLOR_PRIMARIES, COLOR_PRIMARY_BT2020), true); // primary
    EXPECT_EQ(avmuxer_->AddTrack(videoTrackId, videoParams), 0);
    EXPECT_EQ(videoParams->PutIntValue(OH_MD_KEY_TRANSFER_CHARACTERISTICS, ABNORMAL_COLOR_VALUE), true); // transfer
    EXPECT_NE(avmuxer_->AddTrack(videoTrackId, videoParams), 0);
    EXPECT_EQ(videoParams->PutIntValue(OH_MD_KEY_TRANSFER_CHARACTERISTICS, ABNORMAL_COLOR_VALUE_1), true); // transfer
    EXPECT_NE(avmuxer_->AddTrack(videoTrackId, videoParams), 0);
    EXPECT_EQ(videoParams->PutIntValue(OH_MD_KEY_TRANSFER_CHARACTERISTICS, TRANSFER_CHARACTERISTIC_UNSPECIFIED), true);
    EXPECT_EQ(avmuxer_->AddTrack(videoTrackId, videoParams), 0);
    EXPECT_EQ(videoParams->PutIntValue(OH_MD_KEY_MATRIX_COEFFICIENTS, ABNORMAL_COLOR_VALUE), true); // matrix
    EXPECT_NE(avmuxer_->AddTrack(videoTrackId, videoParams), 0);
    EXPECT_EQ(videoParams->PutIntValue(OH_MD_KEY_MATRIX_COEFFICIENTS, ABNORMAL_COLOR_VALUE_1), true); // matrix
    EXPECT_NE(avmuxer_->AddTrack(videoTrackId, videoParams), 0);
    EXPECT_EQ(videoParams->PutIntValue(OH_MD_KEY_MATRIX_COEFFICIENTS, MATRIX_COEFFICIENT_BT2020_NCL), true); // matrix
    EXPECT_EQ(avmuxer_->AddTrack(videoTrackId, videoParams), 0);
    EXPECT_EQ(videoParams->PutIntValue(OH_MD_KEY_RANGE_FLAG, ABNORMAL_COLOR_VALUE), true); // range
    EXPECT_EQ(avmuxer_->AddTrack(videoTrackId, videoParams), 0);
    EXPECT_EQ(videoParams->PutIntValue(OH_MD_KEY_RANGE_FLAG, ABNORMAL_COLOR_VALUE_1), true); // range
    EXPECT_EQ(avmuxer_->AddTrack(videoTrackId, videoParams), 0);
    EXPECT_EQ(videoParams->PutIntValue(OH_MD_KEY_RANGE_FLAG, 1), true); // range
    EXPECT_EQ(avmuxer_->AddTrack(videoTrackId, videoParams), 0);
    EXPECT_EQ(videoParams->PutIntValue(OH_MD_KEY_RANGE_FLAG, 0), true); // range
    EXPECT_EQ(avmuxer_->AddTrack(videoTrackId, videoParams), 0);

    // start
    ASSERT_EQ(avmuxer_->Start(), 0);

    // video write sample
    TrackWriteSample(LOGINFO_INPUT_FILE_PATH, videoTrackId);

    ASSERT_EQ(avmuxer_->Stop(), 0);
}

/**
 * @tc.name: Muxer_SetColorBoxInfo_005
 * @tc.desc: Muxer mp4 colorBox info(include demuxer)
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_SetColorBoxInfo_005, TestSize.Level0)
{
    int32_t videoTrackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_SetColorBoxInfo_005.mp4");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    // 添加视频轨
    std::shared_ptr<FormatMock> videoParams = FormatMockFactory::CreateFormat();
    videoParams->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_VIDEO_HEVC);
    videoParams->PutIntValue(OH_MD_KEY_WIDTH, TEST_WIDTH);
    videoParams->PutIntValue(OH_MD_KEY_HEIGHT, TEST_HEIGHT);

    videoParams->PutIntValue(OH_MD_KEY_COLOR_PRIMARIES, COLOR_PRIMARY_BT2020); // primary
    videoParams->PutIntValue(OH_MD_KEY_TRANSFER_CHARACTERISTICS, TRANSFER_CHARACTERISTIC_UNSPECIFIED); // transfer
    videoParams->PutIntValue(OH_MD_KEY_MATRIX_COEFFICIENTS, MATRIX_COEFFICIENT_BT2020_NCL); // matrix
    videoParams->PutIntValue(OH_MD_KEY_RANGE_FLAG, 0); // range
    videoParams->PutIntValue(OH_MD_KEY_VIDEO_IS_HDR_VIVID, 1);
    ASSERT_EQ(avmuxer_->AddTrack(videoTrackId, videoParams), 0);

    // start
    ASSERT_EQ(avmuxer_->Start(), 0);

    // video write sample
    TrackWriteSample(LOGINFO_INPUT_FILE_PATH, videoTrackId);

    ASSERT_EQ(avmuxer_->Stop(), 0);

    struct stat fileStatus {};
    ASSERT_EQ(stat(outputFile.c_str(), &fileStatus), 0);
    int64_t fileSize = static_cast<int64_t>(fileStatus.st_size);
    OH_AVSource *source = OH_AVSource_CreateWithFD(fd_, 0, fileSize);
    OH_AVFormat *format = OH_AVSource_GetTrackFormat(source, videoTrackId);
    ASSERT_NE(format, nullptr);
    int32_t primaries = ABNORMAL_COLOR_VALUE_1;
    int32_t transfer = ABNORMAL_COLOR_VALUE_1;
    int32_t matrix = ABNORMAL_COLOR_VALUE_1;
    int32_t range = ABNORMAL_COLOR_VALUE_1;
    EXPECT_EQ(OH_AVFormat_GetIntValue(format, OH_MD_KEY_COLOR_PRIMARIES, &primaries), true);
    EXPECT_EQ(OH_AVFormat_GetIntValue(format, OH_MD_KEY_TRANSFER_CHARACTERISTICS, &transfer), true);
    EXPECT_EQ(OH_AVFormat_GetIntValue(format, OH_MD_KEY_MATRIX_COEFFICIENTS, &matrix), true);
    EXPECT_EQ(OH_AVFormat_GetIntValue(format, OH_MD_KEY_RANGE_FLAG, &range), true);
    ASSERT_EQ(primaries, COLOR_PRIMARY_BT2020);
    ASSERT_EQ(transfer, TRANSFER_CHARACTERISTIC_UNSPECIFIED);
    ASSERT_EQ(matrix, MATRIX_COEFFICIENT_BT2020_NCL);
    ASSERT_EQ(range, 0);
    OH_AVSource_Destroy(source);
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
    param->SetData("fast_start", static_cast<int32_t>(1)); // 1 moov 前置
    ret = avmuxer->SetParameter(param);
    EXPECT_EQ(ret, 0);

    std::shared_ptr<Meta> userMeta = std::make_shared<Meta>();
    userMeta->SetData("com.openharmony.version", 5); // 5 test version
    userMeta->SetData("com.openharmony.model", "LNA-AL00");
    userMeta->SetData("com.openharmony.manufacturer", "HW");
    userMeta->SetData("com.openharmony.marketing_name", "HW P60");
    userMeta->SetData("com.openharmony.capture.fps", 30.00f); // 30.00f test capture fps
    userMeta->SetData("model", "LNA-AL00");
    userMeta->SetData("com.openharmony.flag", true);
    ret = avmuxer->SetUserMeta(userMeta);
    EXPECT_EQ(ret, 0);

    close(fd);
}

/**
 * @tc.name: Muxer_AddTrack_TimeMeta
 * @tc.desc: Muxer AddTrack for timed metadata track
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_AddTrack_TimeMeta, TestSize.Level0)
{
    int32_t vidTrackId = -1;
    int32_t metaTrackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_AddTrack_TimeMeta.mp4");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> videoParams =
        FormatMockFactory::CreateVideoFormat(OH_AVCODEC_MIMETYPE_VIDEO_AVC, TEST_WIDTH, TEST_HEIGHT);

    ASSERT_EQ(avmuxer_->AddTrack(vidTrackId, videoParams), 0);
    ASSERT_GE(vidTrackId, 0);

    std::shared_ptr<FormatMock> metadataParams = FormatMockFactory::CreateTimedMetadataFormat(
        TIMED_METADATA_TRACK_MIMETYPE.c_str(), TIMED_METADATA_KEY, 1);
    ASSERT_NE(metadataParams, nullptr);

    ASSERT_NE(avmuxer_->AddTrack(metaTrackId, metadataParams), 0);

    ASSERT_NE(avmuxer_->AddTrack(metaTrackId, metadataParams), 0);

    std::shared_ptr<FormatMock> metadataParams2 = FormatMockFactory::CreateTimedMetadataFormat(
        TIMED_METADATA_TRACK_MIMETYPE.c_str(), TIMED_METADATA_KEY, -1);
    ASSERT_NE(metadataParams, nullptr);

    ASSERT_NE(avmuxer_->AddTrack(metaTrackId, metadataParams2), 0);
}

/**
 * @tc.name: Muxer_SetFlag_004
 * @tc.desc: Muxer Write Sample for timed metadata track
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_SetFlag_004, TestSize.Level0)
{
    int32_t vidTrackId = -1;
    int32_t metaTrackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_Timedmetadata_track.mp4");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> videoParams =
        FormatMockFactory::CreateVideoFormat(OH_AVCODEC_MIMETYPE_VIDEO_AVC, TEST_WIDTH, TEST_HEIGHT);

    ASSERT_EQ(avmuxer_->AddTrack(vidTrackId, videoParams), 0);
    ASSERT_GE(vidTrackId, 0);

    std::shared_ptr<FormatMock> metadataParams = FormatMockFactory::CreateTimedMetadataFormat(
        TIMED_METADATA_TRACK_MIMETYPE.c_str(), TIMED_METADATA_KEY, vidTrackId);
    ASSERT_NE(metadataParams, nullptr);

    ASSERT_EQ(avmuxer_->AddTrack(metaTrackId, metadataParams), 0);
    ASSERT_GE(metaTrackId, 1);

    int32_t ret = avmuxer_->SetTimedMetadata();
    EXPECT_EQ(ret, 0);

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
    ret = WriteSample(vidTrackId, inputFile_, eosFlag, flag);
    while (!eosFlag && (ret == 0)) {
        ret = WriteSample(vidTrackId, inputFile_, eosFlag, flag);
    }
    ASSERT_EQ(ret, 0);
    
    auto inputFileMeta = std::make_shared<std::ifstream>(INPUT_FILE_PATH, std::ios::binary);
    extSize = 0;
    inputFileMeta->read(reinterpret_cast<char*>(&extSize), sizeof(extSize));
    if (extSize > 0) {
        std::vector<uint8_t> buffer(extSize);
        inputFileMeta->read(reinterpret_cast<char*>(buffer.data()), extSize);
    }
    eosFlag = false;
    flag = AVCODEC_BUFFER_FLAGS_DISPOSABLE_EXT_TEST;
    ret = WriteSample(metaTrackId, inputFileMeta, eosFlag, flag);
    while (!eosFlag && (ret == 0)) {
        ret = WriteSample(metaTrackId, inputFileMeta, eosFlag, flag);
    }
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(avmuxer_->Stop(), 0);
}

/**
 * @tc.name: Muxer_MP4_001
 * @tc.desc: Muxer mux mp4 by h264
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_MP4_001, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_AVC.mp4");
    Plugins::OutputFormat outputFormat = Plugins::OutputFormat::MPEG_4;
    int32_t fd = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    std::shared_ptr<AVMuxer> avmuxer = AVMuxerFactory::CreateAVMuxer(fd, outputFormat);
    ASSERT_NE(avmuxer, nullptr);

    std::shared_ptr<Meta> videoParams = std::make_shared<Meta>();
    videoParams->Set<Tag::MIME_TYPE>(Plugins::MimeType::VIDEO_AVC);
    videoParams->Set<Tag::VIDEO_WIDTH>(TEST_WIDTH);
    videoParams->Set<Tag::VIDEO_HEIGHT>(TEST_HEIGHT);
    videoParams->Set<Tag::VIDEO_FRAME_RATE>(60.0); // 60.0 fps
    videoParams->Set<Tag::VIDEO_DELAY>(0);
    ASSERT_EQ(avmuxer->AddTrack(trackId, videoParams), 0);
    ASSERT_GE(trackId, 0);

    std::shared_ptr<Meta> param = std::make_shared<Meta>();
    param->Set<Tag::VIDEO_ROTATION>(Plugins::VideoRotation::VIDEO_ROTATION_90);
    param->Set<Tag::MEDIA_CREATION_TIME>("2023-12-19T03:16:00.000Z");
    param->Set<Tag::MEDIA_LATITUDE>(22.67f); // 22.67f test latitude
    param->Set<Tag::MEDIA_LONGITUDE>(114.06f); // 114.06f test longitude
    param->SetData("fast_start", static_cast<int32_t>(1)); // 1 moov 前置
    EXPECT_EQ(avmuxer->SetParameter(param), 0);
    OHOS::sptr<AVBufferQueueProducer> bqProducer= avmuxer->GetInputBufferQueue(trackId);
    ASSERT_NE(bqProducer, nullptr);

    ASSERT_EQ(avmuxer->Start(), 0);
    std::shared_ptr<Meta> userMeta = std::make_shared<Meta>();
    userMeta->SetData("com.openharmony.version", 5); // 5 test version
    userMeta->SetData("com.openharmony.model", "LNA-AL00");
    userMeta->SetData("com.openharmony.capture.fps", 30.00f); // 30.00f test capture fps
    EXPECT_EQ(avmuxer->SetUserMeta(userMeta), 0);

    inputFile_ = std::make_shared<std::ifstream>(INPUT_FILE_PATH, std::ios::binary);
    int32_t extSize = 0;
    inputFile_->read(reinterpret_cast<char*>(&extSize), sizeof(extSize));
    if (extSize > 0) {
        std::vector<uint8_t> buffer(extSize);
        inputFile_->read(reinterpret_cast<char*>(buffer.data()), extSize);
    }
    bool eosFlag = false;
    int32_t ret = 0;
    do {
        ret = WriteSample(bqProducer, inputFile_, eosFlag);
    } while (!eosFlag && (ret == 0));
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(avmuxer->Stop(), 0);
    close(fd);
}

/**
 * @tc.name: Muxer_MP4_AIGC_001
 * @tc.desc: Muxer mux mp4 include AIGC info
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_MP4_AIGC_001, TestSize.Level0)
{
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_MP4_AIGC_001.mp4");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> metaData = FormatMockFactory::CreateFormat();
    metaData->PutStringValue(Tag::MEDIA_AIGC, "AIGC_test_string");
    EXPECT_EQ(avmuxer_->SetFormat(metaData), 0);
}

/**
 * @tc.name: Muxer_MP4_AIGC_002
 * @tc.desc: Muxer mux mp4 include AIGC info
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerUnitTest, Muxer_MP4_AIGC_002, TestSize.Level0)
{
    int32_t audioTrackId = -1;
    int32_t videoTrackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_MP4_AIGC_002.mp4");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    // 添加音频轨
    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_AAC);
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, 44100); // 44100 sample rate
    audioParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, 2); // 2 channels
    audioParams->PutIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    audioParams->PutLongValue(OH_MD_KEY_BITRATE, 199000); // 199000 bit rate
    audioParams->PutIntValue("audio_samples_per_frame", 1024); // 1024 frame size
    audioParams->PutIntValue(OH_MD_KEY_PROFILE, AAC_PROFILE_LC);
    audioParams->PutIntValue(OH_MD_KEY_AAC_IS_ADTS, 0);
    ASSERT_EQ(avmuxer_->AddTrack(audioTrackId, audioParams), 0);

    // 添加视频轨
    std::shared_ptr<FormatMock> videoParams = FormatMockFactory::CreateFormat();
    videoParams->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    videoParams->PutIntValue(OH_MD_KEY_WIDTH, TEST_WIDTH);
    videoParams->PutIntValue(OH_MD_KEY_HEIGHT, TEST_HEIGHT);
    ASSERT_EQ(avmuxer_->AddTrack(videoTrackId, videoParams), 0);

    // 添加metadata
    std::shared_ptr<FormatMock> metaData = FormatMockFactory::CreateFormat();
    metaData->PutStringValue(Tag::MEDIA_CREATION_TIME, "2025-08-14T00:01:02.000000Z");
    metaData->PutStringValue(Tag::MEDIA_AIGC, "'{Label:value1}'");
    metaData->PutIntValue(Tag::MEDIA_ENABLE_MOOV_FRONT, static_cast<int32_t>(1)); // 1 moov 前置
    metaData->PutStringValue(Tag::MEDIA_COMMENT, "comment_test_str_metadata");
    EXPECT_EQ(avmuxer_->SetFormat(metaData), 0);

    // 添加userdata
    std::shared_ptr<FormatMock> userMeta = FormatMockFactory::CreateFormat();
    userMeta->PutIntValue("com.openharmony.version", 5); // 5 test version
    userMeta->PutStringValue("com.openharmony.model", "LNA-AL00");
    userMeta->PutFloatValue("com.openharmony.capture.fps", 30.00f); // 30.00f test capture fps
    EXPECT_EQ(avmuxer_->SetFormat(userMeta), 0);

    // start
    ASSERT_EQ(avmuxer_->Start(), 0);

    // audio write sample
    std::string inputFilePath = "/data/test/media/aac_44100_2.dat";
    TrackWriteSample(inputFilePath, audioTrackId);

    // video write sample
    TrackWriteSample(INPUT_FILE_PATH, videoTrackId);

    ASSERT_EQ(avmuxer_->Stop(), 0);
}
#endif
} // namespace