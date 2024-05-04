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
#include "native_avbuffer.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;

namespace {
class NativeAVMuxerParamCheckTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp() override;
    void TearDown() override;
};

void NativeAVMuxerParamCheckTest::SetUpTestCase() {}
void NativeAVMuxerParamCheckTest::TearDownTestCase() {}
void NativeAVMuxerParamCheckTest::SetUp() {}
void NativeAVMuxerParamCheckTest::TearDown() {}

constexpr int32_t ROTATION_0 = 0;
constexpr int32_t ROTATION_90 = 90;
constexpr int32_t ROTATION_180 = 180;
constexpr int32_t ROTATION_270 = 270;
constexpr int32_t ROTATION_ERROR = -90;
constexpr int32_t ROTATION_45 = 45;

constexpr int64_t AUDIO_BITRATE = 320000;
constexpr int64_t VIDEO_BITRATE = 524569;
constexpr int32_t CODEC_CONFIG = 100;
constexpr int32_t CHANNEL_COUNT = 1;
constexpr int32_t SAMPLE_RATE = 48000;
constexpr int32_t PROFILE = 0;
constexpr int32_t INFO_SIZE = 100;

constexpr int32_t WIDTH = 352;
constexpr int32_t HEIGHT = 288;
constexpr double FRAME_RATE = 60;
const std::string HEVC_LIB_PATH = std::string(AV_CODEC_PATH) + "/libav_codec_hevc_parser.z.so";
} // namespace

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_001
 * @tc.name      : OH_AVMuxer_Create - fd check
 * @tc.desc      : param check test
 */
HWTEST_F(NativeAVMuxerParamCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_001, TestSize.Level2)
{
    AVMuxerDemo *muxerDemo = new AVMuxerDemo();
    OH_AVOutputFormat format = AV_OUTPUT_FORMAT_M4A;
    int32_t fd = -1;
    OH_AVMuxer *handle = muxerDemo->NativeCreate(fd, format);
    ASSERT_EQ(nullptr, handle);

    fd = muxerDemo->GetErrorFd();
    handle = muxerDemo->NativeCreate(fd, format);
    ASSERT_EQ(nullptr, handle);

    fd = muxerDemo->GetFdByMode(format);
    handle = muxerDemo->NativeCreate(fd, format);
    ASSERT_NE(nullptr, handle);
    muxerDemo->NativeDestroy(handle);
    delete muxerDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_002
 * @tc.name      : OH_AVMuxer_Create - format check
 * @tc.desc      : param check test
 */
HWTEST_F(NativeAVMuxerParamCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_002, TestSize.Level2)
{
    AVMuxerDemo *muxerDemo = new AVMuxerDemo();
    OH_AVOutputFormat format = AV_OUTPUT_FORMAT_DEFAULT;
    int32_t fd = muxerDemo->GetFdByMode(format);
    OH_AVMuxer *handle = muxerDemo->NativeCreate(fd, format);
    ASSERT_NE(nullptr, handle);
    muxerDemo->NativeDestroy(handle);
    handle = nullptr;

    format = AV_OUTPUT_FORMAT_MPEG_4;
    fd = muxerDemo->GetFdByMode(format);
    handle = muxerDemo->NativeCreate(fd, format);
    ASSERT_NE(nullptr, handle);
    muxerDemo->NativeDestroy(handle);
    handle = nullptr;

    format = AV_OUTPUT_FORMAT_M4A;
    fd = muxerDemo->GetFdByMode(format);
    handle = muxerDemo->NativeCreate(fd, format);
    ASSERT_NE(nullptr, handle);
    muxerDemo->NativeDestroy(handle);
    handle = nullptr;
    delete muxerDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_003
 * @tc.name      : OH_AVMuxer_SetRotation - rotation check
 * @tc.desc      : param check test
 */
HWTEST_F(NativeAVMuxerParamCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_003, TestSize.Level2)
{
    AVMuxerDemo *muxerDemo = new AVMuxerDemo();
    OH_AVOutputFormat format = AV_OUTPUT_FORMAT_MPEG_4;
    int32_t fd = muxerDemo->GetFdByMode(format);
    OH_AVMuxer *handle = muxerDemo->NativeCreate(fd, format);
    ASSERT_NE(nullptr, handle);

    int32_t rotation;

    rotation = ROTATION_0;
    OH_AVErrCode ret = muxerDemo->NativeSetRotation(handle, rotation);
    ASSERT_EQ(AV_ERR_OK, ret);

    rotation = ROTATION_90;
    ret = muxerDemo->NativeSetRotation(handle, rotation);
    ASSERT_EQ(AV_ERR_OK, ret);

    rotation = ROTATION_180;
    ret = muxerDemo->NativeSetRotation(handle, rotation);
    ASSERT_EQ(AV_ERR_OK, ret);

    rotation = ROTATION_270;
    ret = muxerDemo->NativeSetRotation(handle, rotation);
    ASSERT_EQ(AV_ERR_OK, ret);

    rotation = ROTATION_ERROR;
    ret = muxerDemo->NativeSetRotation(handle, rotation);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    rotation = ROTATION_45;
    ret = muxerDemo->NativeSetRotation(handle, rotation);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    muxerDemo->NativeDestroy(handle);
    handle = nullptr;
    delete muxerDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_004
 * @tc.name      : OH_AVMuxer_AddTrack - trackFormat(OH_MD_KEY_CODEC_MIME) check
 * @tc.desc      : param check test
 */
HWTEST_F(NativeAVMuxerParamCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_004, TestSize.Level2)
{
    AVMuxerDemo *muxerDemo = new AVMuxerDemo();
    OH_AVOutputFormat format = AV_OUTPUT_FORMAT_MPEG_4;
    int32_t fd = muxerDemo->GetFdByMode(format);
    OH_AVMuxer *handle = muxerDemo->NativeCreate(fd, format);
    ASSERT_NE(nullptr, handle);

    uint8_t a[CODEC_CONFIG];

    OH_AVFormat *trackFormat = OH_AVFormat_Create();
    OH_AVFormat_SetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_AAC);
    OH_AVFormat_SetLongValue(trackFormat, OH_MD_KEY_BITRATE, AUDIO_BITRATE);
    OH_AVFormat_SetBuffer(trackFormat, OH_MD_KEY_CODEC_CONFIG, a, CODEC_CONFIG);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, SAMPLE_RATE);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_PROFILE, PROFILE);

    int32_t trackId;

    OH_AVErrCode ret = muxerDemo->NativeAddTrack(handle, &trackId, trackFormat);
    ASSERT_EQ(0, trackId);

    OH_AVFormat_SetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_MPEG);
    ret = muxerDemo->NativeAddTrack(handle, &trackId, trackFormat);
    ASSERT_EQ(1, trackId);

    OH_AVFormat_SetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, "aaaaaa");
    ret = muxerDemo->NativeAddTrack(handle, &trackId, trackFormat);
    ASSERT_NE(AV_ERR_OK, ret);

    muxerDemo->NativeDestroy(handle);
    OH_AVFormat_Destroy(trackFormat);
    handle = nullptr;
    delete muxerDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_005
 * @tc.name      : OH_AVMuxer_AddTrack - trackFormat(OH_MD_KEY_AUD_CHANNEL_COUNT) check
 * @tc.desc      : param check test
 */
