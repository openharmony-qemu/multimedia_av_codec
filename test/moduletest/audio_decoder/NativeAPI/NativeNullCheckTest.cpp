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
    class NativeNullCheckTest : public testing::Test {
    public:
        static void SetUpTestCase();
        static void TearDownTestCase();
        void SetUp() override;
        void TearDown() override;
    };

    void NativeNullCheckTest::SetUpTestCase() {}
    void NativeNullCheckTest::TearDownTestCase() {}
    void NativeNullCheckTest::SetUp() {}
    void NativeNullCheckTest::TearDown() {}
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_NULL_CHECK_001
 * @tc.name      : OH_AudioDecoder_CreateByMime - mime check
 * @tc.desc      : null check test
 */
HWTEST_F(NativeNullCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_NULL_CHECK_001, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    OH_AVCodec* handle = decoderDemo->NativeCreateByMime(nullptr);
    ASSERT_EQ(nullptr, handle);

    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_NULL_CHECK_002
 * @tc.name      : OH_AudioDecoder_CreateByName - name check
 * @tc.desc      : null check test
 */
HWTEST_F(NativeNullCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_NULL_CHECK_002, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    OH_AVCodec* handle = decoderDemo->NativeCreateByName(nullptr);
    ASSERT_EQ(nullptr, handle);

    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_NULL_CHECK_003
 * @tc.name      : OH_AudioDecoder_Destory - codec check
 * @tc.desc      : null check test
 */
HWTEST_F(NativeNullCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_NULL_CHECK_003, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    OH_AVErrCode ret = decoderDemo->NativeDestroy(nullptr);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_NULL_CHECK_004
 * @tc.name      : OH_AudioDecoder_SetCallback - codec check
 * @tc.desc      : null check test
 */
HWTEST_F(NativeNullCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_NULL_CHECK_004, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    OH_AVCodec* handle = decoderDemo->NativeCreateByName("OH.Media.Codec.Decoder.Audio.Mpeg");
    ASSERT_NE(nullptr, handle);

    struct OH_AVCodecAsyncCallback cb = { &OnError, &OnOutputFormatChanged,
    &OnInputBufferAvailable, &OnOutputBufferAvailable };
    OH_AVErrCode ret = decoderDemo->NativeSetCallback(nullptr, cb);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    decoderDemo->NativeDestroy(handle);
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_NULL_CHECK_005
 * @tc.name      : OH_AudioDecoder_SetCallback - callback check
 * @tc.desc      : null check test
 */
HWTEST_F(NativeNullCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_NULL_CHECK_005, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    OH_AVCodec* handle = decoderDemo->NativeCreateByName("OH.Media.Codec.Decoder.Audio.Mpeg");
    ASSERT_NE(nullptr, handle);

    struct OH_AVCodecAsyncCallback cb = { &OnError, &OnOutputFormatChanged,
    &OnInputBufferAvailable, &OnOutputBufferAvailable };
    OH_AVErrCode ret;

    cb.onError = nullptr;
    ret = decoderDemo->NativeSetCallback(handle, cb);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    cb.onStreamChanged = nullptr;
    ret = decoderDemo->NativeSetCallback(handle, cb);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    cb.onNeedInputData = nullptr;
    ret = decoderDemo->NativeSetCallback(handle, cb);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    cb.onNeedOutputData = nullptr;
    ret = decoderDemo->NativeSetCallback(handle, cb);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    decoderDemo->NativeDestroy(handle);
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_NULL_CHECK_006
 * @tc.name      : OH_AudioDecoder_Configure - codec check
 * @tc.desc      : null check test
 */
HWTEST_F(NativeNullCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_NULL_CHECK_006, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    OH_AVCodec* handle = decoderDemo->NativeCreateByName("OH.Media.Codec.Decoder.Audio.Mpeg");
    ASSERT_NE(nullptr, handle);

    OH_AVFormat* format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 2);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 44100);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, 4);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 169000);

    OH_AVErrCode ret = decoderDemo->NativeConfigure(nullptr, format);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    OH_AVFormat_Destroy(format);
    decoderDemo->NativeDestroy(handle);
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_NULL_CHECK_007
 * @tc.name      : OH_AudioDecoder_Configure - format check
 * @tc.desc      : null check test
 */
HWTEST_F(NativeNullCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_NULL_CHECK_007, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    OH_AVCodec* handle = decoderDemo->NativeCreateByName("OH.Media.Codec.Decoder.Audio.Mpeg");
    ASSERT_NE(nullptr, handle);

    OH_AVErrCode ret = decoderDemo->NativeConfigure(handle, nullptr);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    decoderDemo->NativeDestroy(handle);
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_NULL_CHECK_008
 * @tc.name      : OH_AudioDecoder_Prepare - codec check
 * @tc.desc      : null check test
 */
HWTEST_F(NativeNullCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_NULL_CHECK_008, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    OH_AVCodec* handle = decoderDemo->NativeCreateByName("OH.Media.Codec.Decoder.Audio.Mpeg");
    ASSERT_NE(nullptr, handle);

    OH_AVFormat* format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 2);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 44100);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, 4);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 169000);

    struct OH_AVCodecAsyncCallback cb = { &OnError, &OnOutputFormatChanged,
    &OnInputBufferAvailable, &OnOutputBufferAvailable };
    OH_AVErrCode ret = decoderDemo->NativeSetCallback(handle, cb);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = decoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = decoderDemo->NativePrepare(nullptr);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    OH_AVFormat_Destroy(format);
    decoderDemo->NativeDestroy(handle);
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_NULL_CHECK_009
 * @tc.name      : OH_AudioDecoder_Start - codec check
 * @tc.desc      : null check test
 */
