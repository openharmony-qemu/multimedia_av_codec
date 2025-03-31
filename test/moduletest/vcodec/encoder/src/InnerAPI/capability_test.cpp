/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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
class HwEncInnerCapNdkTest : public testing::Test {
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

constexpr uint32_t DEFAULT_WIDTH = 1920;
constexpr uint32_t DEFAULT_HEIGHT = 1080;
std::string g_codecMime = "video/hevc";
std::string g_codecName = "";
std::shared_ptr<AVCodecVideoEncoder> venc_ = nullptr;
std::shared_ptr<VEncInnerSignal> signal_ = nullptr;
void HwEncInnerCapNdkTest::SetUpTestCase()
{
    OH_AVCapability *cap = OH_AVCodec_GetCapabilityByCategory(g_codecMime.c_str(), true, HARDWARE);
    const char *tmpCodecName = OH_AVCapability_GetName(cap);
    g_codecName = tmpCodecName;
    cout << "g_codecName: " << g_codecName << endl;
}

void HwEncInnerCapNdkTest::TearDownTestCase() {}

void HwEncInnerCapNdkTest::SetUp()
{
    signal_ = make_shared<VEncInnerSignal>();
}

void HwEncInnerCapNdkTest::TearDown()
{
    if (venc_ != nullptr) {
        venc_->Release();
        venc_ = nullptr;
    }

    if (signal_) {
        signal_ = nullptr;
    }
}
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
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_0010
 * @tc.name      : GetSupportedMaxBitrate param correct,Configure
 * @tc.desc      : api test
 */
HWTEST_F(HwEncInnerCapNdkTest, VIDEO_ENCODE_CAPABILITY_API_0010, TestSize.Level0)
{
    std::shared_ptr<AVCodecList> codecCapability = AVCodecListFactory::CreateAVCodecList();
    CapabilityData *capabilityData = nullptr;
    Format format;
    capabilityData = codecCapability->GetCapability(g_codecMime, true, AVCodecCategory::AVCODEC_HARDWARE);
    std::shared_ptr<VideoCaps> codecInfo = std::make_shared<VideoCaps>(capabilityData);
    if (IsEncoderBitrateModeSupported(capabilityData, SQR)) {
        venc_ = VideoEncoderFactory::CreateByName(g_codecName);
        ASSERT_NE(nullptr, venc_);
        ASSERT_GT(codecInfo->GetSupportedMaxBitrate().minVal, 0);
        ASSERT_GT(codecInfo->GetSupportedMaxBitrate().maxVal, 0);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, SQR);
        format.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE,
            codecInfo->GetSupportedMaxBitrate().maxVal + 1);
        ASSERT_EQ(venc_->Configure(format), AVCS_ERR_CODEC_PARAM_INCORRECT);
        venc_->Release();

