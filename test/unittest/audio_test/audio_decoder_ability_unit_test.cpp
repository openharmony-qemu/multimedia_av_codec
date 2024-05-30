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
#include "avcodec_codec_name.h"
#include "avcodec_common.h"
#include "avcodec_errors.h"
#include "media_description.h"
#include "native_avcodec_base.h"
#include "native_avformat.h"
#include "native_avcodec_audiodecoder.h"
#include "securec.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS::MediaAVCodec;

namespace {
const string CODEC_MP3_NAME = std::string(AVCodecCodecName::AUDIO_DECODER_MP3_NAME);
const string CODEC_AAC_NAME = std::string(AVCodecCodecName::AUDIO_DECODER_AAC_NAME);
const string CODEC_OGG_NAME = std::string(AVCodecCodecName::AUDIO_DECODER_VORBIS_NAME);
const string CODEC_FLAC_NAME = std::string(AVCodecCodecName::AUDIO_DECODER_FLAC_NAME);
const string CODEC_AMRWB_NAME = std::string(AVCodecCodecName::AUDIO_DECODER_AMRWB_NAME);
const string CODEC_AMRNB_NAME = std::string(AVCodecCodecName::AUDIO_DECODER_AMRNB_NAME);
const string CODEC_OPUS_NAME = std::string(AVCodecCodecName::AUDIO_DECODER_OPUS_NAME);
const string CODEC_G711MU_NAME = std::string(AVCodecCodecName::AUDIO_DECODER_G711MU_NAME);
const string INPUT_SOURCE_PATH = "/data/test/media/";
constexpr string_view OPUS_SO_FILE_PATH = "/system/lib64/libav_codec_ext_base.z.so";
const int MP3_TESTCASES_NUMS = 15;
const int FLAC_TESTCASES_NUMS = 8;
const int OGG_TESTCASES_NUMS = 11;
const int AAC_TESTCASES_NUMS = 6;

const string INPUT_MP3_FILE_SOURCE_PATH[][5] = {{"MP3_11k_2c_20kb.dat", "11025", "2", "32000", "16"},
                                                {"MP3_16k_2c_32kb.dat", "16000", "2", "32000", "16"},
                                                {"MP3_16k_2c_40kb.dat", "16000", "2", "40000", "16"},
                                                {"MP3_22k_1c_32kb.dat", "22050", "1", "32000", "16"},
                                                {"MP3_24k_2c_80kb.dat", "24000", "2", "80000", "16"},
                                                {"MP3_44k_1c_128kb.dat", "44100", "1", "128000", "16"},
                                                {"MP3_44k_2c_128kb.dat", "44100", "2", "128000", "16"},
                                                {"MP3_44k_2c_160kb.dat", "44100", "2", "160000", "16"},
                                                {"MP3_44k_2c_320kb.dat", "44100", "2", "320000", "16"},
                                                {"MP3_48k_1c_128kb.dat", "48000", "1", "128000", "16"},
                                                {"MP3_48k_1c_320kb.dat", "48000", "1", "320000", "16"},
                                                {"MP3_48k_2c_320kb.dat", "48000", "2", "320000", "16"},
                                                {"MP3_8k_1c_8kb.dat", "8000", "1", "32000", "16"},
                                                {"MP3_8k_2c_16kb.dat", "16000", "2", "32000", "16"},
                                                {"MP3_8k_2c_18kb.dat", "16000", "2", "32000", "16"}};

const string INPUT_FLAC_FILE_SOURCE_PATH[][5] = {{"FLAC_6k_1c_xxkb.dat", "8000", "1", "320000", "16"},
                                                 {"FLAC_6k_2c_xxkb.dat", "8000", "2", "320000", "16"},
                                                 {"FLAC_6k_3c_xxkb.dat", "8000", "3", "320000", "16"},
                                                 {"FLAC_6k_4c_xxkb.dat", "8000", "4", "320000", "16"},
                                                 {"FLAC_6k_5c_xxkb.dat", "8000", "5", "320000", "16"},
                                                 {"FLAC_6k_6c_xxkb.dat", "8000", "6", "320000", "16"},
                                                 {"FLAC_6k_7c_xxkb.dat", "8000", "7", "320000", "16"},
                                                 {"FLAC_6k_8c_xxkb.dat", "8000", "8", "320000", "16"}};

const string INPUT_OGG_FILE_SOURCE_PATH[][5] = {{"OGG_192k_1c_100pkb.dat", "176400", "1", "320000", "16"},
                                                {"OGG_192k_1c_xxkb.dat", "176400", "1", "320000", "16"},
                                                {"OGG_192k_2c_xxkb.dat", "176400", "2", "320000", "16"},
                                                {"OGG_192k_3c_xxkb.dat", "192000", "3", "320000", "16"},
                                                {"OGG_192k_4c_xxkb.dat", "192000", "4", "320000", "16"},
                                                {"OGG_192k_5c_xxkb.dat", "192000", "5", "320000", "16"},
                                                {"OGG_192k_6c_xxkb.dat", "192000", "6", "320000", "16"},
                                                {"OGG_192k_7c_xxkb.dat", "192000", "7", "320000", "16"},
                                                {"OGG_192k_8c_xxkb.dat", "192000", "8", "320000", "16"},
                                                {"OGG_8k_1c_0pkb.dat", "8000", "1", "32000", "16"},
                                                {"OGG_8k_1c_100pkb.dat", "8000", "1", "32000", "16"}};

const string INPUT_OGG_FILE_WITH_HEADER[][5] = {{"OGG_44k_2c_with_header.dat", "44100", "2", "128000", "16"}};

const string INPUT_AAC_FILE_SOURCE_PATH[][5] = {{"AAC_44k_1c_xxkb.dat", "44100", "1", "32000", "16"},
                                                {"AAC_44k_2c_xxkb.dat", "44100", "2", "320000", "16"},
                                                {"AAC_44k_6c_xxkb.dat", "44100", "6", "320000", "16"},
                                                {"AAC_48k_1c_xxkb.dat", "48000", "1", "320000", "16"},
                                                {"AAC_48k_2c_xxkb.dat", "48000", "2", "320000", "16"},
                                                {"AAC_48k_6c_xxkb.dat", "48000", "6", "192000", "16"}};

std::vector<std::vector<string>>  INPUT_AMRWB_FILE_SOURCE_PATH = {{"voice_amrwb_6600.dat", "16000", "1"},
                                                                  {"voice_amrwb_8850.dat", "16000", "1"},
                                                                  {"voice_amrwb_12650.dat", "16000", "1"},
                                                                  {"voice_amrwb_14250.dat", "16000", "1"},
                                                                  {"voice_amrwb_15850.dat", "16000", "1"},
                                                                  {"voice_amrwb_18250.dat", "16000", "1"},
                                                                  {"voice_amrwb_19850.dat", "16000", "1"},
                                                                  {"voice_amrwb_23050.dat", "16000", "1"},
                                                                  {"voice_amrwb_23850.dat", "16000", "1"}};

std::vector<std::vector<string>>  INPUT_AMRNB_FILE_SOURCE_PATH = {{"voice_amrnb_4750.dat", "8000", "1"},
                                                                  {"voice_amrnb_5150.dat", "8000", "1"},
                                                                  {"voice_amrnb_5900.dat", "8000", "1"},
                                                                  {"voice_amrnb_6700.dat", "8000", "1"},
                                                                  {"voice_amrnb_7400.dat", "8000", "1"},
                                                                  {"voice_amrnb_7950.dat", "8000", "1"},
                                                                  {"voice_amrnb_10200.dat", "8000", "1"},
                                                                  {"voice_amrnb_12200.dat", "8000", "1"}};
                                                                
std::vector<std::vector<string>>  INPUT_OPUS_FILE_SOURCE_PATH = {{"voice_opus.dat", "8000", "1"},
                                                                 {"voice_opus.dat", "12000", "1"},
                                                                 {"voice_opus.dat", "16000", "1"},
                                                                 {"voice_opus.dat", "24000", "1"},
                                                                 {"voice_opus.dat", "48000", "1"},
                                                                 {"voice_opus.dat", "8000", "2"},
                                                                 {"voice_opus.dat", "16000", "2"},
                                                                 {"voice_opus.dat", "48000", "2"}};

std::vector<std::vector<string>>  INPUT_G711MU_FILE_SOURCE_PATH = {{"g711mu_8kHz.dat", "8000", "1"}};
constexpr string_view OUTPUT_PCM_FILE_PATH = "/data/test/media/out.pcm";
} // namespace

