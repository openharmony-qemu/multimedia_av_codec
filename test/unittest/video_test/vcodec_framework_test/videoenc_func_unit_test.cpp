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
#include "unittest_utils.h"
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
using namespace OHOS::Media;

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
    bool ReadCustomDataToAVBuffer(const std::string &fileName, std::shared_ptr<AVBuffer> buffer);
    bool GetWaterMarkCapability(int32_t param);

protected:
    std::shared_ptr<CodecListMock> capability_ = nullptr;
    std::shared_ptr<VideoEncSample> videoEnc_ = nullptr;
    std::shared_ptr<FormatMock> format_ = nullptr;
    std::shared_ptr<VEncCallbackTest> vencCallback_ = nullptr;
    std::shared_ptr<VEncCallbackTestExt> vencCallbackExt_ = nullptr;
    std::shared_ptr<VEncParamCallbackTest> vencParamCallback_ = nullptr;
    std::shared_ptr<VEncParamWithAttrCallbackTest> vencParamWithAttrCallback_ = nullptr;
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

    vencParamWithAttrCallback_ = std::make_shared<VEncParamWithAttrCallbackTest>(vencSignal);
    ASSERT_NE(nullptr, vencParamWithAttrCallback_);

    videoEnc_ = std::make_shared<VideoEncSample>(vencSignal);
    ASSERT_NE(nullptr, videoEnc_);

    format_ = FormatMockFactory::CreateFormat();
    ASSERT_NE(nullptr, format_);
}

void TEST_SUIT::TearDown(void)
{
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
    if (videoEnc_->isAVBufferMode_) {
        if (videoEnc_->CreateVideoEncMockByName(name) == false ||
            videoEnc_->SetCallback(vencCallbackExt_) != AV_ERR_OK) {
            return false;
        }
    } else {
        if (videoEnc_->CreateVideoEncMockByName(name) == false || videoEnc_->SetCallback(vencCallback_) != AV_ERR_OK) {
            return false;
        }
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
}

void TEST_SUIT::SetFormatWithParam(int32_t param)
{
    (void)param;
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH_VENC);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT_VENC);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_NV12);
}

bool TEST_SUIT::ReadCustomDataToAVBuffer(const std::string &fileName, std::shared_ptr<AVBuffer> buffer)
{
    std::unique_ptr<std::ifstream> inFile = std::make_unique<std::ifstream>();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(inFile != nullptr, false, "inFile is nullptr");
    inFile->open(fileName.c_str(), std::ios::in | std::ios::binary);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(inFile->is_open(), false, "open file filed, fileName:%s.", fileName.c_str());
    sptr<SurfaceBuffer> surfaceBuffer = buffer->memory_->GetSurfaceBuffer();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(surfaceBuffer != nullptr, false, "surfaceBuffer is nullptr");
    int32_t width = surfaceBuffer->GetWidth();
    int32_t height = surfaceBuffer->GetHeight();
    int32_t bufferSize = width * height * 4;
    uint8_t *in = (uint8_t *)malloc(bufferSize);
    UNITTEST_CHECK_AND_RETURN_RET_LOG(in != nullptr, false, "in is nullptr");
    inFile->read(reinterpret_cast<char *>(in), bufferSize);
    // read data
    int32_t dstWidthStride = surfaceBuffer->GetStride();
    uint8_t *dstAddr = (uint8_t *)surfaceBuffer->GetVirAddr();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(dstAddr != nullptr, false, "dst is nullptr");
    const int32_t srcWidthStride = width << 2;
    uint8_t *inStream = in;
    for (uint32_t i = 0; i < height; ++i) {
        memcpy_s(dstAddr, dstWidthStride, inStream, srcWidthStride);
        dstAddr += dstWidthStride;
        inStream += srcWidthStride;
    }
    inFile->close();
    if (in) {
        free(in);
        in = nullptr;
    }
    return true;
}

