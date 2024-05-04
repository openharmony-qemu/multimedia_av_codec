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

#include <queue>
#include <mutex>
#include <gtest/gtest.h>
#include <iostream>
#include <unistd.h>
#include <atomic>
#include <fstream>
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
constexpr uint32_t DEFAULT_CHANNEL_COUNT = 2;
constexpr uint32_t DEFAULT_SAMPLE_RATE = 44100;
constexpr uint32_t ABNORMAL_SAMPLE_RATE = 999999;
constexpr uint32_t FLAC_192K_SAMPLE_RATE = 192000;
constexpr uint32_t DEFAULT_AAC_TYPE = 1;
constexpr uint32_t ABNORMAL_AAC_TYPE = 999999;
constexpr int32_t DEFAULT_SAMPLE_FORMAT = AudioSampleFormat::SAMPLE_S16LE;
constexpr int32_t ABNORMAL_SAMPLE_FORMAT = AudioSampleFormat::INVALID_WIDTH;
constexpr int32_t FLAC_S24_SAMPLE_FORMAT = AudioSampleFormat::SAMPLE_S24LE;
constexpr int64_t BITS_RATE[7] = {199000, 261000, 60000, 320000};
constexpr uint32_t AMRWB_SAMPLE_RATE = 16000;
constexpr uint32_t AMRNB_SAMPLE_RATE = 8000;
constexpr uint32_t ONE_CHANNEL_COUNT = 1;
constexpr uint32_t ABNORMAL_MAX_CHANNEL_COUNT = 999999;
constexpr uint32_t ABNORMAL_MIN_CHANNEL_COUNT = 0;
constexpr uint32_t ABNORMAL_MAX_INPUT_SIZE = 99999999;
constexpr uint64_t CHANNEL_LAYOUT = OH_AudioChannelLayout::CH_LAYOUT_STEREO;
constexpr uint64_t ABNORMAL_CHANNEL_LAYOUT = OH_AudioChannelLayout::CH_LAYOUT_5POINT1POINT2;
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
constexpr string_view OPUS_SO_FILE_PATH = "/system/lib64/libav_codec_ext_base.z.so";
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
    TYPE_APE = 8,
    TYPE_MAX = 9,
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

class AudioDecoderBufferCapiUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
    bool ReadBuffer(OH_AVBuffer *buffer, uint32_t index);
    int32_t InitFile(AudioBufferFormatType audioType);
    void InputFunc();
    void OutputFunc();
    int32_t CreateCodecFunc(AudioBufferFormatType audioType);
    int32_t CheckSoFunc();
    void HandleInputEOS(const uint32_t index);
    int32_t Configure(AudioBufferFormatType audioType);
    int32_t Start();
    int32_t Stop();
    void Release();

protected:
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
};

void AudioDecoderBufferCapiUnitTest::SetUpTestCase(void)
{
    cout << "[SetUpTestCase]: " << endl;
}

void AudioDecoderBufferCapiUnitTest::TearDownTestCase(void)
{
    cout << "[TearDownTestCase]: " << endl;
}

void AudioDecoderBufferCapiUnitTest::SetUp(void)
{
    cout << "[SetUp]: SetUp!!!" << endl;
}

void AudioDecoderBufferCapiUnitTest::TearDown(void)
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

void AudioDecoderBufferCapiUnitTest::Release()
{
    Stop();
    OH_AudioCodec_Destroy(audioDec_);
}

void AudioDecoderBufferCapiUnitTest::HandleInputEOS(const uint32_t index)
{
    std::cout << "end buffer\n";
    OH_AudioCodec_PushInputBuffer(audioDec_, index);
    signal_->inBufferQueue_.pop();
    signal_->inQueue_.pop();
}

bool AudioDecoderBufferCapiUnitTest::ReadBuffer(OH_AVBuffer *buffer, uint32_t index)
{
    int64_t size;
    int64_t pts;
    inputFile_.read(reinterpret_cast<char *>(&size), sizeof(size));
    if (inputFile_.eof() || inputFile_.gcount() == 0 || size == 0) {
        buffer->buffer_->memory_->SetSize(1);
        buffer->buffer_->flag_ = AVCODEC_BUFFER_FLAGS_EOS;
        HandleInputEOS(index);
        cout << "Set EOS" << endl;
        return false;
    }

    if (inputFile_.gcount() != sizeof(size)) {
        cout << "Fatal: read size fail" << endl;
        return false;
    }

    inputFile_.read(reinterpret_cast<char *>(&buffer->buffer_->pts_), sizeof(buffer->buffer_->pts_));
    if (inputFile_.gcount() != sizeof(pts)) {
        cout << "Fatal: read size fail" << endl;
        return false;
    }

    inputFile_.read((char *)OH_AVBuffer_GetAddr(buffer), size);
    buffer->buffer_->memory_->SetSize(size);
    if (inputFile_.gcount() != size) {
        cout << "Fatal: read buffer fail" << endl;
        return false;
    }

    return true;
}

void AudioDecoderBufferCapiUnitTest::InputFunc()
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
    inputFile_.close();
}

