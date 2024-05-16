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
#include "avcodec_mime_type.h"
#include "media_description.h"
#include "native_avcodec_base.h"
#include "native_avformat.h"
#include "avcodec_common.h"
#include "avcodec_errors.h"
#include "securec.h"
#include "avcodec_audio_common.h"
#include "native_avbuffer.h"
#include "common/native_mfmagic.h"
#include "native_avcodec_audiocodec.h"
#include "native_audio_channel_layout.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS::MediaAVCodec;

namespace {
constexpr uint32_t CHANNEL_COUNT = 2;
constexpr uint32_t ABNORMAL_CHANNEL_COUNT = 10;
constexpr uint32_t SAMPLE_RATE = 44100;
constexpr uint32_t OPUS_SAMPLE_RATE = 48000;
constexpr uint32_t CHANNEL_1CHAN_COUNT = 1;
constexpr uint32_t SAMPLE_RATE_8K = 8000;
constexpr uint32_t ABNORMAL_SAMPLE_RATE = 9999999;
constexpr uint32_t BITS_RATE = 261000;
constexpr int32_t MP3_BIT_RATE = 128000;
constexpr int32_t ABNORMAL_BITS_RATE = -1;
constexpr int32_t BITS_PER_CODED_SAMPLE = AudioSampleFormat::SAMPLE_S16LE;
constexpr int32_t ABNORMAL_BITS_SAMPLE = AudioSampleFormat::INVALID_WIDTH;
constexpr uint64_t CHANNEL_LAYOUT = OH_AudioChannelLayout::CH_LAYOUT_STEREO;
constexpr uint64_t ABNORMAL_CHANNEL_LAYOUT = OH_AudioChannelLayout::CH_LAYOUT_10POINT2;
constexpr uint64_t ABNORMAL_CHANNEL_LAYOUT_UNKNOWN = OH_AudioChannelLayout::CH_LAYOUT_UNKNOWN;
constexpr int32_t SAMPLE_FORMAT = AudioSampleFormat::SAMPLE_S16LE;
constexpr int32_t ABNORMAL_SAMPLE_FORMAT = AudioSampleFormat::SAMPLE_U8;
constexpr uint32_t FLAC_DEFAULT_FRAME_BYTES = 18432;
constexpr uint32_t AAC_DEFAULT_FRAME_BYTES = 2 * 1024 * 4;
constexpr uint32_t G711MU_DEFAULT_FRAME_BYTES = 320;
constexpr uint32_t MP3_DEFAULT_FRAME_BYTES = 4608;
constexpr int32_t MAX_INPUT_SIZE = 8192;
constexpr uint32_t ILLEGAL_CHANNEL_COUNT = 9;
constexpr uint32_t AAC_ILLEGAL_CHANNEL_COUNT = 7;
constexpr uint32_t ILLEGAL_SAMPLE_RATE = 441000;
constexpr int32_t COMPLIANCE_LEVEL = 0;
constexpr int32_t ABNORMAL_COMPLIANCE_LEVEL_L = -9999999;
constexpr int32_t ABNORMAL_COMPLIANCE_LEVEL_R = 9999999;

constexpr string_view FLAC_INPUT_FILE_PATH = "/data/test/media/flac_2c_44100hz_261k.pcm";
constexpr string_view FLAC_OUTPUT_FILE_PATH = "/data/test/media/encoderTest.flac";
constexpr string_view AAC_INPUT_FILE_PATH = "/data/test/media/aac_2c_44100hz_199k.pcm";
constexpr string_view AAC_OUTPUT_FILE_PATH = "/data/test/media/aac_2c_44100hz_encode.aac";
constexpr string_view G711MU_INPUT_FILE_PATH = "/data/test/media/g711mu_8kHz_10s.pcm";
constexpr string_view G711MU_OUTPUT_FILE_PATH = "/data/test/media/g711mu_8kHz_10s_afterEncode.raw";
constexpr string_view OPUS_INPUT_FILE_PATH = "/data/test/media/flac_2c_44100hz_261k.pcm";
constexpr string_view OPUS_OUTPUT_FILE_PATH = "/data/test/media/encoderTest.opus";
constexpr string_view OPUS_SO_FILE_PATH = "/system/lib64/libav_codec_ext_base.z.so";
constexpr string_view MP3_INPUT_FILE_PATH = "/data/test/media/flac_2c_44100hz_261k.pcm";
constexpr string_view MP3_OUTPUT_FILE_PATH = "/data/test/media/mp3_2c_44100hz_afterEncode.mp3";
} // namespace

