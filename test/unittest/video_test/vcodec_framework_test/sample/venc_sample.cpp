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

#include "venc_sample.h"
#include <gtest/gtest.h>
#include "venc_sample.h"
// #ifdef VIDEOENC_CAPI_UNIT_TEST
// #include "surface_capi_mock.h"
// #else
// #include "display_type.h"
// #include "iconsumer_surface.h"
// #include "native_buffer_inner.h"
// #include "surface_inner_mock.h"
// #endif

using namespace std;
using namespace OHOS::MediaAVCodec::VCodecTestParam;
namespace OHOS {
namespace MediaAVCodec {
VEncCallbackTest::VEncCallbackTest(std::shared_ptr<VEncSignal> signal) : signal_(signal) {}

VEncCallbackTest::~VEncCallbackTest() {}

void VEncCallbackTest::OnError(int32_t errorCode)
{
    cout << "ADec Error errorCode=" << errorCode;
    if (signal_ == nullptr) {
        return;
    }
    signal_->errorNum_ += 1;
    cout << ", errorNum=" << signal_->errorNum_ << endl;
}

void VEncCallbackTest::OnStreamChanged(std::shared_ptr<FormatMock> format)
{
    (void)format;
    cout << "VEnc Format Changed" << endl;
}

void VEncCallbackTest::OnNeedInputData(uint32_t index, std::shared_ptr<AVMemoryMock> data)
{
    if (signal_ == nullptr) {
        return;
    }
    unique_lock<mutex> lock(signal_->inMutex_);
    if (!signal_->isRunning_.load()) {
        return;
    }
    signal_->inIndexQueue_.push(index);
    signal_->outMemoryQueue_.push(data);
    signal_->inCond_.notify_all();
}

void VEncCallbackTest::OnNewOutputData(uint32_t index, std::shared_ptr<AVMemoryMock> data, OH_AVCodecBufferAttr attr)
{
    if (signal_ == nullptr) {
        return;
    }
    unique_lock<mutex> lock(signal_->outMutex_);
    if (!signal_->isRunning_.load()) {
        return;
    }
    signal_->outIndexQueue_.push(index);
    signal_->outMemoryQueue_.push(data);
    signal_->outAttrQueue_.push(attr);
    signal_->outCond_.notify_all();
}

VEncCallbackTestExt::VEncCallbackTestExt(std::shared_ptr<VEncSignal> signal) : signal_(signal) {}

VEncCallbackTestExt::~VEncCallbackTestExt() {}

void VEncCallbackTestExt::OnError(int32_t errorCode)
{
    cout << "VEnc Error errorCode=" << errorCode;
    if (signal_ == nullptr) {
        return;
    }
    signal_->errorNum_ += 1;
    cout << ", errorNum=" << signal_->errorNum_ << endl;
}

void VEncCallbackTestExt::OnStreamChanged(std::shared_ptr<FormatMock> format)
{
    (void)format;
    cout << "VEnc Format Changed" << endl;
}

void VEncCallbackTestExt::OnNeedInputData(uint32_t index, std::shared_ptr<AVBufferMock> data)
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

void VEncCallbackTestExt::OnNewOutputData(uint32_t index, std::shared_ptr<AVBufferMock> data)
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
    signal_->outCond_.notify_all();
}

VideoEncSample::VideoEncSample(std::shared_ptr<VEncSignal> signal)
    : signal_(signal), outPath_("/data/test/media/320_240_yuv420p.yuv")
{
}

VideoEncSample::~VideoEncSample()
{
    if (videoEnc_ != nullptr) {
        (void)videoEnc_->Release();
    }
}

bool VideoEncSample::CreateVideoEncMockByMime(const std::string &mime)
{
    videoEnc_ = VCodecMockFactory::CreateVideoEncMockByMime(mime);
    return videoEnc_ != nullptr;
}

bool VideoEncSample::CreateVideoEncMockByName(const std::string &name)
{
    videoEnc_ = VCodecMockFactory::CreateVideoEncMockByName(name);
    return videoEnc_ != nullptr;
}

int32_t VideoEncSample::SetCallback(std::shared_ptr<AVCodecCallbackMock> cb)
{
    if (videoEnc_ == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    return videoEnc_->SetCallback(cb);
}

int32_t VideoEncSample::SetCallback(std::shared_ptr<VideoCodecCallbackMock> cb)
{
    if (videoEnc_ == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    return videoEnc_->SetCallback(cb);
}

int32_t VideoEncSample::Configure(std::shared_ptr<FormatMock> format)
{
    if (videoEnc_ == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    return videoEnc_->Configure(format);
}

int32_t VideoEncSample::Start()
{
    if (signal_ == nullptr || videoEnc_ == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    PrepareInner();
    int32_t ret = videoEnc_->Start();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "Fatal: Start fail");
    RunInner();
    return ret;
}

int32_t VideoEncSample::StartBuffer()
{
    if (signal_ == nullptr || videoEnc_ == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    PrepareInnerExt();
    int32_t ret = videoEnc_->Start();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "Fatal: Start fail");
    RunInner();
    return ret;
}

int32_t VideoEncSample::Stop()
{
    if (videoEnc_ == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    return videoEnc_->Stop();
}

int32_t VideoEncSample::Flush()
{
    if (videoEnc_ == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    return videoEnc_->Flush();
}

int32_t VideoEncSample::Reset()
{
    if (videoEnc_ == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    return videoEnc_->Reset();
}

int32_t VideoEncSample::Release()
{
    if (videoEnc_ == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    return videoEnc_->Release();
}

std::shared_ptr<FormatMock> VideoEncSample::GetOutputDescription()
{
    if (videoEnc_ == nullptr) {
        return nullptr;
    }
    return videoEnc_->GetOutputDescription();
}

int32_t VideoEncSample::SetParameter(std::shared_ptr<FormatMock> format)
{
    if (videoEnc_ == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    return videoEnc_->SetParameter(format);
}

int32_t VideoEncSample::NotifyEos()
{
    if (videoEnc_ == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    return videoEnc_->NotifyEos();
}

int32_t VideoEncSample::PushInputData(uint32_t index, OH_AVCodecBufferAttr &attr)
{
    if (videoEnc_ == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    return videoEnc_->PushInputData(index, attr);
}

int32_t VideoEncSample::FreeOutputData(uint32_t index)
{
    if (videoEnc_ == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    return videoEnc_->FreeOutputData(index);
}

int32_t VideoEncSample::PushInputBuffer(uint32_t index)
{
    if (videoEnc_ == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    return videoEnc_->PushInputBuffer(index);
}

int32_t VideoEncSample::FreeOutputBuffer(uint32_t index)
{
    if (videoEnc_ == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    return videoEnc_->FreeOutputBuffer(index);
}

bool VideoEncSample::IsValid()
{
    if (videoEnc_ == nullptr) {
        return false;
    }
    return videoEnc_->IsValid();
}

void VideoEncSample::SetOutPath(const std::string &path)
{
    outPath_ = path + ".dat";
}

void VideoEncSample::FlushInner()
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

void VideoEncSample::RunInner()
{
    if (signal_ == nullptr) {
        return;
    }
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

void VideoEncSample::PrepareInner()
{
    if (signal_ == nullptr) {
        return;
    }
    FlushInner();
    signal_->isRunning_.store(true);
    inFile_ = std::make_unique<std::ifstream>();
    ASSERT_NE(inFile_, nullptr);
    inFile_->open(inPath_, std::ios::in | std::ios::binary);
    ASSERT_TRUE(inFile_->is_open());
    time_ = chrono::time_point_cast<chrono::milliseconds>(chrono::system_clock::now()).time_since_epoch().count();
    inputLoop_ = make_unique<thread>(&VideoEncSample::InputLoopFunc, this);
    ASSERT_NE(inputLoop_, nullptr);
    outputLoop_ = make_unique<thread>(&VideoEncSample::OutputLoopFunc, this);
    ASSERT_NE(outputLoop_, nullptr);
}

void VideoEncSample::PrepareInnerExt()
{
    if (signal_ == nullptr) {
        return;
    }
    FlushInner();
    signal_->isRunning_.store(true);
    inFile_ = std::make_unique<std::ifstream>();
    ASSERT_NE(inFile_, nullptr);
    inFile_->open(inPath_, std::ios::in | std::ios::binary);
    ASSERT_TRUE(inFile_->is_open());
    time_ = chrono::time_point_cast<chrono::milliseconds>(chrono::system_clock::now()).time_since_epoch().count();
    inputLoop_ = make_unique<thread>(&VideoEncSample::InputLoopFuncExt, this);
    ASSERT_NE(inputLoop_, nullptr);
    outputLoop_ = make_unique<thread>(&VideoEncSample::OutputLoopFuncExt, this);
    ASSERT_NE(outputLoop_, nullptr);
}

void VideoEncSample::InputLoopFunc()
{
    ASSERT_NE(signal_, nullptr);
    ASSERT_NE(videoEnc_, nullptr);
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
        EXPECT_EQ(ret, AV_ERR_OK);
        UNITTEST_CHECK_AND_BREAK_LOG(ret == AV_ERR_OK, "Fatal: PushInputData fail, exit");

        frameInputCount_++;
        signal_->inIndexQueue_.pop();
        signal_->inMemoryQueue_.pop();
    }
}

int32_t VideoEncSample::InputLoopInner()
{
    uint32_t index = signal_->inIndexQueue_.front();
    std::shared_ptr<AVMemoryMock> buffer = signal_->inMemoryQueue_.front();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(buffer != nullptr, AV_ERR_INVALID_VAL, "Fatal: GetInputBuffer fail");

    uint64_t bufferSize = DEFAULT_WIDTH * DEFAULT_HEIGHT * 3 / 2;
    struct OH_AVCodecBufferAttr attr = {0, 0, 0, AVCODEC_BUFFER_FLAG_NONE};
    if (frameInputCount_ < EOS_COUNT_VENC) {
        char *fileBuffer = static_cast<char *>(malloc(sizeof(char) * bufferSize + 1));
        UNITTEST_CHECK_AND_RETURN_RET_LOG(fileBuffer != nullptr, AV_ERR_INVALID_VAL, "Fatal: malloc fail.");
        (void)inFile_->read(fileBuffer, bufferSize);
        if (inFile_->eof() || memcpy_s(buffer->GetAddr(), buffer->GetSize(), fileBuffer, bufferSize) != EOK) {
            attr.flags = AVCODEC_BUFFER_FLAG_EOS;
        }
        free(fileBuffer);
    }
    if (frameInputCount_ >= EOS_COUNT_VENC || attr.flags == AVCODEC_BUFFER_FLAG_EOS) {
        attr.flags = AVCODEC_BUFFER_FLAG_EOS;
        cout << "Input EOS Frame, frameCount = " << frameInputCount_ << endl;
        int32_t ret = PushInputData(index, attr);
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
    return PushInputData(index, attr);
}

void VideoEncSample::OutputLoopFunc()
{
    ASSERT_NE(signal_, nullptr);
    ASSERT_NE(videoEnc_, nullptr);
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
        EXPECT_EQ(ret, AV_ERR_OK);
        UNITTEST_CHECK_AND_BREAK_LOG(ret == AV_ERR_OK, "Fatal: OutputLoopInner fail, exit");

        signal_->outIndexQueue_.pop();
        signal_->outAttrQueue_.pop();
        signal_->outMemoryQueue_.pop();
    }
    unique_lock<mutex> lock(signal_->mutex_);
    signal_->isRunning_.store(false);
    signal_->cond_.notify_all();
}

int32_t VideoEncSample::OutputLoopInner()
{
    struct OH_AVCodecBufferAttr attr = signal_->outAttrQueue_.front();
    uint32_t index = signal_->outIndexQueue_.front();
    uint32_t ret = AV_ERR_OK;
    auto buffer = signal_->outMemoryQueue_.front();

    if (frameOutputCount_ != EOS_COUNT_VENC) {
        if (outFile_ != nullptr && isDump_ && !isSurfaceMode_) {
            if (!outFile_->is_open()) {
                cout << "output data fail" << endl;
            } else {
                UNITTEST_CHECK_AND_RETURN_RET_LOG(buffer != nullptr, AV_ERR_INVALID_VAL,
                                                  "Fatal: GetOutputBuffer fail, exit");
                outFile_->write(reinterpret_cast<char *>(buffer->GetAddr()), attr.size);
            }
        }
        ret = FreeOutputData(index);
        UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "Fatal: FreeOutputData fail index: %d", index);
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
        EXPECT_EQ(frameOutputCount_, frameInputCount_);
        signal_->isRunning_.store(false);
        signal_->cond_.notify_all();
        return AV_ERR_OK;
    }
    return AV_ERR_OK;
}

void VideoEncSample::OutputLoopFuncExt()
{
    ASSERT_NE(signal_, nullptr);
    ASSERT_NE(videoEnc_, nullptr);
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
        EXPECT_EQ(ret, AV_ERR_OK);
        UNITTEST_CHECK_AND_BREAK_LOG(ret == AV_ERR_OK, "Fatal: OutputLoopInnerExt fail, exit");

        signal_->outIndexQueue_.pop();
        signal_->outBufferQueue_.pop();
    }
    unique_lock<mutex> lock(signal_->mutex_);
    signal_->isRunning_.store(false);
    signal_->cond_.notify_all();
}

int32_t VideoEncSample::OutputLoopInnerExt()
{
    uint32_t index = signal_->outIndexQueue_.front();
    uint32_t ret = AV_ERR_OK;
    auto buffer = signal_->outBufferQueue_.front();

    struct OH_AVCodecBufferAttr attr = buffer->GetBufferAttr();
    if (frameOutputCount_ != EOS_COUNT_VENC) {
        if (outFile_ != nullptr && isDump_ && !isSurfaceMode_) {
            if (!outFile_->is_open()) {
                cout << "output data fail" << endl;
            } else {
                UNITTEST_CHECK_AND_RETURN_RET_LOG(buffer != nullptr, AV_ERR_INVALID_VAL,
                                                  "Fatal: GetOutputBuffer fail, exit");
                outFile_->write(reinterpret_cast<char *>(buffer->GetAddr()), attr.size);
            }
        }
        ret = FreeOutputBuffer(index);
        UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "Fatal: FreeOutputData fail index: %d", index);
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
        EXPECT_EQ(frameOutputCount_, frameInputCount_);
        signal_->isRunning_.store(false);
        signal_->cond_.notify_all();
        return AV_ERR_OK;
    }
    return AV_ERR_OK;
}

void VideoEncSample::InputLoopFuncExt()
{
    ASSERT_NE(signal_, nullptr);
    ASSERT_NE(videoEnc_, nullptr);
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
        EXPECT_EQ(ret, AV_ERR_OK);
        UNITTEST_CHECK_AND_BREAK_LOG(ret == AV_ERR_OK, "Fatal: PushInputData fail, exit");

        frameInputCount_++;
        signal_->inBufferQueue_.pop();
        signal_->inIndexQueue_.pop();
    }
}

int32_t VideoEncSample::InputLoopInnerExt()
{
    uint32_t index = signal_->inIndexQueue_.front();
    std::shared_ptr<AVBufferMock> buffer = signal_->inBufferQueue_.front();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(buffer != nullptr, AV_ERR_INVALID_VAL, "Fatal: GetInputBuffer fail");

    uint64_t bufferSize = DEFAULT_WIDTH * DEFAULT_HEIGHT * 3 / 2;
    struct OH_AVCodecBufferAttr attr = {0, 0, 0, AVCODEC_BUFFER_FLAG_NONE};
    if (frameInputCount_ < EOS_COUNT_VENC) {
        char *fileBuffer = static_cast<char *>(malloc(sizeof(char) * bufferSize + 1));
        UNITTEST_CHECK_AND_RETURN_RET_LOG(fileBuffer != nullptr, AV_ERR_INVALID_VAL, "Fatal: malloc fail.");
        (void)inFile_->read(fileBuffer, bufferSize);
        if (inFile_->eof() || memcpy_s(buffer->GetAddr(), bufferSize, fileBuffer, bufferSize) != EOK) {
            attr.flags = AVCODEC_BUFFER_FLAG_EOS;
        }
        free(fileBuffer);
    }
    if (frameInputCount_ >= EOS_COUNT_VENC || attr.flags == AVCODEC_BUFFER_FLAG_EOS) {
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
        attr.flags = AVCODEC_BUFFER_FLAG_CODEC_DATA;
        isFirstFrame_ = false;
    } else {
        attr.flags = AVCODEC_BUFFER_FLAG_NONE;
    }
    attr.size = bufferSize;
    buffer->SetBufferAttr(attr);
    return PushInputBuffer(index);
}
} // namespace MediaAVCodec
} // namespace OHOS
