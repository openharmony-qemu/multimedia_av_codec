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
#include <thread>
#include <vector>
#include <ctime>
#include "gtest/gtest.h"
#include "AudioEncoderDemoCommon.h"
#include "fcntl.h"
#include "media_description.h"
#include "avcodec_info.h"
#include "avcodec_errors.h"
#include "av_common.h"
#include "meta/format.h"
#include "avcodec_audio_common.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;

namespace {
    class InnerStablityTest : public testing::Test {
    public:
        static void SetUpTestCase();
        static void TearDownTestCase();
        void SetUp() override;
        void TearDown() override;
    };

    void InnerStablityTest::SetUpTestCase() {}
    void InnerStablityTest::TearDownTestCase() {}
    void InnerStablityTest::SetUp() {}
    void InnerStablityTest::TearDown() {}
    constexpr int RUN_TIMES = 100000;
    constexpr int RUN_TIME = 12 * 3600;
    string inputFilePath = "/data/audioIn.mp3";
    string outputFilePath = "/data/audioOut.pcm";
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_STABILITY_001
 * @tc.name      : InnerCreateByMime(1000 times)
 * @tc.desc      : Stability test
 */
HWTEST_F(InnerStablityTest, SUB_MULTIMEDIA_AUDIO_ENCODER_STABILITY_001, TestSize.Level2)
{
    srand(time(nullptr) * 10);
    AudioEncoderDemo* encoderDemo = new AudioEncoderDemo();
    ASSERT_NE(nullptr, encoderDemo);
    string mimeType[] = { "audio/mp4a-latm", "audio/flac" };

    for (int i = 0; i < RUN_TIMES; i++)
    {
        int typeIndex = rand() % 4;
        encoderDemo->InnerCreateByMime(mimeType[typeIndex].c_str());
        cout << "run time is: " << i << endl;
        encoderDemo->InnerDestroy();
    }

    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_STABILITY_002
 * @tc.name      : InnerCreateByName(1000 times)
 * @tc.desc      : Stability test
 */
HWTEST_F(InnerStablityTest, SUB_MULTIMEDIA_AUDIO_ENCODER_STABILITY_002, TestSize.Level2)
{
    srand(time(nullptr) * 10);
    AudioEncoderDemo* encoderDemo = new AudioEncoderDemo();
    ASSERT_NE(nullptr, encoderDemo);
    for (int i = 0; i < RUN_TIMES; i++)
    {
        encoderDemo->InnerCreateByName("OH.Media.Codec.Encoder.Audio.AAC");
        cout << "run time is: " << i << endl;
        encoderDemo->InnerDestroy();
    }
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_STABILITY_003
 * @tc.name      : InnerPrepare(1000 times)
 * @tc.desc      : Stability test
 */
HWTEST_F(InnerStablityTest, SUB_MULTIMEDIA_AUDIO_ENCODER_STABILITY_003, TestSize.Level2)
{
    AudioEncoderDemo* encoderDemo = new AudioEncoderDemo();
    encoderDemo->InnerCreateByName("OH.Media.Codec.Encoder.Audio.AAC");
    ASSERT_NE(nullptr, encoderDemo);
    Format format;

    format.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 44100);
    format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 112000);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE, AudioSampleFormat::SAMPLE_F32LE);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, AudioSampleFormat::SAMPLE_F32LE);
    format.PutLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, MONO);

    encoderDemo->InnerConfigure(format);

    for (int i = 0; i < RUN_TIMES; i++)
    {
        int32_t ret = encoderDemo->InnerPrepare();
        cout << "run time is: " << i << ", ret is:" << ret << endl;
    }

    encoderDemo->InnerDestroy();

    delete encoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_STABILITY_004
 * @tc.name      : InnerStart(1000 times)
 * @tc.desc      : Stability test
 */
HWTEST_F(InnerStablityTest, SUB_MULTIMEDIA_AUDIO_ENCODER_STABILITY_004, TestSize.Level2)
{
    AudioEncoderDemo* encoderDemo = new AudioEncoderDemo();
    encoderDemo->InnerCreateByName("OH.Media.Codec.Encoder.Audio.AAC");
    ASSERT_NE(nullptr, encoderDemo);
    Format format;

    format.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 44100);
    format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 112000);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE, AudioSampleFormat::SAMPLE_F32LE);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, AudioSampleFormat::SAMPLE_F32LE);
    format.PutLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, MONO);

    std::shared_ptr<AEncSignal> signal_ = encoderDemo->getSignal();
    std::shared_ptr<InnerAEnDemoCallback> cb_ = make_unique<InnerAEnDemoCallback>(signal_);
    int32_t ret = encoderDemo->InnerSetCallback(cb_);

    encoderDemo->InnerConfigure(format);
    encoderDemo->InnerPrepare();

    for (int i = 0; i < RUN_TIMES; i++)
    {
        ret = encoderDemo->InnerStart();
        cout << "run time is: " << i << ", ret is:" << ret << endl;
    }

    encoderDemo->InnerDestroy();

    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_STABILITY_005
 * @tc.name      : InnerStop(1000 times)
 * @tc.desc      : Stability test
 */
