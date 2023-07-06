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
#include "AudioEncoderDemoCommon.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;

namespace {
class NativeParamCheckTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp() override;
    void TearDown() override;
};

void NativeParamCheckTest::SetUpTestCase() {}
void NativeParamCheckTest::TearDownTestCase() {}
void NativeParamCheckTest::SetUp() {}
void NativeParamCheckTest::TearDown() {}

constexpr int32_t CHANNEL_COUNT_FLAC = 2;
constexpr int32_t SAMPLE_RATE_FLAC = 48000;
constexpr int64_t BITS_RATE_FLAC = 128000;
} // namespace

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_PARAM_CHECK_001
 * @tc.name      : OH_AudioEncoder_CreateByMime - mime check
 * @tc.desc      : param check test
 */
HWTEST_F(NativeParamCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_PARAM_CHECK_001, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();
    OH_AVCodec *handle = encoderDemo->NativeCreateByMime("aaa");
    ASSERT_EQ(nullptr, handle);

    handle = encoderDemo->NativeCreateByMime(OH_AVCODEC_MIMETYPE_AUDIO_AAC);
    ASSERT_NE(nullptr, handle);

    encoderDemo->NativeDestroy(handle);
    handle = nullptr;
    handle = encoderDemo->NativeCreateByMime(OH_AVCODEC_MIMETYPE_AUDIO_FLAC);
    ASSERT_NE(nullptr, handle);
    encoderDemo->NativeDestroy(handle);
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_PARAM_CHECK_002
 * @tc.name      : OH_AudioEncoder_CreateByName - name check
 * @tc.desc      : param check test
 */
HWTEST_F(NativeParamCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_PARAM_CHECK_002, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();
    OH_AVCodec *handle = encoderDemo->NativeCreateByName("aaa");
    ASSERT_EQ(nullptr, handle);

    handle = encoderDemo->NativeCreateByName("OH.Media.Codec.Encoder.Audio.AAC");
    ASSERT_NE(nullptr, handle);
    encoderDemo->NativeDestroy(handle);
    handle = nullptr;

    handle = encoderDemo->NativeCreateByName("OH.Media.Codec.Encoder.Audio.Flac");
    ASSERT_NE(nullptr, handle);
    encoderDemo->NativeDestroy(handle);
    handle = nullptr;
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_PARAM_CHECK_003
 * @tc.name      : OH_AudioEncoder_Configure - format(MD_KEY_BITRATE) check
 * @tc.desc      : param check test
 */
HWTEST_F(NativeParamCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_PARAM_CHECK_003, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();
    OH_AVCodec *handle = encoderDemo->NativeCreateByName("OH.Media.Codec.Encoder.Audio.AAC");
    ASSERT_NE(nullptr, handle);

    OH_AVFormat *format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 2);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 44100);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, OH_BitsPerSample::SAMPLE_F32LE);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, OH_BitsPerSample::SAMPLE_F32LE);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_CHANNEL_LAYOUT, STEREO);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AAC_IS_ADTS, 1);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 169000);

    OH_AVErrCode ret = encoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    encoderDemo->NativeReset(handle);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, -1);
    ret = encoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_UNSUPPORT, ret);

    encoderDemo->NativeReset(handle);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITRATE, 1);
    ret = encoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_UNSUPPORT, ret);

    encoderDemo->NativeReset(handle);
    OH_AVFormat_SetStringValue(format, OH_MD_KEY_BITRATE, "aaa");
    ret = encoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_UNSUPPORT, ret);

    encoderDemo->NativeReset(handle);
    OH_AVFormat_SetFloatValue(format, OH_MD_KEY_BITRATE, 0.1);
    ret = encoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_UNSUPPORT, ret);

    encoderDemo->NativeReset(handle);
    OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_BITRATE, 0.1);
    ret = encoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_UNSUPPORT, ret);

    uint8_t a[100];
    encoderDemo->NativeReset(handle);
    OH_AVFormat_SetBuffer(format, OH_MD_KEY_BITRATE, a, 100);
    ret = encoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_UNSUPPORT, ret);

    OH_AVFormat_Destroy(format);
    encoderDemo->NativeDestroy(handle);
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_PARAM_CHECK_004
 * @tc.name      : OH_AudioEncoder_Configure - format(OH_MD_KEY_AUD_CHANNEL_COUNT) check
 * @tc.desc      : param check test
 */
