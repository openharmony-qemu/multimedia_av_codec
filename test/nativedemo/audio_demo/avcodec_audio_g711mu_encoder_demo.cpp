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
#include "avcodec_audio_common.h"
#include "avcodec_audio_g711mu_encoder_demo.h"

using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::MediaAVCodec::AudioG711muDemo;
using namespace std;
namespace {
constexpr uint32_t CHANNEL_COUNT = 1;
constexpr uint32_t SAMPLE_RATE = 8000;
constexpr uint32_t FRAME_DURATION_US = 33000;
constexpr int32_t SAMPLE_FORMAT = AudioSampleFormat::SAMPLE_S16LE;
constexpr int32_t INPUT_FRAME_BYTES = 320; // 20ms

constexpr string_view INPUT_FILE_PATH = "/data/test/media/g711mu_8kHz_10s.pcm";
constexpr string_view OUTPUT_FILE_PATH = "/data/test/media/g711mu_8kHz_10s_afterEncode.raw";
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

void AEncG711muDemo::RunCase()
{
    std::cout << "RunCase enter" << std::endl;
    DEMO_CHECK_AND_RETURN_LOG(CreateEnc() == AVCS_ERR_OK, "Fatal: CreateEnc fail");

    OH_AVFormat *format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), SAMPLE_RATE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(), SAMPLE_FORMAT);

    DEMO_CHECK_AND_RETURN_LOG(Configure(format) == AVCS_ERR_OK, "Fatal: Configure fail");

    auto fmt = OH_AudioEncoder_GetOutputDescription(audioEnc_);
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
    std::cout << "GetOutputDescription " << "channels: " << channels
              << ", sampleRate: " << sampleRate
              << ", bitRate: " << bitRate
              << ", sampleFormat: " << sampleFormat
              << ", channelLayout: " << channelLayout
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

AEncG711muDemo::AEncG711muDemo()
    : isRunning_(false),
      inputFile_(std::make_unique<std::ifstream>(INPUT_FILE_PATH, std::ios::binary)),
      outputFile_(std::make_unique<std::ofstream>(OUTPUT_FILE_PATH, std::ios::binary)),
      audioEnc_(nullptr),
      signal_(nullptr),
      frameCount_(0)
{
}

AEncG711muDemo::~AEncG711muDemo()
{
    if (signal_) {
        delete signal_;
        signal_ = nullptr;
    }
}

int32_t AEncG711muDemo::CreateEnc()
{
    audioEnc_ = OH_AudioEncoder_CreateByName((AVCodecCodecName::AUDIO_ENCODER_G711MU_NAME).data());
    DEMO_CHECK_AND_RETURN_RET_LOG(audioEnc_ != nullptr, AVCS_ERR_UNKNOWN, "Fatal: CreateByName fail");

    signal_ = new AEncSignal();
    DEMO_CHECK_AND_RETURN_RET_LOG(signal_ != nullptr, AVCS_ERR_UNKNOWN, "Fatal: No memory");

    cb_ = {&OnError, &OnOutputFormatChanged, &OnInputBufferAvailable, &OnOutputBufferAvailable};
    int32_t ret = OH_AudioEncoder_SetCallback(audioEnc_, cb_, signal_);
    DEMO_CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_UNKNOWN, "Fatal: SetCallback fail");

    return AVCS_ERR_OK;
}

int32_t AEncG711muDemo::Configure(OH_AVFormat *format)
{
    return OH_AudioEncoder_Configure(audioEnc_, format);
}

int32_t AEncG711muDemo::Start()
{
    isRunning_.store(true);

    inputLoop_ = make_unique<thread>(&AEncG711muDemo::InputFunc, this);
    DEMO_CHECK_AND_RETURN_RET_LOG(inputLoop_ != nullptr, AVCS_ERR_UNKNOWN, "Fatal: No memory");

    outputLoop_ = make_unique<thread>(&AEncG711muDemo::OutputFunc, this);
    DEMO_CHECK_AND_RETURN_RET_LOG(outputLoop_ != nullptr, AVCS_ERR_UNKNOWN, "Fatal: No memory");

    return OH_AudioEncoder_Start(audioEnc_);
}

int32_t AEncG711muDemo::Stop()
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

int32_t AEncG711muDemo::Flush()
{
    return 0;
}

int32_t AEncG711muDemo::Reset()
{
    return 0;
}

int32_t AEncG711muDemo::Release()
{
    return 0;
}

void AEncG711muDemo::HandleEOS(const uint32_t &index)
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

void AEncG711muDemo::InputFunc()
{
    DEMO_CHECK_AND_RETURN_LOG(inputFile_ != nullptr && inputFile_->is_open(), "Fatal: open file fail");
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
            inputFile_->read(reinterpret_cast<char *>(OH_AVMemory_GetAddr(buffer)), INPUT_FRAME_BYTES);
            if (inputFile_->gcount() == 0) {
                HandleEOS(index);
                break;
            }
        } else {
            HandleEOS(index);
            break;
        }
        DEMO_CHECK_AND_BREAK_LOG(buffer != nullptr, "Fatal: GetInputBuffer fail");
        OH_AVCodecBufferAttr info;
        info.size = INPUT_FRAME_BYTES;
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
        timeStamp_ += FRAME_DURATION_US;
        signal_->inQueue_.pop();
        signal_->inBufferQueue_.pop();
        frameCount_++;
        if (ret != AVCS_ERR_OK) {
            cout << "Fatal error, exit" << endl;
            break;
        }
    }
    inputFile_->close();
}

void AEncG711muDemo::OutputFunc()
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

        OH_AVCodecBufferAttr attr = signal_->attrQueue_.front();
        OH_AVMemory *data = signal_->outBufferQueue_.front();
        if (data != nullptr) {
            cout << "OutputFunc write file,buffer index" << index << ", data size = :" << attr.size << endl;
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