void AudioDecoderBufferCapiUnitTest::OutputFunc()
{
    if (!pcmOutputFile_.is_open()) {
        std::cout << "open " << OUTPUT_MP3_PCM_FILE_PATH << " failed!" << std::endl;
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

int32_t AudioDecoderBufferCapiUnitTest::Start()
{
    isRunning_.store(true);
    inputLoop_ = make_unique<thread>(&AudioDecoderBufferCapiUnitTest::InputFunc, this);
    if (inputLoop_ == nullptr) {
        cout << "Fatal: No memory" << endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }

    outputLoop_ = make_unique<thread>(&AudioDecoderBufferCapiUnitTest::OutputFunc, this);
    if (outputLoop_ == nullptr) {
        cout << "Fatal: No memory" << endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }

    return OH_AudioCodec_Start(audioDec_);
}

int32_t AudioDecoderBufferCapiUnitTest::Stop()
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

int32_t AudioDecoderBufferCapiUnitTest::InitFile(AudioBufferFormatType audioType)
{
    if (audioType == AudioBufferFormatType::TYPE_AAC) {
        inputFile_.open(INPUT_AAC_FILE_PATH, std::ios::binary);
        pcmOutputFile_.open(OUTPUT_AAC_PCM_FILE_PATH.data(), std::ios::out | std::ios::binary);
    } else if (audioType == AudioBufferFormatType::TYPE_FLAC) {
        inputFile_.open(INPUT_FLAC_FILE_PATH, std::ios::binary);
        pcmOutputFile_.open(OUTPUT_FLAC_PCM_FILE_PATH.data(), std::ios::out | std::ios::binary);
    } else if (audioType == AudioBufferFormatType::TYPE_MP3) {
        inputFile_.open(INPUT_MP3_FILE_PATH, std::ios::binary);
        pcmOutputFile_.open(OUTPUT_MP3_PCM_FILE_PATH.data(), std::ios::out | std::ios::binary);
    } else if (audioType == AudioBufferFormatType::TYPE_VORBIS) {
        inputFile_.open(INPUT_VORBIS_FILE_PATH, std::ios::binary);
        pcmOutputFile_.open(OUTPUT_VORBIS_PCM_FILE_PATH.data(), std::ios::out | std::ios::binary);
    } else if (audioType == AudioBufferFormatType::TYPE_AMRNB) {
        inputFile_.open(INPUT_AMRNB_FILE_PATH, std::ios::binary);
        pcmOutputFile_.open(OUTPUT_AMRNB_PCM_FILE_PATH.data(), std::ios::out | std::ios::binary);
    } else if (audioType == AudioBufferFormatType::TYPE_AMRWB) {
        inputFile_.open(INPUT_AMRWB_FILE_PATH, std::ios::binary);
        pcmOutputFile_.open(OUTPUT_AMRWB_PCM_FILE_PATH.data(), std::ios::out | std::ios::binary);
    } else if (audioType == AudioBufferFormatType::TYPE_G711MU) {
        inputFile_.open(INPUT_G711MU_FILE_PATH, std::ios::binary);
        pcmOutputFile_.open(OUTPUT_G711MU_PCM_FILE_PATH.data(), std::ios::out | std::ios::binary);
    } else if (audioType == AudioBufferFormatType::TYPE_OPUS) {
        inputFile_.open(INPUT_OPUS_FILE_PATH, std::ios::binary);
        pcmOutputFile_.open(OUTPUT_OPUS_PCM_FILE_PATH.data(), std::ios::out | std::ios::binary);
    } else if (audioType == AudioBufferFormatType::TYPE_APE) {
        inputFile_.open(INPUT_APE_FILE_PATH, std::ios::binary);
        pcmOutputFile_.open(OUTPUT_APE_PCM_FILE_PATH.data(), std::ios::out | std::ios::binary);
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

int32_t AudioDecoderBufferCapiUnitTest::CreateCodecFunc(AudioBufferFormatType audioType)
{
    if (audioType == AudioBufferFormatType::TYPE_MP3) {
        audioDec_ = OH_AudioCodec_CreateByName((AVCodecCodecName::AUDIO_DECODER_MP3_NAME).data());
    } else if (audioType == AudioBufferFormatType::TYPE_FLAC) {
        audioDec_ = OH_AudioCodec_CreateByName((AVCodecCodecName::AUDIO_DECODER_FLAC_NAME).data());
    } else if (audioType == AudioBufferFormatType::TYPE_AAC) {
        audioDec_ = OH_AudioCodec_CreateByName((AVCodecCodecName::AUDIO_DECODER_AAC_NAME).data());
    } else if (audioType == AudioBufferFormatType::TYPE_VORBIS) {
        audioDec_ = OH_AudioCodec_CreateByName((AVCodecCodecName::AUDIO_DECODER_VORBIS_NAME).data());
    } else if (audioType == AudioBufferFormatType::TYPE_AMRWB) {
        audioDec_ = OH_AudioCodec_CreateByName((AVCodecCodecName::AUDIO_DECODER_AMRWB_NAME).data());
    } else if (audioType == AudioBufferFormatType::TYPE_AMRNB) {
        audioDec_ = OH_AudioCodec_CreateByName((AVCodecCodecName::AUDIO_DECODER_AMRNB_NAME).data());
    } else if (audioType == AudioBufferFormatType::TYPE_G711MU) {
        audioDec_ = OH_AudioCodec_CreateByName((AVCodecCodecName::AUDIO_DECODER_G711MU_NAME).data());
    } else if (audioType == AudioBufferFormatType::TYPE_OPUS) {
        audioDec_ = OH_AudioCodec_CreateByName((AVCodecCodecName::AUDIO_DECODER_OPUS_NAME).data());
    } else if (audioType == AudioBufferFormatType::TYPE_APE) {
        audioDec_ = OH_AudioCodec_CreateByName((AVCodecCodecName::AUDIO_DECODER_APE_NAME).data());
    } else {
        cout << "audio name not support" << endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }

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

int32_t AudioDecoderBufferCapiUnitTest::Configure(AudioBufferFormatType audioType)
{
    format_ = OH_AVFormat_Create();
    if (format_ == nullptr) {
        cout << "Fatal: create format failed" << endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }
    int32_t channelCount = DEFAULT_CHANNEL_COUNT;
    int32_t sampleRate = DEFAULT_SAMPLE_RATE;
    if (audioType == AudioBufferFormatType::TYPE_AAC) {
        OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_AAC_IS_ADTS.data(), DEFAULT_AAC_TYPE);
        OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(),
                                DEFAULT_SAMPLE_FORMAT);
    } else if (audioType == AudioBufferFormatType::TYPE_AMRNB || audioType == AudioBufferFormatType::TYPE_G711MU ||
        audioType == AudioBufferFormatType::TYPE_OPUS || audioType == AudioBufferFormatType::TYPE_APE) {
        channelCount = 1;
        sampleRate = AMRNB_SAMPLE_RATE;
    } else if (audioType == AudioBufferFormatType::TYPE_AMRWB) {
        channelCount = 1;
        sampleRate = AMRWB_SAMPLE_RATE;
        OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(),
                                DEFAULT_SAMPLE_FORMAT);
    }
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), channelCount);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), sampleRate);
    OH_AVFormat_SetLongValue(format_, MediaDescriptionKey::MD_KEY_BITRATE.data(), BITS_RATE[(uint32_t)audioType]);
    if (audioType == AudioBufferFormatType::TYPE_VORBIS) {
        OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(),
                                DEFAULT_SAMPLE_FORMAT);
        int64_t extradataSize;
        if (!inputFile_.is_open()) {
            cout << "Fatal: file is not open" << endl;
            return OH_AVErrCode::AV_ERR_UNKNOWN;
        }
        inputFile_.read(reinterpret_cast<char *>(&extradataSize), sizeof(int64_t));
        if (inputFile_.gcount() != sizeof(int64_t) || extradataSize < 0) {
            cout << "Fatal: read extradataSize bytes error" << endl;
            return OH_AVErrCode::AV_ERR_UNKNOWN;
        }
        char buffer[extradataSize];
        inputFile_.read(buffer, extradataSize);
        if (inputFile_.gcount() != extradataSize) {
            cout << "Fatal: read extradata bytes error" << endl;
            return OH_AVErrCode::AV_ERR_UNKNOWN;
        }
        OH_AVFormat_SetBuffer(format_, MediaDescriptionKey::MD_KEY_CODEC_CONFIG.data(), (uint8_t *)buffer,
                              extradataSize);
    }
    return OH_AudioCodec_Configure(audioDec_, format_);
}