HWTEST_F(NativeParamCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_PARAM_CHECK_004, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();
    OH_AVCodec *handle = encoderDemo->NativeCreateByName("OH.Media.Codec.Encoder.Audio.AAC");
    ASSERT_NE(nullptr, handle);

    OH_AVFormat *format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 2);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 44100);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, OH_BitsPerSample::SAMPLE_F32LE);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, OH_BitsPerSample::SAMPLE_F32LE);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_CHANNEL_LAYOUT, STEREO);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AAC_IS_ADTS, 1);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 169000);

    OH_AVErrCode ret = encoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    encoderDemo->NativeReset(handle);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, -1);
    ret = encoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_UNSUPPORT, ret);

    encoderDemo->NativeReset(handle);
    OH_AVFormat_SetStringValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, "aaa");
    ret = encoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_UNSUPPORT, ret);

    encoderDemo->NativeReset(handle);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 0);
    ret = encoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_UNSUPPORT, ret);

    encoderDemo->NativeReset(handle);
    OH_AVFormat_SetFloatValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 0.1);
    ret = encoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_UNSUPPORT, ret);

    encoderDemo->NativeReset(handle);
    OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 0.1);
    ret = encoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_UNSUPPORT, ret);

    uint8_t a[100];
    encoderDemo->NativeReset(handle);
    OH_AVFormat_SetBuffer(format, OH_MD_KEY_AUD_CHANNEL_COUNT, a, 100);
    ret = encoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_UNSUPPORT, ret);

    OH_AVFormat_Destroy(format);
    encoderDemo->NativeDestroy(handle);
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_PARAM_CHECK_005
 * @tc.name      : OH_AudioEncoder_Configure - format(OH_MD_KEY_AUD_SAMPLE_RATE) check
 * @tc.desc      : param check test
 */
HWTEST_F(NativeParamCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_PARAM_CHECK_005, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();
    OH_AVCodec *handle = encoderDemo->NativeCreateByName("OH.Media.Codec.Encoder.Audio.AAC");
    ASSERT_NE(nullptr, handle);

    OH_AVFormat *format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 2);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 44100);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, OH_BitsPerSample::SAMPLE_F32LE);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, OH_BitsPerSample::SAMPLE_F32LE);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_CHANNEL_LAYOUT, STEREO);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AAC_IS_ADTS, 1);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 169000);

    OH_AVErrCode ret = encoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    encoderDemo->NativeReset(handle);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, -1);
    ret = encoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_UNSUPPORT, ret);

    encoderDemo->NativeReset(handle);
    OH_AVFormat_SetStringValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, "aaa");
    ret = encoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_UNSUPPORT, ret);

    encoderDemo->NativeReset(handle);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 0);
    ret = encoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_UNSUPPORT, ret);

    encoderDemo->NativeReset(handle);
    OH_AVFormat_SetFloatValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 0.1);
    ret = encoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_UNSUPPORT, ret);

    encoderDemo->NativeReset(handle);
    OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 0.1);
    ret = encoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_UNSUPPORT, ret);

    uint8_t a[100];
    encoderDemo->NativeReset(handle);
    OH_AVFormat_SetBuffer(format, OH_MD_KEY_AUD_SAMPLE_RATE, a, 100);
    ret = encoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_UNSUPPORT, ret);

    OH_AVFormat_Destroy(format);
    encoderDemo->NativeDestroy(handle);
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_PARAM_CHECK_006
 * @tc.name      : OH_AudioEncoder_SetParameter - format(MD_KEY_BITRATE) check
 * @tc.desc      : param check test
 */
