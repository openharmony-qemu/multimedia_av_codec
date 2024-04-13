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
#include <gtest/gtest.h>
#include <gtest/hwext/gtest-multithread.h>
#include "heap_memory_thread.h"
#include "unittest_utils.h"
#include "vdec_sample.h"

#define PRINT_HILOG
#define TEST_ID vdec->sampleId_
#define SAMPLE_ID "[SAMPLE_ID]:" << TEST_ID
#include "unittest_log.h"
#define TITLE_LOG UNITTEST_INFO_LOG("")
#define CALLBACK_CHECK_LOG(index, signal)                                                                              \
    do {                                                                                                               \
                                                                                                                       \
        UNITTEST_INFO_LOG("index:%d", index);                                                                          \
        if ((signal)->isFlushing_ || !(signal)->isRunning_) {                                                          \
            return;                                                                                                    \
        }                                                                                                              \
    } while (0)

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_TEST, "VideoDecSample"};
} // namespace
using namespace std;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace testing::ext;
using namespace testing::mt;

namespace {
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
    lock_guard<mutex> lock(signal->inMutex_);
    CALLBACK_CHECK_LOG(index, signal);
    OH_AVCodecBufferAttr attr;
    vdec->HandleInputFrame(data, attr);
    vdec->PushInputData(index, attr);
}

void OutDataHandle(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, OH_AVCodecBufferAttr *attr, void *userData)
{
    auto signal = reinterpret_cast<VideoDecSignal *>(userData);
    auto vdec = signal->codec_.lock();
    lock_guard<mutex> lock(signal->outMutex_);
    CALLBACK_CHECK_LOG(index, signal);
    vdec->HandleOutputFrame(data, *attr);
    vdec->ReleaseOutputData(index);
}

void InDataOperate(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, void *userData)
{
    auto signal = reinterpret_cast<VideoDecSignal *>(userData);
    auto vdec = signal->codec_.lock();
    ++signal->controlNum_;
    if (signal->controlNum_ == TEST_FREQUENCY) {
        EXPECT_EQ(vdec->Operate(), AV_ERR_OK) << SAMPLE_ID;
        return;
    }
    lock_guard<mutex> lock(signal->inMutex_);
    CALLBACK_CHECK_LOG(index, signal);
    OH_AVCodecBufferAttr attr;
    vdec->HandleInputFrame(data, attr);
    vdec->PushInputData(index, attr);
}

void OutDataOperate(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, OH_AVCodecBufferAttr *attr, void *userData)
{
    auto signal = reinterpret_cast<VideoDecSignal *>(userData);
    auto vdec = signal->codec_.lock();
    ++signal->controlNum_;
    if (signal->controlNum_ == TEST_FREQUENCY) {
        EXPECT_EQ(vdec->Operate(), AV_ERR_OK) << SAMPLE_ID;
        return;
    }
    lock_guard<mutex> lock(signal->outMutex_);
    CALLBACK_CHECK_LOG(index, signal);
    vdec->HandleOutputFrame(data, *attr);
    vdec->ReleaseOutputData(index);
}

void InDataQueue(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, void *userData)
{
    auto signal = reinterpret_cast<VideoDecSignal *>(userData);
    auto vdec = signal->codec_.lock();
    lock_guard<mutex> lock(signal->inMutex_);
    CALLBACK_CHECK_LOG(index, signal);
    signal->inQueue_.push(index);
    signal->inMemoryQueue_.push(data);
    signal->inCond_.notify_all();
}

void OutDataQueue(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, OH_AVCodecBufferAttr *attr, void *userData)
{
    auto signal = reinterpret_cast<VideoDecSignal *>(userData);
    auto vdec = signal->codec_.lock();
    lock_guard<mutex> lock(signal->outMutex_);
    CALLBACK_CHECK_LOG(index, signal);
    signal->outQueue_.push(index);
    signal->outMemoryQueue_.push(data);
    signal->outAttrQueue_.push(*attr);
    signal->outCond_.notify_all();
}

void InBufferHandle(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData)
{
    auto signal = reinterpret_cast<VideoDecSignal *>(userData);
    auto vdec = signal->codec_.lock();
    lock_guard<mutex> lock(signal->inMutex_);
    CALLBACK_CHECK_LOG(index, signal);
    vdec->HandleInputFrame(buffer);
    vdec->PushInputData(index);
}

