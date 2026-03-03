/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#include <unordered_map>
#include "avcodec_log.h"
#include "native_avcodec_base.h"
#include "unittest_utils.h"
#include "video_processing_types.h"
#include "video_processing.h"
#include "videodec_hdrvivid2sdr_unit_test.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace testing::ext;
using namespace testing::mt;
using namespace OHOS::MediaAVCodec::VCodecTestParam;
using namespace TESTBASE;

namespace HevcTestSuit {
enum class ResourceType : int32_t { SDR, HDR, HDR_HLG_FULL };

const std::unordered_map<ResourceType, int32_t> RESOURCE_TESTCODE = {
    {ResourceType::SDR, HW_HEVC},
    {ResourceType::HDR, HW_HDR},
    {ResourceType::HDR_HLG_FULL, HW_HDR_HLG_FULL},
};

class HEVC_TEST_SUIT
    : public HdrVivid2SdrBaseSuit,
      public testing::TestWithParam<std::tuple<std::string_view, ResourceType, AVCodecCategory>> {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);
    void CreateByNameWithParam(std::string_view param);
    void PrepareSource(ResourceType param);
    void ConfigureHdrVivid2Sdr(std::string_view testCode, ResourceType resourceType);
    static constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK,
                                                          STRINGFY(HEVC_TEST_SUIT)};
};

void HEVC_TEST_SUIT::SetUpTestCase(void)
{
    auto capability = CodecListMockFactory::GetCapabilityByCategory((CodecMimeType::VIDEO_HEVC).data(), false,
                                                                    AVCodecCategory::AVCODEC_HARDWARE);
    ASSERT_NE(nullptr, capability) << (CodecMimeType::VIDEO_HEVC).data() << " can not found!" << std::endl;
    ISHDR2SDRSupported();
}

void HEVC_TEST_SUIT::TearDownTestCase(void) {}

void HEVC_TEST_SUIT::SetUp(void)
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

    const ::testing::TestInfo *testInfo_ = ::testing::UnitTest::GetInstance()->current_test_info();
    std::string testCaseName = testInfo_->name();
    AVCODEC_LOGI("%{public}s", testCaseName.c_str());
}

void HEVC_TEST_SUIT::TearDown(void)
{
    if (format_ != nullptr) {
        format_->Destroy();
    }
    videoDec_ = nullptr;
}

void HEVC_TEST_SUIT::CreateByNameWithParam(std::string_view param)
{
    std::string codecName = "";
    if (param == CodecMimeType::VIDEO_HEVC) {
        capability_ = CodecListMockFactory::GetCapabilityByCategory((CodecMimeType::VIDEO_HEVC).data(), false,
                                                                    AVCodecCategory::AVCODEC_HARDWARE);
    } else {
        capability_ = CodecListMockFactory::GetCapabilityByCategory((CodecMimeType::VIDEO_AVC).data(), false,
                                                                    AVCodecCategory::AVCODEC_HARDWARE);
    }
    codecName = capability_->GetName();
    std::cout << "CodecName: " << codecName << "\n";
    ASSERT_TRUE(CreateVideoCodecByName(codecName));
}

void HEVC_TEST_SUIT::PrepareSource(ResourceType param)
{
    auto testCode = RESOURCE_TESTCODE.at(param);
    std::string sourcePath = decSourcePathMap_.at(testCode);
    if (testCode != HW_AVC && testCode != SW_AVC) {
        videoDec_->SetSourceType(false);
    }
    videoDec_->testParam_ = testCode;
    std::cout << "SourcePath: " << sourcePath << std::endl;
    videoDec_->SetSource(sourcePath);
    const ::testing::TestInfo *testInfo_ = ::testing::UnitTest::GetInstance()->current_test_info();
    string prefix = "/data/test/media/";
    string fileName = testInfo_->name();
    auto check = [](char it) { return it == '/'; };
    (void)fileName.erase(std::remove_if(fileName.begin(), fileName.end(), check), fileName.end());
    videoDec_->SetOutPath(prefix + fileName);
}

void GetInitialParam(ResourceType resourceType, int32_t &initialWidth, int32_t &initialHeight,
                     int32_t &initialColorSpace)
{
    switch (resourceType) {
        case ResourceType::HDR:
            initialWidth = 1280; // 1280: resource width
            initialHeight = 720; // 720: resource height
            initialColorSpace = OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_LIMIT;
            break;
        case ResourceType::HDR_HLG_FULL:
            initialWidth = 1920;  // 1920: resource width
            initialHeight = 1440; // 1440: resource height
            initialColorSpace = OH_NativeBuffer_ColorSpace::OH_COLORSPACE_P3_FULL;
            break;
        default:
            initialWidth = 1280; // 1280: resource width
            initialHeight = 720; // 720: resource height
            initialColorSpace = OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_LIMIT;
            break;
    }
}

void CheckFormatKey(std::shared_ptr<FormatMock> format, ResourceType resourceType)
{
    int32_t initialWidth;
    int32_t initialHeight;
    int32_t initialColorSpace;
    GetInitialParam(resourceType, initialWidth, initialHeight, initialColorSpace);
    int32_t width = 0;
    int32_t height = 0;
    int32_t pictureWidth = 0;
    int32_t pictureHeight = 0;
    int32_t stride = 0;
    int32_t sliceHeight = 0;
    int32_t colorSpace = 0;

    EXPECT_TRUE(format->GetIntValue(Media::Tag::VIDEO_WIDTH, width));
    EXPECT_TRUE(format->GetIntValue(Media::Tag::VIDEO_HEIGHT, height));
    EXPECT_TRUE(format->GetIntValue(Media::Tag::VIDEO_PIC_WIDTH, pictureWidth));
    EXPECT_TRUE(format->GetIntValue(Media::Tag::VIDEO_PIC_HEIGHT, pictureHeight));
    EXPECT_TRUE(format->GetIntValue(Media::Tag::VIDEO_STRIDE, stride));
    EXPECT_TRUE(format->GetIntValue(Media::Tag::VIDEO_SLICE_HEIGHT, sliceHeight));
    EXPECT_TRUE(format->GetIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE, colorSpace));

    EXPECT_EQ(width, initialWidth);
    EXPECT_EQ(height, initialHeight);
    EXPECT_GE(pictureWidth, initialWidth - 1);
    EXPECT_GE(pictureHeight, initialHeight - 1);
    EXPECT_GE(stride, initialWidth);
    EXPECT_GE(sliceHeight, initialHeight);
    EXPECT_EQ(colorSpace, initialColorSpace);
}