HWTEST_F(NativeParamCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_PARAM_CHECK_006, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();
    OH_AVCodec *handle = encoderDemo->NativeCreateByName("OH.Media.Codec.Encoder.Audio.AAC");
    ASSERT_NE(nullptr, handle);

    OH_AVFormat *format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 2);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 44100);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, OH_BitsPerSample::SAMPLE_F32LE);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, OH_BitsPerSample::SAMPLE_F32LE);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_CHANNEL_LAYOUT, STEREO);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AAC_IS_ADTS, 1);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 169000);

    OH_AVErrCode ret = encoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = encoderDemo->NativePrepare(handle);
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = encoderDemo->NativeStart(handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = encoderDemo->NativeSetParameter(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, -1);
    ret = encoderDemo->NativeSetParameter(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVFormat_SetStringValue(format, OH_MD_KEY_BITRATE, "aaa");
    ret = encoderDemo->NativeSetParameter(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITRATE, 0);
    ret = encoderDemo->NativeSetParameter(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVFormat_SetFloatValue(format, OH_MD_KEY_BITRATE, 0.1);
    ret = encoderDemo->NativeSetParameter(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_BITRATE, 0.1);
    ret = encoderDemo->NativeSetParameter(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    uint8_t a[100];
    OH_AVFormat_SetBuffer(format, OH_MD_KEY_AUD_SAMPLE_RATE, a, 100);
    ret = encoderDemo->NativeSetParameter(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVFormat_Destroy(format);
    encoderDemo->NativeDestroy(handle);
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_PARAM_CHECK_007
 * @tc.name      : OH_AudioEncoder_SetParameter - format(OH_MD_KEY_AUD_CHANNEL_COUNT) check
 * @tc.desc      : param check test
 */
HWTEST_F(NativeParamCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_PARAM_CHECK_007, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();
    OH_AVCodec *handle = encoderDemo->NativeCreateByName("OH.Media.Codec.Encoder.Audio.AAC");
    ASSERT_NE(nullptr, handle);

    OH_AVFormat *format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 2);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 44100);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, OH_BitsPerSample::SAMPLE_F32LE);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, OH_BitsPerSample::SAMPLE_F32LE);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_CHANNEL_LAYOUT, STEREO);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AAC_IS_ADTS, 1);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 169000);

    OH_AVErrCode ret = encoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = encoderDemo->NativePrepare(handle);
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = encoderDemo->NativeStart(handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = encoderDemo->NativeSetParameter(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, -1);
    ret = encoderDemo->NativeSetParameter(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVFormat_SetStringValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, "aaa");
    ret = encoderDemo->NativeSetParameter(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVFormat_SetLongValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 0);
    ret = encoderDemo->NativeSetParameter(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVFormat_SetFloatValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 0.1);
    ret = encoderDemo->NativeSetParameter(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 0.1);
    ret = encoderDemo->NativeSetParameter(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    uint8_t a[100];
    OH_AVFormat_SetBuffer(format, OH_MD_KEY_AUD_CHANNEL_COUNT, a, 100);
    ret = encoderDemo->NativeSetParameter(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVFormat_Destroy(format);
    encoderDemo->NativeDestroy(handle);
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_PARAM_CHECK_008
 * @tc.name      : OH_AudioEncoder_SetParameter - format(OH_MD_KEY_AUD_SAMPLE_RATE) check
 * @tc.desc      : param check test
 */
HWTEST_F(NativeParamCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_PARAM_CHECK_008, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();
    OH_AVCodec *handle = encoderDemo->NativeCreateByName("OH.Media.Codec.Encoder.Audio.AAC");
    ASSERT_NE(nullptr, handle);

    OH_AVFormat *format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 2);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 44100);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, OH_BitsPerSample::SAMPLE_F32LE);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, OH_BitsPerSample::SAMPLE_F32LE);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_CHANNEL_LAYOUT, STEREO);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AAC_IS_ADTS, 1);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 169000);

    OH_AVErrCode ret = encoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = encoderDemo->NativePrepare(handle);
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = encoderDemo->NativeStart(handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = encoderDemo->NativeSetParameter(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, -1);
    ret = encoderDemo->NativeSetParameter(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVFormat_SetStringValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, "aaa");
    ret = encoderDemo->NativeSetParameter(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVFormat_SetLongValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 0);
    ret = encoderDemo->NativeSetParameter(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVFormat_SetFloatValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 0.1);
    ret = encoderDemo->NativeSetParameter(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 0.1);
    ret = encoderDemo->NativeSetParameter(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    uint8_t a[100];
    OH_AVFormat_SetBuffer(format, OH_MD_KEY_AUD_SAMPLE_RATE, a, 100);
    ret = encoderDemo->NativeSetParameter(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVFormat_Destroy(format);
    encoderDemo->NativeDestroy(handle);
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_PARAM_CHECK_009
 * @tc.name      : OH_AudioEncoder_PushInputData - index check
 * @tc.desc      : param check test
 */
HWTEST_F(NativeParamCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_PARAM_CHECK_009, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();
    OH_AVCodec *handle = encoderDemo->NativeCreateByName("OH.Media.Codec.Encoder.Audio.AAC");
    ASSERT_NE(nullptr, handle);

    OH_AVFormat *format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 2);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 44100);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, OH_BitsPerSample::SAMPLE_F32LE);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, OH_BitsPerSample::SAMPLE_F32LE);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_CHANNEL_LAYOUT, STEREO);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AAC_IS_ADTS, 1);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 169000);

    struct OH_AVCodecAsyncCallback cb = {&OnError, &OnOutputFormatChanged, &OnInputBufferAvailable,
                                         &OnOutputBufferAvailable};
    OH_AVErrCode ret = encoderDemo->NativeSetCallback(handle, cb);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = encoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = encoderDemo->NativePrepare(handle);
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = encoderDemo->NativeStart(handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVCodecBufferAttr info;
    info.size = 100;
    info.offset = 0;
    info.pts = 0;
    info.flags = AVCODEC_BUFFER_FLAGS_NONE;

    uint32_t index = encoderDemo->NativeGetInputIndex();
    ret = encoderDemo->NativePushInputData(handle, index, info);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = encoderDemo->NativePushInputData(handle, -1, info);
    ASSERT_EQ(AV_ERR_NO_MEMORY, ret);

    OH_AVFormat_Destroy(format);
    encoderDemo->NativeDestroy(handle);
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_PARAM_CHECK_010
 * @tc.name      : OH_AudioEncoder_PushInputData - info.size check
 * @tc.desc      : param check test
 */
HWTEST_F(NativeParamCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_PARAM_CHECK_010, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();
    OH_AVCodec *handle = encoderDemo->NativeCreateByName("OH.Media.Codec.Encoder.Audio.AAC");
    ASSERT_NE(nullptr, handle);

    OH_AVFormat *format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 2);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 44100);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, OH_BitsPerSample::SAMPLE_F32LE);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, OH_BitsPerSample::SAMPLE_F32LE);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_CHANNEL_LAYOUT, STEREO);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AAC_IS_ADTS, 1);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 169000);

    struct OH_AVCodecAsyncCallback cb = {&OnError, &OnOutputFormatChanged, &OnInputBufferAvailable,
                                         &OnOutputBufferAvailable};
    OH_AVErrCode ret = encoderDemo->NativeSetCallback(handle, cb);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = encoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = encoderDemo->NativePrepare(handle);
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = encoderDemo->NativeStart(handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVCodecBufferAttr info;
    info.size = 100;
    info.offset = 0;
    info.pts = 0;
    info.flags = AVCODEC_BUFFER_FLAGS_NONE;

    uint32_t index = encoderDemo->NativeGetInputIndex();
    ret = encoderDemo->NativePushInputData(handle, index, info);
    ASSERT_EQ(AV_ERR_OK, ret);

    index = encoderDemo->NativeGetInputIndex();
    info.size = -1;
    ret = encoderDemo->NativePushInputData(handle, index, info);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    OH_AVFormat_Destroy(format);
    encoderDemo->NativeDestroy(handle);
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_PARAM_CHECK_011
 * @tc.name      : OH_AudioEncoder_PushInputData - info.offset check
 * @tc.desc      : param check test
 */
HWTEST_F(NativeParamCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_PARAM_CHECK_011, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();
    OH_AVCodec *handle = encoderDemo->NativeCreateByName("OH.Media.Codec.Encoder.Audio.AAC");
    ASSERT_NE(nullptr, handle);

    OH_AVFormat *format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 2);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 44100);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, OH_BitsPerSample::SAMPLE_F32LE);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, OH_BitsPerSample::SAMPLE_F32LE);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_CHANNEL_LAYOUT, STEREO);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AAC_IS_ADTS, 1);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 169000);

    struct OH_AVCodecAsyncCallback cb = {&OnError, &OnOutputFormatChanged, &OnInputBufferAvailable,
                                         &OnOutputBufferAvailable};
    OH_AVErrCode ret = encoderDemo->NativeSetCallback(handle, cb);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = encoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = encoderDemo->NativePrepare(handle);
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = encoderDemo->NativeStart(handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVCodecBufferAttr info;
    info.size = 100;
    info.offset = 0;
    info.pts = 0;
    info.flags = AVCODEC_BUFFER_FLAGS_NONE;

    uint32_t index = encoderDemo->NativeGetInputIndex();
    ret = encoderDemo->NativePushInputData(handle, index, info);
    ASSERT_EQ(AV_ERR_OK, ret);

    index = encoderDemo->NativeGetInputIndex();
    info.offset = -1;
    ret = encoderDemo->NativePushInputData(handle, index, info);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    OH_AVFormat_Destroy(format);
    encoderDemo->NativeDestroy(handle);
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_PARAM_CHECK_012
 * @tc.name      : OH_AudioEncoder_PushInputData - info.pts check
 * @tc.desc      : param check test
 */
HWTEST_F(NativeParamCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_PARAM_CHECK_012, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();
    OH_AVCodec *handle = encoderDemo->NativeCreateByName("OH.Media.Codec.Encoder.Audio.AAC");
    ASSERT_NE(nullptr, handle);

    OH_AVFormat *format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 2);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 44100);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, OH_BitsPerSample::SAMPLE_F32LE);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, OH_BitsPerSample::SAMPLE_F32LE);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_CHANNEL_LAYOUT, STEREO);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AAC_IS_ADTS, 1);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 169000);

    struct OH_AVCodecAsyncCallback cb = {&OnError, &OnOutputFormatChanged, &OnInputBufferAvailable,
                                         &OnOutputBufferAvailable};
    OH_AVErrCode ret = encoderDemo->NativeSetCallback(handle, cb);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = encoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = encoderDemo->NativePrepare(handle);
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = encoderDemo->NativeStart(handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVCodecBufferAttr info;
    info.size = 100;
    info.offset = 0;
    info.pts = 0;
    info.flags = AVCODEC_BUFFER_FLAGS_NONE;

    uint32_t index = encoderDemo->NativeGetInputIndex();
    ret = encoderDemo->NativePushInputData(handle, index, info);
    ASSERT_EQ(AV_ERR_OK, ret);

    index = encoderDemo->NativeGetInputIndex();
    info.pts = -1;
    ret = encoderDemo->NativePushInputData(handle, index, info);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVFormat_Destroy(format);
    encoderDemo->NativeDestroy(handle);
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_PARAM_CHECK_013
 * @tc.name      : OH_AudioEncoder_FreeOutputData - index check
 * @tc.desc      : param check test
 */
HWTEST_F(NativeParamCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_PARAM_CHECK_013, TestSize.Level2)
{
    AudioEncoderDemo *encoderDemo = new AudioEncoderDemo();
    OH_AVCodec *handle = encoderDemo->NativeCreateByName("OH.Media.Codec.Encoder.Audio.AAC");
    ASSERT_NE(nullptr, handle);

    OH_AVFormat *format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 2);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 44100);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, OH_BitsPerSample::SAMPLE_F32LE);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, OH_BitsPerSample::SAMPLE_F32LE);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_CHANNEL_LAYOUT, STEREO);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AAC_IS_ADTS, 1);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 169000);

    struct OH_AVCodecAsyncCallback cb = {&OnError, &OnOutputFormatChanged, &OnInputBufferAvailable,
                                         &OnOutputBufferAvailable};
    OH_AVErrCode ret = encoderDemo->NativeSetCallback(handle, cb);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = encoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = encoderDemo->NativePrepare(handle);
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = encoderDemo->NativeStart(handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = encoderDemo->NativeFreeOutputData(handle, -1);
    ASSERT_EQ(AV_ERR_NO_MEMORY, ret);

    OH_AVFormat_Destroy(format);
    encoderDemo->NativeDestroy(handle);
    delete encoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_PARAM_CHECK_014
 * @tc.name      : OH_AudioEncoder_Configure - format(FLAC MD_KEY_BITRATE) check
 * @tc.desc      : param check test
 */
HWTEST_F(NativeParamCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_PARAM_CHECK_014, TestSize.Level2)
{
    AudioEncoderDemo* encoderDemo = new AudioEncoderDemo();
    OH_AVCodec* handle = encoderDemo->NativeCreateByName("OH.Media.Codec.Encoder.Audio.Flac");
    ASSERT_NE(nullptr, handle);

    OH_AVFormat* format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, CHANNEL_COUNT_FLAC);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, SAMPLE_RATE_FLAC);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, OH_BitsPerSample::SAMPLE_S16LE);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, OH_BitsPerSample::SAMPLE_S16LE);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_CHANNEL_LAYOUT, STEREO);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, BITS_RATE_FLAC);

    OH_AVErrCode ret = encoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    encoderDemo->NativeReset(handle);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, -1);
    ret = encoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_UNSUPPORT, ret);

    encoderDemo->NativeReset(handle);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITRATE, 1);
    ret = encoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_UNSUPPORT, ret);

    encoderDemo->NativeReset(handle);
    OH_AVFormat_SetStringValue(format, OH_MD_KEY_BITRATE, "aaa");
    ret = encoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_UNSUPPORT, ret);

    encoderDemo->NativeReset(handle);
    OH_AVFormat_SetFloatValue(format, OH_MD_KEY_BITRATE, 0.1);
    ret = encoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_UNSUPPORT, ret);

    encoderDemo->NativeReset(handle);
    OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_BITRATE, 0.1);
    ret = encoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_UNSUPPORT, ret);

    uint8_t a[100];
    encoderDemo->NativeReset(handle);
    OH_AVFormat_SetBuffer(format, OH_MD_KEY_BITRATE, a, 100);
    ret = encoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_UNSUPPORT, ret);

    OH_AVFormat_Destroy(format);
    encoderDemo->NativeDestroy(handle);
    delete encoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_PARAM_CHECK_015
 * @tc.name      : OH_AudioEncoder_SetParameter - format(FLAC OH_MD_KEY_AUD_CHANNEL_COUNT) check
 * @tc.desc      : param check test
 */
