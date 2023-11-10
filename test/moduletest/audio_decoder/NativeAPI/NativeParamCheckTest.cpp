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
#include "AudioDecoderDemoCommon.h"

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
} // namespace

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_PARAM_CHECK_001
 * @tc.name      : OH_AudioDecoder_CreateByMime - mime check
 * @tc.desc      : param check test
 */
HWTEST_F(NativeParamCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_PARAM_CHECK_001, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    OH_AVCodec *handle = decoderDemo->NativeCreateByMime("aaa");
    ASSERT_EQ(nullptr, handle);

    handle = decoderDemo->NativeCreateByMime(OH_AVCODEC_MIMETYPE_AUDIO_AAC);
    ASSERT_NE(nullptr, handle);

    decoderDemo->NativeDestroy(handle);
    handle = nullptr;
    handle = decoderDemo->NativeCreateByMime(OH_AVCODEC_MIMETYPE_AUDIO_MPEG);
    ASSERT_NE(nullptr, handle);
    decoderDemo->NativeDestroy(handle);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_PARAM_CHECK_002
 * @tc.name      : OH_AudioDecoder_CreateByName - name check
 * @tc.desc      : param check test
 */
HWTEST_F(NativeParamCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_PARAM_CHECK_002, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    OH_AVCodec *handle = decoderDemo->NativeCreateByName("aaa");
    ASSERT_EQ(nullptr, handle);

    handle = decoderDemo->NativeCreateByName("OH.Media.Codec.Encoder.Audio.AAC");
    ASSERT_EQ(nullptr, handle);

    handle = decoderDemo->NativeCreateByName("OH.Media.Codec.Decoder.Audio.AAC");
    ASSERT_NE(nullptr, handle);

    decoderDemo->NativeDestroy(handle);
    handle = nullptr;
    handle = decoderDemo->NativeCreateByName("OH.Media.Codec.Decoder.Audio.Mpeg");
    ASSERT_NE(nullptr, handle);

    decoderDemo->NativeDestroy(handle);
    handle = nullptr;
    handle = decoderDemo->NativeCreateByName("OH.Media.Codec.Decoder.Audio.Flac");
    ASSERT_NE(nullptr, handle);

    decoderDemo->NativeDestroy(handle);
    handle = nullptr;
    handle = decoderDemo->NativeCreateByName("OH.Media.Codec.Decoder.Audio.Vorbis");
    ASSERT_NE(nullptr, handle);

    decoderDemo->NativeDestroy(handle);
    handle = nullptr;
    handle = decoderDemo->NativeCreateByName("OH.Media.Codec.Decoder.Audio.Amrwb");
    ASSERT_NE(nullptr, handle);

    decoderDemo->NativeDestroy(handle);
    handle = nullptr;
    handle = decoderDemo->NativeCreateByName("OH.Media.Codec.Decoder.Audio.Amrnb");
    ASSERT_NE(nullptr, handle);

    decoderDemo->NativeDestroy(handle);
    handle = nullptr;
    handle = decoderDemo->NativeCreateByName("OH.Media.Codec.Decoder.Audio.G711mu");
    ASSERT_NE(nullptr, handle);

    decoderDemo->NativeDestroy(handle);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_PARAM_CHECK_003
 * @tc.name      : OH_AudioDecoder_Configure - format(MD_KEY_BITRATE) check
 * @tc.desc      : param check test
 */
HWTEST_F(NativeParamCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_PARAM_CHECK_003, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    OH_AVCodec *handle = decoderDemo->NativeCreateByName("OH.Media.Codec.Decoder.Audio.Mpeg");
    ASSERT_NE(nullptr, handle);

    OH_AVFormat *format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 2);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 44100);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 169000);

    OH_AVErrCode ret = decoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    decoderDemo->NativeReset(handle);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, -1);
    ret = decoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    decoderDemo->NativeReset(handle);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITRATE, 1);
    ret = decoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    decoderDemo->NativeReset(handle);
    OH_AVFormat_SetStringValue(format, OH_MD_KEY_BITRATE, "aaa");
    ret = decoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    decoderDemo->NativeReset(handle);
    OH_AVFormat_SetFloatValue(format, OH_MD_KEY_BITRATE, 0.1);
    ret = decoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    decoderDemo->NativeReset(handle);
    OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_BITRATE, 0.1);
    ret = decoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    uint8_t a[100];
    decoderDemo->NativeReset(handle);
    OH_AVFormat_SetBuffer(format, OH_MD_KEY_BITRATE, a, 100);
    ret = decoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVFormat_Destroy(format);
    decoderDemo->NativeDestroy(handle);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_PARAM_CHECK_004
 * @tc.name      : OH_AudioDecoder_Configure - format(OH_MD_KEY_AUD_CHANNEL_COUNT) check
 * @tc.desc      : param check test
 */