namespace OHOS {
namespace MediaAVCodec {
class ADecSignal {
public:
    std::mutex inMutex_;
    std::mutex outMutex_;
    std::mutex startMutex_;
    std::condition_variable inCond_;
    std::condition_variable outCond_;
    std::condition_variable startCond_;
    std::queue<uint32_t> inQueue_;
    std::queue<uint32_t> outQueue_;
    std::queue<OH_AVMemory *> inBufferQueue_;
    std::queue<OH_AVMemory *> outBufferQueue_;
    std::queue<OH_AVCodecBufferAttr> attrQueue_;
};

static void OnError(OH_AVCodec *codec, int32_t errorCode, void *userData)
{
    (void)codec;
    (void)errorCode;
    (void)userData;
    cout << "Error received, errorCode:" << errorCode << endl;
}

static void OnOutputFormatChanged(OH_AVCodec *codec, OH_AVFormat *format, void *userData)
{
    (void)codec;
    (void)format;
    (void)userData;
    cout << "OnOutputFormatChanged received" << endl;
}

static void OnInputBufferAvailable(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, void *userData)
{
    (void)codec;
    ADecSignal *signal = static_cast<ADecSignal *>(userData);
    unique_lock<mutex> lock(signal->inMutex_);
    signal->inQueue_.push(index);
    signal->inBufferQueue_.push(data);
    signal->inCond_.notify_all();
}

static void OnOutputBufferAvailable(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, OH_AVCodecBufferAttr *attr,
                                    void *userData)
{
    (void)codec;
    ADecSignal *signal = static_cast<ADecSignal *>(userData);
    unique_lock<mutex> lock(signal->outMutex_);
    signal->outQueue_.push(index);
    signal->outBufferQueue_.push(data);
    if (attr) {
        signal->attrQueue_.push(*attr);
    } else {
        cout << "OnOutputBufferAvailable error, attr is nullptr!" << endl;
    }
    signal->outCond_.notify_all();
}

class AudioCodeCapiDecoderUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
    int32_t InitFile(const string &codecName, string inputTestFile);
    void InputFunc();
    void OutputFunc();
    void HandleInputEOS(const uint32_t index);
    int32_t HandleNormalInput(const uint32_t &index, const int64_t pts, const size_t size);
    int32_t Stop();
    void Release();
    int32_t SetVorbisHeader();
    int32_t CheckSoFunc();

protected:
    std::unique_ptr<std::ifstream> soFile_;
    std::atomic<bool> isRunning_ = false;
    std::unique_ptr<std::thread> inputLoop_;
    std::unique_ptr<std::thread> outputLoop_;
    struct OH_AVCodecAsyncCallback cb_;
    ADecSignal *signal_;
    OH_AVCodec *audioDec_;
    OH_AVFormat *format_;
    bool isFirstFrame_ = true;
    std::ifstream inputFile_;
    std::ofstream pcmOutputFile_;
    bool vorbisHasExtradata_ = true;
};

void AudioCodeCapiDecoderUnitTest::SetUpTestCase(void)
{
    cout << "[SetUpTestCase]: " << endl;
}

void AudioCodeCapiDecoderUnitTest::TearDownTestCase(void)
{
    cout << "[TearDownTestCase]: " << endl;
}

void AudioCodeCapiDecoderUnitTest::SetUp(void)
{
    cout << "[SetUp]: SetUp!!!" << endl;
}

void AudioCodeCapiDecoderUnitTest::TearDown(void)
{
    cout << "[TearDown]: over!!!" << endl;

    if (signal_) {
        delete signal_;
        signal_ = nullptr;
    }
    if (inputFile_.is_open()) {
        inputFile_.close();
    }
    if (pcmOutputFile_.is_open()) {
        pcmOutputFile_.close();
    }
    sleep(1);
}

void AudioCodeCapiDecoderUnitTest::Release()
{
    Stop();
    if (signal_) {
        delete signal_;
        signal_ = nullptr;
    }
    if (inputFile_.is_open()) {
        inputFile_.close();
    }
    if (pcmOutputFile_.is_open()) {
        pcmOutputFile_.close();
    }
    if (format_) {
        OH_AVFormat_Destroy(format_);
    }
    OH_AudioDecoder_Destroy(audioDec_);
}

void AudioCodeCapiDecoderUnitTest::HandleInputEOS(const uint32_t index)
{
    OH_AVCodecBufferAttr info;
    info.size = 0;
    info.offset = 0;
    info.pts = 0;
    info.flags = AVCODEC_BUFFER_FLAGS_EOS;
    OH_AudioDecoder_PushInputData(audioDec_, index, info);
    signal_->inBufferQueue_.pop();
    signal_->inQueue_.pop();
}

int32_t AudioCodeCapiDecoderUnitTest::HandleNormalInput(const uint32_t &index, const int64_t pts, const size_t size)
{
    OH_AVCodecBufferAttr info;
    info.size = size;
    info.offset = 0;
    info.pts = pts;

    int32_t ret = AV_ERR_OK;
    if (isFirstFrame_) {
        info.flags = AVCODEC_BUFFER_FLAGS_CODEC_DATA;
        ret = OH_AudioDecoder_PushInputData(audioDec_, index, info);
        EXPECT_EQ(AV_ERR_OK, ret);
        isFirstFrame_ = false;
    } else {
        info.flags = AVCODEC_BUFFER_FLAGS_NONE;
        ret = OH_AudioDecoder_PushInputData(audioDec_, index, info);
        EXPECT_EQ(AV_ERR_OK, ret);
    }
    signal_->inQueue_.pop();
    signal_->inBufferQueue_.pop();
    return ret;
}

void AudioCodeCapiDecoderUnitTest::InputFunc()
{
    int64_t size;
    int64_t pts;

    while (isRunning_.load()) {
        unique_lock<mutex> lock(signal_->inMutex_);
        signal_->inCond_.wait(lock, [this]() { return (signal_->inQueue_.size() > 0 || !isRunning_.load()); });

        if (!isRunning_.load()) {
            break;
        }

        uint32_t index = signal_->inQueue_.front();
        auto buffer = signal_->inBufferQueue_.front();
        if (buffer == nullptr) {
            cout << "Fatal: GetInputBuffer fail" << endl;
            break;
        }
        inputFile_.read(reinterpret_cast<char *>(&size), sizeof(size));
        if (inputFile_.eof() || inputFile_.gcount() == 0) {
            HandleInputEOS(index);
            cout << "end buffer\n";
            break;
        }
        if (inputFile_.gcount() != sizeof(size)) {
            cout << "Fatal: read size fail" << endl;
            break;
        }
        inputFile_.read(reinterpret_cast<char *>(&pts), sizeof(pts));
        if (inputFile_.gcount() != sizeof(pts)) {
            cout << "Fatal: read size fail" << endl;
            break;
        }
        inputFile_.read((char *)OH_AVMemory_GetAddr(buffer), size);
        if (inputFile_.gcount() != size) {
            cout << "Fatal: read buffer fail" << endl;
            break;
        }

        int32_t ret = HandleNormalInput(index, pts, size);
        if (ret != AV_ERR_OK) {
            cout << "Fatal error, exit" << endl;
            break;
        }
    }
    inputFile_.close();
}

void AudioCodeCapiDecoderUnitTest::OutputFunc()
{
    if (!pcmOutputFile_.is_open()) {
        std::cout << "open " << OUTPUT_PCM_FILE_PATH << " failed!" << std::endl;
    }
    while (isRunning_.load()) {
        unique_lock<mutex> lock(signal_->outMutex_);
        signal_->outCond_.wait(lock, [this]() { return (signal_->outQueue_.size() > 0 || !isRunning_.load()); });

        if (!isRunning_.load()) {
            cout << "wait to stop, exit" << endl;
            break;
        }

        uint32_t index = signal_->outQueue_.front();
        OH_AVCodecBufferAttr attr = signal_->attrQueue_.front();
        OH_AVMemory *data = signal_->outBufferQueue_.front();
        if (data != nullptr) {
            pcmOutputFile_.write(reinterpret_cast<char *>(OH_AVMemory_GetAddr(data)), attr.size);
        }

        if (attr.flags == AVCODEC_BUFFER_FLAGS_EOS) {
            cout << "decode eos" << endl;
            isRunning_.store(false);
            signal_->startCond_.notify_all();
        }
        signal_->outBufferQueue_.pop();
        signal_->attrQueue_.pop();
        signal_->outQueue_.pop();
        EXPECT_EQ(AV_ERR_OK, OH_AudioDecoder_FreeOutputData(audioDec_, index));
    }

    pcmOutputFile_.close();
}

int32_t AudioCodeCapiDecoderUnitTest::Stop()
{
    isRunning_.store(false);
    if (inputLoop_ != nullptr && inputLoop_->joinable()) {
        unique_lock<mutex> lock(signal_->inMutex_);
        signal_->inCond_.notify_all();
        lock.unlock();
        inputLoop_->join();
    }

    if (outputLoop_ != nullptr && outputLoop_->joinable()) {
        unique_lock<mutex> lock(signal_->outMutex_);
        signal_->outCond_.notify_all();
        lock.unlock();
        outputLoop_->join();
    }
    return OH_AudioDecoder_Stop(audioDec_);
}

int32_t AudioCodeCapiDecoderUnitTest::SetVorbisHeader()
{
    // set identification header
    int64_t headerSize;
    inputFile_.read(reinterpret_cast<char *>(&headerSize), sizeof(int64_t));
    if (headerSize < 0) {
        return AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
    }
    char idBuffer[headerSize];
    inputFile_.read(idBuffer, headerSize);
    if (inputFile_.gcount() != headerSize) {
        cout << "read extradata bytes error" << endl;
        return AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
    }
    OH_AVFormat_SetBuffer(format_, MediaDescriptionKey::MD_KEY_IDENTIFICATION_HEADER.data(), (uint8_t *)idBuffer,
                          headerSize);

    // set setup header
    inputFile_.read(reinterpret_cast<char *>(&headerSize), sizeof(int64_t));
    if (headerSize < 0) {
        return AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
    }
    char setupBuffer[headerSize];
    inputFile_.read(setupBuffer, headerSize);
    if (inputFile_.gcount() != headerSize) {
        cout << "read extradata bytes error" << endl;
        return AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
    }
    OH_AVFormat_SetBuffer(format_, MediaDescriptionKey::MD_KEY_SETUP_HEADER.data(), (uint8_t *)setupBuffer,
                          headerSize);
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

int32_t AudioCodeCapiDecoderUnitTest::CheckSoFunc()
{
    soFile_ = std::make_unique<std::ifstream>(OPUS_SO_FILE_PATH, std::ios::binary);
    if (!soFile_->is_open()) {
        cout << "Fatal: Open so file failed" << endl;
        return false;
    }
    soFile_->close();
    return true;
}

int32_t AudioCodeCapiDecoderUnitTest::InitFile(const string &codecName, string inputTestFile)
{
    format_ = OH_AVFormat_Create();
    if (codecName.compare(CODEC_AAC_NAME) == 0) {
        OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_AAC_IS_ADTS.data(), 1);
    }
    audioDec_ = OH_AudioDecoder_CreateByName(codecName.c_str());
    if (audioDec_ == nullptr) {
        return AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
    }

    inputFile_.open(INPUT_SOURCE_PATH + inputTestFile, std::ios::binary);
    pcmOutputFile_.open(OUTPUT_PCM_FILE_PATH.data(), std::ios::out | std::ios::binary);
    if (!inputFile_.is_open()) {
        cout << "Fatal: open input file failed" << endl;
        return AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
    }
    if (!pcmOutputFile_.is_open()) {
        cout << "Fatal: open output file failed" << endl;
        return AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
    }

    signal_ = new ADecSignal();
    cb_ = {&OnError, &OnOutputFormatChanged, &OnInputBufferAvailable, &OnOutputBufferAvailable};
    int32_t ret = OH_AudioDecoder_SetCallback(audioDec_, cb_, signal_);
    if (ret != AVCodecServiceErrCode::AVCS_ERR_OK) {
        return AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
    }

    if (codecName.compare(CODEC_OGG_NAME) == 0) {
        if (!vorbisHasExtradata_) {
            return SetVorbisHeader();
        }
        int64_t extradataSize;
        inputFile_.read(reinterpret_cast<char *>(&extradataSize), sizeof(int64_t));
        if (extradataSize < 0) {
            return AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
        }
        char buffer[extradataSize];
        inputFile_.read(buffer, extradataSize);
        if (inputFile_.gcount() != extradataSize) {
            cout << "read extradata bytes error" << endl;
        }
        OH_AVFormat_SetBuffer(format_, MediaDescriptionKey::MD_KEY_CODEC_CONFIG.data(), (uint8_t *)buffer,
                              extradataSize);
    }

    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

HWTEST_F(AudioCodeCapiDecoderUnitTest, audioDecoder_Normalcase_01, TestSize.Level1)
{
    bool result;
    for (int i = 0; i < MP3_TESTCASES_NUMS; i++) {
        cout << "decode start " << INPUT_MP3_FILE_SOURCE_PATH[i][0] << endl;
        ASSERT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, InitFile(CODEC_MP3_NAME, INPUT_MP3_FILE_SOURCE_PATH[i][0]));

        OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(),
                                stoi(INPUT_MP3_FILE_SOURCE_PATH[i][1]));
        OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(),
                                stoi(INPUT_MP3_FILE_SOURCE_PATH[i][2]));
        OH_AVFormat_SetLongValue(format_, MediaDescriptionKey::MD_KEY_BITRATE.data(),
                                 stol(INPUT_MP3_FILE_SOURCE_PATH[i][3]));
        OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE.data(),
                                stoi(INPUT_MP3_FILE_SOURCE_PATH[i][4]));
        OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE.data(), (uint32_t)16);

        EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioDecoder_Configure(audioDec_, format_));

        isRunning_.store(true);
        inputLoop_ = make_unique<thread>(&AudioCodeCapiDecoderUnitTest::InputFunc, this);
        EXPECT_NE(nullptr, inputLoop_);
        outputLoop_ = make_unique<thread>(&AudioCodeCapiDecoderUnitTest::OutputFunc, this);
        EXPECT_NE(nullptr, outputLoop_);

        EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioDecoder_Start(audioDec_));
        {
            unique_lock<mutex> lock(signal_->startMutex_);
            signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
        }
        EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Stop());
        result = std::filesystem::file_size(OUTPUT_PCM_FILE_PATH) < 20;
        EXPECT_EQ(result, false) << "error occur, decode fail" << INPUT_MP3_FILE_SOURCE_PATH[i][0] << endl;

        Release();
    }
}

