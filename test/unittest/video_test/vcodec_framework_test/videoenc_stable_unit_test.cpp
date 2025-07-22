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
#include "avcodec_log.h"
#include "heap_memory_thread.h"
#include "unittest_utils.h"
#include "venc_sample.h"

#define PRINT_HILOG
#define TEST_ID venc->sampleId_
#define SAMPLE_ID "[SAMPLE_ID]:" << TEST_ID
#include "unittest_log.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace testing::ext;
using namespace testing::mt;

namespace {

class VideoEncStableTest : public testing::TestWithParam<std::string> {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);
    static constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, STRINGFY(TEST_SUIT)};

private:
    shared_ptr<HeapMemoryThread> heapThread_ = nullptr;
};

void VideoEncStableTest::SetUpTestCase(void) {}

void VideoEncStableTest::TearDownTestCase(void) {}

void VideoEncStableTest::SetUp(void)
{
    heapThread_ = make_shared<HeapMemoryThread>();

    const ::testing::TestInfo *testInfo_ = ::testing::UnitTest::GetInstance()->current_test_info();
    std::string testCaseName = testInfo_->name();
    AVCODEC_LOGI("%{public}s", testCaseName.c_str());
}

void VideoEncStableTest::TearDown(void)
{
    heapThread_ = nullptr;
}

string GetTestName()
{
    const ::testing::TestInfo *testInfo = ::testing::UnitTest::GetInstance()->current_test_info();
    string fileName = testInfo->name();
    auto check = [](char it) { return it == '/'; };
    (void)fileName.erase(std::remove_if(fileName.begin(), fileName.end(), check), fileName.end());
    return fileName;
}

/**
 * @tc.name: VideoEncoder_Multithread_Release_001
 * @tc.desc: 1. push/free buffer in callback;
 *           2. release not in callback;
 */
