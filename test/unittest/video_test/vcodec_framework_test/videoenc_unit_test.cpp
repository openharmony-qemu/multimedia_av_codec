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
#include "codeclist_mock.h"
#include "venc_sample.h"
#ifdef VIDEOENC_CAPI_UNIT_TEST
#include "native_avmagic.h"
#include "videoenc_capi_mock.h"
#define TEST_SUIT VideoEncCapiTest
#else
#define TEST_SUIT VideoEncInnerTest
#endif

using namespace std;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace testing::ext;
using namespace testing::mt;
using namespace OHOS::MediaAVCodec::VCodecTestParam;

namespace {
std::atomic<int32_t> g_vencCount = 0;
std::string g_vencName = "";

void MultiThreadCreateVEnc()
{
    std::shared_ptr<VEncSignal> vencSignal = std::make_shared<VEncSignal>();
    std::shared_ptr<VEncCallbackTest> vencCallback = std::make_shared<VEncCallbackTest>(vencSignal);
    ASSERT_NE(nullptr, vencCallback);

    std::shared_ptr<VideoEncSample> videoEnc = std::make_shared<VideoEncSample>(vencSignal);
    ASSERT_NE(nullptr, videoEnc);

    EXPECT_LE(g_vencCount.load(), 64); // 64: max instances supported
    if (videoEnc->CreateVideoEncMockByName(g_vencName)) {
        g_vencCount++;
        cout << "create successed, num:" << g_vencCount.load() << endl;
    } else {
        cout << "create failed, num:" << g_vencCount.load() << endl;
        return;
    }
    sleep(1);
    videoEnc->Release();
    g_vencCount--;
}

#ifdef VIDEOENC_CAPI_UNIT_TEST
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
    std::shared_ptr<VideoEncSample> videoEnc_ = nullptr;
    std::shared_ptr<FormatMock> format_ = nullptr;
    std::shared_ptr<VEncCallbackTest> vencCallback_ = nullptr;
    std::shared_ptr<VEncCallbackTestExt> vencCallbackExt_ = nullptr;
    std::shared_ptr<VEncParamCallbackTest> vencParamCallback_ = nullptr;
    bool isAVBufferMode_ = false;
    bool isTemporalScalabilitySyncIdr_ = false;
#ifdef VIDEOENC_CAPI_UNIT_TEST
    OH_AVCodec *codec_ = nullptr;
#endif
};

void TEST_SUIT::SetUpTestCase(void)
{
    auto capability = CodecListMockFactory::GetCapabilityByCategory((CodecMimeType::VIDEO_AVC).data(), true,
                                                                    AVCodecCategory::AVCODEC_HARDWARE);
    ASSERT_NE(nullptr, capability) << (CodecMimeType::VIDEO_AVC).data() << " can not found!" << std::endl;
    g_vencName = capability->GetName();
}

void TEST_SUIT::TearDownTestCase(void) {}

void TEST_SUIT::SetUp(void)
{
    std::shared_ptr<VEncSignal> vencSignal = std::make_shared<VEncSignal>();
    vencCallback_ = std::make_shared<VEncCallbackTest>(vencSignal);
    ASSERT_NE(nullptr, vencCallback_);

    vencCallbackExt_ = std::make_shared<VEncCallbackTestExt>(vencSignal);
    ASSERT_NE(nullptr, vencCallbackExt_);

    vencParamCallback_ = std::make_shared<VEncParamCallbackTest>(vencSignal);
    ASSERT_NE(nullptr, vencParamCallback_);

    videoEnc_ = std::make_shared<VideoEncSample>(vencSignal);
    ASSERT_NE(nullptr, videoEnc_);

    format_ = FormatMockFactory::CreateFormat();
    ASSERT_NE(nullptr, format_);
}

void TEST_SUIT::TearDown(void)
{
    isAVBufferMode_ = false;
    isTemporalScalabilitySyncIdr_ = false;
    if (format_ != nullptr) {
        format_->Destroy();
    }
    videoEnc_ = nullptr;
#ifdef VIDEOENC_CAPI_UNIT_TEST
    if (codec_ != nullptr) {
        EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(codec_));
        codec_ = nullptr;
    }
#endif
}

bool TEST_SUIT::CreateVideoCodecByMime(const std::string &encMime)
{
    if (videoEnc_->CreateVideoEncMockByMime(encMime) == false || videoEnc_->SetCallback(vencCallback_) != AV_ERR_OK) {
        return false;
    }
    return true;
}