HWTEST_F(AudioCodeCapiDecoderUnitTest, audioDecoder_Normalcase_02, TestSize.Level1)
{
    bool result;
    for (int i = 0; i < FLAC_TESTCASES_NUMS; i++) {
        cout << "decode start " << INPUT_FLAC_FILE_SOURCE_PATH[i][0] << endl;
        ASSERT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, InitFile(CODEC_FLAC_NAME, INPUT_FLAC_FILE_SOURCE_PATH[i][0]));
        OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(),
                                stoi(INPUT_FLAC_FILE_SOURCE_PATH[i][1]));
        OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(),
                                stoi(INPUT_FLAC_FILE_SOURCE_PATH[i][2]));
        OH_AVFormat_SetLongValue(format_, MediaDescriptionKey::MD_KEY_BITRATE.data(),
                                 stol(INPUT_FLAC_FILE_SOURCE_PATH[i][3]));
        OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE.data(),
                                    stoi(INPUT_FLAC_FILE_SOURCE_PATH[i][4]));

        EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioDecoder_Configure(audioDec_, format_));

        isRunning_.store(true);
        inputLoop_ = make_unique<thread>(&AudioCodeCapiDecoderUnitTest::InputFunc, this);
        EXPECT_NE(nullptr, inputLoop_);
        outputLoop_ = make_unique<thread>(&AudioCodeCapiDecoderUnitTest::OutputFunc, this);
        EXPECT_NE(nullptr, outputLoop_);

        EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioDecoder_Start(audioDec_));
        {
            unique_lock<mutex> lock(signal_->startMutex_);
            signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
        }
        EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Stop());
        result = std::filesystem::file_size(OUTPUT_PCM_FILE_PATH) < 20;
        EXPECT_EQ(result, false) << "error occur, decode fail" << INPUT_FLAC_FILE_SOURCE_PATH[i][0] << endl;

        Release();
    }
}

