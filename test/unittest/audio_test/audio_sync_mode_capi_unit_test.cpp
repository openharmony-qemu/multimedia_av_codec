/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include <vector>
#include <queue>
#include <mutex>
#include <gtest/gtest.h>
#include <iostream>
#include <unistd.h>
#include <atomic>
#include <fstream>
#include <queue>
#include <string>
#include <thread>
#include "securec.h"
#include "avcodec_codec_name.h"
#include "avcodec_common.h"
#include "avcodec_errors.h"
#include "avcodec_mime_type.h"
#include "media_description.h"
#include "native_avcodec_base.h"
#include "native_avformat.h"
#include "avcodec_common.h"
#include "avcodec_errors.h"
#include "native_avcodec_audiocodec.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::Media::Plugins;

namespace {
constexpr int32_t CHANNEL_COUNT = 2;
constexpr int32_t SAMPLE_RATE = 44100;
constexpr int64_t BIT_RATE = 128000;
constexpr uint32_t AAC_SAMPLE_PER_FRAME = 1024;
constexpr uint32_t G711MU_DEFAULT_FRAME_BYTES = 320; // 8kHz 20ms: 2*160
const string OPUS_SO_FILE_PATH = std::string(AV_CODEC_PATH) + "/libav_codec_ext_base.z.so";
constexpr uint32_t OPUS_SAMPLE_RATE = 48000;
constexpr long OPUS_BITS_RATE = 15000;
constexpr int32_t OPUS_COMPLIANCE_LEVEL = 10;
constexpr int32_t OPUS_FRAME_SAMPLE_SIZES = 960 * 2 * 2;
constexpr uint32_t MAX_OUTPUT_TRY_CNT = 30;
constexpr string_view AAC_INPUT_FILE_PATH = "/data/test/media/aac_2c_44100hz_199k.pcm";
constexpr string_view AAC_OUTPUT_FILE_PATH = "/data/test/media/aac_2c_44100hz_encode.aac";
constexpr string_view FLAC_INPUT_FILE_PATH = "/data/test/media/flac_2c_44100hz_261k.pcm";
constexpr string_view FLAC_OUTPUT_FILE_PATH = "/data/test/media/encoderTest.flac";
constexpr string_view G711MU_INPUT_FILE_PATH = "/data/test/media/g711mu_8kHz_10s.pcm";
constexpr string_view G711MU_OUTPUT_FILE_PATH = "/data/test/media/g711mu_8kHz_10s_afterEncode.raw";
constexpr string_view OPUS_INPUT_FILE_PATH = "/data/test/media/flac_2c_44100hz_261k.pcm";
constexpr string_view OPUS_OUTPUT_FILE_PATH = "/data/test/media/encoderTest.opus";

// decoder test
constexpr string_view INPUT_AAC_FILE_PATH = "/data/test/media/aac_2c_44100hz_199k.dat";
constexpr string_view OUTPUT_AAC_PCM_FILE_PATH = "/data/test/media/aac_2c_44100hz_199k.pcm";
constexpr string_view INPUT_FLAC_FILE_PATH = "/data/test/media/flac_2c_44100hz_261k.dat";
constexpr string_view OUTPUT_FLAC_PCM_FILE_PATH = "/data/test/media/flac_2c_44100hz_261k.pcm";
constexpr string_view INPUT_MP3_FILE_PATH = "/data/test/media/mp3_2c_44100hz_60k.dat";
constexpr string_view OUTPUT_MP3_PCM_FILE_PATH = "/data/test/media/mp3_2c_44100hz_60k.pcm";
constexpr string_view INPUT_VORBIS_FILE_PATH = "/data/test/media/vorbis_2c_44100hz_320k.dat";
constexpr string_view OUTPUT_VORBIS_PCM_FILE_PATH = "/data/test/media/vorbis_2c_44100hz_320k.pcm";
constexpr string_view INPUT_AMRNB_FILE_PATH = "/data/test/media/voice_amrnb_12200.dat";
constexpr string_view OUTPUT_AMRNB_PCM_FILE_PATH = "/data/test/media/voice_amrnb_12200.pcm";
constexpr string_view INPUT_AMRWB_FILE_PATH = "/data/test/media/voice_amrwb_23850.dat";
constexpr string_view OUTPUT_AMRWB_PCM_FILE_PATH = "/data/test/media/voice_amrwb_23850.pcm";
constexpr string_view INPUT_G711MU_FILE_PATH = "/data/test/media/g711mu_8kHz.dat";
constexpr string_view OUTPUT_G711MU_PCM_FILE_PATH = "/data/test/media/g711mu_8kHz_decode.pcm";
constexpr string_view INPUT_OPUS_FILE_PATH = "/data/test/media/voice_opus.dat";
constexpr string_view OUTPUT_OPUS_PCM_FILE_PATH = "/data/test/media/opus_decode.pcm";
constexpr string_view INPUT_APE_FILE_PATH = "/data/test/media/voice_ape.dat";
constexpr string_view OUTPUT_APE_PCM_FILE_PATH = "/data/test/media/ape_decode.pcm";
constexpr string_view INPUT_AC3_FILE_PATH = "/data/test/media/voice_ac3.dat";
constexpr string_view OUTPUT_AC3_PCM_FILE_PATH = "/data/test/media/ac3_decode.pcm";
constexpr string_view INPUT_AAC_LC_ADTS_FILE_PATH = "/data/test/media/aac_2c_44100hz_199k_lc.dat";
constexpr string_view OUTPUT_AAC_LC_ADTS_FILE_PATH = "/data/test/media/aac_2c_44100hz_199k_lc.pcm";
}