int32_t AudioDecoderBufferCapiUnitTest::CheckSoFunc()
{
    soFile_ = std::make_unique<std::ifstream>(OPUS_SO_FILE_PATH, std::ios::binary);
    if (!soFile_->is_open()) {
        cout << "Fatal: Open so file failed" << endl;
        return false;
    }
    soFile_->close();
    return true;
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Mp3_CreateByMime_01, TestSize.Level1)
{
    audioDec_ = OH_AudioCodec_CreateByMime(AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_MPEG.data(), false);
    EXPECT_NE(nullptr, audioDec_);
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Mp3_CreateByName_01, TestSize.Level1)
{
    audioDec_ = OH_AudioCodec_CreateByName((AVCodecCodecName::AUDIO_DECODER_MP3_NAME).data());
    EXPECT_NE(nullptr, audioDec_);
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Mp3_Configure_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_MP3));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_MP3));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Mp3_Configure_02, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_MP3));
    format_ = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format_);

    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(), DEFAULT_SAMPLE_FORMAT);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), DEFAULT_SAMPLE_RATE);
    OH_AVFormat_SetLongValue(format_, MediaDescriptionKey::MD_KEY_BITRATE.data(),
        BITS_RATE[(uint32_t)AudioBufferFormatType::TYPE_MP3]);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, format_)); // missing channel count

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioDec_));
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), ABNORMAL_MIN_CHANNEL_COUNT);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, format_)); // abnormal min channel count

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioDec_));
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), ABNORMAL_MAX_CHANNEL_COUNT);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, format_)); // abnormal max channel count

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioDec_));
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), DEFAULT_CHANNEL_COUNT);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, format_)); // normal channel count

    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Mp3_Configure_03, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_MP3));
    format_ = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format_);

    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(), DEFAULT_SAMPLE_FORMAT);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), DEFAULT_CHANNEL_COUNT);
    OH_AVFormat_SetLongValue(format_, MediaDescriptionKey::MD_KEY_BITRATE.data(),
        BITS_RATE[(uint32_t)AudioBufferFormatType::TYPE_MP3]);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, format_)); // missing sample rate

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioDec_));
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), ABNORMAL_SAMPLE_RATE);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, format_)); // abnormal sample rate

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioDec_));
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), DEFAULT_SAMPLE_RATE);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, format_)); // normal sample rate

    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Mp3_Configure_04, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_MP3));
    format_ = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format_);

    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(), DEFAULT_SAMPLE_FORMAT);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), DEFAULT_CHANNEL_COUNT);
    OH_AVFormat_SetLongValue(format_, MediaDescriptionKey::MD_KEY_BITRATE.data(),
        BITS_RATE[(uint32_t)AudioBufferFormatType::TYPE_MP3]);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), DEFAULT_SAMPLE_RATE);
    OH_AVFormat_SetLongValue(format_, MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT.data(), CHANNEL_LAYOUT);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, format_)); // normal channel layout
    
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioDec_));
    OH_AVFormat_SetLongValue(format_, MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT.data(), ABNORMAL_CHANNEL_LAYOUT);
    ASSERT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, format_)); // abnormal channel layout
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Mp3_SetParameter_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_MP3));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_MP3));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_MP3));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Flush(audioDec_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_SetParameter(audioDec_, format_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Mp3_SetParameter_02, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_MP3));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_MP3));
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_SetParameter(audioDec_, format_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Mp3_Start_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_MP3));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_MP3));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_MP3));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Mp3_Start_02, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_MP3));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_MP3));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_MP3));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Stop());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Start(audioDec_));
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Mp3_Stop_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_MP3));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_MP3));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_MP3));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    sleep(1);

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Stop());
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Mp3_Flush_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_MP3));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_MP3));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_MP3));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Flush(audioDec_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Mp3_Reset_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_MP3));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_MP3));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioDec_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Mp3_Reset_02, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_MP3));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_MP3));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_MP3));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Stop());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioDec_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Mp3_Reset_03, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_MP3));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_MP3));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_MP3));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioDec_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Mp3_Destroy_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_MP3));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_MP3));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_MP3));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Stop());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Destroy(audioDec_));
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Mp3_Destroy_02, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_MP3));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_MP3));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_MP3));

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Destroy(audioDec_));
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Mp3_GetOutputFormat_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_MP3));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_MP3));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_MP3));

    EXPECT_NE(nullptr, OH_AudioCodec_GetOutputDescription(audioDec_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Mp3_IsValid_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_MP3));
    bool isValid = false;
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_IsValid(audioDec_, &isValid));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Mp3_Prepare_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_MP3));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_MP3));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Prepare(audioDec_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Mp3_PushInputData_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_MP3));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_MP3));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_MP3));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());

    // case0 传参异常
    const uint32_t index = 1024;
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_PushInputBuffer(audioDec_, index));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Mp3_ReleaseOutputBuffer_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_MP3));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_MP3));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_MP3));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());

    // case0 传参异常
    const uint32_t index = 1024;
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_FreeOutputBuffer(audioDec_, index));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Flac_CreateByMime_01, TestSize.Level1)
{
    audioDec_ = OH_AudioCodec_CreateByMime(AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_FLAC.data(), false);
    EXPECT_NE(nullptr, audioDec_);
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Flac_CreateByName_01, TestSize.Level1)
{
    audioDec_ = OH_AudioCodec_CreateByName((AVCodecCodecName::AUDIO_DECODER_FLAC_NAME).data());
    EXPECT_NE(nullptr, audioDec_);
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Flac_Configure_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_FLAC));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_FLAC));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Flac_Configure_02, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_FLAC));
    format_ = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format_);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), ABNORMAL_MIN_CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), DEFAULT_SAMPLE_RATE);
    OH_AVFormat_SetLongValue(format_, MediaDescriptionKey::MD_KEY_BITRATE.data(),
                             BITS_RATE[(uint32_t)AudioBufferFormatType::TYPE_FLAC]);
    ASSERT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, format_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Flac_Configure_03, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_FLAC));
    format_ = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format_);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), ABNORMAL_MAX_CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), DEFAULT_SAMPLE_RATE);
    OH_AVFormat_SetLongValue(format_, MediaDescriptionKey::MD_KEY_BITRATE.data(),
                             BITS_RATE[(uint32_t)AudioBufferFormatType::TYPE_FLAC]);
    ASSERT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, format_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Flac_Configure_04, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_FLAC));
    format_ = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format_);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), DEFAULT_CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), ABNORMAL_SAMPLE_RATE);
    OH_AVFormat_SetLongValue(format_, MediaDescriptionKey::MD_KEY_BITRATE.data(),
                             BITS_RATE[(uint32_t)AudioBufferFormatType::TYPE_FLAC]);
    ASSERT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, format_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Flac_Configure_05, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_FLAC));
    format_ = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format_);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), DEFAULT_CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), FLAC_192K_SAMPLE_RATE);
    OH_AVFormat_SetLongValue(format_, MediaDescriptionKey::MD_KEY_BITRATE.data(),
                             BITS_RATE[(uint32_t)AudioBufferFormatType::TYPE_FLAC]);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, format_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Flac_Configure_06, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_FLAC));
    format_ = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format_);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), DEFAULT_CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), DEFAULT_SAMPLE_RATE);
    OH_AVFormat_SetLongValue(format_, MediaDescriptionKey::MD_KEY_BITRATE.data(),
                             BITS_RATE[(uint32_t)AudioBufferFormatType::TYPE_FLAC]);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(), DEFAULT_SAMPLE_FORMAT);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, format_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Flac_Configure_07, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_FLAC));
    format_ = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format_);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), DEFAULT_CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), DEFAULT_SAMPLE_RATE);
    OH_AVFormat_SetLongValue(format_, MediaDescriptionKey::MD_KEY_BITRATE.data(),
                             BITS_RATE[(uint32_t)AudioBufferFormatType::TYPE_FLAC]);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(), FLAC_S24_SAMPLE_FORMAT);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, format_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Flac_Configure_08, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_FLAC));
    format_ = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format_);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), DEFAULT_CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), DEFAULT_SAMPLE_RATE);
    OH_AVFormat_SetLongValue(format_, MediaDescriptionKey::MD_KEY_BITRATE.data(),
                             BITS_RATE[(uint32_t)AudioBufferFormatType::TYPE_FLAC]);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(), DEFAULT_SAMPLE_FORMAT);
    OH_AVFormat_SetLongValue(format_, MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT.data(), CHANNEL_LAYOUT);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, format_)); // normal channel layout

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioDec_));
    OH_AVFormat_SetLongValue(format_, MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT.data(), ABNORMAL_CHANNEL_LAYOUT);
    ASSERT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, format_)); // abnormal channel layout
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Flac_SetParameter_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_FLAC));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_FLAC));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_FLAC));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Flush(audioDec_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_SetParameter(audioDec_, format_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Flac_SetParameter_02, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_FLAC));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_FLAC));
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_SetParameter(audioDec_, format_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Flac_Start_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_FLAC));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_FLAC));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_FLAC));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Flac_Start_02, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_FLAC));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_FLAC));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_FLAC));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Stop());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Start(audioDec_));
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Flac_Stop_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_FLAC));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_FLAC));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_FLAC));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    sleep(1);

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Stop());
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Flac_Flush_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_FLAC));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_FLAC));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_FLAC));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Flush(audioDec_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Flac_Reset_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_FLAC));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_FLAC));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioDec_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Flac_Reset_02, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_FLAC));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_FLAC));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_FLAC));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Stop());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioDec_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Flac_Reset_03, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_FLAC));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_FLAC));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_FLAC));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioDec_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Flac_Destroy_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_FLAC));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_FLAC));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_FLAC));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Stop());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Destroy(audioDec_));
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Flac_Destroy_02, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_FLAC));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_FLAC));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_FLAC));

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Destroy(audioDec_));
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Flac_GetOutputFormat_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_FLAC));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_FLAC));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_FLAC));

    EXPECT_NE(nullptr, OH_AudioCodec_GetOutputDescription(audioDec_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Flac_IsValid_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_FLAC));
    bool isValid = false;
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_IsValid(audioDec_, &isValid));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Flac_Prepare_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_FLAC));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_FLAC));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Prepare(audioDec_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Flac_PushInputData_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_FLAC));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_FLAC));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_FLAC));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());

    // case0 传参异常
    const uint32_t index = 1024;
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_PushInputBuffer(audioDec_, index));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Flac_ReleaseOutputBuffer_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_FLAC));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_FLAC));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_FLAC));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());

    // case0 传参异常
    const uint32_t index = 1024;
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_FreeOutputBuffer(audioDec_, index));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Flac_192k_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_FLAC));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_FLAC));
    format_ = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format_);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), DEFAULT_CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), FLAC_192K_SAMPLE_RATE);
    OH_AVFormat_SetLongValue(format_, MediaDescriptionKey::MD_KEY_BITRATE.data(),
                             BITS_RATE[(uint32_t)AudioBufferFormatType::TYPE_FLAC]);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, format_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Stop());
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Aac_CreateByMime_01, TestSize.Level1)
{
    audioDec_ = OH_AudioCodec_CreateByMime(AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_AAC.data(), false);
    EXPECT_NE(nullptr, audioDec_);
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Aac_CreateByName_01, TestSize.Level1)
{
    audioDec_ = OH_AudioCodec_CreateByName((AVCodecCodecName::AUDIO_DECODER_AAC_NAME).data());
    EXPECT_NE(nullptr, audioDec_);
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Aac_Configure_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_AAC));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_AAC));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Aac_Configure_02, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_AAC));
    format_ = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format_);

    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(), DEFAULT_SAMPLE_FORMAT);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), DEFAULT_CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), DEFAULT_SAMPLE_RATE);
    OH_AVFormat_SetLongValue(format_, MediaDescriptionKey::MD_KEY_BITRATE.data(),
        BITS_RATE[(uint32_t)AudioBufferFormatType::TYPE_AAC]);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_AAC_IS_ADTS.data(), ABNORMAL_AAC_TYPE);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, format_)); // abnormal aac type

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioDec_));
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_AAC_IS_ADTS.data(), DEFAULT_AAC_TYPE);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, format_)); // normal aac type

    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Aac_Configure_03, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_AAC));
    format_ = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format_);

    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(), DEFAULT_SAMPLE_FORMAT);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_AAC_IS_ADTS.data(), DEFAULT_AAC_TYPE);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), DEFAULT_SAMPLE_RATE);
    OH_AVFormat_SetLongValue(format_, MediaDescriptionKey::MD_KEY_BITRATE.data(),
        BITS_RATE[(uint32_t)AudioBufferFormatType::TYPE_AAC]);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, format_)); // missing channel count

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioDec_));
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), ABNORMAL_MIN_CHANNEL_COUNT);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, format_)); // abnormal min channel count

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioDec_));
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), ABNORMAL_MAX_CHANNEL_COUNT);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, format_)); // abnormal max channel count

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioDec_));
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), DEFAULT_CHANNEL_COUNT);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, format_)); // normal channel count

    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Aac_Configure_04, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_AAC));
    format_ = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format_);

    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(), DEFAULT_SAMPLE_FORMAT);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_AAC_IS_ADTS.data(), DEFAULT_AAC_TYPE);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), DEFAULT_CHANNEL_COUNT);
    OH_AVFormat_SetLongValue(format_, MediaDescriptionKey::MD_KEY_BITRATE.data(),
        BITS_RATE[(uint32_t)AudioBufferFormatType::TYPE_AAC]);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, format_)); // missing sample rate

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioDec_));
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), ABNORMAL_SAMPLE_RATE);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, format_)); // abnormal sample rate

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioDec_));
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), DEFAULT_SAMPLE_RATE);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, format_)); // normal sample rate

    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Aac_Configure_05, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_AAC));
    format_ = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format_);

    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_AAC_IS_ADTS.data(), DEFAULT_AAC_TYPE);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), DEFAULT_CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), DEFAULT_SAMPLE_RATE);
    OH_AVFormat_SetLongValue(format_, MediaDescriptionKey::MD_KEY_BITRATE.data(),
        BITS_RATE[(uint32_t)AudioBufferFormatType::TYPE_AAC]);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(), ABNORMAL_SAMPLE_FORMAT);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, format_)); // abnormal sample format

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioDec_));
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(), DEFAULT_SAMPLE_FORMAT);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, format_)); // normal sample format

    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Aac_SetParameter_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_AAC));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_AAC));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_AAC));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Flush(audioDec_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_SetParameter(audioDec_, format_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Aac_SetParameter_02, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_AAC));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_AAC));
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_SetParameter(audioDec_, format_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Aac_Start_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_AAC));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_AAC));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_AAC));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Aac_Start_02, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_AAC));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_AAC));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_AAC));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Stop());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Start(audioDec_));
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Aac_Stop_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_AAC));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_AAC));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_AAC));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    sleep(1);

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Stop());
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Aac_Flush_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_AAC));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_AAC));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_AAC));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Flush(audioDec_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Aac_Reset_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_AAC));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_AAC));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioDec_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Aac_Reset_02, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_AAC));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_AAC));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_AAC));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Stop());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioDec_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Aac_Reset_03, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_AAC));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_AAC));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_AAC));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioDec_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Aac_Destroy_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_AAC));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_AAC));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_AAC));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Stop());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Destroy(audioDec_));
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Aac_Destroy_02, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_AAC));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_AAC));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_AAC));

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Destroy(audioDec_));
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Aac_GetOutputFormat_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_AAC));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_AAC));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_AAC));

    EXPECT_NE(nullptr, OH_AudioCodec_GetOutputDescription(audioDec_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Aac_IsValid_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_AAC));
    bool isValid = false;
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_IsValid(audioDec_, &isValid));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Aac_Prepare_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_AAC));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_AAC));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Prepare(audioDec_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Aac_PushInputData_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_AAC));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_AAC));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_AAC));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());

    // case0 传参异常
    const uint32_t index = 1024;
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_PushInputBuffer(audioDec_, index));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Aac_ReleaseOutputBuffer_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_AAC));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_AAC));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_AAC));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());

    // case0 传参异常
    const uint32_t index = 1024;
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_FreeOutputBuffer(audioDec_, index));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Vorbis_CreateByMime_01, TestSize.Level1)
{
    audioDec_ = OH_AudioCodec_CreateByMime(AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_VORBIS.data(), false);
    EXPECT_NE(nullptr, audioDec_);
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Vorbis_CreateByName_01, TestSize.Level1)
{
    audioDec_ = OH_AudioCodec_CreateByName((AVCodecCodecName::AUDIO_DECODER_VORBIS_NAME).data());
    EXPECT_NE(nullptr, audioDec_);
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Vorbis_Configure_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_VORBIS));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_VORBIS));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_VORBIS));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Vorbis_Configure_02, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_VORBIS));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_VORBIS));
    format_ = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format_);

    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(), DEFAULT_SAMPLE_FORMAT);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), DEFAULT_SAMPLE_RATE);
    OH_AVFormat_SetLongValue(format_, MediaDescriptionKey::MD_KEY_BITRATE.data(),
        BITS_RATE[(uint32_t)AudioBufferFormatType::TYPE_VORBIS]);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, format_)); // missing channel count

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioDec_));
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), ABNORMAL_MIN_CHANNEL_COUNT);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, format_)); // abnormal min channel count

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioDec_));
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), ABNORMAL_MAX_CHANNEL_COUNT);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, format_)); // abnormal max channel count

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioDec_));
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), DEFAULT_CHANNEL_COUNT);
    int64_t extradataSize;
    EXPECT_EQ(true, inputFile_.is_open());
    inputFile_.read(reinterpret_cast<char *>(&extradataSize), sizeof(int64_t));
    EXPECT_EQ(false, inputFile_.gcount() != sizeof(int64_t) || extradataSize < 0);
    char buffer[extradataSize];
    inputFile_.read(buffer, extradataSize);
    EXPECT_EQ(false, inputFile_.gcount() != extradataSize);
    OH_AVFormat_SetBuffer(format_, MediaDescriptionKey::MD_KEY_CODEC_CONFIG.data(), (uint8_t *)buffer,
                            extradataSize);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, format_)); // normal channel count

    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Vorbis_Configure_03, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_VORBIS));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_VORBIS));
    format_ = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format_);

    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(), DEFAULT_SAMPLE_FORMAT);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), DEFAULT_CHANNEL_COUNT);
    OH_AVFormat_SetLongValue(format_, MediaDescriptionKey::MD_KEY_BITRATE.data(),
        BITS_RATE[(uint32_t)AudioBufferFormatType::TYPE_VORBIS]);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, format_)); // missing sample rate

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioDec_));
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), ABNORMAL_SAMPLE_RATE);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, format_)); // abnormal sample rate

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioDec_));
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), DEFAULT_SAMPLE_RATE);
    int64_t extradataSize;
    EXPECT_EQ(true, inputFile_.is_open());
    inputFile_.read(reinterpret_cast<char *>(&extradataSize), sizeof(int64_t));
    EXPECT_EQ(false, inputFile_.gcount() != sizeof(int64_t) || extradataSize < 0);
    char buffer[extradataSize];
    inputFile_.read(buffer, extradataSize);
    EXPECT_EQ(false, inputFile_.gcount() != extradataSize);
    OH_AVFormat_SetBuffer(format_, MediaDescriptionKey::MD_KEY_CODEC_CONFIG.data(), (uint8_t *)buffer,
                            extradataSize);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, format_)); // normal sample rate

    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Vorbis_Configure_04, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_VORBIS));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_VORBIS));
    format_ = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format_);

    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), DEFAULT_CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), DEFAULT_SAMPLE_RATE);
    OH_AVFormat_SetLongValue(format_, MediaDescriptionKey::MD_KEY_BITRATE.data(),
        BITS_RATE[(uint32_t)AudioBufferFormatType::TYPE_VORBIS]);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(), ABNORMAL_SAMPLE_FORMAT);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, format_)); // abnormal sample format

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioDec_));
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(), DEFAULT_SAMPLE_FORMAT);
    int64_t extradataSize;
    EXPECT_EQ(true, inputFile_.is_open());
    inputFile_.read(reinterpret_cast<char *>(&extradataSize), sizeof(int64_t));
    EXPECT_EQ(false, inputFile_.gcount() != sizeof(int64_t) || extradataSize < 0);
    char buffer[extradataSize];
    inputFile_.read(buffer, extradataSize);
    EXPECT_EQ(false, inputFile_.gcount() != extradataSize);
    OH_AVFormat_SetBuffer(format_, MediaDescriptionKey::MD_KEY_CODEC_CONFIG.data(), (uint8_t *)buffer,
                            extradataSize);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, format_)); // normal sample format

    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Vorbis_SetParameter_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_VORBIS));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_VORBIS));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_VORBIS));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Flush(audioDec_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_SetParameter(audioDec_, format_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Vorbis_SetParameter_02, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_VORBIS));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_VORBIS));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_VORBIS));
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_SetParameter(audioDec_, format_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Vorbis_Start_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_VORBIS));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_VORBIS));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_VORBIS));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Vorbis_Start_02, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_VORBIS));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_VORBIS));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_VORBIS));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Stop());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Start(audioDec_));
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Vorbis_Stop_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_VORBIS));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_VORBIS));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_VORBIS));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    sleep(1);

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Stop());
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Vorbis_Flush_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_VORBIS));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_VORBIS));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_VORBIS));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Flush(audioDec_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Vorbis_Reset_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_VORBIS));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_VORBIS));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_VORBIS));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioDec_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Vorbis_Reset_02, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_VORBIS));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_VORBIS));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_VORBIS));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Stop());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioDec_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Vorbis_Reset_03, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_VORBIS));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_VORBIS));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_VORBIS));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioDec_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Vorbis_Destroy_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_VORBIS));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_VORBIS));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_VORBIS));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Stop());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Destroy(audioDec_));
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Vorbis_Destroy_02, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_VORBIS));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_VORBIS));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_VORBIS));

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Destroy(audioDec_));
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Vorbis_GetOutputFormat_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_VORBIS));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_VORBIS));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_VORBIS));

    EXPECT_NE(nullptr, OH_AudioCodec_GetOutputDescription(audioDec_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Vorbis_IsValid_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_VORBIS));
    bool isValid = false;
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_IsValid(audioDec_, &isValid));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Vorbis_Prepare_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_VORBIS));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_VORBIS));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_VORBIS));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Prepare(audioDec_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Vorbis_PushInputData_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_VORBIS));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_VORBIS));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_VORBIS));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());

    // case0 传参异常
    const uint32_t index = 1024;
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_PushInputBuffer(audioDec_, index));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Vorbis_ReleaseOutputBuffer_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_VORBIS));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_VORBIS));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_VORBIS));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());

    // case0 传参异常
    const uint32_t index = 1024;
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_FreeOutputBuffer(audioDec_, index));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, vorbisMissingHeader, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_VORBIS));
    format_ = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format_);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), DEFAULT_CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), DEFAULT_SAMPLE_RATE);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_MAX_INPUT_SIZE.data(), ABNORMAL_MAX_INPUT_SIZE);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, format_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, vorbisMissingIdHeader, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_VORBIS));
    format_ = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format_);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), DEFAULT_CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), DEFAULT_SAMPLE_RATE);
    uint8_t buffer[1];
    OH_AVFormat_SetBuffer(format_, MediaDescriptionKey::MD_KEY_IDENTIFICATION_HEADER.data(), buffer, 1);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, format_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, vorbisInvalidHeader, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_VORBIS));
    format_ = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format_);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), DEFAULT_CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), DEFAULT_SAMPLE_RATE);
    uint8_t buffer[1];
    OH_AVFormat_SetBuffer(format_, MediaDescriptionKey::MD_KEY_IDENTIFICATION_HEADER.data(), buffer, 1);
    OH_AVFormat_SetBuffer(format_, MediaDescriptionKey::MD_KEY_SETUP_HEADER.data(), buffer, 1);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_UNKNOWN, OH_AudioCodec_Configure(audioDec_, format_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Amrwb_CreateByMime_01, TestSize.Level1)
{
    audioDec_ = OH_AudioCodec_CreateByMime(AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_AMRWB.data(), false);
    EXPECT_NE(nullptr, audioDec_);
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Amrwb_CreateByName_01, TestSize.Level1)
{
    audioDec_ = OH_AudioCodec_CreateByName((AVCodecCodecName::AUDIO_DECODER_AMRWB_NAME).data());
    EXPECT_NE(nullptr, audioDec_);
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Amrwb_Configure_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_AMRWB));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_AMRWB));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Amrwb_Configure_02, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_AMRWB));
    format_ = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format_);

    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(), DEFAULT_SAMPLE_FORMAT);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), AMRWB_SAMPLE_RATE);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, format_)); // missing channel count

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioDec_));
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), ABNORMAL_MIN_CHANNEL_COUNT);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, format_)); // abnormal min channel count

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioDec_));
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), ONE_CHANNEL_COUNT);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, format_)); // normal channel count

    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Amrwb_Configure_03, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_AMRWB));
    format_ = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format_);

    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(), DEFAULT_SAMPLE_FORMAT);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), ONE_CHANNEL_COUNT);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, format_)); // missing sample rate

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioDec_));
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), ABNORMAL_SAMPLE_RATE);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, format_)); // abnormal sample rate

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioDec_));
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), AMRWB_SAMPLE_RATE);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, format_)); // normal sample rate

    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Amrwb_Configure_04, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_AMRWB));
    format_ = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format_);

    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), ONE_CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), AMRWB_SAMPLE_RATE);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(), ABNORMAL_SAMPLE_FORMAT);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, format_)); // abnormal sample format

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioDec_));
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(), DEFAULT_SAMPLE_FORMAT);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, format_)); // normal sample format

    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Amrwb_SetParameter_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_AMRWB));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_AMRWB));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_AMRWB));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Flush(audioDec_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_SetParameter(audioDec_, format_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Amrwb_SetParameter_02, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_AMRWB));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_AMRWB));
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_SetParameter(audioDec_, format_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Amrwb_Start_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_AMRWB));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_AMRWB));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_AMRWB));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Amrwb_Start_02, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_AMRWB));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_AMRWB));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_AMRWB));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Stop());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Start(audioDec_));
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Amrwb_Stop_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_AMRWB));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_AMRWB));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_AMRWB));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    sleep(1);

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Stop());
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Amrwb_Flush_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_AMRWB));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_AMRWB));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_AMRWB));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Flush(audioDec_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Amrwb_Reset_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_AMRWB));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_AMRWB));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioDec_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Amrwb_Reset_02, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_AMRWB));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_AMRWB));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_AMRWB));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Stop());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioDec_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Amrwb_Reset_03, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_AMRWB));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_AMRWB));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_AMRWB));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioDec_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Amrwb_Destroy_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_AMRWB));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_AMRWB));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_AMRWB));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Stop());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Destroy(audioDec_));
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Amrwb_Destroy_02, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_AMRWB));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_AMRWB));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_AMRWB));

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Destroy(audioDec_));
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Amrwb_GetOutputFormat_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_AMRWB));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_AMRWB));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_AMRWB));

    EXPECT_NE(nullptr, OH_AudioCodec_GetOutputDescription(audioDec_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Amrwb_IsValid_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_AMRWB));
    bool isValid = false;
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_IsValid(audioDec_, &isValid));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Amrwb_Prepare_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_AMRWB));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_AMRWB));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Prepare(audioDec_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Amrwb_PushInputData_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_AMRWB));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_AMRWB));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_AMRWB));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());

    // case0 传参异常
    const uint32_t index = 1024;
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_PushInputBuffer(audioDec_, index));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Amrwb_ReleaseOutputBuffer_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_AMRWB));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_AMRWB));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_AMRWB));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());

    // case0 传参异常
    const uint32_t index = 1024;
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_FreeOutputBuffer(audioDec_, index));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Amrnb_CreateByMime_01, TestSize.Level1)
{
    audioDec_ = OH_AudioCodec_CreateByMime(AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_AMRNB.data(), false);
    EXPECT_NE(nullptr, audioDec_);
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Amrnb_CreateByName_01, TestSize.Level1)
{
    audioDec_ = OH_AudioCodec_CreateByName((AVCodecCodecName::AUDIO_DECODER_AMRNB_NAME).data());
    EXPECT_NE(nullptr, audioDec_);
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Amrnb_Configure_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_AMRNB));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_AMRNB));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Amrnb_Configure_02, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_AMRNB));
    format_ = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format_);

    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(), DEFAULT_SAMPLE_FORMAT);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), AMRNB_SAMPLE_RATE);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, format_)); // missing channel count

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioDec_));
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), ABNORMAL_MIN_CHANNEL_COUNT);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, format_)); // abnormal min channel count

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioDec_));
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), ONE_CHANNEL_COUNT);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, format_)); // normal channel count

    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Amrnb_Configure_03, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_AMRNB));
    format_ = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format_);

    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(), DEFAULT_SAMPLE_FORMAT);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), ONE_CHANNEL_COUNT);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, format_)); // missing sample rate

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioDec_));
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), ABNORMAL_SAMPLE_RATE);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, format_)); // abnormal sample rate

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioDec_));
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), AMRNB_SAMPLE_RATE);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, format_)); // normal sample rate

    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Amrnb_SetParameter_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_AMRNB));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_AMRNB));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_AMRNB));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Flush(audioDec_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_SetParameter(audioDec_, format_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Amrnb_SetParameter_02, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_AMRNB));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_AMRNB));
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_SetParameter(audioDec_, format_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Amrnb_Start_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_AMRNB));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_AMRNB));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_AMRNB));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Amrnb_Start_02, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_AMRNB));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_AMRNB));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_AMRNB));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Stop());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Start(audioDec_));
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Amrnb_Stop_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_AMRNB));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_AMRNB));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_AMRNB));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    sleep(1);

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Stop());
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Amrnb_Flush_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_AMRNB));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_AMRNB));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_AMRNB));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Flush(audioDec_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Amrnb_Reset_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_AMRNB));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_AMRNB));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioDec_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Amrnb_Reset_02, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_AMRNB));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_AMRNB));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_AMRNB));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Stop());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioDec_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Amrnb_Reset_03, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_AMRNB));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_AMRNB));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_AMRNB));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioDec_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Amrnb_Destroy_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_AMRNB));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_AMRNB));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_AMRNB));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Stop());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Destroy(audioDec_));
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Amrnb_Destroy_02, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_AMRNB));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_AMRNB));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_AMRNB));

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Destroy(audioDec_));
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Amrnb_GetOutputFormat_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_AMRNB));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_AMRNB));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_AMRNB));

    EXPECT_NE(nullptr, OH_AudioCodec_GetOutputDescription(audioDec_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Amrnb_IsValid_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_AMRNB));
    bool isValid = false;
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_IsValid(audioDec_, &isValid));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Amrnb_Prepare_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_AMRNB));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_AMRNB));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Prepare(audioDec_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Amrnb_PushInputData_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_AMRNB));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_AMRNB));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_AMRNB));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());

    // case0 传参异常
    const uint32_t index = 1024;
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_PushInputBuffer(audioDec_, index));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Amrnb_ReleaseOutputBuffer_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_AMRNB));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_AMRNB));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_AMRNB));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());

    // case0 传参异常
    const uint32_t index = 1024;
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_FreeOutputBuffer(audioDec_, index));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_G711mu_CreateByMime_01, TestSize.Level1)
{
    audioDec_ = OH_AudioCodec_CreateByMime(AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_G711MU.data(), false);
    EXPECT_NE(nullptr, audioDec_);
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_G711mu_CreateByName_01, TestSize.Level1)
{
    audioDec_ = OH_AudioCodec_CreateByName((AVCodecCodecName::AUDIO_DECODER_G711MU_NAME).data());
    EXPECT_NE(nullptr, audioDec_);
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_G711mu_Configure_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_G711MU));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_G711MU));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_G711mu_SetParameter_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_G711MU));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_G711MU));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_G711MU));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Flush(audioDec_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_SetParameter(audioDec_, format_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_G711mu_SetParameter_02, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_G711MU));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_G711MU));
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_SetParameter(audioDec_, format_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_G711mu_Start_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_G711MU));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_G711MU));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_G711MU));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_G711mu_Start_02, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_G711MU));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_G711MU));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_G711MU));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Stop());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Start(audioDec_));
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_G711mu_Stop_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_G711MU));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_G711MU));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_G711MU));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    sleep(1);

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Stop());
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_G711mu_Flush_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_G711MU));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_G711MU));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_G711MU));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Flush(audioDec_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_G711mu_Reset_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_G711MU));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_G711MU));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioDec_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_G711mu_Reset_02, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_G711MU));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_G711MU));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_G711MU));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Stop());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioDec_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_G711mu_Reset_03, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_G711MU));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_G711MU));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_G711MU));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioDec_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_G711mu_Destroy_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_G711MU));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_G711MU));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_G711MU));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Stop());
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Destroy(audioDec_));
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_G711mu_Destroy_02, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_G711MU));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_G711MU));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_G711MU));

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Destroy(audioDec_));
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_G711mu_GetOutputFormat_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_G711MU));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_G711MU));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_G711MU));

    EXPECT_NE(nullptr, OH_AudioCodec_GetOutputDescription(audioDec_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_G711mu_IsValid_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_G711MU));
    bool isValid = false;
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_IsValid(audioDec_, &isValid));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_G711mu_Prepare_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_G711MU));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_G711MU));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Prepare(audioDec_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_G711mu_PushInputData_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_G711MU));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_G711MU));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_G711MU));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());

    // case0 传参异常
    const uint32_t index = 1024;
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_PushInputBuffer(audioDec_, index));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_G711mu_ReleaseOutputBuffer_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_G711MU));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_G711MU));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_G711MU));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());

    // case0 传参异常
    const uint32_t index = 1024;
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_FreeOutputBuffer(audioDec_, index));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_g711muCheckChannelCount, TestSize.Level1)
{
    CreateCodecFunc(AudioBufferFormatType::TYPE_G711MU);
    format_ = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format_);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), AMRNB_SAMPLE_RATE);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, format_)); // missing channel count

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioDec_));
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), ABNORMAL_MAX_CHANNEL_COUNT);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, format_)); // illegal channel count

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioDec_));
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), ONE_CHANNEL_COUNT);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, format_)); // normal channel count

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Destroy(audioDec_));
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_g711muCheckSampleRate, TestSize.Level1)
{
    CreateCodecFunc(AudioBufferFormatType::TYPE_G711MU);
    format_ = OH_AVFormat_Create();
    EXPECT_NE(nullptr, format_);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), ONE_CHANNEL_COUNT);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, format_)); // missing sample rate

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioDec_));
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), DEFAULT_SAMPLE_RATE);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, format_)); // illegal sample rate

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioDec_));
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), AMRNB_SAMPLE_RATE);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, format_)); // normal sample rate

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Destroy(audioDec_));
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Opus_CreateByMime_01, TestSize.Level1)
{
    if (!CheckSoFunc()) {
        return;
    }
    audioDec_ = OH_AudioCodec_CreateByMime(AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_OPUS.data(), false);
    EXPECT_NE(nullptr, audioDec_);
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Opus_CreateByName_01, TestSize.Level1)
{
    if (!CheckSoFunc()) {
        return;
    }
    audioDec_ = OH_AudioCodec_CreateByName((AVCodecCodecName::AUDIO_DECODER_OPUS_NAME).data());
    EXPECT_NE(nullptr, audioDec_);
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Opus_Configure_01, TestSize.Level1)
{
    if (!CheckSoFunc()) {
        return;
    }
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_OPUS));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_OPUS));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Opus_SetParameter_01, TestSize.Level1)
{
    if (!CheckSoFunc()) {
        return;
    }
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_OPUS));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_OPUS));
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_SetParameter(audioDec_, format_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Opus_Stop_01, TestSize.Level1)
{
    if (!CheckSoFunc()) {
        return;
    }
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_OPUS));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_OPUS));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_OPUS));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    sleep(1);

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Stop());
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Opus_Flush_01, TestSize.Level1)
{
    if (!CheckSoFunc()) {
        return;
    }
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_OPUS));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_OPUS));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_OPUS));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Flush(audioDec_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Opus_Reset_01, TestSize.Level1)
{
    if (!CheckSoFunc()) {
        return;
    }
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_OPUS));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_OPUS));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioDec_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Opus_GetOutputFormat_01, TestSize.Level1)
{
    if (!CheckSoFunc()) {
        return;
    }
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_OPUS));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_OPUS));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_OPUS));

    EXPECT_NE(nullptr, OH_AudioCodec_GetOutputDescription(audioDec_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Opus_IsValid_01, TestSize.Level1)
{
    if (!CheckSoFunc()) {
        return;
    }
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_OPUS));
    bool isValid = false;
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_IsValid(audioDec_, &isValid));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Opus_Prepare_01, TestSize.Level1)
{
    if (!CheckSoFunc()) {
        return;
    }
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_OPUS));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_OPUS));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Prepare(audioDec_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Ape_CreateByMime_01, TestSize.Level1)
{
    audioDec_ = OH_AudioCodec_CreateByMime(AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_APE.data(), false);
    EXPECT_NE(nullptr, audioDec_);
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Ape_CreateByName_01, TestSize.Level1)
{
    audioDec_ = OH_AudioCodec_CreateByName((AVCodecCodecName::AUDIO_DECODER_APE_NAME).data());
    EXPECT_NE(nullptr, audioDec_);
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_APE_Configure_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_APE));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_APE));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Ape_SetParameter_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_APE));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_APE));
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_SetParameter(audioDec_, format_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Ape_SetParameter_02, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_APE));
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), 1);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), 0);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(),
                                AudioSampleFormat::SAMPLE_S16LE);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_SetParameter(audioDec_, format_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Ape_SetParameter_03, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_APE));
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), 1);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), AMRWB_SAMPLE_RATE);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(),
                                AudioSampleFormat::SAMPLE_U8);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_SetParameter(audioDec_, format_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Ape_SetParameter_04, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_APE));
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), 1);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), AMRWB_SAMPLE_RATE);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(),
                                AudioSampleFormat::SAMPLE_S32LE);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_SetParameter(audioDec_, format_));
    Release();
}
HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Ape_Stop_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_APE));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_APE));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_APE));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    sleep(1);

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Stop());
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Ape_Flush_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_APE));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_APE));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_APE));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Flush(audioDec_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Ape_Reset_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_APE));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_APE));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Reset(audioDec_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Ape_GetOutputFormat_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(AudioBufferFormatType::TYPE_APE));
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_APE));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_APE));

    EXPECT_NE(nullptr, OH_AudioCodec_GetOutputDescription(audioDec_));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Ape_IsValid_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_APE));
    bool isValid = false;
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_IsValid(audioDec_, &isValid));
    Release();
}

HWTEST_F(AudioDecoderBufferCapiUnitTest, audioDecoder_Ape_Prepare_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, CreateCodecFunc(AudioBufferFormatType::TYPE_APE));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure(AudioBufferFormatType::TYPE_APE));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioCodec_Prepare(audioDec_));
    Release();
}
}
}