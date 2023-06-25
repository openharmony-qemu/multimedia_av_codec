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
#include "AVMuxerDemoCommon.h"
#include "plugin_definition.h"
#include "avcodec_info.h"
#include "avcodec_errors.h"
#include "avcodec_common.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::MediaAVCodec::Plugin;
using namespace Ffmpeg;

namespace {
    class FFmpegAVMuxerParamCheckTest : public testing::Test {
    public:
        static void SetUpTestCase();
        static void TearDownTestCase();
        void SetUp() override;
        void TearDown() override;
    };

    void FFmpegAVMuxerParamCheckTest::SetUpTestCase() {}
    void FFmpegAVMuxerParamCheckTest::TearDownTestCase() {}
    void FFmpegAVMuxerParamCheckTest::SetUp() {}
    void FFmpegAVMuxerParamCheckTest::TearDown() {}
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_001
 * @tc.name      : FFmpegCreate - fd check
 * @tc.desc      : param check test
 */
HWTEST_F(FFmpegAVMuxerParamCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_001, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    OutputFormat format = OUTPUT_FORMAT_MPEG_4;
    int32_t fd = -1;
    muxerDemo->FFmpegCreate(fd);
    muxerDemo->FFmpegDestroy();

    fd = muxerDemo->FFmpeggetFdByMode(format);
    muxerDemo->FFmpegCreate(fd);
    muxerDemo->FFmpegDestroy();

    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_003
 * @tc.name      : FFmpegSetRotation - longitude check
 * @tc.desc      : param check test
 */
HWTEST_F(FFmpegAVMuxerParamCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_003, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    OutputFormat format = OUTPUT_FORMAT_MPEG_4;
    int32_t fd = muxerDemo->FFmpeggetFdByMode(format);
    muxerDemo->FFmpegCreate(fd);
  
    int32_t rotation;

    rotation = 0;
    Status ret = muxerDemo->FFmpegSetRotation(rotation);
    ASSERT_EQ(Status::OK, ret);

    rotation = 90;
    ret = muxerDemo->FFmpegSetRotation(rotation);
    ASSERT_EQ(Status::OK, ret);

    rotation = 180;
    ret = muxerDemo->FFmpegSetRotation(rotation);
    ASSERT_EQ(Status::OK, ret);

    rotation = 270;
    ret = muxerDemo->FFmpegSetRotation(rotation);
    ASSERT_EQ(Status::OK, ret);

    rotation = -90;
    ret = muxerDemo->FFmpegSetRotation(rotation);
    ASSERT_EQ(Status::ERROR_INVALID_DATA, ret);

    rotation = 45;
    ret = muxerDemo->FFmpegSetRotation(rotation);
    ASSERT_EQ(Status::ERROR_INVALID_DATA, ret);

    muxerDemo->FFmpegDestroy();
    delete muxerDemo;
}


/*
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_004
 * @tc.name      : FFmpegAddTrack - trackDesc(MD_KEY_CODEC_MIME) check
 * @tc.desc      : param check test
 */
HWTEST_F(FFmpegAVMuxerParamCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_004, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    OutputFormat format = OUTPUT_FORMAT_MPEG_4;
    int32_t fd = muxerDemo->FFmpeggetFdByMode(format);
    muxerDemo->FFmpegCreate(fd);

    MediaDescription trackDesc;


    trackDesc.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::AUDIO_AAC);
    trackDesc.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 48000);
    trackDesc.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
    Status ret;
    int32_t trackId;
    ret = muxerDemo->FFmpegAddTrack(trackId, trackDesc);
    ASSERT_EQ(Status::OK, ret);

