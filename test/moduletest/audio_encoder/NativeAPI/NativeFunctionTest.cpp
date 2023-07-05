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
    class NativeFunctionTest : public testing::Test {
    public:
        static void SetUpTestCase();
        static void TearDownTestCase();
        void SetUp() override;
        void TearDown() override;
    };

    void NativeFunctionTest::SetUpTestCase() {}
    void NativeFunctionTest::TearDownTestCase() {}
    void NativeFunctionTest::SetUp() {}
    void NativeFunctionTest::TearDown() {}

    constexpr uint32_t DEFAULT_AAC_TYPE = 1;
    constexpr int32_t CHANNEL_COUNT_AAC = 2;
    constexpr int32_t SAMPLE_RATE_AAC = 16000;
    constexpr int64_t BITS_RATE_AAC = 129000;
    constexpr int32_t CHANNEL_COUNT_FLAC = 2;
    constexpr int32_t SAMPLE_RATE_FLAC = 48000;
    constexpr int64_t BITS_RATE_FLAC = 128000;
    constexpr int32_t COMPLIANCE_LEVEL = -2;

    constexpr int32_t CHANNEL_COUNT_MONO = 1;
    constexpr int32_t CHANNEL_COUNT_STEREO = 2;
    constexpr int32_t CHANNEL_COUNT_7POINT1 = 8;

    int32_t testResult[16] = { -1 };

    vector<string> SplitStringFully(const string& str, const string& separator)
    {
        vector<string> dest;
        string substring;
        string::size_type start = 0;
        string::size_type index = str.find_first_of(separator, start);

        while (index != string::npos) {
            substring = str.substr(start, index - start);
            dest.push_back(substring);
            start = str.find_first_not_of(separator, index);
            if (start == string::npos) {
                return dest;
            }

            index = str.find_first_of(separator, start);
        }
        substring = str.substr(start);
        dest.push_back(substring);

        return dest;
    }

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

    OH_AVFormat* getAVFormatByEncoder(string encoderName)
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
        }
        return format;
    }

    void runEncode(string encoderName, string inputFile, string outputFile, int32_t threadId)
    {
        AudioEncoderDemo* encoderDemo = new AudioEncoderDemo();

        OH_AVFormat* format = getAVFormatByEncoder(encoderName);

        encoderDemo->NativeRunCase(inputFile, outputFile, encoderName.c_str(), format);
        
        OH_AVFormat_Destroy(format);
        delete encoderDemo;

        testResult[threadId] = AV_ERR_OK;
    }

    OH_AVFormat* getAVFormatAAC(int32_t channelCount, int32_t sampleRate, int64_t bitrate)
    {
        OH_AVFormat* format = OH_AVFormat_Create();
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, channelCount);
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, sampleRate);
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, OH_BitsPerSample::SAMPLE_F32LE);
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, OH_BitsPerSample::SAMPLE_F32LE);

        switch (channelCount) {
            case CHANNEL_COUNT_MONO:
                OH_AVFormat_SetLongValue(format, OH_MD_KEY_CHANNEL_LAYOUT, MONO);
                break;
            case CHANNEL_COUNT_STEREO:
                OH_AVFormat_SetLongValue(format, OH_MD_KEY_CHANNEL_LAYOUT, STEREO);
                break;
            case CHANNEL_COUNT_7POINT1:
                OH_AVFormat_SetLongValue(format, OH_MD_KEY_CHANNEL_LAYOUT, CH_7POINT1);
                break;
            default:
                OH_AVFormat_SetLongValue(format, OH_MD_KEY_CHANNEL_LAYOUT, UNKNOWN_CHANNEL_LAYOUT);
                break;
        }

        OH_AVFormat_SetIntValue(format, OH_MD_KEY_AAC_IS_ADTS, DEFAULT_AAC_TYPE);
        OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, bitrate);

        return format;
    }


    OH_AVFormat* getAVFormatFlac(int32_t channelCount, int32_t sampleRate, int32_t sampleFormat, int64_t bitrate)
    {
        OH_AVFormat* format = OH_AVFormat_Create();
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, channelCount);
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, sampleRate);
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, sampleFormat);
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, sampleFormat);

        int32_t bitsPerSample;
        if (sampleFormat == OH_BitsPerSample::SAMPLE_S16LE) {
            bitsPerSample = S16_BITS_PER_SAMPLE;
        } else {
            bitsPerSample = S32_BITS_PER_SAMPLE;
        }

        int32_t inputBufSize;
        switch (channelCount) {
            case CHANNEL_COUNT_MONO:
                OH_AVFormat_SetLongValue(format, OH_MD_KEY_CHANNEL_LAYOUT, MONO);
                inputBufSize = COMMON_FLAC_NUM * channelCount * bitsPerSample;
                break;
            case CHANNEL_COUNT_STEREO:
                OH_AVFormat_SetLongValue(format, OH_MD_KEY_CHANNEL_LAYOUT, STEREO);
                inputBufSize = COMMON_FLAC_NUM * channelCount * bitsPerSample;
                break;
            case CHANNEL_COUNT_7POINT1:
                OH_AVFormat_SetLongValue(format, OH_MD_KEY_CHANNEL_LAYOUT, CH_7POINT1);
                inputBufSize = COMMON_FLAC_NUM * channelCount * bitsPerSample;
                break;
            default:
                OH_AVFormat_SetLongValue(format, OH_MD_KEY_CHANNEL_LAYOUT, UNKNOWN_CHANNEL_LAYOUT);
                inputBufSize = COMMON_FLAC_NUM * UNKNOWN_CHANNEL * bitsPerSample;
                break;
        }

        OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, bitrate);
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_COMPLIANCE_LEVEL, COMPLIANCE_LEVEL);
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_MAX_INPUT_SIZE, inputBufSize);

        return format;
    }
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_FUNCTION_001
 * @tc.name      : aac(different sample rate)
 * @tc.desc      : function check
 */
