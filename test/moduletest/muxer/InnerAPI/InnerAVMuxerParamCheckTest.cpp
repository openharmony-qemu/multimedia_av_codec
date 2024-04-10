/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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

#include <string>
#include "gtest/gtest.h"
#include "AVMuxerDemo.h"
#include "avcodec_errors.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::Media;

namespace {
    class InnerAVMuxerParamCheckTest : public testing::Test {
    public:
        static void SetUpTestCase();
        static void TearDownTestCase();
        void SetUp() override;
        void TearDown() override;
    };

    void InnerAVMuxerParamCheckTest::SetUpTestCase() {}
    void InnerAVMuxerParamCheckTest::TearDownTestCase() {}
    void InnerAVMuxerParamCheckTest::SetUp() {}
    void InnerAVMuxerParamCheckTest::TearDown() {}
    const std::string HEVC_LIB_PATH = std::string(AV_CODEC_PATH) + "/libav_codec_hevc_parser.z.so";
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_001
 * @tc.name      : InnerCreate - fd check
 * @tc.desc      : param check test
 */
HWTEST_F(InnerAVMuxerParamCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_001, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Plugins::OutputFormat format = Plugins::OutputFormat::MPEG_4;
    int32_t fd = -1;
    int32_t ret = muxerDemo->InnerCreate(fd, format);
    ASSERT_EQ(AVCS_ERR_NO_MEMORY, ret);
    muxerDemo->InnerDestroy();

    fd = muxerDemo->InnerGetFdByMode(format);
    ret = muxerDemo->InnerCreate(fd, format);
    ASSERT_EQ(AVCS_ERR_OK, ret);
    muxerDemo->InnerDestroy();
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_002
 * @tc.name      : InnerCreate - format check
 * @tc.desc      : param check test
 */
HWTEST_F(InnerAVMuxerParamCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_002, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Plugins::OutputFormat format = Plugins::OutputFormat::MPEG_4;
    int32_t fd = muxerDemo->InnerGetFdByMode(format);
    int32_t ret = muxerDemo->InnerCreate(fd, format);
    ASSERT_EQ(AVCS_ERR_OK, ret);
    muxerDemo->InnerDestroy();

    format = Plugins::OutputFormat::M4A;
    fd = muxerDemo->InnerGetFdByMode(format);
    ret = muxerDemo->InnerCreate(fd, format);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    muxerDemo->InnerDestroy();

    format = Plugins::OutputFormat::DEFAULT;
    fd = muxerDemo->InnerGetFdByMode(format);
    ret = muxerDemo->InnerCreate(fd, format);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    muxerDemo->InnerDestroy();
    delete muxerDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_003
 * @tc.name      : InnerSetRotation - rotation check
 * @tc.desc      : param check test
 */
HWTEST_F(InnerAVMuxerParamCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_003, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Plugins::OutputFormat format = Plugins::OutputFormat::MPEG_4;
    int32_t fd = muxerDemo->InnerGetFdByMode(format);
    std::cout<<"fd "<< fd << endl;
    int32_t ret = muxerDemo->InnerCreate(fd, format);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    int32_t rotation;

    rotation = 0;
    ret = muxerDemo->InnerSetRotation(rotation);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    rotation = 90;
    ret = muxerDemo->InnerSetRotation(rotation);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    rotation = 180;
    ret = muxerDemo->InnerSetRotation(rotation);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    rotation = 270;
    ret = muxerDemo->InnerSetRotation(rotation);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    rotation = -90;
    ret = muxerDemo->InnerSetRotation(rotation);
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, ret);

    rotation = 45;
    ret = muxerDemo->InnerSetRotation(rotation);
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, ret);

    muxerDemo->InnerDestroy();

    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_004
 * @tc.name      : InnerAddTrack - (MediaDescriptionKey::MD_KEY_CODEC_MIME) check
 * @tc.desc      : param check test
 */
HWTEST_F(InnerAVMuxerParamCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_004, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Plugins::OutputFormat format = Plugins::OutputFormat::MPEG_4;
    int32_t fd = muxerDemo->InnerGetFdByMode(format);
    std::cout<<"fd "<< fd << endl;

    int32_t ret = muxerDemo->InnerCreate(fd, format);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<Meta> audioParams = std::make_shared<Meta>();
    std::vector<uint8_t> a(100);

    audioParams->Set<Tag::MIME_TYPE>(Plugins::MimeType::AUDIO_AAC);
    audioParams->Set<Tag::MEDIA_CODEC_CONFIG>(a);
    audioParams->Set<Tag::AUDIO_CHANNEL_COUNT>(1);
    audioParams->Set<Tag::AUDIO_SAMPLE_RATE>(48000);

    int trackIndex = 0;
    int32_t trackId;

    trackId = muxerDemo->InnerAddTrack(trackIndex, audioParams);
    ASSERT_EQ(0, trackId);

    audioParams->Set<Tag::MIME_TYPE>(Plugins::MimeType::AUDIO_MPEG);
    trackId = muxerDemo->InnerAddTrack(trackIndex, audioParams);
    ASSERT_EQ(0, trackId);

    audioParams->Set<Tag::MIME_TYPE>(Plugins::MimeType::AUDIO_FLAC);
    trackId = muxerDemo->InnerAddTrack(trackIndex, audioParams);
    ASSERT_EQ(AVCS_ERR_UNSUPPORT_FILE_TYPE, trackId);

    audioParams->Set<Tag::MIME_TYPE>("aaaaaa");
    trackId = muxerDemo->InnerAddTrack(trackIndex, audioParams);
    ASSERT_EQ(AVCS_ERR_UNSUPPORT_FILE_TYPE, trackId);

    muxerDemo->InnerDestroy();

    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_005
 * @tc.name      : InnerAddTrack - (MediaDescriptionKey::MD_KEY_CHANNEL_COUNT) check
 * @tc.desc      : param check test
 */
HWTEST_F(InnerAVMuxerParamCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_005, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Plugins::OutputFormat format = Plugins::OutputFormat::MPEG_4;
    int32_t fd = muxerDemo->InnerGetFdByMode(format);
    int32_t ret = muxerDemo->InnerCreate(fd, format);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<Meta> audioParams = std::make_shared<Meta>();
    std::vector<uint8_t> a(100);

    audioParams->Set<Tag::MIME_TYPE>(Plugins::MimeType::AUDIO_AAC);
    audioParams->Set<Tag::MEDIA_CODEC_CONFIG>(a);
    audioParams->Set<Tag::AUDIO_CHANNEL_COUNT>(1);
    audioParams->Set<Tag::AUDIO_SAMPLE_RATE>(48000);

    int trackIndex = 0;
    int32_t trackId;

    trackId = muxerDemo->InnerAddTrack(trackIndex, audioParams);
    ASSERT_EQ(0, trackId);

    audioParams->Set<Tag::AUDIO_CHANNEL_COUNT>(-1);
    trackId = muxerDemo->InnerAddTrack(trackIndex, audioParams);
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, trackId);

    muxerDemo->InnerDestroy();
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_006
 * @tc.name      : InnerAddTrack - (MediaDescriptionKey::MD_KEY_SAMPLE_RATE) check
 * @tc.desc      : param check test
 */
HWTEST_F(InnerAVMuxerParamCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_006, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Plugins::OutputFormat format = Plugins::OutputFormat::MPEG_4;
    int32_t fd = muxerDemo->InnerGetFdByMode(format);
    int32_t ret = muxerDemo->InnerCreate(fd, format);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<Meta> audioParams = std::make_shared<Meta>();
    std::vector<uint8_t> a(100);

    audioParams->Set<Tag::MIME_TYPE>(Plugins::MimeType::AUDIO_AAC);
    audioParams->Set<Tag::MEDIA_CODEC_CONFIG>(a);
    audioParams->Set<Tag::AUDIO_CHANNEL_COUNT>(1);
    audioParams->Set<Tag::AUDIO_SAMPLE_RATE>(48000);

    int trackIndex = 0;
    int32_t trackId;

    trackId = muxerDemo->InnerAddTrack(trackIndex, audioParams);
    ASSERT_EQ(0, trackId);

    audioParams->Set<Tag::AUDIO_SAMPLE_RATE>(-1);
    trackId = muxerDemo->InnerAddTrack(trackIndex, audioParams);
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, trackId);

    muxerDemo->InnerDestroy();
    delete muxerDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_007
 * @tc.name      : InnerAddTrack - video (MediaDescriptionKey::MD_KEY_CODEC_MIME) check
 * @tc.desc      : param check test
 */
HWTEST_F(InnerAVMuxerParamCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_007, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Plugins::OutputFormat format = Plugins::OutputFormat::MPEG_4;
    int32_t fd = muxerDemo->InnerGetFdByMode(format);
    int32_t ret = muxerDemo->InnerCreate(fd, format);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<Meta> videoParams = std::make_shared<Meta>();
    std::vector<uint8_t> a(100);

    videoParams->Set<Tag::MIME_TYPE>(Plugins::MimeType::VIDEO_MPEG4);
    videoParams->Set<Tag::MEDIA_CODEC_CONFIG>(a);
    videoParams->Set<Tag::VIDEO_WIDTH>(352);
    videoParams->Set<Tag::VIDEO_HEIGHT>(288);

    int32_t trackId;
    int trackIndex = 0;

    trackId = muxerDemo->InnerAddTrack(trackIndex, videoParams);
    ASSERT_EQ(0, trackId);

    videoParams->Set<Tag::MIME_TYPE>(Plugins::MimeType::VIDEO_AVC);
    trackId = muxerDemo->InnerAddTrack(trackIndex, videoParams);
    ASSERT_EQ(0, trackId);

    videoParams->Set<Tag::MIME_TYPE>(Plugins::MimeType::IMAGE_JPG);
    trackId = muxerDemo->InnerAddTrack(trackIndex, videoParams);
    ASSERT_EQ(0, trackId);

    videoParams->Set<Tag::MIME_TYPE>(Plugins::MimeType::IMAGE_PNG);
    trackId = muxerDemo->InnerAddTrack(trackIndex, videoParams);
    ASSERT_EQ(0, trackId);

    videoParams->Set<Tag::MIME_TYPE>(Plugins::MimeType::IMAGE_BMP);
    trackId = muxerDemo->InnerAddTrack(trackIndex, videoParams);
    ASSERT_EQ(0, trackId);

    muxerDemo->InnerDestroy();
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_008
 * @tc.name      : InnerAddTrack - video (MediaDescriptionKey::MD_KEY_WIDTH) check
 * @tc.desc      : param check test
 */
HWTEST_F(InnerAVMuxerParamCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_008, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Plugins::OutputFormat format = Plugins::OutputFormat::MPEG_4;
    int32_t fd = muxerDemo->InnerGetFdByMode(format);
    int32_t ret = muxerDemo->InnerCreate(fd, format);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<Meta> videoParams = std::make_shared<Meta>();
    std::vector<uint8_t> a(100);

    videoParams->Set<Tag::MIME_TYPE>(Plugins::MimeType::VIDEO_MPEG4);
    videoParams->Set<Tag::MEDIA_CODEC_CONFIG>(a);
    videoParams->Set<Tag::VIDEO_WIDTH>(352);
    videoParams->Set<Tag::VIDEO_HEIGHT>(288);

    int trackIndex = 0;
    int32_t trackId;

    trackId = muxerDemo->InnerAddTrack(trackIndex, videoParams);
    ASSERT_EQ(0, trackId);

    videoParams->Set<Tag::VIDEO_WIDTH>(-1);
    trackId = muxerDemo->InnerAddTrack(trackIndex, videoParams);
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, trackId);

    muxerDemo->InnerDestroy();
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_009
 * @tc.name      : InnerAddTrack - video (MediaDescriptionKey::MD_KEY_HEIGHT) check
 * @tc.desc      : param check test
 */
HWTEST_F(InnerAVMuxerParamCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_009, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Plugins::OutputFormat format = Plugins::OutputFormat::MPEG_4;
    int32_t fd = muxerDemo->InnerGetFdByMode(format);
    int32_t ret = muxerDemo->InnerCreate(fd, format);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<Meta> videoParams = std::make_shared<Meta>();
    std::vector<uint8_t> a(100);

    videoParams->Set<Tag::MIME_TYPE>(Plugins::MimeType::VIDEO_MPEG4);
    videoParams->Set<Tag::MEDIA_CODEC_CONFIG>(a);
    videoParams->Set<Tag::VIDEO_WIDTH>(352);
    videoParams->Set<Tag::VIDEO_HEIGHT>(288);

    int trackIndex = 0;
    int32_t trackId;

    trackId = muxerDemo->InnerAddTrack(trackIndex, videoParams);
    ASSERT_EQ(0, trackId);

    videoParams->Set<Tag::VIDEO_HEIGHT>(-1);
    trackId = muxerDemo->InnerAddTrack(trackIndex, videoParams);
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, trackId);

    muxerDemo->InnerDestroy();
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_010
 * @tc.name      : InnerAddTrack - (any key) check
 * @tc.desc      : param check test
 */
HWTEST_F(InnerAVMuxerParamCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_010, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Plugins::OutputFormat format = Plugins::OutputFormat::MPEG_4;
    int32_t fd = muxerDemo->InnerGetFdByMode(format);
    int32_t ret = muxerDemo->InnerCreate(fd, format);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<Meta> audioParams = std::make_shared<Meta>();
    audioParams->SetData("aaaaa", "bbbbb");

    int trackIndex = 0;
    int32_t trackId;

    trackId = muxerDemo->InnerAddTrack(trackIndex, audioParams);
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, trackId);

    muxerDemo->InnerDestroy();
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_011
 * @tc.name      : InnerWriteSample - info.trackIndex check
 * @tc.desc      : param check test
 */
HWTEST_F(InnerAVMuxerParamCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_011, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Plugins::OutputFormat format = Plugins::OutputFormat::MPEG_4;
    int32_t fd = muxerDemo->InnerGetFdByMode(format);
    int32_t ret = muxerDemo->InnerCreate(fd, format);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<Meta> audioParams = std::make_shared<Meta>();
    std::vector<uint8_t> a(100);

    audioParams->Set<Tag::MIME_TYPE>(Plugins::MimeType::AUDIO_AAC);
    audioParams->Set<Tag::MEDIA_CODEC_CONFIG>(a);
    audioParams->Set<Tag::AUDIO_CHANNEL_COUNT>(1);
    audioParams->Set<Tag::AUDIO_SAMPLE_RATE>(48000);

    int trackIndex = 0;
    int32_t trackId;

    trackId = muxerDemo->InnerAddTrack(trackIndex, audioParams);
    ASSERT_EQ(0, trackId);


    ret = muxerDemo->InnerStart();
    ASSERT_EQ(AVCS_ERR_OK, ret);

    uint8_t data[100];
    std::shared_ptr<AVBuffer> avMemBuffer = AVBuffer::CreateAVBuffer(data, sizeof(data), sizeof(data));

    ret = muxerDemo->InnerWriteSample(trackIndex, avMemBuffer);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    trackIndex = -1;
    ret = muxerDemo->InnerWriteSample(trackIndex, avMemBuffer);
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, ret);

    muxerDemo->InnerDestroy();
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_012
 * @tc.name      : InnerWriteSample - info.presentationTimeUs check
 * @tc.desc      : param check test
 */
HWTEST_F(InnerAVMuxerParamCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_012, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Plugins::OutputFormat format = Plugins::OutputFormat::MPEG_4;
    int32_t fd = muxerDemo->InnerGetFdByMode(format);
    int32_t ret = muxerDemo->InnerCreate(fd, format);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<Meta> audioParams = std::make_shared<Meta>();
    std::vector<uint8_t> a(100);

    audioParams->Set<Tag::MIME_TYPE>(Plugins::MimeType::AUDIO_AAC);
    audioParams->Set<Tag::MEDIA_CODEC_CONFIG>(a);
    audioParams->Set<Tag::AUDIO_CHANNEL_COUNT>(1);
    audioParams->Set<Tag::AUDIO_SAMPLE_RATE>(48000);

    int trackIndex = 0;

    ret = muxerDemo->InnerAddTrack(trackIndex, audioParams);
    ASSERT_EQ(0, ret);
    std::cout << "trackIndex: " << trackIndex << std::endl;
    ret = muxerDemo->InnerStart();
    ASSERT_EQ(AVCS_ERR_OK, ret);

    uint8_t data[100];
    std::shared_ptr<AVBuffer> avMemBuffer = AVBuffer::CreateAVBuffer(data, sizeof(data), sizeof(data));

    ret = muxerDemo->InnerWriteSample(trackIndex, avMemBuffer);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    avMemBuffer->pts_ = -1;
    ret = muxerDemo->InnerWriteSample(trackIndex, avMemBuffer);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    muxerDemo->InnerDestroy();
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_013
 * @tc.name      : InnerWriteSample - info.size check
 * @tc.desc      : param check test
 */
HWTEST_F(InnerAVMuxerParamCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_013, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Plugins::OutputFormat format = Plugins::OutputFormat::MPEG_4;
    int32_t fd = muxerDemo->InnerGetFdByMode(format);
    int32_t ret = muxerDemo->InnerCreate(fd, format);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<Meta> audioParams = std::make_shared<Meta>();
    std::vector<uint8_t> a(100);

    audioParams->Set<Tag::MIME_TYPE>(Plugins::MimeType::AUDIO_AAC);
    audioParams->Set<Tag::MEDIA_CODEC_CONFIG>(a);
    audioParams->Set<Tag::AUDIO_CHANNEL_COUNT>(1);
    audioParams->Set<Tag::AUDIO_SAMPLE_RATE>(48000);

    int trackIndex = 0;
    int32_t trackId;

    trackId = muxerDemo->InnerAddTrack(trackIndex, audioParams);
    ASSERT_EQ(0, trackId);

    ret = muxerDemo->InnerStart();
    ASSERT_EQ(AVCS_ERR_OK, ret);

    uint8_t data[100];
    std::shared_ptr<AVBuffer> avMemBuffer = AVBuffer::CreateAVBuffer(data, sizeof(data), sizeof(data));

    ret = muxerDemo->InnerWriteSample(trackIndex, avMemBuffer);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    avMemBuffer->memory_->SetSize(-1);
    ret = muxerDemo->InnerWriteSample(trackIndex, avMemBuffer);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    muxerDemo->InnerDestroy();
    delete muxerDemo;
}
/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_014
 * @tc.name      : InnerWriteSample - info.offset check
 * @tc.desc      : param check test
 */
HWTEST_F(InnerAVMuxerParamCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_014, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Plugins::OutputFormat format = Plugins::OutputFormat::MPEG_4;
    int32_t fd = muxerDemo->InnerGetFdByMode(format);
    int32_t ret = muxerDemo->InnerCreate(fd, format);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<Meta> audioParams = std::make_shared<Meta>();
    std::vector<uint8_t> a(100);

    audioParams->Set<Tag::MIME_TYPE>(Plugins::MimeType::AUDIO_AAC);
    audioParams->Set<Tag::MEDIA_CODEC_CONFIG>(a);
    audioParams->Set<Tag::AUDIO_CHANNEL_COUNT>(1);
    audioParams->Set<Tag::AUDIO_SAMPLE_RATE>(48000);

    int trackIndex = 0;
    int32_t trackId;

    trackId = muxerDemo->InnerAddTrack(trackIndex, audioParams);
    ASSERT_EQ(0, trackId);

    ret = muxerDemo->InnerStart();
    ASSERT_EQ(AVCS_ERR_OK, ret);

    uint8_t data[100];
    std::shared_ptr<AVBuffer> avMemBuffer = AVBuffer::CreateAVBuffer(data, sizeof(data), sizeof(data));

    ret = muxerDemo->InnerWriteSample(trackIndex, avMemBuffer);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    avMemBuffer->memory_->SetOffset(-1);
    ret = muxerDemo->InnerWriteSample(trackIndex, avMemBuffer);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    muxerDemo->InnerDestroy();
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_015
 * @tc.name      : InnerWriteSample - info.flags check
 * @tc.desc      : param check test
 */
HWTEST_F(InnerAVMuxerParamCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_015, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Plugins::OutputFormat format = Plugins::OutputFormat::MPEG_4;
    int32_t fd = muxerDemo->InnerGetFdByMode(format);
    int32_t ret = muxerDemo->InnerCreate(fd, format);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<Meta> audioParams = std::make_shared<Meta>();
    std::vector<uint8_t> a(100);

    audioParams->Set<Tag::MIME_TYPE>(Plugins::MimeType::AUDIO_AAC);
    audioParams->Set<Tag::MEDIA_CODEC_CONFIG>(a);
    audioParams->Set<Tag::AUDIO_CHANNEL_COUNT>(1);
    audioParams->Set<Tag::AUDIO_SAMPLE_RATE>(48000);

    int trackIndex = 0;
    int32_t trackId;

    trackId = muxerDemo->InnerAddTrack(trackIndex, audioParams);
    ASSERT_EQ(0, trackId);

    ret = muxerDemo->InnerStart();
    ASSERT_EQ(AVCS_ERR_OK, ret);

    uint8_t data[100];
    std::shared_ptr<AVBuffer> avMemBuffer = AVBuffer::CreateAVBuffer(data, sizeof(data), sizeof(data));

    avMemBuffer->flag_ = static_cast<uint32_t>(Plugins::AVBufferFlag::SYNC_FRAME);
    ret = muxerDemo->InnerWriteSample(trackIndex, avMemBuffer);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    muxerDemo->InnerDestroy();
    delete muxerDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_016
 * @tc.name      : InnerAddTrack - video (MediaDescriptionKey::MD_KEY_COLOR_PRIMARIES) check
 * @tc.desc      : param check test
 */
HWTEST_F(InnerAVMuxerParamCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_016, TestSize.Level2)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        std::cout << "the hevc of mimetype is not supported" << std::endl;
        return;
    }

    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Plugins::OutputFormat format = Plugins::OutputFormat::MPEG_4;
    int32_t fd = muxerDemo->InnerGetFdByMode(format);
    int32_t ret = muxerDemo->InnerCreate(fd, format);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    std::shared_ptr<Meta> videoParams = std::make_shared<Meta>();
    std::vector<uint8_t> a(100);

    videoParams->Set<Tag::MIME_TYPE>(Plugins::MimeType::VIDEO_HEVC);
    videoParams->Set<Tag::MEDIA_CODEC_CONFIG>(a);
    videoParams->Set<Tag::VIDEO_WIDTH>(352);
    videoParams->Set<Tag::VIDEO_HEIGHT>(288);
    videoParams->Set<Tag::VIDEO_FRAME_RATE>(60);
    videoParams->Set<Tag::VIDEO_DELAY>(2);
    videoParams->Set<Tag::VIDEO_COLOR_PRIMARIES>(Plugins::ColorPrimary::BT709);
    videoParams->Set<Tag::VIDEO_COLOR_TRC>(Plugins::TransferCharacteristic::BT709);
    videoParams->Set<Tag::VIDEO_COLOR_MATRIX_COEFF>(Plugins::MatrixCoefficient::BT709);
    videoParams->Set<Tag::VIDEO_COLOR_RANGE>(false);
    videoParams->Set<Tag::VIDEO_IS_HDR_VIVID>(true);

    videoParams->Set<Tag::MEDIA_CREATION_TIME>(nullptr);
    videoParams->SetData(nullptr, nullptr);
    videoParams->SetData(0, 0);

    int trackIndex = 0;
    int32_t trackId;

    trackId = muxerDemo->InnerAddTrack(trackIndex, videoParams);
    ASSERT_EQ(0, trackId);

    videoParams->Set<Tag::VIDEO_COLOR_PRIMARIES>(static_cast<Plugins::ColorPrimary>(0));
    trackId = muxerDemo->InnerAddTrack(trackIndex, videoParams);
    ASSERT_NE(0, trackId);

    videoParams->Set<Tag::VIDEO_COLOR_PRIMARIES>(Plugins::ColorPrimary::BT709);
    videoParams->Set<Tag::VIDEO_COLOR_TRC>(static_cast<Plugins::TransferCharacteristic>(0));
    trackId = muxerDemo->InnerAddTrack(trackIndex, videoParams);
    ASSERT_NE(0, trackId);

    videoParams->Set<Tag::VIDEO_COLOR_TRC>(Plugins::TransferCharacteristic::BT709);
    videoParams->Set<Tag::VIDEO_COLOR_MATRIX_COEFF>(static_cast<Plugins::MatrixCoefficient>(3));
    trackId = muxerDemo->InnerAddTrack(trackIndex, videoParams);
    ASSERT_NE(0, trackId);

    videoParams->Set<Tag::VIDEO_COLOR_MATRIX_COEFF>(Plugins::MatrixCoefficient::BT709);
    videoParams->Set<Tag::VIDEO_IS_HDR_VIVID>(false);
    trackId = muxerDemo->InnerAddTrack(trackIndex, videoParams);
    ASSERT_EQ(0, trackId);

    muxerDemo->InnerDestroy();
    delete muxerDemo;
}
