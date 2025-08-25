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
#include "venc_async_sample.h"
#include "venc_sync_sample.h"
#ifdef VIDEOENC_CAPI_UNIT_TEST
#include "native_avmagic.h"
#include "videoenc_capi_mock.h"
#define TEST_SUIT VideoEncBitrateModeCapiTest
#else
#define TEST_SUIT VideoEncBitrateModeInnerTest
#endif
#define BITRATE_MODE_MAX 255

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
    void CreateByNameWithParam(int32_t param);
    void SetFormatWithBitrateMode(int32_t bitrateMode);
    void PrepareSource();

protected:
    std::shared_ptr<CodecListMock> capability_ = nullptr;
    std::shared_ptr<VideoEncSample> videoEnc_ = nullptr;
    std::shared_ptr<FormatMock> format_ = nullptr;
    std::shared_ptr<VEncCallbackTest> vencCallback_ = nullptr;
    std::shared_ptr<VEncCallbackTestExt> vencCallbackExt_ = nullptr;
    std::shared_ptr<VEncParamCallbackTest> vencParamCallback_ = nullptr;
    std::shared_ptr<VEncParamWithAttrCallbackTest> vencParamWithAttrCallback_ = nullptr;

private:
    int32_t default_bitrate = 1000000;
    int32_t default_max_bitrate = 20000000;
    int32_t default_quality = 30;
    int32_t default_sqr_factor = 30;
};

void TEST_SUIT::SetUpTestCase(void)
{
    auto capability = CodecListMockFactory::GetCapabilityByCategory((CodecMimeType::VIDEO_HEVC).data(), true,
                                                                    AVCodecCategory::AVCODEC_HARDWARE);
    ASSERT_NE(nullptr, capability) << (CodecMimeType::VIDEO_HEVC).data() << " can not found!" << std::endl;
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
}

void TEST_SUIT::SetFormatWithBitrateMode(int32_t bitrateMode)
{
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH_VENC);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT_VENC);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_NV12);
    if (capability_->IsEncoderBitrateModeSupported(static_cast<OH_BitrateMode>(bitrateMode))) {
        format_->PutIntValue(OH_MD_KEY_VIDEO_ENCODE_BITRATE_MODE, bitrateMode);
    } else {
        std::cout << "BitrateMode: " << bitrateMode << " not supported\n";
    }
}

INSTANTIATE_TEST_SUITE_P(,
                         TEST_SUIT,
                         testing::Values(BITRATE_MODE_CBR,
                                         BITRATE_MODE_VBR,
                                         BITRATE_MODE_CQ,
                                         BITRATE_MODE_SQR,
                                         BITRATE_MODE_MAX)
                         );