HWMTEST_F(VideoEncStableTest, VideoEncoder_Multithread_Release_001, TestSize.Level1, VideoEncSample::threadNum_)
{
    auto venc = make_shared<VideoEncSample>();
    auto signal = make_shared<VCodecSignal>(venc);
    venc->frameCount_ = 30; // 30: input frame num
    venc->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    venc->inPath_ = "1280_720_nv.yuv";
    venc->outPath_ = GetTestName();
    EXPECT_EQ(venc->Create(), true);
    struct OH_AVCodecAsyncCallback cb;
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputData = InDataHandle;
    cb.onNeedOutputData = OutDataHandle;
    signal->isRunning_ = true;
    EXPECT_EQ(venc->SetCallback(cb, signal), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(venc->Configure(), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(venc->Start(), AV_ERR_OK) << SAMPLE_ID;

    EXPECT_TRUE(venc->WaitForEos()) << SAMPLE_ID;
    EXPECT_EQ(venc->Release(), AV_ERR_OK) << SAMPLE_ID;
}

/**
 * @tc.name: VideoEncoder_Multithread_Release_AVBuffer_001
 * @tc.desc: 1. push/free buffer in callback;
 *           2. release not in callback;
 */
HWMTEST_F(VideoEncStableTest, VideoEncoder_Multithread_Release_AVBuffer_001, TestSize.Level1,
          VideoEncSample::threadNum_)
{
    auto venc = make_shared<VideoEncSample>();
    auto signal = make_shared<VCodecSignal>(venc);
    venc->frameCount_ = 30; // 30: input frame num
    venc->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    venc->inPath_ = "1280_720_nv.yuv";
    venc->outPath_ = GetTestName();
    EXPECT_EQ(venc->Create(), true);
    struct OH_AVCodecCallback cb;
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputBuffer = InBufferHandle;
    cb.onNewOutputBuffer = OutBufferHandle;
    signal->isRunning_ = true;
    EXPECT_EQ(venc->RegisterCallback(cb, signal), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(venc->Configure(), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(venc->Start(), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_TRUE(venc->WaitForEos()) << SAMPLE_ID;
    EXPECT_EQ(venc->Release(), AV_ERR_OK) << SAMPLE_ID;
}

INSTANTIATE_TEST_SUITE_P(, VideoEncStableTest, testing::Values("Flush", "Stop", "Reset"));

/**
 * @tc.name: VideoEncoder_Multithread_001
 * @tc.desc: 1. push/free buffer in callback;
 *           2. operate not in callback;
 */
AVCODEC_MTEST_P(VideoEncStableTest, VideoEncoder_Multithread_001, TestSize.Level1, VideoEncSample::threadNum_)
{
    auto venc = make_shared<VideoEncSample>();
    auto signal = make_shared<VCodecSignal>(venc);
    venc->operation_ = VideoEncStableTest::GetParam();
    venc->frameCount_ = 30; // 30: input frame num
    venc->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    venc->inPath_ = "1280_720_nv.yuv";
    venc->outPath_ = GetTestName();
    EXPECT_EQ(venc->Create(), true);
    struct OH_AVCodecAsyncCallback cb;
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputData = InDataHandle;
    cb.onNeedOutputData = OutDataHandle;
    signal->isRunning_ = true;
    EXPECT_EQ(venc->SetCallback(cb, signal), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(venc->Configure(), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(venc->Start(), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(venc->Operate(), AV_ERR_OK) << SAMPLE_ID;

    EXPECT_TRUE(venc->WaitForEos()) << SAMPLE_ID;
    EXPECT_EQ(venc->Release(), AV_ERR_OK) << SAMPLE_ID;
}

/**
 * @tc.name: VideoEncoder_Multithread_002
 * @tc.desc: 1. push/free buffer in callback;
 *           2. operate in input callback;
 */
AVCODEC_MTEST_P(VideoEncStableTest, VideoEncoder_Multithread_002, TestSize.Level1, VideoEncSample::threadNum_)
{
    auto venc = make_shared<VideoEncSample>();
    auto signal = make_shared<VCodecSignal>(venc);
    venc->operation_ = VideoEncStableTest::GetParam();
    venc->frameCount_ = 60; // 60: input frame num
    venc->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    venc->inPath_ = "1280_720_nv.yuv";
    venc->outPath_ = GetTestName();
    EXPECT_EQ(venc->Create(), true);
    struct OH_AVCodecAsyncCallback cb;
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputData = InDataOperate;
    cb.onNeedOutputData = OutDataHandle;
    signal->isRunning_ = true;
    EXPECT_EQ(venc->SetCallback(cb, signal), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(venc->Configure(), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(venc->Start(), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_TRUE(venc->WaitForEos()) << SAMPLE_ID;
    EXPECT_EQ(venc->Release(), AV_ERR_OK) << SAMPLE_ID;
}

/**
 * @tc.name: VideoEncoder_Multithread_003
 * @tc.desc: 1. push/free buffer in callback;
 *           2. operate in output callback;
 */
AVCODEC_MTEST_P(VideoEncStableTest, VideoEncoder_Multithread_003, TestSize.Level1, VideoEncSample::threadNum_)
{
    auto venc = make_shared<VideoEncSample>();
    auto signal = make_shared<VCodecSignal>(venc);
    venc->operation_ = VideoEncStableTest::GetParam();
    venc->frameCount_ = 60; // 60: input frame num
    venc->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    venc->inPath_ = "1280_720_nv.yuv";
    venc->outPath_ = GetTestName();
    EXPECT_EQ(venc->Create(), true);
    struct OH_AVCodecAsyncCallback cb;
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputData = InDataHandle;
    cb.onNeedOutputData = OutDataOperate;
    signal->isRunning_ = true;
    EXPECT_EQ(venc->SetCallback(cb, signal), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(venc->Configure(), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(venc->Start(), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_TRUE(venc->WaitForEos()) << SAMPLE_ID;
    EXPECT_EQ(venc->Release(), AV_ERR_OK) << SAMPLE_ID;
}

/**
 * @tc.name: VideoEncoder_Multithread_004
 * @tc.desc: 1. push/free buffer in callback;
 *           2. operate not in callback;
 *           3. set surface;
 */
AVCODEC_MTEST_P(VideoEncStableTest, VideoEncoder_Multithread_004, TestSize.Level1, VideoEncSample::threadNum_)
{
    auto venc = make_shared<VideoEncSample>();
    auto signal = make_shared<VCodecSignal>(venc);
    venc->operation_ = VideoEncStableTest::GetParam();
    venc->frameCount_ = 60; // 60: input frame num
    venc->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    venc->inPath_ = "1280_720_nv.yuv";
    venc->outPath_ = GetTestName();
    EXPECT_EQ(venc->Create(), true);
    struct OH_AVCodecAsyncCallback cb;
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputData = nullptr;
    cb.onNeedOutputData = OutDataHandle;
    signal->isRunning_ = true;
    EXPECT_EQ(venc->SetCallback(cb, signal), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(venc->Configure(), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(venc->GetInputSurface(), AV_ERR_OK) << SAMPLE_ID;

    EXPECT_EQ(venc->Start(), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(venc->Operate(), AV_ERR_OK) << SAMPLE_ID;

    EXPECT_TRUE(venc->WaitForEos()) << SAMPLE_ID;
    EXPECT_EQ(venc->Release(), AV_ERR_OK) << SAMPLE_ID;
}

/**
 * @tc.name: VideoEncoder_Multithread_005
 * @tc.desc: 1. push/free buffer in callback;
 *           2. operate in output callback;
 *           3. set surface;
 */
AVCODEC_MTEST_P(VideoEncStableTest, VideoEncoder_Multithread_005, TestSize.Level1, VideoEncSample::threadNum_)
{
    auto venc = make_shared<VideoEncSample>();
    auto signal = make_shared<VCodecSignal>(venc);
    venc->operation_ = VideoEncStableTest::GetParam();
    venc->frameCount_ = 60; // 60: input frame num
    venc->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    venc->inPath_ = "1280_720_nv.yuv";
    venc->outPath_ = GetTestName();
    EXPECT_EQ(venc->Create(), true);
    struct OH_AVCodecAsyncCallback cb;
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputData = nullptr;
    cb.onNeedOutputData = OutDataOperate;
    signal->isRunning_ = true;
    EXPECT_EQ(venc->SetCallback(cb, signal), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(venc->Configure(), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(venc->GetInputSurface(), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(venc->Start(), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_TRUE(venc->WaitForEos()) << SAMPLE_ID;
    EXPECT_EQ(venc->Release(), AV_ERR_OK) << SAMPLE_ID;
}

/**
 * @tc.name: VideoEncoder_Multithread_With_Queue_001
 * @tc.desc: 1. push/free buffer in queue;
 *           2. operate not in callback;
 */
AVCODEC_MTEST_P(VideoEncStableTest, VideoEncoder_Multithread_With_Queue_001, TestSize.Level1,
                VideoEncSample::threadNum_)
{
    auto venc = make_shared<VideoEncSample>();
    auto signal = make_shared<VCodecSignal>(venc);
    venc->operation_ = VideoEncStableTest::GetParam();
    venc->frameCount_ = 60; // 60: input frame num
    venc->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    venc->inPath_ = "1280_720_nv.yuv";
    venc->outPath_ = GetTestName();
    EXPECT_EQ(venc->Create(), true);
    struct OH_AVCodecAsyncCallback cb;
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputData = InDataQueue;
    cb.onNeedOutputData = OutDataQueue;
    EXPECT_EQ(venc->SetCallback(cb, signal), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(venc->Configure(), AV_ERR_OK) << SAMPLE_ID;

    signal->isRunning_ = true;
    venc->inputLoop_ = make_unique<thread>([&signal]() { InputBufferLoop(signal); });
    venc->outputLoop_ = make_unique<thread>([&signal]() { OutputBufferLoop(signal); });
    EXPECT_EQ(venc->Start(), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(venc->Operate(), AV_ERR_OK) << SAMPLE_ID;

    EXPECT_TRUE(venc->WaitForEos()) << SAMPLE_ID;
    EXPECT_EQ(venc->Release(), AV_ERR_OK) << SAMPLE_ID;
}

/**
 * @tc.name: VideoEncoder_Multithread_With_Queue_002
 * @tc.desc: 1. push/free buffer in queue;
 *           2. operate in input callback;
 */
AVCODEC_MTEST_P(VideoEncStableTest, VideoEncoder_Multithread_With_Queue_002, TestSize.Level1,
                VideoEncSample::threadNum_)
{
    auto venc = make_shared<VideoEncSample>();
    auto signal = make_shared<VCodecSignal>(venc);
    venc->operation_ = VideoEncStableTest::GetParam();
    venc->frameCount_ = 60; // 60: input frame num
    venc->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    venc->inPath_ = "1280_720_nv.yuv";
    venc->outPath_ = GetTestName();
    EXPECT_EQ(venc->Create(), true);
    struct OH_AVCodecAsyncCallback cb;
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputData = InDataOperate;
    cb.onNeedOutputData = OutDataQueue;
    EXPECT_EQ(venc->SetCallback(cb, signal), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(venc->Configure(), AV_ERR_OK) << SAMPLE_ID;

    signal->isRunning_ = true;
    venc->outputLoop_ = make_unique<thread>([&signal]() { OutputBufferLoop(signal); });
    EXPECT_EQ(venc->Start(), AV_ERR_OK) << SAMPLE_ID;

    EXPECT_TRUE(venc->WaitForEos()) << SAMPLE_ID;
    EXPECT_EQ(venc->Release(), AV_ERR_OK) << SAMPLE_ID;
}

/**
 * @tc.name: VideoEncoder_Multithread_With_Queue_003
 * @tc.desc: 1. push/free buffer in queue;
 *           2. operate in output callback;
 */
AVCODEC_MTEST_P(VideoEncStableTest, VideoEncoder_Multithread_With_Queue_003, TestSize.Level1,
                VideoEncSample::threadNum_)
{
    auto venc = make_shared<VideoEncSample>();
    auto signal = make_shared<VCodecSignal>(venc);
    venc->operation_ = VideoEncStableTest::GetParam();
    venc->frameCount_ = 60; // 60: input frame num
    venc->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    venc->inPath_ = "1280_720_nv.yuv";
    venc->outPath_ = GetTestName();
    EXPECT_EQ(venc->Create(), true);
    struct OH_AVCodecAsyncCallback cb;
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputData = InDataQueue;
    cb.onNeedOutputData = OutDataOperate;
    EXPECT_EQ(venc->SetCallback(cb, signal), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(venc->Configure(), AV_ERR_OK) << SAMPLE_ID;

    signal->isRunning_ = true;
    venc->inputLoop_ = make_unique<thread>([&signal]() { InputBufferLoop(signal); });
    EXPECT_EQ(venc->Start(), AV_ERR_OK) << SAMPLE_ID;

    EXPECT_TRUE(venc->WaitForEos()) << SAMPLE_ID;
    EXPECT_EQ(venc->Release(), AV_ERR_OK) << SAMPLE_ID;
}

/**
 * @tc.name: VideoEncoder_Multithread_With_Queue_004
 * @tc.desc: 1. push/free buffer in queue;
 *           2. operate not in callback;
 *           3. set surface.
 */
AVCODEC_MTEST_P(VideoEncStableTest, VideoEncoder_Multithread_With_Queue_004, TestSize.Level1,
                VideoEncSample::threadNum_)
{
    auto venc = make_shared<VideoEncSample>();
    auto signal = make_shared<VCodecSignal>(venc);
    venc->operation_ = VideoEncStableTest::GetParam();
    venc->frameCount_ = 60; // 60: input frame num
    venc->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    venc->inPath_ = "1280_720_nv.yuv";
    venc->outPath_ = GetTestName();
    EXPECT_EQ(venc->Create(), true);
    struct OH_AVCodecAsyncCallback cb;
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputData = nullptr;
    cb.onNeedOutputData = OutDataQueue;
    signal->isRunning_ = true;
    EXPECT_EQ(venc->SetCallback(cb, signal), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(venc->Configure(), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(venc->GetInputSurface(), AV_ERR_OK) << SAMPLE_ID;

    venc->outputLoop_ = make_unique<thread>([&signal]() { OutputBufferLoop(signal); });
    EXPECT_EQ(venc->Start(), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(venc->Operate(), AV_ERR_OK) << SAMPLE_ID;

    EXPECT_TRUE(venc->WaitForEos()) << SAMPLE_ID;
    EXPECT_EQ(venc->Release(), AV_ERR_OK) << SAMPLE_ID;
}

/**
 * @tc.name: VideoEncoder_Multithread_AVBuffer_001
 * @tc.desc: 1. push/free buffer in callback;
 *           2. operate not in callback;
 */
AVCODEC_MTEST_P(VideoEncStableTest, VideoEncoder_Multithread_AVBuffer_001, TestSize.Level1, VideoEncSample::threadNum_)
{
    auto venc = make_shared<VideoEncSample>();
    auto signal = make_shared<VCodecSignal>(venc);
    venc->operation_ = VideoEncStableTest::GetParam();
    venc->frameCount_ = 30; // 30: input frame num
    venc->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    venc->inPath_ = "1280_720_nv.yuv";
    venc->outPath_ = GetTestName();
    EXPECT_EQ(venc->Create(), true);
    struct OH_AVCodecCallback cb;
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputBuffer = InBufferHandle;
    cb.onNewOutputBuffer = OutBufferHandle;
    signal->isRunning_ = true;
    EXPECT_EQ(venc->RegisterCallback(cb, signal), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(venc->Configure(), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(venc->Start(), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(venc->Operate(), AV_ERR_OK) << SAMPLE_ID;

    EXPECT_TRUE(venc->WaitForEos()) << SAMPLE_ID;
    EXPECT_EQ(venc->Release(), AV_ERR_OK) << SAMPLE_ID;
}

/**
 * @tc.name: VideoEncoder_Multithread_AVBuffer_002
 * @tc.desc: 1. push/free buffer in callback;
 *           2. operate in input callback;
 */
AVCODEC_MTEST_P(VideoEncStableTest, VideoEncoder_Multithread_AVBuffer_002, TestSize.Level1, VideoEncSample::threadNum_)
{
    auto venc = make_shared<VideoEncSample>();
    auto signal = make_shared<VCodecSignal>(venc);
    venc->operation_ = VideoEncStableTest::GetParam();
    venc->frameCount_ = 60; // 60: input frame num
    venc->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    venc->inPath_ = "1280_720_nv.yuv";
    venc->outPath_ = GetTestName();
    EXPECT_EQ(venc->Create(), true);
    struct OH_AVCodecCallback cb;
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputBuffer = InBufferOperate;
    cb.onNewOutputBuffer = OutBufferHandle;
    signal->isRunning_ = true;
    EXPECT_EQ(venc->RegisterCallback(cb, signal), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(venc->Configure(), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(venc->Start(), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_TRUE(venc->WaitForEos()) << SAMPLE_ID;
    EXPECT_EQ(venc->Release(), AV_ERR_OK) << SAMPLE_ID;
}

/**
 * @tc.name: VideoEncoder_Multithread_AVBuffer_003
 * @tc.desc: 1. push/free buffer in callback;
 *           2. operate in output callback;
 */
AVCODEC_MTEST_P(VideoEncStableTest, VideoEncoder_Multithread_AVBuffer_003, TestSize.Level1, VideoEncSample::threadNum_)
{
    auto venc = make_shared<VideoEncSample>();
    auto signal = make_shared<VCodecSignal>(venc);
    venc->operation_ = VideoEncStableTest::GetParam();
    venc->frameCount_ = 60; // 60: input frame num
    venc->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    venc->inPath_ = "1280_720_nv.yuv";
    venc->outPath_ = GetTestName();
    EXPECT_EQ(venc->Create(), true);
    struct OH_AVCodecCallback cb;
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputBuffer = InBufferHandle;
    cb.onNewOutputBuffer = OutBufferOperate;
    signal->isRunning_ = true;
    EXPECT_EQ(venc->RegisterCallback(cb, signal), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(venc->Configure(), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(venc->Start(), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_TRUE(venc->WaitForEos()) << SAMPLE_ID;
    EXPECT_EQ(venc->Release(), AV_ERR_OK) << SAMPLE_ID;
}

/**
 * @tc.name: VideoEncoder_Multithread_AVBuffer_004
 * @tc.desc: 1. push/free buffer in callback;
 *           2. operate not in callback;
 *           3. set surface;
 */
AVCODEC_MTEST_P(VideoEncStableTest, VideoEncoder_Multithread_AVBuffer_004, TestSize.Level1, VideoEncSample::threadNum_)
{
    auto venc = make_shared<VideoEncSample>();
    auto signal = make_shared<VCodecSignal>(venc);
    venc->operation_ = VideoEncStableTest::GetParam();
    venc->frameCount_ = 60; // 60: input frame num
    venc->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    venc->inPath_ = "1280_720_nv.yuv";
    venc->outPath_ = GetTestName();
    EXPECT_EQ(venc->Create(), true);
    struct OH_AVCodecCallback cb;
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputBuffer = nullptr;
    cb.onNewOutputBuffer = OutBufferHandle;
    signal->isRunning_ = true;
    EXPECT_EQ(venc->RegisterCallback(cb, signal), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(venc->Configure(), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(venc->GetInputSurface(), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(venc->Start(), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(venc->Operate(), AV_ERR_OK) << SAMPLE_ID;

    EXPECT_TRUE(venc->WaitForEos()) << SAMPLE_ID;
    EXPECT_EQ(venc->Release(), AV_ERR_OK) << SAMPLE_ID;
}

/**
 * @tc.name: VideoEncoder_Multithread_AVBuffer_005
 * @tc.desc: 1. push/free buffer in callback;
 *           2. operate in output callback;
 *           3. set surface;
 */
AVCODEC_MTEST_P(VideoEncStableTest, VideoEncoder_Multithread_AVBuffer_005, TestSize.Level1, VideoEncSample::threadNum_)
{
    auto venc = make_shared<VideoEncSample>();
    auto signal = make_shared<VCodecSignal>(venc);
    venc->operation_ = VideoEncStableTest::GetParam();
    venc->frameCount_ = 60; // 60: input frame num
    venc->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    venc->inPath_ = "1280_720_nv.yuv";
    venc->outPath_ = GetTestName();
    EXPECT_EQ(venc->Create(), true);
    struct OH_AVCodecCallback cb;
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputBuffer = nullptr;
    cb.onNewOutputBuffer = OutBufferOperate;
    signal->isRunning_ = true;
    EXPECT_EQ(venc->RegisterCallback(cb, signal), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(venc->Configure(), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(venc->GetInputSurface(), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(venc->Start(), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_TRUE(venc->WaitForEos()) << SAMPLE_ID;
    EXPECT_EQ(venc->Release(), AV_ERR_OK) << SAMPLE_ID;
}

/**
 * @tc.name: VideoEncoder_Multithread_AVBuffer_With_Queue_001
 * @tc.desc: 1. push/free buffer in queue;
 *           2. operate not in callback;
 */
AVCODEC_MTEST_P(VideoEncStableTest, VideoEncoder_Multithread_AVBuffer_With_Queue_001, TestSize.Level1,
                VideoEncSample::threadNum_)
{
    auto venc = make_shared<VideoEncSample>();
    auto signal = make_shared<VCodecSignal>(venc);
    venc->operation_ = VideoEncStableTest::GetParam();
    venc->frameCount_ = 60; // 60: input frame num
    venc->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    venc->inPath_ = "1280_720_nv.yuv";
    venc->outPath_ = GetTestName();
    EXPECT_EQ(venc->Create(), true);
    struct OH_AVCodecCallback cb;
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputBuffer = InBufferQueue;
    cb.onNewOutputBuffer = OutBufferQueue;
    EXPECT_EQ(venc->RegisterCallback(cb, signal), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(venc->Configure(), AV_ERR_OK) << SAMPLE_ID;

    signal->isRunning_ = true;
    venc->inputLoop_ = make_unique<thread>([&signal]() { InputBufferLoop(signal); });
    venc->outputLoop_ = make_unique<thread>([&signal]() { OutputBufferLoop(signal); });
    EXPECT_EQ(venc->Start(), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(venc->Operate(), AV_ERR_OK) << SAMPLE_ID;

    EXPECT_TRUE(venc->WaitForEos()) << SAMPLE_ID;
    EXPECT_EQ(venc->Release(), AV_ERR_OK) << SAMPLE_ID;
}

/**
 * @tc.name: VideoEncoder_Multithread_AVBuffer_With_Queue_002
 * @tc.desc: 1. push/free buffer in queue;
 *           2. operate in input callback;
 */
AVCODEC_MTEST_P(VideoEncStableTest, VideoEncoder_Multithread_AVBuffer_With_Queue_002, TestSize.Level1,
                VideoEncSample::threadNum_)
{
    auto venc = make_shared<VideoEncSample>();
    auto signal = make_shared<VCodecSignal>(venc);
    venc->operation_ = VideoEncStableTest::GetParam();
    venc->frameCount_ = 60; // 60: input frame num
    venc->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    venc->inPath_ = "1280_720_nv.yuv";
    venc->outPath_ = GetTestName();
    EXPECT_EQ(venc->Create(), true);
    struct OH_AVCodecCallback cb;
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputBuffer = InBufferOperate;
    cb.onNewOutputBuffer = OutBufferQueue;
    EXPECT_EQ(venc->RegisterCallback(cb, signal), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(venc->Configure(), AV_ERR_OK) << SAMPLE_ID;

    signal->isRunning_ = true;
    venc->outputLoop_ = make_unique<thread>([&signal]() { OutputBufferLoop(signal); });
    EXPECT_EQ(venc->Start(), AV_ERR_OK) << SAMPLE_ID;

    EXPECT_TRUE(venc->WaitForEos()) << SAMPLE_ID;
    EXPECT_EQ(venc->Release(), AV_ERR_OK) << SAMPLE_ID;
}

/**
 * @tc.name: VideoEncoder_Multithread_AVBuffer_With_Queue_003
 * @tc.desc: 1. push/free buffer in queue;
 *           2. operate in output callback;
 */
AVCODEC_MTEST_P(VideoEncStableTest, VideoEncoder_Multithread_AVBuffer_With_Queue_003, TestSize.Level1,
                VideoEncSample::threadNum_)
{
    auto venc = make_shared<VideoEncSample>();
    auto signal = make_shared<VCodecSignal>(venc);
    venc->operation_ = VideoEncStableTest::GetParam();
    venc->frameCount_ = 60; // 60: input frame num
    venc->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    venc->inPath_ = "1280_720_nv.yuv";
    venc->outPath_ = GetTestName();
    EXPECT_EQ(venc->Create(), true);
    struct OH_AVCodecCallback cb;
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputBuffer = InBufferQueue;
    cb.onNewOutputBuffer = OutBufferOperate;
    EXPECT_EQ(venc->RegisterCallback(cb, signal), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(venc->Configure(), AV_ERR_OK) << SAMPLE_ID;

    signal->isRunning_ = true;
    venc->inputLoop_ = make_unique<thread>([&signal]() { InputBufferLoop(signal); });
    EXPECT_EQ(venc->Start(), AV_ERR_OK) << SAMPLE_ID;

    EXPECT_TRUE(venc->WaitForEos()) << SAMPLE_ID;
    EXPECT_EQ(venc->Release(), AV_ERR_OK) << SAMPLE_ID;
}

/**
 * @tc.name: VideoEncoder_Multithread_AVBuffer_With_Queue_004
 * @tc.desc: 1. push/free buffer in queue;
 *           2. operate not in callback;
 *           3. set surface.
 */
AVCODEC_MTEST_P(VideoEncStableTest, VideoEncoder_Multithread_AVBuffer_With_Queue_004, TestSize.Level1,
                VideoEncSample::threadNum_)
{
    auto venc = make_shared<VideoEncSample>();
    auto signal = make_shared<VCodecSignal>(venc);
    venc->operation_ = VideoEncStableTest::GetParam();
    venc->frameCount_ = 60; // 60: input frame num
    venc->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    venc->inPath_ = "1280_720_nv.yuv";
    venc->outPath_ = GetTestName();
    EXPECT_EQ(venc->Create(), true);
    struct OH_AVCodecCallback cb;
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputBuffer = nullptr;
    cb.onNewOutputBuffer = OutBufferQueue;
    signal->isRunning_ = true;
    EXPECT_EQ(venc->RegisterCallback(cb, signal), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(venc->Configure(), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(venc->GetInputSurface(), AV_ERR_OK) << SAMPLE_ID;

    venc->outputLoop_ = make_unique<thread>([&signal]() { OutputBufferLoop(signal); });
    EXPECT_EQ(venc->Start(), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(venc->Operate(), AV_ERR_OK) << SAMPLE_ID;

    EXPECT_TRUE(venc->WaitForEos()) << SAMPLE_ID;
    EXPECT_EQ(venc->Release(), AV_ERR_OK) << SAMPLE_ID;
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
            VideoEncSample::needDump_ = true;
            DecArgv(i, argc, argv);
        } else if (timeout > 0) {
            VideoEncSample::sampleTimout_ = timeout;
            DecArgv(i, argc, argv);
        } else if (threadNum > 0) {
            VideoEncSample::threadNum_ = threadNum;
            DecArgv(i, argc, argv);
        }
    }
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}