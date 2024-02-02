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

#include "avcodec_audio_decoder_demo.h"
#include <iostream>
#include <unistd.h>
#include <chrono>
#include "avcodec_codec_name.h"
#include "avcodec_common.h"
#include "avcodec_errors.h"
#include "demo_log.h"
#include "media_description.h"
#include "native_avcodec_base.h"
#include "native_avformat.h"
#include "securec.h"

using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::MediaAVCodec::AudioDemo;
using namespace std;
namespace {
constexpr uint32_t CHANNEL_COUNT = 2;
constexpr uint32_t SAMPLE_RATE = 44100;
constexpr uint32_t DEFAULT_AAC_TYPE = 1;
constexpr int64_t BITS_RATE[TYPE_MAX] = {199000, 261000, 60000, 320000};
constexpr uint32_t AMRWB_SAMPLE_RATE = 16000;
constexpr uint32_t AMRNB_SAMPLE_RATE = 8000;
constexpr uint32_t OPUS_SAMPLE_RATE = 48000;
constexpr string_view INPUT_AAC_FILE_PATH = "/data/test/media/aac_2c_44100hz_199k.dat";
constexpr string_view OUTPUT_AAC_PCM_FILE_PATH = "/data/test/media/aac_2c_44100hz_199k.pcm";
constexpr string_view INPUT_FLAC_FILE_PATH = "/data/test/media/flac_2c_44100hz_261k.dat";
constexpr string_view OUTPUT_FLAC_PCM_FILE_PATH = "/data/test/media/flac_2c_44100hz_261k.pcm";
constexpr string_view INPUT_MP3_FILE_PATH = "/data/test/media/mp3_2c_44100hz_60k.dat";
constexpr string_view OUTPUT_MP3_PCM_FILE_PATH = "/data/test/media/mp3_2c_44100hz_60k.pcm";
constexpr string_view INPUT_VORBIS_FILE_PATH = "/data/test/media/vorbis_2c_44100hz_320k.dat";
constexpr string_view OUTPUT_VORBIS_PCM_FILE_PATH = "/data/test/media/vorbis_2c_44100hz_320k.pcm";
constexpr string_view INPUT_AMRNB_FILE_PATH = "/data/test/media/voice_amrnb_10200.dat";
constexpr string_view OUTPUT_AMRNB_PCM_FILE_PATH = "/data/test/media/voice_amrnb_10200.pcm";
constexpr string_view INPUT_AMRWB_FILE_PATH = "/data/test/media/voice_amrwb_23850.dat";
constexpr string_view OUTPUT_AMRWB_PCM_FILE_PATH = "/data/test/media/voice_amrwb_23850.pcm";
constexpr string_view INPUT_OPUS_FILE_PATH = "/data/test/media/opus_48000.dat";
constexpr string_view OUTPUT_OPUS_PCM_FILE_PATH = "/data/test/media/opus_48000.pcm";
constexpr string_view INPUT_G711MU_FILE_PATH = "/data/test/media/g711mu_8kHz.dat";
constexpr string_view OUTPUT_G711MU_PCM_FILE_PATH = "/data/test/media/g711mu_8kHz_decode.pcm";
} // namespace

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