namespace OHOS {
namespace MediaAVCodec {
enum class AudioBufferFormatType : int32_t {
    TYPE_AAC = 0,
    TYPE_FLAC = 1,
    TYPE_MP3 = 2,
    TYPE_VORBIS = 3,
    TYPE_AMRNB = 4,
    TYPE_AMRWB = 5,
    TYPE_G711MU = 6,
    TYPE_OPUS = 7,
    TYPE_MAX = 8,
};

class AudioCodecBufferSignal {
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

static void OnInputBufferAvailable(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData)
{
    (void)codec;
    AudioCodecBufferSignal *signal = static_cast<AudioCodecBufferSignal *>(userData);
    unique_lock<mutex> lock(signal->inMutex_);
    signal->inQueue_.push(index);
    signal->inBufferQueue_.push(buffer);
    signal->inCond_.notify_all();
}

static void OnOutputBufferAvailable(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData)
{
    (void)codec;
    AudioCodecBufferSignal *signal = static_cast<AudioCodecBufferSignal *>(userData);
    unique_lock<mutex> lock(signal->outMutex_);
    signal->outQueue_.push(index);
    signal->outBufferQueue_.push(buffer);
    if (buffer) {
        cout << "OnOutputBufferAvailable received, index:" << index << ", size:" << buffer->buffer_->memory_->GetSize()
             << ", flags:" << buffer->buffer_->flag_ << ", pts: " << buffer->buffer_->pts_ << endl;
    } else {
        cout << "OnOutputBufferAvailable error, attr is nullptr!" << endl;
    }
    signal->outCond_.notify_all();
}

class AudioEncoderBufferCapiUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
    void SleepTest();
    void Release();
    void HandleEOS(const uint32_t &index);
    void InputFunc();
    void OutputFunc();
    int32_t GetFileSize(const std::string &filePath);
    int32_t GetFrameBytes();
    int32_t InitFile(AudioBufferFormatType audioType);
    int32_t CreateCodecFunc(AudioBufferFormatType audioType);
    int32_t CheckSoFunc();
    int32_t Start();
    int32_t Stop();
    void JoinThread();

protected:
    std::atomic<bool> isRunning_ = false;
    std::unique_ptr<std::thread> inputLoop_;
    std::unique_ptr<std::thread> outputLoop_;
    struct OH_AVCodecCallback cb_;
    AudioCodecBufferSignal *signal_ = nullptr;
    OH_AVCodec *audioEnc_ = nullptr;
    OH_AVFormat *format = nullptr;
    bool isFirstFrame_ = true;
    std::unique_ptr<std::ifstream> inputFile_;
    std::unique_ptr<std::ofstream> outputFile_;
    std::unique_ptr<std::ifstream> soFile_;
    int32_t fileSize_ = 0;
    uint32_t frameBytes_ = FLAC_DEFAULT_FRAME_BYTES; // default for flac
};

void AudioEncoderBufferCapiUnitTest::SetUpTestCase(void)
{
    cout << "[SetUpTestCase]: " << endl;
}

void AudioEncoderBufferCapiUnitTest::TearDownTestCase(void)
{
    cout << "[TearDownTestCase]: " << endl;
}

void AudioEncoderBufferCapiUnitTest::SetUp(void)
{
    cout << "[SetUp]: SetUp!!!" << endl;
}

void AudioEncoderBufferCapiUnitTest::TearDown(void)
{
    cout << "[TearDown]: over!!!" << endl;

    if (signal_) {
        delete signal_;
        signal_ = nullptr;
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

void AudioEncoderBufferCapiUnitTest::SleepTest()
{
    sleep(1);
}

void AudioEncoderBufferCapiUnitTest::Release()
{
    SleepTest();
    Stop();
    OH_AudioCodec_Destroy(audioEnc_);
}

void AudioEncoderBufferCapiUnitTest::HandleEOS(const uint32_t &index)
{
    OH_AudioCodec_PushInputBuffer(audioEnc_, index);
    std::cout << "end buffer\n";
    signal_->inQueue_.pop();
    signal_->inBufferQueue_.pop();
}

void AudioEncoderBufferCapiUnitTest::InputFunc()
{
    if (!inputFile_->is_open()) {
        cout << "Fatal: open file fail" << endl;
        return;
    }
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
        if (!inputFile_->eof()) {
            inputFile_->read((char *)OH_AVBuffer_GetAddr(buffer), frameBytes_);
            buffer->buffer_->memory_->SetSize(frameBytes_);
            fileSize_ -= frameBytes_;
        } else {
            buffer->buffer_->memory_->SetSize(1);
            buffer->buffer_->flag_ = AVCODEC_BUFFER_FLAGS_EOS;
            HandleEOS(index);
            break;
        }
        int32_t ret = AVCS_ERR_OK;
        if (isFirstFrame_) {
            buffer->buffer_->flag_ = AVCODEC_BUFFER_FLAGS_CODEC_DATA;
            ret = OH_AudioCodec_PushInputBuffer(audioEnc_, index);
            isFirstFrame_ = false;
        } else {
            buffer->buffer_->flag_ = AVCODEC_BUFFER_FLAGS_NONE;
            ret = OH_AudioCodec_PushInputBuffer(audioEnc_, index);
        }
        signal_->inQueue_.pop();
        signal_->inBufferQueue_.pop();
        if (ret != AVCS_ERR_OK) {
            cout << "Fatal error, exit" << endl;
            break;
        }
    }
    cout << "stop, exit" << endl;
    inputFile_->close();
}

void AudioEncoderBufferCapiUnitTest::OutputFunc()
{
    if (!outputFile_->is_open()) {
        cout << "Fatal: open output file fail" << endl;
        return;
    }
    while (isRunning_.load()) {
        unique_lock<mutex> lock(signal_->outMutex_);
        signal_->outCond_.wait(lock, [this]() { return (signal_->outQueue_.size() > 0 || !isRunning_.load()); });

        if (!isRunning_.load()) {
            cout << "wait to stop, exit" << endl;
            break;
        }
        uint32_t index = signal_->outQueue_.front();
        OH_AVBuffer *avBuffer = signal_->outBufferQueue_.front();
        if (avBuffer == nullptr) {
            cout << "OutputFunc OH_AVBuffer is nullptr" << endl;
            continue;
        }
        if (avBuffer != nullptr) {
            cout << "OutputFunc write file,buffer index:"
                << index << ", data size:" << avBuffer->buffer_->memory_->GetSize() << endl;
            outputFile_->write(reinterpret_cast<char *>(OH_AVBuffer_GetAddr(avBuffer)),
                               avBuffer->buffer_->memory_->GetSize());
        }
        if (avBuffer != nullptr &&
            (avBuffer->buffer_->flag_ == AVCODEC_BUFFER_FLAGS_EOS || avBuffer->buffer_->memory_->GetSize()== 0)) {
            cout << "encode eos" << endl;
            isRunning_.store(false);
            signal_->startCond_.notify_all();
        }
        signal_->outBufferQueue_.pop();
        signal_->outQueue_.pop();
        if (OH_AudioCodec_FreeOutputBuffer(audioEnc_, index) != AV_ERR_OK) {
            cout << "Fatal: FreeOutputData fail" << endl;
            break;
        }
        if (avBuffer->buffer_->flag_ == AVCODEC_BUFFER_FLAGS_EOS) {
            cout << "decode eos" << endl;
            isRunning_.store(false);
            signal_->startCond_.notify_all();
        }
    }
    cout << "stop, exit" << endl;
    outputFile_->close();
}

int32_t AudioEncoderBufferCapiUnitTest::GetFileSize(const std::string &filePath)
{
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file) {
        std::cerr << "Failed to open file: " << filePath << std::endl;
        return -1;
    }

    std::streampos fileSize = file.tellg();  // 获取文件大小
    file.close();

    return (int32_t)fileSize;
}

int32_t AudioEncoderBufferCapiUnitTest::InitFile(AudioBufferFormatType audioType)
{
    if (audioType == AudioBufferFormatType::TYPE_FLAC) {
        fileSize_ = GetFileSize(FLAC_INPUT_FILE_PATH.data());
        inputFile_ = std::make_unique<std::ifstream>(FLAC_INPUT_FILE_PATH, std::ios::binary);
        outputFile_ = std::make_unique<std::ofstream>(FLAC_OUTPUT_FILE_PATH, std::ios::binary);
    } else if (audioType == AudioBufferFormatType::TYPE_AAC) {
        fileSize_ = GetFileSize(AAC_INPUT_FILE_PATH.data());
        inputFile_ = std::make_unique<std::ifstream>(AAC_INPUT_FILE_PATH, std::ios::binary);
        outputFile_ = std::make_unique<std::ofstream>(AAC_OUTPUT_FILE_PATH, std::ios::binary);
    } else if (audioType == AudioBufferFormatType::TYPE_G711MU) {
        fileSize_ = GetFileSize(G711MU_INPUT_FILE_PATH.data());
        inputFile_ = std::make_unique<std::ifstream>(G711MU_INPUT_FILE_PATH, std::ios::binary);
        outputFile_ = std::make_unique<std::ofstream>(G711MU_OUTPUT_FILE_PATH, std::ios::binary);
    } else if (audioType == AudioBufferFormatType::TYPE_OPUS) {
        fileSize_ = GetFileSize(AAC_INPUT_FILE_PATH.data());
        inputFile_ = std::make_unique<std::ifstream>(OPUS_INPUT_FILE_PATH, std::ios::binary);
        outputFile_ = std::make_unique<std::ofstream>(OPUS_OUTPUT_FILE_PATH, std::ios::binary);
    } else if (audioType == AudioBufferFormatType::TYPE_MP3) {
        fileSize_ = GetFileSize(MP3_INPUT_FILE_PATH.data());
        inputFile_ = std::make_unique<std::ifstream>(MP3_INPUT_FILE_PATH, std::ios::binary);
        outputFile_ = std::make_unique<std::ofstream>(MP3_OUTPUT_FILE_PATH, std::ios::binary);
    } else {
        cout << "audio name not support" << endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }
    if (!inputFile_->is_open()) {
        cout << "Fatal: open input file failed" << endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }
    if (!outputFile_->is_open()) {
        cout << "Fatal: open output file failed" << endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }
    return OH_AVErrCode::AV_ERR_OK;
}

int32_t AudioEncoderBufferCapiUnitTest::CreateCodecFunc(AudioBufferFormatType audioType)
{
    if (audioType == AudioBufferFormatType::TYPE_FLAC) {
        audioEnc_ = OH_AudioCodec_CreateByName((AVCodecCodecName::AUDIO_ENCODER_FLAC_NAME).data());
    } else if (audioType == AudioBufferFormatType::TYPE_AAC) {
        audioEnc_ = OH_AudioCodec_CreateByName((AVCodecCodecName::AUDIO_ENCODER_AAC_NAME).data());
    } else if (audioType == AudioBufferFormatType::TYPE_G711MU) {
        audioEnc_ = OH_AudioCodec_CreateByName((AVCodecCodecName::AUDIO_ENCODER_G711MU_NAME).data());
    } else if (audioType == AudioBufferFormatType::TYPE_OPUS) {
        audioEnc_ = OH_AudioCodec_CreateByName((AVCodecCodecName::AUDIO_ENCODER_OPUS_NAME).data());
    } else if (audioType == AudioBufferFormatType::TYPE_MP3) {
        audioEnc_ = OH_AudioCodec_CreateByName((AVCodecCodecName::AUDIO_ENCODER_MP3_NAME).data());
    } else {
        cout << "audio name not support" << endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }

    if (audioEnc_ == nullptr) {
        cout << "Fatal: CreateByName fail" << endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }

    signal_ = new AudioCodecBufferSignal();
    if (signal_ == nullptr) {
        cout << "Fatal: create signal fail" << endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }
    cb_ = {&OnError, &OnOutputFormatChanged, &OnInputBufferAvailable, &OnOutputBufferAvailable};
    int32_t ret = OH_AudioCodec_RegisterCallback(audioEnc_, cb_, signal_);
    if (ret != OH_AVErrCode::AV_ERR_OK) {
        cout << "Fatal: SetCallback fail" << endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }

    return OH_AVErrCode::AV_ERR_OK;
}

int32_t AudioEncoderBufferCapiUnitTest::Start()
{
    isRunning_.store(true);
    inputLoop_ = make_unique<thread>(&AudioEncoderBufferCapiUnitTest::InputFunc, this);
    if (inputLoop_ == nullptr) {
        cout << "Fatal: No memory" << endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }

    outputLoop_ = make_unique<thread>(&AudioEncoderBufferCapiUnitTest::OutputFunc, this);
    if (outputLoop_ == nullptr) {
        cout << "Fatal: No memory" << endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }

    return OH_AudioCodec_Start(audioEnc_);
}

void AudioEncoderBufferCapiUnitTest::JoinThread()
{
    SleepTest();
    isRunning_.store(false);

    if (inputLoop_ != nullptr && inputLoop_->joinable()) {
        {
            unique_lock<mutex> lock(signal_->inMutex_);
            signal_->inCond_.notify_all();
        }
        inputLoop_->join();
        inputLoop_ = nullptr;
        while (!signal_->inQueue_.empty()) {
            signal_->inQueue_.pop();
        }
        while (!signal_->inBufferQueue_.empty()) {
            signal_->inBufferQueue_.pop();
        }
    }

    if (outputLoop_ != nullptr && outputLoop_->joinable()) {
        {
            unique_lock<mutex> lock(signal_->outMutex_);
            signal_->outCond_.notify_all();
        }
        outputLoop_->join();
        outputLoop_ = nullptr;
        while (!signal_->outQueue_.empty()) {
            signal_->outQueue_.pop();
        }
        while (!signal_->outBufferQueue_.empty()) {
            signal_->outBufferQueue_.pop();
        }
    }
}

int32_t AudioEncoderBufferCapiUnitTest::Stop()
{
    isRunning_.store(false);

    if (inputLoop_ != nullptr && inputLoop_->joinable()) {
        {
            unique_lock<mutex> lock(signal_->inMutex_);
            signal_->inCond_.notify_all();
        }
        inputLoop_->join();
        inputLoop_ = nullptr;
        while (!signal_->inQueue_.empty()) {
            signal_->inQueue_.pop();
        }
        while (!signal_->inBufferQueue_.empty()) {
            signal_->inBufferQueue_.pop();
        }
    }

    if (outputLoop_ != nullptr && outputLoop_->joinable()) {
        {
            unique_lock<mutex> lock(signal_->outMutex_);
            signal_->outCond_.notify_all();
        }
        outputLoop_->join();
        outputLoop_ = nullptr;
        while (!signal_->outQueue_.empty()) {
            signal_->outQueue_.pop();
        }
        while (!signal_->outBufferQueue_.empty()) {
            signal_->outBufferQueue_.pop();
        }
    }

    return OH_AudioCodec_Stop(audioEnc_);
}

int32_t AudioEncoderBufferCapiUnitTest::CheckSoFunc()
{
    soFile_ = std::make_unique<std::ifstream>(OPUS_SO_FILE_PATH, std::ios::binary);
    if (!soFile_->is_open()) {
        cout << "Fatal: Open so file failed" << endl;
        return false;
    }
    soFile_->close();
    return true;
}

HWTEST_F(AudioEncoderBufferCapiUnitTest, encodeTest_01, TestSize.Level1)
{
    InitFile(AudioBufferFormatType::TYPE_AAC);
    CreateCodecFunc(AudioBufferFormatType::TYPE_AAC);
    format = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), SAMPLE_RATE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(),
                            AudioSampleFormat::SAMPLE_F32LE);
    
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_MAX_INPUT_SIZE.data(), 0);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), CHANNEL_COUNT);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioEnc_, format)); // normal channel count
    Start();

    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }

    JoinThread();
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Flush(audioEnc_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Destroy(audioEnc_));
}