namespace OHOS {
namespace MediaAVCodec {

class AudioSyncModeCapiUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
    bool CheckOpusSoFunc();
    void EncoderRun();
    void EncoderRunIndependentOutput();
    bool DecoderFillInputBuffer(OH_AVBuffer *buffer);
    void DecoderRun();
    void DecoderRunIndependentOutput();
    void OutputThread();
protected:
    std::atomic<bool> isRunning_ = false;
    std::unique_ptr<std::thread> outThread_;
    OH_AVCodec *codec_ = nullptr;
    OH_AVFormat *format_ = nullptr;
    std::unique_ptr<std::ifstream> inputFile_;
    std::unique_ptr<std::ofstream> outputFile_;
    int32_t channels_ = CHANNEL_COUNT;
    int32_t sampleRate_ = SAMPLE_RATE;
    uint32_t inputFrameBytes_ = 8192;
    uint32_t inputSizeCnt_ = 0;
    uint32_t decodeInputFrameCnt_ = 0;
    uint32_t outputFrameCnt_ = 0;
    uint32_t outputFormatChangedTimes = 0;
    int32_t outputSampleRate = 0;
    int32_t outputChannels = 0;
    int64_t inTimeout_ = 20000; // 20000us: 20ms
    int64_t outTimeout_ = 20000; // 20000us: 20ms
};

void AudioSyncModeCapiUnitTest::SetUpTestCase(void)
{
    cout << "[SetUpTestCase]: " << endl;
}

void AudioSyncModeCapiUnitTest::TearDownTestCase(void)
{
    cout << "[TearDownTestCase]: " << endl;
}

void AudioSyncModeCapiUnitTest::SetUp(void)
{
    format_ = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_ENABLE_SYNC_MODE, 1);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_AUD_SAMPLE_RATE, sampleRate_);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_AUD_CHANNEL_COUNT, channels_);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, OH_BitsPerSample::SAMPLE_F32LE);
    OH_AVFormat_SetLongValue(format_, OH_MD_KEY_BITRATE, BIT_RATE);
    inputSizeCnt_ = 0;
    outputFrameCnt_ = 0;
}

void AudioSyncModeCapiUnitTest::TearDown(void)
{
    if (format_ != nullptr) {
        OH_AVFormat_Destroy(format_);
        format_ = nullptr;
    }
    if (inputFile_ != nullptr) {
        if (inputFile_->is_open()) {
            inputFile_->close();
        }
    }
    if (outputFile_ != nullptr) {
        if (outputFile_->is_open()) {
            outputFile_->close();
        }
    }
}

bool AudioSyncModeCapiUnitTest::CheckOpusSoFunc()
{
    std::unique_ptr<std::ifstream> soFile = std::make_unique<std::ifstream>(OPUS_SO_FILE_PATH, std::ios::binary);
    if (!soFile->is_open()) {
        cout << "Fatal: Open opus so file failed" << endl;
        return false;
    }
    soFile->close();
    return true;
}

void AudioSyncModeCapiUnitTest::EncoderRun()
{
    ASSERT_EQ(OH_AudioCodec_Prepare(codec_), AV_ERR_OK);
    ASSERT_EQ(OH_AudioCodec_Start(codec_), AV_ERR_OK);
    bool inputEnd = false;
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVCodecBufferAttr attr;
    for (;;) {
        uint32_t index = 0;
        if (!inputEnd) {
            ret = OH_AudioCodec_QueryInputBuffer(codec_, &index, inTimeout_);
            if (ret == AV_ERR_TRY_AGAIN_LATER) {
                continue;
            }
            ASSERT_EQ(ret, AV_ERR_OK);
            OH_AVBuffer *inputBuf = OH_AudioCodec_GetInputBuffer(codec_, index);
            ASSERT_NE(inputBuf, nullptr);
            memset_s(&attr, sizeof(attr), 0, sizeof(attr));
            if (!inputFile_->eof()) {
                inputFile_->read((char *)OH_AVBuffer_GetAddr(inputBuf), inputFrameBytes_);
                int32_t readSize = inputFile_->gcount();
                attr.size = readSize;
                attr.flags = readSize != 0 ? AVCODEC_BUFFER_FLAGS_NONE : AVCODEC_BUFFER_FLAGS_EOS;
                inputSizeCnt_ += readSize;
            } else {
                inputEnd = true;
                attr.size = 0;
                attr.flags = AVCODEC_BUFFER_FLAGS_EOS;
                cout << "input index:" << index << ", EOS" << endl;
            }
            EXPECT_EQ(OH_AVBuffer_SetBufferAttr(inputBuf, &attr), AV_ERR_OK);
            EXPECT_EQ(OH_AudioCodec_PushInputBuffer(codec_, index), AV_ERR_OK);
        }
        ret = OH_AudioCodec_QueryOutputBuffer(codec_, &index, outTimeout_);
        if (ret == AV_ERR_TRY_AGAIN_LATER) {
            cout << "output index get timeout" << endl;
            continue;
        }
        EXPECT_EQ(ret, AV_ERR_OK);
        OH_AVBuffer *outputBuf = OH_AudioCodec_GetOutputBuffer(codec_, index);
        ASSERT_NE(outputBuf, nullptr);
        ASSERT_EQ(OH_AVBuffer_GetBufferAttr(outputBuf, &attr), AV_ERR_OK);
        if (attr.flags & AVCODEC_BUFFER_FLAGS_EOS) {
            cout << "output eos" << endl;
            break;
        }
        outputFrameCnt_++;
        outputFile_->write(reinterpret_cast<char *>(OH_AVBuffer_GetAddr(outputBuf)), attr.size);
        ASSERT_EQ(OH_AudioCodec_FreeOutputBuffer(codec_, index), AV_ERR_OK);
    }
}

