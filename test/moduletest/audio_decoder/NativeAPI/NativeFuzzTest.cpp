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
#include <iostream>
#include <ctime>
#include <thread>
#include <vector>
#include "gtest/gtest.h"
#include "AudioDecoderDemoCommon.h"


using namespace std;
using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;
constexpr uint32_t SIZE_INFO = 100;

namespace {
    class NativeFuzzTest : public testing::Test {
    public:
        static void SetUpTestCase();
        static void TearDownTestCase();
        void SetUp() override;
        void TearDown() override;
    };

    void NativeFuzzTest::SetUpTestCase() {}
    void NativeFuzzTest::TearDownTestCase() {}
    void NativeFuzzTest::SetUp() {}
    void NativeFuzzTest::TearDown() {}

    constexpr uint32_t FUZZ_TEST_NUM = 1000;
    constexpr uint32_t CONSTASNTS = 256;
    std::atomic<bool> runningFlag = true;

    string rand_str(const int len)
    {
        string str;
        char c;
        int idx;
        for (idx = 0; idx < len; idx++) {
            c = rand() % CONSTASNTS;
            str.push_back(c);
        }
        return str;
    }

    int32_t getIntRand()
    {
        int32_t data = -10000 + rand() % 20001;
        return data;
    }

    void inputFunc(AudioDecoderDemo* decoderDemo, OH_AVCodec* handle)
    {
        OH_AVCodecBufferAttr info;
        info.size = SIZE_INFO;
        info.offset = 0;
        info.pts = 0;
        info.flags = AVCODEC_BUFFER_FLAGS_NONE;

        for (int i = 0; i < FUZZ_TEST_NUM; i++) {
            cout << "current run time is " << i << endl;
            int32_t inputIndex = decoderDemo->NativeGetInputIndex();
            uint8_t* data = decoderDemo->NativeGetInputBuf();

            uint8_t* inputData = (uint8_t*)malloc(info.size);
            if (inputData == nullptr) {
                cout << "malloc failed" << endl;
                return;
            }
            (void)memcpy_s(data, info.size, inputData, info.size);
            cout << "index is: " << inputIndex << endl;

            OH_AVErrCode ret = decoderDemo->NativePushInputData(handle, inputIndex, info);
            cout << "PushInputData return: " << ret << endl;
            free(inputData);
        }
        runningFlag.store(false);
    }

    void outputFunc(AudioDecoderDemo* decoderDemo, OH_AVCodec* handle)
    {
        while (runningFlag.load()) {
            int32_t outputIndex = decoderDemo->NativeGetOutputIndex();
            OH_AVErrCode ret = decoderDemo->NativeFreeOutputData(handle, outputIndex);
            cout << "FreeOutputData return: " << ret << endl;
        }
    }
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUZZ_001
 * @tc.name      : OH_AudioDecoder_CreateByMime
 * @tc.desc      : Fuzz test
 */
HWTEST_F(NativeFuzzTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUZZ_001, TestSize.Level2)
{
    srand(time(nullptr) * 10);
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    ASSERT_NE(nullptr, decoderDemo);

    OH_AVCodec* handle = nullptr;

    for (int i = 0; i < FUZZ_TEST_NUM; i++) {
        cout << "current run time is: " << i << endl;
        int32_t strLen = rand() % 1000;
        string randStr = rand_str(strLen);
        handle = decoderDemo->NativeCreateByMime(randStr.c_str());
        if (handle != nullptr) {
            decoderDemo->NativeDestroy(handle);
            handle = nullptr;
        }
    }
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUZZ_002
 * @tc.name      : OH_AudioDecoder_CreateByName
 * @tc.desc      : Fuzz test
 */
HWTEST_F(NativeFuzzTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUZZ_002, TestSize.Level2)
{
    srand(time(nullptr) * 10);
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    ASSERT_NE(nullptr, decoderDemo);

    OH_AVCodec* handle = nullptr;

    for (int i = 0; i < FUZZ_TEST_NUM; i++) {
        cout << "current run time is: " << i << endl;
        int32_t strLen = rand() % 1000;
        string randStr = rand_str(strLen);
        handle = decoderDemo->NativeCreateByName(randStr.c_str());
        if (handle != nullptr) {
            decoderDemo->NativeDestroy(handle);
            handle = nullptr;
        }
    }
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUZZ_003
 * @tc.name      : OH_AudioDecoder_Configure
 * @tc.desc      : Fuzz test
 */
HWTEST_F(NativeFuzzTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUZZ_003, TestSize.Level2)
{
    srand(time(nullptr) * 10);
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    ASSERT_NE(nullptr, decoderDemo);

    OH_AVCodec* handle = decoderDemo->NativeCreateByName("OH.Media.Codec.Decoder.Audio.Mpeg");
    struct OH_AVCodecAsyncCallback cb = { &OnError, &OnOutputFormatChanged,
    &OnInputBufferAvailable, &OnOutputBufferAvailable };
    decoderDemo->NativeSetCallback(handle, cb);

    for (int i = 0; i < FUZZ_TEST_NUM; i++) {
        cout << "current run time is: " << i << endl;

        int32_t channel = getIntRand();
        int32_t sampleRate = getIntRand();
        int32_t codedSample = getIntRand();
        int32_t bitrate = getIntRand();

        OH_AVFormat* format = OH_AVFormat_Create();
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, channel);
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, sampleRate);
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, codedSample);
        OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, bitrate);