        venc_ = VideoEncoderFactory::CreateByName(g_codecName);
        ASSERT_NE(nullptr, venc_);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, SQR);
        format.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE,
            codecInfo->GetSupportedMaxBitrate().minVal - 1);
        ASSERT_EQ(venc_->Configure(format), AVCS_ERR_CODEC_PARAM_INCORRECT);
        venc_->Release();

        venc_ = VideoEncoderFactory::CreateByName(g_codecName);
        ASSERT_NE(nullptr, venc_);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, SQR);
        format.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE,
            codecInfo->GetSupportedMaxBitrate().minVal);
        ASSERT_EQ(venc_->Configure(format), AVCS_ERR_OK);
        venc_->Release();
    } else {
        ASSERT_EQ(0, codecInfo->GetSupportedMaxBitrate().minVal);
        ASSERT_EQ(0, codecInfo->GetSupportedMaxBitrate().maxVal);
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_0020
 * @tc.name      : GetSupportedSqrFactor param correct,Configure
 * @tc.desc      : api test
 */
HWTEST_F(HwEncInnerCapNdkTest, VIDEO_ENCODE_CAPABILITY_API_0020, TestSize.Level1)
{
    std::shared_ptr<AVCodecList> codecCapability = AVCodecListFactory::CreateAVCodecList();
    CapabilityData *capabilityData = nullptr;
    Format format;
    capabilityData = codecCapability->GetCapability(g_codecMime, true, AVCodecCategory::AVCODEC_HARDWARE);
    std::shared_ptr<VideoCaps> codecInfo = std::make_shared<VideoCaps>(capabilityData);
    if (IsEncoderBitrateModeSupported(capabilityData, SQR)) {
        venc_ = VideoEncoderFactory::CreateByName(g_codecName);
        ASSERT_NE(nullptr, venc_);
        ASSERT_EQ(0, codecInfo->GetSupportedSqrFactor().minVal);
        ASSERT_GT(codecInfo->GetSupportedSqrFactor().maxVal, 0);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, SQR);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR,
            codecInfo->GetSupportedSqrFactor().maxVal + 1);
        ASSERT_EQ(venc_->Configure(format), AVCS_ERR_CODEC_PARAM_INCORRECT);
        venc_->Release();

        venc_ = VideoEncoderFactory::CreateByName(g_codecName);
        ASSERT_NE(nullptr, venc_);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, SQR);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR,
            codecInfo->GetSupportedSqrFactor().minVal - 1);
        ASSERT_EQ(venc_->Configure(format), AVCS_ERR_CODEC_PARAM_INCORRECT);
        venc_->Release();

        venc_ = VideoEncoderFactory::CreateByName(g_codecName);
        ASSERT_NE(nullptr, venc_);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, SQR);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR,
            codecInfo->GetSupportedSqrFactor().minVal);
        ASSERT_EQ(venc_->Configure(format), AVCS_ERR_OK);
        venc_->Release();
    } else {
        ASSERT_EQ(0, codecInfo->GetSupportedSqrFactor().minVal);
        ASSERT_EQ(0, codecInfo->GetSupportedSqrFactor().maxVal);
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_FUNC_0010
 * @tc.name      : 使能SQR_max_bitrate,confige
 * @tc.desc      : api test
 */
HWTEST_F(HwEncInnerCapNdkTest, VIDEO_ENCODE_CAPABILITY_FUNC_0010, TestSize.Level0)
{
    std::shared_ptr<AVCodecList> codecCapability = AVCodecListFactory::CreateAVCodecList();
    CapabilityData *capabilityData = nullptr;
    Format format;
    capabilityData = codecCapability->GetCapability(g_codecMime, true, AVCodecCategory::AVCODEC_HARDWARE);
    std::shared_ptr<VideoCaps> codecInfo = std::make_shared<VideoCaps>(capabilityData);
    if (IsEncoderBitrateModeSupported(capabilityData, SQR)) {
        venc_ = VideoEncoderFactory::CreateByName(g_codecName);
        ASSERT_NE(nullptr, venc_);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, SQR);
        ASSERT_EQ(venc_->Configure(format), AVCS_ERR_OK);
        venc_->Release();

        venc_ = VideoEncoderFactory::CreateByName(g_codecName);
        ASSERT_NE(nullptr, venc_);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, SQR);
        format.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE,
            codecInfo->GetSupportedMaxBitrate().maxVal);
        ASSERT_EQ(venc_->Configure(format), AVCS_ERR_OK);
        venc_->Release();

        venc_ = VideoEncoderFactory::CreateByName(g_codecName);
        ASSERT_NE(nullptr, venc_);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, SQR);
        format.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE,
            codecInfo->GetSupportedMaxBitrate().maxVal + 1);
        ASSERT_EQ(venc_->Configure(format), AVCS_ERR_CODEC_PARAM_INCORRECT);
        venc_->Release();
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_FUNC_0020
 * @tc.name      : 使能SQR_factor,confige
 * @tc.desc      : api test
 */
