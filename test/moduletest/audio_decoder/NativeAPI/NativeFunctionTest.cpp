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
#include <thread>
#include "gtest/gtest.h"
#include "AudioDecoderDemoCommon.h"


using namespace std;
using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;
constexpr int32_t SIZE_7 = 7;
constexpr int32_t SIZE_5 = 5;
constexpr int32_t SIZE_4 = 4;
constexpr int32_t INDEX_0 = 0;
constexpr int32_t INDEX_1 = 1;
constexpr int32_t INDEX_2 = 2;
constexpr int32_t INDEX_3 = 3;
constexpr int32_t INDEX_4 = 4;
constexpr int32_t INDEX_5 = 5;
constexpr int32_t AMRWB_CHANNEL_COUNT = 1;
constexpr int32_t AMRWB_SAMPLE_RATE = 16000;
constexpr int32_t AMRNB_CHANNEL_COUNT = 1;
constexpr int32_t AMRNB_SAMPLE_RATE = 8000;

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

    void string_replace(std::string& strBig, const std::string& strsrc, const std::string& strdst)
    {
        std::string::size_type pos = 0;
        std::string::size_type srclen = strsrc.size();
        std::string::size_type dstlen = strdst.size();

        while ((pos = strBig.find(strsrc, pos)) != std::string::npos) {
            strBig.replace(pos, srclen, strdst);
            pos += dstlen;
        }
    }

    bool compareFile(string file1, string file2)
    {
        string ans1, ans2;
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
        for (uint32_t i = 0; i < ans1.size(); i++) {
            if (ans1[i] != ans2[i]) {
                return false;
            }
        }
        return true;
    }

    void getParamsByName(string decoderName, string inputFile, int32_t& channelCount,
        int32_t& sampleRate, long& bitrate)
    {
        vector<string> dest = SplitStringFully(inputFile, "_");
        if (decoderName == "OH.Media.Codec.Decoder.Audio.AAC") {
            if (dest.size() < SIZE_7) {
                cout << "split error !!!" << endl;
                return;
            }
            channelCount = stoi(dest[INDEX_5]);
            sampleRate = stoi(dest[INDEX_4]);

            string bitStr = dest[INDEX_3];
            string_replace(bitStr, "k", "000");
            bitrate = atol(bitStr.c_str());
        } else if (decoderName == "OH.Media.Codec.Decoder.Audio.Mpeg") {
            if (dest.size() < SIZE_5) {
                cout << "split error !!!" << endl;
                return;
            }
            channelCount = stoi(dest[INDEX_3]);
            sampleRate = stoi(dest[INDEX_2]);

            string bitStr = dest[INDEX_1];
            string_replace(bitStr, "k", "000");
            bitrate = atol(bitStr.c_str());
        } else if (decoderName == "OH.Media.Codec.Decoder.Audio.Flac") {
            if (dest.size() < SIZE_4) {
                cout << "split error !!!" << endl;
                return;
            }
            channelCount = stoi(dest[INDEX_2]);
            sampleRate = stoi(dest[INDEX_1]);

            string bitStr = dest[INDEX_1];
            string_replace(bitStr, "k", "000");
            bitrate = atol(bitStr.c_str());
        } else {
            if (dest.size() < SIZE_5) {
                cout << "split error !!!" << endl;
                return;
            }
            channelCount = stoi(dest[INDEX_3]);
            sampleRate = stoi(dest[INDEX_2]);

            string bitStr = dest[INDEX_1];
            string_replace(bitStr, "k", "000");
            bitrate = atol(bitStr.c_str());
        }
    }

    void runDecode(string decoderName, string inputFile, string outputFile, int32_t threadId)
    {
        AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
        int32_t channelCount;
        int32_t sampleRate;
        long bitrate;
        getParamsByName(decoderName, inputFile, channelCount, sampleRate, bitrate);
        if (decoderName == "OH.Media.Codec.Decoder.Audio.AAC") {
            OH_AVFormat* format = OH_AVFormat_Create();
            OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, channelCount);
            OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, sampleRate);
            OH_AVFormat_SetIntValue(format, OH_MD_KEY_AAC_IS_ADTS, DEFAULT_AAC_TYPE);

            decoderDemo->NativeRunCase(inputFile, outputFile, decoderName.c_str(), format);
            OH_AVFormat_Destroy(format);
        } else if (decoderName == "OH.Media.Codec.Decoder.Audio.Mpeg") {
            OH_AVFormat* format = OH_AVFormat_Create();
            OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, channelCount);
            OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, sampleRate);

            decoderDemo->NativeRunCase(inputFile, outputFile, decoderName.c_str(), format);

            OH_AVFormat_Destroy(format);
        } else if (decoderName == "OH.Media.Codec.Decoder.Audio.Flac") {
            OH_AVFormat* format = OH_AVFormat_Create();
            OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, channelCount);
            OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, sampleRate);
            OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, OH_BitsPerSample::SAMPLE_S16P);
            OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, bitrate);

            decoderDemo->NativeRunCase(inputFile, outputFile, decoderName.c_str(), format);

            OH_AVFormat_Destroy(format);
        } else if (decoderName == "OH.Media.Codec.Decoder.Audio.Amrwb") {
            OH_AVFormat* format = OH_AVFormat_Create();
            OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, AMRWB_CHANNEL_COUNT);
            OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, AMRWB_SAMPLE_RATE);

            decoderDemo->NativeRunCase(inputFile, outputFile, decoderName.c_str(), format);

            OH_AVFormat_Destroy(format);
        } else if (decoderName == "OH.Media.Codec.Decoder.Audio.Amrnb") {
            OH_AVFormat* format = OH_AVFormat_Create();
            OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, AMRNB_CHANNEL_COUNT);
            OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, AMRNB_SAMPLE_RATE);

            decoderDemo->NativeRunCase(inputFile, outputFile, decoderName.c_str(), format);

            OH_AVFormat_Destroy(format);
        } else {
            OH_AVFormat* format = OH_AVFormat_Create();
            OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, channelCount);
            OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, sampleRate);

            decoderDemo->NativeRunCase(inputFile, outputFile, decoderName.c_str(), format);
            OH_AVFormat_Destroy(format);
        }
        testResult[threadId] = AV_ERR_OK;
        delete decoderDemo;
    }
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_001
 * @tc.name      : aac(different sample rate)
 * @tc.desc      : function check
 */
