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
#include "AudioDecoderDemoCommon.h"
#include "avcodec_errors.h"
#include "avcodec_common.h"
#include "media_description.h"
#include "securec.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;


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

    int32_t testResult[10] = { -1 };
    
    bool compareFile(string file1, string file2)
    {
        string ans1, ans2;
        int i;
        (void)freopen(file1.c_str(), "r", stdin);
        char c;
        while (scanf_s("%c", &c, 1) != EOF) {
            ans1 += c;
        }
        (void)fclose(stdin);

        (void)freopen(file2.c_str(), "r", stdin);
        while (scanf_s("%c", &c, 1) != EOF) {
            ans2 += c;
        }
        (void)fclose(stdin);
        if (ans1.size() != ans2.size()) {
            return false;
        }
        for (i = 0; i < ans1.size(); i++) {
            if (ans1[i] != ans2[i]) {
                return false;
            }
        }
        return true;
    }

    void runDecode(string decoderName, string inputFile, string outputFile, int32_t threadId)
    {
        AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();

        if (decoderName == "OH.Media.Codec.Decoder.Audio.AAC") {
            Format format;
            uint32_t CHANNEL_COUNT = 1;
            uint32_t SAMPLE_RATE = 16100;
            uint32_t BITS_RATE = 128000;
            uint32_t DEFAULT_AAC_IS_ADTS = 1;
            format.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, CHANNEL_COUNT);
            format.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, SAMPLE_RATE);
            format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, BITS_RATE);
            format.PutIntValue(MediaDescriptionKey::MD_KEY_AAC_IS_ADTS, DEFAULT_AAC_IS_ADTS);
            decoderDemo->InnerRunCase(inputFile, outputFile, decoderName.c_str(), format);
        } else if (decoderName == "OH.Media.Codec.Decoder.Audio.Mpeg") {
            Format format;
            uint32_t CHANNEL_COUNT = 2;
            uint32_t SAMPLE_RATE = 44100;
            uint32_t BITS_RATE = 128000;
            format.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, CHANNEL_COUNT);
            format.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, SAMPLE_RATE);
            format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, BITS_RATE);
            decoderDemo->InnerRunCase(inputFile, outputFile, decoderName.c_str(), format);
        } else if (decoderName == "OH.Media.Codec.Decoder.Audio.Flac") {
            Format format;
            uint32_t CHANNEL_COUNT = 2;
            uint32_t SAMPLE_RATE = 8000;
            uint32_t BITS_RATE = 199000;
            uint32_t BITS_PER_CODED_SAMPLE = 24;
            format.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, CHANNEL_COUNT);
            format.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, SAMPLE_RATE);
            format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, BITS_RATE);
            format.PutIntValue(MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE, BITS_PER_CODED_SAMPLE);
            decoderDemo->InnerRunCase(inputFile, outputFile, decoderName.c_str(), format);
        } else {
            Format format;
            uint32_t CHANNEL_COUNT = 2;
            uint32_t SAMPLE_RATE = 48000;
            uint32_t BITS_RATE = 45000;
            format.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, CHANNEL_COUNT);
            format.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, SAMPLE_RATE);
            format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, BITS_RATE);
            decoderDemo->InnerRunCase(inputFile, outputFile, decoderName.c_str(), format);
        }
        testResult[threadId] = AVCS_ERR_OK;
        delete decoderDemo;
    }
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_001
 * @tc.name      : AAC different sample_rate
 * @tc.desc      : Function test
 */
HWTEST_F(InnerFunctionTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_001, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    ASSERT_NE(nullptr, decoderDemo);

    string decoderName = "OH.Media.Codec.Decoder.Audio.AAC";

    int32_t sample_rate_List[] = {7350, 16000, 44100, 48000, 96000};

    for (int i = 0; i < 5; i++)
    {
        Format format;
        int32_t sample_rate = sample_rate_List[i];
        cout << "sample_rate is: " << sample_rate << endl;
        format.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, sample_rate);
        format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 128000);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_AAC_IS_ADTS, 1);

        string inputFile = "fltp_128k_" + to_string(sample_rate) + "_1_dayuhaitang.aac";
        string outputFile = "FUNCTION_001_" + to_string(sample_rate) + "_1_aac_output.pcm";

        decoderDemo->InnerRunCase(inputFile, outputFile, decoderName.c_str(), format);
    }
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_002
 * @tc.name      : MP3 different sample_rate
 * @tc.desc      : Function test
 */
