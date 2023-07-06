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
#include "avcodec_info.h"
#include "avcodec_errors.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;

namespace {
    class InnerFuzzTest : public testing::Test {
    public:
        static void SetUpTestCase();
        static void TearDownTestCase();
        void SetUp() override;
        void TearDown() override;
    };

    void InnerFuzzTest::SetUpTestCase() {}
    void InnerFuzzTest::TearDownTestCase() {}
    void InnerFuzzTest::SetUp() {}
    void InnerFuzzTest::TearDown() {}

    constexpr int FUZZ_TEST_NUM = 1000000;
    constexpr int DEFAULT_INFO_SIZE = 100;
    constexpr int DEFAULT_RAND_SIZE = 256;
    std::atomic<bool> runningFlag = true;

    string getRandStr(const int len)
    {
        string str;
        char c;
        int idx;
        for (idx = 0; idx < len; idx++) {
            c = rand() % DEFAULT_RAND_SIZE;
            str.push_back(c);
        }
        return str;
    }


    int32_t getIntRand()
    {
        int32_t data = -10000 + rand() % 20001;
        return data;
    }

    void InputFunc(AudioDecoderDemo* decoderDemo)
    {
        AVCodecBufferInfo info;
        AVCodecBufferFlag flag;
        info.size = DEFAULT_INFO_SIZE;
        info.offset = 0;
        info.presentationTimeUs = 0;
        flag = AVCODEC_BUFFER_FLAG_NONE;

        for (int i = 0; i < FUZZ_TEST_NUM; i++) {
            cout << "current run time is " << i << endl;
            std::shared_ptr<ADecSignal> signal_ = decoderDemo->getSignal();
            int index = signal_->inQueue_.front();
            std::shared_ptr<AVSharedMemory> buffer = signal_->inInnerBufQueue_.front();

            uint8_t* inputData = (uint8_t*)malloc(info.size);
            if (inputData == nullptr) {
                break ;
            }
            (void)memcpy_s(buffer->GetBase(), info.size, inputData, info.size);
            cout << "index is: " << index << endl;

            int32_t ret = decoderDemo->InnerQueueInputBuffer(index, info, flag);
            ASSERT_EQ(AVCS_ERR_OK, ret);
            cout << "InnerQueueInputBuffer return: " << ret << endl;
        }
        runningFlag.store(false);
    }

    void OutputFunc(AudioDecoderDemo* decoderDemo)
    {
        while (runningFlag.load()) {
            std::shared_ptr<ADecSignal> signal_ = decoderDemo->getSignal();
            int index = signal_->outQueue_.front();
            int32_t ret = decoderDemo->InnerReleaseOutputBuffer(index);
            cout << "ReleaseOutputData return: " << ret << endl;
        }
    }
}
/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUZZ_001
 * @tc.name      : InnerCreateByMime
 * @tc.desc      : Fuzz test
 */
HWTEST_F(InnerFuzzTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUZZ_001, TestSize.Level2)
{
    srand(time(nullptr) * 10);
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    ASSERT_NE(nullptr, decoderDemo);

    for (int i = 0; i < FUZZ_TEST_NUM; i++) {
        cout << "current run time is: " << i << endl;
        int32_t strLen = rand() % 1000;
        string randStr = getRandStr(strLen);
        decoderDemo->InnerCreateByMime(randStr.c_str());
        decoderDemo->InnerDestroy();
    }

    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUZZ_002
 * @tc.name      : InnerCreateByName
 * @tc.desc      : Fuzz test
 */
HWTEST_F(InnerFuzzTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUZZ_002, TestSize.Level2)
{
    srand(time(nullptr) * 10);
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    ASSERT_NE(nullptr, decoderDemo);

    int strLen = 0;
    string test_value = "";

    for (int i = 0; i < FUZZ_TEST_NUM; i++) {
        cout << "current run time is: " << i << endl;
        strLen = rand() % 65536;
        test_value = getRandStr(strLen);
        decoderDemo->InnerCreateByName(test_value.c_str());
        decoderDemo->InnerDestroy();
    }
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUZZ_003
 * @tc.name      : InnerConfigure
 * @tc.desc      : Fuzz test
 */
HWTEST_F(InnerFuzzTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUZZ_003, TestSize.Level2)
{
    srand(time(nullptr) * 10);
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    ASSERT_NE(nullptr, decoderDemo);

    int32_t ret = decoderDemo->InnerCreateByName("OH.Media.Codec.Decoder.Audio.Mpeg");
    ASSERT_EQ(AVCS_ERR_OK, ret);
    Format format;

    for (int i = 0; i < FUZZ_TEST_NUM; i++) {
        cout << "current run time is: " << i << endl;
        int32_t bitRate = getIntRand();
        int32_t audioChannels = getIntRand();
        int32_t audioSampleRate = getIntRand();
        int32_t codedSample = getIntRand();

        cout << "BIT_RATE is: " << bitRate << endl;
        cout << ", AUDIO_CHANNELS len is: " << audioChannels << endl;
        cout << ", SAMPLE_RATE len is: " << audioSampleRate << endl;

        format.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, audioChannels);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, audioSampleRate);
        format.PutIntValue("bits_per_coded_sample", codedSample);
        format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, bitRate);

        decoderDemo->InnerConfigure(format);
        decoderDemo->InnerReset();
    }
    decoderDemo->InnerDestroy();
    delete decoderDemo;
}