HWTEST_F(HwEncInnerCapNdkTest, VIDEO_ENCODE_CAPABILITY_FUNC_0020, TestSize.Level1)
{
    std::shared_ptr<AVCodecList> codecCapability = AVCodecListFactory::CreateAVCodecList();
    CapabilityData *capabilityData = nullptr;
    Format format;
    capabilityData = codecCapability->GetCapability(g_codecMime, true, AVCodecCategory::AVCODEC_HARDWARE);
    std::shared_ptr<VideoCaps> codecInfo = std::make_shared<VideoCaps>(capabilityData);
    if (IsEncoderBitrateModeSupported(capabilityData, SQR)) {
        venc_ = VideoEncoderFactory::CreateByName(g_codecName);
        ASSERT_NE(nullptr, venc_);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, SQR);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR,
            codecInfo->GetSupportedSqrFactor().maxVal);
        ASSERT_EQ(venc_->Configure(format), AVCS_ERR_OK);
        venc_->Release();

        venc_ = VideoEncoderFactory::CreateByName(g_codecName);
        ASSERT_NE(nullptr, venc_);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, SQR);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR,
            codecInfo->GetSupportedSqrFactor().maxVal + 1);
        ASSERT_EQ(venc_->Configure(format), AVCS_ERR_CODEC_PARAM_INCORRECT);
        venc_->Release();
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_FUNC_0030
 * @tc.name      : 使能SQR_max_bitrate_factor,confige
 * @tc.desc      : api test
 */
HWTEST_F(HwEncInnerCapNdkTest, VIDEO_ENCODE_CAPABILITY_FUNC_0030, TestSize.Level2)
{
    std::shared_ptr<AVCodecList> codecCapability = AVCodecListFactory::CreateAVCodecList();
    CapabilityData *capabilityData = nullptr;
    Format format;
    capabilityData = codecCapability->GetCapability(g_codecMime, true, AVCodecCategory::AVCODEC_HARDWARE);
    std::shared_ptr<VideoCaps> codecInfo = std::make_shared<VideoCaps>(capabilityData);
    if (IsEncoderBitrateModeSupported(capabilityData, SQR)) {
        venc_ = VideoEncoderFactory::CreateByName(g_codecName);
        ASSERT_NE(nullptr, venc_);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, SQR);
        format.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE,
            codecInfo->GetSupportedMaxBitrate().maxVal);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR,
            codecInfo->GetSupportedSqrFactor().maxVal);
        ASSERT_EQ(venc_->Configure(format), AVCS_ERR_OK);
        venc_->Release();

        venc_ = VideoEncoderFactory::CreateByName(g_codecName);
        ASSERT_NE(nullptr, venc_);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, SQR);
        format.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE,
            codecInfo->GetSupportedMaxBitrate().maxVal);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR,
            codecInfo->GetSupportedSqrFactor().maxVal + 1);
        ASSERT_EQ(venc_->Configure(format), AVCS_ERR_CODEC_PARAM_INCORRECT);
        venc_->Release();

        venc_ = VideoEncoderFactory::CreateByName(g_codecName);
        ASSERT_NE(nullptr, venc_);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, SQR);
        format.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE,
            codecInfo->GetSupportedMaxBitrate().maxVal + 1);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR,
            codecInfo->GetSupportedSqrFactor().maxVal);
        ASSERT_EQ(venc_->Configure(format), AVCS_ERR_CODEC_PARAM_INCORRECT);
        venc_->Release();

        venc_ = VideoEncoderFactory::CreateByName(g_codecName);
        ASSERT_NE(nullptr, venc_);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, SQR);
        format.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE,
            codecInfo->GetSupportedMaxBitrate().maxVal + 1);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR,
            codecInfo->GetSupportedSqrFactor().maxVal + 1);
        ASSERT_EQ(venc_->Configure(format), AVCS_ERR_CODEC_PARAM_INCORRECT);
        venc_->Release();
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_FUNC_0040
 * @tc.name      : 不使能SQR_max_bitrate,confige
 * @tc.desc      : api test
 */
