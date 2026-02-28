/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
#include <fstream>
#include <string>
#include <vector>
#include <fcntl.h>
#include "avmuxer_sample.h"
#include "avmuxer.h"
#include "native_avbuffer.h"
#include "native_audio_channel_layout.h"
#ifdef FFMPEG_MUXER_TEST
#include "avmuxer_ffmpeg_plugin_mock.h"
#else
#include "avmuxer_mpeg4_plugin_mock.h"
#endif
#include "native_avsource.h"
#include "demuxer_mock.h"


using namespace testing::ext;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::Media;
using namespace OHOS;
namespace {
#ifdef FFMPEG_MUXER_TEST
using UnitTestMuxerType = AVMuxerFfmpegPluginMock;
#else
using UnitTestMuxerType = AVMuxerMpeg4PluginMock;
#endif
constexpr int32_t TEST_CHANNEL_COUNT = 2;
constexpr int32_t TEST_SAMPLE_RATE = 44100;
constexpr int32_t TEST_WIDTH = 720;
constexpr int32_t TEST_HEIGHT = 480;
constexpr int32_t TEST_ROTATION = 90;
constexpr int32_t INVALID_FORMAT = -99;
const std::string MP4_MUXER_PLUGIN_NAME = "Mpeg4Mux_mp4";
const std::string TEST_FILE_PATH = "/data/test/media/";
const std::string INPUT_FILE_PATH = "/data/test/media/h264_720_480.dat";
const std::string LOGINFO_INPUT_FILE_PATH = "/data/test/media/h265_logInfo.dat";
const std::string INPUT_AUDIO_FILE_PATH = "/data/test/media/aac_2c_44100hz_199k_muxer.dat";
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

static int32_t WriteSampleUnitTest(int32_t trackId, std::shared_ptr<std::ifstream> file,
    bool &eosFlag, uint32_t flag, UnitTestMuxerType* avmuxer)
{
    if (avmuxer == nullptr) {
        return -1;
    }
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

    if (info.size == 0) {
        info.size = 1;
    }
    OH_AVBuffer *buffer = OH_AVBuffer_Create(info.size);
    file->read(reinterpret_cast<char*>(OH_AVBuffer_GetAddr(buffer)), info.size);
    OH_AVBuffer_SetBufferAttr(buffer, &info);
    int32_t ret = avmuxer->WriteSampleBuffer(trackId, buffer);
    OH_AVBuffer_Destroy(buffer);
    return ret;
}
} // namespace

class Mpeg4MuxerUnitTest : public testing::Test {
public:
    // SetUpTestCase: Called before all test cases
    static void SetUpTestCase(void);
    // TearDownTestCase: Called after all test case
    static void TearDownTestCase(void);
    // SetUp: Called before each test cases
    void SetUp(void);
    // TearDown: Called after each test cases
    void TearDown(void);

    int32_t WriteSample(int32_t trackId, std::shared_ptr<std::ifstream> file, bool &eosFlag, uint32_t flag);

    int32_t WriteSample(sptr<Media::AVBufferQueueProducer> bqProducer,
        std::shared_ptr<std::ifstream> file, bool &eosFlag);
    
    int32_t WriteSampleHaveBFrame(int32_t trackId, std::shared_ptr<std::ifstream> file,
        bool &eosFlag, uint32_t flag, int64_t pts);

    void TrackWriteSample(std::string inputFilePath, int32_t trackId);
    void Mpeg4GltfGenerateFile(std::string outputFile, std::shared_ptr<Meta> &param);

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
    uint8_t annexBuffer_[72] = {
        0x00, 0x00, 0x00, 0x01,
        0x40, 0x01, 0x0c, 0x01, 0xff, 0xff, 0x01, 0x60, 0x00, 0x00, 0x03, 0x00, 0x90, 0x00, 0x00, 0x03,
        0x00, 0x00, 0x03, 0x00, 0x5d, 0x95, 0x98, 0x09,
        0x00, 0x00, 0x00, 0x01,
        0x42, 0x01, 0x01, 0x01, 0x60, 0x00, 0x00, 0x03, 0x00, 0x90, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03,
        0x00, 0x5d, 0xa0, 0x05, 0xa2, 0x01, 0xe1, 0x65, 0x95, 0x9a, 0x49, 0x32, 0xb9, 0xa0, 0x20, 0x00,
        0x00, 0x03, 0x00, 0x20, 0x00, 0x00, 0x07, 0x81
    };
};

void Mpeg4MuxerUnitTest::SetUpTestCase() {}

void Mpeg4MuxerUnitTest::TearDownTestCase() {}

void Mpeg4MuxerUnitTest::SetUp()
{
    avmuxer_ = std::make_shared<AVMuxerSample>();
}

void Mpeg4MuxerUnitTest::TearDown()
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

int32_t Mpeg4MuxerUnitTest::WriteSample(int32_t trackId, std::shared_ptr<std::ifstream> file,
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

    if (info.size == 0) {
        info.size = 1;
    }
    OH_AVBuffer *buffer = OH_AVBuffer_Create(info.size);
    file->read(reinterpret_cast<char*>(OH_AVBuffer_GetAddr(buffer)), info.size);
    OH_AVBuffer_SetBufferAttr(buffer, &info);
    int32_t ret = avmuxer_->WriteSampleBuffer(trackId, buffer);
    OH_AVBuffer_Destroy(buffer);
    return ret;
}

int32_t Mpeg4MuxerUnitTest::WriteSampleHaveBFrame(int32_t trackId, std::shared_ptr<std::ifstream> file,
    bool &eosFlag, uint32_t flag, int64_t pts)
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

    info.pts = pts;
    if (info.size == 0) {
        info.size = 1;
    }
    OH_AVBuffer *buffer = OH_AVBuffer_Create(info.size);
    file->read(reinterpret_cast<char*>(OH_AVBuffer_GetAddr(buffer)), info.size);
    OH_AVBuffer_SetBufferAttr(buffer, &info);
    int32_t ret = 0;
    if (pts != -1) {
        avmuxer_->WriteSampleBuffer(trackId, buffer);
    }
    OH_AVBuffer_Destroy(buffer);
    return ret;
}

