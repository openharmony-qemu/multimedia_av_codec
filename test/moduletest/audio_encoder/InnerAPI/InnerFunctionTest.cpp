/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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
#include <unistd.h>
#include "gtest/gtest.h"
#include "AudioEncoderDemoCommon.h"
#include "avcodec_errors.h"
#include "avcodec_common.h"
#include "media_description.h"
#include "securec.h"
#include "avcodec_audio_common.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"
#include "libavutil/samplefmt.h"
#include "libavutil/timestamp.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;
constexpr uint32_t SAMPLE_RATE_16000 = 16000;
constexpr uint32_t SAMPLE_RATE_128000 = 128000;
constexpr uint32_t SAMPLE_RATE_112000 = 112000;
constexpr uint32_t CHANNEL_COUNT = 2;
constexpr uint32_t CHANNEL_COUNT_2 = -2;
int32_t testResult[10] = { -1 };

namespace {
    class InnerFunctionTest : public testing::Test {
    public:
        static void SetUpTestCase();
        static void TearDownTestCase();
        void SetUp() override;
        void TearDown() override;
    };

    void InnerFunctionTest::SetUpTestCase() {}
    void InnerFunctionTest::TearDownTestCase() {}
    void InnerFunctionTest::SetUp() {}
    void InnerFunctionTest::TearDown() {}
    
    void runEncode(string encoderName, string inputFile, string outputFile, int32_t threadId)
    {
        AudioEncoderDemo* encoderDemo = new AudioEncoderDemo();
        if (encoderName == "OH.Media.Codec.Encoder.Audio.AAC") {
            Format format;
            format.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
            format.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, SAMPLE_RATE_16000);
            format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, SAMPLE_RATE_128000);
            format.PutIntValue(MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE, AudioSampleFormat::SAMPLE_F32LE);
            format.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, AudioSampleFormat::SAMPLE_F32LE);
            format.PutLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, MONO);
            format.PutIntValue(MediaDescriptionKey::MD_KEY_AAC_IS_ADTS, 1);
            encoderDemo->InnerRunCase(inputFile, outputFile, encoderName.c_str(), format);
        } else if (encoderName == "OH.Media.Codec.Encoder.Audio.Flac") {
            Format format;
            format.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, CHANNEL_COUNT);
            format.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, SAMPLE_RATE_16000);
            format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, SAMPLE_RATE_112000);
            format.PutIntValue(MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE, AudioSampleFormat::SAMPLE_S32LE);
            format.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, AudioSampleFormat::SAMPLE_S32LE);
            format.PutLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, STEREO);
            format.PutIntValue(MediaDescriptionKey::MD_KEY_COMPLIANCE_LEVEL, CHANNEL_COUNT_2);

            encoderDemo->InnerRunCase(inputFile, outputFile, encoderName.c_str(), format);
        }
        testResult[threadId] = AVCS_ERR_OK;
        delete encoderDemo;
    }
    }

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_FUNCTION_001
 * @tc.name      : aac(different sample rate)
 * @tc.desc      : Function test
 */
HWTEST_F(InnerFunctionTest, SUB_MULTIMEDIA_AUDIO_ENCODER_FUNCTION_001, TestSize.Level2)
{
    AudioEncoderDemo* encoderDemo = new AudioEncoderDemo();
    ASSERT_NE(nullptr, encoderDemo);

    string encoderName = "OH.Media.Codec.Encoder.Audio.AAC";

    int32_t sample_rate_List[] = {7350, 8000, 16000, 44100, 48000, 96000};

    for (int i = 0; i < 6; i++)
    {
        Format format;
        int sample_rate = sample_rate_List[i];
        format.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, sample_rate);
        format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, SAMPLE_RATE_128000);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE, AudioSampleFormat::SAMPLE_F32LE);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, AudioSampleFormat::SAMPLE_F32LE);
        format.PutLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, MONO);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_AAC_IS_ADTS, 1);

        string inputFile = "f32le_" + to_string(sample_rate) + "_1_dayuhaitang.pcm";
        string outputFile = "FUNCTION_001_" + to_string(sample_rate) + "_1_output.aac";

        cout << "inputFile  " <<  inputFile << endl;
        encoderDemo->InnerRunCase(inputFile, outputFile, encoderName.c_str(), format);
    }
    delete encoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_FUNCTION_002
 * @tc.name      : AAC different Channel_count
 * @tc.desc      : Function test
 */