    trackDesc.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::AUDIO_MPEG);
    ret = muxerDemo->FFmpegAddTrack(trackId, trackDesc);
    ASSERT_EQ(Status::OK, ret);

    trackDesc.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::AUDIO_FLAC);
    ret = muxerDemo->FFmpegAddTrack(trackId, trackDesc);
    ASSERT_EQ(Status::OK, ret);

    trackDesc.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, "aaaaaa");
    ret = muxerDemo->FFmpegAddTrack(trackId, trackDesc);
    ASSERT_EQ(Status::ERROR_INVALID_DATA, ret);

    trackDesc.PutIntValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, 0);
    ret = muxerDemo->FFmpegAddTrack(trackId, trackDesc);
    ASSERT_EQ(Status::ERROR_MISMATCHED_TYPE, ret);

    trackDesc.PutLongValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, 0);
    ret = muxerDemo->FFmpegAddTrack(trackId, trackDesc);
    ASSERT_EQ(Status::ERROR_MISMATCHED_TYPE, ret);

    trackDesc.PutFloatValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, 0.1);
    ret = muxerDemo->FFmpegAddTrack(trackId, trackDesc);
    ASSERT_EQ(Status::ERROR_MISMATCHED_TYPE, ret);

    trackDesc.PutDoubleValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, 0.1);
    ret = muxerDemo->FFmpegAddTrack(trackId, trackDesc);
    ASSERT_EQ(Status::ERROR_MISMATCHED_TYPE, ret);

    uint8_t b[100];
    trackDesc.PutBuffer(MediaDescriptionKey::MD_KEY_CODEC_MIME, b, 100);
    ret = muxerDemo->FFmpegAddTrack(trackId, trackDesc);
    ASSERT_EQ(Status::ERROR_MISMATCHED_TYPE, ret);

    muxerDemo->FFmpegDestroy();
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_005
 * @tc.name      : FFmpegAddTrack - trackDesc(MD_KEY_CHANNEL_COUNT) check
 * @tc.desc      : param check test
 */
HWTEST_F(FFmpegAVMuxerParamCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_005, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    OutputFormat format = OUTPUT_FORMAT_MPEG_4;
    int32_t fd = muxerDemo->FFmpeggetFdByMode(format);
    muxerDemo->FFmpegCreate(fd);

    MediaDescription audioParams;
    audioParams.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::AUDIO_AAC);
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 48000);

    Status ret;
    int32_t trackId;

    ret = muxerDemo->FFmpegAddTrack(trackId, audioParams);
    ASSERT_EQ(Status::OK, ret);

    audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, -1);
    ret = muxerDemo->FFmpegAddTrack(trackId, audioParams);
    ASSERT_EQ(Status::ERROR_MISMATCHED_TYPE, ret);

    audioParams.PutStringValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, "aaaaaa");
    ret = muxerDemo->FFmpegAddTrack(trackId, audioParams);
    ASSERT_EQ(Status::ERROR_MISMATCHED_TYPE, ret);

    audioParams.PutLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 0);
    ret = muxerDemo->FFmpegAddTrack(trackId, audioParams);
    ASSERT_EQ(Status::ERROR_MISMATCHED_TYPE, ret);

    audioParams.PutFloatValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 0.1);
    ret = muxerDemo->FFmpegAddTrack(trackId, audioParams);
    ASSERT_EQ(Status::ERROR_MISMATCHED_TYPE, ret);

    audioParams.PutDoubleValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 0.1);
    ret = muxerDemo->FFmpegAddTrack(trackId, audioParams);
    ASSERT_EQ(Status::ERROR_MISMATCHED_TYPE, ret);

    uint8_t b[100];
    audioParams.PutBuffer(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, b, 100);
    ret = muxerDemo->FFmpegAddTrack(trackId, audioParams);
    ASSERT_EQ(Status::ERROR_MISMATCHED_TYPE, ret);

    muxerDemo->FFmpegDestroy();
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_006
 * @tc.name      : FFmpegAddTrack - MediaDescription(MD_KEY_SAMPLE_RATE) check
 * @tc.desc      : param check test
 */
