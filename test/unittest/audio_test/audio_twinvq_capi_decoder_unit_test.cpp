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

#include <atomic>
#include <cstdint>
#include <fcntl.h>
#include <fstream>
#include <gtest/gtest.h>
#include <iostream>
#include <libavutil/samplefmt.h>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unistd.h>
#include "avcodec_audio_common.h"
#include "avcodec_codec_name.h"
#include "avcodec_common.h"
#include "avcodec_errors.h"
#include "avcodec_mime_type.h"
#include "common/native_mfmagic.h"
#include "common/status.h"
#include "media_description.h"
#include "native_audio_channel_layout.h"
#include "native_avbuffer.h"
#include "native_avcapability.h"
#include "native_avcodec_audiocodec.h"
#include "native_avcodec_audiodecoder.h"
#include "native_avcodec_base.h"
#include "native_avdemuxer.h"
#include "native_avsource.h"
#include "securec.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS::MediaAVCodec;

namespace {
constexpr size_t FIELDSIZE = 8;
const string CODEC_TWINVQ_NAME = std::string(std::string(AVCodecCodecName::AUDIO_DECODER_TWINVQ_NAME));
constexpr int32_t DEFAULT_CHANNELS = 1;
constexpr uint32_t DEFAULT_SAMPLE_RATE = 8000;
constexpr string_view TWINVQ_FILE_TODEMUX = "/data/test/media/twinvq_8000_mono.dat";
constexpr string_view OUTPUT_TWINVQ_PCM_FILE_PATH = "/data/test/media/test_decoder_twinvq.pcm";
constexpr string_view INPUT_TWINVQ_VQF_1_PATH = "/data/test/media/twinvq_8000_stereo.dat";
constexpr string_view INPUT_TWINVQ_VQF_2_PATH = "/data/test/media/twinvq_11025_mono.dat";
constexpr string_view INPUT_TWINVQ_VQF_3_PATH = "/data/test/media/twinvq_16000_mono.dat";
constexpr string_view INPUT_TWINVQ_VQF_4_PATH = "/data/test/media/twinvq_22050_mono.dat";
constexpr string_view INPUT_TWINVQ_VQF_5_PATH = "/data/test/media/twinvq_44100_mono.dat";
constexpr string_view INPUT_TWINVQ_VQF_6_PATH = "/data/test/media/twinvq_44100_mono_48kbs.dat";

constexpr string_view OUTPUT_TWINVQ_PCM_1_PATH = "/data/test/media/twinvq_8000_stereo.pcm";
constexpr string_view OUTPUT_TWINVQ_PCM_2_PATH = "/data/test/media/twinvq_11025_mono.pcm";
constexpr string_view OUTPUT_TWINVQ_PCM_3_PATH = "/data/test/media/twinvq_16000_mono.pcm";
constexpr string_view OUTPUT_TWINVQ_PCM_4_PATH = "/data/test/media/twinvq_22050_mono.pcm";
constexpr string_view OUTPUT_TWINVQ_PCM_5_PATH = "/data/test/media/twinvq_44100_mono.pcm";
constexpr string_view OUTPUT_TWINVQ_PCM_6_PATH = "/data/test/media/twinvq_44100_mono_48kbs.pcm";
constexpr string_view OUTPUT_TWINVQ_PCM_7_PATH = "/data/test/media/twinvq_8000_mono.pcm";
const string OPUS_SO_FILE_PATH = std::string(AV_CODEC_PATH) + "/libav_codec_ext_base.z.so";
} // namespace

namespace OHOS {
namespace MediaAVCodec {
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

static uint32_t g_outputFormatChangedTimes = 0;
static int32_t g_outputSampleRate = 0;
static int32_t g_outputChannels = 0;

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
    (void)userData;
    g_outputFormatChangedTimes++;
    OH_AVFormat_GetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, &g_outputChannels);
    OH_AVFormat_GetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, &g_outputSampleRate);
    cout << "OnOutputFormatChanged received, rate:" << g_outputSampleRate << ",channel:" << g_outputChannels << endl;
}

static void OnInputBufferAvailable(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *data, void *userData)
{
    (void)codec;
    AudioCodecBufferSignal *signal = static_cast<AudioCodecBufferSignal *>(userData);
    unique_lock<mutex> lock(signal->inMutex_);
    signal->inQueue_.push(index);
    signal->inBufferQueue_.push(data);
    signal->inCond_.notify_all();
}