HWTEST_F(HwEncInnerCapNdkTest, VIDEO_ENCODE_CAPABILITY_FUNC_0040, TestSize.Level0)
{
    std::shared_ptr<AVCodecList> codecCapability = AVCodecListFactory::CreateAVCodecList();
    CapabilityData *capabilityData = nullptr;
    Format format;
    capabilityData = codecCapability->GetCapability(g_codecMime, true, AVCodecCategory::AVCODEC_HARDWARE);
    std::shared_ptr<VideoCaps> codecInfo = std::make_shared<VideoCaps>(capabilityData);
    if (IsEncoderBitrateModeSupported(capabilityData, SQR)) {
        venc_ = VideoEncoderFactory::CreateByName(g_codecName);
        ASSERT_NE(nullptr, venc_);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
        format.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE,
            codecInfo->GetSupportedMaxBitrate().maxVal);
        ASSERT_EQ(venc_->Configure(format), AVCS_ERR_OK);
        venc_->Release();

        venc_ = VideoEncoderFactory::CreateByName(g_codecName);
        ASSERT_NE(nullptr, venc_);
        format.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE,
            codecInfo->GetSupportedMaxBitrate().maxVal + 1);
        ASSERT_EQ(venc_->Configure(format), AVCS_ERR_CODEC_PARAM_INCORRECT);
        venc_->Release();
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_FUNC_0050
 * @tc.name      : 不使能SQR_factor,confige
 * @tc.desc      : api test
 */
HWTEST_F(HwEncInnerCapNdkTest, VIDEO_ENCODE_CAPABILITY_FUNC_0050, TestSize.Level1)
{
    std::shared_ptr<AVCodecList> codecCapability = AVCodecListFactory::CreateAVCodecList();
    CapabilityData *capabilityData = nullptr;
    Format format;
    capabilityData = codecCapability->GetCapability(g_codecMime, true, AVCodecCategory::AVCODEC_HARDWARE);
    std::shared_ptr<VideoCaps> codecInfo = std::make_shared<VideoCaps>(capabilityData);
    if (IsEncoderBitrateModeSupported(capabilityData, SQR)) {
        venc_ = VideoEncoderFactory::CreateByName(g_codecName);
        ASSERT_NE(nullptr, venc_);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR,
            codecInfo->GetSupportedSqrFactor().maxVal);
        ASSERT_EQ(venc_->Configure(format), AVCS_ERR_OK);
        venc_->Release();

        venc_ = VideoEncoderFactory::CreateByName(g_codecName);
        ASSERT_NE(nullptr, venc_);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR,
            codecInfo->GetSupportedSqrFactor().maxVal + 1);
        ASSERT_EQ(venc_->Configure(format), AVCS_ERR_CODEC_PARAM_INCORRECT);
        venc_->Release();
    }
}
/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_FUNC_0060
 * @tc.name      : 不使能SQR_max_bitrate_factor,confige
 * @tc.desc      : api test
 */
HWTEST_F(HwEncInnerCapNdkTest, VIDEO_ENCODE_CAPABILITY_FUNC_0060, TestSize.Level2)
{
    std::shared_ptr<AVCodecList> codecCapability = AVCodecListFactory::CreateAVCodecList();
    CapabilityData *capabilityData = nullptr;
    Format format;
    capabilityData = codecCapability->GetCapability(g_codecMime, true, AVCodecCategory::AVCODEC_HARDWARE);
    std::shared_ptr<VideoCaps> codecInfo = std::make_shared<VideoCaps>(capabilityData);
    if (IsEncoderBitrateModeSupported(capabilityData, SQR)) {
        venc_ = VideoEncoderFactory::CreateByName(g_codecName);
        ASSERT_NE(nullptr, venc_);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
        format.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE,
            codecInfo->GetSupportedMaxBitrate().maxVal);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR,
            codecInfo->GetSupportedSqrFactor().maxVal);
        ASSERT_EQ(venc_->Configure(format), AVCS_ERR_OK);
        venc_->Release();

        venc_ = VideoEncoderFactory::CreateByName(g_codecName);
        ASSERT_NE(nullptr, venc_);
        format.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE,
            codecInfo->GetSupportedMaxBitrate().maxVal);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR,
            codecInfo->GetSupportedSqrFactor().maxVal + 1);
        ASSERT_EQ(venc_->Configure(format), AVCS_ERR_CODEC_PARAM_INCORRECT);
        venc_->Release();

        venc_ = VideoEncoderFactory::CreateByName(g_codecName);
        ASSERT_NE(nullptr, venc_);
        format.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE,
            codecInfo->GetSupportedMaxBitrate().maxVal + 1);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR,
            codecInfo->GetSupportedSqrFactor().maxVal);
        ASSERT_EQ(venc_->Configure(format), AVCS_ERR_CODEC_PARAM_INCORRECT);
        venc_->Release();

        venc_ = VideoEncoderFactory::CreateByName(g_codecName);
        ASSERT_NE(nullptr, venc_);
        format.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE,
            codecInfo->GetSupportedMaxBitrate().maxVal + 1);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR,
            codecInfo->GetSupportedSqrFactor().maxVal + 1);
        ASSERT_EQ(venc_->Configure(format), AVCS_ERR_CODEC_PARAM_INCORRECT);
        venc_->Release();
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_FUNC_0070
 * @tc.name      : 使能VBR_max_bitrate,confige
 * @tc.desc      : api test
 */