        cout << "OH_MD_KEY_AUD_CHANNEL_COUNT is: " << channel << endl;
        cout << "OH_MD_KEY_AUD_SAMPLE_RATE is : " << sampleRate << endl;
        cout << "bits_per_coded_sample is: " << codedSample << ", OH_MD_KEY_BITRATE is: " << bitrate << endl;

        OH_AVErrCode ret = decoderDemo->NativeConfigure(handle, format);
        cout << "Configure return: " << ret << endl;

        decoderDemo->NativeReset(handle);
        OH_AVFormat_Destroy(format);
    }
    decoderDemo->NativeDestroy(handle);
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUZZ_004
 * @tc.name      : OH_AudioDecoder_SetParameter
 * @tc.desc      : Fuzz test
 */
HWTEST_F(NativeFuzzTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUZZ_004, TestSize.Level2)
{
    srand(time(nullptr) * 10);
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    ASSERT_NE(nullptr, decoderDemo);

    OH_AVCodec* handle = decoderDemo->NativeCreateByName("OH.Media.Codec.Decoder.Audio.Mpeg");
    struct OH_AVCodecAsyncCallback cb = { &OnError, &OnOutputFormatChanged,
    &OnInputBufferAvailable, &OnOutputBufferAvailable };
    decoderDemo->NativeSetCallback(handle, cb);

    OH_AVFormat* format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 2);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 44100);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, 4);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 169000);

    OH_AVErrCode ret = decoderDemo->NativeConfigure(handle, format);
    cout << "Configure return: " << ret << endl;
    OH_AVFormat_Destroy(format);

    ret = decoderDemo->NativePrepare(handle);
    cout << "Prepare return: " << ret << endl;

    ret = decoderDemo->NativeStart(handle);
    cout << "Start return: " << ret << endl;

    for (int i = 0; i < FUZZ_TEST_NUM; i++) {
        cout << "current run time is: " << i << endl;

        int32_t channel = getIntRand();
        int32_t sampleRate = getIntRand();
        int32_t codedSample = getIntRand();
        int32_t bitrate = getIntRand();

        format = OH_AVFormat_Create();
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, channel);
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, sampleRate);
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, codedSample);
        OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, bitrate);

        cout << "OH_MD_KEY_AUD_CHANNEL_COUNT is: " << channel << endl;
        cout << "OH_MD_KEY_AUD_SAMPLE_RATE is: " << sampleRate << endl;
        cout << "bits_per_coded_sample is: " << codedSample << ", OH_MD_KEY_BITRATE is: " << bitrate << endl;

        ret = decoderDemo->NativeSetParameter(handle, format);
        cout << "SetParameter return: " << ret << endl;

        OH_AVFormat_Destroy(format);
    }
    decoderDemo->NativeDestroy(handle);
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUZZ_005
 * @tc.name      : OH_AudioDecoder_PushInputData
 * @tc.desc      : Fuzz test
 */
