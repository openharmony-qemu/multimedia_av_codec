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
    class NativeStablityTest : public testing::Test {
    public:
        static void SetUpTestCase();
        static void TearDownTestCase();
        void SetUp() override;
        void TearDown() override;
    };

    void NativeStablityTest::SetUpTestCase() {}
    void NativeStablityTest::TearDownTestCase() {}
    void NativeStablityTest::SetUp() {}
    void NativeStablityTest::TearDown() {}

    constexpr int RUN_TIMES = 2000;
    constexpr int RUN_TIME = 12 * 3600;
    constexpr uint32_t DEFAULT_AAC_TYPE = 1;

    constexpr int32_t CHANNEL_COUNT_AAC = 2;
    constexpr int32_t SAMPLE_RATE_AAC = 44100;
    constexpr int64_t BITS_RATE_AAC = 129000;

    constexpr int32_t CHANNEL_COUNT_FLAC = 2;
    constexpr int32_t SAMPLE_RATE_FLAC = 48000;
    constexpr int64_t BITS_RATE_FLAC = 128000;
    constexpr int32_t INPUT_SIZE_FLAC = COMMON_FLAC_NUM * CHANNEL_COUNT_FLAC * S16_BITS_PER_SAMPLE;

    int32_t g_testResult[16] = { -1 };

    OH_AVFormat* GetAVFormatByEncoder(string encoderName)
    {
        OH_AVFormat* format = OH_AVFormat_Create();
        if (encoderName == "OH.Media.Codec.Encoder.Audio.AAC") {
            OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, CHANNEL_COUNT_AAC);
            OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, SAMPLE_RATE_AAC);
            OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, OH_BitsPerSample::SAMPLE_F32LE);
            OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, OH_BitsPerSample::SAMPLE_F32LE);
            OH_AVFormat_SetLongValue(format, OH_MD_KEY_CHANNEL_LAYOUT, STEREO);
            OH_AVFormat_SetIntValue(format, OH_MD_KEY_AAC_IS_ADTS, DEFAULT_AAC_TYPE);
            OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, BITS_RATE_AAC);
        } else {
            OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, CHANNEL_COUNT_FLAC);
            OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, SAMPLE_RATE_FLAC);
            OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, OH_BitsPerSample::SAMPLE_S16LE);
            OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, OH_BitsPerSample::SAMPLE_S16LE);
            OH_AVFormat_SetLongValue(format, OH_MD_KEY_CHANNEL_LAYOUT, STEREO);
            OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, BITS_RATE_FLAC);
            OH_AVFormat_SetIntValue(format, OH_MD_KEY_MAX_INPUT_SIZE, INPUT_SIZE_FLAC);
        }
        return format;
    }

    void RunEncode(string encoderName, string inputFile, string outputFile, int32_t threadId)
    {
        AudioEncoderDemo* encoderDemo = new AudioEncoderDemo();

        OH_AVFormat* format = GetAVFormatByEncoder(encoderName);

        encoderDemo->NativeRunCase(inputFile, outputFile, encoderName.c_str(), format);

        OH_AVFormat_Destroy(format);
        delete encoderDemo;
    }

    void RunLongTimeFlush(string encoderName, string inputFile, string outputFile, int32_t threadId)
    {
        AudioEncoderDemo* encoderDemo = new AudioEncoderDemo();
        bool needConfigure = true;

        time_t startTime = time(nullptr);
        ASSERT_NE(startTime, -1);
        time_t curTime = startTime;

        OH_AVCodec* handle = encoderDemo->NativeCreateByName(encoderName.c_str());
        OH_AVFormat* format = GetAVFormatByEncoder(encoderName);
        struct OH_AVCodecAsyncCallback cb = {&OnError, &OnOutputFormatChanged, &OnInputBufferAvailable,
            &OnOutputBufferAvailable};
        encoderDemo->NativeSetCallback(handle, cb);

        while (difftime(curTime, startTime) < RUN_TIME) {
            encoderDemo->NativeRunCaseWithoutCreate(handle, inputFile, outputFile, format, encoderName.c_str(),
                needConfigure);
            needConfigure = false;
            encoderDemo->NativeFlush(handle);
            curTime = time(nullptr);
            ASSERT_NE(curTime, -1);
        }

        OH_AVFormat_Destroy(format);
        encoderDemo->NativeDestroy(handle);
        delete encoderDemo;
    }

    void RunLongTimeReset(string encoderName, string inputFile, string outputFile, int32_t threadId)
    {
        AudioEncoderDemo* encoderDemo = new AudioEncoderDemo();
        bool needConfigure = true;

        time_t startTime = time(nullptr);
        ASSERT_NE(startTime, -1);
        time_t curTime = startTime;

        OH_AVCodec* handle = encoderDemo->NativeCreateByName(encoderName.c_str());
        OH_AVFormat* format = GetAVFormatByEncoder(encoderName);
        struct OH_AVCodecAsyncCallback cb = {&OnError, &OnOutputFormatChanged, &OnInputBufferAvailable,
            &OnOutputBufferAvailable};
        encoderDemo->NativeSetCallback(handle, cb);

        while (difftime(curTime, startTime) < RUN_TIME) {
            encoderDemo->NativeRunCaseWithoutCreate(handle, inputFile, outputFile, format, encoderName.c_str(),
                needConfigure);
            needConfigure = false;
            encoderDemo->NativeFlush(handle);
            curTime = time(nullptr);
            ASSERT_NE(curTime, -1);
        }

        OH_AVFormat_Destroy(format);
        encoderDemo->NativeDestroy(handle);
        delete encoderDemo;
    }

    void RunLongTimeStop(string encoderName, string inputFile, string outputFile, int32_t threadId)
    {
        AudioEncoderDemo* encoderDemo = new AudioEncoderDemo();
        bool needConfigure = true;

        time_t startTime = time(nullptr);
        if (startTime < 0) {
            return;
        }
        time_t curTime = startTime;

        OH_AVCodec* handle = encoderDemo->NativeCreateByName(encoderName.c_str());
        OH_AVFormat* format = GetAVFormatByEncoder(encoderName);
        struct OH_AVCodecAsyncCallback cb = { &OnError, &OnOutputFormatChanged, &OnInputBufferAvailable,
            &OnOutputBufferAvailable };
        encoderDemo->NativeSetCallback(handle, cb);

        while (difftime(curTime, startTime) < RUN_TIME) {
            encoderDemo->NativeRunCaseWithoutCreate(handle, inputFile, outputFile, format, encoderName.c_str(),
                needConfigure);
            needConfigure = false;
            encoderDemo->NativeStop(handle);
            curTime = time(nullptr);
            ASSERT_NE(curTime, -1);
        }

        OH_AVFormat_Destroy(format);
        encoderDemo->NativeDestroy(handle);
        delete encoderDemo;
    }
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_STABILITY_001
 * @tc.name      : OH_AudioEncoder_CreateByMime 2000 times
 * @tc.desc      : stability
 */