bool ADecDemo::InitFile(AudioFormatType audioType)
{
    if (audioType == TYPE_AAC) {
        inputFile_.open(INPUT_AAC_FILE_PATH, std::ios::binary);
        pcmOutputFile_.open(OUTPUT_AAC_PCM_FILE_PATH.data(), std::ios::out | std::ios::binary);
    } else if (audioType == TYPE_FLAC) {
        inputFile_.open(INPUT_FLAC_FILE_PATH, std::ios::binary);
        pcmOutputFile_.open(OUTPUT_FLAC_PCM_FILE_PATH.data(), std::ios::out | std::ios::binary);
    } else if (audioType == TYPE_MP3) {
        inputFile_.open(INPUT_MP3_FILE_PATH, std::ios::binary);
        pcmOutputFile_.open(OUTPUT_MP3_PCM_FILE_PATH.data(), std::ios::out | std::ios::binary);
    } else if (audioType == TYPE_VORBIS) {
        inputFile_.open(INPUT_VORBIS_FILE_PATH, std::ios::binary);
        pcmOutputFile_.open(OUTPUT_VORBIS_PCM_FILE_PATH.data(), std::ios::out | std::ios::binary);
    } else if (audioType == TYPE_AMRNB) {
        inputFile_.open(INPUT_AMRNB_FILE_PATH, std::ios::binary);
        pcmOutputFile_.open(OUTPUT_AMRNB_PCM_FILE_PATH.data(), std::ios::out | std::ios::binary);
    } else if (audioType == TYPE_AMRWB) {
        inputFile_.open(INPUT_AMRWB_FILE_PATH, std::ios::binary);
        pcmOutputFile_.open(OUTPUT_AMRWB_PCM_FILE_PATH.data(), std::ios::out | std::ios::binary);
    } else if (audioType == TYPE_OPUS) {
        inputFile_.open(INPUT_OPUS_FILE_PATH, std::ios::binary);
        pcmOutputFile_.open(OUTPUT_OPUS_PCM_FILE_PATH.data(), std::ios::out | std::ios::binary);
    } else if (audioType == TYPE_G711MU) {
        inputFile_.open(INPUT_G711MU_FILE_PATH, std::ios::binary);
        pcmOutputFile_.open(OUTPUT_G711MU_PCM_FILE_PATH.data(), std::ios::out | std::ios::binary);
    } else {
        std::cout << "audio format type not support\n";
        return false;
    }
    DEMO_CHECK_AND_RETURN_RET_LOG(inputFile_.is_open(), false, "Fatal: open input file failed");
    DEMO_CHECK_AND_RETURN_RET_LOG(pcmOutputFile_.is_open(), false, "Fatal: open output file failed");
    return true;
}

void ADecDemo::RunCase(AudioFormatType audioType)
{
    DEMO_CHECK_AND_RETURN_LOG(InitFile(audioType), "Fatal: InitFile file failed");
    audioType_ = audioType;
    DEMO_CHECK_AND_RETURN_LOG(CreateDec() == AVCS_ERR_OK, "Fatal: CreateDec fail");
    OH_AVFormat *format = OH_AVFormat_Create();
    int32_t channelCount = 1;
    int32_t sampleRate = SAMPLE_RATE;
    if (audioType == TYPE_AAC) {
        channelCount = CHANNEL_COUNT;
        OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AAC_IS_ADTS.data(), DEFAULT_AAC_TYPE);
        OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(),
                                OH_BitsPerSample::SAMPLE_S16LE);
    } else if (audioType == TYPE_AMRNB || audioType == TYPE_G711MU) {
        sampleRate = AMRNB_SAMPLE_RATE;
    } else if (audioType == TYPE_AMRWB) {
        sampleRate = AMRWB_SAMPLE_RATE;
        OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(),
                                OH_BitsPerSample::SAMPLE_S16LE);
    } else if (audioType == TYPE_OPUS) {
        sampleRate = OPUS_SAMPLE_RATE;
    }
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), channelCount);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), sampleRate);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_BITRATE.data(), BITS_RATE[audioType]);
    if (audioType == TYPE_VORBIS) {
        OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(),
                                OH_BitsPerSample::SAMPLE_S16LE);
        int64_t extradataSize;
        DEMO_CHECK_AND_RETURN_LOG(inputFile_.is_open(), "Fatal: file is not open");
        inputFile_.read(reinterpret_cast<char *>(&extradataSize), sizeof(int64_t));
        DEMO_CHECK_AND_RETURN_LOG(inputFile_.gcount() == sizeof(int64_t), "Fatal: read extradataSize bytes error");
        if (extradataSize < 0) {
            return;
        }
        char buffer[extradataSize];
        inputFile_.read(buffer, extradataSize);
        DEMO_CHECK_AND_RETURN_LOG(inputFile_.gcount() == extradataSize, "Fatal: read extradata bytes error");
        OH_AVFormat_SetBuffer(format, MediaDescriptionKey::MD_KEY_CODEC_CONFIG.data(), (uint8_t *)buffer,
                              extradataSize);
    }
    DEMO_CHECK_AND_RETURN_LOG(Configure(format) == AVCS_ERR_OK, "Fatal: Configure fail");
    DEMO_CHECK_AND_RETURN_LOG(Start() == AVCS_ERR_OK, "Fatal: Start fail");

    unique_lock<mutex> lock(signal_->startMutex_);
    signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });

    DEMO_CHECK_AND_RETURN_LOG(Stop() == AVCS_ERR_OK, "Fatal: Stop fail");
    DEMO_CHECK_AND_RETURN_LOG(Release() == AVCS_ERR_OK, "Fatal: Release fail");
}