HWTEST_F(FFmpegAVMuxerParamCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_006, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    OutputFormat format = OUTPUT_FORMAT_MPEG_4;
    int32_t fd = muxerDemo->FFmpeggetFdByMode(format);
    muxerDemo->FFmpegCreate(fd);

    MediaDescription trackDesc;
    trackDesc.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::AUDIO_AAC);
    trackDesc.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
    trackDesc.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 48000);

    Status ret;
    int32_t trackId;
    ret = muxerDemo->FFmpegAddTrack(trackId, trackDesc);
    ASSERT_EQ(Status::OK, ret);

    trackDesc.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, -1);
    ret = muxerDemo->FFmpegAddTrack(trackId, trackDesc);
    ASSERT_EQ(Status::ERROR_MISMATCHED_TYPE, ret);

    trackDesc.PutStringValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, "aaaaaa");
    ret = muxerDemo->FFmpegAddTrack(trackId, trackDesc);
    ASSERT_EQ(Status::ERROR_MISMATCHED_TYPE, ret);

    trackDesc.PutLongValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 0);
    ret = muxerDemo->FFmpegAddTrack(trackId, trackDesc);
    ASSERT_EQ(Status::ERROR_MISMATCHED_TYPE, ret);

    trackDesc.PutFloatValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 0.1);
    ret = muxerDemo->FFmpegAddTrack(trackId, trackDesc);
    ASSERT_EQ(Status::ERROR_MISMATCHED_TYPE, ret);

    trackDesc.PutDoubleValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 0.1);
    ret = muxerDemo->FFmpegAddTrack(trackId, trackDesc);
    ASSERT_EQ(Status::ERROR_MISMATCHED_TYPE, ret);

    uint8_t b[100];
    trackDesc.PutBuffer(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, b, 100);
    ret = muxerDemo->FFmpegAddTrack(trackId, trackDesc);
    ASSERT_EQ(Status::ERROR_MISMATCHED_TYPE, ret);

    muxerDemo->FFmpegDestroy();
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_007
 * @tc.name      : FFmpegAddTrack - video trackDesc(MD_KEY_CODEC_MIME) check
 * @tc.desc      : param check test
 */
HWTEST_F(FFmpegAVMuxerParamCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_007, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    OutputFormat format = OUTPUT_FORMAT_MPEG_4;
    int32_t fd = muxerDemo->FFmpeggetFdByMode(format);
    muxerDemo->FFmpegCreate(fd);

    MediaDescription trackDesc;
    trackDesc.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_AVC);
    trackDesc.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 352);
    trackDesc.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 288);

    Status ret;
    int32_t trackId;

    ret = muxerDemo->FFmpegAddTrack(trackId, trackDesc);
    ASSERT_EQ(Status::OK, ret);

    trackDesc.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_MPEG4);
    ret = muxerDemo->FFmpegAddTrack(trackId, trackDesc);
    ASSERT_EQ(1, trackId);

    trackDesc.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::IMAGE_PNG);
    ret = muxerDemo->FFmpegAddTrack(trackId, trackDesc);
    ASSERT_EQ(Status::OK, ret);

    trackDesc.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::IMAGE_BMP);
    ret = muxerDemo->FFmpegAddTrack(trackId, trackDesc);
    ASSERT_EQ(Status::OK, ret);

    trackDesc.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::IMAGE_JPG);
    ret = muxerDemo->FFmpegAddTrack(trackId, trackDesc);
    ASSERT_EQ(Status::OK, ret);

    trackDesc.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_HEVC);
    ret = muxerDemo->FFmpegAddTrack(trackId, trackDesc);
    ASSERT_EQ(Status::ERROR_INVALID_DATA, ret);

    trackDesc.PutIntValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, 0);
    ret = muxerDemo->FFmpegAddTrack(trackId, trackDesc);
    ASSERT_EQ(Status::ERROR_MISMATCHED_TYPE, ret);

    trackDesc.PutLongValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, 0);
    ret = muxerDemo->FFmpegAddTrack(trackId, trackDesc);
    ASSERT_EQ(Status::ERROR_MISMATCHED_TYPE, ret);

    trackDesc.PutFloatValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, 0.1);
    ret = muxerDemo->FFmpegAddTrack(trackId, trackDesc);
    ASSERT_EQ(Status::ERROR_MISMATCHED_TYPE, ret);

    trackDesc.PutDoubleValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, 0.1);
    ret = muxerDemo->FFmpegAddTrack(trackId, trackDesc);
    ASSERT_EQ(Status::ERROR_MISMATCHED_TYPE, ret);

    uint8_t b[100];
    trackDesc.PutBuffer(MediaDescriptionKey::MD_KEY_CODEC_MIME, b, 100);
    ret = muxerDemo->FFmpegAddTrack(trackId, trackDesc);
    ASSERT_EQ(Status::ERROR_MISMATCHED_TYPE, ret);

    muxerDemo->FFmpegDestroy();
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_008
 * @tc.name      : FFmpegAddTrack - video trackDesc(MD_KEY_WIDTH) check
 * @tc.desc      : param check test
 */