HWTEST_F(NativeStablityTest, SUB_MULTIMEDIA_AUDIO_ENCODER_STABILITY_001, TestSize.Level2)
{
    AudioEncoderDemo* encoderDemo = new AudioEncoderDemo();
    double totalTime = 0;
    struct timeval start, end;
    for (int i = 0; i < RUN_TIMES; i++)
    {
        gettimeofday(&start, NULL);
        OH_AVCodec* handle = encoderDemo->NativeCreateByMime(OH_AVCODEC_MIMETYPE_AUDIO_AAC);
        gettimeofday(&end, NULL);
        totalTime += (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
        ASSERT_NE(nullptr, handle);
        encoderDemo->NativeDestroy(handle);
    }
    cout << "2000 times finish, run time is " << totalTime << endl;
    delete encoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_STABILITY_002
 * @tc.name      : OH_AudioEncoder_CreateByName 2000 times
 * @tc.desc      : stability
 */
HWTEST_F(NativeStablityTest, SUB_MULTIMEDIA_AUDIO_ENCODER_STABILITY_002, TestSize.Level2)
{
    AudioEncoderDemo* encoderDemo = new AudioEncoderDemo();
    double totalTime = 0;
    struct timeval start, end;
    for (int i = 0; i < RUN_TIMES; i++)
    {
        gettimeofday(&start, NULL);
        OH_AVCodec* handle = encoderDemo->NativeCreateByName("OH.Media.Codec.Encoder.Audio.AAC");
        gettimeofday(&end, NULL);
        totalTime += (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
        ASSERT_NE(nullptr, handle);
        encoderDemo->NativeDestroy(handle);
    }
    cout << "2000 times finish, run time is " << totalTime << endl;
    delete encoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_STABILITY_003
 * @tc.name      : OH_AudioEncoder_Destroy 2000 times
 * @tc.desc      : stability
 */
HWTEST_F(NativeStablityTest, SUB_MULTIMEDIA_AUDIO_ENCODER_STABILITY_003, TestSize.Level2)
{
    AudioEncoderDemo* encoderDemo = new AudioEncoderDemo();
    double totalTime = 0;
    struct timeval start, end;

    for (int i = 0; i < RUN_TIMES; i++)
    {
        OH_AVCodec* handle = encoderDemo->NativeCreateByName("OH.Media.Codec.Encoder.Audio.AAC");
        ASSERT_NE(nullptr, handle);
        gettimeofday(&start, NULL);
        encoderDemo->NativeDestroy(handle);
        gettimeofday(&end, NULL);
        totalTime += (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
    }
    cout << "2000 times finish, run time is " << totalTime << endl;
    delete encoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_STABILITY_004
 * @tc.name      : OH_AudioEncoder_SetCallback 2000 times
 * @tc.desc      : stability
 */
HWTEST_F(NativeStablityTest, SUB_MULTIMEDIA_AUDIO_ENCODER_STABILITY_004, TestSize.Level2)
{
    OH_AVErrCode ret;
    AudioEncoderDemo* encoderDemo = new AudioEncoderDemo();
    double totalTime = 0;
    struct timeval start, end;

    OH_AVCodec* handle = encoderDemo->NativeCreateByName("OH.Media.Codec.Encoder.Audio.AAC");
    ASSERT_NE(nullptr, handle);
    struct OH_AVCodecAsyncCallback cb = {&OnError, &OnOutputFormatChanged, &OnInputBufferAvailable,
        &OnOutputBufferAvailable};
    for (int i = 0; i < RUN_TIMES; i++)
    {
        gettimeofday(&start, NULL);
        ret = encoderDemo->NativeSetCallback(handle, cb);
        gettimeofday(&end, NULL);
        totalTime += (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
        ASSERT_EQ(AV_ERR_OK, ret);
    }
    cout << "2000 times finish, run time is " << totalTime << endl;
    encoderDemo->NativeDestroy(handle);
    delete encoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_STABILITY_005
 * @tc.name      : OH_AudioEncoder_Configure 2000 times
 * @tc.desc      : stability
 */
HWTEST_F(NativeStablityTest, SUB_MULTIMEDIA_AUDIO_ENCODER_STABILITY_005, TestSize.Level2)
{
    OH_AVErrCode ret;
    AudioEncoderDemo* encoderDemo = new AudioEncoderDemo();
    double totalTime = 0;
    struct timeval start, end;

    OH_AVFormat* format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 2);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 16000);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, OH_BitsPerSample::SAMPLE_F32LE);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, OH_BitsPerSample::SAMPLE_F32LE);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_CHANNEL_LAYOUT, STEREO);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AAC_IS_ADTS, DEFAULT_AAC_TYPE);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 169000);

    for (int i = 0; i < RUN_TIMES; i++)
    {
        OH_AVCodec* handle = encoderDemo->NativeCreateByName("OH.Media.Codec.Encoder.Audio.AAC");
        ASSERT_NE(nullptr, handle);
        struct OH_AVCodecAsyncCallback cb = {&OnError, &OnOutputFormatChanged, &OnInputBufferAvailable,
            &OnOutputBufferAvailable};
        ret = encoderDemo->NativeSetCallback(handle, cb);
        ASSERT_EQ(AV_ERR_OK, ret);
        gettimeofday(&start, NULL);
        ret = encoderDemo->NativeConfigure(handle, format);
        gettimeofday(&end, NULL);
        totalTime += (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
        ASSERT_EQ(AV_ERR_OK, ret);

        encoderDemo->NativeDestroy(handle);
    }
    cout << "2000 times finish, run time is " << totalTime << endl;

    OH_AVFormat_Destroy(format);
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_STABILITY_006
 * @tc.name      : OH_AudioEncoder_Prepare 2000 times
 * @tc.desc      : stability
 */
HWTEST_F(NativeStablityTest, SUB_MULTIMEDIA_AUDIO_ENCODER_STABILITY_006, TestSize.Level2)
{
    OH_AVErrCode ret;
    AudioEncoderDemo* encoderDemo = new AudioEncoderDemo();
    double totalTime = 0;
    struct timeval start, end;

    OH_AVFormat* format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 2);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 16000);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, OH_BitsPerSample::SAMPLE_F32LE);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, OH_BitsPerSample::SAMPLE_F32LE);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_CHANNEL_LAYOUT, STEREO);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AAC_IS_ADTS, DEFAULT_AAC_TYPE);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 169000);

    for (int i = 0; i < RUN_TIMES; i++)
    {
        OH_AVCodec* handle = encoderDemo->NativeCreateByName("OH.Media.Codec.Encoder.Audio.AAC");
        ASSERT_NE(nullptr, handle);
        struct OH_AVCodecAsyncCallback cb = {&OnError, &OnOutputFormatChanged, &OnInputBufferAvailable,
            &OnOutputBufferAvailable};
        ret = encoderDemo->NativeSetCallback(handle, cb);
        ASSERT_EQ(AV_ERR_OK, ret);
        ret = encoderDemo->NativeConfigure(handle, format);
        ASSERT_EQ(AV_ERR_OK, ret);
        gettimeofday(&start, NULL);
        ret = encoderDemo->NativePrepare(handle);
        gettimeofday(&end, NULL);
        totalTime += (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
        ASSERT_EQ(AV_ERR_OK, ret);
        encoderDemo->NativeDestroy(handle);
    }
    cout << "2000 times finish, run time is " << totalTime << endl;

    OH_AVFormat_Destroy(format);
    delete encoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_STABILITY_007
 * @tc.name      : OH_AudioEncoder_Start 2000 times
 * @tc.desc      : stability
 */
HWTEST_F(NativeStablityTest, SUB_MULTIMEDIA_AUDIO_ENCODER_STABILITY_007, TestSize.Level2)
{
    OH_AVErrCode ret;
    AudioEncoderDemo* encoderDemo = new AudioEncoderDemo();
    double totalTime = 0;
    struct timeval start, end;

    OH_AVFormat* format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 2);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 16000);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, OH_BitsPerSample::SAMPLE_F32LE);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, OH_BitsPerSample::SAMPLE_F32LE);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_CHANNEL_LAYOUT, STEREO);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AAC_IS_ADTS, DEFAULT_AAC_TYPE);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 169000);

    for (int i = 0; i < RUN_TIMES; i++)
    {
        OH_AVCodec* handle = encoderDemo->NativeCreateByName("OH.Media.Codec.Encoder.Audio.AAC");
        ASSERT_NE(nullptr, handle);
        struct OH_AVCodecAsyncCallback cb = {&OnError, &OnOutputFormatChanged, &OnInputBufferAvailable,
            &OnOutputBufferAvailable};
        ret = encoderDemo->NativeSetCallback(handle, cb);
        ASSERT_EQ(AV_ERR_OK, ret);
        ret = encoderDemo->NativeConfigure(handle, format);
        ASSERT_EQ(AV_ERR_OK, ret);
        ret = encoderDemo->NativePrepare(handle);
        ASSERT_EQ(AV_ERR_OK, ret);
        gettimeofday(&start, NULL);
        ret = OH_AudioEncoder_Start(handle);
        gettimeofday(&end, NULL);
        totalTime += (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
        ASSERT_EQ(AV_ERR_OK, ret);
        encoderDemo->NativeDestroy(handle);
    }
    cout << "2000 times finish, run time is " << totalTime << endl;

    OH_AVFormat_Destroy(format);
    delete encoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_STABILITY_008
 * @tc.name      : OH_AudioEncoder_Stop 2000 times
 * @tc.desc      : stability
 */
HWTEST_F(NativeStablityTest, SUB_MULTIMEDIA_AUDIO_ENCODER_STABILITY_008, TestSize.Level2)
{
    OH_AVErrCode ret;
    AudioEncoderDemo* encoderDemo = new AudioEncoderDemo();
    double totalTime = 0;
    struct timeval start, end;

    OH_AVFormat* format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 2);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 16000);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, OH_BitsPerSample::SAMPLE_F32LE);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, OH_BitsPerSample::SAMPLE_F32LE);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_CHANNEL_LAYOUT, STEREO);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AAC_IS_ADTS, DEFAULT_AAC_TYPE);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 169000);

    for (int i = 0; i < RUN_TIMES; i++)
    {
        OH_AVCodec* handle = encoderDemo->NativeCreateByName("OH.Media.Codec.Encoder.Audio.AAC");
        ASSERT_NE(nullptr, handle);
        struct OH_AVCodecAsyncCallback cb = {&OnError, &OnOutputFormatChanged, &OnInputBufferAvailable,
            &OnOutputBufferAvailable};
        ret = encoderDemo->NativeSetCallback(handle, cb);
        ASSERT_EQ(AV_ERR_OK, ret);
        ret = encoderDemo->NativeConfigure(handle, format);
        ASSERT_EQ(AV_ERR_OK, ret);
        ret = encoderDemo->NativePrepare(handle);
        ASSERT_EQ(AV_ERR_OK, ret);
        ret = OH_AudioEncoder_Start(handle);
        ASSERT_EQ(AV_ERR_OK, ret);
        gettimeofday(&start, NULL);
        ret = OH_AudioEncoder_Stop(handle);
        gettimeofday(&end, NULL);
        totalTime += (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
        ASSERT_EQ(AV_ERR_OK, ret);
        encoderDemo->NativeDestroy(handle);
    }
    cout << "2000 times finish, run time is " << totalTime << endl;

    OH_AVFormat_Destroy(format);
    delete encoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_STABILITY_009
 * @tc.name      : OH_AudioEncoder_Flush 2000 times
 * @tc.desc      : stability
 */
HWTEST_F(NativeStablityTest, SUB_MULTIMEDIA_AUDIO_ENCODER_STABILITY_009, TestSize.Level2)
{
    OH_AVErrCode ret;
    AudioEncoderDemo* encoderDemo = new AudioEncoderDemo();
    double totalTime = 0;
    struct timeval start, end;

    OH_AVFormat* format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 2);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 16000);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, OH_BitsPerSample::SAMPLE_F32LE);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, OH_BitsPerSample::SAMPLE_F32LE);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_CHANNEL_LAYOUT, STEREO);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AAC_IS_ADTS, DEFAULT_AAC_TYPE);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 169000);

    for (int i = 0; i < RUN_TIMES; i++)
    {
        OH_AVCodec* handle = encoderDemo->NativeCreateByName("OH.Media.Codec.Encoder.Audio.AAC");
        ASSERT_NE(nullptr, handle);
        struct OH_AVCodecAsyncCallback cb = {&OnError, &OnOutputFormatChanged, &OnInputBufferAvailable,
            &OnOutputBufferAvailable};
        ret = encoderDemo->NativeSetCallback(handle, cb);
        ASSERT_EQ(AV_ERR_OK, ret);
        ret = encoderDemo->NativeConfigure(handle, format);
        ASSERT_EQ(AV_ERR_OK, ret);
        ret = encoderDemo->NativePrepare(handle);
        ASSERT_EQ(AV_ERR_OK, ret);
        ret = OH_AudioEncoder_Start(handle);
        ASSERT_EQ(AV_ERR_OK, ret);
        gettimeofday(&start, NULL);
        ret = encoderDemo->NativeFlush(handle);
        gettimeofday(&end, NULL);
        totalTime += (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
        ASSERT_EQ(AV_ERR_OK, ret);
        encoderDemo->NativeDestroy(handle);
    }
    cout << "2000 times finish, run time is " << totalTime << endl;

    OH_AVFormat_Destroy(format);
    delete encoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_STABILITY_010
 * @tc.name      : OH_AudioEncoder_Reset 2000 times
 * @tc.desc      : stability
 */
HWTEST_F(NativeStablityTest, SUB_MULTIMEDIA_AUDIO_ENCODER_STABILITY_010, TestSize.Level2)
{
    AudioEncoderDemo* encoderDemo = new AudioEncoderDemo();
    double totalTime = 0;
    struct timeval start, end;

    OH_AVCodec* handle = encoderDemo->NativeCreateByName("OH.Media.Codec.Encoder.Audio.AAC");
    ASSERT_NE(nullptr, handle);

    for (int i = 0; i < RUN_TIMES; i++)
    {
        gettimeofday(&start, NULL);
        OH_AudioEncoder_Reset(handle);
        gettimeofday(&end, NULL);
        totalTime += (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
    }
    cout << "2000 times finish, run time is " << totalTime << endl;
    delete encoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_STABILITY_011
 * @tc.name      : OH_AudioEncoder_GetOutputDescription 2000 times
 * @tc.desc      : stability
 */
HWTEST_F(NativeStablityTest, SUB_MULTIMEDIA_AUDIO_ENCODER_STABILITY_011, TestSize.Level2)
{
    AudioEncoderDemo* encoderDemo = new AudioEncoderDemo();

    string encoderName = "OH.Media.Codec.Encoder.Audio.AAC";

    string inputFile = "f32le_44100_2_dayuhaitang.pcm";
    string outputFile = "STABILITY_011.aac";

    OH_AVFormat* format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 2);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 44100);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, OH_BitsPerSample::SAMPLE_F32LE);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, OH_BitsPerSample::SAMPLE_F32LE);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_CHANNEL_LAYOUT, STEREO);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AAC_IS_ADTS, DEFAULT_AAC_TYPE);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 169000);
    ASSERT_NE(nullptr, format);

    encoderDemo->setTimerFlag(TIMER_GETOUTPUTDESCRIPTION);
    encoderDemo->NativeRunCase(inputFile, outputFile, encoderName.c_str(), format);

    OH_AVFormat_Destroy(format);
    delete encoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_STABILITY_012
 * @tc.name      : OH_AudioEncoder_SetParameter 2000 times
 * @tc.desc      : stability
 */