static void OnOutputBufferAvailable(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *data, void *userData)
{
    (void)codec;
    AudioCodecBufferSignal *signal = static_cast<AudioCodecBufferSignal *>(userData);
    unique_lock<mutex> lock(signal->outMutex_);
    signal->outQueue_.push(index);
    signal->outBufferQueue_.push(data);
    signal->outCond_.notify_all();
}

class AudioCodeTwinvqDecoderUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
    bool ReadBuffer(OH_AVBuffer *buffer, uint32_t index);
    int32_t InitFile(string_view input, string_view output);
    void InputFunc();
    void OutputFunc();
    int32_t CreateCodecFunc();
    void HandleInputEOS(const uint32_t index);
    int32_t Configure();
    int32_t Start();
    int32_t Stop();
    void Release();
    int32_t SetExtraData(OH_AVFormat *format);

protected:
    std::atomic<bool> isRunning_ = false;
    std::unique_ptr<std::thread> inputLoop_;
    std::unique_ptr<std::thread> outputLoop_;
    struct OH_AVCodecCallback cb_;
    AudioCodecBufferSignal *signal_ = nullptr;
    OH_AVCodec *audioDec_ = nullptr;
    OH_AVFormat *format_ = nullptr;
    bool isTestingFormat_ = false;
    bool isFirstFrame_ = true;
    uint32_t frameCount_ = 0;
    std::ifstream infile;
    std::ofstream pcmOutputFile_;
    std::vector<uint8_t> rawData_;
    size_t currentDataPos_ = 0;
    bool headerParsed_ = false;
};

void AudioCodeTwinvqDecoderUnitTest::SetUpTestCase(void)
{
    cout << "[SetUpTestCase]: " << endl;
}

void AudioCodeTwinvqDecoderUnitTest::TearDownTestCase(void)
{
    cout << "[TearDownTestCase]: " << endl;
}

void AudioCodeTwinvqDecoderUnitTest::SetUp(void)
{
    g_outputFormatChangedTimes = 0;
    g_outputSampleRate = 0;
    g_outputChannels = 0;
    cout << "[SetUp]: SetUp!!!" << endl;
}

void AudioCodeTwinvqDecoderUnitTest::TearDown(void)
{
    if (isTestingFormat_) {
        EXPECT_EQ(g_outputFormatChangedTimes, 1);
    } else {
        EXPECT_EQ(g_outputFormatChangedTimes, 0);
    }
    cout << "[TearDown]: over!!!" << endl;

    if (signal_) {
        delete signal_;
        signal_ = nullptr;
    }
    if (infile.is_open()) {
        infile.close();
    }
    if (pcmOutputFile_.is_open()) {
        pcmOutputFile_.close();
    }

    if (format_ != nullptr) {
        OH_AVFormat_Destroy(format_);
        format_ = nullptr;
    }
}

void AudioCodeTwinvqDecoderUnitTest::Release()
{
    Stop();
    OH_AudioCodec_Destroy(audioDec_);
}

void AudioCodeTwinvqDecoderUnitTest::HandleInputEOS(uint32_t index)
{
    auto buffer = signal_->inBufferQueue_.front();
    buffer->buffer_->memory_->SetSize(0);
    buffer->buffer_->flag_ = AVCODEC_BUFFER_FLAGS_EOS;
    OH_AudioCodec_PushInputBuffer(audioDec_, index);
    signal_->inBufferQueue_.pop();
    signal_->inQueue_.pop();
}

bool AudioCodeTwinvqDecoderUnitTest::ReadBuffer(OH_AVBuffer *buffer, uint32_t index)
{
    constexpr size_t fileHeaderSize = 12;
    if (!headerParsed_) {
        if (rawData_.size() < fileHeaderSize) {
            cout << "Error: file too small for header" << endl;
            HandleInputEOS(index);
            return false;
        }
        uint32_t extraDataLen = 0;
        memcpy_s(&extraDataLen, sizeof(extraDataLen), rawData_.data() + FIELDSIZE, sizeof(extraDataLen));
        const size_t totalHeader = fileHeaderSize + extraDataLen;
        if (rawData_.size() < totalHeader) {
            HandleInputEOS(index);
            return false;
        }
        currentDataPos_ = totalHeader;
        headerParsed_ = true;
    }
    constexpr size_t frameHeaderSize = 16;
    if (currentDataPos_ + frameHeaderSize > rawData_.size()) {
        buffer->buffer_->memory_->SetSize(1);
        buffer->buffer_->flag_ = AVCODEC_BUFFER_FLAGS_EOS;
        HandleInputEOS(index);
        return false;
    }
    uint64_t frameSize = 0;
    int64_t pts = 0;
    memcpy_s(&frameSize, sizeof(frameSize), rawData_.data() + currentDataPos_, sizeof(frameSize));
    memcpy_s(&pts, sizeof(pts), rawData_.data() + currentDataPos_ + FIELDSIZE, sizeof(pts));
    currentDataPos_ += frameHeaderSize;
    if (currentDataPos_ + frameSize > rawData_.size()) {
        buffer->buffer_->memory_->SetSize(1);
        buffer->buffer_->flag_ = AVCODEC_BUFFER_FLAGS_EOS;
        HandleInputEOS(index);
        return false;
    }
    void *addr = OH_AVBuffer_GetAddr(buffer);
    errno_t ret = memcpy_s(addr, buffer->buffer_->memory_->GetCapacity(), rawData_.data() + currentDataPos_, frameSize);
    if (ret != EOK) {
        HandleInputEOS(index);
        return false;
    }
    buffer->buffer_->memory_->SetSize(frameSize);
    buffer->buffer_->pts_ = pts;
    buffer->buffer_->flag_ = isFirstFrame_ ? AVCODEC_BUFFER_FLAGS_CODEC_DATA : AVCODEC_BUFFER_FLAGS_NONE;
    currentDataPos_ += frameSize;
    isFirstFrame_ = false;
    return true;
}

