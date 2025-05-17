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
#include <arpa/inet.h>
#include <sys/time.h>
#include <utility>
#include "fcodec_sample.h"
#include "native_avbuffer_info.h"
#include <iostream>
#include <chrono>
using namespace OHOS;
using namespace OHOS::Media;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::MediaAVCodec::Codec;
using namespace std;
namespace {

} // namespace

void FCodecServerSample::CallBack::OnError(AVCodecErrorType errorType, int32_t errorCode)
{
    cout << "---- OnError ----" << endl;
    tester->isRunning_.store(false);
    tester->signal_->inCond_.notify_all();
}

void FCodecServerSample::CallBack::OnOutputFormatChanged(const Format &format)
{
    cout << "---- OnOutputFormatChanged ----" << endl;
    tester->GetOutputFormat();
}

void FCodecServerSample::CallBack::OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    unique_lock<mutex> lock(tester->signal_->inMutex_);
    tester->GetOutputFormat();
    tester->signal_->inIdxQueue_.push(index);
    tester->signal_->inBufferQueue_.push(buffer);
    tester->signal_->inCond_.notify_all();
}

void FCodecServerSample::CallBack::OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    tester->fcodec_->RenderOutputBuffer(index);
}

FCodecServerSample::~FCodecServerSample()
{
    if (fcodec_ != nullptr) {
        fcodec_->Stop();
        fcodec_->Release();
    }
    cs = nullptr;
    ps = nullptr;
    if (signal_ != nullptr) {
        delete signal_;
        signal_ = nullptr;
    }
    cout << "FCodec released" << endl;
}

int32_t FCodecServerSample::Configure()
{
    Format fmt;
    fmt.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, width);
    fmt.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, height);
    fmt.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, 0);
    fmt.PutIntValue(MediaDescriptionKey::MD_KEY_SCALE_TYPE, 1);
    fmt.PutIntValue(MediaDescriptionKey::MD_KEY_ROTATION_ANGLE, 1);
    fmt.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, frameRate);
    return fcodec_->Configure(fmt);
}

int32_t FCodecServerSample::SetSurface()
{
    cs = Surface::CreateSurfaceAsConsumer();
    sptr<IBufferConsumerListener> listener = new TestConsumerListener(cs);
    cs->RegisterConsumerListener(listener);
    auto p = cs->GetProducer();
    ps = Surface::CreateSurfaceAsProducer(p);
    return fcodec_->SetOutputSurface(ps);
}

int32_t FCodecServerSample::SetCallback()
{
    shared_ptr<CallBack> cb = make_shared<CallBack>(this);
    return fcodec_->SetCallback(cb);
}

void FCodecServerSample::RunFCodecDecoder()
{
    fcodec_ = sptr<FCodec>(new FCodec("OH.Media.Codec.Decoder.Video.AVC"));
    if (fcodec_ == nullptr) {
        cout << "Create failed" << endl;
        return;
    }
    int32_t err = Configure();
    if (err != AVCS_ERR_OK) {
        cout << "Configure failed" << endl;
        return;
    }
    err = SetSurface();
    if (err != AVCS_ERR_OK) {
        cout << "SetSurface failed" << endl;
        return;
    }
    signal_ = new VDecSignal();
    if (signal_ == nullptr) {
        cout << "Failed to new VDecSignal" << endl;
        return;
    }
    err = SetCallback();
    if (err != AVCS_ERR_OK) {
        cout << "SetCallback failed" << endl;
        return;
    }
    err = fcodec_->Start();
    if (err != AVCS_ERR_OK) {
        cout << "Start failed" << endl;
        return;
    }
    isRunning_.store(true);
    inputLoop_ = make_unique<thread>(&FCodecServerSample::InputFunc, this);
    if (inputLoop_ == nullptr) {
        cout << "Failed to create input loop" << endl;
        isRunning_.store(false);
    }
}

void FCodecServerSample::InputFunc()
{
    int32_t time = 1000;
    while (sendFrameIndex < frameIndex) {
        if (!isRunning_.load()) {
            break;
        }
        unique_lock<mutex> lock(signal_->inMutex_);
        signal_->inCond_.wait_for(lock, std::chrono::milliseconds(time), [this]() {
            if (!isRunning_.load()) {
                cout << "quit signal" << endl;
                return true;
            }
            return signal_->inIdxQueue_.size() > 0;
        });
        if (!isRunning_.load() || signal_->inIdxQueue_.size() == 0) {
            break;
        }
        uint32_t index = signal_->inIdxQueue_.front();
        auto buffer = signal_->inBufferQueue_.front();
        signal_->inIdxQueue_.pop();
        signal_->inBufferQueue_.pop();
        lock.unlock();
        if (buffer->memory_ == nullptr) {
            isRunning_.store(false);
            break;
        }
        uint8_t *bufferAddr = buffer->memory_->GetAddr();
        uint32_t bufferSize = buffer->memory_->GetCapacity();
        if (fuzzSize <= bufferSize) {
            buffer->memory_->SetSize(fuzzSize);
        } else {
            buffer->memory_->SetSize(bufferSize);
        }
        if (memcpy_s(bufferAddr, bufferSize, fuzzData, fuzzSize) != EOK) {
            break;
        }
        if (fuzzSize % 2u == 0u) {
            buffer->flag_ = AVCODEC_BUFFER_FLAGS_CODEC_DATA;
        }
        buffer->flag_ = AVCODEC_BUFFER_FLAG_NONE;
        int32_t err = fcodec_->QueueInputBuffer(index);
        if (err != AVCS_ERR_OK) {
            cout << "QueueInputBuffer failed" << endl;
            break;
        }
        sendFrameIndex++;
    }
}

void FCodecServerSample::WaitForEos()
{
    if (inputLoop_ && inputLoop_->joinable()) {
        inputLoop_->join();
    }
}

void FCodecServerSample::GetOutputFormat()
{
    Format fmt;
    int32_t err = fcodec_->GetOutputFormat(fmt);
    if (err != AVCS_ERR_OK) {
        cout << "GetOutputFormat failed" << endl;
        isRunning_.store(false);
        signal_->inCond_.notify_all();
    }
}

void FCodecServerSample::Flush()
{
    int32_t err = fcodec_->Flush();
    if (err != AVCS_ERR_OK) {
        cout << "Flush failed" << endl;
        isRunning_.store(false);
        signal_->inCond_.notify_all();
    }
}

void FCodecServerSample::Reset()
{
    int32_t err = fcodec_->Reset();
    if (err != AVCS_ERR_OK) {
        cout << "Reset failed" << endl;
        isRunning_.store(false);
        signal_->inCond_.notify_all();
    }
}