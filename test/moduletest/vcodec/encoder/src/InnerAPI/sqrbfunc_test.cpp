
/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
#include <iostream>
#include <cstdio>
#include <string>

#include "gtest/gtest.h"
#include "avcodec_common.h"
#include "meta/format.h"
#include "avcodec_video_encoder.h"
#include "videoenc_inner_sample.h"
#include "native_avcapability.h"
#include "avcodec_info.h"
#include "avcodec_list.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace testing::ext;

namespace {
class HwEncInnerFuncBTest : public testing::Test {
public:
    // SetUpTestCase: Called before all test cases
    static void SetUpTestCase(void);
    // TearDownTestCase: Called after all test case
    static void TearDownTestCase(void);
    // SetUp: Called before each test cases
    void SetUp() override;
    // TearDown: Called after each test cases
    void TearDown() override;
};

std::string g_codecMimeHevc = "video/hevc";
std::string g_codecNameHevc = "";


void HwEncInnerFuncBTest::SetUpTestCase()
{
    OH_AVCapability *capHevc = OH_AVCodec_GetCapabilityByCategory(g_codecMimeHevc.c_str(), true, HARDWARE);
    const char *tmpCodecNameHevc = OH_AVCapability_GetName(capHevc);
    g_codecNameHevc = tmpCodecNameHevc;
    cout << "g_codecNameHevc: " << g_codecNameHevc << endl;
}

void HwEncInnerFuncBTest::TearDownTestCase() {}
void HwEncInnerFuncBTest::SetUp() {}
void HwEncInnerFuncBTest::TearDown() {}
} // namespace

namespace {
bool IsEncoderBitrateModeSupported(CapabilityData *capData, VideoEncodeBitrateMode bitrateMode)
{
    if (!AVCodecInfo::isEncoder(capData->codecType)) {
        return false;
    }
    std::shared_ptr<VideoCaps> codecInfo = std::make_shared<VideoCaps>(capData);
    const auto &bitrateModeVec = codecInfo->GetSupportedBitrateMode();
    return find(bitrateModeVec.begin(), bitrateModeVec.end(), bitrateMode) != bitrateModeVec.end();
}

/**
 * @tc.number    : VIDEO_ENCODE_INNER_REPEAT_FUNC_0100
 * @tc.name      : config bframe
 * @tc.desc      : function test
 */
HWTEST_F(HwEncInnerFuncBTest, VIDEO_HW_ENCODE_INNER_B_FUNC_001, TestSize.Level0)
{
    Format format;
    std::shared_ptr<AVCodecList> codecCapability = AVCodecListFactory::CreateAVCodecList();
    CapabilityData *capabilityData = nullptr;
    capabilityData = codecCapability->GetCapability(g_codecMimeHevc, true, AVCodecCategory::AVCODEC_HARDWARE);
    std::shared_ptr<AVCodecInfo> codecInfo = std::make_shared<AVCodecInfo>(capabilityData);
    bool retBsupport = codecInfo->IsFeatureSupported(AVCapabilityFeature::VIDEO_ENCODER_B_FRAME);
    if (!retBsupport) {
        return;
    }
    auto vEncInnerSample = make_unique<VEncNdkInnerSample>();
    vEncInnerSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
    vEncInnerSample->DEFAULT_WIDTH = 1280;
    vEncInnerSample->DEFAULT_HEIGHT = 720;
    vEncInnerSample->DEFAULT_BITRATE_MODE = CBR;
    vEncInnerSample->MODE_ENABLE = true;
    vEncInnerSample->GOPMODE_ENABLE = true;
    vEncInnerSample->B_ENABLE = true;
    vEncInnerSample->DEFAULT_BFRAME = 1;
    vEncInnerSample->DEFAULT_GOP_MODE = 1;
    vEncInnerSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_B_FUNC_001.h265";
    int32_t bframe = codecInfo->GetFeatureProperties(AVCapabilityFeature::VIDEO_ENCODER_B_FRAME, format);
    ASSERT_EQ(bframe, AV_ERR_OK);
    int32_t value = 0;
    format.GetIntValue(Media::Tag::VIDEO_ENCODER_MAX_B_FRAME, value);
    ASSERT_GT(value, 0);
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->CreateByName(g_codecNameHevc));
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->SetCallback());
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->ConfigureVideoEncoderSqr());
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->StartVideoEncoder());
    vEncInnerSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->errCount);
}

/**
 * @tc.number    : VIDEO_ENCODE_INNER_REPEAT_FUNC_002
 * @tc.name      : config bframe
 * @tc.desc      : function test
 */
