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
#include "AudioDecoderDemoCommon.h"
#include "fcntl.h"
#include "media_description.h"
#include "avcodec_info.h"
#include "avcodec_errors.h"
#include "av_common.h"
#include "meta/format.h"

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
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_001
 * @tc.name      : InnerCreateByMime(1000 times)
 * @tc.desc      : Stability test
 */
HWTEST_F(InnerStablityTest, SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_001, TestSize.Level2)
{
    srand(time(nullptr) * 10);
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();

    string mimeType[] = { "audio/mp4a-latm", "audio/mpeg", "audio/vorbis", "audio/flac", "audio/amr-wb",
                          "audio/3gpp", "audio/g711mu" };

    for (int i = 0; i < RUN_TIMES; i++)
    {
        int typeIndex = rand() % 4;
        decoderDemo->InnerCreateByMime(mimeType[typeIndex].c_str());
        cout << "run time is: " << i << endl;
        int32_t ret = decoderDemo->InnerDestroy();
        ASSERT_EQ(AVCS_ERR_OK, ret);
    }

    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_002
 * @tc.name      : InnerCreateByName(1000 times)
 * @tc.desc      : Stability test
 */
HWTEST_F(InnerStablityTest, SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_002, TestSize.Level2)
{
    srand(time(nullptr) * 10);
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();

    for (int i = 0; i < RUN_TIMES; i++)
    {
        decoderDemo->InnerCreateByName("OH.Media.Codec.Decoder.Audio.Mpeg");
        cout << "run time is: " << i << endl;
        int32_t ret = decoderDemo->InnerDestroy();
        ASSERT_EQ(AVCS_ERR_OK, ret);
    }
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_003
 * @tc.name      : InnerPrepare(1000 times)
 * @tc.desc      : Stability test
 */
HWTEST_F(InnerStablityTest, SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_003, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    int32_t ret = decoderDemo->InnerCreateByName("OH.Media.Codec.Decoder.Audio.Mpeg");
    ASSERT_EQ(AVCS_ERR_OK, ret);
    std::shared_ptr<ADecSignal> signal_ = decoderDemo->getSignal();
    std::shared_ptr<InnerADecDemoCallback> cb_ = make_unique<InnerADecDemoCallback>(signal_);
    decoderDemo->InnerSetCallback(cb_);

    Format format;

    format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 320000);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 48000);

    decoderDemo->InnerConfigure(format);

    for (int i = 0; i < RUN_TIMES; i++)
    {
        ret = decoderDemo->InnerPrepare();
        cout << "run time is: " << i << ", ret is:" << ret << endl;
    }

    decoderDemo->InnerDestroy();

    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_004
 * @tc.name      : InnerStart(1000 times)
 * @tc.desc      : Stability test
 */
HWTEST_F(InnerStablityTest, SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_004, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    int32_t ret = decoderDemo->InnerCreateByName("OH.Media.Codec.Decoder.Audio.Mpeg");
    ASSERT_EQ(AVCS_ERR_OK, ret);
    std::shared_ptr<ADecSignal> signal_ = decoderDemo->getSignal();
    std::shared_ptr<InnerADecDemoCallback> cb_ = make_unique<InnerADecDemoCallback>(signal_);
    decoderDemo->InnerSetCallback(cb_);
    Format format;

    format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 320000);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 48000);

    decoderDemo->InnerConfigure(format);
    decoderDemo->InnerPrepare();

    for (int i = 0; i < RUN_TIMES; i++)
    {
        ret = decoderDemo->InnerStart();
        cout << "run time is: " << i << ", ret is:" << ret << endl;
    }

    decoderDemo->InnerDestroy();

    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_005
 * @tc.name      : InnerStop(1000 times)
 * @tc.desc      : Stability test
 */