HWTEST_F(InnerFunctionTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_002, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    ASSERT_NE(nullptr, decoderDemo);

    string decoderName = "OH.Media.Codec.Decoder.Audio.Mpeg";

    int32_t sample_rate_List[] = {8000, 16000, 44100, 48000};

    for (int i = 0; i < 4; i++)
    {
        Format format;
        int32_t sample_rate = sample_rate_List[i];
        cout << "sample_rate is: " << sample_rate << endl;
        format.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 2);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, sample_rate);
        format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 40000);

        string inputFile = "fltp_40k_" + to_string(sample_rate) + "_2_dayuhaitang.mp3";;
        string outputFile = "FUNCTION_002_" + to_string(sample_rate) + "_2_mp3_output.pcm";

        decoderDemo->InnerRunCase(inputFile, outputFile, decoderName.c_str(), format);
    }
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_003
 * @tc.name      : vorbis sample_rate
 * @tc.desc      : Function test
 */
HWTEST_F(InnerFunctionTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_003, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    ASSERT_NE(nullptr, decoderDemo);

    string decoderName = "OH.Media.Codec.Decoder.Audio.Vorbis";

    int32_t sample_rate_List[] = {8000, 16000, 44100, 48000};

    for (int i = 0; i < 4; i++)
    {
        Format format;
        int32_t sample_rate = sample_rate_List[i];
        cout << "sample_rate is: " << sample_rate << endl;
        format.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 2);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, sample_rate);
        format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 45000);

        string inputFile = "fltp_45k_" + to_string(sample_rate) + "_2_dayuhaitang.ogg";
        string outputFile = "FUNCTION_003_" + to_string(sample_rate) + "_2_vorbis_output.pcm";

        decoderDemo->InnerRunCase(inputFile, outputFile, decoderName.c_str(), format);
    }
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_004
 * @tc.name      : flac different sample_rate
 * @tc.desc      : Function test
 */
HWTEST_F(InnerFunctionTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_004, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    ASSERT_NE(nullptr, decoderDemo);

    string decoderName = "OH.Media.Codec.Decoder.Audio.Flac";

    int32_t sample_rate_List[] = {8000, 16000, 44100, 48000, 192000};

    for (int i = 0; i < 5; i++)
    {
        Format format;
        int32_t sample_rate = sample_rate_List[i];
        cout << "sample_rate is: " << sample_rate << endl;
        format.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 2);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, sample_rate);
        format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 199000);
        format.PutIntValue("bits_per_coded_sample", 24);

        string inputFile = "s16_" + to_string(sample_rate) + "_2_dayuhaitang.flac";;
        string outputFile = "FUNCTION_004_" + to_string(sample_rate) + "_2_flac_output.pcm";

        decoderDemo->InnerRunCase(inputFile, outputFile, decoderName.c_str(), format);
    }
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_005
 * @tc.name      : AAC different Channel_count
 * @tc.desc      : Function test
 */
HWTEST_F(InnerFunctionTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_005, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    ASSERT_NE(nullptr, decoderDemo);

    string decoderName = "OH.Media.Codec.Decoder.Audio.AAC";

    int Channel_count_List[] = {1, 2, 8};

    for (int i = 0; i < 3; i++)
    {
        Format format;
        int Channel_count = Channel_count_List[i];
        format.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, Channel_count);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 44100);
        format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 128000);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_AAC_IS_ADTS, 1);

        string inputFile = "fltp_128k_44100_" + to_string(Channel_count) + "_dayuhaitang.aac";
        string outputFile = "FUNCTION_005_44100_" + to_string(Channel_count) + "_aac_output.pcm";

        decoderDemo->InnerRunCase(inputFile, outputFile, decoderName.c_str(), format);
    }
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_006
 * @tc.name      : mp3 different Channel_count
 * @tc.desc      : Function test
 */
