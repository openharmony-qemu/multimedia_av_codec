/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#include "meta/meta_key.h"
#include "vdec_sample.h"
#ifdef VIDEODEC_CAPI_UNIT_TEST
#include "native_avmagic.h"
#include "videodec_capi_mock.h"
#define TEST_SUIT VideoDecCapiTest
#else
#define TEST_SUIT VideoDecInnerTest
#endif

using namespace std;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace testing::ext;
using namespace testing::mt;
using namespace OHOS::MediaAVCodec::VCodecTestParam;

namespace {
std::atomic<int32_t> g_vdecCount = 0;
std::string g_vdecName = "";

void MultiThreadCreateVDec()
{
    std::shared_ptr<VDecSignal> vdecSignal = std::make_shared<VDecSignal>();
    std::shared_ptr<VDecCallbackTest> adecCallback = std::make_shared<VDecCallbackTest>(vdecSignal);
    ASSERT_NE(nullptr, adecCallback);

    std::shared_ptr<VideoDecSample> videoDec = std::make_shared<VideoDecSample>(vdecSignal);
    ASSERT_NE(nullptr, videoDec);

    EXPECT_LE(g_vdecCount.load(), 64); // 64: max instances supported
    if (videoDec->CreateVideoDecMockByName(g_vdecName)) {
        g_vdecCount++;
        cout << "create successed, num:" << g_vdecCount.load() << endl;
    } else {
        cout << "create failed, num:" << g_vdecCount.load() << endl;
        return;
    }
    sleep(1);
    videoDec->Release();
    g_vdecCount--;
}

#ifdef VIDEODEC_CAPI_UNIT_TEST
struct OH_AVCodecCallback GetVoidCallback()
{
    struct OH_AVCodecCallback cb;
    cb.onError = [](OH_AVCodec *codec, int32_t errorCode, void *userData) {
        (void)codec;
        (void)errorCode;
        (void)userData;
    };
    cb.onStreamChanged = [](OH_AVCodec *codec, OH_AVFormat *format, void *userData) {
        (void)codec;
        (void)format;
        (void)userData;
    };
    cb.onNeedInputBuffer = [](OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData) {
        (void)codec;
        (void)index;
        (void)buffer;
        (void)userData;
    };
    cb.onNewOutputBuffer = [](OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData) {
        (void)codec;
        (void)index;
        (void)buffer;
        (void)userData;
    };
    return cb;
}
struct OH_AVCodecAsyncCallback GetVoidAsyncCallback()
{
    struct OH_AVCodecAsyncCallback cb;
    cb.onError = [](OH_AVCodec *codec, int32_t errorCode, void *userData) {
        (void)codec;
        (void)errorCode;
        (void)userData;
    };
    cb.onStreamChanged = [](OH_AVCodec *codec, OH_AVFormat *format, void *userData) {
        (void)codec;
        (void)format;
        (void)userData;
    };
    cb.onNeedInputData = [](OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, void *userData) {
        (void)codec;
        (void)index;
        (void)data;
        (void)userData;
    };
    cb.onNeedOutputData = [](OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, OH_AVCodecBufferAttr *attr,
                             void *userData) {
        (void)codec;
        (void)index;
        (void)data;
        (void)attr;
        (void)userData;
    };
    return cb;
}
#endif
class TEST_SUIT : public testing::TestWithParam<int32_t> {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);

    bool CreateVideoCodecByName(const std::string &decName);
    bool CreateVideoCodecByMime(const std::string &decMime);
    void CreateByNameWithParam(int32_t param);
    void SetFormatWithParam(int32_t param);
    void PrepareSource(int32_t param);

protected:
    std::shared_ptr<CodecListMock> capability_ = nullptr;
    std::shared_ptr<VideoDecSample> videoDec_ = nullptr;
    std::shared_ptr<FormatMock> format_ = nullptr;
    std::shared_ptr<VDecCallbackTest> vdecCallback_ = nullptr;
    std::shared_ptr<VDecCallbackTestExt> vdecCallbackExt_ = nullptr;
    bool isAVBufferMode_ = false;
#ifdef VIDEODEC_CAPI_UNIT_TEST
    OH_AVCodec *codec_ = nullptr;
#endif
};