HWTEST_F(NativeAVMuxerParamCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_005, TestSize.Level2)
{
    AVMuxerDemo *muxerDemo = new AVMuxerDemo();
    OH_AVOutputFormat format = AV_OUTPUT_FORMAT_MPEG_4;
    int32_t fd = muxerDemo->GetFdByMode(format);
    OH_AVMuxer *handle = muxerDemo->NativeCreate(fd, format);
    ASSERT_NE(nullptr, handle);

    uint8_t a[CODEC_CONFIG];

    OH_AVFormat *trackFormat = OH_AVFormat_Create();
    OH_AVFormat_SetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_AAC);
    OH_AVFormat_SetLongValue(trackFormat, OH_MD_KEY_BITRATE, AUDIO_BITRATE);
    OH_AVFormat_SetBuffer(trackFormat, OH_MD_KEY_CODEC_CONFIG, a, CODEC_CONFIG);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, SAMPLE_RATE);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_PROFILE, PROFILE);

    int32_t trackId;

    OH_AVErrCode ret = muxerDemo->NativeAddTrack(handle, &trackId, trackFormat);
    ASSERT_EQ(0, trackId);

    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, -1);
    ret = muxerDemo->NativeAddTrack(handle, &trackId, trackFormat);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    OH_AVFormat_SetStringValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, "aaaaaa");
    ret = muxerDemo->NativeAddTrack(handle, &trackId, trackFormat);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    OH_AVFormat_SetLongValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, 0);
    ret = muxerDemo->NativeAddTrack(handle, &trackId, trackFormat);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    OH_AVFormat_SetFloatValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, 0.1);
    ret = muxerDemo->NativeAddTrack(handle, &trackId, trackFormat);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    OH_AVFormat_SetDoubleValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, 0.1);
    ret = muxerDemo->NativeAddTrack(handle, &trackId, trackFormat);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    uint8_t b[100];
    OH_AVFormat_SetBuffer(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, b, 100);
    ret = muxerDemo->NativeAddTrack(handle, &trackId, trackFormat);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    muxerDemo->NativeDestroy(handle);
    OH_AVFormat_Destroy(trackFormat);
    handle = nullptr;
    delete muxerDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_006
 * @tc.name      : OH_AVMuxer_AddTrack - trackFormat(OH_MD_KEY_AUD_SAMPLE_RATE) check
 * @tc.desc      : param check test
 */
HWTEST_F(NativeAVMuxerParamCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_006, TestSize.Level2)
{
    AVMuxerDemo *muxerDemo = new AVMuxerDemo();
    OH_AVOutputFormat format = AV_OUTPUT_FORMAT_MPEG_4;
    int32_t fd = muxerDemo->GetFdByMode(format);
    OH_AVMuxer *handle = muxerDemo->NativeCreate(fd, format);
    ASSERT_NE(nullptr, handle);

    uint8_t a[CODEC_CONFIG];

    OH_AVFormat *trackFormat = OH_AVFormat_Create();
    OH_AVFormat_SetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_AAC);
    OH_AVFormat_SetLongValue(trackFormat, OH_MD_KEY_BITRATE, AUDIO_BITRATE);
    OH_AVFormat_SetBuffer(trackFormat, OH_MD_KEY_CODEC_CONFIG, a, CODEC_CONFIG);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, SAMPLE_RATE);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_PROFILE, PROFILE);

    int32_t trackId;

    OH_AVErrCode ret = muxerDemo->NativeAddTrack(handle, &trackId, trackFormat);
    ASSERT_EQ(0, trackId);

    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, -1);
    ret = muxerDemo->NativeAddTrack(handle, &trackId, trackFormat);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    OH_AVFormat_SetStringValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, "aaaaaa");
    ret = muxerDemo->NativeAddTrack(handle, &trackId, trackFormat);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    OH_AVFormat_SetLongValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, 0);
    ret = muxerDemo->NativeAddTrack(handle, &trackId, trackFormat);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    OH_AVFormat_SetFloatValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, 0.1);
    ret = muxerDemo->NativeAddTrack(handle, &trackId, trackFormat);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    OH_AVFormat_SetDoubleValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, 0.1);
    ret = muxerDemo->NativeAddTrack(handle, &trackId, trackFormat);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    uint8_t b[100];
    OH_AVFormat_SetBuffer(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, b, 100);
    ret = muxerDemo->NativeAddTrack(handle, &trackId, trackFormat);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    muxerDemo->NativeDestroy(handle);
    OH_AVFormat_Destroy(trackFormat);
    handle = nullptr;
    delete muxerDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_007
 * @tc.name      : OH_AVMuxer_AddTrack - video trackFormat(OH_MD_KEY_CODEC_MIME) check
 * @tc.desc      : param check test
 */
HWTEST_F(NativeAVMuxerParamCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_007, TestSize.Level2)
{
    AVMuxerDemo *muxerDemo = new AVMuxerDemo();
    OH_AVOutputFormat format = AV_OUTPUT_FORMAT_MPEG_4;
    int32_t fd = muxerDemo->GetFdByMode(format);
    OH_AVMuxer *handle = muxerDemo->NativeCreate(fd, format);
    ASSERT_NE(nullptr, handle);

    uint8_t a[CODEC_CONFIG];

    OH_AVFormat *trackFormat = OH_AVFormat_Create();
    OH_AVFormat_SetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    OH_AVFormat_SetLongValue(trackFormat, OH_MD_KEY_BITRATE, VIDEO_BITRATE);
    OH_AVFormat_SetBuffer(trackFormat, OH_MD_KEY_CODEC_CONFIG, a, CODEC_CONFIG);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_YUVI420);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_WIDTH, WIDTH);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_HEIGHT, HEIGHT);

    int32_t trackId;

    OH_AVErrCode ret = muxerDemo->NativeAddTrack(handle, &trackId, trackFormat);
    ASSERT_EQ(AV_ERR_OK, ret);
    ASSERT_EQ(0, trackId);

    OH_AVFormat_SetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_VIDEO_MPEG4);
    ret = muxerDemo->NativeAddTrack(handle, &trackId, trackFormat);
    ASSERT_EQ(AV_ERR_OK, ret);
    ASSERT_EQ(1, trackId);

    if (access(HEVC_LIB_PATH.c_str(), F_OK) == 0) {
        OH_AVFormat_SetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_VIDEO_HEVC);
        ret = muxerDemo->NativeAddTrack(handle, &trackId, trackFormat);
        ASSERT_EQ(AV_ERR_OK, ret);
        ASSERT_EQ(2, trackId);
    }

    muxerDemo->NativeDestroy(handle);
    OH_AVFormat_Destroy(trackFormat);
    handle = nullptr;
    delete muxerDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_008
 * @tc.name      : OH_AVMuxer_AddTrack - video trackFormat(OH_MD_KEY_WIDTH) check
 * @tc.desc      : param check test
 */