void HEVC_TEST_SUIT::ConfigureHdrVivid2Sdr(std::string_view mimeType, ResourceType resourceType)
{
    auto testCode = RESOURCE_TESTCODE.at(resourceType);
    CreateByNameWithParam(mimeType);
    SetFormatWithParam(VideoPixelFormat::NV12);
    PrepareSource(resourceType);
    switch (testCode) {
        case HW_HDR:
            format_->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
                                 OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_LIMIT);
            break;
        case HW_HDR_HLG_FULL:
            format_->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
                                 OH_NativeBuffer_ColorSpace::OH_COLORSPACE_P3_FULL);
            break;
        default:
            format_->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
                                 OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_LIMIT);
            break;
    }
}

INSTANTIATE_TEST_SUITE_P(,
                         HEVC_TEST_SUIT,
                         testing::Values(std::make_tuple(CodecMimeType::VIDEO_HEVC, ResourceType::SDR,
                                                         AVCodecCategory::AVCODEC_HARDWARE),
                                         std::make_tuple(CodecMimeType::VIDEO_HEVC, ResourceType::HDR,
                                                         AVCodecCategory::AVCODEC_HARDWARE),
                                         std::make_tuple(CodecMimeType::VIDEO_HEVC, ResourceType::HDR_HLG_FULL,
                                                         AVCodecCategory::AVCODEC_HARDWARE),
                                         std::make_tuple(CodecMimeType::VIDEO_HEVC, ResourceType::SDR,
                                                         AVCodecCategory::AVCODEC_SOFTWARE),
                                         std::make_tuple(CodecMimeType::VIDEO_HEVC, ResourceType::HDR,
                                                         AVCodecCategory::AVCODEC_SOFTWARE),
                                         std::make_tuple(CodecMimeType::VIDEO_HEVC, ResourceType::HDR_HLG_FULL,
                                                         AVCodecCategory::AVCODEC_SOFTWARE)));

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_0011
 * @tc.desc: set invalid key
 * @tc.type: FUNC
 */
HWTEST_P(HEVC_TEST_SUIT, VideoDecoder_HRDVivid2SDR_0011, TestSize.Level1)
{
    auto params = GetParam();
    std::string_view mimeType = std::get<0>(params);
    ResourceType resourceType = std::get<1>(params);
    int32_t colorSpace = INT32_MIN;
    CreateByNameWithParam(mimeType);
    SetFormatWithParam(VideoPixelFormat::NV12);
    PrepareSource(resourceType);
    format_->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE, colorSpace);
    ASSERT_EQ(AV_ERR_INVALID_VAL, videoDec_->Configure(format_));
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_0021
 * @tc.desc: set invalid key
 * @tc.type: FUNC
 */
HWTEST_P(HEVC_TEST_SUIT, VideoDecoder_HRDVivid2SDR_0021, TestSize.Level1)
{
    auto params = GetParam();
    std::string_view mimeType = std::get<0>(params);
    ResourceType resourceType = std::get<1>(params);
    int32_t colorSpace = INT32_MAX;
    CreateByNameWithParam(mimeType.data());
    SetFormatWithParam(VideoPixelFormat::NV12);
    PrepareSource(resourceType);
    format_->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE, colorSpace);
    ASSERT_EQ(AV_ERR_INVALID_VAL, videoDec_->Configure(format_));
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_1011
 * @tc.desc: 1. key pixel format unset;
 *           2. decoder mode is buffer;
 *           3. prepare function is not called;
 *           4. key color space is BT709_LIMIT;
 * @tc.type: FUNC
 */
HWTEST_P(HEVC_TEST_SUIT, VideoDecoder_HRDVivid2SDR_1011, TestSize.Level1)
{
    auto params = GetParam();
    std::string_view mimeType = std::get<0>(params);
    ResourceType resourceType = std::get<1>(params);
    CreateByNameWithParam(mimeType.data());
    std::shared_ptr<FormatMock> format = FormatMockFactory::CreateFormat();
    format->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    PrepareSource(resourceType);
    format->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
                        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_LIMIT);
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format));
    ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, videoDec_->Start());
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_1032
 * @tc.desc: 1. key pixel format unset;
 *           2. decoder mode is buffer;
 *           3. prepare function is not called;
 *           4. key color space is OH_COLORSPACE_P3_FULL;
 * @tc.type: FUNC
 */
HWTEST_P(HEVC_TEST_SUIT, VideoDecoder_HRDVivid2SDR_1032, TestSize.Level1)
{
    auto params = GetParam();
    std::string_view mimeType = std::get<0>(params);
    ResourceType resourceType = std::get<1>(params);
    CreateByNameWithParam(mimeType.data());
    std::shared_ptr<FormatMock> format = FormatMockFactory::CreateFormat();
    format->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    PrepareSource(resourceType);
    format->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE, OH_NativeBuffer_ColorSpace::OH_COLORSPACE_P3_FULL);
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format));
    ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, videoDec_->Start());
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_1041
 * @tc.desc: 1. key pixel format is NV12;
 *           2. decoder mode is buffer;
 *           3. prepare function is not called;
 *           4. key color space is BT709_LIMIT;
 * @tc.type: FUNC
 */