void AudioSyncModeCapiUnitTest::EncoderRunIndependentOutput()
{
    ASSERT_EQ(OH_AudioCodec_Prepare(codec_), AV_ERR_OK);
    ASSERT_EQ(OH_AudioCodec_Start(codec_), AV_ERR_OK);
    isRunning_.store(true);
    outThread_ = make_unique<thread>(&AudioSyncModeCapiUnitTest::OutputThread, this);
    for (;;) {
        uint32_t index = 0;
        OH_AVErrCode ret = OH_AudioCodec_QueryInputBuffer(codec_, &index, inTimeout_);
        if (ret == AV_ERR_TRY_AGAIN_LATER) {
            continue;
        }
        ASSERT_EQ(ret, AV_ERR_OK);
        OH_AVBuffer *inputBuf = OH_AudioCodec_GetInputBuffer(codec_, index);
        ASSERT_NE(inputBuf, nullptr);
        OH_AVCodecBufferAttr attr;
        memset_s(&attr, sizeof(attr), 0, sizeof(attr));
        if (!inputFile_->eof()) {
            inputFile_->read((char *)OH_AVBuffer_GetAddr(inputBuf), inputFrameBytes_);
            int32_t readSize = inputFile_->gcount();
            attr.size = readSize;
            attr.flags = readSize != 0 ? AVCODEC_BUFFER_FLAGS_NONE : AVCODEC_BUFFER_FLAGS_EOS;
            inputSizeCnt_ += readSize;
            cout << "input index:" << index << ", size:" << readSize << endl;
        } else {
            attr.size = 0;
            attr.flags = AVCODEC_BUFFER_FLAGS_EOS;
            cout << "input index:" << index << ", EOS" << endl;
        }
        EXPECT_EQ(OH_AVBuffer_SetBufferAttr(inputBuf, &attr), AV_ERR_OK);
        EXPECT_EQ(OH_AudioCodec_PushInputBuffer(codec_, index), AV_ERR_OK);
        if (inputFile_->eof()) {
            isRunning_.store(false);
            break;
        }
    }
    if (outThread_ != nullptr && outThread_->joinable()) {
        outThread_->join();
    }
}

void AudioSyncModeCapiUnitTest::OutputThread()
{
    uint32_t timeoutCnt = 0;
    for (;;) {
        uint32_t index = 0;
        OH_AVErrCode ret = OH_AudioCodec_QueryOutputBuffer(codec_, &index, outTimeout_);
        if (!isRunning_.load() && timeoutCnt > MAX_OUTPUT_TRY_CNT) {
            cout << "input end but not EOS output" << endl;
            break;
        }
        if (ret == AV_ERR_TRY_AGAIN_LATER) {
            cout << "thread output index get timeout" << endl;
            timeoutCnt++;
            continue;
        }
        timeoutCnt = 0;
        EXPECT_EQ(ret, AV_ERR_OK);
        OH_AVBuffer *outputBuf = OH_AudioCodec_GetOutputBuffer(codec_, index);
        ASSERT_NE(outputBuf, nullptr);
        OH_AVCodecBufferAttr attr;
        memset_s(&attr, sizeof(attr), 0, sizeof(attr));
        ASSERT_EQ(OH_AVBuffer_GetBufferAttr(outputBuf, &attr), AV_ERR_OK);
        cout << "thread output index:" << index << ", size:" << attr.size << endl;
        if (attr.flags & AVCODEC_BUFFER_FLAGS_EOS) {
            cout << "thread output eos" << endl;
            ASSERT_EQ(OH_AudioCodec_FreeOutputBuffer(codec_, index), AV_ERR_OK);
            break;
        }
        outputFrameCnt_++;
        outputFile_->write(reinterpret_cast<char *>(OH_AVBuffer_GetAddr(outputBuf)), attr.size);
        ASSERT_EQ(OH_AudioCodec_FreeOutputBuffer(codec_, index), AV_ERR_OK);
    }
}

bool AudioSyncModeCapiUnitTest::DecoderFillInputBuffer(OH_AVBuffer *buffer)
{
    OH_AVCodecBufferAttr attr;
    memset_s(&attr, sizeof(attr), 0, sizeof(attr));
    attr.flags = AVCODEC_BUFFER_FLAGS_EOS;
    bool finish = true;
    do {
        int64_t size;
        inputFile_->read(reinterpret_cast<char *>(&size), sizeof(size));
        if (inputFile_->eof() || inputFile_->gcount() != sizeof(size)) {
            cout << "read size not enough, Set EOS" << endl;
            break;
        }

        inputFile_->read(reinterpret_cast<char *>(&attr.pts), sizeof(attr.pts));
        if (inputFile_->gcount() != sizeof(attr.pts)) {
            cout << "read pts fail" << endl;
            break;
        }
        attr.size = static_cast<int32_t>(size);
        if (attr.size > 0) {
            inputFile_->read((char *)OH_AVBuffer_GetAddr(buffer), attr.size);
            if (inputFile_->gcount() != attr.size) {
                cout << "read buffer fail" << endl;
                break;
            }
            attr.flags = AVCODEC_BUFFER_FLAGS_NONE;
            decodeInputFrameCnt_++;
            finish = false;
        }
    } while (0);
    EXPECT_EQ(OH_AVBuffer_SetBufferAttr(buffer, &attr), AV_ERR_OK);
    return finish;
}