HWTEST_F(NativeAVMuxerParamCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_008, TestSize.Level2)
{
    AVMuxerDemo *muxerDemo = new AVMuxerDemo();
    OH_AVOutputFormat format = AV_OUTPUT_FORMAT_MPEG_4;
    int32_t fd = muxerDemo->GetFdByMode(format);
    OH_AVMuxer *handle = muxerDemo->NativeCreate(fd, format);
    ASSERT_NE(nullptr, handle);

    uint8_t a[CODEC_CONFIG];

    OH_AVFormat *trackFormat = OH_AVFormat_Create();
    OH_AVFormat_SetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    OH_AVFormat_SetLongValue(trackFormat, OH_MD_KEY_BITRATE, 524569);
    OH_AVFormat_SetBuffer(trackFormat, OH_MD_KEY_CODEC_CONFIG, a, CODEC_CONFIG);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_YUVI420);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_WIDTH, WIDTH);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_HEIGHT, HEIGHT);
    OH_AVFormat_SetDoubleValue(trackFormat, OH_MD_KEY_FRAME_RATE, FRAME_RATE);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_PROFILE, PROFILE);

    int32_t trackId;

    OH_AVErrCode ret = muxerDemo->NativeAddTrack(handle, &trackId, trackFormat);
    ASSERT_EQ(0, trackId);

    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_WIDTH, -1);
    ret = muxerDemo->NativeAddTrack(handle, &trackId, trackFormat);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    OH_AVFormat_SetStringValue(trackFormat, OH_MD_KEY_WIDTH, "aaa");
    ret = muxerDemo->NativeAddTrack(handle, &trackId, trackFormat);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    OH_AVFormat_SetLongValue(trackFormat, OH_MD_KEY_WIDTH, 0);
    ret = muxerDemo->NativeAddTrack(handle, &trackId, trackFormat);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    OH_AVFormat_SetFloatValue(trackFormat, OH_MD_KEY_WIDTH, 0.1);
    ret = muxerDemo->NativeAddTrack(handle, &trackId, trackFormat);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    OH_AVFormat_SetDoubleValue(trackFormat, OH_MD_KEY_WIDTH, 0.1);
    ret = muxerDemo->NativeAddTrack(handle, &trackId, trackFormat);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    uint8_t b[100];
    OH_AVFormat_SetBuffer(trackFormat, OH_MD_KEY_WIDTH, b, 100);
    ret = muxerDemo->NativeAddTrack(handle, &trackId, trackFormat);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    muxerDemo->NativeDestroy(handle);
    OH_AVFormat_Destroy(trackFormat);
    handle = nullptr;
    delete muxerDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_009
 * @tc.name      : OH_AVMuxer_AddTrack - video trackFormat(OH_MD_KEY_HEIGHT) check
 * @tc.desc      : param check test
 */
HWTEST_F(NativeAVMuxerParamCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_009, TestSize.Level2)
{
    AVMuxerDemo *muxerDemo = new AVMuxerDemo();
    OH_AVOutputFormat format = AV_OUTPUT_FORMAT_MPEG_4;
    int32_t fd = muxerDemo->GetFdByMode(format);
    OH_AVMuxer *handle = muxerDemo->NativeCreate(fd, format);
    ASSERT_NE(nullptr, handle);

    uint8_t a[CODEC_CONFIG];

    OH_AVFormat *trackFormat = OH_AVFormat_Create();
    OH_AVFormat_SetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    OH_AVFormat_SetLongValue(trackFormat, OH_MD_KEY_BITRATE, 524569);
    OH_AVFormat_SetBuffer(trackFormat, OH_MD_KEY_CODEC_CONFIG, a, CODEC_CONFIG);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_YUVI420);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_WIDTH, WIDTH);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_HEIGHT, HEIGHT);
    OH_AVFormat_SetDoubleValue(trackFormat, OH_MD_KEY_FRAME_RATE, FRAME_RATE);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_PROFILE, PROFILE);

    int32_t trackId;

    OH_AVErrCode ret = muxerDemo->NativeAddTrack(handle, &trackId, trackFormat);
    ASSERT_EQ(0, trackId);

    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_HEIGHT, -1);
    ret = muxerDemo->NativeAddTrack(handle, &trackId, trackFormat);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    OH_AVFormat_SetStringValue(trackFormat, OH_MD_KEY_HEIGHT, "aaa");
    ret = muxerDemo->NativeAddTrack(handle, &trackId, trackFormat);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    OH_AVFormat_SetLongValue(trackFormat, OH_MD_KEY_HEIGHT, 0);
    ret = muxerDemo->NativeAddTrack(handle, &trackId, trackFormat);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    OH_AVFormat_SetFloatValue(trackFormat, OH_MD_KEY_HEIGHT, 0.1);
    ret = muxerDemo->NativeAddTrack(handle, &trackId, trackFormat);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    OH_AVFormat_SetDoubleValue(trackFormat, OH_MD_KEY_HEIGHT, 0.1);
    ret = muxerDemo->NativeAddTrack(handle, &trackId, trackFormat);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    uint8_t b[100];
    OH_AVFormat_SetBuffer(trackFormat, OH_MD_KEY_HEIGHT, b, 100);
    ret = muxerDemo->NativeAddTrack(handle, &trackId, trackFormat);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    muxerDemo->NativeDestroy(handle);
    OH_AVFormat_Destroy(trackFormat);
    handle = nullptr;
    delete muxerDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_010
 * @tc.name      : OH_AVMuxer_AddTrack - trackFormat(any key) check
 * @tc.desc      : param check test
 */
