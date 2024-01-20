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
#include "avcodec_audio_avbuffer_aac_encoder_demo.h"

using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::MediaAVCodec::AudioAacEncDemo;
using namespace std;
namespace {
constexpr uint32_t CHANNEL_COUNT = 2;
constexpr uint32_t SAMPLE_RATE = 44100;
constexpr uint32_t FRAME_DURATION_US = 33000;
constexpr int32_t SAMPLE_FORMAT = AudioSampleFormat::SAMPLE_S32LE;
constexpr int32_t INPUT_FRAME_BYTES = 2 * 1024 * 4;

constexpr string_view INPUT_FILE_PATH = "/data/test/media/aac_2c_44100hz_199k.pcm";
constexpr string_view OUTPUT_FILE_PATH = "/data/test/media/encode2.aac";
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
    } else {
        cout << "OnOutputBufferAvailable error, attr is nullptr!" << endl;
    }
    signal->outCond_.notify_all();
}

void AudioBufferAacEncDemo::RunCase()
{
    std::cout << "RunCase enter" << std::endl;
    DEMO_CHECK_AND_RETURN_LOG(CreateEnc() == AVCS_ERR_OK, "Fatal: CreateEnc fail");

    OH_AVFormat *format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), SAMPLE_RATE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(), SAMPLE_FORMAT);

    DEMO_CHECK_AND_RETURN_LOG(Configure(format) == AVCS_ERR_OK, "Fatal: Configure fail");

    auto fmt = OH_AudioCodec_GetOutputDescription(audioEnc_);
    int channels;
    int sampleRate;
    int64_t bitRate;
    int sampleFormat;
    int64_t channelLayout;
    int frameSize;
    OH_AVFormat_GetIntValue(fmt, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), &channels);
    OH_AVFormat_GetIntValue(fmt, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), &sampleRate);
    OH_AVFormat_GetLongValue(fmt, MediaDescriptionKey::MD_KEY_BITRATE.data(), &bitRate);
    OH_AVFormat_GetIntValue(fmt, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(), &sampleFormat);
    OH_AVFormat_GetLongValue(fmt, MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT.data(), &channelLayout);
    OH_AVFormat_GetIntValue(fmt, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLES_PER_FRAME.data(), &frameSize);
    std::cout << "GetOutputDescription "
              << "channels: " << channels << ", sampleRate: " << sampleRate << ", bitRate: " << bitRate
              << ", sampleFormat: " << sampleFormat << ", channelLayout: " << channelLayout
              << ", frameSize: " << frameSize << std::endl;

    DEMO_CHECK_AND_RETURN_LOG(Start() == AVCS_ERR_OK, "Fatal: Start fail");
    auto start = chrono::steady_clock::now();

    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }

    auto end = chrono::steady_clock::now();
    std::cout << "Encode finished, time = " << std::chrono::duration_cast<chrono::milliseconds>(end - start).count()
              << " ms" << std::endl;
    DEMO_CHECK_AND_RETURN_LOG(Stop() == AVCS_ERR_OK, "Fatal: Stop fail");
    DEMO_CHECK_AND_RETURN_LOG(Release() == AVCS_ERR_OK, "Fatal: Release fail");
    OH_AVFormat_Destroy(format);
}

int32_t AudioBufferAacEncDemo::GetFileSize(const std::string &filePath)
{
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file) {
        std::cerr << "Failed to open file: " << filePath << std::endl;
        return -1;
    }

    std::streampos fileSize = file.tellg(); // 获取文件大小
    file.close();

    return (int32_t)fileSize;
}

AudioBufferAacEncDemo::AudioBufferAacEncDemo() : isRunning_(false), audioEnc_(nullptr), signal_(nullptr), frameCount_(0)
{
    fileSize_ = GetFileSize(INPUT_FILE_PATH.data());
    inputFile_ = std::make_unique<std::ifstream>(INPUT_FILE_PATH, std::ios::binary);
    outputFile_ = std::make_unique<std::ofstream>(OUTPUT_FILE_PATH, std::ios::binary);
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
    audioEnc_ = OH_AudioCodec_CreateByName((AVCodecCodecName::AUDIO_ENCODER_AAC_NAME).data());
    DEMO_CHECK_AND_RETURN_RET_LOG(audioEnc_ != nullptr, AVCS_ERR_UNKNOWN, "Fatal: CreateByName fail");

    signal_ = new AEncSignal();
    DEMO_CHECK_AND_RETURN_RET_LOG(signal_ != nullptr, AVCS_ERR_UNKNOWN, "Fatal: No memory");

    cb_ = {&OnError, &OnOutputFormatChanged, &OnInputBufferAvailable, &OnOutputBufferAvailable};
    int32_t ret = OH_AudioCodec_RegisterCallback(audioEnc_, cb_, signal_);
    DEMO_CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_UNKNOWN, "Fatal: SetCallback fail");

    return AVCS_ERR_OK;
}