HWTEST_F(FFmpegAVMuxerParamCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_008, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    OutputFormat format = OUTPUT_FORMAT_MPEG_4;
    int32_t fd = muxerDemo->FFmpeggetFdByMode(format);
    muxerDemo->FFmpegCreate(fd);

    MediaDescription trackDesc;
    trackDesc.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_AVC);
  
    trackDesc.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 352);
    trackDesc.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 288);

    Status ret;
    int32_t trackId;

    ret = muxerDemo->FFmpegAddTrack(trackId, trackDesc);
    ASSERT_EQ(Status::OK, ret);

    trackDesc.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, -1);
    ret = muxerDemo->FFmpegAddTrack(trackId, trackDesc);
    ASSERT_EQ(Status::ERROR_MISMATCHED_TYPE, ret);

    trackDesc.PutStringValue(MediaDescriptionKey::MD_KEY_WIDTH, "aaa");
    ret = muxerDemo->FFmpegAddTrack(trackId, trackDesc);
    ASSERT_EQ(Status::ERROR_MISMATCHED_TYPE, ret);

    trackDesc.PutLongValue(MediaDescriptionKey::MD_KEY_WIDTH, 0);
    ret = muxerDemo->FFmpegAddTrack(trackId, trackDesc);
    ASSERT_EQ(Status::ERROR_MISMATCHED_TYPE, ret);

    trackDesc.PutFloatValue(MediaDescriptionKey::MD_KEY_WIDTH, 0.1);
    ret = muxerDemo->FFmpegAddTrack(trackId, trackDesc);
    ASSERT_EQ(Status::ERROR_MISMATCHED_TYPE, ret);

    trackDesc.PutDoubleValue(MediaDescriptionKey::MD_KEY_WIDTH, 0.1);
    ret = muxerDemo->FFmpegAddTrack(trackId, trackDesc);
    ASSERT_EQ(Status::ERROR_MISMATCHED_TYPE, ret);

    uint8_t b[100];
    trackDesc.PutBuffer(MediaDescriptionKey::MD_KEY_WIDTH, b, 100);
    ret = muxerDemo->FFmpegAddTrack(trackId, trackDesc);
    ASSERT_EQ(Status::ERROR_MISMATCHED_TYPE, ret);

    muxerDemo->FFmpegDestroy();
    delete muxerDemo;
}
/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_009
 * @tc.name      : FFmpegAddTrack - video trackDesc(MD_KEY_HEIGHT) check
 * @tc.desc      : param check test
 */
HWTEST_F(FFmpegAVMuxerParamCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_009, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    OutputFormat format = OUTPUT_FORMAT_MPEG_4;
    int32_t fd = muxerDemo->FFmpeggetFdByMode(format);
    muxerDemo->FFmpegCreate(fd);

    MediaDescription trackDesc;
    trackDesc.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_AVC);
  
    trackDesc.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 352);
    trackDesc.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 288);

    Status ret;
    int32_t trackId;

    ret = muxerDemo->FFmpegAddTrack(trackId, trackDesc);
    ASSERT_EQ(Status::OK, ret);

    trackDesc.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, -1);
    ret = muxerDemo->FFmpegAddTrack(trackId, trackDesc);
    ASSERT_EQ(Status::ERROR_MISMATCHED_TYPE, ret);

    trackDesc.PutStringValue(MediaDescriptionKey::MD_KEY_HEIGHT, "aaa");
    ret = muxerDemo->FFmpegAddTrack(trackId, trackDesc);
    ASSERT_EQ(Status::ERROR_MISMATCHED_TYPE, ret);

    trackDesc.PutLongValue(MediaDescriptionKey::MD_KEY_HEIGHT, 0);
    ret = muxerDemo->FFmpegAddTrack(trackId, trackDesc);
    ASSERT_EQ(Status::ERROR_MISMATCHED_TYPE, ret);

    trackDesc.PutFloatValue(MediaDescriptionKey::MD_KEY_HEIGHT, 0.1);
    ret = muxerDemo->FFmpegAddTrack(trackId, trackDesc);
    ASSERT_EQ(Status::ERROR_MISMATCHED_TYPE, ret);

    trackDesc.PutDoubleValue(MediaDescriptionKey::MD_KEY_HEIGHT, 0.1);
    ret = muxerDemo->FFmpegAddTrack(trackId, trackDesc);
    ASSERT_EQ(Status::ERROR_MISMATCHED_TYPE, ret);

    uint8_t b[100];
    trackDesc.PutBuffer(MediaDescriptionKey::MD_KEY_HEIGHT, b, 100);
    ret = muxerDemo->FFmpegAddTrack(trackId, trackDesc);
    ASSERT_EQ(Status::ERROR_MISMATCHED_TYPE, ret);

    muxerDemo->FFmpegDestroy();
    delete muxerDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_010
 * @tc.name      : AddTrack - trackDesc(any key) check
 * @tc.desc      : param check test
 */
