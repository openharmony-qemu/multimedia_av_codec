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
#include <ctime>
#include <sys/time.h>
#include "gtest/gtest.h"
#include "AudioDecoderDemoCommon.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;
constexpr uint32_t SIZE_7 = 7;
constexpr uint32_t SIZE_5 = 5;
constexpr uint32_t SIZE_4 = 4;
constexpr uint32_t PER_CODED_SAMPL = 16;

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

    constexpr int32_t SPLIT_INDEX_1 = 1;
    constexpr int32_t SPLIT_INDEX_2 = 2;
    constexpr int32_t SPLIT_INDEX_3 = 3;
    constexpr int32_t SPLIT_INDEX_4 = 4;
    constexpr int32_t SPLIT_INDEX_5 = 5;

    int32_t g_testResult[16] = { -1 };

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

    void StringReplace(std::string& strBig, const std::string& strsrc, const std::string& strdst)
    {
        std::string::size_type pos = 0;
        std::string::size_type srclen = strsrc.size();
        std::string::size_type dstlen = strdst.size();

        while ((pos = strBig.find(strsrc, pos)) != std::string::npos) {
            strBig.replace(pos, srclen, strdst);
            pos += dstlen;
        }
    }

    void GetParamsByName(string decoderName, string inputFile, int32_t& channelCount,
        int32_t& sampleRate, long& bitrate)
    {
        vector<string> dest = SplitStringFully(inputFile, "_");
        if (decoderName == "OH.Media.Codec.Decoder.Audio.AAC") {
            if (dest.size() < SIZE_7) {
                cout << "split error !!!" << endl;
                return;
            }
            channelCount = stoi(dest[SPLIT_INDEX_5]);
            sampleRate = stoi(dest[SPLIT_INDEX_4]);

            string bitStr = dest[SPLIT_INDEX_3];
            StringReplace(bitStr, "k", "000");
            bitrate = atol(bitStr.c_str());
        } else if (decoderName == "OH.Media.Codec.Decoder.Audio.Mpeg") {
            if (dest.size() < SIZE_5) {
                cout << "split error !!!" << endl;
                return;
            }
            channelCount = stoi(dest[SPLIT_INDEX_3]);
            sampleRate = stoi(dest[SPLIT_INDEX_2]);

            string bitStr = dest[SPLIT_INDEX_1];
            StringReplace(bitStr, "k", "000");
            bitrate = atol(bitStr.c_str());
        } else if (decoderName == "OH.Media.Codec.Decoder.Audio.Flac") {
            if (dest.size() < SIZE_4) {
                cout << "split error !!!" << endl;
                return;
            }
            channelCount = stoi(dest[SPLIT_INDEX_2]);
            sampleRate = stoi(dest[SPLIT_INDEX_1]);

            string bitStr = dest[SPLIT_INDEX_1];
            StringReplace(bitStr, "k", "000");
            bitrate = atol(bitStr.c_str());
        } else {
            if (dest.size() < SIZE_5) {
                cout << "split error !!!" << endl;
                return;
            }
            channelCount = stoi(dest[SPLIT_INDEX_3]);
            sampleRate = stoi(dest[SPLIT_INDEX_2]);

            string bitStr = dest[SPLIT_INDEX_1];
            StringReplace(bitStr, "k", "000");
            bitrate = atol(bitStr.c_str());
        }
    }

    OH_AVFormat* GetAVFormatByDecoder(string decoderName, string inputFile)
    {
        OH_AVFormat* format;
        int32_t channelCount;
        int32_t sampleRate;
        long bitrate;
        GetParamsByName(decoderName, inputFile, channelCount, sampleRate, bitrate);
        if (decoderName == "OH.Media.Codec.Decoder.Audio.AAC") {
            format = OH_AVFormat_Create();
            OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, channelCount);
            OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, sampleRate);
            OH_AVFormat_SetIntValue(format, OH_MD_KEY_AAC_IS_ADTS, DEFAULT_AAC_TYPE);
            OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, bitrate);
        } else if (decoderName == "OH.Media.Codec.Decoder.Audio.Mpeg") {
            format = OH_AVFormat_Create();
            OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, channelCount);
            OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, sampleRate);
            OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, bitrate);
        } else if (decoderName == "OH.Media.Codec.Decoder.Audio.Flac") {
            format = OH_AVFormat_Create();
            OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, channelCount);
            OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, sampleRate);
            OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, PER_CODED_SAMPL);
            OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, bitrate);
        } else {
            format = OH_AVFormat_Create();
            OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, channelCount);
            OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, sampleRate);
            OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, bitrate);
        }
        return format;
    }

    void RunDecode(string decoderName, string inputFile, string outputFile, int32_t threadId)
    {
        AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();

        OH_AVFormat* format = GetAVFormatByDecoder(decoderName, inputFile);

        decoderDemo->NativeRunCase(inputFile, outputFile, decoderName.c_str(), format);
        OH_AVFormat_Destroy(format);

        g_testResult[threadId] = AV_ERR_OK;

        delete decoderDemo;
    }

    void TestFFmpeg()
    {
        AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();

        decoderDemo->TestFFmpeg("fltp_aac_low_128k_16000_1_dayuhaitang.aac");

        delete decoderDemo;
    }

    void RunLongTimeFlush(string decoderName, string inputFile, string outputFile, int32_t threadId)
    {
        AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
        bool needConfigure = true;

        time_t startTime = time(nullptr);
        ASSERT_NE(startTime, -1);
        time_t curTime = startTime;

        OH_AVCodec* handle = decoderDemo->NativeCreateByName(decoderName.c_str());
        OH_AVFormat* format = GetAVFormatByDecoder(decoderName, inputFile);
        struct OH_AVCodecAsyncCallback cb = { &OnError, &OnOutputFormatChanged,
            &OnInputBufferAvailable, &OnOutputBufferAvailable };
        decoderDemo->NativeSetCallback(handle, cb);

        while (difftime(curTime, startTime) < RUN_TIME) {
            decoderDemo->NativeRunCaseWithoutCreate(handle, inputFile, outputFile,
                format, decoderName.c_str(), needConfigure);
            needConfigure = false;
            decoderDemo->NativeFlush(handle);
            curTime = time(nullptr);
            ASSERT_NE(curTime, -1);
        }
        OH_AVFormat_Destroy(format);
        decoderDemo->NativeDestroy(handle);
        delete decoderDemo;
        g_testResult[threadId] = AV_ERR_OK;
    }

    void RunLongTimeReset(string decoderName, string inputFile, string outputFile, int32_t threadId)
    {
        AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
        bool needConfigure = true;

        time_t startTime = time(nullptr);
        ASSERT_NE(startTime, -1);
        time_t curTime = startTime;

        OH_AVCodec* handle = decoderDemo->NativeCreateByName(decoderName.c_str());
        OH_AVFormat* format = GetAVFormatByDecoder(decoderName, inputFile);
        struct OH_AVCodecAsyncCallback cb = { &OnError, &OnOutputFormatChanged,
            &OnInputBufferAvailable, &OnOutputBufferAvailable };
        decoderDemo->NativeSetCallback(handle, cb);

        while (difftime(curTime, startTime) < RUN_TIME) {
            decoderDemo->NativeRunCaseWithoutCreate(handle, inputFile, outputFile,
                format, decoderName.c_str(), needConfigure);
            needConfigure = false;
            decoderDemo->NativeFlush(handle);
            curTime = time(nullptr);
            ASSERT_NE(curTime, -1);
        }

        OH_AVFormat_Destroy(format);
        decoderDemo->NativeDestroy(handle);
        delete decoderDemo;
        g_testResult[threadId] = AV_ERR_OK;
    }

    void RunLongTimeStop(string decoderName, string inputFile, string outputFile, int32_t threadId)
    {
        AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
        bool needConfigure = true;

        time_t startTime = time(nullptr);
        ASSERT_NE(startTime, -1);
        time_t curTime = startTime;

        OH_AVCodec* handle = decoderDemo->NativeCreateByName(decoderName.c_str());
        OH_AVFormat* format = GetAVFormatByDecoder(decoderName, inputFile);
        struct OH_AVCodecAsyncCallback cb = { &OnError, &OnOutputFormatChanged,
            &OnInputBufferAvailable, &OnOutputBufferAvailable };
        decoderDemo->NativeSetCallback(handle, cb);

        while (difftime(curTime, startTime) < RUN_TIME) {
            decoderDemo->NativeRunCaseWithoutCreate(handle, inputFile, outputFile,
                format, decoderName.c_str(), needConfigure);
            needConfigure = false;
            decoderDemo->NativeStop(handle);
            curTime = time(nullptr);
            ASSERT_NE(curTime, -1);
        }

        OH_AVFormat_Destroy(format);
        decoderDemo->NativeDestroy(handle);
        delete decoderDemo;
        g_testResult[threadId] = AV_ERR_OK;
    }
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_001
 * @tc.name      : OH_AudioDecoder_CreateByMime 2000 times
 * @tc.desc      : stability
 */