HWTEST_F(NativeFunctionTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_001, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    string decoderName = "OH.Media.Codec.Decoder.Audio.AAC";

    string fileList[] = {"fltp_aac_low_128k_7350_1_dayuhaitang.aac", "fltp_aac_low_128k_16000_1_dayuhaitang.aac",
        "fltp_aac_low_128k_44100_1_dayuhaitang.aac", "fltp_aac_low_128k_48000_1_dayuhaitang.aac",
        "fltp_aac_low_128k_96000_1_dayuhaitang.aac"};

    for (int i = 0; i < SIZE_5; i++)
    {
        vector<string> dest = SplitStringFully(fileList[i], "_");
        if (dest.size() < SIZE_7)
        {
            cout << "split error !!!" << endl;
            return;
        }
        int32_t channelCount = stoi(dest[INDEX_5]);
        int32_t sampleRate = stoi(dest[INDEX_4]);

        string bitStr = dest[INDEX_3];
        string_replace(bitStr, "k", "000");
        long bitrate = atol(bitStr.c_str());

        OH_AVFormat* format = OH_AVFormat_Create();
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, channelCount);
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, sampleRate);
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_AAC_IS_ADTS, DEFAULT_AAC_TYPE);
        OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, bitrate);
        ASSERT_NE(nullptr, format);

        string inputFile = fileList[i];
        string outputFile = "FUNCTION_001_" + to_string(sampleRate) + ".pcm";

        decoderDemo->NativeRunCase(inputFile, outputFile, decoderName.c_str(), format);

        OH_AVFormat_Destroy(format);
    }
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_002
 * @tc.name      : aac(different channel num)
 * @tc.desc      : function check
 */
HWTEST_F(NativeFunctionTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_002, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    string decoderName = "OH.Media.Codec.Decoder.Audio.AAC";

    string fileList[] = { "fltp_aac_low_128k_44100_1_dayuhaitang.aac", "fltp_aac_low_128k_44100_2_dayuhaitang.aac",
    "fltp_aac_low_128k_44100_8_dayuhaitang.aac" };

    for (int i = 0; i < 3; i++)
    {
        vector<string> dest = SplitStringFully(fileList[i], "_");
        if (dest.size() < SIZE_7)
        {
            cout << "split error !!!" << endl;
            return;
        }
        int32_t channelCount = stoi(dest[INDEX_5]);
        int32_t sampleRate = stoi(dest[INDEX_4]);

        string bitStr = dest[INDEX_3];
        string_replace(bitStr, "k", "000");
        long bitrate = atol(bitStr.c_str());

        OH_AVFormat* format = OH_AVFormat_Create();
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, channelCount);
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, sampleRate);
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_AAC_IS_ADTS, DEFAULT_AAC_TYPE);
        OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, bitrate);
        ASSERT_NE(nullptr, format);

        string inputFile = fileList[i];
        string outputFile = "FUNCTION_002_" + to_string(channelCount) + ".pcm";

        decoderDemo->NativeRunCase(inputFile, outputFile, decoderName.c_str(), format);

        OH_AVFormat_Destroy(format);
    }
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_003
 * @tc.name      : aac(different bitrate)
 * @tc.desc      : function check
 */
HWTEST_F(NativeFunctionTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_003, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    string decoderName = "OH.Media.Codec.Decoder.Audio.AAC";

    string fileList[] = { "fltp_aac_low_32k_16000_2_dayuhaitang.aac", "fltp_aac_low_128k_44100_2_dayuhaitang.aac",
    "fltp_aac_low_500k_96000_2_dayuhaitang.aac" };

    for (int i = 0; i < 3; i++)
    {
        vector<string> dest = SplitStringFully(fileList[i], "_");
        if (dest.size() < SIZE_7)
        {
            cout << "split error !!!" << endl;
            return;
        }
        int32_t channelCount = stoi(dest[INDEX_5]);
        int32_t sampleRate = stoi(dest[INDEX_4]);

        string bitStr = dest[INDEX_3];
        string_replace(bitStr, "k", "000");
        long bitrate = atol(bitStr.c_str());

        OH_AVFormat* format = OH_AVFormat_Create();
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, channelCount);
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, sampleRate);
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_AAC_IS_ADTS, DEFAULT_AAC_TYPE);
        OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, bitrate);
        ASSERT_NE(nullptr, format);

        string inputFile = fileList[i];
        string outputFile = "FUNCTION_003_" + to_string(bitrate) + ".pcm";

        decoderDemo->NativeRunCase(inputFile, outputFile, decoderName.c_str(), format);

        OH_AVFormat_Destroy(format);
    }
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_004
 * @tc.name      : mp3(different sample rate)
 * @tc.desc      : function check
 */
HWTEST_F(NativeFunctionTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_004, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    string decoderName = "OH.Media.Codec.Decoder.Audio.Mpeg";

    string fileList[] = { "fltp_40k_8000_2_dayuhaitang.mp3", "fltp_40k_16000_2_dayuhaitang.mp3",
    "fltp_40k_44100_2_dayuhaitang.mp3", "fltp_40k_48000_2_dayuhaitang.mp3"};

    for (int i = 0; i < 4; i++)
    {
        vector<string> dest = SplitStringFully(fileList[i], "_");
        if (dest.size() < SIZE_5)
        {
            cout << "split error !!!" << endl;
            return;
        }
        int32_t channelCount = stoi(dest[INDEX_3]);
        int32_t sampleRate = stoi(dest[INDEX_2]);

        string bitStr = dest[INDEX_1];
        string_replace(bitStr, "k", "000");
        long bitrate = atol(bitStr.c_str());

        OH_AVFormat* format = OH_AVFormat_Create();
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, channelCount);
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, sampleRate);
        OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, bitrate);
        ASSERT_NE(nullptr, format);

        string inputFile = fileList[i];
        string outputFile = "FUNCTION_004_" + to_string(sampleRate) + ".pcm";

        decoderDemo->NativeRunCase(inputFile, outputFile, decoderName.c_str(), format);

        OH_AVFormat_Destroy(format);
    }
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_005
 * @tc.name      : mp3(different sample format)
 * @tc.desc      : function check
 */
