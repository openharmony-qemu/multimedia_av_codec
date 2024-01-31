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
#include "securec.h"
#include "avcodec_common.h"
#include "avcodec_errors.h"
#include "media_description.h"
#include "native_avformat.h"
#include "demo_log.h"
#include "avcodec_codec_name.h"
#include "ffmpeg_converter.h"
#include "avcodec_audio_opus_encoder_demo.h"

using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::MediaAVCodec::AudioOpusDemo;
using namespace std;

namespace {
constexpr int32_t CHANNEL_COUNT = 1;
constexpr int32_t SAM_RATE_COUNT = 8000;
constexpr int32_t BIT_RATE_COUNT = 150000;
constexpr int32_t BIT_PER_CODE_COUNT = 16;
constexpr int32_t COMPLEXITY_COUNT = 10;

constexpr string_view INPUT_FILE_PATH = "/data/test/media/test.pcm";
constexpr string_view OUTPUT_FILE_PATH = "/data/test/media/opus_encoder_test.opus";
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
    AEncSignal *signal = static_cast<AEncSignal *>(userData);
    unique_lock<mutex> lock(signal->inMutex_);
    signal->inQueue_.push(index);
    signal->inBufferQueue_.push(data);
    signal->inCond_.notify_all();
}

static void OnOutputBufferAvailable(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, OH_AVCodecBufferAttr *attr,
                                    void *userData)
{
    (void)codec;
    AEncSignal *signal = static_cast<AEncSignal *>(userData);
    unique_lock<mutex> lock(signal->outMutex_);
    signal->outQueue_.push(index);
    signal->outBufferQueue_.push(data);
    if (attr) {
        cout << "OnOutputBufferAvailable received, index:" << index << ", attr->size:" << attr->size
             << ", attr->flags:" << attr->flags << ", pts " << attr->pts << endl;
        signal->attrQueue_.push(*attr);
    } else {
        cout << "OnOutputBufferAvailable error, attr is nullptr!" << endl;
    }
    signal->outCond_.notify_all();
}

void AEncOpusDemo::RunCase()
{
    CreateEnc();
    OH_AVFormat *format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), SAM_RATE_COUNT);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_BITRATE.data(), BIT_RATE_COUNT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(),
        AudioSampleFormat::SAMPLE_S16LE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE.data(), BIT_PER_CODE_COUNT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_COMPLIANCE_LEVEL.data(), COMPLEXITY_COUNT);
    Configure(format);
    Start();
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    Stop();
    Release();
}

AEncOpusDemo::AEncOpusDemo()
    : isRunning_(false),
      inputFile_(std::make_unique<std::ifstream>(INPUT_FILE_PATH, std::ios::binary)),
      outputFile_(std::make_unique<std::ofstream>(OUTPUT_FILE_PATH, std::ios::binary)),
      audioEnc_(nullptr),
      signal_(nullptr),
      frameCount_(0)
{
}

AEncOpusDemo::~AEncOpusDemo()
{
    if (signal_) {
        delete signal_;
        signal_ = nullptr;
    }
}

int32_t AEncOpusDemo::CreateEnc()
{
    audioEnc_ = OH_AudioEncoder_CreateByName((AVCodecCodecName::AUDIO_ENCODER_OPUS_NAME).data());
    signal_ = new AEncSignal();
    cb_ = {&OnError, &OnOutputFormatChanged, &OnInputBufferAvailable, &OnOutputBufferAvailable};

    isFirstFrame_ = true;
    frameCount_ = 0;

    int32_t ret = OH_AudioEncoder_SetCallback(audioEnc_, cb_, signal_);
    DEMO_CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_UNKNOWN, "Fatal: SetCallback fail");

    return AVCS_ERR_OK;
}

int32_t AEncOpusDemo::Configure(OH_AVFormat *format)
{
    return OH_AudioEncoder_Configure(audioEnc_, format);
}

int32_t AEncOpusDemo::Start()
{
    isRunning_.store(true);

    inputLoop_ = make_unique<thread>(&AEncOpusDemo::InputFunc, this);
    DEMO_CHECK_AND_RETURN_RET_LOG(inputLoop_ != nullptr, AVCS_ERR_UNKNOWN, "Fatal: No memory");

    outputLoop_ = make_unique<thread>(&AEncOpusDemo::OutputFunc, this);
    DEMO_CHECK_AND_RETURN_RET_LOG(outputLoop_ != nullptr, AVCS_ERR_UNKNOWN, "Fatal: No memory");

    return OH_AudioEncoder_Start(audioEnc_);
}

int32_t AEncOpusDemo::Stop()
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

    return OH_AudioEncoder_Stop(audioEnc_);
}

int32_t AEncOpusDemo::Flush()
{
    return 0;
}

int32_t AEncOpusDemo::Reset()
{
    return 0;
}