HWTEST_F(NativeStablityTest, SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_001, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    double totalTime = 0;
    struct timeval start, end;
    for (int i = 0; i < RUN_TIMES; i++)
    {
        gettimeofday(&start, nullptr);
        OH_AVCodec* handle = decoderDemo->NativeCreateByMime(OH_AVCODEC_MIMETYPE_AUDIO_AAC);
        gettimeofday(&end, nullptr);
        totalTime += (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
        ASSERT_NE(nullptr, handle);
        decoderDemo->NativeDestroy(handle);
    }
    cout << "2000 times finish, run time is " << totalTime << endl;
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_002
 * @tc.name      : OH_AudioDecoder_CreateByName 2000 times
 * @tc.desc      : stability
 */
HWTEST_F(NativeStablityTest, SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_002, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    double totalTime = 0;
    struct timeval start, end;
    for (int i = 0; i < RUN_TIMES; i++)
    {
        gettimeofday(&start, nullptr);
        OH_AVCodec* handle = decoderDemo->NativeCreateByName("OH.Media.Codec.Decoder.Audio.Mpeg");
        gettimeofday(&end, nullptr);
        totalTime += (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
        ASSERT_NE(nullptr, handle);
        decoderDemo->NativeDestroy(handle);
    }
    cout << "2000 times finish, run time is " << totalTime << endl;
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_003
 * @tc.name      : OH_AudioDecoder_Destroy 2000 times
 * @tc.desc      : stability
 */
HWTEST_F(NativeStablityTest, SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_003, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    double totalTime = 0;
    struct timeval start, end;

    for (int i = 0; i < RUN_TIMES; i++)
    {
        OH_AVCodec* handle = decoderDemo->NativeCreateByName("OH.Media.Codec.Decoder.Audio.Mpeg");
        ASSERT_NE(nullptr, handle);
        gettimeofday(&start, nullptr);
        decoderDemo->NativeDestroy(handle);
        gettimeofday(&end, nullptr);
        totalTime += (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
    }
    cout << "2000 times finish, run time is " << totalTime << endl;
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_004
 * @tc.name      : OH_AudioDecoder_SetCallback 2000 times
 * @tc.desc      : stability
 */
HWTEST_F(NativeStablityTest, SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_004, TestSize.Level2)
{
    OH_AVErrCode ret;
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    double totalTime = 0;
    struct timeval start, end;

    OH_AVCodec* handle = decoderDemo->NativeCreateByName("OH.Media.Codec.Decoder.Audio.Mpeg");
    ASSERT_NE(nullptr, handle);
    struct OH_AVCodecAsyncCallback cb = { &OnError, &OnOutputFormatChanged,
    &OnInputBufferAvailable, &OnOutputBufferAvailable };
    for (int i = 0; i < RUN_TIMES; i++)
    {
        gettimeofday(&start, nullptr);
        ret = decoderDemo->NativeSetCallback(handle, cb);
        gettimeofday(&end, nullptr);
        totalTime += (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
        ASSERT_EQ(AV_ERR_OK, ret);
    }
    cout << "2000 times finish, run time is " << totalTime << endl;
    decoderDemo->NativeDestroy(handle);
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_005
 * @tc.name      : OH_AudioDecoder_Configure 2000 times
 * @tc.desc      : stability
 */
HWTEST_F(NativeStablityTest, SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_005, TestSize.Level2)
{
    OH_AVErrCode ret;
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    double totalTime = 0;
    struct timeval start, end;

    OH_AVFormat* format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 2);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 44100);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 169000);

    for (int i = 0; i < RUN_TIMES; i++)
    {
        OH_AVCodec* handle = decoderDemo->NativeCreateByName("OH.Media.Codec.Decoder.Audio.Mpeg");
        ASSERT_NE(nullptr, handle);
        struct OH_AVCodecAsyncCallback cb = { &OnError, &OnOutputFormatChanged,
        &OnInputBufferAvailable, &OnOutputBufferAvailable };
        ret = decoderDemo->NativeSetCallback(handle, cb);
        ASSERT_EQ(AV_ERR_OK, ret);
        gettimeofday(&start, nullptr);
        ret = decoderDemo->NativeConfigure(handle, format);
        gettimeofday(&end, nullptr);
        totalTime += (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
        ASSERT_EQ(AV_ERR_OK, ret);

        decoderDemo->NativeDestroy(handle);
    }
    cout << "2000 times finish, run time is " << totalTime << endl;

    OH_AVFormat_Destroy(format);
    delete decoderDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_006
 * @tc.name      : OH_AudioDecoder_Prepare 2000 times
 * @tc.desc      : stability
 */
HWTEST_F(NativeStablityTest, SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_006, TestSize.Level2)
{
    OH_AVErrCode ret;
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    double totalTime = 0;
    struct timeval start, end;

    OH_AVFormat* format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 2);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 44100);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 169000);

    for (int i = 0; i < RUN_TIMES; i++)
    {
        OH_AVCodec* handle = decoderDemo->NativeCreateByName("OH.Media.Codec.Decoder.Audio.Mpeg");
        ASSERT_NE(nullptr, handle);
        struct OH_AVCodecAsyncCallback cb = { &OnError, &OnOutputFormatChanged,
        &OnInputBufferAvailable, &OnOutputBufferAvailable };
        ret = decoderDemo->NativeSetCallback(handle, cb);
        ASSERT_EQ(AV_ERR_OK, ret);
        ret = decoderDemo->NativeConfigure(handle, format);
        ASSERT_EQ(AV_ERR_OK, ret);
        gettimeofday(&start, nullptr);
        ret = decoderDemo->NativePrepare(handle);
        gettimeofday(&end, nullptr);
        totalTime += (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
        ASSERT_EQ(AV_ERR_OK, ret);
        decoderDemo->NativeDestroy(handle);
    }
    cout << "2000 times finish, run time is " << totalTime << endl;

    OH_AVFormat_Destroy(format);
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_007
 * @tc.name      : OH_AudioDecoder_Start 2000 times
 * @tc.desc      : stability
 */
HWTEST_F(NativeStablityTest, SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_007, TestSize.Level2)
{
    OH_AVErrCode ret;
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    double totalTime = 0;
    struct timeval start, end;

    OH_AVFormat* format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 2);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 44100);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 169000);

    for (int i = 0; i < RUN_TIMES; i++)
    {
        OH_AVCodec* handle = decoderDemo->NativeCreateByName("OH.Media.Codec.Decoder.Audio.Mpeg");
        ASSERT_NE(nullptr, handle);
        struct OH_AVCodecAsyncCallback cb = { &OnError, &OnOutputFormatChanged,
        &OnInputBufferAvailable, &OnOutputBufferAvailable };
        ret = decoderDemo->NativeSetCallback(handle, cb);
        ASSERT_EQ(AV_ERR_OK, ret);
        ret = decoderDemo->NativeConfigure(handle, format);
        ASSERT_EQ(AV_ERR_OK, ret);
        ret = decoderDemo->NativePrepare(handle);
        ASSERT_EQ(AV_ERR_OK, ret);
        gettimeofday(&start, nullptr);
        ret = OH_AudioDecoder_Start(handle);
        gettimeofday(&end, nullptr);
        totalTime += (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
        ASSERT_EQ(AV_ERR_OK, ret);
        decoderDemo->NativeDestroy(handle);
    }
    cout << "2000 times finish, run time is " << totalTime << endl;

    OH_AVFormat_Destroy(format);
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_008
 * @tc.name      : OH_AudioDecoder_Stop 2000 times
 * @tc.desc      : stability
 */
HWTEST_F(NativeStablityTest, SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_008, TestSize.Level2)
{
    OH_AVErrCode ret;
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    double totalTime = 0;
    struct timeval start, end;

    OH_AVFormat* format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 2);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 44100);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 169000);

    for (int i = 0; i < RUN_TIMES; i++)
    {
        OH_AVCodec* handle = decoderDemo->NativeCreateByName("OH.Media.Codec.Decoder.Audio.Mpeg");
        ASSERT_NE(nullptr, handle);
        struct OH_AVCodecAsyncCallback cb = { &OnError, &OnOutputFormatChanged,
        &OnInputBufferAvailable, &OnOutputBufferAvailable };
        ret = decoderDemo->NativeSetCallback(handle, cb);
        ASSERT_EQ(AV_ERR_OK, ret);
        ret = decoderDemo->NativeConfigure(handle, format);
        ASSERT_EQ(AV_ERR_OK, ret);
        ret = decoderDemo->NativePrepare(handle);
        ASSERT_EQ(AV_ERR_OK, ret);
        ret = OH_AudioDecoder_Start(handle);
        ASSERT_EQ(AV_ERR_OK, ret);
        gettimeofday(&start, nullptr);
        ret = OH_AudioDecoder_Stop(handle);
        gettimeofday(&end, nullptr);
        totalTime += (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
        ASSERT_EQ(AV_ERR_OK, ret);
        decoderDemo->NativeDestroy(handle);
    }
    cout << "2000 times finish, run time is " << totalTime << endl;

    OH_AVFormat_Destroy(format);
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_009
 * @tc.name      : OH_AudioDecoder_Flush 2000 times
 * @tc.desc      : stability
 */
HWTEST_F(NativeStablityTest, SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_009, TestSize.Level2)
{
    OH_AVErrCode ret;
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    double totalTime = 0;
    struct timeval start, end;

    OH_AVFormat* format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 2);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 44100);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 169000);

    for (int i = 0; i < RUN_TIMES; i++)
    {
        OH_AVCodec* handle = decoderDemo->NativeCreateByName("OH.Media.Codec.Decoder.Audio.Mpeg");
        ASSERT_NE(nullptr, handle);
        struct OH_AVCodecAsyncCallback cb = { &OnError, &OnOutputFormatChanged,
        &OnInputBufferAvailable, &OnOutputBufferAvailable };
        ret = decoderDemo->NativeSetCallback(handle, cb);
        ASSERT_EQ(AV_ERR_OK, ret);
        ret = decoderDemo->NativeConfigure(handle, format);
        ASSERT_EQ(AV_ERR_OK, ret);
        ret = decoderDemo->NativePrepare(handle);
        ASSERT_EQ(AV_ERR_OK, ret);
        ret = OH_AudioDecoder_Start(handle);
        ASSERT_EQ(AV_ERR_OK, ret);
        gettimeofday(&start, nullptr);
        ret = decoderDemo->NativeFlush(handle);
        gettimeofday(&end, nullptr);
        totalTime += (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
        ASSERT_EQ(AV_ERR_OK, ret);
        decoderDemo->NativeDestroy(handle);
    }
    cout << "2000 times finish, run time is " << totalTime << endl;

    OH_AVFormat_Destroy(format);
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_010
 * @tc.name      : OH_AudioDecoder_Reset 2000 times
 * @tc.desc      : stability
 */
HWTEST_F(NativeStablityTest, SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_010, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    double totalTime = 0;
    struct timeval start, end;

    OH_AVCodec* handle = decoderDemo->NativeCreateByName("OH.Media.Codec.Decoder.Audio.Mpeg");
    ASSERT_NE(nullptr, handle);

    for (int i = 0; i < RUN_TIMES; i++)
    {
        gettimeofday(&start, nullptr);
        OH_AudioDecoder_Reset(handle);
        gettimeofday(&end, nullptr);
        totalTime += (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
    }
    cout << "2000 times finish, run time is " << totalTime << endl;
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_011
 * @tc.name      : OH_AudioDecoder_GetOutputDescription 2000 times
 * @tc.desc      : stability
 */
HWTEST_F(NativeStablityTest, SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_011, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();

    string decoderName = "OH.Media.Codec.Decoder.Audio.AAC";

    string inputFile = "fltp_aac_low_500k_96000_2_dayuhaitang.aac";
    string outputFile = "STABILITY_010.pcm";

    vector<string> dest = SplitStringFully(inputFile, "_");
    if (dest.size() < SIZE_7)
    {
        cout << "split error !!!" << endl;
        return;
    }
    int32_t channelCount = stoi(dest[5]);
    int32_t sampleRate = stoi(dest[4]);

    string bitStr = dest[3];
    StringReplace(bitStr, "k", "000");
    long bitrate = atol(bitStr.c_str());

    OH_AVFormat* format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, channelCount);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, sampleRate);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AAC_IS_ADTS, DEFAULT_AAC_TYPE);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, bitrate);
    ASSERT_NE(nullptr, format);

    decoderDemo->setTimerFlag(TIMER_GETOUTPUTDESCRIPTION);
    decoderDemo->NativeRunCase(inputFile, outputFile, decoderName.c_str(), format);

    OH_AVFormat_Destroy(format);
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_012
 * @tc.name      : OH_AudioDecoder_SetParameter 2000 times
 * @tc.desc      : stability
 */
HWTEST_F(NativeStablityTest, SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_012, TestSize.Level2)
{
    OH_AVErrCode ret;
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    double totalTime = 0;
    struct timeval start, end;

    OH_AVFormat* format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 2);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 44100);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 169000);

    OH_AVCodec* handle = decoderDemo->NativeCreateByName("OH.Media.Codec.Decoder.Audio.Mpeg");
    ASSERT_NE(nullptr, handle);

    struct OH_AVCodecAsyncCallback cb = { &OnError, &OnOutputFormatChanged,
    &OnInputBufferAvailable, &OnOutputBufferAvailable };
    ret = decoderDemo->NativeSetCallback(handle, cb);
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = decoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = decoderDemo->NativePrepare(handle);
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = decoderDemo->NativeStart(handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    for (int i = 0; i < RUN_TIMES; i++)
    {
        gettimeofday(&start, nullptr);
        ret = decoderDemo->NativeSetParameter(handle, format);
        gettimeofday(&end, nullptr);
        totalTime += (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
        ASSERT_EQ(AV_ERR_OK, ret);
    }

    cout << "2000 times finish, run time is " << totalTime << endl;
    decoderDemo->NativeDestroy(handle);
    OH_AVFormat_Destroy(format);
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_013
 * @tc.name      : OH_AudioDecoder_PushInputData 2000 times
 * @tc.desc      : stability
 */
HWTEST_F(NativeStablityTest, SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_013, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();

    string decoderName = "OH.Media.Codec.Decoder.Audio.AAC";

    string inputFile = "fltp_aac_low_500k_96000_2_dayuhaitang.aac";
    string outputFile = "STABILITY_010.pcm";

    vector<string> dest = SplitStringFully(inputFile, "_");
    if (dest.size() < 7)
    {
        cout << "split error !!!" << endl;
        return;
    }
    int32_t channelCount = stoi(dest[5]);
    int32_t sampleRate = stoi(dest[4]);

    string bitStr = dest[3];
    StringReplace(bitStr, "k", "000");
    long bitrate = atol(bitStr.c_str());

    OH_AVFormat* format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, channelCount);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, sampleRate);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AAC_IS_ADTS, DEFAULT_AAC_TYPE);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, bitrate);
    ASSERT_NE(nullptr, format);

    decoderDemo->setTimerFlag(TIMER_INPUT);
    decoderDemo->NativeRunCase(inputFile, outputFile, decoderName.c_str(), format);

    OH_AVFormat_Destroy(format);
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_014
 * @tc.name      : OH_AudioDecoder_FreeOutputData 2000 times
 * @tc.desc      : stability
 */