HWTEST_F(NativeFunctionTest, SUB_MULTIMEDIA_AUDIO_ENCODER_FUNCTION_001, TestSize.Level2)
{
    AudioEncoderDemo* encoderDemo = new AudioEncoderDemo();
    string encoderName = "OH.Media.Codec.Encoder.Audio.AAC";

    string fileList[] = {
        "f32le_7350_2_dayuhaitang.pcm",
        "f32le_16000_2_dayuhaitang.pcm",
        "f32le_44100_2_dayuhaitang.pcm",
        "f32le_48000_2_dayuhaitang.pcm",
        "f32le_96000_2_dayuhaitang.pcm"
    };

    for (int i = 0; i < 5; i++)
    {
        vector<string> dest = SplitStringFully(fileList[i], "_");
        if (dest.size() < 4) {
            cout << "split error !!!" << endl;
            return;
        }
        int32_t channelCount = stoi(dest[2]);
        int32_t sampleRate = stoi(dest[1]);

        cout << "channel count is " << channelCount << ", sample rate is " << sampleRate << endl;

        OH_AVFormat* format = getAVFormatAAC(channelCount, sampleRate, BITS_RATE_AAC);
        ASSERT_NE(nullptr, format);

        string inputFile = fileList[i];
        string outputFile = "FUNCTION_001_" + to_string(sampleRate) + ".aac";

        encoderDemo->NativeRunCase(inputFile, outputFile, encoderName.c_str(), format);

        OH_AVFormat_Destroy(format);
    }
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_FUNCTION_002
 * @tc.name      : aac(different channel num)
 * @tc.desc      : function check
 */
HWTEST_F(NativeFunctionTest, SUB_MULTIMEDIA_AUDIO_ENCODER_FUNCTION_002, TestSize.Level2)
{
    AudioEncoderDemo* encoderDemo = new AudioEncoderDemo();
    string encoderName = "OH.Media.Codec.Encoder.Audio.AAC";

    string fileList[] = {
        "f32le_7350_1_dayuhaitang.pcm",
        "f32le_16000_2_dayuhaitang.pcm",
        "f32le_96000_8_dayuhaitang.pcm"
    };

    for (int i = 0; i < 3; i++)
    {
        vector<string> dest = SplitStringFully(fileList[i], "_");
        if (dest.size() < 4)
        {
            cout << "split error !!!" << endl;
            return;
        }
        int32_t channelCount = stoi(dest[2]);
        int32_t sampleRate = stoi(dest[1]);

        cout << "channel count is " << channelCount << ", sample rate is " << sampleRate << endl;

        OH_AVFormat* format = getAVFormatAAC(channelCount, sampleRate, BITS_RATE_AAC);
        ASSERT_NE(nullptr, format);

        string inputFile = fileList[i];
        string outputFile = "FUNCTION_002_" + to_string(channelCount) + ".aac";

        encoderDemo->NativeRunCase(inputFile, outputFile, encoderName.c_str(), format);

        OH_AVFormat_Destroy(format);
    }
    delete encoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_FUNCTION_003
 * @tc.name      : aac(different bitrate)
 * @tc.desc      : function check
 */
HWTEST_F(NativeFunctionTest, SUB_MULTIMEDIA_AUDIO_ENCODER_FUNCTION_003, TestSize.Level2)
{
    AudioEncoderDemo* encoderDemo = new AudioEncoderDemo();
    string encoderName = "OH.Media.Codec.Encoder.Audio.AAC";

    long bitrateList[] = {32000L, 96000L, 128000L, 256000L, 300000L, 500000L};
    string inputFile = "f32le_96000_2_dayuhaitang.pcm";

    for (int i = 0; i < 6; i++)
    {
        vector<string> dest = SplitStringFully(inputFile, "_");
        if (dest.size() < 4)
        {
            cout << "split error !!!" << endl;
            return;
        }
        int32_t channelCount = stoi(dest[2]);
        int32_t sampleRate = stoi(dest[1]);

        cout << "channel count is " << channelCount << ", sample rate is " << sampleRate << endl;

        OH_AVFormat* format = getAVFormatAAC(channelCount, sampleRate, bitrateList[i]);
        ASSERT_NE(nullptr, format);

        string outputFile = "FUNCTION_003_" + to_string(bitrateList[i]) + ".aac";

        encoderDemo->NativeRunCase(inputFile, outputFile, encoderName.c_str(), format);

        OH_AVFormat_Destroy(format);
    }
    delete encoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_FUNCTION_004
 * @tc.name      : flac(different sample rate)
 * @tc.desc      : function check
 */
HWTEST_F(NativeFunctionTest, SUB_MULTIMEDIA_AUDIO_ENCODER_FUNCTION_004, TestSize.Level2)
{
    AudioEncoderDemo* encoderDemo = new AudioEncoderDemo();
    string encoderName = "OH.Media.Codec.Encoder.Audio.Flac";

    string fileList[] = {
        "s16_8000_2_dayuhaitang.pcm",
        "s16_16000_2_dayuhaitang.pcm",
        "s16_44100_2_dayuhaitang.pcm",
        "s16_48000_2_dayuhaitang.pcm",
        "s16_96000_2_dayuhaitang.pcm"
    };

    for (int i = 0; i < 5; i++)
    {
        vector<string> dest = SplitStringFully(fileList[i], "_");
        if (dest.size() < 4)
        {
            cout << "split error !!!" << endl;
            return;
        }
        int32_t channelCount = stoi(dest[2]);
        int32_t sampleRate = stoi(dest[1]);

        OH_AVFormat* format = getAVFormatFlac(channelCount, sampleRate, OH_BitsPerSample::SAMPLE_S16LE, BITS_RATE_FLAC);
        ASSERT_NE(nullptr, format);

        string inputFile = fileList[i];
        string outputFile = "FUNCTION_004_" + to_string(sampleRate) + ".flac";

        encoderDemo->NativeRunCase(inputFile, outputFile, encoderName.c_str(), format);

        OH_AVFormat_Destroy(format);
    }
    delete encoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_FUNCTION_005
 * @tc.name      : flac(different sample format)
 * @tc.desc      : function check
 */
HWTEST_F(NativeFunctionTest, SUB_MULTIMEDIA_AUDIO_ENCODER_FUNCTION_005, TestSize.Level2)
{
    AudioEncoderDemo* encoderDemo = new AudioEncoderDemo();
    string encoderName = "OH.Media.Codec.Encoder.Audio.Flac";

    string fileList[] = {
        "s32_8000_2_dayuhaitang.pcm",
        "s32_16000_2_dayuhaitang.pcm",
        "s32_44100_2_dayuhaitang.pcm",
        "s32_48000_2_dayuhaitang.pcm",
        "s32_96000_2_dayuhaitang.pcm"
    };

    long bitrate = 500000;

    for (int i = 0; i < 5; i++)
    {
        vector<string> dest = SplitStringFully(fileList[i], "_");
        if (dest.size() < 4) {
            cout << "split error !!!" << endl;
            return;
        }
        int32_t channelCount = stoi(dest[2]);
        int32_t sampleRate = stoi(dest[1]);

        int32_t sampleFormat;
        if (dest[0] == "s16") {
            sampleFormat = OH_BitsPerSample::SAMPLE_S16LE;
        } else {
            sampleFormat = OH_BitsPerSample::SAMPLE_S32LE;
        }

        OH_AVFormat* format = getAVFormatFlac(channelCount, sampleRate, sampleFormat, bitrate);
        ASSERT_NE(nullptr, format);

        string inputFile = fileList[i];
        string outputFile = "FUNCTION_005_" + dest[0] + "_" + to_string(sampleRate) + ".flac";

        encoderDemo->NativeRunCase(inputFile, outputFile, encoderName.c_str(), format);

        OH_AVFormat_Destroy(format);
    }
    delete encoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_FUNCTION_006
 * @tc.name      : flac(different channel num)
 * @tc.desc      : function check
 */
HWTEST_F(NativeFunctionTest, SUB_MULTIMEDIA_AUDIO_ENCODER_FUNCTION_006, TestSize.Level2)
{
    AudioEncoderDemo* encoderDemo = new AudioEncoderDemo();
    string encoderName = "OH.Media.Codec.Encoder.Audio.Flac";

    string fileList[] = {
        "s16_8000_1_dayuhaitang.pcm",
        "s16_48000_1_dayuhaitang.pcm",
        "s16_48000_2_dayuhaitang.pcm",
        "s16_48000_8_dayuhaitang.pcm"
    };

    long bitrate = 500000;

    for (int i = 0; i < 4; i++)
    {
        vector<string> dest = SplitStringFully(fileList[i], "_");
        if (dest.size() < 4)
        {
            cout << "split error !!!" << endl;
            return;
        }
        int32_t channelCount = stoi(dest[2]);
        int32_t sampleRate = stoi(dest[1]);

        int32_t sampleFormat;
        if (dest[0] == "s16") {
            sampleFormat = OH_BitsPerSample::SAMPLE_S16LE;
        } else {
            sampleFormat = OH_BitsPerSample::SAMPLE_S32LE;
        }

        OH_AVFormat* format = getAVFormatFlac(channelCount, sampleRate, sampleFormat, bitrate);
        ASSERT_NE(nullptr, format);

        string inputFile = fileList[i];
        string outputFile = "FUNCTION_006_" + dest[0] + "_" + to_string(channelCount) + ".flac";

        encoderDemo->NativeRunCase(inputFile, outputFile, encoderName.c_str(), format);

        OH_AVFormat_Destroy(format);
    }
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_FUNCTION_007
 * @tc.name      : Flush(AAC)
 * @tc.desc      : function check
 */
HWTEST_F(NativeFunctionTest, SUB_MULTIMEDIA_AUDIO_ENCODER_FUNCTION_007, TestSize.Level2)
{
    AudioEncoderDemo* encoderDemo = new AudioEncoderDemo();
    string decoderName = "OH.Media.Codec.Encoder.Audio.AAC";

    string inputFile = "f32le_16000_2_dayuhaitang.pcm";
    string firstOutputFile = "FUNCTION_007_1.aac";
    string secondOutputFile = "FUNCTION_007_2.aac";

    OH_AVFormat* format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 2);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 16000);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, OH_BitsPerSample::SAMPLE_F32LE);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, OH_BitsPerSample::SAMPLE_F32LE);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_CHANNEL_LAYOUT, STEREO);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AAC_IS_ADTS, DEFAULT_AAC_TYPE);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 129000);

    encoderDemo->NativeRunCaseFlush(inputFile, firstOutputFile, secondOutputFile, decoderName.c_str(), format);

    bool isSame = compareFile(firstOutputFile, secondOutputFile);
    ASSERT_EQ(true, isSame);

    OH_AVFormat_Destroy(format);
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_FUNCTION_008
 * @tc.name      : Flush(flac)
 * @tc.desc      : function check
 */