HWTEST_F(InnerStablityTest, SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_005, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    int32_t ret = decoderDemo->InnerCreateByName("OH.Media.Codec.Decoder.Audio.Mpeg");
    ASSERT_EQ(AVCS_ERR_OK, ret);
    std::shared_ptr<ADecSignal> signal_ = decoderDemo->getSignal();
    std::shared_ptr<InnerADecDemoCallback> cb_ = make_unique<InnerADecDemoCallback>(signal_);
    decoderDemo->InnerSetCallback(cb_);
    Format format;

    format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 320000);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 48000);

    decoderDemo->InnerConfigure(format);
    decoderDemo->InnerPrepare();
    decoderDemo->InnerStart();

    for (int i = 0; i < RUN_TIMES; i++)
    {
        ret = decoderDemo->InnerStop();
        cout << "run time is: " << i << ", ret is:" << ret << endl;
    }

    decoderDemo->InnerDestroy();

    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_006
 * @tc.name      : InnerDestroy(1000 times)
 * @tc.desc      : Stability test
 */
HWTEST_F(InnerStablityTest, SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_006, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    int32_t ret = decoderDemo->InnerCreateByName("OH.Media.Codec.Decoder.Audio.Mpeg");
    ASSERT_EQ(AVCS_ERR_OK, ret);
    std::shared_ptr<ADecSignal> signal_ = decoderDemo->getSignal();
    std::shared_ptr<InnerADecDemoCallback> cb_ = make_unique<InnerADecDemoCallback>(signal_);
    decoderDemo->InnerSetCallback(cb_);
    Format format;

    format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 320000);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 48000);

    decoderDemo->InnerConfigure(format);
    decoderDemo->InnerPrepare();
    decoderDemo->InnerStart();
    decoderDemo->InnerStop();

    for (int i = 0; i < RUN_TIMES; i++)
    {
        ret = decoderDemo->InnerDestroy();
        cout << "run time is: " << i << ", ret is:" << ret << endl;
    }
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_007
 * @tc.name      : InnerReset(1000 times)
 * @tc.desc      : Stability test
 */
HWTEST_F(InnerStablityTest, SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_007, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    int32_t ret = decoderDemo->InnerCreateByName("OH.Media.Codec.Decoder.Audio.Mpeg");
    ASSERT_EQ(AVCS_ERR_OK, ret);
    std::shared_ptr<ADecSignal> signal_ = decoderDemo->getSignal();
    std::shared_ptr<InnerADecDemoCallback> cb_ = make_unique<InnerADecDemoCallback>(signal_);
    decoderDemo->InnerSetCallback(cb_);
    Format format;

    format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 320000);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 48000);

    decoderDemo->InnerConfigure(format);
    decoderDemo->InnerPrepare();
    decoderDemo->InnerStart();

    for (int i = 0; i < RUN_TIMES; i++)
    {
        ret = decoderDemo->InnerReset();
        cout << "run time is: " << i << ", ret is:" << ret << endl;
    }
    decoderDemo->InnerDestroy();
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_008
 * @tc.name      : InnerFlush(1000 times)
 * @tc.desc      : Stability test
 */
HWTEST_F(InnerStablityTest, SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_008, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    int32_t ret = decoderDemo->InnerCreateByName("OH.Media.Codec.Decoder.Audio.Mpeg");
    ASSERT_EQ(AVCS_ERR_OK, ret);
    std::shared_ptr<ADecSignal> signal_ = decoderDemo->getSignal();
    std::shared_ptr<InnerADecDemoCallback> cb_ = make_unique<InnerADecDemoCallback>(signal_);
    decoderDemo->InnerSetCallback(cb_);
    Format format;

    format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 320000);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 48000);

    decoderDemo->InnerConfigure(format);
    decoderDemo->InnerPrepare();
    decoderDemo->InnerStart();

    for (int i = 0; i < RUN_TIMES; i++)
    {
        ret = decoderDemo->InnerFlush();
        cout << "run time is: " << i << ", ret is:" << ret << endl;
    }
    decoderDemo->InnerDestroy();
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_009
 * @tc.name      : InnerRelease(1000 times)
 * @tc.desc      : Stability test
 */