HWTEST_F(NativeNullCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_NULL_CHECK_009, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    OH_AVCodec* handle = decoderDemo->NativeCreateByName("OH.Media.Codec.Decoder.Audio.Mpeg");
    ASSERT_NE(nullptr, handle);

    OH_AVFormat* format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 2);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 44100);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, 4);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 169000);

    struct OH_AVCodecAsyncCallback cb = { &OnError, &OnOutputFormatChanged,
    &OnInputBufferAvailable, &OnOutputBufferAvailable };
    OH_AVErrCode ret = decoderDemo->NativeSetCallback(handle, cb);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = decoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = decoderDemo->NativePrepare(handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = decoderDemo->NativeStart(nullptr);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    OH_AVFormat_Destroy(format);
    decoderDemo->NativeDestroy(handle);
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_NULL_CHECK_010
 * @tc.name      : OH_AudioDecoder_Stop - codec check
 * @tc.desc      : null check test
 */
HWTEST_F(NativeNullCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_NULL_CHECK_010, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    OH_AVCodec* handle = decoderDemo->NativeCreateByName("OH.Media.Codec.Decoder.Audio.Mpeg");
    ASSERT_NE(nullptr, handle);

    OH_AVFormat* format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 2);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 44100);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, 4);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 169000);

    struct OH_AVCodecAsyncCallback cb = { &OnError, &OnOutputFormatChanged,
    &OnInputBufferAvailable, &OnOutputBufferAvailable };
    OH_AVErrCode ret = decoderDemo->NativeSetCallback(handle, cb);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = decoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = decoderDemo->NativePrepare(handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = decoderDemo->NativeStart(handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = decoderDemo->NativeStop(nullptr);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    OH_AVFormat_Destroy(format);
    decoderDemo->NativeDestroy(handle);
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_NULL_CHECK_011
 * @tc.name      : OH_AudioDecoder_Flush - codec check
 * @tc.desc      : null check test
 */
HWTEST_F(NativeNullCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_NULL_CHECK_011, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    OH_AVCodec* handle = decoderDemo->NativeCreateByName("OH.Media.Codec.Decoder.Audio.Mpeg");
    ASSERT_NE(nullptr, handle);

    OH_AVFormat* format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 2);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 44100);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, 4);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 169000);

    struct OH_AVCodecAsyncCallback cb = { &OnError, &OnOutputFormatChanged,
    &OnInputBufferAvailable, &OnOutputBufferAvailable };
    OH_AVErrCode ret = decoderDemo->NativeSetCallback(handle, cb);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = decoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = decoderDemo->NativePrepare(handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = decoderDemo->NativeStart(handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = decoderDemo->NativeFlush(nullptr);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    OH_AVFormat_Destroy(format);
    decoderDemo->NativeDestroy(handle);
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_NULL_CHECK_012
 * @tc.name      : OH_AudioDecoder_Reset - codec check
 * @tc.desc      : null check test
 */
HWTEST_F(NativeNullCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_NULL_CHECK_012, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    OH_AVCodec* handle = decoderDemo->NativeCreateByName("OH.Media.Codec.Decoder.Audio.Mpeg");
    ASSERT_NE(nullptr, handle);

    OH_AVErrCode ret = decoderDemo->NativeReset(nullptr);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
    
    decoderDemo->NativeDestroy(handle);
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_NULL_CHECK_013
 * @tc.name      : OH_AudioDecoder_GetOutputDescription - codec check
 * @tc.desc      : null check test
 */
HWTEST_F(NativeNullCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_NULL_CHECK_013, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    OH_AVCodec* handle = decoderDemo->NativeCreateByName("OH.Media.Codec.Decoder.Audio.Mpeg");
    ASSERT_NE(nullptr, handle);

    OH_AVFormat* format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 2);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 44100);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, 4);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 169000);

    struct OH_AVCodecAsyncCallback cb = { &OnError, &OnOutputFormatChanged,
    &OnInputBufferAvailable, &OnOutputBufferAvailable };
    OH_AVErrCode ret = decoderDemo->NativeSetCallback(handle, cb);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = decoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = decoderDemo->NativePrepare(handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = decoderDemo->NativeStart(handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVFormat* formatRet = decoderDemo->NativeGetOutputDescription(nullptr);
    ASSERT_EQ(nullptr, formatRet);

    OH_AVFormat_Destroy(format);
    decoderDemo->NativeDestroy(handle);
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_NULL_CHECK_014
 * @tc.name      : OH_AudioDecoder_SetParameter - codec check
 * @tc.desc      : null check test
 */
HWTEST_F(NativeNullCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_NULL_CHECK_014, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    OH_AVCodec* handle = decoderDemo->NativeCreateByName("OH.Media.Codec.Decoder.Audio.Mpeg");
    ASSERT_NE(nullptr, handle);

    OH_AVFormat* format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 2);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 44100);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, 4);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 169000);

    struct OH_AVCodecAsyncCallback cb = { &OnError, &OnOutputFormatChanged,
    &OnInputBufferAvailable, &OnOutputBufferAvailable };
    OH_AVErrCode ret = decoderDemo->NativeSetCallback(handle, cb);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = decoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = decoderDemo->NativePrepare(handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = decoderDemo->NativeStart(handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = decoderDemo->NativeSetParameter(nullptr, format);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    OH_AVFormat_Destroy(format);
    decoderDemo->NativeDestroy(handle);
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_NULL_CHECK_015
 * @tc.name      : OH_AudioDecoder_SetParameter - format check
 * @tc.desc      : null check test
 */
HWTEST_F(NativeNullCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_NULL_CHECK_015, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    OH_AVCodec* handle = decoderDemo->NativeCreateByName("OH.Media.Codec.Decoder.Audio.Mpeg");
    ASSERT_NE(nullptr, handle);

    OH_AVFormat* format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 2);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 44100);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, 4);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 169000);

    struct OH_AVCodecAsyncCallback cb = { &OnError, &OnOutputFormatChanged,
    &OnInputBufferAvailable, &OnOutputBufferAvailable };
    OH_AVErrCode ret = decoderDemo->NativeSetCallback(handle, cb);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = decoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = decoderDemo->NativePrepare(handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = decoderDemo->NativeStart(handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = decoderDemo->NativeSetParameter(handle, nullptr);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    OH_AVFormat_Destroy(format);
    decoderDemo->NativeDestroy(handle);
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_NULL_CHECK_016
 * @tc.name      : OH_AudioDecoder_PushInputData - codec check
 * @tc.desc      : null check test
 */
HWTEST_F(NativeNullCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_NULL_CHECK_016, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    OH_AVCodec* handle = decoderDemo->NativeCreateByName("OH.Media.Codec.Decoder.Audio.Mpeg");
    ASSERT_NE(nullptr, handle);

    OH_AVFormat* format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 2);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 44100);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, 4);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 169000);

    struct OH_AVCodecAsyncCallback cb = { &OnError, &OnOutputFormatChanged,
    &OnInputBufferAvailable, &OnOutputBufferAvailable };
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
    ret = decoderDemo->NativePushInputData(nullptr, index, info);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    OH_AVFormat_Destroy(format);
    decoderDemo->NativeDestroy(handle);
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_NULL_CHECK_017
 * @tc.name      : OH_AudioDecoder_FreeOutputData - codec check
 * @tc.desc      : null check test
 */