HWTEST_F(NativeFunctionTest, SUB_MULTIMEDIA_AUDIO_ENCODER_FUNCTION_008, TestSize.Level2)
{
    AudioEncoderDemo* encoderDemo = new AudioEncoderDemo();
    string decoderName = "OH.Media.Codec.Encoder.Audio.Flac";

    string inputFile = "s16_48000_2_dayuhaitang.pcm";
    string firstOutputFile = "FUNCTION_008_1.flac";
    string secondOutputFile = "FUNCTION_008_2.flac";

    OH_AVFormat* format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 2);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 48000);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, OH_BitsPerSample::SAMPLE_S16LE);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, OH_BitsPerSample::SAMPLE_S16LE);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_CHANNEL_LAYOUT, STEREO);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AAC_IS_ADTS, DEFAULT_AAC_TYPE);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 129000);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_MAX_INPUT_SIZE,
        COMMON_FLAC_NUM * CHANNEL_COUNT_STEREO * S16_BITS_PER_SAMPLE);

    encoderDemo->NativeRunCaseFlush(inputFile, firstOutputFile, secondOutputFile, decoderName.c_str(), format);

    bool isSame = compareFile(firstOutputFile, secondOutputFile);
    ASSERT_EQ(true, isSame);

    OH_AVFormat_Destroy(format);
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_FUNCTION_009
 * @tc.name      : Reset(AAC)
 * @tc.desc      : function check
 */