HWTEST_F(AudioCodeCapiDecoderUnitTest, oggDecodeWithHeader, TestSize.Level1)
{
    cout << "decode start " << INPUT_OGG_FILE_WITH_HEADER[0][0] << endl;
    vorbisHasExtradata_ = false;
    ASSERT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, InitFile(CODEC_OGG_NAME, INPUT_OGG_FILE_WITH_HEADER[0][0]));
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(),
                            stoi(INPUT_OGG_FILE_WITH_HEADER[0][1]));
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(),
                            stoi(INPUT_OGG_FILE_WITH_HEADER[0][2]));
    OH_AVFormat_SetLongValue(format_, MediaDescriptionKey::MD_KEY_BITRATE.data(),
                             stol(INPUT_OGG_FILE_WITH_HEADER[0][3]));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioDecoder_Configure(audioDec_, format_));

    isRunning_.store(true);
    inputLoop_ = make_unique<thread>(&AudioCodeCapiDecoderUnitTest::InputFunc, this);
    EXPECT_NE(nullptr, inputLoop_);
    outputLoop_ = make_unique<thread>(&AudioCodeCapiDecoderUnitTest::OutputFunc, this);
    EXPECT_NE(nullptr, outputLoop_);

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioDecoder_Start(audioDec_));
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Stop());
    auto result = std::filesystem::file_size(OUTPUT_PCM_FILE_PATH) < 20;
    EXPECT_EQ(result, false) << "error occur, decode fail" << INPUT_OGG_FILE_WITH_HEADER[0][0] << endl;

    Release();
}

