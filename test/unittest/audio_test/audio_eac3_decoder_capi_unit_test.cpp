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

#include <queue>
#include <mutex>
#include <libavutil/samplefmt.h>
#include <gtest/gtest.h>
#include <iostream>
#include <unistd.h>
#include <atomic>
#include <fstream>
#include <string>
#include <thread>
#include <fcntl.h>
#include "avcodec_codec_name.h"
#include "avcodec_common.h"
#include "avcodec_errors.h"
#include "avcodec_mime_type.h"
#include "media_description.h"
#include "native_avcodec_base.h"
#include "securec.h"
#include "avcodec_audio_common.h"
#include "native_avbuffer.h"
#include "common/native_mfmagic.h"
#include "native_avcodec_audiocodec.h"
#include "native_audio_channel_layout.h"
#include "media_key_system_mock.h"
#include "native_avcapability.h"
#include "native_avdemuxer.h"
#include "native_avsource.h"
#include "common/status.h"
#include "native_avcodec_audiodecoder.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS::Media;
using namespace OHOS::MediaAVCodec;

namespace {
const string CODEC_EAC3_NAME = std::string(std::string(AVCodecCodecName::AUDIO_DECODER_EAC3_NAME));
constexpr int32_t MAX_CHANNELS = 16;
constexpr uint32_t DEFAULT_SAMPLE_RATE = 48000;
constexpr string_view EAC3_FILE_TODEMUX = "/data/test/media/48000_mono_eac3.avi";
constexpr string_view OUTPUT_EAC3_PCM_FILE_PATH = "/data/test/media/test_decoder_eac3.pcm";
constexpr string_view INPUT_EAC3_WAV_1_PATH = "/data/test/media/48000_mono_eac3.wav";
constexpr string_view INPUT_EAC3_WAV_2_PATH = "/data/test/media/48000_stereo_eac3.wav";
constexpr string_view INPUT_EAC3_WAV_3_PATH = "/data/test/media/48000_30back_eac3.wav";
constexpr string_view INPUT_EAC3_WAV_4_PATH = "/data/test/media/44100_quad_eac3.wav";
constexpr string_view INPUT_EAC3_WAV_5_PATH = "/data/test/media/44100_quad_side_eac3.wav";
constexpr string_view INPUT_EAC3_WAV_6_PATH = "/data/test/media/44100_40_eac3.wav";
constexpr string_view INPUT_EAC3_WAV_7_PATH = "/data/test/media/44100_50_eac3.wav";
constexpr string_view INPUT_EAC3_WAV_8_PATH = "/data/test/media/44100_2_channels_eac3.wav";
constexpr string_view INPUT_EAC3_WAV_9_PATH = "/data/test/media/44100_4_channels_eac3.wav";
constexpr string_view INPUT_EAC3_WAV_10_PATH = "/data/test/media/32000_21_eac3.wav";
constexpr string_view INPUT_EAC3_WAV_11_PATH = "/data/test/media/32000_31_eac3.wav";
constexpr string_view INPUT_EAC3_WAV_12_PATH = "/data/test/media/32000_51_eac3.wav";
constexpr string_view INPUT_EAC3_WAV_13_PATH = "/data/test/media/32000_41_eac3.wav";
constexpr string_view INPUT_EAC3_WAV_14_PATH = "/data/test/media/48000_30_eac3.wav";
constexpr string_view INPUT_EAC3_WAV_15_PATH = "/data/test/media/44100_50side_eac3.wav";

constexpr string_view OUTPUT_EAC3_PCM_1_PATH = "/data/test/media/test_48000_mono_eac3.pcm";
constexpr string_view OUTPUT_EAC3_PCM_2_PATH = "/data/test/media/test_48000_stereo_eac3.pcm";
constexpr string_view OUTPUT_EAC3_PCM_3_PATH = "/data/test/media/test_48000_30back_eac3.pcm";
constexpr string_view OUTPUT_EAC3_PCM_4_PATH = "/data/test/media/test_44100_quad_eac3.pcm";
constexpr string_view OUTPUT_EAC3_PCM_5_PATH = "/data/test/media/test_44100_quad_side_eac3.pcm";
constexpr string_view OUTPUT_EAC3_PCM_6_PATH = "/data/test/media/test_44100_40_eac3.pcm";
constexpr string_view OUTPUT_EAC3_PCM_7_PATH = "/data/test/media/test_44100_50_eac3.pcm";
constexpr string_view OUTPUT_EAC3_PCM_8_PATH = "/data/test/media/test_44100_2_channels_eac3.pcm";
constexpr string_view OUTPUT_EAC3_PCM_9_PATH = "/data/test/media/test_44100_4_channels_eac3.pcm";
constexpr string_view OUTPUT_EAC3_PCM_10_PATH = "/data/test/media/test_32000_21_eac3.pcm";
constexpr string_view OUTPUT_EAC3_PCM_11_PATH = "/data/test/media/test_32000_31_eac3.pcm";
constexpr string_view OUTPUT_EAC3_PCM_12_PATH = "/data/test/media/test_32000_51_eac3.pcm";
constexpr string_view OUTPUT_EAC3_PCM_13_PATH = "/data/test/media/test_32000_41_eac3.pcm";
constexpr string_view OUTPUT_EAC3_PCM_14_PATH = "/data/test/media/test_48000_30_eac3.pcm";
constexpr string_view OUTPUT_EAC3_PCM_15_PATH = "/data/test/media/test_44100_50side_eac3.pcm";
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
static int64_t g_outputChannelLayout = 0;
static int32_t g_outputSampleFormat = 0;
constexpr int SAMPLERATE = 48000;
constexpr int SAMPLES_PER_FRAME = 1536;

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
    OH_AVFormat_GetLongValue(format, OH_MD_KEY_CHANNEL_LAYOUT, &g_outputChannelLayout);
    OH_AVFormat_GetIntValue(format, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, &g_outputSampleFormat);
    cout << "OnOutputFormatChanged received, rate:" << g_outputSampleRate << ",channel:" << g_outputChannels << endl;
    cout << "OnOutputFormatChanged received, layout:" << g_outputChannelLayout
        << ",format:" << g_outputSampleFormat << endl;
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

class AudioCodeEac3DecoderUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
    int32_t InitFile(string_view input);
    int32_t InitFile(string_view input, string_view output);
    void InputFunc();
    void OutputFunc();
    bool ReadBuffer(OH_AVBuffer *buffer, uint32_t index);
    int32_t CreateCodecFunc();
    void HandleEOS(const uint32_t &index);
    int32_t Configure();
    int32_t Start();
    int32_t Stop();
    void Release();
    int64_t GetFileSize(const char *fileName);
    void SetEOS(uint32_t index, OH_AVBuffer *buffer);

protected:
    int fd_ = -1;
    bool isTestingFormat_ = false;
    std::atomic<bool> isRunning_ = false;
    std::unique_ptr<std::thread> inputLoop_;
    std::unique_ptr<std::thread> outputLoop_;
    struct OH_AVCodecCallback cb_;
    AudioCodecBufferSignal *signal_ = nullptr;
    OH_AVCodec *audioDec_ = nullptr;
    OH_AVFormat *format_ = nullptr;
    bool isFirstFrame_ = true;
    uint32_t frameCount_ = 0;
    std::ifstream inputFile_;
    std::unique_ptr<std::ifstream> soFile_;
    std::ofstream pcmOutputFile_;
    OH_AVDemuxer *demuxer = nullptr;
    OH_AVSource *source = nullptr;
};