HWTEST_F(AudioEncoderBufferCapiUnitTest, encodeTest_02, TestSize.Level1)
{
    InitFile(AudioBufferFormatType::TYPE_AAC);
    CreateCodecFunc(AudioBufferFormatType::TYPE_AAC);
    format = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), SAMPLE_RATE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(),
                            AudioSampleFormat::SAMPLE_F32LE);
    
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_MAX_INPUT_SIZE.data(), 1);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), CHANNEL_COUNT);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioEnc_, format)); // normal channel count
    Start();

    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }

    JoinThread();
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Flush(audioEnc_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Destroy(audioEnc_));
}

HWTEST_F(AudioEncoderBufferCapiUnitTest, mp3CheckChannelCount, TestSize.Level1)
{
    CreateCodecFunc(AudioBufferFormatType::TYPE_MP3);
    format = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), SAMPLE_RATE_8K);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(),
                            AudioSampleFormat::SAMPLE_S16LE);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_BITRATE.data(), MP3_BIT_RATE);

    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioEnc_, format)); // missing channel count
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioEnc_));
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), ILLEGAL_CHANNEL_COUNT);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioEnc_, format)); // illegal channel count

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioEnc_));
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), CHANNEL_1CHAN_COUNT);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioEnc_, format)); // normal channel count

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Destroy(audioEnc_));
}