HWTEST_F(AudioCodeCapiDecoderUnitTest, audioDecoder_Normalcase_03, TestSize.Level1)
{
    vorbisHasExtradata_ = true;
    bool result;
    for (int i = 0; i < OGG_TESTCASES_NUMS; i++) {
        cout << "decode start " << INPUT_OGG_FILE_SOURCE_PATH[i][0] << endl;
        ASSERT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, InitFile(CODEC_OGG_NAME, INPUT_OGG_FILE_SOURCE_PATH[i][0]));

        OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(),
                                stoi(INPUT_OGG_FILE_SOURCE_PATH[i][1]));
        OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(),
                                stoi(INPUT_OGG_FILE_SOURCE_PATH[i][2]));
        OH_AVFormat_SetLongValue(format_, MediaDescriptionKey::MD_KEY_BITRATE.data(),
                                 stol(INPUT_OGG_FILE_SOURCE_PATH[i][3]));
        OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE.data(),
                                    stoi(INPUT_OGG_FILE_SOURCE_PATH[i][4]));
        EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioDecoder_Configure(audioDec_, format_));

        isRunning_.store(true);
        inputLoop_ = make_unique<thread>(&AudioCodeCapiDecoderUnitTest::InputFunc, this);
        EXPECT_NE(nullptr, inputLoop_);
        outputLoop_ = make_unique<thread>(&AudioCodeCapiDecoderUnitTest::OutputFunc, this);
        EXPECT_NE(nullptr, outputLoop_);

        EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioDecoder_Start(audioDec_));
        {
            unique_lock<mutex> lock(signal_->startMutex_);
            signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
        }
        EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Stop());
        result = std::filesystem::file_size(OUTPUT_PCM_FILE_PATH) < 20;
        EXPECT_EQ(result, false) << "error occur, decode fail" << INPUT_OGG_FILE_SOURCE_PATH[i][0] << endl;

        Release();
    }
}