HWTEST_F(NativeStablityTest, SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_014, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();

    string decoderName = "OH.Media.Codec.Decoder.Audio.AAC";

    string inputFile = "fltp_aac_low_500k_96000_2_dayuhaitang.aac";
    string outputFile = "STABILITY_010.pcm";

    vector<string> dest = SplitStringFully(inputFile, "_");
    if (dest.size() < 7)
    {
        cout << "split error !!!" << endl;
        return;
    }
    int32_t channelCount = stoi(dest[5]);
    int32_t sampleRate = stoi(dest[4]);

    string bitStr = dest[3];
    StringReplace(bitStr, "k", "000");
    long bitrate = atol(bitStr.c_str());

    OH_AVFormat* format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, channelCount);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, sampleRate);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AAC_IS_ADTS, DEFAULT_AAC_TYPE);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, bitrate);
    ASSERT_NE(nullptr, format);

    decoderDemo->setTimerFlag(TIMER_FREEOUTPUT);
    decoderDemo->NativeRunCase(inputFile, outputFile, decoderName.c_str(), format);

    OH_AVFormat_Destroy(format);
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_015
 * @tc.name      : OH_AudioDecoder_IsValid 2000 times
 * @tc.desc      : stability
 */
HWTEST_F(NativeStablityTest, SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_015, TestSize.Level2)
{
    OH_AVErrCode ret;
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    double totalTime = 0;
    struct timeval start, end;
    bool isValid;

    OH_AVFormat* format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 2);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 44100);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 169000);

    OH_AVCodec* handle = decoderDemo->NativeCreateByName("OH.Media.Codec.Decoder.Audio.Mpeg");
    ASSERT_NE(nullptr, handle);

    struct OH_AVCodecAsyncCallback cb = { &OnError, &OnOutputFormatChanged,
    &OnInputBufferAvailable, &OnOutputBufferAvailable };
    ret = decoderDemo->NativeSetCallback(handle, cb);
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = decoderDemo->NativeConfigure(handle, format);
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = decoderDemo->NativePrepare(handle);
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = decoderDemo->NativeStart(handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    for (int i = 0; i < RUN_TIMES; i++)
    {
        gettimeofday(&start, nullptr);
        ret = decoderDemo->NativeIsValid(handle, &isValid);
        gettimeofday(&end, nullptr);
        totalTime += (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
        cout << "IsValid ret is " << ret << endl;
    }

    cout << "2000 times finish, run time is " << totalTime << endl;
    decoderDemo->NativeDestroy(handle);
    OH_AVFormat_Destroy(format);
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_016
 * @tc.name      : decoder(long time)
 * @tc.desc      : stability
 */
HWTEST_F(NativeStablityTest, SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_016, TestSize.Level2)
{
    string decoderList[] = { "OH.Media.Codec.Decoder.Audio.AAC", "OH.Media.Codec.Decoder.Audio.Mpeg",
    "OH.Media.Codec.Decoder.Audio.Flac", "OH.Media.Codec.Decoder.Audio.Vorbis" };
    string decoderName;
    string inputFile;
    string outputFile = "STABILITY_016.pcm";

    time_t startTime = time(nullptr);
    ASSERT_NE(startTime, -1);
    time_t curTime = startTime;

    while (difftime(curTime, startTime) < RUN_TIME)
    {
        for (int i = 0; i < 4; i++)
        {
            decoderName = decoderList[i];
            if (decoderName == "OH.Media.Codec.Decoder.Audio.AAC") {
                inputFile = "fltp_aac_low_128k_16000_1_dayuhaitang.aac";
            } else if (decoderName == "OH.Media.Codec.Decoder.Audio.Mpeg") {
                inputFile = "fltp_40k_16000_2_dayuhaitang.mp3";
            } else if (decoderName == "OH.Media.Codec.Decoder.Audio.Flac") {
                inputFile = "s16_8000_2_dayuhaitang.flac";
            } else {
                inputFile = "fltp_45k_48000_2_dayuhaitang.ogg";
            }

            cout << "cur decoder name is " << decoderName << ", input file is "
            << inputFile << ", output file is " << outputFile << endl;
            RunDecode(decoderName, inputFile, outputFile, i);
            ASSERT_EQ(AV_ERR_OK, g_testResult[i]);
        }
        curTime = time(nullptr);
        ASSERT_NE(curTime, -1);
    }
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_017
 * @tc.name      : Flush(long time)
 * @tc.desc      : stability
 */
HWTEST_F(NativeStablityTest, SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_017, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    string decoderName = "OH.Media.Codec.Decoder.Audio.AAC";
    string inputFile = "fltp_aac_low_128k_16000_1_dayuhaitang.aac";
    string outputFile = "STABILITY_017.pcm";
    bool needConfigure = true;

    time_t startTime = time(nullptr);
    ASSERT_NE(startTime, -1);
    time_t curTime = startTime;

    OH_AVCodec* handle = decoderDemo->NativeCreateByName(decoderName.c_str());
    OH_AVFormat* format = GetAVFormatByDecoder(decoderName, inputFile);
    struct OH_AVCodecAsyncCallback cb = { &OnError, &OnOutputFormatChanged,
    &OnInputBufferAvailable, &OnOutputBufferAvailable };
    decoderDemo->NativeSetCallback(handle, cb);

    while (difftime(curTime, startTime) < RUN_TIME)
    {
        decoderDemo->NativeRunCaseWithoutCreate(handle, inputFile, outputFile,
        format, decoderName.c_str(), needConfigure);
        needConfigure = false;
        decoderDemo->NativeFlush(handle);
        curTime = time(nullptr);
        ASSERT_NE(curTime, -1);
    }

    OH_AVFormat_Destroy(format);
    decoderDemo->NativeDestroy(handle);
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_018
 * @tc.name      : Reset(long time)
 * @tc.desc      : stability
 */
HWTEST_F(NativeStablityTest, SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_018, TestSize.Level2)
{
    AudioDecoderDemo* decoderDemo = new AudioDecoderDemo();
    string decoderName = "OH.Media.Codec.Decoder.Audio.AAC";
    string inputFile = "fltp_aac_low_128k_16000_1_dayuhaitang.aac";
    string outputFile = "STABILITY_018.pcm";
    bool needConfigure = true;

    time_t startTime = time(nullptr);
    ASSERT_NE(startTime, -1);
    time_t curTime = startTime;

    OH_AVCodec* handle = decoderDemo->NativeCreateByName(decoderName.c_str());
    OH_AVFormat* format = GetAVFormatByDecoder(decoderName, inputFile);
    struct OH_AVCodecAsyncCallback cb = { &OnError, &OnOutputFormatChanged,
    &OnInputBufferAvailable, &OnOutputBufferAvailable };
    decoderDemo->NativeSetCallback(handle, cb);

    while (difftime(curTime, startTime) < RUN_TIME)
    {
        decoderDemo->NativeRunCaseWithoutCreate(handle, inputFile, outputFile,
        format, decoderName.c_str(), needConfigure);
        decoderDemo->NativeReset(handle);
        curTime = time(nullptr);
        ASSERT_NE(curTime, -1);
    }

    OH_AVFormat_Destroy(format);
    decoderDemo->NativeDestroy(handle);
    delete decoderDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_019
 * @tc.name      : thread decoder(long time)
 * @tc.desc      : stability
 */
HWTEST_F(NativeStablityTest, SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_019, TestSize.Level2)
{
    string decoderList[] = { "OH.Media.Codec.Decoder.Audio.AAC", "OH.Media.Codec.Decoder.Audio.Mpeg",
    "OH.Media.Codec.Decoder.Audio.Flac", "OH.Media.Codec.Decoder.Audio.Vorbis" };
    string decoderName;
    string inputFile;
    vector<thread> threadVec;

    time_t startTime = time(nullptr);
    ASSERT_NE(startTime, -1);
    time_t curTime = startTime;

    while (difftime(curTime, startTime) < RUN_TIME)
    {
        threadVec.clear();
        for (int32_t i = 0; i < 16; i++)
        {
            decoderName = decoderList[i % 4];
            if (decoderName == "OH.Media.Codec.Decoder.Audio.AAC") {
                inputFile = "fltp_aac_low_128k_16000_1_dayuhaitang.aac";
            } else if (decoderName == "OH.Media.Codec.Decoder.Audio.Mpeg") {
                inputFile = "fltp_40k_16000_2_dayuhaitang.mp3";
            } else if (decoderName == "OH.Media.Codec.Decoder.Audio.Flac") {
                inputFile = "s16_8000_2_dayuhaitang.flac";
            } else {
                inputFile = "fltp_45k_48000_2_dayuhaitang.ogg";
            }

            string outputFile = "STABILITY_019_" + to_string(i) + ".pcm";
            cout << "cur decoder name is " << decoderName << ", input file is "
            << inputFile << ", output file is " << outputFile << endl;
            threadVec.push_back(thread(RunDecode, decoderName, inputFile, outputFile, i));
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
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_020
 * @tc.name      : thread decoder Flush(long time)
 * @tc.desc      : stability
 */
HWTEST_F(NativeStablityTest, SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_020, TestSize.Level2)
{
    string decoderList[] = { "OH.Media.Codec.Decoder.Audio.AAC", "OH.Media.Codec.Decoder.Audio.Mpeg",
    "OH.Media.Codec.Decoder.Audio.Flac", "OH.Media.Codec.Decoder.Audio.Vorbis" };
    string decoderName;
    string inputFile;
    vector<thread> threadVec;

    for (int32_t i = 0; i < 16; i++)
    {
        decoderName = decoderList[i % 4];
        if (decoderName == "OH.Media.Codec.Decoder.Audio.AAC") {
            inputFile = "fltp_aac_low_128k_16000_1_dayuhaitang.aac";
        } else if (decoderName == "OH.Media.Codec.Decoder.Audio.Mpeg") {
            inputFile = "fltp_40k_16000_2_dayuhaitang.mp3";
        } else if (decoderName == "OH.Media.Codec.Decoder.Audio.Flac") {
            inputFile = "s16_8000_2_dayuhaitang.flac";
        } else {
            inputFile = "fltp_45k_48000_2_dayuhaitang.ogg";
        }

        string outputFile = "STABILITY_020_" + to_string(i) + ".pcm";
        cout << "cur decoder name is " << decoderName << ", input file is " << inputFile
        << ", output file is " << outputFile << endl;
        threadVec.push_back(thread(RunLongTimeFlush, decoderName, inputFile, outputFile, i));
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
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_021
 * @tc.name      : thread decoder Reset(long time)
 * @tc.desc      : stability
 */
HWTEST_F(NativeStablityTest, SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_021, TestSize.Level2)
{
    string decoderList[] = { "OH.Media.Codec.Decoder.Audio.AAC", "OH.Media.Codec.Decoder.Audio.Mpeg",
    "OH.Media.Codec.Decoder.Audio.Flac", "OH.Media.Codec.Decoder.Audio.Vorbis" };
    string decoderName;
    string inputFile;
    vector<thread> threadVec;

    for (int32_t i = 0; i < 16; i++)
    {
        decoderName = decoderList[i % 4];
        if (decoderName == "OH.Media.Codec.Decoder.Audio.AAC") {
            inputFile = "fltp_aac_low_128k_16000_1_dayuhaitang.aac";
        } else if (decoderName == "OH.Media.Codec.Decoder.Audio.Mpeg") {
            inputFile = "fltp_40k_16000_2_dayuhaitang.mp3";
        } else if (decoderName == "OH.Media.Codec.Decoder.Audio.Flac") {
            inputFile = "s16_8000_2_dayuhaitang.flac";
        } else {
            inputFile = "fltp_45k_48000_2_dayuhaitang.ogg";
        }

        string outputFile = "STABILITY_021_" + to_string(i) + ".pcm";
        cout << "cur decoder name is " << decoderName << ", input file is "
        << inputFile << ", output file is " << outputFile << endl;
        threadVec.push_back(thread(RunLongTimeReset, decoderName, inputFile, outputFile, i));
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
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_022
 * @tc.name      : Flush(long time)
 * @tc.desc      : stability
 */
HWTEST_F(NativeStablityTest, SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_022, TestSize.Level2)
{
    string decoderList[] = { "OH.Media.Codec.Decoder.Audio.AAC", "OH.Media.Codec.Decoder.Audio.Mpeg",
    "OH.Media.Codec.Decoder.Audio.Flac", "OH.Media.Codec.Decoder.Audio.Vorbis" };
    string decoderName;
    string inputFile;
    vector<thread> threadVec;

    for (int32_t i = 0; i < 16; i++)
    {
        decoderName = decoderList[i % 4];
        if (decoderName == "OH.Media.Codec.Decoder.Audio.AAC") {
            inputFile = "fltp_aac_low_128k_16000_1_dayuhaitang.aac";
        }
        else if (decoderName == "OH.Media.Codec.Decoder.Audio.Mpeg") {
            inputFile = "fltp_40k_16000_2_dayuhaitang.mp3";
        }
        else if (decoderName == "OH.Media.Codec.Decoder.Audio.Flac") {
            inputFile = "s16_8000_2_dayuhaitang.flac";
        }
        else {
            inputFile = "fltp_45k_48000_2_dayuhaitang.ogg";
        }

        string outputFile = "STABILITY_021_" + to_string(i) + ".pcm";
        cout << "cur decoder name is " << decoderName << ", input file is "
            << inputFile << ", output file is " << outputFile << endl;
        threadVec.push_back(thread(RunLongTimeStop, decoderName, inputFile, outputFile, i));
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
 * @tc.number    : SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_100
 * @tc.name      : decoder(long time)
 * @tc.desc      : stability
 */
HWTEST_F(NativeStablityTest, SUB_MULTIMEDIA_AUDIO_DECODER_STABILITY_100, TestSize.Level2)
{
    time_t startTime = time(nullptr);
    ASSERT_NE(startTime, -1);
    time_t curTime = startTime;

    while (difftime(curTime, startTime) < RUN_TIME)
    {
        TestFFmpeg();
        curTime = time(nullptr);
        ASSERT_NE(curTime, -1);
    }
}