HWTEST_F(NativeAVMuxerParamCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_010, TestSize.Level2)
{
    AVMuxerDemo *muxerDemo = new AVMuxerDemo();
    OH_AVOutputFormat format = AV_OUTPUT_FORMAT_MPEG_4;
    int32_t fd = muxerDemo->GetFdByMode(format);
    OH_AVMuxer *handle = muxerDemo->NativeCreate(fd, format);
    ASSERT_NE(nullptr, handle);

    OH_AVFormat *trackFormat = OH_AVFormat_Create();
    OH_AVFormat_SetStringValue(trackFormat, "aaaaa", "bbbbb");

    int32_t trackId;

    OH_AVErrCode ret = muxerDemo->NativeAddTrack(handle, &trackId, trackFormat);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    muxerDemo->NativeDestroy(handle);
    OH_AVFormat_Destroy(trackFormat);
    handle = nullptr;
    delete muxerDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_011
 * @tc.name      : OH_AVMuxer_WriteSampleBuffer - trackIndex check
 * @tc.desc      : param check test
 */
HWTEST_F(NativeAVMuxerParamCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_011, TestSize.Level2)
{
    AVMuxerDemo *muxerDemo = new AVMuxerDemo();
    OH_AVOutputFormat format = AV_OUTPUT_FORMAT_MPEG_4;
    int32_t fd = muxerDemo->GetFdByMode(format);
    OH_AVMuxer *handle = muxerDemo->NativeCreate(fd, format);
    ASSERT_NE(nullptr, handle);

    uint8_t a[CODEC_CONFIG];

    OH_AVFormat *trackFormat = OH_AVFormat_Create();
    OH_AVFormat_SetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_AAC);
    OH_AVFormat_SetLongValue(trackFormat, OH_MD_KEY_BITRATE, AUDIO_BITRATE);
    OH_AVFormat_SetBuffer(trackFormat, OH_MD_KEY_CODEC_CONFIG, a, CODEC_CONFIG);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, SAMPLE_RATE);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_PROFILE, PROFILE);

    int32_t trackId;

    OH_AVErrCode ret = muxerDemo->NativeAddTrack(handle, &trackId, trackFormat);
    ASSERT_EQ(0, trackId);

    ret = muxerDemo->NativeStart(handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVMemory *avMemBuffer = OH_AVMemory_Create(INFO_SIZE);

    OH_AVCodecBufferAttr info;
    info.pts = 0;
    info.size = INFO_SIZE;
    info.offset = 0;
    info.flags = 0;

    ret = muxerDemo->NativeWriteSampleBuffer(handle, trackId, avMemBuffer, info);
    ASSERT_EQ(AV_ERR_OK, ret);

    trackId = -1;
    ret = muxerDemo->NativeWriteSampleBuffer(handle, trackId, avMemBuffer, info);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    OH_AVMemory_Destroy(avMemBuffer);
    muxerDemo->NativeDestroy(handle);
    OH_AVFormat_Destroy(trackFormat);
    handle = nullptr;
    delete muxerDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_012
 * @tc.name      : OH_AVMuxer_WriteSampleBuffer - info.pts check
 * @tc.desc      : param check test
 */
HWTEST_F(NativeAVMuxerParamCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_012, TestSize.Level2)
{
    AVMuxerDemo *muxerDemo = new AVMuxerDemo();
    OH_AVOutputFormat format = AV_OUTPUT_FORMAT_MPEG_4;
    int32_t fd = muxerDemo->GetFdByMode(format);
    OH_AVMuxer *handle = muxerDemo->NativeCreate(fd, format);
    ASSERT_NE(nullptr, handle);

    uint8_t a[CODEC_CONFIG];

    OH_AVFormat *trackFormat = OH_AVFormat_Create();
    OH_AVFormat_SetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_AAC);
    OH_AVFormat_SetLongValue(trackFormat, OH_MD_KEY_BITRATE, AUDIO_BITRATE);
    OH_AVFormat_SetBuffer(trackFormat, OH_MD_KEY_CODEC_CONFIG, a, CODEC_CONFIG);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, SAMPLE_RATE);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_PROFILE, PROFILE);

    int32_t trackId;

    OH_AVErrCode ret = muxerDemo->NativeAddTrack(handle, &trackId, trackFormat);
    ASSERT_EQ(0, trackId);

    ret = muxerDemo->NativeStart(handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVMemory *avMemBuffer = OH_AVMemory_Create(INFO_SIZE);

    OH_AVCodecBufferAttr info;
    info.pts = 0;
    info.size = INFO_SIZE;
    info.offset = 0;
    info.flags = 0;

    ret = muxerDemo->NativeWriteSampleBuffer(handle, trackId, avMemBuffer, info);
    ASSERT_EQ(AV_ERR_OK, ret);

    info.pts = -1;
    ret = muxerDemo->NativeWriteSampleBuffer(handle, trackId, avMemBuffer, info);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVMemory_Destroy(avMemBuffer);
    muxerDemo->NativeDestroy(handle);
    OH_AVFormat_Destroy(trackFormat);
    handle = nullptr;
    delete muxerDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_013
 * @tc.name      : OH_AVMuxer_WriteSampleBuffer - info.size check
 * @tc.desc      : param check test
 */
HWTEST_F(NativeAVMuxerParamCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_013, TestSize.Level2)
{
    AVMuxerDemo *muxerDemo = new AVMuxerDemo();
    OH_AVOutputFormat format = AV_OUTPUT_FORMAT_MPEG_4;
    int32_t fd = muxerDemo->GetFdByMode(format);
    OH_AVMuxer *handle = muxerDemo->NativeCreate(fd, format);
    ASSERT_NE(nullptr, handle);

    uint8_t a[CODEC_CONFIG];

    OH_AVFormat *trackFormat = OH_AVFormat_Create();
    OH_AVFormat_SetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_AAC);
    OH_AVFormat_SetLongValue(trackFormat, OH_MD_KEY_BITRATE, AUDIO_BITRATE);
    OH_AVFormat_SetBuffer(trackFormat, OH_MD_KEY_CODEC_CONFIG, a, CODEC_CONFIG);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, SAMPLE_RATE);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_PROFILE, PROFILE);

    int32_t trackId;

    OH_AVErrCode ret = muxerDemo->NativeAddTrack(handle, &trackId, trackFormat);
    ASSERT_EQ(0, trackId);

    ret = muxerDemo->NativeStart(handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVMemory *avMemBuffer = OH_AVMemory_Create(INFO_SIZE);

    OH_AVCodecBufferAttr info;
    info.pts = 0;
    info.size = INFO_SIZE;
    info.offset = 0;
    info.flags = 0;

    ret = muxerDemo->NativeWriteSampleBuffer(handle, trackId, avMemBuffer, info);
    ASSERT_EQ(AV_ERR_OK, ret);

    info.size = -1;
    ret = muxerDemo->NativeWriteSampleBuffer(handle, trackId, avMemBuffer, info);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    OH_AVMemory_Destroy(avMemBuffer);
    muxerDemo->NativeDestroy(handle);
    OH_AVFormat_Destroy(trackFormat);
    handle = nullptr;
    delete muxerDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_014
 * @tc.name      : OH_AVMuxer_WriteSampleBuffer - info.offset check
 * @tc.desc      : param check test
 */
HWTEST_F(NativeAVMuxerParamCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_014, TestSize.Level2)
{
    AVMuxerDemo *muxerDemo = new AVMuxerDemo();
    OH_AVOutputFormat format = AV_OUTPUT_FORMAT_MPEG_4;
    int32_t fd = muxerDemo->GetFdByMode(format);
    OH_AVMuxer *handle = muxerDemo->NativeCreate(fd, format);
    ASSERT_NE(nullptr, handle);

    uint8_t a[CODEC_CONFIG];

    OH_AVFormat *trackFormat = OH_AVFormat_Create();
    OH_AVFormat_SetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_AAC);
    OH_AVFormat_SetLongValue(trackFormat, OH_MD_KEY_BITRATE, AUDIO_BITRATE);
    OH_AVFormat_SetBuffer(trackFormat, OH_MD_KEY_CODEC_CONFIG, a, CODEC_CONFIG);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, SAMPLE_RATE);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_PROFILE, PROFILE);

    int32_t trackId;

    OH_AVErrCode ret = muxerDemo->NativeAddTrack(handle, &trackId, trackFormat);
    ASSERT_EQ(0, trackId);

    ret = muxerDemo->NativeStart(handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVMemory *avMemBuffer = OH_AVMemory_Create(INFO_SIZE);

    OH_AVCodecBufferAttr info;
    info.pts = 0;
    info.size = INFO_SIZE;
    info.offset = 0;
    info.flags = 0;

    ret = muxerDemo->NativeWriteSampleBuffer(handle, trackId, avMemBuffer, info);
    ASSERT_EQ(AV_ERR_OK, ret);

    info.offset = -1;
    ret = muxerDemo->NativeWriteSampleBuffer(handle, trackId, avMemBuffer, info);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    OH_AVMemory_Destroy(avMemBuffer);
    muxerDemo->NativeDestroy(handle);
    OH_AVFormat_Destroy(trackFormat);
    handle = nullptr;
    delete muxerDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_015
 * @tc.name      : OH_AVMuxer_WriteSampleBuffer - info.flags check
 * @tc.desc      : param check test
 */
HWTEST_F(NativeAVMuxerParamCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_015, TestSize.Level2)
{
    AVMuxerDemo *muxerDemo = new AVMuxerDemo();
    OH_AVOutputFormat format = AV_OUTPUT_FORMAT_MPEG_4;
    int32_t fd = muxerDemo->GetFdByMode(format);
    OH_AVMuxer *handle = muxerDemo->NativeCreate(fd, format);
    ASSERT_NE(nullptr, handle);

    uint8_t a[CODEC_CONFIG];

    OH_AVFormat *trackFormat = OH_AVFormat_Create();
    OH_AVFormat_SetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_AAC);
    OH_AVFormat_SetLongValue(trackFormat, OH_MD_KEY_BITRATE, AUDIO_BITRATE);
    OH_AVFormat_SetBuffer(trackFormat, OH_MD_KEY_CODEC_CONFIG, a, CODEC_CONFIG);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, SAMPLE_RATE);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_PROFILE, PROFILE);

    int32_t trackId;

    OH_AVErrCode ret = muxerDemo->NativeAddTrack(handle, &trackId, trackFormat);
    ASSERT_EQ(0, trackId);

    ret = muxerDemo->NativeStart(handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVMemory *avMemBuffer = OH_AVMemory_Create(INFO_SIZE);

    OH_AVCodecBufferAttr info;
    info.pts = 0;
    info.size = INFO_SIZE;
    info.offset = 0;
    info.flags = 0;

    ret = muxerDemo->NativeWriteSampleBuffer(handle, trackId, avMemBuffer, info);
    ASSERT_EQ(AV_ERR_OK, ret);

    info.flags = -1;
    ret = muxerDemo->NativeWriteSampleBuffer(handle, trackId, avMemBuffer, info);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVMemory_Destroy(avMemBuffer);
    muxerDemo->NativeDestroy(handle);
    OH_AVFormat_Destroy(trackFormat);
    handle = nullptr;
    delete muxerDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_016
 * @tc.name      : OH_AVMuxer_WriteSampleBuffer - sample check
 * @tc.desc      : param check test
 */
HWTEST_F(NativeAVMuxerParamCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_016, TestSize.Level2)
{
    AVMuxerDemo *muxerDemo = new AVMuxerDemo();
    OH_AVOutputFormat format = AV_OUTPUT_FORMAT_MPEG_4;
    int32_t fd = muxerDemo->GetFdByMode(format);
    OH_AVMuxer *handle = muxerDemo->NativeCreate(fd, format);
    ASSERT_NE(nullptr, handle);

    uint8_t a[CODEC_CONFIG];

    OH_AVFormat *trackFormat = OH_AVFormat_Create();
    OH_AVFormat_SetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_AAC);
    OH_AVFormat_SetLongValue(trackFormat, OH_MD_KEY_BITRATE, AUDIO_BITRATE);
    OH_AVFormat_SetBuffer(trackFormat, OH_MD_KEY_CODEC_CONFIG, a, CODEC_CONFIG);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, SAMPLE_RATE);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_PROFILE, PROFILE);

    int32_t trackId;

    OH_AVErrCode ret = muxerDemo->NativeAddTrack(handle, &trackId, trackFormat);
    ASSERT_EQ(0, trackId);

    ret = muxerDemo->NativeStart(handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVMemory *avMemBuffer = OH_AVMemory_Create(10);

    OH_AVCodecBufferAttr info;
    info.pts = 0;
    info.size = INFO_SIZE;
    info.offset = 0;
    info.flags = 0;

    ret = muxerDemo->NativeWriteSampleBuffer(handle, trackId, avMemBuffer, info);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    OH_AVMemory_Destroy(avMemBuffer);
    muxerDemo->NativeDestroy(handle);
    OH_AVFormat_Destroy(trackFormat);
    handle = nullptr;
    delete muxerDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_017
 * @tc.name      : OH_AVMuxer_AddTrack - video trackFormat(OH_MD_KEY_COLOR_PRIMARIES) check
 * @tc.desc      : param check test
 */
HWTEST_F(NativeAVMuxerParamCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_017, TestSize.Level2)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        std::cout << "the hevc of mimetype is not supported" << std::endl;
        return;
    }
    AVMuxerDemo *muxerDemo = new AVMuxerDemo();
    OH_AVOutputFormat format = AV_OUTPUT_FORMAT_MPEG_4;
    int32_t fd = muxerDemo->GetFdByMode(format);
    OH_AVMuxer *handle = muxerDemo->NativeCreate(fd, format);
    ASSERT_NE(nullptr, handle);
    uint8_t a[CODEC_CONFIG];
    OH_AVFormat *trackFormat = OH_AVFormat_Create();
    OH_AVFormat_SetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_VIDEO_HEVC);
    OH_AVFormat_SetLongValue(trackFormat, OH_MD_KEY_BITRATE, 524569);
    OH_AVFormat_SetBuffer(trackFormat, OH_MD_KEY_CODEC_CONFIG, a, CODEC_CONFIG);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_WIDTH, WIDTH);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_HEIGHT, HEIGHT);
    OH_AVFormat_SetDoubleValue(trackFormat, OH_MD_KEY_FRAME_RATE, FRAME_RATE);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_COLOR_PRIMARIES, OH_ColorPrimary::COLOR_PRIMARY_BT709);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_TRANSFER_CHARACTERISTICS,
        OH_TransferCharacteristic::TRANSFER_CHARACTERISTIC_BT709);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_MATRIX_COEFFICIENTS,
        OH_MatrixCoefficient::MATRIX_COEFFICIENT_BT709);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_RANGE_FLAG, 0);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_VIDEO_IS_HDR_VIVID, 1);
    int32_t trackId;
    OH_AVErrCode ret = muxerDemo->NativeAddTrack(handle, &trackId, trackFormat);
    ASSERT_EQ(0, trackId);
    ASSERT_EQ(AV_ERR_OK, ret);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_COLOR_PRIMARIES, 0);
    ret = muxerDemo->NativeAddTrack(handle, &trackId, trackFormat);
    ASSERT_NE(AV_ERR_OK, ret);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_COLOR_PRIMARIES, OH_ColorPrimary::COLOR_PRIMARY_BT709);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_TRANSFER_CHARACTERISTICS, 0);
    ret = muxerDemo->NativeAddTrack(handle, &trackId, trackFormat);
    ASSERT_NE(AV_ERR_OK, ret);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_TRANSFER_CHARACTERISTICS,
        OH_TransferCharacteristic::TRANSFER_CHARACTERISTIC_BT709);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_MATRIX_COEFFICIENTS, 3);
    ret = muxerDemo->NativeAddTrack(handle, &trackId, trackFormat);
    ASSERT_NE(AV_ERR_OK, ret);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_MATRIX_COEFFICIENTS,
        OH_MatrixCoefficient::MATRIX_COEFFICIENT_BT709);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_VIDEO_IS_HDR_VIVID, 0);
    ret = muxerDemo->NativeAddTrack(handle, &trackId, trackFormat);
    ASSERT_EQ(AV_ERR_OK, ret);
    muxerDemo->NativeDestroy(handle);
    OH_AVFormat_Destroy(trackFormat);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_018
 * @tc.name      : OH_AVMuxer_WriteSampleBuffer - trackIndex check
 * @tc.desc      : param check test
 */