HWTEST_F(NativeFunctionTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_005, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    string decoderName = "OH.Media.Codec.Decoder.Audio.Mpeg";

    string fileList[] = { "fltp_128k_48000_2_dayuhaitang.mp3", "s16p_128k_48000_2_dayuhaitang.mp3"};

    for (int i = 0; i < 2; i++)
    {
        vector<string> dest = SplitStringFully(fileList[i], "_");
        if (dest.size() < SIZE_5)
        {
            cout << "split error !!!" << endl;
            return;
        }
        int32_t channelCount = stoi(dest[INDEX_3]);
        int32_t sampleRate = stoi(dest[INDEX_2]);

        string bitStr = dest[INDEX_1];
        string_replace(bitStr, "k", "000");
        long bitrate = atol(bitStr.c_str());
        string sampleFormat = dest[INDEX_0];

        OH_AVFormat* format = OH_AVFormat_Create();
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, channelCount);
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, sampleRate);
        OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, bitrate);
        ASSERT_NE(nullptr, format);

        string inputFile = fileList[i];
        string outputFile = "FUNCTION_005_" + sampleFormat + ".pcm";

        decoderDemo->NativeRunCase(inputFile, outputFile, decoderName.c_str(), format);

        OH_AVFormat_Destroy(format);
    }
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_006
 * @tc.name      : mp3(different channel num)
 * @tc.desc      : function check
 */
HWTEST_F(NativeFunctionTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_006, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    string decoderName = "OH.Media.Codec.Decoder.Audio.Mpeg";

    string fileList[] = { "fltp_128k_44100_1_dayuhaitang.mp3", "fltp_128k_44100_2_dayuhaitang.mp3" };

    for (int i = 0; i < 2; i++)
    {
        vector<string> dest = SplitStringFully(fileList[i], "_");
        if (dest.size() < SIZE_5)
        {
            cout << "split error !!!" << endl;
            return;
        }
        int32_t channelCount = stoi(dest[INDEX_3]);
        int32_t sampleRate = stoi(dest[INDEX_2]);

        string bitStr = dest[INDEX_1];
        string_replace(bitStr, "k", "000");
        long bitrate = atol(bitStr.c_str());

        OH_AVFormat* format = OH_AVFormat_Create();
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, channelCount);
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, sampleRate);
        OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, bitrate);
        ASSERT_NE(nullptr, format);

        string inputFile = fileList[i];
        string outputFile = "FUNCTION_006_" + to_string(channelCount) + ".pcm";

        decoderDemo->NativeRunCase(inputFile, outputFile, decoderName.c_str(), format);

        OH_AVFormat_Destroy(format);
    }
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_007
 * @tc.name      : mp3(different bitrate)
 * @tc.desc      : function check
 */
HWTEST_F(NativeFunctionTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_007, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    string decoderName = "OH.Media.Codec.Decoder.Audio.Mpeg";

    string fileList[] = { "fltp_40k_44100_2_dayuhaitang.mp3", "fltp_128k_44100_2_dayuhaitang.mp3",
    "fltp_320k_44100_2_dayuhaitang.mp3"};

    for (int i = 0; i < 3; i++)
    {
        vector<string> dest = SplitStringFully(fileList[i], "_");
        if (dest.size() < SIZE_5)
        {
            cout << "split error !!!" << endl;
            return;
        }
        int32_t channelCount = stoi(dest[INDEX_3]);
        int32_t sampleRate = stoi(dest[INDEX_2]);

        string bitStr = dest[INDEX_1];
        string_replace(bitStr, "k", "000");
        long bitrate = atol(bitStr.c_str());

        OH_AVFormat* format = OH_AVFormat_Create();
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, channelCount);
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, sampleRate);
        OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, bitrate);
        ASSERT_NE(nullptr, format);

        string inputFile = fileList[i];
        string outputFile = "FUNCTION_007_" + to_string(bitrate) + ".pcm";

        decoderDemo->NativeRunCase(inputFile, outputFile, decoderName.c_str(), format);

        OH_AVFormat_Destroy(format);
    }
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_008
 * @tc.name      : flac(different sample rate)
 * @tc.desc      : function check
 */
HWTEST_F(NativeFunctionTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_008, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    string decoderName = "OH.Media.Codec.Decoder.Audio.Flac";

    string fileList[] = { "s16_8000_2_dayuhaitang.flac", "s16_16000_2_dayuhaitang.flac",
    "s16_44100_2_dayuhaitang.flac", "s16_48000_2_dayuhaitang.flac"};

    for (int i = 0; i < SIZE_4; i++)
    {
        vector<string> dest = SplitStringFully(fileList[i], "_");
        if (dest.size() < 4)
        {
            cout << "split error !!!" << endl;
            return;
        }
        int32_t channelCount = stoi(dest[INDEX_2]);
        int32_t sampleRate = stoi(dest[INDEX_1]);

        string bitStr = dest[INDEX_1];
        string_replace(bitStr, "k", "000");
        long bitrate = atol(bitStr.c_str());

        OH_AVFormat* format = OH_AVFormat_Create();
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, channelCount);
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, sampleRate);
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, OH_BitsPerSample::SAMPLE_S16P);
        OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, bitrate);
        ASSERT_NE(nullptr, format);

        string inputFile = fileList[i];
        string outputFile = "FUNCTION_008_" + to_string(sampleRate) + ".pcm";

        decoderDemo->NativeRunCase(inputFile, outputFile, decoderName.c_str(), format);

        OH_AVFormat_Destroy(format);
    }
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_009
 * @tc.name      : flac(different sample format)
 * @tc.desc      : function check
 */
