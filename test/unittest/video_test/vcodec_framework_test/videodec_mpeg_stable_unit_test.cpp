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
#include "native_avcapability.h"
#include "unittest_utils.h"
#include "vdec_sample.h"

#define PRINT_HILOG
#define TEST_ID vdec->sampleId_
#define SAMPLE_ID "[SAMPLE_ID]:" << TEST_ID
#include "unittest_log.h"
#define MPEG_STABLE_TEST

using namespace std;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace testing::ext;
using namespace testing::mt;

namespace {

class VideoDecStableTest : public testing::TestWithParam<std::string> {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);
    static constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_TEST, STRINGFY(TEST_SUIT)};

private:
    shared_ptr<HeapMemoryThread> heapThread_ = nullptr;
};

void VideoDecStableTest::SetUpTestCase(void) {}

void VideoDecStableTest::TearDownTestCase(void) {}

void VideoDecStableTest::SetUp(void)
{
    heapThread_ = make_shared<HeapMemoryThread>();

    const ::testing::TestInfo *testInfo_ = ::testing::UnitTest::GetInstance()->current_test_info();
    std::string testCaseName = testInfo_->name();
    AVCODEC_LOGI("%{public}s", testCaseName.c_str());
}

void VideoDecStableTest::TearDown(void)
{
    heapThread_ = nullptr;
}

#ifdef MPEG_STABLE_TEST
string GetTestName()
{
    const ::testing::TestInfo *testInfo = ::testing::UnitTest::GetInstance()->current_test_info();
    string fileName = testInfo->name();
    fileName = fileName + (VideoDecSample::isHardware_ ? "_H" : "_F");
    auto check = [](char it) { return it == '/'; };
    (void)fileName.erase(std::remove_if(fileName.begin(), fileName.end(), check), fileName.end());
    return fileName;
}

enum class Mpeg2VideoPaths {
    MPEG2_PROFILE_SIMPLE = 0,
    MPEG2_PROFILE_MAIN,
    MPEG2_PROFILE_SNR,
    MPEG2_PROFILE_SPATIAL,
    MPEG2_PROFILE_HIGH,
    MPEG2_PROFILE_422,
};

enum class Mpeg4VideoPaths {
    MPEG4_PROFILE_ADVANCED_CODING_EFFICIENCY = 0,
    MPEG4_PROFILE_ADVANCED_CORE,
    MPEG4_PROFILE_ADVANCED_REAL_TIME_SIMPLE,
    MPEG4_PROFILE_ADVANCED_SCALABLE_TEXTURE,
    MPEG4_PROFILE_ADVANCED_SIMPLE,
    MPEG4_PROFILE_BASIC_ANIMATED_TEXTURE,
    MPEG4_PROFILE_CORE_SCALABLE,
    MPEG4_PROFILE_CORE,
    MPEG4_PROFILE_HYBRID,
    MPEG4_PROFILE_MAIN,
    MPEG4_PROFILE_NBIT,
    MPEG4_PROFILE_SCALABLE_TEXTURE,
    MPEG4_PROFILE_SIMPLE,
    MPEG4_PROFILE_SIMPLE_FA,
    MPEG4_PROFILE_SIMPLE_SCALABLE,
};