HWTEST_P(HEVC_TEST_SUIT, VideoDecoder_HRDVivid2SDR_1041, TestSize.Level1)
{
    auto params = GetParam();
    std::string_view mimeType = std::get<0>(params);
    ResourceType resourceType = std::get<1>(params);
    ConfigureHdrVivid2Sdr(mimeType, resourceType);
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, videoDec_->Start());
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_1051
 * @tc.desc: 1. key pixel format unset;
 *           2. decoder mode is surface;
 *           3. prepare function is not called;
 *           4. key color space is BT709_LIMIT;
 * @tc.type: FUNC
 */
HWTEST_P(HEVC_TEST_SUIT, VideoDecoder_HRDVivid2SDR_1051, TestSize.Level1)
{
    auto params = GetParam();
    std::string_view mimeType = std::get<0>(params);
    ResourceType resourceType = std::get<1>(params);
    CreateByNameWithParam(mimeType.data());
    std::shared_ptr<FormatMock> format = FormatMockFactory::CreateFormat();
    format->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    PrepareSource(resourceType);
    format->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
                        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_LIMIT);
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format));
    ASSERT_EQ(AV_ERR_OK, videoDec_->SetOutputSurface());
    ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, videoDec_->Start());
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_1052
 * @tc.desc: 1. key pixel format unset;
 *           2. decoder mode is surface;
 *           3. prepare function is not called;
 *           4. key color space is OH_COLORSPACE_P3_FULL;
 * @tc.type: FUNC
 */
HWTEST_P(HEVC_TEST_SUIT, VideoDecoder_HRDVivid2SDR_1052, TestSize.Level1)
{
    auto params = GetParam();
    std::string_view mimeType = std::get<0>(params);
    ResourceType resourceType = std::get<1>(params);
    CreateByNameWithParam(mimeType.data());
    std::shared_ptr<FormatMock> format = FormatMockFactory::CreateFormat();
    format->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    PrepareSource(resourceType);
    format->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE, OH_NativeBuffer_ColorSpace::OH_COLORSPACE_P3_FULL);
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format));
    ASSERT_EQ(AV_ERR_OK, videoDec_->SetOutputSurface());
    ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, videoDec_->Start());
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_1061
 * @tc.desc: 1. key pixel format is NV12;
 *           2. decoder mode is surface;
 *           3. prepare function is not called;
 *           4. key color space is BT709_LIMIT;
 * @tc.type: FUNC
 */
HWTEST_P(HEVC_TEST_SUIT, VideoDecoder_HRDVivid2SDR_1061, TestSize.Level1)
{
    auto params = GetParam();
    std::string_view mimeType = std::get<0>(params);
    ResourceType resourceType = std::get<1>(params);
    ConfigureHdrVivid2Sdr(mimeType, resourceType);
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    ASSERT_EQ(AV_ERR_OK, videoDec_->SetOutputSurface());
    ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, videoDec_->Start());
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_1071
 * @tc.desc: 1. key pixel format unset;
 *           2. decoder mode is buffer;
 *           3. prepare function is called before start function;
 *           4. key color space is BT709_LIMIT;
 * @tc.type: FUNC
 */
HWTEST_P(HEVC_TEST_SUIT, VideoDecoder_HRDVivid2SDR_1071, TestSize.Level1)
{
    auto params = GetParam();
    std::string_view mimeType = std::get<0>(params);
    ResourceType resourceType = std::get<1>(params);
    CreateByNameWithParam(mimeType.data());
    std::shared_ptr<FormatMock> format = FormatMockFactory::CreateFormat();
    format->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    PrepareSource(resourceType);
    format->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
                        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_LIMIT);
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format));
    ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, videoDec_->Prepare());
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_1072
 * @tc.desc: 1. key pixel format unset;
 *           2. decoder mode is buffer;
 *           3. prepare function is called before start function;
 *           4. key color space is OH_COLORSPACE_P3_FULL;
 * @tc.type: FUNC
 */
HWTEST_P(HEVC_TEST_SUIT, VideoDecoder_HRDVivid2SDR_1072, TestSize.Level1)
{
    auto params = GetParam();
    std::string_view mimeType = std::get<0>(params);
    ResourceType resourceType = std::get<1>(params);
    CreateByNameWithParam(mimeType.data());
    std::shared_ptr<FormatMock> format = FormatMockFactory::CreateFormat();
    format->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    PrepareSource(resourceType);
    format->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE, OH_NativeBuffer_ColorSpace::OH_COLORSPACE_P3_FULL);
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format));
    ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, videoDec_->Prepare());
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_1081
 * @tc.desc: 1. key pixel format is NV12;
 *           2. decoder mode is buffer;
 *           3. prepare function is called before start function;
 *           4. key color space is BT709_LIMIT;
 * @tc.type: FUNC
 */
HWTEST_P(HEVC_TEST_SUIT, VideoDecoder_HRDVivid2SDR_1081, TestSize.Level1)
{
    auto params = GetParam();
    std::string_view mimeType = std::get<0>(params);
    ResourceType resourceType = std::get<1>(params);
    ConfigureHdrVivid2Sdr(mimeType, resourceType);
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, videoDec_->Prepare());
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_1082
 * @tc.desc: 1. key pixel format is NV12;
 *           2. decoder mode is buffer;
 *           3. prepare function is called before start function;
 *           4. key color space is OH_COLORSPACE_P3_FULL;
 * @tc.type: FUNC
 */