void OutBufferHandle(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData)
{
    auto signal = reinterpret_cast<VideoDecSignal *>(userData);
    auto vdec = signal->codec_.lock();
    lock_guard<mutex> lock(signal->outMutex_);
    CALLBACK_CHECK_LOG(index, signal);
    vdec->HandleOutputFrame(buffer);
    vdec->ReleaseOutputData(index);
}

void InBufferOperate(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData)
{
    auto signal = reinterpret_cast<VideoDecSignal *>(userData);
    auto vdec = signal->codec_.lock();
    ++signal->controlNum_;
    if (signal->controlNum_ == TEST_FREQUENCY) {
        EXPECT_EQ(vdec->Operate(), AV_ERR_OK) << SAMPLE_ID;
        return;
    }
    lock_guard<mutex> lock(signal->inMutex_);
    CALLBACK_CHECK_LOG(index, signal);
    vdec->HandleInputFrame(buffer);
    vdec->PushInputData(index);
}

void OutBufferOperate(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData)
{
    auto signal = reinterpret_cast<VideoDecSignal *>(userData);
    auto vdec = signal->codec_.lock();
    ++signal->controlNum_;
    if (signal->controlNum_ == TEST_FREQUENCY) {
        EXPECT_EQ(vdec->Operate(), AV_ERR_OK) << SAMPLE_ID;
        return;
    }
    lock_guard<mutex> lock(signal->inMutex_);
    CALLBACK_CHECK_LOG(index, signal);
    vdec->HandleOutputFrame(buffer);
    vdec->ReleaseOutputData(index);
}

void InBufferQueue(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData)
{
    auto signal = reinterpret_cast<VideoDecSignal *>(userData);
    auto vdec = signal->codec_.lock();
    lock_guard<mutex> lock(signal->inMutex_);
    EXPECT_NO_THROW((void)OH_AVBuffer_GetAddr(buffer));
    CALLBACK_CHECK_LOG(index, signal);
    signal->inQueue_.push(index);
    signal->inBufferQueue_.push(buffer);
    signal->inCond_.notify_all();
}

void OutBufferQueue(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData)
{
    auto signal = reinterpret_cast<VideoDecSignal *>(userData);
    auto vdec = signal->codec_.lock();
    lock_guard<mutex> lock(signal->outMutex_);
    EXPECT_NO_THROW((void)OH_AVBuffer_GetAddr(buffer));
    CALLBACK_CHECK_LOG(index, signal);
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
            signal->outCond_.wait(lock, [&signal]() { return !signal->isFlushing_.load(); });
            continue;
        }
        if (!signal->isRunning_.load()) {
            return;
        }
        TITLE_LOG;
        uint32_t index = 0;
        OH_AVCodecBufferAttr attr;
        vdec->HandleOutputFrame(index, attr);
        vdec->ReleaseOutputData(index);
    }
}

class VideoDecStableTest : public testing::TestWithParam<std::string> {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);

private:
    shared_ptr<HeapMemoryThread> heapThread_ = nullptr;
};

void VideoDecStableTest::SetUpTestCase(void)
{
    (void)InDataVoid;
    (void)OutDataVoid;
    (void)InBufferVoid;
    (void)OutBufferVoid;
}

void VideoDecStableTest::TearDownTestCase(void) {}

void VideoDecStableTest::SetUp(void)
{
    heapThread_ = make_shared<HeapMemoryThread>();
}

void VideoDecStableTest::TearDown(void)
{
    heapThread_ = nullptr;
}

string GetTestName()
{
    const ::testing::TestInfo *testInfo = ::testing::UnitTest::GetInstance()->current_test_info();
    string fileName = testInfo->name();
    fileName = fileName + (VideoDecSample::isHardware_ ? "_H" : "_F");
    auto check = [](char it) { return it == '/'; };
    (void)fileName.erase(std::remove_if(fileName.begin(), fileName.end(), check), fileName.end());
    return fileName;
}

/**.
 * @tc.name: VideoDecoder_Multithread_Release_001
 * @tc.desc: 1. push/free buffer in callback;
 *           2. release not in callback;
 */