void AudioCodeEac3DecoderUnitTest::SetUpTestCase(void)
{
    cout << "[SetUpTestCase]: " << endl;
}

void AudioCodeEac3DecoderUnitTest::TearDownTestCase(void)
{
    cout << "[TearDownTestCase]: " << endl;
}

void AudioCodeEac3DecoderUnitTest::SetUp(void)
{
    g_outputFormatChangedTimes = 0;
    g_outputSampleRate = 0;
    g_outputChannels = 0;
    cout << "[SetUp]: SetUp!!!" << endl;
}

void AudioCodeEac3DecoderUnitTest::TearDown(void)
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
    if (inputFile_.is_open()) {
        inputFile_.close();
    }
    if (pcmOutputFile_.is_open()) {
        pcmOutputFile_.close();
    }

    if (format_ != nullptr) {
        OH_AVFormat_Destroy(format_);
        format_ = nullptr;
    }
}

void AudioCodeEac3DecoderUnitTest::Release()
{
    Stop();
    OH_AudioCodec_Destroy(audioDec_);
}

void AudioCodeEac3DecoderUnitTest::HandleEOS(const uint32_t &index)
{
    OH_AudioCodec_PushInputBuffer(audioDec_, index);
    std::cout << "end buffer\n";
    signal_->inQueue_.pop();
    signal_->inBufferQueue_.pop();
}