int32_t AudioBufferAacEncDemo::Configure(OH_AVFormat *format)
{
    return OH_AudioCodec_Configure(audioEnc_, format);
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
    std::cout << "end buffer\n";
    signal_->inQueue_.pop();
    signal_->inBufferQueue_.pop();
}

void AudioBufferAacEncDemo::InputFunc()
{
    DEMO_CHECK_AND_RETURN_LOG(inputFile_ != nullptr && inputFile_->is_open(), "Fatal: open file fail");
    int32_t sumReadSize = 0;
    while (isRunning_.load()) {
        unique_lock<mutex> lock(signal_->inMutex_);
        signal_->inCond_.wait(lock, [this]() { return (signal_->inQueue_.size() > 0 || !isRunning_.load()); });
        if (!isRunning_.load()) {
            break;
        }
        uint32_t index = signal_->inQueue_.front();
        auto buffer = signal_->inBufferQueue_.front();
        DEMO_CHECK_AND_BREAK_LOG(buffer != nullptr, "Fatal: GetInputBuffer fail");
        if (!inputFile_->eof()) {
            inputFile_->read(reinterpret_cast<char *>(OH_AVBuffer_GetAddr(buffer)), INPUT_FRAME_BYTES);
            buffer->buffer_->memory_->SetSize(INPUT_FRAME_BYTES);
            sumReadSize += INPUT_FRAME_BYTES;
        } else {
            buffer->buffer_->memory_->SetSize(1);
            buffer->buffer_->flag_ = AVCODEC_BUFFER_FLAGS_EOS;
            HandleEOS(index);
            sumReadSize += 0;
            break;
        }
        DEMO_CHECK_AND_BREAK_LOG(buffer != nullptr, "Fatal: GetInputBuffer fail");

        cout << "InputFunc, INPUT_FRAME_BYTES:" << INPUT_FRAME_BYTES << " flag:" << buffer->buffer_->flag_
             << " sumReadSize:" << sumReadSize << " fileSize_:" << fileSize_
             << " process:" << 100 * sumReadSize / fileSize_ << "%" << endl;   // 100

        int32_t ret = AVCS_ERR_OK;
        if (isFirstFrame_) {
            buffer->buffer_->flag_ = AVCODEC_BUFFER_FLAGS_CODEC_DATA;
            ret = OH_AudioCodec_PushInputBuffer(audioEnc_, index);
            isFirstFrame_ = false;
        } else {
            buffer->buffer_->flag_ = AVCODEC_BUFFER_FLAGS_NONE;
            ret = OH_AudioCodec_PushInputBuffer(audioEnc_, index);
        }
        timeStamp_ += FRAME_DURATION_US;
        signal_->inQueue_.pop();
        signal_->inBufferQueue_.pop();
        frameCount_++;
        if (ret != AVCS_ERR_OK) {
            cout << "Fatal error, exit" << endl;
            break;
        }
    }
    cout << "stop, exit" << endl;
    inputFile_->close();
}

void AudioBufferAacEncDemo::OutputFunc()
{
    DEMO_CHECK_AND_RETURN_LOG(outputFile_ != nullptr && outputFile_->is_open(), "Fatal: open output file fail");
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
            cout << "OutputFunc write file,buffer index:" << index
                 << ", data size:" << avBuffer->buffer_->memory_->GetSize() << endl;
            outputFile_->write(reinterpret_cast<char *>(OH_AVBuffer_GetAddr(avBuffer)),
                               avBuffer->buffer_->memory_->GetSize());
        }
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
        if (avBuffer->buffer_->flag_ == AVCODEC_BUFFER_FLAGS_EOS) {
            cout << "decode eos" << endl;
            isRunning_.store(false);
            signal_->startCond_.notify_all();
        }
    }
    cout << "stop, exit" << endl;
    outputFile_->close();
}