ADecDemo::ADecDemo() : audioDec_(nullptr), signal_(nullptr), audioType_(TYPE_AAC) {}

ADecDemo::~ADecDemo()
{
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

int32_t ADecDemo::CreateDec()
{
    if (audioType_ == TYPE_AAC) {
        audioDec_ = OH_AudioDecoder_CreateByName((AVCodecCodecName::AUDIO_DECODER_AAC_NAME).data());
    } else if (audioType_ == TYPE_FLAC) {
        audioDec_ = OH_AudioDecoder_CreateByName((AVCodecCodecName::AUDIO_DECODER_FLAC_NAME).data());
    } else if (audioType_ == TYPE_MP3) {
        audioDec_ = OH_AudioDecoder_CreateByName((AVCodecCodecName::AUDIO_DECODER_MP3_NAME).data());
    } else if (audioType_ == TYPE_VORBIS) {
        audioDec_ = OH_AudioDecoder_CreateByName((AVCodecCodecName::AUDIO_DECODER_VORBIS_NAME).data());
    } else if (audioType_ == TYPE_AMRNB) {
        audioDec_ = OH_AudioDecoder_CreateByName((AVCodecCodecName::AUDIO_DECODER_AMRNB_NAME).data());
    } else if (audioType_ == TYPE_AMRWB) {
        audioDec_ = OH_AudioDecoder_CreateByName((AVCodecCodecName::AUDIO_DECODER_AMRWB_NAME).data());
    } else if (audioType_ == TYPE_OPUS) {
        audioDec_ = OH_AudioDecoder_CreateByName((AVCodecCodecName::AUDIO_DECODER_OPUS_NAME).data());
    } else if (audioType_ == TYPE_G711MU) {
        audioDec_ = OH_AudioDecoder_CreateByName((AVCodecCodecName::AUDIO_DECODER_G711MU_NAME).data());
    } else {
        return AVCS_ERR_INVALID_VAL;
    }
    DEMO_CHECK_AND_RETURN_RET_LOG(audioDec_ != nullptr, AVCS_ERR_UNKNOWN, "Fatal: CreateByName fail");

    signal_ = new ADecSignal();
    DEMO_CHECK_AND_RETURN_RET_LOG(signal_ != nullptr, AVCS_ERR_UNKNOWN, "Fatal: No memory");

    cb_ = {&OnError, &OnOutputFormatChanged, &OnInputBufferAvailable, &OnOutputBufferAvailable};
    int32_t ret = OH_AudioDecoder_SetCallback(audioDec_, cb_, signal_);
    DEMO_CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_UNKNOWN, "Fatal: SetCallback fail");

    return AVCS_ERR_OK;
}

int32_t ADecDemo::Configure(OH_AVFormat *format)
{
    return OH_AudioDecoder_Configure(audioDec_, format);
}

