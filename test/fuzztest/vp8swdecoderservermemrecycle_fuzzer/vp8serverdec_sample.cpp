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
#include "video_decoder.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::MediaAVCodec::Codec;
using namespace std;
namespace {
    constexpr int32_t EIGHT = 8;
    constexpr int32_t SIXTEEN = 16;
    constexpr int32_t TWENTY_FOUR = 24;
    constexpr int64_t NANOS_IN_SECOND = 1000000000L;
    constexpr int64_t NANOS_IN_MICRO = 1000L;
    constexpr int32_t MAX_SEND_FRAMES = 10;
    typedef enum OH_AVCodecBufferFlags {
        AVCODEC_BUFFER_FLAGS_NONE = 0,
        AVCODEC_BUFFER_FLAGS_EOS = 1 << 0,
        AVCODEC_BUFFER_FLAGS_SYNC_FRAME = 1 << 1,
        AVCODEC_BUFFER_FLAGS_INCOMPLETE_FRAME = 1 << 2,
        AVCODEC_BUFFER_FLAGS_CODEC_DATA = 1 << 3,
        AVCODEC_BUFFER_FLAGS_DISCARD = 1 << 4,
        AVCODEC_BUFFER_FLAGS_DISPOSABLE = 1 << 5,
    } OH_AVCodecBufferFlags;

    void clearIntqueue(std::queue<uint32_t> &q)
    {
        std::queue<uint32_t> empty;
        swap(empty, q);
    }
} // namespace

int64_t Vp8VDecServerSample::GetSystemTimeUs()
{
    struct timespec now;
    (void)clock_gettime(CLOCK_BOOTTIME, &now);
    int64_t nanoTime = static_cast<int64_t>(now.tv_sec) * NANOS_IN_SECOND + now.tv_nsec;
    return nanoTime / NANOS_IN_MICRO;
}

void Vp8VDecServerSample::CallBack::OnError(AVCodecErrorType errorType, int32_t errorCode)
{
    cout << "errorType: " << errorType << endl;
    cout << "errorCode:" << errorCode << endl;
    tester->Flush();
    tester->Reset();
}

void Vp8VDecServerSample::CallBack::OnOutputFormatChanged(const Format &format)
{
    tester->GetOutputFormat();
}

void Vp8VDecServerSample::CallBack::OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    unique_lock<mutex> lock(tester->signal_->inMutex_);
    tester->signal_->inIdxQueue_.push(index);
    tester->signal_->inBufferQueue_.push(buffer);
    tester->signal_->inCond_.notify_all();
}

void Vp8VDecServerSample::CallBack::OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    if (buffer->flag_ == AVCODEC_BUFFER_FLAGS_EOS) {
        tester->isEOS_.store(true);
        tester->signal_->endCond_.notify_all();
        cout << " get eos output " << endl;
    }
    tester->codec_->ReleaseOutputBuffer(index);
}

Vp8VDecServerSample::~Vp8VDecServerSample()
{
    if (codec_ != nullptr) {
        codec_->Stop();
        codec_->Release();
        VideoDecoder *codec = static_cast<VideoDecoder*>(codec_.get());
        codec->DecStrongRef(codec);
    }
    for (auto cs : cs_vector) {
        if (cs != nullptr) {
            cs->DecStrongRef(cs);
        }
    }
}

int32_t Vp8VDecServerSample::ConfigServerDecoder()
{
    Format fmt;
    fmt.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, defaultWidth);
    fmt.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, defaultHeight);
    fmt.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, defaultPixelFormat);
    fmt.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, defaultFrameRate);
    fmt.PutIntValue(MediaDescriptionKey::MD_KEY_ROTATION_ANGLE, defaultRotation);
    fmt.PutIntValue(MediaDescriptionKey::MD_KEY_SCALE_TYPE, ScalingMode::SCALING_MODE_SCALE_TO_WINDOW);
    return codec_->Configure(fmt);
}

int32_t Vp8VDecServerSample::SetCallback()
{
    shared_ptr<CallBack> cb = make_shared<CallBack>(this);
    return codec_->SetCallback(cb);
}

int32_t Vp8VDecServerSample::SetOutputSurface()
{
    auto cs = Surface::CreateSurfaceAsConsumer();
    cs_vector.push_back(cs);
    sptr<IBufferConsumerListener> listener = new ConsumerListener(cs);
    cs->RegisterConsumerListener(listener);
    auto p = cs->GetProducer();
    auto ps = Surface::CreateSurfaceAsProducer(p);
    ps_vector.push_back(ps);
    return codec_->SetOutputSurface(ps);
}

