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

#include "avcodec_audio_avbuffer_decoder_demo.h"
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
#include "native_avbuffer.h"
#include "native_avmemory.h"
#include "securec.h"
#ifdef SUPPORT_DRM
#include "native_mediakeysession.h"
#include "native_mediakeysystem.h"
#endif

using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::MediaAVCodec::AudioBufferDemo;
using namespace std;
namespace {
constexpr uint32_t CHANNEL_COUNT = 2;
constexpr uint32_t SAMPLE_RATE = 44100;
constexpr uint32_t DEFAULT_AAC_TYPE = 1;
constexpr int64_t BITS_RATE[static_cast<uint32_t>(AudioBufferFormatType::TYPE_MAX)] = {199000, 261000, 60000, 320000};
constexpr uint32_t AMRWB_SAMPLE_RATE = 16000;
constexpr uint32_t AMRNB_SAMPLE_RATE = 8000;
constexpr string_view INPUT_AAC_FILE_PATH = "/data/test/media/aac_2c_44100hz_199k.dat";
constexpr string_view OUTPUT_AAC_PCM_FILE_PATH = "/data/test/media/aac_2c_44100hz_199k_dec.pcm";
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
constexpr string_view INPUT_G711MU_FILE_PATH = "/data/test/media/g711mu_8kHz.dat";
constexpr string_view OUTPUT_G711MU_PCM_FILE_PATH = "/data/test/media/g711mu_8kHz_decode.pcm";
constexpr string_view INPUT_APE_FILE_PATH = "/data/test/media/ape.dat";
constexpr string_view OUTPUT_APE_PCM_FILE_PATH = "/data/test/media/ape_decode.pcm";
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

bool ADecBufferDemo::InitFile(AudioBufferFormatType audioType)
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
    } else if (audioType == AudioBufferFormatType::TYPE_APE) {
        inputFile_.open(INPUT_APE_FILE_PATH, std::ios::binary);
        pcmOutputFile_.open(OUTPUT_APE_PCM_FILE_PATH.data(), std::ios::out | std::ios::binary);
    } else {
        std::cout << "audio format type not support\n";
        return false;
    }
    DEMO_CHECK_AND_RETURN_RET_LOG(inputFile_.is_open(), false, "Fatal: open input file failed");
    DEMO_CHECK_AND_RETURN_RET_LOG(pcmOutputFile_.is_open(), false, "Fatal: open output file failed");
    return true;
}

void ADecBufferDemo::RunCase(AudioBufferFormatType audioType)
{
    DEMO_CHECK_AND_RETURN_LOG(InitFile(audioType), "Fatal: InitFile file failed");
    audioType_ = audioType;
    DEMO_CHECK_AND_RETURN_LOG(CreateDec() == AVCS_ERR_OK, "Fatal: CreateDec fail");

    OH_AVFormat *format = OH_AVFormat_Create();
    int32_t channelCount = CHANNEL_COUNT;
    int32_t sampleRate = SAMPLE_RATE;
    if (audioType == AudioBufferFormatType::TYPE_AAC) {
        OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AAC_IS_ADTS.data(), DEFAULT_AAC_TYPE);
        OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(),
                                OH_BitsPerSample::SAMPLE_S16LE);
    } else if (audioType == AudioBufferFormatType::TYPE_AMRNB || audioType == AudioBufferFormatType::TYPE_G711MU) {
        channelCount = 1;
        sampleRate = AMRNB_SAMPLE_RATE;
    } else if (audioType == AudioBufferFormatType::TYPE_AMRWB || audioType == AudioBufferFormatType::TYPE_APE) {
        channelCount = 1;
        sampleRate = AMRWB_SAMPLE_RATE;
        OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(),
                                OH_BitsPerSample::SAMPLE_S16LE);
    }
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), channelCount);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), sampleRate);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_BITRATE.data(), BITS_RATE[(uint32_t)audioType]);
    if (audioType == AudioBufferFormatType::TYPE_VORBIS) {
        OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(),
                                OH_BitsPerSample::SAMPLE_S16LE);
        // extradata for vorbis
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
        OH_AVFormat_SetBuffer(format, MediaDescriptionKey::MD_KEY_CODEC_CONFIG.data(),
                              reinterpret_cast<uint8_t *>(buffer), extradataSize);
    }
    DEMO_CHECK_AND_RETURN_LOG(Configure(format) == AVCS_ERR_OK, "Fatal: Configure fail");
    DEMO_CHECK_AND_RETURN_LOG(Start() == AVCS_ERR_OK, "Fatal: Start fail");

    auto start = chrono::steady_clock::now();

    unique_lock<mutex> lock(signal_->startMutex_);
    signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });

    auto end = chrono::steady_clock::now();
    std::cout << "Decode finished, time = " << std::chrono::duration_cast<chrono::milliseconds>(end - start).count()
              << " ms" << std::endl;

    DEMO_CHECK_AND_RETURN_LOG(Stop() == AVCS_ERR_OK, "Fatal: Stop fail");
    DEMO_CHECK_AND_RETURN_LOG(Release() == AVCS_ERR_OK, "Fatal: Release fail");
}