HWTEST_F(InnerFunctionTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_006, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    ASSERT_NE(nullptr, decoderDemo);

    string decoderName = "OH.Media.Codec.Decoder.Audio.Mpeg";

    int Channel_count_List[] = {1, 2};

    for (int i = 0; i < 2; i++)
    {
        Format format;
        int Channel_count = Channel_count_List[i];
        format.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, Channel_count);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 44100);
        format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 320000);

        string inputFile = "fltp_128k_44100_" + to_string(Channel_count) + "_dayuhaitang.mp3";
        string outputFile = "FUNCTION_006_44100_" + to_string(Channel_count) + "_mp3_output.pcm";

        decoderDemo->InnerRunCase(inputFile, outputFile, decoderName.c_str(), format);
    }
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_007
 * @tc.name      : flac different Channel_count
 * @tc.desc      : Function test
 */
HWTEST_F(InnerFunctionTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_007, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    ASSERT_NE(nullptr, decoderDemo);

    string decoderName = "OH.Media.Codec.Decoder.Audio.Flac";

    int Channel_count_List[] = {1, 2, 8};

    for (int i = 0; i < 3; i++)
    {
        Format format;
        int Channel_count = Channel_count_List[i];
        format.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, Channel_count);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 48000);
        format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 320000);
        format.PutIntValue("bits_per_coded_sample", 24);

        string inputFile = "s16_48000_" + to_string(Channel_count) + "_dayuhaitang.flac";
        string outputFile = "FUNCTION_007_48000_" + to_string(Channel_count) + "_flac_output.pcm";

        decoderDemo->InnerRunCase(inputFile, outputFile, decoderName.c_str(), format);
    }
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_008
 * @tc.name      : vorbis different Channel_count
 * @tc.desc      : Function test
 */
HWTEST_F(InnerFunctionTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_008, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    ASSERT_NE(nullptr, decoderDemo);

    string decoderName = "OH.Media.Codec.Decoder.Audio.Vorbis";

    int Channel_count_List[] = {1, 2};

    for (int i = 0; i < 2; i++)
    {
        Format format;
        int Channel_count = Channel_count_List[i];
        format.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, Channel_count);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 48000);
        format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 128000);
        format.PutIntValue("bits_per_coded_sample", 4);

        string inputFile = "fltp_128k_48000_" + to_string(Channel_count) + "_dayuhaitang.ogg";
        string outputFile = "FUNCTION_008_48000_" + to_string(Channel_count) + "_vorbis_output.pcm";

        decoderDemo->InnerRunCase(inputFile, outputFile, decoderName.c_str(), format);
    }
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_009
 * @tc.name      : AAC different bitrate
 * @tc.desc      : Function test
 */
HWTEST_F(InnerFunctionTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_009, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    ASSERT_NE(nullptr, decoderDemo);

    string decoderName = "OH.Media.Codec.Decoder.Audio.AAC";

    int32_t bitrateList[] = {32000, 128000};
    int32_t sample_rate_List[] = {16000, 44100};
    string fileList[] = {"fltp_32k_16000_2_dayuhaitang.aac", "fltp_128k_44100_2_dayuhaitang.aac"};
    for (int i = 0; i < 2; i++)
    {
        Format format;
        int32_t bitrate = bitrateList[i];
        int32_t sample_rate = sample_rate_List[i];
        format.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 2);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, sample_rate);
        format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, bitrate);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_AAC_IS_ADTS, 1);

        string inputFile = fileList[i];
        string outputFile = "FUNCTION_009_16000_2_" + to_string(bitrate) + "_aac_output.pcm";

        decoderDemo->InnerRunCase(inputFile, outputFile, decoderName.c_str(), format);
    }
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_010
 * @tc.name      : mp3 different bitrate
 * @tc.desc      : Function test
 */