HWTEST_P(HEVC_TEST_SUIT, VideoDecoder_HRDVivid2SDR_1082, TestSize.Level1)
{
    auto params = GetParam();
    std::string_view mimeType = std::get<0>(params);
    ResourceType resourceType = std::get<1>(params);
    ConfigureHdrVivid2Sdr(mimeType, resourceType);
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, videoDec_->Prepare());
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_1091
 * @tc.desc: 1. key pixel format unset;
 *           2. decoder mode is surface;
 *           3. prepare function is called before start function;
 *           4. key color space is BT709_LIMIT;
 *           5. resource is hdr;
 * @tc.type: FUNC
 */
HWTEST_F(HEVC_TEST_SUIT, VideoDecoder_HRDVivid2SDR_1091, TestSize.Level1)
{
    std::string_view mimeType = CodecMimeType::VIDEO_HEVC;
    ResourceType resourceType = ResourceType::HDR;
    CreateByNameWithParam(mimeType.data());
    std::shared_ptr<FormatMock> format = FormatMockFactory::CreateFormat();
    format->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    PrepareSource(resourceType);
    format->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
                        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_LIMIT);
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format));
    ASSERT_EQ(AV_ERR_OK, videoDec_->SetOutputSurface());
    ASSERT_EQ(AV_ERR_OK, videoDec_->Prepare());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    std::shared_ptr<FormatMock> curFormat = videoDec_->GetOutputDescription();
    CheckFormatKey(curFormat, resourceType);
    EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_1092
 * @tc.desc: 1. key pixel format unset;
 *           2. decoder mode is surface;
 *           3. prepare function is called before start function;
 *           4. key color space is OH_COLORSPACE_P3_FULL;
 *           5. resource is hdr hlg full;
 * @tc.type: FUNC
 */
HWTEST_F(HEVC_TEST_SUIT, VideoDecoder_HRDVivid2SDR_1092, TestSize.Level1)
{
    std::string_view mimeType = CodecMimeType::VIDEO_HEVC;
    ResourceType resourceType = ResourceType::HDR_HLG_FULL;
    CreateByNameWithParam(mimeType.data());
    std::shared_ptr<FormatMock> format = FormatMockFactory::CreateFormat();
    format->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH_HLG_FULL);
    format->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT_HLG_FULL);
    PrepareSource(resourceType);
    format->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE, OH_NativeBuffer_ColorSpace::OH_COLORSPACE_P3_FULL);
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format));
    ASSERT_EQ(AV_ERR_OK, videoDec_->SetOutputSurface());
    ASSERT_EQ(AV_ERR_OK, videoDec_->Prepare());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    std::shared_ptr<FormatMock> curFormat = videoDec_->GetOutputDescription();
    CheckFormatKey(curFormat, resourceType);
    EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_1101
 * @tc.desc: 1. key pixel format is NV12;
 *           2. decoder mode is surface;
 *           3. prepare function is called before start function;
 *           4. key color space is BT709_LIMIT;
 *           5. resource is hdr;
 * @tc.type: FUNC
 */
HWTEST_F(HEVC_TEST_SUIT, VideoDecoder_HRDVivid2SDR_1101, TestSize.Level1)
{
    std::string_view mimeType = CodecMimeType::VIDEO_HEVC;
    ResourceType resourceType = ResourceType::HDR;
    ConfigureHdrVivid2Sdr(mimeType, resourceType);
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    ASSERT_EQ(AV_ERR_OK, videoDec_->SetOutputSurface());
    ASSERT_EQ(AV_ERR_OK, videoDec_->Prepare());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    std::shared_ptr<FormatMock> curFormat = videoDec_->GetOutputDescription();
    CheckFormatKey(curFormat, resourceType);
    EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_1102
 * @tc.desc: 1. key pixel format is NV12;
 *           2. decoder mode is surface;
 *           3. prepare function is called before start function;
 *           4. key color space is BT709_LIMIT;
 *           5. resource is hdr hlg full;
 * @tc.type: FUNC
 */
HWTEST_F(HEVC_TEST_SUIT, VideoDecoder_HRDVivid2SDR_1102, TestSize.Level1)
{
    std::string_view mimeType = CodecMimeType::VIDEO_HEVC;
    ResourceType resourceType = ResourceType::HDR_HLG_FULL;
    ConfigureHdrVivid2Sdr(mimeType, resourceType);
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    ASSERT_EQ(AV_ERR_OK, videoDec_->SetOutputSurface());
    ASSERT_EQ(AV_ERR_OK, videoDec_->Prepare());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    std::shared_ptr<FormatMock> curFormat = videoDec_->GetOutputDescription();
    CheckFormatKey(curFormat, resourceType);
    EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_1111
 * @tc.desc: 1. key pixel format is NV12;
 *           2. key color space is BT2020_HLG_LIMIT;
 * @tc.type: FUNC
 */
HWTEST_P(HEVC_TEST_SUIT, VideoDecoder_HRDVivid2SDR_1111, TestSize.Level1)
{
    auto params = GetParam();
    std::string_view mimeType = std::get<0>(params);
    ResourceType resourceType = std::get<1>(params);
    CreateByNameWithParam(mimeType.data());
    SetFormatWithParam(VideoPixelFormat::NV12);
    PrepareSource(resourceType);
    format_->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
                         OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT2020_HLG_LIMIT);
    ASSERT_EQ(AV_ERR_VIDEO_UNSUPPORTED_COLOR_SPACE_CONVERSION, videoDec_->Configure(format_));
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_1121
 * @tc.desc: 1. key pixel format unset;
 *           2. key color space is BT2020_HLG_LIMIT;
 * @tc.type: FUNC
 */
HWTEST_P(HEVC_TEST_SUIT, VideoDecoder_HRDVivid2SDR_1121, TestSize.Level1)
{
    auto params = GetParam();
    std::string_view mimeType = std::get<0>(params);
    ResourceType resourceType = std::get<1>(params);
    CreateByNameWithParam(mimeType.data());
    std::shared_ptr<FormatMock> format = FormatMockFactory::CreateFormat();
    format->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    PrepareSource(resourceType);
    format->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
                        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT2020_HLG_LIMIT);
    ASSERT_EQ(AV_ERR_VIDEO_UNSUPPORTED_COLOR_SPACE_CONVERSION, videoDec_->Configure(format));
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_1131
 * @tc.desc: 1. key pixel format is NV21;
 *           2. decoder mode is buffer;
 *           3. prepare function is not called;
 *           4. key color space is BT709_LIMIT;
 * @tc.type: FUNC
 */
HWTEST_P(HEVC_TEST_SUIT, VideoDecoder_HRDVivid2SDR_1131, TestSize.Level1)
{
    IsPixelFormatSupported(VideoPixelFormat::NV21);
    auto params = GetParam();
    std::string_view mimeType = std::get<0>(params);
    ResourceType resourceType = std::get<1>(params);
    CreateByNameWithParam(mimeType.data());
    SetFormatWithParam(VideoPixelFormat::NV21);
    PrepareSource(resourceType);
    format_->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
                        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_LIMIT);
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, videoDec_->Start());
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_1141
 * @tc.desc: 1. key pixel format is NV21;
 *           2. decoder mode is surface;
 *           3. prepare function is not called;
 *           4. key color space is BT709_LIMIT;
 * @tc.type: FUNC
 */
HWTEST_P(HEVC_TEST_SUIT, VideoDecoder_HRDVivid2SDR_1141, TestSize.Level1)
{
    IsPixelFormatSupported(VideoPixelFormat::NV21);
    auto params = GetParam();
    std::string_view mimeType = std::get<0>(params);
    ResourceType resourceType = std::get<1>(params);
    CreateByNameWithParam(mimeType.data());
    SetFormatWithParam(VideoPixelFormat::NV21);
    PrepareSource(resourceType);
    format_->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
                        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_LIMIT);
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    ASSERT_EQ(AV_ERR_OK, videoDec_->SetOutputSurface());
    ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, videoDec_->Start());
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_1151
 * @tc.desc: 1. key pixel format is NV21;
 *           2. decoder mode is buffer;
 *           3. prepare function is called before start function;
 *           4. key color space is BT709_LIMIT;
 * @tc.type: FUNC
 */
