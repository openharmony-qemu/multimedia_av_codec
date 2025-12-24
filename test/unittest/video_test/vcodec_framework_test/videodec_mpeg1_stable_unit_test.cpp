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
#include <gtest/gtest.h>
#include <gtest/hwext/gtest-multithread.h>
#include "heap_memory_thread.h"
#include "native_avcapability.h"
#include "native_avmagic.h"
#include "unittest_utils.h"
#include "vdec_sample.h"

#define PRINT_HILOG
#define TEST_ID vdec->sampleId_
#define SAMPLE_ID "[SAMPLE_ID]:" << TEST_ID
#include "unittest_log.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace testing::ext;
using namespace testing::mt;

namespace {
class VideoDecMpeg1DecTest : public testing::TestWithParam<std::string> {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);

private:
    shared_ptr<HeapMemoryThread> heapThread_ = nullptr;
};

void VideoDecMpeg1DecTest::SetUpTestCase(void) {}

void VideoDecMpeg1DecTest::TearDownTestCase(void) {}

void VideoDecMpeg1DecTest::SetUp(void)
{
    heapThread_ = make_shared<HeapMemoryThread>();
}

void VideoDecMpeg1DecTest::TearDown(void)
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
 * @tc.name: VideoDecoder_mpeg1decoder_Release_001
 * @tc.desc: 1. push/free buffer in callback;
 *           2. release not in callback;
 */