std::string getMpeg4InputPath(Mpeg4VideoPaths path)
{
    switch (path) {
        case Mpeg4VideoPaths::MPEG4_PROFILE_ADVANCED_CODING_EFFICIENCY:
            return "AdvancedCoding.m4v";
        case Mpeg4VideoPaths::MPEG4_PROFILE_ADVANCED_CORE:
            return "AdvancedCore.m4v";
        case Mpeg4VideoPaths::MPEG4_PROFILE_ADVANCED_REAL_TIME_SIMPLE:
            return "AdvancedRealTimeSimple.m4v";
        case Mpeg4VideoPaths::MPEG4_PROFILE_ADVANCED_SCALABLE_TEXTURE:
            return "AdvancedScalableTexture.m4v";
        case Mpeg4VideoPaths::MPEG4_PROFILE_ADVANCED_SIMPLE:
            return "AdvancedSimple.m4v";
        case Mpeg4VideoPaths::MPEG4_PROFILE_BASIC_ANIMATED_TEXTURE:
            return "BasicAnimatedTexture.m4v";
        case Mpeg4VideoPaths::MPEG4_PROFILE_CORE_SCALABLE:
            return "CodeScalable.m4v";
        case Mpeg4VideoPaths::MPEG4_PROFILE_CORE:
            return "Core.m4v";
        case Mpeg4VideoPaths::MPEG4_PROFILE_HYBRID:
            return "Hybrid.m4v";
        case Mpeg4VideoPaths::MPEG4_PROFILE_MAIN:
            return "Main.m4v";
        case Mpeg4VideoPaths::MPEG4_PROFILE_NBIT:
            return "Nbit.m4v";
        case Mpeg4VideoPaths::MPEG4_PROFILE_SCALABLE_TEXTURE:
            return "ScalableTexture.m4v";
        case Mpeg4VideoPaths::MPEG4_PROFILE_SIMPLE:
            return "Simple.m4v";
        case Mpeg4VideoPaths::MPEG4_PROFILE_SIMPLE_FA:
            return "SimpleFaceAnimation.m4v";
        case Mpeg4VideoPaths::MPEG4_PROFILE_SIMPLE_SCALABLE:
            return "SimpleScalable.m4v";
        default:
            return "";
    }
}

std::string getMpeg2InputPath(Mpeg2VideoPaths path)
{
    switch (path) {
        case Mpeg2VideoPaths::MPEG2_PROFILE_422:
            return "422.m2v";
        case Mpeg2VideoPaths::MPEG2_PROFILE_HIGH:
            return "high.m2v";
        case Mpeg2VideoPaths::MPEG2_PROFILE_MAIN:
            return "main.m2v";
        case Mpeg2VideoPaths::MPEG2_PROFILE_SIMPLE:
            return "simple.m2v";
        case Mpeg2VideoPaths::MPEG2_PROFILE_SNR:
            return "snr.m2v";
        case Mpeg2VideoPaths::MPEG2_PROFILE_SPATIAL:
            return "ss.m2v";
        default:
            return "";
    }
}

/**
 * @tc.name: VideoDecoderMpeg4_Checkprofileandlevel
 * @tc.desc: 1. check supported levels and profiles;
 */
HWMTEST_F(VideoDecStableTest, VideoDecoderMpeg4_Checkprofileandlevel, TestSize.Level1, VideoDecSample::threadNum_)
{
    const int32_t *profiles = nullptr;
    uint32_t profilesNum = -1;
    OH_AVCapability *cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_MPEG4_PART2, false, SOFTWARE);
    EXPECT_EQ(OH_AVCapability_GetSupportedProfiles(cap, &profiles, &profilesNum), AV_ERR_OK);
    EXPECT_EQ(profilesNum, static_cast<uint32_t>(Mpeg4VideoPaths::MPEG4_PROFILE_SIMPLE_SCALABLE) + 1);
    for (int32_t i = 0; i < profilesNum; i++) {
        int32_t profile = profiles[i];
        EXPECT_GE(profile, static_cast<uint32_t>(Mpeg4VideoPaths::MPEG4_PROFILE_ADVANCED_CODING_EFFICIENCY));
        EXPECT_LE(profile, static_cast<uint32_t>(Mpeg4VideoPaths::MPEG4_PROFILE_SIMPLE_SCALABLE) + 3);
        const int32_t *levels = nullptr;
        uint32_t levelsNum = -1;
        EXPECT_EQ(OH_AVCapability_GetSupportedLevelsForProfile(cap, profile, &levels, &levelsNum), AV_ERR_OK);
        EXPECT_GT(levelsNum, MPEG4_LEVEL_0);
        EXPECT_LE(levelsNum, MPEG4_LEVEL_6 + 1);
        for (int32_t j = 0; j < levelsNum; j++) {
            int32_t level = levels[j];
            EXPECT_GE(level, MPEG4_LEVEL_0);
            EXPECT_LE(level, MPEG4_LEVEL_6);
        }
    }
}

/**
 * @tc.name: VideoDecoderMpeg2_Checkprofileandlevel
 * @tc.desc: 1. check supported levels and profiles;
 */
