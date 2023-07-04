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

#include "vdec_sample.h"
#include <gtest/gtest.h>

using namespace std;
using namespace OHOS::MediaAVCodec::VCodecTestParam;
namespace OHOS {
namespace MediaAVCodec {
VDecCallbackTest::VDecCallbackTest(std::shared_ptr<VDecSignal> signal) : signal_(signal) {}

VDecCallbackTest::~VDecCallbackTest() {}

void VDecCallbackTest::OnError(int32_t errorCode)
{
    cout << "ADec Error errorCode=" << errorCode;
    if (signal_ == nullptr) {
        return;
    }
    signal_->errorNum_ += 1;
    cout << ", errorNum=" << signal_->errorNum_ << endl;
}

void VDecCallbackTest::OnStreamChanged(std::shared_ptr<FormatMock> format)
{
    (void)format;
    cout << "VDec Format Changed" << endl;
}

void VDecCallbackTest::OnNeedInputData(uint32_t index, std::shared_ptr<AVMemoryMock> data)
{
    if (signal_ == nullptr) {
        return;
    }
    unique_lock<mutex> lock(signal_->inMutex_);
    if (!signal_->isRunning_.load()) {
        return;
    }
    signal_->inIndexQueue_.push(index);
    signal_->inBufferQueue_.push(data);
    signal_->inCond_.notify_all();
}

void VDecCallbackTest::OnNewOutputData(uint32_t index, std::shared_ptr<AVMemoryMock> data, OH_AVCodecBufferAttr attr)
{
    if (signal_ == nullptr) {
        return;
    }
    unique_lock<mutex> lock(signal_->outMutex_);
    if (!signal_->isRunning_.load()) {
        return;
    }
    signal_->outIndexQueue_.push(index);
    signal_->outBufferQueue_.push(data);
    signal_->outAttrQueue_.push(attr);
    signal_->outCond_.notify_all();
}

TestConsumerListener::TestConsumerListener(sptr<Surface> cs, std::string_view name) : cs_(cs)
{
    outFile_ = std::make_unique<std::ofstream>();
    outFile_->open(name.data(), std::ios::out | std::ios::binary);
}

TestConsumerListener::~TestConsumerListener()
{
    if (outFile_ != nullptr) {
        outFile_->close();
    }
}

void TestConsumerListener::OnBufferAvailable()
{
    sptr<SurfaceBuffer> buffer;
    int32_t flushFence;

    cs_->AcquireBuffer(buffer, flushFence, timestamp_, damage_);

    (void)outFile_->write(reinterpret_cast<char *>(buffer->GetVirAddr()), buffer->GetSize());
    cs_->ReleaseBuffer(buffer, -1);
}

VideoDecSample::VideoDecSample(std::shared_ptr<VDecSignal> signal) : signal_(signal) {}

VideoDecSample::~VideoDecSample()
{
    FlushInner();
    if (videoDec_ != nullptr) {
        (void)videoDec_->Release();
    }
    if (inFile_ != nullptr && inFile_->is_open()) {
        inFile_->close();
    }
    if (outFile_ != nullptr && outFile_->is_open()) {
        outFile_->close();
    }
}

bool VideoDecSample::CreateVideoDecMockByMime(const std::string &mime)
{
    videoDec_ = VCodecMockFactory::CreateVideoDecMockByMime(mime);
    return videoDec_ != nullptr;
}

bool VideoDecSample::CreateVideoDecMockByName(const std::string &name)
{
    videoDec_ = VCodecMockFactory::CreateVideoDecMockByName(name);
    return videoDec_ != nullptr;
}

int32_t VideoDecSample::SetCallback(std::shared_ptr<AVCodecCallbackMock> cb)
{
    if (videoDec_ == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    return videoDec_->SetCallback(cb);
}

int32_t VideoDecSample::SetOutputSurface()
{
    if (videoDec_ == nullptr) {
        return AV_ERR_INVALID_VAL;
    }

    consumer_ = Surface::CreateSurfaceAsConsumer();
    sptr<IBufferConsumerListener> listener = new TestConsumerListener(consumer_, outSurfacePath_);
    consumer_->RegisterConsumerListener(listener);
    auto p = consumer_->GetProducer();
    producer_ = Surface::CreateSurfaceAsProducer(p);
    std::shared_ptr<SurfaceMock> surface = SurfaceMockFactory::CreateSurface(producer_);
    int32_t ret = videoDec_->SetOutputSurface(surface);
    isSurfaceMode_ = (ret == AV_ERR_OK);
    return ret;
}

int32_t VideoDecSample::Configure(std::shared_ptr<FormatMock> format)
{
    if (videoDec_ == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    return videoDec_->Configure(format);
}

int32_t VideoDecSample::Start()
{
    if (signal_ == nullptr || videoDec_ == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    FlushInner();
    signal_->isRunning_.store(true);

    inFile_ = std::make_unique<std::ifstream>();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(inFile_ != nullptr, AV_ERR_INVALID_VAL, "Fatal: No memory");
    inFile_->open(inPath_, std::ios::in | std::ios::binary);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(inFile_->is_open(), AV_ERR_INVALID_VAL, "inFile_ can not find");

    int32_t ret = AV_ERR_OK;
    time_ = chrono::time_point_cast<chrono::milliseconds>(chrono::system_clock::now()).time_since_epoch().count();
    inputLoop_ = make_unique<thread>(&VideoDecSample::InputLoopFunc, this);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(inputLoop_ != nullptr, AV_ERR_INVALID_VAL, "Fatal: InputLoopFunc fail");

    outputLoop_ = make_unique<thread>(&VideoDecSample::OutputLoopFunc, this);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(outputLoop_ != nullptr, AV_ERR_INVALID_VAL, "Fatal: OutputLoopFunc fail");
    ret = videoDec_->Start();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "Fatal: Start fail");
    StartInner();
    return ret;
}

int32_t VideoDecSample::Stop()
{
    FlushInner();
    if (videoDec_ == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    return videoDec_->Stop();
}

int32_t VideoDecSample::Flush()
{
    FlushInner();
    if (videoDec_ == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    return videoDec_->Flush();
}

int32_t VideoDecSample::Reset()
{
    FlushInner();
    if (videoDec_ == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    return videoDec_->Reset();
}

int32_t VideoDecSample::Release()
{
    FlushInner();
    if (videoDec_ == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    return videoDec_->Release();
}

std::shared_ptr<FormatMock> VideoDecSample::GetOutputDescription()
{
    if (videoDec_ == nullptr) {
        return nullptr;
    }
    return videoDec_->GetOutputDescription();
}

int32_t VideoDecSample::SetParameter(std::shared_ptr<FormatMock> format)
{
    if (videoDec_ == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    return videoDec_->SetParameter(format);
}

int32_t VideoDecSample::PushInputData(uint32_t index, OH_AVCodecBufferAttr &attr)
{
    if (videoDec_ == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    return videoDec_->PushInputData(index, attr);
}

int32_t VideoDecSample::RenderOutputData(uint32_t index)
{
    if (videoDec_ == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    return videoDec_->RenderOutputData(index);
}

int32_t VideoDecSample::FreeOutputData(uint32_t index)
{
    if (videoDec_ == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    return videoDec_->FreeOutputData(index);
}

bool VideoDecSample::IsValid()
{
    if (videoDec_ == nullptr) {
        return false;
    }
    return videoDec_->IsValid();
}

void VideoDecSample::SetOutPath(const std::string &path)
{
    outPath_ = path + ".yuv";
    outSurfacePath_ = path + ".rgba";
}

void VideoDecSample::SetSource(const std::string &path)
{
    inPath_ = path;
}

void VideoDecSample::FlushInner()
{
    if (signal_ == nullptr) {
        return;
    }
    signal_->isRunning_.store(false);
    if (inputLoop_ != nullptr && inputLoop_->joinable()) {
        unique_lock<mutex> queueLock(signal_->inMutex_);
        signal_->inIndexQueue_.push(10000); // push 10000 to stop queue
        signal_->inCond_.notify_all();
        queueLock.unlock();
        inputLoop_->join();
        inputLoop_.reset();
        std::queue<uint32_t> tempIndex;
        std::swap(tempIndex, signal_->inIndexQueue_);
        std::queue<std::shared_ptr<AVMemoryMock>> tempInBufferr;
        std::swap(tempInBufferr, signal_->inBufferQueue_);
    }
    if (outputLoop_ != nullptr && outputLoop_->joinable()) {
        unique_lock<mutex> lock(signal_->outMutex_);
        signal_->outIndexQueue_.push(10000); // push 10000 to stop queue
        signal_->outCond_.notify_all();
        lock.unlock();
        outputLoop_->join();
        outputLoop_.reset();
        std::queue<uint32_t> tempIndex;
        std::swap(tempIndex, signal_->outIndexQueue_);
        std::queue<OH_AVCodecBufferAttr> tempOutAttr;
        std::swap(tempOutAttr, signal_->outAttrQueue_);
        std::queue<std::shared_ptr<AVMemoryMock>> tempOutBufferr;
        std::swap(tempOutBufferr, signal_->outBufferQueue_);
    }
}

void VideoDecSample::StartInner()
{
    if (signal_ == nullptr) {
        return;
    }
    unique_lock<mutex> lock(signal_->mutex_);
    auto lck = [this]() { return !signal_->isRunning_.load(); };
    bool isTimeout = signal_->cond_.wait_for(lock, chrono::seconds(SAMPLE_TIMEOUT), lck);
    lock.unlock();
    int64_t tempTime =
        chrono::time_point_cast<chrono::milliseconds>(chrono::system_clock::now()).time_since_epoch().count();
    if (!isTimeout) {
        cout << "Start func timeout, time used: " << tempTime - time_ << "ms" << endl;
    } else {
        cout << "Start func finish, time used: " << tempTime - time_ << "ms" << endl;
    }
    FlushInner();
}

void VideoDecSample::InputLoopFunc()
{
    UNITTEST_CHECK_AND_RETURN_LOG(signal_ != nullptr, "Fatal: signal_ is null");
    UNITTEST_CHECK_AND_RETURN_LOG(videoDec_ != nullptr, "Fatal: videoDec_ is null");
    inFile_->read(reinterpret_cast<char *>(&datSize_), sizeof(int64_t));
    frameCount_ = 0;
    isFirstFrame_ = true;
    while (true) {
        UNITTEST_CHECK_AND_BREAK_LOG(signal_->isRunning_.load(), "Fatal: isRunning is false");
        unique_lock<mutex> lock(signal_->inMutex_);
        signal_->inCond_.wait(lock, [this]() { return signal_->inIndexQueue_.size() > 0; });
        UNITTEST_CHECK_AND_BREAK_LOG(signal_->isRunning_.load(), "InputLoopFunc stop");

        int32_t ret = InputLoopInner();
        UNITTEST_CHECK_AND_BREAK_LOG(ret == AV_ERR_OK, "Fatal: PushInputData fail, exit");

        frameCount_++;
        signal_->inIndexQueue_.pop();
        signal_->inBufferQueue_.pop();
    }
}

int32_t VideoDecSample::InputLoopInner()
{
    uint32_t index = signal_->inIndexQueue_.front();
    std::shared_ptr<AVMemoryMock> buffer = signal_->inBufferQueue_.front();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(buffer != nullptr, AV_ERR_INVALID_VAL, "Fatal: GetInputBuffer fail");
    UNITTEST_CHECK_AND_RETURN_RET_LOG(inFile_ != nullptr && inFile_->is_open(), AV_ERR_INVALID_VAL,
                                      "Fatal: open file fail");

    uint64_t bufferSize = 0;
    uint64_t bufferPts = 0;
    struct OH_AVCodecBufferAttr attr = {0, 0, 0, AVCODEC_BUFFER_FLAG_NONE};
    bool isOutOfLength = (frameCount_ >= datSize_ || frameCount_ >= EOS_COUNT);
    if (!isOutOfLength) {
        inFile_->read(reinterpret_cast<char *>(&bufferSize), sizeof(int64_t));
        inFile_->read(reinterpret_cast<char *>(&bufferPts), sizeof(int64_t));
        char *fileBuffer = static_cast<char *>(malloc(sizeof(char) * bufferSize + 1));
        UNITTEST_CHECK_AND_RETURN_RET_LOG(fileBuffer != nullptr, AV_ERR_INVALID_VAL, "Fatal: malloc fail.");
        (void)inFile_->read(fileBuffer, bufferSize);
        if (inFile_->eof() || memcpy_s(buffer->GetAddr(), buffer->GetSize(), fileBuffer, bufferSize) != EOK) {
            attr.flags = AVCODEC_BUFFER_FLAG_EOS;
        }
        free(fileBuffer);
    }
    if (isOutOfLength || attr.flags == AVCODEC_BUFFER_FLAG_EOS) {
        attr.flags = AVCODEC_BUFFER_FLAG_EOS;
        cout << "EOS Frame, frameCount = " << frameCount_ << endl;
        int32_t ret = videoDec_->PushInputData(index, attr);
        if (inFile_ != nullptr && inFile_->is_open()) {
            inFile_->close();
        }
        return ret;
    }
    if (isFirstFrame_) {
        attr.flags = AVCODEC_BUFFER_FLAG_CODEC_DATA;
        isFirstFrame_ = false;
    } else {
        attr.flags = AVCODEC_BUFFER_FLAG_NONE;
    }
    attr.size = bufferSize;
    attr.pts = bufferPts;
    return videoDec_->PushInputData(index, attr);
}

void VideoDecSample::OutputLoopFunc()
{
    UNITTEST_CHECK_AND_RETURN_LOG(signal_ != nullptr, "Fatal: signal_ is null");
    UNITTEST_CHECK_AND_RETURN_LOG(videoDec_ != nullptr, "Fatal: videoDec_ is null");
    if (!isSurfaceMode_) {
        outFile_ = std::make_unique<std::ofstream>();
        UNITTEST_CHECK_AND_RETURN_LOG(outFile_ != nullptr, "Fatal: No memory");
        outFile_->open(outPath_, std::ios::out | std::ios::binary | std::ios::ate);
        UNITTEST_CHECK_AND_RETURN_LOG(outFile_->is_open(), "outFile_ can not find");
    }
    while (true) {
        UNITTEST_CHECK_AND_BREAK_LOG(signal_->isRunning_.load(), "Fatal: isRunning is false");
        unique_lock<mutex> lock(signal_->outMutex_);
        signal_->outCond_.wait(lock, [this]() { return signal_->outIndexQueue_.size() > 0; });
        UNITTEST_CHECK_AND_BREAK_LOG(signal_->isRunning_.load(), "OutputLoopFunc stop");

        int32_t ret = OutputLoopInner();
        UNITTEST_CHECK_AND_BREAK_LOG(ret == AV_ERR_OK, "Fatal: OutputLoopInner fail, exit");

        signal_->outIndexQueue_.pop();
        signal_->outAttrQueue_.pop();
        signal_->outBufferQueue_.pop();
    }
    unique_lock<mutex> lock(signal_->mutex_);
    signal_->isRunning_.store(false);
    signal_->cond_.notify_all();
}

int32_t VideoDecSample::OutputLoopInner()
{
    struct OH_AVCodecBufferAttr attr = signal_->outAttrQueue_.front();
    uint32_t index = signal_->outIndexQueue_.front();
    uint32_t ret = AV_ERR_OK;
    auto buffer = signal_->outBufferQueue_.front();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(buffer != nullptr, AV_ERR_INVALID_VAL, "Fatal: GetOutputBuffer fail, exit");

    if (!isSurfaceMode_) {
        if (outFile_ != nullptr && NEED_DUMP) {
            if (!outFile_->is_open()) {
                cout << "output data fail" << endl;
            } else {
                outFile_->write(reinterpret_cast<char *>(buffer->GetAddr()), attr.size);
            }
        }
        if (index != EOS_COUNT) {
            ret = videoDec_->FreeOutputData(index);
            UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "Fatal: FreeOutputData fail index: %d", index);
        }
    } else {
        if (index != EOS_COUNT && videoDec_->RenderOutputData(index) != AV_ERR_OK) {
            ret = videoDec_->RenderOutputData(index);
            UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "Fatal: RenderOutputData fail index: %d", index);
        }
    }
    if (attr.flags == AVCODEC_BUFFER_FLAG_EOS) {
        if (!isSurfaceMode_ && outFile_ != nullptr && outFile_->is_open()) {
            outFile_->close();
        }
        cout << "Get EOS Frame, output func exit" << endl;
        unique_lock<mutex> lock(signal_->mutex_);
        signal_->isRunning_.store(false);
        signal_->cond_.notify_all();
        return AV_ERR_OK;
    }
    return AV_ERR_OK;
}
} // namespace MediaAVCodec
} // namespace OHOS