HWTEST_F(AudioEncoderBufferCapiUnitTest, mp3CheckSampleFormat, TestSize.Level1)
{
    CreateCodecFunc(AudioBufferFormatType::TYPE_MP3);
    format = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), CHANNEL_1CHAN_COUNT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), SAMPLE_RATE_8K);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_BITRATE.data(), MP3_BIT_RATE);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioEnc_, format)); // missing sample format

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioEnc_));
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(),
                            AudioSampleFormat::SAMPLE_U8);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioEnc_, format)); // illegal sample format

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioEnc_));
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(),
                            AudioSampleFormat::SAMPLE_S16LE);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioEnc_, format)); // normal sample format

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Destroy(audioEnc_));
}

HWTEST_F(AudioEncoderBufferCapiUnitTest, mp3CheckSampleRate, TestSize.Level1)
{
    CreateCodecFunc(AudioBufferFormatType::TYPE_MP3);
    format = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), CHANNEL_1CHAN_COUNT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(),
                            AudioSampleFormat::SAMPLE_S16LE);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_BITRATE.data(), MP3_BIT_RATE);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioEnc_, format)); // missing sample rate

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioEnc_));
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), ILLEGAL_SAMPLE_RATE);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioEnc_, format)); // illegal sample rate

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioEnc_));
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), SAMPLE_RATE);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioEnc_, format)); // normal sample rate

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Destroy(audioEnc_));
}

HWTEST_F(AudioEncoderBufferCapiUnitTest, mp3CheckBitRate, TestSize.Level1)
{
    CreateCodecFunc(AudioBufferFormatType::TYPE_MP3);
    format = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), CHANNEL_1CHAN_COUNT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(),
                            AudioSampleFormat::SAMPLE_S16LE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), SAMPLE_RATE);

    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioEnc_, format)); // missing bit rate

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioEnc_));
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_BITRATE.data(), ABNORMAL_BITS_RATE);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioEnc_, format)); // illegal bit rate

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioEnc_));
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_BITRATE.data(), MP3_BIT_RATE); // normal bit rate

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Destroy(audioEnc_));
}