HWMTEST_F(VideoDecStableTest, VideoDecoderMpeg2_Checkprofileandlevel, TestSize.Level1, VideoDecSample::threadNum_)
{
    const int32_t *profiles = nullptr;
    uint32_t profilesNum = -1;
    OH_AVCapability *cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_MPEG2, false, SOFTWARE);
    EXPECT_EQ(OH_AVCapability_GetSupportedProfiles(cap, &profiles, &profilesNum), AV_ERR_OK);
    EXPECT_EQ(profilesNum, MPEG2_PROFILE_422 + 1);
    for (int32_t i = 0; i < profilesNum; i++) {
        int32_t profile = profiles[i];
        EXPECT_GE(profile, MPEG2_PROFILE_SIMPLE);
        EXPECT_LE(profile, MPEG2_PROFILE_422);
        const int32_t *levels = nullptr;
        uint32_t levelsNum = -1;
        EXPECT_EQ(OH_AVCapability_GetSupportedLevelsForProfile(cap, profile, &levels, &levelsNum), AV_ERR_OK);
        EXPECT_GT(levelsNum, MPEG2_LEVEL_LL);
        EXPECT_LE(levelsNum, MPEG2_LEVEL_HL + 1);
        for (int32_t j = 0; j < levelsNum; j++) {
            int32_t level = levels[j];
            EXPECT_GE(level, MPEG2_LEVEL_LL);
            EXPECT_LE(level, MPEG2_LEVEL_HL);
        }
    }
}

/**
 * @tc.name: VideoDecoderMpeg2_Checkcombinations
 * @tc.desc: 1. check supported Combinations levels and profiles;
 */
HWMTEST_F(VideoDecStableTest, VideoDecoderMpeg2_Checkcombinations, TestSize.Level1, VideoDecSample::threadNum_)
{
    OH_AVCapability *cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_MPEG4_PART2, false, SOFTWARE);
    ASSERT_EQ(true, OH_AVCapability_AreProfileAndLevelSupported(cap, MPEG4_PROFILE_ADVANCED_CODING_EFFICIENCY, 2));
    ASSERT_EQ(true, OH_AVCapability_AreProfileAndLevelSupported(cap, MPEG4_PROFILE_ADVANCED_CODING_EFFICIENCY, 3));
    ASSERT_EQ(true, OH_AVCapability_AreProfileAndLevelSupported(cap, MPEG4_PROFILE_ADVANCED_CODING_EFFICIENCY, 4));
    ASSERT_EQ(true, OH_AVCapability_AreProfileAndLevelSupported(cap, MPEG4_PROFILE_ADVANCED_CODING_EFFICIENCY, 6));

    cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_MPEG2, false, SOFTWARE);
    ASSERT_EQ(true, OH_AVCapability_AreProfileAndLevelSupported(cap, MPEG2_PROFILE_MAIN, 0));
    ASSERT_EQ(true, OH_AVCapability_AreProfileAndLevelSupported(cap, MPEG2_PROFILE_MAIN, 1));
    ASSERT_EQ(true, OH_AVCapability_AreProfileAndLevelSupported(cap, MPEG2_PROFILE_MAIN, 2));
    ASSERT_EQ(true, OH_AVCapability_AreProfileAndLevelSupported(cap, MPEG2_PROFILE_MAIN, 3));
}

/**
 * @tc.name: VideoDecoderMpeg2_Multithread_Release
 * @tc.desc: 1. push/free buffer in callback;
 *           2. release not in callback;
 */