HWTEST_F(InnerStablityTest, SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_009, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    int32_t ret = decoderDemo->InnerCreateByName("OH.Media.Codec.Decoder.Audio.Mpeg");
    ASSERT_EQ(AVCS_ERR_OK, ret);
    std::shared_ptr<ADecSignal> signal_ = decoderDemo->getSignal();
    std::shared_ptr<InnerADecDemoCallback> cb_ = make_unique<InnerADecDemoCallback>(signal_);
    decoderDemo->InnerSetCallback(cb_);
    Format format;

    format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 320000);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 48000);

    decoderDemo->InnerConfigure(format);
    decoderDemo->InnerPrepare();
    decoderDemo->InnerStart();

    for (int i = 0; i < RUN_TIMES; i++)
    {
        ret = decoderDemo->InnerRelease();
        cout << "run time is: " << i << ", ret is:" << ret << endl;
    }
    decoderDemo->InnerDestroy();
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_010
 * @tc.name      : MP3(long time)
 * @tc.desc      : Stability test
 */
HWTEST_F(InnerStablityTest, SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_010, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    time_t startTime = time(nullptr);
    ASSERT_NE(startTime, -1);
    time_t curTime = startTime;

    while (difftime(curTime, startTime) < RUN_TIME)
    {
        cout << "run time: " << difftime(curTime, startTime) << " seconds" << endl;
        std::string inputFilePath = "s16p_128k_16000_1_dayuhaitang.mp3";
        std::string outputFilePath = "SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_010.pcm";

        Format format;
        format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 128000);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 16000);

        decoderDemo->InnerRunCase(inputFilePath, outputFilePath, "OH.Media.Codec.Decoder.Audio.Mpeg", format);
        curTime = time(nullptr);
        ASSERT_NE(curTime, -1);
    }
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_011
 * @tc.name      : aac(long time)
 * @tc.desc      : Stability test
 */
HWTEST_F(InnerStablityTest, SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_011, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    time_t startTime = time(nullptr);
    ASSERT_NE(startTime, -1);
    time_t curTime = startTime;

    while (difftime(curTime, startTime) < RUN_TIME)
    {
        cout << "run time: " << difftime(curTime, startTime) << " seconds" << endl;
        std::string inputFilePath = "fltp_aac_low_128k_16000_2_dayuhaitang.aac";
        std::string outputFilePath = "SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_011.pcm";

        Format format;
        format.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 2);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 16000);
        format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 128000);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_AAC_IS_ADTS, 1);

        decoderDemo->InnerRunCase(inputFilePath, outputFilePath, "avdec_aac", format);
        curTime = time(nullptr);
        ASSERT_NE(curTime, -1);
    }
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_012
 * @tc.name      : avdec_vorbis(long time)
 * @tc.desc      : Stability test
 */
HWTEST_F(InnerStablityTest, SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_012, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    time_t startTime = time(nullptr);
    ASSERT_NE(startTime, -1);
    time_t curTime = startTime;

    while (difftime(curTime, startTime) < RUN_TIME)
    {
        cout << "run time: " << difftime(curTime, startTime) << " seconds" << endl;
        std::string inputFilePath = "fltp_128k_16000_2_dayuhaitang.ogg";
        std::string outputFilePath = "SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_012.pcm";

        Format format;
        format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 128000);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 2);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 16000);

        decoderDemo->InnerRunCase(inputFilePath, outputFilePath, "avdec_vorbis", format);
        curTime = time(nullptr);
        ASSERT_NE(curTime, -1);
    }
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_013
 * @tc.name      : avdec_flac(long time)
 * @tc.desc      : Stability test
 */
HWTEST_F(InnerStablityTest, SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_013, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    time_t startTime = time(nullptr);
    ASSERT_NE(startTime, -1);
    time_t curTime = startTime;

    while (difftime(curTime, startTime) < RUN_TIME)
    {
        cout << "run time: " << difftime(curTime, startTime) << " seconds" << endl;
        std::string inputFilePath = "s32_16000_2_dayuhaitang.flac";
        std::string outputFilePath = "SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_013.pcm";

        Format format;
        format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 357000);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 2);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 16000);
        format.PutIntValue("bits_per_coded_sample", 24);

        decoderDemo->InnerRunCase(inputFilePath, outputFilePath, "avdec_flac", format);
        curTime = time(nullptr);
        ASSERT_NE(curTime, -1);
    }
    delete decoderDemo;
}