HWTEST_F(HwEncInnerCapNdkTest, VIDEO_ENCODE_CAPABILITY_FUNC_0070, TestSize.Level2)
{
    std::shared_ptr<AVCodecList> codecCapability = AVCodecListFactory::CreateAVCodecList();
    CapabilityData *capabilityData = nullptr;
    Format format;
    capabilityData = codecCapability->GetCapability(g_codecMime, true, AVCodecCategory::AVCODEC_HARDWARE);
    std::shared_ptr<VideoCaps> codecInfo = std::make_shared<VideoCaps>(capabilityData);
    if (IsEncoderBitrateModeSupported(capabilityData, VBR)) {
        venc_ = VideoEncoderFactory::CreateByName(g_codecName);
        ASSERT_NE(nullptr, venc_);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, VBR);
        format.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE,
            codecInfo->GetSupportedMaxBitrate().maxVal);
        ASSERT_EQ(venc_->Configure(format), AVCS_ERR_OK);
        venc_->Release();
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_FUNC_0080
 * @tc.name      : 使能VBR_factor,confige
 * @tc.desc      : api test
 */
HWTEST_F(HwEncInnerCapNdkTest, VIDEO_ENCODE_CAPABILITY_FUNC_0080, TestSize.Level2)
{
    std::shared_ptr<AVCodecList> codecCapability = AVCodecListFactory::CreateAVCodecList();
    CapabilityData *capabilityData = nullptr;
    Format format;
    capabilityData = codecCapability->GetCapability(g_codecMime, true, AVCodecCategory::AVCODEC_HARDWARE);
    std::shared_ptr<VideoCaps> codecInfo = std::make_shared<VideoCaps>(capabilityData);
    if (IsEncoderBitrateModeSupported(capabilityData, VBR)) {
        venc_ = VideoEncoderFactory::CreateByName(g_codecName);
        ASSERT_NE(nullptr, venc_);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, VBR);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR,
            codecInfo->GetSupportedSqrFactor().maxVal);
        ASSERT_EQ(venc_->Configure(format), AVCS_ERR_CODEC_PARAM_INCORRECT);
        venc_->Release();
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_FUNC_0090
 * @tc.name      : 使能VBR_max_bitrate_factor,confige
 * @tc.desc      : api test
 */
