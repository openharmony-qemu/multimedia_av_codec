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
#include "gtest/gtest.h"
#include "AudioEncoderDemoCommon.h"
#include "avcodec_info.h"
#include "avcodec_errors.h"
#include "avcodec_audio_common.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;
constexpr uint32_t SIZE_NUM = 100;

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

    constexpr uint32_t FUZZ_TEST_NUM = 1000000;
    constexpr uint32_t CONSTASNTS = 256;
    std::atomic<bool> runningFlag = true;
    string getRandStr(const int len)
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

    void inputFunc(AudioEncoderDemo* encoderDemo)
    {
        AVCodecBufferInfo info;
        AVCodecBufferFlag flag;
        info.size = SIZE_NUM;
        info.offset = 0;
        info.presentationTimeUs = 0;
        flag = AVCODEC_BUFFER_FLAG_NONE;

        for (int i = 0; i < FUZZ_TEST_NUM; i++) {
            cout << "current run time is " << i << endl;
            std::shared_ptr<AEncSignal> signal_ = encoderDemo->getSignal();
            int index = signal_->inQueue_.front();
            std::shared_ptr<AVSharedMemory> buffer = signal_->inInnerBufQueue_.front();

            uint8_t* inputData = (uint8_t*)malloc(info.size);
            if (inputData == nullptr) {
                cout << "malloc failed" << endl;
                return;
            }
            (void)memset_s(inputData, info.size, 0, info.size);
            memcpy_s(buffer->GetBase(), info.size, inputData, info.size);
            cout << "index is: " << index << endl;

            int32_t ret = encoderDemo->InnerQueueInputBuffer(index, info, flag);
            ASSERT_EQ(AVCS_ERR_OK, ret);
            cout << "InnerQueueInputBuffer return: " << ret << endl;
        }
        runningFlag.store(false);
    }

    void outputFunc(AudioEncoderDemo* encoderDemo)
    {
        while (runningFlag.load()) {
            std::shared_ptr<AEncSignal> signal_ = encoderDemo->getSignal();
            int index = signal_->outQueue_.front();
            int32_t ret = encoderDemo->InnerReleaseOutputBuffer(index);
            cout << "InnerReleaseOutputBuffer return: " << ret << endl;
        }
    }
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_FUZZ_001
 * @tc.name      : InnerCreateByMime
 * @tc.desc      : Fuzz test
 */
HWTEST_F(InnerFuzzTest, SUB_MULTIMEDIA_AUDIO_ENCODER_FUZZ_001, TestSize.Level2)
{
    srand(time(nullptr) * 10);
    AudioEncoderDemo* encoderDemo = new AudioEncoderDemo();
    ASSERT_NE(nullptr, encoderDemo);
    for (int i = 0; i < FUZZ_TEST_NUM; i++) {
        cout << "current run time is: " << i << endl;
        int32_t strLen = rand() % 1000;
        string randStr = getRandStr(strLen);
        encoderDemo->InnerCreateByMime(randStr.c_str());
        encoderDemo->InnerDestroy();
    }
    
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_FUZZ_002
 * @tc.name      : InnerCreateByName
 * @tc.desc      : Fuzz test
 */
HWTEST_F(InnerFuzzTest, SUB_MULTIMEDIA_AUDIO_ENCODER_FUZZ_002, TestSize.Level2)
{
    srand(time(nullptr) * 10);
    AudioEncoderDemo* encoderDemo = new AudioEncoderDemo();
    ASSERT_NE(nullptr, encoderDemo);
    int strLen = 0;
    string test_value = "";

    for (int i = 0; i < FUZZ_TEST_NUM; i++) {
        cout << "current run time is: " << i << endl;
        strLen = rand() % 65536;
        test_value = getRandStr(strLen);
        encoderDemo->InnerCreateByName(test_value.c_str());
        encoderDemo->InnerDestroy();
    }
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_FUZZ_003
 * @tc.name      : InnerConfigure
 * @tc.desc      : Fuzz test
 */
HWTEST_F(InnerFuzzTest, SUB_MULTIMEDIA_AUDIO_ENCODER_FUZZ_003, TestSize.Level2)
{
    srand(time(nullptr) * 10);
    AudioEncoderDemo* encoderDemo = new AudioEncoderDemo();
    ASSERT_NE(nullptr, encoderDemo);
    encoderDemo->InnerCreateByName("OH.Media.Codec.Encoder.Audio.AAC");
    Format format;

    for (int i = 0; i < FUZZ_TEST_NUM; i++) {
        cout << "current run time is: " << i << endl;
        int32_t bitRate = getIntRand();
        int32_t audioChannels = getIntRand();
        int32_t audioSampleRate = getIntRand();
        int32_t audioSampleKey = getIntRand();
        int32_t audioSampleFormat = getIntRand();
        int32_t audioChannelLayout = getIntRand();

        cout << "BIT_RATE is: " << bitRate << endl;
        cout << ", AUDIO_CHANNELS len is: " << audioChannels << endl;
        cout << ", SAMPLE_RATE len is: " << audioSampleRate << endl;

        format.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, audioChannels);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, audioSampleRate);
        format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, bitRate);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE, audioSampleKey);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, audioSampleFormat);
        format.PutLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, audioChannelLayout);

        encoderDemo->InnerConfigure(format);
        encoderDemo->InnerReset();
    }
    encoderDemo->InnerDestroy();
    delete encoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_FUZZ_004
 * @tc.name      : InnerSetParameter
 * @tc.desc      : Fuzz test
 */