HWTEST_F(InnerFunctionTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_010, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    ASSERT_NE(nullptr, decoderDemo);

    string decoderName = "OH.Media.Codec.Decoder.Audio.Mpeg";

    int32_t bitrateList[] = {40000, 128000, 320000};

    for (int i = 0; i < 3; i++)
    {
        Format format;
        int32_t bitrate = bitrateList[i];
        format.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 2);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 44100);
        format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, bitrate);

        string inputFile = "fltp_" + to_string((int)(bitrate/1000)) + "k_44100_2_dayuhaitang.mp3";
        string outputFile = "FUNCTION_010_44100_2_" + to_string(bitrate) + "_mp3_output.pcm";

        decoderDemo->InnerRunCase(inputFile, outputFile, decoderName.c_str(), format);
    }
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_011
 * @tc.name      : flac different bitrate
 * @tc.desc      : Function test
 */
HWTEST_F(InnerFunctionTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_011, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    ASSERT_NE(nullptr, decoderDemo);

    string decoderName = "OH.Media.Codec.Decoder.Audio.Flac";

    int32_t bitrateList[] = {195000, 780000};
    int32_t sample_rate_List[] = {8000, 44100};
    string fileList[] = {"s16_8000_2_dayuhaitang.flac", "s16_44100_2_dayuhaitang.flac"};
    for (int i = 0; i < 2; i++)
    {
        Format format;
        int32_t bitrate = bitrateList[i];
        int32_t sample_rate = sample_rate_List[i];
        format.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 2);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, sample_rate);
        format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, bitrate);
        format.PutIntValue("bits_per_coded_sample", 24);

        string inputFile = fileList[i];
        string outputFile = "FUNCTION_011_"+ to_string(sample_rate) +"_2_" + to_string(bitrate) + "_flac_output.pcm";

        decoderDemo->InnerRunCase(inputFile, outputFile, decoderName.c_str(), format);
    }
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_012
 * @tc.name      : MP3 different sample_format
 * @tc.desc      : Function test
 */
HWTEST_F(InnerFunctionTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_012, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    ASSERT_NE(nullptr, decoderDemo);

    string decoderName = "OH.Media.Codec.Decoder.Audio.Mpeg";

    string SampleFormatList[] = {"fltp", "s16p"};
    for (int j = 0; j < 2; j++) {
        Format format;
        int32_t sample_rate = 48000;
        string SampleFormat = SampleFormatList[j];
        format.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 2);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, sample_rate);
        format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 128000);

        string inputFile = SampleFormat + "_128k_"+to_string(sample_rate) + "_2_dayuhaitang.mp3";
        string outputFile = "FUNCTION_012_" + SampleFormat + to_string(sample_rate) + "_2_" + "_mp3_output.pcm";

        decoderDemo->InnerRunCase(inputFile, outputFile, decoderName.c_str(), format);
    }

    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_013
 * @tc.name      : flac different sample_format
 * @tc.desc      : Function test
 */
HWTEST_F(InnerFunctionTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_013, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    ASSERT_NE(nullptr, decoderDemo);

    string decoderName = "OH.Media.Codec.Decoder.Audio.Flac";

    string SampleFormatList[] = {"s32", "s16"};

    for (int j = 0; j < 2; j++) {
        Format format;
        int32_t sample_rate = 48000;
        string SampleFormat = SampleFormatList[j];
        format.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 2);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, sample_rate);
        format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 128000);
        format.PutIntValue("bits_per_coded_sample", 24);

        string inputFile = SampleFormat + "_"+to_string(sample_rate) + "_2_dayuhaitang.flac";
        string outputFile = "FUNCTION_013_" + SampleFormat + to_string(sample_rate) + "_2_" + "_flac_output.pcm";

        decoderDemo->InnerRunCase(inputFile, outputFile, decoderName.c_str(), format);
    }
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_014
 * @tc.name      : aac flush
 * @tc.desc      : Function test
 */
HWTEST_F(InnerFunctionTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_014, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    string decoderName = "OH.Media.Codec.Decoder.Audio.AAC";

    string inputFile = "fltp_128k_16000_1_dayuhaitang.aac";

    Format format;
    format.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 16000);
    format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 128000);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_AAC_IS_ADTS, 1);

    string firstOutputFile = "FUNCTION_014_16000_1_aac_output_1_flush.pcm";
    string secondOutputFile = "FUNCTION_014_16000_1_aac_output_2_flush.pcm";

    decoderDemo->InnerRunCaseFlush(inputFile, firstOutputFile, secondOutputFile, decoderName.c_str(), format);

    bool isSame = compareFile(firstOutputFile, secondOutputFile);
    ASSERT_EQ(true, isSame);

    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_015
 * @tc.name      : MP3 flush
 * @tc.desc      : Function test
 */