HWTEST_F(HwEncInnerCapNdkTest, VIDEO_ENCODE_CAPABILITY_FUNC_0090, TestSize.Level2)
{
    std::shared_ptr<AVCodecList> codecCapability = AVCodecListFactory::CreateAVCodecList();
    CapabilityData *capabilityData = nullptr;
    Format format;
    capabilityData = codecCapability->GetCapability(g_codecMime, true, AVCodecCategory::AVCODEC_HARDWARE);
    std::shared_ptr<VideoCaps> codecInfo = std::make_shared<VideoCaps>(capabilityData);
    if (IsEncoderBitrateModeSupported(capabilityData, VBR)) {
        venc_ = VideoEncoderFactory::CreateByName(g_codecName);
        ASSERT_NE(nullptr, venc_);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, VBR);
        format.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE,
            codecInfo->GetSupportedMaxBitrate().maxVal);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR,
            codecInfo->GetSupportedSqrFactor().maxVal);
        ASSERT_EQ(venc_->Configure(format), AVCS_ERR_CODEC_PARAM_INCORRECT);
        venc_->Release();
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_FUNC_0100
 * @tc.name      : 能使SQR,不能使SQR,其他模式参数,confige
 * @tc.desc      : api test
 */
HWTEST_F(HwEncInnerCapNdkTest, VIDEO_ENCODE_CAPABILITY_FUNC_0100, TestSize.Level2)
{
    std::shared_ptr<AVCodecList> codecCapability = AVCodecListFactory::CreateAVCodecList();
    CapabilityData *capabilityData = nullptr;
    Format format;
    Format fmt;
    capabilityData = codecCapability->GetCapability(g_codecMime, true, AVCodecCategory::AVCODEC_HARDWARE);
    std::shared_ptr<VideoCaps> codecInfo = std::make_shared<VideoCaps>(capabilityData);
    if (IsEncoderBitrateModeSupported(capabilityData, SQR)) {
        venc_ = VideoEncoderFactory::CreateByName(g_codecName);
        ASSERT_NE(nullptr, venc_);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
        format.PutIntValue(OH_MD_KEY_QUALITY, 30);
        format.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE,
            codecInfo->GetSupportedMaxBitrate().maxVal);
        ASSERT_EQ(venc_->Configure(format), AVCS_ERR_CODEC_PARAM_INCORRECT);
        venc_->Release();

        venc_ = VideoEncoderFactory::CreateByName(g_codecName);
        ASSERT_NE(nullptr, venc_);
        fmt.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
        fmt.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
        fmt.PutIntValue(OH_MD_KEY_QUALITY, 30);
        fmt.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR,
            codecInfo->GetSupportedSqrFactor().maxVal);
        ASSERT_EQ(venc_->Configure(fmt), AVCS_ERR_CODEC_PARAM_INCORRECT);
        venc_->Release();

        venc_ = VideoEncoderFactory::CreateByName(g_codecName);
        ASSERT_NE(nullptr, venc_);
        format.PutIntValue(OH_MD_KEY_QUALITY, 30);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, SQR);
        format.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE,
            codecInfo->GetSupportedMaxBitrate().maxVal);
        ASSERT_EQ(venc_->Configure(format), AVCS_ERR_CODEC_PARAM_INCORRECT);
        venc_->Release();

        venc_ = VideoEncoderFactory::CreateByName(g_codecName);
        ASSERT_NE(nullptr, venc_);
        fmt.PutIntValue(OH_MD_KEY_QUALITY, 30);
        fmt.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, SQR);
        fmt.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR,
            codecInfo->GetSupportedSqrFactor().maxVal);
        ASSERT_EQ(venc_->Configure(fmt), AVCS_ERR_CODEC_PARAM_INCORRECT);
        venc_->Release();
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_FUNC_0110
 * @tc.name      : 不支持平台，使能SQR，confige
 * @tc.desc      : api test
 */
