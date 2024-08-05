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
#include "audio_encoder_reset_demo.h"

using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::MediaAVCodec::AudioAacEncDemo;
using namespace std;
namespace {
constexpr uint32_t CHANNEL_COUNT_1 = 1;
constexpr uint32_t CHANNEL_COUNT = 2;
constexpr uint32_t BIT_RATE_6000 = 6000;
constexpr uint32_t BIT_RATE_6700 = 6700;
constexpr uint32_t BIT_RATE_8850 = 8850;
constexpr uint32_t BIT_RATE_64000 = 64000;
constexpr uint32_t BIT_RATE_96000 = 96000;
constexpr uint32_t SAMPLE_RATE_8000 = 8000;
constexpr uint32_t SAMPLE_RATE_16000 = 16000;
constexpr uint32_t SAMPLE_RATE = 44100;
constexpr uint32_t FRAME_DURATION_US = 33000;
constexpr int32_t SAMPLE_FORMAT_S16 = AudioSampleFormat::SAMPLE_S16LE;
constexpr int32_t SAMPLE_FORMAT_S32 = AudioSampleFormat::SAMPLE_S32LE;
constexpr int32_t BIT_PER_CODE_COUNT = 16;
constexpr int32_t COMPLEXITY_COUNT = 10;
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

void AudioBufferAacEncDemo::Setformat(OH_AVFormat *format)
{
    int32_t channelCount = CHANNEL_COUNT_1;
    int32_t sampleRate = SAMPLE_RATE;
    long bitrate = BIT_RATE_64000;
    uint64_t channelLayout;
    int32_t sampleFormat = SAMPLE_FORMAT_S16;
    if (audioType_ == AudioBufferFormatType::TYPE_AAC) {
        channelCount = CHANNEL_COUNT;
        sampleRate = SAMPLE_RATE;
        bitrate = BIT_RATE_96000;
        sampleFormat = SAMPLE_FORMAT_S32;
    } else if (audioType_ == AudioBufferFormatType::TYPE_OPUS) {
        sampleRate = SAMPLE_RATE_8000;
        bitrate = BIT_RATE_64000;
        OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE.data(), BIT_PER_CODE_COUNT);
        OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_COMPLIANCE_LEVEL.data(), COMPLEXITY_COUNT);
    } else if (audioType_ == AudioBufferFormatType::TYPE_G711MU) {
        sampleRate = SAMPLE_RATE_8000;
        bitrate = BIT_RATE_64000;
    } else if (audioType_ == AudioBufferFormatType::TYPE_LBVC) {
        sampleRate = SAMPLE_RATE_16000; //采样率16000
        bitrate = BIT_RATE_6000; // 码率 6000
    } else if (audioType_ == AudioBufferFormatType::TYPE_FLAC) {
        channelCount = CHANNEL_COUNT_1;
        sampleRate = SAMPLE_RATE;
        bitrate = BIT_RATE_64000;
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, OH_BitsPerSample::SAMPLE_S16LE);
    } else if (audioType_ == AudioBufferFormatType::TYPE_AMRNB) {
        sampleRate = SAMPLE_RATE_8000;
        bitrate = BIT_RATE_6700;
    } else if (audioType_ == AudioBufferFormatType::TYPE_AMRWB) {
        sampleRate = SAMPLE_RATE_16000;
        bitrate = BIT_RATE_8850;
    } else if (audioType_ == AudioBufferFormatType::TYPE_MP3) {
        sampleRate = SAMPLE_RATE;
        bitrate = BIT_RATE_64000;
    }
    channelLayout = GetChannelLayout(channelCount);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_CHANNEL_LAYOUT, channelLayout);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), channelCount);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), sampleRate);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_BITRATE.data(), bitrate);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(), sampleFormat);
    return;
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
    if (audioType_ == AudioBufferFormatType::TYPE_AAC) {
        audioEnc_ = OH_AudioCodec_CreateByName((AVCodecCodecName::AUDIO_ENCODER_AAC_NAME).data());
        cout << "CreateEnc aac!" << endl;
    } else if (audioType_ == AudioBufferFormatType::TYPE_FLAC) {
        audioEnc_ = OH_AudioCodec_CreateByName((AVCodecCodecName::AUDIO_ENCODER_FLAC_NAME).data());
        cout << "CreateEnc flac!" << endl;
    } else if (audioType_ == AudioBufferFormatType::TYPE_OPUS) {
        audioEnc_ = OH_AudioCodec_CreateByName((AVCodecCodecName::AUDIO_ENCODER_OPUS_NAME).data());
        cout << "CreateEnc opus!" << endl;
    } else if (audioType_ == AudioBufferFormatType::TYPE_G711MU) {
        audioEnc_ = OH_AudioCodec_CreateByName((AVCodecCodecName::AUDIO_ENCODER_G711MU_NAME).data());
        cout << "CreateEnc g711!" << endl;
    } else if (audioType_ == AudioBufferFormatType::TYPE_LBVC) {
        audioEnc_ = OH_AudioCodec_CreateByName((AVCodecCodecName::AUDIO_ENCODER_LBVC_NAME).data());
        cout << "CreateEnc lbvc!" << endl;
    } else if (audioType_ == AudioBufferFormatType::TYPE_AMRNB) {
        audioEnc_ = OH_AudioCodec_CreateByName((AVCodecCodecName::AUDIO_ENCODER_AMRNB_NAME).data());
        cout << "CreateEnc amrnb!" << endl;
    } else if (audioType_ == AudioBufferFormatType::TYPE_AMRWB) {
        audioEnc_ = OH_AudioCodec_CreateByName((AVCodecCodecName::AUDIO_ENCODER_AMRWB_NAME).data());
        cout << "CreateEnc amrwb!" << endl;
    } else if (audioType_ == AudioBufferFormatType::TYPE_MP3) {
        audioEnc_ = OH_AudioCodec_CreateByName((AVCodecCodecName::AUDIO_ENCODER_MP3_NAME).data());
        cout << "CreateEnc mp3!" << endl;
    } else {
        return AVCS_ERR_INVALID_VAL;
    }
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
    size_t frameBytes = 1152;
    if (audioType_ == AudioBufferFormatType::TYPE_OPUS) {
        size_t opussize = 960;
        frameBytes = opussize;
    } else if (audioType_ == AudioBufferFormatType::TYPE_G711MU || audioType_ == AudioBufferFormatType::TYPE_AMRNB) {
        size_t gmusize = 320;
        frameBytes = gmusize;
    } else if (audioType_ == AudioBufferFormatType::TYPE_LBVC || audioType_ == AudioBufferFormatType::TYPE_AMRWB) {
        size_t lbvcsize = 640;
        frameBytes = lbvcsize;
    } else if (audioType_ == AudioBufferFormatType::TYPE_AAC) {
        size_t aacsize = 1024;
        frameBytes = aacsize;
    }
    size_t currentSize = inputdatasize < frameBytes ? inputdatasize : frameBytes;
    while (isRunning_.load()) {
        unique_lock<mutex> lock(signal_->inMutex_);
        signal_->inCond_.wait(lock, [this]() { return (signal_->inQueue_.size() > 0 || !isRunning_.load()); });
        if (!isRunning_.load()) {
            break;
        }
        uint32_t index = signal_->inQueue_.front();
        auto buffer = signal_->inBufferQueue_.front();
        DEMO_CHECK_AND_BREAK_LOG(buffer != nullptr, "Fatal: GetInputBuffer fail");
        strncpy_s(reinterpret_cast<char*>(OH_AVBuffer_GetAddr(buffer)), currentSize, inputdata.c_str(), currentSize);
        buffer->buffer_->memory_->SetSize(currentSize);
        int32_t ret = AVCS_ERR_OK;
        if (isFirstFrame_) {
            buffer->buffer_->flag_ = AVCODEC_BUFFER_FLAGS_CODEC_DATA;
            ret = OH_AudioCodec_PushInputBuffer(audioEnc_, index);
            isFirstFrame_ = false;
        } else {
            buffer->buffer_->memory_->SetSize(1);
            buffer->buffer_->flag_ = AVCODEC_BUFFER_FLAGS_EOS;
            HandleEOS(index);
            isRunning_.store(false);
            break;
        }
        timeStamp_ += FRAME_DURATION_US;
        signal_->inQueue_.pop();
        signal_->inBufferQueue_.pop();
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

OH_AVCodec *AudioBufferAacEncDemo::CreateByMime(const char *mime)
{
    if (mime != nullptr) {
        if (strcmp(mime, "audio/mp4a-latm") == 0) {
            audioType_ = AudioBufferFormatType::TYPE_AAC;
            cout << "creat, aac" << endl;
        } else if (strcmp(mime, "audio/flac") == 0) {
            audioType_ = AudioBufferFormatType::TYPE_FLAC;
            cout << "creat, flac" << endl;
        } else if (strcmp(mime, "audio/lbvc") == 0) {
            audioType_ = AudioBufferFormatType::TYPE_LBVC;
            cout << "creat, LBVC" << endl;
        } else {
            audioType_ = AudioBufferFormatType::TYPE_G711MU;
        }
    }

    return OH_AudioCodec_CreateByMime(mime, true);
}

OH_AVCodec *AudioBufferAacEncDemo::CreateByName(const char *name)
{
    return OH_AudioCodec_CreateByName(name);
}

OH_AVErrCode AudioBufferAacEncDemo::Destroy(OH_AVCodec *codec)
{
    OH_AVErrCode ret = OH_AudioCodec_Destroy(codec);
    ClearQueue();
    return ret;
}

OH_AVErrCode AudioBufferAacEncDemo::SetCallback(OH_AVCodec *codec)
{
    if (codec == nullptr) {
        cout << "SetCallback, codec null" << endl;
    }
    if (signal_ == nullptr) {
        cout << "SetCallback, signal_ null" << endl;
    }
    cb_ = {&OnError, &OnOutputFormatChanged, &OnInputBufferAvailable, &OnOutputBufferAvailable};
    return OH_AudioCodec_RegisterCallback(codec, cb_, signal_);
}

OH_AVErrCode AudioBufferAacEncDemo::Prepare(OH_AVCodec *codec)
{
    return OH_AudioCodec_Prepare(codec);
}

OH_AVErrCode AudioBufferAacEncDemo::Start(OH_AVCodec *codec)
{
    return OH_AudioCodec_Start(codec);
}

OH_AVErrCode AudioBufferAacEncDemo::Stop(OH_AVCodec *codec)
{
    OH_AVErrCode ret = OH_AudioCodec_Stop(codec);
    ClearQueue();
    return ret;
}

OH_AVErrCode AudioBufferAacEncDemo::Flush(OH_AVCodec *codec)
{
    OH_AVErrCode ret = OH_AudioCodec_Flush(codec);
    ClearQueue();
    return ret;
}

OH_AVErrCode AudioBufferAacEncDemo::Reset(OH_AVCodec *codec)
{
    return OH_AudioCodec_Reset(codec);
}

OH_AVFormat *AudioBufferAacEncDemo::GetOutputDescription(OH_AVCodec *codec)
{
    return OH_AudioCodec_GetOutputDescription(codec);
}

OH_AVErrCode AudioBufferAacEncDemo::PushInputData(OH_AVCodec *codec, uint32_t index)
{
    OH_AVCodecBufferAttr info;
    if (!signal_->inBufferQueue_.empty()) {
        unique_lock<mutex> lock(signal_->inMutex_);
        auto buffer = signal_->inBufferQueue_.front();
        OH_AVBuffer_GetBufferAttr(buffer, &info);
        info.size = 100; // size 100
        OH_AVErrCode ret = OH_AVBuffer_SetBufferAttr(buffer, &info);
        if (ret != AV_ERR_OK) {
            return ret;
        }
        signal_->inBufferQueue_.pop();
    }
    return OH_AudioCodec_PushInputBuffer(codec, index);
}

OH_AVErrCode AudioBufferAacEncDemo::PushInputDataEOS(OH_AVCodec *codec, uint32_t index)
{
    OH_AVCodecBufferAttr info;
    info.size = 0;
    info.offset = 0;
    info.pts = 0;
    info.flags = AVCODEC_BUFFER_FLAGS_EOS;
    if (!signal_->inBufferQueue_.empty()) {
        unique_lock<mutex> lock(signal_->inMutex_);
        auto buffer = signal_->inBufferQueue_.front();
        OH_AVBuffer_SetBufferAttr(buffer, &info);
        signal_->inBufferQueue_.pop();
    }
    return OH_AudioCodec_PushInputBuffer(codec, index);
}

OH_AVErrCode AudioBufferAacEncDemo::FreeOutputData(OH_AVCodec *codec, uint32_t index)
{
    return OH_AudioCodec_FreeOutputBuffer(codec, index);
}

OH_AVErrCode AudioBufferAacEncDemo::IsValid(OH_AVCodec *codec, bool *isValid)
{
    return OH_AudioCodec_IsValid(codec, isValid);
}

uint32_t AudioBufferAacEncDemo::GetInputIndex()
{
    uint32_t sleeptime = 0;
    uint32_t index;
    uint32_t timeout = 5;
    while (signal_->inQueue_.empty() && sleeptime < timeout) {
        sleep(1);
        sleeptime++;
    }
    if (sleeptime >= timeout) {
        return 0;
    } else {
        index = signal_->inQueue_.front();
        signal_->inQueue_.pop();
    }
    return index;
}

uint32_t AudioBufferAacEncDemo::GetOutputIndex()
{
    uint32_t sleeptime = 0;
    uint32_t index;
    uint32_t timeout = 5;
    while (signal_->outQueue_.empty() && sleeptime < timeout) {
        sleep(1);
        sleeptime++;
    }
    if (sleeptime >= timeout) {
        return 0;
    } else {
        index = signal_->outQueue_.front();
        signal_->outQueue_.pop();
    }
    return index;
}

void AudioBufferAacEncDemo::ClearQueue()
{
    while (!signal_->inQueue_.empty()) {
        signal_->inQueue_.pop();
    }

    while (!signal_->outQueue_.empty()) {
        signal_->outQueue_.pop();
    }

    while (!signal_->inBufferQueue_.empty()) {
        signal_->inBufferQueue_.pop();
    }

    while (!signal_->outBufferQueue_.empty()) {
        signal_->outBufferQueue_.pop();
    }
}

bool AudioBufferAacEncDemo::RunCaseReset(const uint8_t *data, size_t size)
{
    std::string codecdata(reinterpret_cast<const char *>(data), size);
    inputdata = codecdata;
    inputdatasize = size;
    DEMO_CHECK_AND_RETURN_RET_LOG(CreateEnc() == AVCS_ERR_OK, false, "Fatal: CreateEnc fail");
    std::cout << "RunCase CreateEnc" << std::endl;
    OH_AVFormat *format = OH_AVFormat_Create();
    Setformat(format);
    DEMO_CHECK_AND_RETURN_RET_LOG(Configure(format) == AVCS_ERR_OK, false, "Fatal: Configure fail");
    std::cout << "RunCase format" << std::endl;
    DEMO_CHECK_AND_RETURN_RET_LOG(Start() == AVCS_ERR_OK, false, "Fatal: Start fail");
    std::cout << "RunCase Start" << std::endl;
    auto start = chrono::steady_clock::now();
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    auto end = chrono::steady_clock::now();
    std::cout << "Encode finished, time = " << std::chrono::duration_cast<chrono::milliseconds>(end - start).count()
              << " ms" << std::endl;
    DEMO_CHECK_AND_RETURN_RET_LOG(Stop() == AVCS_ERR_OK, false, "Fatal: Stop fail");
    //reset
    if (AV_ERR_OK != Reset(audioEnc_)) {
        return false;
    }
    //reset
    DEMO_CHECK_AND_RETURN_RET_LOG(Release() == AVCS_ERR_OK, false, "Fatal: Release fail");
    OH_AVFormat_Destroy(format);
    sleep(1);
    return true;
}