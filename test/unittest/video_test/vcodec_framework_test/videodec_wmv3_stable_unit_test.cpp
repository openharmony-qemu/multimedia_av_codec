/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
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
constexpr uint32_t WMV3_SAPMLE_WIDTH = 352;
constexpr uint32_t WMV3_SAMPLE_HEIGHT = 288;
std::string WMV3_STREAM_FILE_NAME = "352_288_10.wmv3";

constexpr uint32_t WMV3_MAIN_WIDTH = 1280;
constexpr uint32_t WMV3_MAIN_HEIGHT = 720;
std::string WMV3_MAIN_STREAM_FILE_NAME = "profile1_1280_720_24.wmv3";
std::string WMV3_DECODER_DUMP_FILE_NAME = "fcodec.dump";

class VideoDecStableTestWmv3 : public testing::TestWithParam<std::string> {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);

private:
    shared_ptr<HeapMemoryThread> heapThread_ = nullptr;
};

void VideoDecStableTestWmv3::SetUpTestCase(void) {}

void VideoDecStableTestWmv3::TearDownTestCase(void) {}

void VideoDecStableTestWmv3::SetUp(void)
{
    heapThread_ = make_shared<HeapMemoryThread>();
}

void VideoDecStableTestWmv3::TearDown(void)
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
 * @tc.name: VideoDecoder_wmv3decoder_Create_001
 * @tc.desc: 1. create 64 wmv3decoder instances success;
 *           2. create more instances fail;
 */
HWMTEST_F(VideoDecStableTestWmv3, VideoDecoder_wmv3decoder_Create_001, TestSize.Level1, VideoDecSample::threadNum_)
{
    std::vector<std::shared_ptr<VideoDecSample>> decoderList;
    int32_t instanceNum = 64; // 64: max instance num
    for (int i = 0; i < instanceNum; i++) {
        auto vdec = make_shared<VideoDecSample>();
        auto signal = make_shared<VCodecSignal>(vdec);
        vdec->frameCount_ = 30; // 30: input frame num
        vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_WMV3;
        vdec->inPath_ = WMV3_STREAM_FILE_NAME;
        vdec->sampleWidth_ = WMV3_SAPMLE_WIDTH;
        vdec->sampleHeight_ = WMV3_SAMPLE_HEIGHT;
        vdec->dumpKey_ = WMV3_DECODER_DUMP_FILE_NAME;
        vdec->dumpValue_ = "0";
        EXPECT_EQ(vdec->Create(), true);
        decoderList.push_back(vdec);
        struct OH_AVCodecCallback cb;
        cb.onError = OnErrorVoid;
        cb.onStreamChanged = OnStreamChangedVoid;
        cb.onNeedInputBuffer = InBufferHandle;
        cb.onNewOutputBuffer = OutBufferHandle;
        EXPECT_EQ(vdec->RegisterCallback(cb, signal), AV_ERR_OK) << SAMPLE_ID;
    }
    for (int i = 0; i < instanceNum; i++) {
        auto vdec = decoderList[i];
        EXPECT_EQ(vdec->Release(), AV_ERR_OK) << SAMPLE_ID;
    }
}

/**.
 * @tc.name: VideoDecoder_wmv3decoder_Release_001
 * @tc.desc: 1. push/free buffer in callback;
 *           2. release not in callback;
 */