HWTEST_F(NativeFunctionTest, SUB_MULTIMEDIA_AUDIO_ENCODER_FUNCTION_009, TestSize.Level2)
{
    AudioEncoderDemo* encoderDemo = new AudioEncoderDemo();
    string decoderName = "OH.Media.Codec.Encoder.Audio.AAC";

    string inputFile = "f32le_16000_2_dayuhaitang.pcm";
    string firstOutputFile = "FUNCTION_009_1.aac";
    string secondOutputFile = "FUNCTION_009_2.aac";

    OH_AVFormat* format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 2);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 16000);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, OH_BitsPerSample::SAMPLE_F32LE);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, OH_BitsPerSample::SAMPLE_F32LE);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_CHANNEL_LAYOUT, STEREO);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AAC_IS_ADTS, DEFAULT_AAC_TYPE);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 129000);

    encoderDemo->NativeRunCaseReset(inputFile, firstOutputFile, secondOutputFile, decoderName.c_str(), format);

    bool isSame = compareFile(firstOutputFile, secondOutputFile);
    ASSERT_EQ(true, isSame);

    OH_AVFormat_Destroy(format);
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_FUNCTION_010
 * @tc.name      : Reset(flac)
 * @tc.desc      : function check
 */
HWTEST_F(NativeFunctionTest, SUB_MULTIMEDIA_AUDIO_ENCODER_FUNCTION_010, TestSize.Level2)
{
    AudioEncoderDemo* encoderDemo = new AudioEncoderDemo();
    string decoderName = "OH.Media.Codec.Encoder.Audio.Flac";

    string inputFile = "s16_48000_2_dayuhaitang.pcm";
    string firstOutputFile = "FUNCTION_010_1.flac";
    string secondOutputFile = "FUNCTION_010_2.flac";

    OH_AVFormat* format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 2);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 48000);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, OH_BitsPerSample::SAMPLE_S16LE);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, OH_BitsPerSample::SAMPLE_S16LE);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_CHANNEL_LAYOUT, STEREO);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AAC_IS_ADTS, DEFAULT_AAC_TYPE);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 129000);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_MAX_INPUT_SIZE,
        COMMON_FLAC_NUM * CHANNEL_COUNT_STEREO * S16_BITS_PER_SAMPLE);

    encoderDemo->NativeRunCaseReset(inputFile, firstOutputFile, secondOutputFile, decoderName.c_str(), format);

    bool isSame = compareFile(firstOutputFile, secondOutputFile);
    ASSERT_EQ(true, isSame);

    OH_AVFormat_Destroy(format);
    delete encoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_FUNCTION_011
 * @tc.name      : OH_AudioEncoder_GetOutputDescription
 * @tc.desc      : function check
 */