void AudioSyncModeCapiUnitTest::DecoderRun()
{
    ASSERT_EQ(OH_AudioCodec_Prepare(codec_), AV_ERR_OK);
    ASSERT_EQ(OH_AudioCodec_Start(codec_), AV_ERR_OK);
    bool inputEnd = false;
    OH_AVErrCode ret = AV_ERR_OK;
    for (;;) {
        uint32_t index = 0;
        if (!inputEnd) {
            ret = OH_AudioCodec_QueryInputBuffer(codec_, &index, inTimeout_);
            if (ret == AV_ERR_TRY_AGAIN_LATER) {
                continue;
            }
            ASSERT_EQ(ret, AV_ERR_OK);
            OH_AVBuffer *inputBuf = OH_AudioCodec_GetInputBuffer(codec_, index);
            ASSERT_NE(inputBuf, nullptr);
            inputEnd = DecoderFillInputBuffer(inputBuf);
            EXPECT_EQ(OH_AudioCodec_PushInputBuffer(codec_, index), AV_ERR_OK);
        }
        ret = OH_AudioCodec_QueryOutputBuffer(codec_, &index, outTimeout_);
        if (ret == AV_ERR_TRY_AGAIN_LATER) {
            cout << "output index get timeout" << endl;
            continue;
        } else if (ret == AV_ERR_STREAM_CHANGED) {
            OH_AVFormat *outFormat = OH_AudioCodec_GetOutputDescription(codec_);
            outputFormatChangedTimes++;
            OH_AVFormat_GetIntValue(outFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &outputChannels);
            OH_AVFormat_GetIntValue(outFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &outputSampleRate);
            continue;
        }
        EXPECT_EQ(ret, AV_ERR_OK);
        OH_AVBuffer *outputBuf = OH_AudioCodec_GetOutputBuffer(codec_, index);
        ASSERT_NE(outputBuf, nullptr);
        OH_AVCodecBufferAttr attr;
        memset_s(&attr, sizeof(attr), 0, sizeof(attr));
        ASSERT_EQ(OH_AVBuffer_GetBufferAttr(outputBuf, &attr), AV_ERR_OK);
        if (attr.flags & AVCODEC_BUFFER_FLAGS_EOS) {
            cout << "output eos" << endl;
            ASSERT_EQ(OH_AudioCodec_FreeOutputBuffer(codec_, index), AV_ERR_OK);
            break;
        }
        outputFrameCnt_++;
        outputFile_->write(reinterpret_cast<char *>(OH_AVBuffer_GetAddr(outputBuf)), attr.size);
        ASSERT_EQ(OH_AudioCodec_FreeOutputBuffer(codec_, index), AV_ERR_OK);
    }
}

void AudioSyncModeCapiUnitTest::DecoderRunIndependentOutput()
{
    ASSERT_EQ(OH_AudioCodec_Prepare(codec_), AV_ERR_OK);
    ASSERT_EQ(OH_AudioCodec_Start(codec_), AV_ERR_OK);
    isRunning_.store(true);
    outThread_ = make_unique<thread>(&AudioSyncModeCapiUnitTest::OutputThread, this);
    for (;;) {
        uint32_t index = 0;
        OH_AVErrCode ret = OH_AudioCodec_QueryInputBuffer(codec_, &index, inTimeout_);
        if (ret == AV_ERR_TRY_AGAIN_LATER) {
            continue;
        }
        ASSERT_EQ(ret, AV_ERR_OK);
        OH_AVBuffer *inputBuf = OH_AudioCodec_GetInputBuffer(codec_, index);
        ASSERT_NE(inputBuf, nullptr);
        DecoderFillInputBuffer(inputBuf);
        EXPECT_EQ(OH_AudioCodec_PushInputBuffer(codec_, index), AV_ERR_OK);
        if (inputFile_->eof()) {
            isRunning_.store(false);
            break;
        }
    }
    if (outThread_ != nullptr && outThread_->joinable()) {
        outThread_->join();
    }
}

HWTEST_F(AudioSyncModeCapiUnitTest, aac_encode_01, TestSize.Level1)
{
    inputFile_ = std::make_unique<std::ifstream>(AAC_INPUT_FILE_PATH, std::ios::binary);
    outputFile_ = std::make_unique<std::ofstream>(AAC_OUTPUT_FILE_PATH, std::ios::binary);
    ASSERT_EQ(inputFile_->is_open(), true);
    ASSERT_EQ(outputFile_->is_open(), true);

    codec_ = OH_AudioCodec_CreateByMime(OH_AVCODEC_MIMETYPE_AUDIO_AAC, true);
    ASSERT_NE(codec_, nullptr);
    ASSERT_EQ(OH_AudioCodec_Configure(codec_, format_), AV_ERR_OK);
    EncoderRun();
    OH_AudioCodec_Destroy(codec_);
}

HWTEST_F(AudioSyncModeCapiUnitTest, aac_encode_02, TestSize.Level1)
{
    inputFile_ = std::make_unique<std::ifstream>(AAC_INPUT_FILE_PATH, std::ios::binary);
    outputFile_ = std::make_unique<std::ofstream>(AAC_OUTPUT_FILE_PATH, std::ios::binary);
    ASSERT_EQ(inputFile_->is_open(), true);
    ASSERT_EQ(outputFile_->is_open(), true);

    codec_ = OH_AudioCodec_CreateByName(std::string(AVCodecCodecName::AUDIO_ENCODER_AAC_NAME).c_str());
    ASSERT_NE(codec_, nullptr);
    ASSERT_EQ(OH_AudioCodec_Configure(codec_, format_), AV_ERR_OK);
    EncoderRun();
    uint32_t needFrames = inputSizeCnt_ / CHANNEL_COUNT / sizeof(int32_t) / AAC_SAMPLE_PER_FRAME + 1;
    EXPECT_EQ(needFrames, outputFrameCnt_);
    OH_AudioCodec_Destroy(codec_);
}