bool TEST_SUIT::GetWaterMarkCapability(int32_t param)
{
    std::string codecName = "";
    std::shared_ptr<AVCodecList> codecCapability = AVCodecListFactory::CreateAVCodecList();
    CapabilityData *capabilityData = nullptr;
    switch (param) {
        case VCodecTestCode::HW_AVC:
            capabilityData = codecCapability->GetCapability(CodecMimeType::VIDEO_AVC.data(), true,
                                                            AVCodecCategory::AVCODEC_HARDWARE);
            break;
        case VCodecTestCode::HW_HEVC:
            capabilityData = codecCapability->GetCapability(CodecMimeType::VIDEO_HEVC.data(), true,
                                                            AVCodecCategory::AVCODEC_HARDWARE);
            break;
        default:
            capabilityData = codecCapability->GetCapability(CodecMimeType::VIDEO_AVC.data(), true,
                                                            AVCodecCategory::AVCODEC_SOFTWARE);
            break;
        }
    if (capabilityData == nullptr) {
        std::cout << "capabilityData is nullptr" << std::endl;
        return false;
    }
    if (capabilityData->featuresMap.count(static_cast<int32_t>(AVCapabilityFeature::VIDEO_WATERMARK))) {
        std::cout << "Support watermark" << std::endl;
        return true;
    } else {
        std::cout << " Not support watermark" << std::endl;
        return false;
    }
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
 * @tc.name: VideoEncoder_Invalid_SetParameterCallback_001
 * @tc.desc: video setcallback
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoEncoder_Invalid_SetParameterCallback_001, TestSize.Level1)
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
 * @tc.name: VideoEncoder_Invalid_SetParameterCallback_002
 * @tc.desc: video setcallback
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoEncoder_Invalid_SetParameterCallback_002, TestSize.Level1)
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
 * @tc.name: VideoEncoder_Invalid_SetParameterCallback_003
 * @tc.desc: video setcallback
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoEncoder_Invalid_SetParameterCallback_003, TestSize.Level1)
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
#else