void TEST_SUIT::SetUpTestCase(void)
{
    auto capability = CodecListMockFactory::GetCapabilityByCategory((CodecMimeType::VIDEO_AVC).data(), false,
                                                                    AVCodecCategory::AVCODEC_HARDWARE);
    ASSERT_NE(nullptr, capability) << (CodecMimeType::VIDEO_AVC).data() << " can not found!" << std::endl;
    g_vdecName = capability->GetName();
}

void TEST_SUIT::TearDownTestCase(void) {}

void TEST_SUIT::SetUp(void)
{
    std::shared_ptr<VDecSignal> vdecSignal = std::make_shared<VDecSignal>();
    vdecCallback_ = std::make_shared<VDecCallbackTest>(vdecSignal);
    ASSERT_NE(nullptr, vdecCallback_);

    vdecCallbackExt_ = std::make_shared<VDecCallbackTestExt>(vdecSignal);
    ASSERT_NE(nullptr, vdecCallbackExt_);

    videoDec_ = std::make_shared<VideoDecSample>(vdecSignal);
    ASSERT_NE(nullptr, videoDec_);

    format_ = FormatMockFactory::CreateFormat();
    ASSERT_NE(nullptr, format_);
}

void TEST_SUIT::TearDown(void)
{
    if (format_ != nullptr) {
        format_->Destroy();
    }
    videoDec_ = nullptr;
#ifdef VIDEODEC_CAPI_UNIT_TEST
    if (codec_ != nullptr) {
        EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Destroy(codec_));
        codec_ = nullptr;
    }
#endif
}

bool TEST_SUIT::CreateVideoCodecByMime(const std::string &decMime)
{
    if (videoDec_->CreateVideoDecMockByMime(decMime) == false || videoDec_->SetCallback(vdecCallback_) != AV_ERR_OK) {
        return false;
    }
    return true;
}

bool TEST_SUIT::CreateVideoCodecByName(const std::string &decName)
{
    if (isAVBufferMode_) {
        if (videoDec_->CreateVideoDecMockByName(decName) == false ||
            videoDec_->SetCallback(vdecCallbackExt_) != AV_ERR_OK) {
            return false;
        }
    } else {
        if (videoDec_->CreateVideoDecMockByName(decName) == false ||
            videoDec_->SetCallback(vdecCallback_) != AV_ERR_OK) {
            return false;
        }
    }
    return true;
}

void TEST_SUIT::CreateByNameWithParam(int32_t param)
{
    std::string codecName = "";
    switch (param) {
        case VCodecTestCode::SW_AVC:
            capability_ = CodecListMockFactory::GetCapabilityByCategory(CodecMimeType::VIDEO_AVC.data(), false,
                                                                        AVCodecCategory::AVCODEC_SOFTWARE);
            break;
        case VCodecTestCode::HW_AVC:
            capability_ = CodecListMockFactory::GetCapabilityByCategory(CodecMimeType::VIDEO_AVC.data(), false,
                                                                        AVCodecCategory::AVCODEC_HARDWARE);
            break;
        case VCodecTestCode::HW_HEVC:
            capability_ = CodecListMockFactory::GetCapabilityByCategory(CodecMimeType::VIDEO_HEVC.data(), false,
                                                                        AVCodecCategory::AVCODEC_HARDWARE);
            break;
        default:
            capability_ = CodecListMockFactory::GetCapabilityByCategory(CodecMimeType::VIDEO_AVC.data(), false,
                                                                        AVCodecCategory::AVCODEC_SOFTWARE);
            break;
    }
    codecName = capability_->GetName();
    std::cout << "CodecName: " << codecName << "\n";
    ASSERT_TRUE(CreateVideoCodecByName(codecName));
}

void TEST_SUIT::PrepareSource(int32_t param)
{
    std::string sourcePath = decSourcePathMap_.at(param);
    if (param == VCodecTestCode::HW_HEVC) {
        videoDec_->SetSourceType(false);
    }
    videoDec_->testParam_ = param;
    std::cout << "SourcePath: " << sourcePath << std::endl;
    videoDec_->SetSource(sourcePath);
    const ::testing::TestInfo *testInfo_ = ::testing::UnitTest::GetInstance()->current_test_info();
    string prefix = "/data/test/media/";
    string fileName = testInfo_->name();
    auto check = [](char it) { return it == '/'; };
    (void)fileName.erase(std::remove_if(fileName.begin(), fileName.end(), check), fileName.end());
    videoDec_->SetOutPath(prefix + fileName);
}