/**
* @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUZZ_004
* @tc.name      : InnerSetParameter
* @tc.desc      : Fuzz test
*/
HWTEST_F(InnerFuzzTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUZZ_004, TestSize.Level2)
{
    srand(time(nullptr) * 10);
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();

    int32_t ret = decoderDemo->InnerCreateByName("OH.Media.Codec.Decoder.Audio.Mpeg");
    ASSERT_EQ(AVCS_ERR_OK, ret);

    Format format;

    std::shared_ptr<ADecSignal> signal = decoderDemo->getSignal();

    std::shared_ptr<InnerADecDemoCallback> cb_ = make_unique<InnerADecDemoCallback>(signal);
    ret = decoderDemo->InnerSetCallback(cb_);
    cout << "SetCallback ret is: " << ret << endl;

    format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 320000);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 48000);
    format.PutIntValue("bits_per_coded_sample", 4);

    ret = decoderDemo->InnerConfigure(format);
    cout << "Configure return: " << ret << endl;
    ret = decoderDemo->InnerPrepare();
    cout << "Prepare return: " << ret << endl;

    ret = decoderDemo->InnerStart();
    cout << "Start return: " << ret << endl;

    for (int i = 0; i < FUZZ_TEST_NUM; i++) {
        Format generalFormat;
        cout << "current run time is: " << i << endl;
        int32_t bitRate = getIntRand();
        int32_t audioChannels = getIntRand();
        int32_t audioSampleRate = getIntRand();
        int32_t codedSample = getIntRand();

        cout << "BIT_RATE is: " << bitRate << endl;
        cout << ", AUDIO_CHANNELS len is: " << audioChannels << endl;
        cout << ", SAMPLE_RATE len is: " << audioSampleRate << endl;

        generalFormat.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, audioChannels);
        generalFormat.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, audioSampleRate);
        generalFormat.PutIntValue("bits_per_coded_sample", codedSample);
        generalFormat.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, bitRate);
        ret = decoderDemo->InnerSetParameter(generalFormat);
        cout << "ret code is: " << ret << endl;
    }
    decoderDemo->InnerDestroy();
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUZZ_005
 * @tc.name      : InnerQueueInputBuffer
 * @tc.desc      : Fuzz test
 */