HWTEST_F(AudioSyncModeCapiUnitTest, aac_encode_03, TestSize.Level1)
{
    inputFile_ = std::make_unique<std::ifstream>(AAC_INPUT_FILE_PATH, std::ios::binary);
    outputFile_ = std::make_unique<std::ofstream>(AAC_OUTPUT_FILE_PATH, std::ios::binary);
    ASSERT_EQ(inputFile_->is_open(), true);
    ASSERT_EQ(outputFile_->is_open(), true);

    codec_ = OH_AudioCodec_CreateByName(std::string(AVCodecCodecName::AUDIO_ENCODER_VENDOR_AAC_NAME).c_str());
    if (codec_ != nullptr) {
        ASSERT_EQ(OH_AudioCodec_Configure(codec_, format_), AV_ERR_OK);
        EncoderRun();
        uint32_t needFrames = inputSizeCnt_ / CHANNEL_COUNT / sizeof(int32_t) / AAC_SAMPLE_PER_FRAME + 2; // 2:padding
        EXPECT_EQ(needFrames, outputFrameCnt_);
        OH_AudioCodec_Destroy(codec_);
    }
}

HWTEST_F(AudioSyncModeCapiUnitTest, aac_encode_04, TestSize.Level1)
{
    inputFile_ = std::make_unique<std::ifstream>(AAC_INPUT_FILE_PATH, std::ios::binary);
    outputFile_ = std::make_unique<std::ofstream>(AAC_OUTPUT_FILE_PATH, std::ios::binary);
    ASSERT_EQ(inputFile_->is_open(), true);
    ASSERT_EQ(outputFile_->is_open(), true);

    codec_ = OH_AudioCodec_CreateByName(std::string(AVCodecCodecName::AUDIO_ENCODER_AAC_NAME).c_str());
    ASSERT_NE(codec_, nullptr);
    ASSERT_EQ(OH_AudioCodec_Configure(codec_, format_), AV_ERR_OK);
    EncoderRunIndependentOutput();
    uint32_t needFrames = inputSizeCnt_ / CHANNEL_COUNT / sizeof(int32_t) / AAC_SAMPLE_PER_FRAME + 1;
    EXPECT_EQ(needFrames, outputFrameCnt_);
    OH_AudioCodec_Destroy(codec_);
}

HWTEST_F(AudioSyncModeCapiUnitTest, flac_encode_01, TestSize.Level1)
{
    inputFile_ = std::make_unique<std::ifstream>(FLAC_INPUT_FILE_PATH, std::ios::binary);
    outputFile_ = std::make_unique<std::ofstream>(FLAC_OUTPUT_FILE_PATH, std::ios::binary);
    ASSERT_EQ(inputFile_->is_open(), true);
    ASSERT_EQ(outputFile_->is_open(), true);

    OH_AVFormat_SetLongValue(format_, OH_MD_KEY_BITRATE, 261000);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_BITS_PER_CODED_SAMPLE, AudioSampleFormat::SAMPLE_S16LE);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, AudioSampleFormat::SAMPLE_S16LE);
    OH_AVFormat_SetLongValue(format_, OH_MD_KEY_CHANNEL_LAYOUT, AudioChannelLayout::STEREO);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_COMPLIANCE_LEVEL, 0);

    codec_ = OH_AudioCodec_CreateByMime(OH_AVCODEC_MIMETYPE_AUDIO_FLAC, true);
    ASSERT_NE(codec_, nullptr);
    ASSERT_EQ(OH_AudioCodec_Configure(codec_, format_), AV_ERR_OK);
    EncoderRun();
    OH_AudioCodec_Destroy(codec_);
}

HWTEST_F(AudioSyncModeCapiUnitTest, g711mu_encode_01, TestSize.Level1)
{
    inputFile_ = std::make_unique<std::ifstream>(G711MU_INPUT_FILE_PATH, std::ios::binary);
    outputFile_ = std::make_unique<std::ofstream>(G711MU_OUTPUT_FILE_PATH, std::ios::binary);
    ASSERT_EQ(inputFile_->is_open(), true);
    ASSERT_EQ(outputFile_->is_open(), true);

    inputFrameBytes_ = G711MU_DEFAULT_FRAME_BYTES;
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_AUD_CHANNEL_COUNT, 1);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_AUD_SAMPLE_RATE, 8000);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, AudioSampleFormat::SAMPLE_S16LE);

    codec_ = OH_AudioCodec_CreateByMime(OH_AVCODEC_MIMETYPE_AUDIO_G711MU, true);
    ASSERT_NE(codec_, nullptr);
    ASSERT_EQ(OH_AudioCodec_Configure(codec_, format_), AV_ERR_OK);
    EncoderRun();
    uint32_t needFrames = inputSizeCnt_ / G711MU_DEFAULT_FRAME_BYTES;
    EXPECT_EQ(needFrames, outputFrameCnt_);
    OH_AudioCodec_Destroy(codec_);
}