bool TEST_SUIT::CreateVideoCodecByName(const std::string &name)
{
    if (isAVBufferMode_) {
        if (videoEnc_->CreateVideoEncMockByName(name) == false ||
            videoEnc_->SetCallback(vencCallbackExt_) != AV_ERR_OK) {
            return false;
        }
    } else {
        if (videoEnc_->CreateVideoEncMockByName(name) == false || videoEnc_->SetCallback(vencCallback_) != AV_ERR_OK) {
            return false;
        }
    }
    if (isTemporalScalabilitySyncIdr_) {
        videoEnc_->isTemporalScalabilitySyncIdr_ = true;
    }
    return true;
}

void TEST_SUIT::CreateByNameWithParam(int32_t param)
{
    std::string codecName = "";
    switch (param) {
        case VCodecTestCode::HW_AVC:
            capability_ = CodecListMockFactory::GetCapabilityByCategory(CodecMimeType::VIDEO_AVC.data(), true,
                                                                        AVCodecCategory::AVCODEC_HARDWARE);
            break;
        case VCodecTestCode::HW_HEVC:
            capability_ = CodecListMockFactory::GetCapabilityByCategory(CodecMimeType::VIDEO_HEVC.data(), true,
                                                                        AVCodecCategory::AVCODEC_HARDWARE);
            break;
        default:
            capability_ = CodecListMockFactory::GetCapabilityByCategory(CodecMimeType::VIDEO_AVC.data(), true,
                                                                        AVCodecCategory::AVCODEC_SOFTWARE);
            break;
    }
    codecName = capability_->GetName();
    std::cout << "CodecName: " << codecName << "\n";
    ASSERT_TRUE(CreateVideoCodecByName(codecName));
}

void TEST_SUIT::PrepareSource(int32_t param)
{
    const ::testing::TestInfo *testInfo_ = ::testing::UnitTest::GetInstance()->current_test_info();
    string prefix = "/data/test/media/";
    string fileName = testInfo_->name();
    auto check = [](char it) { return it == '/'; };
    (void)fileName.erase(std::remove_if(fileName.begin(), fileName.end(), check), fileName.end());
    videoEnc_->SetOutPath(prefix + fileName);
    videoEnc_->SetIsHdrVivid(param == HW_HDR);
}

void TEST_SUIT::SetFormatWithParam(int32_t param)
{
    (void)param;
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH_VENC);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT_VENC);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_NV12);
}

INSTANTIATE_TEST_SUITE_P(, TEST_SUIT, testing::Values(HW_AVC, HW_HEVC));