HWTEST_F(HwEncInnerCapNdkTest, VIDEO_ENCODE_CAPABILITY_FUNC_0110, TestSize.Level2)
{
    std::shared_ptr<AVCodecList> codecCapability = AVCodecListFactory::CreateAVCodecList();
    CapabilityData *capabilityData = nullptr;
    Format format;
    Format format1;
    Format format2;
    capabilityData = codecCapability->GetCapability(g_codecMime, true, AVCodecCategory::AVCODEC_HARDWARE);
    std::shared_ptr<VideoCaps> codecInfo = std::make_shared<VideoCaps>(capabilityData);
    if (!IsEncoderBitrateModeSupported(capabilityData, SQR)) {
        venc_ = VideoEncoderFactory::CreateByName(g_codecName);
        ASSERT_NE(nullptr, venc_);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, SQR);
        format.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE,
            codecInfo->GetSupportedMaxBitrate().maxVal);
        ASSERT_EQ(venc_->Configure(format), AVCS_ERR_CODEC_PARAM_INCORRECT);
        venc_->Release();

        venc_ = VideoEncoderFactory::CreateByName(g_codecName);
        ASSERT_NE(nullptr, venc_);
        format1.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
        format1.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
        format1.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR,
            codecInfo->GetSupportedSqrFactor().maxVal);
        format1.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, SQR);
        ASSERT_EQ(venc_->Configure(format1), AVCS_ERR_CODEC_PARAM_INCORRECT);
        venc_->Release();

        venc_ = VideoEncoderFactory::CreateByName(g_codecName);
        ASSERT_NE(nullptr, venc_);
        format2.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
        format2.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
        format2.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, SQR);
        format2.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR,
            codecInfo->GetSupportedSqrFactor().maxVal);
        format2.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE,
            codecInfo->GetSupportedMaxBitrate().maxVal);
        ASSERT_EQ(venc_->Configure(format2), AVCS_ERR_CODEC_PARAM_INCORRECT);
        venc_->Release();
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_FUNC_0120
 * @tc.name      : 不支持平台，不使能SQR，confige
 * @tc.desc      : api test
 */
HWTEST_F(HwEncInnerCapNdkTest, VIDEO_ENCODE_CAPABILITY_FUNC_0120, TestSize.Level2)
{
    std::shared_ptr<AVCodecList> codecCapability = AVCodecListFactory::CreateAVCodecList();
    CapabilityData *capabilityData = nullptr;
    Format format;
    Format format1;
    Format format2;
    capabilityData = codecCapability->GetCapability(g_codecMime, true, AVCodecCategory::AVCODEC_HARDWARE);
    std::shared_ptr<VideoCaps> codecInfo = std::make_shared<VideoCaps>(capabilityData);
    if (!IsEncoderBitrateModeSupported(capabilityData, SQR)) {
        venc_ = VideoEncoderFactory::CreateByName(g_codecName);
        ASSERT_NE(nullptr, venc_);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
        format.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE,
            codecInfo->GetSupportedMaxBitrate().maxVal);
        ASSERT_EQ(venc_->Configure(format), AVCS_ERR_OK);
        venc_->Release();

        venc_ = VideoEncoderFactory::CreateByName(g_codecName);
        ASSERT_NE(nullptr, venc_);
        format1.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
        format1.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
        format1.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR,
            codecInfo->GetSupportedSqrFactor().maxVal);
        ASSERT_EQ(venc_->Configure(format1), AVCS_ERR_CODEC_PARAM_INCORRECT);
        venc_->Release();

        venc_ = VideoEncoderFactory::CreateByName(g_codecName);
        ASSERT_NE(nullptr, venc_);
        format2.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
        format2.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
        format2.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR,
            codecInfo->GetSupportedSqrFactor().maxVal);
        format2.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE,
            codecInfo->GetSupportedMaxBitrate().maxVal);
        ASSERT_EQ(venc_->Configure(format2), AVCS_ERR_CODEC_PARAM_INCORRECT);
        venc_->Release();
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_FUNC_0130
 * @tc.name      : 不支持平台，使能VBR,config
 * @tc.desc      : api test
 */