void AudioCodeTwinvqDecoderUnitTest::InputFunc()
{
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
        if (ReadBuffer(buffer, index) == false) {
            break;
        }
        int32_t ret;
        if (isFirstFrame_) {
            buffer->buffer_->flag_ = AVCODEC_BUFFER_FLAGS_CODEC_DATA;
            ret = OH_AudioCodec_PushInputBuffer(audioDec_, index);
            isFirstFrame_ = false;
        } else {
            buffer->buffer_->flag_ = AVCODEC_BUFFER_FLAGS_NONE;
            ret = OH_AudioCodec_PushInputBuffer(audioDec_, index);
        }
        signal_->inQueue_.pop();
        signal_->inBufferQueue_.pop();
        frameCount_++;
        if (ret != AVCS_ERR_OK) {
            cout << "Fatal error, exit" << endl;
            break;
        }
    }
    cout << "stop, exit" << endl;
    infile.close();
}

void AudioCodeTwinvqDecoderUnitTest::OutputFunc()
{
    if (!pcmOutputFile_.is_open()) {
        std::cout << "open " << OUTPUT_TWINVQ_PCM_FILE_PATH << " failed!" << std::endl;
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
            cout << "OutputFunc OH_AVBuffer is nullptr" << endl;
            continue;
        }
        pcmOutputFile_.write(reinterpret_cast<char *>(OH_AVBuffer_GetAddr(data)), data->buffer_->memory_->GetSize());

        if (data != nullptr &&
            (data->buffer_->flag_ == AVCODEC_BUFFER_FLAGS_EOS || data->buffer_->memory_->GetSize() == 0)) {
            cout << "decode eos" << endl;
            isRunning_.store(false);
            signal_->startCond_.notify_all();
        }
        signal_->outBufferQueue_.pop();
        signal_->outQueue_.pop();
        if (OH_AudioCodec_FreeOutputBuffer(audioDec_, index) != AV_ERR_OK) {
            cout << "Fatal: FreeOutputData fail" << endl;
            break;
        }

        if (data->buffer_->flag_ == AVCODEC_BUFFER_FLAGS_EOS) {
            cout << "decode eos" << endl;
            isRunning_.store(false);
            signal_->startCond_.notify_all();
        }
    }
    cout << "stop, exit" << endl;
    pcmOutputFile_.close();
}

int32_t AudioCodeTwinvqDecoderUnitTest::Start()
{
    isRunning_.store(true);
    inputLoop_ = make_unique<thread>(&AudioCodeTwinvqDecoderUnitTest::InputFunc, this);
    if (inputLoop_ == nullptr) {
        cout << "Fatal: No memory" << endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }

    outputLoop_ = make_unique<thread>(&AudioCodeTwinvqDecoderUnitTest::OutputFunc, this);
    if (outputLoop_ == nullptr) {
        cout << "Fatal: No memory" << endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }

    return OH_AudioCodec_Start(audioDec_);
}

int32_t AudioCodeTwinvqDecoderUnitTest::Stop()
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
    std::cout << "start stop!\n";
    return OH_AudioCodec_Stop(audioDec_);
}