HWTEST_F(NativeParamCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_PARAM_CHECK_015, TestSize.Level2)
{
    AudioEncoderDemo* encoderDemo = new AudioEncoderDemo();
    OH_AVCodec* handle = encoderDemo->NativeCreateByName("OH.Media.Codec.Encoder.Audio.Flac");
    ASSERT_NE(nullptr, handle);

    OH_AVFormat* format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, CHANNEL_COUNT_FLAC);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, SAMPLE_RATE_FLAC);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, OH_BitsPerSample::SAMPLE_S16LE);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, OH_BitsPerSample::SAMPLE_S16LE);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_CHANNEL_LAYOUT, STEREO);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, BITS_RATE_FLAC);

    OH_AVErrCode ret = encoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    encoderDemo->NativeReset(handle);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, -1);
    ret = encoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_UNSUPPORT, ret);

    encoderDemo->NativeReset(handle);
    OH_AVFormat_SetStringValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, "aaa");
    ret = encoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_UNSUPPORT, ret);

    encoderDemo->NativeReset(handle);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 0);
    ret = encoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_UNSUPPORT, ret);

    encoderDemo->NativeReset(handle);
    OH_AVFormat_SetFloatValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 0.1);
    ret = encoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_UNSUPPORT, ret);

    encoderDemo->NativeReset(handle);
    OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 0.1);
    ret = encoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_UNSUPPORT, ret);

    uint8_t a[100];
    encoderDemo->NativeReset(handle);
    OH_AVFormat_SetBuffer(format, OH_MD_KEY_AUD_CHANNEL_COUNT, a, 100);
    ret = encoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_UNSUPPORT, ret);

    OH_AVFormat_Destroy(format);
    encoderDemo->NativeDestroy(handle);
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_PARAM_CHECK_016
 * @tc.name      : OH_AudioEncoder_SetParameter - format(FLAC OH_MD_KEY_AUD_SAMPLE_RATE) check
 * @tc.desc      : param check test
 */
