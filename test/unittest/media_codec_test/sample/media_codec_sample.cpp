
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
constexpr int32_t SAMPLE_TIMEOUT = 10;
} // namespace

namespace OHOS {
namespace MediaAVCodec {

void CodecSampleCallback::OnError(AVCodecErrorType errorType, int32_t errorCode)
{
    (void)errorType;
    EXPECT_EQ(errorCode, 0) << "ADec Error errorCode=" << errorCode;
    errorNum_ += 1;
    cout << ", errorNum=" << errorNum_ << endl;
}

void CodecSampleCallback::OnOutputFormatChanged(const std::shared_ptr<Meta> &format)
{
    cout << "VDec Format Changed" << endl;
    auto codec = codec_.lock();
    codec->SetParameter(format);
}

void CodecSampleCallback::OnSurfaceModeDataFilled(std::shared_ptr<AVBuffer> &buffer)
{
    auto codec = codec_.lock();
    codec->SurfaceModeReturnBuffer(buffer, frameCount_ % 2 == 0);
    ++frameCount_;
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
    sptr<SurfaceBuffer> buffer;
    int32_t flushFence;

    cs_->AcquireBuffer(buffer, flushFence, timestamp_, damage_);

    (void)outFile_->write(reinterpret_cast<char *>(buffer->GetVirAddr()), buffer->GetSize());
    cs_->ReleaseBuffer(buffer, -1);
}

CodecSample::CodecSample()
{

}

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
        std::cout << "Init Media codec by name failed" << std::endl;
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
        std::cout << "Init Media codec by mime failed" << std::endl;
        return false;
    }
    return true;
}

Status CodecSample::Configure(const std::shared_ptr<Meta> &meta)
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
    Status ret = codec_->SetOutputBufferQueue(outputPd_);
    outCond_.notify_all();
    return ret;
}

Status CodecSample::SetOutputSurface(sptr<Surface> &surface)
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

std::shared_ptr<AVBufferQueueProducer> CodecSample::GetInputBufferQueue()
{
    inputPd_ = codec_->GetInputBufferQueue();
    inCond_.notify_all();
    return inputPd_;
}

sptr<Surface> CodecSample::GetInputSurface()
{
    return codec_->GetInputSurface();
}

Status CodecSample::Start()
{
    if (inputLoop_ != nullptr) {
        inputLoop_->detach();
    }
    if (outputLoop_ != nullptr) {
        outputLoop_->detach();
    }
    time_ = chrono::time_point_cast<chrono::milliseconds>(chrono::system_clock::now()).time_since_epoch().count();
    inputLoop_ = make_unique<thread>(&CodecSample::InputLoopFunc, this);
    EXPECT_NE(inputLoop_, nullptr);
    outputLoop_ = make_unique<thread>(&CodecSample::OutputLoopFunc, this);
    EXPECT_NE(outputLoop_, nullptr);

    Status ret = codec_->Start();
    isRunning_.store(true);
    unique_lock<mutex> lock(mutex_);
    auto lck = [this]() { return isReleased_.load() || !isRunning_.load(); };
    bool isNotTimeout = cond_.wait_for(lock, chrono::seconds(SAMPLE_TIMEOUT), lck);
    lock.unlock();
    if (isReleased_.load()) {
        return Status::OK;
    }
    int64_t tempTime =
        chrono::time_point_cast<chrono::milliseconds>(chrono::system_clock::now()).time_since_epoch().count();
    EXPECT_TRUE(isNotTimeout);
    if (!isNotTimeout) {
        cout << "Run func timeout, time used: " << tempTime - time_ << "ms" << endl;
    } else {
        cout << "Run func finish, time used: " << tempTime - time_ << "ms" << endl;
        isRunning_.store(false);
    }
    return ret;
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

Status CodecSample::SetParameter(const std::shared_ptr<Meta> &parameter)
{
    return codec_->SetParameter(parameter);
}

std::shared_ptr<Meta> CodecSample::GetOutputFormat()
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
    outPath_ = path + ".yuv";
    outSurfacePath_ = path + ".rgba";
}