HWTEST_F(NativeFunctionTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_009, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    string decoderName = "OH.Media.Codec.Decoder.Audio.Flac";

    string fileList[] = { "s16_48000_2_dayuhaitang.flac", "s32_48000_2_dayuhaitang.flac"};

    for (int i = 0; i < 2; i++)
    {
        vector<string> dest = SplitStringFully(fileList[i], "_");
        if (dest.size() < 4)
        {
            cout << "split error !!!" << endl;
            return;
        }
        int32_t channelCount = stoi(dest[INDEX_2]);
        int32_t sampleRate = stoi(dest[INDEX_1]);

        string bitStr = dest[INDEX_1];
        string_replace(bitStr, "k", "000");
        long bitrate = atol(bitStr.c_str());
        string sampleFormat = dest[INDEX_0];

        OH_AVFormat* format = OH_AVFormat_Create();
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, channelCount);
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, sampleRate);
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, OH_BitsPerSample::SAMPLE_S16P);
        OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, bitrate);
        ASSERT_NE(nullptr, format);

        string inputFile = fileList[i];
        string outputFile = "FUNCTION_009_" + sampleFormat + ".pcm";

        decoderDemo->NativeRunCase(inputFile, outputFile, decoderName.c_str(), format);

        OH_AVFormat_Destroy(format);
    }
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_010
 * @tc.name      : flac(different channel num)
 * @tc.desc      : function check
 */
HWTEST_F(NativeFunctionTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_010, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    string decoderName = "OH.Media.Codec.Decoder.Audio.Flac";

    string fileList[] = { "s16_48000_1_dayuhaitang.flac", "s16_48000_2_dayuhaitang.flac",
    "s16_48000_8_dayuhaitang.flac"};

    for (int i = 0; i < 3; i++)
    {
        vector<string> dest = SplitStringFully(fileList[i], "_");
        if (dest.size() < 4)
        {
            cout << "split error !!!" << endl;
            return;
        }
        int32_t channelCount = stoi(dest[INDEX_2]);
        int32_t sampleRate = stoi(dest[INDEX_1]);

        string bitStr = dest[INDEX_1];
        string_replace(bitStr, "k", "000");
        long bitrate = atol(bitStr.c_str());

        OH_AVFormat* format = OH_AVFormat_Create();
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, channelCount);
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, sampleRate);
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, OH_BitsPerSample::SAMPLE_S16P);
        OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, bitrate);
        ASSERT_NE(nullptr, format);

        string inputFile = fileList[i];
        string outputFile = "FUNCTION_010_" + to_string(channelCount) + ".pcm";

        decoderDemo->NativeRunCase(inputFile, outputFile, decoderName.c_str(), format);

        OH_AVFormat_Destroy(format);
    }
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_011
 * @tc.name      : vorbis(different sample rate)
 * @tc.desc      : function check
 */
HWTEST_F(NativeFunctionTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_011, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    string decoderName = "OH.Media.Codec.Decoder.Audio.Vorbis";

    string fileList[] = { "fltp_45k_8000_2_dayuhaitang.ogg", "fltp_45k_16000_2_dayuhaitang.ogg",
    "fltp_45k_44100_2_dayuhaitang.ogg", "fltp_45k_48000_2_dayuhaitang.ogg",
    "fltp_280k_96000_2_dayuhaitang.ogg"};

    for (int i = 0; i < 5; i++)
    {
        vector<string> dest = SplitStringFully(fileList[i], "_");
        if (dest.size() < 5)
        {
            cout << "split error !!!" << endl;
            return;
        }
        int32_t channelCount = stoi(dest[INDEX_3]);
        int32_t sampleRate = stoi(dest[INDEX_2]);

        string bitStr = dest[INDEX_1];
        string_replace(bitStr, "k", "000");
        long bitrate = atol(bitStr.c_str());

        OH_AVFormat* format = OH_AVFormat_Create();
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, channelCount);
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, sampleRate);
        OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, bitrate);
        ASSERT_NE(nullptr, format);

        string inputFile = fileList[i];
        string outputFile = "FUNCTION_011_" + to_string(sampleRate) + ".pcm";

        decoderDemo->NativeRunCase(inputFile, outputFile, decoderName.c_str(), format);

        OH_AVFormat_Destroy(format);
    }
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_012
 * @tc.name      : vorbis(different channel num)
 * @tc.desc      : function check
 */
HWTEST_F(NativeFunctionTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_012, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    string decoderName = "OH.Media.Codec.Decoder.Audio.Vorbis";

    string fileList[] = { "fltp_128k_48000_1_dayuhaitang.ogg", "fltp_128k_48000_2_dayuhaitang.ogg",
    "fltp_500k_48000_8_dayuhaitang.ogg"};

    for (int i = 0; i < 3; i++)
    {
        vector<string> dest = SplitStringFully(fileList[i], "_");
        if (dest.size() < SIZE_5)
        {
            cout << "split error !!!" << endl;
            return;
        }
        int32_t channelCount = stoi(dest[INDEX_3]);
        int32_t sampleRate = stoi(dest[INDEX_2]);

        string bitStr = dest[INDEX_1];
        string_replace(bitStr, "k", "000");
        long bitrate = atol(bitStr.c_str());

        OH_AVFormat* format = OH_AVFormat_Create();
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, channelCount);
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, sampleRate);
        OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, bitrate);
        ASSERT_NE(nullptr, format);

        string inputFile = fileList[i];
        string outputFile = "FUNCTION_012_" + to_string(channelCount) + ".pcm";

        decoderDemo->NativeRunCase(inputFile, outputFile, decoderName.c_str(), format);

        OH_AVFormat_Destroy(format);
    }
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_013
 * @tc.name      : vorbis(different bitrate)
 * @tc.desc      : function check
 */
HWTEST_F(NativeFunctionTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_013, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    string decoderName = "OH.Media.Codec.Decoder.Audio.Vorbis";

    string fileList[] = { "fltp_45k_48000_2_dayuhaitang.ogg", "fltp_128k_48000_2_dayuhaitang.ogg",
    "fltp_500k_48000_2_dayuhaitang.ogg" };

    for (int i = 0; i < 3; i++)
    {
        vector<string> dest = SplitStringFully(fileList[i], "_");
        if (dest.size() < SIZE_5)
        {
            cout << "split error !!!" << endl;
            return;
        }
        int32_t channelCount = stoi(dest[INDEX_3]);
        int32_t sampleRate = stoi(dest[INDEX_2]);

        string bitStr = dest[INDEX_1];
        string_replace(bitStr, "k", "000");
        long bitrate = atol(bitStr.c_str());

        OH_AVFormat* format = OH_AVFormat_Create();
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, channelCount);
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, sampleRate);
        OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, bitrate);
        ASSERT_NE(nullptr, format);

        string inputFile = fileList[i];
        string outputFile = "FUNCTION_013_" + to_string(bitrate) + ".pcm";

        decoderDemo->NativeRunCase(inputFile, outputFile, decoderName.c_str(), format);

        OH_AVFormat_Destroy(format);
    }
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_014
 * @tc.name      : Flush(AAC)
 * @tc.desc      : function check
 */