HWTEST_F(FFmpegAVMuxerParamCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_010, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    OutputFormat format = OUTPUT_FORMAT_MPEG_4;
    int32_t fd = muxerDemo->FFmpeggetFdByMode(format);
    muxerDemo->FFmpegCreate(fd);

    MediaDescription trackDesc;
    trackDesc.PutStringValue("aaaaa", "bbbbb");

    Status ret;
    int32_t trackId;

    ret = muxerDemo->FFmpegAddTrack(trackId, trackDesc);
    ASSERT_EQ(Status::ERROR_MISMATCHED_TYPE, ret);

    muxerDemo->FFmpegDestroy();
    delete muxerDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_011
 * @tc.name      : WriteSampleBuffer - trackIndex check
 * @tc.desc      : param check test
 */
HWTEST_F(FFmpegAVMuxerParamCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_011, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    OutputFormat format = OUTPUT_FORMAT_MPEG_4;
    int32_t fd = muxerDemo->FFmpeggetFdByMode(format);
    muxerDemo->FFmpegCreate(fd);

    MediaDescription trackDesc;
    trackDesc.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::AUDIO_AAC);
    trackDesc.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
    trackDesc.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 48000);

    Status ret;
    int32_t trackId;
    trackId = 0;
    ret = muxerDemo->FFmpegAddTrack(trackId, trackDesc);
    ASSERT_EQ(Status::OK, ret);

    ret = muxerDemo->FFmpegStart();
    ASSERT_EQ(Status::OK, ret);

    uint8_t data[100];

    AVCodecBufferInfo info;
    AVCodecBufferFlag flag;
    info.presentationTimeUs= 0;
    info.size = 100;
    uint32_t trackIndex = 0;
    flag = AVCODEC_BUFFER_FLAG_NONE;
    info.offset = 0;

    ret = muxerDemo->FFmpegWriteSample(trackIndex, data, info, flag);
    ASSERT_EQ(Status::OK, ret);

    trackId = -1;
    ret = muxerDemo->FFmpegWriteSample(trackIndex, data, info, flag);
    ASSERT_EQ(Status::OK, ret);

    muxerDemo->FFmpegDestroy();
    delete muxerDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_012
 * @tc.name      : FFmpegWriteSampleBuffer - info.presentationTimeUscheck
 * @tc.desc      : param check test
 */
HWTEST_F(FFmpegAVMuxerParamCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_012, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    OutputFormat format = OUTPUT_FORMAT_MPEG_4;
    int32_t fd = muxerDemo->FFmpeggetFdByMode(format);
    muxerDemo->FFmpegCreate(fd);

    MediaDescription trackDesc;
    trackDesc.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::AUDIO_AAC);
    trackDesc.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
    trackDesc.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 48000);

    Status ret;
    int32_t trackId;

    ret = muxerDemo->FFmpegAddTrack(trackId, trackDesc);
    ASSERT_EQ(Status::OK, ret);

    ret = muxerDemo->FFmpegStart();
    ASSERT_EQ(Status::OK, ret);

    uint8_t data[100];

    AVCodecBufferInfo info;
    AVCodecBufferFlag flag;
    info.presentationTimeUs= 0;
    info.size = 100;
    uint32_t trackIndex = 0;
    flag = AVCODEC_BUFFER_FLAG_NONE;
    info.offset = 0;

    ret = muxerDemo->FFmpegWriteSample(trackIndex, data, info, flag);
    ASSERT_EQ(Status::OK, ret);

    info.presentationTimeUs= -1;
    ret = muxerDemo->FFmpegWriteSample(trackIndex, data, info, flag);
    ASSERT_EQ(Status::OK, ret);

    muxerDemo->FFmpegDestroy();
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_013
 * @tc.name      : FFmpegWriteSampleBuffer - info.size check
 * @tc.desc      : param check test
 */