HWTEST_F(InnerFuzzTest, SUB_MULTIMEDIA_AUDIO_ENCODER_FUZZ_004, TestSize.Level2)
{
    srand(time(nullptr) * 10);
    AudioEncoderDemo* encoderDemo = new AudioEncoderDemo();
    ASSERT_NE(nullptr, encoderDemo);
    encoderDemo->InnerCreateByName("OH.Media.Codec.Encoder.Audio.AAC");

    Format format;
    int32_t ret;

    format.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 44100);
    format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 112000);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE, AudioSampleFormat::SAMPLE_F32LE);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, AudioSampleFormat::SAMPLE_F32LE);
    format.PutLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, MONO);

    std::shared_ptr<AEncSignal> signal_ = encoderDemo->getSignal();
    std::shared_ptr<InnerAEnDemoCallback> cb_ = make_unique<InnerAEnDemoCallback>(signal_);
    ret = encoderDemo->InnerSetCallback(cb_);
    cout << "SetCallback ret is: " << ret << endl;

    ret = encoderDemo->InnerConfigure(format);
    cout << "Configure return: " << ret << endl;
    ret = encoderDemo->InnerPrepare();
    cout << "Prepare return: " << ret << endl;

    ret = encoderDemo->InnerStart();
    cout << "Start return: " << ret << endl;

    for (int i = 0; i < FUZZ_TEST_NUM; i++) {
        Format generalFormat;
        cout << "current run time is: " << i << endl;
        int32_t bitRate = getIntRand();
        int32_t audioChannels = getIntRand();
        int32_t audioSampleRate = getIntRand();
        int32_t audioSampleKey = getIntRand();
        int32_t audioSampleFormat = getIntRand();
        int32_t audioChannelLayout = getIntRand();
        cout << "BIT_RATE is: " << bitRate << endl;
        cout << ", AUDIO_CHANNELS len is: " << audioChannels << endl;
        cout << ", SAMPLE_RATE len is: " << audioSampleRate << endl;
        generalFormat.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, audioChannels);
        generalFormat.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, audioSampleRate);
        generalFormat.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, bitRate);
        generalFormat.PutIntValue(MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE, audioSampleKey);
        generalFormat.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, audioSampleFormat);
        generalFormat.PutLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, audioChannelLayout);
        ret = encoderDemo->InnerSetParameter(generalFormat);
        cout << "ret code is: " << ret << endl;
    }
    encoderDemo->InnerDestroy();
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_FUZZ_005
 * @tc.name      : InnerQueueInputBuffer
 * @tc.desc      : Fuzz test
 */