HWTEST_F(InnerFunctionTest, SUB_MULTIMEDIA_AUDIO_ENCODER_FUNCTION_002, TestSize.Level2)
{
    AudioEncoderDemo* encoderDemo = new AudioEncoderDemo();
    ASSERT_NE(nullptr, encoderDemo);

    string encoderName = "OH.Media.Codec.Encoder.Audio.AAC";

    int Channel_count_List[] = {1, 2, 8};

    for (int i = 0; i < 3; i++)
    {
        Format format;
        int Channel_count = Channel_count_List[i];
        switch (Channel_count)
        {
        case 1:
            format.PutLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, MONO);
            break;
        case 2:
            format.PutLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, STEREO);
            break;
        case 8:
            format.PutLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, CH_7POINT1);
            break;
        default:
            format.PutLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, UNKNOWN_CHANNEL_LAYOUT);
            break;
        }

        format.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, Channel_count);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, SAMPLE_RATE_16000);
        format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, SAMPLE_RATE_128000);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE, AudioSampleFormat::SAMPLE_F32LE);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, AudioSampleFormat::SAMPLE_F32LE);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_AAC_IS_ADTS, 1);

        string inputFile = "f32le_16000_" + to_string(Channel_count) + "_dayuhaitang.pcm";
        string outputFile = "FUNCTION_002_16000_" + to_string(Channel_count) + "_output.aac";

        cout << "inputFile  " <<  inputFile << endl;
        encoderDemo->InnerRunCase(inputFile, outputFile, encoderName.c_str(), format);
    }
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_FUNCTION_003
 * @tc.name      : AAC different bitrate
 * @tc.desc      : Function test
 */
HWTEST_F(InnerFunctionTest, SUB_MULTIMEDIA_AUDIO_ENCODER_FUNCTION_003, TestSize.Level2)
{
    AudioEncoderDemo* encoderDemo = new AudioEncoderDemo();
    ASSERT_NE(nullptr, encoderDemo);

    string encoderName = "OH.Media.Codec.Encoder.Audio.AAC";

    int32_t bitrateList[] = {32000, 96000, 128000, 256000, 300000, 500000};

    for (int i = 0; i < 6; i++)
    {
        Format format;
        int32_t bitrate = bitrateList[i];

        format.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, SAMPLE_RATE_16000);
        format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, bitrate);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE, AudioSampleFormat::SAMPLE_F32LE);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, AudioSampleFormat::SAMPLE_F32LE);
        format.PutLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, MONO);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_AAC_IS_ADTS, 1);

        string inputFile = "f32le_16000_1_dayuhaitang.pcm";
        string outputFile = "FUNCTION_003_16000_1_" + to_string((int)(bitrate/1000)) + "k_output.aac";

        cout << "inputFile  " <<  inputFile << endl;
        encoderDemo->InnerRunCase(inputFile, outputFile, encoderName.c_str(), format);
    }
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_FUNCTION_004
 * @tc.name      : FLAC(different sample rate)
 * @tc.desc      : Function test
 */
HWTEST_F(InnerFunctionTest, SUB_MULTIMEDIA_AUDIO_ENCODER_FUNCTION_004, TestSize.Level2)
{
    AudioEncoderDemo* encoderDemo = new AudioEncoderDemo();
    ASSERT_NE(nullptr, encoderDemo);

    string encoderName = "OH.Media.Codec.Encoder.Audio.Flac";

    int32_t sample_rate_List[] = {8000, 16000, 44100, 48000, 96000};

    for (int i = 0; i < 5; i++)
    {
        Format format;
        int32_t sample_rate = sample_rate_List[i];
        format.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, CHANNEL_COUNT);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, sample_rate);
        format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 50000);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE, AudioSampleFormat::SAMPLE_S32LE);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, AudioSampleFormat::SAMPLE_S32LE);
        format.PutLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, STEREO);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_COMPLIANCE_LEVEL, -2);

        string inputFile = "s16_" + to_string(sample_rate) + "_2_dayuhaitang.pcm";
        string outputFile = "FUNCTION_004_" + to_string(sample_rate) + "_2_output.flac";

        cout << "inputFile  " <<  inputFile << endl;
        encoderDemo->InnerRunCase(inputFile, outputFile, encoderName.c_str(), format);
    }
    delete encoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_FUNCTION_005
 * @tc.name      : flac different format
 * @tc.desc      : Function test
 */