HWTEST_F(NativeParamCheckTest, SUB_MULTIMEDIA_AUDIO_ENCODER_PARAM_CHECK_016, TestSize.Level2)
{
    AudioEncoderDemo* encoderDemo = new AudioEncoderDemo();
    OH_AVCodec* handle = encoderDemo->NativeCreateByName("OH.Media.Codec.Encoder.Audio.Flac");
    ASSERT_NE(nullptr, handle);

    OH_AVFormat* format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, CHANNEL_COUNT_FLAC);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, SAMPLE_RATE_FLAC);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, OH_BitsPerSample::SAMPLE_S16LE);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, OH_BitsPerSample::SAMPLE_S16LE);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_CHANNEL_LAYOUT, STEREO);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, BITS_RATE_FLAC);

    OH_AVErrCode ret = encoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    encoderDemo->NativeReset(handle);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, -1);
    ret = encoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_UNSUPPORT, ret);

    encoderDemo->NativeReset(handle);
    OH_AVFormat_SetStringValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, "aaa");
    ret = encoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_UNSUPPORT, ret);

    encoderDemo->NativeReset(handle);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 0);
    ret = encoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_UNSUPPORT, ret);

    encoderDemo->NativeReset(handle);
    OH_AVFormat_SetFloatValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 0.1);
    ret = encoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_UNSUPPORT, ret);

    encoderDemo->NativeReset(handle);
    OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 0.1);
    ret = encoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_UNSUPPORT, ret);

    uint8_t a[100];
    encoderDemo->NativeReset(handle);
    OH_AVFormat_SetBuffer(format, OH_MD_KEY_AUD_SAMPLE_RATE, a, 100);
    ret = encoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_UNSUPPORT, ret);

    OH_AVFormat_Destroy(format);
    encoderDemo->NativeDestroy(handle);
    delete encoderDemo;
}