void AudioCodeEac3DecoderUnitTest::SetEOS(uint32_t index, OH_AVBuffer *buffer)
{
    OH_AVCodecBufferAttr attr;
    attr.pts = 0;
    attr.size = 0;
    attr.offset = 0;
    attr.flags = AVCODEC_BUFFER_FLAGS_EOS;
    OH_AVBuffer_SetBufferAttr(buffer, &attr);
    int32_t res = OH_AudioCodec_PushInputBuffer(audioDec_, index);
    cout << "OH_AudioCodec_PushInputBuffer    EOS   res: " << res << endl;
}

bool AudioCodeEac3DecoderUnitTest::ReadBuffer(OH_AVBuffer *buffer, uint32_t index)
{
    OH_AVCodecBufferAttr attr;
    OH_AVDemuxer_ReadSampleBuffer(demuxer, 0, buffer);
    OH_AVBuffer_GetBufferAttr(buffer, &attr);
    if (attr.flags & AVCODEC_BUFFER_FLAGS_EOS) {
        SetEOS(index, buffer);
        return false;
    }
    buffer->buffer_->pts_ = frameCount_ * SAMPLES_PER_FRAME * 1000000LL / SAMPLERATE;
    buffer->buffer_->flag_ = AVCODEC_BUFFER_FLAGS_NONE;
    return true;
}

int64_t AudioCodeEac3DecoderUnitTest::GetFileSize(const char *fileName)
{
    int64_t fileSize = 0;
    if (fileName != nullptr) {
        struct stat fileStatus {};
        if (stat(fileName, &fileStatus) == 0) {
            fileSize = static_cast<int64_t>(fileStatus.st_size);
        }
    }
    return fileSize;
}

void AudioCodeEac3DecoderUnitTest::InputFunc()
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
            buffer->buffer_->memory_->SetSize(1);
            buffer->buffer_->flag_ = AVCODEC_BUFFER_FLAGS_EOS;
            HandleEOS(index);
            break;
        } else {
            OH_AVCodecBufferAttr attr;
            OH_AVBuffer_GetBufferAttr(buffer, &attr);
            if (attr.size == 0) {
                continue;
            }
            buffer->buffer_->memory_->SetSize(attr.size);
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
    inputFile_.close();
}

void AudioCodeEac3DecoderUnitTest::OutputFunc()
{
    if (!pcmOutputFile_.is_open()) {
        std::cout << "open " << OUTPUT_EAC3_PCM_FILE_PATH << " failed!" << std::endl;
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
    }
    cout << "stop, exit" << endl;
    pcmOutputFile_.close();
}

int32_t AudioCodeEac3DecoderUnitTest::Start()
{
    isRunning_.store(true);
    inputLoop_ = make_unique<thread>(&AudioCodeEac3DecoderUnitTest::InputFunc, this);
    if (inputLoop_ == nullptr) {
        cout << "Fatal: No memory" << endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }
    outputLoop_ = make_unique<thread>(&AudioCodeEac3DecoderUnitTest::OutputFunc, this);
    if (outputLoop_ == nullptr) {
        cout << "Fatal: No memory" << endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }
    return OH_AudioCodec_Start(audioDec_);
}

int32_t AudioCodeEac3DecoderUnitTest::Stop()
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