HWTEST_F(AudioCodeCapiDecoderUnitTest, audioDecoder_Normalcase_04, TestSize.Level1)
{
    bool result;
    for (int i = 0; i < AAC_TESTCASES_NUMS; i++) {
        cout << "decode start " << INPUT_AAC_FILE_SOURCE_PATH[i][0] << endl;
        ASSERT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, InitFile(CODEC_AAC_NAME, INPUT_AAC_FILE_SOURCE_PATH[i][0]));

        OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(),
                                stoi(INPUT_AAC_FILE_SOURCE_PATH[i][1]));
        OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(),
                                stoi(INPUT_AAC_FILE_SOURCE_PATH[i][2]));
        OH_AVFormat_SetLongValue(format_, MediaDescriptionKey::MD_KEY_BITRATE.data(),
                                 stol(INPUT_AAC_FILE_SOURCE_PATH[i][3]));
        OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE.data(),
                                    stoi(INPUT_AAC_FILE_SOURCE_PATH[i][4]));

        EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioDecoder_Configure(audioDec_, format_));

        isRunning_.store(true);
        inputLoop_ = make_unique<thread>(&AudioCodeCapiDecoderUnitTest::InputFunc, this);
        EXPECT_NE(nullptr, inputLoop_);
        outputLoop_ = make_unique<thread>(&AudioCodeCapiDecoderUnitTest::OutputFunc, this);
        EXPECT_NE(nullptr, outputLoop_);

        EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioDecoder_Start(audioDec_));
        {
            unique_lock<mutex> lock(signal_->startMutex_);
            signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
        }
        EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Stop());
        result = std::filesystem::file_size(OUTPUT_PCM_FILE_PATH) < 20;
        EXPECT_EQ(result, false) << "error occur, decode fail" << INPUT_AAC_FILE_SOURCE_PATH[i][0] << endl;

        Release();
    }
}

