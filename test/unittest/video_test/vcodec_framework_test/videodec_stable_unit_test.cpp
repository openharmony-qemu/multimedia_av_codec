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
#include <gtest/gtest.h>
#include <gtest/hwext/gtest-multithread.h>
#include "vdec_sample.h"
#define TEST_ID vdec->sampleId_
#include "unittest_log.h"
#define TITLE_LOG UNITTEST_INFO_LOG("")
#define CALLBACK_LOG(index, signal)                                                                                    \
    do {                                                                                                               \
                                                                                                                       \
        UNITTEST_INFO_LOG("index:%d", index);                                                                          \
        if ((signal)->isFlushing_ || !(signal)->isRunning_) {                                                          \
            return;                                                                                                    \
        }                                                                                                              \
    } while (0)

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "VideoDecSample"};
} // namespace
using namespace std;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace testing::ext;
using namespace testing::mt;

namespace {
constexpr uint64_t TEST_RANGE = 20;
constexpr uint64_t TEST_THREAD_COUNT = 4;
constexpr uint64_t TEST_FREQUENCY = 29;

void OnErrorVoid(OH_AVCodec *codec, int32_t errorCode, void *userData)
{
    (void)codec;
    (void)errorCode;
    (void)userData;
}

void OnStreamChangedVoid(OH_AVCodec *codec, OH_AVFormat *format, void *userData)
{
    (void)codec;
    (void)format;
    (void)userData;
}

void InDataVoid(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, void *userData)
{
    (void)codec;
    (void)index;
    (void)data;
    (void)userData;
}

void OutDataVoid(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, OH_AVCodecBufferAttr *attr, void *userData)
{
    (void)codec;
    (void)index;
    (void)data;
    (void)attr;
    (void)userData;
}

void InBufferVoid(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData)
{
    (void)codec;
    (void)index;
    (void)buffer;
    (void)userData;
}

void OutBufferVoid(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData)
{
    (void)codec;
    (void)index;
    (void)buffer;
    (void)userData;
}

void InDataHandle(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, void *userData)
{
    auto signal = reinterpret_cast<VideoDecSignal *>(userData);
    auto vdec = signal->codec_.lock();
    EXPECT_TRUE(vdec->codec_ == codec);
    lock_guard<mutex> lock(signal->inMutex_);
    CALLBACK_LOG(index, signal);
    OH_AVCodecBufferAttr attr;
    vdec->HandleInputFrame(data, attr);
    vdec->PushInputData(index, attr);
}

void OutDataHandle(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, OH_AVCodecBufferAttr *attr, void *userData)
{
    auto signal = reinterpret_cast<VideoDecSignal *>(userData);
    auto vdec = signal->codec_.lock();
    EXPECT_TRUE(vdec->codec_ == codec);
    lock_guard<mutex> lock(signal->outMutex_);
    CALLBACK_LOG(index, signal);
    vdec->HandleOutputFrame(data, *attr);
    vdec->FreeOutputData(index);
}

void OutDataRender(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, OH_AVCodecBufferAttr *attr, void *userData)
{
    auto signal = reinterpret_cast<VideoDecSignal *>(userData);
    auto vdec = signal->codec_.lock();
    EXPECT_TRUE(vdec->codec_ == codec);
    lock_guard<mutex> lock(signal->outMutex_);
    CALLBACK_LOG(index, signal);
    vdec->HandleOutputFrame(data, *attr);
    vdec->RenderOutputData(index);
}

void InDataFlush(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, void *userData)
{
    auto signal = reinterpret_cast<VideoDecSignal *>(userData);
    auto vdec = signal->codec_.lock();
    EXPECT_TRUE(vdec->codec_ == codec);
    ++signal->controlNum_;
    if (signal->controlNum_ == TEST_FREQUENCY) {
        EXPECT_EQ(vdec->Flush(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
        EXPECT_EQ(vdec->Start(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
        return;
    }
    lock_guard<mutex> lock(signal->inMutex_);
    CALLBACK_LOG(index, signal);
    OH_AVCodecBufferAttr attr;
    vdec->HandleInputFrame(data, attr);
    vdec->PushInputData(index, attr);
}

void OutDataFlush(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, OH_AVCodecBufferAttr *attr, void *userData)
{
    auto signal = reinterpret_cast<VideoDecSignal *>(userData);
    auto vdec = signal->codec_.lock();
    EXPECT_TRUE(vdec->codec_ == codec);
    ++signal->controlNum_;
    if (signal->controlNum_ == TEST_FREQUENCY) {
        EXPECT_EQ(vdec->Flush(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
        EXPECT_EQ(vdec->Start(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
        return;
    }
    lock_guard<mutex> lock(signal->outMutex_);
    CALLBACK_LOG(index, signal);
    vdec->HandleOutputFrame(data, *attr);
    vdec->FreeOutputData(index);
}

void InDataStop(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, void *userData)
{
    auto signal = reinterpret_cast<VideoDecSignal *>(userData);
    auto vdec = signal->codec_.lock();
    EXPECT_TRUE(vdec->codec_ == codec);
    ++signal->controlNum_;
    if (signal->controlNum_ == TEST_FREQUENCY) {
        EXPECT_EQ(vdec->Flush(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
        EXPECT_EQ(vdec->Start(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
        return;
    }
    lock_guard<mutex> lock(signal->inMutex_);
    CALLBACK_LOG(index, signal);
    OH_AVCodecBufferAttr attr;
    vdec->HandleInputFrame(data, attr);
    vdec->PushInputData(index, attr);
}

void OutDataStop(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, OH_AVCodecBufferAttr *attr, void *userData)
{
    auto signal = reinterpret_cast<VideoDecSignal *>(userData);
    auto vdec = signal->codec_.lock();
    EXPECT_TRUE(vdec->codec_ == codec);
    ++signal->controlNum_;
    if (signal->controlNum_ == TEST_FREQUENCY) {
        EXPECT_EQ(vdec->Flush(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
        EXPECT_EQ(vdec->Start(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
        return;
    }
    lock_guard<mutex> lock(signal->outMutex_);
    CALLBACK_LOG(index, signal);
    vdec->HandleOutputFrame(data, *attr);
    vdec->FreeOutputData(index);
}

void InDataQueue(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, void *userData)
{
    auto signal = reinterpret_cast<VideoDecSignal *>(userData);
    auto vdec = signal->codec_.lock();
    EXPECT_TRUE(vdec->codec_ == codec);
    lock_guard<mutex> lock(signal->inMutex_);
    CALLBACK_LOG(index, signal);
    signal->inQueue_.push(index);
    signal->inMemoryQueue_.push(data);
    signal->inCond_.notify_all();
}

void OutDataQueue(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, OH_AVCodecBufferAttr *attr, void *userData)
{
    auto signal = reinterpret_cast<VideoDecSignal *>(userData);
    auto vdec = signal->codec_.lock();
    EXPECT_TRUE(vdec->codec_ == codec);
    lock_guard<mutex> lock(signal->outMutex_);
    CALLBACK_LOG(index, signal);
    signal->outQueue_.push(index);
    signal->outMemoryQueue_.push(data);
    signal->outAttrQueue_.push(*attr);
    signal->outCond_.notify_all();
}

void InBufferHandle(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData)
{
    auto signal = reinterpret_cast<VideoDecSignal *>(userData);
    auto vdec = signal->codec_.lock();
    EXPECT_TRUE(vdec->codec_ == codec);
    lock_guard<mutex> lock(signal->inMutex_);
    CALLBACK_LOG(index, signal);
    vdec->HandleInputFrame(buffer);
    vdec->PushInputData(index);
}

void OutBufferHandle(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData)
{
    auto signal = reinterpret_cast<VideoDecSignal *>(userData);
    auto vdec = signal->codec_.lock();
    EXPECT_TRUE(vdec->codec_ == codec);
    lock_guard<mutex> lock(signal->outMutex_);
    CALLBACK_LOG(index, signal);
    vdec->HandleOutputFrame(buffer);
    vdec->FreeOutputData(index);
}

void OutBufferRender(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData)
{
    auto signal = reinterpret_cast<VideoDecSignal *>(userData);
    auto vdec = signal->codec_.lock();
    EXPECT_TRUE(vdec->codec_ == codec);
    lock_guard<mutex> lock(signal->outMutex_);
    CALLBACK_LOG(index, signal);
    vdec->HandleOutputFrame(buffer);
    vdec->RenderOutputData(index);
}

void InBufferFlush(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData)
{
    auto signal = reinterpret_cast<VideoDecSignal *>(userData);
    auto vdec = signal->codec_.lock();
    EXPECT_TRUE(vdec->codec_ == codec);
    ++signal->controlNum_;
    if (signal->controlNum_ == TEST_FREQUENCY) {
        EXPECT_EQ(vdec->Flush(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
        EXPECT_EQ(vdec->Start(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
        return;
    }
    lock_guard<mutex> lock(signal->inMutex_);
    CALLBACK_LOG(index, signal);
    vdec->HandleInputFrame(buffer);
    vdec->PushInputData(index);
}

void OutBufferFlush(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData)
{
    auto signal = reinterpret_cast<VideoDecSignal *>(userData);
    auto vdec = signal->codec_.lock();
    EXPECT_TRUE(vdec->codec_ == codec);
    ++signal->controlNum_;
    if (signal->controlNum_ == TEST_FREQUENCY) {
        EXPECT_EQ(vdec->Flush(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
        EXPECT_EQ(vdec->Start(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
        return;
    }
    lock_guard<mutex> lock(signal->outMutex_);
    CALLBACK_LOG(index, signal);
    vdec->HandleOutputFrame(buffer);
    vdec->FreeOutputData(index);
}

void InBufferStop(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData)
{
    auto signal = reinterpret_cast<VideoDecSignal *>(userData);
    auto vdec = signal->codec_.lock();
    EXPECT_TRUE(vdec->codec_ == codec);
    ++signal->controlNum_;
    if (signal->controlNum_ == TEST_FREQUENCY) {
        EXPECT_EQ(vdec->Stop(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
        EXPECT_EQ(vdec->Start(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
        return;
    }
    lock_guard<mutex> lock(signal->inMutex_);
    CALLBACK_LOG(index, signal);
    vdec->HandleInputFrame(buffer);
    vdec->PushInputData(index);
}

void OutBufferStop(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData)
{
    auto signal = reinterpret_cast<VideoDecSignal *>(userData);
    auto vdec = signal->codec_.lock();
    EXPECT_TRUE(vdec->codec_ == codec);
    ++signal->controlNum_;
    if (signal->controlNum_ == TEST_FREQUENCY) {
        EXPECT_EQ(vdec->Stop(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
        EXPECT_EQ(vdec->Start(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
        return;
    }
    lock_guard<mutex> lock(signal->outMutex_);
    CALLBACK_LOG(index, signal);
    vdec->HandleOutputFrame(buffer);
    vdec->FreeOutputData(index);
}

void InBufferQueue(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData)
{
    auto signal = reinterpret_cast<VideoDecSignal *>(userData);
    auto vdec = signal->codec_.lock();
    EXPECT_TRUE(vdec->codec_ == codec);
    lock_guard<mutex> lock(signal->inMutex_);
    if (signal->isFlushing_ || !signal->isRunning_) {
        return;
    }
    CALLBACK_LOG(index, signal);
    signal->inQueue_.push(index);
    signal->inBufferQueue_.push(buffer);
    signal->inCond_.notify_all();
}

void OutBufferQueue(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData)
{
    auto signal = reinterpret_cast<VideoDecSignal *>(userData);
    auto vdec = signal->codec_.lock();
    EXPECT_TRUE(vdec->codec_ == codec);
    lock_guard<mutex> lock(signal->outMutex_);
    if (signal->isFlushing_ || !signal->isRunning_) {
        return;
    }
    CALLBACK_LOG(index, signal);
    signal->outQueue_.push(index);
    signal->outBufferQueue_.push(buffer);
    signal->outCond_.notify_all();
}

void InputBufferLoop(shared_ptr<VideoDecSignal> &signal)
{
    auto vdec = signal->codec_.lock();
    EXPECT_NE(vdec, nullptr);
    string name = "inloop_" + to_string(vdec->sampleId_);
    pthread_setname_np(pthread_self(), name.substr(0, 15).c_str()); // 15: max thread name
    while (signal->isRunning_.load()) {
        unique_lock<mutex> lock(signal->inMutex_);
        signal->inCond_.wait(lock, [&signal]() {
            return !signal->isRunning_.load() || signal->isFlushing_.load() || signal->inQueue_.size() > 0;
        });
        if (signal->isFlushing_.load()) {
            signal->inCond_.wait(lock, [&signal]() { return !signal->isFlushing_.load(); });
            continue;
        }
        if (!signal->isRunning_.load()) {
            return;
        }
        TITLE_LOG;
        uint32_t index = 0;
        OH_AVCodecBufferAttr attr;
        vdec->HandleInputFrame(index, attr);
        vdec->PushInputData(index, attr);
    }
}

void OutputBufferLoop(shared_ptr<VideoDecSignal> &signal)
{
    auto vdec = signal->codec_.lock();
    EXPECT_NE(vdec, nullptr);
    string name = "outloop_" + to_string(vdec->sampleId_);
    pthread_setname_np(pthread_self(), name.substr(0, 15).c_str()); // 15: max thread name
    while (signal->isRunning_.load()) {
        unique_lock<mutex> lock(signal->outMutex_);
        signal->outCond_.wait(lock, [&signal]() {
            return !signal->isRunning_.load() || signal->isFlushing_.load() || signal->outQueue_.size() > 0;
        });
        if (signal->isFlushing_.load()) {
            signal->inCond_.wait(lock, [&signal]() { return !signal->isFlushing_.load(); });
            continue;
        }
        if (!signal->isRunning_.load()) {
            return;
        }
        TITLE_LOG;
        uint32_t index = 0;
        OH_AVCodecBufferAttr attr;
        vdec->HandleOutputFrame(index, attr);
        vdec->FreeOutputData(index);
    }
}

void OutputSurfaceLoop(shared_ptr<VideoDecSignal> &signal)
{
    auto vdec = signal->codec_.lock();
    EXPECT_NE(vdec, nullptr);
    string name = "outloop_" + to_string(vdec->sampleId_);
    pthread_setname_np(pthread_self(), name.substr(0, 15).c_str()); // 15: max thread name
    while (signal->isRunning_.load()) {
        unique_lock<mutex> lock(signal->outMutex_);
        signal->outCond_.wait(lock, [&signal]() {
            return !signal->isRunning_.load() || signal->isFlushing_.load() || signal->outQueue_.size() > 0;
        });
        if (signal->isFlushing_.load()) {
            signal->inCond_.wait(lock, [&signal]() { return !signal->isFlushing_.load(); });
            continue;
        }
        if (!signal->isRunning_.load()) {
            return;
        }
        TITLE_LOG;
        uint32_t index = 0;
        OH_AVCodecBufferAttr attr;
        vdec->HandleOutputFrame(index, attr);
        vdec->RenderOutputData(index);
        UNITTEST_INFO_LOG("end ");
    }
}

class VideoDecUnitTest : public testing::TestWithParam<int32_t> {
public:
    // SetUpTestCase: Called before all test cases
    static void SetUpTestCase(void);
    // TearDownTestCase: Called after all test case
    static void TearDownTestCase(void);
    // SetUp: Called before each test cases
    void SetUp(void);
    // TearDown: Called after each test cases
    void TearDown(void);

private:
    shared_ptr<HeapMemoryThread> heapThread_ = nullptr;
};

void VideoDecUnitTest::SetUpTestCase(void)
{
    (void)InDataVoid;
    (void)OutDataVoid;
    (void)InBufferVoid;
    (void)OutBufferVoid;
}

void VideoDecUnitTest::TearDownTestCase(void) {}

void VideoDecUnitTest::SetUp(void)
{
    heapThread_ = make_shared<HeapMemoryThread>();
}

void VideoDecUnitTest::TearDown(void)
{
    heapThread_ = nullptr;
}

string GetTestName()
{
    static atomic<int32_t> testid = 0;
    const ::testing::TestInfo *testInfo = ::testing::UnitTest::GetInstance()->current_test_info();
    string fileName = testInfo->name() + std::to_string(testid = (testid + 1) % TEST_RANGE) + ".yuv";
    auto check = [](char it) { return it == '/'; };
    (void)fileName.erase(std::remove_if(fileName.begin(), fileName.end(), check), fileName.end());
    return fileName;
}

/**
 * @tc.name: video_decoder_multithread_release_001
 * @tc.desc: 1. push/free buffer in callback;
 *           2. release not in callback;
 * @tc.type: FUNC
 */
HWMTEST_F(VideoDecUnitTest, video_decoder_multithread_release_001, TestSize.Level1, TEST_THREAD_COUNT)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VideoDecSignal>(vdec);
    vdec->needDump_ = true;
    vdec->isHardware_ = true;
    vdec->frameCount_ = 30; // 30: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    vdec->inPath_ = "720_1280_25_avcc.h264";
    vdec->outPath_ = GetTestName();
    EXPECT_EQ(vdec->Create(), true);
    struct OH_AVCodecAsyncCallback cb;
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputData = InDataHandle;
    cb.onNeedOutputData = OutDataHandle;
    signal->isRunning_ = true;
    EXPECT_EQ(vdec->SetCallback(cb, signal), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Configure(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    vdec->WaitForEos();
    vdec->Release();
}

/**
 * @tc.name: video_decoder_multithread_release_buffer_001
 * @tc.desc: 1. push/free buffer in callback;
 *           2. release not in callback;
 * @tc.type: FUNC
 */
HWMTEST_F(VideoDecUnitTest, video_decoder_multithread_release_buffer_001, TestSize.Level1, TEST_THREAD_COUNT)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VideoDecSignal>(vdec);
    vdec->needDump_ = true;
    vdec->isHardware_ = true;
    vdec->frameCount_ = 30; // 30: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    vdec->inPath_ = "720_1280_25_avcc.h264";
    vdec->outPath_ = GetTestName();
    EXPECT_EQ(vdec->Create(), true);
    struct OH_AVCodecCallback cb;
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputBuffer = InBufferHandle;
    cb.onNewOutputBuffer = OutBufferHandle;
    signal->isRunning_ = true;
    EXPECT_EQ(vdec->RegisterCallback(cb, signal), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Configure(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    vdec->WaitForEos();
    vdec->Release();
}

/**
 * @tc.name: video_decoder_multithread_flush_001
 * @tc.desc: 1. push/free buffer in callback;
 *           2. flush not in callback;
 * @tc.type: FUNC
 */
HWMTEST_F(VideoDecUnitTest, video_decoder_multithread_flush_001, TestSize.Level1, TEST_THREAD_COUNT)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VideoDecSignal>(vdec);
    vdec->needDump_ = true;
    vdec->isHardware_ = true;
    vdec->frameCount_ = 30; // 30: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    vdec->inPath_ = "720_1280_25_avcc.h264";
    vdec->outPath_ = GetTestName();
    EXPECT_EQ(vdec->Create(), true);
    struct OH_AVCodecAsyncCallback cb;
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputData = InDataHandle;
    cb.onNeedOutputData = OutDataHandle;
    signal->isRunning_ = true;
    EXPECT_EQ(vdec->SetCallback(cb, signal), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Configure(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Flush(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    vdec->WaitForEos();
    vdec->Release();
}

/**
 * @tc.name: video_decoder_multithread_flush_002
 * @tc.desc: 1. push/free buffer in callback;
 *           2. flush in callback;
 * @tc.type: FUNC
 */
HWMTEST_F(VideoDecUnitTest, video_decoder_multithread_flush_002, TestSize.Level1, TEST_THREAD_COUNT)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VideoDecSignal>(vdec);
    vdec->needDump_ = true;
    vdec->isHardware_ = true;
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    vdec->inPath_ = "720_1280_25_avcc.h264";
    vdec->outPath_ = GetTestName();
    EXPECT_EQ(vdec->Create(), true);
    struct OH_AVCodecAsyncCallback cb;
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputData = InDataFlush;
    cb.onNeedOutputData = OutDataHandle;
    signal->isRunning_ = true;
    EXPECT_EQ(vdec->SetCallback(cb, signal), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Configure(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    vdec->WaitForEos();
    vdec->Release();
}

/**
 * @tc.name: video_decoder_multithread_flush_003
 * @tc.desc: 1. push/free buffer in callback;
 *           2. flush in output callback;
 * @tc.type: FUNC
 */
HWMTEST_F(VideoDecUnitTest, video_decoder_multithread_flush_003, TestSize.Level1, TEST_THREAD_COUNT)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VideoDecSignal>(vdec);
    vdec->needDump_ = true;
    vdec->isHardware_ = true;
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    vdec->inPath_ = "720_1280_25_avcc.h264";
    vdec->outPath_ = GetTestName();
    EXPECT_EQ(vdec->Create(), true);
    struct OH_AVCodecAsyncCallback cb;
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputData = InDataHandle;
    cb.onNeedOutputData = OutDataFlush;
    signal->isRunning_ = true;
    EXPECT_EQ(vdec->SetCallback(cb, signal), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Configure(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    vdec->WaitForEos();
    vdec->Release();
}

/**
 * @tc.name: video_decoder_multithread_flush_004
 * @tc.desc: 1. push/free buffer in callback;
 *           2. flush in callback;
 *           3. set surface;
 * @tc.type: FUNC
 */
HWMTEST_F(VideoDecUnitTest, video_decoder_multithread_flush_004, TestSize.Level1, TEST_THREAD_COUNT)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VideoDecSignal>(vdec);
    vdec->needDump_ = true;
    vdec->isHardware_ = true;
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    vdec->inPath_ = "720_1280_25_avcc.h264";
    vdec->outPath_ = GetTestName();
    EXPECT_EQ(vdec->Create(), true);
    struct OH_AVCodecAsyncCallback cb;
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputData = InDataFlush;
    cb.onNeedOutputData = OutDataRender;
    signal->isRunning_ = true;
    EXPECT_EQ(vdec->SetCallback(cb, signal), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Configure(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->SetOutputSurface(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    vdec->WaitForEos();
    vdec->Release();
}

/**
 * @tc.name: video_decoder_multithread_flush_with_queue_001
 * @tc.desc: 1. push/free buffer in queue;
 *           2. flush not in output callback;
 * @tc.type: FUNC
 */
HWMTEST_F(VideoDecUnitTest, video_decoder_multithread_flush_with_queue_001, TestSize.Level1, TEST_THREAD_COUNT)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VideoDecSignal>(vdec);
    vdec->needDump_ = true;
    vdec->isHardware_ = true;
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    vdec->inPath_ = "720_1280_25_avcc.h264";
    vdec->outPath_ = GetTestName();
    EXPECT_EQ(vdec->Create(), true);
    struct OH_AVCodecAsyncCallback cb;
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputData = InDataQueue;
    cb.onNeedOutputData = OutDataQueue;
    EXPECT_EQ(vdec->SetCallback(cb, signal), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Configure(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;

    signal->isRunning_ = true;
    vdec->inputLoop_ = make_unique<thread>([&signal]() { InputBufferLoop(signal); });
    vdec->outputLoop_ = make_unique<thread>([&signal]() { OutputBufferLoop(signal); });
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Flush(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;

    vdec->WaitForEos();
    vdec->Release();
}

/**
 * @tc.name: video_decoder_multithread_flush_with_queue_002
 * @tc.desc: 1. push/free buffer in queue;
 *           2. flush not in output callback;
 *           3. set surface.
 * @tc.type: FUNC
 */
HWMTEST_F(VideoDecUnitTest, video_decoder_multithread_flush_with_queue_002, TestSize.Level1, TEST_THREAD_COUNT)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VideoDecSignal>(vdec);
    vdec->needDump_ = true;
    vdec->isHardware_ = true;
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    vdec->inPath_ = "720_1280_25_avcc.h264";
    vdec->outPath_ = GetTestName();
    EXPECT_EQ(vdec->Create(), true);
    struct OH_AVCodecAsyncCallback cb;
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputData = InDataQueue;
    cb.onNeedOutputData = OutDataQueue;
    EXPECT_EQ(vdec->SetCallback(cb, signal), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Configure(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->SetOutputSurface(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;

    signal->isRunning_ = true;
    vdec->inputLoop_ = make_unique<thread>([&signal]() { InputBufferLoop(signal); });
    vdec->outputLoop_ = make_unique<thread>([&signal]() { OutputSurfaceLoop(signal); });
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Flush(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;

    vdec->WaitForEos();
    vdec->Release();
}

/**
 * @tc.name: video_decoder_multithread_flush_with_queue_003
 * @tc.desc: 1. push buffer in callback;
 *           2. flush in input callback;
 *           3. free buffer in queue;
 * @tc.type: FUNC
 */
HWMTEST_F(VideoDecUnitTest, video_decoder_multithread_flush_with_queue_003, TestSize.Level1, TEST_THREAD_COUNT)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VideoDecSignal>(vdec);
    vdec->needDump_ = true;
    vdec->isHardware_ = true;
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    vdec->inPath_ = "720_1280_25_avcc.h264";
    vdec->outPath_ = GetTestName();
    EXPECT_EQ(vdec->Create(), true);
    struct OH_AVCodecAsyncCallback cb;
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputData = InDataFlush;
    cb.onNeedOutputData = OutDataQueue;
    EXPECT_EQ(vdec->SetCallback(cb, signal), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Configure(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;

    signal->isRunning_ = true;
    vdec->outputLoop_ = make_unique<thread>([&signal]() { OutputBufferLoop(signal); });
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;

    vdec->WaitForEos();
    vdec->Release();
}

/**
 * @tc.name: video_decoder_multithread_flush_with_queue_004
 * @tc.desc: 1. push buffer in callback;
 *           2. flush in input callback;
 *           3. render buffer in queue;
 *           4. set surface;
 * @tc.type: FUNC
 */
HWMTEST_F(VideoDecUnitTest, video_decoder_multithread_flush_with_queue_004, TestSize.Level1, TEST_THREAD_COUNT)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VideoDecSignal>(vdec);
    vdec->needDump_ = true;
    vdec->isHardware_ = true;
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    vdec->inPath_ = "720_1280_25_avcc.h264";
    vdec->outPath_ = GetTestName();
    EXPECT_EQ(vdec->Create(), true);
    struct OH_AVCodecAsyncCallback cb;
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputData = InDataFlush;
    cb.onNeedOutputData = OutDataQueue;
    EXPECT_EQ(vdec->SetCallback(cb, signal), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Configure(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->SetOutputSurface(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;

    signal->isRunning_ = true;
    vdec->outputLoop_ = make_unique<thread>([&signal]() { OutputSurfaceLoop(signal); });
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;

    vdec->WaitForEos();
    vdec->Release();
}

/**
 * @tc.name: video_decoder_multithread_flush_buffer_001
 * @tc.desc: 1. push/free buffer in callback;
 *           2. flush not in callback;
 * @tc.type: FUNC
 */
HWMTEST_F(VideoDecUnitTest, video_decoder_multithread_flush_buffer_001, TestSize.Level1, TEST_THREAD_COUNT)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VideoDecSignal>(vdec);
    vdec->needDump_ = true;
    vdec->isHardware_ = true;
    vdec->frameCount_ = 30; // 30: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    vdec->inPath_ = "720_1280_25_avcc.h264";
    vdec->outPath_ = GetTestName();
    EXPECT_EQ(vdec->Create(), true);
    struct OH_AVCodecCallback cb;
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputBuffer = InBufferHandle;
    cb.onNewOutputBuffer = OutBufferHandle;
    signal->isRunning_ = true;
    EXPECT_EQ(vdec->RegisterCallback(cb, signal), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Configure(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Flush(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    vdec->WaitForEos();
    vdec->Release();
}

/**
 * @tc.name: video_decoder_multithread_flush_buffer_002
 * @tc.desc: 1. push/free buffer in callback;
 *           2. flush in input callback;
 * @tc.type: FUNC
 */
HWMTEST_F(VideoDecUnitTest, video_decoder_multithread_flush_buffer_002, TestSize.Level1, TEST_THREAD_COUNT)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VideoDecSignal>(vdec);
    vdec->needDump_ = true;
    vdec->isHardware_ = true;
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    vdec->inPath_ = "720_1280_25_avcc.h264";
    vdec->outPath_ = GetTestName();
    EXPECT_EQ(vdec->Create(), true);
    struct OH_AVCodecCallback cb;
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputBuffer = InBufferFlush;
    cb.onNewOutputBuffer = OutBufferHandle;
    signal->isRunning_ = true;
    EXPECT_EQ(vdec->RegisterCallback(cb, signal), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Configure(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    vdec->WaitForEos();
    vdec->Release();
}

/**
 * @tc.name: video_decoder_multithread_flush_buffer_003
 * @tc.desc: 1. push/free buffer in callback;
 *           2. flush in output callback;
 * @tc.type: FUNC
 */
HWMTEST_F(VideoDecUnitTest, video_decoder_multithread_flush_buffer_003, TestSize.Level1, TEST_THREAD_COUNT)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VideoDecSignal>(vdec);
    vdec->needDump_ = true;
    vdec->isHardware_ = true;
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    vdec->inPath_ = "720_1280_25_avcc.h264";
    vdec->outPath_ = GetTestName();
    EXPECT_EQ(vdec->Create(), true);
    struct OH_AVCodecCallback cb;
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputBuffer = InBufferHandle;
    cb.onNewOutputBuffer = OutBufferFlush;
    signal->isRunning_ = true;
    EXPECT_EQ(vdec->RegisterCallback(cb, signal), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Configure(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    vdec->WaitForEos();
    vdec->Release();
}

/**
 * @tc.name: video_decoder_multithread_flush_buffer_004
 * @tc.desc: 1. push/free buffer in callback;
 *           2. flush in callback;
 *           3. set surface;
 * @tc.type: FUNC
 */
HWMTEST_F(VideoDecUnitTest, video_decoder_multithread_flush_buffer_004, TestSize.Level1, TEST_THREAD_COUNT)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VideoDecSignal>(vdec);
    vdec->needDump_ = true;
    vdec->isHardware_ = true;
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    vdec->inPath_ = "720_1280_25_avcc.h264";
    vdec->outPath_ = GetTestName();
    EXPECT_EQ(vdec->Create(), true);
    struct OH_AVCodecCallback cb;
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputBuffer = InBufferFlush;
    cb.onNewOutputBuffer = OutBufferRender;
    signal->isRunning_ = true;
    EXPECT_EQ(vdec->RegisterCallback(cb, signal), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Configure(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->SetOutputSurface(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    vdec->WaitForEos();
    vdec->Release();
}

/**
 * @tc.name: video_decoder_multithread_flush_with_queue_buffer_001
 * @tc.desc: 1. push/free buffer in queue;
 *           2. flush not in output callback;
 * @tc.type: FUNC
 */
HWMTEST_F(VideoDecUnitTest, video_decoder_multithread_flush_with_queue_buffer_001, TestSize.Level1, TEST_THREAD_COUNT)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VideoDecSignal>(vdec);
    vdec->needDump_ = true;
    vdec->isHardware_ = true;
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    vdec->inPath_ = "720_1280_25_avcc.h264";
    vdec->outPath_ = GetTestName();
    EXPECT_EQ(vdec->Create(), true);
    struct OH_AVCodecCallback cb;
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputBuffer = InBufferQueue;
    cb.onNewOutputBuffer = OutBufferQueue;
    EXPECT_EQ(vdec->RegisterCallback(cb, signal), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Configure(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;

    signal->isRunning_ = true;
    vdec->inputLoop_ = make_unique<thread>([&signal]() { InputBufferLoop(signal); });
    vdec->outputLoop_ = make_unique<thread>([&signal]() { OutputBufferLoop(signal); });
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Flush(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;

    vdec->WaitForEos();
    vdec->Release();
}

/**
 * @tc.name: video_decoder_multithread_flush_with_queue_buffer_002
 * @tc.desc: 1. push/free buffer in queue;
 *           2. flush not in output callback;
 *           3. set surface.
 * @tc.type: FUNC
 */
HWMTEST_F(VideoDecUnitTest, video_decoder_multithread_flush_with_queue_buffer_002, TestSize.Level1, TEST_THREAD_COUNT)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VideoDecSignal>(vdec);
    vdec->needDump_ = true;
    vdec->isHardware_ = true;
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    vdec->inPath_ = "720_1280_25_avcc.h264";
    vdec->outPath_ = GetTestName();
    EXPECT_EQ(vdec->Create(), true);
    struct OH_AVCodecCallback cb;
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputBuffer = InBufferQueue;
    cb.onNewOutputBuffer = OutBufferQueue;
    EXPECT_EQ(vdec->RegisterCallback(cb, signal), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Configure(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->SetOutputSurface(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;

    signal->isRunning_ = true;
    vdec->inputLoop_ = make_unique<thread>([&signal]() { InputBufferLoop(signal); });
    vdec->outputLoop_ = make_unique<thread>([&signal]() { OutputSurfaceLoop(signal); });
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Flush(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;

    vdec->WaitForEos();
    vdec->Release();
}

/**
 * @tc.name: video_decoder_multithread_flush_with_queue_buffer_003
 * @tc.desc: 1. push buffer in callback;
 *           2. flush in input callback;
 *           3. free buffer in queue;
 * @tc.type: FUNC
 */
HWMTEST_F(VideoDecUnitTest, video_decoder_multithread_flush_with_queue_buffer_003, TestSize.Level1, TEST_THREAD_COUNT)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VideoDecSignal>(vdec);
    vdec->needDump_ = true;
    vdec->isHardware_ = true;
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    vdec->inPath_ = "720_1280_25_avcc.h264";
    vdec->outPath_ = GetTestName();
    EXPECT_EQ(vdec->Create(), true);
    struct OH_AVCodecCallback cb;
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputBuffer = InBufferFlush;
    cb.onNewOutputBuffer = OutBufferQueue;
    EXPECT_EQ(vdec->RegisterCallback(cb, signal), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Configure(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;

    signal->isRunning_ = true;
    vdec->outputLoop_ = make_unique<thread>([&signal]() { OutputBufferLoop(signal); });
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;

    vdec->WaitForEos();
    vdec->Release();
}

/**
 * @tc.name: video_decoder_multithread_flush_with_queue_buffer_004
 * @tc.desc: 1. push buffer in callback;
 *           2. flush in input callback;
 *           3. render buffer in queue;
 *           4. set surface;
 * @tc.type: FUNC
 */
HWMTEST_F(VideoDecUnitTest, video_decoder_multithread_flush_with_queue_buffer_004, TestSize.Level1, TEST_THREAD_COUNT)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VideoDecSignal>(vdec);
    vdec->needDump_ = true;
    vdec->isHardware_ = true;
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    vdec->inPath_ = "720_1280_25_avcc.h264";
    vdec->outPath_ = GetTestName();
    EXPECT_EQ(vdec->Create(), true);
    struct OH_AVCodecCallback cb;
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputBuffer = InBufferFlush;
    cb.onNewOutputBuffer = OutBufferQueue;
    EXPECT_EQ(vdec->RegisterCallback(cb, signal), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Configure(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->SetOutputSurface(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;

    signal->isRunning_ = true;
    vdec->outputLoop_ = make_unique<thread>([&signal]() { OutputSurfaceLoop(signal); });
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;

    vdec->WaitForEos();
    vdec->Release();
}

/**
 * @tc.name: video_decoder_multithread_stop_001
 * @tc.desc: 1. push/free buffer in callback;
 *           2. stop not in callback;
 * @tc.type: FUNC
 */
HWMTEST_F(VideoDecUnitTest, video_decoder_multithread_stop_001, TestSize.Level1, TEST_THREAD_COUNT)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VideoDecSignal>(vdec);
    vdec->needDump_ = true;
    vdec->isHardware_ = true;
    vdec->frameCount_ = 30; // 30: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    vdec->inPath_ = "720_1280_25_avcc.h264";
    vdec->outPath_ = GetTestName();
    EXPECT_EQ(vdec->Create(), true);
    struct OH_AVCodecAsyncCallback cb;
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputData = InDataHandle;
    cb.onNeedOutputData = OutDataHandle;
    signal->isRunning_ = true;
    EXPECT_EQ(vdec->SetCallback(cb, signal), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Configure(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Stop(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    vdec->WaitForEos();
    vdec->Release();
}

/**
 * @tc.name: video_decoder_multithread_stop_002
 * @tc.desc: 1. push/free buffer in callback;
 *           2. stop in callback;
 * @tc.type: FUNC
 */
HWMTEST_F(VideoDecUnitTest, video_decoder_multithread_stop_002, TestSize.Level1, TEST_THREAD_COUNT)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VideoDecSignal>(vdec);
    vdec->needDump_ = true;
    vdec->isHardware_ = true;
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    vdec->inPath_ = "720_1280_25_avcc.h264";
    vdec->outPath_ = GetTestName();
    EXPECT_EQ(vdec->Create(), true);
    struct OH_AVCodecAsyncCallback cb;
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputData = InDataStop;
    cb.onNeedOutputData = OutDataHandle;
    signal->isRunning_ = true;
    EXPECT_EQ(vdec->SetCallback(cb, signal), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Configure(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    vdec->WaitForEos();
    vdec->Release();
}

/**
 * @tc.name: video_decoder_multithread_stop_003
 * @tc.desc: 1. push/free buffer in callback;
 *           2. stop in output callback;
 * @tc.type: FUNC
 */
HWMTEST_F(VideoDecUnitTest, video_decoder_multithread_stop_003, TestSize.Level1, TEST_THREAD_COUNT)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VideoDecSignal>(vdec);
    vdec->needDump_ = true;
    vdec->isHardware_ = true;
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    vdec->inPath_ = "720_1280_25_avcc.h264";
    vdec->outPath_ = GetTestName();
    EXPECT_EQ(vdec->Create(), true);
    struct OH_AVCodecAsyncCallback cb;
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputData = InDataHandle;
    cb.onNeedOutputData = OutDataStop;
    signal->isRunning_ = true;
    EXPECT_EQ(vdec->SetCallback(cb, signal), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Configure(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    vdec->WaitForEos();
    vdec->Release();
}

/**
 * @tc.name: video_decoder_multithread_stop_004
 * @tc.desc: 1. push/free buffer in callback;
 *           2. stop in callback;
 *           3. set surface;
 * @tc.type: FUNC
 */
HWMTEST_F(VideoDecUnitTest, video_decoder_multithread_stop_004, TestSize.Level1, TEST_THREAD_COUNT)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VideoDecSignal>(vdec);
    vdec->needDump_ = true;
    vdec->isHardware_ = true;
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    vdec->inPath_ = "720_1280_25_avcc.h264";
    vdec->outPath_ = GetTestName();
    EXPECT_EQ(vdec->Create(), true);
    struct OH_AVCodecAsyncCallback cb;
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputData = InDataStop;
    cb.onNeedOutputData = OutDataRender;
    signal->isRunning_ = true;
    EXPECT_EQ(vdec->SetCallback(cb, signal), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Configure(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->SetOutputSurface(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    vdec->WaitForEos();
    vdec->Release();
}

/**
 * @tc.name: video_decoder_multithread_stop_with_queue_001
 * @tc.desc: 1. push/free buffer in queue;
 *           2. stop not in output callback;
 * @tc.type: FUNC
 */
HWMTEST_F(VideoDecUnitTest, video_decoder_multithread_stop_with_queue_001, TestSize.Level1, TEST_THREAD_COUNT)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VideoDecSignal>(vdec);
    vdec->needDump_ = true;
    vdec->isHardware_ = true;
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    vdec->inPath_ = "720_1280_25_avcc.h264";
    vdec->outPath_ = GetTestName();
    EXPECT_EQ(vdec->Create(), true);
    struct OH_AVCodecAsyncCallback cb;
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputData = InDataQueue;
    cb.onNeedOutputData = OutDataQueue;
    EXPECT_EQ(vdec->SetCallback(cb, signal), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Configure(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;

    signal->isRunning_ = true;
    vdec->inputLoop_ = make_unique<thread>([&signal]() { InputBufferLoop(signal); });
    vdec->outputLoop_ = make_unique<thread>([&signal]() { OutputBufferLoop(signal); });
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Stop(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;

    vdec->WaitForEos();
    vdec->Release();
}

/**
 * @tc.name: video_decoder_multithread_stop_with_queue_002
 * @tc.desc: 1. push/free buffer in queue;
 *           2. stop not in output callback;
 *           3. set surface.
 * @tc.type: FUNC
 */
HWMTEST_F(VideoDecUnitTest, video_decoder_multithread_stop_with_queue_002, TestSize.Level1, TEST_THREAD_COUNT)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VideoDecSignal>(vdec);
    vdec->needDump_ = true;
    vdec->isHardware_ = true;
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    vdec->inPath_ = "720_1280_25_avcc.h264";
    vdec->outPath_ = GetTestName();
    EXPECT_EQ(vdec->Create(), true);
    struct OH_AVCodecAsyncCallback cb;
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputData = InDataQueue;
    cb.onNeedOutputData = OutDataQueue;
    EXPECT_EQ(vdec->SetCallback(cb, signal), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Configure(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->SetOutputSurface(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;

    signal->isRunning_ = true;
    vdec->inputLoop_ = make_unique<thread>([&signal]() { InputBufferLoop(signal); });
    vdec->outputLoop_ = make_unique<thread>([&signal]() { OutputSurfaceLoop(signal); });
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Stop(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;

    vdec->WaitForEos();
    vdec->Release();
}

/**
 * @tc.name: video_decoder_multithread_stop_with_queue_003
 * @tc.desc: 1. push buffer in callback;
 *           2. stop in input callback;
 *           3. free buffer in queue;
 * @tc.type: FUNC
 */
HWMTEST_F(VideoDecUnitTest, video_decoder_multithread_stop_with_queue_003, TestSize.Level1, TEST_THREAD_COUNT)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VideoDecSignal>(vdec);
    vdec->needDump_ = true;
    vdec->isHardware_ = true;
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    vdec->inPath_ = "720_1280_25_avcc.h264";
    vdec->outPath_ = GetTestName();
    EXPECT_EQ(vdec->Create(), true);
    struct OH_AVCodecAsyncCallback cb;
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputData = InDataStop;
    cb.onNeedOutputData = OutDataQueue;
    EXPECT_EQ(vdec->SetCallback(cb, signal), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Configure(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;

    signal->isRunning_ = true;
    vdec->outputLoop_ = make_unique<thread>([&signal]() { OutputBufferLoop(signal); });
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;

    vdec->WaitForEos();
    vdec->Release();
}

/**
 * @tc.name: video_decoder_multithread_stop_with_queue_004
 * @tc.desc: 1. push buffer in callback;
 *           2. stop in input callback;
 *           3. render buffer in queue;
 *           4. set surface;
 * @tc.type: FUNC
 */
HWMTEST_F(VideoDecUnitTest, video_decoder_multithread_stop_with_queue_004, TestSize.Level1, TEST_THREAD_COUNT)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VideoDecSignal>(vdec);
    vdec->needDump_ = true;
    vdec->isHardware_ = true;
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    vdec->inPath_ = "720_1280_25_avcc.h264";
    vdec->outPath_ = GetTestName();
    EXPECT_EQ(vdec->Create(), true);
    struct OH_AVCodecAsyncCallback cb;
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputData = InDataStop;
    cb.onNeedOutputData = OutDataQueue;
    EXPECT_EQ(vdec->SetCallback(cb, signal), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Configure(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->SetOutputSurface(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;

    signal->isRunning_ = true;
    vdec->outputLoop_ = make_unique<thread>([&signal]() { OutputSurfaceLoop(signal); });
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;

    vdec->WaitForEos();
    vdec->Release();
}

/**
 * @tc.name: video_decoder_multithread_stop_buffer_001
 * @tc.desc: 1. push/free buffer in callback;
 *           2. stop not in callback;
 * @tc.type: FUNC
 */
HWMTEST_F(VideoDecUnitTest, video_decoder_multithread_stop_buffer_001, TestSize.Level1, TEST_THREAD_COUNT)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VideoDecSignal>(vdec);
    vdec->needDump_ = true;
    vdec->isHardware_ = true;
    vdec->frameCount_ = 30; // 30: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    vdec->inPath_ = "720_1280_25_avcc.h264";
    vdec->outPath_ = GetTestName();
    EXPECT_EQ(vdec->Create(), true);
    struct OH_AVCodecCallback cb;
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputBuffer = InBufferHandle;
    cb.onNewOutputBuffer = OutBufferHandle;
    signal->isRunning_ = true;
    EXPECT_EQ(vdec->RegisterCallback(cb, signal), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Configure(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Stop(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    vdec->WaitForEos();
    vdec->Release();
}

/**
 * @tc.name: video_decoder_multithread_stop_buffer_002
 * @tc.desc: 1. push/free buffer in callback;
 *           2. stop in input callback;
 * @tc.type: FUNC
 */
HWMTEST_F(VideoDecUnitTest, video_decoder_multithread_stop_buffer_002, TestSize.Level1, TEST_THREAD_COUNT)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VideoDecSignal>(vdec);
    vdec->needDump_ = true;
    vdec->isHardware_ = true;
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    vdec->inPath_ = "720_1280_25_avcc.h264";
    vdec->outPath_ = GetTestName();
    EXPECT_EQ(vdec->Create(), true);
    struct OH_AVCodecCallback cb;
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputBuffer = InBufferStop;
    cb.onNewOutputBuffer = OutBufferHandle;
    signal->isRunning_ = true;
    EXPECT_EQ(vdec->RegisterCallback(cb, signal), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Configure(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    vdec->WaitForEos();
    vdec->Release();
}

/**
 * @tc.name: video_decoder_multithread_stop_buffer_003
 * @tc.desc: 1. push/free buffer in callback;
 *           2. stop in output callback;
 * @tc.type: FUNC
 */
HWMTEST_F(VideoDecUnitTest, video_decoder_multithread_stop_buffer_003, TestSize.Level1, TEST_THREAD_COUNT)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VideoDecSignal>(vdec);
    vdec->needDump_ = true;
    vdec->isHardware_ = true;
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    vdec->inPath_ = "720_1280_25_avcc.h264";
    vdec->outPath_ = GetTestName();
    EXPECT_EQ(vdec->Create(), true);
    struct OH_AVCodecCallback cb;
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputBuffer = InBufferHandle;
    cb.onNewOutputBuffer = OutBufferStop;
    signal->isRunning_ = true;
    EXPECT_EQ(vdec->RegisterCallback(cb, signal), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Configure(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    vdec->WaitForEos();
    vdec->Release();
}

/**
 * @tc.name: video_decoder_multithread_stop_buffer_004
 * @tc.desc: 1. push/free buffer in callback;
 *           2. stop in callback;
 *           3. set surface;
 * @tc.type: FUNC
 */
HWMTEST_F(VideoDecUnitTest, video_decoder_multithread_stop_buffer_004, TestSize.Level1, TEST_THREAD_COUNT)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VideoDecSignal>(vdec);
    vdec->needDump_ = true;
    vdec->isHardware_ = true;
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    vdec->inPath_ = "720_1280_25_avcc.h264";
    vdec->outPath_ = GetTestName();
    EXPECT_EQ(vdec->Create(), true);
    struct OH_AVCodecCallback cb;
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputBuffer = InBufferStop;
    cb.onNewOutputBuffer = OutBufferRender;
    signal->isRunning_ = true;
    EXPECT_EQ(vdec->RegisterCallback(cb, signal), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Configure(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->SetOutputSurface(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    vdec->WaitForEos();
    vdec->Release();
}

/**
 * @tc.name: video_decoder_multithread_stop_with_queue_buffer_001
 * @tc.desc: 1. push/free buffer in queue;
 *           2. stop not in output callback;
 * @tc.type: FUNC
 */
HWMTEST_F(VideoDecUnitTest, video_decoder_multithread_stop_with_queue_buffer_001, TestSize.Level1, TEST_THREAD_COUNT)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VideoDecSignal>(vdec);
    vdec->needDump_ = true;
    vdec->isHardware_ = true;
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    vdec->inPath_ = "720_1280_25_avcc.h264";
    vdec->outPath_ = GetTestName();
    EXPECT_EQ(vdec->Create(), true);
    struct OH_AVCodecCallback cb;
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputBuffer = InBufferQueue;
    cb.onNewOutputBuffer = OutBufferQueue;
    EXPECT_EQ(vdec->RegisterCallback(cb, signal), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Configure(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;

    signal->isRunning_ = true;
    vdec->inputLoop_ = make_unique<thread>([&signal]() { InputBufferLoop(signal); });
    vdec->outputLoop_ = make_unique<thread>([&signal]() { OutputBufferLoop(signal); });
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Stop(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;

    vdec->WaitForEos();
    vdec->Release();
}

/**
 * @tc.name: video_decoder_multithread_stop_with_queue_buffer_002
 * @tc.desc: 1. push/free buffer in queue;
 *           2. stop not in output callback;
 *           3. set surface.
 * @tc.type: FUNC
 */
HWMTEST_F(VideoDecUnitTest, video_decoder_multithread_stop_with_queue_buffer_002, TestSize.Level1, TEST_THREAD_COUNT)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VideoDecSignal>(vdec);
    vdec->needDump_ = true;
    vdec->isHardware_ = true;
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    vdec->inPath_ = "720_1280_25_avcc.h264";
    vdec->outPath_ = GetTestName();
    EXPECT_EQ(vdec->Create(), true);
    struct OH_AVCodecCallback cb;
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputBuffer = InBufferQueue;
    cb.onNewOutputBuffer = OutBufferQueue;
    EXPECT_EQ(vdec->RegisterCallback(cb, signal), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Configure(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->SetOutputSurface(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;

    signal->isRunning_ = true;
    vdec->inputLoop_ = make_unique<thread>([&signal]() { InputBufferLoop(signal); });
    vdec->outputLoop_ = make_unique<thread>([&signal]() { OutputSurfaceLoop(signal); });
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Stop(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;

    vdec->WaitForEos();
    vdec->Release();
}

/**
 * @tc.name: video_decoder_multithread_stop_with_queue_buffer_003
 * @tc.desc: 1. push buffer in callback;
 *           2. stop in input callback;
 *           3. free buffer in queue;
 * @tc.type: FUNC
 */
HWMTEST_F(VideoDecUnitTest, video_decoder_multithread_stop_with_queue_buffer_003, TestSize.Level1, TEST_THREAD_COUNT)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VideoDecSignal>(vdec);
    vdec->needDump_ = true;
    vdec->isHardware_ = true;
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    vdec->inPath_ = "720_1280_25_avcc.h264";
    vdec->outPath_ = GetTestName();
    EXPECT_EQ(vdec->Create(), true);
    struct OH_AVCodecCallback cb;
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputBuffer = InBufferStop;
    cb.onNewOutputBuffer = OutBufferQueue;
    EXPECT_EQ(vdec->RegisterCallback(cb, signal), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Configure(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;

    signal->isRunning_ = true;
    vdec->outputLoop_ = make_unique<thread>([&signal]() { OutputBufferLoop(signal); });
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;

    vdec->WaitForEos();
    vdec->Release();
}

/**
 * @tc.name: video_decoder_multithread_stop_with_queue_buffer_004
 * @tc.desc: 1. push buffer in callback;
 *           2. stop in input callback;
 *           3. render buffer in queue;
 *           4. set surface;
 * @tc.type: FUNC
 */
HWMTEST_F(VideoDecUnitTest, video_decoder_multithread_stop_with_queue_buffer_004, TestSize.Level1, TEST_THREAD_COUNT)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VideoDecSignal>(vdec);
    vdec->needDump_ = true;
    vdec->isHardware_ = true;
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    vdec->inPath_ = "720_1280_25_avcc.h264";
    vdec->outPath_ = GetTestName();
    EXPECT_EQ(vdec->Create(), true);
    struct OH_AVCodecCallback cb;
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputBuffer = InBufferStop;
    cb.onNewOutputBuffer = OutBufferQueue;
    EXPECT_EQ(vdec->RegisterCallback(cb, signal), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->Configure(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;
    EXPECT_EQ(vdec->SetOutputSurface(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;

    signal->isRunning_ = true;
    vdec->outputLoop_ = make_unique<thread>([&signal]() { OutputSurfaceLoop(signal); });
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << "[SAMPLE_ID]:" << TEST_ID;

    vdec->WaitForEos();
    vdec->Release();
}
} // namespace