HWMTEST_F(VideoDecMpeg1DecTest, VideoDecoder_mpeg1decoder_Release_001, TestSize.Level1, VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->frameCount_ = 30; // 30: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_MPEG1;
    vdec->inPath_ = "test.mpeg1";
    vdec->sampleWidth_ = 720;
    vdec->sampleHeight_ = 480;
    vdec->releaseOtherBuffer_ = true;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = "mpeg1decoder.dump";
    vdec->dumpValue_ = "11";
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
 * @tc.name: VideoDecoder_mpeg1decoder_Create_AVBuffer_001
 * @tc.desc: 1. push/free buffer in callback;
 *           2. release not in callback;
 */
HWMTEST_F(VideoDecMpeg1DecTest, VideoDecoder_mpeg1decoder_Create_AVBuffer_001, TestSize.Level1,
          VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->frameCount_ = 30; // 30: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_MPEG1;
    vdec->inPath_ = "test.mpeg1";
    vdec->sampleWidth_ = 720;
    vdec->sampleHeight_ = 480;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = "mpeg1decoder.dump";
    vdec->dumpValue_ = "01";
    vdec->samplePixel_ = AV_PIXEL_FORMAT_RGBA;
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

/**
 * @tc.name: VideoDecoder_mpeg1decoder_Create_AVBuffer_002
 * @tc.desc: 1. push/free buffer in callback;
 *           2. release not in callback;
 */
HWMTEST_F(VideoDecMpeg1DecTest, VideoDecoder_mpeg1decoder_Create_AVBuffer_002, TestSize.Level1,
          VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->frameCount_ = 30; // 30: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_MPEG1;
    vdec->inPath_ = "test.mpeg1";
    vdec->sampleWidth_ = 720;
    vdec->sampleHeight_ = 480;
    vdec->setSurfaceParam_ = true;
    vdec->releaseOtherBuffer_ = true;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = "mpeg1decoder.dump";
    vdec->dumpValue_ = "11";
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
    EXPECT_EQ(vdec->SetParameter(), AV_ERR_OK) << SAMPLE_ID;

    EXPECT_TRUE(vdec->WaitForEos()) << SAMPLE_ID;
    EXPECT_EQ(vdec->Release(), AV_ERR_OK) << SAMPLE_ID;
}

/**
 * @tc.name: VideoDecoder_mpeg1decoder_Create_AVBuffer_003
 * @tc.desc: 1. push/free buffer in callback;
 *           2. release not in callback;
 */
HWMTEST_F(VideoDecMpeg1DecTest, VideoDecoder_mpeg1decoder_Create_AVBuffer_003, TestSize.Level1,
          VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->frameCount_ = 30; // 30: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_MPEG1;
    vdec->inPath_ = "test.mpeg1";
    vdec->sampleWidth_ = 720;
    vdec->sampleHeight_ = 480;
    vdec->outPath_ = GetTestName();
    vdec->samplePixel_ = AV_PIXEL_FORMAT_RGBA;
    vdec->dumpKey_ = "mpeg1decoder.dump";
    vdec->dumpValue_ = "11";
    vdec->setSurfaceParam_ = true;
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
    EXPECT_EQ(vdec->SetParameter(), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(vdec->SetOutputSurface(true), AV_ERR_OK) << SAMPLE_ID;

    EXPECT_TRUE(vdec->WaitForEos()) << SAMPLE_ID;
    EXPECT_EQ(vdec->Release(), AV_ERR_OK) << SAMPLE_ID;
}

INSTANTIATE_TEST_SUITE_P(, VideoDecMpeg1DecTest, testing::Values("Flush", "Stop", "Reset", "SetOutputSurface"));

/**
 * @tc.name: VideoDecoder_mpeg1decoder_001
 * @tc.desc: 1. push/free buffer in callback;
 *           2. operate not in callback;
 */
AVCODEC_MTEST_P(VideoDecMpeg1DecTest, VideoDecoder_mpeg1decoder_001, TestSize.Level1, VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecMpeg1DecTest::GetParam();
    vdec->frameCount_ = 30; // 30: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_MPEG1;
    vdec->inPath_ = "test.mpeg1";
    vdec->sampleWidth_ = 720;
    vdec->sampleHeight_ = 480;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = "mpeg1decoder.dump";
    vdec->dumpValue_ = "0";
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
 * @tc.name: VideoDecoder_mpeg1decoder_002
 * @tc.desc: 1. push/free buffer in callback;
 *           2. operate in input callback;
 */
AVCODEC_MTEST_P(VideoDecMpeg1DecTest, VideoDecoder_mpeg1decoder_002, TestSize.Level1, VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecMpeg1DecTest::GetParam();
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_MPEG1;
    vdec->inPath_ = "test.mpeg1";
    vdec->sampleWidth_ = 720;
    vdec->sampleHeight_ = 480;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = "mpeg1decoder.dump";
    vdec->dumpValue_ = "0";
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
 * @tc.name: VideoDecoder_mpeg1decoder_003
 * @tc.desc: 1. push/free buffer in callback;
 *           2. operate in output callback;
 */
AVCODEC_MTEST_P(VideoDecMpeg1DecTest, VideoDecoder_mpeg1decoder_003, TestSize.Level1, VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecMpeg1DecTest::GetParam();
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_MPEG1;
    vdec->inPath_ = "test.mpeg1";
    vdec->sampleWidth_ = 720;
    vdec->sampleHeight_ = 480;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = "mpeg1decoder.dump";
    vdec->dumpValue_ = "0";
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
 * @tc.name: VideoDecoder_mpeg1decoder_004
 * @tc.desc: 1. push/free buffer in callback;
 *           2. operate not in callback;
 *           3. set surface;
 */
AVCODEC_MTEST_P(VideoDecMpeg1DecTest, VideoDecoder_mpeg1decoder_004, TestSize.Level1, VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecMpeg1DecTest::GetParam();
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_MPEG1;
    vdec->inPath_ = "test.mpeg1";
    vdec->sampleWidth_ = 720;
    vdec->sampleHeight_ = 480;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = "mpeg1decoder.dump";
    vdec->dumpValue_ = "0";
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
 * @tc.name: VideoDecoder_mpeg1decoder_005
 * @tc.desc: 1. push/free buffer in callback;
 *           2. operate in input callback;
 *           3. set surface;
 */
AVCODEC_MTEST_P(VideoDecMpeg1DecTest, VideoDecoder_mpeg1decoder_005, TestSize.Level1, VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecMpeg1DecTest::GetParam();
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_MPEG1;
    vdec->inPath_ = "test.mpeg1";
    vdec->sampleWidth_ = 720;
    vdec->sampleHeight_ = 480;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = "mpeg1decoder.dump";
    vdec->dumpValue_ = "0";
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
 * @tc.name: VideoDecoder_mpeg1decoder_With_Queue_001
 * @tc.desc: 1. push/free buffer in queue;
 *           2. operate not in callback;
 */
AVCODEC_MTEST_P(VideoDecMpeg1DecTest, VideoDecoder_mpeg1decoder_With_Queue_001, TestSize.Level1,
                VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecMpeg1DecTest::GetParam();
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_MPEG1;
    vdec->inPath_ = "test.mpeg1";
    vdec->sampleWidth_ = 720;
    vdec->sampleHeight_ = 480;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = "mpeg1decoder.dump";
    vdec->dumpValue_ = "0";
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
 * @tc.name: VideoDecoder_mpeg1decoder_With_Queue_002
 * @tc.desc: 1. push/free buffer in queue;
 *           2. operate in input callback;
 */
AVCODEC_MTEST_P(VideoDecMpeg1DecTest, VideoDecoder_mpeg1decoder_With_Queue_002, TestSize.Level1,
                VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecMpeg1DecTest::GetParam();
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_MPEG1;
    vdec->inPath_ = "test.mpeg1";
    vdec->sampleWidth_ = 720;
    vdec->sampleHeight_ = 480;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = "mpeg1decoder.dump";
    vdec->dumpValue_ = "0";
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
 * @tc.name: VideoDecoder_mpeg1decoder_With_Queue_003
 * @tc.desc: 1. push buffer in callback;
 *           2. operate in output callback;
 *           3. free buffer in queue;
 */
AVCODEC_MTEST_P(VideoDecMpeg1DecTest, VideoDecoder_mpeg1decoder_With_Queue_003, TestSize.Level1,
                VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecMpeg1DecTest::GetParam();
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_MPEG1;
    vdec->inPath_ = "test.mpeg1";
    vdec->sampleWidth_ = 720;
    vdec->sampleHeight_ = 480;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = "mpeg1decoder.dump";
    vdec->dumpValue_ = "0";
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
 * @tc.name: VideoDecoder_mpeg1decoder_With_Queue_004
 * @tc.desc: 1. push buffer in callback;
 *           2. operate not in callback;
 *           3. render buffer in queue;
 *           4. set surface;
 */
AVCODEC_MTEST_P(VideoDecMpeg1DecTest, VideoDecoder_mpeg1decoder_With_Queue_004, TestSize.Level1,
                VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecMpeg1DecTest::GetParam();
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_MPEG1;
    vdec->inPath_ = "test.mpeg1";
    vdec->sampleWidth_ = 720;
    vdec->sampleHeight_ = 480;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = "mpeg1decoder.dump";
    vdec->dumpValue_ = "0";
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
 * @tc.name: VideoDecoder_mpeg1decoder_With_Queue_005
 * @tc.desc: 1. push buffer in callback;
 *           2. operate in input callback;
 *           3. render buffer in queue;
 *           4. set surface;
 */
AVCODEC_MTEST_P(VideoDecMpeg1DecTest, VideoDecoder_mpeg1decoder_With_Queue_005, TestSize.Level1,
                VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecMpeg1DecTest::GetParam();
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_MPEG1;
    vdec->inPath_ = "test.mpeg1";
    vdec->sampleWidth_ = 720;
    vdec->sampleHeight_ = 480;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = "mpeg1decoder.dump";
    vdec->dumpValue_ = "0";
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
 * @tc.name: VideoDecoder_mpeg1decoder_AVBuffer_001
 * @tc.desc: 1. push/free buffer in callback;
 *           2. operate not in callback;
 */
AVCODEC_MTEST_P(VideoDecMpeg1DecTest, VideoDecoder_mpeg1decoder_AVBuffer_001, TestSize.Level1,
                VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecMpeg1DecTest::GetParam();
    vdec->frameCount_ = 30; // 30: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_MPEG1;
    vdec->inPath_ = "test.mpeg1";
    vdec->sampleWidth_ = 720;
    vdec->sampleHeight_ = 480;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = "mpeg1decoder.dump";
    vdec->dumpValue_ = "0";
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
 * @tc.name: VideoDecoder_mpeg1decoder_AVBuffer_002
 * @tc.desc: 1. push/free buffer in callback;
 *           2. operate in input callback;
 */
AVCODEC_MTEST_P(VideoDecMpeg1DecTest, VideoDecoder_mpeg1decoder_AVBuffer_002, TestSize.Level1,
                VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecMpeg1DecTest::GetParam();
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_MPEG1;
    vdec->inPath_ = "test.mpeg1";
    vdec->sampleWidth_ = 720;
    vdec->sampleHeight_ = 480;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = "mpeg1decoder.dump";
    vdec->dumpValue_ = "0";
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
 * @tc.name: VideoDecoder_mpeg1decoder_AVBuffer_003
 * @tc.desc: 1. push/free buffer in callback;
 *           2. operate in output callback;
 */
AVCODEC_MTEST_P(VideoDecMpeg1DecTest, VideoDecoder_mpeg1decoder_AVBuffer_003, TestSize.Level1,
                VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecMpeg1DecTest::GetParam();
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_MPEG1;
    vdec->inPath_ = "test.mpeg1";
    vdec->sampleWidth_ = 720;
    vdec->sampleHeight_ = 480;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = "mpeg1decoder.dump";
    vdec->dumpValue_ = "0";
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
 * @tc.name: VideoDecoder_mpeg1decoder_AVBuffer_004
 * @tc.desc: 1. push/free buffer in callback;
 *           2. operate not in callback;
 *           3. set surface;
 */
AVCODEC_MTEST_P(VideoDecMpeg1DecTest, VideoDecoder_mpeg1decoder_AVBuffer_004, TestSize.Level1,
                VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecMpeg1DecTest::GetParam();
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_MPEG1;
    vdec->inPath_ = "test.mpeg1";
    vdec->sampleWidth_ = 720;
    vdec->sampleHeight_ = 480;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = "mpeg1decoder.dump";
    vdec->dumpValue_ = "0";
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
 * @tc.name: VideoDecoder_mpeg1decoder_AVBuffer_005
 * @tc.desc: 1. push/free buffer in callback;
 *           2. operate in input callback;
 *           3. set surface;
 */
AVCODEC_MTEST_P(VideoDecMpeg1DecTest, VideoDecoder_mpeg1decoder_AVBuffer_005, TestSize.Level1,
                VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecMpeg1DecTest::GetParam();
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_MPEG1;
    vdec->inPath_ = "test.mpeg1";
    vdec->sampleWidth_ = 720;
    vdec->sampleHeight_ = 480;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = "mpeg1decoder.dump";
    vdec->dumpValue_ = "0";
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
 * @tc.name: VideoDecoder_mpeg1decoder_AVBuffer_With_Queue_001
 * @tc.desc: 1. push/free buffer in queue;
 *           2. operate not in callback;
 */
AVCODEC_MTEST_P(VideoDecMpeg1DecTest, VideoDecoder_mpeg1decoder_AVBuffer_With_Queue_001, TestSize.Level1,
                VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecMpeg1DecTest::GetParam();
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_MPEG1;
    vdec->inPath_ = "test.mpeg1";
    vdec->sampleWidth_ = 720;
    vdec->sampleHeight_ = 480;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = "mpeg1decoder.dump";
    vdec->dumpValue_ = "0";
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
 * @tc.name: VideoDecoder_mpeg1decoder_AVBuffer_With_Queue_002
 * @tc.desc: 1. push/free buffer in queue;
 *           2. operate in input callback;
 */
AVCODEC_MTEST_P(VideoDecMpeg1DecTest, VideoDecoder_mpeg1decoder_AVBuffer_With_Queue_002, TestSize.Level1,
                VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecMpeg1DecTest::GetParam();
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_MPEG1;
    vdec->inPath_ = "test.mpeg1";
    vdec->sampleWidth_ = 720;
    vdec->sampleHeight_ = 480;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = "mpeg1decoder.dump";
    vdec->dumpValue_ = "0";
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
 * @tc.name: VideoDecoder_mpeg1decoder_AVBuffer_With_Queue_003
 * @tc.desc: 1. push buffer in callback;
 *           2. operate in output callback;
 *           3. free buffer in queue;
 */
AVCODEC_MTEST_P(VideoDecMpeg1DecTest, VideoDecoder_mpeg1decoder_AVBuffer_With_Queue_003, TestSize.Level1,
                VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecMpeg1DecTest::GetParam();
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_MPEG1;
    vdec->inPath_ = "test.mpeg1";
    vdec->sampleWidth_ = 720;
    vdec->sampleHeight_ = 480;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = "mpeg1decoder.dump";
    vdec->dumpValue_ = "0";
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
 * @tc.name: VideoDecoder_mpeg1decoder_AVBuffer_With_Queue_004
 * @tc.desc: 1. push buffer in callback;
 *           2. operate not in callback;
 *           3. render buffer in queue;
 *           4. set surface;
 */
AVCODEC_MTEST_P(VideoDecMpeg1DecTest, VideoDecoder_mpeg1decoder_AVBuffer_With_Queue_004, TestSize.Level1,
                VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecMpeg1DecTest::GetParam();
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_MPEG1;
    vdec->inPath_ = "test.mpeg1";
    vdec->sampleWidth_ = 720;
    vdec->sampleHeight_ = 480;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = "mpeg1decoder.dump";
    vdec->dumpValue_ = "0";
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
 * @tc.name: VideoDecoder_mpeg1decoder_AVBuffer_With_Queue_005
 * @tc.desc: 1. push buffer in callback;
 *           2. operate in input callback;
 *           3. render buffer in queue;
 *           4. set surface;
 */
AVCODEC_MTEST_P(VideoDecMpeg1DecTest, VideoDecoder_mpeg1decoder_AVBuffer_With_Queue_005, TestSize.Level1,
                VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecMpeg1DecTest::GetParam();
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_MPEG1;
    vdec->inPath_ = "test.mpeg1";
    vdec->sampleWidth_ = 720;
    vdec->sampleHeight_ = 480;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = "mpeg1decoder.dump";
    vdec->dumpValue_ = "0";
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

bool CheckCapabilitySupport()
{
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_MPEG1, false,
                                                                     SOFTWARE);
    return capability != nullptr;
}
} // namespace

int main(int argc, char **argv)
{
    uint64_t threadNum = 0;
    uint64_t timeout = 0;
    VideoDecSample::isHardware_ = false;
    testing::GTEST_FLAG(output) = "xml:./";
    for (int i = 0; i < argc; ++i) {
        cout << argv[i] << endl;
        threadNum = GetNum(argv[i], "--thread_num");
        timeout = GetNum(argv[i], "--timeout");
        if (strcmp(argv[i], "--need_dump") == 0) {
            VideoDecSample::needDump_ = true;
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
    if (!CheckCapabilitySupport()) {
        testing::GTEST_FLAG(filter) = "-*";
    }
    return RUN_ALL_TESTS();
}