void ADecBufferDemo::RunDrmCase(AudioBufferFormatType audioType)
{
    std::cout << "ADecBufferDemo::RunDrmCase" <<std::endl;
#ifdef SUPPORT_DRM
    audioType_ = audioType;
    DEMO_CHECK_AND_RETURN_LOG(CreateDec() == AVCS_ERR_OK, "Fatal: CreateDec fail");

    // test 1:create mediakeysystem
    std::cout << "Test OH_MediaKeySystem_Create" << std::endl;
    MediaKeySystem *system = NULL;
    uint32_t errNo = OH_MediaKeySystem_Create("com.clearplay.drm", &system);
    std::cout << "Test OH_MediaKeySystem_Create result" << system <<"and ret" << errNo << std::endl;

    // test 2:create mediakeysystem
    std::cout << "Test OH_MediaKeySystem_CreateMediaKeySession" <<std::endl;
    DRM_ContentProtectionLevel contentProtectionLevel = CONTENT_PROTECTION_LEVEL_SW_CRYPTO;
    MediaKeySession *session = NULL;
    errNo = OH_MediaKeySystem_CreateMediaKeySession(system, &contentProtectionLevel, &session);
    std::cout <<"Test OH_MediaKeySystem_CreateMediaKeySession result" << session << "and ret" << errNo << std::endl;

    // test 3:SetDecryptConfigTest
    std::cout <<"Test OH_AudioCodec_SetDecryptionConfig"<<std::endl;
    errNo = OH_AudioCodec_SetDecryptionConfig(audioDec_, session, false);
    std::cout <<"Test OH_AudioCodec_SetDecryptionConfig result"<<session<<"and ret"<<errNo<<std::endl;
#endif
}

ADecBufferDemo::ADecBufferDemo() : audioDec_(nullptr), signal_(nullptr), audioType_(AudioBufferFormatType::TYPE_AAC) {}

ADecBufferDemo::~ADecBufferDemo()
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

int32_t ADecBufferDemo::CreateDec()
{
    if (audioType_ == AudioBufferFormatType::TYPE_AAC) {
        audioDec_ = OH_AudioCodec_CreateByName((AVCodecCodecName::AUDIO_DECODER_AAC_NAME).data());
    } else if (audioType_ == AudioBufferFormatType::TYPE_FLAC) {
        audioDec_ = OH_AudioCodec_CreateByName((AVCodecCodecName::AUDIO_DECODER_FLAC_NAME).data());
    } else if (audioType_ == AudioBufferFormatType::TYPE_MP3) {
        audioDec_ = OH_AudioCodec_CreateByName((AVCodecCodecName::AUDIO_DECODER_MP3_NAME).data());
    } else if (audioType_ == AudioBufferFormatType::TYPE_VORBIS) {
        audioDec_ = OH_AudioCodec_CreateByName((AVCodecCodecName::AUDIO_DECODER_VORBIS_NAME).data());
    } else if (audioType_ == AudioBufferFormatType::TYPE_AMRNB) {
        audioDec_ = OH_AudioCodec_CreateByName((AVCodecCodecName::AUDIO_DECODER_AMRNB_NAME).data());
    } else if (audioType_ == AudioBufferFormatType::TYPE_AMRWB) {
        audioDec_ = OH_AudioCodec_CreateByName((AVCodecCodecName::AUDIO_DECODER_AMRWB_NAME).data());
    } else if (audioType_ == AudioBufferFormatType::TYPE_G711MU) {
        audioDec_ = OH_AudioCodec_CreateByName((AVCodecCodecName::AUDIO_DECODER_G711MU_NAME).data());
    } else if (audioType_ == AudioBufferFormatType::TYPE_APE) {
        audioDec_ = OH_AudioCodec_CreateByName((AVCodecCodecName::AUDIO_DECODER_APE_NAME).data());
    } else {
        return AVCS_ERR_INVALID_VAL;
    }
    DEMO_CHECK_AND_RETURN_RET_LOG(audioDec_ != nullptr, AVCS_ERR_UNKNOWN, "Fatal: CreateByName fail");

    signal_ = new ADecBufferSignal();
    DEMO_CHECK_AND_RETURN_RET_LOG(signal_ != nullptr, AVCS_ERR_UNKNOWN, "Fatal: No memory");

    cb_ = {&OnError, &OnOutputFormatChanged, &OnInputBufferAvailable, &OnOutputBufferAvailable};
    int32_t ret = OH_AudioCodec_RegisterCallback(audioDec_, cb_, signal_);
    DEMO_CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_UNKNOWN, "Fatal: SetCallback fail");

    return AVCS_ERR_OK;
}