int32_t Mpeg4MuxerUnitTest::WriteSample(sptr<AVBufferQueueProducer> bqProducer,
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

void Mpeg4MuxerUnitTest::TrackWriteSample(std::string inputFilePath, int32_t trackId)
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


void Mpeg4MuxerUnitTest::Mpeg4GltfGenerateFile(std::string outputFile, std::shared_ptr<Meta> &param)
{
    int32_t trackId = -1;
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    std::shared_ptr<AVMuxerMock> avmuxerMock = AVMuxerMockFactory::CreateMuxer(fd_, AV_OUTPUT_FORMAT_MPEG_4);
    ASSERT_NE(avmuxerMock, nullptr);
    UnitTestMuxerType* avmuxer = reinterpret_cast<UnitTestMuxerType*>(avmuxerMock.get());

    ASSERT_EQ(avmuxer->SetParameter(param), 0);

    int32_t ret = avmuxer->AddTrack(trackId, param);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);
    ASSERT_EQ(avmuxer->Start(), 0);

    inputFile_ = std::make_shared<std::ifstream>(INPUT_AUDIO_FILE_PATH, std::ios::binary);

    int32_t extSize = 0;
    inputFile_->read(reinterpret_cast<char*>(&extSize), sizeof(extSize));
    if (extSize > 0) {
        std::vector<uint8_t> buffer(extSize);
        inputFile_->read(reinterpret_cast<char*>(buffer.data()), extSize);
    }

    bool eosFlag = false;
    uint32_t flag = AVCODEC_BUFFER_FLAGS_SYNC_FRAME;
    ret = WriteSampleUnitTest(trackId, inputFile_, eosFlag, flag, avmuxer);
    while (!eosFlag && (ret == 0)) {
        ret = WriteSampleUnitTest(trackId, inputFile_, eosFlag, flag, avmuxer);
    }
    EXPECT_EQ(ret, 0);
    ASSERT_EQ(avmuxer->Stop(), 0);
}