HWTEST_F(NativeFuzzTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUZZ_005, TestSize.Level2)
{
    srand(time(nullptr) * 10);
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    ASSERT_NE(nullptr, decoderDemo);

    OH_AVCodec* handle = decoderDemo->NativeCreateByName("OH.Media.Codec.Decoder.Audio.Mpeg");
    struct OH_AVCodecAsyncCallback cb = { &OnError, &OnOutputFormatChanged,
    &OnInputBufferAvailable, &OnOutputBufferAvailable };
    decoderDemo->NativeSetCallback(handle, cb);

    OH_AVFormat* format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 2);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 44100);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, 4);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 169000);

    OH_AVErrCode ret = decoderDemo->NativeConfigure(handle, format);
    cout << "Configure return: " << ret << endl;
    OH_AVFormat_Destroy(format);

    ret = decoderDemo->NativePrepare(handle);
    cout << "Prepare return: " << ret << endl;

    ret = decoderDemo->NativeStart(handle);
    cout << "Start return: " << ret << endl;

    for (int i = 0; i < FUZZ_TEST_NUM; i++) {
        int32_t index = getIntRand();

        OH_AVCodecBufferAttr info;
        info.size = getIntRand();
        info.offset = getIntRand();
        info.pts = getIntRand();
        info.flags = getIntRand();

        cout << "index is: " << index << ", info.size is: " << info.size << endl;
        cout << "info.offset is: " << info.offset << ", info.pts is: " << info.pts << endl;
        cout << "info.flags is: " << info.flags << endl;

        ret = decoderDemo->NativePushInputData(handle, index, info);
        cout << "PushInputData return: " << ret << endl;
    }
    decoderDemo->NativeDestroy(handle);
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUZZ_006
 * @tc.name      : OH_AudioDecoder_FreeOutputData
 * @tc.desc      : Fuzz test
 */
HWTEST_F(NativeFuzzTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUZZ_006, TestSize.Level2)
{
    srand(time(nullptr) * 10);
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    ASSERT_NE(nullptr, decoderDemo);

    OH_AVCodec* handle = decoderDemo->NativeCreateByName("OH.Media.Codec.Decoder.Audio.Mpeg");
    struct OH_AVCodecAsyncCallback cb = { &OnError, &OnOutputFormatChanged,
    &OnInputBufferAvailable, &OnOutputBufferAvailable };
    decoderDemo->NativeSetCallback(handle, cb);

    OH_AVFormat* format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 2);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 44100);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, 4);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 169000);

    OH_AVErrCode ret = decoderDemo->NativeConfigure(handle, format);
    cout << "Configure return: " << ret << endl;
    OH_AVFormat_Destroy(format);

    ret = decoderDemo->NativePrepare(handle);
    cout << "Prepare return: " << ret << endl;

    ret = decoderDemo->NativeStart(handle);
    cout << "Start return: " << ret << endl;

    OH_AVCodecBufferAttr info;
    info.size = 100;
    info.offset = 0;
    info.pts = 0;
    info.flags = AVCODEC_BUFFER_FLAGS_NONE;

    int32_t inputIndex = decoderDemo->NativeGetInputIndex();

    for (int i = 0; i < FUZZ_TEST_NUM; i++) {
        int32_t outputIndex = getIntRand();

        cout << "index is: " << outputIndex << endl;

        ret = decoderDemo->NativePushInputData(handle, inputIndex, info);
        cout << "PushInputData return: " << ret << endl;

        ret = decoderDemo->NativeFreeOutputData(handle, outputIndex);
        cout << "FreeOutputData return: " << ret << endl;
    }
    decoderDemo->NativeDestroy(handle);
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUZZ_007
 * @tc.name      : input file fuzz
 * @tc.desc      : Fuzz test
 */
HWTEST_F(NativeFuzzTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUZZ_007, TestSize.Level2)
{
    srand(time(nullptr) * 10);
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    ASSERT_NE(nullptr, decoderDemo);

    OH_AVCodec* handle = decoderDemo->NativeCreateByName("OH.Media.Codec.Decoder.Audio.Mpeg");
    struct OH_AVCodecAsyncCallback cb = { &OnError, &OnOutputFormatChanged,
    &OnInputBufferAvailable, &OnOutputBufferAvailable };
    decoderDemo->NativeSetCallback(handle, cb);

    OH_AVFormat* format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 2);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 44100);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, 4);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 169000);

    OH_AVErrCode ret = decoderDemo->NativeConfigure(handle, format);
    cout << "Configure return: " << ret << endl;
    OH_AVFormat_Destroy(format);

    ret = decoderDemo->NativePrepare(handle);
    cout << "Prepare return: " << ret << endl;

    ret = decoderDemo->NativeStart(handle);
    cout << "Start return: " << ret << endl;

    vector<thread> threadVec;
    threadVec.push_back(thread(inputFunc, decoderDemo, handle));
    threadVec.push_back(thread(outputFunc, decoderDemo, handle));
    for (uint32_t i = 0; i < threadVec.size(); i++)
    {
        threadVec[i].join();
    }

    decoderDemo->NativeDestroy(handle);
    delete decoderDemo;
}