HWTEST_F(NativeFunctionTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_014, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    string decoderName = "OH.Media.Codec.Decoder.Audio.AAC";

    string inputFile = "fltp_aac_low_128k_16000_1_dayuhaitang.aac";
    string firstOutputFile = "FUNCTION_014_1.pcm";
    string secondOutputFile = "FUNCTION_014_2.pcm";

    vector<string> dest = SplitStringFully(inputFile, "_");
    if (dest.size() < SIZE_7)
    {
        cout << "split error !!!" << endl;
        return;
    }
    int32_t channelCount = stoi(dest[INDEX_5]);
    int32_t sampleRate = stoi(dest[INDEX_4]);

    string bitStr = dest[INDEX_3];
    string_replace(bitStr, "k", "000");
    long bitrate = atol(bitStr.c_str());

    OH_AVFormat* format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, channelCount);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, sampleRate);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AAC_IS_ADTS, DEFAULT_AAC_TYPE);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, bitrate);

    decoderDemo->NativeRunCaseFlush(inputFile, firstOutputFile, secondOutputFile, decoderName.c_str(), format);

    bool isSame = compareFile(firstOutputFile, secondOutputFile);
    ASSERT_EQ(true, isSame);

    OH_AVFormat_Destroy(format);
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_015
 * @tc.name      : Flush(mp3)
 * @tc.desc      : function check
 */
HWTEST_F(NativeFunctionTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_015, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    string decoderName = "OH.Media.Codec.Decoder.Audio.Mpeg";

    string inputFile = "fltp_40k_16000_2_dayuhaitang.mp3";
    string firstOutputFile = "FUNCTION_015_1.pcm";
    string secondOutputFile = "FUNCTION_015_2.pcm";

    vector<string> dest = SplitStringFully(inputFile, "_");
    if (dest.size() < SIZE_5)
    {
        cout << "split error !!!" << endl;
        return;
    }
    int32_t channelCount = stoi(dest[INDEX_3]);
    int32_t sampleRate = stoi(dest[INDEX_2]);

    string bitStr = dest[INDEX_1];
    string_replace(bitStr, "k", "000");
    long bitrate = atol(bitStr.c_str());

    OH_AVFormat* format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, channelCount);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, sampleRate);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, bitrate);

    decoderDemo->NativeRunCaseFlush(inputFile, firstOutputFile, secondOutputFile, decoderName.c_str(), format);

    bool isSame = compareFile(firstOutputFile, secondOutputFile);
    ASSERT_EQ(true, isSame);

    OH_AVFormat_Destroy(format);
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_016
 * @tc.name      : Flush(flac)
 * @tc.desc      : function check
 */
HWTEST_F(NativeFunctionTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_016, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    string decoderName = "OH.Media.Codec.Decoder.Audio.Flac";

    string inputFile = "s16_8000_2_dayuhaitang.flac";
    string firstOutputFile = "FUNCTION_016_1.pcm";
    string secondOutputFile = "FUNCTION_016_2.pcm";

    vector<string> dest = SplitStringFully(inputFile, "_");
    if (dest.size() < 4)
    {
        cout << "split error !!!" << endl;
        return;
    }
    int32_t channelCount = stoi(dest[INDEX_2]);
    int32_t sampleRate = stoi(dest[INDEX_1]);

    string bitStr = dest[INDEX_1];
    string_replace(bitStr, "k", "000");
    long bitrate = atol(bitStr.c_str());

    OH_AVFormat* format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, channelCount);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, sampleRate);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, bitrate);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, OH_BitsPerSample::SAMPLE_S16P);

    decoderDemo->NativeRunCaseFlush(inputFile, firstOutputFile, secondOutputFile, decoderName.c_str(), format);

    bool isSame = compareFile(firstOutputFile, secondOutputFile);
    ASSERT_EQ(true, isSame);

    OH_AVFormat_Destroy(format);
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_017
 * @tc.name      : Flush(vorbis)
 * @tc.desc      : function check
 */
HWTEST_F(NativeFunctionTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_017, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    string decoderName = "OH.Media.Codec.Decoder.Audio.Vorbis";

    string inputFile = "fltp_45k_8000_2_dayuhaitang.ogg";
    string firstOutputFile = "FUNCTION_017_1.pcm";
    string secondOutputFile = "FUNCTION_017_2.pcm";

    vector<string> dest = SplitStringFully(inputFile, "_");
    if (dest.size() < 5)
    {
        cout << "split error !!!" << endl;
        return;
    }
    int32_t channelCount = stoi(dest[INDEX_3]);
    int32_t sampleRate = stoi(dest[INDEX_2]);

    string bitStr = dest[INDEX_1];
    string_replace(bitStr, "k", "000");
    long bitrate = atol(bitStr.c_str());

    OH_AVFormat* format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, channelCount);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, sampleRate);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, bitrate);

    decoderDemo->NativeRunCaseFlush(inputFile, firstOutputFile, secondOutputFile, decoderName.c_str(), format);

    bool isSame = compareFile(firstOutputFile, secondOutputFile);
    ASSERT_EQ(true, isSame);

    OH_AVFormat_Destroy(format);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_017
 * @tc.name      : Flush(amrwb)
 * @tc.desc      : function check
 */