HWTEST_F(HwEncInnerCapNdkTest, VIDEO_ENCODE_CAPABILITY_FUNC_0130, TestSize.Level2)
{
    std::shared_ptr<AVCodecList> codecCapability = AVCodecListFactory::CreateAVCodecList();
    CapabilityData *capabilityData = nullptr;
    Format format;
    Format format1;
    Format format2;
    capabilityData = codecCapability->GetCapability(g_codecMime, true, AVCodecCategory::AVCODEC_HARDWARE);
    std::shared_ptr<VideoCaps> codecInfo = std::make_shared<VideoCaps>(capabilityData);
    if (!IsEncoderBitrateModeSupported(capabilityData, VBR)) {
        venc_ = VideoEncoderFactory::CreateByName(g_codecName);
        ASSERT_NE(nullptr, venc_);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, VBR);
        format.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE,
            codecInfo->GetSupportedMaxBitrate().maxVal);
        ASSERT_EQ(venc_->Configure(format), AVCS_ERR_OK);
        venc_->Release();

        venc_ = VideoEncoderFactory::CreateByName(g_codecName);
        ASSERT_NE(nullptr, venc_);
        format1.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
        format1.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
        format1.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, VBR);
        format1.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR,
            codecInfo->GetSupportedSqrFactor().maxVal);
        ASSERT_EQ(venc_->Configure(format1), AVCS_ERR_CODEC_PARAM_INCORRECT);
        venc_->Release();

        venc_ = VideoEncoderFactory::CreateByName(g_codecName);
        ASSERT_NE(nullptr, venc_);
        format2.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
        format2.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
        format2.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, VBR);
        format2.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR,
            codecInfo->GetSupportedSqrFactor().maxVal);
        format2.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE,
            codecInfo->GetSupportedMaxBitrate().maxVal);
        ASSERT_EQ(venc_->Configure(format2), AVCS_ERR_CODEC_PARAM_INCORRECT);
        venc_->Release();
    }
}
/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_FUNC_0140
 * @tc.name      : 不支持平台，配置其他模式参数
 * @tc.desc      : api test
 */
HWTEST_F(HwEncInnerCapNdkTest, VIDEO_ENCODE_CAPABILITY_FUNC_0140, TestSize.Level2)
{
    std::shared_ptr<AVCodecList> codecCapability = AVCodecListFactory::CreateAVCodecList();
    CapabilityData *capabilityData = nullptr;
    Format format;
    Format format1;
    capabilityData = codecCapability->GetCapability(g_codecMime, true, AVCodecCategory::AVCODEC_HARDWARE);
    std::shared_ptr<VideoCaps> codecInfo = std::make_shared<VideoCaps>(capabilityData);
    if (!IsEncoderBitrateModeSupported(capabilityData, SQR)) {
        venc_ = VideoEncoderFactory::CreateByName(g_codecName);
        ASSERT_NE(nullptr, venc_);
        format.PutIntValue(OH_MD_KEY_QUALITY, 30);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
        format.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE,
            codecInfo->GetSupportedMaxBitrate().maxVal);
        ASSERT_EQ(venc_->Configure(format), AVCS_ERR_CODEC_PARAM_INCORRECT);
        venc_->Release();

        venc_ = VideoEncoderFactory::CreateByName(g_codecName);
        ASSERT_NE(nullptr, venc_);
        format1.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
        format1.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
        format1.PutIntValue(OH_MD_KEY_QUALITY, 30);
        format1.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR,
            codecInfo->GetSupportedSqrFactor().maxVal);
        ASSERT_EQ(venc_->Configure(format1), AVCS_ERR_CODEC_PARAM_INCORRECT);
        venc_->Release();

        venc_ = VideoEncoderFactory::CreateByName(g_codecName);
        ASSERT_NE(nullptr, venc_);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, SQR);
        format.PutIntValue(OH_MD_KEY_QUALITY, 30);
        format.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE,
            codecInfo->GetSupportedMaxBitrate().maxVal);
        ASSERT_EQ(venc_->Configure(format), AVCS_ERR_CODEC_PARAM_INCORRECT);
        venc_->Release();

        venc_ = VideoEncoderFactory::CreateByName(g_codecName);
        ASSERT_NE(nullptr, venc_);
        format1.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, SQR);
        format1.PutIntValue(OH_MD_KEY_QUALITY, 30);
        format1.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR,
            codecInfo->GetSupportedSqrFactor().maxVal);
        ASSERT_EQ(venc_->Configure(format1), AVCS_ERR_CODEC_PARAM_INCORRECT);
        venc_->Release();
    }
}
} // namespace