HWTEST_P(HEVC_TEST_SUIT, VideoDecoder_HRDVivid2SDR_1151, TestSize.Level1)
{
    IsPixelFormatSupported(VideoPixelFormat::NV21);
    auto params = GetParam();
    std::string_view mimeType = std::get<0>(params);
    ResourceType resourceType = std::get<1>(params);
    CreateByNameWithParam(mimeType.data());
    SetFormatWithParam(VideoPixelFormat::NV21);
    PrepareSource(resourceType);
    format_->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
                        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_LIMIT);
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, videoDec_->Prepare());
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_1161
 * @tc.desc: 1. key pixel format is NV21;
 *           2. decoder mode is surface;
 *           3. prepare function is called before start function;
 *           4. key color space is BT709_LIMIT;
 *           5. resource is hdr;
 * @tc.type: FUNC
 */
HWTEST_F(HEVC_TEST_SUIT, VideoDecoder_HRDVivid2SDR_1161, TestSize.Level1)
{
    IsPixelFormatSupported(VideoPixelFormat::NV21);
    std::string_view mimeType = CodecMimeType::VIDEO_HEVC;
    ResourceType resourceType = ResourceType::HDR;
    CreateByNameWithParam(mimeType);
    SetFormatWithParam(VideoPixelFormat::NV21);
    PrepareSource(resourceType);
    format_->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
                        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_LIMIT);
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    ASSERT_EQ(AV_ERR_OK, videoDec_->SetOutputSurface());
    ASSERT_EQ(AV_ERR_OK, videoDec_->Prepare());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    std::shared_ptr<FormatMock> curFormat = videoDec_->GetOutputDescription();
    CheckFormatKey(curFormat, resourceType);
    EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_1162
 * @tc.desc: 1. key pixel format is NV21;
 *           2. decoder mode is surface;
 *           3. prepare function is called before start function;
 *           4. key color space is BT709_LIMIT;
 *           5. resource is hdr hlg full;
 * @tc.type: FUNC
 */
HWTEST_F(HEVC_TEST_SUIT, VideoDecoder_HRDVivid2SDR_1162, TestSize.Level1)
{
    IsPixelFormatSupported(VideoPixelFormat::NV21);
    std::string_view mimeType = CodecMimeType::VIDEO_HEVC;
    ResourceType resourceType = ResourceType::HDR_HLG_FULL;
    CreateByNameWithParam(mimeType);
    SetFormatWithParam(VideoPixelFormat::NV21);
    PrepareSource(resourceType);
    format_->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
                        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_P3_FULL);
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    ASSERT_EQ(AV_ERR_OK, videoDec_->SetOutputSurface());
    ASSERT_EQ(AV_ERR_OK, videoDec_->Prepare());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    std::shared_ptr<FormatMock> curFormat = videoDec_->GetOutputDescription();
    CheckFormatKey(curFormat, resourceType);
    EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_1171
 * @tc.desc: 1. key pixel format is NV21;
 *           2. key color space is BT2020_HLG_LIMIT;
 * @tc.type: FUNC
 */