HWTEST_F(InnerFuzzTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUZZ_005, TestSize.Level2)
{
    srand(time(nullptr) * 10);
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();

    int32_t ret = decoderDemo->InnerCreateByName("OH.Media.Codec.Decoder.Audio.Mpeg");
    ASSERT_EQ(AVCS_ERR_OK, ret);
    Format format;
    format.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 44100);
    format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 169000);
    format.PutIntValue("bits_per_coded_sample", 4);

    int index = 0;

    std::shared_ptr<ADecSignal> signal = decoderDemo->getSignal();

    std::shared_ptr<InnerADecDemoCallback> cb_ = make_unique<InnerADecDemoCallback>(signal);
    ret = decoderDemo->InnerSetCallback(cb_);
    cout << "SetCallback ret is: " << ret << endl;

    ret = decoderDemo->InnerConfigure(format);
    cout << "Configure ret is: " << ret << endl;
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = decoderDemo->InnerPrepare();
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = decoderDemo->InnerStart();
    ASSERT_EQ(AVCS_ERR_OK, ret);

    AVCodecBufferInfo info;
    AVCodecBufferFlag flag;
    info.presentationTimeUs = 0;

    for (int i = 0; i < FUZZ_TEST_NUM; i++) {
        cout << "current run time is: " << i << endl;
        index = getIntRand();
        int dataLen = rand() % 65536;

        info.presentationTimeUs += 21;
        info.size = dataLen;
        info.offset = getIntRand();
        flag = AVCODEC_BUFFER_FLAG_NONE;

        cout << "info.presentationTimeUs is:" << info.presentationTimeUs << endl;
        cout << "info.size is:" << info.size << endl;
        cout << "info.offset is:" << info.offset << endl;
        cout << "flag is:" << flag << endl;

        ret = decoderDemo->InnerQueueInputBuffer(index, info, flag);
        cout << "ret code is: " << ret << endl;
    }
    decoderDemo->InnerDestroy();
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUZZ_006
 * @tc.name      : InnerGetOutputFormat
 * @tc.desc      : Fuzz test
 */
HWTEST_F(InnerFuzzTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUZZ_006, TestSize.Level2)
{
    srand(time(nullptr) * 10);
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();

    int32_t ret = decoderDemo->InnerCreateByName("OH.Media.Codec.Decoder.Audio.Mpeg");
    ASSERT_EQ(AVCS_ERR_OK, ret);
    Format format;
    format.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 44100);
    format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 169000);
    format.PutIntValue("bits_per_coded_sample", 4);

    std::shared_ptr<ADecSignal> signal = decoderDemo->getSignal();

    std::shared_ptr<InnerADecDemoCallback> cb_ = make_unique<InnerADecDemoCallback>(signal);
    ret = decoderDemo->InnerSetCallback(cb_);
    cout << "SetCallback ret is: " << ret << endl;

    ret = decoderDemo->InnerConfigure(format);
    cout << "Configure ret is: " << ret << endl;
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = decoderDemo->InnerPrepare();
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = decoderDemo->InnerStart();
    ASSERT_EQ(AVCS_ERR_OK, ret);

    int strLen = 0;
    string test_key = "";
    string test_value = "";

    for (int i = 0; i < FUZZ_TEST_NUM; i++) {
        cout << "current run time is: " << i << endl;
        strLen = rand() % 65536;
        test_key = getRandStr(strLen);
        cout << "test key len is:" << strLen << endl;
        strLen = rand() % 65536;
        test_value = getRandStr(strLen);
        cout << "test value len is:" << strLen << endl;
        format.PutStringValue(test_key.c_str(), test_value.c_str());
        ret = decoderDemo->InnerGetOutputFormat(format);
        cout << "ret code is: " << ret << endl;
    }
    decoderDemo->InnerDestroy();
    delete decoderDemo;
}

/**
* @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUZZ_007
* @tc.name      : input file fuzz
* @tc.desc      : Fuzz test
*/
HWTEST_F(InnerFuzzTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUZZ_007, TestSize.Level2)
{
    srand(time(nullptr) * 10);
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();

    int32_t ret = decoderDemo->InnerCreateByName("OH.Media.Codec.Decoder.Audio.Mpeg");
    ASSERT_EQ(AVCS_ERR_OK, ret);
    Format format;
    format.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 44100);
    format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 169000);
    format.PutIntValue("bits_per_coded_sample", 4);

    std::shared_ptr<ADecSignal> signal = decoderDemo->getSignal();

    std::shared_ptr<InnerADecDemoCallback> cb_ = make_unique<InnerADecDemoCallback>(signal);
    ret = decoderDemo->InnerSetCallback(cb_);
    cout << "SetCallback ret is: " << ret << endl;

    ret = decoderDemo->InnerConfigure(format);
    cout << "Configure ret is: " << ret << endl;
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = decoderDemo->InnerPrepare();
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = decoderDemo->InnerStart();
    ASSERT_EQ(AVCS_ERR_OK, ret);

    vector<thread> threadVec;
    threadVec.push_back(thread(InputFunc, decoderDemo));
    threadVec.push_back(thread(OutputFunc, decoderDemo));
    for (uint32_t i = 0; i < threadVec.size(); i++)
    {
        threadVec[i].join();
    }
    decoderDemo->InnerDestroy();
    delete decoderDemo;
}