int32_t AudioCodeTwinvqDecoderUnitTest::InitFile(string_view inputFilename, string_view outputFilename)
{
    pcmOutputFile_.open(outputFilename.data(), std::ios::out | std::ios::binary);
    if (!pcmOutputFile_.is_open()) {
        cout << "Fatal: open output file failed: " << outputFilename << endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }
    infile.open(inputFilename.data(), std::ios::binary);
    if (!infile.is_open()) {
        cout << "Fatal: open input file failed: " << inputFilename << endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }

    infile.seekg(0, std::ios::end);
    size_t fileSize = infile.tellg();
    infile.seekg(0, std::ios::beg);
    rawData_.resize(fileSize);
    infile.read(reinterpret_cast<char*>(rawData_.data()), fileSize);
    infile.close();
    if (infile.gcount() != static_cast<std::streamsize>(fileSize)) {
        cout << "Error: Failed to read full input file" << endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }
    currentDataPos_ = 0;
    headerParsed_ = false;
    isFirstFrame_ = true;
    return OH_AVErrCode::AV_ERR_OK;
}

int32_t AudioCodeTwinvqDecoderUnitTest::CreateCodecFunc()
{
    audioDec_ = OH_AudioCodec_CreateByName((std::string(AVCodecCodecName::AUDIO_DECODER_TWINVQ_NAME)).data());
    if (audioDec_ == nullptr) {
        cout << "Fatal: CreateByName fail" << endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }
    signal_ = new AudioCodecBufferSignal();
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

int32_t AudioCodeTwinvqDecoderUnitTest::SetExtraData(OH_AVFormat *format)
{
    if (!format) {
        return OH_AVErrCode::AV_ERR_INVALID_VAL;
    }
    static constexpr size_t minHeaderSize{12};
    if (rawData_.size() < minHeaderSize) {
        std::cout << "Fatal: file too small for header" << std::endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }
    uint32_t extraDataSize{};
    if (memcpy_s(&extraDataSize, sizeof(extraDataSize),
                 rawData_.data() + FIELDSIZE, sizeof(extraDataSize)) != EOK) {
        std::cout << "Fatal: memcpy_s extraDataSize failed" << std::endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }
    if (extraDataSize == 0) {
        return OH_AVErrCode::AV_ERR_OK;
    }
    if (rawData_.size() < minHeaderSize + extraDataSize) {
        std::cout << "Fatal: file too small for extra data" << std::endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }
    OH_AVFormat_SetBuffer(format, OH_MD_KEY_CODEC_CONFIG,
                          rawData_.data() + minHeaderSize, extraDataSize);
    return OH_AVErrCode::AV_ERR_OK;
}

int32_t AudioCodeTwinvqDecoderUnitTest::Configure()
{
    format_ = OH_AVFormat_Create();
    if (format_ == nullptr) {
        cout << "Fatal: create format failed" << endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }
    uint32_t bitRate = 8000;
    OH_AVFormat_SetLongValue(format_, MediaDescriptionKey::MD_KEY_BITRATE.data(), bitRate);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), DEFAULT_CHANNELS);
    OH_AVFormat_SetLongValue(format_, MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT.data(),
        OH_AudioChannelLayout::CH_LAYOUT_MONO);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), DEFAULT_SAMPLE_RATE);
    int32_t ret = SetExtraData(format_);
    if (ret != OH_AVErrCode::AV_ERR_OK) {
        return ret;
    }
    SetExtraData(format_);
    return OH_AudioCodec_Configure(audioDec_, format_);
}

HWTEST_F(AudioCodeTwinvqDecoderUnitTest, AudioDemuxAndDecode_Twinvq_001, TestSize.Level1)
{
    isTestingFormat_ = true;
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(TWINVQ_FILE_TODEMUX, OUTPUT_TWINVQ_PCM_FILE_PATH));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    Release();
}

HWTEST_F(AudioCodeTwinvqDecoderUnitTest, CheckSampleFormat_Twinvq_001, TestSize.Level1)
{
    isTestingFormat_ = true;
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(INPUT_TWINVQ_VQF_1_PATH, OUTPUT_TWINVQ_PCM_1_PATH));
    ASSERT_EQ(AV_ERR_OK, CreateCodecFunc());
    OH_AVFormat *fmt = OH_AVFormat_Create();
    uint32_t bitRate = 16000;
    OH_AVFormat_SetLongValue(fmt, MediaDescriptionKey::MD_KEY_BITRATE.data(), bitRate);
    OH_AVFormat_SetIntValue(fmt, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), 2);
    OH_AVFormat_SetLongValue(fmt, MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT.data(),
        OH_AudioChannelLayout::CH_LAYOUT_STEREO);
    OH_AVFormat_SetIntValue(fmt, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), 8000);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, SetExtraData(fmt));
    OH_AVFormat_SetIntValue(fmt, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(),
        AVSampleFormat::AV_SAMPLE_FMT_FLTP);
    EXPECT_EQ(AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, fmt));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    OH_AVFormat_Destroy(fmt);
    Release();
}