HWTEST_F(NativeParamCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_PARAM_CHECK_004, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    OH_AVCodec *handle = decoderDemo->NativeCreateByName("OH.Media.Codec.Decoder.Audio.Mpeg");
    ASSERT_NE(nullptr, handle);

    OH_AVFormat *format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 2);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 44100);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 169000);

    OH_AVErrCode ret = decoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    decoderDemo->NativeReset(handle);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, -1);
    ret = decoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    decoderDemo->NativeReset(handle);
    OH_AVFormat_SetStringValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, "aaa");
    ret = decoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    decoderDemo->NativeReset(handle);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 0);
    ret = decoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    decoderDemo->NativeReset(handle);
    OH_AVFormat_SetFloatValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 0.1);
    ret = decoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    decoderDemo->NativeReset(handle);
    OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 0.1);
    ret = decoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    uint8_t a[100];
    decoderDemo->NativeReset(handle);
    OH_AVFormat_SetBuffer(format, OH_MD_KEY_AUD_CHANNEL_COUNT, a, 100);
    ret = decoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    OH_AVFormat_Destroy(format);
    decoderDemo->NativeDestroy(handle);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_PARAM_CHECK_005
 * @tc.name      : OH_AudioDecoder_Configure - format(OH_MD_KEY_AUD_SAMPLE_RATE) check
 * @tc.desc      : param check test
 */
HWTEST_F(NativeParamCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_PARAM_CHECK_005, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    OH_AVCodec *handle = decoderDemo->NativeCreateByName("OH.Media.Codec.Decoder.Audio.Mpeg");
    ASSERT_NE(nullptr, handle);

    OH_AVFormat *format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 2);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 44100);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 169000);

    OH_AVErrCode ret = decoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    decoderDemo->NativeReset(handle);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, -1);
    ret = decoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    decoderDemo->NativeReset(handle);
    OH_AVFormat_SetStringValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, "aaa");
    ret = decoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    decoderDemo->NativeReset(handle);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 0);
    ret = decoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    decoderDemo->NativeReset(handle);
    OH_AVFormat_SetFloatValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 0.1);
    ret = decoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    decoderDemo->NativeReset(handle);
    OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 0.1);
    ret = decoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    uint8_t a[100];
    decoderDemo->NativeReset(handle);
    OH_AVFormat_SetBuffer(format, OH_MD_KEY_AUD_SAMPLE_RATE, a, 100);
    ret = decoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    OH_AVFormat_Destroy(format);
    decoderDemo->NativeDestroy(handle);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_PARAM_CHECK_006
 * @tc.name      : OH_AudioDecoder_SetParameter - format(MD_KEY_BITRATE) check
 * @tc.desc      : param check test
 */