HWTEST_F(NativeFunctionTest, SUB_MULTIMEDIA_AUDIO_ENCODER_FUNCTION_011, TestSize.Level2)
{
    AudioEncoderDemo* encoderDemo = new AudioEncoderDemo();
    string encoderName = "OH.Media.Codec.Encoder.Audio.AAC";

    string inputFile = "f32le_16000_2_dayuhaitang.pcm";
    string outputFile = "FUNCTION_011.aac";

    OH_AVFormat* format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 2);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 16000);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, OH_BitsPerSample::SAMPLE_F32LE);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, OH_BitsPerSample::SAMPLE_F32LE);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_CHANNEL_LAYOUT, STEREO);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AAC_IS_ADTS, DEFAULT_AAC_TYPE);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 129000);

    OH_AVFormat* formatGet = encoderDemo->NativeRunCaseGetOutputDescription(inputFile, outputFile, encoderName.c_str(),
        format);
    ASSERT_NE(nullptr, formatGet);

    int32_t channelNum_Get;
    int32_t sampleRate_Get;
    int64_t bitrate_Get;
    OH_AVFormat_GetIntValue(formatGet, OH_MD_KEY_AUD_CHANNEL_COUNT, &channelNum_Get);
    OH_AVFormat_GetIntValue(formatGet, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate_Get);
    OH_AVFormat_GetLongValue(formatGet, OH_MD_KEY_BITRATE, &bitrate_Get);

    const char* testStr = nullptr;
    OH_AVFormat_GetStringValue(formatGet, OH_MD_KEY_CODEC_MIME, &testStr);

    cout << "channel num is " << channelNum_Get << ", sample rate is " <<
    sampleRate_Get << ", bitrate is " << bitrate_Get << endl;

    cout << "OH_MD_KEY_CODEC_MIME is " << testStr << endl;

    ASSERT_EQ(2, channelNum_Get);
    ASSERT_EQ(16000, sampleRate_Get);
    ASSERT_EQ(129000, bitrate_Get);

    OH_AVFormat_Destroy(format);
    OH_AVFormat_Destroy(formatGet);
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_FUNCTION_012
 * @tc.name      : AAC(thread)
 * @tc.desc      : Function test
 */