HWTEST_F(FFmpegAVMuxerParamCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_013, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    OutputFormat format = OUTPUT_FORMAT_MPEG_4;
    int32_t fd = muxerDemo->FFmpeggetFdByMode(format);
    muxerDemo->FFmpegCreate(fd);

    MediaDescription trackDesc;
    trackDesc.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::AUDIO_AAC);

    trackDesc.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
    trackDesc.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 48000);

    Status ret;
    int32_t trackId;

    ret = muxerDemo->FFmpegAddTrack(trackId, trackDesc);
    ASSERT_EQ(Status::OK, ret);

    ret = muxerDemo->FFmpegStart();
    ASSERT_EQ(Status::OK, ret);

    uint8_t data[100];

    AVCodecBufferInfo info;
    AVCodecBufferFlag flag;
    info.presentationTimeUs= 0;
    info.size = 100;
    uint32_t trackIndex = 0;
    flag = AVCODEC_BUFFER_FLAG_NONE;
    info.offset = 0;

    ret = muxerDemo->FFmpegWriteSample(trackIndex, data, info, flag);
    ASSERT_EQ(Status::OK, ret);

    info.size = -1;
    ret = muxerDemo->FFmpegWriteSample(trackIndex, data, info, flag);
    ASSERT_EQ(Status::OK, ret);

    muxerDemo->FFmpegDestroy();
    delete muxerDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_014
 * @tc.name      : FFmpegWriteSampleBuffer -  info.trackIndex check
 * @tc.desc      : param check test
 */
HWTEST_F(FFmpegAVMuxerParamCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_014, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    OutputFormat format = OUTPUT_FORMAT_MPEG_4;
    int32_t fd = muxerDemo->FFmpeggetFdByMode(format);
    muxerDemo->FFmpegCreate(fd);

    MediaDescription trackDesc;
    trackDesc.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::AUDIO_AAC);

    trackDesc.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
    trackDesc.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 48000);


    Status ret;
    int32_t trackId;

    ret = muxerDemo->FFmpegAddTrack(trackId, trackDesc);
    ASSERT_EQ(Status::OK, ret);

    ret = muxerDemo->FFmpegStart();
    ASSERT_EQ(Status::OK, ret);

    uint8_t data[100];

    AVCodecBufferInfo info;
    AVCodecBufferFlag flag;
    info.presentationTimeUs= 0;
    info.size = 100;
    uint32_t trackIndex = 0;
    flag = AVCODEC_BUFFER_FLAG_NONE;
    info.offset = 0;

    ret = muxerDemo->FFmpegWriteSample(trackIndex, data, info, flag);
    ASSERT_EQ(Status::OK, ret);

    info.offset = -1;
    ret = muxerDemo->FFmpegWriteSample(trackIndex, data, info, flag);
    ASSERT_EQ(Status::ERROR_MISMATCHED_TYPE, ret);

    muxerDemo->FFmpegDestroy();
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_015
 * @tc.name      : FFmpegWriteSampleBuffer - flag check
 * @tc.desc      : param check test
 */
HWTEST_F(FFmpegAVMuxerParamCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_015, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    OutputFormat format = OUTPUT_FORMAT_MPEG_4;
    int32_t fd = muxerDemo->FFmpeggetFdByMode(format);
    muxerDemo->FFmpegCreate(fd);

    MediaDescription trackDesc;
    trackDesc.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::AUDIO_AAC);

    trackDesc.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
    trackDesc.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 48000);


    Status ret;
    int32_t trackId;

    ret = muxerDemo->FFmpegAddTrack(trackId, trackDesc);
    ASSERT_EQ(Status::OK, ret);

    ret = muxerDemo->FFmpegStart();
    ASSERT_EQ(Status::OK, ret);

    uint8_t data[100];

    AVCodecBufferInfo info;
    AVCodecBufferFlag flag;
    info.presentationTimeUs= 0;
    info.size = 100;
    uint32_t trackIndex = 0;
    flag = AVCODEC_BUFFER_FLAG_NONE;
    info.offset = 0;

    ret = muxerDemo->FFmpegWriteSample(trackIndex, data, info, flag);
    ASSERT_EQ(Status::OK, ret);

    muxerDemo->FFmpegDestroy();
    delete muxerDemo;
}