HWTEST_F(NativeFunctionTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_FLUSH_AMRWB, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    string decoderName = "OH.Media.Codec.Decoder.Audio.Amrwb";

    string inputFile = "voice_amrwb_23850.amr";
    string firstOutputFile = "FUNCTION_FLUSH_AMRWB_1.pcm";
    string secondOutputFile = "FUNCTION_FLUSH_AMRWB_2.pcm";

    int32_t channelCount = AMRWB_CHANNEL_COUNT;
    int32_t sampleRate = AMRWB_SAMPLE_RATE;

    OH_AVFormat* format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, channelCount);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, sampleRate);

    decoderDemo->NativeRunCaseFlush(inputFile, firstOutputFile, secondOutputFile, decoderName.c_str(), format);

    bool isSame = compareFile(firstOutputFile, secondOutputFile);
    ASSERT_EQ(true, isSame);

    OH_AVFormat_Destroy(format);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_017
 * @tc.name      : Flush(amrnb)
 * @tc.desc      : function check
 */
HWTEST_F(NativeFunctionTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_FLUSH_AMRNB, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    string decoderName = "OH.Media.Codec.Decoder.Audio.Amrnb";

    string inputFile = "voice_amrnb_12200.amr";
    string firstOutputFile = "FUNCTION_FLUSH_AMRNB_1.pcm";
    string secondOutputFile = "FUNCTION_FLUSH_AMRNB_2.pcm";

    int32_t channelCount = AMRNB_CHANNEL_COUNT;
    int32_t sampleRate = AMRNB_SAMPLE_RATE;

    OH_AVFormat* format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, channelCount);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, sampleRate);

    decoderDemo->NativeRunCaseFlush(inputFile, firstOutputFile, secondOutputFile, decoderName.c_str(), format);

    bool isSame = compareFile(firstOutputFile, secondOutputFile);
    ASSERT_EQ(true, isSame);

    OH_AVFormat_Destroy(format);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_018
 * @tc.name      : Reset(AAC)
 * @tc.desc      : function check
 */
HWTEST_F(NativeFunctionTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_018, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    string decoderName = "OH.Media.Codec.Decoder.Audio.AAC";

    string inputFile = "fltp_aac_low_128k_16000_1_dayuhaitang.aac";
    string firstOutputFile = "FUNCTION_018_1.pcm";
    string secondOutputFile = "FUNCTION_018_2.pcm";

    vector<string> dest = SplitStringFully(inputFile, "_");
    if (dest.size() < 7)
    {
        cout << "split error !!!" << endl;
        return;
    }
    int32_t channelCount = stoi(dest[INDEX_5]);
    int32_t sampleRate = stoi(dest[INDEX_4]);

    string bitStr = dest[INDEX_3];
    string_replace(bitStr, "k", "000");
    long bitrate = atol(bitStr.c_str());

    OH_AVFormat* format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, channelCount);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, sampleRate);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AAC_IS_ADTS, DEFAULT_AAC_TYPE);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, bitrate);

    decoderDemo->NativeRunCaseReset(inputFile, firstOutputFile, secondOutputFile, decoderName.c_str(), format);

    bool isSame = compareFile(firstOutputFile, secondOutputFile);
    ASSERT_EQ(true, isSame);

    OH_AVFormat_Destroy(format);
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_019
 * @tc.name      : Reset(mp3)
 * @tc.desc      : function check
 */
HWTEST_F(NativeFunctionTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_019, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    string decoderName = "OH.Media.Codec.Decoder.Audio.Mpeg";

    string inputFile = "fltp_40k_16000_2_dayuhaitang.mp3";
    string firstOutputFile = "FUNCTION_019_1.pcm";
    string secondOutputFile = "FUNCTION_019_2.pcm";

    vector<string> dest = SplitStringFully(inputFile, "_");
    if (dest.size() < 5)
    {
        cout << "split error !!!" << endl;
        return;
    }
    int32_t channelCount = stoi(dest[INDEX_3]);
    int32_t sampleRate = stoi(dest[INDEX_2]);

    string bitStr = dest[INDEX_1];
    string_replace(bitStr, "k", "000");
    long bitrate = atol(bitStr.c_str());

    OH_AVFormat* format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, channelCount);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, sampleRate);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, bitrate);

    decoderDemo->NativeRunCaseReset(inputFile, firstOutputFile, secondOutputFile, decoderName.c_str(), format);

    bool isSame = compareFile(firstOutputFile, secondOutputFile);
    ASSERT_EQ(true, isSame);

    OH_AVFormat_Destroy(format);
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_020
 * @tc.name      : Reset(flac)
 * @tc.desc      : function check
 */
HWTEST_F(NativeFunctionTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_020, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    string decoderName = "OH.Media.Codec.Decoder.Audio.Flac";

    string inputFile = "s16_8000_2_dayuhaitang.flac";
    string firstOutputFile = "FUNCTION_020_1.pcm";
    string secondOutputFile = "FUNCTION_020_2.pcm";

    vector<string> dest = SplitStringFully(inputFile, "_");
    if (dest.size() < 4)
    {
        cout << "split error !!!" << endl;
        return;
    }
    int32_t channelCount = stoi(dest[INDEX_2]);
    int32_t sampleRate = stoi(dest[INDEX_1]);

    string bitStr = dest[INDEX_1];
    string_replace(bitStr, "k", "000");
    long bitrate = atol(bitStr.c_str());

    OH_AVFormat* format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, channelCount);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, sampleRate);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, bitrate);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, OH_BitsPerSample::SAMPLE_S16P);

    decoderDemo->NativeRunCaseReset(inputFile, firstOutputFile, secondOutputFile, decoderName.c_str(), format);

    bool isSame = compareFile(firstOutputFile, secondOutputFile);
    ASSERT_EQ(true, isSame);

    OH_AVFormat_Destroy(format);
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_021
 * @tc.name      : Reset(vorbis)
 * @tc.desc      : function check
 */
HWTEST_F(NativeFunctionTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_021, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    string decoderName = "OH.Media.Codec.Decoder.Audio.Vorbis";

    string inputFile = "fltp_45k_48000_2_dayuhaitang.ogg";
    string firstOutputFile = "FUNCTION_021_1.pcm";
    string secondOutputFile = "FUNCTION_021_2.pcm";

    vector<string> dest = SplitStringFully(inputFile, "_");
    if (dest.size() < 5)
    {
        cout << "split error !!!" << endl;
        return;
    }
    int32_t channelCount = stoi(dest[INDEX_3]);
    int32_t sampleRate = stoi(dest[INDEX_2]);

    string bitStr = dest[INDEX_1];
    string_replace(bitStr, "k", "000");
    long bitrate = atol(bitStr.c_str());

    OH_AVFormat* format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, channelCount);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, sampleRate);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, bitrate);

    decoderDemo->NativeRunCaseReset(inputFile, firstOutputFile, secondOutputFile, decoderName.c_str(), format);

    bool isSame = compareFile(firstOutputFile, secondOutputFile);
    ASSERT_EQ(true, isSame);

    OH_AVFormat_Destroy(format);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_021
 * @tc.name      : Reset(Amrwb)
 * @tc.desc      : function check
 */
