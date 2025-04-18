/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#include "hevcserverdec_sample.h"
#include <iostream>
#include "hevc_decoder_api.h"
using namespace OHOS;
using namespace OHOS::Media;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::MediaAVCodec::Codec;
using namespace std;
namespace {
constexpr int32_t WIDTH = 1920;
constexpr int32_t HIGHT = 1080;
constexpr int32_t FORMAT = 2;
constexpr int32_t ANGLE = 0;
constexpr int32_t FORMAT_RATE = 30;
constexpr int32_t TIME = 12345;
constexpr int32_t MAX_SEND_FRAMES = 10;
} // namespace

class TestConsumerListener : public IBufferConsumerListener {
public:
    TestConsumerListener(sptr<Surface> cs, std::string_view name) : cs(cs) {};
    ~TestConsumerListener() {}
    void OnBufferAvailable() override
    {
        sptr<SurfaceBuffer> buffer;
        int32_t flushFence;
        cs->AcquireBuffer(buffer, flushFence, timestamp, damage);
        cs->ReleaseBuffer(buffer, -1);
    }

private:
    int64_t timestamp = 0;
    Rect damage = {};
    sptr<Surface> cs {nullptr};
};

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
        HevcDecoder *codec = reinterpret_cast<HevcDecoder*>(codec_.get());
        codec->DecStrongRef(codec);
    }
    if (signal_ != nullptr) {
        delete signal_;
        signal_ = nullptr;
    }
}

int32_t VDecServerSample::ConfigServerDecoder()
{
    Format fmt;
    fmt.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, WIDTH);
    fmt.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, HIGHT);
    fmt.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, FORMAT);
    fmt.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, FORMAT_RATE);
    fmt.PutIntValue(MediaDescriptionKey::MD_KEY_ROTATION_ANGLE, ANGLE);
    return codec_->Configure(fmt);
}

int32_t VDecServerSample::SetCallback()
{
    shared_ptr<CallBack> cb = make_shared<CallBack>(this);
    return codec_->SetCallback(cb);
}

void VDecServerSample::CreateSurface()
{
    cs[0] = Surface::CreateSurfaceAsConsumer();
    sptr<IBufferConsumerListener> listener = new TestConsumerListener(cs[0], outDIR);
    cs[0]->RegisterConsumerListener(listener);
    auto p = cs[0]->GetProducer();
    ps[0] = Surface::CreateSurfaceAsProducer(p);
    nativeWindow[0] = CreateNativeWindowFromSurface(&ps[0]);
}

void VDecServerSample::RunVideoServerDecoder()
{
    CreateHevcDecoderByName("OH.Media.Codec.Decoder.Video.HEVC", codec_);
    if (codec_ == nullptr) {
        cout << "Create failed" << endl;
        return;
    }
    int32_t err = ConfigServerDecoder();
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

void VDecServerSample::RunVideoServerSurfaceDecoder()
{
    int32_t err;
    CreateSurface();
    CreateHevcDecoderByName("OH.Media.Codec.Decoder.Video.HEVC", codec_);
    if (codec_ == nullptr) {
        cout << "Create failed" << endl;
        return;
    }
    std::vector<CapabilityData> caps;
    err = GetHevcDecoderCapabilityList(caps);
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