int32_t AudioCodeEac3DecoderUnitTest::InitFile(string_view input)
{
    inputFile_.open(input.data(), std::ios::binary);
    pcmOutputFile_.open(OUTPUT_EAC3_PCM_FILE_PATH.data(), std::ios::out | std::ios::binary);
    if (!inputFile_.is_open()) {
        cout << "Fatal: open input file failed" << endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }
    if (!pcmOutputFile_.is_open()) {
        cout << "Fatal: open output file failed" << endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }
    int fd = open(input.data(), O_RDONLY);
    int64_t size = GetFileSize(input.data());
    cout << input.data() << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    if (source == nullptr) {
        cout << "Fatal: source is null" << endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    if (demuxer == nullptr) {
        cout << "Fatal: demuxer is null" << endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }
    OH_AVErrCode ret = OH_AVDemuxer_SelectTrackByID(demuxer, 0);
    if (ret != OH_AVErrCode::AV_ERR_OK) {
        cout << "Fatal: OH_AVDemuxer_SelectTrackByID is fail" << endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }
    return OH_AVErrCode::AV_ERR_OK;
}

int32_t AudioCodeEac3DecoderUnitTest::InitFile(string_view input, string_view output)
{
    inputFile_.open(input.data(), std::ios::binary);
    if (!inputFile_.is_open()) {
        cout << "Fatal: open input file failed:" << input << endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }
    pcmOutputFile_.open(output.data(), std::ios::out | std::ios::binary);
    if (!pcmOutputFile_.is_open()) {
        cout << "Fatal: open output file failed" << output << endl;
        inputFile_.close();
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }
    int fd = open(input.data(), O_RDONLY);
    int64_t size = GetFileSize(input.data());
    cout << input.data() << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    if (source == nullptr) {
        cout << "Fatal: source is null" << endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    if (demuxer == nullptr) {
        cout << "Fatal: demuxer is null" << endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }
    OH_AVErrCode ret = OH_AVDemuxer_SelectTrackByID(demuxer, 0);
    if (ret != OH_AVErrCode::AV_ERR_OK) {
        cout << "Fatal: OH_AVDemuxer_SelectTrackByID is fail" << endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }
    return OH_AVErrCode::AV_ERR_OK;
}
int32_t AudioCodeEac3DecoderUnitTest::CreateCodecFunc()
{
    audioDec_ = OH_AudioCodec_CreateByName((std::string(AVCodecCodecName::AUDIO_DECODER_EAC3_NAME)).data());
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

int32_t AudioCodeEac3DecoderUnitTest::Configure()
{
    format_ = OH_AVFormat_Create();
    if (format_ == nullptr) {
        cout << "Fatal: create format failed" << endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }
    uint32_t bitRate = 384000;
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), MAX_CHANNELS);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), DEFAULT_SAMPLE_RATE);
    OH_AVFormat_SetLongValue(format_, MediaDescriptionKey::MD_KEY_BITRATE.data(), bitRate);
    return OH_AudioCodec_Configure(audioDec_, format_);
}

HWTEST_F(AudioCodeEac3DecoderUnitTest, AudioDemuxAndDecode_Eac3_001, TestSize.Level1)
{
    isTestingFormat_ = true;
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(EAC3_FILE_TODEMUX, OUTPUT_EAC3_PCM_FILE_PATH));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    Release();
}

HWTEST_F(AudioCodeEac3DecoderUnitTest, CheckSampleFormat_Eac3_001, TestSize.Level1)
{
    isTestingFormat_ = true;
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(INPUT_EAC3_WAV_1_PATH, OUTPUT_EAC3_PCM_1_PATH));
    ASSERT_EQ(AV_ERR_OK, CreateCodecFunc());
    OH_AVFormat *fmt = OH_AVFormat_Create();
    EXPECT_EQ(AV_ERR_OK, OH_AudioCodec_Reset(audioDec_));
    OH_AVFormat_SetIntValue(fmt, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), 1);
    OH_AVFormat_SetLongValue(fmt, MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT.data(),
        OH_AudioChannelLayout::CH_LAYOUT_MONO);
    OH_AVFormat_SetIntValue(fmt, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), 48000);
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

HWTEST_F(AudioCodeEac3DecoderUnitTest, CheckSampleFormat_Eac3_002, TestSize.Level1)
{
    isTestingFormat_ = true;
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(INPUT_EAC3_WAV_2_PATH, OUTPUT_EAC3_PCM_2_PATH));
    ASSERT_EQ(AV_ERR_OK, CreateCodecFunc());
    OH_AVFormat *fmt = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(fmt, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), 2);
    OH_AVFormat_SetLongValue(fmt, MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT.data(),
        OH_AudioChannelLayout::CH_LAYOUT_STEREO);
    OH_AVFormat_SetIntValue(fmt, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), 48000);
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

