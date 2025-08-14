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
#include "sample.h"
#include <arpa/inet.h>
#include <sys/time.h>
#include <utility>
#include <iostream>
using namespace OHOS;
using namespace OHOS::Media;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::MediaAVCodec::Codec;
using namespace std;
namespace {
constexpr int32_t EIGHT = 8;
constexpr int32_t SIXTEEN = 16;
constexpr int32_t TWENTY_FOUR = 24;
constexpr uint8_t SEI = 6;
constexpr uint8_t SPS = 7;
constexpr uint8_t PPS = 8;
constexpr uint32_t START_CODE_SIZE = 4;
constexpr uint8_t START_CODE[START_CODE_SIZE] = {0, 0, 0, 1};
constexpr uint8_t H265_NALU_TYPE = 0x1f;
constexpr int64_t NANOS_IN_SECOND = 1000000000L;
constexpr int64_t NANOS_IN_MICRO = 1000L;
typedef enum OH_AVCodecBufferFlags {
    AVCODEC_BUFFER_FLAGS_NONE = 0,
    /* Indicates that the Buffer is an End-of-Stream frame */
    AVCODEC_BUFFER_FLAGS_EOS = 1 << 0,
    /* Indicates that the Buffer contains keyframes */
    AVCODEC_BUFFER_FLAGS_SYNC_FRAME = 1 << 1,
    /* Indicates that the data contained in the Buffer is only part of a frame */
    AVCODEC_BUFFER_FLAGS_INCOMPLETE_FRAME = 1 << 2,
    /* Indicates that the Buffer contains Codec-Specific-Data */
    AVCODEC_BUFFER_FLAGS_CODEC_DATA = 1 << 3,
    /* Flag is used to discard packets which are required to maintain valid decoder state but are not required */
    AVCODEC_BUFFER_FLAGS_DISCARD = 1 << 4,
    /* Flag is used to indicate packets that contain  frames that can be discarded by the decoder */
    AVCODEC_BUFFER_FLAGS_DISPOSABLE = 1 << 5,
} OH_AVCodecBufferFlags;

void clearIntqueue(std::queue<uint32_t> &q)
{
    std::queue<uint32_t> empty;
    swap(empty, q);
}
} // namespace

int64_t VDecServerSample::GetSystemTimeUs()
{
    struct timespec now;
    (void)clock_gettime(CLOCK_BOOTTIME, &now);
    int64_t nanoTime = static_cast<int64_t>(now.tv_sec) * NANOS_IN_SECOND + now.tv_nsec;
    return nanoTime / NANOS_IN_MICRO;
}

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
    if (buffer->flag_ == AVCODEC_BUFFER_FLAGS_EOS) {
        tester->isEOS_.store(true);
        tester->signal_->endCond_.notify_all();
        cout << " get eos output " << endl;
    }
    tester->codec_->ReleaseOutputBuffer(index);
}

VDecServerSample::~VDecServerSample()
{
    if (codec_ != nullptr) {
        codec_->Stop();
        codec_->Release();
    }
    if (signal_ != nullptr) {
        signal_ = nullptr;
    }
    cout << "FCodec released" << endl;
}

int32_t VDecServerSample::ConfigServerDecoder()
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

int32_t VDecServerSample::SetCallback()
{
    shared_ptr<CallBack> cb = make_shared<CallBack>(this);
    return codec_->SetCallback(cb);
}

int32_t VDecServerSample::SetOutputSurface()
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

void VDecServerSample::PrepareDecoder()
{
    codec_ = sptr<FCodec>(new FCodec("OH.Media.Codec.Decoder.Video.AVC"));
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
    signal_ = std::make_shared<VDecSignal>();
    if (signal_ == nullptr) {
        cout << "Failed to new VDecSignal" << endl;
        return;
    }
    err = SetCallback();
    if (err != AVCS_ERR_OK) {
        cout << "SetCallback failed" << endl;
        return;
    }
    if (isSurfMode) {
        err = SetOutputSurface();
        if (err != AVCS_ERR_OK) {
            cout << "SetOutputSurface failed" << endl;
        }
    }
    err = codec_->Start();
    if (err != AVCS_ERR_OK) {
        cout << "Start failed" << endl;
        return;
    }
    isRunning_.store(true);
}

