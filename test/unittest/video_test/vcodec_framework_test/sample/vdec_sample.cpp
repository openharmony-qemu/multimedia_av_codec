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

namespace {
constexpr uint8_t AVCC_FRAME_HEAD_LEN = 4;
constexpr uint8_t OFFSET_8 = 8;
constexpr uint8_t OFFSET_16 = 16;
constexpr uint8_t OFFSET_24 = 24;
constexpr uint32_t NS_TO_US = 1000;         // nano second to micro second
constexpr uint32_t S_TO_US = 1000'000;      // second to micro second

static inline int64_t GetTimeUs()
{
    struct timespec now;
    (void)clock_gettime(CLOCK_BOOTTIME, &now);
    return ((int64_t)now.tv_sec * S_TO_US + (now.tv_nsec / NS_TO_US));
}
}

namespace OHOS {
namespace MediaAVCodec {
VDecCallbackTest::VDecCallbackTest(std::shared_ptr<VDecSignal> signal) : signal_(signal) {}

VDecCallbackTest::~VDecCallbackTest() {}

void VDecCallbackTest::OnError(int32_t errorCode)
{
    cout << "VDec Error errorCode=" << errorCode;
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
    if (!signal_->isRunning_.load() && !signal_->isPreparing_.load()) {
        return;
    }
    signal_->inIndexQueue_.push(index);
    signal_->inMemoryQueue_.push(data);
    signal_->inCond_.notify_all();
}

void VDecCallbackTest::OnNewOutputData(uint32_t index, std::shared_ptr<AVMemoryMock> data, OH_AVCodecBufferAttr attr)
{
    if (signal_ == nullptr) {
        return;
    }
    unique_lock<mutex> lock(signal_->outMutex_);
    if (!signal_->isRunning_.load() && !signal_->isPreparing_.load()) {
        return;
    }
    signal_->outIndexQueue_.push(index);
    signal_->outMemoryQueue_.push(data);
    signal_->outAttrQueue_.push(attr);
    signal_->outCond_.notify_all();
}

VDecCallbackTestExt::VDecCallbackTestExt(std::shared_ptr<VDecSignal> signal) : signal_(signal) {}

VDecCallbackTestExt::~VDecCallbackTestExt() {}

void VDecCallbackTestExt::OnError(int32_t errorCode)
{
    cout << "VDec Error errorCode=" << errorCode;
    if (signal_ == nullptr) {
        return;
    }
    signal_->errorNum_ += 1;
    cout << ", errorNum=" << signal_->errorNum_ << endl;
}

void VDecCallbackTestExt::OnStreamChanged(std::shared_ptr<FormatMock> format)
{
    (void)format;
    cout << "VDec Format Changed" << endl;
}

void VDecCallbackTestExt::OnNeedInputData(uint32_t index, std::shared_ptr<AVBufferMock> data)
{
    if (signal_ == nullptr) {
        return;
    }
    unique_lock<mutex> lock(signal_->inMutex_);
    if (!signal_->isRunning_.load() && !signal_->isPreparing_.load()) {
        return;
    }
    signal_->inIndexQueue_.push(index);
    signal_->inBufferQueue_.push(data);
    signal_->inCond_.notify_all();
}

void VDecCallbackTestExt::OnNewOutputData(uint32_t index, std::shared_ptr<AVBufferMock> data)
{
    if (signal_ == nullptr) {
        return;
    }
    unique_lock<mutex> lock(signal_->outMutex_);
    if (!signal_->isRunning_.load() && !signal_->isPreparing_.load()) {
        return;
    }
    signal_->outIndexQueue_.push(index);
    signal_->outBufferQueue_.push(data);
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
    consumer_ = nullptr;
    producer_ = nullptr;
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

int32_t VideoDecSample::SetCallback(std::shared_ptr<VideoCodecCallbackMock> cb)
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
    PrepareInner();
    int32_t ret = videoDec_->Start();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "Fatal: Start fail");
    RunInner();
    WaitForEos();
    return ret;
}

int32_t VideoDecSample::StartBuffer()
{
    if (signal_ == nullptr || videoDec_ == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    PrepareInner();
    int32_t ret = videoDec_->Start();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "Fatal: Start fail");
    RunInnerExt();
    WaitForEos();
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

int32_t VideoDecSample::PushInputBuffer(uint32_t index)
{
    if (videoDec_ == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    return videoDec_->PushInputBuffer(index);
}

int32_t VideoDecSample::RenderOutputBuffer(uint32_t index)
{
    if (videoDec_ == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    return videoDec_->RenderOutputBuffer(index);
}

int32_t VideoDecSample::FreeOutputBuffer(uint32_t index)
{
    if (videoDec_ == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    return videoDec_->FreeOutputBuffer(index);
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
        std::queue<uint32_t> tempIndex;
        std::swap(tempIndex, signal_->inIndexQueue_);
        std::queue<std::shared_ptr<AVMemoryMock>> tempInMemory;
        std::swap(tempInMemory, signal_->inMemoryQueue_);
        std::queue<std::shared_ptr<AVBufferMock>> tempInBuffer;
        std::swap(tempInBuffer, signal_->inBufferQueue_);
        queueLock.unlock();
        signal_->inCond_.notify_all();
        inputLoop_->join();

        frameInputCount_ = frameOutputCount_ = 0;
        inFile_ = std::make_unique<std::ifstream>();
        ASSERT_NE(inFile_, nullptr);
        inFile_->open(inPath_, std::ios::in | std::ios::binary);
        ASSERT_TRUE(inFile_->is_open());
    }
    if (outputLoop_ != nullptr && outputLoop_->joinable()) {
        unique_lock<mutex> lock(signal_->outMutex_);
        std::queue<uint32_t> tempIndex;
        std::swap(tempIndex, signal_->outIndexQueue_);
        std::queue<OH_AVCodecBufferAttr> tempOutAttr;
        std::swap(tempOutAttr, signal_->outAttrQueue_);
        std::queue<std::shared_ptr<AVMemoryMock>> tempOutMemory;
        std::swap(tempOutMemory, signal_->outMemoryQueue_);
        std::queue<std::shared_ptr<AVBufferMock>> tempOutBuffer;
        std::swap(tempOutBuffer, signal_->outBufferQueue_);
        lock.unlock();
        signal_->outCond_.notify_all();
        outputLoop_->join();
    }
}

void VideoDecSample::RunInner()
{
    if (signal_ == nullptr) {
        return;
    }
    signal_->isPreparing_.store(false);
    signal_->isRunning_.store(true);
    inputLoop_ = make_unique<thread>(&VideoDecSample::InputLoopFunc, this);
    ASSERT_NE(inputLoop_, nullptr);
    signal_->inCond_.notify_all();

    outputLoop_ = make_unique<thread>(&VideoDecSample::OutputLoopFunc, this);
    ASSERT_NE(outputLoop_, nullptr);
    signal_->outCond_.notify_all();
}

void VideoDecSample::RunInnerExt()
{
    if (signal_ == nullptr) {
        return;
    }
    signal_->isPreparing_.store(false);
    signal_->isRunning_.store(true);
    inputLoop_ = make_unique<thread>(&VideoDecSample::InputLoopFuncExt, this);
    ASSERT_NE(inputLoop_, nullptr);
    signal_->inCond_.notify_all();

    outputLoop_ = make_unique<thread>(&VideoDecSample::OutputLoopFuncExt, this);
    ASSERT_NE(outputLoop_, nullptr);
    signal_->outCond_.notify_all();
}

void VideoDecSample::WaitForEos()
{
    unique_lock<mutex> lock(signal_->mutex_);
    auto lck = [this]() { return !signal_->isRunning_.load(); };
    bool isNotTimeout = signal_->cond_.wait_for(lock, chrono::seconds(SAMPLE_TIMEOUT), lck);
    lock.unlock();
    int64_t tempTime =
        chrono::time_point_cast<chrono::milliseconds>(chrono::system_clock::now()).time_since_epoch().count();
    EXPECT_TRUE(isNotTimeout);
    if (!isNotTimeout) {
        cout << "Run func timeout, time used: " << tempTime - time_ << "ms" << endl;
    } else {
        cout << "Run func finish, time used: " << tempTime - time_ << "ms" << endl;
    }
    FlushInner();
}

void VideoDecSample::PrepareInner()
{
    if (signal_ == nullptr) {
        return;
    }
    FlushInner();
    signal_->isPreparing_.store(true);
    signal_->isRunning_.store(false);
    inFile_ = std::make_unique<std::ifstream>();
    ASSERT_NE(inFile_, nullptr);
    inFile_->open(inPath_, std::ios::in | std::ios::binary);
    ASSERT_TRUE(inFile_->is_open());
    time_ = chrono::time_point_cast<chrono::milliseconds>(chrono::system_clock::now()).time_since_epoch().count();
}

void VideoDecSample::InputLoopFunc()
{
    ASSERT_NE(signal_, nullptr);
    ASSERT_NE(videoDec_, nullptr);
    frameInputCount_ = 0;
    isFirstFrame_ = true;
    while (true) {
        UNITTEST_CHECK_AND_BREAK_LOG(signal_->isRunning_.load(), "InputLoopFunc stop running");
        unique_lock<mutex> lock(signal_->inMutex_);
        signal_->inCond_.wait(
            lock, [this]() { return (signal_->inIndexQueue_.size() > 0) || (!signal_->isRunning_.load()); });
        UNITTEST_CHECK_AND_BREAK_LOG(signal_->isRunning_.load(), "InputLoopFunc stop running");
        UNITTEST_CHECK_AND_BREAK_LOG(inFile_ != nullptr && inFile_->is_open(), "inFile_ is closed");

        int32_t ret = InputLoopInner();
        EXPECT_EQ(ret, AV_ERR_OK) << "frameInputCount_: " << frameInputCount_ << "\n";
        UNITTEST_CHECK_AND_BREAK_LOG(ret == AV_ERR_OK, "Fatal: PushInputData fail, exit");

        frameInputCount_++;
        signal_->inIndexQueue_.pop();
        signal_->inMemoryQueue_.pop();
    }
}

int32_t VideoDecSample::InputLoopInner()
{
    uint32_t index = signal_->inIndexQueue_.front();
    std::shared_ptr<AVMemoryMock> buffer = signal_->inMemoryQueue_.front();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(buffer != nullptr, AV_ERR_INVALID_VAL, "Fatal: GetInputBuffer fail, index: %d",
                                      index);
    struct OH_AVCodecBufferAttr attr = {0, 0, 0, AVCODEC_BUFFER_FLAG_NONE};
    
    char ch[4] = {};
    (void)inFile_->read(ch, AVCC_FRAME_HEAD_LEN);
    uint32_t bufferSize = (uint32_t)(((ch[3] & 0xFF)) | ((ch[2] & 0xFF) << OFFSET_8) | ((ch[1] & 0xFF) << OFFSET_16) |
                                     ((ch[0] & 0xFF) << OFFSET_24));
    (void)inFile_->read(reinterpret_cast<char *>(buffer->GetAddr() + AVCC_FRAME_HEAD_LEN), bufferSize);
    if (inFile_->eof()) {
        attr.flags = AVCODEC_BUFFER_FLAG_EOS;
        cout << "Input EOS Frame, frameCount = " << frameInputCount_ << endl;
        int32_t ret = PushInputData(index, attr);
        if (inFile_ != nullptr && inFile_->is_open()) {
            inFile_->close();
        }
        return ret;
    }
    if (isFirstFrame_) {
        attr.flags |= AVCODEC_BUFFER_FLAG_CODEC_DATA;
        isFirstFrame_ = false;
    }
    attr.size = bufferSize;
    attr.pts = GetTimeUs();
    return PushInputData(index, attr);
}

void VideoDecSample::OutputLoopFunc()
{
    ASSERT_NE(signal_, nullptr);
    ASSERT_NE(videoDec_, nullptr);
    if (isDump_) {
        outFile_ = std::make_unique<std::ofstream>();
        ASSERT_NE(outFile_, nullptr) << "Fatal: No memory";
        outFile_->open(outPath_, std::ios::out | std::ios::binary | std::ios::ate);
        ASSERT_TRUE(outFile_->is_open()) << "outFile_ can not find";
    }
    frameOutputCount_ = 0;
    while (true) {
        UNITTEST_CHECK_AND_BREAK_LOG(signal_->isRunning_.load(), "OutputLoopFunc stop running");
        unique_lock<mutex> lock(signal_->outMutex_);
        signal_->outCond_.wait(
            lock, [this]() { return (signal_->outIndexQueue_.size() > 0) || (!signal_->isRunning_.load()); });
        UNITTEST_CHECK_AND_BREAK_LOG(signal_->isRunning_.load(), "OutputLoopFunc stop running");

        int32_t ret = OutputLoopInner();
        EXPECT_EQ(ret, AV_ERR_OK) << "frameOutputCount_: " << frameOutputCount_ << "\n";
        UNITTEST_CHECK_AND_BREAK_LOG(ret == AV_ERR_OK, "Fatal: OutputLoopInner fail, exit");

        signal_->outIndexQueue_.pop();
        signal_->outAttrQueue_.pop();
        signal_->outMemoryQueue_.pop();
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
    auto buffer = signal_->outMemoryQueue_.front();

    if (frameOutputCount_ != EOS_COUNT) {
        if (attr.flags != AVCODEC_BUFFER_FLAG_EOS && !isSurfaceMode_ && isDump_) {
            if (!outFile_->is_open()) {
                cout << "output data fail" << endl;
            } else {
                UNITTEST_CHECK_AND_RETURN_RET_LOG(buffer != nullptr, AV_ERR_INVALID_VAL,
                                                  "Fatal: GetOutputBuffer fail, exit. index: %d", index);
                outFile_->write(reinterpret_cast<char *>(buffer->GetAddr()), attr.size);
            }
        }
        if (!isSurfaceMode_) {
            ret = FreeOutputData(index);
            UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "Fatal: FreeOutputData fail, index: %d", index);
        } else {
            ret = RenderOutputData(index);
            UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "Fatal: RenderOutputData fail, index: %d", index);
        }
    } else {
        cout << "Output EOS Frame, frameCount = " << frameOutputCount_ << endl;
    }
    ++frameOutputCount_;
    if (attr.flags == AVCODEC_BUFFER_FLAG_EOS) {
        if (!isSurfaceMode_ && outFile_ != nullptr && outFile_->is_open()) {
            outFile_->close();
        }
        cout << "Get EOS Frame, output func exit" << endl;
        unique_lock<mutex> lock(signal_->mutex_);
        EXPECT_LE(frameOutputCount_, frameInputCount_);
        signal_->isRunning_.store(false);
        signal_->cond_.notify_all();
        return AV_ERR_OK;
    }
    return AV_ERR_OK;
}

void VideoDecSample::OutputLoopFuncExt()
{
    ASSERT_NE(signal_, nullptr);
    ASSERT_NE(videoDec_, nullptr);
    if (isDump_) {
        outFile_ = std::make_unique<std::ofstream>();
        ASSERT_NE(outFile_, nullptr) << "Fatal: No memory";
        outFile_->open(outPath_, std::ios::out | std::ios::binary | std::ios::ate);
        ASSERT_TRUE(outFile_->is_open()) << "outFile_ can not find";
    }
    frameOutputCount_ = 0;
    while (true) {
        UNITTEST_CHECK_AND_BREAK_LOG(signal_->isRunning_.load(), "OutputLoopFunc stop running");
        unique_lock<mutex> lock(signal_->outMutex_);
        signal_->outCond_.wait(
            lock, [this]() { return (signal_->outIndexQueue_.size() > 0) || (!signal_->isRunning_.load()); });
        UNITTEST_CHECK_AND_BREAK_LOG(signal_->isRunning_.load(), "OutputLoopFunc stop running");
        int32_t ret = OutputLoopInnerExt();
        EXPECT_EQ(ret, AV_ERR_OK) << "frameOutputCount_: " << frameOutputCount_ << "\n";
        UNITTEST_CHECK_AND_BREAK_LOG(ret == AV_ERR_OK, "Fatal: OutputLoopInnerExt fail, exit");

        signal_->outIndexQueue_.pop();
        signal_->outBufferQueue_.pop();
    }
    unique_lock<mutex> lock(signal_->mutex_);
    signal_->isRunning_.store(false);
    signal_->cond_.notify_all();
}

int32_t VideoDecSample::OutputLoopInnerExt()
{
    uint32_t index = signal_->outIndexQueue_.front();
    uint32_t ret = AV_ERR_OK;
    auto buffer = signal_->outBufferQueue_.front();

    struct OH_AVCodecBufferAttr attr = buffer->GetBufferAttr();
    if (frameOutputCount_ != EOS_COUNT) {
        if ((attr.flags != AVCODEC_BUFFER_FLAG_EOS) && !isSurfaceMode_ && isDump_) {
            if (!outFile_->is_open()) {
                cout << "output data fail" << endl;
            } else {
                UNITTEST_CHECK_AND_RETURN_RET_LOG(buffer != nullptr, AV_ERR_INVALID_VAL,
                                                  "Fatal: GetOutputBuffer fail, exit, index: %d", index);
                char *bufferAddr = reinterpret_cast<char *>(buffer->GetAddr());
                UNITTEST_CHECK_AND_RETURN_RET_LOG(bufferAddr != nullptr, AV_ERR_INVALID_VAL,
                                                  "Fatal: GetOutputBuffer fail, exit, index: %d", index);
                outFile_->write(bufferAddr, attr.size);
            }
        }
        if (!isSurfaceMode_) {
            ret = FreeOutputBuffer(index);
            UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "Fatal: FreeOutputData fail index: %d", index);
        } else {
            ret = RenderOutputBuffer(index);
            UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "Fatal: RenderOutputData fail index: %d", index);
        }
    } else {
        cout << "Output EOS Frame, frameCount = " << frameOutputCount_ << endl;
    }
    ++frameOutputCount_;
    if (attr.flags == AVCODEC_BUFFER_FLAG_EOS) {
        if (!isSurfaceMode_ && outFile_ != nullptr && outFile_->is_open()) {
            outFile_->close();
        }
        cout << "Get EOS Frame, output func exit" << endl;
        unique_lock<mutex> lock(signal_->mutex_);
        EXPECT_LE(frameOutputCount_, frameInputCount_);
        signal_->isRunning_.store(false);
        signal_->cond_.notify_all();
        return AV_ERR_OK;
    }
    return AV_ERR_OK;
}

void VideoDecSample::InputLoopFuncExt()
{
    ASSERT_NE(signal_, nullptr);
    ASSERT_NE(videoDec_, nullptr);
    frameInputCount_ = 0;
    isFirstFrame_ = true;
    while (true) {
        UNITTEST_CHECK_AND_BREAK_LOG(signal_->isRunning_.load(), "InputLoopFunc stop running");
        unique_lock<mutex> lock(signal_->inMutex_);
        signal_->inCond_.wait(
            lock, [this]() { return (signal_->inBufferQueue_.size() > 0) || (!signal_->isRunning_.load()); });
        UNITTEST_CHECK_AND_BREAK_LOG(signal_->isRunning_.load(), "InputLoopFunc stop running");
        UNITTEST_CHECK_AND_BREAK_LOG(inFile_ != nullptr && inFile_->is_open(), "inFile_ is closed");

        int32_t ret = InputLoopInnerExt();
        EXPECT_EQ(ret, AV_ERR_OK) << "frameInputCount_: " << frameInputCount_ << "\n";
        UNITTEST_CHECK_AND_BREAK_LOG(ret == AV_ERR_OK, "Fatal: PushInputData fail, exit");

        frameInputCount_++;
        signal_->inBufferQueue_.pop();
        signal_->inIndexQueue_.pop();
    }
}

int32_t VideoDecSample::InputLoopInnerExt()
{
    uint32_t index = signal_->inIndexQueue_.front();
    std::shared_ptr<AVBufferMock> buffer = signal_->inBufferQueue_.front();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(buffer != nullptr, AV_ERR_INVALID_VAL, "Fatal: GetInputBuffer fail, index: %d",
                                      index);

    struct OH_AVCodecBufferAttr attr = {0, 0, 0, AVCODEC_BUFFER_FLAG_NONE};
    char ch[4] = {};
    (void)inFile_->read(ch, AVCC_FRAME_HEAD_LEN);
    uint32_t bufferSize = (uint32_t)(((ch[3] & 0xFF)) | ((ch[2] & 0xFF) << OFFSET_8) | ((ch[1] & 0xFF) << OFFSET_16) |
                                     ((ch[0] & 0xFF) << OFFSET_24));
    (void)inFile_->read(reinterpret_cast<char *>(buffer->GetAddr() + AVCC_FRAME_HEAD_LEN), bufferSize);
    if (inFile_->eof()) {
        attr.flags = AVCODEC_BUFFER_FLAG_EOS;
        cout << "Input EOS Frame, frameCount = " << frameInputCount_ << endl;
        buffer->SetBufferAttr(attr);
        int32_t ret = PushInputBuffer(index);
        if (inFile_ != nullptr && inFile_->is_open()) {
            inFile_->close();
        }
        return ret;
    }
    if (isFirstFrame_) {
        attr.flags |= AVCODEC_BUFFER_FLAG_CODEC_DATA;
        isFirstFrame_ = false;
    }
    attr.size = bufferSize;
    attr.pts = GetTimeUs();
    buffer->SetBufferAttr(attr);
    return PushInputBuffer(index);
}
} // namespace MediaAVCodec
} // namespace OHOS
