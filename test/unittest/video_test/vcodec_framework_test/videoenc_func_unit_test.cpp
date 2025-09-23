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
#include "avcodec_log.h"
#include "avcodec_suspend.h"
#include "codeclist_mock.h"
#include "unittest_utils.h"

#ifdef VIDEOENC_CAPI_UNIT_TEST
#include "native_avmagic.h"
#include "videoenc_capi_mock.h"
#endif
#include "videoenc_func_test_suit.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace testing::ext;
using namespace testing::mt;
using namespace OHOS::MediaAVCodec::VCodecTestParam;
using namespace OHOS::Media;

namespace VFTSUIT {
std::atomic<int32_t> g_vencCount = 0;
std::string g_vencName = "";

void MultiThreadCreateVEnc()
{
    std::shared_ptr<VEncSignal> vencSignal = std::make_shared<VEncSignal>();
    std::shared_ptr<VEncCallbackTest> vencCallback = std::make_shared<VEncCallbackTest>(vencSignal);
    ASSERT_NE(nullptr, vencCallback);

    std::shared_ptr<VideoEncSample> videoEnc = std::make_shared<VideoEncSample>(vencSignal);
    ASSERT_NE(nullptr, videoEnc);

    EXPECT_LE(g_vencCount.load(), 25); // 25: max instances supported
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

void TEST_SUIT::SetUpTestCase(void)
{
    auto capability = CodecListMockFactory::GetCapabilityByCategory((CodecMimeType::VIDEO_AVC).data(), true,
                                                                    AVCodecCategory::AVCODEC_HARDWARE);
    ASSERT_NE(nullptr, capability) << (CodecMimeType::VIDEO_AVC).data() << " can not found!" << std::endl;
    g_vencName = capability->GetName();
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
    if (dstAddr == nullptr) {
        if (in) {
            free(in);
            in = nullptr;
        }
        return false;
    }

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

bool TEST_SUIT::GetTemporalScalabilityCapability(int32_t param)
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
    if (capabilityData->featuresMap.count(
        static_cast<int32_t>(AVCapabilityFeature::VIDEO_ENCODER_TEMPORAL_SCALABILITY))) {
        std::cout << "Support TemporalScalability" << std::endl;
        return true;
    } else {
        std::cout << "Not support TemporalScalability" << std::endl;
        return false;
    }
}

INSTANTIATE_TEST_SUITE_P(, TEST_SUIT, testing::Values(HW_AVC, HW_HEVC));

#if defined(VIDEOENC_INNER_UNIT_TEST)
#if defined(VIDEOENC_ASYNC_UNIT_TEST)
/**
 * @tc.name: VideoEncoder_Invalid_SetParameterCallback_001
 * @tc.desc: video setcallback
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoEncoder_Invalid_SetParameterCallback_001, TestSize.Level1)
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
#endif // VIDEOENC_ASYNC_UNIT_TEST
/**
 * @tc.name: VideoEncoder_Multithread_Create_001
 * @tc.desc: try create 40 instances
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoEncoder_Multithread_Create_001, TestSize.Level1)
{
    SET_THREAD_NUM(40);
    g_vencCount = 0;
    GTEST_RUN_TASK(MultiThreadCreateVEnc);
    cout << "remaining num: " << g_vencCount.load() << endl;
}

/**
 * @tc.name: VideoEncoder_SetROIParameter_001
 * @tc.desc: SetROIParameter and check if meta has roi key-value
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_SetROIParameter_001, TestSize.Level1)
{
    videoEnc_->roiRects_ = "100,100-100,100=-4";
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoEnc_->SetCallback(vencParamCallback_));
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, VideoEncodeBitrateMode::CBR);
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    ASSERT_EQ(AV_ERR_OK, videoEnc_->CreateInputSurface());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
}

/**
 * @tc.name: VideoEncoder_SetROIParameter_002
 * @tc.desc: SetROIParameter and check if meta has roi key-value
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_SetROIParameter_002, TestSize.Level1)
{
    videoEnc_->roiRects_ = "100,100-100,100=-4";
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoEnc_->SetCallback(vencParamCallback_));
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, VideoEncodeBitrateMode::VBR);
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    ASSERT_EQ(AV_ERR_OK, videoEnc_->CreateInputSurface());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
}

/**
 * @tc.name: VideoEncoder_SetROIParameter_003
 * @tc.desc: SetROIParameter and check if meta has roi key-value
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_SetROIParameter_003, TestSize.Level1)
{
    videoEnc_->isAVBufferMode_ = true;
    videoEnc_->roiRects_ = "100,100-100,100=-4";
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, VideoEncodeBitrateMode::CBR);
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
}

/**
 * @tc.name: VideoEncoder_SetROIParameter_004
 * @tc.desc: SetROIParameter and check if meta has roi key-value
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_SetROIParameter_004, TestSize.Level1)
{
    videoEnc_->isAVBufferMode_ = true;
    videoEnc_->roiRects_ = "100,100-100,100=-4";
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, VideoEncodeBitrateMode::VBR);
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
}

/**
 * @tc.name: VideoEncoder_SetROIParameter_005
 * @tc.desc: SetROIParameter and check if meta has roi key-value
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_SetROIParameter_005, TestSize.Level1)
{
    videoEnc_->isAVBufferMode_ = true;
    videoEnc_->roiRects_ = "";
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, VideoEncodeBitrateMode::CBR);
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
}

/**
 * @tc.name: VideoEncoder_SetROIParameter_006
 * @tc.desc: SetROIParameter and check if meta has roi key-value
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_SetROIParameter_006, TestSize.Level1)
{
    videoEnc_->isAVBufferMode_ = true;
    videoEnc_->roiRects_ = "100,100-200,200";
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, VideoEncodeBitrateMode::CBR);
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
}

/**
 * @tc.name: VideoEncoder_SetROIParameter_007
 * @tc.desc: SetROIParameter and check if meta has roi key-value
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_SetROIParameter_007, TestSize.Level1)
{
    videoEnc_->isAVBufferMode_ = true;
    videoEnc_->roiRects_ = "1,2,3,4,5-6,1=-4";
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, VideoEncodeBitrateMode::CBR);
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
}

/**
 * @tc.name: VideoEncoder_SetROIParameter_008
 * @tc.desc: SetROIParameter and check if meta has roi key-value
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_SetROIParameter_008, TestSize.Level1)
{
    videoEnc_->isAVBufferMode_ = true;
    videoEnc_->roiRects_ = "100,100-200,200=-4;300,300-400,400=5";
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, VideoEncodeBitrateMode::VBR);
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
}

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
#endif // VIDEOENC_INNER_UNIT_TEST
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
 * @tc.name: VideoEncoder_HRDVivid2SDR_001
 * @tc.desc: set key output_color_space, value is INT32_MIN
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_HRDVivid2SDR_001, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE, INT32_MIN);
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_HRDVivid2SDR_002
 * @tc.desc: set key output_color_space, value is INT32_MAX
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_HRDVivid2SDR_002, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE, INT32_MAX);
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_HRDVivid2SDR_003
 * @tc.desc: set key output_color_space, value is BT2020_HLG_LIMIT
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_HRDVivid2SDR_003, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT2020_HLG_LIMIT);
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_HRDVivid2SDR_004
 * @tc.desc: set key output_color_space, value is BT709_LIMIT
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_HRDVivid2SDR_004, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_LIMIT);
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_Hardware_Freeze_001
 * @tc.desc: encoder is Flush and freeze process
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_Hardware_Freeze_001, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Flush());
    pid_t pid = getpid();
    std::vector<pid_t> pidList = {pid};
    auto ret = AVCodecSuspend::SuspendFreeze(pidList);
    ASSERT_EQ(AVCS_ERR_OK, ret);
}

/**
 * @tc.name: VideoEncoder_Hardware_Active_001
 * @tc.desc: active process of freeze and encoder is Flush
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_Hardware_Active_001, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Flush());
    pid_t pid = getpid();
    std::vector<pid_t> pidList = {pid};
    auto ret = AVCodecSuspend::SuspendFreeze(pidList);
    ASSERT_EQ(AVCS_ERR_OK, ret);
    ret = AVCodecSuspend::SuspendActive(pidList);
    ASSERT_EQ(AVCS_ERR_OK, ret);
}

/**
 * @tc.name: VideoEncoder_Hardware_Active_All_001
 * @tc.desc: active all process of freeze and decoder is Flush
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_Hardware_Active_All_001, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Flush());
    pid_t pid = getpid();
    std::vector<pid_t> pidList = {pid};
    auto ret = AVCodecSuspend::SuspendFreeze(pidList);
    ASSERT_EQ(AVCS_ERR_OK, ret);
    ret = AVCodecSuspend::SuspendActiveAll();
    ASSERT_EQ(AVCS_ERR_OK, ret);
}
#ifdef HMOS_TEST
/**
 * @tc.name: VideoEncoder_SetPTSParameter_001
 * @tc.desc: SetPTSParameter, avbuffer mode
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_SetPTSParameter_001, TestSize.Level1)
{
    videoEnc_->enableVariableFrameRate_ = true;
    videoEnc_->isAVBufferMode_ = true;
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_ENABLE_PTS_BASED_RATECONTROL, 1);
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

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

/**
 * @tc.name: GetParameter_001
 * @tc.desc: Test GetParameter interface normal case
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, GetParameter_001, TestSize.Level1)
{
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    auto allocator = AVAllocatorFactory::CreateSurfaceAllocator(DEFAULT_CONFIG);
    std::shared_ptr<AVBuffer> avbuffer = AVBuffer::CreateAVBuffer(allocator);
    std::shared_ptr<AVBufferMock> buffer = AVBufferMockFactory::CreateAVBuffer(avbuffer);
    std::shared_ptr<FormatMock> param = buffer->GetParameter();
    EXPECT_NE(param, nullptr);
    bool result = param->PutStringValue("test_key", "test_value");
    EXPECT_TRUE(result);
}
#endif // HMOS_TEST
} // namespace