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
#include "avcodec_info.h"
#include "avcodec_errors.h"
#include "avcodec_common.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;


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
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_001
 * @tc.name      : InnerCreate - fd check
 * @tc.desc      : param check test
 */
HWTEST_F(InnerAVMuxerParamCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_001, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    OutputFormat format = OUTPUT_FORMAT_MPEG_4;
    int32_t fd = -1;
    int32_t ret = muxerDemo->InnerCreate(fd, format);
    ASSERT_EQ(AVCS_ERR_NO_MEMORY, ret);
    muxerDemo->InnerDestroy();

    fd = muxerDemo->InnergetFdByMode(format);
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
    OutputFormat format = OUTPUT_FORMAT_MPEG_4;
    int32_t fd = muxerDemo->InnergetFdByMode(format);
    int32_t ret = muxerDemo->InnerCreate(fd, format);
    ASSERT_EQ(AVCS_ERR_OK, ret);
    muxerDemo->InnerDestroy();

    format = OUTPUT_FORMAT_M4A;
    fd = muxerDemo->InnergetFdByMode(format);
    ret = muxerDemo->InnerCreate(fd, format);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    muxerDemo->InnerDestroy();

    format = OUTPUT_FORMAT_DEFAULT;
    fd = muxerDemo->InnergetFdByMode(format);
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
    OutputFormat format = OUTPUT_FORMAT_MPEG_4;
    int32_t fd = muxerDemo->InnergetFdByMode(format);
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
    OutputFormat format = OUTPUT_FORMAT_MPEG_4;
    int32_t fd = muxerDemo->InnergetFdByMode(format);
    std::cout<<"fd "<< fd << endl;

    int32_t ret = muxerDemo->InnerCreate(fd, format);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    MediaDescription audioParams;
    uint8_t a[100];

    audioParams.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::AUDIO_AAC);
    audioParams.PutBuffer(MediaDescriptionKey::MD_KEY_CODEC_CONFIG, a, 100);
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 48000);

    int trackIndex = 0;
    int32_t trackId;

    trackId = muxerDemo->InnerAddTrack(trackIndex, audioParams);
    ASSERT_EQ(0, trackId);

    audioParams.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::AUDIO_MPEG);
    trackId = muxerDemo->InnerAddTrack(trackIndex, audioParams);
    ASSERT_EQ(0, trackId);

    audioParams.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::AUDIO_FLAC);
    trackId = muxerDemo->InnerAddTrack(trackIndex, audioParams);
    ASSERT_EQ(AVCS_ERR_UNSUPPORT_CONTAINER_TYPE, trackId);

    audioParams.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, "aaaaaa");
    trackId = muxerDemo->InnerAddTrack(trackIndex, audioParams);
    ASSERT_EQ(AVCS_ERR_UNSUPPORT_CONTAINER_TYPE, trackId);

    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, 0);
    trackId = muxerDemo->InnerAddTrack(trackIndex, audioParams);
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, trackId);

    audioParams.PutLongValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, 0);
    trackId = muxerDemo->InnerAddTrack(trackIndex, audioParams);
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, trackId);

    audioParams.PutFloatValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, 0.1);
    trackId = muxerDemo->InnerAddTrack(trackIndex, audioParams);
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, trackId);

    audioParams.PutDoubleValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, 0.1);
    trackId = muxerDemo->InnerAddTrack(trackIndex, audioParams);
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, trackId);

    uint8_t b[100];
    audioParams.PutBuffer(MediaDescriptionKey::MD_KEY_CODEC_MIME, b, 100);
    trackId = muxerDemo->InnerAddTrack(trackIndex, audioParams);
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, trackId);

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
    OutputFormat format = OUTPUT_FORMAT_MPEG_4;
    int32_t fd = muxerDemo->InnergetFdByMode(format);
    int32_t ret = muxerDemo->InnerCreate(fd, format);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    MediaDescription audioParams;
    uint8_t a[100];

    audioParams.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::AUDIO_AAC);
    audioParams.PutBuffer(MediaDescriptionKey::MD_KEY_CODEC_CONFIG, a, 100);
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 48000);

    int trackIndex = 0;
    int32_t trackId;

    trackId = muxerDemo->InnerAddTrack(trackIndex, audioParams);
    ASSERT_EQ(0, trackId);

    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, -1);
    trackId = muxerDemo->InnerAddTrack(trackIndex, audioParams);
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, trackId);

    audioParams.PutStringValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, "aaaaaa");
    trackId = muxerDemo->InnerAddTrack(trackIndex, audioParams);
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, trackId);

    audioParams.PutLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 0);
    trackId = muxerDemo->InnerAddTrack(trackIndex, audioParams);
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, trackId);

    audioParams.PutFloatValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 0.1);
    trackId = muxerDemo->InnerAddTrack(trackIndex, audioParams);
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, trackId);

    audioParams.PutDoubleValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 0.1);
    trackId = muxerDemo->InnerAddTrack(trackIndex, audioParams);
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, trackId);

    uint8_t b[100];
    audioParams.PutBuffer(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, b, 100);
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
    OutputFormat format = OUTPUT_FORMAT_MPEG_4;
    int32_t fd = muxerDemo->InnergetFdByMode(format);
    int32_t ret = muxerDemo->InnerCreate(fd, format);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    MediaDescription audioParams;
    uint8_t a[100];

    audioParams.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::AUDIO_AAC);
    audioParams.PutBuffer(MediaDescriptionKey::MD_KEY_CODEC_CONFIG, a, 100);
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 48000);

    int trackIndex = 0;
    int32_t trackId;

    trackId = muxerDemo->InnerAddTrack(trackIndex, audioParams);
    ASSERT_EQ(0, trackId);

    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, -1);
    trackId = muxerDemo->InnerAddTrack(trackIndex, audioParams);
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, trackId);

    audioParams.PutStringValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, "aaaaaa");
    trackId = muxerDemo->InnerAddTrack(trackIndex, audioParams);
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, trackId);

    audioParams.PutLongValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 0);
    trackId = muxerDemo->InnerAddTrack(trackIndex, audioParams);
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, trackId);

    audioParams.PutFloatValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 0.1);
    trackId = muxerDemo->InnerAddTrack(trackIndex, audioParams);
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, trackId);

    audioParams.PutDoubleValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 0.1);
    trackId = muxerDemo->InnerAddTrack(trackIndex, audioParams);
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, trackId);

    uint8_t b[100];
    audioParams.PutBuffer(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, b, 100);
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
    OutputFormat format = OUTPUT_FORMAT_MPEG_4;
    int32_t fd = muxerDemo->InnergetFdByMode(format);
    int32_t ret = muxerDemo->InnerCreate(fd, format);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    MediaDescription videoParams;
    uint8_t a[100];

    videoParams.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME,  CodecMimeType::VIDEO_MPEG4);
    videoParams.PutBuffer(MediaDescriptionKey::MD_KEY_CODEC_CONFIG, a, 100);
    videoParams.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 352);
    videoParams.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 288);

    int32_t trackId;
    int trackIndex = 0;

    trackId = muxerDemo->InnerAddTrack(trackIndex, videoParams);
    ASSERT_EQ(0, trackId);

    videoParams.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_AVC);
    trackId = muxerDemo->InnerAddTrack(trackIndex, videoParams);
    ASSERT_EQ(0, trackId);

    videoParams.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_HEVC);
    trackId = muxerDemo->InnerAddTrack(trackIndex, videoParams);
    ASSERT_EQ(0, trackId);

    videoParams.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::IMAGE_JPG);
    trackId = muxerDemo->InnerAddTrack(trackIndex, videoParams);
    ASSERT_EQ(0, trackId);

    videoParams.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::IMAGE_PNG);
    trackId = muxerDemo->InnerAddTrack(trackIndex, videoParams);
    ASSERT_EQ(0, trackId);

    videoParams.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::IMAGE_BMP);
    trackId = muxerDemo->InnerAddTrack(trackIndex, videoParams);
    ASSERT_EQ(0, trackId);

    videoParams.PutIntValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, 0);
    trackId = muxerDemo->InnerAddTrack(trackIndex, videoParams);
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, trackId);

    videoParams.PutLongValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, 0);
    trackId = muxerDemo->InnerAddTrack(trackIndex, videoParams);
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, trackId);

    videoParams.PutFloatValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, 0.1);
    trackId = muxerDemo->InnerAddTrack(trackIndex, videoParams);
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, trackId);

    videoParams.PutDoubleValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, 0.1);
    trackId = muxerDemo->InnerAddTrack(trackIndex, videoParams);
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, trackId);

    uint8_t b[100];
    videoParams.PutBuffer(MediaDescriptionKey::MD_KEY_CODEC_MIME, b, 100);
    trackId = muxerDemo->InnerAddTrack(trackIndex, videoParams);
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, trackId);

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
    OutputFormat format = OUTPUT_FORMAT_MPEG_4;
    int32_t fd = muxerDemo->InnergetFdByMode(format);
    int32_t ret = muxerDemo->InnerCreate(fd, format);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    MediaDescription videoParams;
    uint8_t a[100];

    videoParams.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME,  CodecMimeType::VIDEO_MPEG4);
    videoParams.PutBuffer(MediaDescriptionKey::MD_KEY_CODEC_CONFIG, a, 100);
    videoParams.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 352);
    videoParams.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 288);

    int trackIndex = 0;
    int32_t trackId;

    trackId = muxerDemo->InnerAddTrack(trackIndex, videoParams);
    ASSERT_EQ(0, trackId);

    videoParams.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, -1);
    trackId = muxerDemo->InnerAddTrack(trackIndex, videoParams);
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, trackId);

    videoParams.PutStringValue(MediaDescriptionKey::MD_KEY_WIDTH, "aaa");
    trackId = muxerDemo->InnerAddTrack(trackIndex, videoParams);
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, trackId);

    videoParams.PutLongValue(MediaDescriptionKey::MD_KEY_WIDTH, 0);
    trackId = muxerDemo->InnerAddTrack(trackIndex, videoParams);
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, trackId);

    videoParams.PutFloatValue(MediaDescriptionKey::MD_KEY_WIDTH, 0.1);
    trackId = muxerDemo->InnerAddTrack(trackIndex, videoParams);
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, trackId);

    videoParams.PutDoubleValue(MediaDescriptionKey::MD_KEY_WIDTH, 0.1);
    trackId = muxerDemo->InnerAddTrack(trackIndex, videoParams);
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, trackId);

    uint8_t b[100];
    videoParams.PutBuffer(MediaDescriptionKey::MD_KEY_WIDTH, b, 100);
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
    OutputFormat format = OUTPUT_FORMAT_MPEG_4;
    int32_t fd = muxerDemo->InnergetFdByMode(format);
    int32_t ret = muxerDemo->InnerCreate(fd, format);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    MediaDescription videoParams;
    uint8_t a[100];

    videoParams.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME,  CodecMimeType::VIDEO_MPEG4);
    videoParams.PutBuffer(MediaDescriptionKey::MD_KEY_CODEC_CONFIG, a, 100);
    videoParams.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 352);
    videoParams.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 288);

    int trackIndex = 0;
    int32_t trackId;

    trackId = muxerDemo->InnerAddTrack(trackIndex, videoParams);
    ASSERT_EQ(0, trackId);

    videoParams.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, -1);
    trackId = muxerDemo->InnerAddTrack(trackIndex, videoParams);
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, trackId);

    videoParams.PutStringValue(MediaDescriptionKey::MD_KEY_HEIGHT, "aaa");
    trackId = muxerDemo->InnerAddTrack(trackIndex, videoParams);
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, trackId);

    videoParams.PutLongValue(MediaDescriptionKey::MD_KEY_HEIGHT, 0);
    trackId = muxerDemo->InnerAddTrack(trackIndex, videoParams);
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, trackId);

    videoParams.PutFloatValue(MediaDescriptionKey::MD_KEY_HEIGHT, 0.1);
    trackId = muxerDemo->InnerAddTrack(trackIndex, videoParams);
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, trackId);

    videoParams.PutDoubleValue(MediaDescriptionKey::MD_KEY_HEIGHT, 0.1);
    trackId = muxerDemo->InnerAddTrack(trackIndex, videoParams);
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, trackId);

    uint8_t b[100];
    videoParams.PutBuffer(MediaDescriptionKey::MD_KEY_HEIGHT, b, 100);
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
    OutputFormat format = OUTPUT_FORMAT_MPEG_4;
    int32_t fd = muxerDemo->InnergetFdByMode(format);
    int32_t ret = muxerDemo->InnerCreate(fd, format);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    MediaDescription audioParams;
    audioParams.PutStringValue("aaaaa", "bbbbb");

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
    OutputFormat format = OUTPUT_FORMAT_MPEG_4;
    int32_t fd = muxerDemo->InnergetFdByMode(format);
    int32_t ret = muxerDemo->InnerCreate(fd, format);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    MediaDescription audioParams;
    uint8_t a[100];

    audioParams.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::AUDIO_AAC);
    audioParams.PutBuffer(MediaDescriptionKey::MD_KEY_CODEC_CONFIG, a, 100);
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 48000);

    int trackIndex = 0;
    int32_t trackId;

    trackId = muxerDemo->InnerAddTrack(trackIndex, audioParams);
    ASSERT_EQ(0, trackId);


    ret = muxerDemo->InnerStart();
    ASSERT_EQ(AVCS_ERR_OK, ret);

    uint8_t data[100];
    AVCodecBufferInfo info;
    info.presentationTimeUs = 0;
    info.size = 100;
    AVCodecBufferFlag flag = AVCODEC_BUFFER_FLAG_NONE;
    info.offset = 0;
    
    std::shared_ptr<AVSharedMemoryBase> avMemBuffer = std::make_shared<AVSharedMemoryBase>
    (info.size, AVSharedMemory::FLAGS_READ_ONLY, "sampleData");

    avMemBuffer->Init();
    (void)memcpy_s(avMemBuffer->GetBase(), avMemBuffer->GetSize(), data, info.size);

    ret = muxerDemo->InnerWriteSample(trackIndex, avMemBuffer, info, flag);
    ASSERT_EQ(AVCS_ERR_OK, ret);
    
    trackIndex = -1;
    ret = muxerDemo->InnerWriteSample(trackIndex, avMemBuffer, info, flag);
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
    OutputFormat format = OUTPUT_FORMAT_MPEG_4;
    int32_t fd = muxerDemo->InnergetFdByMode(format);
    int32_t ret = muxerDemo->InnerCreate(fd, format);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    MediaDescription audioParams;
    uint8_t a[100];

    audioParams.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::AUDIO_AAC);
    audioParams.PutBuffer(MediaDescriptionKey::MD_KEY_CODEC_CONFIG, a, 100);
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 48000);

    int trackIndex = 0;

    ret = muxerDemo->InnerAddTrack(trackIndex, audioParams);
    ASSERT_EQ(0, ret);
    std::cout << "trackIndex: " << trackIndex << std::endl;
    ret = muxerDemo->InnerStart();
    ASSERT_EQ(AVCS_ERR_OK, ret);

    uint8_t data[100];
    AVCodecBufferInfo info;
    info.presentationTimeUs = 0;
    info.size = 100;
    AVCodecBufferFlag flag = AVCODEC_BUFFER_FLAG_NONE;
    info.offset = 0;
    
    std::shared_ptr<AVSharedMemoryBase> avMemBuffer = std::make_shared<AVSharedMemoryBase>
    (info.size, AVSharedMemory::FLAGS_READ_ONLY, "sampleData");
    avMemBuffer->Init();
    (void)memcpy_s(avMemBuffer->GetBase(), avMemBuffer->GetSize(), data, info.size);
    std::cout << "avMemBuffer: " << avMemBuffer << std::endl;
    ret = muxerDemo->InnerWriteSample(trackIndex, avMemBuffer, info, flag);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    info.presentationTimeUs = -1;
    ret = muxerDemo->InnerWriteSample(trackIndex, avMemBuffer, info, flag);
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
    OutputFormat format = OUTPUT_FORMAT_MPEG_4;
    int32_t fd = muxerDemo->InnergetFdByMode(format);
    int32_t ret = muxerDemo->InnerCreate(fd, format);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    MediaDescription audioParams;
    uint8_t a[100];

    audioParams.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::AUDIO_AAC);
    audioParams.PutBuffer(MediaDescriptionKey::MD_KEY_CODEC_CONFIG, a, 100);
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 48000);

    int trackIndex = 0;
    int32_t trackId;

    trackId = muxerDemo->InnerAddTrack(trackIndex, audioParams);
    ASSERT_EQ(0, trackId);

    ret = muxerDemo->InnerStart();
    ASSERT_EQ(AVCS_ERR_OK, ret);

    uint8_t data[100];
    AVCodecBufferInfo info;
    info.presentationTimeUs = 0;
    info.size = 100;
    AVCodecBufferFlag flag = AVCODEC_BUFFER_FLAG_NONE;
    info.offset = 0;
    
    std::shared_ptr<AVSharedMemoryBase> avMemBuffer = std::make_shared<AVSharedMemoryBase>
    (info.size, AVSharedMemory::FLAGS_READ_ONLY, "sampleData");
    avMemBuffer->Init();
    (void)memcpy_s(avMemBuffer->GetBase(), avMemBuffer->GetSize(), data, info.size);
    ret = muxerDemo->InnerWriteSample(trackIndex, avMemBuffer, info, flag);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    info.size = -1;
    ret = muxerDemo->InnerWriteSample(trackIndex, avMemBuffer, info, flag);
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, ret);

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
    OutputFormat format = OUTPUT_FORMAT_MPEG_4;
    int32_t fd = muxerDemo->InnergetFdByMode(format);
    int32_t ret = muxerDemo->InnerCreate(fd, format);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    MediaDescription audioParams;
    uint8_t a[100];

    audioParams.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::AUDIO_AAC);
    audioParams.PutBuffer(MediaDescriptionKey::MD_KEY_CODEC_CONFIG, a, 100);
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 48000);

    int trackIndex = 0;
    int32_t trackId;

    trackId = muxerDemo->InnerAddTrack(trackIndex, audioParams);
    ASSERT_EQ(0, trackId);

    ret = muxerDemo->InnerStart();
    ASSERT_EQ(AVCS_ERR_OK, ret);

    uint8_t data[100];
    AVCodecBufferInfo info;
    info.presentationTimeUs = 0;
    info.size = 100;
    AVCodecBufferFlag flag = AVCODEC_BUFFER_FLAG_NONE;
    info.offset = 0;
    
    std::shared_ptr<AVSharedMemoryBase> avMemBuffer = std::make_shared<AVSharedMemoryBase>
    (info.size, AVSharedMemory::FLAGS_READ_ONLY, "sampleData");
    avMemBuffer->Init();
    (void)memcpy_s(avMemBuffer->GetBase(), avMemBuffer->GetSize(), data, info.size);
    ret = muxerDemo->InnerWriteSample(trackIndex, avMemBuffer, info, flag);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    info.offset = -1;
    ret = muxerDemo->InnerWriteSample(trackIndex, avMemBuffer, info, flag);
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, ret);

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
    OutputFormat format = OUTPUT_FORMAT_MPEG_4;
    int32_t fd = muxerDemo->InnergetFdByMode(format);
    int32_t ret = muxerDemo->InnerCreate(fd, format);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    MediaDescription audioParams;
    uint8_t a[100];

    audioParams.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::AUDIO_AAC);
    audioParams.PutBuffer(MediaDescriptionKey::MD_KEY_CODEC_CONFIG, a, 100);
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 48000);

    int trackIndex = 0;
    int32_t trackId;

    trackId = muxerDemo->InnerAddTrack(trackIndex, audioParams);
    ASSERT_EQ(0, trackId);

    ret = muxerDemo->InnerStart();
    ASSERT_EQ(AVCS_ERR_OK, ret);

    uint8_t data[100];
    AVCodecBufferInfo info;
    info.presentationTimeUs = 0;
    info.size = 100;
    AVCodecBufferFlag flag = AVCODEC_BUFFER_FLAG_NONE;
    info.offset = 0;
    
    std::shared_ptr<AVSharedMemoryBase> avMemBuffer =
        std::make_shared<AVSharedMemoryBase>(info.size, AVSharedMemory::FLAGS_READ_ONLY, "sampleData");
    avMemBuffer->Init();
    (void)memcpy_s(avMemBuffer->GetBase(), avMemBuffer->GetSize(), data, info.size);
    ret = muxerDemo->InnerWriteSample(trackIndex, avMemBuffer, info, flag);
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
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    OutputFormat format = OUTPUT_FORMAT_MPEG_4;
    int32_t fd = muxerDemo->InnergetFdByMode(format);
    int32_t ret = muxerDemo->InnerCreate(fd, format);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    MediaDescription videoParams;
    uint8_t a[100];

    videoParams.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME,  CodecMimeType::VIDEO_HEVC);
    videoParams.PutBuffer(MediaDescriptionKey::MD_KEY_CODEC_CONFIG, a, 100);
    videoParams.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 352);
    videoParams.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 288);
    videoParams.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, 60);
    videoParams.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_DELAY, 2);
    videoParams.PutIntValue(MediaDescriptionKey::MD_KEY_COLOR_PRIMARIES,
        ColorPrimary::COLOR_PRIMARY_BT709);
    videoParams.PutIntValue(MediaDescriptionKey::MD_KEY_TRANSFER_CHARACTERISTICS,
        TransferCharacteristic::TRANSFER_CHARACTERISTIC_BT709);
    videoParams.PutIntValue(MediaDescriptionKey::MD_KEY_MATRIX_COEFFICIENTS,
        MatrixCoefficient::MATRIX_COEFFICIENT_BT709);
    videoParams.PutIntValue(MediaDescriptionKey::MD_KEY_RANGE_FLAG, 0);
    videoParams.PutIntValue(MediaDescriptionKey::MD_KEY_HDR_TYPE, HDRType::HDR_VIVID);

    int trackIndex = 0;
    int32_t trackId;

    trackId = muxerDemo->InnerAddTrack(trackIndex, videoParams);
    ASSERT_EQ(0, trackId);

    muxerDemo->InnerDestroy();
    delete muxerDemo;
}