int32_t ADecBufferDemo::Configure(OH_AVFormat *format)
{
    return OH_AudioCodec_Configure(audioDec_, format);
}

int32_t ADecBufferDemo::Start()
{
    isRunning_.store(true);

    inputLoop_ = make_unique<thread>(&ADecBufferDemo::InputFunc, this);
    DEMO_CHECK_AND_RETURN_RET_LOG(inputLoop_ != nullptr, AVCS_ERR_UNKNOWN, "Fatal: No memory");

    outputLoop_ = make_unique<thread>(&ADecBufferDemo::OutputFunc, this);
    DEMO_CHECK_AND_RETURN_RET_LOG(outputLoop_ != nullptr, AVCS_ERR_UNKNOWN, "Fatal: No memory");

    return OH_AudioCodec_Start(audioDec_);
}

int32_t ADecBufferDemo::Stop()
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

int32_t ADecBufferDemo::Flush()
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
        while (!signal_->outBufferQueue_.empty()) {
            signal_->outBufferQueue_.pop();
        }
        std::cout << "clear output buffer!\n";
    }
    return OH_AudioCodec_Flush(audioDec_);
}

int32_t ADecBufferDemo::Reset()
{
    return OH_AudioCodec_Reset(audioDec_);
}

int32_t ADecBufferDemo::Release()
{
    return OH_AudioCodec_Destroy(audioDec_);
}

void ADecBufferDemo::HandleInputEOS(const uint32_t index)
{
    std::cout << "end buffer\n";
    OH_AudioCodec_PushInputBuffer(audioDec_, index);
    signal_->inBufferQueue_.pop();
    signal_->inQueue_.pop();
}

void ADecBufferDemo::InputFunc()
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
            buffer->buffer_->memory_->SetSize(1);
            buffer->buffer_->flag_ = AVCODEC_BUFFER_FLAGS_EOS;
            HandleInputEOS(index);
            cout << "Set EOS" << endl;
            break;
        }
        DEMO_CHECK_AND_BREAK_LOG(inputFile_.gcount() == sizeof(size), "Fatal: read size fail");
        inputFile_.read(reinterpret_cast<char *>(&buffer->buffer_->pts_), sizeof(buffer->buffer_->pts_));
        DEMO_CHECK_AND_BREAK_LOG(inputFile_.gcount() == sizeof(pts), "Fatal: read pts fail");
        inputFile_.read(reinterpret_cast<char *>(OH_AVBuffer_GetAddr(buffer)), size);
        buffer->buffer_->memory_->SetSize(size);
        DEMO_CHECK_AND_BREAK_LOG(inputFile_.gcount() == size, "Fatal: read buffer fail");

        cout << "SetSize" << size << endl;
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
            cout << "Fatal error, exit " << ret <<endl;
            break;
        }
    }
    cout << "stop, exit" << endl;
    inputFile_.close();
}

void ADecBufferDemo::OutputFunc()
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
        OH_AVBuffer *data = signal_->outBufferQueue_.front();
        if (data == nullptr) {
            cout << "OutputFunc OH_AVBuffer is nullptr" << endl;
            continue;
        }
        pcmOutputFile_.write(reinterpret_cast<char *>(OH_AVBuffer_GetAddr(data)), data->buffer_->memory_->GetSize());

        if (data->buffer_->flag_ == AVCODEC_BUFFER_FLAGS_EOS || data->buffer_->memory_->GetSize() == 0) {
            cout << "decode eos 1" << endl;
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
            cout << "decode eos 2" << endl;
            isRunning_.store(false);
            signal_->startCond_.notify_all();
        }
    }
    cout << "stop, exit" << endl;
    pcmOutputFile_.close();
}