void CodecSample::SetInPath(const std::string &path)
{
    inPath_ = path;
}

void CodecSample::InputLoopFunc()
{
    {
        std::unique_lock<std::mutex> lock(inMutex_);
        inCond_.wait(lock, [this]() { return isReleased_.load() || (!isReleased_.load() && (inputPd_ != nullptr)); });
        UNITTEST_CHECK_AND_RETURN_LOG(!isReleased_.load(), "InputLoopFunc stop running");
    }

    inFile_ = std::make_unique<std::ifstream>();
    ASSERT_NE(inFile_, nullptr) << "Fatal: No memory";
    inFile_->open(inPath_, std::ios::in | std::ios::binary);
    ASSERT_TRUE(inFile_->is_open()) << "inFile_ can not find";

    inFile_->read(reinterpret_cast<char *>(&datSize_), sizeof(int64_t));
    uint64_t bufferSize = 0;
    uint64_t bufferPts = 0;
    frameInputCount_ = 0;
    while (frameInputCount_ != EOS_COUNT && frameInputCount_ < datSize_) {
        inFile_->read(reinterpret_cast<char *>(&bufferSize), sizeof(int64_t));
        inFile_->read(reinterpret_cast<char *>(&bufferPts), sizeof(int64_t));

        std::shared_ptr<AVBuffer> buffer = nullptr;
        ASSERT_EQ(Status::OK, inputPd_->RequestBuffer(buffer, {.size = bufferSize}, -1));
        UNITTEST_CHECK_AND_RETURN_LOG(!isReleased_.load(), "InputLoopFunc stop running");
        ASSERT_NE(buffer, nullptr);
        ASSERT_NE(buffer->memory_, nullptr);

        inFile_->read(reinterpret_cast<char *>(buffer->memory_->GetAddr()), bufferSize);
        buffer->memory_->SetSize(bufferSize);
        buffer->pts_ = bufferPts;
        buffer->flag_ = AVCODEC_BUFFER_FLAG_NONE;

        ++frameInputCount_;
        if (frameInputCount_ == 1) {
            buffer->flag_ = AVCODEC_BUFFER_FLAG_CODEC_DATA;
        }
        if (frameInputCount_ == EOS_COUNT || frameInputCount_ == datSize_) {
            buffer->flag_ = AVCODEC_BUFFER_FLAG_EOS;
        }
        inputPd_->PushBuffer(buffer, true);
    }
}

void CodecSample::OutputLoopFunc()
{
    {
        std::unique_lock<std::mutex> lock(outMutex_);
        outCond_.wait(lock, [this]() { return isReleased_.load() || (isRunning_.load() && (outputCs_ != nullptr)); });
        UNITTEST_CHECK_AND_RETURN_LOG(!isReleased_.load(), "InputLoopFunc stop running");
    }

    outFile_ = std::make_unique<std::ofstream>();
    ASSERT_NE(outFile_, nullptr) << "Fatal: No memory";
    outFile_->open(outPath_, std::ios::out | std::ios::binary | std::ios::ate);
    ASSERT_TRUE(outFile_->is_open()) << "outFile_ can not find";

    frameOutputCount_ = 0;
    while (frameOutputCount_ != EOS_COUNT && frameOutputCount_ < datSize_) {
        std::shared_ptr<AVBuffer> buffer = nullptr;
        ASSERT_EQ(Status::OK, outputCs_->AcquireBuffer(buffer));
        UNITTEST_CHECK_AND_RETURN_LOG(!isReleased_.load(), "InputLoopFunc stop running");
        ASSERT_NE(buffer, nullptr);
        ASSERT_NE(buffer->memory_, nullptr);
        outFile_->write(reinterpret_cast<char *>(buffer->memory_->GetAddr()), buffer->memory_->GetSize());
        ++frameOutputCount_;

        outputCs_->ReleaseBuffer(buffer);
        if (buffer->flag_ == AVCODEC_BUFFER_FLAG_EOS) {
            break;
        }
    }
    unique_lock<std::mutex> lockStart(mutex_);
    isRunning_.store(false);
    cond_.notify_all();
}
} // namespace MediaAVCodec
} // namespace OHOS