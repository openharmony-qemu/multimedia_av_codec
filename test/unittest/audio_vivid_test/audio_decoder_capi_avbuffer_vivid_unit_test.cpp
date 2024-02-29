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
#include <atomic>
#include <chrono>
#include <fstream>
#include <gtest/gtest.h>
#include <iostream>
#include <queue>
#include <string>
#include <thread>
#include <unistd.h>

#include "avcodec_codec_name.h"
#include "avcodec_mime_type.h"
#include "common/native_mfmagic.h"
#include "media_description.h"
#include "native_avbuffer.h"
#include "native_avcodec_audiocodec.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS::MediaAVCodec;
namespace {
const string CODEC_VIVID_NAME = std::string(AVCodecCodecName::AUDIO_DECODER_VIVID_NAME);
constexpr uint32_t VIVID_MIN_CHANNEL_COUNT = 0;
constexpr uint32_t VIVID_MAX_CHANNEL_COUNT = 17;
constexpr uint32_t MAX_CHANNEL_COUNT = 2;
constexpr uint32_t DEFAULT_SAMPLE_RATE = 48000;
constexpr uint32_t VIVID_MAX_INPUT_SIZE = 99999999;
constexpr string_view INPUT_VIVID_FILE_PATH = "/data/test/media/vivid_2c_44100hz_320k.dat";
constexpr string_view OUTPUT_VIVID_PCM_FILE_PATH = "/data/test/media/vivid_2c_44100hz_320k.pcm";
} // namespace

namespace OHOS {
namespace MediaAVCodec {
class ADecBufferSignal {
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
    ADecBufferSignal *signal = static_cast<ADecBufferSignal *>(userData);
    unique_lock<mutex> lock(signal->inMutex_);
    signal->inQueue_.push(index);
    signal->inBufferQueue_.push(data);
    signal->inCond_.notify_all();
}

static void OnOutputBufferAvailable(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *data, void *userData)
{
    (void)codec;
    ADecBufferSignal *signal = static_cast<ADecBufferSignal *>(userData);
    unique_lock<mutex> lock(signal->outMutex_);
    signal->outQueue_.push(index);
    signal->outBufferQueue_.push(data);
    signal->outCond_.notify_all();
}

class AudioVividCodeCapiDecoderUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
    int32_t InitFile(const string &codecName);
    void InputFunc();
    void OutputFunc();
    int32_t CreateCodecFunc(const string &codecName);
    int32_t HandleInputBuffer(const uint32_t index);
    int32_t Configure();
    int32_t Configure24Bit();
    int32_t Start();
    int32_t Stop();
    void Release();

protected:
    std::atomic<bool> isRunning_ = false;
    std::unique_ptr<std::thread> inputLoop_;
    std::unique_ptr<std::thread> outputLoop_;
    struct OH_AVCodecCallback cb_;
    ADecBufferSignal *signal_ = nullptr;
    OH_AVCodec *audioDec_ = nullptr;
    OH_AVFormat *format_ = nullptr;
    bool isFirstFrame_ = true;
    std::ifstream inputFile_;
    std::ofstream pcmOutputFile_;
};

void AudioVividCodeCapiDecoderUnitTest::SetUpTestCase(void)
{
    cout << "[SetUpTestCase]: " << endl;
}

void AudioVividCodeCapiDecoderUnitTest::TearDownTestCase(void)
{
    cout << "[TearDownTestCase]: " << endl;
}

void AudioVividCodeCapiDecoderUnitTest::SetUp(void)
{
    cout << "[SetUp]: SetUp!!!" << endl;
}

void AudioVividCodeCapiDecoderUnitTest::TearDown(void)
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
}

void AudioVividCodeCapiDecoderUnitTest::Release()
{
    Stop();
    OH_AudioCodec_Destroy(audioDec_);
}

int32_t AudioVividCodeCapiDecoderUnitTest::HandleInputBuffer(const uint32_t index)
{
    int32_t ret = OH_AudioCodec_PushInputBuffer(audioDec_, index);
    signal_->inBufferQueue_.pop();
    signal_->inQueue_.pop();
    return ret;
}

void AudioVividCodeCapiDecoderUnitTest::InputFunc()
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