HWTEST_F(AudioEncoderBufferCapiUnitTest, audioEncoder_mp3_Start, TestSize.Level1) {
    frameBytes_ = MP3_DEFAULT_FRAME_BYTES;
    format = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(),
                            AudioSampleFormat::SAMPLE_S16LE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), SAMPLE_RATE);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_BITRATE.data(), MP3_BIT_RATE);
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_MP3));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_MP3));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioEnc_, format));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Prepare(audioEnc_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Stop());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Start(audioEnc_));
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    OH_AVFormat_Destroy(format);
    Release();
}

HWTEST_F(AudioEncoderBufferCapiUnitTest, audioEncoder_normalcase_mp3, TestSize.Level1)
{
    frameBytes_ = MP3_DEFAULT_FRAME_BYTES;
    InitFile(AudioBufferFormatType::TYPE_MP3);
    CreateCodecFunc(AudioBufferFormatType::TYPE_MP3);
    format = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(),
                            AudioSampleFormat::SAMPLE_S16LE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), SAMPLE_RATE);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_BITRATE.data(), MP3_BIT_RATE);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioEnc_, format));
    OH_AVFormat_Destroy(format);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Stop());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Destroy(audioEnc_));
}

HWTEST_F(AudioEncoderBufferCapiUnitTest, audioEncoder_CreateByName_01, TestSize.Level1)
{
    audioEnc_ = OH_AudioCodec_CreateByName((AVCodecCodecName::AUDIO_ENCODER_AAC_NAME).data());
    EXPECT_NE(nullptr, audioEnc_);
    Release();
}

HWTEST_F(AudioEncoderBufferCapiUnitTest, audioEncoder_CreateByMime_01, TestSize.Level1)
{
    audioEnc_ = OH_AudioCodec_CreateByMime((AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_FLAC).data(), true);
    EXPECT_NE(nullptr, audioEnc_);
}

HWTEST_F(AudioEncoderBufferCapiUnitTest, audioEncoder_Prepare_01, TestSize.Level1)
{
    CreateCodecFunc(AudioBufferFormatType::TYPE_FLAC);
    format = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), SAMPLE_RATE);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_BITRATE.data(), BITS_RATE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE.data(), BITS_PER_CODED_SAMPLE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(), SAMPLE_FORMAT);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT.data(), CHANNEL_LAYOUT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_COMPLIANCE_LEVEL.data(), COMPLIANCE_LEVEL);

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioEnc_, format));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Prepare(audioEnc_));
}

HWTEST_F(AudioEncoderBufferCapiUnitTest, audioEncoder_GetOutputDescription_01, TestSize.Level1)
{
    CreateCodecFunc(AudioBufferFormatType::TYPE_FLAC);
    format = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), SAMPLE_RATE);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_BITRATE.data(), BITS_RATE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE.data(), BITS_PER_CODED_SAMPLE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(), SAMPLE_FORMAT);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT.data(), CHANNEL_LAYOUT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_COMPLIANCE_LEVEL.data(), COMPLIANCE_LEVEL);

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioEnc_, format));
    EXPECT_NE(nullptr, OH_AudioCodec_GetOutputDescription(audioEnc_));
}

HWTEST_F(AudioEncoderBufferCapiUnitTest, audioEncoder_IsValid_01, TestSize.Level1)
{
    CreateCodecFunc(AudioBufferFormatType::TYPE_FLAC);
    format = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), SAMPLE_RATE);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_BITRATE.data(), BITS_RATE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE.data(), BITS_PER_CODED_SAMPLE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(), SAMPLE_FORMAT);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT.data(), CHANNEL_LAYOUT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_COMPLIANCE_LEVEL.data(), COMPLIANCE_LEVEL);

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioEnc_, format));
    bool value = true;
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_IsValid(audioEnc_, &value));
}

HWTEST_F(AudioEncoderBufferCapiUnitTest, audioEncoder_SetParameter_01, TestSize.Level1)
{
    CreateCodecFunc(AudioBufferFormatType::TYPE_FLAC);
    format = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), SAMPLE_RATE);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_BITRATE.data(), BITS_RATE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE.data(), BITS_PER_CODED_SAMPLE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(), SAMPLE_FORMAT);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT.data(), CHANNEL_LAYOUT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_COMPLIANCE_LEVEL.data(), COMPLIANCE_LEVEL);

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioEnc_, format));

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Start(audioEnc_));

    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }

    JoinThread();

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Flush(audioEnc_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_SetParameter(audioEnc_, format));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Destroy(audioEnc_));
}


HWTEST_F(AudioEncoderBufferCapiUnitTest, audioEncoder_Configure_01, TestSize.Level1)
{
    CreateCodecFunc(AudioBufferFormatType::TYPE_FLAC);
    format = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), SAMPLE_RATE);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_BITRATE.data(), BITS_RATE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE.data(), BITS_PER_CODED_SAMPLE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(), SAMPLE_FORMAT);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT.data(), CHANNEL_LAYOUT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_COMPLIANCE_LEVEL.data(), COMPLIANCE_LEVEL);

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioEnc_, format));
}

HWTEST_F(AudioEncoderBufferCapiUnitTest, audioEncoder_Configure_02, TestSize.Level1)
{
    CreateCodecFunc(AudioBufferFormatType::TYPE_FLAC);
    format = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), SAMPLE_RATE);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_BITRATE.data(), BITS_RATE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE.data(), BITS_PER_CODED_SAMPLE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(), SAMPLE_FORMAT);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT.data(), ABNORMAL_CHANNEL_LAYOUT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_COMPLIANCE_LEVEL.data(), COMPLIANCE_LEVEL);

    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioEnc_, format));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioEnc_));

    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT.data(),
        ABNORMAL_CHANNEL_LAYOUT_UNKNOWN);

    ASSERT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioEnc_, format));
}

HWTEST_F(AudioEncoderBufferCapiUnitTest, audioEncoder_Configure_03, TestSize.Level1)
{
    CreateCodecFunc(AudioBufferFormatType::TYPE_FLAC);
    format = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), SAMPLE_RATE);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_BITRATE.data(), BITS_RATE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE.data(), ABNORMAL_BITS_SAMPLE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(), SAMPLE_FORMAT);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT.data(), CHANNEL_LAYOUT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_COMPLIANCE_LEVEL.data(), COMPLIANCE_LEVEL);

    ASSERT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioEnc_, format));
}

