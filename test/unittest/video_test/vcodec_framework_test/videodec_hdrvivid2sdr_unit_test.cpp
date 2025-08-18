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
#include "avcodec_log.h"
#include "meta/meta_key.h"
#include "native_avcodec_base.h"
#include "native_avmagic.h"
#include "unittest_utils.h"
#include "videodec_capi_mock.h"

#ifdef VIDEODEC_ASYNC_UNIT_TEST
#include "vdec_async_sample.h"
#else
#include "vdec_sync_sample.h"
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
    void ConfigureHdrVivid2Sdr(int32_t testCode);
    static constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, STRINGFY(TEST_SUIT)};

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

    const ::testing::TestInfo *testInfo_ = ::testing::UnitTest::GetInstance()->current_test_info();
    std::string testCaseName = testInfo_->name();
    AVCODEC_LOGI("%{public}s", testCaseName.c_str());
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
        case VCodecTestCode::HW_HDR_HLG_FULL:
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
    if (param == VCodecTestCode::HW_HEVC || param == VCodecTestCode::HW_HDR_HLG_FULL) {
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
void CheckFormatKey(std::shared_ptr<FormatMock> format)
{
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

void CheckFormatKeyForP3Full(std::shared_ptr<VideoDecSample> videoDec, std::shared_ptr<FormatMock> format)
{
    format = videoDec->GetOutputDescription();
    constexpr int32_t originalVideoWidth = 1920;
    constexpr int32_t originalVideoHeight = 1440;
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
    EXPECT_EQ(colorSpace, OH_NativeBuffer_ColorSpace::OH_COLORSPACE_P3_FULL);
}

void TEST_SUIT::ConfigureHdrVivid2Sdr(int32_t testCode)
{
    CreateByNameWithParam(testCode);
    SetFormatWithParam(testCode);
    PrepareSource(testCode);
    if (testCode == VCodecTestCode::HW_HDR_HLG_FULL) {
        format_->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
            OH_NativeBuffer_ColorSpace::OH_COLORSPACE_P3_FULL);
    } else {
        format_->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
            OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_LIMIT);
    }
}
#endif