int32_t Vp8VDecServerSample::InitDecoder()
{
    int32_t err;
    Media::Meta codecInfo;
    int32_t instanceid = 0;
    codecInfo.SetData("av_codec_event_info_instance_id", instanceid);
    err = codec_->Init(codecInfo);
    if (err != AVCS_ERR_OK) {
        cout << "decoder Init failed!" << endl;
        return err;
    }
    err = ConfigServerDecoder();
    if (err != AVCS_ERR_OK) {
        cout << "ConfigServerDecoder failed" << endl;
        return err;
    }
    err = SetCallback();
    if (err != AVCS_ERR_OK) {
        cout << "SetCallback failed" << endl;
        return err;
    }
    if (isSurfMode) {
        err = SetOutputSurface();
        if (err != AVCS_ERR_OK) {
            cout << "SetOutputSurface failed" << endl;
            return err;
        }
    }
    return err;
}

void Vp8VDecServerSample::RunVideoServerDecoder()
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
    int32_t err = InitDecoder();
    if (err != AVCS_ERR_OK) {
        cout << "Init decoder failed" << endl;
        return;
    }
    err = codec_->Start();
    if (err != AVCS_ERR_OK) {
        cout << "Start failed" << endl;
        return;
    }
    isRunning_.store(true);
    inFile_ = make_unique<ifstream>();
    if (inFile_ == nullptr) {
        Stop();
        return;
    }
    inFile_->open(inpDir, ios::in | ios::binary);
    if (!inFile_->is_open()) {
        cout << "open input file failed" << endl;
        Stop();
        inFile_->close();
        inFile_.reset();
        inFile_ = nullptr;
        return;
    }
    inputLoop_ = make_unique<thread>(&Vp8VDecServerSample::InputFunc, this);
    if (inputLoop_ == nullptr) {
        cout << "Failed to create input loop" << endl;
        isRunning_.store(false);
        Stop();
        ReleaseInFile();
        return;
    }
    if (isSurfMode) {
        err = SetOutputSurface();
        if (err != AVCS_ERR_OK) {
            cout << "SetOutputSurface failed" << endl;
        }
    }
}

void Vp8VDecServerSample::WaitForEos()
{
    if (inputLoop_ && inputLoop_->joinable()) {
        inputLoop_->join();
    }
}

void Vp8VDecServerSample::GetOutputFormat()
{
    Format fmt;
    int32_t err = codec_->GetOutputFormat(fmt);
    if (err != AVCS_ERR_OK) {
        cout << "GetOutputFormat fail" << endl;
        isRunning_.store(false);
        signal_->inCond_.notify_all();
        signal_->endCond_.notify_all();
    }
}

void Vp8VDecServerSample::Flush()
{
    int32_t err = codec_->Flush();
    if (err != AVCS_ERR_OK) {
        cout << "Flush fail" << endl;
        isRunning_.store(false);
        signal_->inCond_.notify_all();
        signal_->endCond_.notify_all();
    }
}

void Vp8VDecServerSample::Reset()
{
    int32_t err = codec_->Reset();
    if (err != AVCS_ERR_OK) {
        cout << "Reset fail" << endl;
        isRunning_.store(false);
        signal_->inCond_.notify_all();
        signal_->endCond_.notify_all();
    }
}

void Vp8VDecServerSample::Stop()
{
    StopInloop();
    ReleaseInFile();
    int32_t err = codec_->Stop();
    if (err != AVCS_ERR_OK) {
        cout << "Stop fail" << endl;
        isRunning_.store(false);
        signal_->inCond_.notify_all();
        signal_->endCond_.notify_all();
    }
}

void Vp8VDecServerSample::SetEOS(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    buffer->pts_ = GetSystemTimeUs();
    buffer->flag_ = AVCODEC_BUFFER_FLAGS_EOS;
    int32_t res = codec_->QueueInputBuffer(index);
    cout << "OH_VideoDecoder_PushInputData EOS res:" << res << endl;
    unique_lock<mutex> lock(signal_->outMutex_);
    signal_->endCond_.wait(lock, [this]() {
        if (!isRunning_.load()) {
            cout << "quit signal" << endl;
            return true;
        }
        return isEOS_.load();
    });
}

int32_t Vp8VDecServerSample::ReadData(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    int NUM_4 = 4;
    uint8_t ch[4] = {};
    (void)inFile_->read(reinterpret_cast<char *>(ch), NUM_4);
    if (repeatRun && inFile_->eof()) {
        inFile_->clear();
        inFile_->seekg(0, ios::beg);
        cout << "repeat" << endl;
        return 0;
    } else if (inFile_->eof()) {
        SetEOS(index, buffer);
        return 1;
    }
    uint32_t bufferSize = static_cast<uint32_t>(((ch[3] & 0xFF)) | ((ch[2] & 0xFF) << EIGHT) |
    ((ch[1] & 0xFF) << SIXTEEN) | ((ch[0] & 0xFF) << TWENTY_FOUR));
    return SendData(bufferSize, index, buffer);
}