HWTEST_F(NativeParamCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_PARAM_CHECK_006, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    OH_AVCodec *handle = decoderDemo->NativeCreateByName("OH.Media.Codec.Decoder.Audio.Mpeg");
    ASSERT_NE(nullptr, handle);

    OH_AVFormat *format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 2);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 44100);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 169000);

    OH_AVErrCode ret = decoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = decoderDemo->NativePrepare(handle);
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = decoderDemo->NativeStart(handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = decoderDemo->NativeSetParameter(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, -1);
    ret = decoderDemo->NativeSetParameter(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVFormat_SetStringValue(format, OH_MD_KEY_BITRATE, "aaa");
    ret = decoderDemo->NativeSetParameter(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITRATE, 0);
    ret = decoderDemo->NativeSetParameter(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVFormat_SetFloatValue(format, OH_MD_KEY_BITRATE, 0.1);
    ret = decoderDemo->NativeSetParameter(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_BITRATE, 0.1);
    ret = decoderDemo->NativeSetParameter(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    uint8_t a[100];
    OH_AVFormat_SetBuffer(format, OH_MD_KEY_AUD_SAMPLE_RATE, a, 100);
    ret = decoderDemo->NativeSetParameter(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVFormat_Destroy(format);
    decoderDemo->NativeDestroy(handle);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_PARAM_CHECK_007
 * @tc.name      : OH_AudioDecoder_SetParameter - format(OH_MD_KEY_AUD_CHANNEL_COUNT) check
 * @tc.desc      : param check test
 */
HWTEST_F(NativeParamCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_PARAM_CHECK_007, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    OH_AVCodec *handle = decoderDemo->NativeCreateByName("OH.Media.Codec.Decoder.Audio.Mpeg");
    ASSERT_NE(nullptr, handle);

    OH_AVFormat *format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 2);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 44100);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 169000);

    OH_AVErrCode ret = decoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = decoderDemo->NativePrepare(handle);
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = decoderDemo->NativeStart(handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = decoderDemo->NativeSetParameter(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, -1);
    ret = decoderDemo->NativeSetParameter(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVFormat_SetStringValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, "aaa");
    ret = decoderDemo->NativeSetParameter(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVFormat_SetLongValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 0);
    ret = decoderDemo->NativeSetParameter(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVFormat_SetFloatValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 0.1);
    ret = decoderDemo->NativeSetParameter(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 0.1);
    ret = decoderDemo->NativeSetParameter(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    uint8_t a[100];
    OH_AVFormat_SetBuffer(format, OH_MD_KEY_AUD_CHANNEL_COUNT, a, 100);
    ret = decoderDemo->NativeSetParameter(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVFormat_Destroy(format);
    decoderDemo->NativeDestroy(handle);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_PARAM_CHECK_008
 * @tc.name      : OH_AudioDecoder_SetParameter - format(OH_MD_KEY_AUD_SAMPLE_RATE) check
 * @tc.desc      : param check test
 */
HWTEST_F(NativeParamCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_PARAM_CHECK_008, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    OH_AVCodec *handle = decoderDemo->NativeCreateByName("OH.Media.Codec.Decoder.Audio.Mpeg");
    ASSERT_NE(nullptr, handle);

    OH_AVFormat *format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 2);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 44100);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 169000);

    OH_AVErrCode ret = decoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = decoderDemo->NativePrepare(handle);
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = decoderDemo->NativeStart(handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = decoderDemo->NativeSetParameter(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, -1);
    ret = decoderDemo->NativeSetParameter(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVFormat_SetStringValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, "aaa");
    ret = decoderDemo->NativeSetParameter(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVFormat_SetLongValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 0);
    ret = decoderDemo->NativeSetParameter(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVFormat_SetFloatValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 0.1);
    ret = decoderDemo->NativeSetParameter(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 0.1);
    ret = decoderDemo->NativeSetParameter(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    uint8_t a[100];
    OH_AVFormat_SetBuffer(format, OH_MD_KEY_AUD_SAMPLE_RATE, a, 100);
    ret = decoderDemo->NativeSetParameter(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVFormat_Destroy(format);
    decoderDemo->NativeDestroy(handle);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_PARAM_CHECK_009
 * @tc.name      : OH_AudioDecoder_PushInputData - index check
 * @tc.desc      : param check test
 */
HWTEST_F(NativeParamCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_PARAM_CHECK_009, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    OH_AVCodec *handle = decoderDemo->NativeCreateByName("OH.Media.Codec.Decoder.Audio.Mpeg");
    ASSERT_NE(nullptr, handle);

    OH_AVFormat *format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 2);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 44100);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 169000);

    struct OH_AVCodecAsyncCallback cb = {&OnError, &OnOutputFormatChanged, &OnInputBufferAvailable,
                                         &OnOutputBufferAvailable};
    OH_AVErrCode ret = decoderDemo->NativeSetCallback(handle, cb);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = decoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = decoderDemo->NativePrepare(handle);
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = decoderDemo->NativeStart(handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVCodecBufferAttr info;
    info.size = 100;
    info.offset = 0;
    info.pts = 0;
    info.flags = AVCODEC_BUFFER_FLAGS_NONE;

    uint32_t index = decoderDemo->NativeGetInputIndex();
    ret = decoderDemo->NativePushInputData(handle, index, info);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = decoderDemo->NativePushInputData(handle, -1, info);
    ASSERT_EQ(AV_ERR_NO_MEMORY, ret);

    OH_AVFormat_Destroy(format);
    decoderDemo->NativeDestroy(handle);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_PARAM_CHECK_010
 * @tc.name      : OH_AudioDecoder_PushInputData - info.size check
 * @tc.desc      : param check test
 */
HWTEST_F(NativeParamCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_PARAM_CHECK_010, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    OH_AVCodec *handle = decoderDemo->NativeCreateByName("OH.Media.Codec.Decoder.Audio.Mpeg");
    ASSERT_NE(nullptr, handle);

    OH_AVFormat *format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 2);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 44100);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 169000);

    struct OH_AVCodecAsyncCallback cb = {&OnError, &OnOutputFormatChanged, &OnInputBufferAvailable,
                                         &OnOutputBufferAvailable};
    OH_AVErrCode ret = decoderDemo->NativeSetCallback(handle, cb);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = decoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = decoderDemo->NativePrepare(handle);
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = decoderDemo->NativeStart(handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVCodecBufferAttr info;
    info.size = 100;
    info.offset = 0;
    info.pts = 0;
    info.flags = AVCODEC_BUFFER_FLAGS_NONE;

    uint32_t index = decoderDemo->NativeGetInputIndex();
    ret = decoderDemo->NativePushInputData(handle, index, info);
    ASSERT_EQ(AV_ERR_OK, ret);

    index = decoderDemo->NativeGetInputIndex();
    info.size = -1;
    ret = decoderDemo->NativePushInputData(handle, index, info);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    OH_AVFormat_Destroy(format);
    decoderDemo->NativeDestroy(handle);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_PARAM_CHECK_011
 * @tc.name      : OH_AudioDecoder_PushInputData - info.offset check
 * @tc.desc      : param check test
 */
HWTEST_F(NativeParamCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_PARAM_CHECK_011, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    OH_AVCodec *handle = decoderDemo->NativeCreateByName("OH.Media.Codec.Decoder.Audio.Mpeg");
    ASSERT_NE(nullptr, handle);

    OH_AVFormat *format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 2);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 44100);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 169000);

    struct OH_AVCodecAsyncCallback cb = {&OnError, &OnOutputFormatChanged, &OnInputBufferAvailable,
                                         &OnOutputBufferAvailable};
    OH_AVErrCode ret = decoderDemo->NativeSetCallback(handle, cb);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = decoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = decoderDemo->NativePrepare(handle);
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = decoderDemo->NativeStart(handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVCodecBufferAttr info;
    info.size = 100;
    info.offset = 0;
    info.pts = 0;
    info.flags = AVCODEC_BUFFER_FLAGS_NONE;

    uint32_t index = decoderDemo->NativeGetInputIndex();
    ret = decoderDemo->NativePushInputData(handle, index, info);
    ASSERT_EQ(AV_ERR_OK, ret);

    index = decoderDemo->NativeGetInputIndex();
    info.offset = -1;
    ret = decoderDemo->NativePushInputData(handle, index, info);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    OH_AVFormat_Destroy(format);
    decoderDemo->NativeDestroy(handle);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_PARAM_CHECK_012
 * @tc.name      : OH_AudioDecoder_PushInputData - info.pts check
 * @tc.desc      : param check test
 */
HWTEST_F(NativeParamCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_PARAM_CHECK_012, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    OH_AVCodec *handle = decoderDemo->NativeCreateByName("OH.Media.Codec.Decoder.Audio.Mpeg");
    ASSERT_NE(nullptr, handle);

    OH_AVFormat *format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 2);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 44100);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 169000);

    struct OH_AVCodecAsyncCallback cb = {&OnError, &OnOutputFormatChanged, &OnInputBufferAvailable,
                                         &OnOutputBufferAvailable};
    OH_AVErrCode ret = decoderDemo->NativeSetCallback(handle, cb);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = decoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = decoderDemo->NativePrepare(handle);
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = decoderDemo->NativeStart(handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVCodecBufferAttr info;
    info.size = 100;
    info.offset = 0;
    info.pts = 0;
    info.flags = AVCODEC_BUFFER_FLAGS_NONE;

    uint32_t index = decoderDemo->NativeGetInputIndex();
    ret = decoderDemo->NativePushInputData(handle, index, info);
    ASSERT_EQ(AV_ERR_OK, ret);

    index = decoderDemo->NativeGetInputIndex();
    info.pts = -1;
    ret = decoderDemo->NativePushInputData(handle, index, info);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVFormat_Destroy(format);
    decoderDemo->NativeDestroy(handle);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_PARAM_CHECK_013
 * @tc.name      : OH_AudioDecoder_FreeOutputData - index check
 * @tc.desc      : param check test
 */
HWTEST_F(NativeParamCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_PARAM_CHECK_013, TestSize.Level2)
{
    AudioDecoderDemo *decoderDemo = new AudioDecoderDemo();
    OH_AVCodec *handle = decoderDemo->NativeCreateByName("OH.Media.Codec.Decoder.Audio.Mpeg");
    ASSERT_NE(nullptr, handle);

    OH_AVFormat *format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 2);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 44100);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 169000);

    struct OH_AVCodecAsyncCallback cb = {&OnError, &OnOutputFormatChanged, &OnInputBufferAvailable,
                                         &OnOutputBufferAvailable};
    OH_AVErrCode ret = decoderDemo->NativeSetCallback(handle, cb);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = decoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = decoderDemo->NativePrepare(handle);
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = decoderDemo->NativeStart(handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = decoderDemo->NativeFreeOutputData(handle, -1);
    ASSERT_EQ(AV_ERR_NO_MEMORY, ret);

    OH_AVFormat_Destroy(format);
    decoderDemo->NativeDestroy(handle);
    delete decoderDemo;
}