HWTEST_F(InnerFunctionTest, SUB_MULTIMEDIA_AUDIO_ENCODER_FUNCTION_005, TestSize.Level2)
{
    AudioEncoderDemo* encoderDemo = new AudioEncoderDemo();
    ASSERT_NE(nullptr, encoderDemo);

    string encoderName = "OH.Media.Codec.Encoder.Audio.Flac";

    string formatList[] = { "s16", "s32" };
    uint32_t sample_format;
    for (int i = 0; i < 2; i++)
    {
        Format format;
        string sample_Media::FORMAT_TYPE = formatList[i];
            if (sample_Media::FORMAT_TYPE == "s16") {
            sample_format = AudioSampleFormat::SAMPLE_S16LE;
        }
        if (sample_Media::FORMAT_TYPE == "s32") {
            sample_format = AudioSampleFormat::SAMPLE_S32LE;
        }
        format.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, CHANNEL_COUNT);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 48000);
        format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 50000);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE, sample_format);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, sample_format);
        format.PutLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, STEREO);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_COMPLIANCE_LEVEL, -2);

        string inputFile = sample_Media::FORMAT_TYPE + "_48000_2_dayuhaitang.pcm";
        string outputFile = "FUNCTION_005_48000_2_1536k_" + sample_Media::FORMAT_TYPE + "_output.flac";

        cout << "inputFile  " <<  inputFile << endl;
        encoderDemo->InnerRunCase(inputFile, outputFile, encoderName.c_str(), format);
    }
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_FUNCTION_006
 * @tc.name      : flac different Channel_count
 * @tc.desc      : Function test
 */
HWTEST_F(InnerFunctionTest, SUB_MULTIMEDIA_AUDIO_ENCODER_FUNCTION_006, TestSize.Level2)
{
    AudioEncoderDemo* encoderDemo = new AudioEncoderDemo();
    ASSERT_NE(nullptr, encoderDemo);

    string encoderName = "OH.Media.Codec.Encoder.Audio.Flac";

    int Channel_count_List[] = { 1, 2, 8};

    for (int i = 0; i < 3; i++)
    {
        Format format;
        int32_t Channel_count = Channel_count_List[i];
        
        switch (Channel_count)
        {
        case 1:
            format.PutLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, MONO);
            break;
        case 2:
            format.PutLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, STEREO);
            break;
        case 8:
            format.PutLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, CH_7POINT1);
            break;
        default:
            format.PutLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, UNKNOWN_CHANNEL_LAYOUT);
            break;
        }

        format.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, Channel_count);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 48000);
        format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 50000);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE, AudioSampleFormat::SAMPLE_S32LE);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, AudioSampleFormat::SAMPLE_S32LE);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_COMPLIANCE_LEVEL, -2);

        string inputFile = "s16_48000_" + to_string(Channel_count) + "_dayuhaitang.pcm";
        string outputFile = "FUNCTION_006_48000_" + to_string(Channel_count) + "_output.flac";

        cout << "inputFile  " <<  inputFile << endl;
        encoderDemo->InnerRunCase(inputFile, outputFile, encoderName.c_str(), format);
    }
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_FUNCTION_007
 * @tc.name      : aac flush
 * @tc.desc      : Function test
 */
HWTEST_F(InnerFunctionTest, SUB_MULTIMEDIA_AUDIO_ENCODER_FUNCTION_007, TestSize.Level2)
{
    AudioEncoderDemo* encoderDemo = new AudioEncoderDemo();
    ASSERT_NE(nullptr, encoderDemo);

    string encoderName = "OH.Media.Codec.Encoder.Audio.AAC";

    string inputFile = "f32le_44100_1_dayuhaitang.pcm";
    string firstOutputFile = "FUNCTION_007_InnerFlush1.aac";
    string secondOutputFile = "FUNCTION_007_InnerFlush2.aac";

    Format format;
    format.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 44100);
    format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, SAMPLE_RATE_112000);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE, AudioSampleFormat::SAMPLE_F32LE);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, AudioSampleFormat::SAMPLE_F32LE);
    format.PutLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, MONO);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_AAC_IS_ADTS, 1);

    cout << "inputFile  " <<  inputFile << endl;
    encoderDemo->InnerRunCaseFlush(inputFile, firstOutputFile, secondOutputFile, encoderName.c_str(), format);

    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_FUNCTION_008
 * @tc.name      : aac reset
 * @tc.desc      : Function test
 */
HWTEST_F(InnerFunctionTest, SUB_MULTIMEDIA_AUDIO_ENCODER_FUNCTION_008, TestSize.Level2)
{
    AudioEncoderDemo* encoderDemo = new AudioEncoderDemo();
    ASSERT_NE(nullptr, encoderDemo);

    string encoderName = "OH.Media.Codec.Encoder.Audio.AAC";

    string inputFile = "f32le_44100_1_dayuhaitang.pcm";
    string firstOutputFile = "FUNCTION_008_InnerReset1.aac";
    string secondOutputFile = "FUNCTION_008_InnerReset2.aac";

    Format format;
    format.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 44100);
    format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 112000);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE, AudioSampleFormat::SAMPLE_F32LE);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, AudioSampleFormat::SAMPLE_F32LE);
    format.PutLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, MONO);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_AAC_IS_ADTS, 1);

    cout << "inputFile  " <<  inputFile << endl;
    encoderDemo->InnerRunCaseReset(inputFile, firstOutputFile, secondOutputFile, encoderName.c_str(), format);

    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_FUNCTION_009
 * @tc.name      : flac flush
 * @tc.desc      : Function test
 */