HWTEST_P(HEVC_TEST_SUIT, VideoDecoder_HRDVivid2SDR_1171, TestSize.Level1)
{
    IsPixelFormatSupported(VideoPixelFormat::NV21);
    auto params = GetParam();
    std::string_view mimeType = std::get<0>(params);
    ResourceType resourceType = std::get<1>(params);
    CreateByNameWithParam(mimeType.data());
    SetFormatWithParam(VideoPixelFormat::NV21);
    PrepareSource(resourceType);
    format_->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
                        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT2020_HLG_LIMIT);
    ASSERT_EQ(AV_ERR_VIDEO_UNSUPPORTED_COLOR_SPACE_CONVERSION, videoDec_->Configure(format_));
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_1181
 * @tc.desc: 1. key pixel format is NV12;
 *           2. key color space is BT709_LIMIT
 *           3. decoder mode is surface;
 *           4. prepare function is called before start function;
 *           5. start -> flush -> stop;
 * @tc.type: FUNC
 */
HWTEST_P(HEVC_TEST_SUIT, VideoDecoder_HRDVivid2SDR_1181, TestSize.Level1)
{
    auto params = GetParam();
    ResourceType resourceType = std::get<1>(params);
    if (resourceType != ResourceType::SDR) {
        std::string_view mimeType = std::get<0>(params);
        ConfigureHdrVivid2Sdr(mimeType, resourceType);
        ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
        ASSERT_EQ(AV_ERR_OK, videoDec_->SetOutputSurface());
        ASSERT_EQ(AV_ERR_OK, videoDec_->Prepare());
        EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
        EXPECT_EQ(AV_ERR_OK, videoDec_->Flush());
        EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
    }
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_1191
 * @tc.desc: unordered post processing function invocation
 * @tc.type: FUNC
 */
HWTEST_P(HEVC_TEST_SUIT, VideoDecoder_HRDVivid2SDR_1191, TestSize.Level1)
{
    auto params = GetParam();
    ResourceType resourceType = std::get<1>(params);
    if (resourceType != ResourceType::SDR) {
        std::string_view mimeType = std::get<0>(params);
        ConfigureHdrVivid2Sdr(mimeType, resourceType);
        ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
        ASSERT_EQ(AV_ERR_OK, videoDec_->SetOutputSurface());
        ASSERT_EQ(AV_ERR_OK, videoDec_->Prepare());
        EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
        EXPECT_EQ(AV_ERR_INVALID_STATE, videoDec_->Prepare());
        EXPECT_EQ(AV_ERR_OK, videoDec_->Flush());
        EXPECT_EQ(AV_ERR_INVALID_STATE, videoDec_->Prepare());
        EXPECT_EQ(AV_ERR_OK, videoDec_->Reset());
        ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
        ASSERT_EQ(AV_ERR_OK, videoDec_->SetOutputSurface());
        ASSERT_EQ(AV_ERR_OK, videoDec_->Prepare());
        EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
        EXPECT_EQ(AV_ERR_OK, videoDec_->Flush());
        EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
    }
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_1201
 * @tc.desc: unordered post processing function invocation
 * @tc.type: FUNC
 */
HWTEST_P(HEVC_TEST_SUIT, VideoDecoder_HRDVivid2SDR_1201, TestSize.Level1)
{
    auto params = GetParam();
    ResourceType resourceType = std::get<1>(params);
    if (resourceType != ResourceType::SDR) {
        std::string_view mimeType = std::get<0>(params);
        ConfigureHdrVivid2Sdr(mimeType, resourceType);
        ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
        ASSERT_EQ(AV_ERR_OK, videoDec_->SetOutputSurface());
        ASSERT_EQ(AV_ERR_OK, videoDec_->Prepare());
        EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
        EXPECT_EQ(AV_ERR_OK, videoDec_->Flush());
        EXPECT_EQ(AV_ERR_OK, videoDec_->Reset());
        ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
        ASSERT_EQ(AV_ERR_OK, videoDec_->SetOutputSurface());
        ASSERT_EQ(AV_ERR_OK, videoDec_->Prepare());
        EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
        EXPECT_EQ(AV_ERR_INVALID_STATE, videoDec_->Prepare());
        EXPECT_EQ(AV_ERR_OK, videoDec_->Flush());
        EXPECT_EQ(AV_ERR_INVALID_STATE, videoDec_->Prepare());
        EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
    }
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_1211
 * @tc.desc: unordered post processing function invocation
 * @tc.type: FUNC
 */
HWTEST_P(HEVC_TEST_SUIT, VideoDecoder_HRDVivid2SDR_1211, TestSize.Level1)
{
    auto params = GetParam();
    ResourceType resourceType = std::get<1>(params);
    if (resourceType != ResourceType::SDR) {
        std::string_view mimeType = std::get<0>(params);
        ConfigureHdrVivid2Sdr(mimeType, resourceType);
        for (int i = 0; i < 3; i++) {
            ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
            ASSERT_EQ(AV_ERR_OK, videoDec_->SetOutputSurface());
            ASSERT_EQ(AV_ERR_OK, videoDec_->Prepare());
            EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
            EXPECT_EQ(AV_ERR_INVALID_STATE, videoDec_->Prepare());
            EXPECT_EQ(AV_ERR_OK, videoDec_->Flush());
            EXPECT_EQ(AV_ERR_INVALID_STATE, videoDec_->Prepare());
            EXPECT_EQ(AV_ERR_OK, videoDec_->Reset());
        }
    }
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_1221
 * @tc.desc: unordered post processing function invocation
 * @tc.type: FUNC
 */
HWTEST_P(HEVC_TEST_SUIT, VideoDecoder_HRDVivid2SDR_1221, TestSize.Level1)
{
    auto params = GetParam();
    ResourceType resourceType = std::get<1>(params);
    if (resourceType != ResourceType::SDR) {
        std::string_view mimeType = std::get<0>(params);
        ConfigureHdrVivid2Sdr(mimeType, resourceType);
        for (int i = 0; i < 3; i++) {
            ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
            ASSERT_EQ(AV_ERR_OK, videoDec_->SetOutputSurface());
            ASSERT_EQ(AV_ERR_OK, videoDec_->Prepare());
            EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
            EXPECT_EQ(AV_ERR_OK, videoDec_->Flush());
            EXPECT_EQ(AV_ERR_OK, videoDec_->Reset());
        }
    }
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_2011
 * @tc.desc: 1. key pixel format is RGBA;
 *           2. key color space is BT709_LIMIT;
 * @tc.type: FUNC
 */
HWTEST_P(HEVC_TEST_SUIT, VideoDecoder_HRDVivid2SDR_2011, TestSize.Level1)
{
    IsPixelFormatSupported(VideoPixelFormat::RGBA);
    auto params = GetParam();
    std::string_view mimeType = std::get<0>(params);
    ResourceType resourceType = std::get<1>(params);
    CreateByNameWithParam(mimeType.data());
    SetFormatWithParam(VideoPixelFormat::RGBA);
    PrepareSource(resourceType);
    format_->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
                        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_LIMIT);
    ASSERT_EQ(AV_ERR_UNSUPPORT, videoDec_->Configure(format_));
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_2021
 * @tc.desc: 1. key pixel format unset;
 *           2. key color space is BT709_LIMIT;
 * @tc.type: FUNC
 */
HWTEST_P(HEVC_TEST_SUIT, VideoDecoder_HRDVivid2SDR_2021, TestSize.Level1)
{
    auto params = GetParam();
    std::string_view mimeType = std::get<0>(params);
    ResourceType resourceType = std::get<1>(params);
    CreateByNameWithParam(mimeType.data());
    std::shared_ptr<FormatMock> format = FormatMockFactory::CreateFormat();
    format->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    PrepareSource(resourceType);
    format->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
                        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_LIMIT);
    ASSERT_EQ(AV_ERR_UNSUPPORT, videoDec_->Configure(format));
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_2031
 * @tc.desc: 1. key pixel format is NV12;
 *           2. key color space is BT709_LIMIT;
 * @tc.type: FUNC
 */
HWTEST_P(HEVC_TEST_SUIT, VideoDecoder_HRDVivid2SDR_2031, TestSize.Level1)
{
    auto params = GetParam();
    std::string_view mimeType = std::get<0>(params);
    ResourceType resourceType = std::get<1>(params);
    CreateByNameWithParam(mimeType.data());
    SetFormatWithParam(VideoPixelFormat::NV12);
    PrepareSource(resourceType);
    format_->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
                         OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_LIMIT);
    ASSERT_EQ(AV_ERR_UNSUPPORT, videoDec_->Configure(format_));
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_2041
 * @tc.desc: 1. key pixel format is RGBA;
 *           2. key color space is BT2020_HLG_LIMIT;
 * @tc.type: FUNC
 */
HWTEST_P(HEVC_TEST_SUIT, VideoDecoder_HRDVivid2SDR_2041, TestSize.Level1)
{
    IsPixelFormatSupported(VideoPixelFormat::RGBA);
    auto params = GetParam();
    std::string_view mimeType = std::get<0>(params);
    ResourceType resourceType = std::get<1>(params);
    CreateByNameWithParam(mimeType.data());
    SetFormatWithParam(VideoPixelFormat::RGBA);
    PrepareSource(resourceType);
    format_->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
                        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT2020_HLG_LIMIT);
    ASSERT_EQ(AV_ERR_VIDEO_UNSUPPORTED_COLOR_SPACE_CONVERSION, videoDec_->Configure(format_));
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_2051
 * @tc.desc: 1. key pixel format unset;
 *           2. key color space is BT2020_HLG_LIMIT;
 * @tc.type: FUNC
 */
HWTEST_P(HEVC_TEST_SUIT, VideoDecoder_HRDVivid2SDR_2051, TestSize.Level1)
{
    auto params = GetParam();
    std::string_view mimeType = std::get<0>(params);
    ResourceType resourceType = std::get<1>(params);
    CreateByNameWithParam(mimeType.data());
    std::shared_ptr<FormatMock> format = FormatMockFactory::CreateFormat();
    format->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    PrepareSource(resourceType);
    format->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
                        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT2020_HLG_LIMIT);
    ASSERT_EQ(AV_ERR_VIDEO_UNSUPPORTED_COLOR_SPACE_CONVERSION, videoDec_->Configure(format));
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_2061
 * @tc.desc: 1. key pixel format is NV12;
 *           2. key color space is BT2020_HLG_LIMIT;
 * @tc.type: FUNC
 */
HWTEST_P(HEVC_TEST_SUIT, VideoDecoder_HRDVivid2SDR_2061, TestSize.Level1)
{
    auto params = GetParam();
    std::string_view mimeType = std::get<0>(params);
    ResourceType resourceType = std::get<1>(params);
    CreateByNameWithParam(mimeType.data());
    SetFormatWithParam(VideoPixelFormat::NV12);
    PrepareSource(resourceType);
    format_->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
                         OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT2020_HLG_LIMIT);
    ASSERT_EQ(AV_ERR_VIDEO_UNSUPPORTED_COLOR_SPACE_CONVERSION, videoDec_->Configure(format_));
}
} // namespace HevcTestSuit

namespace AvcTestSuit {
class AVC_TEST_SUIT : public HdrVivid2SdrBaseSuit, public testing::TestWithParam<int32_t> {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);
    void CreateByNameWithParam(int32_t param);
    void PrepareSource(int32_t param);
    static constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK,
                                                          STRINGFY(AVC_TEST_SUIT)};
};

void AVC_TEST_SUIT::SetUpTestCase(void)
{
    auto capability = CodecListMockFactory::GetCapabilityByCategory((CodecMimeType::VIDEO_AVC).data(), false,
                                                                    AVCodecCategory::AVCODEC_HARDWARE);
    ASSERT_NE(nullptr, capability) << (CodecMimeType::VIDEO_AVC).data() << " can not found!" << std::endl;
    ISHDR2SDRSupported();
}

void AVC_TEST_SUIT::TearDownTestCase(void) {}

void AVC_TEST_SUIT::SetUp(void)
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

    const ::testing::TestInfo *testInfo_ = ::testing::UnitTest::GetInstance()->current_test_info();
    std::string testCaseName = testInfo_->name();
    AVCODEC_LOGI("%{public}s", testCaseName.c_str());
}

void AVC_TEST_SUIT::TearDown(void)
{
    if (format_ != nullptr) {
        format_->Destroy();
    }
    videoDec_ = nullptr;
}

void AVC_TEST_SUIT::CreateByNameWithParam(int32_t param)
{
    std::string codecName = "";
    switch (param) {
        case VCodecTestCode::HW_AVC:
            capability_ = CodecListMockFactory::GetCapabilityByCategory(CodecMimeType::VIDEO_AVC.data(), false,
                                                                        AVCodecCategory::AVCODEC_HARDWARE);
            break;
        case VCodecTestCode::SW_AVC:
            capability_ = CodecListMockFactory::GetCapabilityByCategory(CodecMimeType::VIDEO_AVC.data(), false,
                                                                        AVCodecCategory::AVCODEC_SOFTWARE);
            break;
        default:
            capability_ = CodecListMockFactory::GetCapabilityByCategory(CodecMimeType::VIDEO_AVC.data(), false,
                                                                        AVCodecCategory::AVCODEC_HARDWARE);
            break;
    }
    codecName = capability_->GetName();
    std::cout << "CodecName: " << codecName << "\n";
    ASSERT_TRUE(CreateVideoCodecByName(codecName));
}

void AVC_TEST_SUIT::PrepareSource(int32_t param)
{
    const ::testing::TestInfo *testInfo_ = ::testing::UnitTest::GetInstance()->current_test_info();
    string prefix = "/data/test/media/";
    string fileName = testInfo_->name();
    auto check = [](char it) { return it == '/'; };
    (void)fileName.erase(std::remove_if(fileName.begin(), fileName.end(), check), fileName.end());
    videoDec_->SetOutPath(prefix + fileName);
}

INSTANTIATE_TEST_SUITE_P(, AVC_TEST_SUIT, testing::Values(HW_AVC, SW_AVC));

/**
 * @tc.name: VideoDec_HRDVivid2SDR_001
 * @tc.desc: set invalid key of color space
 * @tc.type: FUNC
 */
HWTEST_P(AVC_TEST_SUIT, VideoDec_HRDVivid2SDR_001, TestSize.Level1)
{
    int32_t colorSpace = INT32_MIN;
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(VideoPixelFormat::NV12);
    PrepareSource(GetParam());
    format_->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE, colorSpace);
    ASSERT_EQ(AV_ERR_INVALID_VAL, videoDec_->Configure(format_));
}

