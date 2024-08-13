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
#include "meta/meta_key.h"
#include "unittest_utils.h"
#include "vdec_sample.h"
#ifdef VIDEODEC_HDRVIVID2SDR_CAPI_UNIT_TEST
#include "native_avcodec_base.h"
#include "native_avmagic.h"
#include "videodec_capi_mock.h"
#define TEST_SUIT VideoDecHDRVivid2SDRCapiTest
#else
#include "media_description.h"
#define TEST_SUIT VideoDecHDRVivid2SDRInnerTest
#endif

using namespace std;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace testing::ext;
using namespace testing::mt;
using namespace OHOS::MediaAVCodec::VCodecTestParam;

namespace {
std::string g_vdecName = "";
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
    void SetHDRFormat();
    void SetAVCFormat();
    void PrepareSource(int32_t param);
protected:
    std::shared_ptr<CodecListMock> capability_ = nullptr;
    std::shared_ptr<VideoDecSample> videoDec_ = nullptr;
    std::shared_ptr<FormatMock> format_ = nullptr;
    std::shared_ptr<VDecCallbackTest> vdecCallback_ = nullptr;
    std::shared_ptr<VDecCallbackTestExt> vdecCallbackExt_ = nullptr;
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
    if (videoDec_->isAVBufferMode_) {
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
        case VCodecTestCode::HW_HDR:
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

void TEST_SUIT::SetHDRFormat()
{
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV21));
}

void TEST_SUIT::SetAVCFormat()
{
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::RGBA));
}

#ifdef HMOS_TEST
void CheckFormatKey(std::shared_ptr<VideoDecSample> videoDec, std::shared_ptr<FormatMock> format)
{
    format = videoDec->GetOutputDescription();
    constexpr int32_t originalVideoWidth = 1280;
    constexpr int32_t originalVideoHeight = 720;
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

    EXPECT_EQ(width, originalVideoWidth);
    EXPECT_EQ(height, originalVideoHeight);
    EXPECT_GE(pictureWidth, originalVideoWidth - 1);
    EXPECT_GE(pictureHeight, originalVideoHeight - 1);
    EXPECT_GE(stride, originalVideoWidth);
    EXPECT_GE(sliceHeight, originalVideoHeight);
    EXPECT_EQ(colorSpace, OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_LIMIT);
}
#endif

INSTANTIATE_TEST_SUITE_P(, TEST_SUIT, testing::Values(HW_AVC, SW_AVC, HW_HEVC, HW_HDR));