HWTEST_F(InnerStablityTest, SUB_MULTIMEDIA_AUDIO_ENCODER_STABILITY_005, TestSize.Level2)
{
    AudioEncoderDemo* encoderDemo = new AudioEncoderDemo();
    encoderDemo->InnerCreateByName("OH.Media.Codec.Encoder.Audio.AAC");
    ASSERT_NE(nullptr, encoderDemo);
    Format format;

    format.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 44100);
    format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 112000);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE, AudioSampleFormat::SAMPLE_F32LE);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, AudioSampleFormat::SAMPLE_F32LE);
    format.PutLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, MONO);

    std::shared_ptr<AEncSignal> signal_ = encoderDemo->getSignal();
    std::shared_ptr<InnerAEnDemoCallback> cb_ = make_unique<InnerAEnDemoCallback>(signal_);
    int32_t ret = encoderDemo->InnerSetCallback(cb_);

    encoderDemo->InnerConfigure(format);
    encoderDemo->InnerPrepare();
    encoderDemo->InnerStart();

    for (int i = 0; i < RUN_TIMES; i++)
    {
        ret = encoderDemo->InnerStop();
        cout << "run time is: " << i << ", ret is:" << ret << endl;
    }

    encoderDemo->InnerDestroy();

    delete encoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_STABILITY_006
 * @tc.name      : InnerDestroy(1000 times)
 * @tc.desc      : Stability test
 */
HWTEST_F(InnerStablityTest, SUB_MULTIMEDIA_AUDIO_ENCODER_STABILITY_006, TestSize.Level2)
{
    AudioEncoderDemo* encoderDemo = new AudioEncoderDemo();
    encoderDemo->InnerCreateByName("OH.Media.Codec.Encoder.Audio.AAC");
    ASSERT_NE(nullptr, encoderDemo);
    Format format;

    format.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 44100);
    format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 112000);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE, AudioSampleFormat::SAMPLE_F32LE);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, AudioSampleFormat::SAMPLE_F32LE);
    format.PutLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, MONO);

    std::shared_ptr<AEncSignal> signal_ = encoderDemo->getSignal();
    std::shared_ptr<InnerAEnDemoCallback> cb_ = make_unique<InnerAEnDemoCallback>(signal_);
    int32_t ret = encoderDemo->InnerSetCallback(cb_);

    encoderDemo->InnerConfigure(format);
    encoderDemo->InnerPrepare();
    encoderDemo->InnerStart();
    encoderDemo->InnerStop();

    for (int i = 0; i < RUN_TIMES; i++)
    {
        ret = encoderDemo->InnerDestroy();
        cout << "run time is: " << i << ", ret is:" << ret << endl;
    }

    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_STABILITY_007
 * @tc.name      : InnerReset(1000 times)
 * @tc.desc      : Stability test
 */
HWTEST_F(InnerStablityTest, SUB_MULTIMEDIA_AUDIO_ENCODER_STABILITY_007, TestSize.Level2)
{
    AudioEncoderDemo* encoderDemo = new AudioEncoderDemo();
    encoderDemo->InnerCreateByName("OH.Media.Codec.Encoder.Audio.AAC");
    ASSERT_NE(nullptr, encoderDemo);
    Format format;

    format.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 44100);
    format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 112000);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE, AudioSampleFormat::SAMPLE_F32LE);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, AudioSampleFormat::SAMPLE_F32LE);
    format.PutLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, MONO);

    std::shared_ptr<AEncSignal> signal_ = encoderDemo->getSignal();
    std::shared_ptr<InnerAEnDemoCallback> cb_ = make_unique<InnerAEnDemoCallback>(signal_);
    int32_t ret = encoderDemo->InnerSetCallback(cb_);

    encoderDemo->InnerConfigure(format);
    encoderDemo->InnerPrepare();
    encoderDemo->InnerStart();

    for (int i = 0; i < RUN_TIMES; i++)
    {
        ret = encoderDemo->InnerReset();
        cout << "run time is: " << i << ", ret is:" << ret << endl;
    }
    encoderDemo->InnerDestroy();
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_STABILITY_008
 * @tc.name      : InnerFlush(1000 times)
 * @tc.desc      : Stability test
 */