/**
 * @tc.name: VideoDec_HRDVivid2SDR_002
 * @tc.desc: set invalid key of color space
 * @tc.type: FUNC
 */
HWTEST_P(AVC_TEST_SUIT, VideoDec_HRDVivid2SDR_002, TestSize.Level1)
{
    int32_t colorSpace = INT32_MAX;
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(VideoPixelFormat::NV12);
    PrepareSource(GetParam());
    format_->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE, colorSpace);
    ASSERT_EQ(AV_ERR_INVALID_VAL, videoDec_->Configure(format_));
}

/**
 * @tc.name: VideoDec_HRDVivid2SDR_003
 * @tc.desc: set key color space is BT709_LIMIT
 * @tc.type: FUNC
 */
HWTEST_P(AVC_TEST_SUIT, VideoDec_HRDVivid2SDR_003, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(VideoPixelFormat::NV12);
    PrepareSource(GetParam());
    format_->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
                         OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_LIMIT);
    ASSERT_EQ(AV_ERR_VIDEO_UNSUPPORTED_COLOR_SPACE_CONVERSION, videoDec_->Configure(format_));
}

/**
 * @tc.name: VideoDec_HRDVivid2SDR_004
 * @tc.desc: set key color space is P3_FULL
 * @tc.type: FUNC
 */