HWTEST_F(NativeStablityTest, SUB_MULTIMEDIA_AUDIO_ENCODER_STABILITY_012, TestSize.Level2)
{
    OH_AVErrCode ret;
    AudioEncoderDemo* encoderDemo = new AudioEncoderDemo();
    double totalTime = 0;
    struct timeval start, end;

    OH_AVFormat* format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 2);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 44100);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, OH_BitsPerSample::SAMPLE_F32LE);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, OH_BitsPerSample::SAMPLE_F32LE);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_CHANNEL_LAYOUT, STEREO);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AAC_IS_ADTS, DEFAULT_AAC_TYPE);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 169000);

    OH_AVCodec* handle = encoderDemo->NativeCreateByName("OH.Media.Codec.Encoder.Audio.AAC");
    ASSERT_NE(nullptr, handle);

    struct OH_AVCodecAsyncCallback cb = {&OnError, &OnOutputFormatChanged, &OnInputBufferAvailable,
        &OnOutputBufferAvailable};
    ret = encoderDemo->NativeSetCallback(handle, cb);
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = encoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = encoderDemo->NativePrepare(handle);
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = encoderDemo->NativeStart(handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    for (int i = 0; i < RUN_TIMES; i++)
    {
        gettimeofday(&start, NULL);
        ret = encoderDemo->NativeSetParameter(handle, format);
        gettimeofday(&end, NULL);
        totalTime += (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
        ASSERT_EQ(AV_ERR_OK, ret);
    }

    cout << "2000 times finish, run time is " << totalTime << endl;
    encoderDemo->NativeDestroy(handle);
    OH_AVFormat_Destroy(format);
    delete encoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_STABILITY_013
 * @tc.name      : OH_AudioEncoder_PushInputData 2000 times
 * @tc.desc      : stability
 */
HWTEST_F(NativeStablityTest, SUB_MULTIMEDIA_AUDIO_ENCODER_STABILITY_013, TestSize.Level2)
{
    AudioEncoderDemo* encoderDemo = new AudioEncoderDemo();

    string encoderName = "OH.Media.Codec.Encoder.Audio.AAC";

    string inputFile = "f32le_44100_2_dayuhaitang.pcm";
    string outputFile = "STABILITY_013.aac";

    OH_AVFormat* format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 2);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 44100);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, OH_BitsPerSample::SAMPLE_F32LE);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, OH_BitsPerSample::SAMPLE_F32LE);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_CHANNEL_LAYOUT, STEREO);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AAC_IS_ADTS, DEFAULT_AAC_TYPE);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 169000);
    ASSERT_NE(nullptr, format);

    encoderDemo->setTimerFlag(TIMER_INPUT);
    encoderDemo->NativeRunCase(inputFile, outputFile, encoderName.c_str(), format);

    OH_AVFormat_Destroy(format);
    delete encoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_STABILITY_014
 * @tc.name      : OH_AudioEncoder_FreeOutputData 2000 times
 * @tc.desc      : stability
 */