HWTEST_F(AudioCodeEac3DecoderUnitTest, CheckSampleFormat_Eac3_003, TestSize.Level1)
{
    isTestingFormat_ = true;
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(INPUT_EAC3_WAV_3_PATH, OUTPUT_EAC3_PCM_3_PATH));
    ASSERT_EQ(AV_ERR_OK, CreateCodecFunc());
    OH_AVFormat *fmt = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(fmt, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), 3);
    OH_AVFormat_SetLongValue(fmt, MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT.data(),
        OH_AudioChannelLayout::CH_LAYOUT_3POINT0);
    OH_AVFormat_SetIntValue(fmt, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), 48000);
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

HWTEST_F(AudioCodeEac3DecoderUnitTest, CheckSampleFormat_Eac3_004, TestSize.Level1)
{
    isTestingFormat_ = true;
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(INPUT_EAC3_WAV_4_PATH, OUTPUT_EAC3_PCM_4_PATH));
    ASSERT_EQ(AV_ERR_OK, CreateCodecFunc());
    OH_AVFormat *fmt = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(fmt, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), 4);
    OH_AVFormat_SetLongValue(fmt, MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT.data(),
        OH_AudioChannelLayout::CH_LAYOUT_QUAD);
    OH_AVFormat_SetIntValue(fmt, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), 44100);
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

HWTEST_F(AudioCodeEac3DecoderUnitTest, CheckSampleFormat_Eac3_005, TestSize.Level1)
{
    isTestingFormat_ = true;
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(INPUT_EAC3_WAV_5_PATH, OUTPUT_EAC3_PCM_5_PATH));
    ASSERT_EQ(AV_ERR_OK, CreateCodecFunc());
    OH_AVFormat *fmt = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(fmt, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), 4);
    OH_AVFormat_SetLongValue(fmt, MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT.data(),
        OH_AudioChannelLayout::CH_LAYOUT_QUAD_SIDE);
    OH_AVFormat_SetIntValue(fmt, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), 44100);
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

HWTEST_F(AudioCodeEac3DecoderUnitTest, CheckSampleFormat_Eac3_006, TestSize.Level1)
{
    isTestingFormat_ = true;
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(INPUT_EAC3_WAV_6_PATH, OUTPUT_EAC3_PCM_6_PATH));
    ASSERT_EQ(AV_ERR_OK, CreateCodecFunc());
    OH_AVFormat *fmt = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(fmt, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), 4);
    OH_AVFormat_SetLongValue(fmt, MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT.data(),
        OH_AudioChannelLayout::CH_LAYOUT_4POINT0);
    OH_AVFormat_SetIntValue(fmt, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), 44100);
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

HWTEST_F(AudioCodeEac3DecoderUnitTest, CheckSampleFormat_Eac3_007, TestSize.Level1)
{
    isTestingFormat_ = true;
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(INPUT_EAC3_WAV_7_PATH, OUTPUT_EAC3_PCM_7_PATH));
    ASSERT_EQ(AV_ERR_OK, CreateCodecFunc());
    OH_AVFormat *fmt = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(fmt, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), 5);
    OH_AVFormat_SetLongValue(fmt, MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT.data(),
        OH_AudioChannelLayout::CH_LAYOUT_5POINT0_BACK);
    OH_AVFormat_SetIntValue(fmt, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), 44100);
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

HWTEST_F(AudioCodeEac3DecoderUnitTest, capability_001, TestSize.Level1)
{
    OH_AVCapability *cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_AUDIO_EAC3, false, SOFTWARE);
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

