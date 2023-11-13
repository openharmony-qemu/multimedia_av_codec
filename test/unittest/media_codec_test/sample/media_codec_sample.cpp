
/*
 * Copyright (c) 2023-2023 Huawei Device Co., Ltd.
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

#include "media_codec_sample.h"
#include <iomanip>                // TODO
#include <iostream>               // TODO
#include "av_shared_memory_ext.h" // TODO
#include "meta/meta.h"
#include "plugin/codec_plugin.h"
#include "plugin/plugin_event.h"
#include "unittest_log.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::Media;

namespace {
constexpr int32_t EOS_COUNT = 100;
constexpr int32_t SAMPLE_TIMEOUT = 1000;
} // namespace

namespace OHOS {
namespace MediaAVCodec {

void CodecSampleCallback::OnError(AVCodecErrorType errorType, int32_t errorCode)
{
    auto codec = codec_.lock();
    if (!codec->isRunning_ || codec->isReleased_) {
        return;
    }
    (void)errorType;
    EXPECT_EQ(errorCode, 0) << "ADec Error errorCode=" << errorCode;
    errorNum_ += 1;
    std::cout << ", errorNum=" << errorNum_ << std::endl;
}

void CodecSampleCallback::OnOutputFormatChanged(const std::shared_ptr<Format> &format)
{
    auto codec = codec_.lock();
    if (!codec->isRunning_ || codec->isReleased_) {
        return;
    }
    std::cout << "VDec Format Changed" << std::endl;
}

void CodecSampleCallback::OnSurfaceModeDataFilled(std::shared_ptr<AVBuffer> &buffer)
{
    auto codec = codec_.lock();
    if (!codec->isRunning_ || codec->isReleased_) {
        return;
    }
    codec->SetSurfaceModeBuffer(buffer);
}

void ConsumerCallback::OnBufferAvailable()
{
    auto codec = codec_.lock();
    if (!codec->isRunning_ || codec->isReleased_) {
        return;
    }
    codec->OutputLoopFuncBuffer();
}

class TestConsumerListener : public IBufferConsumerListener {
public:
    TestConsumerListener(sptr<Surface> cs, std::string_view name);
    ~TestConsumerListener();
    void OnBufferAvailable() override;

private:
    int64_t timestamp_ = 0;
    Rect damage_ = {};
    sptr<Surface> cs_ = nullptr;
    std::unique_ptr<std::ofstream> outFile_;
};

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
    // static std::atomic<int32_t> frameCount = 0;
    // std::cout << "out surface frameCount = " << frameCount++ << "\n";
    sptr<SurfaceBuffer> buffer;
    int32_t flushFence;

    cs_->AcquireBuffer(buffer, flushFence, timestamp_, damage_);

    (void)outFile_->write(reinterpret_cast<char *>(buffer->GetVirAddr()), buffer->GetSize());
    cs_->ReleaseBuffer(buffer, -1);
}

CodecSample::CodecSample() {}

CodecSample::~CodecSample()
{
    isReleased_.store(true);
    outCond_.notify_all();
    inCond_.notify_all();
    cond_.notify_all();
    if (inputLoop_ != nullptr) {
        inputLoop_->detach();
    }
    if (outputLoop_ != nullptr) {
        outputLoop_->detach();
    }
    if (inFile_ != nullptr && inFile_->is_open()) {
        inFile_->close();
    }
    if (outFile_ != nullptr && outFile_->is_open()) {
        outFile_->close();
    }
}

bool CodecSample::CreateByName(const std::string &name)
{
    codec_ = std::make_shared<MediaCodec>();
    Status ret = codec_->Init(name);
    if (ret != Status::OK) {
        codec_ = nullptr;
        std::cout << "Init Media codec by name failed"
                  << "\n";
        return false;
    }
    return true;
}

bool CodecSample::CreateByMime(const std::string &mime)
{
    codec_ = std::make_shared<MediaCodec>();
    Status ret = codec_->Init(mime, false);
    if (ret != Status::OK) {
        codec_ = nullptr;
        std::cout << "Init Media codec by mime failed"
                  << "\n";
        return false;
    }
    return true;
}

Status CodecSample::Configure(const std::shared_ptr<Format> &meta)
{
    return codec_->Configure(meta);
}

Status CodecSample::SetCodecCallback(std::shared_ptr<CodecCallback> &codecCallback)
{
    return codec_->SetCodecCallback(codecCallback);
}

Status CodecSample::SetOutputBufferQueue(std::shared_ptr<AVBufferQueue> &bufferQueue)
{
    if (bufferQueue == nullptr) {
        outputPd_ = nullptr;
        return codec_->SetOutputBufferQueue(outputPd_);
    }
    outputPd_ = bufferQueue->GetProducer();
    outputCs_ = bufferQueue->GetConsumer();

    sptr<IConsumerListener> csListener = new ConsumerCallback(shared_from_this());
    outputCs_->SetBufferAvailableListener(csListener);

    Status ret = codec_->SetOutputBufferQueue(outputPd_);
    outCond_.notify_all();
    return ret;
}

Status CodecSample::SetOutputSurface()
{
    consumer_ = Surface::CreateSurfaceAsConsumer();
    sptr<IBufferConsumerListener> listener = new TestConsumerListener(consumer_, outSurfacePath_);
    consumer_->RegisterConsumerListener(listener);
    auto p = consumer_->GetProducer();
    producer_ = Surface::CreateSurfaceAsProducer(p);
    Status ret = codec_->SetOutputSurface(producer_);
    isSurfaceMode_ = (ret == Status::OK);
    return ret;
}

Status CodecSample::Prepare()
{
    return codec_->Prepare();
}

sptr<AVBufferQueueProducer> CodecSample::GetInputBufferQueue()
{
    inputPd_ = codec_->GetInputBufferQueue();
    inCond_.notify_all();
    return inputPd_;
}

sptr<Surface> CodecSample::GetInputSurface()
{
    return codec_->GetInputSurface();
}

bool CodecSample::BeforeStart()
{
    outFile_ = std::make_unique<std::ofstream>();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(outFile_ != nullptr, false, "Fatal: No memory");
    outFile_->open(outPath_, std::ios::out | std::ios::binary | std::ios::ate);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(outFile_->is_open(), false, "outFile_ can not find");
    frameOutputCount_ = 0;

    inFile_ = std::make_unique<std::ifstream>();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(inFile_ != nullptr, false, "Fatal: No memory");
    inFile_->open(inPath_, std::ios::in | std::ios::binary);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(inFile_->is_open(), false, "inFile_ can not find");
    frameInputCount_ = 0;
    inFile_->read(reinterpret_cast<char *>(&datSize_), sizeof(int64_t));

    UNITTEST_CHECK_AND_RETURN_RET_LOG(!isReleased_.load(), true, "codec released, ignore loopfunc");
    ReStartLoopFunc();
    time_ = chrono::time_point_cast<chrono::milliseconds>(chrono::system_clock::now()).time_since_epoch().count();
    return true;
}

void CodecSample::ReStartLoopFunc()
{
    if (inputLoop_ != nullptr && inputLoop_->joinable()) {
        inCond_.notify_all();
        inputLoop_->join();
    }
    if (outputLoop_ != nullptr && outputLoop_->joinable()) {
        {
            std::unique_lock<mutex> queueLock(outMutex_);
            outBufferQueue_.push(nullptr);
        }
        outCond_.notify_all();
        outputLoop_->join();
        {
            std::unique_lock<mutex> queueLock(outMutex_);
            std::queue<std::shared_ptr<AVBuffer>> tempQueue;
            std::swap(tempQueue, outBufferQueue_);
        }
    }

    if (isEncoder_) {
        if (isSurfaceMode_) {
            inputLoop_ = make_unique<thread>(&CodecSample::InputLoopFuncSurface, this);
        } else {
            inputLoop_ = make_unique<thread>(&CodecSample::InputLoopFuncBuffer, this);
        }
        // outputLoop_ = make_unique<thread>(&CodecSample::OutputLoopFuncBuffer, this);
    } else {
        inputLoop_ = make_unique<thread>(&CodecSample::InputLoopFuncBuffer, this);
        if (isSurfaceMode_) {
            outputLoop_ = make_unique<thread>(&CodecSample::OutputLoopFuncSurface, this);
        }
        // else {
        //     outputLoop_ = make_unique<thread>(&CodecSample::OutputLoopFuncBuffer, this);
        // }
    }
}

Status CodecSample::Start()
{
    isRunning_.store(false);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(CodecSample::BeforeStart(), Status::ERROR_UNKNOWN, "BeforeStart failed");

    Status ret = codec_->Start();
    isRunning_.store(true);
    inCond_.notify_all();
    outCond_.notify_all();

    UNITTEST_CHECK_AND_RETURN_RET_LOG(CodecSample::AfterStart(), Status::ERROR_UNKNOWN, "AfterStart failed");
    return ret;
}

bool CodecSample::AfterStart()
{
    unique_lock<mutex> lock(mutex_);
    using namespace chrono;
    auto lck = [this]() { return isReleased_.load() || !isRunning_.load(); };
    bool isNotTimeout = cond_.wait_for(lock, seconds(SAMPLE_TIMEOUT), lck);
    lock.unlock();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(!isReleased_.load(), true, "codec released, ignore loopfunc");

    int64_t tempTime = time_point_cast<milliseconds>(system_clock::now()).time_since_epoch().count();
    isRunning_.store(false);
    if (!isNotTimeout) {
        cout << "Run func timeout, time used: " << tempTime - time_ << "ms" << endl;
        return false;
    } else {
        cout << "Run func finish, time used: " << tempTime - time_ << "ms" << endl;
        return true;
    }
}

Status CodecSample::Stop()
{
    return codec_->Stop();
}

Status CodecSample::Flush()
{
    return codec_->Flush();
}

Status CodecSample::Reset()
{
    return codec_->Reset();
}

Status CodecSample::Release()
{
    return codec_->Release();
}

Status CodecSample::SetParameter(const std::shared_ptr<Format> &parameter)
{
    return codec_->SetParameter(parameter);
}

std::shared_ptr<Format> CodecSample::GetOutputFormat()
{
    return codec_->GetOutputFormat();
}

Status CodecSample::SurfaceModeReturnBuffer(std::shared_ptr<AVBuffer> &buffer, bool available)
{

    return codec_->SurfaceModeReturnBuffer(buffer, available);
}

Status CodecSample::NotifyEOS()
{
    return codec_->NotifyEOS();
}

void CodecSample::SetOutPath(const std::string &path)
{
    outPath_ = path + "_buffer.dat";
    outSurfacePath_ = path + "_surface.dat";
}

void CodecSample::SetInPath(const std::string &path)
{
    inPath_ = path;
}

void CodecSample::SetIsEncoder(bool isEncoder)
{
    isEncoder_ = isEncoder;
}

void CodecSample::InputLoopFuncSurface()
{
    // {
    //     std::unique_lock<std::mutex> lock(inMutex_);
    //     inCond_.wait(lock, [this]() { return isReleased_.load() || (!isReleased_.load() && (inputPd_ != nullptr));
    // UNITTEST_CHECK_AND_RETURN_LOG(!isReleased_.load(), "codec released, ignore loopfunc");
    // }
    // uint64_t bufferSize = 0;
    // uint64_t bufferPts = 0;
    // while (frameInputCount_ != EOS_COUNT && frameInputCount_ < datSize_) {
    //     UNITTEST_CHECK_AND_CONTINUE_LOG(isRunning_.load(), "Waitting for running");
    //     inFile_->read(reinterpret_cast<char *>(&bufferSize), sizeof(int64_t));
    //     inFile_->read(reinterpret_cast<char *>(&bufferPts), sizeof(int64_t));

    //     std::shared_ptr<AVBuffer> buffer = nullptr;
    //     ASSERT_EQ(Status::OK, inputPd_->RequestBuffer(buffer,
    //                                                   {.size = bufferSize,
    //                                                    .memoryType = MemoryType::SHARED_MEMORY,
    //                                                    .memoryFlag = MemoryFlag::MEMORY_READ_WRITE},
    //                                                   -1));
    // UNITTEST_CHECK_AND_RETURN_LOG(!isReleased_.load(), "codec released, ignore loopfunc");
    //     ASSERT_NE(buffer, nullptr);
    //     ASSERT_NE(buffer->memory_, nullptr);
    //     ASSERT_NE(buffer->memory_->GetAddr(), nullptr);

    //     inFile_->read(reinterpret_cast<char *>(buffer->memory_->GetAddr()), bufferSize);
    //     buffer->memory_->SetSize(bufferSize);
    //     buffer->pts_ = bufferPts;
    //     buffer->flag_ = AVCODEC_BUFFER_FLAG_NONE;

    //     std::cout << "in == LoopId: " << std::hex << (buffer->GetUniqueId() >> 48) << "\n"
    //               << "in == LoopframeInputCount_: " << std::dec << frameInputCount_ << "\n"
    //               << "in == LoopSize: " << std::dec << buffer->memory_->GetSize() << "\n\n";
    //     ++frameInputCount_;
    //     if (frameInputCount_ == 1) {
    //         buffer->flag_ = AVCODEC_BUFFER_FLAG_CODEC_DATA;
    //     }
    //     if (frameInputCount_ == EOS_COUNT || frameInputCount_ == datSize_) {
    //         buffer->flag_ = AVCODEC_BUFFER_FLAG_EOS;
    //     }
    //     inputPd_->PushBuffer(buffer, true);
    // }
}

void CodecSample::InputLoopFuncBuffer()
{
    {
        std::unique_lock<std::mutex> lock(inMutex_);
        inCond_.wait(lock, [this]() { return isReleased_.load() || (isRunning_.load() && (inputPd_ != nullptr)); });
        UNITTEST_CHECK_AND_RETURN_LOG(!isReleased_.load(), "codec released, ignore loopfunc");
    }
    uint64_t bufferSize = 0;
    uint64_t bufferPts = 0;
    while (frameInputCount_ != EOS_COUNT && frameInputCount_ < datSize_) {
        inFile_->read(reinterpret_cast<char *>(&bufferSize), sizeof(int64_t));
        inFile_->read(reinterpret_cast<char *>(&bufferPts), sizeof(int64_t));

        std::shared_ptr<AVBuffer> buffer = nullptr;
        AVBufferConfig config;
        config.size = bufferSize;
        config.memoryFlag = MemoryFlag::MEMORY_READ_WRITE;
        ASSERT_EQ(Status::OK, inputPd_->RequestBuffer(buffer, config, -1));
        UNITTEST_CHECK_AND_RETURN_LOG(!isReleased_.load(), "codec released, ignore loopfunc");
        ASSERT_NE(buffer, nullptr);
        ASSERT_NE(buffer->memory_, nullptr);
        ASSERT_NE(buffer->memory_->GetAddr(), nullptr);

        inFile_->read(reinterpret_cast<char *>(buffer->memory_->GetAddr()), bufferSize);
        buffer->memory_->SetSize(bufferSize);
        buffer->pts_ = bufferPts;
        buffer->flag_ = AVCODEC_BUFFER_FLAG_NONE;

        // std::cout << "in == LoopId: " << std::hex << (buffer->GetUniqueId() >> 48) << "\n"
        //           << "in == LoopframeInputCount_: " << std::dec << frameInputCount_ << "\n"
        //           << "in == LoopSize: " << std::dec << buffer->memory_->GetSize() << "\n\n";
        if (frameInputCount_ == 1) {
            buffer->flag_ = AVCODEC_BUFFER_FLAG_CODEC_DATA;
        }
        if (frameInputCount_ == EOS_COUNT || frameInputCount_ == datSize_) {
            buffer->flag_ = AVCODEC_BUFFER_FLAG_EOS;
        }
        ++frameInputCount_;
        inputPd_->PushBuffer(buffer, true);
    }
}

void CodecSample::SetSurfaceModeBuffer(std::shared_ptr<AVBuffer> &buffer)
{
    std::unique_lock<std::mutex> lock(outMutex_);
    outBufferQueue_.push(buffer);
    outCond_.notify_all();
}

void CodecSample::OutputLoopFuncSurface()
{
    std::shared_ptr<AVBuffer> buffer = nullptr;
    while (frameOutputCount_ != EOS_COUNT && frameOutputCount_ < datSize_) {
        {
            std::unique_lock<std::mutex> lock(outMutex_);
            outCond_.wait(lock, [this]() { return (outBufferQueue_.size() > 0) && isRunning_.load(); });
            // UNITTEST_CHECK_AND_BREAK_LOG(isRunning_.load(), "OutputLoopFuncSurface stop running");
            UNITTEST_CHECK_AND_BREAK_LOG(!isReleased_.load(), "codec released, ignore loopfunc");
            buffer = outBufferQueue_.front();
            outBufferQueue_.pop();
        }
        UNITTEST_CHECK_AND_BREAK_LOG(buffer != nullptr, "OutputLoopFuncSurface stop running");
        ASSERT_NE(buffer->memory_, nullptr);
        ASSERT_NE(buffer->memory_->GetAddr(), nullptr);
        // std::cout << "==============OutputLoopFunc============ \n"
        //           << "out == LoopId: " << std::hex << (buffer->GetUniqueId() >> 48) << "\n"
        //           << "out == Loop frameCount = " << std::dec << frameOutputCount_ << "\n"
        //           << "out == LoopSize: " << buffer->memory_->GetSize() << "\n\n";
        ++frameOutputCount_;
        bool available = true; //(frameOutputCount_ % 2) == 0;
        codec_->SurfaceModeReturnBuffer(buffer, available);
        if (buffer->flag_ == AVCODEC_BUFFER_FLAG_EOS || frameOutputCount_ == EOS_COUNT ||
            frameOutputCount_ >= datSize_) {
            break;
        }
    }
    unique_lock<std::mutex> lockStart(mutex_);
    isRunning_.store(false);
    cond_.notify_all();
}

void CodecSample::OutputLoopFuncBuffer()
{
    std::unique_lock<std::mutex> lock(outMutex_);
    UNITTEST_CHECK_AND_RETURN_LOG(frameOutputCount_ != EOS_COUNT && frameOutputCount_ < datSize_, "End of file stream");
    UNITTEST_CHECK_AND_RETURN_LOG(isRunning_.load(), "Waitting for running");
    UNITTEST_CHECK_AND_RETURN_LOG(!isReleased_.load(), "codec released, ignore loopfunc");
    std::shared_ptr<AVBuffer> buffer = nullptr;
    Status ret = outputCs_->AcquireBuffer(buffer);
    UNITTEST_CHECK_AND_RETURN_LOG(ret == Status::OK, "Waitting for output buffer");
    ASSERT_NE(buffer, nullptr);
    ASSERT_NE(buffer->memory_, nullptr);
    ASSERT_NE(buffer->memory_->GetAddr(), nullptr);
    // std::cout << "==============OutputLoopFunc============ \n"
    //           << "out == LoopId: " << std::hex << (buffer->GetUniqueId() >> 48) << "\n"
    //           << "out == Loop frameCount = " << frameOutputCount_ << "\n"
    //           << "out == LoopSize: " << std::dec << buffer->memory_->GetSize() << "\n\n";

    outFile_->write(reinterpret_cast<char *>(buffer->memory_->GetAddr()), buffer->memory_->GetSize());

    outputCs_->ReleaseBuffer(buffer);
    if (buffer->flag_ == AVCODEC_BUFFER_FLAG_EOS || frameOutputCount_ == EOS_COUNT || frameOutputCount_ >= datSize_) {
        unique_lock<std::mutex> lockStart(mutex_);
        isRunning_.store(false);
        cond_.notify_all();
    }
    ++frameOutputCount_;
}
} // namespace MediaAVCodec
} // namespace OHOS