HWTEST_F(NativeStablityTest, SUB_MULTIMEDIA_AUDIO_ENCODER_STABILITY_014, TestSize.Level2)
{
    AudioEncoderDemo* encoderDemo = new AudioEncoderDemo();

    string encoderName = "OH.Media.Codec.Encoder.Audio.AAC";

    string inputFile = "f32le_44100_2_dayuhaitang.pcm";
    string outputFile = "STABILITY_014.aac";

    OH_AVFormat* format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 2);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 44100);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, OH_BitsPerSample::SAMPLE_F32LE);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, OH_BitsPerSample::SAMPLE_F32LE);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_CHANNEL_LAYOUT, STEREO);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AAC_IS_ADTS, DEFAULT_AAC_TYPE);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 169000);
    ASSERT_NE(nullptr, format);

    encoderDemo->setTimerFlag(TIMER_FREEOUTPUT);
    encoderDemo->NativeRunCase(inputFile, outputFile, encoderName.c_str(), format);

    OH_AVFormat_Destroy(format);
    delete encoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_STABILITY_015
 * @tc.name      : OH_AudioEncoder_IsValid 2000 times
 * @tc.desc      : stability
 */
HWTEST_F(NativeStablityTest, SUB_MULTIMEDIA_AUDIO_ENCODER_STABILITY_015, TestSize.Level2)
{
    OH_AVErrCode ret;
    AudioEncoderDemo* encoderDemo = new AudioEncoderDemo();
    double totalTime = 0;
    struct timeval start, end;
    bool isValid;

    OH_AVFormat* format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 2);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 44100);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, OH_BitsPerSample::SAMPLE_F32LE);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, OH_BitsPerSample::SAMPLE_F32LE);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_CHANNEL_LAYOUT, STEREO);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AAC_IS_ADTS, DEFAULT_AAC_TYPE);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 169000);

    OH_AVCodec* handle = encoderDemo->NativeCreateByName("OH.Media.Codec.Encoder.Audio.AAC");
    ASSERT_NE(nullptr, handle);

    struct OH_AVCodecAsyncCallback cb = {&OnError, &OnOutputFormatChanged, &OnInputBufferAvailable,
        &OnOutputBufferAvailable};
    ret = encoderDemo->NativeSetCallback(handle, cb);
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = encoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = encoderDemo->NativePrepare(handle);
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = encoderDemo->NativeStart(handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    for (int i = 0; i < RUN_TIMES; i++)
    {
        gettimeofday(&start, NULL);
        ret = encoderDemo->NativeIsValid(handle, &isValid);
        gettimeofday(&end, NULL);
        totalTime += (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
        cout << "IsValid ret is " << ret << endl;
    }

    cout << "2000 times finish, run time is " << totalTime << endl;
    encoderDemo->NativeDestroy(handle);
    OH_AVFormat_Destroy(format);
    delete encoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_STABILITY_016
 * @tc.name      : encoder(long time)
 * @tc.desc      : stability
 */
HWTEST_F(NativeStablityTest, SUB_MULTIMEDIA_AUDIO_ENCODER_STABILITY_016, TestSize.Level2)
{
    string encoderList[] = { "OH.Media.Codec.Encoder.Audio.AAC", "OH.Media.Codec.Encoder.Audio.Flac"};
    string encoderName;
    string inputFile;
    string outputFile;

    time_t startTime = time(nullptr);
    ASSERT_NE(startTime, -1);
    time_t curTime = startTime;

    while (difftime(curTime, startTime) < RUN_TIME)
    {
        for (int i = 0; i < 2; i++)
        {
            encoderName = encoderList[i];
            if (encoderName == "OH.Media.Codec.Encoder.Audio.AAC")
            {
                inputFile = "f32le_44100_2_dayuhaitang.pcm";
                outputFile = "STABILITY_016.aac";
            }
            else
            {
                inputFile = "s16_48000_2_dayuhaitang.pcm";
                outputFile = "STABILITY_016.flac";
            }

            cout << "cur decoder name is " << encoderName << ", input file is " << inputFile << ", output file is " <<
                outputFile << endl;
            RunEncode(encoderName, inputFile, outputFile, i);
        }
        curTime = time(nullptr);
        ASSERT_NE(curTime, -1);
    }
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_STABILITY_017
 * @tc.name      : Flush(long time)
 * @tc.desc      : stability
 */
HWTEST_F(NativeStablityTest, SUB_MULTIMEDIA_AUDIO_ENCODER_STABILITY_017, TestSize.Level2)
{
    AudioEncoderDemo* encoderDemo = new AudioEncoderDemo();
    string encoderName = "OH.Media.Codec.Encoder.Audio.AAC";
    string inputFile = "f32le_44100_2_dayuhaitang.pcm";
    string outputFile = "STABILITY_017.aac";
    bool needConfigure = true;

    time_t startTime = time(nullptr);
    ASSERT_NE(startTime, -1);
    time_t curTime = startTime;

    OH_AVCodec* handle = encoderDemo->NativeCreateByName(encoderName.c_str());
    OH_AVFormat* format = GetAVFormatByEncoder(encoderName);
    struct OH_AVCodecAsyncCallback cb = {&OnError, &OnOutputFormatChanged, &OnInputBufferAvailable,
        &OnOutputBufferAvailable};
    encoderDemo->NativeSetCallback(handle, cb);

    while (difftime(curTime, startTime) < RUN_TIME)
    {
        encoderDemo->NativeRunCaseWithoutCreate(handle, inputFile, outputFile, format, encoderName.c_str(),
            needConfigure);
        needConfigure = false;
        encoderDemo->NativeFlush(handle);
        curTime = time(nullptr);
        ASSERT_NE(curTime, -1);
    }

    OH_AVFormat_Destroy(format);
    encoderDemo->NativeDestroy(handle);
    delete encoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_STABILITY_018
 * @tc.name      : Reset(long time)
 * @tc.desc      : stability
 */
HWTEST_F(NativeStablityTest, SUB_MULTIMEDIA_AUDIO_ENCODER_STABILITY_018, TestSize.Level2)
{
    AudioEncoderDemo* encoderDemo = new AudioEncoderDemo();
    string encoderName = "OH.Media.Codec.Encoder.Audio.AAC";
    string inputFile = "f32le_44100_2_dayuhaitang.pcm";
    string outputFile = "STABILITY_018.aac";
    bool needConfigure = true;

    time_t startTime = time(nullptr);
    ASSERT_NE(startTime, -1);
    time_t curTime = startTime;

    OH_AVCodec* handle = encoderDemo->NativeCreateByName(encoderName.c_str());
    OH_AVFormat* format = GetAVFormatByEncoder(encoderName);
    struct OH_AVCodecAsyncCallback cb = {&OnError, &OnOutputFormatChanged, &OnInputBufferAvailable,
        &OnOutputBufferAvailable};
    encoderDemo->NativeSetCallback(handle, cb);

    while (difftime(curTime, startTime) < RUN_TIME)
    {
        encoderDemo->NativeRunCaseWithoutCreate(handle, inputFile, outputFile, format, encoderName.c_str(),
            needConfigure);
        encoderDemo->NativeReset(handle);
        curTime = time(nullptr);
        ASSERT_NE(curTime, -1);
    }

    OH_AVFormat_Destroy(format);
    encoderDemo->NativeDestroy(handle);
    delete encoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_STABILITY_019
 * @tc.name      : thread decoder(long time)
 * @tc.desc      : stability
 */
HWTEST_F(NativeStablityTest, SUB_MULTIMEDIA_AUDIO_ENCODER_STABILITY_019, TestSize.Level2)
{
    string encoderList[] = { "OH.Media.Codec.Encoder.Audio.AAC", "OH.Media.Codec.Encoder.Audio.Flac" };
    string encoderName;
    string inputFile;
    string outputFile;
    vector<thread> threadVec;

    time_t startTime = time(nullptr);
    ASSERT_NE(startTime, -1);
    time_t curTime = startTime;

    while (difftime(curTime, startTime) < RUN_TIME)
    {
        threadVec.clear();
        for (int32_t i = 0; i < 16; i++)
        {
            encoderName = encoderList[i % 2];
            if (encoderName == "OH.Media.Codec.Encoder.Audio.AAC")
            {
                inputFile = "f32le_44100_2_dayuhaitang.pcm";
                outputFile = "STABILITY_019_" + to_string(i) + ".aac";
            }
            else
            {
                inputFile = "s16_48000_2_dayuhaitang.pcm";
                outputFile = "STABILITY_019_" + to_string(i) + ".flac";
            }
            cout << "cur decoder name is " << encoderName << ", input file is " << inputFile << ", output file is " <<
                outputFile << endl;
            threadVec.push_back(thread(RunEncode, encoderName, inputFile, outputFile, i));
        }
        for (uint32_t i = 0; i < threadVec.size(); i++)
        {
            threadVec[i].join();
        }
        for (int32_t i = 0; i < 16; i++)
        {
            ASSERT_EQ(AV_ERR_OK, g_testResult[i]);
        }
        curTime = time(nullptr);
        ASSERT_NE(curTime, -1);
    }
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_STABILITY_020
 * @tc.name      : thread encoder Flush(long time)
 * @tc.desc      : stability
 */
HWTEST_F(NativeStablityTest, SUB_MULTIMEDIA_AUDIO_ENCODER_STABILITY_020, TestSize.Level2)
{
    string encoderList[] = { "OH.Media.Codec.Encoder.Audio.AAC", "OH.Media.Codec.Encoder.Audio.Flac" };
    string encoderName;
    string inputFile;
    string outputFile;
    vector<thread> threadVec;

    for (int32_t i = 0; i < 16; i++)
    {
        encoderName = encoderList[i % 2];
        if (encoderName == "OH.Media.Codec.Encoder.Audio.AAC")
        {
            inputFile = "f32le_44100_2_dayuhaitang.pcm";
            outputFile = "STABILITY_019_" + to_string(i) + ".aac";
        }
        else
        {
            inputFile = "s16_48000_2_dayuhaitang.pcm";
            outputFile = "STABILITY_019_" + to_string(i) + ".flac";
        }
        cout << "cur encoder name is " << encoderName << ", input file is " << inputFile << ", output file is " <<
            outputFile << endl;
        threadVec.push_back(thread(RunLongTimeFlush, encoderName, inputFile, outputFile, i));
    }
    for (uint32_t i = 0; i < threadVec.size(); i++)
    {
        threadVec[i].join();
    }
    for (int32_t i = 0; i < 16; i++)
    {
        ASSERT_EQ(AV_ERR_OK, g_testResult[i]);
    }
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_STABILITY_021
 * @tc.name      : thread encoder Reset(long time)
 * @tc.desc      : stability
 */
HWTEST_F(NativeStablityTest, SUB_MULTIMEDIA_AUDIO_ENCODER_STABILITY_021, TestSize.Level2)
{
    string encoderList[] = { "OH.Media.Codec.Encoder.Audio.AAC", "OH.Media.Codec.Encoder.Audio.Flac" };
    string encoderName;
    string inputFile;
    string outputFile;
    vector<thread> threadVec;

    for (int32_t i = 0; i < 16; i++)
    {
        encoderName = encoderList[i % 2];
        if (encoderName == "OH.Media.Codec.Encoder.Audio.AAC")
        {
            inputFile = "f32le_44100_2_dayuhaitang.pcm";
            outputFile = "STABILITY_019_" + to_string(i) + ".aac";
        }
        else
        {
            inputFile = "s16_48000_2_dayuhaitang.pcm";
            outputFile = "STABILITY_019_" + to_string(i) + ".flac";
        }
        cout << "cur encoder name is " << encoderName << ", input file is " << inputFile << ", output file is " <<
            outputFile << endl;
        threadVec.push_back(thread(RunLongTimeReset, encoderName, inputFile, outputFile, i));
    }
    for (uint32_t i = 0; i < threadVec.size(); i++)
    {
        threadVec[i].join();
    }
    for (int32_t i = 0; i < 16; i++)
    {
        ASSERT_EQ(AV_ERR_OK, g_testResult[i]);
    }
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_STABILITY_022
 * @tc.name      : thread encoder Reset(long time)
 * @tc.desc      : stability
 */
HWTEST_F(NativeStablityTest, SUB_MULTIMEDIA_AUDIO_ENCODER_STABILITY_022, TestSize.Level2)
{
    string encoderList[] = { "OH.Media.Codec.Encoder.Audio.AAC", "OH.Media.Codec.Encoder.Audio.Flac" };
    string encoderName;
    string inputFile;
    string outputFile;
    vector<thread> threadVec;

    for (int32_t i = 0; i < 16; i++)
    {
        encoderName = encoderList[i % 2];
        if (encoderName == "OH.Media.Codec.Encoder.Audio.AAC")
        {
            inputFile = "f32le_44100_2_dayuhaitang.pcm";
            outputFile = "STABILITY_019_" + to_string(i) + ".aac";
        }
        else
        {
            inputFile = "s16_48000_2_dayuhaitang.pcm";
            outputFile = "STABILITY_019_" + to_string(i) + ".flac";
        }
        cout << "cur encoder name is " << encoderName << ", input file is " << inputFile << ", output file is " <<
            outputFile << endl;
        threadVec.push_back(thread(RunLongTimeStop, encoderName, inputFile, outputFile, i));
    }
    for (uint32_t i = 0; i < threadVec.size(); i++)
    {
        threadVec[i].join();
    }
    for (int32_t i = 0; i < 16; i++)
    {
        ASSERT_EQ(AV_ERR_OK, g_testResult[i]);
    }
}