HWMTEST_F(VideoDecStableTest, VideoDecoderMpeg2_Multithread_Release, TestSize.Level1, VideoDecSample::threadNum_)
{
    for (int i = 0; i < static_cast<int>(Mpeg2VideoPaths::MPEG2_PROFILE_SIMPLE) + 1; ++i) {
        Mpeg2VideoPaths path = static_cast<Mpeg2VideoPaths>(i);
        std::string inputPath = getMpeg2InputPath(path);
        auto vdec = make_shared<VideoDecSample>();
        auto signal = make_shared<VCodecSignal>(vdec);
        vdec->frameCount_ = 600;
        vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_MPEG2;
        vdec->inPath_ = inputPath;
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
}

/**
 * @tc.name: VideoDecoderMpeg4_Multithread_Release
 * @tc.desc: 1. push/free buffer in callback;
 *           2. release not in callback;
 */
HWMTEST_F(VideoDecStableTest, VideoDecoderMpeg4_Multithread_Release, TestSize.Level1, VideoDecSample::threadNum_)
{
    for (int i = 0; i < static_cast<int>(Mpeg4VideoPaths::MPEG4_PROFILE_ADVANCED_CODING_EFFICIENCY) + 1; ++i) {
        Mpeg4VideoPaths path = static_cast<Mpeg4VideoPaths>(i);
        std::string inputPath = getMpeg4InputPath(path);
        auto vdec = make_shared<VideoDecSample>();
        auto signal = make_shared<VCodecSignal>(vdec);
        vdec->frameCount_ = 600;
        vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_MPEG4_PART2;
        vdec->inPath_ = inputPath;
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
}

/**
 * @tc.name: VideoDecoderMpeg2_Multithread_Release_AVBuffer
 * @tc.desc: 1. push/free buffer in callback;
 *           2. release not in callback;
 */
HWMTEST_F(VideoDecStableTest, VideoDecoderMpeg2_Multithread_Release_AVBuffer, TestSize.Level1,
          VideoDecSample::threadNum_)
{
    for (int i = 0; i < static_cast<int>(Mpeg2VideoPaths::MPEG2_PROFILE_SIMPLE) + 1; ++i) {
        Mpeg2VideoPaths path = static_cast<Mpeg2VideoPaths>(i);
        std::string inputPath = getMpeg2InputPath(path);
        auto vdec = make_shared<VideoDecSample>();
        auto signal = make_shared<VCodecSignal>(vdec);
        vdec->frameCount_ = 600;
        vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_MPEG2;
        vdec->inPath_ = inputPath;
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
}

/**
 * @tc.name: VideoDecoderMpeg4_Multithread_Release_AVBuffer
 * @tc.desc: 1. push/free buffer in callback;
 *           2. release not in callback;
 */
HWMTEST_F(VideoDecStableTest, VideoDecoderMpeg4_Multithread_Release_AVBuffer, TestSize.Level1,
          VideoDecSample::threadNum_)
{
    for (int i = 0; i < static_cast<int>(Mpeg4VideoPaths::MPEG4_PROFILE_ADVANCED_CODING_EFFICIENCY) + 1; ++i) {
        Mpeg4VideoPaths path = static_cast<Mpeg4VideoPaths>(i);
        std::string inputPath = getMpeg4InputPath(path);
        auto vdec = make_shared<VideoDecSample>();
        auto signal = make_shared<VCodecSignal>(vdec);
        vdec->frameCount_ = 600;
        vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_MPEG4_PART2;
        vdec->inPath_ = inputPath;
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
}
#endif

/**.
 * @tc.name: VideoDecoder_Multithread_Release_001
 * @tc.desc: 1. push/free buffer in callback;
 *           2. release not in callback;
 */
HWMTEST_F(VideoDecStableTest, VideoDecoder_Multithread_Release_001, TestSize.Level1, VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->frameCount_ = 604;
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_MPEG4_PART2;
    vdec->inPath_ = "mpeg4.m4v";
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
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->frameCount_ = 604;
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_MPEG4_PART2;
    vdec->inPath_ = "mpeg4.m4v";
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

/**
 * @tc.name: VideoDecoder_Multithread_Start_Four_Times
 * @tc.desc: 1. start 4 times
 */
HWMTEST_F(VideoDecStableTest, VideoDecoder_Multithread_Start_Four_Times, TestSize.Level1, VideoDecSample::threadNum_)
{
    auto vdec = make_shared<VideoDecSample>();
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->frameCount_ = 604;
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_MPEG4_PART2;
    vdec->inPath_ = "mpeg4.m4v";
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
    EXPECT_EQ(vdec->Start(), AV_ERR_INVALID_STATE) << SAMPLE_ID;
    EXPECT_EQ(vdec->Start(), AV_ERR_INVALID_STATE) << SAMPLE_ID;
    EXPECT_EQ(vdec->Start(), AV_ERR_INVALID_STATE) << SAMPLE_ID;

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
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->frameCount_ = 604;
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_MPEG4_PART2;
    vdec->inPath_ = "mpeg4.m4v";
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
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->frameCount_ = 604;
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_MPEG4_PART2;
    vdec->inPath_ = "mpeg4.m4v";
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
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecStableTest::GetParam();
    vdec->frameCount_ = 604;
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_MPEG4_PART2;
    vdec->inPath_ = "mpeg4.m4v";
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
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecStableTest::GetParam();
    vdec->frameCount_ = 604;
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_MPEG4_PART2;
    vdec->inPath_ = "mpeg4.m4v";
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
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecStableTest::GetParam();
    vdec->frameCount_ = 604;
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_MPEG4_PART2;
    vdec->inPath_ = "mpeg4.m4v";
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
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecStableTest::GetParam();
    vdec->frameCount_ = 604;
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_MPEG4_PART2;
    vdec->inPath_ = "mpeg4.m4v";
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
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecStableTest::GetParam();
    vdec->frameCount_ = 604;
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_MPEG4_PART2;
    vdec->inPath_ = "mpeg4.m4v";
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
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecStableTest::GetParam();
    vdec->frameCount_ = 604;
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_MPEG4_PART2;
    vdec->inPath_ = "mpeg4.m4v";
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
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecStableTest::GetParam();
    vdec->frameCount_ = 604;
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_MPEG4_PART2;
    vdec->inPath_ = "mpeg4.m4v";
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
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecStableTest::GetParam();
    vdec->frameCount_ = 604;
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_MPEG4_PART2;
    vdec->inPath_ = "mpeg4.m4v";
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
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecStableTest::GetParam();
    vdec->frameCount_ = 604;
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_MPEG4_PART2;
    vdec->inPath_ = "mpeg4.m4v";
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
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecStableTest::GetParam();
    vdec->frameCount_ = 604;
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_MPEG4_PART2;
    vdec->inPath_ = "mpeg4.m4v";
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
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecStableTest::GetParam();
    vdec->frameCount_ = 604;
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_MPEG4_PART2;
    vdec->inPath_ = "mpeg4.m4v";
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
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecStableTest::GetParam();
    vdec->frameCount_ = 604;
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_MPEG4_PART2;
    vdec->inPath_ = "mpeg4.m4v";
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
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecStableTest::GetParam();
    vdec->frameCount_ = 604;
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_MPEG4_PART2;
    vdec->inPath_ = "mpeg4.m4v";
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
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecStableTest::GetParam();
    vdec->frameCount_ = 604;
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_MPEG4_PART2;
    vdec->inPath_ = "mpeg4.m4v";
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
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecStableTest::GetParam();
    vdec->frameCount_ = 604;
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_MPEG4_PART2;
    vdec->inPath_ = "mpeg4.m4v";
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
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecStableTest::GetParam();
    vdec->frameCount_ = 604;
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_MPEG4_PART2;
    vdec->inPath_ = "mpeg4.m4v";
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
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecStableTest::GetParam();
    vdec->frameCount_ = 604;
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_MPEG4_PART2;
    vdec->inPath_ = "mpeg4.m4v";
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
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecStableTest::GetParam();
    vdec->frameCount_ = 604;
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_MPEG4_PART2;
    vdec->inPath_ = "mpeg4.m4v";
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
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecStableTest::GetParam();
    vdec->frameCount_ = 604;
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_MPEG4_PART2;
    vdec->inPath_ = "mpeg4.m4v";
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
    auto signal = make_shared<VCodecSignal>(vdec);
    vdec->operation_ = VideoDecStableTest::GetParam();
    vdec->frameCount_ = 604;
    vdec->mime_ = OH_AVCODEC_MIMETYPE_VIDEO_MPEG4_PART2;
    vdec->inPath_ = "mpeg4.m4v";
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
        } else if (strcmp(argv[i], "--rosen") == 0) {
            VideoDecSample::isRosenWindow_ = true;
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