INSTANTIATE_TEST_SUITE_P(, TEST_SUIT, testing::Values(HW_AVC, SW_AVC, HW_HEVC, HW_HDR, HW_HDR_HLG_FULL));

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_0011
 * @tc.desc: set invalid key
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_0011, TestSize.Level1)
{
    int32_t colorSpace = INT32_MIN;
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE, colorSpace);
    ASSERT_EQ(AV_ERR_INVALID_VAL, videoDec_->Configure(format_));
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_0021
 * @tc.desc: set invalid key
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_0021, TestSize.Level1)
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
 * @tc.name: VideoDecoder_HRDVivid2SDR_0031
 * @tc.desc: 1. key pixel format unset;
 *           2. decoder mode is buffer;
 *           3. prepare function is not called;
 *           4. key color space is BT709_LIMIT
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_0031, TestSize.Level1)
{
    auto testCode = GetParam();
    CreateByNameWithParam(testCode);
    std::shared_ptr<FormatMock> format = FormatMockFactory::CreateFormat();
    format->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    PrepareSource(testCode);
    format->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_LIMIT);

    if (testCode == VCodecTestCode::HW_HDR || testCode == VCodecTestCode::HW_HEVC ||
        testCode == VCodecTestCode::HW_HDR_HLG_FULL) {
        ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format));
        ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, videoDec_->Start());
    } else {
        ASSERT_EQ(AV_ERR_VIDEO_UNSUPPORTED_COLOR_SPACE_CONVERSION, videoDec_->Configure(format));
    }
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_0032
 * @tc.desc: 1. key pixel format unset;
 *           2. decoder mode is buffer;
 *           3. prepare function is not called;
 *           4. key color space is OH_COLORSPACE_P3_FULL
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_0032, TestSize.Level1)
{
    auto testCode = GetParam();
    CreateByNameWithParam(testCode);
    std::shared_ptr<FormatMock> format = FormatMockFactory::CreateFormat();
    format->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    PrepareSource(testCode);
    format->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_P3_FULL);

    if (testCode == VCodecTestCode::HW_HDR || testCode == VCodecTestCode::HW_HEVC ||
        testCode == VCodecTestCode::HW_HDR_HLG_FULL) {
        ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format));
        ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, videoDec_->Start());
    } else {
        ASSERT_EQ(AV_ERR_VIDEO_UNSUPPORTED_COLOR_SPACE_CONVERSION, videoDec_->Configure(format));
    }
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_0041
 * @tc.desc: 1. key pixel format is NV12;
 *           2. decoder mode is buffer;
 *           3. prepare function is not called;
 *           4. key color space is BT709_LIMIT
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_0041, TestSize.Level1)
{
    auto testCode = GetParam();
    ConfigureHdrVivid2Sdr(testCode);

    if (testCode == VCodecTestCode::HW_HDR || testCode == VCodecTestCode::HW_HEVC ||
        testCode == VCodecTestCode::HW_HDR_HLG_FULL) {
        ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
        ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, videoDec_->Start());
    } else {
        ASSERT_EQ(AV_ERR_VIDEO_UNSUPPORTED_COLOR_SPACE_CONVERSION, videoDec_->Configure(format_));
    }
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_0051
 * @tc.desc: 1. key pixel format unset;
 *           2. decoder mode is surface;
 *           3. prepare function is not called;
 *           4. key color space is BT709_LIMIT
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_0051, TestSize.Level1)
{
    auto testCode = GetParam();
    CreateByNameWithParam(testCode);
    std::shared_ptr<FormatMock> format = FormatMockFactory::CreateFormat();
    format->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    PrepareSource(testCode);
    format->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_LIMIT);

    if (testCode == VCodecTestCode::HW_HDR || testCode == VCodecTestCode::HW_HEVC ||
        testCode == VCodecTestCode::HW_HDR_HLG_FULL) {
        ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format));
        ASSERT_EQ(AV_ERR_OK, videoDec_->SetOutputSurface());
        ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, videoDec_->Start());
    } else {
        ASSERT_EQ(AV_ERR_VIDEO_UNSUPPORTED_COLOR_SPACE_CONVERSION, videoDec_->Configure(format));
    }
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_0052
 * @tc.desc: 1. key pixel format unset;
 *           2. decoder mode is surface;
 *           3. prepare function is not called;
 *           4. key color space is OH_COLORSPACE_P3_FULL
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_0052, TestSize.Level1)
{
    auto testCode = GetParam();
    CreateByNameWithParam(testCode);
    std::shared_ptr<FormatMock> format = FormatMockFactory::CreateFormat();
    format->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    PrepareSource(testCode);
    format->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_P3_FULL);

    if (testCode == VCodecTestCode::HW_HDR || testCode == VCodecTestCode::HW_HEVC ||
        testCode == VCodecTestCode::HW_HDR_HLG_FULL) {
        ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format));
        ASSERT_EQ(AV_ERR_OK, videoDec_->SetOutputSurface());
        ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, videoDec_->Start());
    } else {
        ASSERT_EQ(AV_ERR_VIDEO_UNSUPPORTED_COLOR_SPACE_CONVERSION, videoDec_->Configure(format));
    }
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_0061
 * @tc.desc: 1. key pixel format is NV12;
 *           2. decoder mode is surface;
 *           3. prepare function is not called;
 *           4. key color space is BT709_LIMIT
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_0061, TestSize.Level1)
{
    auto testCode = GetParam();
    ConfigureHdrVivid2Sdr(testCode);

    if (testCode == VCodecTestCode::HW_HDR || testCode == VCodecTestCode::HW_HEVC ||
        testCode == VCodecTestCode::HW_HDR_HLG_FULL) {
        ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
        ASSERT_EQ(AV_ERR_OK, videoDec_->SetOutputSurface());
        ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, videoDec_->Start());
    } else {
        ASSERT_EQ(AV_ERR_VIDEO_UNSUPPORTED_COLOR_SPACE_CONVERSION, videoDec_->Configure(format_));
    }
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_0071
 * @tc.desc: 1. key pixel format unset;
 *           2. decoder mode is buffer;
 *           3. prepare function is called before start function;
 *           4. key color space is BT709_LIMIT
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_0071, TestSize.Level1)
{
    auto testCode = GetParam();
    CreateByNameWithParam(testCode);
    std::shared_ptr<FormatMock> format = FormatMockFactory::CreateFormat();
    format->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    PrepareSource(testCode);
    format->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_LIMIT);

    if (testCode == VCodecTestCode::HW_HDR || testCode == VCodecTestCode::HW_HEVC ||
        testCode == VCodecTestCode::HW_HDR_HLG_FULL) {
        ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format));
        ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, videoDec_->Prepare());
    } else {
        ASSERT_EQ(AV_ERR_VIDEO_UNSUPPORTED_COLOR_SPACE_CONVERSION, videoDec_->Configure(format));
    }
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_0072
 * @tc.desc: 1. key pixel format unset;
 *           2. decoder mode is buffer;
 *           3. prepare function is called before start function;
 *           4. key color space is OH_COLORSPACE_P3_FULL
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_0072, TestSize.Level1)
{
    auto testCode = GetParam();
    CreateByNameWithParam(testCode);
    std::shared_ptr<FormatMock> format = FormatMockFactory::CreateFormat();
    format->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    PrepareSource(testCode);
    format->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_P3_FULL);

    if (testCode == VCodecTestCode::HW_HDR || testCode == VCodecTestCode::HW_HEVC ||
        testCode == VCodecTestCode::HW_HDR_HLG_FULL) {
        ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format));
        ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, videoDec_->Prepare());
    } else {
        ASSERT_EQ(AV_ERR_VIDEO_UNSUPPORTED_COLOR_SPACE_CONVERSION, videoDec_->Configure(format));
    }
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_0081
 * @tc.desc: 1. key pixel format is NV12;
 *           2. decoder mode is buffer;
 *           3. prepare function is called before start function;
 *           4. key color space is BT709_LIMIT
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_0081, TestSize.Level1)
{
    auto testCode = GetParam();
    ConfigureHdrVivid2Sdr(testCode);

    if (testCode == VCodecTestCode::HW_HDR || testCode == VCodecTestCode::HW_HEVC ||
        testCode == VCodecTestCode::HW_HDR_HLG_FULL) {
        ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
        ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, videoDec_->Prepare());
    } else {
        ASSERT_EQ(AV_ERR_VIDEO_UNSUPPORTED_COLOR_SPACE_CONVERSION, videoDec_->Configure(format_));
    }
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_0082
 * @tc.desc: 1. key pixel format is NV12;
 *           2. decoder mode is buffer;
 *           3. prepare function is called before start function;
 *           4. key color space is OH_COLORSPACE_P3_FULL
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_0082, TestSize.Level1)
{
    auto testCode = GetParam();
    ConfigureHdrVivid2Sdr(testCode);

    if (testCode == VCodecTestCode::HW_HDR || testCode == VCodecTestCode::HW_HEVC ||
        testCode == VCodecTestCode::HW_HDR_HLG_FULL) {
        ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
        ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, videoDec_->Prepare());
    } else {
        ASSERT_EQ(AV_ERR_VIDEO_UNSUPPORTED_COLOR_SPACE_CONVERSION, videoDec_->Configure(format_));
    }
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_0091
 * @tc.desc: 1. key pixel format unset;
 *           2. decoder mode is surface;
 *           3. prepare function is called before start function;
 *           4. key color space is BT709_LIMIT
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_0091, TestSize.Level1)
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
        std::shared_ptr<FormatMock> curFormat = videoDec_->GetOutputDescription();
        CheckFormatKey(curFormat);
        EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
    } else if (testCode == VCodecTestCode::HW_AVC || testCode == VCodecTestCode::SW_AVC) {
        ASSERT_EQ(AV_ERR_VIDEO_UNSUPPORTED_COLOR_SPACE_CONVERSION, videoDec_->Configure(format));
    }
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_0092
 * @tc.desc: 1. key pixel format unset;
 *           2. decoder mode is surface;
 *           3. prepare function is called before start function;
 *           4. key color space is OH_COLORSPACE_P3_FULL
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_0092, TestSize.Level1)
{
    auto testCode = GetParam();
    CreateByNameWithParam(testCode);
    std::shared_ptr<FormatMock> format = FormatMockFactory::CreateFormat();
    format->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH_HLG_FULL);
    format->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT_HLG_FULL);
    PrepareSource(testCode);
    format->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_P3_FULL);

    if (testCode == VCodecTestCode::HW_HDR_HLG_FULL) {
        ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format));
        ASSERT_EQ(AV_ERR_OK, videoDec_->SetOutputSurface());
        ASSERT_EQ(AV_ERR_OK, videoDec_->Prepare());
        EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
        CheckFormatKeyForP3Full(videoDec_, format);
        EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
    } else if (testCode == VCodecTestCode::HW_AVC || testCode == VCodecTestCode::SW_AVC) {
        ASSERT_EQ(AV_ERR_VIDEO_UNSUPPORTED_COLOR_SPACE_CONVERSION, videoDec_->Configure(format));
    }
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_0101
 * @tc.desc: 1. key pixel format is NV12;
 *           2. decoder mode is surface;
 *           3. prepare function is called before start function;
 *           4. key color space is BT709_LIMIT
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_0101, TestSize.Level1)
{
    auto testCode = GetParam();
    ConfigureHdrVivid2Sdr(testCode);

    if (testCode == VCodecTestCode::HW_HDR) {
        ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
        ASSERT_EQ(AV_ERR_OK, videoDec_->SetOutputSurface());
        ASSERT_EQ(AV_ERR_OK, videoDec_->Prepare());
        EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
        std::shared_ptr<FormatMock> curFormat = videoDec_->GetOutputDescription();
        CheckFormatKey(curFormat);
        EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
    } else if (testCode == VCodecTestCode::HW_HDR_HLG_FULL) {
        ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
        ASSERT_EQ(AV_ERR_OK, videoDec_->SetOutputSurface());
        ASSERT_EQ(AV_ERR_OK, videoDec_->Prepare());
        EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
        CheckFormatKeyForP3Full(videoDec_, format_);
        EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
    } else if (testCode == VCodecTestCode::HW_AVC || testCode == VCodecTestCode::SW_AVC) {
        ASSERT_EQ(AV_ERR_VIDEO_UNSUPPORTED_COLOR_SPACE_CONVERSION, videoDec_->Configure(format_));
    }
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_0111
 * @tc.desc: 1. key pixel format is NV12;
 *           2. key color space is BT2020_HLG_LIMIT
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_0111, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT2020_HLG_LIMIT);

    ASSERT_EQ(AV_ERR_VIDEO_UNSUPPORTED_COLOR_SPACE_CONVERSION, videoDec_->Configure(format_));
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_0121
 * @tc.desc: 1. key pixel format unset;
 *           2. key color space is BT2020_HLG_LIMIT
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_0121, TestSize.Level1)
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
 * @tc.name: VideoDecoder_HRDVivid2SDR_0131
 * @tc.desc: 1. key pixel format is NV21;
 *           2. decoder mode is buffer;
 *           3. prepare function is not called;
 *           4. key color space is BT709_LIMIT
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_0131, TestSize.Level1)
{
    auto testCode = GetParam();
    if (testCode == VCodecTestCode::HW_HDR || testCode == VCodecTestCode::HW_HEVC ||
        testCode == VCodecTestCode::HW_HDR_HLG_FULL) {
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
 * @tc.name: VideoDecoder_HRDVivid2SDR_0141
 * @tc.desc: 1. key pixel format is NV21;
 *           2. decoder mode is surface;
 *           3. prepare function is not called;
 *           4. key color space is BT709_LIMIT
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_0141, TestSize.Level1)
{
    auto testCode = GetParam();
    if (testCode == VCodecTestCode::HW_HDR || testCode == VCodecTestCode::HW_HEVC ||
        testCode == VCodecTestCode::HW_HDR_HLG_FULL) {
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
 * @tc.name: VideoDecoder_HRDVivid2SDR_0151
 * @tc.desc: 1. key pixel format is NV21;
 *           2. decoder mode is buffer;
 *           3. prepare function is called before start function;
 *           4. key color space is BT709_LIMIT
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_0151, TestSize.Level1)
{
    auto testCode = GetParam();
    if (testCode == VCodecTestCode::HW_HDR || testCode == VCodecTestCode::HW_HEVC ||
        testCode == VCodecTestCode::HW_HDR_HLG_FULL) {
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
 * @tc.name: VideoDecoder_HRDVivid2SDR_0161
 * @tc.desc: 1. key pixel format is NV21;
 *           2. decoder mode is surface;
 *           3. prepare function is called before start function;
 *           4. key color space is BT709_LIMIT
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_HRDVivid2SDR_0161, TestSize.Level1)
{
    CreateByNameWithParam(VCodecTestCode::HW_HDR);
    SetHDRFormat();
    PrepareSource(VCodecTestCode::HW_HDR);
    format_->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_LIMIT);

    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    ASSERT_EQ(AV_ERR_OK, videoDec_->SetOutputSurface());
    ASSERT_EQ(AV_ERR_OK, videoDec_->Prepare());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    std::shared_ptr<FormatMock> curFormat = videoDec_->GetOutputDescription();
    CheckFormatKey(curFormat);
    EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_0171
 * @tc.desc: 1. key pixel format is NV21;
 *           2. key color space is BT2020_HLG_LIMIT
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_0171, TestSize.Level1)
{
    auto testCode = GetParam();
    if (testCode == VCodecTestCode::HW_HDR || testCode == VCodecTestCode::HW_HEVC ||
        testCode == VCodecTestCode::HW_HDR_HLG_FULL) {
        CreateByNameWithParam(testCode);
        SetHDRFormat();
        PrepareSource(testCode);
        format_->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
            OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT2020_HLG_LIMIT);

        ASSERT_EQ(AV_ERR_VIDEO_UNSUPPORTED_COLOR_SPACE_CONVERSION, videoDec_->Configure(format_));
    }
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_0241
 * @tc.desc: 1. key pixel format is NV12;
 *           2. key color space is BT709_LIMIT
 *           3. decoder mode is surface;
 *           4. prepare function is called before start function;
 *           5. start -> flush -> stop
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_HRDVivid2SDR_0241, TestSize.Level1)
{
    ConfigureHdrVivid2Sdr(VCodecTestCode::HW_HDR);
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    ASSERT_EQ(AV_ERR_OK, videoDec_->SetOutputSurface());
    ASSERT_EQ(AV_ERR_OK, videoDec_->Prepare());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Flush());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_Capi_0242
 * @tc.desc: 1. key pixel format is NV12;
 *           2. key color space is OH_COLORSPACE_P3_FULL
 *           3. decoder mode is surface;
 *           4. prepare function is called before start function;
 *           5. start -> flush -> stop
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_HRDVivid2SDR_Capi_0242, TestSize.Level1)
{
    ConfigureHdrVivid2Sdr(VCodecTestCode::HW_HDR_HLG_FULL);
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    ASSERT_EQ(AV_ERR_OK, videoDec_->SetOutputSurface());
    ASSERT_EQ(AV_ERR_OK, videoDec_->Prepare());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Flush());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_0251
 * @tc.desc: unordered post processing function invocation
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_HRDVivid2SDR_0251, TestSize.Level1)
{
    ConfigureHdrVivid2Sdr(VCodecTestCode::HW_HDR);
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

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_Capi_0252
 * @tc.desc: unordered post processing function invocation
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_HRDVivid2SDR_Capi_0252, TestSize.Level1)
{
    ConfigureHdrVivid2Sdr(VCodecTestCode::HW_HDR_HLG_FULL);
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

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_0261
 * @tc.desc: unordered post processing function invocation
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_HRDVivid2SDR_0261, TestSize.Level1)
{
    ConfigureHdrVivid2Sdr(VCodecTestCode::HW_HDR);
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

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_Capi_0262
 * @tc.desc: unordered post processing function invocation
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_HRDVivid2SDR_Capi_0262, TestSize.Level1)
{
    ConfigureHdrVivid2Sdr(VCodecTestCode::HW_HDR_HLG_FULL);
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

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_0271
 * @tc.desc: unordered post processing function invocation
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_HRDVivid2SDR_0271, TestSize.Level1)
{
    ConfigureHdrVivid2Sdr(VCodecTestCode::HW_HDR);
    for (int i = 0; i < 3 ; i++) {
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

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_0272
 * @tc.desc: unordered post processing function invocation
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_HRDVivid2SDR_0272, TestSize.Level1)
{
    ConfigureHdrVivid2Sdr(VCodecTestCode::HW_HDR_HLG_FULL);
    for (int i = 0; i < 3 ; i++) {
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

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_0281
 * @tc.desc: unordered post processing function invocation
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_HRDVivid2SDR_0281, TestSize.Level1)
{
    ConfigureHdrVivid2Sdr(VCodecTestCode::HW_HDR);
    for (int i = 0; i < 3 ; i++) {
        ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
        ASSERT_EQ(AV_ERR_OK, videoDec_->SetOutputSurface());
        ASSERT_EQ(AV_ERR_OK, videoDec_->Prepare());
        EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
        EXPECT_EQ(AV_ERR_OK, videoDec_->Flush());
        EXPECT_EQ(AV_ERR_OK, videoDec_->Reset());
    }
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_0282
 * @tc.desc: unordered post processing function invocation
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_HRDVivid2SDR_0282, TestSize.Level1)
{
    ConfigureHdrVivid2Sdr(VCodecTestCode::HW_HDR_HLG_FULL);
    for (int i = 0; i < 3 ; i++) {
        ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
        ASSERT_EQ(AV_ERR_OK, videoDec_->SetOutputSurface());
        ASSERT_EQ(AV_ERR_OK, videoDec_->Prepare());
        EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
        EXPECT_EQ(AV_ERR_OK, videoDec_->Flush());
        EXPECT_EQ(AV_ERR_OK, videoDec_->Reset());
    }
}
#else
/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_0181
 * @tc.desc: 1. key pixel format is RGBA;
 *           2. key color space is BT709_LIMIT
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_0181, TestSize.Level1)
{
    auto testCode = GetParam();
    CreateByNameWithParam(testCode);
    SetAVCFormat();
    PrepareSource(testCode);
    format_->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_LIMIT);

    if (testCode == VCodecTestCode::HW_HEVC || testCode == VCodecTestCode::HW_HDR ||
        testCode == VCodecTestCode::HW_HDR_HLG_FULL) {
        ASSERT_EQ(AV_ERR_UNSUPPORT, videoDec_->Configure(format_));
    } else {
        ASSERT_EQ(AV_ERR_VIDEO_UNSUPPORTED_COLOR_SPACE_CONVERSION, videoDec_->Configure(format_));
    }
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_0191
 * @tc.desc: 1. key pixel format unset;
 *           2. key color space is BT709_LIMIT
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_0191, TestSize.Level1)
{
    auto testCode = GetParam();
    CreateByNameWithParam(testCode);
    std::shared_ptr<FormatMock> format = FormatMockFactory::CreateFormat();
    format->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    PrepareSource(testCode);
    format->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_LIMIT);
    
    if (testCode == VCodecTestCode::HW_HEVC || testCode == VCodecTestCode::HW_HDR ||
        testCode == VCodecTestCode::HW_HDR_HLG_FULL) {
        ASSERT_EQ(AV_ERR_UNSUPPORT, videoDec_->Configure(format));
    } else {
        ASSERT_EQ(AV_ERR_VIDEO_UNSUPPORTED_COLOR_SPACE_CONVERSION, videoDec_->Configure(format));
    }
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_0201
 * @tc.desc: 1. key pixel format is NV12;
 *           2. key color space is BT2020_HLG_LIMIT
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_0201, TestSize.Level1)
{
    auto testCode = GetParam();
    CreateByNameWithParam(testCode);
    SetFormatWithParam(testCode);
    PrepareSource(testCode);
    format_->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_LIMIT);

    if (testCode == VCodecTestCode::HW_HEVC || testCode == VCodecTestCode::HW_HDR ||
        testCode == VCodecTestCode::HW_HDR_HLG_FULL) {
        ASSERT_EQ(AV_ERR_UNSUPPORT, videoDec_->Configure(format_));
    } else {
        ASSERT_EQ(AV_ERR_VIDEO_UNSUPPORTED_COLOR_SPACE_CONVERSION, videoDec_->Configure(format_));
    }
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_0211
 * @tc.desc: 1. key pixel format is RGBA;
 *           2. key color space is BT2020_HLG_LIMIT
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_0211, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetAVCFormat();
    PrepareSource(GetParam());
    format_->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT2020_HLG_LIMIT);

    ASSERT_EQ(AV_ERR_VIDEO_UNSUPPORTED_COLOR_SPACE_CONVERSION, videoDec_->Configure(format_));
}

/**
 * @tc.name: VideoDecoder_HRDVivid2SDR_0221
 * @tc.desc: 1. key pixel format unset;
 *           2. key color space is BT2020_HLG_LIMIT
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_0221, TestSize.Level1)
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
 * @tc.name: VideoDecoder_HRDVivid2SDR_0231
 * @tc.desc: 1. key pixel format is NV12;
 *           2. key color space is BT2020_HLG_LIMIT
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_HRDVivid2SDR_0231, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE,
        OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT2020_HLG_LIMIT);

    ASSERT_EQ(AV_ERR_VIDEO_UNSUPPORTED_COLOR_SPACE_CONVERSION, videoDec_->Configure(format_));
}
#endif // HMOS_TEST
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