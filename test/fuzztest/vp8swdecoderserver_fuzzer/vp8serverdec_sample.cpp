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
#include "vp8serverdec_sample.h"
#include <iostream>
#include "vpx_decoder_api.h"
#include "window.h"
#include "window_manager.h"
#include "window_option.h"
using namespace OHOS;
using namespace OHOS::Media;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::MediaAVCodec::Codec;
using namespace std;
namespace {
constexpr int32_t WIDTH = 720;
constexpr int32_t HEIGHT = 480;
constexpr int32_t FORMAT = 2;
constexpr int32_t ANGLE = 0;
constexpr int32_t FRAME_RATE = 30;
constexpr int32_t TIME = 12345;
constexpr int32_t MAX_SEND_FRAMES = 10;
} // namespace

void VDecServerSample::CallBack::OnError(AVCodecErrorType errorType, int32_t errorCode)
{
    tester->Flush();
    tester->Reset();
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
        VpxDecoder *codec = static_cast<VpxDecoder*>(codec_.get());
        codec->DecStrongRef(codec);
    }
}

int32_t VDecServerSample::ConfigServerDecoder()
{
    Format fmt;
    fmt.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, WIDTH);
    fmt.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, HEIGHT);
    fmt.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, FORMAT);
    fmt.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, FRAME_RATE);
    fmt.PutIntValue(MediaDescriptionKey::MD_KEY_ROTATION_ANGLE, ANGLE);
    fmt.PutIntValue(MediaDescriptionKey::MD_KEY_SCALE_TYPE, ScalingMode::SCALING_MODE_SCALE_TO_WINDOW);
    return codec_->Configure(fmt);
}

int32_t VDecServerSample::SetCallback()
{
    shared_ptr<CallBack> cb = make_shared<CallBack>(this);
    return codec_->SetCallback(cb);
}

void VDecServerSample::RunVideoServerDecoder()
{
    signal_ = std::make_shared<VDecSignal>();
    if (signal_ == nullptr) {
        return;
    }
    CreateVpxDecoderByName("OH.Media.Codec.Decoder.Video.VP8", codec_);
    if (codec_ == nullptr) {
        cout << "Create failed" << endl;
        return;
    }
    int32_t err;
    Media::Meta codecInfo;
    int32_t instanceid = 0;
    codecInfo.SetData("av_codec_event_info_instance_id", instanceid);
    err = codec_->Init(codecInfo);
    if (err != AVCS_ERR_OK) {
        cout << "decoder Init failed!" << endl;
        return;
    }
    err = ConfigServerDecoder();
    if (err != AVCS_ERR_OK) {
        cout << "ConfigServerDecoder failed" << endl;
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
    while (sendFrameIndex < MAX_SEND_FRAMES) {
        if (!isRunning_.load()) {
            break;
        }
        unique_lock<mutex> lock(signal_->inMutex_);
        signal_->inCond_.wait(lock, [this]() {
            if (!isRunning_.load()) {
                cout << "quit signal" << endl;
                return true;
            }
            return signal_->inIdxQueue_.size() > 0;
        });
        if (!isRunning_.load()) {
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
        buffer->pts_ = TIME;
        buffer->flag_ = 0;
        buffer->memory_->SetOffset(0);
        buffer->memory_->SetSize(fuzzSize);
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

void VDecServerSample::Start()
{
    int32_t err = codec_->Start();
    if (err != AVCS_ERR_OK) {
        cout << "Start fail" << endl;
        return;
    }
    isRunning_.store(true);
    if (inputLoop_ && inputLoop_->joinable()) {
        return;
    }
    inputLoop_ = make_unique<thread>(&VDecServerSample::InputFunc, this);
    if (inputLoop_ == nullptr) {
        cout << "Failed to create input loop" << endl;
        isRunning_.store(false);
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

void VDecServerSample::Stop()
{
    int32_t err = codec_->Stop();
    if (err != AVCS_ERR_OK) {
        cout << "Stop fail" << endl;
        isRunning_.store(false);
        signal_->inCond_.notify_all();
    }
}