void AudioVividCodeCapiDecoderUnitTest::OutputFunc()
{
    if (!pcmOutputFile_.is_open()) {
        std::cout << "open " << OUTPUT_VIVID_PCM_FILE_PATH << " failed!" << std::endl;
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

int32_t AudioVividCodeCapiDecoderUnitTest::Start()
{
    isRunning_.store(true);
    inputLoop_ = make_unique<thread>(&AudioVividCodeCapiDecoderUnitTest::InputFunc, this);
    if (inputLoop_ == nullptr) {
        cout << "Fatal: No memory" << endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }

    outputLoop_ = make_unique<thread>(&AudioVividCodeCapiDecoderUnitTest::OutputFunc, this);
    if (outputLoop_ == nullptr) {
        cout << "Fatal: No memory" << endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }

    return OH_AudioCodec_Start(audioDec_);
}

int32_t AudioVividCodeCapiDecoderUnitTest::Stop()
{
    isRunning_.store(false);
    if (!signal_) {
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }
    signal_->startCond_.notify_all();
    if (inputLoop_ != nullptr && inputLoop_->joinable()) {
        {
            unique_lock<mutex> lock(signal_->inMutex_);
            signal_->inCond_.notify_all();
        }
        inputLoop_->join();
    }

    if (outputLoop_ != nullptr && outputLoop_->joinable()) {
        {
            unique_lock<mutex> lock(signal_->outMutex_);
            signal_->outCond_.notify_all();
        }
        outputLoop_->join();
    }
    return OH_AudioCodec_Stop(audioDec_);
}

int32_t AudioVividCodeCapiDecoderUnitTest::InitFile(const string &codecName)
{
    if (codecName.compare(CODEC_VIVID_NAME) == 0) {
        inputFile_.open(INPUT_VIVID_FILE_PATH.data(), std::ios::binary);
        pcmOutputFile_.open(OUTPUT_VIVID_PCM_FILE_PATH.data(), std::ios::out | std::ios::binary);
    } else {
        cout << "Fatal: audio format type not support" << endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }

    if (!inputFile_.is_open()) {
        cout << "Fatal: open input file failed" << endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }
    if (!pcmOutputFile_.is_open()) {
        cout << "Fatal: open output file failed" << endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }
    return OH_AVErrCode::AV_ERR_OK;
}

int32_t AudioVividCodeCapiDecoderUnitTest::CreateCodecFunc(const string &codecName)
{
    if (codecName.compare(CODEC_VIVID_NAME) == 0) {
        audioDec_ = OH_AudioCodec_CreateByName((AVCodecCodecName::AUDIO_DECODER_VIVID_NAME).data());
    } else {
        cout << "audio name not support" << endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }

    if (audioDec_ == nullptr) {
        cout << "Fatal: CreateByName fail" << endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }

    signal_ = new ADecBufferSignal();
    if (signal_ == nullptr) {
        cout << "Fatal: create signal fail" << endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }
    cb_ = {&OnError, &OnOutputFormatChanged, &OnInputBufferAvailable, &OnOutputBufferAvailable};
    int32_t ret = OH_AudioCodec_RegisterCallback(audioDec_, cb_, signal_);
    if (ret != OH_AVErrCode::AV_ERR_OK) {
        cout << "Fatal: SetCallback fail" << endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }

    return OH_AVErrCode::AV_ERR_OK;
}

int32_t AudioVividCodeCapiDecoderUnitTest::Configure()
{
    format_ = OH_AVFormat_Create();
    if (format_ == nullptr) {
        cout << "Fatal: create format failed" << endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }

    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), MAX_CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), DEFAULT_SAMPLE_RATE);

    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(),
                            OH_BitsPerSample::SAMPLE_S16LE);

    return OH_AudioCodec_Configure(audioDec_, format_);
}

int32_t AudioVividCodeCapiDecoderUnitTest::Configure24Bit()
{
    format_ = OH_AVFormat_Create();
    if (format_ == nullptr) {
        cout << "Fatal: create format failed" << endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }

    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), MAX_CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), DEFAULT_SAMPLE_RATE);

    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(),
                            OH_BitsPerSample::SAMPLE_S24LE);

    return OH_AudioCodec_Configure(audioDec_, format_);
}

HWTEST_F(AudioVividCodeCapiDecoderUnitTest, audioDecoder_Vivid_CreateByMime_01, TestSize.Level1)
{
    audioDec_ = OH_AudioCodec_CreateByMime(AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_VIVID.data(), false);
    EXPECT_NE(nullptr, audioDec_);
    Release();
}

HWTEST_F(AudioVividCodeCapiDecoderUnitTest, audioDecoder_Vivid_CreateByName_01, TestSize.Level1)
{
    audioDec_ = OH_AudioCodec_CreateByName((AVCodecCodecName::AUDIO_DECODER_VIVID_NAME).data());
    EXPECT_NE(nullptr, audioDec_);
    Release();
}

HWTEST_F(AudioVividCodeCapiDecoderUnitTest, audioDecoder_Vivid_Configure_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(CODEC_VIVID_NAME));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure());
    Release();
}

HWTEST_F(AudioVividCodeCapiDecoderUnitTest, audioDecoder_Vivid_Configure_02, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(CODEC_VIVID_NAME));
    format_ = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format_);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), VIVID_MIN_CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), DEFAULT_SAMPLE_RATE);
    ASSERT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, format_));
    Release();
}