/**
 * @tc.name: VideoEncoder_Multithread_Create_001
 * @tc.desc: try create 100 instances
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoEncoder_Multithread_Create_001, TestSize.Level1)
{
    SET_THREAD_NUM(100);
    g_vencCount = 0;
    GTEST_RUN_TASK(MultiThreadCreateVEnc);
    cout << "remaining num: " << g_vencCount.load() << endl;
}

/**
 * @tc.name: VideoEncoder_CreateWithNull_001
 * @tc.desc: video create
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoEncoder_CreateWithNull_001, TestSize.Level1)
{
    ASSERT_FALSE(CreateVideoCodecByName(""));
}

/**
 * @tc.name: VideoEncoder_CreateWithNull_002
 * @tc.desc: video create
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoEncoder_CreateWithNull_002, TestSize.Level1)
{
    ASSERT_FALSE(CreateVideoCodecByMime(""));
}

/**
 * @tc.name: VideoEncoder_Create_001
 * @tc.desc: video create
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoEncoder_Create_001, TestSize.Level1)
{
    ASSERT_TRUE(CreateVideoCodecByName(g_vencName));
}

/**
 * @tc.name: VideoEncoder_Create_002
 * @tc.desc: video create
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoEncoder_Create_002, TestSize.Level1)
{
    ASSERT_TRUE(CreateVideoCodecByMime((CodecMimeType::VIDEO_AVC).data()));
}

/**
 * @tc.name: VideoEncoder_Setcallback_001
 * @tc.desc: video setcallback
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoEncoder_Setcallback_001, TestSize.Level1)
{
    ASSERT_TRUE(videoEnc_->CreateVideoEncMockByName(g_vencName));
    ASSERT_EQ(AV_ERR_OK, videoEnc_->SetCallback(vencCallback_));
    ASSERT_EQ(AV_ERR_OK, videoEnc_->SetCallback(vencCallback_));
}

/**
 * @tc.name: VideoEncoder_Setcallback_002
 * @tc.desc: video setcallback
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoEncoder_Setcallback_002, TestSize.Level1)
{
    ASSERT_TRUE(videoEnc_->CreateVideoEncMockByName(g_vencName));
    ASSERT_EQ(AV_ERR_OK, videoEnc_->SetCallback(vencCallbackExt_));
    ASSERT_EQ(AV_ERR_OK, videoEnc_->SetCallback(vencCallbackExt_));
}

/**
 * @tc.name: VideoEncoder_Setcallback_003
 * @tc.desc: video setcallback
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoEncoder_Setcallback_003, TestSize.Level1)
{
    ASSERT_TRUE(videoEnc_->CreateVideoEncMockByName(g_vencName));
    ASSERT_EQ(AV_ERR_OK, videoEnc_->SetCallback(vencCallback_));
    ASSERT_NE(AV_ERR_OK, videoEnc_->SetCallback(vencCallbackExt_));
}

/**
 * @tc.name: VideoEncoder_Setcallback_004
 * @tc.desc: video setcallback
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoEncoder_Setcallback_004, TestSize.Level1)
{
    ASSERT_TRUE(videoEnc_->CreateVideoEncMockByName(g_vencName));
    ASSERT_EQ(AV_ERR_OK, videoEnc_->SetCallback(vencCallbackExt_));
    ASSERT_NE(AV_ERR_OK, videoEnc_->SetCallback(vencCallback_));
}

/**
 * @tc.name: VideoEncoder_SetParameterCallback_001
 * @tc.desc: video setcallback
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoEncoder_SetParameterCallback_001, TestSize.Level1)
{
    ASSERT_TRUE(videoEnc_->CreateVideoEncMockByName(g_vencName));
    ASSERT_EQ(AV_ERR_OK, videoEnc_->SetCallback(vencCallback_));
    ASSERT_EQ(AV_ERR_OK, videoEnc_->SetCallback(vencCallback_));
    ASSERT_EQ(AV_ERR_OK, videoEnc_->SetCallback(vencParamCallback_));
    ASSERT_EQ(AV_ERR_OK, videoEnc_->SetCallback(vencParamCallback_));
    ASSERT_EQ(AV_ERR_OK, videoEnc_->SetCallback(vencCallback_));
    ASSERT_EQ(AV_ERR_OK, videoEnc_->SetCallback(vencCallback_));
    ASSERT_NE(AV_ERR_OK, videoEnc_->SetCallback(vencCallbackExt_));
}

/**
 * @tc.name: VideoEncoder_SetParameterCallback_002
 * @tc.desc: video setcallback
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoEncoder_SetParameterCallback_002, TestSize.Level1)
{
    ASSERT_TRUE(videoEnc_->CreateVideoEncMockByName(g_vencName));
    ASSERT_EQ(AV_ERR_OK, videoEnc_->SetCallback(vencCallbackExt_));
    ASSERT_EQ(AV_ERR_OK, videoEnc_->SetCallback(vencCallbackExt_));
    ASSERT_EQ(AV_ERR_OK, videoEnc_->SetCallback(vencParamCallback_));
    ASSERT_EQ(AV_ERR_OK, videoEnc_->SetCallback(vencParamCallback_));
    ASSERT_EQ(AV_ERR_OK, videoEnc_->SetCallback(vencCallbackExt_));
    ASSERT_EQ(AV_ERR_OK, videoEnc_->SetCallback(vencCallbackExt_));
    ASSERT_NE(AV_ERR_OK, videoEnc_->SetCallback(vencCallback_));
}

/**
 * @tc.name: VideoEncoder_SetParameterCallback_003
 * @tc.desc: video setcallback
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoEncoder_SetParameterCallback_003, TestSize.Level1)
{
    ASSERT_TRUE(videoEnc_->CreateVideoEncMockByName(g_vencName));
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH_VENC);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT_VENC);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_NV12);

    ASSERT_EQ(AV_ERR_OK, videoEnc_->SetCallback(vencParamCallback_));
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    ASSERT_NE(AV_ERR_OK, videoEnc_->Start());
}

/**
 * @tc.name: VideoEncoder_SetParameterCallback_004
 * @tc.desc: video setcallback
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoEncoder_SetParameterCallback_004, TestSize.Level1)
{
    ASSERT_TRUE(videoEnc_->CreateVideoEncMockByName(g_vencName));
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH_VENC);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT_VENC);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_NV12);

    ASSERT_EQ(AV_ERR_OK, videoEnc_->SetCallback(vencParamCallback_));
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    ASSERT_NE(AV_ERR_OK, videoEnc_->Start());
}

#ifdef VIDEOENC_CAPI_UNIT_TEST
/**
 * @tc.name: VideoEncoder_Setcallback_Invalid_001
 * @tc.desc: video setcallback
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoEncoder_Setcallback_Invalid_001, TestSize.Level1)
{
    codec_ = OH_VideoEncoder_CreateByMime((CodecMimeType::VIDEO_AVC).data());
    ASSERT_NE(nullptr, codec_);
    struct OH_AVCodecCallback cb = GetVoidCallback();
    EXPECT_EQ(AV_ERR_INVALID_VAL, OH_VideoEncoder_RegisterCallback(nullptr, cb, nullptr));
}

/**
 * @tc.name: VideoEncoder_Setcallback_Invalid_002
 * @tc.desc: video setcallback
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoEncoder_Setcallback_Invalid_002, TestSize.Level1)
{
    codec_ = OH_VideoEncoder_CreateByMime((CodecMimeType::VIDEO_AVC).data());
    ASSERT_NE(nullptr, codec_);
    struct OH_AVCodecCallback cb = GetVoidCallback();
    cb.onNewOutputBuffer = nullptr;
    EXPECT_EQ(AV_ERR_INVALID_VAL, OH_VideoEncoder_RegisterCallback(codec_, cb, nullptr));
}

/**
 * @tc.name: VideoEncoder_Setcallback_Invalid_003
 * @tc.desc: video setcallback
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoEncoder_Setcallback_Invalid_003, TestSize.Level1)
{
    codec_ = OH_VideoEncoder_CreateByMime((CodecMimeType::VIDEO_AVC).data());
    ASSERT_NE(nullptr, codec_);
    struct OH_AVCodecCallback cb = GetVoidCallback();
    codec_->magic_ = AVMagic::AVCODEC_MAGIC_VIDEO_DECODER;
    EXPECT_EQ(AV_ERR_INVALID_VAL, OH_VideoEncoder_RegisterCallback(codec_, cb, nullptr));
    codec_->magic_ = AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER;
}

/**
 * @tc.name: VideoEncoder_PushInputBuffer_Invalid_001
 * @tc.desc: video push input buffer
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoEncoder_PushInputBuffer_Invalid_001, TestSize.Level1)
{
    codec_ = OH_VideoEncoder_CreateByMime((CodecMimeType::VIDEO_AVC).data());
    ASSERT_NE(nullptr, codec_);
    struct OH_AVCodecCallback cb = GetVoidCallback();
    OH_AVCodecBufferAttr attr = {0, 0, 0, 0};
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_RegisterCallback(codec_, cb, nullptr));
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_PushInputData(codec_, 0, attr));
}

/**
 * @tc.name: VideoEncoder_PushInputBuffer_Invalid_002
 * @tc.desc: video push input buffer
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoEncoder_PushInputBuffer_Invalid_002, TestSize.Level1)
{
    codec_ = OH_VideoEncoder_CreateByMime((CodecMimeType::VIDEO_AVC).data());
    ASSERT_NE(nullptr, codec_);
    struct OH_AVCodecAsyncCallback cb = GetVoidAsyncCallback();
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_SetCallback(codec_, cb, nullptr));
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_PushInputBuffer(codec_, 0));
}

/**
 * @tc.name: VideoEncoder_PushInputBuffer_Invalid_003
 * @tc.desc: video push input buffer
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoEncoder_PushInputBuffer_Invalid_003, TestSize.Level1)
{
    codec_ = OH_VideoEncoder_CreateByMime((CodecMimeType::VIDEO_AVC).data());
    ASSERT_NE(nullptr, codec_);
    EXPECT_EQ(AV_ERR_INVALID_VAL, OH_VideoEncoder_PushInputBuffer(nullptr, 0));
}

/**
 * @tc.name: VideoEncoder_PushInputBuffer_Invalid_004
 * @tc.desc: video push input buffer
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoEncoder_PushInputBuffer_Invalid_004, TestSize.Level1)
{
    codec_ = OH_VideoEncoder_CreateByMime((CodecMimeType::VIDEO_AVC).data());
    ASSERT_NE(nullptr, codec_);
    codec_->magic_ = AVMagic::AVCODEC_MAGIC_VIDEO_DECODER;
    EXPECT_EQ(AV_ERR_INVALID_VAL, OH_VideoEncoder_PushInputBuffer(codec_, 0));
    codec_->magic_ = AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER;
}

/**
 * @tc.name: VideoEncoder_Free_Buffer_Invalid_001
 * @tc.desc: video free buffer
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoEncoder_Free_Buffer_Invalid_001, TestSize.Level1)
{
    codec_ = OH_VideoEncoder_CreateByMime((CodecMimeType::VIDEO_AVC).data());
    ASSERT_NE(nullptr, codec_);
    struct OH_AVCodecCallback cb = GetVoidCallback();
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_RegisterCallback(codec_, cb, nullptr));
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_FreeOutputData(codec_, 0));
}

/**
 * @tc.name: VideoEncoder_Free_Buffer_Invalid_002
 * @tc.desc: video free buffer
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoEncoder_Free_Buffer_Invalid_002, TestSize.Level1)
{
    codec_ = OH_VideoEncoder_CreateByMime((CodecMimeType::VIDEO_AVC).data());
    ASSERT_NE(nullptr, codec_);
    struct OH_AVCodecAsyncCallback cb = GetVoidAsyncCallback();
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_SetCallback(codec_, cb, nullptr));
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_FreeOutputBuffer(codec_, 0));
}

/**
 * @tc.name: VideoEncoder_Free_Buffer_Invalid_003
 * @tc.desc: video free buffer
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoEncoder_Free_Buffer_Invalid_003, TestSize.Level1)
{
    codec_ = OH_VideoEncoder_CreateByMime((CodecMimeType::VIDEO_AVC).data());
    ASSERT_NE(nullptr, codec_);
    EXPECT_EQ(AV_ERR_INVALID_VAL, OH_VideoEncoder_FreeOutputBuffer(nullptr, 0));
}

/**
 * @tc.name: VideoEncoder_Free_Buffer_Invalid_004
 * @tc.desc: video free buffer
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoEncoder_Free_Buffer_Invalid_004, TestSize.Level1)
{
    codec_ = OH_VideoEncoder_CreateByMime((CodecMimeType::VIDEO_AVC).data());
    ASSERT_NE(nullptr, codec_);
    codec_->magic_ = AVMagic::AVCODEC_MAGIC_VIDEO_DECODER;
    EXPECT_EQ(AV_ERR_INVALID_VAL, OH_VideoEncoder_FreeOutputBuffer(codec_, 0));
    codec_->magic_ = AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER;
}
#endif

/**
 * @tc.name: VideoEncoder_Start_001
 * @tc.desc: correct flow 1
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_Start_001, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
}

/**
 * @tc.name: VideoEncoder_Start_002
 * @tc.desc: correct flow 2
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_Start_002, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
}

/**
 * @tc.name: VideoEncoder_Start_003
 * @tc.desc: correct flow 2
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_Start_003, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Flush());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
}

/**
 * @tc.name: VideoEncoder_Start_004
 * @tc.desc: wrong flow 1
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_Start_004, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_NE(AV_ERR_OK, videoEnc_->Start());
}

/**
 * @tc.name: VideoEncoder_Start_005
 * @tc.desc: wrong flow 2
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_Start_005, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    PrepareSource(GetParam());
    EXPECT_NE(AV_ERR_OK, videoEnc_->Start());
}

/**
 * @tc.name: VideoEncoder_Start_Buffer_001
 * @tc.desc: correct flow 1
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_Start_Buffer_001, TestSize.Level1)
{
    isAVBufferMode_ = true;
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
}

/**
 * @tc.name: VideoEncoder_Start_Buffer_002
 * @tc.desc: correct flow 2
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_Start_Buffer_002, TestSize.Level1)
{
    isAVBufferMode_ = true;
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
}

/**
 * @tc.name: VideoEncoder_Start_Buffer_003
 * @tc.desc: correct flow 2
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_Start_Buffer_003, TestSize.Level1)
{
    isAVBufferMode_ = true;
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Flush());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
}

/**
 * @tc.name: VideoEncoder_Start_Buffer_004
 * @tc.desc: wrong flow 2
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_Start_Buffer_004, TestSize.Level1)
{
    isAVBufferMode_ = true;
    CreateByNameWithParam(GetParam());
    PrepareSource(GetParam());
    EXPECT_NE(AV_ERR_OK, videoEnc_->Start());
}

/**
 * @tc.name: VideoEncoder_Stop_001
 * @tc.desc: correct flow 1
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_Stop_001, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_Stop_002
 * @tc.desc: correct flow 1
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_Stop_002, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Flush());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_Stop_003
 * @tc.desc: wrong flow 1
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_Stop_003, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));

    EXPECT_NE(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_Flush_001
 * @tc.desc: correct flow 1
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_Flush_001, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Flush());
}

/**
 * @tc.name: VideoEncoder_Flush_002
 * @tc.desc: wrong flow 1
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_Flush_002, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Release());
    EXPECT_NE(AV_ERR_OK, videoEnc_->Flush());
}

/**
 * @tc.name: VideoEncoder_Reset_001
 * @tc.desc: correct flow 1
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_Reset_001, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));

    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Reset());
}

/**
 * @tc.name: VideoEncoder_Reset_002
 * @tc.desc: correct flow 2
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_Reset_002, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));

    EXPECT_EQ(AV_ERR_OK, videoEnc_->Reset());
}

/**
 * @tc.name: VideoEncoder_Reset_003
 * @tc.desc: correct flow 3
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_Reset_003, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));

    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Reset());
}

/**
 * @tc.name: VideoEncoder_Release_001
 * @tc.desc: correct flow 1
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_Release_001, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));

    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Release());
}

/**
 * @tc.name: VideoEncoder_Release_002
 * @tc.desc: correct flow 2
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_Release_002, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));

    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Release());
}

/**
 * @tc.name: VideoEncoder_Release_003
 * @tc.desc: correct flow 3
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_Release_003, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));

    EXPECT_EQ(AV_ERR_OK, videoEnc_->Release());
}

/**
 * @tc.name: VideoEncoder_PushParameter_001
 * @tc.desc: video encodec SetSurface
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_PushParameter_001, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    ASSERT_NE(AV_ERR_OK, videoEnc_->SetCallback(vencParamCallback_));
}

/**
 * @tc.name: VideoEncoder_SetSurface_001
 * @tc.desc: video encodec SetSurface
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_SetSurface_001, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    ASSERT_EQ(AV_ERR_OK, videoEnc_->CreateInputSurface());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_SetSurface_002
 * @tc.desc: wrong flow 1
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_SetSurface_002, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_NE(AV_ERR_OK, videoEnc_->CreateInputSurface());
}

/**
 * @tc.name: VideoEncoder_SetSurface_003
 * @tc.desc: wrong flow 2
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_SetSurface_003, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    ASSERT_NE(AV_ERR_OK, videoEnc_->CreateInputSurface());
}

/**
 * @tc.name: VideoEncoder_SetSurface_Buffer_001
 * @tc.desc: video encodec SetSurface
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_SetSurface_Buffer_001, TestSize.Level1)
{
    isAVBufferMode_ = true;
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    ASSERT_EQ(AV_ERR_OK, videoEnc_->CreateInputSurface());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_SetSurface_Buffer_002
 * @tc.desc: wrong flow 2
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_SetSurface_Buffer_002, TestSize.Level1)
{
    isAVBufferMode_ = true;
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));

    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    ASSERT_NE(AV_ERR_OK, videoEnc_->CreateInputSurface());
}

/**
 * @tc.name: VideoEncoder_Abnormal_001
 * @tc.desc: video codec abnormal func
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_Abnormal_001, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_ROTATION_ANGLE, 20); // invalid rotation_angle 20
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_MAX_INPUT_SIZE, -1); // invalid max input size -1

    videoEnc_->Configure(format_);
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Reset());
    EXPECT_NE(AV_ERR_OK, videoEnc_->Start());
    EXPECT_NE(AV_ERR_OK, videoEnc_->Flush());
    EXPECT_NE(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_Abnormal_002
 * @tc.desc: video codec abnormal func
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_Abnormal_002, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    PrepareSource(GetParam());
    EXPECT_NE(AV_ERR_OK, videoEnc_->Start());
}

/**
 * @tc.name: VideoEncoder_Abnormal_003
 * @tc.desc: video codec abnormal func
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_Abnormal_003, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    PrepareSource(GetParam());
    EXPECT_NE(AV_ERR_OK, videoEnc_->Flush());
}

/**
 * @tc.name: VideoEncoder_Abnormal_004
 * @tc.desc: video codec abnormal func
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_Abnormal_004, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    PrepareSource(GetParam());
    EXPECT_NE(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_SetParameter_001
 * @tc.desc: video codec SetParameter
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_SetParameter_001, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));

    format_ = FormatMockFactory::CreateFormat();
    ASSERT_NE(nullptr, format_);

    format_->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH_VENC);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT_VENC);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::YUV420P));
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, DEFAULT_FRAME_RATE);

    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->SetParameter(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_SetParameter_002
 * @tc.desc: video codec SetParameter
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_SetParameter_002, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));

    format_ = FormatMockFactory::CreateFormat();
    ASSERT_NE(nullptr, format_);

    format_->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, -2);  // invalid width size -2
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, -2); // invalid height size -2
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::YUV420P));
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, DEFAULT_FRAME_RATE);

    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->SetParameter(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_GetOutputDescription_001
 * @tc.desc: video codec GetOutputDescription
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_GetOutputDescription_001, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));

    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    format_ = videoEnc_->GetOutputDescription();
    EXPECT_NE(nullptr, format_);
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_HDR_Function_001
 * @tc.desc: video encodec SetSurface
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoEncoder_HDR_Function_001, TestSize.Level1)
{
    capability_ = CodecListMockFactory::GetCapabilityByCategory(CodecMimeType::VIDEO_HEVC.data(), true,
                                                                AVCodecCategory::AVCODEC_HARDWARE);
    std::string codecName = capability_->GetName();
    std::cout << "CodecName: " << codecName << "\n";
    ASSERT_TRUE(CreateVideoCodecByName(codecName));
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH_VENC);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT_VENC);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_NV12);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_PROFILE, static_cast<int32_t>(HEVCProfile::HEVC_PROFILE_MAIN_10));
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_I_FRAME_INTERVAL, 100); // 100: i frame interval

    const ::testing::TestInfo *testInfo_ = ::testing::UnitTest::GetInstance()->current_test_info();
    string prefix = "/data/test/media/";
    string fileName = testInfo_->name();
    auto check = [](char it) { return it == '/'; };
    (void)fileName.erase(std::remove_if(fileName.begin(), fileName.end(), check), fileName.end());
    videoEnc_->SetOutPath(prefix + fileName);
    videoEnc_->SetIsHdrVivid(true);

    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    ASSERT_EQ(AV_ERR_OK, videoEnc_->CreateInputSurface());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

#ifdef HMOS_TEST
/**
 * @tc.name: VideoEncoder_TemporalScalability_001
 * @tc.desc: unable temporal scalability encode, buffer mode
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_TemporalScalability_001, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_TEMPORAL_SCALABILITY, 0);
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_TemporalScalability_002
 * @tc.desc: unable temporal scalability encode, but set temporal gop parameter, buffer mode
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_TemporalScalability_002, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_TEMPORAL_GOP_SIZE, -1);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_TEMPORAL_GOP_REFERENCE_MODE, 3);
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_TemporalScalability_003
 * @tc.desc: enable temporal scalability encode, adjacent reference mode, buffer mode
 * expect level stream
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_TemporalScalability_003, TestSize.Level1)
{
    isAVBufferMode_ = true;
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_TEMPORAL_SCALABILITY, 2);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_TEMPORAL_GOP_SIZE, 2);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_TEMPORAL_GOP_REFERENCE_MODE,
                         static_cast<int32_t>(OH_TemporalGopReferenceMode::ADJACENT_REFERENCE));
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_TemporalScalability_004
 * @tc.desc: enable temporal scalability encode, jump reference mode, buffer mode
 * expect level stream
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_TemporalScalability_004, TestSize.Level1)
{
    isAVBufferMode_ = true;
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_TEMPORAL_SCALABILITY, 1);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_TEMPORAL_GOP_SIZE, 2);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_TEMPORAL_GOP_REFERENCE_MODE,
                         static_cast<int32_t>(OH_TemporalGopReferenceMode::JUMP_REFERENCE));
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_TemporalScalability_005
 * @tc.desc: set invalid temporal gop size 1
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_TemporalScalability_005, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_TEMPORAL_SCALABILITY, 1);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_TEMPORAL_GOP_SIZE, 1);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_TEMPORAL_GOP_REFERENCE_MODE,
                         static_cast<int32_t>(OH_TemporalGopReferenceMode::ADJACENT_REFERENCE));
    EXPECT_NE(AV_ERR_OK, videoEnc_->Configure(format_));
}

/**
 * @tc.name: VideoEncoder_TemporalScalability_006
 * @tc.desc: set invalid temporal gop size: gop size.(default gop size 60)
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_TemporalScalability_006, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_TEMPORAL_SCALABILITY, 1);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_TEMPORAL_GOP_SIZE, 60);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_TEMPORAL_GOP_REFERENCE_MODE, 1);
    EXPECT_NE(AV_ERR_OK, videoEnc_->Configure(format_));
}

/**
 * @tc.name: VideoEncoder_TemporalScalability_007
 * @tc.desc: set invalid temporal reference mode: 3
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_TemporalScalability_007, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_TEMPORAL_SCALABILITY, 1);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_TEMPORAL_GOP_SIZE, 4);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_TEMPORAL_GOP_REFERENCE_MODE, 3);
    EXPECT_NE(AV_ERR_OK, videoEnc_->Configure(format_));
}

/**
 * @tc.name: VideoEncoder_TemporalScalability_008
 * @tc.desc: set unsupport gop size: 2
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_TemporalScalability_008, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutDoubleValue(Media::Tag::VIDEO_FRAME_RATE, 2.0);
    format_->PutIntValue(Media::Tag::VIDEO_I_FRAME_INTERVAL, 1000);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_TEMPORAL_SCALABILITY, 1);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_TEMPORAL_GOP_SIZE, 4);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_TEMPORAL_GOP_REFERENCE_MODE,
                         static_cast<int32_t>(OH_TemporalGopReferenceMode::ADJACENT_REFERENCE));
    EXPECT_NE(AV_ERR_OK, videoEnc_->Configure(format_));
}

/**
 * @tc.name: VideoEncoder_TemporalScalability_009
 * @tc.desc: set int framerate and enalbe temporal scalability encode, use default framerate 30.0
 * expect level stream
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_TemporalScalability_009, TestSize.Level1)
{
    isAVBufferMode_ = true;
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_FRAME_RATE, 25);
    format_->PutIntValue(Media::Tag::VIDEO_I_FRAME_INTERVAL, 2000);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_TEMPORAL_SCALABILITY, 1);
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_TemporalScalability_010
 * @tc.desc: set invalid framerate 0.0 and enalbe temporal scalability encode, configure fail
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_TemporalScalability_010, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutDoubleValue(Media::Tag::VIDEO_FRAME_RATE, 0.0);
    format_->PutIntValue(Media::Tag::VIDEO_I_FRAME_INTERVAL, 2000);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_TEMPORAL_SCALABILITY, 1);
    EXPECT_NE(AV_ERR_OK, videoEnc_->Configure(format_));
}

/**
 * @tc.name: VideoEncoder_TemporalScalability_011
 * @tc.desc: gopsize 3 and enalbe temporal scalability encode
 * expect level stream
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_TemporalScalability_011, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutDoubleValue(Media::Tag::VIDEO_FRAME_RATE, 3.0);
    format_->PutIntValue(Media::Tag::VIDEO_I_FRAME_INTERVAL, 1000);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_TEMPORAL_SCALABILITY, 1);
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    ASSERT_EQ(AV_ERR_OK, videoEnc_->CreateInputSurface());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_TemporalScalability_012
 * @tc.desc: set i frame interval 0 and enalbe temporal scalability encode, configure fail
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_TemporalScalability_012, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutDoubleValue(Media::Tag::VIDEO_FRAME_RATE, 60.0);
    format_->PutIntValue(Media::Tag::VIDEO_I_FRAME_INTERVAL, 0);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_TEMPORAL_SCALABILITY, 1);
    EXPECT_NE(AV_ERR_OK, videoEnc_->Configure(format_));
}

/**
 * @tc.name: VideoEncoder_TemporalScalability_013
 * @tc.desc: set i frame interval -1 and enalbe temporal scalability encode
 * expect level stream only one idr frame
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_TemporalScalability_013, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_I_FRAME_INTERVAL, -1);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_TEMPORAL_SCALABILITY, 1);
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    ASSERT_EQ(AV_ERR_OK, videoEnc_->CreateInputSurface());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_TemporalScalability_014
 * @tc.desc: enable temporal scalability encode on surface mode without set parametercallback
 * expect level stream
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_TemporalScalability_014, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_TEMPORAL_SCALABILITY, 1);
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    ASSERT_EQ(AV_ERR_OK, videoEnc_->CreateInputSurface());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_TemporalScalability_015
 * @tc.desc: enable temporal scalability encode on surface mode with set parametercallback
 * expect level stream
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_TemporalScalability_015, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoEnc_->SetCallback(vencParamCallback_));
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_TEMPORAL_SCALABILITY, 1);
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    ASSERT_EQ(AV_ERR_OK, videoEnc_->CreateInputSurface());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_TemporalScalability_016
 * @tc.desc: enable temporal scalability encode on buffer mode and request i frame at 13th frame
 * expect level stream
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_TemporalScalability_016, TestSize.Level1)
{
    isAVBufferMode_ = true;
    isTemporalScalabilitySyncIdr_ = true;
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_TEMPORAL_SCALABILITY, 1);
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_TemporalScalability_017
 * @tc.desc: enable temporal scalability encode on surface mode and request i frame at 13th frame
 * expect level stream
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_TemporalScalability_017, TestSize.Level1)
{
    isTemporalScalabilitySyncIdr_ = true;
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoEnc_->SetCallback(vencParamCallback_));
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_TEMPORAL_SCALABILITY, 1);
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    ASSERT_EQ(AV_ERR_OK, videoEnc_->CreateInputSurface());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}
#endif
} // namespace