HWMTEST_F(VideoDecStableTestWmv3, VideoDecoder_wmv3decoder_Release_001, TestSize.Level1, VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->frameCount_ = 30; // 30: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_WMV3;
    vdec->inPath_ = WMV3_STREAM_FILE_NAME;
    vdec->sampleWidth_ = WMV3_SAPMLE_WIDTH;
    vdec->sampleHeight_ = WMV3_SAMPLE_HEIGHT;
    vdec->releaseOtherBuffer_ = true;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = WMV3_DECODER_DUMP_FILE_NAME;
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
 * @tc.name: VideoDecoder_wmv3decoder_Release_AVBuffer_001
 * @tc.desc: 1. push/free buffer in callback;
 *           2. release not in callback;
 */
HWMTEST_F(VideoDecStableTestWmv3, VideoDecoder_wmv3decoder_Release_AVBuffer_001, TestSize.Level1,
          VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->frameCount_ = 30; // 30: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_WMV3;
    vdec->inPath_ = WMV3_STREAM_FILE_NAME;
    vdec->sampleWidth_ = WMV3_SAPMLE_WIDTH;
    vdec->sampleHeight_ = WMV3_SAMPLE_HEIGHT;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = WMV3_DECODER_DUMP_FILE_NAME;
    vdec->dumpValue_ = "10";
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
 * @tc.name: VideoDecoder_wmv3decoder_Create_AVBuffer_001
 * @tc.desc: 1. push/free buffer in callback;
 *           2. release not in callback;
 */
HWMTEST_F(VideoDecStableTestWmv3, VideoDecoder_wmv3decoder_Create_AVBuffer_001, TestSize.Level1,
          VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->frameCount_ = 30; // 30: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_WMV3;
    vdec->inPath_ = WMV3_STREAM_FILE_NAME;
    vdec->sampleWidth_ = WMV3_SAPMLE_WIDTH;
    vdec->sampleHeight_ = WMV3_SAMPLE_HEIGHT;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = WMV3_DECODER_DUMP_FILE_NAME;
    vdec->dumpValue_ = "1";
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
 * @tc.name: VideoDecoder_wmv3decoder_Create_AVBuffer_002
 * @tc.desc: 1. push/free buffer in callback;
 *           2. release not in callback;
 */
HWMTEST_F(VideoDecStableTestWmv3, VideoDecoder_wmv3decoder_Create_AVBuffer_002, TestSize.Level1,
          VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->frameCount_ = 30; // 30: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_WMV3;
    vdec->inPath_ = WMV3_STREAM_FILE_NAME;
    vdec->sampleWidth_ = WMV3_SAPMLE_WIDTH;
    vdec->sampleHeight_ = WMV3_SAMPLE_HEIGHT;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = WMV3_DECODER_DUMP_FILE_NAME;
    vdec->dumpValue_ = "01";
    vdec->samplePixel_ = AV_PIXEL_FORMAT_NV21;
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
 * @tc.name: VideoDecoder_wmv3decoder_Create_AVBuffer_003
 * @tc.desc: 1. push/free buffer in callback;
 *           2. release not in callback;
 */
HWMTEST_F(VideoDecStableTestWmv3, VideoDecoder_wmv3decoder_Create_AVBuffer_003, TestSize.Level1,
          VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->frameCount_ = 30; // 30: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_WMV3;
    vdec->inPath_ = WMV3_STREAM_FILE_NAME;
    vdec->sampleWidth_ = WMV3_SAPMLE_WIDTH;
    vdec->sampleHeight_ = WMV3_SAMPLE_HEIGHT;
    vdec->setSurfaceParam_ = true;
    vdec->releaseOtherBuffer_ = true;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = WMV3_DECODER_DUMP_FILE_NAME;
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
 * @tc.name: VideoDecoder_wmv3decoder_Create_AVBuffer_004
 * @tc.desc: 1. push/free buffer in callback;
 *           2. release not in callback;
 */
HWMTEST_F(VideoDecStableTestWmv3, VideoDecoder_wmv3decoder_Create_AVBuffer_004, TestSize.Level1,
          VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->frameCount_ = 30; // 30: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_WMV3;
    vdec->inPath_ = WMV3_STREAM_FILE_NAME;
    vdec->sampleWidth_ = WMV3_SAPMLE_WIDTH;
    vdec->sampleHeight_ = WMV3_SAMPLE_HEIGHT;
    vdec->outPath_ = GetTestName();
    vdec->samplePixel_ = AV_PIXEL_FORMAT_NV21;
    vdec->dumpKey_ = WMV3_DECODER_DUMP_FILE_NAME;
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

/**
 * @tc.name: VideoDecoder_wmv3decoder_Create_AVBuffer_Main_001
 * @tc.desc: 1. push/free buffer in callback;
 *           2. release not in callback;
 */
HWMTEST_F(VideoDecStableTestWmv3, VideoDecoder_wmv3decoder_Create_AVBuffer_Main_001, TestSize.Level1,
          VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->frameCount_ = 21; // 21: input frame num
    vdec->skipOutFrameHalfCheck_ = true;
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_WMV3;
    vdec->inPath_ = WMV3_MAIN_STREAM_FILE_NAME;
    vdec->sampleWidth_ = WMV3_MAIN_WIDTH;
    vdec->sampleHeight_ = WMV3_MAIN_HEIGHT;
    vdec->releaseOtherBuffer_ = true;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = WMV3_DECODER_DUMP_FILE_NAME;
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
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << SAMPLE_ID;

    EXPECT_TRUE(vdec->WaitForEos()) << SAMPLE_ID;
    EXPECT_EQ(vdec->Release(), AV_ERR_OK) << SAMPLE_ID;
}

/**
 * @tc.name: VideoDecoder_wmv3decoder_Create_AVBuffer_Main_002
 * @tc.desc: 1. push/free buffer in callback;
 *           2. release not in callback;
 */
HWMTEST_F(VideoDecStableTestWmv3, VideoDecoder_wmv3decoder_Create_AVBuffer_Main_002, TestSize.Level1,
          VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->frameCount_ = 21; // 21: input frame num
    vdec->skipOutFrameHalfCheck_ = true;
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_WMV3;
    vdec->inPath_ = WMV3_MAIN_STREAM_FILE_NAME;
    vdec->sampleWidth_ = WMV3_MAIN_WIDTH;
    vdec->sampleHeight_ = WMV3_MAIN_HEIGHT;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = WMV3_DECODER_DUMP_FILE_NAME;
    vdec->dumpValue_ = "11";
    vdec->samplePixel_ = AV_PIXEL_FORMAT_NV21;
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
    EXPECT_EQ(vdec->Start(), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(vdec->SetParameter(), AV_ERR_OK) << SAMPLE_ID;

    EXPECT_TRUE(vdec->WaitForEos()) << SAMPLE_ID;
    EXPECT_EQ(vdec->Release(), AV_ERR_OK) << SAMPLE_ID;
}

/**
 * @tc.name: VideoDecoder_wmv3decoder_Create_AVBuffer_Main_003
 * @tc.desc: 1. push/free buffer in callback;
 *           2. release not in callback;
 */
HWMTEST_F(VideoDecStableTestWmv3, VideoDecoder_wmv3decoder_Create_AVBuffer_Main_003, TestSize.Level1,
          VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->frameCount_ = 21; // 21: input frame num
    vdec->skipOutFrameHalfCheck_ = true;
    vdec->maxOutputBufferCount_ = true;
    vdec->maxInputBufferCount_ = true;
    vdec->defaultBufferCount_ = 20;
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_WMV3;
    vdec->inPath_ = WMV3_MAIN_STREAM_FILE_NAME;
    vdec->sampleWidth_ = WMV3_MAIN_WIDTH;
    vdec->sampleHeight_ = WMV3_MAIN_HEIGHT;
    vdec->setSurfaceParam_ = true;
    vdec->releaseOtherBuffer_ = true;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = WMV3_DECODER_DUMP_FILE_NAME;
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
    EXPECT_EQ(vdec->SetOutputSurface(true), AV_ERR_OK) << SAMPLE_ID;

    EXPECT_TRUE(vdec->WaitForEos()) << SAMPLE_ID;
    EXPECT_EQ(vdec->SetParameter(), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(vdec->Release(), AV_ERR_OK) << SAMPLE_ID;
}

/**
 * @tc.name: VideoDecoder_wmv3decoder_Create_AVBuffer_Main_004
 * @tc.desc: 1. push/free buffer in callback;
 *           2. release not in callback;
 */
HWMTEST_F(VideoDecStableTestWmv3, VideoDecoder_wmv3decoder_Create_AVBuffer_Main_004, TestSize.Level1,
          VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->frameCount_ = 21; // 21: input frame num
    vdec->skipOutFrameHalfCheck_ = true;
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_WMV3;
    vdec->inPath_ = WMV3_MAIN_STREAM_FILE_NAME;
    vdec->sampleWidth_ = WMV3_MAIN_WIDTH;
    vdec->sampleHeight_ = WMV3_MAIN_HEIGHT;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = WMV3_DECODER_DUMP_FILE_NAME;
    vdec->dumpValue_ = "11";
    vdec->samplePixel_ = AV_PIXEL_FORMAT_NV21;
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
    EXPECT_EQ(vdec->SetParameter(), AV_ERR_OK) << SAMPLE_ID;
    EXPECT_EQ(vdec->Release(), AV_ERR_OK) << SAMPLE_ID;
}

/**
 * @tc.name: VideoDecoder_wmv3decoder_Create_AVBuffer_Main_005
 * @tc.desc: 1. push buffer in callback;
 *           2. operate in input callback;
 *           3. render buffer in queue;
 *           4. set rotation maxOut/InputBufferCount defaultmaxBufferCount scaleMode;
 *           5. set surface;
 */
HWMTEST_F(VideoDecStableTestWmv3, VideoDecoder_wmv3decoder_Create_AVBuffer_Main_005, TestSize.Level1,
          VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->frameCount_ = 21; // 21: input frame num
    vdec->skipOutFrameHalfCheck_ = true;
    vdec->ohRotation_ = true;
    vdec->maxOutputBufferCount_ = true;
    vdec->maxInputBufferCount_ = true;
    vdec->defaultBufferCount_ = 0;
    vdec->scaleMode_ = true;
    vdec->lowLatency_ = true;
    vdec->setPixelFormat_ = false;
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_WMV3;
    vdec->inPath_ = WMV3_MAIN_STREAM_FILE_NAME;
    vdec->sampleWidth_ = WMV3_MAIN_WIDTH;
    vdec->sampleHeight_ = WMV3_MAIN_HEIGHT;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = WMV3_DECODER_DUMP_FILE_NAME;
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
    EXPECT_EQ(vdec->SetOutputSurface(true), AV_ERR_OK) << SAMPLE_ID;

    EXPECT_TRUE(vdec->WaitForEos()) << SAMPLE_ID;
    EXPECT_EQ(vdec->Release(), AV_ERR_OK) << SAMPLE_ID;
}

void InitAVCodecCallback(OH_AVCodecCallback &cb)
{
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputBuffer = InBufferHandle;
    cb.onNewOutputBuffer = OutBufferHandle;
}
/**
 * @tc.name: VideoDecoder_wmv3decoder_Create_AVBuffer_Main_006
 * @tc.desc: 1. 2 codec use same surface;
 */
HWMTEST_F(VideoDecStableTestWmv3, VideoDecoder_wmv3decoder_Create_AVBuffer_Main_006, TestSize.Level1,
          VideoDecSample::threadNum_)
{
    auto vdec1 = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec1);
    auto vdec2 = make_shared<VideoDecSample>();
    vdec1->frameCount_ = 21; // 21: input frame num
    vdec1->skipOutFrameHalfCheck_ = true;
    vdec1->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_WMV3;
    vdec1->inPath_ = WMV3_STREAM_FILE_NAME;
    vdec1->sampleWidth_ = WMV3_SAPMLE_WIDTH;
    vdec1->sampleHeight_ = WMV3_SAMPLE_HEIGHT;
    vdec1->outPath_ = GetTestName();
    vdec1->dumpKey_ = WMV3_DECODER_DUMP_FILE_NAME;
    vdec1->dumpValue_ = "0";

    vdec2->frameCount_ = 21; // 21: input frame num
    vdec2->skipOutFrameHalfCheck_ = true;
    vdec2->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_WMV3;
    vdec2->inPath_ = WMV3_STREAM_FILE_NAME;
    vdec2->sampleWidth_ = WMV3_SAPMLE_WIDTH;
    vdec2->sampleHeight_ = WMV3_SAMPLE_HEIGHT;
    vdec2->outPath_ = GetTestName();
    vdec2->dumpKey_ = WMV3_DECODER_DUMP_FILE_NAME;
    vdec2->dumpValue_ = "0";
    EXPECT_EQ(vdec1->Create(), true);
    EXPECT_EQ(vdec2->Create(), true);
    struct OH_AVCodecCallback cb1;
    struct OH_AVCodecCallback cb2;
    InitAVCodecCallback(cb1);
    InitAVCodecCallback(cb2);
    signal->isRunning_ = true;
    EXPECT_EQ(vdec1->RegisterCallback(cb1, signal), AV_ERR_OK) << "[SAMPLE_ID]:" << vdec1->sampleId_;
    EXPECT_EQ(vdec1->Configure(), AV_ERR_OK) << "[SAMPLE_ID]:" << vdec1->sampleId_;
    EXPECT_EQ(vdec1->SetOutputSurface(), AV_ERR_OK) << "[SAMPLE_ID]:" << vdec1->sampleId_;
    EXPECT_EQ(vdec2->RegisterCallback(cb2, signal), AV_ERR_OK) << "[SAMPLE_ID]:" << vdec2->sampleId_;
    EXPECT_EQ(vdec2->Configure(), AV_ERR_OK) << "[SAMPLE_ID]:" << vdec2->sampleId_;
    EXPECT_EQ(vdec2->SetOutputSurface(), AV_ERR_OK) << "[SAMPLE_ID]:" << vdec2->sampleId_;
    EXPECT_EQ(vdec1->Start(), AV_ERR_OK) << "[SAMPLE_ID]:" << vdec1->sampleId_;
    EXPECT_TRUE(vdec1->WaitForEos()) << "[SAMPLE_ID]:" << vdec1->sampleId_;

    EXPECT_EQ(vdec2->SetOutputSurface(vdec1->GetSurfaceWindow(false)), AV_ERR_OK) << "[SAMPLE_ID]:" << vdec2->sampleId_;
    EXPECT_EQ(vdec1->Release(), AV_ERR_OK) << "[SAMPLE_ID]:" << vdec1->sampleId_;
    EXPECT_EQ(vdec2->Start(), AV_ERR_OK) << "[SAMPLE_ID]:" << vdec2->sampleId_;
    EXPECT_EQ(vdec2->SetOutputSurface(true), AV_ERR_OK) << "[SAMPLE_ID]:" << vdec2->sampleId_;
    EXPECT_TRUE(vdec2->WaitForEos()) << "[SAMPLE_ID]:" << vdec2->sampleId_;
    EXPECT_EQ(vdec2->SetOutputSurface(true), AV_ERR_OK) << "[SAMPLE_ID]:" << vdec2->sampleId_;
    EXPECT_EQ(vdec2->Release(), AV_ERR_OK) << "[SAMPLE_ID]:" << vdec2->sampleId_;
}

INSTANTIATE_TEST_SUITE_P(, VideoDecStableTestWmv3, testing::Values("Flush", "Stop", "Reset", "SetOutputSurface"));

/**
 * @tc.name: VideoDecoder_wmv3decoder_001
 * @tc.desc: 1. push/free buffer in callback;
 *           2. operate not in callback;
 */
AVCODEC_MTEST_P(VideoDecStableTestWmv3, VideoDecoder_wmv3decoder_001, TestSize.Level1, VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecStableTestWmv3::GetParam();
    vdec->frameCount_ = 30; // 30: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_WMV3;
    vdec->inPath_ = WMV3_STREAM_FILE_NAME;
    vdec->sampleWidth_ = WMV3_SAPMLE_WIDTH;
    vdec->sampleHeight_ = WMV3_SAMPLE_HEIGHT;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = WMV3_DECODER_DUMP_FILE_NAME;
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
 * @tc.name: VideoDecoder_wmv3decoder_002
 * @tc.desc: 1. push/free buffer in callback;
 *           2. operate in input callback;
 */
AVCODEC_MTEST_P(VideoDecStableTestWmv3, VideoDecoder_wmv3decoder_002, TestSize.Level1, VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecStableTestWmv3::GetParam();
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_WMV3;
    vdec->inPath_ = WMV3_STREAM_FILE_NAME;
    vdec->sampleWidth_ = WMV3_SAPMLE_WIDTH;
    vdec->sampleHeight_ = WMV3_SAMPLE_HEIGHT;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = WMV3_DECODER_DUMP_FILE_NAME;
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
 * @tc.name: VideoDecoder_wmv3decoder_003
 * @tc.desc: 1. push/free buffer in callback;
 *           2. operate in output callback;
 */
AVCODEC_MTEST_P(VideoDecStableTestWmv3, VideoDecoder_wmv3decoder_003, TestSize.Level1, VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecStableTestWmv3::GetParam();
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_WMV3;
    vdec->inPath_ = WMV3_STREAM_FILE_NAME;
    vdec->sampleWidth_ = WMV3_SAPMLE_WIDTH;
    vdec->sampleHeight_ = WMV3_SAMPLE_HEIGHT;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = WMV3_DECODER_DUMP_FILE_NAME;
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
 * @tc.name: VideoDecoder_wmv3decoder_004
 * @tc.desc: 1. push/free buffer in callback;
 *           2. operate not in callback;
 *           3. set surface;
 */
AVCODEC_MTEST_P(VideoDecStableTestWmv3, VideoDecoder_wmv3decoder_004, TestSize.Level1, VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecStableTestWmv3::GetParam();
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_WMV3;
    vdec->inPath_ = WMV3_STREAM_FILE_NAME;
    vdec->sampleWidth_ = WMV3_SAPMLE_WIDTH;
    vdec->sampleHeight_ = WMV3_SAMPLE_HEIGHT;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = WMV3_DECODER_DUMP_FILE_NAME;
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
 * @tc.name: VideoDecoder_wmv3decoder_005
 * @tc.desc: 1. push/free buffer in callback;
 *           2. operate in input callback;
 *           3. set surface;
 */
AVCODEC_MTEST_P(VideoDecStableTestWmv3, VideoDecoder_wmv3decoder_005, TestSize.Level1, VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecStableTestWmv3::GetParam();
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_WMV3;
    vdec->inPath_ = WMV3_STREAM_FILE_NAME;
    vdec->sampleWidth_ = WMV3_SAPMLE_WIDTH;
    vdec->sampleHeight_ = WMV3_SAMPLE_HEIGHT;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = WMV3_DECODER_DUMP_FILE_NAME;
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
 * @tc.name: VideoDecoder_wmv3decoder_main_001
 * @tc.desc: 1. push/free buffer in callback;
 *           2. operate not in callback;
 */
AVCODEC_MTEST_P(VideoDecStableTestWmv3, VideoDecoder_wmv3decoder_main_001, TestSize.Level1,
    VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecStableTestWmv3::GetParam();
    vdec->frameCount_ = 21; // 21: input frame num
    vdec->skipOutFrameHalfCheck_ = true;
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_WMV3;
    vdec->inPath_ = WMV3_MAIN_STREAM_FILE_NAME;
    vdec->sampleWidth_ = WMV3_MAIN_WIDTH;
    vdec->sampleHeight_ = WMV3_MAIN_HEIGHT;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = WMV3_DECODER_DUMP_FILE_NAME;
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
 * @tc.name: VideoDecoder_wmv3decoder_main_002
 * @tc.desc: 1. push/free buffer in callback;
 *           2. operate in input callback;
 */
AVCODEC_MTEST_P(VideoDecStableTestWmv3, VideoDecoder_wmv3decoder_main_002,
                                TestSize.Level1, VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecStableTestWmv3::GetParam();
    vdec->frameCount_ = 21; // 21: input frame num
    vdec->skipOutFrameHalfCheck_ = true;
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_WMV3;
    vdec->inPath_ = WMV3_MAIN_STREAM_FILE_NAME;
    vdec->sampleWidth_ = WMV3_MAIN_WIDTH;
    vdec->sampleHeight_ = WMV3_MAIN_HEIGHT;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = WMV3_DECODER_DUMP_FILE_NAME;
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
 * @tc.name: VideoDecoder_wmv3decoder_main_003
 * @tc.desc: 1. push/free buffer in callback;
 *           2. operate in output callback;
 */
AVCODEC_MTEST_P(VideoDecStableTestWmv3, VideoDecoder_wmv3decoder_main_003, TestSize.Level1,
    VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecStableTestWmv3::GetParam();
    vdec->frameCount_ = 21; // 21: input frame num
    vdec->skipOutFrameHalfCheck_ = true;
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_WMV3;
    vdec->inPath_ = WMV3_MAIN_STREAM_FILE_NAME;
    vdec->sampleWidth_ = WMV3_MAIN_WIDTH;
    vdec->sampleHeight_ = WMV3_MAIN_HEIGHT;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = WMV3_DECODER_DUMP_FILE_NAME;
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
 * @tc.name: VideoDecoder_wmv3decoder_main_004
 * @tc.desc: 1. push/free buffer in callback;
 *           2. operate not in callback;
 *           3. set surface;
 */
AVCODEC_MTEST_P(VideoDecStableTestWmv3, VideoDecoder_wmv3decoder_main_004, TestSize.Level1,
    VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecStableTestWmv3::GetParam();
    vdec->frameCount_ = 21; // 21: input frame num
    vdec->skipOutFrameHalfCheck_ = true;
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_WMV3;
    vdec->inPath_ = WMV3_MAIN_STREAM_FILE_NAME;
    vdec->sampleWidth_ = WMV3_MAIN_WIDTH;
    vdec->sampleHeight_ = WMV3_MAIN_HEIGHT;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = WMV3_DECODER_DUMP_FILE_NAME;
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
 * @tc.name: VideoDecoder_wmv3decoder_main_005
 * @tc.desc: 1. push/free buffer in callback;
 *           2. operate in input callback;
 *           3. set surface;
 */
AVCODEC_MTEST_P(VideoDecStableTestWmv3, VideoDecoder_wmv3decoder_main_005,
                                TestSize.Level1, VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecStableTestWmv3::GetParam();
    vdec->frameCount_ = 21; // 21: input frame num
    vdec->skipOutFrameHalfCheck_ = true;
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_WMV3;
    vdec->inPath_ = WMV3_MAIN_STREAM_FILE_NAME;
    vdec->sampleWidth_ = WMV3_MAIN_WIDTH;
    vdec->sampleHeight_ = WMV3_MAIN_HEIGHT;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = WMV3_DECODER_DUMP_FILE_NAME;
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
 * @tc.name: VideoDecoder_wmv3decoder_With_Queue_001
 * @tc.desc: 1. push/free buffer in queue;
 *           2. operate not in callback;
 */
AVCODEC_MTEST_P(VideoDecStableTestWmv3, VideoDecoder_wmv3decoder_With_Queue_001, TestSize.Level1,
                VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecStableTestWmv3::GetParam();
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_WMV3;
    vdec->inPath_ = WMV3_STREAM_FILE_NAME;
    vdec->sampleWidth_ = WMV3_SAPMLE_WIDTH;
    vdec->sampleHeight_ = WMV3_SAMPLE_HEIGHT;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = WMV3_DECODER_DUMP_FILE_NAME;
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
 * @tc.name: VideoDecoder_wmv3decoder_With_Queue_002
 * @tc.desc: 1. push/free buffer in queue;
 *           2. operate in input callback;
 */
AVCODEC_MTEST_P(VideoDecStableTestWmv3, VideoDecoder_wmv3decoder_With_Queue_002, TestSize.Level1,
                VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecStableTestWmv3::GetParam();
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_WMV3;
    vdec->inPath_ = WMV3_STREAM_FILE_NAME;
    vdec->sampleWidth_ = WMV3_SAPMLE_WIDTH;
    vdec->sampleHeight_ = WMV3_SAMPLE_HEIGHT;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = WMV3_DECODER_DUMP_FILE_NAME;
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
 * @tc.name: VideoDecoder_wmv3decoder_With_Queue_003
 * @tc.desc: 1. push buffer in callback;
 *           2. operate in output callback;
 *           3. free buffer in queue;
 */
AVCODEC_MTEST_P(VideoDecStableTestWmv3, VideoDecoder_wmv3decoder_With_Queue_003, TestSize.Level1,
                VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecStableTestWmv3::GetParam();
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_WMV3;
    vdec->inPath_ = WMV3_STREAM_FILE_NAME;
    vdec->sampleWidth_ = WMV3_SAPMLE_WIDTH;
    vdec->sampleHeight_ = WMV3_SAMPLE_HEIGHT;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = WMV3_DECODER_DUMP_FILE_NAME;
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
 * @tc.name: VideoDecoder_wmv3decoder_With_Queue_004
 * @tc.desc: 1. push buffer in callback;
 *           2. operate not in callback;
 *           3. render buffer in queue;
 *           4. set surface;
 */
AVCODEC_MTEST_P(VideoDecStableTestWmv3, VideoDecoder_wmv3decoder_With_Queue_004, TestSize.Level1,
                VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecStableTestWmv3::GetParam();
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_WMV3;
    vdec->inPath_ = WMV3_STREAM_FILE_NAME;
    vdec->sampleWidth_ = WMV3_SAPMLE_WIDTH;
    vdec->sampleHeight_ = WMV3_SAMPLE_HEIGHT;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = WMV3_DECODER_DUMP_FILE_NAME;
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
 * @tc.name: VideoDecoder_wmv3decoder_With_Queue_005
 * @tc.desc: 1. push buffer in callback;
 *           2. operate in input callback;
 *           3. render buffer in queue;
 *           4. set surface;
 */
AVCODEC_MTEST_P(VideoDecStableTestWmv3, VideoDecoder_wmv3decoder_With_Queue_005, TestSize.Level1,
                VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecStableTestWmv3::GetParam();
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_WMV3;
    vdec->inPath_ = WMV3_STREAM_FILE_NAME;
    vdec->sampleWidth_ = WMV3_SAPMLE_WIDTH;
    vdec->sampleHeight_ = WMV3_SAMPLE_HEIGHT;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = WMV3_DECODER_DUMP_FILE_NAME;
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
 * @tc.name: VideoDecoder_wmv3decoder_With_Queue_Main_001
 * @tc.desc: 1. push/free buffer in queue;
 *           2. operate not in callback;
 */
AVCODEC_MTEST_P(VideoDecStableTestWmv3, VideoDecoder_wmv3decoder_With_Queue_Main_001, TestSize.Level1,
                VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecStableTestWmv3::GetParam();
    vdec->frameCount_ = 21; // 21: input frame num
    vdec->skipOutFrameHalfCheck_ = true;
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_WMV3;
    vdec->inPath_ = WMV3_MAIN_STREAM_FILE_NAME;
    vdec->sampleWidth_ = WMV3_MAIN_WIDTH;
    vdec->sampleHeight_ = WMV3_MAIN_HEIGHT;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = WMV3_DECODER_DUMP_FILE_NAME;
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
 * @tc.name: VideoDecoder_wmv3decoder_With_Queue_Main_002
 * @tc.desc: 1. push/free buffer in queue;
 *           2. operate in input callback;
 */
AVCODEC_MTEST_P(VideoDecStableTestWmv3, VideoDecoder_wmv3decoder_With_Queue_Main_002, TestSize.Level1,
                VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecStableTestWmv3::GetParam();
    vdec->frameCount_ = 21; // 21: input frame num
    vdec->skipOutFrameHalfCheck_ = true;
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_WMV3;
    vdec->inPath_ = WMV3_MAIN_STREAM_FILE_NAME;
    vdec->sampleWidth_ = WMV3_MAIN_WIDTH;
    vdec->sampleHeight_ = WMV3_MAIN_HEIGHT;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = WMV3_DECODER_DUMP_FILE_NAME;
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
 * @tc.name: VideoDecoder_wmv3decoder_With_Queue_Main_003
 * @tc.desc: 1. push buffer in callback;
 *           2. operate in output callback;
 *           3. free buffer in queue;
 */
AVCODEC_MTEST_P(VideoDecStableTestWmv3, VideoDecoder_wmv3decoder_With_Queue_Main_003, TestSize.Level1,
                VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecStableTestWmv3::GetParam();
    vdec->frameCount_ = 21; // 21: input frame num
    vdec->skipOutFrameHalfCheck_ = true;
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_WMV3;
    vdec->inPath_ = WMV3_MAIN_STREAM_FILE_NAME;
    vdec->sampleWidth_ = WMV3_MAIN_WIDTH;
    vdec->sampleHeight_ = WMV3_MAIN_HEIGHT;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = WMV3_DECODER_DUMP_FILE_NAME;
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
 * @tc.name: VideoDecoder_wmv3decoder_With_Queue_Main_004
 * @tc.desc: 1. push buffer in callback;
 *           2. operate not in callback;
 *           3. render buffer in queue;
 *           4. set surface;
 */
AVCODEC_MTEST_P(VideoDecStableTestWmv3, VideoDecoder_wmv3decoder_With_Queue_Main_004, TestSize.Level1,
                VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecStableTestWmv3::GetParam();
    vdec->frameCount_ = 21; // 21: input frame num
    vdec->skipOutFrameHalfCheck_ = true;
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_WMV3;
    vdec->inPath_ = WMV3_MAIN_STREAM_FILE_NAME;
    vdec->sampleWidth_ = WMV3_MAIN_WIDTH;
    vdec->sampleHeight_ = WMV3_MAIN_HEIGHT;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = WMV3_DECODER_DUMP_FILE_NAME;
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
 * @tc.name: VideoDecoder_wmv3decoder_With_Queue_Main_005
 * @tc.desc: 1. push buffer in callback;
 *           2. operate in input callback;
 *           3. render buffer in queue;
 *           4. set surface;
 */
AVCODEC_MTEST_P(VideoDecStableTestWmv3, VideoDecoder_wmv3decoder_With_Queue_Main_005, TestSize.Level1,
                VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecStableTestWmv3::GetParam();
    vdec->frameCount_ = 21; // 21: input frame num
    vdec->skipOutFrameHalfCheck_ = true;
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_WMV3;
    vdec->inPath_ = WMV3_MAIN_STREAM_FILE_NAME;
    vdec->sampleWidth_ = WMV3_MAIN_WIDTH;
    vdec->sampleHeight_ = WMV3_MAIN_HEIGHT;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = WMV3_DECODER_DUMP_FILE_NAME;
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
 * @tc.name: VideoDecoder_wmv3decoder_AVBuffer_001
 * @tc.desc: 1. push/free buffer in callback;
 *           2. operate not in callback;
 */
AVCODEC_MTEST_P(VideoDecStableTestWmv3, VideoDecoder_wmv3decoder_AVBuffer_001, TestSize.Level1,
    VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecStableTestWmv3::GetParam();
    vdec->frameCount_ = 30; // 30: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_WMV3;
    vdec->inPath_ = WMV3_STREAM_FILE_NAME;
    vdec->sampleWidth_ = WMV3_SAPMLE_WIDTH;
    vdec->sampleHeight_ = WMV3_SAMPLE_HEIGHT;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = WMV3_DECODER_DUMP_FILE_NAME;
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
 * @tc.name: VideoDecoder_wmv3decoder_AVBuffer_002
 * @tc.desc: 1. push/free buffer in callback;
 *           2. operate in input callback;
 */
AVCODEC_MTEST_P(VideoDecStableTestWmv3, VideoDecoder_wmv3decoder_AVBuffer_002, TestSize.Level1,
    VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecStableTestWmv3::GetParam();
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_WMV3;
    vdec->inPath_ = WMV3_STREAM_FILE_NAME;
    vdec->sampleWidth_ = WMV3_SAPMLE_WIDTH;
    vdec->sampleHeight_ = WMV3_SAMPLE_HEIGHT;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = WMV3_DECODER_DUMP_FILE_NAME;
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
 * @tc.name: VideoDecoder_wmv3decoder_AVBuffer_003
 * @tc.desc: 1. push/free buffer in callback;
 *           2. operate in output callback;
 */
AVCODEC_MTEST_P(VideoDecStableTestWmv3, VideoDecoder_wmv3decoder_AVBuffer_003, TestSize.Level1,
    VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecStableTestWmv3::GetParam();
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_WMV3;
    vdec->inPath_ = WMV3_STREAM_FILE_NAME;
    vdec->sampleWidth_ = WMV3_SAPMLE_WIDTH;
    vdec->sampleHeight_ = WMV3_SAMPLE_HEIGHT;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = WMV3_DECODER_DUMP_FILE_NAME;
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
 * @tc.name: VideoDecoder_wmv3decoder_AVBuffer_004
 * @tc.desc: 1. push/free buffer in callback;
 *           2. operate not in callback;
 *           3. set surface;
 */
AVCODEC_MTEST_P(VideoDecStableTestWmv3, VideoDecoder_wmv3decoder_AVBuffer_004,
        TestSize.Level1, VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecStableTestWmv3::GetParam();
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_WMV3;
    vdec->inPath_ = WMV3_STREAM_FILE_NAME;
    vdec->sampleWidth_ = WMV3_SAPMLE_WIDTH;
    vdec->sampleHeight_ = WMV3_SAMPLE_HEIGHT;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = WMV3_DECODER_DUMP_FILE_NAME;
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
 * @tc.name: VideoDecoder_wmv3decoder_AVBuffer_005
 * @tc.desc: 1. push/free buffer in callback;
 *           2. operate in input callback;
 *           3. set surface;
 */
AVCODEC_MTEST_P(VideoDecStableTestWmv3, VideoDecoder_wmv3decoder_AVBuffer_005, TestSize.Level1,
    VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecStableTestWmv3::GetParam();
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_WMV3;
    vdec->inPath_ = WMV3_STREAM_FILE_NAME;
    vdec->sampleWidth_ = WMV3_SAPMLE_WIDTH;
    vdec->sampleHeight_ = WMV3_SAMPLE_HEIGHT;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = WMV3_DECODER_DUMP_FILE_NAME;
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
 * @tc.name: VideoDecoder_wmv3decoder_AVBuffer_Main_001
 * @tc.desc: 1. push/free buffer in callback;
 *           2. operate not in callback;
 */
AVCODEC_MTEST_P(VideoDecStableTestWmv3, VideoDecoder_wmv3decoder_AVBuffer_Main_001, TestSize.Level1,
                VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecStableTestWmv3::GetParam();
    vdec->frameCount_ = 21; // 21: input frame num
    vdec->skipOutFrameHalfCheck_ = true;
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_WMV3;
    vdec->inPath_ = WMV3_MAIN_STREAM_FILE_NAME;
    vdec->sampleWidth_ = WMV3_MAIN_WIDTH;
    vdec->sampleHeight_ = WMV3_MAIN_HEIGHT;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = WMV3_DECODER_DUMP_FILE_NAME;
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
 * @tc.name: VideoDecoder_wmv3decoder_AVBuffer_Main_002
 * @tc.desc: 1. push/free buffer in callback;
 *           2. operate in input callback;
 */
AVCODEC_MTEST_P(VideoDecStableTestWmv3, VideoDecoder_wmv3decoder_AVBuffer_Main_002, TestSize.Level1,
                VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecStableTestWmv3::GetParam();
    vdec->frameCount_ = 21; // 21: input frame num
    vdec->skipOutFrameHalfCheck_ = true;
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_WMV3;
    vdec->inPath_ = WMV3_MAIN_STREAM_FILE_NAME;
    vdec->sampleWidth_ = WMV3_MAIN_WIDTH;
    vdec->sampleHeight_ = WMV3_MAIN_HEIGHT;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = WMV3_DECODER_DUMP_FILE_NAME;
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
 * @tc.name: VideoDecoder_wmv3decoder_AVBuffer_Main_003
 * @tc.desc: 1. push/free buffer in callback;
 *           2. operate in output callback;
 */
AVCODEC_MTEST_P(VideoDecStableTestWmv3, VideoDecoder_wmv3decoder_AVBuffer_Main_003, TestSize.Level1,
                VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecStableTestWmv3::GetParam();
    vdec->frameCount_ = 21; // 21: input frame num
    vdec->skipOutFrameHalfCheck_ = true;
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_WMV3;
    vdec->inPath_ = WMV3_MAIN_STREAM_FILE_NAME;
    vdec->sampleWidth_ = WMV3_MAIN_WIDTH;
    vdec->sampleHeight_ = WMV3_MAIN_HEIGHT;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = WMV3_DECODER_DUMP_FILE_NAME;
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
 * @tc.name: VideoDecoder_wmv3decoder_AVBuffer_Main_004
 * @tc.desc: 1. push/free buffer in callback;
 *           2. operate not in callback;
 *           3. set surface;
 */
AVCODEC_MTEST_P(VideoDecStableTestWmv3, VideoDecoder_wmv3decoder_AVBuffer_Main_004, TestSize.Level1,
                VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecStableTestWmv3::GetParam();
    vdec->frameCount_ = 21; // 21: input frame num
    vdec->skipOutFrameHalfCheck_ = true;
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_WMV3;
    vdec->inPath_ = WMV3_MAIN_STREAM_FILE_NAME;
    vdec->sampleWidth_ = WMV3_MAIN_WIDTH;
    vdec->sampleHeight_ = WMV3_MAIN_HEIGHT;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = WMV3_DECODER_DUMP_FILE_NAME;
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
 * @tc.name: VideoDecoder_wmv3decoder_AVBuffer_Main_005
 * @tc.desc: 1. push/free buffer in callback;
 *           2. operate in input callback;
 *           3. set surface;
 */
AVCODEC_MTEST_P(VideoDecStableTestWmv3, VideoDecoder_wmv3decoder_AVBuffer_Main_005, TestSize.Level1,
                VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecStableTestWmv3::GetParam();
    vdec->frameCount_ = 21; // 21: input frame num
    vdec->skipOutFrameHalfCheck_ = true;
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_WMV3;
    vdec->inPath_ = WMV3_MAIN_STREAM_FILE_NAME;
    vdec->sampleWidth_ = WMV3_MAIN_WIDTH;
    vdec->sampleHeight_ = WMV3_MAIN_HEIGHT;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = WMV3_DECODER_DUMP_FILE_NAME;
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
 * @tc.name: VideoDecoder_wmv3decoder_AVBuffer_With_Queue_001
 * @tc.desc: 1. push/free buffer in queue;
 *           2. operate not in callback;
 */
AVCODEC_MTEST_P(VideoDecStableTestWmv3, VideoDecoder_wmv3decoder_AVBuffer_With_Queue_001, TestSize.Level1,
                VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecStableTestWmv3::GetParam();
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_WMV3;
    vdec->inPath_ = WMV3_STREAM_FILE_NAME;
    vdec->sampleWidth_ = WMV3_SAPMLE_WIDTH;
    vdec->sampleHeight_ = WMV3_SAMPLE_HEIGHT;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = WMV3_DECODER_DUMP_FILE_NAME;
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
 * @tc.name: VideoDecoder_wmv3decoder_AVBuffer_With_Queue_002
 * @tc.desc: 1. push/free buffer in queue;
 *           2. operate in input callback;
 */
AVCODEC_MTEST_P(VideoDecStableTestWmv3, VideoDecoder_wmv3decoder_AVBuffer_With_Queue_002, TestSize.Level1,
                VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecStableTestWmv3::GetParam();
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_WMV3;
    vdec->inPath_ = WMV3_STREAM_FILE_NAME;
    vdec->sampleWidth_ = WMV3_SAPMLE_WIDTH;
    vdec->sampleHeight_ = WMV3_SAMPLE_HEIGHT;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = WMV3_DECODER_DUMP_FILE_NAME;
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
 * @tc.name: VideoDecoder_wmv3decoder_AVBuffer_With_Queue_003
 * @tc.desc: 1. push buffer in callback;
 *           2. operate in output callback;
 *           3. free buffer in queue;
 */
AVCODEC_MTEST_P(VideoDecStableTestWmv3, VideoDecoder_wmv3decoder_AVBuffer_With_Queue_003, TestSize.Level1,
                VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecStableTestWmv3::GetParam();
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_WMV3;
    vdec->inPath_ = WMV3_STREAM_FILE_NAME;
    vdec->sampleWidth_ = WMV3_SAPMLE_WIDTH;
    vdec->sampleHeight_ = WMV3_SAMPLE_HEIGHT;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = WMV3_DECODER_DUMP_FILE_NAME;
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
 * @tc.name: VideoDecoder_wmv3decoder_AVBuffer_With_Queue_004
 * @tc.desc: 1. push buffer in callback;
 *           2. operate not in callback;
 *           3. render buffer in queue;
 *           4. set surface;
 */
AVCODEC_MTEST_P(VideoDecStableTestWmv3, VideoDecoder_wmv3decoder_AVBuffer_With_Queue_004, TestSize.Level1,
                VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecStableTestWmv3::GetParam();
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_WMV3;
    vdec->inPath_ = WMV3_STREAM_FILE_NAME;
    vdec->sampleWidth_ = WMV3_SAPMLE_WIDTH;
    vdec->sampleHeight_ = WMV3_SAMPLE_HEIGHT;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = WMV3_DECODER_DUMP_FILE_NAME;
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
 * @tc.name: VideoDecoder_wmv3decoder_AVBuffer_With_Queue_005
 * @tc.desc: 1. push buffer in callback;
 *           2. operate in input callback;
 *           3. render buffer in queue;
 *           4. set surface;
 */
AVCODEC_MTEST_P(VideoDecStableTestWmv3, VideoDecoder_wmv3decoder_AVBuffer_With_Queue_005, TestSize.Level1,
                VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecStableTestWmv3::GetParam();
    vdec->frameCount_ = 60; // 60: input frame num
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_WMV3;
    vdec->inPath_ = WMV3_STREAM_FILE_NAME;
    vdec->sampleWidth_ = WMV3_SAPMLE_WIDTH;
    vdec->sampleHeight_ = WMV3_SAMPLE_HEIGHT;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = WMV3_DECODER_DUMP_FILE_NAME;
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

/**
 * @tc.name: VideoDecoder_wmv3decoder_AVBuffer_With_Queue_Main_001
 * @tc.desc: 1. push/free buffer in queue;
 *           2. operate not in callback;
 */
AVCODEC_MTEST_P(VideoDecStableTestWmv3, VideoDecoder_wmv3decoder_AVBuffer_With_Queue_Main_001, TestSize.Level1,
                VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecStableTestWmv3::GetParam();
    vdec->frameCount_ = 21; // 21: input frame num
    vdec->skipOutFrameHalfCheck_ = true;
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_WMV3;
    vdec->inPath_ = WMV3_MAIN_STREAM_FILE_NAME;
    vdec->sampleWidth_ = WMV3_MAIN_WIDTH;
    vdec->sampleHeight_ = WMV3_MAIN_HEIGHT;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = WMV3_DECODER_DUMP_FILE_NAME;
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
 * @tc.name: VideoDecoder_wmv3decoder_AVBuffer_With_Queue_Main_002
 * @tc.desc: 1. push/free buffer in queue;
 *           2. operate in input callback;
 */
AVCODEC_MTEST_P(VideoDecStableTestWmv3, VideoDecoder_wmv3decoder_AVBuffer_With_Queue_Main_002, TestSize.Level1,
                VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecStableTestWmv3::GetParam();
    vdec->frameCount_ = 21; // 21: input frame num
    vdec->skipOutFrameHalfCheck_ = true;
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_WMV3;
    vdec->inPath_ = WMV3_MAIN_STREAM_FILE_NAME;
    vdec->sampleWidth_ = WMV3_MAIN_WIDTH;
    vdec->sampleHeight_ = WMV3_MAIN_HEIGHT;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = WMV3_DECODER_DUMP_FILE_NAME;
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
 * @tc.name: VideoDecoder_wmv3decoder_AVBuffer_With_Queue_Main_003
 * @tc.desc: 1. push buffer in callback;
 *           2. operate in output callback;
 *           3. free buffer in queue;
 */
AVCODEC_MTEST_P(VideoDecStableTestWmv3, VideoDecoder_wmv3decoder_AVBuffer_With_Queue_Main_003, TestSize.Level1,
                VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecStableTestWmv3::GetParam();
    vdec->frameCount_ = 21; // 21: input frame num
    vdec->skipOutFrameHalfCheck_ = true;
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_WMV3;
    vdec->inPath_ = WMV3_MAIN_STREAM_FILE_NAME;
    vdec->sampleWidth_ = WMV3_MAIN_WIDTH;
    vdec->sampleHeight_ = WMV3_MAIN_HEIGHT;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = WMV3_DECODER_DUMP_FILE_NAME;
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
 * @tc.name: VideoDecoder_wmv3decoder_AVBuffer_With_Queue_Main_004
 * @tc.desc: 1. push buffer in callback;
 *           2. operate not in callback;
 *           3. render buffer in queue;
 *           4. set surface;
 */
AVCODEC_MTEST_P(VideoDecStableTestWmv3, VideoDecoder_wmv3decoder_AVBuffer_With_Queue_Main_004, TestSize.Level1,
                VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecStableTestWmv3::GetParam();
    vdec->frameCount_ = 21; // 21: input frame num
    vdec->skipOutFrameHalfCheck_ = true;
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_WMV3;
    vdec->inPath_ = WMV3_MAIN_STREAM_FILE_NAME;
    vdec->sampleWidth_ = WMV3_MAIN_WIDTH;
    vdec->sampleHeight_ = WMV3_MAIN_HEIGHT;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = WMV3_DECODER_DUMP_FILE_NAME;
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
 * @tc.name: VideoDecoder_wmv3decoder_AVBuffer_With_Queue_Main_005
 * @tc.desc: 1. push buffer in callback;
 *           2. operate in input callback;
 *           3. render buffer in queue;
 *           4. set surface;
 */
AVCODEC_MTEST_P(VideoDecStableTestWmv3, VideoDecoder_wmv3decoder_AVBuffer_With_Queue_Main_005, TestSize.Level1,
                VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecStableTestWmv3::GetParam();
    vdec->frameCount_ = 21; // 21: input frame num
    vdec->skipOutFrameHalfCheck_ = true;
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_WMV3;
    vdec->inPath_ = WMV3_MAIN_STREAM_FILE_NAME;
    vdec->sampleWidth_ = WMV3_MAIN_WIDTH;
    vdec->sampleHeight_ = WMV3_MAIN_HEIGHT;
    vdec->outPath_ = GetTestName();
    vdec->dumpKey_ = WMV3_DECODER_DUMP_FILE_NAME;
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
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV3, false, SOFTWARE);
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