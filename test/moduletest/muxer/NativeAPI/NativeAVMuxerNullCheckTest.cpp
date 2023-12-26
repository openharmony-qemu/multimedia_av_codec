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

using namespace std;
using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;


namespace {
    class NativeAVMuxerNullCheckTest : public testing::Test {
    public:
        static void SetUpTestCase();
        static void TearDownTestCase();
        void SetUp() override;
        void TearDown() override;
    };

    void NativeAVMuxerNullCheckTest::SetUpTestCase() {}
    void NativeAVMuxerNullCheckTest::TearDownTestCase() {}
    void NativeAVMuxerNullCheckTest::SetUp() {}
    void NativeAVMuxerNullCheckTest::TearDown() {}

    constexpr int64_t BITRATE = 32000;
    constexpr int32_t CODEC_CONFIG = 100;
    constexpr int32_t CHANNEL_COUNT = 1;
    constexpr int32_t SAMPLE_RATE = 48000;
    constexpr int32_t PROFILE = 0;
    constexpr int32_t INFO_SIZE = 100;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_NULL_CHECK_001
 * @tc.name      : NativeSetRotation - muxer check
 * @tc.desc      : nullptr test
 */
HWTEST_F(NativeAVMuxerNullCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_NULL_CHECK_001, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    OH_AVOutputFormat format = AV_OUTPUT_FORMAT_M4A;
    int32_t fd = muxerDemo->GetFdByMode(format);
    OH_AVMuxer* handle = muxerDemo->NativeCreate(fd, format);
    ASSERT_NE(nullptr, handle);

    int32_t rotation = 0;

    OH_AVErrCode ret = muxerDemo->NativeSetRotation(nullptr, rotation);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
    muxerDemo->NativeDestroy(handle);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_NULL_CHECK_002
 * @tc.name      : OH_AVMuxer_AddTrack - muxer check
 * @tc.desc      : nullptr test
 */
HWTEST_F(NativeAVMuxerNullCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_NULL_CHECK_002, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    OH_AVOutputFormat format = AV_OUTPUT_FORMAT_M4A;
    int32_t fd = muxerDemo->GetFdByMode(format);
    OH_AVMuxer* handle = muxerDemo->NativeCreate(fd, format);
    ASSERT_NE(nullptr, handle);

    int32_t trackId = -1;

    uint8_t a[100];

    OH_AVFormat* trackFormat = OH_AVFormat_Create();
    OH_AVFormat_SetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_AAC);
    OH_AVFormat_SetLongValue(trackFormat, OH_MD_KEY_BITRATE, BITRATE);
    OH_AVFormat_SetBuffer(trackFormat, OH_MD_KEY_CODEC_CONFIG, a, CODEC_CONFIG);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, SAMPLE_RATE);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_PROFILE, PROFILE);

    OH_AVErrCode ret = muxerDemo->NativeAddTrack(nullptr, &trackId, trackFormat);
    ASSERT_EQ(-1, trackId);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
    muxerDemo->NativeDestroy(handle);
    OH_AVFormat_Destroy(trackFormat);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_NULL_CHECK_003
 * @tc.name      : OH_AVMuxer_AddTrack - trackId check
 * @tc.desc      : nullptr test
 */
HWTEST_F(NativeAVMuxerNullCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_NULL_CHECK_003, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    OH_AVOutputFormat format = AV_OUTPUT_FORMAT_M4A;
    int32_t fd = muxerDemo->GetFdByMode(format);
    OH_AVMuxer* handle = muxerDemo->NativeCreate(fd, format);
    ASSERT_NE(nullptr, handle);

    uint8_t a[100];

    OH_AVFormat* trackFormat = OH_AVFormat_Create();
    OH_AVFormat_SetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_AAC);
    OH_AVFormat_SetLongValue(trackFormat, OH_MD_KEY_BITRATE, BITRATE);
    OH_AVFormat_SetBuffer(trackFormat, OH_MD_KEY_CODEC_CONFIG, a, CODEC_CONFIG);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, SAMPLE_RATE);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_PROFILE, PROFILE);

    OH_AVErrCode ret = muxerDemo->NativeAddTrack(handle, nullptr, trackFormat);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
    muxerDemo->NativeDestroy(handle);
    OH_AVFormat_Destroy(trackFormat);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_NULL_CHECK_004
 * @tc.name      : OH_AVMuxer_AddTrack - trackFormat check
 * @tc.desc      : nullptr test
 */
