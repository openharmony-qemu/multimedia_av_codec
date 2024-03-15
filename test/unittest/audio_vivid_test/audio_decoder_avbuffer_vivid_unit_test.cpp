/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
#include "avcodec_codec_name.h"
#include "common/native_mfmagic.h"
#include "native_avbuffer.h"
#include "native_avcodec_audiocodec.h"
#include <atomic>
#include <fstream>
#include <gtest/gtest.h>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>

#include "avcodec_common.h"
#include "avcodec_errors.h"
#include "media_description.h"
#include "native_avcodec_audiocodec.h"
#include "native_avcodec_base.h"
#include "native_avformat.h"
#include "securec.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS::MediaAVCodec;

namespace {
const string CODEC_VIVID_NAME = std::string(AVCodecCodecName::AUDIO_DECODER_VIVID_NAME);
const string INPUT_SOURCE_PATH = "/data/test/media/";
const int VIVID_TESTCASES_NUMS = 10;

const string BIT_DEPTH_16_STRING = "16";
const string BIT_DEPTH_24_STRING = "24";

const std::vector<std::vector<string>> INPUT_VIVID_FILE_SOURCE_PATH = {
    {"VIVID_48k_1c.dat", "48000", "1", "16"},    {"VIVID_48k_2c.dat", "48000", "2", "16"},
    {"VIVID_48k_6c.dat", "48000", "6", "16"},    {"VIVID_48k_6c_2o.dat", "48000", "8", "16"},
    {"VIVID_48k_hoa.dat", "48000", "16", "16"},  {"VIVID_96k_1c.dat", "96000", "1", "16"},
    {"VIVID_96k_2c.dat", "96000", "2", "16"},    {"VIVID_96k_6c.dat", "96000", "6", "16"},
    {"VIVID_96k_6c_2o.dat", "96000", "8", "16"}, {"VIVID_96k_hoa.dat", "96000", "16", "16"}};

const std::vector<std::vector<string>> INPUT_VIVID_24BIT_FILE_SOURCE_PATH = {
    {"VIVID_48k_1c.dat", "48000", "1", "24"},    {"VIVID_48k_2c.dat", "48000", "2", "24"},
    {"VIVID_48k_6c.dat", "48000", "6", "24"},    {"VIVID_48k_6c_2o.dat", "48000", "8", "24"},
    {"VIVID_48k_hoa.dat", "48000", "16", "24"},  {"VIVID_96k_1c.dat", "96000", "1", "24"},
    {"VIVID_96k_2c.dat", "96000", "2", "24"},    {"VIVID_96k_6c.dat", "96000", "6", "24"},
    {"VIVID_96k_6c_2o.dat", "96000", "8", "24"}, {"VIVID_96k_hoa.dat", "96000", "16", "24"}};
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
    std::queue<OH_AVBuffer *> inBufferQueue_;
    std::queue<OH_AVBuffer *> outBufferQueue_;
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

static void OnInputBufferAvailable(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *data, void *userData)
{
    (void)codec;
    ADecSignal *signal = static_cast<ADecSignal *>(userData);
    unique_lock<mutex> lock(signal->inMutex_);
    signal->inQueue_.push(index);
    signal->inBufferQueue_.push(data);
    signal->inCond_.notify_all();
}

static void OnOutputBufferAvailable(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *data, void *userData)
{
    (void)codec;
    ADecSignal *signal = static_cast<ADecSignal *>(userData);
    unique_lock<mutex> lock(signal->outMutex_);
    signal->outQueue_.push(index);
    signal->outBufferQueue_.push(data);
    signal->outCond_.notify_all();
}

class AudioVividDecoderCapacityUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
    int32_t InitFile(const string &codecName, string inputTestFile);
    void InputFunc();
    void OutputFunc();
    int32_t HandleInputBuffer(const uint32_t index);
    int32_t Stop();
    void Release();

protected:
    std::atomic<bool> isRunning_ = false;
    std::unique_ptr<std::thread> inputLoop_;
    std::unique_ptr<std::thread> outputLoop_;
    struct OH_AVCodecCallback cb_;
    ADecSignal *signal_;
    OH_AVCodec *audioDec_;
    OH_AVFormat *format_;
    bool isFirstFrame_ = true;
    std::ifstream inputFile_;
    std::ofstream pcmOutputFile_;
    bool vorbisHasExtradata_ = true;
};