HWTEST_F(AudioSyncModeCapiUnitTest, opus_encode_01, TestSize.Level1)
{
    if (CheckOpusSoFunc()) {
        inputFile_ = std::make_unique<std::ifstream>(OPUS_INPUT_FILE_PATH, std::ios::binary);
        outputFile_ = std::make_unique<std::ofstream>(OPUS_OUTPUT_FILE_PATH, std::ios::binary);
        ASSERT_EQ(inputFile_->is_open(), true);
        ASSERT_EQ(outputFile_->is_open(), true);

        inputFrameBytes_ = OPUS_FRAME_SAMPLE_SIZES;
        OH_AVFormat_SetLongValue(format_, OH_MD_KEY_BITRATE, OPUS_BITS_RATE);
        OH_AVFormat_SetIntValue(format_, OH_MD_KEY_AUD_SAMPLE_RATE, OPUS_SAMPLE_RATE);
        OH_AVFormat_SetIntValue(format_, OH_MD_KEY_COMPLIANCE_LEVEL, OPUS_COMPLIANCE_LEVEL);
        OH_AVFormat_SetIntValue(format_, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, AudioSampleFormat::SAMPLE_S16LE);

        codec_ = OH_AudioCodec_CreateByMime(OH_AVCODEC_MIMETYPE_AUDIO_OPUS, true);
        ASSERT_NE(codec_, nullptr);
        ASSERT_EQ(OH_AudioCodec_Configure(codec_, format_), AV_ERR_OK);
        EncoderRun();
        OH_AudioCodec_Destroy(codec_);
    }
}

HWTEST_F(AudioSyncModeCapiUnitTest, aac_decode_01, TestSize.Level1)
{
    inputFile_ = std::make_unique<std::ifstream>(INPUT_AAC_FILE_PATH, std::ios::binary);
    outputFile_ = std::make_unique<std::ofstream>(OUTPUT_AAC_PCM_FILE_PATH, std::ios::binary);
    ASSERT_EQ(inputFile_->is_open(), true);
    ASSERT_EQ(outputFile_->is_open(), true);

    codec_ = OH_AudioCodec_CreateByMime(OH_AVCODEC_MIMETYPE_AUDIO_AAC, false);
    ASSERT_NE(codec_, nullptr);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_AAC_IS_ADTS, 1);
    ASSERT_EQ(OH_AudioCodec_Configure(codec_, format_), AV_ERR_OK);
    DecoderRun();
    EXPECT_EQ(decodeInputFrameCnt_, outputFrameCnt_);
    OH_AudioCodec_Destroy(codec_);
}

HWTEST_F(AudioSyncModeCapiUnitTest, aac_decode_02, TestSize.Level1)
{
    inputFile_ = std::make_unique<std::ifstream>(INPUT_AAC_FILE_PATH, std::ios::binary);
    outputFile_ = std::make_unique<std::ofstream>(OUTPUT_AAC_PCM_FILE_PATH, std::ios::binary);
    ASSERT_EQ(inputFile_->is_open(), true);
    ASSERT_EQ(outputFile_->is_open(), true);

    codec_ = OH_AudioCodec_CreateByMime(OH_AVCODEC_MIMETYPE_AUDIO_AAC, false);
    ASSERT_NE(codec_, nullptr);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_AAC_IS_ADTS, 1);
    ASSERT_EQ(OH_AudioCodec_Configure(codec_, format_), AV_ERR_OK);
    DecoderRunIndependentOutput();
    EXPECT_EQ(decodeInputFrameCnt_, outputFrameCnt_);
    OH_AudioCodec_Destroy(codec_);
}

HWTEST_F(AudioSyncModeCapiUnitTest, flac_decode_01, TestSize.Level1)
{
    inputFile_ = std::make_unique<std::ifstream>(INPUT_FLAC_FILE_PATH, std::ios::binary);
    outputFile_ = std::make_unique<std::ofstream>(OUTPUT_FLAC_PCM_FILE_PATH, std::ios::binary);
    ASSERT_EQ(inputFile_->is_open(), true);
    ASSERT_EQ(outputFile_->is_open(), true);

    codec_ = OH_AudioCodec_CreateByMime(OH_AVCODEC_MIMETYPE_AUDIO_FLAC, false);
    ASSERT_NE(codec_, nullptr);
    ASSERT_EQ(OH_AudioCodec_Configure(codec_, format_), AV_ERR_OK);
    DecoderRun();
    EXPECT_EQ(decodeInputFrameCnt_, outputFrameCnt_);
    OH_AudioCodec_Destroy(codec_);
}

HWTEST_F(AudioSyncModeCapiUnitTest, flac_decode_02, TestSize.Level1)
{
    inputFile_ = std::make_unique<std::ifstream>(INPUT_FLAC_FILE_PATH, std::ios::binary);
    outputFile_ = std::make_unique<std::ofstream>(OUTPUT_FLAC_PCM_FILE_PATH, std::ios::binary);
    ASSERT_EQ(inputFile_->is_open(), true);
    ASSERT_EQ(outputFile_->is_open(), true);
    inTimeout_ = -2;
    outTimeout_ = -10;

    codec_ = OH_AudioCodec_CreateByMime(OH_AVCODEC_MIMETYPE_AUDIO_FLAC, false);
    ASSERT_NE(codec_, nullptr);
    ASSERT_EQ(OH_AudioCodec_Configure(codec_, format_), AV_ERR_OK);
    DecoderRun();
    EXPECT_EQ(decodeInputFrameCnt_, outputFrameCnt_);
    OH_AudioCodec_Destroy(codec_);
}