HWTEST_F(NativeAVMuxerNullCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_NULL_CHECK_004, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    OH_AVOutputFormat format = AV_OUTPUT_FORMAT_M4A;
    int32_t fd = muxerDemo->GetFdByMode(format);
    OH_AVMuxer* handle = muxerDemo->NativeCreate(fd, format);
    ASSERT_NE(nullptr, handle);

    int32_t trackId = -1;

    OH_AVFormat* trackFormat = nullptr;

    OH_AVErrCode ret = muxerDemo->NativeAddTrack(handle, &trackId, trackFormat);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
    muxerDemo->NativeDestroy(handle);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_NULL_CHECK_005
 * @tc.name      : OH_AVMuxer_Start - muxer check
 * @tc.desc      : nullptr test
 */
HWTEST_F(NativeAVMuxerNullCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_NULL_CHECK_005, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    OH_AVOutputFormat format = AV_OUTPUT_FORMAT_MPEG_4;
    int32_t fd = muxerDemo->GetFdByMode(format);
    OH_AVMuxer* handle = muxerDemo->NativeCreate(fd, format);
    ASSERT_NE(nullptr, handle);

    int32_t trackId = -1;

    uint8_t a[100];

    OH_AVFormat* trackFormat = OH_AVFormat_Create();
    OH_AVFormat_SetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_AAC);
    OH_AVFormat_SetLongValue(trackFormat, OH_MD_KEY_BITRATE, BITRATE);
    OH_AVFormat_SetBuffer(trackFormat, OH_MD_KEY_CODEC_CONFIG, a, CODEC_CONFIG);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, SAMPLE_RATE);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_PROFILE, PROFILE);

    OH_AVErrCode ret = muxerDemo->NativeAddTrack(handle, &trackId, trackFormat);
    ASSERT_EQ(0, trackId);

    ret = muxerDemo->NativeStart(nullptr);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    muxerDemo->NativeDestroy(handle);
    OH_AVFormat_Destroy(trackFormat);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_NULL_CHECK_006
 * @tc.name      : OH_AVMuxer_WriteSampleBuffer - muxer check
 * @tc.desc      : nullptr test
 */