HWTEST_P(AVC_TEST_SUIT, VideoDec_HRDVivid2SDR_004, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(VideoPixelFormat::NV12);
    PrepareSource(GetParam());
    format_->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE, OH_NativeBuffer_ColorSpace::OH_COLORSPACE_P3_FULL);
    ASSERT_EQ(AV_ERR_VIDEO_UNSUPPORTED_COLOR_SPACE_CONVERSION, videoDec_->Configure(format_));
}

/**
 * @tc.name: VideoDec_HRDVivid2SDR_005
 * @tc.desc: set key color space is BT2020_HLG_LIMIT
 * @tc.type: FUNC
 */
HWTEST_P(AVC_TEST_SUIT, VideoDec_HRDVivid2SDR_005, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(VideoPixelFormat::NV12);
    PrepareSource(GetParam());
    format_->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
                         OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT2020_HLG_LIMIT);
    ASSERT_EQ(AV_ERR_VIDEO_UNSUPPORTED_COLOR_SPACE_CONVERSION, videoDec_->Configure(format_));
}
} // namespace AvcTestSuit

int main(int argc, char **argv)
{
    testing::GTEST_FLAG(output) = "xml:./";
    for (int i = 0; i < argc; ++i) {
        cout << argv[i] << endl;
        if (strcmp(argv[i], "--need_dump") == 0) {
            VideoDecSample::needDump_ = true;
            DecArgv(i, argc, argv);
        }
    }
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}