HWTEST_F(AudioCodeTwinvqDecoderUnitTest, CheckSampleFormat_Twinvq_002, TestSize.Level1)
{
    isTestingFormat_ = true;
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(INPUT_TWINVQ_VQF_2_PATH, OUTPUT_TWINVQ_PCM_2_PATH));
    ASSERT_EQ(AV_ERR_OK, CreateCodecFunc());
    OH_AVFormat *fmt = OH_AVFormat_Create();
    uint32_t bitRate = 8000;
    OH_AVFormat_SetLongValue(fmt, MediaDescriptionKey::MD_KEY_BITRATE.data(), bitRate);
    OH_AVFormat_SetIntValue(fmt, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), 1);
    OH_AVFormat_SetLongValue(fmt, MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT.data(),
        OH_AudioChannelLayout::CH_LAYOUT_MONO);
    OH_AVFormat_SetIntValue(fmt, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), 11025);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, SetExtraData(fmt));
    OH_AVFormat_SetIntValue(fmt, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(),
        AVSampleFormat::AV_SAMPLE_FMT_FLTP);
    EXPECT_EQ(AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, fmt));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    OH_AVFormat_Destroy(fmt);
    Release();
}

HWTEST_F(AudioCodeTwinvqDecoderUnitTest, CheckSampleFormat_Twinvq_003, TestSize.Level1)
{
    isTestingFormat_ = true;
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(INPUT_TWINVQ_VQF_3_PATH, OUTPUT_TWINVQ_PCM_3_PATH));
    ASSERT_EQ(AV_ERR_OK, CreateCodecFunc());
    OH_AVFormat *fmt = OH_AVFormat_Create();
    uint32_t bitRate = 16000;
    OH_AVFormat_SetLongValue(fmt, MediaDescriptionKey::MD_KEY_BITRATE.data(), bitRate);
    OH_AVFormat_SetIntValue(fmt, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), 1);
    OH_AVFormat_SetLongValue(fmt, MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT.data(),
        OH_AudioChannelLayout::CH_LAYOUT_MONO);
    OH_AVFormat_SetIntValue(fmt, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), 16000);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, SetExtraData(fmt));
    OH_AVFormat_SetIntValue(fmt, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(),
        AVSampleFormat::AV_SAMPLE_FMT_FLTP);
    EXPECT_EQ(AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, fmt));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    OH_AVFormat_Destroy(fmt);
    Release();
}

HWTEST_F(AudioCodeTwinvqDecoderUnitTest, CheckSampleFormat_Twinvq_004, TestSize.Level1)
{
    isTestingFormat_ = true;
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(INPUT_TWINVQ_VQF_4_PATH, OUTPUT_TWINVQ_PCM_4_PATH));
    ASSERT_EQ(AV_ERR_OK, CreateCodecFunc());
    OH_AVFormat *fmt = OH_AVFormat_Create();
    uint32_t bitRate = 20000;
    OH_AVFormat_SetLongValue(fmt, MediaDescriptionKey::MD_KEY_BITRATE.data(), bitRate);
    OH_AVFormat_SetIntValue(fmt, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), 1);
    OH_AVFormat_SetLongValue(fmt, MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT.data(),
        OH_AudioChannelLayout::CH_LAYOUT_MONO);
    OH_AVFormat_SetIntValue(fmt, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), 22050);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, SetExtraData(fmt));
    OH_AVFormat_SetIntValue(fmt, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(),
        AVSampleFormat::AV_SAMPLE_FMT_FLTP);
    EXPECT_EQ(AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, fmt));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    OH_AVFormat_Destroy(fmt);
    Release();
}

HWTEST_F(AudioCodeTwinvqDecoderUnitTest, CheckSampleFormat_Twinvq_005, TestSize.Level1)
{
    isTestingFormat_ = true;
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(INPUT_TWINVQ_VQF_5_PATH, OUTPUT_TWINVQ_PCM_5_PATH));
    ASSERT_EQ(AV_ERR_OK, CreateCodecFunc());
    OH_AVFormat *fmt = OH_AVFormat_Create();
    uint32_t bitRate = 40000;
    OH_AVFormat_SetLongValue(fmt, MediaDescriptionKey::MD_KEY_BITRATE.data(), bitRate);
    OH_AVFormat_SetIntValue(fmt, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), 1);
    OH_AVFormat_SetLongValue(fmt, MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT.data(),
        OH_AudioChannelLayout::CH_LAYOUT_MONO);
    OH_AVFormat_SetIntValue(fmt, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), 44100);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, SetExtraData(fmt));;
    OH_AVFormat_SetIntValue(fmt, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(),
        AVSampleFormat::AV_SAMPLE_FMT_FLTP);
    EXPECT_EQ(AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, fmt));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    OH_AVFormat_Destroy(fmt);
    Release();
}