HWTEST_F(NativeAVMuxerNullCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_NULL_CHECK_006, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    OH_AVOutputFormat format = AV_OUTPUT_FORMAT_MPEG_4;
    int32_t fd = muxerDemo->GetFdByMode(format);
    OH_AVMuxer* handle = muxerDemo->NativeCreate(fd, format);
    ASSERT_NE(nullptr, handle);

    int32_t trackId = -1;

    uint8_t a[100];

    OH_AVFormat* trackFormat = OH_AVFormat_Create();
    OH_AVFormat_SetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_AAC);
    OH_AVFormat_SetLongValue(trackFormat, OH_MD_KEY_BITRATE, BITRATE);
    OH_AVFormat_SetBuffer(trackFormat, OH_MD_KEY_CODEC_CONFIG, a, CODEC_CONFIG);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, SAMPLE_RATE);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_PROFILE, PROFILE);

    OH_AVErrCode ret = muxerDemo->NativeAddTrack(handle, &trackId, trackFormat);
    ASSERT_EQ(0, trackId);

    ret = muxerDemo->NativeStart(handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVMemory* avMemBuffer = OH_AVMemory_Create(INFO_SIZE);

    OH_AVCodecBufferAttr info;
    info.pts = 0;
    info.size = INFO_SIZE;
    info.offset = 0;
    info.flags = 0;

    ret = muxerDemo->NativeWriteSampleBuffer(nullptr, trackId, avMemBuffer, info);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    OH_AVMemory_Destroy(avMemBuffer);
    muxerDemo->NativeDestroy(handle);
    OH_AVFormat_Destroy(trackFormat);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_NULL_CHECK_007
 * @tc.name      : OH_AVMuxer_WriteSampleBuffer - sampleBuffer check
 * @tc.desc      : nullptr test
 */
HWTEST_F(NativeAVMuxerNullCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_NULL_CHECK_007, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    OH_AVOutputFormat format = AV_OUTPUT_FORMAT_MPEG_4;
    int32_t fd = muxerDemo->GetFdByMode(format);
    OH_AVMuxer* handle = muxerDemo->NativeCreate(fd, format);
    ASSERT_NE(nullptr, handle);

    int32_t trackId = -1;

    uint8_t a[100];

    OH_AVFormat* trackFormat = OH_AVFormat_Create();
    OH_AVFormat_SetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_AAC);
    OH_AVFormat_SetLongValue(trackFormat, OH_MD_KEY_BITRATE, BITRATE);
    OH_AVFormat_SetBuffer(trackFormat, OH_MD_KEY_CODEC_CONFIG, a, CODEC_CONFIG);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, SAMPLE_RATE);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_PROFILE, PROFILE);

    OH_AVErrCode ret = muxerDemo->NativeAddTrack(handle, &trackId, trackFormat);
    ASSERT_EQ(0, trackId);

    ret = muxerDemo->NativeStart(handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVCodecBufferAttr info;
    info.pts = 0;
    info.size = INFO_SIZE;
    info.offset = 0;
    info.flags = 0;

    ret = muxerDemo->NativeWriteSampleBuffer(handle, trackId, nullptr, info);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    muxerDemo->NativeDestroy(handle);
    OH_AVFormat_Destroy(trackFormat);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_NULL_CHECK_008
 * @tc.name      : OH_AVMuxer_Stop - muxer check
 * @tc.desc      : nullptr test
 */
HWTEST_F(NativeAVMuxerNullCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_NULL_CHECK_008, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    OH_AVOutputFormat format = AV_OUTPUT_FORMAT_MPEG_4;
    int32_t fd = muxerDemo->GetFdByMode(format);
    OH_AVMuxer* handle = muxerDemo->NativeCreate(fd, format);
    ASSERT_NE(nullptr, handle);

    int32_t trackId = -1;

    uint8_t a[100];

    OH_AVFormat* trackFormat = OH_AVFormat_Create();
    OH_AVFormat_SetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_AAC);
    OH_AVFormat_SetLongValue(trackFormat, OH_MD_KEY_BITRATE, BITRATE);
    OH_AVFormat_SetBuffer(trackFormat, OH_MD_KEY_CODEC_CONFIG, a, CODEC_CONFIG);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, SAMPLE_RATE);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_PROFILE, PROFILE);

    OH_AVErrCode ret = muxerDemo->NativeAddTrack(handle, &trackId, trackFormat);
    ASSERT_EQ(0, trackId);

    ret = muxerDemo->NativeStart(handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = muxerDemo->NativeStop(nullptr);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    muxerDemo->NativeDestroy(handle);
    OH_AVFormat_Destroy(trackFormat);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_NULL_CHECK_009
 * @tc.name      : OH_AVMuxer_Destroy - muxer check
 * @tc.desc      : nullptr test
 */
HWTEST_F(NativeAVMuxerNullCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_NULL_CHECK_009, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    OH_AVOutputFormat format = AV_OUTPUT_FORMAT_MPEG_4;
    int32_t fd = muxerDemo->GetFdByMode(format);
    OH_AVMuxer* handle = muxerDemo->NativeCreate(fd, format);
    ASSERT_NE(nullptr, handle);

    OH_AVErrCode ret = muxerDemo->NativeDestroy(nullptr);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    muxerDemo->NativeDestroy(handle);
    delete muxerDemo;
}