HWTEST_F(NativeFunctionTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_AMRWB_RESET, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    string decoderName = "OH.Media.Codec.Decoder.Audio.Amrwb";

    string inputFile = "voice_amrwb_23850.amr";
    string firstOutputFile = "FUNCTION_AMRWB_RESET_1.pcm";
    string secondOutputFile = "FUNCTION_AMRWB_RESET_2.pcm";

    OH_AVFormat* format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, AMRWB_CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, AMRWB_SAMPLE_RATE);

    decoderDemo->NativeRunCaseReset(inputFile, firstOutputFile, secondOutputFile, decoderName.c_str(), format);

    bool isSame = compareFile(firstOutputFile, secondOutputFile);
    ASSERT_EQ(true, isSame);

    OH_AVFormat_Destroy(format);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_021
 * @tc.name      : Reset(Amrnb)
 * @tc.desc      : function check
 */
HWTEST_F(NativeFunctionTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_AMRNB_RESET, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    string decoderName = "OH.Media.Codec.Decoder.Audio.Amrnb";

    string inputFile = "voice_amrnb_12200.amr";
    string firstOutputFile = "FUNCTION_AMRNB_RESET_1.pcm";
    string secondOutputFile = "FUNCTION_AMRNB_RESET_2.pcm";

    OH_AVFormat* format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, AMRNB_CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, AMRNB_SAMPLE_RATE);

    decoderDemo->NativeRunCaseReset(inputFile, firstOutputFile, secondOutputFile, decoderName.c_str(), format);

    bool isSame = compareFile(firstOutputFile, secondOutputFile);
    ASSERT_EQ(true, isSame);

    OH_AVFormat_Destroy(format);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_022
 * @tc.name      : OH_AudioDecoder_GetOutputDescription
 * @tc.desc      : function check
 */
HWTEST_F(NativeFunctionTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_022, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    string decoderName = "OH.Media.Codec.Decoder.Audio.AAC";

    string inputFile = "fltp_aac_low_128k_16000_1_dayuhaitang.aac";
    string outputFile = "FUNCTION_018_1.pcm";

    vector<string> dest = SplitStringFully(inputFile, "_");
    if (dest.size() < 7)
    {
        cout << "split error !!!" << endl;
        return;
    }
    int32_t channelCount = stoi(dest[INDEX_5]);
    int32_t sampleRate = stoi(dest[INDEX_4]);

    string bitStr = dest[INDEX_3];
    string_replace(bitStr, "k", "000");
    long bitrate = atol(bitStr.c_str());

    OH_AVFormat* format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, channelCount);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, sampleRate);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AAC_IS_ADTS, DEFAULT_AAC_TYPE);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, bitrate);

    OH_AVFormat* formatGet = decoderDemo->NativeRunCaseGetOutputDescription(inputFile,
    outputFile, decoderName.c_str(), format);
    ASSERT_NE(nullptr, formatGet);

    int32_t channelNum_Get;
    int32_t sampleRate_Get;
    int64_t bitrate_Get;
    OH_AVFormat_GetIntValue(formatGet, OH_MD_KEY_AUD_CHANNEL_COUNT, &channelNum_Get);
    OH_AVFormat_GetIntValue(formatGet, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate_Get);
    OH_AVFormat_GetLongValue(formatGet, OH_MD_KEY_BITRATE, &bitrate_Get);
    ASSERT_EQ(channelCount, channelNum_Get);
    ASSERT_EQ(sampleRate, sampleRate_Get);
    ASSERT_EQ(bitrate, bitrate_Get);

    OH_AVFormat_Destroy(format);
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_023
 * @tc.name      : AAC(thread)
 * @tc.desc      : Function test
 */