#ifdef VIDEODEC_HDRVIVID2SDR_CAPI_UNIT_TEST
/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_Capi_001
 * @tc.desc: set invalid key
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_Capi_001, TestSize.Level1)
{
    int32_t colorSpace = INT32_MIN;
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE, colorSpace);
    ASSERT_EQ(AV_ERR_INVALID_VAL, videoDec_->Configure(format_));
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_Capi_002
 * @tc.desc: set invalid key
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_Capi_002, TestSize.Level1)
{
    int32_t colorSpace = INT32_MAX;
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE, colorSpace);
    ASSERT_EQ(AV_ERR_INVALID_VAL, videoDec_->Configure(format_));
}
#ifdef HMOS_TEST
/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_Capi_003
 * @tc.desc: 1. key pixel format unset;
 *           2. decoder mode is buffer;
 *           3. prepare function is not called;
 *           4. key color space is BT709_LIMIT
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_Capi_003, TestSize.Level1)
{
    auto testCode = GetParam();
    CreateByNameWithParam(testCode);
    std::shared_ptr<FormatMock> format = FormatMockFactory::CreateFormat();
    format->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    PrepareSource(testCode);
    format->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_LIMIT);

    if (testCode == VCodecTestCode::HW_HDR || testCode == VCodecTestCode::HW_HEVC) {
        ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format));
        ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, videoDec_->Start());
    } else {
        ASSERT_EQ(AV_ERR_VIDEO_UNSUPPORTED_COLOR_SPACE_CONVERSION, videoDec_->Configure(format));
    }
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_Capi_004
 * @tc.desc: 1. key pixel format is NV12;
 *           2. decoder mode is buffer;
 *           3. prepare function is not called;
 *           4. key color space is BT709_LIMIT
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_Capi_004, TestSize.Level1)
{
    auto testCode = GetParam();
    CreateByNameWithParam(testCode);
    SetFormatWithParam(testCode);
    PrepareSource(testCode);
    format_->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_LIMIT);

    if (testCode == VCodecTestCode::HW_HDR || testCode == VCodecTestCode::HW_HEVC) {
        ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
        ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, videoDec_->Start());
    } else {
        ASSERT_EQ(AV_ERR_VIDEO_UNSUPPORTED_COLOR_SPACE_CONVERSION, videoDec_->Configure(format_));
    }
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_Capi_005
 * @tc.desc: 1. key pixel format unset;
 *           2. decoder mode is surface;
 *           3. prepare function is not called;
 *           4. key color space is BT709_LIMIT
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_Capi_005, TestSize.Level1)
{
    auto testCode = GetParam();
    CreateByNameWithParam(testCode);
    std::shared_ptr<FormatMock> format = FormatMockFactory::CreateFormat();
    format->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    PrepareSource(testCode);
    format->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_LIMIT);

    if (testCode == VCodecTestCode::HW_HDR || testCode == VCodecTestCode::HW_HEVC) {
        ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format));
        ASSERT_EQ(AV_ERR_OK, videoDec_->SetOutputSurface());
        ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, videoDec_->Start());
    } else {
        ASSERT_EQ(AV_ERR_VIDEO_UNSUPPORTED_COLOR_SPACE_CONVERSION, videoDec_->Configure(format));
    }
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_Capi_006
 * @tc.desc: 1. key pixel format is NV12;
 *           2. decoder mode is surface;
 *           3. prepare function is not called;
 *           4. key color space is BT709_LIMIT
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_Capi_006, TestSize.Level1)
{
    auto testCode = GetParam();
    CreateByNameWithParam(testCode);
    SetFormatWithParam(testCode);
    PrepareSource(testCode);
    format_->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_LIMIT);

    if (testCode == VCodecTestCode::HW_HDR || testCode == VCodecTestCode::HW_HEVC) {
        ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
        ASSERT_EQ(AV_ERR_OK, videoDec_->SetOutputSurface());
        ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, videoDec_->Start());
    } else {
        ASSERT_EQ(AV_ERR_VIDEO_UNSUPPORTED_COLOR_SPACE_CONVERSION, videoDec_->Configure(format_));
    }
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_Capi_007
 * @tc.desc: 1. key pixel format unset;
 *           2. decoder mode is buffer;
 *           3. prepare function is called before start function;
 *           4. key color space is BT709_LIMIT
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_Capi_007, TestSize.Level1)
{
    auto testCode = GetParam();
    CreateByNameWithParam(testCode);
    std::shared_ptr<FormatMock> format = FormatMockFactory::CreateFormat();
    format->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    PrepareSource(testCode);
    format->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_LIMIT);

    if (testCode == VCodecTestCode::HW_HDR || testCode == VCodecTestCode::HW_HEVC) {
        ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format));
        ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, videoDec_->Prepare());
    } else {
        ASSERT_EQ(AV_ERR_VIDEO_UNSUPPORTED_COLOR_SPACE_CONVERSION, videoDec_->Configure(format));
    }
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_Capi_008
 * @tc.desc: 1. key pixel format is NV12;
 *           2. decoder mode is buffer;
 *           3. prepare function is called before start function;
 *           4. key color space is BT709_LIMIT
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_Capi_008, TestSize.Level1)
{
    auto testCode = GetParam();
    CreateByNameWithParam(testCode);
    SetFormatWithParam(testCode);
    PrepareSource(testCode);
    format_->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_LIMIT);

    if (testCode == VCodecTestCode::HW_HDR || testCode == VCodecTestCode::HW_HEVC) {
        ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
        ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, videoDec_->Prepare());
    } else {
        ASSERT_EQ(AV_ERR_VIDEO_UNSUPPORTED_COLOR_SPACE_CONVERSION, videoDec_->Configure(format_));
    }
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_Capi_009
 * @tc.desc: 1. key pixel format unset;
 *           2. decoder mode is surface;
 *           3. prepare function is called before start function;
 *           4. key color space is BT709_LIMIT
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_Capi_009, TestSize.Level1)
{
    auto testCode = GetParam();
    CreateByNameWithParam(testCode);
    std::shared_ptr<FormatMock> format = FormatMockFactory::CreateFormat();
    format->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    PrepareSource(testCode);
    format->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_LIMIT);

    if (testCode == VCodecTestCode::HW_HDR) {
        ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format));
        ASSERT_EQ(AV_ERR_OK, videoDec_->SetOutputSurface());
        ASSERT_EQ(AV_ERR_OK, videoDec_->Prepare());
        EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
        CheckFormatKey(videoDec_, format);
        EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
    } else if (testCode == VCodecTestCode::HW_AVC || testCode == VCodecTestCode::SW_AVC) {
        ASSERT_EQ(AV_ERR_VIDEO_UNSUPPORTED_COLOR_SPACE_CONVERSION, videoDec_->Configure(format));
    }
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_Capi_010
 * @tc.desc: 1. key pixel format is NV12;
 *           2. decoder mode is surface;
 *           3. prepare function is called before start function;
 *           4. key color space is BT709_LIMIT
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_Capi_010, TestSize.Level1)
{
    auto testCode = GetParam();
    CreateByNameWithParam(testCode);
    SetFormatWithParam(testCode);
    PrepareSource(testCode);
    format_->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_LIMIT);

    if (testCode == VCodecTestCode::HW_HDR) {
        ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
        ASSERT_EQ(AV_ERR_OK, videoDec_->SetOutputSurface());
        ASSERT_EQ(AV_ERR_OK, videoDec_->Prepare());
        EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
        CheckFormatKey(videoDec_, format_);
        EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
    } else if (testCode == VCodecTestCode::HW_AVC || testCode == VCodecTestCode::SW_AVC) {
        ASSERT_EQ(AV_ERR_VIDEO_UNSUPPORTED_COLOR_SPACE_CONVERSION, videoDec_->Configure(format_));
    }
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_Capi_011
 * @tc.desc: 1. key pixel format is NV12;
 *           2. key color space is BT2020_HLG_LIMIT
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_Capi_011, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT2020_HLG_LIMIT);

    ASSERT_EQ(AV_ERR_VIDEO_UNSUPPORTED_COLOR_SPACE_CONVERSION, videoDec_->Configure(format_));
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_Capi_012
 * @tc.desc: 1. key pixel format unset;
 *           2. key color space is BT2020_HLG_LIMIT
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_Capi_012, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    std::shared_ptr<FormatMock> format = FormatMockFactory::CreateFormat();
    format->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    PrepareSource(GetParam());
    format->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT2020_HLG_LIMIT);

    ASSERT_EQ(AV_ERR_VIDEO_UNSUPPORTED_COLOR_SPACE_CONVERSION, videoDec_->Configure(format));
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_Capi_013
 * @tc.desc: 1. key pixel format is NV21;
 *           2. decoder mode is buffer;
 *           3. prepare function is not called;
 *           4. key color space is BT709_LIMIT
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_Capi_013, TestSize.Level1)
{
    auto testCode = GetParam();
    if (testCode == VCodecTestCode::HW_HDR || testCode == VCodecTestCode::HW_HEVC) {
        CreateByNameWithParam(testCode);
        SetHDRFormat();
        PrepareSource(testCode);
        format_->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
            OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_LIMIT);

        ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
        ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, videoDec_->Start());
    }
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_Capi_014
 * @tc.desc: 1. key pixel format is NV21;
 *           2. decoder mode is surface;
 *           3. prepare function is not called;
 *           4. key color space is BT709_LIMIT
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_Capi_014, TestSize.Level1)
{
    auto testCode = GetParam();
    if (testCode == VCodecTestCode::HW_HDR || testCode == VCodecTestCode::HW_HEVC) {
        CreateByNameWithParam(testCode);
        SetHDRFormat();
        PrepareSource(testCode);
        format_->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
            OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_LIMIT);

        ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
        ASSERT_EQ(AV_ERR_OK, videoDec_->SetOutputSurface());
        ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, videoDec_->Start());
    }
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_Capi_015
 * @tc.desc: 1. key pixel format is NV21;
 *           2. decoder mode is buffer;
 *           3. prepare function is called before start function;
 *           4. key color space is BT709_LIMIT
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_Capi_015, TestSize.Level1)
{
    auto testCode = GetParam();
    if (testCode == VCodecTestCode::HW_HDR || testCode == VCodecTestCode::HW_HEVC) {
        CreateByNameWithParam(testCode);
        SetHDRFormat();
        PrepareSource(testCode);
        format_->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
            OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_LIMIT);

        ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
        ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, videoDec_->Prepare());
    }
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_Capi_016
 * @tc.desc: 1. key pixel format is NV21;
 *           2. decoder mode is surface;
 *           3. prepare function is called before start function;
 *           4. key color space is BT709_LIMIT
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_Capi_016, TestSize.Level1)
{
    auto testCode = GetParam();
    if (testCode == VCodecTestCode::HW_HDR) {
        CreateByNameWithParam(testCode);
        SetHDRFormat();
        PrepareSource(testCode);
        format_->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
            OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_LIMIT);

        ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
        ASSERT_EQ(AV_ERR_OK, videoDec_->SetOutputSurface());
        ASSERT_EQ(AV_ERR_OK, videoDec_->Prepare());
        EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
        CheckFormatKey(videoDec_, format_);
        EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
    }
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_Capi_017
 * @tc.desc: 1. key pixel format is NV21;
 *           2. key color space is BT2020_HLG_LIMIT
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_Capi_017, TestSize.Level1)
{
    auto testCode = GetParam();
    if (testCode == VCodecTestCode::HW_HDR || testCode == VCodecTestCode::HW_HEVC) {
        CreateByNameWithParam(testCode);
        SetHDRFormat();
        PrepareSource(testCode);
        format_->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
            OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT2020_HLG_LIMIT);

        ASSERT_EQ(AV_ERR_VIDEO_UNSUPPORTED_COLOR_SPACE_CONVERSION, videoDec_->Configure(format_));
    }
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_Capi_024
 * @tc.desc: 1. key pixel format is NV12;
 *           2. key color space is BT709_LIMIT
 *           3. decoder mode is surface;
 *           4. prepare function is called before start function;
 *           5. start -> flush -> stop
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_Capi_024, TestSize.Level1)
{
    auto testCode = GetParam();
    if (testCode == VCodecTestCode::HW_HDR) {
        CreateByNameWithParam(testCode);
        SetFormatWithParam(testCode);
        PrepareSource(testCode);
        format_->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
            OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_LIMIT);

        ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
        ASSERT_EQ(AV_ERR_OK, videoDec_->SetOutputSurface());
        ASSERT_EQ(AV_ERR_OK, videoDec_->Prepare());
        EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
        EXPECT_EQ(AV_ERR_OK, videoDec_->Flush());
        EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
    }
}
#else
/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_Capi_018
 * @tc.desc: 1. key pixel format is RGBA;
 *           2. key color space is BT709_LIMIT
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_Capi_018, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetAVCFormat();
    PrepareSource(GetParam());
    format_->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_LIMIT);

    ASSERT_EQ(AV_ERR_VIDEO_UNSUPPORTED_COLOR_SPACE_CONVERSION, videoDec_->Configure(format_));
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_Capi_019
 * @tc.desc: 1. key pixel format unset;
 *           2. key color space is BT709_LIMIT
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_Capi_019, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    std::shared_ptr<FormatMock> format = FormatMockFactory::CreateFormat();
    format->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    PrepareSource(GetParam());
    format->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_LIMIT);

    ASSERT_EQ(AV_ERR_VIDEO_UNSUPPORTED_COLOR_SPACE_CONVERSION, videoDec_->Configure(format));
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_Capi_020
 * @tc.desc: 1. key pixel format is NV12;
 *           2. key color space is BT2020_HLG_LIMIT
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_Capi_020, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_LIMIT);

    ASSERT_EQ(AV_ERR_VIDEO_UNSUPPORTED_COLOR_SPACE_CONVERSION, videoDec_->Configure(format_));
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_Capi_021
 * @tc.desc: 1. key pixel format is RGBA;
 *           2. key color space is BT2020_HLG_LIMIT
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_Capi_021, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetAVCFormat();
    PrepareSource(GetParam());
    format_->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT2020_HLG_LIMIT);

    ASSERT_EQ(AV_ERR_VIDEO_UNSUPPORTED_COLOR_SPACE_CONVERSION, videoDec_->Configure(format_));
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_Capi_022
 * @tc.desc: 1. key pixel format unset;
 *           2. key color space is BT2020_HLG_LIMIT
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_Capi_022, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    std::shared_ptr<FormatMock> format = FormatMockFactory::CreateFormat();
    format->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    PrepareSource(GetParam());
    format->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT2020_HLG_LIMIT);

    ASSERT_EQ(AV_ERR_VIDEO_UNSUPPORTED_COLOR_SPACE_CONVERSION, videoDec_->Configure(format));
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_Capi_023
 * @tc.desc: 1. key pixel format is NV12;
 *           2. key color space is BT2020_HLG_LIMIT
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_Capi_023, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT2020_HLG_LIMIT);

    ASSERT_EQ(AV_ERR_VIDEO_UNSUPPORTED_COLOR_SPACE_CONVERSION, videoDec_->Configure(format_));
}
#endif // HMOS_TEST
#else
/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_Inner_001
 * @tc.desc: set invalid key
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_Inner_001, TestSize.Level1)
{
    int32_t colorSpace = INT32_MIN;
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE, colorSpace);
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, videoDec_->Configure(format_));
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_Inner_002
 * @tc.desc: set invalid key
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_Inner_002, TestSize.Level1)
{
    int32_t colorSpace = INT32_MAX;
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE, colorSpace);
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, videoDec_->Configure(format_));
}
#ifdef HMOS_TEST
/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_Inner_003
 * @tc.desc: 1. key pixel format unset;
 *           2. decoder mode is buffer;
 *           3. prepare function is not called;
 *           4. key color space is BT709_LIMIT
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_Inner_003, TestSize.Level1)
{
    auto testCode = GetParam();
    CreateByNameWithParam(testCode);
    std::shared_ptr<FormatMock> format = FormatMockFactory::CreateFormat();
    format->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    PrepareSource(testCode);
    format->PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_LIMIT);

    if (testCode == VCodecTestCode::HW_HDR || testCode == VCodecTestCode::HW_HEVC) {
        ASSERT_EQ(AVCS_ERR_OK, videoDec_->Configure(format));
        ASSERT_EQ(AVCS_ERR_INVALID_OPERATION, videoDec_->Start());
    } else {
        ASSERT_EQ(AVCS_ERR_VIDEO_UNSUPPORT_COLOR_SPACE_CONVERSION, videoDec_->Configure(format));
    }
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_Inner_004
 * @tc.desc: 1. key pixel format is NV12;
 *           2. decoder mode is buffer;
 *           3. prepare function is not called;
 *           4. key color space is BT709_LIMIT
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_Inner_004, TestSize.Level1)
{
    auto testCode = GetParam();
    CreateByNameWithParam(testCode);
    SetFormatWithParam(testCode);
    PrepareSource(testCode);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_LIMIT);

    if (testCode == VCodecTestCode::HW_HDR || testCode == VCodecTestCode::HW_HEVC) {
        ASSERT_EQ(AVCS_ERR_OK, videoDec_->Configure(format_));
        ASSERT_EQ(AVCS_ERR_INVALID_OPERATION, videoDec_->Start());
    } else {
        ASSERT_EQ(AVCS_ERR_VIDEO_UNSUPPORT_COLOR_SPACE_CONVERSION, videoDec_->Configure(format_));
    }
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_Inner_005
 * @tc.desc: 1. key pixel format unset;
 *           2. decoder mode is surface;
 *           3. prepare function is not called;
 *           4. key color space is BT709_LIMIT
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_Inner_005, TestSize.Level1)
{
    auto testCode = GetParam();
    CreateByNameWithParam(testCode);
    std::shared_ptr<FormatMock> format = FormatMockFactory::CreateFormat();
    format->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    PrepareSource(testCode);
    format->PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_LIMIT);

    if (testCode == VCodecTestCode::HW_HDR || testCode == VCodecTestCode::HW_HEVC) {
        ASSERT_EQ(AVCS_ERR_OK, videoDec_->Configure(format));
        ASSERT_EQ(AVCS_ERR_OK, videoDec_->SetOutputSurface());
        ASSERT_EQ(AVCS_ERR_INVALID_OPERATION, videoDec_->Start());
    } else {
        ASSERT_EQ(AVCS_ERR_VIDEO_UNSUPPORT_COLOR_SPACE_CONVERSION, videoDec_->Configure(format));
    }
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_Inner_006
 * @tc.desc: 1. key pixel format is NV12;
 *           2. decoder mode is surface;
 *           3. prepare function is not called;
 *           4. key color space is BT709_LIMIT
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_Inner_006, TestSize.Level1)
{
    auto testCode = GetParam();
    CreateByNameWithParam(testCode);
    SetFormatWithParam(testCode);
    PrepareSource(testCode);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_LIMIT);

    if (testCode == VCodecTestCode::HW_HDR || testCode == VCodecTestCode::HW_HEVC) {
        ASSERT_EQ(AVCS_ERR_OK, videoDec_->Configure(format_));
        ASSERT_EQ(AVCS_ERR_OK, videoDec_->SetOutputSurface());
        ASSERT_EQ(AVCS_ERR_INVALID_OPERATION, videoDec_->Start());
    } else {
        ASSERT_EQ(AVCS_ERR_VIDEO_UNSUPPORT_COLOR_SPACE_CONVERSION, videoDec_->Configure(format_));
    }
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_Inner_007
 * @tc.desc: 1. key pixel format unset;
 *           2. decoder mode is buffer;
 *           3. prepare function is called before start function;
 *           4. key color space is BT709_LIMIT
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_Inner_007, TestSize.Level1)
{
    auto testCode = GetParam();
    CreateByNameWithParam(testCode);
    std::shared_ptr<FormatMock> format = FormatMockFactory::CreateFormat();
    format->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    PrepareSource(testCode);
    format->PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_LIMIT);

    if (testCode == VCodecTestCode::HW_HDR || testCode == VCodecTestCode::HW_HEVC) {
        ASSERT_EQ(AVCS_ERR_OK, videoDec_->Configure(format));
        ASSERT_EQ(AVCS_ERR_INVALID_OPERATION, videoDec_->Prepare());
    } else {
        ASSERT_EQ(AVCS_ERR_VIDEO_UNSUPPORT_COLOR_SPACE_CONVERSION, videoDec_->Configure(format));
    }
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_Inner_008
 * @tc.desc: 1. key pixel format is NV12;
 *           2. decoder mode is buffer;
 *           3. prepare function is called before start function;
 *           4. key color space is BT709_LIMIT
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_Inner_008, TestSize.Level1)
{
    auto testCode = GetParam();
    CreateByNameWithParam(testCode);
    SetFormatWithParam(testCode);
    PrepareSource(testCode);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_LIMIT);

    if (testCode == VCodecTestCode::HW_HDR || testCode == VCodecTestCode::HW_HEVC) {
        ASSERT_EQ(AVCS_ERR_OK, videoDec_->Configure(format_));
        ASSERT_EQ(AVCS_ERR_INVALID_OPERATION, videoDec_->Prepare());
    } else {
        ASSERT_EQ(AVCS_ERR_VIDEO_UNSUPPORT_COLOR_SPACE_CONVERSION, videoDec_->Configure(format_));
    }
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_Inner_009
 * @tc.desc: 1. key pixel format unset;
 *           2. decoder mode is surface;
 *           3. prepare function is called before start function;
 *           4. key color space is BT709_LIMIT
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_Inner_009, TestSize.Level1)
{
    auto testCode = GetParam();
    CreateByNameWithParam(testCode);
    std::shared_ptr<FormatMock> format = FormatMockFactory::CreateFormat();
    format->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    PrepareSource(testCode);
    format->PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_LIMIT);

    if (testCode == VCodecTestCode::HW_HDR) {
        ASSERT_EQ(AVCS_ERR_OK, videoDec_->Configure(format));
        ASSERT_EQ(AVCS_ERR_OK, videoDec_->SetOutputSurface());
        ASSERT_EQ(AVCS_ERR_OK, videoDec_->Prepare());
        EXPECT_EQ(AVCS_ERR_OK, videoDec_->Start());
        CheckFormatKey(videoDec_, format);
        EXPECT_EQ(AVCS_ERR_OK, videoDec_->Stop());
    } else if (testCode == VCodecTestCode::HW_AVC || testCode == VCodecTestCode::SW_AVC) {
        ASSERT_EQ(AVCS_ERR_VIDEO_UNSUPPORT_COLOR_SPACE_CONVERSION, videoDec_->Configure(format));
    }
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_Inner_010
 * @tc.desc: 1. key pixel format is NV12;
 *           2. decoder mode is surface;
 *           3. prepare function is called before start function;
 *           4. key color space is BT709_LIMIT
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_Inner_010, TestSize.Level1)
{
    auto testCode = GetParam();
    CreateByNameWithParam(testCode);
    SetFormatWithParam(testCode);
    PrepareSource(testCode);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_LIMIT);

    if (testCode == VCodecTestCode::HW_HDR) {
        ASSERT_EQ(AVCS_ERR_OK, videoDec_->Configure(format_));
        ASSERT_EQ(AVCS_ERR_OK, videoDec_->SetOutputSurface());
        ASSERT_EQ(AVCS_ERR_OK, videoDec_->Prepare());
        EXPECT_EQ(AVCS_ERR_OK, videoDec_->Start());
        CheckFormatKey(videoDec_, format_);
        EXPECT_EQ(AVCS_ERR_OK, videoDec_->Stop());
    } else if (testCode == VCodecTestCode::HW_AVC || testCode == VCodecTestCode::SW_AVC) {
        ASSERT_EQ(AVCS_ERR_VIDEO_UNSUPPORT_COLOR_SPACE_CONVERSION, videoDec_->Configure(format_));
    }
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_Inner_011
 * @tc.desc: 1. key pixel format is NV12;
 *           2. key color space is BT2020_HLG_LIMIT
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_Inner_011, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT2020_HLG_LIMIT);

    ASSERT_EQ(AVCS_ERR_VIDEO_UNSUPPORT_COLOR_SPACE_CONVERSION, videoDec_->Configure(format_));
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_Inner_012
 * @tc.desc: 1. key pixel format unset;
 *           2. key color space is BT2020_HLG_LIMIT
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_Inner_012, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    std::shared_ptr<FormatMock> format = FormatMockFactory::CreateFormat();
    format->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    PrepareSource(GetParam());
    format->PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT2020_HLG_LIMIT);

    ASSERT_EQ(AVCS_ERR_VIDEO_UNSUPPORT_COLOR_SPACE_CONVERSION, videoDec_->Configure(format));
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_Inner_013
 * @tc.desc: 1. key pixel format is NV21;
 *           2. decoder mode is buffer;
 *           3. prepare function is not called;
 *           4. key color space is BT709_LIMIT
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_Inner_013, TestSize.Level1)
{
    auto testCode = GetParam();
    if (testCode == VCodecTestCode::HW_HDR || testCode == VCodecTestCode::HW_HEVC) {
        CreateByNameWithParam(testCode);
        SetHDRFormat();
        PrepareSource(testCode);
        format_->PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
            OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_LIMIT);

        ASSERT_EQ(AVCS_ERR_OK, videoDec_->Configure(format_));
        ASSERT_EQ(AVCS_ERR_INVALID_OPERATION, videoDec_->Start());
    }
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_Inner_014
 * @tc.desc: 1. key pixel format is NV21;
 *           2. decoder mode is surface;
 *           3. prepare function is not called;
 *           4. key color space is BT709_LIMIT
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_Inner_014, TestSize.Level1)
{
    auto testCode = GetParam();
    if (testCode == VCodecTestCode::HW_HDR || testCode == VCodecTestCode::HW_HEVC) {
        CreateByNameWithParam(testCode);
        SetHDRFormat();
        PrepareSource(testCode);
        format_->PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
            OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_LIMIT);

        ASSERT_EQ(AVCS_ERR_OK, videoDec_->Configure(format_));
        ASSERT_EQ(AVCS_ERR_OK, videoDec_->SetOutputSurface());
        ASSERT_EQ(AVCS_ERR_INVALID_OPERATION, videoDec_->Start());
    }
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_Inner_015
 * @tc.desc: 1. key pixel format is NV21;
 *           2. decoder mode is buffer;
 *           3. prepare function is called before start function;
 *           4. key color space is BT709_LIMIT
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_Inner_015, TestSize.Level1)
{
    auto testCode = GetParam();
    if (testCode == VCodecTestCode::HW_HDR || testCode == VCodecTestCode::HW_HEVC) {
        CreateByNameWithParam(testCode);
        SetHDRFormat();
        PrepareSource(testCode);
        format_->PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
            OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_LIMIT);

        ASSERT_EQ(AVCS_ERR_OK, videoDec_->Configure(format_));
        ASSERT_EQ(AVCS_ERR_INVALID_OPERATION, videoDec_->Prepare());
    }
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_Inner_016
 * @tc.desc: 1. key pixel format is NV21;
 *           2. decoder mode is surface;
 *           3. prepare function is called before start function;
 *           4. key color space is BT709_LIMIT
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_Inner_016, TestSize.Level1)
{
    auto testCode = GetParam();
    if (testCode == VCodecTestCode::HW_HDR) {
        CreateByNameWithParam(testCode);
        SetHDRFormat();
        PrepareSource(testCode);
        format_->PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
            OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_LIMIT);

        ASSERT_EQ(AVCS_ERR_OK, videoDec_->Configure(format_));
        ASSERT_EQ(AVCS_ERR_OK, videoDec_->SetOutputSurface());
        ASSERT_EQ(AVCS_ERR_OK, videoDec_->Prepare());
        EXPECT_EQ(AVCS_ERR_OK, videoDec_->Start());
        CheckFormatKey(videoDec_, format_);
        EXPECT_EQ(AVCS_ERR_OK, videoDec_->Stop());
    }
}