HWTEST_F(AudioCodeCapiDecoderUnitTest, audioDecoder_Normalcase_05, TestSize.Level1)
{
    bool result;
    for (size_t i = 0; i < INPUT_AMRWB_FILE_SOURCE_PATH.size(); i++) {
        cout << "decode start " << INPUT_AMRWB_FILE_SOURCE_PATH[i][0] << endl;
        ASSERT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, InitFile(CODEC_AMRWB_NAME, INPUT_AMRWB_FILE_SOURCE_PATH[i][0]));

        OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(),
                                stoi(INPUT_AMRWB_FILE_SOURCE_PATH[i][1]));
        OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(),
                                stoi(INPUT_AMRWB_FILE_SOURCE_PATH[i][2]));

        EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioDecoder_Configure(audioDec_, format_));

        isRunning_.store(true);
        inputLoop_ = make_unique<thread>(&AudioCodeCapiDecoderUnitTest::InputFunc, this);
        EXPECT_NE(nullptr, inputLoop_);
        outputLoop_ = make_unique<thread>(&AudioCodeCapiDecoderUnitTest::OutputFunc, this);
        EXPECT_NE(nullptr, outputLoop_);

        EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioDecoder_Start(audioDec_));
        {
            unique_lock<mutex> lock(signal_->startMutex_);
            signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
        }
        EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Stop());
        result = std::filesystem::file_size(OUTPUT_PCM_FILE_PATH) < 20;
        EXPECT_EQ(result, false) << "error occur, decode fail" << INPUT_AMRWB_FILE_SOURCE_PATH[i][0] << endl;

        Release();
    }
}