HWTEST_F(AudioVividCodeCapiDecoderUnitTest, audioDecoder_Vivid_Configure_03, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(CODEC_VIVID_NAME));
    format_ = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format_);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), VIVID_MAX_CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), DEFAULT_SAMPLE_RATE);
    ASSERT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, format_));
    Release();
}

HWTEST_F(AudioVividCodeCapiDecoderUnitTest, audioDecoder_Vivid_Configure_04, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(CODEC_VIVID_NAME));
    format_ = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format_);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(),
                            OH_BitsPerSample::SAMPLE_S32LE);
    ASSERT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, format_));
    Release();
}

HWTEST_F(AudioVividCodeCapiDecoderUnitTest, audioDecoder_Vivid_Configure_05, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(CODEC_VIVID_NAME));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure24Bit());
    Release();
}

HWTEST_F(AudioVividCodeCapiDecoderUnitTest, audioDecoder_Vivid_SetParameter_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(CODEC_VIVID_NAME));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(CODEC_VIVID_NAME));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Flush(audioDec_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_SetParameter(audioDec_, format_));
    Release();
}

HWTEST_F(AudioVividCodeCapiDecoderUnitTest, audioDecoder_Vivid_SetParameter_02, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(CODEC_VIVID_NAME));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure());
    ASSERT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_SetParameter(audioDec_, format_));
    Release();
}

HWTEST_F(AudioVividCodeCapiDecoderUnitTest, audioDecoder_Vivid_SetParameter_03, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(CODEC_VIVID_NAME));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure24Bit());
    ASSERT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_SetParameter(audioDec_, format_));
    Release();
}

HWTEST_F(AudioVividCodeCapiDecoderUnitTest, audioDecoder_Vivid_Start_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(CODEC_VIVID_NAME));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(CODEC_VIVID_NAME));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    Release();
}

HWTEST_F(AudioVividCodeCapiDecoderUnitTest, audioDecoder_Vivid_Start_02, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(CODEC_VIVID_NAME));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(CODEC_VIVID_NAME));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Stop());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Start(audioDec_));
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    Release();
}

HWTEST_F(AudioVividCodeCapiDecoderUnitTest, audioDecoder_Vivid_Start_03, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(CODEC_VIVID_NAME));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(CODEC_VIVID_NAME));
    format_ = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format_);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), MAX_CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), DEFAULT_SAMPLE_RATE);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_MAX_INPUT_SIZE.data(), VIVID_MAX_INPUT_SIZE);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, format_));

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Stop());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Start(audioDec_));
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    Release();
}

HWTEST_F(AudioVividCodeCapiDecoderUnitTest, audioDecoder_Vivid_Start_04, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(CODEC_VIVID_NAME));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(CODEC_VIVID_NAME));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure24Bit());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    Release();
}

HWTEST_F(AudioVividCodeCapiDecoderUnitTest, audioDecoder_Vivid_Start_05, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(CODEC_VIVID_NAME));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(CODEC_VIVID_NAME));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure24Bit());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Stop());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Start(audioDec_));
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    Release();
}

HWTEST_F(AudioVividCodeCapiDecoderUnitTest, audioDecoder_Vivid_Stop_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(CODEC_VIVID_NAME));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(CODEC_VIVID_NAME));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    sleep(1);

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Stop());
    Release();
}

HWTEST_F(AudioVividCodeCapiDecoderUnitTest, audioDecoder_Vivid_Stop_02, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(CODEC_VIVID_NAME));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(CODEC_VIVID_NAME));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure24Bit());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    sleep(1);

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Stop());
    Release();
}

HWTEST_F(AudioVividCodeCapiDecoderUnitTest, audioDecoder_Vivid_Flush_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(CODEC_VIVID_NAME));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(CODEC_VIVID_NAME));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Flush(audioDec_));
    Release();
}

HWTEST_F(AudioVividCodeCapiDecoderUnitTest, audioDecoder_Vivid_Flush_02, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(CODEC_VIVID_NAME));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(CODEC_VIVID_NAME));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure24Bit());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Flush(audioDec_));
    Release();
}

HWTEST_F(AudioVividCodeCapiDecoderUnitTest, audioDecoder_Vivid_Reset_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(CODEC_VIVID_NAME));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioDec_));
    Release();
}

HWTEST_F(AudioVividCodeCapiDecoderUnitTest, audioDecoder_Vivid_Reset_02, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(CODEC_VIVID_NAME));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(CODEC_VIVID_NAME));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Stop());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioDec_));
    Release();
}

HWTEST_F(AudioVividCodeCapiDecoderUnitTest, audioDecoder_Vivid_Reset_03, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(CODEC_VIVID_NAME));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(CODEC_VIVID_NAME));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioDec_));
    Release();
}