HWTEST_F(AudioEncoderBufferCapiUnitTest, audioEncoder_Configure_04, TestSize.Level1)
{
    CreateCodecFunc(AudioBufferFormatType::TYPE_FLAC);
    format = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), ABNORMAL_CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), SAMPLE_RATE);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_BITRATE.data(), BITS_RATE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE.data(), BITS_PER_CODED_SAMPLE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(), SAMPLE_FORMAT);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT.data(), CHANNEL_LAYOUT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_COMPLIANCE_LEVEL.data(), COMPLIANCE_LEVEL);

    ASSERT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioEnc_, format));
}

HWTEST_F(AudioEncoderBufferCapiUnitTest, audioEncoder_Configure_05, TestSize.Level1)
{
    CreateCodecFunc(AudioBufferFormatType::TYPE_FLAC);
    format = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), SAMPLE_RATE);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_BITRATE.data(), ABNORMAL_BITS_RATE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE.data(), BITS_PER_CODED_SAMPLE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(), SAMPLE_FORMAT);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT.data(), CHANNEL_LAYOUT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_COMPLIANCE_LEVEL.data(), COMPLIANCE_LEVEL);

    ASSERT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioEnc_, format));
}

HWTEST_F(AudioEncoderBufferCapiUnitTest, audioEncoder_Configure_06, TestSize.Level1)
{
    CreateCodecFunc(AudioBufferFormatType::TYPE_FLAC);
    format = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), SAMPLE_RATE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_BITRATE.data(), BITS_RATE);    // SetIntValue
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE.data(), BITS_PER_CODED_SAMPLE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(), SAMPLE_FORMAT);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT.data(), CHANNEL_LAYOUT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_COMPLIANCE_LEVEL.data(), COMPLIANCE_LEVEL);

    ASSERT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioEnc_, format));
}

HWTEST_F(AudioEncoderBufferCapiUnitTest, audioEncoder_Configure_07, TestSize.Level1)
{
    CreateCodecFunc(AudioBufferFormatType::TYPE_FLAC);
    format = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), SAMPLE_RATE);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_BITRATE.data(), BITS_RATE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE.data(), BITS_PER_CODED_SAMPLE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(), SAMPLE_FORMAT);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT.data(), CHANNEL_LAYOUT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_COMPLIANCE_LEVEL.data(), ABNORMAL_COMPLIANCE_LEVEL_L);

    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioEnc_, format));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioEnc_));

    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_COMPLIANCE_LEVEL.data(), ABNORMAL_COMPLIANCE_LEVEL_R);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioEnc_, format));
}

HWTEST_F(AudioEncoderBufferCapiUnitTest, audioEncoder_Configure_08, TestSize.Level1)
{
    CreateCodecFunc(AudioBufferFormatType::TYPE_FLAC);
    format = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), ABNORMAL_SAMPLE_RATE);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_BITRATE.data(), BITS_RATE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE.data(), BITS_PER_CODED_SAMPLE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(), SAMPLE_FORMAT);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT.data(), CHANNEL_LAYOUT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_COMPLIANCE_LEVEL.data(), COMPLIANCE_LEVEL);

    ASSERT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioEnc_, format));
}

HWTEST_F(AudioEncoderBufferCapiUnitTest, audioEncoder_Configure_09, TestSize.Level1)
{
    CreateCodecFunc(AudioBufferFormatType::TYPE_FLAC);
    format = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), ABNORMAL_SAMPLE_RATE);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_BITRATE.data(), BITS_RATE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE.data(), BITS_PER_CODED_SAMPLE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(), ABNORMAL_SAMPLE_FORMAT);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT.data(), CHANNEL_LAYOUT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_COMPLIANCE_LEVEL.data(), COMPLIANCE_LEVEL);

    ASSERT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioEnc_, format));
}

HWTEST_F(AudioEncoderBufferCapiUnitTest, audioEncoder_normalcase_01, TestSize.Level1)
{
    InitFile(AudioBufferFormatType::TYPE_FLAC);
    CreateCodecFunc(AudioBufferFormatType::TYPE_FLAC);
    format = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), SAMPLE_RATE);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_BITRATE.data(), BITS_RATE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE.data(), BITS_PER_CODED_SAMPLE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(), SAMPLE_FORMAT);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT.data(), CHANNEL_LAYOUT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_COMPLIANCE_LEVEL.data(), COMPLIANCE_LEVEL);

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioEnc_, format));
    OH_AVFormat_Destroy(format);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());

    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }

    JoinThread();
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Destroy(audioEnc_));
}

HWTEST_F(AudioEncoderBufferCapiUnitTest, audioEncoder_normalcase_02, TestSize.Level1)
{
    InitFile(AudioBufferFormatType::TYPE_FLAC);
    CreateCodecFunc(AudioBufferFormatType::TYPE_FLAC);
    format = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), SAMPLE_RATE);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_BITRATE.data(), BITS_RATE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE.data(), BITS_PER_CODED_SAMPLE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(), SAMPLE_FORMAT);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT.data(), CHANNEL_LAYOUT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_COMPLIANCE_LEVEL.data(), COMPLIANCE_LEVEL);

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioEnc_, format));
    OH_AVFormat_Destroy(format);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());

    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Stop());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Destroy(audioEnc_));
}

HWTEST_F(AudioEncoderBufferCapiUnitTest, audioEncoder_normalcase_03, TestSize.Level1)
{
    InitFile(AudioBufferFormatType::TYPE_FLAC);
    CreateCodecFunc(AudioBufferFormatType::TYPE_FLAC);
    format = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), SAMPLE_RATE);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_BITRATE.data(), BITS_RATE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE.data(), BITS_PER_CODED_SAMPLE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(), SAMPLE_FORMAT);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT.data(), CHANNEL_LAYOUT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_COMPLIANCE_LEVEL.data(), COMPLIANCE_LEVEL);

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioEnc_, format));
    OH_AVFormat_Destroy(format);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());

    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }

    JoinThread();
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Flush(audioEnc_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Destroy(audioEnc_));
}