void AudioVividDecoderCapacityUnitTest::SetUpTestCase(void)
{
    cout << "[SetUpTestCase]: " << endl;
}

void AudioVividDecoderCapacityUnitTest::TearDownTestCase(void)
{
    cout << "[TearDownTestCase]: " << endl;
}

void AudioVividDecoderCapacityUnitTest::SetUp(void)
{
    cout << "[SetUp]: SetUp!!!" << endl;
}

void AudioVividDecoderCapacityUnitTest::TearDown(void)
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

void AudioVividDecoderCapacityUnitTest::Release()
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
    OH_AudioCodec_Destroy(audioDec_);
}

int32_t AudioVividDecoderCapacityUnitTest::HandleInputBuffer(const uint32_t index)
{
    int32_t ret = OH_AudioCodec_PushInputBuffer(audioDec_, index);
    signal_->inBufferQueue_.pop();
    signal_->inQueue_.pop();
    return ret;
}

void AudioVividDecoderCapacityUnitTest::InputFunc()
{
    int64_t size;
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
            buffer->buffer_->memory_->SetSize(1);
            buffer->buffer_->flag_ = AVCODEC_BUFFER_FLAGS_EOS;
            HandleInputBuffer(index);
            cout << "end buffer\n";
            break;
        }
        if (inputFile_.gcount() != sizeof(size)) {
            cout << "Fatal: read size fail" << endl;
            break;
        }
        inputFile_.read(reinterpret_cast<char *>(&buffer->buffer_->pts_), sizeof(buffer->buffer_->pts_));
        if (inputFile_.gcount() != sizeof(buffer->buffer_->pts_)) {
            cout << "Fatal: read size fail" << endl;
            break;
        }
        inputFile_.read((char *)OH_AVBuffer_GetAddr(buffer), size);
        if (inputFile_.gcount() != size) {
            cout << "Fatal: read buffer fail" << endl;
            break;
        }
        buffer->buffer_->memory_->SetSize(size);
        if (isFirstFrame_) {
            buffer->buffer_->flag_ = AVCODEC_BUFFER_FLAGS_CODEC_DATA;
            isFirstFrame_ = false;
        } else {
            buffer->buffer_->flag_ = AVCODEC_BUFFER_FLAGS_NONE;
        }
        if (HandleInputBuffer(index) != AV_ERR_OK) {
            cout << "Fatal error, exit" << endl;
            break;
        }
    }
    inputFile_.close();
}

void AudioVividDecoderCapacityUnitTest::OutputFunc()
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
        OH_AVBuffer *data = signal_->outBufferQueue_.front();
        if (data == nullptr) {
            std::cout << "OutputFunc OH_AVBuffer is nullptr" << std::endl;
            isRunning_.store(false);
            signal_->startCond_.notify_all();
            break;
        }
        pcmOutputFile_.write(reinterpret_cast<char *>(OH_AVBuffer_GetAddr(data)), data->buffer_->memory_->GetSize());
        if (data->buffer_->flag_ == AVCODEC_BUFFER_FLAGS_EOS) {
            cout << "decode eos" << endl;
            isRunning_.store(false);
            signal_->startCond_.notify_all();
        }
        signal_->outBufferQueue_.pop();
        signal_->outQueue_.pop();
        EXPECT_EQ(AV_ERR_OK, OH_AudioCodec_FreeOutputBuffer(audioDec_, index));
    }
    pcmOutputFile_.close();
    signal_->startCond_.notify_all();
}

int32_t AudioVividDecoderCapacityUnitTest::Stop()
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
    return OH_AudioCodec_Stop(audioDec_);
}