void TEST_SUIT::SetFormatWithParam(int32_t param)
{
    (void)param;
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
}

INSTANTIATE_TEST_SUITE_P(, TEST_SUIT, testing::Values(HW_AVC, HW_HEVC, SW_AVC));

/**
 * @tc.name: VideoDecoder_Multithread_Create_001
 * @tc.desc: try create 100 instances
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_Multithread_Create_001, TestSize.Level1)
{
    SET_THREAD_NUM(100);
    g_vdecCount = 0;
    GTEST_RUN_TASK(MultiThreadCreateVDec);
    cout << "remaining num: " << g_vdecCount.load() << endl;
}

/**
 * @tc.name: VideoDecoder_CreateWithNull_001
 * @tc.desc: video create
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_CreateWithNull_001, TestSize.Level1)
{
    ASSERT_FALSE(CreateVideoCodecByName(""));
}

/**
 * @tc.name: VideoDecoder_CreateWithNull_002
 * @tc.desc: video create
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_CreateWithNull_002, TestSize.Level1)
{
    ASSERT_FALSE(CreateVideoCodecByMime(""));
}

/**
 * @tc.name: VideoDecoder_SetCallback_001
 * @tc.desc: video setcallback
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_SetCallback_001, TestSize.Level1)
{
    ASSERT_TRUE(videoDec_->CreateVideoDecMockByName(g_vdecName));
    ASSERT_EQ(AV_ERR_OK, videoDec_->SetCallback(vdecCallback_));
    ASSERT_EQ(AV_ERR_OK, videoDec_->SetCallback(vdecCallback_));
}

/**
 * @tc.name: VideoDecoder_SetCallback_002
 * @tc.desc: video setcallback
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_SetCallback_002, TestSize.Level1)
{
    ASSERT_TRUE(videoDec_->CreateVideoDecMockByName(g_vdecName));
    ASSERT_EQ(AV_ERR_OK, videoDec_->SetCallback(vdecCallbackExt_));
    ASSERT_EQ(AV_ERR_OK, videoDec_->SetCallback(vdecCallbackExt_));
}

/**
 * @tc.name: VideoDecoder_SetCallback_003
 * @tc.desc: video setcallback
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_SetCallback_003, TestSize.Level1)
{
    ASSERT_TRUE(videoDec_->CreateVideoDecMockByName(g_vdecName));
    ASSERT_EQ(AV_ERR_OK, videoDec_->SetCallback(vdecCallback_));
    ASSERT_NE(AV_ERR_OK, videoDec_->SetCallback(vdecCallbackExt_));
}

/**
 * @tc.name: VideoDecoder_SetCallback_004
 * @tc.desc: video setcallback
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_SetCallback_004, TestSize.Level1)
{
    ASSERT_TRUE(videoDec_->CreateVideoDecMockByName(g_vdecName));
    ASSERT_EQ(AV_ERR_OK, videoDec_->SetCallback(vdecCallbackExt_));
    ASSERT_NE(AV_ERR_OK, videoDec_->SetCallback(vdecCallback_));
}

#ifdef VIDEODEC_CAPI_UNIT_TEST
/**
 * @tc.name: VideoDecoder_SetCallback_Invalid_001
 * @tc.desc: video setcallback
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_SetCallback_Invalid_001, TestSize.Level1)
{
    codec_ = OH_VideoDecoder_CreateByMime((CodecMimeType::VIDEO_AVC).data());
    ASSERT_NE(nullptr, codec_);
    struct OH_AVCodecCallback cb = GetVoidCallback();
    EXPECT_EQ(AV_ERR_INVALID_VAL, OH_VideoDecoder_RegisterCallback(nullptr, cb, nullptr));
}

/**
 * @tc.name: VideoDecoder_SetCallback_Invalid_002
 * @tc.desc: video setcallback
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_SetCallback_Invalid_002, TestSize.Level1)
{
    codec_ = OH_VideoDecoder_CreateByMime((CodecMimeType::VIDEO_AVC).data());
    ASSERT_NE(nullptr, codec_);
    struct OH_AVCodecCallback cb = GetVoidCallback();
    cb.onNeedInputBuffer = nullptr;
    EXPECT_EQ(AV_ERR_INVALID_VAL, OH_VideoDecoder_RegisterCallback(codec_, cb, nullptr));
}

/**
 * @tc.name: VideoDecoder_SetCallback_Invalid_003
 * @tc.desc: video setcallback
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_SetCallback_Invalid_003, TestSize.Level1)
{
    codec_ = OH_VideoDecoder_CreateByMime((CodecMimeType::VIDEO_AVC).data());
    ASSERT_NE(nullptr, codec_);
    struct OH_AVCodecCallback cb = GetVoidCallback();
    cb.onNewOutputBuffer = nullptr;
    EXPECT_EQ(AV_ERR_INVALID_VAL, OH_VideoDecoder_RegisterCallback(codec_, cb, nullptr));
}

/**
 * @tc.name: VideoDecoder_SetCallback_Invalid_004
 * @tc.desc: video setcallback
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_SetCallback_Invalid_004, TestSize.Level1)
{
    codec_ = OH_VideoDecoder_CreateByMime((CodecMimeType::VIDEO_AVC).data());
    ASSERT_NE(nullptr, codec_);
    struct OH_AVCodecCallback cb = GetVoidCallback();
    codec_->magic_ = AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER;
    EXPECT_EQ(AV_ERR_INVALID_VAL, OH_VideoDecoder_RegisterCallback(codec_, cb, nullptr));
    codec_->magic_ = AVMagic::AVCODEC_MAGIC_VIDEO_DECODER;
}

/**
 * @tc.name: VideoDecoder_PushInputBuffer_Invalid_001
 * @tc.desc: video push input buffer
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_PushInputBuffer_Invalid_001, TestSize.Level1)
{
    codec_ = OH_VideoDecoder_CreateByMime((CodecMimeType::VIDEO_AVC).data());
    ASSERT_NE(nullptr, codec_);
    struct OH_AVCodecCallback cb = GetVoidCallback();
    OH_AVCodecBufferAttr attr = {0, 0, 0, 0};
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_RegisterCallback(codec_, cb, nullptr));
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_PushInputData(codec_, 0, attr));
}

/**
 * @tc.name: VideoDecoder_PushInputBuffer_Invalid_002
 * @tc.desc: video push input buffer
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_PushInputBuffer_Invalid_002, TestSize.Level1)
{
    codec_ = OH_VideoDecoder_CreateByMime((CodecMimeType::VIDEO_AVC).data());
    ASSERT_NE(nullptr, codec_);
    struct OH_AVCodecAsyncCallback cb = GetVoidAsyncCallback();
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_SetCallback(codec_, cb, nullptr));
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_PushInputBuffer(codec_, 0));
}

/**
 * @tc.name: VideoDecoder_PushInputBuffer_Invalid_003
 * @tc.desc: video push input buffer
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_PushInputBuffer_Invalid_003, TestSize.Level1)
{
    codec_ = OH_VideoDecoder_CreateByMime((CodecMimeType::VIDEO_AVC).data());
    ASSERT_NE(nullptr, codec_);
    EXPECT_EQ(AV_ERR_INVALID_VAL, OH_VideoDecoder_PushInputBuffer(nullptr, 0));
}

/**
 * @tc.name: VideoDecoder_PushInputBuffer_Invalid_004
 * @tc.desc: video push input buffer
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_PushInputBuffer_Invalid_004, TestSize.Level1)
{
    codec_ = OH_VideoDecoder_CreateByMime((CodecMimeType::VIDEO_AVC).data());
    ASSERT_NE(nullptr, codec_);
    codec_->magic_ = AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER;
    EXPECT_EQ(AV_ERR_INVALID_VAL, OH_VideoDecoder_PushInputBuffer(codec_, 0));
    codec_->magic_ = AVMagic::AVCODEC_MAGIC_VIDEO_DECODER;
}

/**
 * @tc.name: VideoDecoder_Free_Buffer_Invalid_001
 * @tc.desc: video free buffer
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_Free_Buffer_Invalid_001, TestSize.Level1)
{
    codec_ = OH_VideoDecoder_CreateByMime((CodecMimeType::VIDEO_AVC).data());
    ASSERT_NE(nullptr, codec_);
    struct OH_AVCodecCallback cb = GetVoidCallback();
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_RegisterCallback(codec_, cb, nullptr));
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_FreeOutputData(codec_, 0));
}

/**
 * @tc.name: VideoDecoder_Free_Buffer_Invalid_002
 * @tc.desc: video free buffer
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_Free_Buffer_Invalid_002, TestSize.Level1)
{
    codec_ = OH_VideoDecoder_CreateByMime((CodecMimeType::VIDEO_AVC).data());
    ASSERT_NE(nullptr, codec_);
    struct OH_AVCodecAsyncCallback cb = GetVoidAsyncCallback();
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_SetCallback(codec_, cb, nullptr));
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_FreeOutputBuffer(codec_, 0));
}

/**
 * @tc.name: VideoDecoder_Free_Buffer_Invalid_003
 * @tc.desc: video free buffer
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_Free_Buffer_Invalid_003, TestSize.Level1)
{
    codec_ = OH_VideoDecoder_CreateByMime((CodecMimeType::VIDEO_AVC).data());
    ASSERT_NE(nullptr, codec_);
    EXPECT_EQ(AV_ERR_INVALID_VAL, OH_VideoDecoder_FreeOutputBuffer(nullptr, 0));
}

/**
 * @tc.name: VideoDecoder_Free_Buffer_Invalid_004
 * @tc.desc: video free buffer
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_Free_Buffer_Invalid_004, TestSize.Level1)
{
    codec_ = OH_VideoDecoder_CreateByMime((CodecMimeType::VIDEO_AVC).data());
    ASSERT_NE(nullptr, codec_);
    codec_->magic_ = AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER;
    EXPECT_EQ(AV_ERR_INVALID_VAL, OH_VideoDecoder_FreeOutputBuffer(codec_, 0));
    codec_->magic_ = AVMagic::AVCODEC_MAGIC_VIDEO_DECODER;
}

/**
 * @tc.name: VideoDecoder_Render_Buffer_Invalid_001
 * @tc.desc: video render buffer
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_Render_Buffer_Invalid_001, TestSize.Level1)
{
    codec_ = OH_VideoDecoder_CreateByMime((CodecMimeType::VIDEO_AVC).data());
    ASSERT_NE(nullptr, codec_);
    struct OH_AVCodecCallback cb = GetVoidCallback();
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_RegisterCallback(codec_, cb, nullptr));
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_RenderOutputData(codec_, 0));
}

/**
 * @tc.name: VideoDecoder_Render_Buffer_Invalid_002
 * @tc.desc: video render buffer
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_Render_Buffer_Invalid_002, TestSize.Level1)
{
    codec_ = OH_VideoDecoder_CreateByMime((CodecMimeType::VIDEO_AVC).data());
    ASSERT_NE(nullptr, codec_);
    struct OH_AVCodecAsyncCallback cb = GetVoidAsyncCallback();
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_SetCallback(codec_, cb, nullptr));
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_RenderOutputBuffer(codec_, 0));
}

/**
 * @tc.name: VideoDecoder_Render_Buffer_Invalid_003
 * @tc.desc: video render buffer
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_Render_Buffer_Invalid_003, TestSize.Level1)
{
    codec_ = OH_VideoDecoder_CreateByMime((CodecMimeType::VIDEO_AVC).data());
    ASSERT_NE(nullptr, codec_);
    EXPECT_EQ(AV_ERR_INVALID_VAL, OH_VideoDecoder_RenderOutputBuffer(nullptr, 0));
}

/**
 * @tc.name: VideoDecoder_Render_Buffer_Invalid_004
 * @tc.desc: video render buffer
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_Render_Buffer_Invalid_004, TestSize.Level1)
{
    codec_ = OH_VideoDecoder_CreateByMime((CodecMimeType::VIDEO_AVC).data());
    ASSERT_NE(nullptr, codec_);
    codec_->magic_ = AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER;
    EXPECT_EQ(AV_ERR_INVALID_VAL, OH_VideoDecoder_RenderOutputBuffer(codec_, 0));
    codec_->magic_ = AVMagic::AVCODEC_MAGIC_VIDEO_DECODER;
}
#endif

/**
 * @tc.name: VideoDecoder_Create_001
 * @tc.desc: video create
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Create_001, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
}

/**
 * @tc.name: VideoDecoder_Configure_001
 * @tc.desc: correct key input.
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Configure_001, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_ROTATION_ANGLE, 0);     // set rotation_angle 0
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_MAX_INPUT_SIZE, 15000); // set max input size 15000
    EXPECT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
}

/**
 * @tc.name: VideoDecoder_Configure_002
 * @tc.desc: correct key input with redundancy key input
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Configure_002, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_ROTATION_ANGLE, 0);     // set rotation_angle 0
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_MAX_INPUT_SIZE, 15000); // set max input size 15000
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_AAC_IS_ADTS, 1);        // redundancy key
    EXPECT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
}

/**
 * @tc.name: VideoDecoder_Configure_003
 * @tc.desc: correct key input with wrong value type input
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Configure_003, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, -2); // invalid width size -2
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    EXPECT_NE(AV_ERR_OK, videoDec_->Configure(format_));
}

/**
 * @tc.name: VideoDecoder_Configure_004
 * @tc.desc: correct key input with wrong value type input
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Configure_004, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, -2); // invalid height size -2
    EXPECT_NE(AV_ERR_OK, videoDec_->Configure(format_));
}

/**
 * @tc.name: VideoDecoder_Configure_005
 * @tc.desc: empty format input
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Configure_005, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    EXPECT_NE(AV_ERR_OK, videoDec_->Configure(format_));
}

/**
 * @tc.name: VideoDecoder_Start_001
 * @tc.desc: correct flow 1
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Start_001, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
}

/**
 * @tc.name: VideoDecoder_Start_002
 * @tc.desc: correct flow 2
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Start_002, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));

    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
}

/**
 * @tc.name: VideoDecoder_Start_003
 * @tc.desc: correct flow 2
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Start_003, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));

    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Flush());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
}

/**
 * @tc.name: VideoDecoder_Start_004
 * @tc.desc: wrong flow 1
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Start_004, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));

    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    EXPECT_NE(AV_ERR_OK, videoDec_->Start());
}

/**
 * @tc.name: VideoDecoder_Start_005
 * @tc.desc: wrong flow 2
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Start_005, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    PrepareSource(GetParam());
    EXPECT_NE(AV_ERR_OK, videoDec_->Start());
}

/**
 * @tc.name: VideoDecoder_Start_Buffer_001
 * @tc.desc: correct flow 1
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Start_Buffer_001, TestSize.Level1)
{
    isAVBufferMode_ = true;
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    videoDec_->needCheckSHA_ = true;
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
}

/**
 * @tc.name: VideoDecoder_Start_Buffer_002
 * @tc.desc: correct flow 2
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Start_Buffer_002, TestSize.Level1)
{
    isAVBufferMode_ = true;
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    videoDec_->needCheckSHA_ = true;
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));

    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
}

/**
 * @tc.name: VideoDecoder_Start_Buffer_003
 * @tc.desc: correct flow 2
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Start_Buffer_003, TestSize.Level1)
{
    isAVBufferMode_ = true;
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    videoDec_->needCheckSHA_ = true;
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));

    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Flush());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
}

/**
 * @tc.name: VideoDecoder_Start_Buffer_004
 * @tc.desc: wrong flow 2
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Start_Buffer_004, TestSize.Level1)
{
    isAVBufferMode_ = true;
    CreateByNameWithParam(GetParam());
    PrepareSource(GetParam());
    EXPECT_NE(AV_ERR_OK, videoDec_->Start());
}

/**
 * @tc.name: VideoDecoder_Stop_001
 * @tc.desc: correct flow 1
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Stop_001, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));

    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
}

/**
 * @tc.name: VideoDecoder_Stop_002
 * @tc.desc: correct flow 1
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Stop_002, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));

    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Flush());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
}

/**
 * @tc.name: VideoDecoder_Stop_003
 * @tc.desc: wrong flow 1
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Stop_003, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));

    EXPECT_NE(AV_ERR_OK, videoDec_->Stop());
}

/**
 * @tc.name: VideoDecoder_Flush_001
 * @tc.desc: correct flow 1
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Flush_001, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));

    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Flush());
}

/**
 * @tc.name: VideoDecoder_Flush_002
 * @tc.desc: wrong flow 1
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Flush_002, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));

    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Release());
    EXPECT_NE(AV_ERR_OK, videoDec_->Flush());
}

/**
 * @tc.name: VideoDecoder_Reset_001
 * @tc.desc: correct flow 1
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Reset_001, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));

    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Reset());
}

/**
 * @tc.name: VideoDecoder_Reset_002
 * @tc.desc: correct flow 2
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Reset_002, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));

    EXPECT_EQ(AV_ERR_OK, videoDec_->Reset());
}

/**
 * @tc.name: VideoDecoder_Reset_003
 * @tc.desc: correct flow 3
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Reset_003, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));

    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Reset());
}

/**
 * @tc.name: VideoDecoder_Release_001
 * @tc.desc: correct flow 1
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Release_001, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));

    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Release());
}

/**
 * @tc.name: VideoDecoder_Release_002
 * @tc.desc: correct flow 2
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Release_002, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));

    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Release());
}

/**
 * @tc.name: VideoDecoder_Release_003
 * @tc.desc: correct flow 3
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Release_003, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));

    EXPECT_EQ(AV_ERR_OK, videoDec_->Release());
}

/**
 * @tc.name: VideoDecoder_SetSurface_001
 * @tc.desc: video decodec setsurface
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_SetSurface_001, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    ASSERT_EQ(AV_ERR_OK, videoDec_->SetOutputSurface());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
}

/**
 * @tc.name: VideoDecoder_SetSurface_002
 * @tc.desc: wrong flow 1
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_SetSurface_002, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_NE(AV_ERR_OK, videoDec_->SetOutputSurface());
}

/**
 * @tc.name: VideoDecoder_SetSurface_003
 * @tc.desc: wrong flow 2
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_SetSurface_003, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    ASSERT_NE(AV_ERR_OK, videoDec_->SetOutputSurface());
}

/**
 * @tc.name: VideoDecoder_SetSurface_Buffer_001
 * @tc.desc: video decodec setsurface
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_SetSurface_Buffer_001, TestSize.Level1)
{
    isAVBufferMode_ = true;
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    ASSERT_EQ(AV_ERR_OK, videoDec_->SetOutputSurface());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
}

/**
 * @tc.name: VideoDecoder_SetSurface_Buffer_002
 * @tc.desc: wrong flow 2
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_SetSurface_Buffer_002, TestSize.Level1)
{
    isAVBufferMode_ = true;
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    ASSERT_NE(AV_ERR_OK, videoDec_->SetOutputSurface());
}

/**
 * @tc.name: VideoDecoder_Abnormal_001
 * @tc.desc: video codec abnormal func
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Abnormal_001, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_ROTATION_ANGLE, 20); // invalid rotation_angle 20
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_MAX_INPUT_SIZE, -1); // invalid max input size -1

    videoDec_->Configure(format_);
    EXPECT_EQ(AV_ERR_OK, videoDec_->Reset());
    EXPECT_NE(AV_ERR_OK, videoDec_->Start());
    EXPECT_NE(AV_ERR_OK, videoDec_->Flush());
    EXPECT_NE(AV_ERR_OK, videoDec_->Stop());
}

/**
 * @tc.name: VideoDecoder_Abnormal_002
 * @tc.desc: video codec abnormal func
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Abnormal_002, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    PrepareSource(GetParam());
    EXPECT_NE(AV_ERR_OK, videoDec_->Start());
}

/**
 * @tc.name: VideoDecoder_Abnormal_003
 * @tc.desc: video codec abnormal func
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Abnormal_003, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    PrepareSource(GetParam());
    EXPECT_NE(AV_ERR_OK, videoDec_->Flush());
}

/**
 * @tc.name: VideoDecoder_Abnormal_004
 * @tc.desc: video codec abnormal func
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Abnormal_004, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    PrepareSource(GetParam());
    EXPECT_NE(AV_ERR_OK, videoDec_->Stop());
}

/**
 * @tc.name: VideoDecoder_SetParameter_001
 * @tc.desc: video codec SetParameter
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_SetParameter_001, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));

    format_ = FormatMockFactory::CreateFormat();
    ASSERT_NE(nullptr, format_);

    format_->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::YUV420P));
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, DEFAULT_FRAME_RATE);

    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    EXPECT_EQ(AV_ERR_OK, videoDec_->SetParameter(format_));
    EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
}

/**
 * @tc.name: VideoDecoder_SetParameter_002
 * @tc.desc: video codec SetParameter
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_SetParameter_002, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));

    format_ = FormatMockFactory::CreateFormat();
    ASSERT_NE(nullptr, format_);

    format_->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, -2);  // invalid width size -2
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, -2); // invalid height size -2
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::YUV420P));
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, DEFAULT_FRAME_RATE);

    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    EXPECT_EQ(AV_ERR_OK, videoDec_->SetParameter(format_));
    EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
}

/**
 * @tc.name: VideoDecoder_GetOutputDescription_001
 * @tc.desc: video codec GetOutputDescription
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_GetOutputDescription_001, TestSize.Level1)
{
    CreateByNameWithParam(HW_AVC);
    SetFormatWithParam(HW_AVC);
    PrepareSource(HW_AVC);
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));

    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    format_ = videoDec_->GetOutputDescription();
    std::cout << format_->DumpInfo() << std::endl;
    EXPECT_NE(nullptr, format_);
    EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
}

/**
 * @tc.name: VideoDecoder_GetOutputDescription_002
 * @tc.desc: video codec GetOutputDescription
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_GetOutputDescription_002, TestSize.Level1)
{
    CreateByNameWithParam(HW_AVC);
    SetFormatWithParam(HW_AVC);
    PrepareSource(HW_AVC);
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));

    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    format_ = videoDec_->GetOutputDescription();

    std::string dumpInfo = format_->DumpInfo();
    std::cout << "dumpInfo: [" << dumpInfo << "]\n";
    auto checkFunc = [&dumpInfo](const std::string key) {
        std::string keyStr = key + " = ";
        EXPECT_NE(dumpInfo.find(keyStr), string::npos) << "keyStr: [" << keyStr << "]\n"
                                                       << "dumpInfo: [" << dumpInfo << "]\n";
    };
    checkFunc(Media::Tag::VIDEO_CROP_TOP);
    checkFunc(Media::Tag::VIDEO_CROP_BOTTOM);
    checkFunc(Media::Tag::VIDEO_CROP_LEFT);
    checkFunc(Media::Tag::VIDEO_CROP_RIGHT);
    checkFunc(Media::Tag::VIDEO_STRIDE);

    EXPECT_NE(nullptr, format_);
    EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
}

/**
 * @tc.name: VideoDecoder_GetOutputDescription_003
 * @tc.desc: video codec GetOutputDescription
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_GetOutputDescription_003, TestSize.Level1)
{
    CreateByNameWithParam(HW_AVC);
    SetFormatWithParam(HW_AVC);
    PrepareSource(HW_AVC);
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));

    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    format_ = videoDec_->GetOutputDescription();

    int32_t cropRight = 0;
    int32_t stride = 0;
    int32_t cropBottom = 0;

    EXPECT_TRUE(format_->GetIntValue(Media::Tag::VIDEO_CROP_RIGHT, cropRight));
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::VIDEO_STRIDE, stride));
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::VIDEO_CROP_BOTTOM, cropBottom));

    EXPECT_GE(cropRight, DEFAULT_WIDTH - 1);
    EXPECT_GE(stride, DEFAULT_WIDTH);
    EXPECT_GE(cropBottom, DEFAULT_HEIGHT - 1);

    EXPECT_NE(nullptr, format_);
    EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
}

/**
 * @tc.name: VideoDecoder_HDR_Function_001
 * @tc.desc: video decodec hdr function test
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_HDR_Function_001, TestSize.Level1)
{
    capability_ = CodecListMockFactory::GetCapabilityByCategory(CodecMimeType::VIDEO_HEVC.data(), false,
                                                                AVCodecCategory::AVCODEC_HARDWARE);
    std::string codecName = capability_->GetName();
    if (codecName == "OMX.rk.video_decoder.hevc") {
        return;
    }
    std::cout << "CodecName: " << codecName << "\n";
    ASSERT_TRUE(CreateVideoCodecByName(codecName));

    format_->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));

    VCodecTestCode param = VCodecTestCode::HW_HDR;
    std::string sourcePath = decSourcePathMap_.at(param);
    videoDec_->SetSourceType(false);
    videoDec_->testParam_ = param;
    std::cout << "SourcePath: " << sourcePath << std::endl;
    videoDec_->SetSource(sourcePath);
    const ::testing::TestInfo *testInfo_ = ::testing::UnitTest::GetInstance()->current_test_info();
    string prefix = "/data/test/media/";
    string fileName = testInfo_->name();
    auto check = [](char it) { return it == '/'; };
    (void)fileName.erase(std::remove_if(fileName.begin(), fileName.end(), check), fileName.end());
    videoDec_->SetOutPath(prefix + fileName);

    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    ASSERT_EQ(AV_ERR_OK, videoDec_->SetOutputSurface());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
}
} // namespace