HWTEST_F(AudioCodeEac3DecoderUnitTest, CheckSampleFormat_Eac3_008, TestSize.Level1)
{
    isTestingFormat_ = true;
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(INPUT_EAC3_WAV_8_PATH, OUTPUT_EAC3_PCM_8_PATH));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc());
    OH_AVFormat *fmt = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(fmt, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), 2);
    OH_AVFormat_SetLongValue(fmt, MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT.data(),
        OH_AudioChannelLayout::CH_LAYOUT_STEREO);
    OH_AVFormat_SetIntValue(fmt, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), 48000);
    OH_AVFormat_SetIntValue(fmt, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(),
        AVSampleFormat::AV_SAMPLE_FMT_FLTP);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, fmt));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    OH_AVFormat_Destroy(fmt);
    Release();
}

HWTEST_F(AudioCodeEac3DecoderUnitTest, CheckSampleFormat_Eac3_009, TestSize.Level1)
{
    isTestingFormat_ = true;
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(INPUT_EAC3_WAV_9_PATH, OUTPUT_EAC3_PCM_9_PATH));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc());
    OH_AVFormat *fmt = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(fmt, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), 4);
    OH_AVFormat_SetLongValue(fmt, MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT.data(),
        OH_AudioChannelLayout::CH_LAYOUT_4POINT0);
    OH_AVFormat_SetIntValue(fmt, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), 44100);
    OH_AVFormat_SetIntValue(fmt, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(),
        AVSampleFormat::AV_SAMPLE_FMT_FLTP);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, fmt));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    OH_AVFormat_Destroy(fmt);
    Release();
}

HWTEST_F(AudioCodeEac3DecoderUnitTest, CheckSampleFormat_Eac3_010, TestSize.Level1)
{
    isTestingFormat_ = true;
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(INPUT_EAC3_WAV_10_PATH, OUTPUT_EAC3_PCM_10_PATH));
    ASSERT_EQ(AV_ERR_OK, CreateCodecFunc());
    OH_AVFormat *fmt = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(fmt, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), 3);
    OH_AVFormat_SetLongValue(fmt, MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT.data(),
        OH_AudioChannelLayout::CH_LAYOUT_2POINT1);
    OH_AVFormat_SetIntValue(fmt, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), 32000);
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

HWTEST_F(AudioCodeEac3DecoderUnitTest, CheckSampleFormat_Eac3_011, TestSize.Level1)
{
    isTestingFormat_ = true;
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(INPUT_EAC3_WAV_11_PATH, OUTPUT_EAC3_PCM_11_PATH));
    ASSERT_EQ(AV_ERR_OK, CreateCodecFunc());
    OH_AVFormat *fmt = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(fmt, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), 4);
    OH_AVFormat_SetLongValue(fmt, MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT.data(),
        OH_AudioChannelLayout::CH_LAYOUT_3POINT1);
    OH_AVFormat_SetIntValue(fmt, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), 32000);
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

HWTEST_F(AudioCodeEac3DecoderUnitTest, CheckSampleFormat_Eac3_012, TestSize.Level1)
{
    isTestingFormat_ = true;
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(INPUT_EAC3_WAV_12_PATH, OUTPUT_EAC3_PCM_12_PATH));
    ASSERT_EQ(AV_ERR_OK, CreateCodecFunc());
    OH_AVFormat *fmt = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(fmt, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), 6);
    OH_AVFormat_SetLongValue(fmt, MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT.data(),
        OH_AudioChannelLayout::CH_LAYOUT_5POINT1);
    OH_AVFormat_SetIntValue(fmt, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), 32000);
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

HWTEST_F(AudioCodeEac3DecoderUnitTest, CheckSampleFormat_Eac3_013, TestSize.Level1)
{
    isTestingFormat_ = true;
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(INPUT_EAC3_WAV_13_PATH, OUTPUT_EAC3_PCM_13_PATH));
    ASSERT_EQ(AV_ERR_OK, CreateCodecFunc());
    OH_AVFormat *fmt = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(fmt, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), 5);
    OH_AVFormat_SetLongValue(fmt, MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT.data(),
        OH_AudioChannelLayout::CH_LAYOUT_4POINT1);
    OH_AVFormat_SetIntValue(fmt, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), 32000);
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