HWTEST_F(AudioCodeTwinvqDecoderUnitTest, CheckSampleFormat_Twinvq_006, TestSize.Level1)
{
    isTestingFormat_ = true;
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(INPUT_TWINVQ_VQF_6_PATH, OUTPUT_TWINVQ_PCM_6_PATH));
    ASSERT_EQ(AV_ERR_OK, CreateCodecFunc());
    OH_AVFormat *fmt = OH_AVFormat_Create();
    uint32_t bitRate = 48000;
    OH_AVFormat_SetLongValue(fmt, MediaDescriptionKey::MD_KEY_BITRATE.data(), bitRate);
    OH_AVFormat_SetIntValue(fmt, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), 1);
    OH_AVFormat_SetLongValue(fmt, MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT.data(),
        OH_AudioChannelLayout::CH_LAYOUT_MONO);
    OH_AVFormat_SetIntValue(fmt, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), 44100);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, SetExtraData(fmt));
    OH_AVFormat_SetIntValue(fmt, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(),
        AVSampleFormat::AV_SAMPLE_FMT_FLTP);
    EXPECT_EQ(AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, fmt));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    OH_AVFormat_Destroy(fmt);
    Release();
}

HWTEST_F(AudioCodeTwinvqDecoderUnitTest, capability_001, TestSize.Level1)
{
    OH_AVCapability *cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_AUDIO_TWINVQ, false, SOFTWARE);
    EXPECT_NE(cap, nullptr);
    const int threadCnt = 10;
    std::vector<std::thread> threadPool;
    for (int32_t i = 0; i < threadCnt; i++) {
        threadPool.emplace_back([&cap]() {
            const int32_t *sampleRates = nullptr;
            uint32_t sampleRateNum = 0;
            OH_AVRange* sampleRateRanges[10];
            uint32_t rangesNum = 0;
            OH_AVRange channelCountRange = {0, 0};
            for (int i = 0; i < 10; i++) {
                sampleRateRanges[i] = nullptr;
            }
            EXPECT_EQ(OH_AVCapability_GetAudioSupportedSampleRates(cap, &sampleRates, &sampleRateNum), AV_ERR_OK);
            EXPECT_EQ(OH_AVCapability_GetAudioSupportedSampleRates(cap, &sampleRates, &sampleRateNum), AV_ERR_OK);
            EXPECT_EQ(OH_AVCapability_GetAudioSupportedSampleRates(cap, &sampleRates, &sampleRateNum), AV_ERR_OK);
            EXPECT_EQ(OH_AVCapability_GetAudioSupportedSampleRateRanges(cap, sampleRateRanges, &rangesNum), AV_ERR_OK);
            EXPECT_EQ(OH_AVCapability_GetAudioSupportedSampleRateRanges(cap, sampleRateRanges, &rangesNum), AV_ERR_OK);
            EXPECT_EQ(OH_AVCapability_GetAudioSupportedSampleRateRanges(cap, sampleRateRanges, &rangesNum), AV_ERR_OK);
            EXPECT_EQ(OH_AVCapability_GetAudioChannelCountRange(cap, &channelCountRange), AV_ERR_OK);
            EXPECT_EQ(OH_AVCapability_GetAudioChannelCountRange(cap, &channelCountRange), AV_ERR_OK);
            EXPECT_EQ(OH_AVCapability_GetAudioChannelCountRange(cap, &channelCountRange), AV_ERR_OK);
        });
    }
    for (auto &th : threadPool) {
        th.join();
    }
}

