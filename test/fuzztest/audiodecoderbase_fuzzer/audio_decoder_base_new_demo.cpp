/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
 
#include <cstddef>
#include <cstdint>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <securec.h>
#include <chrono>
#include "avcodec_codec_name.h"
#include "avcodec_errors.h"
#include "demo_log.h"
#include "media_description.h"
#include "native_avcodec_base.h"
#include "native_avformat.h"
#include "native_avbuffer.h"
#include "audio_decoder_base_new_demo.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::MediaAVCodec::AudioBufferNewDemo;

static void OnError(OH_AVCodec *codec, int32_t errorCode, void *userData)
{
    return;
}

static void OnOutputFormatChanged(OH_AVCodec *codec, OH_AVFormat *format, void *userData)
{
    cout << "OnOutputFormatChanged received" << endl;
    return;
}

static void OnInputBufferAvailable(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *data, void *userData)
{
    ADecBufferSignal *signal = static_cast<ADecBufferSignal *>(userData);
    unique_lock<mutex> lock(signal->inMutex_);
    signal->inQueue_.push(index);
    signal->inBufferQueue_.push(data);
    signal->inCond_.notify_all();
}

static void OnOutputBufferAvailable(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *data, void *userData)
{
    ADecBufferSignal *signal = static_cast<ADecBufferSignal *>(userData);
    unique_lock<mutex> lock(signal->outMutex_);
    signal->outQueue_.push(index);
    signal->outBufferQueue_.push(data);
    signal->outCond_.notify_all();
}

OH_AVCodecCallback GetCbFunc()
{
    cb_ = {&OnError, &OnOutputFormatChanged, &OnInputBufferAvailable, &OnOutputBufferAvailable};
    return cb_;
}

void BaseFuzzDemo::InputFunc()
{
    while (isRunning_.load()) {
        unique_lock<mutex> lock(signal_->inMutex_);
        signal_->inCond_.wait(lock, [this]() { return (signal_->inQueue_.size() > 0 || !isRunning_.load()); });
        if (!isRunning_.load()) {
            break;
        }
        uint32_t index = signal_->inQueue_.front();
        auto buffer = signal_->inBufferQueue_.front();
        DEMO_CHECK_AND_BREAK_LOG(buffer != nullptr, "Fatal: GetInputBuffer fail");
        DEMO_CHECK_AND_BREAK_LOG(size_ < sizeof(buffer->buffer_->flag_), "Fatal: GetInputBuffer fail");
        memcpy_s(reinterpret_cast<char *>(&buffer->buffer_->pts_),
                 sizeof(buffer->buffer_->pts_), data_, sizeof(buffer->buffer_->pts_));
        memcpy_s(reinterpret_cast<char *>(&buffer->buffer_->flag_),
                 sizeof(buffer->buffer_->flag_), data_, sizeof(buffer->buffer_->flag_));
        memcpy_s(reinterpret_cast<char *>(OH_AVBuffer_GetAddr(buffer)), size_,
                 data_, size_);
        buffer->buffer_->memory_->SetSize(size_);
        OH_AudioCodec_PushInputBuffer(audioDec_, index);
        signal_->inQueue_.pop();
        signal_->inBufferQueue_.pop();
    }
}

void BaseFuzzDemo::OutputFunc()
{
    while (isRunning_.load()) {
        unique_lock<mutex> lock(signal_->outMutex_);
        signal_->outCond_.wait(lock, [this]() { return (signal_->outQueue_.size() > 0 || !isRunning_.load()); });
        if (!isRunning_.load()) {
            cout << "wait to stop, exit" << endl;
            break;
        }

        uint32_t index = signal_->outQueue_.front();
        signal_->outBufferQueue_.front();
        signal_->outBufferQueue_.pop();
        signal_->outQueue_.pop();
        OH_AudioCodec_FreeOutputBuffer(audioDec_, index);
    }
}

OH_AVErrCode BaseFuzzDemo::Start()
{
    isRunning_.store(true);
    OH_AudioCodec_Start(audioDec_);
    inputLoop_ = make_unique<thread>(&BaseFuzzDemo::InputFunc, this);
    DEMO_CHECK_AND_RETURN_RET_LOG(inputLoop_ != nullptr, AV_ERR_UNKNOWN, "Fatal: No memory");

    outputLoop_ = make_unique<thread>(&BaseFuzzDemo::OutputFunc, this);
    DEMO_CHECK_AND_RETURN_RET_LOG(outputLoop_ != nullptr, AV_ERR_UNKNOWN, "Fatal: No memory");

    return AV_ERR_OK;
}


OH_AVErrCode BaseFuzzDemo::Stop()
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
    return OH_AudioCodec_Stop(audioDec_);
}

int32_t BaseFuzzDemo::Release()
{
    return OH_AudioCodec_Destroy(audioDec_);
}