void VDecServerSample::RunVideoServerDecoder()
{
    PrepareDecoder();
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
    inputLoop_ = make_unique<thread>(&VDecServerSample::InputFunc, this);
    if (inputLoop_ == nullptr) {
        cout << "Failed to create input loop" << endl;
        isRunning_.store(false);
        Stop();
        ReleaseInFile();
        return;
    }
    if (isSurfMode) {
        int32_t err = SetOutputSurface();
        if (err != AVCS_ERR_OK) {
            cout << "SetOutputSurface failed" << endl;
        }
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
        signal_->endCond_.notify_all();
    }
}

void VDecServerSample::Flush()
{
    int32_t err = codec_->Flush();
    if (err != AVCS_ERR_OK) {
        cout << "Flush fail" << endl;
        isRunning_.store(false);
        signal_->inCond_.notify_all();
        signal_->endCond_.notify_all();
    }
}

void VDecServerSample::Reset()
{
    int32_t err = codec_->Reset();
    if (err != AVCS_ERR_OK) {
        cout << "Reset fail" << endl;
        isRunning_.store(false);
        signal_->inCond_.notify_all();
        signal_->endCond_.notify_all();
    }
}

void VDecServerSample::Stop()
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

void VDecServerSample::SetEOS(uint32_t index, std::shared_ptr<AVBuffer> buffer)
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

void VDecServerSample::CopyStartCode(uint8_t *frameBuffer, uint32_t bufferSize, std::shared_ptr<AVBuffer> buffer)
{
    if (memcpy_s(frameBuffer, bufferSize + START_CODE_SIZE, START_CODE, START_CODE_SIZE) != EOK) {
        cout << "Fatal: memory copy failed" << endl;
    }
    buffer->pts_ = GetSystemTimeUs();
    buffer->memory_->SetSize(bufferSize + START_CODE_SIZE);
    buffer->memory_->SetOffset(0);
    switch (frameBuffer[START_CODE_SIZE] & H265_NALU_TYPE) {
        case SPS:
        case PPS:
        case SEI:
            buffer->flag_ = AVCODEC_BUFFER_FLAGS_CODEC_DATA;
            break;
        default: {
            buffer->flag_ = AVCODEC_BUFFER_FLAGS_NONE;
        }
    }
}

int32_t VDecServerSample::ReadData(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    uint8_t ch[4] = {};
    (void)inFile_->read(reinterpret_cast<char *>(ch), START_CODE_SIZE);
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

int32_t VDecServerSample::SendData(uint32_t bufferSize, uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    uint8_t *frameBuffer = new uint8_t[bufferSize + START_CODE_SIZE];
    (void)inFile_->read(reinterpret_cast<char *>(frameBuffer + START_CODE_SIZE), bufferSize);
    int32_t size = buffer->memory_->GetCapacity();
    CopyStartCode(frameBuffer, bufferSize, buffer);
    if (size < (bufferSize + START_CODE_SIZE)) {
        delete[] frameBuffer;
        cout << "ERROR:AVMemory not enough, buffer size " << (bufferSize + START_CODE_SIZE) << " AVMemory Size " << size
             << endl;
        isRunning_.store(false);
        return 1;
    }
    uint8_t *bufferAddr = buffer->memory_->GetAddr();
    if (memcpy_s(bufferAddr, size, frameBuffer, bufferSize + START_CODE_SIZE) != EOK) {
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

void VDecServerSample::InputFunc()
{
    frameCount_ = 1;
    errCount = 0;
    while (true) {
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
        if (!inFile_->eof()) {
            int ret = ReadData(index, buffer);
            if (ret == 1) {
                break;
            }
        }
    }
}

void VDecServerSample::NotifyMemoryRecycle()
{
    int32_t err = codec_->NotifyMemoryRecycle();
    if (err != AVCS_ERR_OK) {
        cout << "NotifyMemoryRecycle fail" << endl;
        isRunning_.store(false);
        signal_->inCond_.notify_all();
        signal_->endCond_.notify_all();
    }
}

void VDecServerSample::NotifyMemoryWriteBack()
{
    int32_t err = codec_->NotifyMemoryWriteBack();
    if (err != AVCS_ERR_OK) {
        cout << "NotifyMemoryWriteBack fail" << endl;
        isRunning_.store(false);
        signal_->inCond_.notify_all();
        signal_->endCond_.notify_all();
    }
}

void VDecServerSample::StopInloop()
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

void VDecServerSample::ReleaseInFile()
{
    if (inFile_ != nullptr) {
        if (inFile_->is_open()) {
            inFile_->close();
        }
        inFile_.reset();
        inFile_ = nullptr;
    }
}