/**
 * @tc.name: VideoEncoder_SetParameterWithAttrCallback_001
 * @tc.desc: SetParameterWithAttrCallback and check if meta has pts key-value
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoEncoder_SetParameterWithAttrCallback_001, TestSize.Level1)
{
    videoEnc_->isDiscardFrame_ = true;
    CreateByNameWithParam(VCodecTestCode::HW_AVC);
    SetFormatWithParam(VCodecTestCode::HW_AVC);
    PrepareSource(VCodecTestCode::HW_AVC);
    ASSERT_EQ(AV_ERR_OK, videoEnc_->SetCallback(vencParamWithAttrCallback_));
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    ASSERT_EQ(AV_ERR_OK, videoEnc_->CreateInputSurface());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_Invalid_SetParameterWithAttrCallback_001
 * @tc.desc: repeat SetParameterWithAttrCallback and test the compatibility of API9 and API10
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoEncoder_Invalid_SetParameterWithAttrCallback_001, TestSize.Level1)
{
    ASSERT_TRUE(videoEnc_->CreateVideoEncMockByName(g_vencName));
    ASSERT_EQ(AV_ERR_OK, videoEnc_->SetCallback(vencCallback_));
    ASSERT_EQ(AV_ERR_OK, videoEnc_->SetCallback(vencParamWithAttrCallback_));
    ASSERT_EQ(AV_ERR_OK, videoEnc_->SetCallback(vencParamWithAttrCallback_));
    ASSERT_EQ(AV_ERR_OK, videoEnc_->SetCallback(vencCallback_));
    ASSERT_NE(AV_ERR_OK, videoEnc_->SetCallback(vencCallbackExt_));
}

/**
 * @tc.name: VideoEncoder_Invalid_SetParameterWithAttrCallback_002
 * @tc.desc: repeat SetParameterWithAttrCallback and test the compatibility of API10 and API9
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoEncoder_Invalid_SetParameterWithAttrCallback_002, TestSize.Level1)
{
    ASSERT_TRUE(videoEnc_->CreateVideoEncMockByName(g_vencName));
    ASSERT_EQ(AV_ERR_OK, videoEnc_->SetCallback(vencCallbackExt_));
    ASSERT_EQ(AV_ERR_OK, videoEnc_->SetCallback(vencParamWithAttrCallback_));
    ASSERT_EQ(AV_ERR_OK, videoEnc_->SetCallback(vencParamWithAttrCallback_));
    ASSERT_EQ(AV_ERR_OK, videoEnc_->SetCallback(vencCallbackExt_));
    ASSERT_NE(AV_ERR_OK, videoEnc_->SetCallback(vencCallback_));
}

/**
 * @tc.name: VideoEncoder_Invalid_SetParameterWithAttrCallback_003
 * @tc.desc: start failed with SetParameterWithAttrCallback and not set surface
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoEncoder_Invalid_SetParameterWithAttrCallback_003, TestSize.Level1)
{
    ASSERT_TRUE(videoEnc_->CreateVideoEncMockByName(g_vencName));
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH_VENC);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT_VENC);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_NV12);

    ASSERT_EQ(AV_ERR_OK, videoEnc_->SetCallback(vencParamWithAttrCallback_));
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    ASSERT_NE(AV_ERR_OK, videoEnc_->Start());
}

/**
 * @tc.name: VideoEncoder_Invalid_SetParameterWithAttrCallback_004
 * @tc.desc: set vencParamWithAttrCallback_ success and set vencParamCallback_ failed
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoEncoder_Invalid_SetParameterWithAttrCallback_004, TestSize.Level1)
{
    ASSERT_TRUE(videoEnc_->CreateVideoEncMockByName(g_vencName));
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH_VENC);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT_VENC);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_NV12);

    ASSERT_EQ(AV_ERR_OK, videoEnc_->SetCallback(vencParamWithAttrCallback_));
    EXPECT_NE(AV_ERR_OK, videoEnc_->SetCallback(vencParamCallback_));
}

/**
 * @tc.name: VideoEncoder_RepeatPreviousFrame_001
 * @tc.desc: key repeat previous frame is invalid
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_RepeatPreviousFrame_001, TestSize.Level1)
{
    constexpr int32_t repeatPreviousFrame = 0;
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_REPEAT_PREVIOUS_FRAME_AFTER, repeatPreviousFrame);
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, videoEnc_->Configure(format_));
}

/**
 * @tc.name: VideoEncoder_RepeatPreviousFrame_002
 * @tc.desc: key repeat previous frame is invalid
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_RepeatPreviousFrame_002, TestSize.Level1)
{
    constexpr int32_t repeatPreviousFrame = -1;
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_REPEAT_PREVIOUS_FRAME_AFTER, repeatPreviousFrame);
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, videoEnc_->Configure(format_));
}

/**
 * @tc.name: VideoEncoder_RepeatPreviousFrame_003
 * @tc.desc: key repeat previous frame is invalid
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_RepeatPreviousFrame_003, TestSize.Level1)
{
    constexpr int32_t repeatPreviousFrame = INT32_MIN;
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_REPEAT_PREVIOUS_FRAME_AFTER, repeatPreviousFrame);
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, videoEnc_->Configure(format_));
}

/**
 * @tc.name: VideoEncoder_RepeatPreviousFrame_004
 * @tc.desc: key repeat previous frame is invalid
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_RepeatPreviousFrame_004, TestSize.Level1)
{
    constexpr int32_t repeatPreviousFrame = 30;
    constexpr int32_t repeatPreviousFrameMaxCount = 0;
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_REPEAT_PREVIOUS_FRAME_AFTER, repeatPreviousFrame);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_REPEAT_PREVIOUS_MAX_COUNT, repeatPreviousFrameMaxCount);
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, videoEnc_->Configure(format_));
}

/**
 * @tc.name: VideoEncoder_RepeatPreviousFrame_005
 * @tc.desc: repeat the previous frame all the time
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_RepeatPreviousFrame_005, TestSize.Level1)
{
    constexpr int32_t repeatPreviousFrame = 30;
    constexpr int32_t repeatPreviousFrameMaxCount = -1;
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_REPEAT_PREVIOUS_FRAME_AFTER, repeatPreviousFrame);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_REPEAT_PREVIOUS_MAX_COUNT, repeatPreviousFrameMaxCount);
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->Configure(format_));
}

/**
 * @tc.name: VideoEncoder_RepeatPreviousFrame_006
 * @tc.desc: repeat the previous frame all the time
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_RepeatPreviousFrame_006, TestSize.Level1)
{
    constexpr int32_t repeatPreviousFrame = 30;
    constexpr int32_t repeatPreviousFrameMaxCount = INT32_MIN;
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_REPEAT_PREVIOUS_FRAME_AFTER, repeatPreviousFrame);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_REPEAT_PREVIOUS_MAX_COUNT, repeatPreviousFrameMaxCount);
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->Configure(format_));
}

/**
 * @tc.name: VideoEncoder_RepeatPreviousFrame_007
 * @tc.desc: repeat the previous frame one times
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_RepeatPreviousFrame_007, TestSize.Level1)
{
    constexpr int32_t repeatPreviousFrame = 30;
    constexpr int32_t repeatPreviousFrameMaxCount = 1;
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_REPEAT_PREVIOUS_FRAME_AFTER, repeatPreviousFrame);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_REPEAT_PREVIOUS_MAX_COUNT, repeatPreviousFrameMaxCount);
    videoEnc_->needSleep_ = true;
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->Configure(format_));
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->CreateInputSurface());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    int32_t frameOutputCountMin = (videoEnc_->frameInputCount_ - 1) * 2 - 7;
    int32_t frameOutputCountMax = (videoEnc_->frameInputCount_ - 1) * 2 + 7;
    EXPECT_LE(videoEnc_->frameOutputCount_, frameOutputCountMax);
    EXPECT_GE(videoEnc_->frameOutputCount_, frameOutputCountMin);
}

/**
 * @tc.name: VideoEncoder_RepeatPreviousFrame_008
 * @tc.desc: repeat the previous frame two times
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_RepeatPreviousFrame_008, TestSize.Level1)
{
    constexpr int32_t repeatPreviousFrame = 20;
    constexpr int32_t repeatPreviousFrameMaxCount = 2;
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_REPEAT_PREVIOUS_FRAME_AFTER, repeatPreviousFrame);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_REPEAT_PREVIOUS_MAX_COUNT, repeatPreviousFrameMaxCount);
    videoEnc_->needSleep_ = true;
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->Configure(format_));
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->CreateInputSurface());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    int32_t frameOutputCountMin = (videoEnc_->frameInputCount_ - 1) * 3 - 7;
    int32_t frameOutputCountMax = (videoEnc_->frameInputCount_ - 1) * 3 + 7;
    EXPECT_LE(videoEnc_->frameOutputCount_, frameOutputCountMax);
    EXPECT_GE(videoEnc_->frameOutputCount_, frameOutputCountMin);
}

/**
 * @tc.name: VideoEncoder_RepeatPreviousFrame_009
 * @tc.desc: repeat the previous frame three times
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_RepeatPreviousFrame_009, TestSize.Level1)
{
    constexpr int32_t repeatPreviousFrame = 10;
    constexpr int32_t repeatPreviousFrameMaxCount = 3;
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_REPEAT_PREVIOUS_FRAME_AFTER, repeatPreviousFrame);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_REPEAT_PREVIOUS_MAX_COUNT, repeatPreviousFrameMaxCount);
    videoEnc_->needSleep_ = true;
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->Configure(format_));
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->CreateInputSurface());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    int32_t frameOutputCountMin = (videoEnc_->frameInputCount_ - 1) * 4 - 7;
    int32_t frameOutputCountMax = (videoEnc_->frameInputCount_ - 1) * 4 + 7;
    EXPECT_LE(videoEnc_->frameOutputCount_, frameOutputCountMax);
    EXPECT_GE(videoEnc_->frameOutputCount_, frameOutputCountMin);
}

#ifdef HMOS_TEST
/**
 * @tc.name: VideoEncoder_RepeatPreviousFrame_010
 * @tc.desc: repeat the previous frame two times
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_RepeatPreviousFrame_010, TestSize.Level1)
{
    constexpr int32_t repeatPreviousFrame = 20;
    constexpr int32_t repeatPreviousFrameMaxCount = 10;
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_REPEAT_PREVIOUS_FRAME_AFTER, repeatPreviousFrame);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_REPEAT_PREVIOUS_MAX_COUNT, repeatPreviousFrameMaxCount);
    videoEnc_->needSleep_ = true;
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->Configure(format_));
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->CreateInputSurface());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    int32_t frameOutputCountMin = (videoEnc_->frameInputCount_ - 1) * 3 - 27;
    int32_t frameOutputCountMax = (videoEnc_->frameInputCount_ - 1) * 3 + 27;
    EXPECT_LE(videoEnc_->frameOutputCount_, frameOutputCountMax);
    EXPECT_GE(videoEnc_->frameOutputCount_, frameOutputCountMin);
}

/**
 * @tc.name: VideoEncoder_RepeatPreviousFrame_011
 * @tc.desc: repeat the previous frame two times
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_RepeatPreviousFrame_011, TestSize.Level1)
{
    constexpr int32_t repeatPreviousFrame = 20;
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_REPEAT_PREVIOUS_FRAME_AFTER, repeatPreviousFrame);
    videoEnc_->needSleep_ = true;
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->Configure(format_));
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->CreateInputSurface());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    int32_t frameOutputCountMin = (videoEnc_->frameInputCount_ - 1) * 3 - 27;
    int32_t frameOutputCountMax = (videoEnc_->frameInputCount_ - 1) * 3 + 27;
    EXPECT_LE(videoEnc_->frameOutputCount_, frameOutputCountMax);
    EXPECT_GE(videoEnc_->frameOutputCount_, frameOutputCountMin);
}
#endif // HMOS_TEST
/**
 * @tc.name: VideoEncoder_SetCustomBuffer_001
 * @tc.desc: encoder with water mark, buffer mode
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_SetCustomBuffer_001, TestSize.Level1)
{
    if (!GetWaterMarkCapability(GetParam())) {
        return;
    };
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    BufferRequestConfig bufferConfig = {
        .width = 300,
        .height = 300,
        .strideAlignment = 0x8,
        .format = GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_8888,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
        .timeout = 0,
    };
    auto allocator = AVAllocatorFactory::CreateSurfaceAllocator(bufferConfig);
    std::shared_ptr<AVBuffer> avbuffer = AVBuffer::CreateAVBuffer(allocator);
    bool ret = ReadCustomDataToAVBuffer("/data/test/media/test.rgba", avbuffer);
    ASSERT_EQ(ret, true);
    std::shared_ptr<AVBufferMock> buffer = AVBufferMockFactory::CreateAVBuffer(avbuffer);
    std::shared_ptr<FormatMock> param = buffer->GetParameter();
    param->PutIntValue(Tag::VIDEO_ENCODER_ENABLE_WATERMARK, 1);
    param->PutIntValue(Tag::VIDEO_COORDINATE_X, 100);
    param->PutIntValue(Tag::VIDEO_COORDINATE_Y, 100);
    param->PutIntValue(Tag::VIDEO_COORDINATE_W, bufferConfig.width);
    param->PutIntValue(Tag::VIDEO_COORDINATE_H, bufferConfig.height);
    buffer->SetParameter(param);
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->Configure(format_));
    ASSERT_EQ(AV_ERR_OK, videoEnc_->SetCustomBuffer(buffer));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
}

/**
 * @tc.name: VideoEncoder_SetCustomBuffer_002
 * @tc.desc: x corrdinate is error
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_SetCustomBuffer_002, TestSize.Level1)
{
    if (!GetWaterMarkCapability(GetParam())) {
        return;
    };
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    auto allocator = AVAllocatorFactory::CreateSurfaceAllocator(DEFAULT_CONFIG);
    std::shared_ptr<AVBuffer> avbuffer = AVBuffer::CreateAVBuffer(allocator);
    std::shared_ptr<AVBufferMock> buffer = AVBufferMockFactory::CreateAVBuffer(avbuffer);
    std::shared_ptr<FormatMock> param = buffer->GetParameter();
    (void)memset_s(buffer->GetAddr(), buffer->GetCapacity(), 0, 10000);
    param->PutIntValue(Tag::VIDEO_ENCODER_ENABLE_WATERMARK, 1);
    param->PutIntValue(Tag::VIDEO_COORDINATE_X, -1);
    param->PutIntValue(Tag::VIDEO_COORDINATE_Y, 11);
    param->PutIntValue(Tag::VIDEO_COORDINATE_W, DEFAULT_CONFIG.width);
    param->PutIntValue(Tag::VIDEO_COORDINATE_H, DEFAULT_CONFIG.height);
    buffer->SetParameter(param);
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->Configure(format_));
    ASSERT_NE(AV_ERR_OK, videoEnc_->SetCustomBuffer(buffer));
}

/**
 * @tc.name: VideoEncoder_SetCustomBuffer_003
 * @tc.desc: w is not set
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_SetCustomBuffer_003, TestSize.Level1)
{
    if (!GetWaterMarkCapability(GetParam())) {
        return;
    };
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    auto allocator = AVAllocatorFactory::CreateSurfaceAllocator(DEFAULT_CONFIG);
    std::shared_ptr<AVBuffer> avbuffer = AVBuffer::CreateAVBuffer(allocator);
    std::shared_ptr<AVBufferMock> buffer = AVBufferMockFactory::CreateAVBuffer(avbuffer);
    std::shared_ptr<FormatMock> param = buffer->GetParameter();
    (void)memset_s(buffer->GetAddr(), buffer->GetCapacity(), 0, 10000);
    param->PutIntValue(Tag::VIDEO_ENCODER_ENABLE_WATERMARK, 1);
    param->PutIntValue(Tag::VIDEO_COORDINATE_X, 10);
    param->PutIntValue(Tag::VIDEO_COORDINATE_Y, 11);
    param->PutIntValue(Tag::VIDEO_COORDINATE_H, DEFAULT_CONFIG.height);
    buffer->SetParameter(param);
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->Configure(format_));
    ASSERT_NE(AV_ERR_OK, videoEnc_->SetCustomBuffer(buffer));
}

/**
 * @tc.name: VideoEncoder_SetCustomBuffer_004
 * @tc.desc: x coordinate is out of range
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_SetCustomBuffer_004, TestSize.Level1)
{
    if (!GetWaterMarkCapability(GetParam())) {
        return;
    };
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    auto allocator = AVAllocatorFactory::CreateSurfaceAllocator(DEFAULT_CONFIG);
    std::shared_ptr<AVBuffer> avbuffer = AVBuffer::CreateAVBuffer(allocator);
    std::shared_ptr<AVBufferMock> buffer = AVBufferMockFactory::CreateAVBuffer(avbuffer);
    std::shared_ptr<FormatMock> param = buffer->GetParameter();
    (void)memset_s(buffer->GetAddr(), buffer->GetCapacity(), 0, 10000);
    param->PutIntValue(Tag::VIDEO_ENCODER_ENABLE_WATERMARK, 1);
    param->PutIntValue(Tag::VIDEO_COORDINATE_X, 10000);
    param->PutIntValue(Tag::VIDEO_COORDINATE_Y, 11);
    param->PutIntValue(Tag::VIDEO_COORDINATE_W, DEFAULT_CONFIG.width);
    param->PutIntValue(Tag::VIDEO_COORDINATE_H, DEFAULT_CONFIG.height);
    buffer->SetParameter(param);
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->Configure(format_));
    ASSERT_NE(AV_ERR_OK, videoEnc_->SetCustomBuffer(buffer));
}

/**
 * @tc.name: VideoEncoder_SetCustomBuffer_005
 * @tc.desc: pixelFormat is error
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_SetCustomBuffer_005, TestSize.Level1)
{
    if (!GetWaterMarkCapability(GetParam())) {
        return;
    };
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    BufferRequestConfig bufferConfig = {
        .width = 100,
        .height = 100,
        .strideAlignment = 0x8,
        .format = GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCBCR_420_SP,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
        .timeout = 0,
    };
    auto allocator = AVAllocatorFactory::CreateSurfaceAllocator(bufferConfig);
    std::shared_ptr<AVBuffer> avbuffer = AVBuffer::CreateAVBuffer(allocator);
    std::shared_ptr<AVBufferMock> buffer = AVBufferMockFactory::CreateAVBuffer(avbuffer);
    std::shared_ptr<FormatMock> param = buffer->GetParameter();
    (void)memset_s(buffer->GetAddr(), buffer->GetCapacity(), 0, 10000);
    param->PutIntValue(Tag::VIDEO_ENCODER_ENABLE_WATERMARK, 1);
    param->PutIntValue(Tag::VIDEO_COORDINATE_X, 10);
    param->PutIntValue(Tag::VIDEO_COORDINATE_Y, 11);
    param->PutIntValue(Tag::VIDEO_COORDINATE_W, DEFAULT_CONFIG.width);
    param->PutIntValue(Tag::VIDEO_COORDINATE_H, DEFAULT_CONFIG.height);
    buffer->SetParameter(param);
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->Configure(format_));
    ASSERT_NE(AV_ERR_OK, videoEnc_->SetCustomBuffer(buffer));
}

/**
 * @tc.name: VideoEncoder_SetCustomBuffer_006
 * @tc.desc: error memoryType
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_SetCustomBuffer_006, TestSize.Level1)
{
    if (!GetWaterMarkCapability(GetParam())) {
        return;
    };
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    std::shared_ptr<AVBufferMock> buffer = AVBufferMockFactory::CreateAVBuffer(10000);
    std::shared_ptr<FormatMock> param = buffer->GetParameter();
    (void)memset_s(buffer->GetAddr(), buffer->GetCapacity(), 0, 10000);
    param->PutIntValue(Tag::VIDEO_ENCODER_ENABLE_WATERMARK, 1);
    param->PutIntValue(Tag::VIDEO_COORDINATE_X, 10);
    param->PutIntValue(Tag::VIDEO_COORDINATE_Y, 11);
    param->PutIntValue(Tag::VIDEO_COORDINATE_W, DEFAULT_CONFIG.width);
    param->PutIntValue(Tag::VIDEO_COORDINATE_H, DEFAULT_CONFIG.height);
    buffer->SetParameter(param);
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->Configure(format_));
    ASSERT_NE(AV_ERR_OK, videoEnc_->SetCustomBuffer(buffer));
}

/**
 * @tc.name: VideoEncoder_SetCustomBuffer_007
 * @tc.desc: error state
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_SetCustomBuffer_007, TestSize.Level1)
{
    if (!GetWaterMarkCapability(GetParam())) {
        return;
    };
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    std::shared_ptr<AVBufferMock> buffer = AVBufferMockFactory::CreateAVBuffer(10000);
    std::shared_ptr<FormatMock> param = buffer->GetParameter();
    (void)memset_s(buffer->GetAddr(), buffer->GetCapacity(), 0, 10000);
    param->PutIntValue(Tag::VIDEO_ENCODER_ENABLE_WATERMARK, 1);
    param->PutIntValue(Tag::VIDEO_COORDINATE_X, 10);
    param->PutIntValue(Tag::VIDEO_COORDINATE_Y, 11);
    param->PutIntValue(Tag::VIDEO_COORDINATE_W, DEFAULT_CONFIG.width);
    param->PutIntValue(Tag::VIDEO_COORDINATE_H, DEFAULT_CONFIG.height);
    buffer->SetParameter(param);
    ASSERT_NE(AV_ERR_OK, videoEnc_->SetCustomBuffer(buffer));
}

/**
 * @tc.name: VideoEncoder_SetCustomBuffer_008
 * @tc.desc: error state
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_SetCustomBuffer_008, TestSize.Level1)
{
    if (!GetWaterMarkCapability(GetParam())) {
        return;
    };
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    std::shared_ptr<AVBufferMock> buffer = AVBufferMockFactory::CreateAVBuffer(10000);
    std::shared_ptr<FormatMock> param = buffer->GetParameter();
    (void)memset_s(buffer->GetAddr(), buffer->GetCapacity(), 0, 10000);
    param->PutIntValue(Tag::VIDEO_ENCODER_ENABLE_WATERMARK, 1);
    param->PutIntValue(Tag::VIDEO_COORDINATE_X, 10);
    param->PutIntValue(Tag::VIDEO_COORDINATE_Y, 11);
    param->PutIntValue(Tag::VIDEO_COORDINATE_W, DEFAULT_CONFIG.width);
    param->PutIntValue(Tag::VIDEO_COORDINATE_H, DEFAULT_CONFIG.height);
    buffer->SetParameter(param);
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    ASSERT_NE(AV_ERR_OK, videoEnc_->SetCustomBuffer(buffer));
}

/**
 * @tc.name: VideoEncoder_SetCustomBuffer_009
 * @tc.desc: encoder with water mark, surface mode.
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_SetCustomBuffer_009, TestSize.Level1)
{
    if (!GetWaterMarkCapability(GetParam())) {
        return;
    };
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    BufferRequestConfig bufferConfig = {
        .width = 400,
        .height = 400,
        .strideAlignment = 0x8,
        .format = GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_8888,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
        .timeout = 0,
    };
    auto allocator = AVAllocatorFactory::CreateSurfaceAllocator(bufferConfig);
    std::shared_ptr<AVBuffer> avbuffer = AVBuffer::CreateAVBuffer(allocator);
    bool ret = ReadCustomDataToAVBuffer("/data/test/media/test.rgba", avbuffer);
    ASSERT_EQ(ret, true);
    std::shared_ptr<AVBufferMock> buffer = AVBufferMockFactory::CreateAVBuffer(avbuffer);
    std::shared_ptr<FormatMock> param = buffer->GetParameter();
    param->PutIntValue(Tag::VIDEO_ENCODER_ENABLE_WATERMARK, 1);
    param->PutIntValue(Tag::VIDEO_COORDINATE_X, 100);
    param->PutIntValue(Tag::VIDEO_COORDINATE_Y, 100);
    param->PutIntValue(Tag::VIDEO_COORDINATE_W, bufferConfig.width);
    param->PutIntValue(Tag::VIDEO_COORDINATE_H, bufferConfig.height);
    buffer->SetParameter(param);
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->Configure(format_));
    ASSERT_EQ(AV_ERR_OK, videoEnc_->SetCustomBuffer(buffer));
    ASSERT_EQ(AV_ERR_OK, videoEnc_->CreateInputSurface());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
}

/**
 * @tc.name: VideoEncoder_SetCustomBuffer_0010
 * @tc.desc: repeat set custom buffer, surface mode.
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_SetCustomBuffer_0010, TestSize.Level1)
{
    if (!GetWaterMarkCapability(GetParam())) {
        return;
    };
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    BufferRequestConfig bufferConfig = {
        .width = 400,
        .height = 400,
        .strideAlignment = 0x8,
        .format = GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_8888,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
        .timeout = 0,
    };
    auto allocator = AVAllocatorFactory::CreateSurfaceAllocator(bufferConfig);
    std::shared_ptr<AVBuffer> avbuffer = AVBuffer::CreateAVBuffer(allocator);
    bool ret = ReadCustomDataToAVBuffer("/data/test/media/test.rgba", avbuffer);
    ASSERT_EQ(ret, true);
    std::shared_ptr<AVBufferMock> buffer = AVBufferMockFactory::CreateAVBuffer(avbuffer);
    std::shared_ptr<FormatMock> param = buffer->GetParameter();
    param->PutIntValue(Tag::VIDEO_ENCODER_ENABLE_WATERMARK, 1);
    param->PutIntValue(Tag::VIDEO_COORDINATE_X, 100);
    param->PutIntValue(Tag::VIDEO_COORDINATE_Y, 100);
    param->PutIntValue(Tag::VIDEO_COORDINATE_W, bufferConfig.width);
    param->PutIntValue(Tag::VIDEO_COORDINATE_H, bufferConfig.height);
    buffer->SetParameter(param);
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->Configure(format_));
    ASSERT_EQ(AV_ERR_OK, videoEnc_->SetCustomBuffer(buffer));
    ASSERT_EQ(AV_ERR_OK, videoEnc_->SetCustomBuffer(buffer));
    ASSERT_EQ(AV_ERR_OK, videoEnc_->CreateInputSurface());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
}
#endif // VIDEOENC_CAPI_UNIT_TEST

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
    videoEnc_->isAVBufferMode_ = true;
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    #ifdef HMOS_TEST
    videoEnc_->testParam_ = GetParam();
    videoEnc_->needCheckSHA_ = false;
    #else
    videoEnc_->needCheckSHA_ = false;
    #endif
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
    videoEnc_->isAVBufferMode_ = true;
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
    videoEnc_->isAVBufferMode_ = true;
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
    videoEnc_->isAVBufferMode_ = true;
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
    videoEnc_->isAVBufferMode_ = true;
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
    videoEnc_->isAVBufferMode_ = true;
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
    videoEnc_->isAVBufferMode_ = true;
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
    videoEnc_->isAVBufferMode_ = true;
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
    videoEnc_->isAVBufferMode_ = true;
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
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_TEMPORAL_GOP_REFERENCE_MODE,
                         static_cast<int32_t>(OH_TemporalGopReferenceMode::JUMP_REFERENCE));
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
    videoEnc_->isAVBufferMode_ = true;
    videoEnc_->isTemporalScalabilitySyncIdr_ = true;
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
    videoEnc_->isTemporalScalabilitySyncIdr_ = true;
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoEnc_->SetCallback(vencParamCallback_));
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_TEMPORAL_SCALABILITY, 1);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_TEMPORAL_GOP_REFERENCE_MODE,
                         static_cast<int32_t>(OH_TemporalGopReferenceMode::JUMP_REFERENCE));
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    ASSERT_EQ(AV_ERR_OK, videoEnc_->CreateInputSurface());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

HWTEST_P(TEST_SUIT, VideoEncoder_TemporalScalability_UNIFORMLY_01, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_TEMPORAL_SCALABILITY, 1);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_TEMPORAL_GOP_REFERENCE_MODE,
                         static_cast<int32_t>(OH_TemporalGopReferenceMode::UNIFORMLY_SCALED_REFERENCE));
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_TEMPORAL_GOP_SIZE, 3);
    ASSERT_NE(AV_ERR_OK, videoEnc_->Configure(format_));
}

HWTEST_P(TEST_SUIT, VideoEncoder_TemporalScalability_UNIFORMLY_02, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_TEMPORAL_SCALABILITY, 1);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_TEMPORAL_GOP_REFERENCE_MODE,
                         static_cast<int32_t>(OH_TemporalGopReferenceMode::UNIFORMLY_SCALED_REFERENCE));
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_TEMPORAL_GOP_SIZE, 8);
    ASSERT_NE(AV_ERR_OK, videoEnc_->Configure(format_));
}

HWTEST_P(TEST_SUIT, VideoEncoder_TemporalScalability_UNIFORMLY_03, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_TEMPORAL_SCALABILITY, 1);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_TEMPORAL_GOP_REFERENCE_MODE,
                         static_cast<int32_t>(OH_TemporalGopReferenceMode::UNIFORMLY_SCALED_REFERENCE));
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_TEMPORAL_GOP_SIZE, 2);
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
}

HWTEST_P(TEST_SUIT, VideoEncoder_TemporalScalability_UNIFORMLY_04, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_TEMPORAL_SCALABILITY, 1);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_TEMPORAL_GOP_REFERENCE_MODE,
                         static_cast<int32_t>(OH_TemporalGopReferenceMode::UNIFORMLY_SCALED_REFERENCE));
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_TEMPORAL_GOP_SIZE, 4);
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
}
#endif
} // namespace

int main(int argc, char **argv)
{
    testing::GTEST_FLAG(output) = "xml:./";
    for (int i = 0; i < argc; ++i) {
        cout << argv[i] << endl;
        if (strcmp(argv[i], "--need_dump") == 0) {
            VideoEncSample::needDump_ = true;
            DecArgv(i, argc, argv);
        }
    }
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}