namespace {

/**
 * @tc.name: Muxer_Create_001
 * @tc.desc: Muxer Create
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_Create_001, TestSize.Level0)
{
    (void)INVALID_FORMAT;
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
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_Create_002, TestSize.Level0)
{
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_Create.mp4");
    fd_ = open(outputFile.c_str(), O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);
}

/**
 * @tc.name: Muxer_AddTrack_001
 * @tc.desc: Muxer AddTrack add audio track
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_AddTrack_001, TestSize.Level0)
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
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_AddTrack_002, TestSize.Level0)
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
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_AddTrack_003, TestSize.Level0)
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
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_AddTrack_004, TestSize.Level0)
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
 * @tc.name: Muxer_AddTrack_006
 * @tc.desc: Muxer AddTrack while create by unexpected value
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_AddTrack_006, TestSize.Level0)
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
 * @tc.desc: Create mp4, add acc 16000 and stop
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_AddTrack_007, TestSize.Level0)
{
    int32_t trackId = -2; // -2: Initialize to an invalid ID
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_AddTrack_007.mp4");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, static_cast<OH_AVOutputFormat>(AV_OUTPUT_FORMAT_MPEG_4));
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> avParam = FormatMockFactory::CreateFormat();
    avParam->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_AAC);
    avParam->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, 16000); // 16000: 16khz sample rate
    avParam->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, 1); // 1: 1 audio channel,mono

    int32_t ret = avmuxer_->AddTrack(trackId, avParam);
    ASSERT_EQ(ret, 0);
    ret = avmuxer_->Start();
    ASSERT_EQ(ret, 0);

    OH_AVCodecBufferAttr info;
    info.flags = 0;
    info.pts = 0;
    info.size = 5;  // test size 5
    OH_AVBuffer *buffer = OH_AVBuffer_Create(info.size);
    OH_AVBuffer_SetBufferAttr(buffer, &info);
    avmuxer_->WriteSampleBuffer(trackId, buffer);
    avmuxer_->WriteSampleBuffer(trackId, buffer);
    ret = avmuxer_->Stop();
    ASSERT_EQ(ret, 0);
}

/**
 * @tc.name: Muxer_AddTrack_008
 * @tc.desc: Create mp4, add mpeg 16000 and stop
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_AddTrack_008, TestSize.Level0)
{
    int32_t trackId = -2; // -2: Initialize to an invalid ID
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_AddTrack_008.mp4");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, static_cast<OH_AVOutputFormat>(AV_OUTPUT_FORMAT_MPEG_4));
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> avParam = FormatMockFactory::CreateFormat();
    avParam->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_MPEG);
    avParam->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, 24000); // 24000: 24khz sample rate
    avParam->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, 1); // 1: 1 audio channel,mono

    int32_t ret = avmuxer_->AddTrack(trackId, avParam);
    ASSERT_EQ(ret, 0);
    ret = avmuxer_->Start();
    ASSERT_EQ(ret, 0);

    OH_AVCodecBufferAttr info;
    info.flags = 0;
    info.pts = 0;
    info.size = 5;  // test size 5
    OH_AVBuffer *buffer = OH_AVBuffer_Create(info.size);
    OH_AVBuffer_SetBufferAttr(buffer, &info);
    avmuxer_->WriteSampleBuffer(trackId, buffer);
    avmuxer_->WriteSampleBuffer(trackId, buffer);
    ret = avmuxer_->Stop();
    ASSERT_EQ(ret, 0);
}

/**
 * @tc.name: Muxer_AddTrack_009
 * @tc.desc: Create mp4, add mpeg 44100 and stop
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_AddTrack_009, TestSize.Level0)
{
    int32_t trackId = -2; // -2: Initialize to an invalid ID
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_AddTrack_009.mp4");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, static_cast<OH_AVOutputFormat>(AV_OUTPUT_FORMAT_MPEG_4));
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> avParam = FormatMockFactory::CreateFormat();
    avParam->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_MPEG);
    avParam->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, 44100); // 44100: 44.1khz sample rate
    avParam->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, 1); // 1: 1 audio channel,mono

    int32_t ret = avmuxer_->AddTrack(trackId, avParam);
    ASSERT_EQ(ret, 0);
    ret = avmuxer_->Start();
    ASSERT_EQ(ret, 0);

    OH_AVCodecBufferAttr info;
    info.flags = 0;
    info.pts = 0;
    info.size = 5;  // test size 5
    OH_AVBuffer *buffer = OH_AVBuffer_Create(info.size);
    OH_AVBuffer_SetBufferAttr(buffer, &info);
    avmuxer_->WriteSampleBuffer(trackId, buffer);
    avmuxer_->WriteSampleBuffer(trackId, buffer);
    ret = avmuxer_->Stop();
    ASSERT_EQ(ret, 0);
}

/**
 * @tc.name: Muxer_Start_001
 * @tc.desc: Muxer Start normal
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_Start_001, TestSize.Level0)
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
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_Start_002, TestSize.Level0)
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
 * @tc.name: Muxer_Stop_001
 * @tc.desc: Muxer Stop() normal
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_Stop_001, TestSize.Level0)
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
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_Stop_002, TestSize.Level0)
{
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_Stop.mp4");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);
    EXPECT_NE(avmuxer_->Stop(), 0);
}

/**
 * @tc.name: Muxer_writeSample_001
 * @tc.desc: Muxer Write Sample normal
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_writeSample_001, TestSize.Level0)
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
    info.flags = AVCODEC_BUFFER_FLAGS_CODEC_DATA;
    info.pts = 0;
    info.offset = 0;
    info.size = sizeof(annexBuffer_);
    ret = avmuxer_->WriteSample(trackId, annexBuffer_, info);
    ASSERT_EQ(ret, 0);
}

/**
 * @tc.name: Muxer_writeSample_003
 * @tc.desc: Muxer Write Sample without Start()
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_writeSample_003, TestSize.Level0)
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
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_writeSample_004, TestSize.Level0)
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
 * @tc.name: Muxer_SetRotation_001
 * @tc.desc: Muxer SetRotation after Create
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_SetRotation_001, TestSize.Level0)
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
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_SetRotation_002, TestSize.Level0)
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
 * @tc.name: Muxer_SetRotation_007
 * @tc.desc: Muxer SetRotation expected value
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_SetRotation_007, TestSize.Level0)
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
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_SetRotation_008, TestSize.Level0)
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
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_Hevc_AddTrack_001, TestSize.Level0)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        std::cout << "the hevc of mimetype is not supported" << std::endl;
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
}

/**
 * @tc.name: Muxer_Hevc_WriteSample_001
 * @tc.desc: Muxer Hevc Write Sample normal
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_Hevc_WriteSample_001, TestSize.Level0)
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
    videoParams->PutDoubleValue(OH_MD_KEY_FRAME_RATE, 60.0);

    int32_t ret = avmuxer_->AddTrack(trackId, videoParams);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);
    ASSERT_EQ(avmuxer_->Start(), 0);

    OH_AVCodecBufferAttr info;
    info.flags = AVCODEC_BUFFER_FLAGS_CODEC_DATA;
    info.pts = 0;
    info.size = sizeof(annexBuffer_);
    OH_AVBuffer *buffer = OH_AVBuffer_Create(info.size);
    ASSERT_EQ(memcpy_s(OH_AVBuffer_GetAddr(buffer), info.size, annexBuffer_, sizeof(annexBuffer_)), 0);
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
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_Hevc_WriteSample_002, TestSize.Level0)
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
    videoParams->PutDoubleValue(OH_MD_KEY_FRAME_RATE, 60.0);

    int32_t ret = avmuxer_->AddTrack(trackId, videoParams);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);
    ASSERT_EQ(avmuxer_->Start(), 0);

    OH_AVCodecBufferAttr info;
    info.flags = AVCODEC_BUFFER_FLAGS_CODEC_DATA;
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
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_Hevc_WriteSample_003, TestSize.Level0)
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
    videoParams->PutDoubleValue(OH_MD_KEY_FRAME_RATE, 60.0);

    int32_t ret = avmuxer_->AddTrack(trackId, videoParams);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);
    ASSERT_EQ(avmuxer_->Start(), 0);

    OH_AVCodecBufferAttr info;
    info.flags = AVCODEC_BUFFER_FLAGS_CODEC_DATA;
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
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_Hevc_WriteSample_004, TestSize.Level0)
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
    ASSERT_NE(ret, 0);

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
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_Hevc_WriteSample_005, TestSize.Level0)
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
    EXPECT_EQ(avmuxer_->Stop(), 0);

    std::shared_ptr<DemuxerMock> demuxer = std::make_shared<DemuxerMock>();
    demuxer->Start(outputFile);
    ASSERT_NE(demuxer->GetUserMetaFormat(), nullptr);
    const char* logInfo = nullptr;
    OH_AVFormat_GetStringValue(demuxer->GetUserMetaFormat(), "com.openharmony.video.sei.h_log", &logInfo);
    ASSERT_NE(logInfo, nullptr);
}

/**
 * @tc.name: Muxer_Hevc_WriteSample_006
 * @tc.desc: Muxer Hevc Write Sample include sei logInfo
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_Hevc_WriteSample_006, TestSize.Level0)
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
    videoParams->PutIntValue(OH_MD_KEY_COLOR_PRIMARIES, 1);
    videoParams->PutIntValue(OH_MD_KEY_TRANSFER_CHARACTERISTICS, 1);
    videoParams->PutIntValue(OH_MD_KEY_MATRIX_COEFFICIENTS, 1);
    videoParams->PutIntValue(OH_MD_KEY_VIDEO_IS_HDR_VIVID, 1);
    videoParams->PutLongValue(OH_MD_KEY_BITRATE, 30000);

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
    EXPECT_EQ(avmuxer_->Stop(), 0);
}

/**
 * @tc.name: Muxer_Hevc_Optimize_Filler_Data_001
 * @tc.desc: Muxer Hevc Optimize Filler Data
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_Hevc_Optimize_Filler_Data_001, TestSize.Level0)
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
        ASSERT_EQ(avmuxer_->Stop(), 0);

        OH_AVCodecBufferAttr info;
        OH_AVBuffer *buffer = OH_AVBuffer_Create(409600);  // 409600
        std::shared_ptr<DemuxerMock> demuxer = std::make_shared<DemuxerMock>();
        demuxer->Start(outputFile);
        ASSERT_EQ(demuxer->GetVideoFrame(buffer, &info), 0);
        ASSERT_EQ(demuxer->GetVideoFrame(buffer, &info), 0);
        ASSERT_EQ(info.size, 110758);  // 110758
        ASSERT_EQ(demuxer->GetVideoFrame(buffer, &info), 0);
        ASSERT_EQ(info.size, 151047);  // 151047
        OH_AVBuffer_Destroy(buffer);
    }
}

/**
 * @tc.name: Muxer_SetFlag_001
 * @tc.desc: Muxer Write Sample flags AVCODEC_BUFFER_FLAGS_DISPOSABLE
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_SetFlag_001, TestSize.Level0)
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
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_SetFlag_002, TestSize.Level0)
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
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_SetFlag_003, TestSize.Level0)
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
 * @tc.name: Muxer_AddTrack_Auxiliary_001
 * @tc.desc: Muxer AddTrack video Auxiliary track.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_AddTrack_Auxiliary_001, TestSize.Level0) {
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
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_AddTrack_Auxiliary_002, TestSize.Level0) {
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
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_AddTrack_Auxiliary_003, TestSize.Level0) {
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
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_AddTrack_Auxiliary_004, TestSize.Level0) {
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
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_AddTrack_Auxiliary_005, TestSize.Level0) {
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
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_Add_Video_Auxiliary, TestSize.Level0) {
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
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_Add_Audio_Auxiliary, TestSize.Level0) {
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

/**
 * @tc.name: Muxer_GENRE_001
 * @tc.desc: Muxer add genre data test
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_GENRE_001, TestSize.Level1)
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
    OH_AVCodecBufferAttr info;
    info.flags = AVCODEC_BUFFER_FLAGS_CODEC_DATA;
    info.pts = 0;
    info.offset = 0;
    info.size = sizeof(annexBuffer_);
    ret = avmuxer_->WriteSample(videoTrackId, annexBuffer_, info);
    ASSERT_EQ(ret, 0);
    info.flags = AVCODEC_BUFFER_FLAGS_SYNC_FRAME;
    ret = avmuxer_->WriteSample(videoTrackId, annexBuffer_, info);
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
 * @tc.name: Muxer_SetFormat_CreationTime_001
 * @tc.desc: Muxer set format with valid creation time
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_SetFormat_CreationTime_001, TestSize.Level0)
{
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_SetFormat.mp4");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;
    
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_MPEG);
    audioParams->PutStringValue(OH_MD_KEY_CREATION_TIME, "2023-12-19T03:16:00.000000Z");
    ASSERT_EQ(avmuxer_->SetFormat(audioParams), 0);
    avmuxer_->Start();
    avmuxer_->Stop();
}

/**
 * @tc.name: Muxer_SetFormat_CreationTime_002
 * @tc.desc: Muxer set format with invalid length
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_SetFormat_CreationTime_002, TestSize.Level0)
{
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_SetFormat.mp4");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;
    
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_MPEG);
    audioParams->PutStringValue(OH_MD_KEY_CREATION_TIME, "2023-12-19T03:16:00.00000000Z");
    ASSERT_EQ(avmuxer_->SetFormat(audioParams), 0);
    avmuxer_->Start();
    avmuxer_->Stop();
}

/**
 * @tc.name: Muxer_SetFormat_CreationTime_003
 * @tc.desc: Muxer set format and fill several bits with char which should have been int
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_SetFormat_CreationTime_003, TestSize.Level0)
{
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_SetFormat.mp4");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;
    
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_MPEG);
    audioParams->PutStringValue(OH_MD_KEY_CREATION_TIME, "202a-12-19T03:16:0b.0000c0Z");
    ASSERT_EQ(avmuxer_->SetFormat(audioParams), 0);
    avmuxer_->Start();
    avmuxer_->Stop();
}

/**
 * @tc.name: Muxer_SetFormat_CreationTime_004
 * @tc.desc: Muxer set format without valid key in Meta
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_SetFormat_CreationTime_004, TestSize.Level0)
{
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_SetFormat.mp4");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;
    
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_MPEG);
    ASSERT_EQ(avmuxer_->SetFormat(audioParams), 0);
    avmuxer_->Start();
    avmuxer_->Stop();
}

/**
 * @tc.name: Muxer_SetFormat_DefinedKey_001
 * @tc.desc: Muxer set format with defined key(include MEDIA_CREATION_TIME)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_SetFormat_DefinedKey_001, TestSize.Level0)
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
    audioParams->PutFloatValue(Tag::MEDIA_LONGITUDE, 114.06f);
    ASSERT_EQ(avmuxer_->SetFormat(audioParams), 0);
    avmuxer_->Start();
    avmuxer_->Stop();
}

/**
 * @tc.name: Muxer_SetFormat_DefinedKey_002
 * @tc.desc: Muxer set format with defined key(no include MEDIA_CREATION_TIME)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_SetFormat_DefinedKey_002, TestSize.Level0)
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
    audioParams->PutFloatValue(Tag::MEDIA_LONGITUDE, 114.06f);
    ASSERT_EQ(avmuxer_->SetFormat(audioParams), 0);
    avmuxer_->Start();
    avmuxer_->Stop();
}

/**
 * @tc.name: Muxer_SetFormat_UserKey_001
 * @tc.desc: Muxer set format with user key(true keys)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_SetFormat_UserKey_001, TestSize.Level0)
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
    ASSERT_EQ(avmuxer_->SetFormat(audioParams), 0);
    avmuxer_->Start();
    avmuxer_->Stop();
}

/**
 * @tc.name: Muxer_SetFormat_UserKey_002
 * @tc.desc: Muxer set format with user key(include wrong keys)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_SetFormat_UserKey_002, TestSize.Level0)
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
    ASSERT_EQ(avmuxer_->SetFormat(audioParams), 0);
    avmuxer_->Start();
    avmuxer_->Stop();
}

/**
 * @tc.name: Muxer_SetFormat_UserKey_003
 * @tc.desc: Muxer set format with user key(include wrong keys and 2 times)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_SetFormat_UserKey_003, TestSize.Level0)
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

    avmuxer_->Start();
    avmuxer_->Stop();
}

/**
 * @tc.name: Muxer_SetFormat_UserKey_004
 * @tc.desc: Muxer set format with user key(string keys length more than 256 characters)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_SetFormat_UserKey_004, TestSize.Level0)
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
    int32_t ret = avmuxer_->SetFormat(audioParams);
    ASSERT_EQ(ret, 0);
    avmuxer_->Start();
    avmuxer_->Stop();
}

/**
 * @tc.name: Muxer_SetFormat_UserKey_005
 * @tc.desc: Muxer set format with user key(string keys length less than 256 characters)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_SetFormat_UserKey_005, TestSize.Level0)
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
    avmuxer_->Start();
    avmuxer_->Stop();
}

/**
 * @tc.name: Muxer_SetFormat_UserKey_006
 * @tc.desc: Muxer set format with user key(include buffer)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_SetFormat_UserKey_006, TestSize.Level0)
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
    avmuxer_->Start();
    avmuxer_->Stop();
}

/**
 * @tc.name: Muxer_SetFormat_UserKey_007
 * @tc.desc: Muxer set format with user key(more than max size)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_SetFormat_UserKey_007, TestSize.Level0)
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
    avmuxer_->Start();
    avmuxer_->Stop();
}

/**
 * @tc.name: Muxer_SetFormat_UserKey_008
 * @tc.desc: Muxer set format with user key(complete)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_SetFormat_UserKey_008, TestSize.Level0)
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
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_SetFormat_DefinedKey_And_UserKey_001, TestSize.Level0)
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
    avmuxer_->Start();
    avmuxer_->Stop();
}

/**
 * @tc.name: Muxer_SetFormat_DefinedKey_And_UserKey_002
 * @tc.desc: Muxer set format with user key(include wrong value)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_SetFormat_DefinedKey_And_UserKey_002, TestSize.Level0)
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
    ASSERT_EQ(ret, 0);
    avmuxer_->Start();
    avmuxer_->Stop();
}

/**
 * @tc.name: Muxer_SetFormat_DefinedKey_And_UserKey_003
 * @tc.desc: Muxer set format with user key(include wrong keys)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_SetFormat_DefinedKey_And_UserKey_003, TestSize.Level0)
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
    avmuxer_->Start();
    avmuxer_->Stop();
}

/**
 * @tc.name: Muxer_SetFormat_Comment_001
 * @tc.desc: Muxer set format with defined key(include MEDIA_COMMENT)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_SetFormat_Comment_001, TestSize.Level0)
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
    avmuxer_->Start();
    avmuxer_->Stop();
}

/**
 * @tc.name: Muxer_SetFormat_Comment_002
 * @tc.desc: Muxer set invalid comment with length 0 (less than 1)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_SetFormat_Comment_002, TestSize.Level0)
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
    ASSERT_EQ(ret, 0);
    avmuxer_->Start();
    avmuxer_->Stop();
}

/**
 * @tc.name: Muxer_SetFormat_IsMoovFront_001
 * @tc.desc: do not set moov in front
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_SetFormat_IsMoovFront_001, TestSize.Level0)
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
    ASSERT_EQ(avmuxer_->Stop(), 0);
}

/**
 * @tc.name: Muxer_SetFormat_IsMoovFront_002
 * @tc.desc: set moov in front
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_SetFormat_IsMoovFront_002, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_SetFormat_IsMoovFront_002.mp4");
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
    ASSERT_EQ(avmuxer_->Stop(), 0);
}

/**
 * @tc.name: Muxer_SetParameter_001
 * @tc.desc: Muxer SetParameter after Create
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_SetParameter_001, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_SetParameter.mp4");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    std::shared_ptr<AVMuxerMock> avmuxerMock = AVMuxerMockFactory::CreateMuxer(fd_, AV_OUTPUT_FORMAT_MPEG_4);
    ASSERT_NE(avmuxerMock, nullptr);
    UnitTestMuxerType* avmuxer = reinterpret_cast<UnitTestMuxerType*>(avmuxerMock.get());

    std::shared_ptr<Meta> videoParams = std::make_shared<Meta>();
    videoParams->Set<Tag::MIME_TYPE>(Plugins::MimeType::VIDEO_AVC);
    videoParams->Set<Tag::VIDEO_WIDTH>(TEST_WIDTH);
    videoParams->Set<Tag::VIDEO_HEIGHT>(TEST_HEIGHT);

    int32_t ret = avmuxer->AddTrack(trackId, videoParams);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);

    std::shared_ptr<Meta> param = std::make_shared<Meta>();
    param->Set<Tag::VIDEO_ROTATION>(Plugins::VideoRotation::VIDEO_ROTATION_0);
    param->Set<Tag::MEDIA_CREATION_TIME>("03:16:00.000");
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

    ASSERT_EQ(avmuxer->Start(), 0);
    ASSERT_EQ(avmuxer->Stop(), 0);
}

/**
 * @tc.name: Muxer_SetParameter_002
 * @tc.desc: Muxer SetParameter after Create
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_SetParameter_002, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_SetParameter.mp4");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    std::shared_ptr<AVMuxerMock> avmuxerMock = AVMuxerMockFactory::CreateMuxer(fd_, AV_OUTPUT_FORMAT_MPEG_4);
    ASSERT_NE(avmuxerMock, nullptr);
    UnitTestMuxerType* avmuxer = reinterpret_cast<UnitTestMuxerType*>(avmuxerMock.get());

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

    ASSERT_EQ(avmuxer->Start(), 0);
    ASSERT_EQ(avmuxer->Stop(), 0);
}

/**
 * @tc.name: Muxer_SetParameter_003
 * @tc.desc: Muxer SetParameter after Create
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_SetParameter_003, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_SetParameter.mp4");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    std::shared_ptr<AVMuxerMock> avmuxerMock = AVMuxerMockFactory::CreateMuxer(fd_, AV_OUTPUT_FORMAT_MPEG_4);
    ASSERT_NE(avmuxerMock, nullptr);
    UnitTestMuxerType* avmuxer = reinterpret_cast<UnitTestMuxerType*>(avmuxerMock.get());

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

    ASSERT_EQ(avmuxer->Start(), 0);
    ASSERT_EQ(avmuxer->Stop(), 0);
}

/**
 * @tc.name: Muxer_SetParameter_004
 * @tc.desc: Muxer SetParameter after Create(include altitude)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_SetParameter_004, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_SetParameter.mp4");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    std::shared_ptr<AVMuxerMock> avmuxerMock = AVMuxerMockFactory::CreateMuxer(fd_, AV_OUTPUT_FORMAT_MPEG_4);
    ASSERT_NE(avmuxerMock, nullptr);
    UnitTestMuxerType* avmuxer = reinterpret_cast<UnitTestMuxerType*>(avmuxerMock.get());

    std::shared_ptr<Meta> videoParams = std::make_shared<Meta>();
    videoParams->Set<Tag::MIME_TYPE>(Plugins::MimeType::VIDEO_AVC);
    videoParams->Set<Tag::VIDEO_WIDTH>(TEST_WIDTH);
    videoParams->Set<Tag::VIDEO_HEIGHT>(TEST_HEIGHT);

    int32_t ret = avmuxer->AddTrack(trackId, videoParams);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);

    std::shared_ptr<Meta> param = std::make_shared<Meta>();
    param->Set<Tag::MEDIA_LATITUDE>(90.0f); // 90.0f test latitude
    param->Set<Tag::MEDIA_LONGITUDE>(180.0f); // 180.0f test longitude
    param->Set<Tag::MEDIA_ALTITUDE>(100.0f); // 100.0f test altitude
    ret = avmuxer->SetParameter(param);
    EXPECT_EQ(ret, 0);

    param->Set<Tag::MEDIA_ALTITUDE>(-300000.0f); // -300000.0f test altitude
    ret = avmuxer->SetParameter(param);
    EXPECT_EQ(ret, 0);

    param->Set<Tag::MEDIA_ALTITUDE>(0.0f); // 0.0f test altitude
    ret = avmuxer->SetParameter(param);
    EXPECT_EQ(ret, 0);

    ASSERT_EQ(avmuxer->Start(), 0);
    ASSERT_EQ(avmuxer->Stop(), 0);
}

/**
 * @tc.name: Muxer_SetUserMeta_001
 * @tc.desc: Muxer SetUserMeta after Create
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_SetUserMeta_001, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_SetUserMeta.mp4");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    std::shared_ptr<AVMuxerMock> avmuxerMock = AVMuxerMockFactory::CreateMuxer(fd_, AV_OUTPUT_FORMAT_MPEG_4);
    ASSERT_NE(avmuxerMock, nullptr);
    UnitTestMuxerType* avmuxer = reinterpret_cast<UnitTestMuxerType*>(avmuxerMock.get());

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

    ASSERT_EQ(avmuxer->Start(), 0);
    ASSERT_EQ(avmuxer->Stop(), 0);
}

/**
 * @tc.name: Muxer_SetMeta_001
 * @tc.desc: Muxer Mp4 set meta(latitude、longitude、altitude)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_SetMeta_001, TestSize.Level0)
{
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_SetMeta_001.mp4");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    // 添加metadata
    std::shared_ptr<FormatMock> metaData = FormatMockFactory::CreateFormat();
    metaData->PutFloatValue(OH_MD_KEY_LATITUDE, 90.0f);
    metaData->PutFloatValue(OH_MD_KEY_LONGITUDE, 180.0f);
    metaData->PutFloatValue(OH_MD_KEY_ALTITUDE, 100.0f);

    EXPECT_EQ(avmuxer->SetFormat(metaData), 0);

    ASSERT_EQ(avmuxer->Start(), 0);
    ASSERT_EQ(avmuxer->Stop(), 0);
}

/**
 * @tc.name: Muxer_SetMeta_002
 * @tc.desc: Muxer Mp4 set meta(latitude、longitude、altitude) include usermeta
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_SetMeta_002, TestSize.Level0)
{
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_SetMeta_002.mp4");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    // 添加metadata
    std::shared_ptr<FormatMock> metaData = FormatMockFactory::CreateFormat();
    metaData->PutFloatValue(OH_MD_KEY_LATITUDE, 90.0f);
    metaData->PutFloatValue(OH_MD_KEY_LONGITUDE, 180.0f);
    metaData->PutFloatValue(OH_MD_KEY_ALTITUDE, 102.0f);
    metaData->PutIntValue("com.openharmony.version", 5);

    EXPECT_EQ(avmuxer->SetFormat(metaData), 0);

    ASSERT_EQ(avmuxer->Start(), 0);
    ASSERT_EQ(avmuxer->Stop(), 0);
}

/**
 * @tc.name: Muxer_SetMeta_003
 * @tc.desc: Muxer Mp4 set meta(latitude、longitude、altitude = 0) include usermeta
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_SetMeta_003, TestSize.Level0)
{
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_SetMeta_003.mp4");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    // 添加metadata
    std::shared_ptr<FormatMock> metaData = FormatMockFactory::CreateFormat();
    metaData->PutFloatValue(OH_MD_KEY_LATITUDE, 90.0f);
    metaData->PutFloatValue(OH_MD_KEY_LONGITUDE, 180.0f);
    metaData->PutFloatValue(OH_MD_KEY_ALTITUDE, 0.0f);
    metaData->PutIntValue("com.openharmony.version", 5);

    EXPECT_EQ(avmuxer->SetFormat(metaData), 0);

    ASSERT_EQ(avmuxer->Start(), 0);
    ASSERT_EQ(avmuxer->Stop(), 0);
}

/**
 * @tc.name: Muxer_AddTrack_TimeMeta
 * @tc.desc: Muxer AddTrack for timed metadata track
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_AddTrack_TimeMeta, TestSize.Level0)
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
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_SetFlag_004, TestSize.Level0)
{
    int32_t vidTrackId = -1;
    int32_t metaTrackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_Timedmetadata_track.mp4");

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, AV_OUTPUT_FORMAT_MPEG_4);
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
    uint32_t flag = 0;
    ret = WriteSample(vidTrackId, inputFile_, eosFlag, flag);
    flag = AVCODEC_BUFFER_FLAGS_DISPOSABLE_EXT_TEST;
    while (!eosFlag && (ret == 0)) {
        ret = WriteSample(vidTrackId, inputFile_, eosFlag, flag);
    }
    
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
    ASSERT_EQ(avmuxer_->Stop(), 0);
}

/**
 * @tc.name: Muxer_MP4_001
 * @tc.desc: Muxer mux mp4 by h264
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_MP4_001, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_MP4_001.mp4");
    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    std::shared_ptr<AVMuxerMock> avmuxerMock = AVMuxerMockFactory::CreateMuxer(fd_, AV_OUTPUT_FORMAT_MPEG_4);
    ASSERT_NE(avmuxerMock, nullptr);
    UnitTestMuxerType* avmuxer = reinterpret_cast<UnitTestMuxerType*>(avmuxerMock.get());
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
    param->Set<Tag::MEDIA_CREATION_TIME>("1800-12-19T03:16:00.000Z");
    param->Set<Tag::MEDIA_LATITUDE>(22.67f); // 22.67f test latitude
    param->Set<Tag::MEDIA_LONGITUDE>(114.06f); // 114.06f test longitude
    param->SetData("fast_start", static_cast<int32_t>(1)); // 1 moov 前置
    EXPECT_EQ(avmuxer->SetParameter(param), 0);

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
    uint32_t flag = AVCODEC_BUFFER_FLAGS_SYNC_FRAME;
    int32_t ret = WriteSampleUnitTest(trackId, inputFile_, eosFlag, flag, avmuxer);
    while (!eosFlag && (ret == 0)) {
        ret = WriteSampleUnitTest(trackId, inputFile_, eosFlag, flag, avmuxer);
    }
    EXPECT_EQ(ret, 0);
    ASSERT_EQ(avmuxer->Stop(), 0);
}

/**
 * @tc.name: Muxer_MP4_AIGC_001
 * @tc.desc: Muxer mux mp4 include AIGC info
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_MP4_AIGC_001, TestSize.Level0)
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
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_MP4_AIGC_002, TestSize.Level0)
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

/**
 * @tc.name: Muxer_MP4_GLTF_001
 * @tc.desc: normal case: muxer mp4 gltf box
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_MP4_GLTF_001, TestSize.Level0)
{
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_MP4_GLTF_001.mp4");
    std::shared_ptr<Meta> param = std::make_shared<Meta>();
    param->Set<Tag::MIME_TYPE>(Plugins::MimeType::AUDIO_AAC);
    param->Set<Tag::AUDIO_SAMPLE_RATE>(44100); // 44100 sample rate
    param->Set<Tag::AUDIO_CHANNEL_COUNT>(2); // 2 channel count
    param->Set<Tag::MEDIA_PROFILE>(AAC_PROFILE_LC);
    param->Set<Tag::AUDIO_SAMPLE_FORMAT>(Plugins::AudioSampleFormat::SAMPLE_S16LE);

    // 添加文件级元数据
    std::vector<uint8_t> binVec = {
        0x89, 0x47, 0x4C, 0x42, 0x02, 0x00, 0x00, 0x00,
        0x10, 0x00, 0x00, 0x00, 0x4E, 0x4F, 0x53, 0x4A,
        0x08, 0x00, 0x00, 0x00
    };
    param->Set<Tag::MEDIA_GLTF_VERSION>(0);
    param->Set<Tag::MEDIA_GLTF_ITEM_NAME>("result_panda_0918.glb");
    param->Set<Tag::MEDIA_GLTF_CONTENT_TYPE>("model/gltf-binary");
    param->Set<Tag::MEDIA_GLTF_CONTENT_ENCODING>("binary");
    param->Set<Tag::MEDIA_GLTF_DATA>(binVec);
    param->Set<Tag::MEDIA_GLTF_ITEM_TYPE>("mime");
    Mpeg4GltfGenerateFile(outputFile, param);

    struct stat fileStatus {};
    ASSERT_EQ(stat(outputFile.c_str(), &fileStatus), 0);
    int64_t fileSize = static_cast<int64_t>(fileStatus.st_size);
    OH_AVSource *source = OH_AVSource_CreateWithFD(fd_, 0, fileSize);
    OH_AVFormat *sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    int32_t isGltf = 0;
    int64_t gltfOffset = 0;
    ASSERT_NE(OH_AVFormat_GetIntValue(sourceFormat, Tag::IS_GLTF, &isGltf), false);
    ASSERT_NE(OH_AVFormat_GetLongValue(sourceFormat, Tag::GLTF_OFFSET, &gltfOffset), false);
    EXPECT_NE(isGltf, 0);
    EXPECT_GT(gltfOffset, 161399);  // mdatSize:161399
    OH_AVSource_Destroy(source);
}

/**
 * @tc.name: Muxer_MP4_GLTF_002
 * @tc.desc: mux mp4 gltf box with incomplete param
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_MP4_GLTF_002, TestSize.Level0)
{
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_MP4_GLTF_002.mp4");
    std::shared_ptr<Meta> param = std::make_shared<Meta>();
    param->Set<Tag::MIME_TYPE>(Plugins::MimeType::AUDIO_AAC);
    param->Set<Tag::AUDIO_SAMPLE_RATE>(44100); // 44100 sample rate
    param->Set<Tag::AUDIO_CHANNEL_COUNT>(2); // 2 channel count
    param->Set<Tag::MEDIA_PROFILE>(AAC_PROFILE_LC);
    param->Set<Tag::AUDIO_SAMPLE_FORMAT>(Plugins::AudioSampleFormat::SAMPLE_S16LE);

    // 添加文件级元数据
    std::vector<uint8_t> binVec = {
        0x89, 0x47, 0x4C, 0x42, 0x02, 0x00, 0x00, 0x00,
        0x10, 0x00, 0x00, 0x00, 0x4E, 0x4F, 0x53, 0x4A,
        0x08, 0x00, 0x00, 0x00
    };
    param->Set<Tag::MEDIA_GLTF_VERSION>(0); // missing other gltf tag
    Mpeg4GltfGenerateFile(outputFile, param);

    struct stat fileStatus {};
    ASSERT_EQ(stat(outputFile.c_str(), &fileStatus), 0);
    int64_t fileSize = static_cast<int64_t>(fileStatus.st_size);
    OH_AVSource *source = OH_AVSource_CreateWithFD(fd_, 0, fileSize);
    OH_AVFormat *sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    int32_t isGltf = 0;
    EXPECT_EQ(OH_AVFormat_GetIntValue(sourceFormat, Tag::IS_GLTF, &isGltf), false);
    OH_AVSource_Destroy(source);
}

/**
 * @tc.name: Muxer_MP4_GLTF_003
 * @tc.desc: normal case: muxer mp4 gltf box, moov front
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_MP4_GLTF_003, TestSize.Level0)
{
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_MP4_GLTF_003.mp4");
    std::shared_ptr<Meta> param = std::make_shared<Meta>();
    param->Set<Tag::MIME_TYPE>(Plugins::MimeType::AUDIO_AAC);
    param->Set<Tag::AUDIO_SAMPLE_RATE>(44100); // 44100 sample rate
    param->Set<Tag::AUDIO_CHANNEL_COUNT>(2); // 2 channel count
    param->Set<Tag::MEDIA_PROFILE>(AAC_PROFILE_LC);
    param->Set<Tag::AUDIO_SAMPLE_FORMAT>(Plugins::AudioSampleFormat::SAMPLE_S16LE);
    param->Set<Tag::MEDIA_ENABLE_MOOV_FRONT>(1); // moov front

    // 添加文件级元数据
    std::vector<uint8_t> binVec = {
        0x89, 0x47, 0x4C, 0x42, 0x02, 0x00, 0x00, 0x00,
        0x10, 0x00, 0x00, 0x00, 0x4E, 0x4F, 0x53, 0x4A,
        0x08, 0x00, 0x00, 0x00
    };
    param->Set<Tag::MEDIA_GLTF_VERSION>(0);
    param->Set<Tag::MEDIA_GLTF_ITEM_NAME>("result_panda_0918.glb");
    param->Set<Tag::MEDIA_GLTF_CONTENT_TYPE>("model/gltf-binary");
    param->Set<Tag::MEDIA_GLTF_CONTENT_ENCODING>("binary");
    param->Set<Tag::MEDIA_GLTF_DATA>(binVec);
    param->Set<Tag::MEDIA_GLTF_ITEM_TYPE>("mime");
    Mpeg4GltfGenerateFile(outputFile, param);

    struct stat fileStatus {};
    ASSERT_EQ(stat(outputFile.c_str(), &fileStatus), 0);
    int64_t fileSize = static_cast<int64_t>(fileStatus.st_size);
    OH_AVSource *source = OH_AVSource_CreateWithFD(fd_, 0, fileSize);
    OH_AVFormat *sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    int32_t isGltf = 0;
    int64_t gltfOffset = 0;
    ASSERT_NE(OH_AVFormat_GetIntValue(sourceFormat, Tag::IS_GLTF, &isGltf), false);
    ASSERT_NE(OH_AVFormat_GetLongValue(sourceFormat, Tag::GLTF_OFFSET, &gltfOffset), false);
    EXPECT_NE(isGltf, 0);
    EXPECT_GT(gltfOffset, 161399);  // mdatSize:161399
    OH_AVSource_Destroy(source);
}

/**
 * @tc.name: Muxer_MP4_GLTF_004
 * @tc.desc: normal case: muxer mp4 gltf box, moov front, version 1
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_MP4_GLTF_004, TestSize.Level0)
{
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_MP4_GLTF_004.mp4");
    std::shared_ptr<Meta> param = std::make_shared<Meta>();
    param->Set<Tag::MIME_TYPE>(Plugins::MimeType::AUDIO_AAC);
    param->Set<Tag::AUDIO_SAMPLE_RATE>(44100); // 44100 sample rate
    param->Set<Tag::AUDIO_CHANNEL_COUNT>(2); // 2 channel count
    param->Set<Tag::MEDIA_PROFILE>(AAC_PROFILE_LC);
    param->Set<Tag::AUDIO_SAMPLE_FORMAT>(Plugins::AudioSampleFormat::SAMPLE_S16LE);
    param->Set<Tag::MEDIA_ENABLE_MOOV_FRONT>(1); // moov front

    // 添加文件级元数据
    std::vector<uint8_t> binVec = {
        0x89, 0x47, 0x4C, 0x42, 0x02, 0x00, 0x00, 0x00,
        0x10, 0x00, 0x00, 0x00, 0x4E, 0x4F, 0x53, 0x4A,
        0x08, 0x00, 0x00, 0x00
    };
    param->Set<Tag::MEDIA_GLTF_VERSION>(1);
    param->Set<Tag::MEDIA_GLTF_ITEM_NAME>("result_panda_0918.glb");
    param->Set<Tag::MEDIA_GLTF_CONTENT_TYPE>("model/gltf-binary");
    param->Set<Tag::MEDIA_GLTF_CONTENT_ENCODING>("binary");
    param->Set<Tag::MEDIA_GLTF_DATA>(binVec);
    param->Set<Tag::MEDIA_GLTF_ITEM_TYPE>("mime");
    Mpeg4GltfGenerateFile(outputFile, param);

    struct stat fileStatus {};
    ASSERT_EQ(stat(outputFile.c_str(), &fileStatus), 0);
    int64_t fileSize = static_cast<int64_t>(fileStatus.st_size);
    OH_AVSource *source = OH_AVSource_CreateWithFD(fd_, 0, fileSize);
    OH_AVFormat *sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    int32_t isGltf = 0;
    int64_t gltfOffset = 0;
    ASSERT_NE(OH_AVFormat_GetIntValue(sourceFormat, Tag::IS_GLTF, &isGltf), false);
    ASSERT_NE(OH_AVFormat_GetLongValue(sourceFormat, Tag::GLTF_OFFSET, &gltfOffset), false);
    EXPECT_NE(isGltf, 0);
    EXPECT_GT(gltfOffset, 161399);  // mdatSize:161399
    OH_AVSource_Destroy(source);
}

/**
 * @tc.name: Muxer_MP4_GLTF_005
 * @tc.desc: normal case: muxer mp4 gltf box, moov front, version 2, itemType:mime
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_MP4_GLTF_005, TestSize.Level0)
{
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_MP4_GLTF_005.mp4");
    std::shared_ptr<Meta> param = std::make_shared<Meta>();
    param->Set<Tag::MIME_TYPE>(Plugins::MimeType::AUDIO_AAC);
    param->Set<Tag::AUDIO_SAMPLE_RATE>(44100); // 44100 sample rate
    param->Set<Tag::AUDIO_CHANNEL_COUNT>(2); // 2 channel count
    param->Set<Tag::MEDIA_PROFILE>(AAC_PROFILE_LC);
    param->Set<Tag::AUDIO_SAMPLE_FORMAT>(Plugins::AudioSampleFormat::SAMPLE_S16LE);
    param->Set<Tag::MEDIA_ENABLE_MOOV_FRONT>(1); // moov front

    // 添加文件级元数据
    std::vector<uint8_t> binVec = {
        0x89, 0x47, 0x4C, 0x42, 0x02, 0x00, 0x00, 0x00,
        0x10, 0x00, 0x00, 0x00, 0x4E, 0x4F, 0x53, 0x4A,
        0x08, 0x00, 0x00, 0x00
    };
    param->Set<Tag::MEDIA_GLTF_VERSION>(2);
    param->Set<Tag::MEDIA_GLTF_ITEM_NAME>("result_panda_0918.glb");
    param->Set<Tag::MEDIA_GLTF_CONTENT_TYPE>("model/gltf-binary");
    param->Set<Tag::MEDIA_GLTF_CONTENT_ENCODING>("binary");
    param->Set<Tag::MEDIA_GLTF_DATA>(binVec);
    param->Set<Tag::MEDIA_GLTF_ITEM_TYPE>("mime");
    Mpeg4GltfGenerateFile(outputFile, param);

    struct stat fileStatus {};
    ASSERT_EQ(stat(outputFile.c_str(), &fileStatus), 0);
    int64_t fileSize = static_cast<int64_t>(fileStatus.st_size);
    OH_AVSource *source = OH_AVSource_CreateWithFD(fd_, 0, fileSize);
    OH_AVFormat *sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    int32_t isGltf = 0;
    int64_t gltfOffset = 0;
    ASSERT_NE(OH_AVFormat_GetIntValue(sourceFormat, Tag::IS_GLTF, &isGltf), false);
    ASSERT_NE(OH_AVFormat_GetLongValue(sourceFormat, Tag::GLTF_OFFSET, &gltfOffset), false);
    EXPECT_NE(isGltf, 0);
    EXPECT_GT(gltfOffset, 161399);  // mdatSize:161399
    OH_AVSource_Destroy(source);
}

/**
 * @tc.name: Muxer_MP4_GLTF_006
 * @tc.desc: normal case: muxer mp4 gltf box, moov front, version 3, itemType:uri
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_MP4_GLTF_006, TestSize.Level0)
{
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_MP4_GLTF_006.mp4");
    std::shared_ptr<Meta> param = std::make_shared<Meta>();
    param->Set<Tag::MIME_TYPE>(Plugins::MimeType::AUDIO_AAC);
    param->Set<Tag::AUDIO_SAMPLE_RATE>(44100); // 44100 sample rate
    param->Set<Tag::AUDIO_CHANNEL_COUNT>(2); // 2 channel count
    param->Set<Tag::MEDIA_PROFILE>(AAC_PROFILE_LC);
    param->Set<Tag::AUDIO_SAMPLE_FORMAT>(Plugins::AudioSampleFormat::SAMPLE_S16LE);
    param->Set<Tag::MEDIA_ENABLE_MOOV_FRONT>(1); // moov front

    // 添加文件级元数据
    std::vector<uint8_t> binVec = {
        0x89, 0x47, 0x4C, 0x42, 0x02, 0x00, 0x00, 0x00,
        0x10, 0x00, 0x00, 0x00, 0x4E, 0x4F, 0x53, 0x4A,
        0x08, 0x00, 0x00, 0x00
    };
    param->Set<Tag::MEDIA_GLTF_VERSION>(2);
    param->Set<Tag::MEDIA_GLTF_ITEM_NAME>("result_panda_0918.glb");
    param->Set<Tag::MEDIA_GLTF_CONTENT_TYPE>("model/gltf-binary");
    param->Set<Tag::MEDIA_GLTF_CONTENT_ENCODING>("binary");
    param->Set<Tag::MEDIA_GLTF_DATA>(binVec);
    param->Set<Tag::MEDIA_GLTF_ITEM_TYPE>("uri");
    Mpeg4GltfGenerateFile(outputFile, param);

    struct stat fileStatus {};
    ASSERT_EQ(stat(outputFile.c_str(), &fileStatus), 0);
    int64_t fileSize = static_cast<int64_t>(fileStatus.st_size);
    OH_AVSource *source = OH_AVSource_CreateWithFD(fd_, 0, fileSize);
    OH_AVFormat *sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    int32_t isGltf = 0;
    int64_t gltfOffset = 0;
    ASSERT_NE(OH_AVFormat_GetIntValue(sourceFormat, Tag::IS_GLTF, &isGltf), false);
    ASSERT_NE(OH_AVFormat_GetLongValue(sourceFormat, Tag::GLTF_OFFSET, &gltfOffset), false);
    EXPECT_NE(isGltf, 0);
    EXPECT_GT(gltfOffset, 161399);  // mdatSize:161399
    OH_AVSource_Destroy(source);
}

/**
 * @tc.name: Muxer_SetFlag_001
 * @tc.desc: Muxer h264 while have B frame
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4MuxerUnitTest, Muxer_Ctts_001, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_ctts_001.mp4");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> videoParams =
        FormatMockFactory::CreateVideoFormat(OH_AVCODEC_MIMETYPE_VIDEO_AVC, TEST_WIDTH, TEST_HEIGHT);
    videoParams->PutDoubleValue(OH_MD_KEY_FRAME_RATE, 60.0);

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
    int32_t  i = 1;
    int64_t pts = 0;
    uint32_t flag = AVCODEC_BUFFER_FLAGS_DISPOSABLE;
    ret = WriteSampleHaveBFrame(trackId, inputFile_, eosFlag, flag, pts);
    while (!eosFlag && (ret == 0)) {
        pts = (i++) * 1000000 / 60;  // 1000000us 60fps
        if (pts % 10 > 5) {  // 10 5
            pts += 1;
        }
        if (i >= 150 && i < 450) {  // drop frame 150 - 450
            pts = -1;
        }
        if (i == 600) {  // 600
            pts -= 20000; // 20000us 20ms
        }
        ret = WriteSampleHaveBFrame(trackId, inputFile_, eosFlag, flag, pts);
    }
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(avmuxer_->Stop(), 0);
}
}