HWTEST_F(AudioCodeTwinvqDecoderUnitTest, CheckSampleInvalidFormat_Twinvq_001, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(TWINVQ_FILE_TODEMUX, OUTPUT_TWINVQ_PCM_7_PATH));
    ASSERT_EQ(AV_ERR_OK, CreateCodecFunc());
    OH_AVFormat *fmt = OH_AVFormat_Create();
    uint32_t bitRate = 48000;
    OH_AVFormat_SetLongValue(fmt, MediaDescriptionKey::MD_KEY_BITRATE.data(), bitRate);
    OH_AVFormat_SetIntValue(fmt, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), 5);
    OH_AVFormat_SetLongValue(fmt, MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT.data(),
        OH_AudioChannelLayout::CH_LAYOUT_MONO);
    OH_AVFormat_SetIntValue(fmt, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), DEFAULT_SAMPLE_RATE);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, SetExtraData(fmt));
    OH_AVFormat_SetIntValue(fmt, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(),
        AVSampleFormat::AV_SAMPLE_FMT_FLTP);
    EXPECT_EQ(AV_ERR_INVALID_VAL, OH_AudioCodec_Configure(audioDec_, fmt));
    Release();
}

HWTEST_F(AudioCodeTwinvqDecoderUnitTest, CheckSampleInvalidFormat_Twinvq_002, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(TWINVQ_FILE_TODEMUX, OUTPUT_TWINVQ_PCM_7_PATH));
    ASSERT_EQ(AV_ERR_OK, CreateCodecFunc());
    OH_AVFormat *fmt = OH_AVFormat_Create();
    uint32_t bitRate = 48000;
    OH_AVFormat_SetLongValue(fmt, MediaDescriptionKey::MD_KEY_BITRATE.data(), bitRate);
    OH_AVFormat_SetIntValue(fmt, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), 1);
    OH_AVFormat_SetLongValue(fmt, MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT.data(),
        OH_AudioChannelLayout::CH_LAYOUT_MONO);
    OH_AVFormat_SetIntValue(fmt, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), 96000);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, SetExtraData(fmt));
    OH_AVFormat_SetIntValue(fmt, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(),
        AVSampleFormat::AV_SAMPLE_FMT_FLTP);
    EXPECT_EQ(AV_ERR_INVALID_VAL, OH_AudioCodec_Configure(audioDec_, fmt));
    Release();
}

HWTEST_F(AudioCodeTwinvqDecoderUnitTest, audioDecoder_Twinvq_CreateByName_01, TestSize.Level1)
{
    audioDec_ = OH_AudioCodec_CreateByName((std::string(AVCodecCodecName::AUDIO_DECODER_TWINVQ_NAME)).data());
    EXPECT_NE(nullptr, audioDec_);
    Release();
}

HWTEST_F(AudioCodeTwinvqDecoderUnitTest, audioDecoder_Twinvq_CreateByName_02, TestSize.Level1)
{
    audioDec_ = OH_AudioCodec_CreateByName(nullptr);
    EXPECT_EQ(nullptr, audioDec_);
}

HWTEST_F(AudioCodeTwinvqDecoderUnitTest, audioDecoder_Twinvq_CreateByName_03, TestSize.Level1)
{
    audioDec_ = OH_AudioCodec_CreateByName("audio_decoder.twinvq");
    EXPECT_EQ(nullptr, audioDec_);
}

HWTEST_F(AudioCodeTwinvqDecoderUnitTest, audioDecoder_Twinvq_CreateByMime_01, TestSize.Level1)
{
    audioDec_ = OH_AudioCodec_CreateByMime(std::string(AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_TWINVQ).data(), false);
    EXPECT_NE(nullptr, audioDec_);
    Release();
}

HWTEST_F(AudioCodeTwinvqDecoderUnitTest, audioDecoder_Twinvq_CreateByMime_02, TestSize.Level1)
{
    audioDec_ = OH_AudioCodec_CreateByMime(nullptr, false);
    EXPECT_EQ(nullptr, audioDec_);
    Release();
}

HWTEST_F(AudioCodeTwinvqDecoderUnitTest, audioDecoder_Twinvq_PushInputData_InvalidIndex_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(TWINVQ_FILE_TODEMUX, OUTPUT_TWINVQ_PCM_FILE_PATH));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    const uint32_t index = 1024;
    EXPECT_EQ(OH_AVErrCode::AV_ERR_INVALID_VAL, OH_AudioCodec_PushInputBuffer(audioDec_, index));
    Release();
}

HWTEST_F(AudioCodeTwinvqDecoderUnitTest, audioDecoder_Twinvq_ReleaseOutputBuffer_InvalidIndex_01, TestSize.Level1)
{
    isTestingFormat_ = true;
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(TWINVQ_FILE_TODEMUX, OUTPUT_TWINVQ_PCM_FILE_PATH));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    const uint32_t index = 100000;
    EXPECT_EQ(OH_AVErrCode::AV_ERR_INVALID_VAL, OH_AudioCodec_FreeOutputBuffer(audioDec_, index));
    Release();
}

