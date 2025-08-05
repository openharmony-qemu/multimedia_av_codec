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

#include <iostream>
#include <unistd.h>
#include <chrono>
#include "securec.h"
#include "avcodec_common.h"
#include "avcodec_errors.h"
#include "media_description.h"
#include "native_avformat.h"
#include "demo_log.h"
#include "avcodec_codec_name.h"
#include "native_avmemory.h"
#include "native_avbuffer.h"
#include "ffmpeg_converter.h"
#include "audio_encoder_demo.h"

using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::MediaAVCodec::AudioAacEncDemo;
using namespace std;
namespace {
constexpr uint32_t SAMPLE_RATE_44100 = 44100;
constexpr uint32_t BIT_RATE_128000 = 128000;
constexpr int32_t SAMPLE_FORMAT_S16 = AudioSampleFormat::SAMPLE_S16LE;
constexpr int32_t CHANNEL_1 = 1;
constexpr int32_t CHANNEL_2 = 2;
constexpr int32_t CHANNEL_3 = 3;
constexpr int32_t CHANNEL_4 = 4;
constexpr int32_t CHANNEL_5 = 5;
constexpr int32_t CHANNEL_6 = 6;
constexpr int32_t CHANNEL_7 = 7;
constexpr int32_t CHANNEL_8 = 8;
} // namespace


static uint64_t GetChannelLayout(int32_t channel)
{
    switch (channel) {
        case CHANNEL_1:
            return MONO;
        case CHANNEL_2:
            return STEREO;
        case CHANNEL_3:
            return CH_2POINT1;
        case CHANNEL_4:
            return CH_3POINT1;
        case CHANNEL_5:
            return CH_4POINT1;
        case CHANNEL_6:
            return CH_5POINT1;
        case CHANNEL_7:
            return CH_6POINT1;
        case CHANNEL_8:
            return CH_7POINT1;
        default:
            return UNKNOWN_CHANNEL_LAYOUT;
    }
}

static void OnError(OH_AVCodec *codec, int32_t errorCode, void *userData)
{
    (void)codec;
    (void)errorCode;
    (void)userData;
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
    AEncSignal *signal = static_cast<AEncSignal *>(userData);
    unique_lock<mutex> lock(signal->inMutex_);
    signal->inQueue_.push(index);
    signal->inBufferQueue_.push(buffer);
    signal->inCond_.notify_all();
}

static void OnOutputBufferAvailable(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData)
{
    (void)codec;
    AEncSignal *signal = static_cast<AEncSignal *>(userData);
    unique_lock<mutex> lock(signal->outMutex_);
    signal->outQueue_.push(index);
    signal->outBufferQueue_.push(buffer);
    if (buffer) {
        cout << "OnOutputBufferAvailable received, index:" << index << ", size:" << buffer->buffer_->memory_->GetSize()
             << ", flags:" << buffer->buffer_->flag_ << ", pts: " << buffer->buffer_->pts_ << endl;
    }
    signal->outCond_.notify_all();
}

bool AudioBufferAacEncDemo::InitFile(const std::string& inputFile)
{
    if (inputFile.find("mp4") != std::string::npos || inputFile.find("m4a") != std::string::npos ||
        inputFile.find("vivid") != std::string::npos) {
        audioType_ = AudioBufferFormatType::TYPE_VIVID;
    } else if (inputFile.find("opus") != std::string::npos) {
        audioType_ = AudioBufferFormatType::TYPE_OPUS;
    } else if (inputFile.find("g711") != std::string::npos) {
        audioType_ = AudioBufferFormatType::TYPE_G711MU;
    } else if (inputFile.find("lbvc") != std::string::npos) {
        audioType_ = AudioBufferFormatType::TYPE_LBVC;
    } else if (inputFile.find("flac") != std::string::npos) {
        audioType_ = AudioBufferFormatType::TYPE_FLAC;
    } else if (inputFile.find("amrnb") != std::string::npos) {
        audioType_ = AudioBufferFormatType::TYPE_AMRNB;
    } else if (inputFile.find("amrwb") != std::string::npos) {
        audioType_ = AudioBufferFormatType::TYPE_AMRWB;
    } else if (inputFile.find("mp3") != std::string::npos) {
        audioType_ = AudioBufferFormatType::TYPE_MP3;
    } else {
        audioType_ = AudioBufferFormatType::TYPE_AAC;
    }

    return true;
}

bool AudioBufferAacEncDemo::RunCase(const uint8_t *data, size_t size)
{
    std::string codecdata(reinterpret_cast<const char *>(data), size);
    inputdata = codecdata;
    inputdatasize = size;
    DEMO_CHECK_AND_RETURN_RET_LOG(CreateEnc() == AVCS_ERR_OK, false, "Fatal: CreateEnc fail");
    OH_AVFormat *format = OH_AVFormat_Create();
    Setformat(format);
    DEMO_CHECK_AND_RETURN_RET_LOG(Configure(format) == AVCS_ERR_OK, false, "Fatal: Configure fail");
    DEMO_CHECK_AND_RETURN_RET_LOG(Start() == AVCS_ERR_OK, false, "Fatal: Start fail");
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    DEMO_CHECK_AND_RETURN_RET_LOG(Stop() == AVCS_ERR_OK, false, "Fatal: Stop fail");
    DEMO_CHECK_AND_RETURN_RET_LOG(Release() == AVCS_ERR_OK, false, "Fatal: Release fail");
    OH_AVFormat_Destroy(format);
    sleep(1);
    return true;
}