int32_t Vp8VDecServerSample::SendData(uint32_t bufferSize, uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    if (bufferSize == 0 || bufferSize > MAX_BUFFER_SIZE) {
        cout << "ERROR: Invalid buffer size " << bufferSize << endl;
        return -1;
    }
    uint8_t *frameBuffer = new uint8_t[bufferSize];
    (void)inFile_->read(reinterpret_cast<char *>(frameBuffer), bufferSize);
    int32_t size = buffer->memory_->GetCapacity();
    buffer->pts_ = GetSystemTimeUs();
    buffer->memory_->SetSize(bufferSize);
    buffer->memory_->SetOffset(0);
    buffer->flag_ = AVCODEC_BUFFER_FLAGS_NONE;
    if (size < bufferSize) {
        delete[] frameBuffer;
        cout << "ERROR:AVMemory not enough, buffer size " << bufferSize <<
                " AVMemory Size " << size << endl;
        isRunning_.store(false);
        return 1;
    }
    uint8_t *bufferAddr = buffer->memory_->GetAddr();
    if (memcpy_s(bufferAddr, size, frameBuffer, bufferSize) != EOK) {
        delete[] frameBuffer;
        cout << "Fatal: memcpy fail" << endl;
        isRunning_.store(false);
        return 1;
    }
    delete[] frameBuffer;
    int32_t ret = codec_->QueueInputBuffer(index);
    if (ret != AV_ERR_OK) {
        errCount++;
        cout << "push input data failed, error:" << ret << endl;
    }
    frameCount_ = frameCount_ + 1;
    if (inFile_->eof()) {
        isRunning_.store(false);
    }
    return 0;
}

int32_t Vp8VDecServerSample::SendFuzzData(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    uint8_t *bufferAddr = buffer->memory_->GetAddr();
    if (memcpy_s(bufferAddr, buffer->memory_->GetCapacity(), fuzzData, fuzzSize) != EOK) {
        cout << "Fatal: memcpy fail" << endl;
        isRunning_.store(false);
        return 1;
    }
    buffer->pts_ = GetSystemTimeUs();
    buffer->flag_ = 0;
    buffer->memory_->SetOffset(0);
    buffer->memory_->SetSize(fuzzSize);
    return codec_->QueueInputBuffer(index);
}

void Vp8VDecServerSample::InputFunc()
{
    frameCount_ = 1;
    errCount = 0;
    while (isRunning_.load()) {
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
        if (sendFrameIndex == MAX_SEND_FRAMES) {
            int ret = SendFuzzData(index, buffer);
            if (ret == 1) {
                break;
            }
            sendFrameIndex++;
            continue;
        }
        if (sendFrameIndex > MAX_SEND_FRAMES) {
            SetEOS(index, buffer);
            break;
        }
        if (!inFile_->eof()) {
            int ret = ReadData(index, buffer);
            if (ret == 1) {
                break;
            }
        }
        sendFrameIndex++;
    }
}

void Vp8VDecServerSample::NotifyMemoryRecycle()
{
    int32_t err = codec_->NotifyMemoryRecycle();
    if (err != AVCS_ERR_OK) {
        cout << "NotifyMemoryRecycle fail" << endl;
        isRunning_.store(false);
        signal_->inCond_.notify_all();
        signal_->endCond_.notify_all();
    }
}

void Vp8VDecServerSample::NotifyMemoryWriteBack()
{
    int32_t err = codec_->NotifyMemoryWriteBack();
    if (err != AVCS_ERR_OK) {
        cout << "NotifyMemoryWriteBack fail" << endl;
        isRunning_.store(false);
        signal_->inCond_.notify_all();
        signal_->endCond_.notify_all();
    }
}

void Vp8VDecServerSample::StopInloop()
{
    if (inputLoop_ != nullptr && inputLoop_->joinable()) {
        unique_lock<mutex> lock(signal_->inMutex_);
        clearIntqueue(signal_->inIdxQueue_);
        signal_->inCond_.notify_all();
        isEOS_.store(true);
        signal_->endCond_.notify_all();
        lock.unlock();

        inputLoop_->join();
        inputLoop_.reset();
    }
}

void Vp8VDecServerSample::ReleaseInFile()
{
    if (inFile_ != nullptr) {
        if (inFile_->is_open()) {
            inFile_->close();
        }
        inFile_.reset();
        inFile_ = nullptr;
    }
}