int32_t ADecDemo::Start()
{
    isRunning_.store(true);

    inputLoop_ = make_unique<thread>(&ADecDemo::InputFunc, this);
    DEMO_CHECK_AND_RETURN_RET_LOG(inputLoop_ != nullptr, AVCS_ERR_UNKNOWN, "Fatal: No memory");

    outputLoop_ = make_unique<thread>(&ADecDemo::OutputFunc, this);
    DEMO_CHECK_AND_RETURN_RET_LOG(outputLoop_ != nullptr, AVCS_ERR_UNKNOWN, "Fatal: No memory");

    return OH_AudioDecoder_Start(audioDec_);
}

int32_t ADecDemo::Stop()
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
        while (!signal_->attrQueue_.empty()) {
            signal_->attrQueue_.pop();
        }
        while (!signal_->outBufferQueue_.empty()) {
            signal_->outBufferQueue_.pop();
        }
    }
    std::cout << "start stop!\n";
    return OH_AudioDecoder_Stop(audioDec_);
}

int32_t ADecDemo::Flush()
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
        std::cout << "clear input buffer!\n";
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
        while (!signal_->attrQueue_.empty()) {
            signal_->attrQueue_.pop();
        }
        while (!signal_->outBufferQueue_.empty()) {
            signal_->outBufferQueue_.pop();
        }
        std::cout << "clear output buffer!\n";
    }
    return OH_AudioDecoder_Flush(audioDec_);
}

int32_t ADecDemo::Reset()
{
    return OH_AudioDecoder_Reset(audioDec_);
}

int32_t ADecDemo::Release()
{
    return OH_AudioDecoder_Destroy(audioDec_);
}

void ADecDemo::HandleInputEOS(const uint32_t index)
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

int32_t ADecDemo::HandleNormalInput(const uint32_t &index, const int64_t pts, const size_t size)
{
    OH_AVCodecBufferAttr info;
    info.size = size;
    info.offset = 0;
    info.pts = pts;

    int32_t ret = AVCS_ERR_OK;
    if (isFirstFrame_) {
        info.flags = AVCODEC_BUFFER_FLAGS_CODEC_DATA;
        ret = OH_AudioDecoder_PushInputData(audioDec_, index, info);
        isFirstFrame_ = false;
    } else {
        info.flags = AVCODEC_BUFFER_FLAGS_NONE;
        ret = OH_AudioDecoder_PushInputData(audioDec_, index, info);
    }
    signal_->inQueue_.pop();
    signal_->inBufferQueue_.pop();
    frameCount_++;
    return ret;
}

void ADecDemo::InputFunc()
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
        DEMO_CHECK_AND_BREAK_LOG(buffer != nullptr, "Fatal: GetInputBuffer fail");
        inputFile_.read(reinterpret_cast<char *>(&size), sizeof(size));
        if (inputFile_.eof() || inputFile_.gcount() == 0 || size == 0) {
            HandleInputEOS(index);
            std::cout << "end buffer\n";
            break;
        }
        DEMO_CHECK_AND_BREAK_LOG(inputFile_.gcount() == sizeof(size), "Fatal: read size fail");
        inputFile_.read(reinterpret_cast<char *>(&pts), sizeof(pts));
        DEMO_CHECK_AND_BREAK_LOG(inputFile_.gcount() == sizeof(pts), "Fatal: read pts fail");
        inputFile_.read((char *)OH_AVMemory_GetAddr(buffer), size);
        DEMO_CHECK_AND_BREAK_LOG(inputFile_.gcount() == size, "Fatal: read buffer fail");

        int32_t ret = HandleNormalInput(index, pts, size);
        if (ret != AVCS_ERR_OK) {
            cout << "Fatal error, exit" << endl;
            break;
        }
    }
    inputFile_.close();
}

void ADecDemo::OutputFunc()
{
    DEMO_CHECK_AND_RETURN_LOG(pcmOutputFile_.is_open(), "Fatal: output file failedis not open");
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
        if (attr.flags != AVCODEC_BUFFER_FLAGS_EOS && data != nullptr) {
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
        if (OH_AudioDecoder_FreeOutputData(audioDec_, index) != AV_ERR_OK) {
            cout << "Fatal: FreeOutputData fail" << endl;
            break;
        }
    }
    pcmOutputFile_.close();
}