HWTEST_F(NativeNullCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_NULL_CHECK_017, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    OH_AVCodec* handle = decoderDemo->NativeCreateByName("OH.Media.Codec.Decoder.Audio.Mpeg");
    ASSERT_NE(nullptr, handle);

    OH_AVFormat* format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 2);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 44100);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, 4);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 169000);

    struct OH_AVCodecAsyncCallback cb = { &OnError, &OnOutputFormatChanged,
    &OnInputBufferAvailable, &OnOutputBufferAvailable };
    OH_AVErrCode ret = decoderDemo->NativeSetCallback(handle, cb);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = decoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = decoderDemo->NativePrepare(handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = decoderDemo->NativeStart(handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    uint32_t index = 0;
    ret = decoderDemo->NativeFreeOutputData(nullptr, index);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    OH_AVFormat_Destroy(format);
    decoderDemo->NativeDestroy(handle);
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_NULL_CHECK_018
 * @tc.name      : OH_AudioDecoder_IsValid - codec check
 * @tc.desc      : null check test
 */
HWTEST_F(NativeNullCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_NULL_CHECK_018, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    OH_AVCodec* handle = decoderDemo->NativeCreateByName("OH.Media.Codec.Decoder.Audio.Mpeg");
    ASSERT_NE(nullptr, handle);

    OH_AVFormat* format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 2);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 44100);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, 4);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 169000);

    struct OH_AVCodecAsyncCallback cb = { &OnError, &OnOutputFormatChanged,
    &OnInputBufferAvailable, &OnOutputBufferAvailable };
    OH_AVErrCode ret = decoderDemo->NativeSetCallback(handle, cb);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = decoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = decoderDemo->NativePrepare(handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = decoderDemo->NativeStart(handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    bool isVaild;
    ret = decoderDemo->NativeIsValid(nullptr, &isVaild);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    OH_AVFormat_Destroy(format);
    decoderDemo->NativeDestroy(handle);
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_NULL_CHECK_019
 * @tc.name      : OH_AudioDecoder_IsValid - codec check
 * @tc.desc      : null check test
 */
HWTEST_F(NativeNullCheckTest, SUB_MULTIMEDIA_AUDIO_DECODER_NULL_CHECK_019, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    OH_AVCodec* handle = decoderDemo->NativeCreateByName("OH.Media.Codec.Decoder.Audio.Mpeg");
    ASSERT_NE(nullptr, handle);

    OH_AVFormat* format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 2);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 44100);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, 4);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 169000);

    struct OH_AVCodecAsyncCallback cb = { &OnError, &OnOutputFormatChanged,
    &OnInputBufferAvailable, &OnOutputBufferAvailable };
    OH_AVErrCode ret = decoderDemo->NativeSetCallback(handle, cb);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = decoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = decoderDemo->NativePrepare(handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = decoderDemo->NativeStart(handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = decoderDemo->NativeIsValid(handle, nullptr);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);

    OH_AVFormat_Destroy(format);
    decoderDemo->NativeDestroy(handle);
    delete decoderDemo;
}