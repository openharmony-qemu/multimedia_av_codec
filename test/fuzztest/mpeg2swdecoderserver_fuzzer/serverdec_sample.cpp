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
#include "serverdec_sample.h"
#include <iostream>
#include <chrono>
using namespace OHOS;
using namespace OHOS::Media;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::MediaAVCodec::Codec;
using namespace std;
namespace {

} // namespace

void VDecServerSample::CallBack::OnError(AVCodecErrorType errorType, int32_t errorCode)
{
    cout << "--OnError--" << endl;
    tester->isRunning_.store(false);
    tester->signal_->inCond_.notify_all();
}

void VDecServerSample::CallBack::OnOutputFormatChanged(const Format &format)
{
    tester->GetOutputFormat();
}

void VDecServerSample::CallBack::OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    unique_lock<mutex> lock(tester->signal_->inMutex_);
    tester->signal_->inIdxQueue_.push(index);
    tester->signal_->inBufferQueue_.push(buffer);
    tester->signal_->inCond_.notify_all();
}

void VDecServerSample::CallBack::OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    tester->codec_->ReleaseOutputBuffer(index);
}

VDecServerSample::~VDecServerSample()
{
    if (codec_ != nullptr) {
        codec_->Stop();
        codec_->Release();
    }
    if (signal_ != nullptr) {
        delete signal_;
        signal_ = nullptr;
    }
}

int32_t VDecServerSample::ConfigServerDecoder()
{
    Format fmt;
    fmt.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, width);
    fmt.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, height);
    fmt.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, 1);
    fmt.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, frameRate);
    fmt.PutIntValue(MediaDescriptionKey::MD_KEY_ROTATION_ANGLE, 0);
    return codec_->Configure(fmt);
}

int32_t VDecServerSample::SetCallback()
{
    shared_ptr<CallBack> cb = make_shared<CallBack>(this);
    return codec_->SetCallback(cb);
}

void VDecServerSample::RunVideoServerDecoder()
{
    codec_ = make_shared<FCodec>("OH.Media.Codec.Decoder.Video.MPEG2");
    if (codec_ == nullptr) {
        cout << "Create failed" << endl;
        return;
    }
    Media::Meta meta{};
    int32_t err = codec_->Init(meta);
    err = ConfigServerDecoder();
    if (err != AVCS_ERR_OK) {
        cout << "ConfigServerDecoder failed" << endl;
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
    err = codec_->Start();
    if (err != AVCS_ERR_OK) {
        cout << "Start failed" << endl;
        return;
    }
    isRunning_.store(true);
    inputLoop_ = make_unique<thread>(&VDecServerSample::InputFunc, this);
    if (inputLoop_ == nullptr) {
        cout << "Failed to create input loop" << endl;
        isRunning_.store(false);
    }
}

void VDecServerSample::InputFunc()
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
        if (memcpy_s(bufferAddr, buffer->memory_->GetCapacity(), fuzzData, fuzzSize) != EOK) {
            break;
        }
        int32_t err = codec_->QueueInputBuffer(index);
        if (err != AVCS_ERR_OK) {
            cout << "QueueInputBuffer fail" << endl;
            break;
        }
        sendFrameIndex++;
    }
}

void VDecServerSample::WaitForEos()
{
    if (inputLoop_ && inputLoop_->joinable()) {
        inputLoop_->join();
    }
}

void VDecServerSample::GetOutputFormat()
{
    Format fmt;
    int32_t err = codec_->GetOutputFormat(fmt);
    if (err != AVCS_ERR_OK) {
        cout << "GetOutputFormat fail" << endl;
        isRunning_.store(false);
        signal_->inCond_.notify_all();
    }
}

void VDecServerSample::Flush()
{
    int32_t err = codec_->Flush();
    if (err != AVCS_ERR_OK) {
        cout << "Flush fail" << endl;
        isRunning_.store(false);
        signal_->inCond_.notify_all();
    }
}

void VDecServerSample::Reset()
{
    int32_t err = codec_->Reset();
    if (err != AVCS_ERR_OK) {
        cout << "Reset fail" << endl;
        isRunning_.store(false);
        signal_->inCond_.notify_all();
    }
}