HWTEST_F(InnerFunctionTest, SUB_MULTIMEDIA_AUDIO_ENCODER_FUNCTION_009, TestSize.Level2)
{
    AudioEncoderDemo* encoderDemo = new AudioEncoderDemo();
    ASSERT_NE(nullptr, encoderDemo);

    string encoderName = "OH.Media.Codec.Encoder.Audio.Flac";

    string inputFile = "s16_44100_2_dayuhaitang.pcm";
    string firstOutputFile = "FUNCTION_009_InnerFlush1.flac";
    string secondOutputFile = "FUNCTION_009_InnerFlush2.flac";


    Format format;
    format.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, CHANNEL_COUNT);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 44100);
    format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, SAMPLE_RATE_112000);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE, AudioSampleFormat::SAMPLE_S32LE);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, AudioSampleFormat::SAMPLE_S32LE);
    format.PutLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, STEREO);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_COMPLIANCE_LEVEL, -2);

    cout << "inputFile  " <<  inputFile << endl;
    encoderDemo->InnerRunCaseFlush(inputFile, firstOutputFile, secondOutputFile, encoderName.c_str(), format);

    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_FUNCTION_010
 * @tc.name      : flac reset
 * @tc.desc      : Function test
 */
HWTEST_F(InnerFunctionTest, SUB_MULTIMEDIA_AUDIO_ENCODER_FUNCTION_010, TestSize.Level2)
{
    AudioEncoderDemo* encoderDemo = new AudioEncoderDemo();
    ASSERT_NE(nullptr, encoderDemo);

    string encoderName = "OH.Media.Codec.Encoder.Audio.Flac";

    string inputFile = "s16_44100_2_dayuhaitang.pcm";
    string firstOutputFile = "FUNCTION_010_InnerReset1.flac";
    string secondOutputFile = "FUNCTION_010_InnerReset2.flac";

    Format format;
    format.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, CHANNEL_COUNT);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 44100);
    format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, SAMPLE_RATE_112000);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE, AudioSampleFormat::SAMPLE_S32LE);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, AudioSampleFormat::SAMPLE_S32LE);
    format.PutLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, STEREO);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_COMPLIANCE_LEVEL, -2);

    cout << "inputFile  " <<  inputFile << endl;
    encoderDemo->InnerRunCaseReset(inputFile, firstOutputFile, secondOutputFile, encoderName.c_str(), format);

    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_FUNCTION_011
 * @tc.name      : AAC(thread)
 * @tc.desc      : Function test
 */
HWTEST_F(InnerFunctionTest, SUB_MULTIMEDIA_AUDIO_ENCODER_FUNCTION_011, TestSize.Level2)
{
    vector<thread> threadVec;
    string decoderName = "OH.Media.Codec.Encoder.Audio.AAC";

    string inputFile = "f32le_16000_1_dayuhaitang.pcm";

    for (int32_t i = 0; i < 16; i++)
    {
        string outputFile = "FUNCTION_011_" + to_string(i) + ".aac";
        threadVec.push_back(thread(runEncode, decoderName, inputFile, outputFile, i));
    }
    for (uint32_t i = 0; i < threadVec.size(); i++)
    {
        threadVec[i].join();
    }
    for (int32_t i = 0; i < 10; i++)
    {
        ASSERT_EQ(AVCS_ERR_OK, testResult[i]);
    }
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_FUNCTION_012
 * @tc.name      : Flac(thread)
 * @tc.desc      : Function test
 */
HWTEST_F(InnerFunctionTest, SUB_MULTIMEDIA_AUDIO_ENCODER_FUNCTION_012, TestSize.Level2)
{
    vector<thread> threadVec;
    string decoderName = "OH.Media.Codec.Encoder.Audio.Flac";

    string inputFile = "s16_16000_2_dayuhaitang.pcm";

    for (int i = 0; i < 16; i++)
    {
        string outputFile = "FUNCTION_012_" + to_string(i) + ".flac";
        threadVec.push_back(thread(runEncode, decoderName, inputFile, outputFile, i));
    }
    for (uint32_t i = 0; i < threadVec.size(); i++)
    {
        threadVec[i].join();
    }
    for (int32_t i = 0; i < 10; i++)
    {
        ASSERT_EQ(AVCS_ERR_OK, testResult[i]);
    }
}