HWTEST_F(InnerFuzzTest, SUB_MULTIMEDIA_AUDIO_ENCODER_FUZZ_005, TestSize.Level2)
{
    srand(time(nullptr) * 10);
    AudioEncoderDemo* encoderDemo = new AudioEncoderDemo();
    ASSERT_NE(nullptr, encoderDemo);
    encoderDemo->InnerCreateByName("OH.Media.Codec.Encoder.Audio.AAC");
    Format format;
    format.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 44100);
    format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 112000);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE, AudioSampleFormat::SAMPLE_F32LE);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, AudioSampleFormat::SAMPLE_F32LE);
    format.PutLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, MONO);
    int32_t ret;
    int index = 0;

    std::shared_ptr<AEncSignal> signal_ = encoderDemo->getSignal();

    std::shared_ptr<InnerAEnDemoCallback> cb_ = make_unique<InnerAEnDemoCallback>(signal_);
    ret = encoderDemo->InnerSetCallback(cb_);
    cout << "SetCallback ret is: " << ret << endl;

    ret = encoderDemo->InnerConfigure(format);
    cout << "Configure ret is: " << ret << endl;
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = encoderDemo->InnerPrepare();
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = encoderDemo->InnerStart();
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


        ret = encoderDemo->InnerQueueInputBuffer(index, info, flag);
        cout << "ret code is: " << ret << endl;
    }
    encoderDemo->InnerDestroy();
    delete encoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_FUZZ_006
 * @tc.name      : InnerGetOutputFormat
 * @tc.desc      : Fuzz test
 */
HWTEST_F(InnerFuzzTest, SUB_MULTIMEDIA_AUDIO_ENCODER_FUZZ_006, TestSize.Level2)
{
    srand(time(nullptr) * 10);
    AudioEncoderDemo* encoderDemo = new AudioEncoderDemo();
    ASSERT_NE(nullptr, encoderDemo);

    encoderDemo->InnerCreateByName("OH.Media.Codec.Encoder.Audio.AAC");

    Format format;
    format.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 44100);
    format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 112000);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE, AudioSampleFormat::SAMPLE_F32LE);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, AudioSampleFormat::SAMPLE_F32LE);
    format.PutLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, MONO);

    int32_t ret;

    std::shared_ptr<AEncSignal> signal_ = encoderDemo->getSignal();

    std::shared_ptr<InnerAEnDemoCallback> cb_ = make_unique<InnerAEnDemoCallback>(signal_);
    ret = encoderDemo->InnerSetCallback(cb_);
    cout << "SetCallback ret is: " << ret << endl;

    ret = encoderDemo->InnerConfigure(format);
    cout << "Configure ret is: " << ret << endl;
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = encoderDemo->InnerPrepare();
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = encoderDemo->InnerStart();
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
        ret = encoderDemo->InnerGetOutputFormat(format);
        cout << "ret code is: " << ret << endl;
    }
    encoderDemo->InnerDestroy();
    delete encoderDemo;
}

/**
* @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_FUZZ_007
* @tc.name      : input file fuzz
* @tc.desc      : Fuzz test
*/
HWTEST_F(InnerFuzzTest, SUB_MULTIMEDIA_AUDIO_ENCODER_FUZZ_007, TestSize.Level2)
{
    srand(time(nullptr) * 10);
    AudioEncoderDemo* encoderDemo = new AudioEncoderDemo();

    int32_t ret = encoderDemo->InnerCreateByName("OH.Media.Codec.Encoder.Audio.AAC");
    ASSERT_EQ(AVCS_ERR_OK, ret);
    Format format;
    format.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 44100);
    format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 112000);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE, AudioSampleFormat::SAMPLE_F32LE);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, AudioSampleFormat::SAMPLE_F32LE);
    format.PutLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, MONO);

    std::shared_ptr<AEncSignal> signal = encoderDemo->getSignal();

    std::shared_ptr<InnerAEnDemoCallback> cb_ = make_unique<InnerAEnDemoCallback>(signal);
    ret = encoderDemo->InnerSetCallback(cb_);
    cout << "SetCallback ret is: " << ret << endl;

    ret = encoderDemo->InnerConfigure(format);
    cout << "Configure ret is: " << ret << endl;
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = encoderDemo->InnerPrepare();
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = encoderDemo->InnerStart();
    ASSERT_EQ(AVCS_ERR_OK, ret);

    vector<thread> threadVec;
    threadVec.push_back(thread(inputFunc, encoderDemo));
    threadVec.push_back(thread(outputFunc, encoderDemo));
    for (uint32_t i = 0; i < threadVec.size(); i++) {
        threadVec[i].join();
    }
    encoderDemo->InnerDestroy();
    delete encoderDemo;
}