HWTEST_F(AudioCodeCapiDecoderUnitTest, audioDecoder_Normalcase_06, TestSize.Level1)
{
    bool result;
    for (size_t i = 0; i < INPUT_AMRNB_FILE_SOURCE_PATH.size(); i++) {
        cout << "decode start " << INPUT_AMRNB_FILE_SOURCE_PATH[i][0] << endl;
        ASSERT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, InitFile(CODEC_AMRNB_NAME, INPUT_AMRNB_FILE_SOURCE_PATH[i][0]));

        OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(),
                                stoi(INPUT_AMRNB_FILE_SOURCE_PATH[i][1]));
        OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(),
                                stoi(INPUT_AMRNB_FILE_SOURCE_PATH[i][2]));

        EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioDecoder_Configure(audioDec_, format_));

        isRunning_.store(true);
        inputLoop_ = make_unique<thread>(&AudioCodeCapiDecoderUnitTest::InputFunc, this);
        EXPECT_NE(nullptr, inputLoop_);
        outputLoop_ = make_unique<thread>(&AudioCodeCapiDecoderUnitTest::OutputFunc, this);
        EXPECT_NE(nullptr, outputLoop_);

        EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioDecoder_Start(audioDec_));
        {
            unique_lock<mutex> lock(signal_->startMutex_);
            signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
        }
        EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Stop());
        result = std::filesystem::file_size(OUTPUT_PCM_FILE_PATH) < 20;
        EXPECT_EQ(result, false) << "error occur, decode fail" << INPUT_AMRNB_FILE_SOURCE_PATH[i][0] << endl;

        Release();
    }
}

HWTEST_F(AudioCodeCapiDecoderUnitTest, audioDecoder_Normalcase_07, TestSize.Level1)
{
    signal_ = nullptr;
    if (!CheckSoFunc()) {
        return;
    }
    bool result;
    for (size_t i = 0; i < INPUT_OPUS_FILE_SOURCE_PATH.size(); i++) {
        cout << "decode start " << INPUT_OPUS_FILE_SOURCE_PATH[i][0] << endl;
        ASSERT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, InitFile(CODEC_OPUS_NAME, INPUT_OPUS_FILE_SOURCE_PATH[i][0]));

        OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(),
                                stoi(INPUT_OPUS_FILE_SOURCE_PATH[i][1]));
        OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(),
                                stoi(INPUT_OPUS_FILE_SOURCE_PATH[i][2]));
        EXPECT_NE(nullptr, signal_);
        EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioDecoder_Configure(audioDec_, format_));

        isRunning_.store(true);
        inputLoop_ = make_unique<thread>(&AudioCodeCapiDecoderUnitTest::InputFunc, this);
        EXPECT_NE(nullptr, inputLoop_);
        outputLoop_ = make_unique<thread>(&AudioCodeCapiDecoderUnitTest::OutputFunc, this);
        EXPECT_NE(nullptr, outputLoop_);

        EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioDecoder_Start(audioDec_));
        {
            unique_lock<mutex> lock(signal_->startMutex_);
            signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
        }
        EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Stop());
        result = std::filesystem::file_size(OUTPUT_PCM_FILE_PATH) < 20;
        EXPECT_EQ(result, false) << "error occur, decode fail" << INPUT_OPUS_FILE_SOURCE_PATH[i][0] << endl;

        Release();
    }
}

HWTEST_F(AudioCodeCapiDecoderUnitTest, audioDecoder_Normalcase_08, TestSize.Level1)
{
    bool result;
    for (size_t i = 0; i < INPUT_G711MU_FILE_SOURCE_PATH.size(); i++) {
        cout << "decode start " << INPUT_G711MU_FILE_SOURCE_PATH[i][0] << endl;
        ASSERT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, InitFile(CODEC_G711MU_NAME, INPUT_G711MU_FILE_SOURCE_PATH[i][0]));

        OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(),
                                stoi(INPUT_G711MU_FILE_SOURCE_PATH[i][1]));
        OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(),
                                stoi(INPUT_G711MU_FILE_SOURCE_PATH[i][2]));

        EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioDecoder_Configure(audioDec_, format_));

        isRunning_.store(true);
        inputLoop_ = make_unique<thread>(&AudioCodeCapiDecoderUnitTest::InputFunc, this);
        EXPECT_NE(nullptr, inputLoop_);
        outputLoop_ = make_unique<thread>(&AudioCodeCapiDecoderUnitTest::OutputFunc, this);
        EXPECT_NE(nullptr, outputLoop_);

        EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioDecoder_Start(audioDec_));
        {
            unique_lock<mutex> lock(signal_->startMutex_);
            signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
        }
        EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Stop());
        result = std::filesystem::file_size(OUTPUT_PCM_FILE_PATH) < 20;
        EXPECT_EQ(result, false) << "error occur, decode fail" << INPUT_G711MU_FILE_SOURCE_PATH[i][0] << endl;

        Release();
    }
}
} // namespace MediaAVCodec
} // namespace OHOS
