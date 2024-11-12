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
#include "meta/meta_key.h"
#include "unittest_utils.h"
#include "codeclist_mock.h"
#include "venc_sample.h"
#include "native_avmagic.h"
#ifdef VIDEOENC_CAPI_UNIT_TEST
#include "native_avmagic.h"
#include "videoenc_capi_mock.h"
#define TEST_SUIT VideoEncQPMapCapiTest
#else
#define TEST_SUIT VideoEncQPMapInnerTest
#endif

using namespace std;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace testing::ext;
using namespace OHOS::MediaAVCodec::VCodecTestParam;
using namespace OHOS::Media;

namespace {
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
    bool GetQPMapCapability(int32 param);

protected:
    std::shared_ptr<VideoEncSample> videoEnc_ = nullptr;//
    std::shared_ptr<FormatMock> format_ = nullptr;//
    std::shared_ptr<VEncCallbackTestExt> vencCallbackExt_ = nullptr;//
    std::shared_ptr<VEncParamCallbackTest> vencParamCallback_ = nullptr;//
};

void TEST_SUIT::SetUpTestCase(void)
{
    auto capability = CodecListMockFactory::GetCapabilityByCategory((CodecMimeType::VIDEO_AVC).data(), true,
                                                                    AVCodecCategory::AVCODEC_HARDWARE);
    ASSERT_NE(nullptr, capability) << (CodecMimeType::VIDEO_AVC).data() << " can not found!" << std::endl;
}

void TEST_SUIT::TearDownTestCase(void) {}

void TEST_SUIT::SetUp(void)
{
    std::shared_ptr<VEncSignal> vencSignal = std::make_shared<VEncSignal>();

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
    if (format_ != nullptr) {
        format_->Destroy();
    }
    videoEnc_ = nullptr;
}

bool TEST_SUIT::CreateVideoCodecByMime(const std::string &encMime)
{
    if (videoEnc_->CreateVideoEncMockByMime(encMime) == false) {
        return false;
    }
    return true;
}

bool TEST_SUIT::CreateVideoCodecByName(const std::string &name)
{
    if (videoEnc_->CreateVideoEncMockByName(name) == false ||
        videoEnc_->SetCallback(vencCallbackExt_) != AV_ERR_OK) {
        return false;
    }

    return true;
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

bool TEST_SUIT::GetQPMapCapability(int32_t param)
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
        static_cast<int32_t>(AVCapabilityFeature::VIDEO_ENCODER_QP_MAP))) {
        std::cout << "Support QP Map" << std::endl;
        return true;
    }
    std::cout << "Not support QP Map" << std::endl;
    return false;
}

INSTANTIATE_TEST_SUITE_P(, TEST_SUIT, testing::Values(HW_AVC, HW_HEVC));

/**
 * @tc.name: VideoEncoder_QPMapCapability_001
 * @tc.desc: configure to enable QPMap Capability for video encode, buffer mode
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_QPMapCapability_001, TestSize.level)
{
    if (!GetQPMapCapability(getParam())) {
        return;
    }
    videoEnc_->isAVBufferMode_ = true;
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_QP_MAP, 1);
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_QPMapCapability_002
 * @tc.desc: configure to enable QPMap Capability for video encode, surface mode with set parametercallback
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_QPMapCapability_002, TestSize.Level1)
{
    if (!GetQPMapCapability(getParam())) {
        return;
    }

    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoEnc_->SetCallback(vencParamCallback_));
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_QP_MAP, 1);
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    ASSERT_EQ(AV_ERR_OK, videoEnc_->CreateInputSurface());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_QPMapCapability_003
 * @tc.desc: set QP Map per frame for video encode, buffer mode
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_QPMapCapability_003, TestSize.level)
{
    if (!GetQPMapCapability(getParam())) {
        return;
    }
    videoEnc_->isAVBufferMode_ = true;
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    videoEnc_->enableQPMapCapability = true;
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_QP_MAP, 1);
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_QPMapCapability_004
 * @tc.desc: set QP Map per frame for video encode, surface mode
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_QPMapCapability_004, TestSize.Level1)
{
    if (!GetQPMapCapability(getParam())) {
        return;
    }
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoEnc_->SetCallback(vencParamCallback_));
    videoEnc_->enableQPMapCapability = true;
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_QP_MAP, 1);
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    ASSERT_EQ(AV_ERR_OK, videoEnc_->CreateInputSurface());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}
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