HWTEST_F(InnerFunctionTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_015, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    string decoderName = "OH.Media.Codec.Decoder.Audio.Mpeg";

    string inputFile = "fltp_128k_44100_2_dayuhaitang.mp3";
    string firstOutputFile = "FUNCTION_015_1_flush.pcm";
    string secondOutputFile = "FUNCTION_015_2_flush.pcm";

    Format format;
    format.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 2);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 44100);
    format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 128000);

    decoderDemo->InnerRunCaseFlush(inputFile, firstOutputFile, secondOutputFile, decoderName.c_str(), format);
    bool isSame = compareFile(firstOutputFile, secondOutputFile);
    ASSERT_EQ(true, isSame);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_016
 * @tc.name      : Flac flush
 * @tc.desc      : Function test
 */
HWTEST_F(InnerFunctionTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_016, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    string decoderName = "OH.Media.Codec.Decoder.Audio.Flac";

    string inputFile = "s16_8000_2_dayuhaitang.flac";
    string firstOutputFile = "FUNCTION_016_1_flush.pcm";
    string secondOutputFile = "FUNCTION_016_2_flush.pcm";

    Format format;
    format.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 2);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 8000);
    format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 40000);
    format.PutIntValue("bits_per_coded_sample", 24);

    decoderDemo->InnerRunCaseFlush(inputFile, firstOutputFile, secondOutputFile, decoderName.c_str(), format);
    bool isSame = compareFile(firstOutputFile, secondOutputFile);
    ASSERT_EQ(true, isSame);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_017
 * @tc.name      : Vorbis flush
 * @tc.desc      : Function test
 */
HWTEST_F(InnerFunctionTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_017, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    string decoderName = "OH.Media.Codec.Decoder.Audio.Vorbis";

    string inputFile = "fltp_45k_8000_2_dayuhaitang.ogg";
    string firstOutputFile = "FUNCTION_017_1_flush.pcm";
    string secondOutputFile = "FUNCTION_017_2_flush.pcm";

    Format format;
    format.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 2);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 8000);
    format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 45000);

    decoderDemo->InnerRunCaseFlush(inputFile, firstOutputFile, secondOutputFile, decoderName.c_str(), format);
    bool isSame = compareFile(firstOutputFile, secondOutputFile);
    ASSERT_EQ(true, isSame);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_018
 * @tc.name      : aac reset
 * @tc.desc      : Function test
 */
HWTEST_F(InnerFunctionTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_018, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    string decoderName = "OH.Media.Codec.Decoder.Audio.AAC";

    Format format;
    format.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 16000);
    format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 128000);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_AAC_IS_ADTS, 1);

    string inputFile = "fltp_128k_16000_1_dayuhaitang.aac";
    string firstOutputFile = "FUNCTION_018_16000_1_aac_output_1_reset.pcm";
    string secondOutputFile = "FUNCTION_018_16000_1_aac_output_2_reset.pcm";

    decoderDemo->InnerRunCaseReset(inputFile, firstOutputFile, secondOutputFile, decoderName.c_str(), format);
    bool isSame = compareFile(firstOutputFile, secondOutputFile);
    ASSERT_EQ(true, isSame);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_019
 * @tc.name      : mp3 reset
 * @tc.desc      : Function test
 */
HWTEST_F(InnerFunctionTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_019, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    string decoderName = "OH.Media.Codec.Decoder.Audio.Mpeg";

    string inputFile = "fltp_128k_44100_2_dayuhaitang.mp3";
    string firstOutputFile = "FUNCTION_019_1_reset.pcm";
    string secondOutputFile = "FUNCTION_019_2_reset.pcm";

    Format format;
    format.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 2);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 44100);
    format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 128000);

    decoderDemo->InnerRunCaseReset(inputFile, firstOutputFile, secondOutputFile, decoderName.c_str(), format);
    bool isSame = compareFile(firstOutputFile, secondOutputFile);
    ASSERT_EQ(true, isSame);

    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_020
 * @tc.name      : Flac reset
 * @tc.desc      : Function test
 */