HWTEST_F(AudioCodeEac3DecoderUnitTest, CheckSampleFormat_Eac3_014, TestSize.Level1)
{
    isTestingFormat_ = true;
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(INPUT_EAC3_WAV_14_PATH, OUTPUT_EAC3_PCM_14_PATH));
    ASSERT_EQ(AV_ERR_OK, CreateCodecFunc());
    OH_AVFormat *fmt = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(fmt, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), 3);
    OH_AVFormat_SetLongValue(fmt, MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT.data(),
        OH_AudioChannelLayout::CH_LAYOUT_SURROUND);
    OH_AVFormat_SetIntValue(fmt, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), 48000);
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

HWTEST_F(AudioCodeEac3DecoderUnitTest, CheckSampleFormat_Eac3_015, TestSize.Level1)
{
    isTestingFormat_ = true;
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(INPUT_EAC3_WAV_15_PATH, OUTPUT_EAC3_PCM_15_PATH));
    ASSERT_EQ(AV_ERR_OK, CreateCodecFunc());
    OH_AVFormat *fmt = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(fmt, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), 8);
    OH_AVFormat_SetLongValue(fmt, MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT.data(),
        OH_AudioChannelLayout::CH_LAYOUT_5POINT0);
    OH_AVFormat_SetIntValue(fmt, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), 44100);
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

HWTEST_F(AudioCodeEac3DecoderUnitTest, CheckSampleInvalidFormat_Eac3_001, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(INPUT_EAC3_WAV_15_PATH, OUTPUT_EAC3_PCM_15_PATH));
    ASSERT_EQ(AV_ERR_OK, CreateCodecFunc());
    OH_AVFormat *fmt = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(fmt, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), 5);
    OH_AVFormat_SetLongValue(fmt, MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT.data(),
        OH_AudioChannelLayout::CH_LAYOUT_5POINT0);
    OH_AVFormat_SetIntValue(fmt, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), 8000);
    OH_AVFormat_SetIntValue(fmt, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(),
        AVSampleFormat::AV_SAMPLE_FMT_FLTP);
    EXPECT_EQ(AV_ERR_INVALID_VAL, OH_AudioCodec_Configure(audioDec_, fmt));
    Release();
}

HWTEST_F(AudioCodeEac3DecoderUnitTest, CheckSampleInvalidFormat_Eac3_002, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(INPUT_EAC3_WAV_15_PATH, OUTPUT_EAC3_PCM_15_PATH));
    ASSERT_EQ(AV_ERR_OK, CreateCodecFunc());
    OH_AVFormat *fmt = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(fmt, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), 20);
    OH_AVFormat_SetLongValue(fmt, MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT.data(),
        OH_AudioChannelLayout::CH_LAYOUT_5POINT0);
    OH_AVFormat_SetIntValue(fmt, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), 44100);
    OH_AVFormat_SetIntValue(fmt, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(),
        AVSampleFormat::AV_SAMPLE_FMT_FLTP);
    EXPECT_EQ(AV_ERR_INVALID_VAL, OH_AudioCodec_Configure(audioDec_, fmt));
    Release();
}

HWTEST_F(AudioCodeEac3DecoderUnitTest, audioDecoder_Eac3_CreateByName_01, TestSize.Level1)
{
    audioDec_ = OH_AudioCodec_CreateByName((std::string(AVCodecCodecName::AUDIO_DECODER_EAC3_NAME)).data());
    EXPECT_NE(nullptr, audioDec_);
    Release();
}

HWTEST_F(AudioCodeEac3DecoderUnitTest, audioDecoder_Eac3_CreateByName_02, TestSize.Level1)
{
    audioDec_ = OH_AudioCodec_CreateByName(nullptr);
    EXPECT_EQ(nullptr, audioDec_);
}

HWTEST_F(AudioCodeEac3DecoderUnitTest, audioDecoder_Eac3_CreateByName_03, TestSize.Level1)
{
    audioDec_ = OH_AudioCodec_CreateByName("audio_decoder.eac3");
    EXPECT_EQ(nullptr, audioDec_);
}

HWTEST_F(AudioCodeEac3DecoderUnitTest, audioDecoder_Eac3_CreateByMime_01, TestSize.Level1)
{
    audioDec_ = OH_AudioCodec_CreateByMime(std::string(AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_EAC3).data(), false);
    EXPECT_NE(nullptr, audioDec_);
    Release();
}