HWTEST_F(NativeFunctionTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_023, TestSize.Level2)
{
    vector<thread> threadVec;
    string decoderName = "OH.Media.Codec.Decoder.Audio.AAC";

    string inputFile = "fltp_aac_low_128k_16000_1_dayuhaitang.aac";

    for (int32_t i = 0; i < 16; i++)
    {
        string outputFile = "FUNCTION_023_" + to_string(i) + ".pcm";
        threadVec.push_back(thread(runDecode, decoderName, inputFile, outputFile, i));
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
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_024
 * @tc.name      : MP3(thread)
 * @tc.desc      : Function test
 */
HWTEST_F(NativeFunctionTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_024, TestSize.Level2)
{
    vector<thread> threadVec;
    string decoderName = "OH.Media.Codec.Decoder.Audio.Mpeg";

    string inputFile = "fltp_40k_16000_2_dayuhaitang.mp3";

    for (int32_t i = 0; i < 16; i++)
    {
        string outputFile = "FUNCTION_024_" + to_string(i) + ".pcm";
        threadVec.push_back(thread(runDecode, decoderName, inputFile, outputFile, i));
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
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_025
 * @tc.name      : Flac(thread)
 * @tc.desc      : Function test
 */
HWTEST_F(NativeFunctionTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_025, TestSize.Level2)
{
    vector<thread> threadVec;
    string decoderName = "OH.Media.Codec.Decoder.Audio.Flac";

    string inputFile = "s16_8000_2_dayuhaitang.flac";

    for (int32_t i = 0; i < 16; i++)
    {
        string outputFile = "FUNCTION_025_" + to_string(i) + ".pcm";
        threadVec.push_back(thread(runDecode, decoderName, inputFile, outputFile, i));
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
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_026
 * @tc.name      : Vorbis(thread)
 * @tc.desc      : Function test
 */
HWTEST_F(NativeFunctionTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_026, TestSize.Level2)
{
    vector<thread> threadVec;
    string decoderName = "OH.Media.Codec.Decoder.Audio.Vorbis";

    string inputFile = "fltp_45k_48000_2_dayuhaitang.ogg";

    for (int32_t i = 0; i < 16; i++)
    {
        string outputFile = "FUNCTION_026_" + to_string(i) + ".pcm";
        threadVec.push_back(thread(runDecode, decoderName, inputFile, outputFile, i));
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
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_026
 * @tc.name      : Amrwb(thread)
 * @tc.desc      : Function test
 */
HWTEST_F(NativeFunctionTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_THREAD_AMRWB, TestSize.Level2)
{
    vector<thread> threadVec;
    string decoderName = "OH.Media.Codec.Decoder.Audio.Amrwb";

    string inputFile = "voice_amrwb_23850.amr";

    for (int32_t i = 0; i < 16; i++)
    {
        string outputFile = "FUNCTION_THREAD_AMRWB_" + to_string(i) + ".pcm";
        threadVec.push_back(thread(runDecode, decoderName, inputFile, outputFile, i));
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
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_026
 * @tc.name      : Amrnb(thread)
 * @tc.desc      : Function test
 */
HWTEST_F(NativeFunctionTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_THREAD_AMRNB, TestSize.Level2)
{
    vector<thread> threadVec;
    string decoderName = "OH.Media.Codec.Decoder.Audio.Amrnb";

    string inputFile = "voice_amrnb_12200.amr";

    for (int32_t i = 0; i < 16; i++)
    {
        string outputFile = "FUNCTION_THREAD_AMRNB_" + to_string(i) + ".pcm";
        threadVec.push_back(thread(runDecode, decoderName, inputFile, outputFile, i));
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
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_100
 * @tc.name      : AAC
 * @tc.desc      : Function test
 */
HWTEST_F(NativeFunctionTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_100, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    string decoderName = "OH.Media.Codec.Decoder.Audio.AAC";

    string inputFile = "wmxq_01_2.0.BIN.aac";
    string outputFile = "FUNCTION_100.pcm";

    OH_AVFormat* format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 2);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 48000);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AAC_IS_ADTS, DEFAULT_AAC_TYPE);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 192000);
    ASSERT_NE(nullptr, format);

    decoderDemo->NativeRunCasePerformance(inputFile, outputFile, decoderName.c_str(), format);

    OH_AVFormat_Destroy(format);
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_200
 * @tc.name      : MP3
 * @tc.desc      : Function test
 */
HWTEST_F(NativeFunctionTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_200, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    string decoderName = "OH.Media.Codec.Decoder.Audio.Mpeg";

    string inputFile = "B_Bird_on_a_wire_L.mp3";
    string outputFile = "FUNCTION_200.pcm";

    OH_AVFormat* format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 1);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 48000);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 192000);
    ASSERT_NE(nullptr, format);

    decoderDemo->NativeRunCasePerformance(inputFile, outputFile, decoderName.c_str(), format);

    OH_AVFormat_Destroy(format);
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_300
 * @tc.name      : FLAC
 * @tc.desc      : Function test
 */
HWTEST_F(NativeFunctionTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_300, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    string decoderName = "OH.Media.Codec.Decoder.Audio.Flac";

    string inputFile = "B_Bird_on_a_wire_1.flac";
    string outputFile = "FUNCTION_300.pcm";

    OH_AVFormat* format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 1);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 48000);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 435000);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, OH_BitsPerSample::SAMPLE_S16P);
    ASSERT_NE(nullptr, format);

    decoderDemo->NativeRunCasePerformance(inputFile, outputFile, decoderName.c_str(), format);

    OH_AVFormat_Destroy(format);
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_301
 * @tc.name      : FLAC(free)
 * @tc.desc      : Function test
 */
HWTEST_F(NativeFunctionTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_301, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    string decoderName = "OH.Media.Codec.Decoder.Audio.Flac";

    string inputFile = "free_loss.flac";
    string outputFile = "FUNCTION_301.pcm";

    OH_AVFormat* format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 1);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 96000);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 622000);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, OH_BitsPerSample::SAMPLE_S16P);
    ASSERT_NE(nullptr, format);

    decoderDemo->NativeRunCasePerformance(inputFile, outputFile, decoderName.c_str(), format);

    OH_AVFormat_Destroy(format);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_302
 * @tc.name      : FLAC
 * @tc.desc      : Function test
 */
HWTEST_F(NativeFunctionTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_302, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    string decoderName = "OH.Media.Codec.Decoder.Audio.Flac";

    string inputFile = "FUNCTION_202.dat";
    string outputFile = "FUNCTION_302.pcm";

    OH_AVFormat* format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 1);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 48000);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 435000);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, OH_BitsPerSample::SAMPLE_S16P);
    ASSERT_NE(nullptr, format);

    decoderDemo->TestRunCase(inputFile, outputFile, decoderName.c_str(), format);

    OH_AVFormat_Destroy(format);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_303
 * @tc.name      : FLAC
 * @tc.desc      : Function test
 */
HWTEST_F(NativeFunctionTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_303, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    string decoderName = "OH.Media.Codec.Decoder.Audio.Flac";

    string inputFile = "FUNCTION_203.dat";
    string outputFile = "FUNCTION_303.pcm";

    OH_AVFormat* format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 1);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 96000);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 622000);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, OH_BitsPerSample::SAMPLE_S16P);
    ASSERT_NE(nullptr, format);

    decoderDemo->TestRunCase(inputFile, outputFile, decoderName.c_str(), format);

    OH_AVFormat_Destroy(format);
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_400
 * @tc.name      : Vorbis
 * @tc.desc      : Function test
 */
HWTEST_F(NativeFunctionTest, SUB_MULTIMEDIA_AUDIO_DECODER_FUNCTION_400, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    string decoderName = "OH.Media.Codec.Decoder.Audio.Vorbis";

    string inputFile = "B_Bird_on_a_wire_L.ogg";
    string outputFile = "FUNCTION_400.pcm";

    OH_AVFormat* format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 1);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 48000);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 480000);
    ASSERT_NE(nullptr, format);

    decoderDemo->NativeRunCasePerformance(inputFile, outputFile, decoderName.c_str(), format);

    OH_AVFormat_Destroy(format);
    delete decoderDemo;
}