int32_t AEncOpusDemo::Release()
{
    return 0;
}

void AEncOpusDemo::HandleEOS(const uint32_t &index)
{
    OH_AVCodecBufferAttr info;
    info.size = 0;
    info.offset = 0;
    info.pts = 0;
    info.flags = AVCODEC_BUFFER_FLAGS_EOS;
    OH_AudioEncoder_PushInputData(audioEnc_, index, info);
    std::cout << "end buffer\n";
    signal_->inQueue_.pop();
    signal_->inBufferQueue_.pop();
}

static int32_t GetFrameBytes()
{
    int32_t frameBytes = CHANNEL_COUNT * sizeof(short) * SAM_RATE_COUNT * 0.02;
    return frameBytes;
}

void AEncOpusDemo::InputFunc()
{
    auto frameBytes = GetFrameBytes();
    DEMO_CHECK_AND_RETURN_LOG(inputFile_ != nullptr && inputFile_->is_open(), "Fatal: open file fail");
    while (isRunning_.load()) {
        cout << "encode 2" << endl;
        unique_lock<mutex> lock(signal_->inMutex_);
        signal_->inCond_.wait(lock, [this]() { return (signal_->inQueue_.size() > 0 || !isRunning_.load()); });
        if (!isRunning_.load()) {
            break;
        }
        uint32_t index = signal_->inQueue_.front();
        auto buffer = signal_->inBufferQueue_.front();
        DEMO_CHECK_AND_BREAK_LOG(buffer != nullptr, "Fatal: GetInputBuffer fail");
        if (!inputFile_->eof()) {
            inputFile_->read(reinterpret_cast<char *>(OH_AVMemory_GetAddr(buffer)), frameBytes);
        } else {
            cout << "eofeofeofeofeof" << endl;
            HandleEOS(index);
            break;
        }
        DEMO_CHECK_AND_BREAK_LOG(buffer != nullptr, "Fatal: GetInputBuffer fail");
        OH_AVCodecBufferAttr info;
        info.size = frameBytes;
        info.offset = 0;

        int32_t ret = AVCS_ERR_OK;
        if (isFirstFrame_) {
            info.flags = AVCODEC_BUFFER_FLAGS_CODEC_DATA;
            ret = OH_AudioEncoder_PushInputData(audioEnc_, index, info);
            isFirstFrame_ = false;
        } else {
            info.flags = AVCODEC_BUFFER_FLAGS_NONE;
            ret = OH_AudioEncoder_PushInputData(audioEnc_, index, info);
        }

        signal_->inQueue_.pop();
        signal_->inBufferQueue_.pop();
        frameCount_++;
        if (ret != AVCS_ERR_OK) {
            cout << "Fatal error, exit" << endl;
            isRunning_ = false;
            break;
        }
    }
    inputFile_->close();
}

void AEncOpusDemo::OutputFunc()
{
    DEMO_CHECK_AND_RETURN_LOG(outputFile_ != nullptr && outputFile_->is_open(), "Fatal: open output file fail");
    int64_t codesize;
    while (isRunning_.load()) {
        unique_lock<mutex> lock(signal_->outMutex_);
        signal_->outCond_.wait(lock, [this]() { return (signal_->outQueue_.size() > 0 || !isRunning_.load()); });

        if (!isRunning_.load()) {
            cout << "wait to stop, exit" << endl;
            break;
        }

        uint32_t index = signal_->outQueue_.front();
        cout << "encode 1" << endl;
        OH_AVCodecBufferAttr attr = signal_->attrQueue_.front();
        OH_AVMemory *data = signal_->outBufferQueue_.front();
        if (data != nullptr) {
            codesize = attr.size;
            outputFile_->write(reinterpret_cast<char *>(&codesize), sizeof(int64_t));
            outputFile_->write(reinterpret_cast<char *>(&codesize), sizeof(int64_t));
            outputFile_->write(reinterpret_cast<char *>(OH_AVMemory_GetAddr(data)), attr.size);
        }
        if (attr.flags == AVCODEC_BUFFER_FLAGS_EOS || attr.size == 0) {
            cout << "encode eos" << endl;
            isRunning_.store(false);
            signal_->startCond_.notify_all();
        }
        signal_->outBufferQueue_.pop();
        signal_->attrQueue_.pop();
        signal_->outQueue_.pop();
        if (OH_AudioEncoder_FreeOutputData(audioEnc_, index) != AV_ERR_OK) {
            cout << "Fatal: FreeOutputData fail" << endl;
            break;
        }
        if (attr.flags == AVCODEC_BUFFER_FLAGS_EOS) {
            cout << "decode eos" << endl;
            isRunning_.store(false);
            signal_->startCond_.notify_all();
        }
    }
    outputFile_->close();
}