HWTEST_F(AudioCodeTwinvqDecoderUnitTest, audioDecoder_Twinvq_Configure_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(TWINVQ_FILE_TODEMUX, OUTPUT_TWINVQ_PCM_FILE_PATH));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure());
    Release();
}

HWTEST_F(AudioCodeTwinvqDecoderUnitTest, audioDecoder_Twinvq_SetParameter_01, TestSize.Level1)
{
    isTestingFormat_ = true;
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(TWINVQ_FILE_TODEMUX, OUTPUT_TWINVQ_PCM_FILE_PATH));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc());
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

HWTEST_F(AudioCodeTwinvqDecoderUnitTest, audioDecoder_Twinvq_SetParameter_02, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(TWINVQ_FILE_TODEMUX, OUTPUT_TWINVQ_PCM_FILE_PATH));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure());
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_SetParameter(audioDec_, format_));
    Release();
}

HWTEST_F(AudioCodeTwinvqDecoderUnitTest, audioDecoder_Twinvq_Start_01, TestSize.Level1)
{
    isTestingFormat_ = true;
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(TWINVQ_FILE_TODEMUX, OUTPUT_TWINVQ_PCM_FILE_PATH));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    Release();
}

HWTEST_F(AudioCodeTwinvqDecoderUnitTest, audioDecoder_Twinvq_Start_02, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(TWINVQ_FILE_TODEMUX, OUTPUT_TWINVQ_PCM_FILE_PATH));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc());
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

HWTEST_F(AudioCodeTwinvqDecoderUnitTest, audioDecoder_Twinvq_Stop_01, TestSize.Level1)
{
    isTestingFormat_ = true;
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(TWINVQ_FILE_TODEMUX, OUTPUT_TWINVQ_PCM_FILE_PATH));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    sleep(1);

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Stop());
    Release();
}

HWTEST_F(AudioCodeTwinvqDecoderUnitTest, audioDecoder_Twinvq_Flush_01, TestSize.Level1)
{
    isTestingFormat_ = true;
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(TWINVQ_FILE_TODEMUX, OUTPUT_TWINVQ_PCM_FILE_PATH));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Flush(audioDec_));
    Release();
}

HWTEST_F(AudioCodeTwinvqDecoderUnitTest, audioDecoder_Twinvq_Reset_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(TWINVQ_FILE_TODEMUX, OUTPUT_TWINVQ_PCM_FILE_PATH));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioDec_));
    Release();
}

HWTEST_F(AudioCodeTwinvqDecoderUnitTest, audioDecoder_Twinvq_Reset_02, TestSize.Level1)
{
    isTestingFormat_ = true;
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(TWINVQ_FILE_TODEMUX, OUTPUT_TWINVQ_PCM_FILE_PATH));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc());
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

HWTEST_F(AudioCodeTwinvqDecoderUnitTest, audioDecoder_Twinvq_Reset_03, TestSize.Level1)
{
    isTestingFormat_ = true;
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(TWINVQ_FILE_TODEMUX, OUTPUT_TWINVQ_PCM_FILE_PATH));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioDec_));
    Release();
}

HWTEST_F(AudioCodeTwinvqDecoderUnitTest, audioDecoder_Twinvq_Destroy_01, TestSize.Level1)
{
    isTestingFormat_ = true;
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(TWINVQ_FILE_TODEMUX, OUTPUT_TWINVQ_PCM_FILE_PATH));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Stop());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Destroy(audioDec_));
}

HWTEST_F(AudioCodeTwinvqDecoderUnitTest, audioDecoder_Twinvq_Destroy_02, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(TWINVQ_FILE_TODEMUX, OUTPUT_TWINVQ_PCM_FILE_PATH));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure());

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Destroy(audioDec_));
}

HWTEST_F(AudioCodeTwinvqDecoderUnitTest, audioDecoder_Twinvq_GetOutputFormat_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(TWINVQ_FILE_TODEMUX, OUTPUT_TWINVQ_PCM_FILE_PATH));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure());

    EXPECT_NE(nullptr, OH_AudioCodec_GetOutputDescription(audioDec_));
    Release();
}

HWTEST_F(AudioCodeTwinvqDecoderUnitTest, audioDecoder_Twinvq_IsValid_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc());
    bool isValid = false;
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_IsValid(audioDec_, &isValid));
    Release();
}

HWTEST_F(AudioCodeTwinvqDecoderUnitTest, audioDecoder_Twinvq_Prepare_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(TWINVQ_FILE_TODEMUX, OUTPUT_TWINVQ_PCM_FILE_PATH));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Prepare(audioDec_));
    Release();
}
}
}