HWTEST_F(AudioEncoderBufferCapiUnitTest, audioEncoder_normalcase_04, TestSize.Level1)
{
    InitFile(AudioBufferFormatType::TYPE_FLAC);
    CreateCodecFunc(AudioBufferFormatType::TYPE_FLAC);
    format = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), SAMPLE_RATE);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_BITRATE.data(), BITS_RATE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE.data(), BITS_PER_CODED_SAMPLE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(), SAMPLE_FORMAT);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT.data(), CHANNEL_LAYOUT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_COMPLIANCE_LEVEL.data(), COMPLIANCE_LEVEL);

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioEnc_, format));
    OH_AVFormat_Destroy(format);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());

    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }

    JoinThread();
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioEnc_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Destroy(audioEnc_));
}

HWTEST_F(AudioEncoderBufferCapiUnitTest, audioEncoder_normalcase_05, TestSize.Level1)
{
    InitFile(AudioBufferFormatType::TYPE_FLAC);
    CreateCodecFunc(AudioBufferFormatType::TYPE_FLAC);
    format = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), SAMPLE_RATE);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_BITRATE.data(), BITS_RATE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE.data(), BITS_PER_CODED_SAMPLE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(), SAMPLE_FORMAT);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT.data(), CHANNEL_LAYOUT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_COMPLIANCE_LEVEL.data(), COMPLIANCE_LEVEL);

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioEnc_, format));
    OH_AVFormat_Destroy(format);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());

    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }

    JoinThread();
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Flush(audioEnc_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Destroy(audioEnc_));
}

HWTEST_F(AudioEncoderBufferCapiUnitTest, aacNormalCase, TestSize.Level1)
{
    InitFile(AudioBufferFormatType::TYPE_AAC);
    CreateCodecFunc(AudioBufferFormatType::TYPE_AAC);
    format = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), SAMPLE_RATE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(),
                            AudioSampleFormat::SAMPLE_F32LE);

    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), CHANNEL_COUNT);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioEnc_, format)); // normal channel count
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());

    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }

    JoinThread();
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Flush(audioEnc_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Destroy(audioEnc_));
}

HWTEST_F(AudioEncoderBufferCapiUnitTest, aacCheckChannelCount, TestSize.Level1)
{
    CreateCodecFunc(AudioBufferFormatType::TYPE_AAC);
    format = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), SAMPLE_RATE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(),
                            AudioSampleFormat::SAMPLE_F32LE);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioEnc_, format)); // missing channel count

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioEnc_));
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), ILLEGAL_CHANNEL_COUNT);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioEnc_, format)); // illegal channel count

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioEnc_));
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), AAC_ILLEGAL_CHANNEL_COUNT);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioEnc_, format)); // aac illegal channel count

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioEnc_));
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), CHANNEL_COUNT);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioEnc_, format)); // normal channel count

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Destroy(audioEnc_));
}

HWTEST_F(AudioEncoderBufferCapiUnitTest, aacCheckSampleFormat, TestSize.Level1)
{
    CreateCodecFunc(AudioBufferFormatType::TYPE_AAC);
    format = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), SAMPLE_RATE);
    
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_BITRATE.data(), BITS_RATE);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioEnc_, format)); // missing sample format

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioEnc_));
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(),
                            AudioSampleFormat::SAMPLE_U8);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioEnc_, format)); // illegal sample format

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioEnc_));
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(),
                            AudioSampleFormat::SAMPLE_F32LE);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioEnc_, format)); // normal sample format

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Destroy(audioEnc_));
}

HWTEST_F(AudioEncoderBufferCapiUnitTest, aacCheckbitRate, TestSize.Level1)
{
    CreateCodecFunc(AudioBufferFormatType::TYPE_AAC);
    format = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), SAMPLE_RATE);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_BITRATE.data(), 500001); // wrong bit rate
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(),
                            AudioSampleFormat::SAMPLE_F32LE);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioEnc_, format));

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Destroy(audioEnc_));
}

HWTEST_F(AudioEncoderBufferCapiUnitTest, aacCheckSampleRate, TestSize.Level1)
{
    CreateCodecFunc(AudioBufferFormatType::TYPE_AAC);
    format = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(),
                            AudioSampleFormat::SAMPLE_F32LE);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioEnc_, format)); // missing sample rate

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioEnc_));
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), ILLEGAL_SAMPLE_RATE);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioEnc_, format)); // illegal sample rate

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioEnc_));
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), SAMPLE_RATE);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioEnc_, format)); // normal sample rate

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Destroy(audioEnc_));
}

HWTEST_F(AudioEncoderBufferCapiUnitTest, aacCheckChannelLayout, TestSize.Level1)
{
    CreateCodecFunc(AudioBufferFormatType::TYPE_AAC);
    format = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(),
                            AudioSampleFormat::SAMPLE_F32LE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), SAMPLE_RATE);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT.data(), CH_LAYOUT_7POINT1);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioEnc_, format)); // channel mismatch channel_layout

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioEnc_));
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT.data(), CHANNEL_LAYOUT);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioEnc_, format)); // normal channel layout

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Destroy(audioEnc_));
}

HWTEST_F(AudioEncoderBufferCapiUnitTest, aacCheckMaxinputSize, TestSize.Level1)
{
    frameBytes_ = AAC_DEFAULT_FRAME_BYTES;
    CreateCodecFunc(AudioBufferFormatType::TYPE_AAC);
    format = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(),
                            AudioSampleFormat::SAMPLE_F32LE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), SAMPLE_RATE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_MAX_INPUT_SIZE.data(), MAX_INPUT_SIZE);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioEnc_, format));
    OH_AVFormat_Destroy(format);

    auto fmt = OH_AudioCodec_GetOutputDescription(audioEnc_);
    EXPECT_NE(fmt, nullptr);
    OH_AVFormat_Destroy(fmt);

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Start(audioEnc_));

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Destroy(audioEnc_));
}