HWTEST_F(HwEncInnerFuncBTest, VIDEO_HW_ENCODE_INNER_B_FUNC_002, TestSize.Level0)
{
    Format format;
    std::shared_ptr<AVCodecList> codecCapability = AVCodecListFactory::CreateAVCodecList();
    CapabilityData *capabilityData = nullptr;
    capabilityData = codecCapability->GetCapability(g_codecMimeHevc, true, AVCodecCategory::AVCODEC_HARDWARE);
    std::shared_ptr<AVCodecInfo> codecInfo = std::make_shared<AVCodecInfo>(capabilityData);
    bool retBsupport = codecInfo->IsFeatureSupported(AVCapabilityFeature::VIDEO_ENCODER_B_FRAME);
    if (!retBsupport) {
        return;
    }
    auto vEncInnerSample = make_unique<VEncNdkInnerSample>();
    vEncInnerSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
    vEncInnerSample->DEFAULT_WIDTH = 1280;
    vEncInnerSample->DEFAULT_HEIGHT = 720;
    vEncInnerSample->DEFAULT_BITRATE_MODE = CBR;
    vEncInnerSample->MODE_ENABLE = true;
    vEncInnerSample->B_ENABLE = true;
    vEncInnerSample->DEFAULT_BFRAME = 1;
    vEncInnerSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_B_FUNC_002.h265";
    int32_t bframe = codecInfo->GetFeatureProperties(AVCapabilityFeature::VIDEO_ENCODER_B_FRAME, format);
    ASSERT_EQ(bframe, AV_ERR_OK);
    int32_t value = 0;
    format.GetIntValue(Media::Tag::VIDEO_ENCODER_MAX_B_FRAME, value);
    ASSERT_GT(value, 0);
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->CreateByName(g_codecNameHevc));
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->SetCallback());
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->ConfigureVideoEncoderSqr());
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->StartVideoEncoder());
    vEncInnerSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->errCount);
}

/**
 * @tc.number    : VIDEO_HW_ENCODE_INNER_SQR_FUNC_001
 * @tc.name      : setparemeter sqrfactor and max bitrate
 * @tc.desc      : function test
 */
HWTEST_F(HwEncInnerFuncBTest, VIDEO_HW_ENCODE_INNER_SQR_FUNC_001, TestSize.Level0)
{
    Format format;
    std::shared_ptr<AVCodecList> codecCapability = AVCodecListFactory::CreateAVCodecList();
    CapabilityData *capabilityData = nullptr;
    capabilityData = codecCapability->GetCapability(g_codecMimeHevc, true, AVCodecCategory::AVCODEC_HARDWARE);
    bool sqrSupport = IsEncoderBitrateModeSupported(capabilityData, SQR);
    if (!sqrSupport) {
        return;
    }
    auto vEncInnerSample = make_unique<VEncNdkInnerSample>();
    vEncInnerSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
    vEncInnerSample->DEFAULT_WIDTH = 1280;
    vEncInnerSample->DEFAULT_HEIGHT = 720;
    vEncInnerSample->DEFAULT_BITRATE_MODE = SQR;
    vEncInnerSample->surfaceInput = true;
    vEncInnerSample->FACTOR_ENABLE = true;
    vEncInnerSample->MAXBITE_ENABLE = true;
    vEncInnerSample->MODE_ENABLE = true;
    vEncInnerSample->enableParameter = true;
    vEncInnerSample->MAXBITE_ENABLE_RUN = true;
    vEncInnerSample->FACTOR_ENABLE_RUN = true;
    vEncInnerSample->DEFAULT_MAX_BITERATE = 100000000;
    vEncInnerSample->DEFAULT_SQR_FACTOR = 32;
    vEncInnerSample->DEFAULT_MAX_BITERATE_RUN = 1000000;
    vEncInnerSample->DEFAULT_SQR_FACTOR_RUN = 51;
    vEncInnerSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_SQR_FUNC_001.h265";
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->CreateByName(g_codecNameHevc));
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->SetCallback());
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->ConfigureVideoEncoderSqr());
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->StartVideoEncoder());
    vEncInnerSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->errCount);
}

/**
 * @tc.number    : VIDEO_HW_ENCODE_INNER_SQR_FUNC_002
 * @tc.name      : setparemeter sqrfactor and max bitrate
 * @tc.desc      : function test
 */
HWTEST_F(HwEncInnerFuncBTest, VIDEO_HW_ENCODE_INNER_SQR_FUNC_002, TestSize.Level0)
{
    Format format;
    std::shared_ptr<AVCodecList> codecCapability = AVCodecListFactory::CreateAVCodecList();
    CapabilityData *capabilityData = nullptr;
    capabilityData = codecCapability->GetCapability(g_codecMimeHevc, true, AVCodecCategory::AVCODEC_HARDWARE);
    bool sqrSupport = IsEncoderBitrateModeSupported(capabilityData, SQR);
    if (!sqrSupport) {
        return;
    }
    auto vEncInnerSample = make_unique<VEncNdkInnerSample>();
    vEncInnerSample->INP_DIR = "/data/test/media/1280_720_nv.yuv";
    vEncInnerSample->DEFAULT_WIDTH = 1280;
    vEncInnerSample->DEFAULT_HEIGHT = 720;
    vEncInnerSample->DEFAULT_BITRATE_MODE = SQR;
    vEncInnerSample->FACTOR_ENABLE = true;
    vEncInnerSample->MAXBITE_ENABLE = true;
    vEncInnerSample->MODE_ENABLE = true;
    vEncInnerSample->enableParameter = true;
    vEncInnerSample->MAXBITE_ENABLE_RUN = true;
    vEncInnerSample->FACTOR_ENABLE_RUN = true;
    vEncInnerSample->DEFAULT_MAX_BITERATE = 6000000;
    vEncInnerSample->DEFAULT_SQR_FACTOR = 32;
    vEncInnerSample->DEFAULT_MAX_BITERATE_RUN = 100000000;
    vEncInnerSample->DEFAULT_SQR_FACTOR_RUN = 1;
    vEncInnerSample->OUT_DIR = "/data/test/media/VIDEO_ENCODE_INNER_SQR_FUNC_002.h265";
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->CreateByName(g_codecNameHevc));
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->SetCallback());
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->ConfigureVideoEncoderSqr());
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->StartVideoEncoder());
    vEncInnerSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncInnerSample->errCount);
}
} // namespace