/**.
 * @tc.name: VideoHevcEnc_001
 * @tc.desc: video hevc enc.
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoHevcEnc_001, TestSize.Level1)
{
    CreateByNameWithParam(HW_HEVC);
    SetFormatWithBitrateMode(GetParam());
    PrepareSource();
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    videoEnc_->WaitForEos();
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Release());
}

/**.
 * @tc.name: VideoHevcEnc_002
 * @tc.desc: video hevc enc with SQR_Factor.
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoHevcEnc_002, TestSize.Level1)
{
    CreateByNameWithParam(HW_HEVC);
    SetFormatWithBitrateMode(GetParam());
    PrepareSource();
    format_->PutIntValue(OH_MD_KEY_SQR_FACTOR, default_sqr_factor);
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    videoEnc_->WaitForEos();
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Release());
}

/**.
 * @tc.name: VideoHevcEnc_003
 * @tc.desc: video hevc enc with error SQR_Factor.
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoHevcEnc_003, TestSize.Level1)
{
    CreateByNameWithParam(HW_HEVC);
    SetFormatWithBitrateMode(GetParam());
    PrepareSource();
    default_sqr_factor = -1;
    format_->PutIntValue(OH_MD_KEY_SQR_FACTOR, default_sqr_factor);
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    videoEnc_->WaitForEos();
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Release());
}

/**.
 * @tc.name: VideoHevcEnc_004
 * @tc.desc: video hevc enc with bitrate.
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoHevcEnc_004, TestSize.Level1)
{
    CreateByNameWithParam(HW_HEVC);
    SetFormatWithBitrateMode(GetParam());
    PrepareSource();
    format_->PutDoubleValue(OH_MD_KEY_BITRATE, default_bitrate);
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    videoEnc_->WaitForEos();
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Release());
}

/**.
 * @tc.name: VideoHevcEnc_005
 * @tc.desc: video hevc enc with error bitrate.
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoHevcEnc_005, TestSize.Level1)
{
    CreateByNameWithParam(HW_HEVC);
    SetFormatWithBitrateMode(GetParam());
    PrepareSource();
    default_bitrate = -1;
    format_->PutDoubleValue(OH_MD_KEY_BITRATE, default_bitrate);
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    videoEnc_->WaitForEos();
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Release());
}

/**.
 * @tc.name: VideoHevcEnc_006
 * @tc.desc: video hevc enc with max_bitrate.
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoHevcEnc_006, TestSize.Level1)
{
    CreateByNameWithParam(HW_HEVC);
    SetFormatWithBitrateMode(GetParam());
    PrepareSource();
    format_->PutDoubleValue(OH_MD_KEY_MAX_BITRATE, default_max_bitrate);
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    videoEnc_->WaitForEos();
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Release());
}

/**.
 * @tc.name: VideoHevcEnc_007
 * @tc.desc: video hevc enc with error max_bitrate.
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoHevcEnc_007, TestSize.Level1)
{
    CreateByNameWithParam(HW_HEVC);
    SetFormatWithBitrateMode(GetParam());
    PrepareSource();
    default_max_bitrate = -1;
    format_->PutDoubleValue(OH_MD_KEY_MAX_BITRATE, default_max_bitrate);
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    videoEnc_->WaitForEos();
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Release());
}

/**.
 * @tc.name: VideoHevcEnc_008
 * @tc.desc: video hevc enc with quality.
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoHevcEnc_008, TestSize.Level1)
{
    CreateByNameWithParam(HW_HEVC);
    SetFormatWithBitrateMode(GetParam());
    PrepareSource();
    format_->PutIntValue(OH_MD_KEY_QUALITY, default_quality);
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    videoEnc_->WaitForEos();
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Release());
}

/**.
 * @tc.name: VideoHevcEnc_009
 * @tc.desc: video hevc enc with error quality.
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoHevcEnc_009, TestSize.Level1)
{
    CreateByNameWithParam(HW_HEVC);
    SetFormatWithBitrateMode(GetParam());
    PrepareSource();
    default_quality = -1;
    format_->PutIntValue(OH_MD_KEY_QUALITY, default_quality);
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    videoEnc_->WaitForEos();
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Release());
}

/**.
 * @tc.name: VideoHevcEnc_010
 * @tc.desc: video hevc enc with sqr、bitrate and quality.
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoHevcEnc_010, TestSize.Level1)
{
    CreateByNameWithParam(HW_HEVC);
    SetFormatWithBitrateMode(GetParam());
    PrepareSource();
    format_->PutIntValue(OH_MD_KEY_SQR_FACTOR, default_sqr_factor);
    format_->PutDoubleValue(OH_MD_KEY_BITRATE, default_bitrate);
    format_->PutIntValue(OH_MD_KEY_QUALITY, default_quality);
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    videoEnc_->WaitForEos();
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Release());
}

/**.
 * @tc.name: VideoHevcEnc_011
 * @tc.desc: video hevc enc with sqr、maxBitrate and quality.
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoHevcEnc_011, TestSize.Level1)
{
    CreateByNameWithParam(HW_HEVC);
    SetFormatWithBitrateMode(GetParam());
    PrepareSource();
    format_->PutIntValue(OH_MD_KEY_SQR_FACTOR, default_sqr_factor);
    format_->PutDoubleValue(OH_MD_KEY_MAX_BITRATE, default_max_bitrate);
    format_->PutIntValue(OH_MD_KEY_QUALITY, default_quality);
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    videoEnc_->WaitForEos();
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Release());
}

/**.
 * @tc.name: VideoHevcEnc_012
 * @tc.desc: video hevc enc with sqr、bitrate and maxBitrate.
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoHevcEnc_012, TestSize.Level1)
{
    CreateByNameWithParam(HW_HEVC);
    SetFormatWithBitrateMode(GetParam());
    PrepareSource();
    format_->PutIntValue(OH_MD_KEY_SQR_FACTOR, default_sqr_factor);
    format_->PutDoubleValue(OH_MD_KEY_BITRATE, default_bitrate);
    format_->PutDoubleValue(OH_MD_KEY_MAX_BITRATE, default_max_bitrate);
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    videoEnc_->WaitForEos();
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Release());
}

/**.
 * @tc.name: VideoHevcEnc_013
 * @tc.desc: video hevc enc with sqr、bitrate and error maxBitrate.
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoHevcEnc_013, TestSize.Level1)
{
    CreateByNameWithParam(HW_HEVC);
    SetFormatWithBitrateMode(GetParam());
    PrepareSource();
    default_max_bitrate = -1;
    format_->PutIntValue(OH_MD_KEY_SQR_FACTOR, default_sqr_factor);
    format_->PutDoubleValue(OH_MD_KEY_BITRATE, default_bitrate);
    format_->PutDoubleValue(OH_MD_KEY_MAX_BITRATE, default_max_bitrate);
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    videoEnc_->WaitForEos();
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Release());
}

/**.
 * @tc.name: VideoHevcEnc_014
 * @tc.desc: video hevc enc setparameter with sqr、bitrate、maxBitrate and quality.
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoHevcEnc_014, TestSize.Level1)
{
    CreateByNameWithParam(HW_HEVC);
    SetFormatWithBitrateMode(GetParam());
    PrepareSource();
    format_->PutIntValue(OH_MD_KEY_SQR_FACTOR, default_sqr_factor);
    format_->PutDoubleValue(OH_MD_KEY_BITRATE, default_bitrate);
    format_->PutDoubleValue(OH_MD_KEY_MAX_BITRATE, default_max_bitrate);
    format_->PutIntValue(OH_MD_KEY_QUALITY, default_quality);
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    default_sqr_factor += 1;
    default_bitrate += 1;
    default_max_bitrate += 1;
    default_quality += 1;
    format_->PutIntValue(OH_MD_KEY_SQR_FACTOR, default_sqr_factor);
    format_->PutDoubleValue(OH_MD_KEY_BITRATE, default_bitrate);
    format_->PutDoubleValue(OH_MD_KEY_MAX_BITRATE, default_max_bitrate);
    format_->PutIntValue(OH_MD_KEY_QUALITY, default_quality);
    EXPECT_EQ(AV_ERR_OK, videoEnc_->SetParameter(format_));
    videoEnc_->WaitForEos();
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Release());
}

/**.
 * @tc.name: VideoHevcEnc_015
 * @tc.desc: video hevc enc setparameter with sqr、 bitrate、error maxBitrate and quality.
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoHevcEnc_015, TestSize.Level1)
{
    CreateByNameWithParam(HW_HEVC);
    SetFormatWithBitrateMode(GetParam());
    PrepareSource();
    format_->PutIntValue(OH_MD_KEY_SQR_FACTOR, default_sqr_factor);
    format_->PutDoubleValue(OH_MD_KEY_BITRATE, default_bitrate);
    format_->PutDoubleValue(OH_MD_KEY_MAX_BITRATE, default_max_bitrate);
    format_->PutIntValue(OH_MD_KEY_QUALITY, default_quality);
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    default_sqr_factor += 1;
    default_bitrate += 1;
    default_quality += 1;
    default_max_bitrate = -1;
    format_->PutIntValue(OH_MD_KEY_SQR_FACTOR, default_sqr_factor);
    format_->PutDoubleValue(OH_MD_KEY_BITRATE, default_bitrate);
    format_->PutDoubleValue(OH_MD_KEY_MAX_BITRATE, default_max_bitrate);
    format_->PutIntValue(OH_MD_KEY_QUALITY, default_quality);
    EXPECT_EQ(AV_ERR_OK, videoEnc_->SetParameter(format_));
    videoEnc_->WaitForEos();
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Release());
}

/**.
 * @tc.name: VideoHevcEnc_016
 * @tc.desc: video hevc enc setparameter with error sqr、error bitrate、error maxBitrate and error quality.
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoHevcEnc_016, TestSize.Level1)
{
    CreateByNameWithParam(HW_HEVC);
    SetFormatWithBitrateMode(GetParam());
    PrepareSource();
    format_->PutIntValue(OH_MD_KEY_SQR_FACTOR, default_sqr_factor);
    format_->PutDoubleValue(OH_MD_KEY_BITRATE, default_bitrate);
    format_->PutDoubleValue(OH_MD_KEY_MAX_BITRATE, default_max_bitrate);
    format_->PutIntValue(OH_MD_KEY_QUALITY, default_quality);
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    default_sqr_factor = -1;
    default_bitrate = -1;
    default_max_bitrate = -1;
    default_quality = -1;
    format_->PutIntValue(OH_MD_KEY_SQR_FACTOR, default_sqr_factor);
    format_->PutDoubleValue(OH_MD_KEY_BITRATE, default_bitrate);
    format_->PutDoubleValue(OH_MD_KEY_MAX_BITRATE, default_max_bitrate);
    format_->PutIntValue(OH_MD_KEY_QUALITY, default_quality);
    EXPECT_EQ(AV_ERR_OK, videoEnc_->SetParameter(format_));
    videoEnc_->WaitForEos();
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Release());
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