HWTEST_F(AudioEncoderBufferCapiUnitTest, g711muCheckChannelCount, TestSize.Level1)
{
    CreateCodecFunc(AudioBufferFormatType::TYPE_G711MU);
    format = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), SAMPLE_RATE_8K);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(),
                            AudioSampleFormat::SAMPLE_S16LE);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioEnc_, format)); // missing channel count

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioEnc_));
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), ILLEGAL_CHANNEL_COUNT);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioEnc_, format)); // illegal channel count

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioEnc_));
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), CHANNEL_1CHAN_COUNT);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioEnc_, format)); // normal channel count

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Destroy(audioEnc_));
}

HWTEST_F(AudioEncoderBufferCapiUnitTest, g711muCheckSampleFormat, TestSize.Level1)
{
    CreateCodecFunc(AudioBufferFormatType::TYPE_G711MU);
    format = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), CHANNEL_1CHAN_COUNT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), SAMPLE_RATE_8K);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioEnc_, format)); // missing sample format

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioEnc_));
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(),
                            AudioSampleFormat::SAMPLE_U8);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioEnc_, format)); // illegal sample format

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioEnc_));
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(),
                            AudioSampleFormat::SAMPLE_S16LE);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioEnc_, format)); // normal sample format

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Destroy(audioEnc_));
}

HWTEST_F(AudioEncoderBufferCapiUnitTest, g711muCheckSampleRate, TestSize.Level1)
{
    CreateCodecFunc(AudioBufferFormatType::TYPE_G711MU);
    format = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), CHANNEL_1CHAN_COUNT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(),
                            AudioSampleFormat::SAMPLE_S16LE);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioEnc_, format)); // missing sample rate

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioEnc_));
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), ILLEGAL_SAMPLE_RATE);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioEnc_, format)); // illegal sample rate

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioEnc_));
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), SAMPLE_RATE_8K);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioEnc_, format)); // normal sample rate

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Destroy(audioEnc_));
}

HWTEST_F(AudioEncoderBufferCapiUnitTest, audioEncoder_G711mu_Start, TestSize.Level1) {
    frameBytes_ = G711MU_DEFAULT_FRAME_BYTES;
    format = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), CHANNEL_1CHAN_COUNT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(),
                            AudioSampleFormat::SAMPLE_S16LE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), SAMPLE_RATE_8K);

    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_G711MU));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_G711MU));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioEnc_, format));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Prepare(audioEnc_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Stop());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Start(audioEnc_));
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    OH_AVFormat_Destroy(format);
    Release();
}

HWTEST_F(AudioEncoderBufferCapiUnitTest, opusCheckChannelCount, TestSize.Level1)
{
    if (!CheckSoFunc()) {
        return;
    }
    CreateCodecFunc(AudioBufferFormatType::TYPE_OPUS);
    format = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), OPUS_SAMPLE_RATE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(),
                            AudioSampleFormat::SAMPLE_S16LE);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_BITRATE.data(), BITS_RATE);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioEnc_, format)); // missing channel count

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioEnc_));
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), ILLEGAL_CHANNEL_COUNT);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioEnc_, format)); // illegal channel count

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioEnc_));
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), CHANNEL_COUNT);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioEnc_, format)); // normal channel count

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Destroy(audioEnc_));
}

HWTEST_F(AudioEncoderBufferCapiUnitTest, opusCheckSampleFormat, TestSize.Level1)
{
    if (!CheckSoFunc()) {
        return;
    }
    CreateCodecFunc(AudioBufferFormatType::TYPE_OPUS);
    format = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), OPUS_SAMPLE_RATE);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_BITRATE.data(), BITS_RATE);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioEnc_, format)); // missing sample format

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioEnc_));
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(),
                            AudioSampleFormat::SAMPLE_U8);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioEnc_, format)); // illegal sample format

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioEnc_));
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(),
                            AudioSampleFormat::SAMPLE_S16LE);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioEnc_, format)); // normal sample format

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Destroy(audioEnc_));
}

HWTEST_F(AudioEncoderBufferCapiUnitTest, opusCheckSampleRate, TestSize.Level1)
{
    if (!CheckSoFunc()) {
        return;
    }
    CreateCodecFunc(AudioBufferFormatType::TYPE_OPUS);
    format = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(),
                            AudioSampleFormat::SAMPLE_S16LE);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_BITRATE.data(), BITS_RATE);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioEnc_, format)); // missing sample rate

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioEnc_));
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), ILLEGAL_SAMPLE_RATE);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioEnc_, format)); // illegal sample rate

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioEnc_));
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), OPUS_SAMPLE_RATE);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioEnc_, format)); // normal sample rate

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Destroy(audioEnc_));
}

HWTEST_F(AudioEncoderBufferCapiUnitTest, opusCheckChannelLayout, TestSize.Level1)
{
    if (!CheckSoFunc()) {
        return;
    }
    CreateCodecFunc(AudioBufferFormatType::TYPE_OPUS);
    format = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(),
                            AudioSampleFormat::SAMPLE_S16LE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), OPUS_SAMPLE_RATE);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_BITRATE.data(), BITS_RATE);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioEnc_, format)); // channel mismatch channel_layout
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Destroy(audioEnc_));
}

HWTEST_F(AudioEncoderBufferCapiUnitTest, opusGetOutputDescription_01, TestSize.Level1)
{
    if (!CheckSoFunc()) {
        return;
    }
    CreateCodecFunc(AudioBufferFormatType::TYPE_OPUS);
    format = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(),
                            AudioSampleFormat::SAMPLE_S16LE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), OPUS_SAMPLE_RATE);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_BITRATE.data(), BITS_RATE);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioEnc_, format)); // channel mismatch channel_layout
    
    EXPECT_NE(nullptr, OH_AudioCodec_GetOutputDescription(audioEnc_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Destroy(audioEnc_));
}
}
}