HWTEST_F(InnerFunctionTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_020, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    string decoderName = "OH.Media.Codec.Decoder.Audio.Flac";

    string inputFile = "s16_44100_2_dayuhaitang.flac";
    string firstOutputFile = "FUNCTION_020_1_reset.pcm";
    string secondOutputFile = "FUNCTION_020_2_reset.pcm";

    Format format;
    format.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 2);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 44100);
    format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 128000);
    format.PutIntValue("bits_per_coded_sample", 24);
    
    decoderDemo->InnerRunCaseReset(inputFile, firstOutputFile, secondOutputFile, decoderName.c_str(), format);

    bool isSame = compareFile(firstOutputFile, secondOutputFile);
    ASSERT_EQ(true, isSame);

    delete decoderDemo;
}
/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_021
 * @tc.name      : Vorbis reset
 * @tc.desc      : Function test
 */
HWTEST_F(InnerFunctionTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_021, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    string decoderName = "OH.Media.Codec.Decoder.Audio.Vorbis";

    string inputFile = "fltp_128k_48000_1_dayuhaitang.ogg";
    string firstOutputFile = "FUNCTION_021_1.pcm";
    string secondOutputFile = "FUNCTION_021_2.pcm";

    Format format;
    format.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 48000);
    format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 128000);

    decoderDemo->InnerRunCaseReset(inputFile, firstOutputFile, secondOutputFile, decoderName.c_str(), format);

    bool isSame = compareFile(firstOutputFile, secondOutputFile);
    ASSERT_EQ(true, isSame);

    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_022
 * @tc.name      : AAC(thread)
 * @tc.desc      : Function test
 */
HWTEST_F(InnerFunctionTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_022, TestSize.Level2)
{
    vector<thread> threadVec;
    string decoderName = "OH.Media.Codec.Decoder.Audio.AAC";

    string inputFile = "fltp_128k_16000_1_dayuhaitang.aac";

    for (int32_t i = 0; i < 16; i++)
    {
        string outputFile = "FUNCTION_022_" + to_string(i) + ".pcm";
        threadVec.push_back(thread(runDecode, decoderName, inputFile, outputFile, i));
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
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_023
 * @tc.name      : MP3(thread)
 * @tc.desc      : Function test
 */
HWTEST_F(InnerFunctionTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_023, TestSize.Level2)
{
    vector<thread> threadVec;
    string decoderName = "OH.Media.Codec.Decoder.Audio.Mpeg";

    string inputFile = "fltp_128k_44100_2_dayuhaitang.mp3";

    for (int32_t i = 0; i < 16; i++)
    {
        string outputFile = "FUNCTION_023_" + to_string(i) + ".pcm";
        threadVec.push_back(thread(runDecode, decoderName, inputFile, outputFile, i));
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
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_024
 * @tc.name      : Flac(thread)
 * @tc.desc      : Function test
 */
HWTEST_F(InnerFunctionTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_024, TestSize.Level2)
{
    vector<thread> threadVec;
    string decoderName = "OH.Media.Codec.Decoder.Audio.Flac";

    string inputFile = "s16_44100_2_dayuhaitang.flac";

    for (int32_t i = 0; i < 16; i++)
    {
        string outputFile = "FUNCTION_024_" + to_string(i) + ".pcm";
        threadVec.push_back(thread(runDecode, decoderName, inputFile, outputFile, i));
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
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_025
 * @tc.name      : Vorbis(thread)
 * @tc.desc      : Function test
 */
HWTEST_F(InnerFunctionTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_025, TestSize.Level2)
{
    vector<thread> threadVec;
    string decoderName = "OH.Media.Codec.Decoder.Audio.Vorbis";

    string inputFile = "fltp_45k_48000_2_dayuhaitang.ogg";

    for (int32_t i = 0; i < 16; i++)
    {
        string outputFile = "FUNCTION_025_" + to_string(i) + ".pcm";
        threadVec.push_back(thread(runDecode, decoderName, inputFile, outputFile, i));
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