int32_t AudioVividDecoderCapacityUnitTest::InitFile(const string &codecName, string inputTestFile)
{
    format_ = OH_AVFormat_Create();
    audioDec_ = OH_AudioCodec_CreateByName(codecName.c_str());
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
    int32_t ret = OH_AudioCodec_RegisterCallback(audioDec_, cb_, signal_);
    if (ret != AVCodecServiceErrCode::AVCS_ERR_OK) {
        return AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
    }
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

HWTEST_F(AudioVividDecoderCapacityUnitTest, audioCodec_Normalcase_07, TestSize.Level1)
{
    bool result;
    for (int i = 0; i < VIVID_TESTCASES_NUMS; i++) {
        cout << "decode start " << INPUT_VIVID_FILE_SOURCE_PATH[i][0] << endl;
        ASSERT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, InitFile(CODEC_VIVID_NAME, INPUT_VIVID_FILE_SOURCE_PATH[i][0]));

        OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(),
                                stoi(INPUT_VIVID_FILE_SOURCE_PATH[i][1]));
        OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(),
                                stoi(INPUT_VIVID_FILE_SOURCE_PATH[i][2]));

        if (INPUT_VIVID_FILE_SOURCE_PATH[i][3] == BIT_DEPTH_16_STRING) {
            OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(),
                                    Media::Plugins::AudioSampleFormat::SAMPLE_S16LE);
        } else if (INPUT_VIVID_FILE_SOURCE_PATH[i][3] == BIT_DEPTH_24_STRING) {
            OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(),
                                    Media::Plugins::AudioSampleFormat::SAMPLE_S24LE);
        }

        EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, format_));

        isRunning_.store(true);
        inputLoop_ = make_unique<thread>(&AudioVividDecoderCapacityUnitTest::InputFunc, this);
        EXPECT_NE(nullptr, inputLoop_);
        outputLoop_ = make_unique<thread>(&AudioVividDecoderCapacityUnitTest::OutputFunc, this);
        EXPECT_NE(nullptr, outputLoop_);

        EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Start(audioDec_));
        {
            unique_lock<mutex> lock(signal_->startMutex_);
            signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
        }
        EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Stop());
        result = std::filesystem::file_size(OUTPUT_PCM_FILE_PATH) < 20;
        EXPECT_EQ(result, false) << "error occur, decode fail" << INPUT_VIVID_FILE_SOURCE_PATH[i][0] << endl;

        Release();
    }
}

HWTEST_F(AudioVividDecoderCapacityUnitTest, audioCodec_Normalcase_08, TestSize.Level1)
{
    bool result;
    for (int i = 0; i < VIVID_TESTCASES_NUMS; i++) {
        cout << "decode start " << INPUT_VIVID_24BIT_FILE_SOURCE_PATH[i][0] << endl;
        ASSERT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK,
                  InitFile(CODEC_VIVID_NAME, INPUT_VIVID_24BIT_FILE_SOURCE_PATH[i][0]));

        OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(),
                                stoi(INPUT_VIVID_24BIT_FILE_SOURCE_PATH[i][1]));
        OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(),
                                stoi(INPUT_VIVID_24BIT_FILE_SOURCE_PATH[i][2]));

        if (INPUT_VIVID_24BIT_FILE_SOURCE_PATH[i][3] == BIT_DEPTH_16_STRING) {
            OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(),
                                    Media::Plugins::AudioSampleFormat::SAMPLE_S16LE);
        } else if (INPUT_VIVID_24BIT_FILE_SOURCE_PATH[i][3] == BIT_DEPTH_24_STRING) {
            OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(),
                                    Media::Plugins::AudioSampleFormat::SAMPLE_S24LE);
        }

        EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, format_));

        isRunning_.store(true);
        inputLoop_ = make_unique<thread>(&AudioVividDecoderCapacityUnitTest::InputFunc, this);
        EXPECT_NE(nullptr, inputLoop_);
        outputLoop_ = make_unique<thread>(&AudioVividDecoderCapacityUnitTest::OutputFunc, this);
        EXPECT_NE(nullptr, outputLoop_);

        EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Start(audioDec_));
        {
            unique_lock<mutex> lock(signal_->startMutex_);
            signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
        }
        EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Stop());
        result = std::filesystem::file_size(OUTPUT_PCM_FILE_PATH) < 20;
        EXPECT_EQ(result, false) << "error occur, decode fail" << INPUT_VIVID_24BIT_FILE_SOURCE_PATH[i][0] << endl;

        Release();
    }
}
} // namespace MediaAVCodec
} // namespace OHOS