/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_Inner_017
 * @tc.desc: 1. key pixel format is NV21;
 *           2. key color space is BT2020_HLG_LIMIT
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_Inner_017, TestSize.Level1)
{
    auto testCode = GetParam();
    if (testCode == VCodecTestCode::HW_HDR || testCode == VCodecTestCode::HW_HEVC) {
        CreateByNameWithParam(testCode);
        SetHDRFormat();
        PrepareSource(testCode);
        format_->PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
            OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT2020_HLG_LIMIT);

        ASSERT_EQ(AVCS_ERR_VIDEO_UNSUPPORT_COLOR_SPACE_CONVERSION, videoDec_->Configure(format_));
    }
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_Inner_024
 * @tc.desc: 1. key pixel format is NV12;
 *           2. key color space is BT709_LIMIT
 *           3. decoder mode is surface;
 *           4. prepare function is called before start function;
 *           5. start -> flush -> stop
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_Inner_024, TestSize.Level1)
{
    auto testCode = GetParam();
    if (testCode == VCodecTestCode::HW_HDR) {
        CreateByNameWithParam(testCode);
        SetFormatWithParam(testCode);
        PrepareSource(testCode);
        format_->PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
            OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_LIMIT);

        ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
        ASSERT_EQ(AV_ERR_OK, videoDec_->SetOutputSurface());
        ASSERT_EQ(AV_ERR_OK, videoDec_->Prepare());
        EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
        EXPECT_EQ(AV_ERR_OK, videoDec_->Flush());
        EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
    }
}
#else
/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_Inner_018
 * @tc.desc: 1. key pixel format is RGBA;
 *           2. key color space is BT709_LIMIT
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_Inner_018, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetAVCFormat();
    PrepareSource(GetParam());
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_LIMIT);

    ASSERT_EQ(AVCS_ERR_VIDEO_UNSUPPORT_COLOR_SPACE_CONVERSION, videoDec_->Configure(format_));
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_Inner_019
 * @tc.desc: 1. key pixel format unset;
 *           2. key color space is BT709_LIMIT
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_Inner_019, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    std::shared_ptr<FormatMock> format = FormatMockFactory::CreateFormat();
    format->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    PrepareSource(GetParam());
    format->PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_LIMIT);

    ASSERT_EQ(AVCS_ERR_VIDEO_UNSUPPORT_COLOR_SPACE_CONVERSION, videoDec_->Configure(format));
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_Inner_020
 * @tc.desc: 1. key pixel format is NV12;
 *           2. key color space is BT2020_HLG_LIMIT
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_Inner_020, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT2020_HLG_LIMIT);

    ASSERT_EQ(AVCS_ERR_VIDEO_UNSUPPORT_COLOR_SPACE_CONVERSION, videoDec_->Configure(format_));
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_Inner_021
 * @tc.desc: 1. key pixel format is RGBA;
 *           2. key color space is BT2020_HLG_LIMIT
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_Inner_021, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetAVCFormat();
    PrepareSource(GetParam());
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT2020_HLG_LIMIT);

    ASSERT_EQ(AVCS_ERR_VIDEO_UNSUPPORT_COLOR_SPACE_CONVERSION, videoDec_->Configure(format_));
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_Inner_022
 * @tc.desc: 1. key pixel format unset;
 *           2. key color space is BT2020_HLG_LIMIT
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_Inner_022, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    std::shared_ptr<FormatMock> format = FormatMockFactory::CreateFormat();
    format->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    PrepareSource(GetParam());
    format->PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT2020_HLG_LIMIT);

    ASSERT_EQ(AVCS_ERR_VIDEO_UNSUPPORT_COLOR_SPACE_CONVERSION, videoDec_->Configure(format));
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_Inner_023
 * @tc.desc: 1. key pixel format is NV12;
 *           2. key color space is BT2020_HLG_LIMIT
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_Inner_023, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT2020_HLG_LIMIT);

    ASSERT_EQ(AVCS_ERR_VIDEO_UNSUPPORT_COLOR_SPACE_CONVERSION, videoDec_->Configure(format_));
}
#endif // HMOS_TEST
#endif // VIDEODEC_HDRVIVID2SDR_CAPI_UNIT_TEST
} // namespace

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