HWTEST_F(AudioCodeEac3DecoderUnitTest, audioDecoder_Eac3_CreateByMime_02, TestSize.Level1)
{
    audioDec_ = OH_AudioCodec_CreateByMime(nullptr, false);
    EXPECT_EQ(nullptr, audioDec_);
    Release();
}

HWTEST_F(AudioCodeEac3DecoderUnitTest, audioDecoder_Eac3_PushInputData_InvalidIndex_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(EAC3_FILE_TODEMUX));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    const uint32_t index = 1024;
    EXPECT_EQ(OH_AVErrCode::AV_ERR_INVALID_VAL, OH_AudioCodec_PushInputBuffer(audioDec_, index));
    Release();
}

HWTEST_F(AudioCodeEac3DecoderUnitTest, audioDecoder_Eac3_ReleaseOutputBuffer_InvalidIndex_01, TestSize.Level1)
{
    isTestingFormat_ = true;
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(EAC3_FILE_TODEMUX));
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

HWTEST_F(AudioCodeEac3DecoderUnitTest, audioDecoder_Eac3_Configure_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure());
    Release();
}

HWTEST_F(AudioCodeEac3DecoderUnitTest, audioDecoder_Eac3_SetParameter_01, TestSize.Level1)
{
    isTestingFormat_ = true;
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(EAC3_FILE_TODEMUX));
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

HWTEST_F(AudioCodeEac3DecoderUnitTest, audioDecoder_Eac3_SetParameter_02, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure());
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_SetParameter(audioDec_, format_));
    Release();
}

HWTEST_F(AudioCodeEac3DecoderUnitTest, audioDecoder_Eac3_Start_01, TestSize.Level1)
{
    isTestingFormat_ = true;
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(EAC3_FILE_TODEMUX));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    Release();
}

HWTEST_F(AudioCodeEac3DecoderUnitTest, audioDecoder_Eac3_Start_02, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(EAC3_FILE_TODEMUX));
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

HWTEST_F(AudioCodeEac3DecoderUnitTest, audioDecoder_Eac3_Stop_01, TestSize.Level1)
{
    isTestingFormat_ = true;
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(EAC3_FILE_TODEMUX));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    sleep(1);

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Stop());
    Release();
}

HWTEST_F(AudioCodeEac3DecoderUnitTest, audioDecoder_Eac3_Flush_01, TestSize.Level1)
{
    isTestingFormat_ = true;
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(EAC3_FILE_TODEMUX));
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

HWTEST_F(AudioCodeEac3DecoderUnitTest, audioDecoder_Eac3_Reset_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioDec_));
    Release();
}

HWTEST_F(AudioCodeEac3DecoderUnitTest, audioDecoder_Eac3_Reset_02, TestSize.Level1)
{
    isTestingFormat_ = true;
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(EAC3_FILE_TODEMUX));
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

HWTEST_F(AudioCodeEac3DecoderUnitTest, audioDecoder_Eac3_Reset_03, TestSize.Level1)
{
    isTestingFormat_ = true;
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(EAC3_FILE_TODEMUX));
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

HWTEST_F(AudioCodeEac3DecoderUnitTest, audioDecoder_Eac3_Destroy_01, TestSize.Level1)
{
    isTestingFormat_ = true;
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(EAC3_FILE_TODEMUX));
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

HWTEST_F(AudioCodeEac3DecoderUnitTest, audioDecoder_Eac3_Destroy_02, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(EAC3_FILE_TODEMUX));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure());

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Destroy(audioDec_));
}

HWTEST_F(AudioCodeEac3DecoderUnitTest, audioDecoder_Eac3_GetOutputFormat_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(EAC3_FILE_TODEMUX));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure());

    EXPECT_NE(nullptr, OH_AudioCodec_GetOutputDescription(audioDec_));
    Release();
}

HWTEST_F(AudioCodeEac3DecoderUnitTest, audioDecoder_Eac3_IsValid_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc());
    bool isValid = false;
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_IsValid(audioDec_, &isValid));
    Release();
}

HWTEST_F(AudioCodeEac3DecoderUnitTest, audioDecoder_Eac3_Prepare_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Prepare(audioDec_));
    Release();
}
} // namespace MediaAVCodec
} // namespace OHOS