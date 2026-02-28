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
#include <tuple>
#include "meta/meta_key.h"
#include "unittest_utils.h"
#include "codeclist_mock.h"
#include "venc_async_sample.h"
#ifdef VIDEOENC_CAPI_UNIT_TEST
#include "native_avmagic.h"
#include "videoenc_capi_mock.h"
#define TEST_SUIT VideoEncFormatCapiTest
#else
#define TEST_SUIT VideoEncFormatInnerTest
#endif

using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace testing::ext;
using namespace OHOS::MediaAVCodec::VCodecTestParam;
using namespace OHOS::Media;

namespace {
class TEST_SUIT : public testing::TestWithParam<std::tuple<int32_t, int32_t>> {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);

    bool CreateVideoCodecByName(const std::string &decName);
    bool CreateVideoCodecByMime(const std::string &decMime);
    void CreateByNameWithParam(int32_t param);
    void SetFormatWithParam(int32_t param);
    void PrepareSource();

protected:
    std::shared_ptr<CodecListMock> capability_ = nullptr;
    std::shared_ptr<VideoEncSample> videoEnc_ = nullptr;
    std::shared_ptr<FormatMock> format_ = nullptr;
    std::shared_ptr<VEncCallbackTest> vencCallback_ = nullptr;
    std::shared_ptr<VEncCallbackTestExt> vencCallbackExt_ = nullptr;
    std::shared_ptr<VEncParamCallbackTest> vencParamCallback_ = nullptr;
    std::shared_ptr<VEncParamWithAttrCallbackTest> vencParamWithAttrCallback_ = nullptr;
};

void TEST_SUIT::SetUpTestCase(void) {}

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

void TEST_SUIT::PrepareSource()
{
    const ::testing::TestInfo *testInfo_ = ::testing::UnitTest::GetInstance()->current_test_info();
    string prefix = "/data/test/media/";
    string fileName = testInfo_->name();
    auto check = [](char it) { return it == '/'; };
    (void)fileName.erase(std::remove_if(fileName.begin(), fileName.end(), check), fileName.end());
    videoEnc_->SetOutPath(prefix + fileName);
    videoEnc_->SetInPath("/data/test/media/1280_720_rgba1010102.rgb");
}

void TEST_SUIT::SetFormatWithParam(int32_t param)
{
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH_VENC);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT_VENC);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, param);
}

INSTANTIATE_TEST_SUITE_P(,
                         TEST_SUIT,
                         testing::Combine(
                             testing::Values(HW_AVC, HW_HEVC), // 编码器类型
                             testing::Values(RGBA, RGBA1010102) // 像素格式
                             ));

/**
 * @tc.name: VideoEncoder_Format_Capi_001
 * @tc.desc: Configure succeed if the pixel format is supported, failed otherwise.
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_Format_Capi_001, TestSize.Level1)
{
    int32_t codecType = std::get<0>(GetParam());
    CreateByNameWithParam(codecType);
    auto vec = capability_->GetVideoSupportedPixelFormats();
    int32_t pixelformat = std::get<1>(GetParam());
    SetFormatWithParam(pixelformat);
    PrepareSource();
    if (std::find(vec.begin(), vec.end(), pixelformat) != vec.end()) { // supported
        ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    } else {
        ASSERT_NE(AV_ERR_OK, videoEnc_->Configure(format_));
    }
}

/**
 * @tc.name: VideoEncoder_Format_Capi_002
 * @tc.desc: input rgba file and run encoder. surface mode.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoEncoder_Format_Capi_002, TestSize.Level1)
{
    auto pixelFormats = capability_->GetVideoSupportedPixelFormats();
    if (std::find(pixelFormats.begin(), pixelFormats.end(), VCodecPixelFormat::RGBA1010102) != pixelFormats.end()) {
        CreateByNameWithParam(HW_HEVC);
        SetFormatWithParam(RGBA1010102);
        PrepareSource();
        auto ret = videoEnc_->Configure(format_);
        if (ret != AV_ERR_OK) {
            return;
        }
        ASSERT_EQ(AV_ERR_OK, videoEnc_->CreateInputSurface());
        EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
        EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
    }
}

/**
 * @tc.name: VideoEncoder_Format_Capi_003
 * @tc.desc: input rgba file and run encoder. buffer mode.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoEncoder_Format_Capi_003, TestSize.Level1)
{
    auto pixelFormats = capability_->GetVideoSupportedPixelFormats();
    if (std::find(pixelFormats.begin(), pixelFormats.end(), VCodecPixelFormat::RGBA1010102) != pixelFormats.end()) {
        videoEnc_->isAVBufferMode_ = true;
        CreateByNameWithParam(HW_HEVC);
        SetFormatWithParam(RGBA1010102);
        format_->PutIntValue(MediaDescriptionKey::MD_KEY_PROFILE, OH_HEVCProfile::HEVC_PROFILE_MAIN_10);
        PrepareSource();
        auto ret = videoEnc_->Configure(format_);
        if (ret != AV_ERR_OK) {
            return;
        }
        EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
        EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
    }
}

} // namespace

int main(int argc, char **argv)
{
    testing::GTEST_FLAG(output) = "xml:./";
    for (int i = 0; i < argc; ++i) {
        std::cout << argv[i] << std::endl;
        if (strcmp(argv[i], "--need_dump") == 0) {
            VideoEncSample::needDump_ = true;
            DecArgv(i, argc, argv);
        }
    }
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}