HWTEST_F(AudioSyncModeCapiUnitTest, amrnb_decode_01, TestSize.Level1)
{
    inputFile_ = std::make_unique<std::ifstream>(INPUT_AMRNB_FILE_PATH, std::ios::binary);
    outputFile_ = std::make_unique<std::ofstream>(OUTPUT_AMRNB_PCM_FILE_PATH, std::ios::binary);
    ASSERT_EQ(inputFile_->is_open(), true);
    ASSERT_EQ(outputFile_->is_open(), true);

    codec_ = OH_AudioCodec_CreateByMime(OH_AVCODEC_MIMETYPE_AUDIO_AMR_NB, false);
    ASSERT_NE(codec_, nullptr);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_AUD_CHANNEL_COUNT, 1);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_AUD_SAMPLE_RATE, 8000);
    ASSERT_EQ(OH_AudioCodec_Configure(codec_, format_), AV_ERR_OK);
    DecoderRun();
    EXPECT_EQ(decodeInputFrameCnt_, outputFrameCnt_);
    OH_AudioCodec_Destroy(codec_);
}

HWTEST_F(AudioSyncModeCapiUnitTest, amrwb_decode_01, TestSize.Level1)
{
    inputFile_ = std::make_unique<std::ifstream>(INPUT_AMRWB_FILE_PATH, std::ios::binary);
    outputFile_ = std::make_unique<std::ofstream>(OUTPUT_AMRWB_PCM_FILE_PATH, std::ios::binary);
    ASSERT_EQ(inputFile_->is_open(), true);
    ASSERT_EQ(outputFile_->is_open(), true);

    codec_ = OH_AudioCodec_CreateByMime(OH_AVCODEC_MIMETYPE_AUDIO_AMR_WB, false);
    ASSERT_NE(codec_, nullptr);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_AUD_CHANNEL_COUNT, 1);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_AUD_SAMPLE_RATE, 16000);
    ASSERT_EQ(OH_AudioCodec_Configure(codec_, format_), AV_ERR_OK);
    DecoderRun();
    EXPECT_EQ(decodeInputFrameCnt_, outputFrameCnt_);
    OH_AudioCodec_Destroy(codec_);
}

HWTEST_F(AudioSyncModeCapiUnitTest, g711mu_decode_01, TestSize.Level1)
{
    inputFile_ = std::make_unique<std::ifstream>(INPUT_G711MU_FILE_PATH, std::ios::binary);
    outputFile_ = std::make_unique<std::ofstream>(OUTPUT_G711MU_PCM_FILE_PATH, std::ios::binary);
    ASSERT_EQ(inputFile_->is_open(), true);
    ASSERT_EQ(outputFile_->is_open(), true);

    codec_ = OH_AudioCodec_CreateByMime(OH_AVCODEC_MIMETYPE_AUDIO_G711MU, false);
    ASSERT_NE(codec_, nullptr);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_AUD_CHANNEL_COUNT, 1);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_AUD_SAMPLE_RATE, 8000);
    ASSERT_EQ(OH_AudioCodec_Configure(codec_, format_), AV_ERR_OK);
    DecoderRun();
    EXPECT_EQ(decodeInputFrameCnt_, outputFrameCnt_);
    OH_AudioCodec_Destroy(codec_);
}

HWTEST_F(AudioSyncModeCapiUnitTest, ape_decode_01, TestSize.Level1)
{
    inputFile_ = std::make_unique<std::ifstream>(INPUT_APE_FILE_PATH, std::ios::binary);
    outputFile_ = std::make_unique<std::ofstream>(OUTPUT_APE_PCM_FILE_PATH, std::ios::binary);
    ASSERT_EQ(inputFile_->is_open(), true);
    ASSERT_EQ(outputFile_->is_open(), true);

    codec_ = OH_AudioCodec_CreateByMime(OH_AVCODEC_MIMETYPE_AUDIO_APE, false);
    ASSERT_NE(codec_, nullptr);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_AUD_CHANNEL_COUNT, 1);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_AUD_SAMPLE_RATE, 8000);
    ASSERT_EQ(OH_AudioCodec_Configure(codec_, format_), AV_ERR_OK);
    DecoderRun();
    OH_AudioCodec_Destroy(codec_);
}

HWTEST_F(AudioSyncModeCapiUnitTest, mp3_decode_01, TestSize.Level1)
{
    inputFile_ = std::make_unique<std::ifstream>(INPUT_MP3_FILE_PATH, std::ios::binary);
    outputFile_ = std::make_unique<std::ofstream>(OUTPUT_MP3_PCM_FILE_PATH, std::ios::binary);
    ASSERT_EQ(inputFile_->is_open(), true);
    ASSERT_EQ(outputFile_->is_open(), true);

    codec_ = OH_AudioCodec_CreateByMime(OH_AVCODEC_MIMETYPE_AUDIO_MPEG, false);
    ASSERT_NE(codec_, nullptr);
    ASSERT_EQ(OH_AudioCodec_Configure(codec_, format_), AV_ERR_OK);
    DecoderRun();
    EXPECT_EQ(decodeInputFrameCnt_, outputFrameCnt_);
    OH_AudioCodec_Destroy(codec_);
}

HWTEST_F(AudioSyncModeCapiUnitTest, ac3_decode_01, TestSize.Level1)
{
    inputFile_ = std::make_unique<std::ifstream>(INPUT_AC3_FILE_PATH, std::ios::binary);
    outputFile_ = std::make_unique<std::ofstream>(OUTPUT_AC3_PCM_FILE_PATH, std::ios::binary);
    ASSERT_EQ(inputFile_->is_open(), true);
    ASSERT_EQ(outputFile_->is_open(), true);

    codec_ = OH_AudioCodec_CreateByName(std::string(AVCodecCodecName::AUDIO_DECODER_AC3_NAME).data());
    ASSERT_NE(codec_, nullptr);
    ASSERT_EQ(OH_AudioCodec_Configure(codec_, format_), AV_ERR_OK);
    DecoderRun();
    EXPECT_EQ(decodeInputFrameCnt_, outputFrameCnt_);
    OH_AudioCodec_Destroy(codec_);
}

