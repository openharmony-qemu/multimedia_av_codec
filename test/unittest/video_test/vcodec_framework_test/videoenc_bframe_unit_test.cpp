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
#include "native_avmagic.h"
#ifdef VIDEOENC_CAPI_UNIT_TEST
#include "native_avmagic.h"
#include "videoenc_capi_mock.h"
#define TEST_SUIT VideoEncBFrameCapiTest
#else
#define TEST_SUIT VideoEncBFrameInnerTest
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

protected:
    std::shared_ptr<CodecListMock> capability_ = nullptr;
    std::shared_ptr<VideoEncSample> videoEnc_ = nullptr;
    std::shared_ptr<FormatMock> format_ = nullptr;
    std::shared_ptr<VEncCallbackTest> vencCallback_ = nullptr;
    std::shared_ptr<VEncCallbackTestExt> vencCallbackExt_ = nullptr;
    std::shared_ptr<VEncParamCallbackTest> vencParamCallback_ = nullptr;
    std::shared_ptr<VEncParamWithAttrCallbackTest> vencParamWithAttrCallback_ = nullptr;
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

INSTANTIATE_TEST_SUITE_P(, TEST_SUIT, testing::Values(HW_AVC, HW_HEVC));

/**
 * @tc.name: VideoEncoder_B_Frame_001
 * @tc.desc: set key VIDEO_ENCODER_ENABLE_B_FRAME and VideoEncodeBFrameGopMode,
 * value is true and VIDEO_ENCODE_GOP_ADAPTIVE_B_MODE
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_B_Frame_001, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_B_FRAME, true);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODE_B_FRAME_GOP_MODE,
        static_cast<int32_t>(Media::Plugins::VideoEncodeBFrameGopMode::VIDEO_ENCODE_GOP_ADAPTIVE_B_MODE));
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_B_Frame_002
 * @tc.desc: set key VIDEO_ENCODER_ENABLE_B_FRAME and VideoEncodeBFrameGopMode,
 * value is true and VIDEO_ENCODE_GOP_H3B_MODE
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_B_Frame_002, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_B_FRAME, true);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODE_B_FRAME_GOP_MODE,
        static_cast<int32_t>(Media::Plugins::VideoEncodeBFrameGopMode::VIDEO_ENCODE_GOP_H3B_MODE));
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_B_Frame_003
 * @tc.desc: set key VIDEO_ENCODER_ENABLE_B_FRAME and VideoEncodeBFrameGopMode,
 * value is true and VIDEO_ENCODE_GOP_ADAPTIVE_B_MODE
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_B_Frame_003, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_B_FRAME, false);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODE_B_FRAME_GOP_MODE,
        static_cast<int32_t>(Media::Plugins::VideoEncodeBFrameGopMode::VIDEO_ENCODE_GOP_ADAPTIVE_B_MODE));
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_B_Frame_004
 * @tc.desc: set key VIDEO_ENCODER_ENABLE_B_FRAME and VideoEncodeBFrameGopMode,
 * value is true and VIDEO_ENCODE_GOP_H3B_MODE
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_B_Frame_004, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_B_FRAME, false);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODE_B_FRAME_GOP_MODE,
        static_cast<int32_t>(Media::Plugins::VideoEncodeBFrameGopMode::VIDEO_ENCODE_GOP_H3B_MODE));
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_B_Frame_005
 * @tc.desc: set key VIDEO_ENCODER_ENABLE_B_FRAME and VideoEncodeBFrameGopMode,
 * value is true and VIDEO_ENCODE_GOP_ADAPTIVE_B_MODE
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_B_Frame_005, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_B_FRAME, -1);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODE_B_FRAME_GOP_MODE,
        static_cast<int32_t>(Media::Plugins::VideoEncodeBFrameGopMode::VIDEO_ENCODE_GOP_ADAPTIVE_B_MODE));
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_B_Frame_006
 * @tc.desc: set key VIDEO_ENCODER_ENABLE_B_FRAME and VideoEncodeBFrameGopMode,
 * value is true and VIDEO_ENCODE_GOP_H3B_MODE
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_B_Frame_006, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_B_FRAME, -1);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODE_B_FRAME_GOP_MODE,
        static_cast<int32_t>(Media::Plugins::VideoEncodeBFrameGopMode::VIDEO_ENCODE_GOP_H3B_MODE));
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_B_Frame_007
 * @tc.desc: set key VIDEO_ENCODER_ENABLE_B_FRAME and VideoEncodeBFrameGopMode,
 * value is INT32_MAX and VIDEO_ENCODE_GOP_ADAPTIVE_B_MODE
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_B_Frame_007, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_B_FRAME, INT32_MAX);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODE_B_FRAME_GOP_MODE,
        static_cast<int32_t>(Media::Plugins::VideoEncodeBFrameGopMode::VIDEO_ENCODE_GOP_ADAPTIVE_B_MODE));
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_B_Frame_008
 * @tc.desc: set key VIDEO_ENCODER_ENABLE_B_FRAME and VideoEncodeBFrameGopMode,
 * value is INT32_MAX and VIDEO_ENCODE_GOP_H3B_MODE
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_B_Frame_008, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_B_FRAME, INT32_MAX);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODE_B_FRAME_GOP_MODE,
        static_cast<int32_t>(Media::Plugins::VideoEncodeBFrameGopMode::VIDEO_ENCODE_GOP_H3B_MODE));
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_B_Frame_009
 * @tc.desc: set key VIDEO_ENCODER_ENABLE_B_FRAME and VideoEncodeBFrameGopMode,
 * value is INT32_MIN and VIDEO_ENCODE_GOP_ADAPTIVE_B_MODE
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_B_Frame_009, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_B_FRAME, INT32_MIN);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODE_B_FRAME_GOP_MODE,
        static_cast<int32_t>(Media::Plugins::VideoEncodeBFrameGopMode::VIDEO_ENCODE_GOP_ADAPTIVE_B_MODE));
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_B_Frame_010
 * @tc.desc: set key VIDEO_ENCODER_ENABLE_B_FRAME and VideoEncodeBFrameGopMode,
 * value is INT32_MIN and VIDEO_ENCODE_GOP_H3B_MODE
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_B_Frame_010, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_B_FRAME, INT32_MIN);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODE_B_FRAME_GOP_MODE,
        static_cast<int32_t>(Media::Plugins::VideoEncodeBFrameGopMode::VIDEO_ENCODE_GOP_H3B_MODE));
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_B_Frame_011
 * @tc.desc: set key VIDEO_ENCODER_ENABLE_B_FRAME and VideoEncodeBFrameGopMode,
 * value is true and VIDEO_ENCODE_GOP_ADAPTIVE_B_MODE
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_B_Frame_011, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_B_FRAME, true);
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_B_Frame_012
 * @tc.desc: set key VIDEO_ENCODER_ENABLE_B_FRAME and VideoEncodeBFrameGopMode, value is false
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_B_Frame_012, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_B_FRAME, false);
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_B_Frame_013
 * @tc.desc: set key VIDEO_ENCODER_ENABLE_B_FRAME and VideoEncodeBFrameGopMode, value is -1
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_B_Frame_013, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_B_FRAME, -1);
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_B_Frame_014
 * @tc.desc: set key VIDEO_ENCODER_ENABLE_B_FRAME and VideoEncodeBFrameGopMode, value is INT32_MIN
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_B_Frame_014, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_B_FRAME, INT32_MIN);
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_B_Frame_015
 * @tc.desc: set key VIDEO_ENCODER_ENABLE_B_FRAME and VideoEncodeBFrameGopMode, value is INT32_MAX
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_B_Frame_015, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_B_FRAME, INT32_MAX);
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_B_Frame_Enable_Temporal_Scalability_001
 * @tc.desc: set key VIDEO_ENCODER_ENABLE_B_FRAME and VideoEncodeBFrameGopMode,enable temporal scalability,
 * value is true and VIDEO_ENCODE_GOP_ADAPTIVE_B_MODE
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_B_Frame_Enable_Temporal_Scalability_001, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_TEMPORAL_SCALABILITY, true);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_TEMPORAL_GOP_SIZE, 4);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_TEMPORAL_GOP_REFERENCE_MODE,
                         static_cast<int32_t>(OH_TemporalGopReferenceMode::UNIFORMLY_SCALED_REFERENCE));
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_B_FRAME, true);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODE_B_FRAME_GOP_MODE,
        static_cast<int32_t>(Media::Plugins::VideoEncodeBFrameGopMode::VIDEO_ENCODE_GOP_ADAPTIVE_B_MODE));
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_B_Frame_Enable_Temporal_Scalability_002
 * @tc.desc: set key VIDEO_ENCODER_ENABLE_B_FRAME and VideoEncodeBFrameGopMode,enable temporal scalability,
 * value is true and VIDEO_ENCODE_GOP_H3B_MODE
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_B_Frame_Enable_Temporal_Scalability_002, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_TEMPORAL_SCALABILITY, true);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_TEMPORAL_GOP_SIZE, 4);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_TEMPORAL_GOP_REFERENCE_MODE,
                         static_cast<int32_t>(OH_TemporalGopReferenceMode::UNIFORMLY_SCALED_REFERENCE));
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_B_FRAME, true);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODE_B_FRAME_GOP_MODE,
        static_cast<int32_t>(Media::Plugins::VideoEncodeBFrameGopMode::VIDEO_ENCODE_GOP_H3B_MODE));
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_B_Frame_Enable_Temporal_Scalability_003
 * @tc.desc: set key VIDEO_ENCODER_ENABLE_B_FRAME and VideoEncodeBFrameGopMode,enable temporal scalability,
 * value is true and VIDEO_ENCODE_GOP_ADAPTIVE_B_MODE
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_B_Frame_Enable_Temporal_Scalability_003, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_TEMPORAL_SCALABILITY, true);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_TEMPORAL_GOP_SIZE, 4);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_TEMPORAL_GOP_REFERENCE_MODE,
                         static_cast<int32_t>(OH_TemporalGopReferenceMode::UNIFORMLY_SCALED_REFERENCE));
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_B_FRAME, false);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODE_B_FRAME_GOP_MODE,
        static_cast<int32_t>(Media::Plugins::VideoEncodeBFrameGopMode::VIDEO_ENCODE_GOP_ADAPTIVE_B_MODE));
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_B_Frame_Enable_Temporal_Scalability_004
 * @tc.desc: set key VIDEO_ENCODER_ENABLE_B_FRAME and VideoEncodeBFrameGopMode,enable temporal scalability,
 * value is true and VIDEO_ENCODE_GOP_H3B_MODE
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_B_Frame_Enable_Temporal_Scalability_004, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_TEMPORAL_SCALABILITY, true);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_TEMPORAL_GOP_SIZE, 4);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_TEMPORAL_GOP_REFERENCE_MODE,
                         static_cast<int32_t>(OH_TemporalGopReferenceMode::UNIFORMLY_SCALED_REFERENCE));
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_B_FRAME, false);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODE_B_FRAME_GOP_MODE,
        static_cast<int32_t>(Media::Plugins::VideoEncodeBFrameGopMode::VIDEO_ENCODE_GOP_H3B_MODE));
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_B_Frame_Enable_Temporal_Scalability_005
 * @tc.desc: set key VIDEO_ENCODER_ENABLE_B_FRAME and VideoEncodeBFrameGopMode,enable temporal scalability,
 * value is true and VIDEO_ENCODE_GOP_ADAPTIVE_B_MODE
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_B_Frame_Enable_Temporal_Scalability_005, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_TEMPORAL_SCALABILITY, true);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_TEMPORAL_GOP_SIZE, 4);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_TEMPORAL_GOP_REFERENCE_MODE,
                         static_cast<int32_t>(OH_TemporalGopReferenceMode::UNIFORMLY_SCALED_REFERENCE));
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_B_FRAME, -1);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODE_B_FRAME_GOP_MODE,
        static_cast<int32_t>(Media::Plugins::VideoEncodeBFrameGopMode::VIDEO_ENCODE_GOP_ADAPTIVE_B_MODE));
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_B_Frame_Enable_Temporal_Scalability_006
 * @tc.desc: set key VIDEO_ENCODER_ENABLE_B_FRAME and VideoEncodeBFrameGopMode,enable temporal scalability,
 * value is true and VIDEO_ENCODE_GOP_H3B_MODE
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_B_Frame_Enable_Temporal_Scalability_006, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_TEMPORAL_SCALABILITY, true);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_TEMPORAL_GOP_SIZE, 4);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_TEMPORAL_GOP_REFERENCE_MODE,
                         static_cast<int32_t>(OH_TemporalGopReferenceMode::UNIFORMLY_SCALED_REFERENCE));
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_B_FRAME, -1);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODE_B_FRAME_GOP_MODE,
        static_cast<int32_t>(Media::Plugins::VideoEncodeBFrameGopMode::VIDEO_ENCODE_GOP_H3B_MODE));
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_B_Frame_Enable_Temporal_Scalability_007
 * @tc.desc: set key VIDEO_ENCODER_ENABLE_B_FRAME and VideoEncodeBFrameGopMode,enable temporal scalability,
 * value is INT32_MAX and VIDEO_ENCODE_GOP_ADAPTIVE_B_MODE
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_B_Frame_Enable_Temporal_Scalability_007, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_TEMPORAL_SCALABILITY, true);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_TEMPORAL_GOP_SIZE, 4);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_TEMPORAL_GOP_REFERENCE_MODE,
                         static_cast<int32_t>(OH_TemporalGopReferenceMode::UNIFORMLY_SCALED_REFERENCE));
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_B_FRAME, INT32_MAX);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODE_B_FRAME_GOP_MODE,
        static_cast<int32_t>(Media::Plugins::VideoEncodeBFrameGopMode::VIDEO_ENCODE_GOP_ADAPTIVE_B_MODE));
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_B_Frame_Enable_Temporal_Scalability_008
 * @tc.desc: set key VIDEO_ENCODER_ENABLE_B_FRAME and VideoEncodeBFrameGopMode,enable temporal scalability,
 * value is INT32_MAX and VIDEO_ENCODE_GOP_H3B_MODE
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_B_Frame_Enable_Temporal_Scalability_008, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_TEMPORAL_SCALABILITY, true);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_TEMPORAL_GOP_SIZE, 4);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_TEMPORAL_GOP_REFERENCE_MODE,
                         static_cast<int32_t>(OH_TemporalGopReferenceMode::UNIFORMLY_SCALED_REFERENCE));
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_B_FRAME, INT32_MAX);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODE_B_FRAME_GOP_MODE,
        static_cast<int32_t>(Media::Plugins::VideoEncodeBFrameGopMode::VIDEO_ENCODE_GOP_H3B_MODE));
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_B_Frame_Enable_Temporal_Scalability_009
 * @tc.desc: set key VIDEO_ENCODER_ENABLE_B_FRAME and VideoEncodeBFrameGopMode,enable temporal scalability,
 * value is INT32_MIN and VIDEO_ENCODE_GOP_ADAPTIVE_B_MODE
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_B_Frame_Enable_Temporal_Scalability_009, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_TEMPORAL_SCALABILITY, true);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_TEMPORAL_GOP_SIZE, 4);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_TEMPORAL_GOP_REFERENCE_MODE,
                         static_cast<int32_t>(OH_TemporalGopReferenceMode::UNIFORMLY_SCALED_REFERENCE));
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_B_FRAME, INT32_MIN);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODE_B_FRAME_GOP_MODE,
        static_cast<int32_t>(Media::Plugins::VideoEncodeBFrameGopMode::VIDEO_ENCODE_GOP_ADAPTIVE_B_MODE));
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_B_Frame_Enable_Temporal_Scalability_010
 * @tc.desc: set key VIDEO_ENCODER_ENABLE_B_FRAME and VideoEncodeBFrameGopMode,enable temporal scalability,
 * value is INT32_MIN and VIDEO_ENCODE_GOP_H3B_MODE
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_B_Frame_Enable_Temporal_Scalability_010, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_TEMPORAL_SCALABILITY, true);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_TEMPORAL_GOP_SIZE, 4);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_TEMPORAL_GOP_REFERENCE_MODE,
                         static_cast<int32_t>(OH_TemporalGopReferenceMode::UNIFORMLY_SCALED_REFERENCE));
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_B_FRAME, INT32_MIN);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODE_B_FRAME_GOP_MODE,
        static_cast<int32_t>(Media::Plugins::VideoEncodeBFrameGopMode::VIDEO_ENCODE_GOP_H3B_MODE));
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_B_Frame_Enable_Temporal_Scalability_011
 * @tc.desc: set key VIDEO_ENCODER_ENABLE_B_FRAME and VideoEncodeBFrameGopMode,enable temporal scalability,
 * value is true and VIDEO_ENCODE_GOP_ADAPTIVE_B_MODE
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_B_Frame_Enable_Temporal_Scalability_011, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_TEMPORAL_SCALABILITY, true);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_TEMPORAL_GOP_SIZE, 4);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_TEMPORAL_GOP_REFERENCE_MODE,
                         static_cast<int32_t>(OH_TemporalGopReferenceMode::UNIFORMLY_SCALED_REFERENCE));
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_B_FRAME, true);
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_B_Frame_Enable_Temporal_Scalability_012
 * @tc.desc: set key VIDEO_ENCODER_ENABLE_B_FRAME and VideoEncodeBFrameGopMode,enable temporal scalability,
 * value is false
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_B_Frame_Enable_Temporal_Scalability_012, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_TEMPORAL_SCALABILITY, true);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_TEMPORAL_GOP_SIZE, 4);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_TEMPORAL_GOP_REFERENCE_MODE,
                         static_cast<int32_t>(OH_TemporalGopReferenceMode::UNIFORMLY_SCALED_REFERENCE));
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_B_FRAME, false);
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_B_Frame_Enable_Temporal_Scalability_013
 * @tc.desc: set key VIDEO_ENCODER_ENABLE_B_FRAME and VideoEncodeBFrameGopMode,enable temporal scalability,
 * value is -1
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_B_Frame_Enable_Temporal_Scalability_013, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_TEMPORAL_SCALABILITY, true);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_TEMPORAL_GOP_SIZE, 4);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_TEMPORAL_GOP_REFERENCE_MODE,
                         static_cast<int32_t>(OH_TemporalGopReferenceMode::UNIFORMLY_SCALED_REFERENCE));
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_B_FRAME, -1);
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_B_Frame_Enable_Temporal_Scalability_014
 * @tc.desc: set key VIDEO_ENCODER_ENABLE_B_FRAME and VideoEncodeBFrameGopMode,enable temporal scalability,
 * value is INT32_MIN
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_B_Frame_Enable_Temporal_Scalability_014, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_TEMPORAL_SCALABILITY, true);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_TEMPORAL_GOP_SIZE, 4);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_TEMPORAL_GOP_REFERENCE_MODE,
                         static_cast<int32_t>(OH_TemporalGopReferenceMode::UNIFORMLY_SCALED_REFERENCE));
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_B_FRAME, INT32_MIN);
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_B_Frame_Enable_Temporal_Scalability_015
 * @tc.desc: set key VIDEO_ENCODER_ENABLE_B_FRAME and VideoEncodeBFrameGopMode,enable temporal scalability,
 * value is INT32_MAX
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_B_Frame_Enable_Temporal_Scalability_015, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_TEMPORAL_SCALABILITY, true);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_TEMPORAL_GOP_SIZE, 4);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_TEMPORAL_GOP_REFERENCE_MODE,
                         static_cast<int32_t>(OH_TemporalGopReferenceMode::UNIFORMLY_SCALED_REFERENCE));
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_B_FRAME, INT32_MAX);
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_B_Frame_LTR_LTR_001
 * @tc.desc: set key VIDEO_ENCODER_ENABLE_B_FRAME, VideoEncodeBFrameGopMode, VIDEO_ENCODER_LTR_FRAME_COUNT,
 * value is true, VIDEO_ENCODE_GOP_ADAPTIVE_B_MODE, 2
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_B_Frame_LTR_001, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_LTR_FRAME_COUNT, 2);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_B_FRAME, true);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODE_B_FRAME_GOP_MODE,
        static_cast<int32_t>(Media::Plugins::VideoEncodeBFrameGopMode::VIDEO_ENCODE_GOP_ADAPTIVE_B_MODE));
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_B_Frame_LTR_002
 * @tc.desc: set key VIDEO_ENCODER_ENABLE_B_FRAME and VideoEncodeBFrameGopMode, VIDEO_ENCODER_LTR_FRAME_COUNT
 * value is true and VIDEO_ENCODE_GOP_H3B_MODE, 2
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_B_Frame_LTR_002, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_LTR_FRAME_COUNT, 2);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_B_FRAME, true);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODE_B_FRAME_GOP_MODE,
        static_cast<int32_t>(Media::Plugins::VideoEncodeBFrameGopMode::VIDEO_ENCODE_GOP_H3B_MODE));
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_B_Frame_LTR_003
 * @tc.desc: set key VIDEO_ENCODER_ENABLE_B_FRAME and VideoEncodeBFrameGopMode, VIDEO_ENCODER_LTR_FRAME_COUNT
 * value is true and VIDEO_ENCODE_GOP_ADAPTIVE_B_MODE, 2
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_B_Frame_LTR_003, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_LTR_FRAME_COUNT, 2);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_B_FRAME, false);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODE_B_FRAME_GOP_MODE,
        static_cast<int32_t>(Media::Plugins::VideoEncodeBFrameGopMode::VIDEO_ENCODE_GOP_ADAPTIVE_B_MODE));
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_B_Frame_LTR_004
 * @tc.desc: set key VIDEO_ENCODER_ENABLE_B_FRAME and VideoEncodeBFrameGopMode, VIDEO_ENCODER_LTR_FRAME_COUNT
 * value is true and VIDEO_ENCODE_GOP_H3B_MODE, 2
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_B_Frame_LTR_004, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_LTR_FRAME_COUNT, 2);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_B_FRAME, false);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODE_B_FRAME_GOP_MODE,
        static_cast<int32_t>(Media::Plugins::VideoEncodeBFrameGopMode::VIDEO_ENCODE_GOP_H3B_MODE));
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_B_Frame_LTR_005
 * @tc.desc: set key VIDEO_ENCODER_ENABLE_B_FRAME and VideoEncodeBFrameGopMode, VIDEO_ENCODER_LTR_FRAME_COUNT
 * value is true and VIDEO_ENCODE_GOP_ADAPTIVE_B_MODE, 2
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_B_Frame_LTR_005, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_LTR_FRAME_COUNT, 2);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_B_FRAME, -1);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODE_B_FRAME_GOP_MODE,
        static_cast<int32_t>(Media::Plugins::VideoEncodeBFrameGopMode::VIDEO_ENCODE_GOP_ADAPTIVE_B_MODE));
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_B_Frame_LTR_006
 * @tc.desc: set key VIDEO_ENCODER_ENABLE_B_FRAME and VideoEncodeBFrameGopMode, VIDEO_ENCODER_LTR_FRAME_COUNT
 * value is true and VIDEO_ENCODE_GOP_H3B_MODE, 2
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_B_Frame_LTR_006, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_LTR_FRAME_COUNT, 2);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_B_FRAME, -1);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODE_B_FRAME_GOP_MODE,
        static_cast<int32_t>(Media::Plugins::VideoEncodeBFrameGopMode::VIDEO_ENCODE_GOP_H3B_MODE));
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_B_Frame_LTR_007
 * @tc.desc: set key VIDEO_ENCODER_ENABLE_B_FRAME and VideoEncodeBFrameGopMode, VIDEO_ENCODER_LTR_FRAME_COUNT
 * value is INT32_MAX and VIDEO_ENCODE_GOP_ADAPTIVE_B_MODE, 2
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_B_Frame_LTR_007, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_LTR_FRAME_COUNT, 2);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_B_FRAME, INT32_MAX);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODE_B_FRAME_GOP_MODE,
        static_cast<int32_t>(Media::Plugins::VideoEncodeBFrameGopMode::VIDEO_ENCODE_GOP_ADAPTIVE_B_MODE));
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_B_Frame_LTR_008
 * @tc.desc: set key VIDEO_ENCODER_ENABLE_B_FRAME and VideoEncodeBFrameGopMode, VIDEO_ENCODER_LTR_FRAME_COUNT
 * value is INT32_MAX and VIDEO_ENCODE_GOP_H3B_MODE, 2
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_B_Frame_LTR_008, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_LTR_FRAME_COUNT, 2);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_B_FRAME, INT32_MAX);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODE_B_FRAME_GOP_MODE,
        static_cast<int32_t>(Media::Plugins::VideoEncodeBFrameGopMode::VIDEO_ENCODE_GOP_H3B_MODE));
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_B_Frame_LTR_009
 * @tc.desc: set key VIDEO_ENCODER_ENABLE_B_FRAME and VideoEncodeBFrameGopMode, VIDEO_ENCODER_LTR_FRAME_COUNT
 * value is INT32_MIN and VIDEO_ENCODE_GOP_ADAPTIVE_B_MODE, 2
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_B_Frame_LTR_009, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_LTR_FRAME_COUNT, 2);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_B_FRAME, INT32_MIN);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODE_B_FRAME_GOP_MODE,
        static_cast<int32_t>(Media::Plugins::VideoEncodeBFrameGopMode::VIDEO_ENCODE_GOP_ADAPTIVE_B_MODE));
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_B_Frame_LTR_010
 * @tc.desc: set key VIDEO_ENCODER_ENABLE_B_FRAME and VideoEncodeBFrameGopMode, VIDEO_ENCODER_LTR_FRAME_COUNT
 * value is INT32_MIN and VIDEO_ENCODE_GOP_H3B_MODE, 2
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_B_Frame_LTR_010, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_LTR_FRAME_COUNT, 2);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_B_FRAME, INT32_MIN);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODE_B_FRAME_GOP_MODE,
        static_cast<int32_t>(Media::Plugins::VideoEncodeBFrameGopMode::VIDEO_ENCODE_GOP_H3B_MODE));
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_B_Frame_LTR_011
 * @tc.desc: set key VIDEO_ENCODER_ENABLE_B_FRAME and VideoEncodeBFrameGopMode, VIDEO_ENCODER_LTR_FRAME_COUNT
 * value is true and VIDEO_ENCODE_GOP_ADAPTIVE_B_MODE, 2
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_B_Frame_LTR_011, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_LTR_FRAME_COUNT, 2);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_B_FRAME, true);
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_B_Frame_LTR_012
 * @tc.desc: set key VIDEO_ENCODER_ENABLE_B_FRAME, VIDEO_ENCODER_LTR_FRAME_COUNT
 * value is false, 2
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_B_Frame_LTR_012, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_LTR_FRAME_COUNT, 2);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_B_FRAME, false);
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_B_Frame_LTR_013
 * @tc.desc: set key VIDEO_ENCODER_ENABLE_B_FRAME, VIDEO_ENCODER_LTR_FRAME_COUNT
 * value is -1, 2
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_B_Frame_LTR_013, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_LTR_FRAME_COUNT, 2);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_B_FRAME, -1);
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_B_Frame_LTR_014
 * @tc.desc: set key VIDEO_ENCODER_ENABLE_B_FRAME, VIDEO_ENCODER_LTR_FRAME_COUNT
 * value is INT32_MIN, 2
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_B_Frame_LTR_014, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_LTR_FRAME_COUNT, 2);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_B_FRAME, INT32_MIN);
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_B_Frame_LTR_015
 * @tc.desc: set key VIDEO_ENCODER_ENABLE_B_FRAME, VIDEO_ENCODER_LTR_FRAME_COUNT
 * value is INT32_MAX, 2
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_B_Frame_LTR_015, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_LTR_FRAME_COUNT, 2);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_B_FRAME, INT32_MAX);
    ASSERT_EQ(AVCS_ERR_OK, videoEnc_->Configure(format_));
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