HWTEST_F(InnerStablityTest, SUB_MULTIMEDIA_AUDIO_ENCODER_STABILITY_008, TestSize.Level2)
{
    AudioEncoderDemo* encoderDemo = new AudioEncoderDemo();
    encoderDemo->InnerCreateByName("OH.Media.Codec.Encoder.Audio.AAC");
    ASSERT_NE(nullptr, encoderDemo);
    Format format;

    format.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 44100);
    format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 112000);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE, AudioSampleFormat::SAMPLE_F32LE);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, AudioSampleFormat::SAMPLE_F32LE);
    format.PutLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, MONO);

    std::shared_ptr<AEncSignal> signal_ = encoderDemo->getSignal();
    std::shared_ptr<InnerAEnDemoCallback> cb_ = make_unique<InnerAEnDemoCallback>(signal_);
    int32_t ret = encoderDemo->InnerSetCallback(cb_);

    encoderDemo->InnerConfigure(format);
    encoderDemo->InnerPrepare();
    encoderDemo->InnerStart();

    for (int i = 0; i < RUN_TIMES; i++)
    {
        ret = encoderDemo->InnerFlush();
        cout << "run time is: " << i << ", ret is:" << ret << endl;
    }
    encoderDemo->InnerDestroy();
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_STABILITY_009
 * @tc.name      : InnerRelease(1000 times)
 * @tc.desc      : Stability test
 */
HWTEST_F(InnerStablityTest, SUB_MULTIMEDIA_AUDIO_ENCODER_STABILITY_009, TestSize.Level2)
{
    AudioEncoderDemo* encoderDemo = new AudioEncoderDemo();
    encoderDemo->InnerCreateByName("OH.Media.Codec.Encoder.Audio.AAC");
    ASSERT_NE(nullptr, encoderDemo);
    Format format;

    format.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 44100);
    format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 112000);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE, AudioSampleFormat::SAMPLE_F32LE);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, AudioSampleFormat::SAMPLE_F32LE);
    format.PutLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, MONO);

    std::shared_ptr<AEncSignal> signal_ = encoderDemo->getSignal();
    std::shared_ptr<InnerAEnDemoCallback> cb_ = make_unique<InnerAEnDemoCallback>(signal_);
    int32_t ret = encoderDemo->InnerSetCallback(cb_);

    encoderDemo->InnerConfigure(format);
    encoderDemo->InnerPrepare();
    encoderDemo->InnerStart();

    for (int i = 0; i < RUN_TIMES; i++)
    {
        ret = encoderDemo->InnerRelease();
        cout << "run time is: " << i << ", ret is:" << ret << endl;
    }
    encoderDemo->InnerDestroy();
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_STABILITY_010
 * @tc.name      : aac(long time)
 * @tc.desc      : Function test
 */
HWTEST_F(InnerStablityTest, SUB_MULTIMEDIA_AUDIO_ENCODER_STABILITY_010, TestSize.Level2)
{
    AudioEncoderDemo* encoderDemo = new AudioEncoderDemo();
    time_t startTime = time(nullptr);
    ASSERT_NE(startTime, -1);
    time_t curTime = startTime;

    while (difftime(curTime, startTime) < RUN_TIME)
    {
        cout << "run time: " << difftime(curTime, startTime) << " seconds" << endl;
        inputFilePath = "f32le_44100_1_dayuhaitang.pcm";
        outputFilePath = "FUNCTION_010_stability.aac";

        Format format;
        format.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 44100);
        format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 112000);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE, AudioSampleFormat::SAMPLE_F32LE);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, AudioSampleFormat::SAMPLE_F32LE);
        format.PutLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, MONO);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_AAC_IS_ADTS, 1);

        encoderDemo->InnerRunCase(inputFilePath, outputFilePath, "OH.Media.Codec.Encoder.Audio.AAC", format);
        curTime = time(nullptr);
        ASSERT_NE(curTime, -1);
    }
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_STABILITY_011
 * @tc.name      : flac(long time)
 * @tc.desc      : Function test
 */
HWTEST_F(InnerStablityTest, SUB_MULTIMEDIA_AUDIO_ENCODER_STABILITY_011, TestSize.Level2)
{
    AudioEncoderDemo* encoderDemo = new AudioEncoderDemo();
    time_t startTime = time(nullptr);
    ASSERT_NE(startTime, -1);
    time_t curTime = startTime;

    while (difftime(curTime, startTime) < RUN_TIME)
    {
        cout << "run time: " << difftime(curTime, startTime) << " seconds" << endl;
        inputFilePath = "s16_44100_2_dayuhaitang.pcm";
        outputFilePath = "FUNCTION_011_stability.flac";

        Format format;
        format.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 2);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 44100);
        format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 112000);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE, AudioSampleFormat::SAMPLE_S32LE);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, AudioSampleFormat::SAMPLE_S32LE);
        format.PutLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, STEREO);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_COMPLIANCE_LEVEL, -2);
        encoderDemo->InnerRunCase(inputFilePath, outputFilePath, "OH.Media.Codec.Encoder.Audio.Flac", format);
        curTime = time(nullptr);
        ASSERT_NE(curTime, -1);
    }
    delete encoderDemo;
}