HWTEST_F(NativeFunctionTest, SUB_MULTIMEDIA_AUDIO_ENCODER_FUNCTION_012, TestSize.Level2)
{
    vector<thread> threadVec;
    string encoderName = "OH.Media.Codec.Encoder.Audio.AAC";

    string inputFile = "f32le_16000_2_dayuhaitang.pcm";

    for (int32_t i = 0; i < 16; i++)
    {
        string outputFile = "FUNCTION_012_" + to_string(i) + ".aac";
        threadVec.push_back(thread(runEncode, encoderName, inputFile, outputFile, i));
    }
    for (uint32_t i = 0; i < threadVec.size(); i++)
    {
        threadVec[i].join();
    }
    for (int32_t i = 0; i < 16; i++)
    {
        ASSERT_EQ(AV_ERR_OK, testResult[i]);
    }
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_FUNCTION_013
 * @tc.name      : flac(thread)
 * @tc.desc      : Function test
 */
HWTEST_F(NativeFunctionTest, SUB_MULTIMEDIA_AUDIO_ENCODER_FUNCTION_013, TestSize.Level2)
{
    vector<thread> threadVec;
    string encoderName = "OH.Media.Codec.Encoder.Audio.Flac";

    string inputFile = "s16_48000_2_dayuhaitang.pcm";

    for (int32_t i = 0; i < 16; i++)
    {
        string outputFile = "FUNCTION_013_" + to_string(i) + ".flac";
        threadVec.push_back(thread(runEncode, encoderName, inputFile, outputFile, i));
    }
    for (uint32_t i = 0; i < threadVec.size(); i++)
    {
        threadVec[i].join();
    }
    for (int32_t i = 0; i < 16; i++)
    {
        ASSERT_EQ(AV_ERR_OK, testResult[i]);
    }
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_FUNCTION_100
 * @tc.name      : AAC
 * @tc.desc      : function check
 */
HWTEST_F(NativeFunctionTest, SUB_MULTIMEDIA_AUDIO_ENCODER_FUNCTION_100, TestSize.Level2)
{
    AudioEncoderDemo* encoderDemo = new AudioEncoderDemo();
    string encoderName = "OH.Media.Codec.Encoder.Audio.AAC";

    string inputFile = "B_Bird_on_a_wire_1_f32.pcm";
    string outputFile = "FUNCTION_100.aac";

    OH_AVFormat* format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 1);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 48000);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, OH_BitsPerSample::SAMPLE_F32LE);

    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, OH_BitsPerSample::SAMPLE_F32LE);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_CHANNEL_LAYOUT, MONO);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AAC_IS_ADTS, DEFAULT_AAC_TYPE);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 128000);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_MAX_INPUT_SIZE, 48000);
    ASSERT_NE(nullptr, format);

    encoderDemo->NativeRunCasePerformance(inputFile, outputFile, encoderName.c_str(), format);

    OH_AVFormat_Destroy(format);
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_FUNCTION_200
 * @tc.name      : FLAC
 * @tc.desc      : function check
 */