HWMTEST_F(VideoDecStableTest, VideoDecoder_Multithread_Release_001, TestSize.Level1, VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VideoDecSignal>(vdec);
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
    EXPECT_EQ(vdec->SetCallback(cb, signal), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(vdec->Configure(), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << SAMPLE_ID;

    EXPECT_TRUE(vdec->WaitForEos()) << SAMPLE_ID;
    EXPECT_EQ(vdec->Release(), AV_ERR_OK) << SAMPLE_ID;
}

/**
 * @tc.name: VideoDecoder_Multithread_Release_AVBuffer_001
 * @tc.desc: 1. push/free buffer in callback;
 *           2. release not in callback;
 */
HWMTEST_F(VideoDecStableTest, VideoDecoder_Multithread_Release_AVBuffer_001, TestSize.Level1,
          VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VideoDecSignal>(vdec);
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
    EXPECT_EQ(vdec->RegisterCallback(cb, signal), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(vdec->Configure(), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << SAMPLE_ID;

    EXPECT_TRUE(vdec->WaitForEos()) << SAMPLE_ID;
    EXPECT_EQ(vdec->Release(), AV_ERR_OK) << SAMPLE_ID;
}

/**.
 * @tc.name: VideoDecoder_Multithread_CreateByMime_001
 * @tc.desc: 1. push/free buffer in callback;
 *           2. release not in callback;
 */
HWMTEST_F(VideoDecStableTest, VideoDecoder_Multithread_CreateByMime_001, TestSize.Level1, VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VideoDecSignal>(vdec);
    vdec->frameCount_ = 30; // 30: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    vdec->inPath_ = "720_1280_25_avcc.h264";
    vdec->outPath_ = GetTestName();
    EXPECT_EQ(vdec->CreateByMime(), true);
    struct OH_AVCodecAsyncCallback cb;
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputData = InDataHandle;
    cb.onNeedOutputData = OutDataHandle;
    signal->isRunning_ = true;
    EXPECT_EQ(vdec->SetCallback(cb, signal), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(vdec->Configure(), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << SAMPLE_ID;

    EXPECT_TRUE(vdec->WaitForEos()) << SAMPLE_ID;
    EXPECT_EQ(vdec->Release(), AV_ERR_OK) << SAMPLE_ID;
}

/**
 * @tc.name: VideoDecoder_Multithread_CreateByMime_AVBuffer_001
 * @tc.desc: 1. push/free buffer in callback;
 *           2. release not in callback;
 */
HWMTEST_F(VideoDecStableTest, VideoDecoder_Multithread_CreateByMime_AVBuffer_001, TestSize.Level1,
          VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VideoDecSignal>(vdec);
    vdec->frameCount_ = 30; // 30: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    vdec->inPath_ = "720_1280_25_avcc.h264";
    vdec->outPath_ = GetTestName();
    EXPECT_EQ(vdec->CreateByMime(), true);
    struct OH_AVCodecCallback cb;
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputBuffer = InBufferHandle;
    cb.onNewOutputBuffer = OutBufferHandle;
    signal->isRunning_ = true;
    EXPECT_EQ(vdec->RegisterCallback(cb, signal), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(vdec->Configure(), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << SAMPLE_ID;

    EXPECT_TRUE(vdec->WaitForEos()) << SAMPLE_ID;
    EXPECT_EQ(vdec->Release(), AV_ERR_OK) << SAMPLE_ID;
}

INSTANTIATE_TEST_SUITE_P(, VideoDecStableTest, testing::Values("Flush", "Stop", "Reset", "SetOutputSurface"));

/**
 * @tc.name: VideoDecoder_Multithread_001
 * @tc.desc: 1. push/free buffer in callback;
 *           2. operate not in callback;
 */
AVCODEC_MTEST_P(VideoDecStableTest, VideoDecoder_Multithread_001, TestSize.Level1, VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VideoDecSignal>(vdec);
    vdec->operation_ = VideoDecStableTest::GetParam();
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
    EXPECT_EQ(vdec->SetCallback(cb, signal), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(vdec->Configure(), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(vdec->Operate(), AV_ERR_OK) << SAMPLE_ID;

    EXPECT_TRUE(vdec->WaitForEos()) << SAMPLE_ID;
    EXPECT_EQ(vdec->Release(), AV_ERR_OK) << SAMPLE_ID;
}

/**
 * @tc.name: VideoDecoder_Multithread_002
 * @tc.desc: 1. push/free buffer in callback;
 *           2. operate in input callback;
 */
AVCODEC_MTEST_P(VideoDecStableTest, VideoDecoder_Multithread_002, TestSize.Level1, VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VideoDecSignal>(vdec);
    vdec->operation_ = VideoDecStableTest::GetParam();
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    vdec->inPath_ = "720_1280_25_avcc.h264";
    vdec->outPath_ = GetTestName();
    EXPECT_EQ(vdec->Create(), true);
    struct OH_AVCodecAsyncCallback cb;
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputData = InDataOperate;
    cb.onNeedOutputData = OutDataHandle;
    signal->isRunning_ = true;
    EXPECT_EQ(vdec->SetCallback(cb, signal), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(vdec->Configure(), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << SAMPLE_ID;

    EXPECT_TRUE(vdec->WaitForEos()) << SAMPLE_ID;
    EXPECT_EQ(vdec->Release(), AV_ERR_OK) << SAMPLE_ID;
}

/**
 * @tc.name: VideoDecoder_Multithread_003
 * @tc.desc: 1. push/free buffer in callback;
 *           2. operate in output callback;
 */
AVCODEC_MTEST_P(VideoDecStableTest, VideoDecoder_Multithread_003, TestSize.Level1, VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VideoDecSignal>(vdec);
    vdec->operation_ = VideoDecStableTest::GetParam();
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    vdec->inPath_ = "720_1280_25_avcc.h264";
    vdec->outPath_ = GetTestName();
    EXPECT_EQ(vdec->Create(), true);
    struct OH_AVCodecAsyncCallback cb;
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputData = InDataHandle;
    cb.onNeedOutputData = OutDataOperate;
    signal->isRunning_ = true;
    EXPECT_EQ(vdec->SetCallback(cb, signal), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(vdec->Configure(), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << SAMPLE_ID;

    EXPECT_TRUE(vdec->WaitForEos()) << SAMPLE_ID;
    EXPECT_EQ(vdec->Release(), AV_ERR_OK) << SAMPLE_ID;
}

/**
 * @tc.name: VideoDecoder_Multithread_004
 * @tc.desc: 1. push/free buffer in callback;
 *           2. operate not in callback;
 *           3. set surface;
 */
AVCODEC_MTEST_P(VideoDecStableTest, VideoDecoder_Multithread_004, TestSize.Level1, VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VideoDecSignal>(vdec);
    vdec->operation_ = VideoDecStableTest::GetParam();
    vdec->frameCount_ = 60; // 60: input frame num
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
    EXPECT_EQ(vdec->SetCallback(cb, signal), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(vdec->Configure(), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(vdec->SetOutputSurface(), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(vdec->Operate(), AV_ERR_OK) << SAMPLE_ID;

    EXPECT_TRUE(vdec->WaitForEos()) << SAMPLE_ID;
    EXPECT_EQ(vdec->Release(), AV_ERR_OK) << SAMPLE_ID;
}

/**
 * @tc.name: VideoDecoder_Multithread_005
 * @tc.desc: 1. push/free buffer in callback;
 *           2. operate in input callback;
 *           3. set surface;
 */
AVCODEC_MTEST_P(VideoDecStableTest, VideoDecoder_Multithread_005, TestSize.Level1, VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VideoDecSignal>(vdec);
    vdec->operation_ = VideoDecStableTest::GetParam();
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    vdec->inPath_ = "720_1280_25_avcc.h264";
    vdec->outPath_ = GetTestName();
    EXPECT_EQ(vdec->Create(), true);
    struct OH_AVCodecAsyncCallback cb;
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputData = InDataOperate;
    cb.onNeedOutputData = OutDataHandle;
    signal->isRunning_ = true;
    EXPECT_EQ(vdec->SetCallback(cb, signal), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(vdec->Configure(), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(vdec->SetOutputSurface(), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << SAMPLE_ID;

    EXPECT_TRUE(vdec->WaitForEos()) << SAMPLE_ID;
    EXPECT_EQ(vdec->Release(), AV_ERR_OK) << SAMPLE_ID;
}

/**
 * @tc.name: VideoDecoder_Multithread_With_Queue_001
 * @tc.desc: 1. push/free buffer in queue;
 *           2. operate not in callback;
 */
AVCODEC_MTEST_P(VideoDecStableTest, VideoDecoder_Multithread_With_Queue_001, TestSize.Level1,
                VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VideoDecSignal>(vdec);
    vdec->operation_ = VideoDecStableTest::GetParam();
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
    EXPECT_EQ(vdec->SetCallback(cb, signal), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(vdec->Configure(), AV_ERR_OK) << SAMPLE_ID;
    signal->isRunning_ = true;
    vdec->inputLoop_ = make_unique<thread>([&signal]() { InputBufferLoop(signal); });
    vdec->outputLoop_ = make_unique<thread>([&signal]() { OutputBufferLoop(signal); });
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(vdec->Operate(), AV_ERR_OK) << SAMPLE_ID;

    EXPECT_TRUE(vdec->WaitForEos()) << SAMPLE_ID;
    EXPECT_EQ(vdec->Release(), AV_ERR_OK) << SAMPLE_ID;
}

/**
 * @tc.name: VideoDecoder_Multithread_With_Queue_002
 * @tc.desc: 1. push/free buffer in queue;
 *           2. operate in input callback;
 */
AVCODEC_MTEST_P(VideoDecStableTest, VideoDecoder_Multithread_With_Queue_002, TestSize.Level1,
                VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VideoDecSignal>(vdec);
    vdec->operation_ = VideoDecStableTest::GetParam();
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    vdec->inPath_ = "720_1280_25_avcc.h264";
    vdec->outPath_ = GetTestName();
    EXPECT_EQ(vdec->Create(), true);
    struct OH_AVCodecAsyncCallback cb;
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputData = InDataOperate;
    cb.onNeedOutputData = OutDataQueue;
    EXPECT_EQ(vdec->SetCallback(cb, signal), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(vdec->Configure(), AV_ERR_OK) << SAMPLE_ID;
    signal->isRunning_ = true;
    vdec->outputLoop_ = make_unique<thread>([&signal]() { OutputBufferLoop(signal); });
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << SAMPLE_ID;

    EXPECT_TRUE(vdec->WaitForEos()) << SAMPLE_ID;
    EXPECT_EQ(vdec->Release(), AV_ERR_OK) << SAMPLE_ID;
}

/**
 * @tc.name: VideoDecoder_Multithread_With_Queue_003
 * @tc.desc: 1. push buffer in callback;
 *           2. operate in output callback;
 *           3. free buffer in queue;
 */
AVCODEC_MTEST_P(VideoDecStableTest, VideoDecoder_Multithread_With_Queue_003, TestSize.Level1,
                VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VideoDecSignal>(vdec);
    vdec->operation_ = VideoDecStableTest::GetParam();
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    vdec->inPath_ = "720_1280_25_avcc.h264";
    vdec->outPath_ = GetTestName();
    EXPECT_EQ(vdec->Create(), true);
    struct OH_AVCodecAsyncCallback cb;
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputData = InDataQueue;
    cb.onNeedOutputData = OutDataOperate;
    EXPECT_EQ(vdec->SetCallback(cb, signal), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(vdec->Configure(), AV_ERR_OK) << SAMPLE_ID;
    signal->isRunning_ = true;
    vdec->inputLoop_ = make_unique<thread>([&signal]() { InputBufferLoop(signal); });
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << SAMPLE_ID;

    EXPECT_TRUE(vdec->WaitForEos()) << SAMPLE_ID;
    EXPECT_EQ(vdec->Release(), AV_ERR_OK) << SAMPLE_ID;
}

/**
 * @tc.name: VideoDecoder_Multithread_With_Queue_004
 * @tc.desc: 1. push buffer in callback;
 *           2. operate not in callback;
 *           3. render buffer in queue;
 *           4. set surface;
 */
AVCODEC_MTEST_P(VideoDecStableTest, VideoDecoder_Multithread_With_Queue_004, TestSize.Level1,
                VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VideoDecSignal>(vdec);
    vdec->operation_ = VideoDecStableTest::GetParam();
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    vdec->inPath_ = "720_1280_25_avcc.h264";
    vdec->outPath_ = GetTestName();
    EXPECT_EQ(vdec->Create(), true);
    struct OH_AVCodecAsyncCallback cb;
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputData = InDataHandle;
    cb.onNeedOutputData = OutDataQueue;
    EXPECT_EQ(vdec->SetCallback(cb, signal), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(vdec->Configure(), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(vdec->SetOutputSurface(), AV_ERR_OK) << SAMPLE_ID;
    signal->isRunning_ = true;
    vdec->outputLoop_ = make_unique<thread>([&signal]() { OutputBufferLoop(signal); });
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(vdec->Operate(), AV_ERR_OK) << SAMPLE_ID;

    EXPECT_TRUE(vdec->WaitForEos()) << SAMPLE_ID;
    EXPECT_EQ(vdec->Release(), AV_ERR_OK) << SAMPLE_ID;
}

/**
 * @tc.name: VideoDecoder_Multithread_With_Queue_005
 * @tc.desc: 1. push buffer in callback;
 *           2. operate in input callback;
 *           3. render buffer in queue;
 *           4. set surface;
 */
AVCODEC_MTEST_P(VideoDecStableTest, VideoDecoder_Multithread_With_Queue_005, TestSize.Level1,
                VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VideoDecSignal>(vdec);
    vdec->operation_ = VideoDecStableTest::GetParam();
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    vdec->inPath_ = "720_1280_25_avcc.h264";
    vdec->outPath_ = GetTestName();
    EXPECT_EQ(vdec->Create(), true);
    struct OH_AVCodecAsyncCallback cb;
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputData = InDataOperate;
    cb.onNeedOutputData = OutDataQueue;
    EXPECT_EQ(vdec->SetCallback(cb, signal), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(vdec->Configure(), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(vdec->SetOutputSurface(), AV_ERR_OK) << SAMPLE_ID;
    signal->isRunning_ = true;
    vdec->outputLoop_ = make_unique<thread>([&signal]() { OutputBufferLoop(signal); });
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << SAMPLE_ID;

    EXPECT_TRUE(vdec->WaitForEos()) << SAMPLE_ID;
    EXPECT_EQ(vdec->Release(), AV_ERR_OK) << SAMPLE_ID;
}

/**
 * @tc.name: VideoDecoder_Multithread_AVBuffer_001
 * @tc.desc: 1. push/free buffer in callback;
 *           2. operate not in callback;
 */
AVCODEC_MTEST_P(VideoDecStableTest, VideoDecoder_Multithread_AVBuffer_001, TestSize.Level1, VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VideoDecSignal>(vdec);
    vdec->operation_ = VideoDecStableTest::GetParam();
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
    EXPECT_EQ(vdec->RegisterCallback(cb, signal), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(vdec->Configure(), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(vdec->Operate(), AV_ERR_OK) << SAMPLE_ID;

    EXPECT_TRUE(vdec->WaitForEos()) << SAMPLE_ID;
    EXPECT_EQ(vdec->Release(), AV_ERR_OK) << SAMPLE_ID;
}

/**
 * @tc.name: VideoDecoder_Multithread_AVBuffer_002
 * @tc.desc: 1. push/free buffer in callback;
 *           2. operate in input callback;
 */
AVCODEC_MTEST_P(VideoDecStableTest, VideoDecoder_Multithread_AVBuffer_002, TestSize.Level1, VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VideoDecSignal>(vdec);
    vdec->operation_ = VideoDecStableTest::GetParam();
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    vdec->inPath_ = "720_1280_25_avcc.h264";
    vdec->outPath_ = GetTestName();
    EXPECT_EQ(vdec->Create(), true);
    struct OH_AVCodecCallback cb;
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputBuffer = InBufferOperate;
    cb.onNewOutputBuffer = OutBufferHandle;
    signal->isRunning_ = true;
    EXPECT_EQ(vdec->RegisterCallback(cb, signal), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(vdec->Configure(), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << SAMPLE_ID;

    EXPECT_TRUE(vdec->WaitForEos()) << SAMPLE_ID;
    EXPECT_EQ(vdec->Release(), AV_ERR_OK) << SAMPLE_ID;
}

/**
 * @tc.name: VideoDecoder_Multithread_AVBuffer_003
 * @tc.desc: 1. push/free buffer in callback;
 *           2. operate in output callback;
 */
AVCODEC_MTEST_P(VideoDecStableTest, VideoDecoder_Multithread_AVBuffer_003, TestSize.Level1, VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VideoDecSignal>(vdec);
    vdec->operation_ = VideoDecStableTest::GetParam();
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    vdec->inPath_ = "720_1280_25_avcc.h264";
    vdec->outPath_ = GetTestName();
    EXPECT_EQ(vdec->Create(), true);
    struct OH_AVCodecCallback cb;
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputBuffer = InBufferHandle;
    cb.onNewOutputBuffer = OutBufferOperate;
    signal->isRunning_ = true;
    EXPECT_EQ(vdec->RegisterCallback(cb, signal), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(vdec->Configure(), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << SAMPLE_ID;

    EXPECT_TRUE(vdec->WaitForEos()) << SAMPLE_ID;
    EXPECT_EQ(vdec->Release(), AV_ERR_OK) << SAMPLE_ID;
}

/**
 * @tc.name: VideoDecoder_Multithread_AVBuffer_004
 * @tc.desc: 1. push/free buffer in callback;
 *           2. operate not in callback;
 *           3. set surface;
 */
AVCODEC_MTEST_P(VideoDecStableTest, VideoDecoder_Multithread_AVBuffer_004, TestSize.Level1, VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VideoDecSignal>(vdec);
    vdec->operation_ = VideoDecStableTest::GetParam();
    vdec->frameCount_ = 60; // 60: input frame num
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
    EXPECT_EQ(vdec->RegisterCallback(cb, signal), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(vdec->Configure(), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(vdec->SetOutputSurface(), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(vdec->Operate(), AV_ERR_OK) << SAMPLE_ID;

    EXPECT_TRUE(vdec->WaitForEos()) << SAMPLE_ID;
    EXPECT_EQ(vdec->Release(), AV_ERR_OK) << SAMPLE_ID;
}

/**
 * @tc.name: VideoDecoder_Multithread_AVBuffer_005
 * @tc.desc: 1. push/free buffer in callback;
 *           2. operate in input callback;
 *           3. set surface;
 */
AVCODEC_MTEST_P(VideoDecStableTest, VideoDecoder_Multithread_AVBuffer_005, TestSize.Level1, VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VideoDecSignal>(vdec);
    vdec->operation_ = VideoDecStableTest::GetParam();
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    vdec->inPath_ = "720_1280_25_avcc.h264";
    vdec->outPath_ = GetTestName();
    EXPECT_EQ(vdec->Create(), true);
    struct OH_AVCodecCallback cb;
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputBuffer = InBufferOperate;
    cb.onNewOutputBuffer = OutBufferHandle;
    signal->isRunning_ = true;
    EXPECT_EQ(vdec->RegisterCallback(cb, signal), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(vdec->Configure(), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(vdec->SetOutputSurface(), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << SAMPLE_ID;

    EXPECT_TRUE(vdec->WaitForEos()) << SAMPLE_ID;
    EXPECT_EQ(vdec->Release(), AV_ERR_OK) << SAMPLE_ID;
}

/**
 * @tc.name: VideoDecoder_Multithread_AVBuffer_With_Queue_001
 * @tc.desc: 1. push/free buffer in queue;
 *           2. operate not in callback;
 */
AVCODEC_MTEST_P(VideoDecStableTest, VideoDecoder_Multithread_AVBuffer_With_Queue_001, TestSize.Level1,
                VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VideoDecSignal>(vdec);
    vdec->operation_ = VideoDecStableTest::GetParam();
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
    EXPECT_EQ(vdec->RegisterCallback(cb, signal), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(vdec->Configure(), AV_ERR_OK) << SAMPLE_ID;
    signal->isRunning_ = true;
    vdec->inputLoop_ = make_unique<thread>([&signal]() { InputBufferLoop(signal); });
    vdec->outputLoop_ = make_unique<thread>([&signal]() { OutputBufferLoop(signal); });
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(vdec->Operate(), AV_ERR_OK) << SAMPLE_ID;

    EXPECT_TRUE(vdec->WaitForEos()) << SAMPLE_ID;
    EXPECT_EQ(vdec->Release(), AV_ERR_OK) << SAMPLE_ID;
}

/**
 * @tc.name: VideoDecoder_Multithread_AVBuffer_With_Queue_002
 * @tc.desc: 1. push/free buffer in queue;
 *           2. operate in input callback;
 */
AVCODEC_MTEST_P(VideoDecStableTest, VideoDecoder_Multithread_AVBuffer_With_Queue_002, TestSize.Level1,
                VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VideoDecSignal>(vdec);
    vdec->operation_ = VideoDecStableTest::GetParam();
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    vdec->inPath_ = "720_1280_25_avcc.h264";
    vdec->outPath_ = GetTestName();
    EXPECT_EQ(vdec->Create(), true);
    struct OH_AVCodecCallback cb;
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputBuffer = InBufferOperate;
    cb.onNewOutputBuffer = OutBufferQueue;
    EXPECT_EQ(vdec->RegisterCallback(cb, signal), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(vdec->Configure(), AV_ERR_OK) << SAMPLE_ID;
    signal->isRunning_ = true;
    vdec->outputLoop_ = make_unique<thread>([&signal]() { OutputBufferLoop(signal); });
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << SAMPLE_ID;

    EXPECT_TRUE(vdec->WaitForEos()) << SAMPLE_ID;
    EXPECT_EQ(vdec->Release(), AV_ERR_OK) << SAMPLE_ID;
}

/**
 * @tc.name: VideoDecoder_Multithread_AVBuffer_With_Queue_003
 * @tc.desc: 1. push buffer in callback;
 *           2. operate in output callback;
 *           3. free buffer in queue;
 */
AVCODEC_MTEST_P(VideoDecStableTest, VideoDecoder_Multithread_AVBuffer_With_Queue_003, TestSize.Level1,
                VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VideoDecSignal>(vdec);
    vdec->operation_ = VideoDecStableTest::GetParam();
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    vdec->inPath_ = "720_1280_25_avcc.h264";
    vdec->outPath_ = GetTestName();
    EXPECT_EQ(vdec->Create(), true);
    struct OH_AVCodecCallback cb;
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputBuffer = InBufferQueue;
    cb.onNewOutputBuffer = OutBufferOperate;
    EXPECT_EQ(vdec->RegisterCallback(cb, signal), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(vdec->Configure(), AV_ERR_OK) << SAMPLE_ID;
    signal->isRunning_ = true;
    vdec->inputLoop_ = make_unique<thread>([&signal]() { InputBufferLoop(signal); });
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << SAMPLE_ID;

    EXPECT_TRUE(vdec->WaitForEos()) << SAMPLE_ID;
    EXPECT_EQ(vdec->Release(), AV_ERR_OK) << SAMPLE_ID;
}

/**
 * @tc.name: VideoDecoder_Multithread_AVBuffer_With_Queue_004
 * @tc.desc: 1. push buffer in callback;
 *           2. operate not in callback;
 *           3. render buffer in queue;
 *           4. set surface;
 */
AVCODEC_MTEST_P(VideoDecStableTest, VideoDecoder_Multithread_AVBuffer_With_Queue_004, TestSize.Level1,
                VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VideoDecSignal>(vdec);
    vdec->operation_ = VideoDecStableTest::GetParam();
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    vdec->inPath_ = "720_1280_25_avcc.h264";
    vdec->outPath_ = GetTestName();
    EXPECT_EQ(vdec->Create(), true);
    struct OH_AVCodecCallback cb;
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputBuffer = InBufferHandle;
    cb.onNewOutputBuffer = OutBufferQueue;
    EXPECT_EQ(vdec->RegisterCallback(cb, signal), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(vdec->Configure(), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(vdec->SetOutputSurface(), AV_ERR_OK) << SAMPLE_ID;
    signal->isRunning_ = true;
    vdec->outputLoop_ = make_unique<thread>([&signal]() { OutputBufferLoop(signal); });
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(vdec->Operate(), AV_ERR_OK) << SAMPLE_ID;

    EXPECT_TRUE(vdec->WaitForEos()) << SAMPLE_ID;
    EXPECT_EQ(vdec->Release(), AV_ERR_OK) << SAMPLE_ID;
}

/**
 * @tc.name: VideoDecoder_Multithread_AVBuffer_With_Queue_005
 * @tc.desc: 1. push buffer in callback;
 *           2. operate in input callback;
 *           3. render buffer in queue;
 *           4. set surface;
 */
AVCODEC_MTEST_P(VideoDecStableTest, VideoDecoder_Multithread_AVBuffer_With_Queue_005, TestSize.Level1,
                VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VideoDecSignal>(vdec);
    vdec->operation_ = VideoDecStableTest::GetParam();
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    vdec->inPath_ = "720_1280_25_avcc.h264";
    vdec->outPath_ = GetTestName();
    EXPECT_EQ(vdec->Create(), true);
    struct OH_AVCodecCallback cb;
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputBuffer = InBufferOperate;
    cb.onNewOutputBuffer = OutBufferQueue;
    EXPECT_EQ(vdec->RegisterCallback(cb, signal), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(vdec->Configure(), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(vdec->SetOutputSurface(), AV_ERR_OK) << SAMPLE_ID;
    signal->isRunning_ = true;
    vdec->outputLoop_ = make_unique<thread>([&signal]() { OutputBufferLoop(signal); });
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << SAMPLE_ID;

    EXPECT_TRUE(vdec->WaitForEos()) << SAMPLE_ID;
    EXPECT_EQ(vdec->Release(), AV_ERR_OK) << SAMPLE_ID;
}
} // namespace

int main(int argc, char **argv)
{
    uint64_t threadNum = 0;
    uint64_t timeout = 0;
    testing::GTEST_FLAG(output) = "xml:./";
    for (int i = 0; i < argc; ++i) {
        cout << argv[i] << endl;
        threadNum = GetNum(argv[i], "--thread_num");
        timeout = GetNum(argv[i], "--timeout");
        if (strcmp(argv[i], "--need_dump") == 0) {
            VideoDecSample::needDump_ = true;
            DecArgv(i, argc, argv);
        } else if (strcmp(argv[i], "--fcodec") == 0) {
            VideoDecSample::isHardware_ = false;
            DecArgv(i, argc, argv);
        } else if (timeout > 0) {
            VideoDecSample::sampleTimout_ = timeout;
            DecArgv(i, argc, argv);
        } else if (threadNum > 0) {
            VideoDecSample::threadNum_ = threadNum;
            DecArgv(i, argc, argv);
        }
    }
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}