HWTEST_F(AudioVividCodeCapiDecoderUnitTest, audioDecoder_Vivid_Reset_04, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(CODEC_VIVID_NAME));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure24Bit());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioDec_));
    Release();
}

HWTEST_F(AudioVividCodeCapiDecoderUnitTest, audioDecoder_Vivid_Reset_05, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(CODEC_VIVID_NAME));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(CODEC_VIVID_NAME));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure24Bit());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Stop());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioDec_));
    Release();
}

HWTEST_F(AudioVividCodeCapiDecoderUnitTest, audioDecoder_Vivid_Reset_06, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(CODEC_VIVID_NAME));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(CODEC_VIVID_NAME));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure24Bit());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioDec_));
    Release();
}

HWTEST_F(AudioVividCodeCapiDecoderUnitTest, audioDecoder_Vivid_Destroy_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(CODEC_VIVID_NAME));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(CODEC_VIVID_NAME));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Stop());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Destroy(audioDec_));
}

HWTEST_F(AudioVividCodeCapiDecoderUnitTest, audioDecoder_Vivid_Destroy_02, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(CODEC_VIVID_NAME));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(CODEC_VIVID_NAME));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure());

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Destroy(audioDec_));
}

HWTEST_F(AudioVividCodeCapiDecoderUnitTest, audioDecoder_Vivid_Destroy_03, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(CODEC_VIVID_NAME));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(CODEC_VIVID_NAME));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure24Bit());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Stop());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Destroy(audioDec_));
}

HWTEST_F(AudioVividCodeCapiDecoderUnitTest, audioDecoder_Vivid_Destroy_04, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(CODEC_VIVID_NAME));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(CODEC_VIVID_NAME));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure24Bit());

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Destroy(audioDec_));
}

HWTEST_F(AudioVividCodeCapiDecoderUnitTest, audioDecoder_Vivid_GetOutputFormat_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(CODEC_VIVID_NAME));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(CODEC_VIVID_NAME));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure());

    EXPECT_NE(nullptr, OH_AudioCodec_GetOutputDescription(audioDec_));
    Release();
}

HWTEST_F(AudioVividCodeCapiDecoderUnitTest, audioDecoder_Vivid_GetOutputFormat_02, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(CODEC_VIVID_NAME));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(CODEC_VIVID_NAME));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure24Bit());

    EXPECT_NE(nullptr, OH_AudioCodec_GetOutputDescription(audioDec_));
    Release();
}

HWTEST_F(AudioVividCodeCapiDecoderUnitTest, audioDecoder_Vivid_IsValid_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(CODEC_VIVID_NAME));
    bool isValid = false;
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_IsValid(audioDec_, &isValid));
    Release();
}

HWTEST_F(AudioVividCodeCapiDecoderUnitTest, audioDecoder_Vivid_Prepare_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(CODEC_VIVID_NAME));
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Prepare(audioDec_));
    Release();
}

HWTEST_F(AudioVividCodeCapiDecoderUnitTest, audioDecoder_Vivid_Prepare_02, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(CODEC_VIVID_NAME));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure());
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Prepare(audioDec_));
    Release();
}

HWTEST_F(AudioVividCodeCapiDecoderUnitTest, audioDecoder_Vivid_Prepare_03, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(CODEC_VIVID_NAME));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure24Bit());
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Prepare(audioDec_));
    Release();
}

HWTEST_F(AudioVividCodeCapiDecoderUnitTest, audioDecoder_Vivid_PushInputData_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(CODEC_VIVID_NAME));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(CODEC_VIVID_NAME));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());

    uint32_t index = 0;
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_PushInputBuffer(audioDec_, index));
    Release();
}

HWTEST_F(AudioVividCodeCapiDecoderUnitTest, audioDecoder_Vivid_PushInputData_02, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(CODEC_VIVID_NAME));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(CODEC_VIVID_NAME));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure24Bit());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());

    uint32_t index = 0;
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_PushInputBuffer(audioDec_, index));
    Release();
}

HWTEST_F(AudioVividCodeCapiDecoderUnitTest, audioDecoder_Vivid_ReleaseOutputBuffer_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(CODEC_VIVID_NAME));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(CODEC_VIVID_NAME));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());

    uint32_t index = 1024;
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_FreeOutputBuffer(audioDec_, index));
    Release();
}

HWTEST_F(AudioVividCodeCapiDecoderUnitTest, audioDecoder_Vivid_ReleaseOutputBuffer_02, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(CODEC_VIVID_NAME));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(CODEC_VIVID_NAME));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure24Bit());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());

    uint32_t index = 1024;
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_FreeOutputBuffer(audioDec_, index));
    Release();
}
} // namespace MediaAVCodec
} // namespace OHOS