HWTEST_F(NativeAVMuxerParamCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_018, TestSize.Level2)
{
    AVMuxerDemo *muxerDemo = new AVMuxerDemo();
    OH_AVOutputFormat format = AV_OUTPUT_FORMAT_MPEG_4;
    int32_t fd = muxerDemo->GetFdByMode(format);
    OH_AVMuxer *handle = muxerDemo->NativeCreate(fd, format);
    ASSERT_NE(nullptr, handle);

    uint8_t a[CODEC_CONFIG];

    OH_AVFormat *trackFormat = OH_AVFormat_Create();
    OH_AVFormat_SetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_AAC);
    OH_AVFormat_SetLongValue(trackFormat, OH_MD_KEY_BITRATE, AUDIO_BITRATE);
    OH_AVFormat_SetBuffer(trackFormat, OH_MD_KEY_CODEC_CONFIG, a, CODEC_CONFIG);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, SAMPLE_RATE);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_PROFILE, PROFILE);

    int32_t trackId;

    OH_AVErrCode ret = muxerDemo->NativeAddTrack(handle, &trackId, trackFormat);
    ASSERT_EQ(0, trackId);

    ret = muxerDemo->NativeStart(handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVBuffer *avBuffer = OH_AVBuffer_Create(INFO_SIZE);

    OH_AVCodecBufferAttr info;
    info.pts = 0;
    info.size = INFO_SIZE;
    info.offset = 0;
    info.flags = 0;
    OH_AVBuffer_SetBufferAttr(avBuffer, &info);
    ret = muxerDemo->NativeWriteSampleBuffer(handle, trackId, avBuffer);
    ASSERT_EQ(AV_ERR_OK, ret);

    trackId = -1;
    ret = muxerDemo->NativeWriteSampleBuffer(handle, trackId, avBuffer);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    OH_AVBuffer_Destroy(avBuffer);
    muxerDemo->NativeDestroy(handle);
    OH_AVFormat_Destroy(trackFormat);
    handle = nullptr;
    delete muxerDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_019
 * @tc.name      : OH_AVMuxer_WriteSampleBuffer - info.pts check
 * @tc.desc      : param check test
 */
HWTEST_F(NativeAVMuxerParamCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_019, TestSize.Level2)
{
    AVMuxerDemo *muxerDemo = new AVMuxerDemo();
    OH_AVOutputFormat format = AV_OUTPUT_FORMAT_MPEG_4;
    int32_t fd = muxerDemo->GetFdByMode(format);
    OH_AVMuxer *handle = muxerDemo->NativeCreate(fd, format);
    ASSERT_NE(nullptr, handle);

    uint8_t a[CODEC_CONFIG];

    OH_AVFormat *trackFormat = OH_AVFormat_Create();
    OH_AVFormat_SetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_AAC);
    OH_AVFormat_SetLongValue(trackFormat, OH_MD_KEY_BITRATE, AUDIO_BITRATE);
    OH_AVFormat_SetBuffer(trackFormat, OH_MD_KEY_CODEC_CONFIG, a, CODEC_CONFIG);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, SAMPLE_RATE);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_PROFILE, PROFILE);

    int32_t trackId;

    OH_AVErrCode ret = muxerDemo->NativeAddTrack(handle, &trackId, trackFormat);
    ASSERT_EQ(0, trackId);

    ret = muxerDemo->NativeStart(handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVBuffer *avBuffer = OH_AVBuffer_Create(INFO_SIZE);

    OH_AVCodecBufferAttr info;
    info.pts = 0;
    info.size = INFO_SIZE;
    info.offset = 0;
    info.flags = 0;
    OH_AVBuffer_SetBufferAttr(avBuffer, &info);
    ret = muxerDemo->NativeWriteSampleBuffer(handle, trackId, avBuffer);
    ASSERT_EQ(AV_ERR_OK, ret);

    info.pts = -1;
    OH_AVBuffer_SetBufferAttr(avBuffer, &info);
    ret = muxerDemo->NativeWriteSampleBuffer(handle, trackId, avBuffer);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVBuffer_Destroy(avBuffer);
    muxerDemo->NativeDestroy(handle);
    OH_AVFormat_Destroy(trackFormat);
    handle = nullptr;
    delete muxerDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_020
 * @tc.name      : OH_AVMuxer_WriteSampleBuffer - info.size check
 * @tc.desc      : param check test
 */
HWTEST_F(NativeAVMuxerParamCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_020, TestSize.Level2)
{
    AVMuxerDemo *muxerDemo = new AVMuxerDemo();
    OH_AVOutputFormat format = AV_OUTPUT_FORMAT_MPEG_4;
    int32_t fd = muxerDemo->GetFdByMode(format);
    OH_AVMuxer *handle = muxerDemo->NativeCreate(fd, format);
    ASSERT_NE(nullptr, handle);

    uint8_t a[CODEC_CONFIG];

    OH_AVFormat *trackFormat = OH_AVFormat_Create();
    OH_AVFormat_SetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_AAC);
    OH_AVFormat_SetLongValue(trackFormat, OH_MD_KEY_BITRATE, AUDIO_BITRATE);
    OH_AVFormat_SetBuffer(trackFormat, OH_MD_KEY_CODEC_CONFIG, a, CODEC_CONFIG);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, SAMPLE_RATE);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_PROFILE, PROFILE);

    int32_t trackId;

    OH_AVErrCode ret = muxerDemo->NativeAddTrack(handle, &trackId, trackFormat);
    ASSERT_EQ(0, trackId);

    ret = muxerDemo->NativeStart(handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVBuffer *avBuffer = OH_AVBuffer_Create(INFO_SIZE);

    OH_AVCodecBufferAttr info;
    info.pts = 0;
    info.size = INFO_SIZE;
    info.offset = 0;
    info.flags = 0;
    OH_AVBuffer_SetBufferAttr(avBuffer, &info);
    ret = muxerDemo->NativeWriteSampleBuffer(handle, trackId, avBuffer);
    ASSERT_EQ(AV_ERR_OK, ret);

    info.size = -1;
    OH_AVBuffer_SetBufferAttr(avBuffer, &info);
    ret = muxerDemo->NativeWriteSampleBuffer(handle, trackId, avBuffer);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVBuffer_Destroy(avBuffer);
    muxerDemo->NativeDestroy(handle);
    OH_AVFormat_Destroy(trackFormat);
    handle = nullptr;
    delete muxerDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_021
 * @tc.name      : OH_AVMuxer_WriteSampleBuffer - info.offset check
 * @tc.desc      : param check test
 */
HWTEST_F(NativeAVMuxerParamCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_021, TestSize.Level2)
{
    AVMuxerDemo *muxerDemo = new AVMuxerDemo();
    OH_AVOutputFormat format = AV_OUTPUT_FORMAT_MPEG_4;
    int32_t fd = muxerDemo->GetFdByMode(format);
    OH_AVMuxer *handle = muxerDemo->NativeCreate(fd, format);
    ASSERT_NE(nullptr, handle);

    uint8_t a[CODEC_CONFIG];

    OH_AVFormat *trackFormat = OH_AVFormat_Create();
    OH_AVFormat_SetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_AAC);
    OH_AVFormat_SetLongValue(trackFormat, OH_MD_KEY_BITRATE, AUDIO_BITRATE);
    OH_AVFormat_SetBuffer(trackFormat, OH_MD_KEY_CODEC_CONFIG, a, CODEC_CONFIG);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, SAMPLE_RATE);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_PROFILE, PROFILE);

    int32_t trackId;

    OH_AVErrCode ret = muxerDemo->NativeAddTrack(handle, &trackId, trackFormat);
    ASSERT_EQ(0, trackId);

    ret = muxerDemo->NativeStart(handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVBuffer *avBuffer = OH_AVBuffer_Create(INFO_SIZE);

    OH_AVCodecBufferAttr info;
    info.pts = 0;
    info.size = INFO_SIZE;
    info.offset = 0;
    info.flags = 0;
    OH_AVBuffer_SetBufferAttr(avBuffer, &info);
    ret = muxerDemo->NativeWriteSampleBuffer(handle, trackId, avBuffer);
    ASSERT_EQ(AV_ERR_OK, ret);

    info.offset = -1;
    OH_AVBuffer_SetBufferAttr(avBuffer, &info);
    ret = muxerDemo->NativeWriteSampleBuffer(handle, trackId, avBuffer);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVBuffer_Destroy(avBuffer);
    muxerDemo->NativeDestroy(handle);
    OH_AVFormat_Destroy(trackFormat);
    handle = nullptr;
    delete muxerDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_022
 * @tc.name      : OH_AVMuxer_WriteSampleBuffer - info.flags check
 * @tc.desc      : param check test
 */
HWTEST_F(NativeAVMuxerParamCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_022, TestSize.Level2)
{
    AVMuxerDemo *muxerDemo = new AVMuxerDemo();
    OH_AVOutputFormat format = AV_OUTPUT_FORMAT_MPEG_4;
    int32_t fd = muxerDemo->GetFdByMode(format);
    OH_AVMuxer *handle = muxerDemo->NativeCreate(fd, format);
    ASSERT_NE(nullptr, handle);

    uint8_t a[CODEC_CONFIG];

    OH_AVFormat *trackFormat = OH_AVFormat_Create();
    OH_AVFormat_SetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_AAC);
    OH_AVFormat_SetLongValue(trackFormat, OH_MD_KEY_BITRATE, AUDIO_BITRATE);
    OH_AVFormat_SetBuffer(trackFormat, OH_MD_KEY_CODEC_CONFIG, a, CODEC_CONFIG);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, SAMPLE_RATE);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_PROFILE, PROFILE);

    int32_t trackId;

    OH_AVErrCode ret = muxerDemo->NativeAddTrack(handle, &trackId, trackFormat);
    ASSERT_EQ(0, trackId);

    ret = muxerDemo->NativeStart(handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVBuffer *avBuffer = OH_AVBuffer_Create(INFO_SIZE);

    OH_AVCodecBufferAttr info;
    info.pts = 0;
    info.size = INFO_SIZE;
    info.offset = 0;
    info.flags = 0;
    OH_AVBuffer_SetBufferAttr(avBuffer, &info);
    ret = muxerDemo->NativeWriteSampleBuffer(handle, trackId, avBuffer);
    ASSERT_EQ(AV_ERR_OK, ret);

    info.flags = -1;
    OH_AVBuffer_SetBufferAttr(avBuffer, &info);
    ret = muxerDemo->NativeWriteSampleBuffer(handle, trackId, avBuffer);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVBuffer_Destroy(avBuffer);
    muxerDemo->NativeDestroy(handle);
    OH_AVFormat_Destroy(trackFormat);
    handle = nullptr;
    delete muxerDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_025
 * @tc.name      : OH_AVMuxer_WriteSampleBuffer - sample check
 * @tc.desc      : param check test
 */
HWTEST_F(NativeAVMuxerParamCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_025, TestSize.Level2)
{
    AVMuxerDemo *muxerDemo = new AVMuxerDemo();
    OH_AVOutputFormat format = AV_OUTPUT_FORMAT_MP3;
    int32_t fd = muxerDemo->GetFdByMode(format);
    OH_AVMuxer *handle = muxerDemo->NativeCreate(fd, format);
    ASSERT_NE(nullptr, handle);

    uint8_t a[CODEC_CONFIG];

    OH_AVFormat *trackFormat = OH_AVFormat_Create();
    OH_AVFormat_SetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_MPEG);
    OH_AVFormat_SetLongValue(trackFormat, OH_MD_KEY_BITRATE, AUDIO_BITRATE);
    OH_AVFormat_SetBuffer(trackFormat, OH_MD_KEY_CODEC_CONFIG, a, CODEC_CONFIG);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, SAMPLE_RATE);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_PROFILE, PROFILE);

    int32_t trackId;

    OH_AVErrCode ret = muxerDemo->NativeAddTrack(handle, &trackId, trackFormat);
    ASSERT_EQ(0, trackId);

    ret = muxerDemo->NativeStart(handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVBuffer *avBuffer = OH_AVBuffer_Create(INFO_SIZE);

    OH_AVCodecBufferAttr info;
    info.pts = 0;
    info.size = INFO_SIZE;
    info.offset = 0;
    info.flags = 0;
    OH_AVBuffer_SetBufferAttr(avBuffer, &info);
    ret = muxerDemo->NativeWriteSampleBuffer(handle, trackId, avBuffer);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVBuffer_Destroy(avBuffer);
    muxerDemo->NativeDestroy(handle);
    OH_AVFormat_Destroy(trackFormat);
    handle = nullptr;
    delete muxerDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_023
 * @tc.name      : OH_AVMuxer_WriteSampleBuffer - sample check
 * @tc.desc      : param check test
 */
HWTEST_F(NativeAVMuxerParamCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_023, TestSize.Level2)
{
    AVMuxerDemo *muxerDemo = new AVMuxerDemo();
    OH_AVOutputFormat format = AV_OUTPUT_FORMAT_MPEG_4;
    int32_t fd = muxerDemo->GetFdByMode(format);
    OH_AVMuxer *handle = muxerDemo->NativeCreate(fd, format);
    ASSERT_NE(nullptr, handle);

    uint8_t a[CODEC_CONFIG];

    OH_AVFormat *trackFormat = OH_AVFormat_Create();
    OH_AVFormat_SetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_AAC);
    OH_AVFormat_SetLongValue(trackFormat, OH_MD_KEY_BITRATE, AUDIO_BITRATE);
    OH_AVFormat_SetBuffer(trackFormat, OH_MD_KEY_CODEC_CONFIG, a, CODEC_CONFIG);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, SAMPLE_RATE);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_PROFILE, PROFILE);

    int32_t trackId;

    OH_AVErrCode ret = muxerDemo->NativeAddTrack(handle, &trackId, trackFormat);
    ASSERT_EQ(0, trackId);

    ret = muxerDemo->NativeStart(handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVBuffer *avBuffer = OH_AVBuffer_Create(0x10);

    OH_AVCodecBufferAttr info;
    info.pts = 0;
    info.size = INFO_SIZE;
    info.offset = 0;
    info.flags = 0;
    OH_AVBuffer_SetBufferAttr(avBuffer, &info);
    ret = muxerDemo->NativeWriteSampleBuffer(handle, trackId, avBuffer);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    OH_AVBuffer_Destroy(avBuffer);
    muxerDemo->NativeDestroy(handle);
    OH_AVFormat_Destroy(trackFormat);
    handle = nullptr;
    delete muxerDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_024
 * @tc.name      : OH_AVMuxer_WriteSampleBuffer - sample check
 * @tc.desc      : param check test
 */
HWTEST_F(NativeAVMuxerParamCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_PARAM_CHECK_024, TestSize.Level2)
{
    AVMuxerDemo *muxerDemo = new AVMuxerDemo();
    OH_AVOutputFormat format = AV_OUTPUT_FORMAT_AMR;
    int32_t fd = muxerDemo->GetFdByMode(format);
    OH_AVMuxer *handle = muxerDemo->NativeCreate(fd, format);
    ASSERT_NE(nullptr, handle);

    OH_AVFormat *trackFormat = OH_AVFormat_Create();
    OH_AVFormat_SetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_AMR_NB);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, 1); // 1 audio channel, mono
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, 8000); // 8000: 8khz sample rate

    int32_t trackId;

    OH_AVErrCode ret = muxerDemo->NativeAddTrack(handle, &trackId, trackFormat);
    ASSERT_EQ(0, trackId);

    ret = muxerDemo->NativeStart(handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVBuffer *avBuffer = OH_AVBuffer_Create(INFO_SIZE);

    OH_AVCodecBufferAttr info;
    info.pts = 0;
    info.size = INFO_SIZE;
    info.offset = 0;
    info.flags = 0;
    OH_AVBuffer_SetBufferAttr(avBuffer, &info);
    ret = muxerDemo->NativeWriteSampleBuffer(handle, trackId, avBuffer);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVBuffer_Destroy(avBuffer);
    muxerDemo->NativeDestroy(handle);
    OH_AVFormat_Destroy(trackFormat);
    handle = nullptr;
    delete muxerDemo;
}