HWTEST_F(NativeFunctionTest, SUB_MULTIMEDIA_AUDIO_ENCODER_FUNCTION_200, TestSize.Level2)
{
    AudioEncoderDemo* encoderDemo = new AudioEncoderDemo();
    string encoderName = "OH.Media.Codec.Encoder.Audio.Flac";

    string inputFile = "B_Bird_on_a_wire_1_1.pcm";
    string outputFile = "FUNCTION_200.flac";

    OH_AVFormat* format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 1);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 48000);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, OH_BitsPerSample::SAMPLE_S16LE);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, OH_BitsPerSample::SAMPLE_S16LE);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_CHANNEL_LAYOUT, MONO);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 429000);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_COMPLIANCE_LEVEL, -2);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_MAX_INPUT_SIZE, 48000);
    ASSERT_NE(nullptr, format);

    encoderDemo->NativeRunCasePerformance(inputFile, outputFile, encoderName.c_str(), format);

    OH_AVFormat_Destroy(format);
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_FUNCTION_201
 * @tc.name      : FLAC
 * @tc.desc      : function check
 */
HWTEST_F(NativeFunctionTest, SUB_MULTIMEDIA_AUDIO_ENCODER_FUNCTION_201, TestSize.Level2)
{
    AudioEncoderDemo* encoderDemo = new AudioEncoderDemo();
    string encoderName = "OH.Media.Codec.Encoder.Audio.Flac";

    string inputFile = "free_loss.pcm";
    string outputFile = "FUNCTION_201.flac";

    OH_AVFormat* format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 1);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 48000);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, OH_BitsPerSample::SAMPLE_S16LE);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, OH_BitsPerSample::SAMPLE_S16LE);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_CHANNEL_LAYOUT, MONO);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 613000);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_COMPLIANCE_LEVEL, -2);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_MAX_INPUT_SIZE, 48000);
    ASSERT_NE(nullptr, format);

    encoderDemo->NativeRunCasePerformance(inputFile, outputFile, encoderName.c_str(), format);

    OH_AVFormat_Destroy(format);
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_FUNCTION_202
 * @tc.name      : FLAC
 * @tc.desc      : function check
 */
HWTEST_F(NativeFunctionTest, SUB_MULTIMEDIA_AUDIO_ENCODER_FUNCTION_202, TestSize.Level2)
{
    AudioEncoderDemo* encoderDemo = new AudioEncoderDemo();
    string encoderName = "OH.Media.Codec.Encoder.Audio.Flac";

    string inputFile = "B_Bird_on_a_wire_1_1.pcm";
    string outputFile = "FUNCTION_202.dat";

    OH_AVFormat* format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 1);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 48000);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, OH_BitsPerSample::SAMPLE_S16LE);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, OH_BitsPerSample::SAMPLE_S16LE);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_CHANNEL_LAYOUT, MONO);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 429000);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_COMPLIANCE_LEVEL, -2);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_MAX_INPUT_SIZE, 48000);
    ASSERT_NE(nullptr, format);

    encoderDemo->TestRunCase(inputFile, outputFile, encoderName.c_str(), format);

    OH_AVFormat_Destroy(format);
    delete encoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_ENCODER_FUNCTION_203
 * @tc.name      : FLAC
 * @tc.desc      : function check
 */
HWTEST_F(NativeFunctionTest, SUB_MULTIMEDIA_AUDIO_ENCODER_FUNCTION_203, TestSize.Level2)
{
    AudioEncoderDemo* encoderDemo = new AudioEncoderDemo();
    string encoderName = "OH.Media.Codec.Encoder.Audio.Flac";

    string inputFile = "free_loss.pcm";
    string outputFile = "FUNCTION_203.dat";

    OH_AVFormat* format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 1);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 96000);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, OH_BitsPerSample::SAMPLE_S16LE);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, OH_BitsPerSample::SAMPLE_S16LE);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_CHANNEL_LAYOUT, MONO);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 613000);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_COMPLIANCE_LEVEL, -2);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_MAX_INPUT_SIZE, 16384);
    ASSERT_NE(nullptr, format);

    encoderDemo->TestRunCase(inputFile, outputFile, encoderName.c_str(), format);

    OH_AVFormat_Destroy(format);
    delete encoderDemo;
}