void AudioBufferAacEncDemo::Setformat(OH_AVFormat *format)
{
    uint64_t channelLayout;
    long bitrate = BIT_RATE_128000;
    int32_t sampleRate = SAMPLE_RATE_44100;
    int32_t channelCount = CHANNEL_2;
    int32_t sampleFormat = SAMPLE_FORMAT_S16;
    channelLayout = GetChannelLayout(channelCount);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_CHANNEL_LAYOUT, channelLayout);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_PROFILE, AAC_PROFILE_HE);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_BITRATE.data(), bitrate);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), sampleRate);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), channelCount);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(), sampleFormat);
}

AudioBufferAacEncDemo::AudioBufferAacEncDemo() : isRunning_(false), audioEnc_(nullptr), signal_(nullptr), frameCount_(0)
{
    signal_ = new AEncSignal();
    DEMO_CHECK_AND_RETURN_LOG(signal_ != nullptr, "Fatal: No memory");
}

AudioBufferAacEncDemo::~AudioBufferAacEncDemo()
{
    if (signal_) {
        delete signal_;
        signal_ = nullptr;
    }
}

int32_t AudioBufferAacEncDemo::CreateEnc()
{
    audioEnc_ = OH_AudioCodec_CreateByMime(OH_AVCODEC_MIMETYPE_AUDIO_AAC, true);
    DEMO_CHECK_AND_RETURN_RET_LOG(audioEnc_ != nullptr, AVCS_ERR_UNKNOWN, "Fatal: CreateByName fail");
    if (signal_ == nullptr) {
        signal_ = new AEncSignal();
        DEMO_CHECK_AND_RETURN_RET_LOG(signal_ != nullptr, AVCS_ERR_UNKNOWN, "Fatal: No memory");
    }
    cb_ = {&OnError, &OnOutputFormatChanged, &OnInputBufferAvailable, &OnOutputBufferAvailable};
    int32_t ret = OH_AudioCodec_RegisterCallback(audioEnc_, cb_, signal_);
    DEMO_CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_UNKNOWN, "Fatal: SetCallback fail");

    return AVCS_ERR_OK;
}

int32_t AudioBufferAacEncDemo::Configure(OH_AVFormat *format)
{
    int32_t ret = OH_AudioCodec_Configure(audioEnc_, format);
    return ret;
}

int32_t AudioBufferAacEncDemo::Start()
{
    isRunning_.store(true);

    inputLoop_ = make_unique<thread>(&AudioBufferAacEncDemo::InputFunc, this);
    DEMO_CHECK_AND_RETURN_RET_LOG(inputLoop_ != nullptr, AVCS_ERR_UNKNOWN, "Fatal: No memory");

    outputLoop_ = make_unique<thread>(&AudioBufferAacEncDemo::OutputFunc, this);
    DEMO_CHECK_AND_RETURN_RET_LOG(outputLoop_ != nullptr, AVCS_ERR_UNKNOWN, "Fatal: No memory");
    return OH_AudioCodec_Start(audioEnc_);
}

int32_t AudioBufferAacEncDemo::Stop()
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

int32_t AudioBufferAacEncDemo::Flush()
{
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
    return OH_AudioCodec_Flush(audioEnc_);
}

int32_t AudioBufferAacEncDemo::Reset()
{
    return OH_AudioCodec_Reset(audioEnc_);
}

int32_t AudioBufferAacEncDemo::Release()
{
    return OH_AudioCodec_Destroy(audioEnc_);
}

void AudioBufferAacEncDemo::HandleEOS(const uint32_t &index)
{
    OH_AudioCodec_PushInputBuffer(audioEnc_, index);
    signal_->inQueue_.pop();
    signal_->inBufferQueue_.pop();
}

void AudioBufferAacEncDemo::InputFunc()
{
    size_t frameBytes = 1024;
    while (isRunning_.load()) {
        unique_lock<mutex> lock(signal_->inMutex_);
        signal_->inCond_.wait(lock, [this]() { return (signal_->inQueue_.size() > 0 || !isRunning_.load()); });
        if (!isRunning_.load()) {
            break;
        }
        uint32_t index = signal_->inQueue_.front();
        auto buffer = signal_->inBufferQueue_.front();
        DEMO_CHECK_AND_BREAK_LOG(buffer != nullptr, "Fatal: GetInputBuffer fail");
        size_t currentSize = inputdatasize < frameBytes ? inputdatasize : frameBytes;
        if (currentSize == 0) {
            buffer->buffer_->flag_ = AVCODEC_BUFFER_FLAGS_EOS;
            OH_AudioCodec_PushInputBuffer(audioEnc_, index);
            break;
        }
        std::string currentStr = inputdata.substr(inputOffset, currentSize);
        int ret;
        strncpy_s(reinterpret_cast<char*>(OH_AVBuffer_GetAddr(buffer)), currentSize, currentStr.c_str(), currentSize);
        buffer->buffer_->memory_->SetSize(currentSize);
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
        inputdatasize -= currentSize;
        inputOffset += currentSize;
        frameCount_++;
        if (ret != AVCS_ERR_OK) {
            isRunning_.store(false);
            break;
        }
    }
    signal_->outCond_.notify_all();
}

void AudioBufferAacEncDemo::OutputFunc()
{
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
        std::cout << "OutputFunc index:" << index << endl;
        if (avBuffer != nullptr &&
            (avBuffer->buffer_->flag_ == AVCODEC_BUFFER_FLAGS_EOS || avBuffer->buffer_->memory_->GetSize() == 0)) {
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
    }
    signal_->startCond_.notify_all();
    cout << "stop, exit" << endl;
}