HWTEST_F(AudioSyncModeCapiUnitTest, vorbis_decode_01, TestSize.Level1)
{
    inputFile_ = std::make_unique<std::ifstream>(INPUT_VORBIS_FILE_PATH, std::ios::binary);
    outputFile_ = std::make_unique<std::ofstream>(OUTPUT_VORBIS_PCM_FILE_PATH, std::ios::binary);
    ASSERT_EQ(inputFile_->is_open(), true);
    ASSERT_EQ(outputFile_->is_open(), true);

    codec_ = OH_AudioCodec_CreateByMime(OH_AVCODEC_MIMETYPE_AUDIO_VORBIS, false);
    ASSERT_NE(codec_, nullptr);
    int64_t extradataSize;
    inputFile_->read(reinterpret_cast<char *>(&extradataSize), sizeof(int64_t));
    ASSERT_EQ(inputFile_->gcount(), sizeof(int64_t));
    ASSERT_EQ(extradataSize > 0, true);
    char buffer[extradataSize];
    inputFile_->read(buffer, extradataSize);
    ASSERT_EQ(inputFile_->gcount(), extradataSize);
    OH_AVFormat_SetBuffer(format_, OH_MD_KEY_CODEC_CONFIG, (uint8_t *)buffer, extradataSize);
    ASSERT_EQ(OH_AudioCodec_Configure(codec_, format_), AV_ERR_OK);
    DecoderRun();
    // vorbis decode check output less than input frame
    OH_AudioCodec_Destroy(codec_);
}

HWTEST_F(AudioSyncModeCapiUnitTest, opus_decode_01, TestSize.Level1)
{
    if (CheckOpusSoFunc()) {
        inputFile_ = std::make_unique<std::ifstream>(INPUT_OPUS_FILE_PATH, std::ios::binary);
        outputFile_ = std::make_unique<std::ofstream>(OUTPUT_OPUS_PCM_FILE_PATH, std::ios::binary);
        ASSERT_EQ(inputFile_->is_open(), true);
        ASSERT_EQ(outputFile_->is_open(), true);

        codec_ = OH_AudioCodec_CreateByMime(OH_AVCODEC_MIMETYPE_AUDIO_OPUS, false);
        ASSERT_NE(codec_, nullptr);
        OH_AVFormat_SetIntValue(format_, OH_MD_KEY_AUD_SAMPLE_RATE, 8000);
        ASSERT_EQ(OH_AudioCodec_Configure(codec_, format_), AV_ERR_OK);
        DecoderRun();
        EXPECT_EQ(decodeInputFrameCnt_, outputFrameCnt_);
        OH_AudioCodec_Destroy(codec_);
    }
}

HWTEST_F(AudioSyncModeCapiUnitTest, aaclc_decode_formatchanged, TestSize.Level1)
{
    inputFile_ = std::make_unique<std::ifstream>(INPUT_AAC_LC_ADTS_FILE_PATH, std::ios::binary);
    outputFile_ = std::make_unique<std::ofstream>(OUTPUT_AAC_LC_ADTS_FILE_PATH, std::ios::binary);
    ASSERT_EQ(inputFile_->is_open(), true);
    ASSERT_EQ(outputFile_->is_open(), true);

    codec_ = OH_AudioCodec_CreateByMime(OH_AVCODEC_MIMETYPE_AUDIO_AAC, false);
    ASSERT_NE(codec_, nullptr);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_AUD_SAMPLE_RATE, 8000);
    ASSERT_EQ(OH_AudioCodec_Configure(codec_, format_), AV_ERR_OK);
    DecoderRun();
    EXPECT_EQ(outputFormatChangedTimes, 1);
    EXPECT_EQ(outputSampleRate, SAMPLE_RATE);
    EXPECT_EQ(outputChannels, CHANNEL_COUNT);
    EXPECT_EQ(decodeInputFrameCnt_, outputFrameCnt_);
    OH_AudioCodec_Destroy(codec_);
}

HWTEST_F(AudioSyncModeCapiUnitTest, sync_invalid_01, TestSize.Level1)
{
    codec_ = OH_AudioCodec_CreateByMime(OH_AVCODEC_MIMETYPE_AUDIO_AAC, false);
    ASSERT_NE(codec_, nullptr);
    uint32_t index = 0;
    EXPECT_EQ(OH_AudioCodec_QueryInputBuffer(codec_, &index, 1000), AV_ERR_INVALID_STATE);
    EXPECT_EQ(OH_AudioCodec_QueryOutputBuffer(codec_, &index, 1000), AV_ERR_INVALID_STATE);
    EXPECT_EQ(OH_AudioCodec_GetInputBuffer(codec_, 0), nullptr);
    EXPECT_EQ(OH_AudioCodec_GetOutputBuffer(codec_, 0), nullptr);
    EXPECT_EQ(OH_AudioCodec_QueryInputBuffer(codec_, nullptr, 1000), AV_ERR_INVALID_VAL);
    EXPECT_EQ(OH_AudioCodec_QueryOutputBuffer(codec_, nullptr, 1000), AV_ERR_INVALID_VAL);
    OH_AudioCodec_Destroy(codec_);

    OH_AVCodec *invalidCodec = OH_AudioCodec_CreateByMime("invalid mime", false);
    EXPECT_EQ(OH_AudioCodec_QueryInputBuffer(invalidCodec, &index, 1000), AV_ERR_INVALID_VAL);
    EXPECT_EQ(OH_AudioCodec_QueryOutputBuffer(invalidCodec, &index, 1000), AV_ERR_INVALID_VAL);
}

}
}
