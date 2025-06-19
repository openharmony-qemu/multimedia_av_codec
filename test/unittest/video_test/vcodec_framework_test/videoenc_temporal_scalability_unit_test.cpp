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
#include "avcodec_log.h"
#include "codeclist_mock.h"
#include "native_avmagic.h"
#include "meta/meta_key.h"
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
using namespace OHOS::MediaAVCodec::VCodecTestParam;
using namespace OHOS::Media;

namespace VFTSUIT {
constexpr int32_t DEFAULT_LTR_COUNT = 4;
constexpr int32_t DEFAULT_INVALID_LTR_COUNT = 1000;
constexpr int32_t DEFAULT_LTR_INTERVAL = 4;

void TEST_SUIT::SetUpTestCase(void)
{
    auto capability = CodecListMockFactory::GetCapabilityByCategory((CodecMimeType::VIDEO_AVC).data(), true,
                                                                    AVCodecCategory::AVCODEC_HARDWARE);
    ASSERT_NE(nullptr, capability) << (CodecMimeType::VIDEO_AVC).data() << " can not found!" << std::endl;
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

bool TEST_SUIT::GetTemporalScalabilityCapability(int32_t param, bool isTemporalScalability)
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
    if (isTemporalScalability && capabilityData->featuresMap.count(
        static_cast<int32_t>(AVCapabilityFeature::VIDEO_ENCODER_TEMPORAL_SCALABILITY))) {
        std::cout << "Support TemporalScalability" << std::endl;
        return true;
    } else if (!isTemporalScalability) {
        auto ltrCap =
        capabilityData->featuresMap.find(
            static_cast<int32_t>(AVCapabilityFeature::VIDEO_ENCODER_LONG_TERM_REFERENCE));
        if (ltrCap == capabilityData->featuresMap.end()) {
            return false;
        }
        int32_t maxLTRFrameCount = 0;
        bool maxLTRFrameCountExist =
            ltrCap->second.GetIntValue(Tag::FEATURE_PROPERTY_VIDEO_ENCODER_MAX_LTR_FRAME_COUNT, maxLTRFrameCount);
        if (!maxLTRFrameCountExist || maxLTRFrameCount < DEFAULT_LTR_COUNT) {
            return false;
        }
        std::cout << "Support LongTermReference" << std::endl;
        return true;
    } else {
        std::cout << "Not support TemporalScalability" << std::endl;
        return false;
    }
}

INSTANTIATE_TEST_SUITE_P(, TEST_SUIT, testing::Values(HW_AVC, HW_HEVC));

/**
 * @tc.name: VideoEncoder_TemporalScalability_001
 * @tc.desc: unable temporal scalability encode, buffer mode
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_TemporalScalability_001, TestSize.Level1)
{
    if (!GetTemporalScalabilityCapability(GetParam(), true)) {
        return;
    };
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
    if (!GetTemporalScalabilityCapability(GetParam(), true)) {
        return;
    };
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
    if (!GetTemporalScalabilityCapability(GetParam(), true)) {
        return;
    };
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
    if (!GetTemporalScalabilityCapability(GetParam(), true)) {
        return;
    };
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
    if (!GetTemporalScalabilityCapability(GetParam(), true)) {
        return;
    };
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
    if (!GetTemporalScalabilityCapability(GetParam(), true)) {
        return;
    };
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
    if (!GetTemporalScalabilityCapability(GetParam(), true)) {
        return;
    };
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
    if (!GetTemporalScalabilityCapability(GetParam(), true)) {
        return;
    };
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
    if (!GetTemporalScalabilityCapability(GetParam(), true)) {
        return;
    };
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
    if (!GetTemporalScalabilityCapability(GetParam(), true)) {
        return;
    };
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
    if (!GetTemporalScalabilityCapability(GetParam(), true)) {
        return;
    };
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
    if (!GetTemporalScalabilityCapability(GetParam(), true)) {
        return;
    };
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
    if (!GetTemporalScalabilityCapability(GetParam(), true)) {
        return;
    };
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
    if (!GetTemporalScalabilityCapability(GetParam(), true)) {
        return;
    };
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
    if (!GetTemporalScalabilityCapability(GetParam(), true)) {
        return;
    };
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
    if (!GetTemporalScalabilityCapability(GetParam(), true)) {
        return;
    };
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
    if (!GetTemporalScalabilityCapability(GetParam(), true)) {
        return;
    };
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
    if (!GetTemporalScalabilityCapability(GetParam(), true)) {
        return;
    };
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
    if (!GetTemporalScalabilityCapability(GetParam(), true)) {
        return;
    };
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
    if (!GetTemporalScalabilityCapability(GetParam(), true)) {
        return;
    };
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_TEMPORAL_SCALABILITY, 1);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_TEMPORAL_GOP_REFERENCE_MODE,
                         static_cast<int32_t>(OH_TemporalGopReferenceMode::UNIFORMLY_SCALED_REFERENCE));
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_TEMPORAL_GOP_SIZE, 2);
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    ASSERT_EQ(AV_ERR_OK, videoEnc_->CreateInputSurface());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

HWTEST_P(TEST_SUIT, VideoEncoder_TemporalScalability_UNIFORMLY_04, TestSize.Level1)
{
    if (!GetTemporalScalabilityCapability(GetParam(), true)) {
        return;
    };
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_ENABLE_TEMPORAL_SCALABILITY, 1);
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_TEMPORAL_GOP_REFERENCE_MODE,
                         static_cast<int32_t>(OH_TemporalGopReferenceMode::UNIFORMLY_SCALED_REFERENCE));
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_TEMPORAL_GOP_SIZE, 4);
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_Feature_Long_Term_Reference_001
 * @tc.desc: enable feature long term reference by buffer mode
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_Feature_Long_Term_Reference_001, TestSize.Level1)
{
    if (!GetTemporalScalabilityCapability(GetParam(), false)) {
        return;
    };
    videoEnc_->isAVBufferMode_ = true;
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    videoEnc_->ltrParam.enableUseLtr = true;
    videoEnc_->ltrParam.ltrInterval = DEFAULT_LTR_INTERVAL;
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_LTR_FRAME_COUNT, DEFAULT_LTR_COUNT);
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_Feature_Long_Term_Reference_002
 * @tc.desc: enable feature long term reference by surface mode
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_Feature_Long_Term_Reference_002, TestSize.Level1)
{
    if (!GetTemporalScalabilityCapability(GetParam(), false)) {
        return;
    };
    videoEnc_->isAVBufferMode_ = true;
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoEnc_->SetCallback(vencParamCallback_));
    videoEnc_->ltrParam.enableUseLtr = true;
    videoEnc_->ltrParam.ltrInterval = DEFAULT_LTR_INTERVAL;
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_LTR_FRAME_COUNT, DEFAULT_LTR_COUNT);
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    ASSERT_EQ(AV_ERR_OK, videoEnc_->CreateInputSurface());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: VideoEncoder_Feature_Long_Term_Reference_003
 * @tc.desc: ltr frame count is invalid
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_Feature_Long_Term_Reference_003, TestSize.Level1)
{
    if (!GetTemporalScalabilityCapability(GetParam(), false)) {
        return;
    };
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    format_->PutIntValue(Media::Tag::VIDEO_ENCODER_LTR_FRAME_COUNT, DEFAULT_INVALID_LTR_COUNT);
    ASSERT_EQ(AV_ERR_INVALID_VAL, videoEnc_->Configure(format_));
}
} // namespace

int main(int argc, char **argv)
{
    testing::GTEST_FLAG(output) = "xml:./";
    for (int i = 0; i < argc; ++i) {
        cout << argv[i] << endl;
        if (strcmp(argv[i], "--need_dump") == 0) {
            VideoEncAsyncSample::needDump_ = true;
            DecArgv(i, argc, argv);
        }
    }
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}