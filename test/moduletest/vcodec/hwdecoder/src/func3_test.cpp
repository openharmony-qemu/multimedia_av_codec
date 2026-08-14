/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
#ifndef ONLY_FOR_FLAGSHIP_CHIP
#define ONLY_FOR_FLAGSHIP_CHIP 0
#endif

#include <string>
#include "gtest/gtest.h"
#include "native_avcodec_videodecoder.h"
#include "native_averrors.h"
#include "videodec_sample.h"
#include "videodec_api11_sample.h"
#include "native_avcodec_base.h"
#include "avcodec_codec_name.h"
#include "native_avcapability.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;

namespace OHOS {
namespace Media {
class HwdecFunc3NdkTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp() override;
    void TearDown() override;
    void InputFunc();
    void OutputFunc();
    void Release();
    int32_t Stop();

protected:
    const char *INP_DIR_1080_30 = "/data/test/media/1920_1080_10_30Mb.h264";
    const char *INP_DIR_1080_20 = "/data/test/media/1920_1080_20M_30.h265";
    const char *INP_DIR_VVC_1080 = "/data/test/media/1920_1080_10bit.vvc";
    const char *INP_DIR_8192_4320_8BIT_264 = "/data/test/media/8192_4320_50M_8bit.h264";
    const char *INP_DIR_7680_4320_8BIT_264 = "/data/test/media/7680_4320_20M_8bit.h264";
    const char *INP_DIR_4320_8192_8BIT_264 = "/data/test/media/4320_8192_50M_8bit.h264";
    const char *INP_DIR_4320_7680_8BIT_264 = "/data/test/media/4320_7680_20M_8bit.h264";
    const char *INP_DIR_8192_4352_8BIT_264 = "/data/test/media/8192_4352_50M_8bit.h264";
    const char *INP_DIR_8192_4992_8BIT_265 = "/data/test/media/8192_4992_50M_8bit.h265";
    const char *INP_DIR_7680_4320_8BIT_265 = "/data/test/media/7680_4320_20M_8bit.h265";
    const char *INP_DIR_8192_4992_10BIT_265 = "/data/test/media/8192_4992_50M_10bit.h265";
    const char *INP_DIR_7680_4320_10BIT_HDR_265 = "/data/test/media/7680_4320_20M_10bit_hdr.h265";
    const char *INP_DIR_4992_8192_8BIT_265 = "/data/test/media/4992_8192_50M_8bit.h265";
    const char *INP_DIR_4320_7680_8BIT_265 = "/data/test/media/4320_7680_20M_8bit.h265";
    const char *INP_DIR_4992_8192_10BIT_HDR_265 = "/data/test/media/4992_8192_50M_10bit_hdr.h265";
    const char *INP_DIR_4320_7680_10BIT_HDR_265 = "/data/test/media/4320_7680_20M_10bit_hdr.h265";
    const char *INP_DIR_8192_8192_10BIT_265 = "/data/test/media/8192_8192_50M_10bit.h265";
    const char *INP_DIR_RES_8K_264 = "/data/test/media/resChange8k.h264";
    const char *INP_DIR_RES_8K_265 = "/data/test/media/resChange8k.h265";
};
} // namespace Media
} // namespace OHOS

namespace {
static OH_AVCapability *cap = nullptr;
static OH_AVCapability *cap_hevc = nullptr;
static OH_AVCapability *cap_vvc = nullptr;
static string g_codecName = "";
static string g_codecNameHEVC = "";
static string g_codecNameVVC = "";
const std::vector<OH_NativeBuffer_TransformType> transfromTypes = {
    NATIVEBUFFER_ROTATE_NONE,
    NATIVEBUFFER_ROTATE_90,
    NATIVEBUFFER_ROTATE_180,
    NATIVEBUFFER_ROTATE_270,
    NATIVEBUFFER_FLIP_H,
    NATIVEBUFFER_FLIP_V,
    NATIVEBUFFER_FLIP_H_ROT90,
    NATIVEBUFFER_FLIP_V_ROT90,
    NATIVEBUFFER_FLIP_H_ROT180,
    NATIVEBUFFER_FLIP_V_ROT180,
    NATIVEBUFFER_FLIP_H_ROT270,
    NATIVEBUFFER_FLIP_V_ROT270
};
} // namespace

void HwdecFunc3NdkTest::SetUpTestCase()
{
    cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_AVC, false, HARDWARE);
    g_codecName = OH_AVCapability_GetName(cap);
    cout << "codecname: " << g_codecName << endl;
    cap_hevc = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_HEVC, false, HARDWARE);
    g_codecNameHEVC = OH_AVCapability_GetName(cap_hevc);
    cout << "g_codecNameHEVC: " << g_codecNameHEVC << endl;
    cap_vvc = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_VVC, false, HARDWARE);
    g_codecNameVVC = OH_AVCapability_GetName(cap_vvc);
    cout << "g_codecNameVVC: " << g_codecNameVVC << endl;
}
void HwdecFunc3NdkTest::TearDownTestCase() {}
void HwdecFunc3NdkTest::SetUp() {}
void HwdecFunc3NdkTest::TearDown() {}

int32_t GetVideoMaxWidthForHeight(OH_AVCapability *capability, uint32_t height)
{
    OH_AVRange widthRange;
    memset_s(&widthRange, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    if (OH_AVCapability_GetVideoWidthRangeForHeight(capability, height, &widthRange) != AV_ERR_OK) {
        return -1;
    }
    cout << "max width: " << widthRange.maxVal << endl;
    return widthRange.maxVal;
}

int32_t GetVideoMaxHeightForWidth(OH_AVCapability *capability, uint32_t width)
{
    OH_AVRange heightRange;
    memset_s(&heightRange, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    if (OH_AVCapability_GetVideoHeightRangeForWidth(capability, width, &heightRange) != AV_ERR_OK) {
        return -1;
    }
    cout << "max height: " << heightRange.maxVal << endl;
    return heightRange.maxVal;
}

namespace {
/**
 * @tc.number    : VIDEO_DECODE_TRANSFORM_0010
 * @tc.name      : 硬解H264, config transform
 * @tc.desc      : function test
 */
HWTEST_F(HwdecFunc3NdkTest, VIDEO_DECODE_TRANSFORM_0010, TestSize.Level0)
{
    if (cap != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_1080_30;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecName));
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        vDecSample->SF_OUTPUT = true;
        vDecSample->CreateSurface();
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetConfigTransform());
        ASSERT_EQ(0, vDecSample->GetSurfaceTransform(0));
        ASSERT_EQ(AV_ERR_OK, vDecSample->Reset());
        vDecSample->setTransform = true;
        vDecSample->DEFAULT_TRANSFORM = -1;
        ASSERT_EQ(AV_ERR_INVALID_VAL, vDecSample->SetConfigTransform());
        ASSERT_EQ(0, vDecSample->GetSurfaceTransform(0));
        ASSERT_EQ(AV_ERR_OK, vDecSample->Reset());
        for (const auto& transformtype : transfromTypes) {
            vDecSample->DEFAULT_TRANSFORM = transformtype;
            ASSERT_EQ(AV_ERR_OK, vDecSample->SetConfigTransform());
            ASSERT_EQ(vDecSample->DEFAULT_TRANSFORM, vDecSample->GetSurfaceTransform(0));
            ASSERT_EQ(AV_ERR_OK, vDecSample->Reset());
        }
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetSurface());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        ASSERT_EQ(vDecSample->DEFAULT_TRANSFORM, vDecSample->GetSurfaceTransform(0));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_TRANSFORM_0020
 * @tc.name      : 硬解H265, config transform
 * @tc.desc      : function test
 */
HWTEST_F(HwdecFunc3NdkTest, VIDEO_DECODE_TRANSFORM_0020, TestSize.Level0)
{
    if (cap_hevc != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_1080_20;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameHEVC));
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        vDecSample->SF_OUTPUT = true;
        vDecSample->CreateSurface();
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetConfigTransform());
        ASSERT_EQ(0, vDecSample->GetSurfaceTransform(0));
        ASSERT_EQ(AV_ERR_OK, vDecSample->Reset());
        vDecSample->setTransform = true;
        vDecSample->DEFAULT_TRANSFORM = -1;
        ASSERT_EQ(AV_ERR_INVALID_VAL, vDecSample->SetConfigTransform());
        ASSERT_EQ(0, vDecSample->GetSurfaceTransform(0));
        ASSERT_EQ(AV_ERR_OK, vDecSample->Reset());
        for (const auto& transformtype : transfromTypes) {
            vDecSample->DEFAULT_TRANSFORM = transformtype;
            ASSERT_EQ(AV_ERR_OK, vDecSample->SetConfigTransform());
            ASSERT_EQ(vDecSample->DEFAULT_TRANSFORM, vDecSample->GetSurfaceTransform(0));
            ASSERT_EQ(AV_ERR_OK, vDecSample->Reset());
        }
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetSurface());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        ASSERT_EQ(vDecSample->DEFAULT_TRANSFORM, vDecSample->GetSurfaceTransform(0));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_TRANSFORM_0030
 * @tc.name      : 硬解H266, config transform
 * @tc.desc      : function test
 */
HWTEST_F(HwdecFunc3NdkTest, VIDEO_DECODE_TRANSFORM_0030, TestSize.Level0)
{
    if (g_codecNameVVC.find("hisi") != string::npos) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_VVC_1080;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->is8bitYuv = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameVVC));
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        vDecSample->SF_OUTPUT = true;
        vDecSample->CreateSurface();
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetConfigTransform());
        ASSERT_EQ(0, vDecSample->GetSurfaceTransform(0));
        ASSERT_EQ(AV_ERR_OK, vDecSample->Reset());
        vDecSample->setTransform = true;
        vDecSample->DEFAULT_TRANSFORM = -1;
        ASSERT_EQ(AV_ERR_INVALID_VAL, vDecSample->SetConfigTransform());
        ASSERT_EQ(0, vDecSample->GetSurfaceTransform(0));
        ASSERT_EQ(AV_ERR_OK, vDecSample->Reset());
        for (const auto& transformtype : transfromTypes) {
            vDecSample->DEFAULT_TRANSFORM = transformtype;
            ASSERT_EQ(AV_ERR_OK, vDecSample->SetConfigTransform());
            ASSERT_EQ(vDecSample->DEFAULT_TRANSFORM, vDecSample->GetSurfaceTransform(0));
            ASSERT_EQ(AV_ERR_OK, vDecSample->Reset());
        }
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetSurface());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        ASSERT_EQ(vDecSample->DEFAULT_TRANSFORM, vDecSample->GetSurfaceTransform(0));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_TRANSFORM_0090
 * @tc.name      : 硬解H264, setparameter transform
 * @tc.desc      : function test
 */
HWTEST_F(HwdecFunc3NdkTest, VIDEO_DECODE_TRANSFORM_0090, TestSize.Level0)
{
    if (cap != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_1080_30;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecName));
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        vDecSample->SF_OUTPUT = true;
        vDecSample->CreateSurface();
        vDecSample->DEFAULT_TRANSFORM = -1;
        ASSERT_EQ(AV_ERR_INVALID_VAL, vDecSample->SetParameterTransform());
        ASSERT_EQ(0, vDecSample->GetSurfaceTransform(0));
        ASSERT_EQ(AV_ERR_OK, vDecSample->Reset());
        for (const auto& transformtype : transfromTypes) {
            vDecSample->DEFAULT_TRANSFORM = transformtype;
            ASSERT_EQ(AV_ERR_OK, vDecSample->SetParameterTransform());
            ASSERT_EQ(vDecSample->DEFAULT_TRANSFORM, vDecSample->GetSurfaceTransform(0));
            ASSERT_EQ(AV_ERR_OK, vDecSample->Reset());
        }
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetSurface());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetParameter());
        ASSERT_EQ(vDecSample->DEFAULT_TRANSFORM, vDecSample->GetSurfaceTransform(0));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_TRANSFORM_0100
 * @tc.name      : 硬解H264, config and setparameter transform
 * @tc.desc      : function test
 */
HWTEST_F(HwdecFunc3NdkTest, VIDEO_DECODE_TRANSFORM_0100, TestSize.Level0)
{
    if (cap != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_1080_30;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecName));
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        vDecSample->SF_OUTPUT = true;
        vDecSample->CreateSurface();
        vDecSample->setTransform = true;
        vDecSample->DEFAULT_TRANSFORM = NATIVEBUFFER_ROTATE_90;
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetConfigTransform());
        vDecSample->DEFAULT_TRANSFORM = -1;
        ASSERT_EQ(AV_ERR_INVALID_VAL, vDecSample->SetParameter());
        ASSERT_EQ(NATIVEBUFFER_ROTATE_90, vDecSample->GetSurfaceTransform(0));
        ASSERT_EQ(AV_ERR_OK, vDecSample->Reset());
        for (const auto& transformtype : transfromTypes) {
            vDecSample->DEFAULT_TRANSFORM = NATIVEBUFFER_ROTATE_90;
            ASSERT_EQ(AV_ERR_OK, vDecSample->SetConfigTransform());
            vDecSample->DEFAULT_TRANSFORM = transformtype;
            ASSERT_EQ(AV_ERR_OK, vDecSample->SetParameter());
            ASSERT_EQ(transformtype, vDecSample->GetSurfaceTransform(0));
            ASSERT_EQ(AV_ERR_OK, vDecSample->Reset());
        }
        vDecSample->DEFAULT_TRANSFORM = NATIVEBUFFER_ROTATE_90;
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetSurface());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->DEFAULT_TRANSFORM = NATIVEBUFFER_FLIP_V_ROT270;
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetParameter());
        ASSERT_EQ(vDecSample->DEFAULT_TRANSFORM, vDecSample->GetSurfaceTransform(0));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_TRANSFORM_0110
 * @tc.name      : 硬解H265, setparameter transform
 * @tc.desc      : function test
 */
HWTEST_F(HwdecFunc3NdkTest, VIDEO_DECODE_TRANSFORM_0110, TestSize.Level0)
{
    if (cap_hevc != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_1080_20;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameHEVC));
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        vDecSample->SF_OUTPUT = true;
        vDecSample->CreateSurface();
        vDecSample->DEFAULT_TRANSFORM = -1;
        ASSERT_EQ(AV_ERR_INVALID_VAL, vDecSample->SetParameterTransform());
        ASSERT_EQ(0, vDecSample->GetSurfaceTransform(0));
        ASSERT_EQ(AV_ERR_OK, vDecSample->Reset());
        for (const auto& transformtype : transfromTypes) {
            vDecSample->DEFAULT_TRANSFORM = transformtype;
            ASSERT_EQ(AV_ERR_OK, vDecSample->SetParameterTransform());
            ASSERT_EQ(transformtype, vDecSample->GetSurfaceTransform(0));
            ASSERT_EQ(AV_ERR_OK, vDecSample->Reset());
        }
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetSurface());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetParameter());
        ASSERT_EQ(vDecSample->DEFAULT_TRANSFORM, vDecSample->GetSurfaceTransform(0));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_TRANSFORM_0120
 * @tc.name      : 硬解H266, setparameter transform
 * @tc.desc      : function test
 */
HWTEST_F(HwdecFunc3NdkTest, VIDEO_DECODE_TRANSFORM_0120, TestSize.Level0)
{
    if (g_codecNameVVC.find("hisi") != string::npos) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_VVC_1080;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->is8bitYuv = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameVVC));
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        vDecSample->SF_OUTPUT = true;
        vDecSample->CreateSurface();
        vDecSample->DEFAULT_TRANSFORM = -1;
        ASSERT_EQ(AV_ERR_INVALID_VAL, vDecSample->SetParameterTransform());
        ASSERT_EQ(0, vDecSample->GetSurfaceTransform(0));
        ASSERT_EQ(AV_ERR_OK, vDecSample->Reset());
        for (const auto& transformtype : transfromTypes) {
            vDecSample->DEFAULT_TRANSFORM = transformtype;
            ASSERT_EQ(AV_ERR_OK, vDecSample->SetParameterTransform());
            ASSERT_EQ(transformtype, vDecSample->GetSurfaceTransform(0));
            ASSERT_EQ(AV_ERR_OK, vDecSample->Reset());
        }
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetSurface());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetParameter());
        ASSERT_EQ(vDecSample->DEFAULT_TRANSFORM, vDecSample->GetSurfaceTransform(0));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_TRANSFORM_0180
 * @tc.name      : 硬解H264, surface chnange, transform follow
 * @tc.desc      : function test
 */
HWTEST_F(HwdecFunc3NdkTest, VIDEO_DECODE_TRANSFORM_0180, TestSize.Level0)
{
    if (cap != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_1080_30;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = true;
        vDecSample->needAutoSwitch = false;
        vDecSample->autoSwitchSurface = true;
        vDecSample->setTransform = true;
        vDecSample->DEFAULT_TRANSFORM = NATIVEBUFFER_FLIP_H_ROT90;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecName));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
        ASSERT_EQ(vDecSample->beforeSwitchTransform, vDecSample->afterSwitchTransform);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_TRANSFORM_0190
 * @tc.name      : 硬解H265, surface chnange, transform follow
 * @tc.desc      : function test
 */
HWTEST_F(HwdecFunc3NdkTest, VIDEO_DECODE_TRANSFORM_0190, TestSize.Level0)
{
    if (cap_hevc != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_1080_20;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = true;
        vDecSample->needAutoSwitch = false;
        vDecSample->autoSwitchSurface = true;
        vDecSample->setTransform = true;
        vDecSample->DEFAULT_TRANSFORM = NATIVEBUFFER_FLIP_H_ROT90;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameHEVC));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
        ASSERT_EQ(vDecSample->beforeSwitchTransform, vDecSample->afterSwitchTransform);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_TRANSFORM_0200
 * @tc.name      : 硬解H266, surface chnange, transform follow
 * @tc.desc      : function test
 */
HWTEST_F(HwdecFunc3NdkTest, VIDEO_DECODE_TRANSFORM_0200, TestSize.Level0)
{
    if (g_codecNameVVC.find("hisi") != string::npos) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_VVC_1080;
        vDecSample->is8bitYuv = false;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = true;
        vDecSample->needAutoSwitch = false;
        vDecSample->autoSwitchSurface = true;
        vDecSample->setTransform = true;
        vDecSample->DEFAULT_TRANSFORM = NATIVEBUFFER_FLIP_H_ROT90;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameVVC));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
        ASSERT_EQ(vDecSample->beforeSwitchTransform, vDecSample->afterSwitchTransform);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_TRANSFORM_0230
 * @tc.name      : 硬解H264, config transform,release decode
 * @tc.desc      : function test
 */
HWTEST_F(HwdecFunc3NdkTest, VIDEO_DECODE_TRANSFORM_0230, TestSize.Level0)
{
    if (cap != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_1080_30;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = true;
        vDecSample->setTransform = true;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        vDecSample->DEFAULT_TRANSFORM = NATIVEBUFFER_FLIP_H_ROT270;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecName));
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        vDecSample->CreateSurface();
        int32_t beforeReleaseTroform = vDecSample->GetSurfaceTransform(0);
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetSurface());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        ASSERT_EQ(NATIVEBUFFER_FLIP_H_ROT270, vDecSample->GetSurfaceTransform(0));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
        ASSERT_EQ(AV_ERR_OK, vDecSample->Release());
        int32_t afterReleaseTroform = vDecSample->GetSurfaceTransform(0);
        ASSERT_EQ(beforeReleaseTroform, afterReleaseTroform);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_TRANSFORM_0240
 * @tc.name      : 硬解H265, config transform,reset decode
 * @tc.desc      : function test
 */
HWTEST_F(HwdecFunc3NdkTest, VIDEO_DECODE_TRANSFORM_0240, TestSize.Level0)
{
    if (cap_hevc != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_1080_20;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = true;
        vDecSample->setTransform = true;
        vDecSample->DEFAULT_TRANSFORM = NATIVEBUFFER_FLIP_H_ROT270;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameHEVC));
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        vDecSample->CreateSurface();
        int32_t beforeReleaseTroform = vDecSample->GetSurfaceTransform(0);
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetSurface());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        ASSERT_EQ(NATIVEBUFFER_FLIP_H_ROT270, vDecSample->GetSurfaceTransform(0));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
        ASSERT_EQ(AV_ERR_OK, vDecSample->Reset());
        int32_t afterReleaseTroform = vDecSample->GetSurfaceTransform(0);
        ASSERT_EQ(beforeReleaseTroform, afterReleaseTroform);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_ANOTHER_0010
 * @tc.name      : 解码器1正常解码，解码器2状态轮转
 * @tc.desc      : function test
 */
HWTEST_F(HwdecFunc3NdkTest, VIDEO_DECODE_ANOTHER_0010, TestSize.Level2)
{
    if (cap_hevc != nullptr) {
        auto vDecSample = make_shared<VDecNdkSample>();
        vDecSample->INP_DIR = INP_DIR_1080_20;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameHEVC));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        auto vDecSample2 = make_shared<VDecAPI11Sample>();
        ASSERT_EQ(AV_ERR_OK, vDecSample2->CreateVideoDecoder(g_codecNameHEVC));
        ASSERT_EQ(AV_ERR_OK, vDecSample2->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample2->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample2->Start());
        ASSERT_EQ(AV_ERR_OK, vDecSample2->Flush());
        ASSERT_EQ(AV_ERR_OK, vDecSample2->Start());
        ASSERT_EQ(AV_ERR_OK, vDecSample2->Stop());
        ASSERT_EQ(AV_ERR_OK, vDecSample2->Start());
        ASSERT_EQ(AV_ERR_OK, vDecSample2->Reset());
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_ANOTHER_0020
 * @tc.name      : 解码器1正常解码，解码器2状态机轮转, 共用surface
 * @tc.desc      : function test
 */
HWTEST_F(HwdecFunc3NdkTest, VIDEO_DECODE_ANOTHER_0020, TestSize.Level2)
{
    if (cap_hevc != nullptr) {
        auto vDecSample = make_shared<VDecNdkSample>();
        auto vDecSample2 = make_shared<VDecAPI11Sample>();
        vDecSample2->INP_DIR = INP_DIR_1080_20;
        vDecSample2->DEFAULT_WIDTH = 1920;
        vDecSample2->DEFAULT_HEIGHT = 1080;
        vDecSample2->SF_OUTPUT = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameHEVC));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->DecodeSetSurface());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->Start());
        
        ASSERT_EQ(AV_ERR_OK, vDecSample2->CreateVideoDecoder(g_codecNameHEVC));
        ASSERT_EQ(AV_ERR_OK, vDecSample2->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample2->SetAnotherSurface(vDecSample->nativeWindow[0]));
        ASSERT_EQ(AV_ERR_OK, vDecSample2->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample2->StartVideoDecoder());

        ASSERT_EQ(AV_ERR_OK, vDecSample->Stop());
        ASSERT_EQ(AV_ERR_OK, vDecSample->Start());
        ASSERT_EQ(AV_ERR_OK, vDecSample->Flush());
        ASSERT_EQ(AV_ERR_OK, vDecSample->Start());
        ASSERT_EQ(AV_ERR_OK, vDecSample->Reset());
        ASSERT_EQ(AV_ERR_OK, vDecSample->Release());
        vDecSample2->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample2->errCount);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_8K_0010
 * @tc.name      : h264解码8k，同步buffer，8bit
 * @tc.desc      : function test
 */
HWTEST_F(HwdecFunc3NdkTest, VIDEO_DECODE_8K_0010, TestSize.Level0)
{
    if (cap != nullptr) {
        int32_t maxWidth = GetVideoMaxWidthForHeight(cap, 4320);
        int32_t maxHeight = GetVideoMaxHeightForWidth(cap, 8192);
        if (maxWidth >= 8192 && maxHeight >= 4320) {
            auto vDecSample = make_shared<VDecAPI11Sample>();
            vDecSample->INP_DIR = INP_DIR_8192_4320_8BIT_264;
            vDecSample->DEFAULT_WIDTH = 8192;
            vDecSample->DEFAULT_HEIGHT = 4320;
            vDecSample->DEFAULT_FRAME_RATE = 30;
            vDecSample->SF_OUTPUT = false;
            vDecSample->outputYuvFlag = false;
            vDecSample->enbleSyncMode = 1;
            vDecSample->NocaleHash = false;
            ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecName));
            ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
            ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
            vDecSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
            ASSERT_EQ(5, vDecSample->outFrameCount);
        }
    }
}

/**
 * @tc.number    : VIDEO_DECODE_8K_0020
 * @tc.name      : h264解码8k，同步surface，8bit
 * @tc.desc      : function test
 */
HWTEST_F(HwdecFunc3NdkTest, VIDEO_DECODE_8K_0020, TestSize.Level1)
{
    if (cap != nullptr) {
        int32_t maxWidth = GetVideoMaxWidthForHeight(cap, 4320);
        int32_t maxHeight = GetVideoMaxHeightForWidth(cap, 8192);
        if (maxWidth >= 8192 && maxHeight >= 4320) {
            auto vDecSample = make_shared<VDecAPI11Sample>();
            vDecSample->INP_DIR = INP_DIR_8192_4320_8BIT_264;
            vDecSample->DEFAULT_WIDTH = 8192;
            vDecSample->DEFAULT_HEIGHT = 4320;
            vDecSample->DEFAULT_FRAME_RATE = 30;
            vDecSample->SF_OUTPUT = true;
            vDecSample->outputYuvFlag = false;
            vDecSample->enbleSyncMode = 1;
            ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecName));
            ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
            ASSERT_EQ(AV_ERR_OK, vDecSample->DecodeSetSurface());
            ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
            vDecSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
            ASSERT_EQ(5, vDecSample->outFrameCount);
        }
    }
}

/**
 * @tc.number    : VIDEO_DECODE_8K_0030
 * @tc.name      : h264解码8k，异步buffer，8bit
 * @tc.desc      : function test
 */
HWTEST_F(HwdecFunc3NdkTest, VIDEO_DECODE_8K_0030, TestSize.Level0)
{
    if (cap != nullptr) {
        int32_t maxWidth = GetVideoMaxWidthForHeight(cap, 4320);
        int32_t maxHeight = GetVideoMaxHeightForWidth(cap, 8192);
        if (maxWidth >= 8192 && maxHeight >= 4320) {
            auto vDecSample = make_shared<VDecAPI11Sample>();
            vDecSample->INP_DIR = INP_DIR_8192_4320_8BIT_264;
            vDecSample->DEFAULT_WIDTH = 8192;
            vDecSample->DEFAULT_HEIGHT = 4320;
            vDecSample->DEFAULT_FRAME_RATE = 30;
            vDecSample->SF_OUTPUT = false;
            vDecSample->outputYuvFlag = false;
            vDecSample->NocaleHash = false;
            ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecName));
            ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
            ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
            ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
            vDecSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
            ASSERT_EQ(5, vDecSample->outFrameCount);
        }
    }
}

/**
 * @tc.number    : VIDEO_DECODE_8K_0040
 * @tc.name      : h264解码8k，异步surface，8bit
 * @tc.desc      : function test
 */
HWTEST_F(HwdecFunc3NdkTest, VIDEO_DECODE_8K_0040, TestSize.Level1)
{
    if (cap != nullptr) {
        int32_t maxWidth = GetVideoMaxWidthForHeight(cap, 4320);
        int32_t maxHeight = GetVideoMaxHeightForWidth(cap, 8192);
        if (maxWidth >= 8192 && maxHeight >= 4320) {
            auto vDecSample = make_shared<VDecAPI11Sample>();
            vDecSample->INP_DIR = INP_DIR_8192_4320_8BIT_264;
            vDecSample->DEFAULT_WIDTH = 8192;
            vDecSample->DEFAULT_HEIGHT = 4320;
            vDecSample->DEFAULT_FRAME_RATE = 30;
            vDecSample->SF_OUTPUT = true;
            vDecSample->outputYuvFlag = false;
            ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecName));
            ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
            ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
            ASSERT_EQ(AV_ERR_OK, vDecSample->DecodeSetSurface());
            ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
            vDecSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
            ASSERT_EQ(5, vDecSample->outFrameCount);
        }
    }
}

/**
 * @tc.number    : VIDEO_DECODE_8K_0050
 * @tc.name      : h264解码8k，同步buffer，8bit
 * @tc.desc      : function test
 */
HWTEST_F(HwdecFunc3NdkTest, VIDEO_DECODE_8K_0050, TestSize.Level2)
{
    if (cap != nullptr) {
        int32_t maxWidth = GetVideoMaxWidthForHeight(cap, 4320);
        int32_t maxHeight = GetVideoMaxHeightForWidth(cap, 7680);
        if (maxWidth >= 7680 && maxHeight >= 4320) {
            auto vDecSample = make_shared<VDecAPI11Sample>();
            vDecSample->INP_DIR = INP_DIR_7680_4320_8BIT_264;
            vDecSample->DEFAULT_WIDTH = 7680;
            vDecSample->DEFAULT_HEIGHT = 4320;
            vDecSample->DEFAULT_FRAME_RATE = 30;
            vDecSample->SF_OUTPUT = false;
            vDecSample->outputYuvFlag = false;
            vDecSample->enbleSyncMode = 1;
            vDecSample->NocaleHash = false;
            ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecName));
            ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
            ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
            vDecSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
            ASSERT_EQ(5, vDecSample->outFrameCount);
        }
    }
}

/**
 * @tc.number    : VIDEO_DECODE_8K_0060
 * @tc.name      : h264解码8k，异步surface，8bit
 * @tc.desc      : function test
 */
HWTEST_F(HwdecFunc3NdkTest, VIDEO_DECODE_8K_0060, TestSize.Level2)
{
    if (cap != nullptr) {
        int32_t maxWidth = GetVideoMaxWidthForHeight(cap, 4320);
        int32_t maxHeight = GetVideoMaxHeightForWidth(cap, 7680);
        if (maxWidth >= 7680 && maxHeight >= 4320) {
            auto vDecSample = make_shared<VDecAPI11Sample>();
            vDecSample->INP_DIR = INP_DIR_7680_4320_8BIT_264;
            vDecSample->DEFAULT_WIDTH = 7680;
            vDecSample->DEFAULT_HEIGHT = 4320;
            vDecSample->DEFAULT_FRAME_RATE = 30;
            vDecSample->SF_OUTPUT = true;
            vDecSample->outputYuvFlag = false;
            ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecName));
            ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
            ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
            ASSERT_EQ(AV_ERR_OK, vDecSample->DecodeSetSurface());
            ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
            vDecSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
            ASSERT_EQ(5, vDecSample->outFrameCount);
        }
    }
}

/**
 * @tc.number    : VIDEO_DECODE_8K_0070
 * @tc.name      : h264解码8k，同步surface，8bit
 * @tc.desc      : function test
 */
HWTEST_F(HwdecFunc3NdkTest, VIDEO_DECODE_8K_0070, TestSize.Level2)
{
    if (cap != nullptr) {
        int32_t maxWidth = GetVideoMaxWidthForHeight(cap, 8192);
        int32_t maxHeight = GetVideoMaxHeightForWidth(cap, 4320);
        if (maxWidth >= 4320 && maxHeight >= 8192) {
            auto vDecSample = make_shared<VDecAPI11Sample>();
            vDecSample->INP_DIR = INP_DIR_4320_8192_8BIT_264;
            vDecSample->DEFAULT_WIDTH = 4320;
            vDecSample->DEFAULT_HEIGHT = 8192;
            vDecSample->DEFAULT_FRAME_RATE = 30;
            vDecSample->SF_OUTPUT = true;
            vDecSample->outputYuvFlag = false;
            vDecSample->enbleSyncMode = 1;
            ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecName));
            ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
            ASSERT_EQ(AV_ERR_OK, vDecSample->DecodeSetSurface());
            ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
            vDecSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
            ASSERT_EQ(5, vDecSample->outFrameCount);
        }
    }
}

/**
 * @tc.number    : VIDEO_DECODE_8K_0080
 * @tc.name      : h264解码8k，异步buffer，8bit
 * @tc.desc      : function test
 */
HWTEST_F(HwdecFunc3NdkTest, VIDEO_DECODE_8K_0080, TestSize.Level2)
{
    if (cap != nullptr) {
        int32_t maxWidth = GetVideoMaxWidthForHeight(cap, 8192);
        int32_t maxHeight = GetVideoMaxHeightForWidth(cap, 4320);
        if (maxWidth >= 4320 && maxHeight >= 8192) {
            auto vDecSample = make_shared<VDecAPI11Sample>();
            vDecSample->INP_DIR = INP_DIR_4320_8192_8BIT_264;
            vDecSample->DEFAULT_WIDTH = 4320;
            vDecSample->DEFAULT_HEIGHT = 8192;
            vDecSample->DEFAULT_FRAME_RATE = 30;
            vDecSample->SF_OUTPUT = false;
            vDecSample->outputYuvFlag = false;
            vDecSample->NocaleHash = false;
            ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecName));
            ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
            ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
            ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
            vDecSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
            ASSERT_EQ(5, vDecSample->outFrameCount);
        }
    }
}

/**
 * @tc.number    : VIDEO_DECODE_8K_0090
 * @tc.name      : h264解码8k，同步buffer，8bit
 * @tc.desc      : function test
 */
HWTEST_F(HwdecFunc3NdkTest, VIDEO_DECODE_8K_0090, TestSize.Level2)
{
    if (cap != nullptr) {
        int32_t maxWidth = GetVideoMaxWidthForHeight(cap, 7680);
        int32_t maxHeight = GetVideoMaxHeightForWidth(cap, 4320);
        if (maxWidth >= 4320 && maxHeight >= 7680) {
            auto vDecSample = make_shared<VDecAPI11Sample>();
            vDecSample->INP_DIR = INP_DIR_4320_7680_8BIT_264;
            vDecSample->DEFAULT_WIDTH = 4320;
            vDecSample->DEFAULT_HEIGHT = 7680;
            vDecSample->DEFAULT_FRAME_RATE = 30;
            vDecSample->SF_OUTPUT = false;
            vDecSample->outputYuvFlag = false;
            vDecSample->enbleSyncMode = 1;
            vDecSample->NocaleHash = false;
            ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecName));
            ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
            ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
            vDecSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
            ASSERT_EQ(5, vDecSample->outFrameCount);
        }
    }
}

/**
 * @tc.number    : VIDEO_DECODE_8K_0100
 * @tc.name      : h264解码8k，异步surface，8bit
 * @tc.desc      : function test
 */
HWTEST_F(HwdecFunc3NdkTest, VIDEO_DECODE_8K_0100, TestSize.Level2)
{
    if (cap != nullptr) {
        int32_t maxWidth = GetVideoMaxWidthForHeight(cap, 7680);
        int32_t maxHeight = GetVideoMaxHeightForWidth(cap, 4320);
        if (maxWidth >= 4320 && maxHeight >= 7680) {
            auto vDecSample = make_shared<VDecAPI11Sample>();
            vDecSample->INP_DIR = INP_DIR_4320_7680_8BIT_264;
            vDecSample->DEFAULT_WIDTH = 4320;
            vDecSample->DEFAULT_HEIGHT = 7680;
            vDecSample->DEFAULT_FRAME_RATE = 30;
            vDecSample->SF_OUTPUT = true;
            vDecSample->outputYuvFlag = false;
            ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecName));
            ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
            ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
            ASSERT_EQ(AV_ERR_OK, vDecSample->DecodeSetSurface());
            ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
            vDecSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
            ASSERT_EQ(5, vDecSample->outFrameCount);
        }
    }
}

/**
 * @tc.number    : VIDEO_DECODE_8K_0110
 * @tc.name      : h264解码8k，同步surface，8bit
 * @tc.desc      : function test
 */
HWTEST_F(HwdecFunc3NdkTest, VIDEO_DECODE_8K_0110, TestSize.Level2)
{
    if (cap != nullptr) {
        int32_t maxWidth = GetVideoMaxWidthForHeight(cap, 4352);
        int32_t maxHeight = GetVideoMaxHeightForWidth(cap, 8192);
        if (maxWidth >= 8192 && maxHeight >= 4352) {
            auto vDecSample = make_shared<VDecAPI11Sample>();
            vDecSample->INP_DIR = INP_DIR_8192_4352_8BIT_264;
            vDecSample->DEFAULT_WIDTH = 8192;
            vDecSample->DEFAULT_HEIGHT = 4352;
            vDecSample->DEFAULT_FRAME_RATE = 30;
            vDecSample->SF_OUTPUT = true;
            vDecSample->outputYuvFlag = false;
            vDecSample->enbleSyncMode = 1;
            vDecSample->is8bitYuv = false;
            ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecName));
            ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
            ASSERT_EQ(AV_ERR_OK, vDecSample->DecodeSetSurface());
            ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
            vDecSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
            ASSERT_EQ(5, vDecSample->outFrameCount);
        }
    }
}

/**
 * @tc.number    : VIDEO_DECODE_8K_0120
 * @tc.name      : h264解码8k，异步buffer，8bit
 * @tc.desc      : function test
 */
HWTEST_F(HwdecFunc3NdkTest, VIDEO_DECODE_8K_0120, TestSize.Level2)
{
    if (cap != nullptr) {
        int32_t maxWidth = GetVideoMaxWidthForHeight(cap, 4352);
        int32_t maxHeight = GetVideoMaxHeightForWidth(cap, 8192);
        if (maxWidth >= 8192 && maxHeight >= 4352) {
            auto vDecSample = make_shared<VDecAPI11Sample>();
            vDecSample->INP_DIR = INP_DIR_8192_4352_8BIT_264;
            vDecSample->DEFAULT_WIDTH = 8192;
            vDecSample->DEFAULT_HEIGHT = 4352;
            vDecSample->DEFAULT_FRAME_RATE = 30;
            vDecSample->SF_OUTPUT = false;
            vDecSample->outputYuvFlag = false;
            vDecSample->is8bitYuv = false;
            vDecSample->NocaleHash = false;
            ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecName));
            ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
            ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
            ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
            vDecSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
            ASSERT_EQ(5, vDecSample->outFrameCount);
        }
    }
}

/**
 * @tc.number    : VIDEO_DECODE_8K_0130
 * @tc.name      : h265解码8k，同步buffer，8bit
 * @tc.desc      : function test
 */
HWTEST_F(HwdecFunc3NdkTest, VIDEO_DECODE_8K_0130, TestSize.Level2)
{
    if (cap_hevc != nullptr) {
        int32_t maxWidth = GetVideoMaxWidthForHeight(cap_hevc, 4992);
        int32_t maxHeight = GetVideoMaxHeightForWidth(cap_hevc, 8192);
        if (maxWidth >= 8192 && maxHeight >= 4992) {
            auto vDecSample = make_shared<VDecAPI11Sample>();
            vDecSample->INP_DIR = INP_DIR_8192_4992_8BIT_265;
            vDecSample->DEFAULT_WIDTH = 8192;
            vDecSample->DEFAULT_HEIGHT = 4992;
            vDecSample->DEFAULT_FRAME_RATE = 30;
            vDecSample->SF_OUTPUT = false;
            vDecSample->outputYuvFlag = false;
            vDecSample->enbleSyncMode = 1;
            vDecSample->NocaleHash = false;
            ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameHEVC));
            ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
            ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
            vDecSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
            ASSERT_EQ(5, vDecSample->outFrameCount);
        }
    }
}

/**
 * @tc.number    : VIDEO_DECODE_8K_0140
 * @tc.name      : h265解码8k，同步surface，8bit
 * @tc.desc      : function test
 */
HWTEST_F(HwdecFunc3NdkTest, VIDEO_DECODE_8K_0140, TestSize.Level2)
{
    if (cap_hevc != nullptr) {
        int32_t maxWidth = GetVideoMaxWidthForHeight(cap_hevc, 4992);
        int32_t maxHeight = GetVideoMaxHeightForWidth(cap_hevc, 8192);
        if (maxWidth >= 8192 && maxHeight >= 4992) {
            auto vDecSample = make_shared<VDecAPI11Sample>();
            vDecSample->INP_DIR = INP_DIR_8192_4992_8BIT_265;
            vDecSample->DEFAULT_WIDTH = 8192;
            vDecSample->DEFAULT_HEIGHT = 4992;
            vDecSample->DEFAULT_FRAME_RATE = 30;
            vDecSample->SF_OUTPUT = true;
            vDecSample->outputYuvFlag = false;
            vDecSample->enbleSyncMode = 1;
            ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameHEVC));
            ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
            ASSERT_EQ(AV_ERR_OK, vDecSample->DecodeSetSurface());
            ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
            vDecSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
            ASSERT_EQ(5, vDecSample->outFrameCount);
        }
    }
}

/**
 * @tc.number    : VIDEO_DECODE_8K_0150
 * @tc.name      : h265解码8k，异步buffer，8bit
 * @tc.desc      : function test
 */
HWTEST_F(HwdecFunc3NdkTest, VIDEO_DECODE_8K_0150, TestSize.Level2)
{
    if (cap_hevc != nullptr) {
        int32_t maxWidth = GetVideoMaxWidthForHeight(cap_hevc, 4992);
        int32_t maxHeight = GetVideoMaxHeightForWidth(cap_hevc, 8192);
        if (maxWidth >= 8192 && maxHeight >= 4992) {
            auto vDecSample = make_shared<VDecAPI11Sample>();
            vDecSample->INP_DIR = INP_DIR_8192_4992_8BIT_265;
            vDecSample->DEFAULT_WIDTH = 8192;
            vDecSample->DEFAULT_HEIGHT = 4992;
            vDecSample->DEFAULT_FRAME_RATE = 30;
            vDecSample->SF_OUTPUT = false;
            vDecSample->outputYuvFlag = false;
            vDecSample->NocaleHash = false;
            ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameHEVC));
            ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
            ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
            ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
            vDecSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
            ASSERT_EQ(5, vDecSample->outFrameCount);
        }
    }
}

/**
 * @tc.number    : VIDEO_DECODE_8K_0160
 * @tc.name      : h265解码8k，异步surface，8bit
 * @tc.desc      : function test
 */
HWTEST_F(HwdecFunc3NdkTest, VIDEO_DECODE_8K_0160, TestSize.Level2)
{
    if (cap_hevc != nullptr) {
        int32_t maxWidth = GetVideoMaxWidthForHeight(cap_hevc, 4992);
        int32_t maxHeight = GetVideoMaxHeightForWidth(cap_hevc, 8192);
        if (maxWidth >= 8192 && maxHeight >= 4992) {
            auto vDecSample = make_shared<VDecAPI11Sample>();
            vDecSample->INP_DIR = INP_DIR_8192_4992_8BIT_265;
            vDecSample->DEFAULT_WIDTH = 8192;
            vDecSample->DEFAULT_HEIGHT = 4992;
            vDecSample->DEFAULT_FRAME_RATE = 30;
            vDecSample->SF_OUTPUT = true;
            vDecSample->outputYuvFlag = false;
            ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameHEVC));
            ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
            ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
            ASSERT_EQ(AV_ERR_OK, vDecSample->DecodeSetSurface());
            ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
            vDecSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
            ASSERT_EQ(5, vDecSample->outFrameCount);
        }
    }
}

/**
 * @tc.number    : VIDEO_DECODE_8K_0170
 * @tc.name      : h265解码8k，同步buffer，8bit
 * @tc.desc      : function test
 */
HWTEST_F(HwdecFunc3NdkTest, VIDEO_DECODE_8K_0170, TestSize.Level2)
{
    if (cap_hevc != nullptr) {
        int32_t maxWidth = GetVideoMaxWidthForHeight(cap_hevc, 4320);
        int32_t maxHeight = GetVideoMaxHeightForWidth(cap_hevc, 7680);
        if (maxWidth >= 7680 && maxHeight >= 4320) {
            auto vDecSample = make_shared<VDecAPI11Sample>();
            vDecSample->INP_DIR = INP_DIR_7680_4320_8BIT_265;
            vDecSample->DEFAULT_WIDTH = 7680;
            vDecSample->DEFAULT_HEIGHT = 4320;
            vDecSample->DEFAULT_FRAME_RATE = 30;
            vDecSample->SF_OUTPUT = false;
            vDecSample->outputYuvFlag = false;
            vDecSample->enbleSyncMode = 1;
            vDecSample->NocaleHash = false;
            ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameHEVC));
            ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
            ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
            vDecSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
            ASSERT_EQ(5, vDecSample->outFrameCount);
        }
    }
}

/**
 * @tc.number    : VIDEO_DECODE_8K_0180
 * @tc.name      : h265解码8k，异步surface，8bit
 * @tc.desc      : function test
 */
HWTEST_F(HwdecFunc3NdkTest, VIDEO_DECODE_8K_0180, TestSize.Level2)
{
    if (cap_hevc != nullptr) {
        int32_t maxWidth = GetVideoMaxWidthForHeight(cap_hevc, 4320);
        int32_t maxHeight = GetVideoMaxHeightForWidth(cap_hevc, 7680);
        if (maxWidth >= 7680 && maxHeight >= 4320) {
            auto vDecSample = make_shared<VDecAPI11Sample>();
            vDecSample->INP_DIR = INP_DIR_7680_4320_8BIT_265;
            vDecSample->DEFAULT_WIDTH = 7680;
            vDecSample->DEFAULT_HEIGHT = 4320;
            vDecSample->DEFAULT_FRAME_RATE = 30;
            vDecSample->SF_OUTPUT = true;
            vDecSample->outputYuvFlag = false;
            ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameHEVC));
            ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
            ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
            ASSERT_EQ(AV_ERR_OK, vDecSample->DecodeSetSurface());
            ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
            vDecSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
            ASSERT_EQ(5, vDecSample->outFrameCount);
        }
    }
}

/**
 * @tc.number    : VIDEO_DECODE_8K_0190
 * @tc.name      : h265解码8k，同步surface，10bit
 * @tc.desc      : function test
 */
HWTEST_F(HwdecFunc3NdkTest, VIDEO_DECODE_8K_0190, TestSize.Level2)
{
    if (cap_hevc != nullptr) {
        int32_t maxWidth = GetVideoMaxWidthForHeight(cap_hevc, 4992);
        int32_t maxHeight = GetVideoMaxHeightForWidth(cap_hevc, 8192);
        if (maxWidth >= 8192 && maxHeight >= 4992) {
            auto vDecSample = make_shared<VDecAPI11Sample>();
            vDecSample->INP_DIR = INP_DIR_8192_4992_10BIT_265;
            vDecSample->DEFAULT_WIDTH = 8192;
            vDecSample->DEFAULT_HEIGHT = 4992;
            vDecSample->DEFAULT_FRAME_RATE = 30;
            vDecSample->SF_OUTPUT = true;
            vDecSample->outputYuvFlag = false;
            vDecSample->enbleSyncMode = 1;
            vDecSample->is8bitYuv = false;
            ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameHEVC));
            ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
            ASSERT_EQ(AV_ERR_OK, vDecSample->DecodeSetSurface());
            ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
            vDecSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
            ASSERT_EQ(5, vDecSample->outFrameCount);
        }
    }
}

/**
 * @tc.number    : VIDEO_DECODE_8K_0200
 * @tc.name      : h265解码8k，异步buffer，10bit
 * @tc.desc      : function test
 */
HWTEST_F(HwdecFunc3NdkTest, VIDEO_DECODE_8K_0200, TestSize.Level2)
{
    if (cap_hevc != nullptr) {
        int32_t maxWidth = GetVideoMaxWidthForHeight(cap_hevc, 4992);
        int32_t maxHeight = GetVideoMaxHeightForWidth(cap_hevc, 8192);
        if (maxWidth >= 8192 && maxHeight >= 4992) {
            auto vDecSample = make_shared<VDecAPI11Sample>();
            vDecSample->INP_DIR = INP_DIR_8192_4992_10BIT_265;
            vDecSample->DEFAULT_WIDTH = 8192;
            vDecSample->DEFAULT_HEIGHT = 4992;
            vDecSample->DEFAULT_FRAME_RATE = 30;
            vDecSample->SF_OUTPUT = false;
            vDecSample->outputYuvFlag = false;
            vDecSample->is8bitYuv = false;
            vDecSample->NocaleHash = false;
            ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameHEVC));
            ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
            ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
            ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
            vDecSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
            ASSERT_EQ(5, vDecSample->outFrameCount);
        }
    }
}

/**
 * @tc.number    : VIDEO_DECODE_8K_0210
 * @tc.name      : h265解码8k，同步buffer，10bit hdr
 * @tc.desc      : function test
 */
HWTEST_F(HwdecFunc3NdkTest, VIDEO_DECODE_8K_0210, TestSize.Level2)
{
    if (cap_hevc != nullptr) {
        int32_t maxWidth = GetVideoMaxWidthForHeight(cap_hevc, 4320);
        int32_t maxHeight = GetVideoMaxHeightForWidth(cap_hevc, 7680);
        if (maxWidth >= 7680 && maxHeight >= 4320) {
            auto vDecSample = make_shared<VDecAPI11Sample>();
            vDecSample->INP_DIR = INP_DIR_7680_4320_10BIT_HDR_265;
            vDecSample->DEFAULT_WIDTH = 7680;
            vDecSample->DEFAULT_HEIGHT = 4320;
            vDecSample->DEFAULT_FRAME_RATE = 30;
            vDecSample->SF_OUTPUT = false;
            vDecSample->outputYuvFlag = false;
            vDecSample->enbleSyncMode = 1;
            vDecSample->is8bitYuv = false;
            vDecSample->NocaleHash = false;
            ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameHEVC));
            ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
            ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
            vDecSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
            ASSERT_EQ(5, vDecSample->outFrameCount);
        }
    }
}

/**
 * @tc.number    : VIDEO_DECODE_8K_0220
 * @tc.name      : h265解码8k，异步surface，10bit hdr
 * @tc.desc      : function test
 */
HWTEST_F(HwdecFunc3NdkTest, VIDEO_DECODE_8K_0220, TestSize.Level2)
{
    if (cap_hevc != nullptr) {
        int32_t maxWidth = GetVideoMaxWidthForHeight(cap_hevc, 4320);
        int32_t maxHeight = GetVideoMaxHeightForWidth(cap_hevc, 7680);
        if (maxWidth >= 7680 && maxHeight >= 4320) {
            auto vDecSample = make_shared<VDecAPI11Sample>();
            vDecSample->INP_DIR = INP_DIR_7680_4320_10BIT_HDR_265;
            vDecSample->DEFAULT_WIDTH = 7680;
            vDecSample->DEFAULT_HEIGHT = 4320;
            vDecSample->DEFAULT_FRAME_RATE = 30;
            vDecSample->SF_OUTPUT = true;
            vDecSample->outputYuvFlag = false;
            vDecSample->is8bitYuv = false;
            ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameHEVC));
            ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
            ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
            ASSERT_EQ(AV_ERR_OK, vDecSample->DecodeSetSurface());
            ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
            vDecSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
            ASSERT_EQ(5, vDecSample->outFrameCount);
        }
    }
}

/**
 * @tc.number    : VIDEO_DECODE_8K_0230
 * @tc.name      : h265解码8k，同步surface，8bit
 * @tc.desc      : function test
 */
HWTEST_F(HwdecFunc3NdkTest, VIDEO_DECODE_8K_0230, TestSize.Level2)
{
    if (cap_hevc != nullptr) {
        int32_t maxWidth = GetVideoMaxWidthForHeight(cap_hevc, 8192);
        int32_t maxHeight = GetVideoMaxHeightForWidth(cap_hevc, 4992);
        if (maxWidth >= 4992 && maxHeight >= 8192) {
            auto vDecSample = make_shared<VDecAPI11Sample>();
            vDecSample->INP_DIR = INP_DIR_4992_8192_8BIT_265;
            vDecSample->DEFAULT_WIDTH = 4992;
            vDecSample->DEFAULT_HEIGHT = 8192;
            vDecSample->DEFAULT_FRAME_RATE = 30;
            vDecSample->SF_OUTPUT = true;
            vDecSample->outputYuvFlag = false;
            vDecSample->enbleSyncMode = 1;
            ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameHEVC));
            ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
            ASSERT_EQ(AV_ERR_OK, vDecSample->DecodeSetSurface());
            ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
            vDecSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
            ASSERT_EQ(5, vDecSample->outFrameCount);
        }
    }
}

/**
 * @tc.number    : VIDEO_DECODE_8K_0240
 * @tc.name      : h265解码8k，异步buffer，8bit
 * @tc.desc      : function test
 */
HWTEST_F(HwdecFunc3NdkTest, VIDEO_DECODE_8K_0240, TestSize.Level2)
{
    if (cap_hevc != nullptr) {
        int32_t maxWidth = GetVideoMaxWidthForHeight(cap_hevc, 8192);
        int32_t maxHeight = GetVideoMaxHeightForWidth(cap_hevc, 4320);
        if (maxWidth >= 4992 && maxHeight >= 8192) {
            auto vDecSample = make_shared<VDecAPI11Sample>();
            vDecSample->INP_DIR = INP_DIR_4992_8192_8BIT_265;
            vDecSample->DEFAULT_WIDTH = 4320;
            vDecSample->DEFAULT_HEIGHT = 8192;
            vDecSample->DEFAULT_FRAME_RATE = 30;
            vDecSample->SF_OUTPUT = false;
            vDecSample->outputYuvFlag = false;
            vDecSample->NocaleHash = false;
            ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameHEVC));
            ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
            ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
            ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
            vDecSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
            ASSERT_EQ(5, vDecSample->outFrameCount);
        }
    }
}

/**
 * @tc.number    : VIDEO_DECODE_8K_0250
 * @tc.name      : h265解码8k，同步buffer，8bit
 * @tc.desc      : function test
 */
HWTEST_F(HwdecFunc3NdkTest, VIDEO_DECODE_8K_0250, TestSize.Level2)
{
    if (cap_hevc != nullptr) {
        int32_t maxWidth = GetVideoMaxWidthForHeight(cap_hevc, 7680);
        int32_t maxHeight = GetVideoMaxHeightForWidth(cap_hevc, 4320);
        if (maxWidth >= 4320 && maxHeight >= 7680) {
            auto vDecSample = make_shared<VDecAPI11Sample>();
            vDecSample->INP_DIR = INP_DIR_4320_7680_8BIT_265;
            vDecSample->DEFAULT_WIDTH = 4320;
            vDecSample->DEFAULT_HEIGHT = 7680;
            vDecSample->DEFAULT_FRAME_RATE = 30;
            vDecSample->SF_OUTPUT = false;
            vDecSample->outputYuvFlag = false;
            vDecSample->enbleSyncMode = 1;
            vDecSample->NocaleHash = false;
            ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameHEVC));
            ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
            ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
            vDecSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
            ASSERT_EQ(5, vDecSample->outFrameCount);
        }
    }
}

/**
 * @tc.number    : VIDEO_DECODE_8K_0260
 * @tc.name      : h265解码8k，异步surface，8bit
 * @tc.desc      : function test
 */
HWTEST_F(HwdecFunc3NdkTest, VIDEO_DECODE_8K_0260, TestSize.Level2)
{
    if (cap_hevc != nullptr) {
        int32_t maxWidth = GetVideoMaxWidthForHeight(cap_hevc, 7680);
        int32_t maxHeight = GetVideoMaxHeightForWidth(cap_hevc, 4320);
        if (maxWidth >= 4320 && maxHeight >= 7680) {
            auto vDecSample = make_shared<VDecAPI11Sample>();
            vDecSample->INP_DIR = INP_DIR_4320_7680_8BIT_265;
            vDecSample->DEFAULT_WIDTH = 4320;
            vDecSample->DEFAULT_HEIGHT = 7680;
            vDecSample->DEFAULT_FRAME_RATE = 30;
            vDecSample->SF_OUTPUT = true;
            vDecSample->outputYuvFlag = false;
            ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameHEVC));
            ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
            ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
            ASSERT_EQ(AV_ERR_OK, vDecSample->DecodeSetSurface());
            ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
            vDecSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
            ASSERT_EQ(5, vDecSample->outFrameCount);
        }
    }
}

/**
 * @tc.number    : VIDEO_DECODE_8K_0270
 * @tc.name      : h265解码8k，同步surface，10bit
 * @tc.desc      : function test
 */
HWTEST_F(HwdecFunc3NdkTest, VIDEO_DECODE_8K_0270, TestSize.Level2)
{
    if (cap_hevc != nullptr) {
        int32_t maxWidth = GetVideoMaxWidthForHeight(cap_hevc, 8192);
        int32_t maxHeight = GetVideoMaxHeightForWidth(cap_hevc, 4992);
        if (maxWidth >= 4992 && maxHeight >= 8192) {
            auto vDecSample = make_shared<VDecAPI11Sample>();
            vDecSample->INP_DIR = INP_DIR_4992_8192_10BIT_HDR_265;
            vDecSample->DEFAULT_WIDTH = 4992;
            vDecSample->DEFAULT_HEIGHT = 8192;
            vDecSample->DEFAULT_FRAME_RATE = 30;
            vDecSample->SF_OUTPUT = true;
            vDecSample->outputYuvFlag = false;
            vDecSample->enbleSyncMode = 1;
            vDecSample->is8bitYuv = false;
            ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameHEVC));
            ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
            ASSERT_EQ(AV_ERR_OK, vDecSample->DecodeSetSurface());
            ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
            vDecSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
            ASSERT_EQ(5, vDecSample->outFrameCount);
        }
    }
}

/**
 * @tc.number    : VIDEO_DECODE_8K_0280
 * @tc.name      : h265解码8k，异步buffer，10bit
 * @tc.desc      : function test
 */
HWTEST_F(HwdecFunc3NdkTest, VIDEO_DECODE_8K_0280, TestSize.Level2)
{
    if (cap_hevc != nullptr) {
        int32_t maxWidth = GetVideoMaxWidthForHeight(cap_hevc, 8192);
        int32_t maxHeight = GetVideoMaxHeightForWidth(cap_hevc, 4992);
        if (maxWidth >= 4992 && maxHeight >= 8192) {
            auto vDecSample = make_shared<VDecAPI11Sample>();
            vDecSample->INP_DIR = INP_DIR_4992_8192_10BIT_HDR_265;
            vDecSample->DEFAULT_WIDTH = 4992;
            vDecSample->DEFAULT_HEIGHT = 8192;
            vDecSample->DEFAULT_FRAME_RATE = 30;
            vDecSample->SF_OUTPUT = false;
            vDecSample->outputYuvFlag = false;
            vDecSample->is8bitYuv = false;
            vDecSample->NocaleHash = false;
            ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameHEVC));
            ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
            ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
            ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
            vDecSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
            ASSERT_EQ(5, vDecSample->outFrameCount);
        }
    }
}

/**
 * @tc.number    : VIDEO_DECODE_8K_0290
 * @tc.name      : h265解码8k，同步buffer，10bit
 * @tc.desc      : function test
 */
HWTEST_F(HwdecFunc3NdkTest, VIDEO_DECODE_8K_0290, TestSize.Level2)
{
    if (cap_hevc != nullptr) {
        int32_t maxWidth = GetVideoMaxWidthForHeight(cap_hevc, 7680);
        int32_t maxHeight = GetVideoMaxHeightForWidth(cap_hevc, 4320);
        if (maxWidth >= 4320 && maxHeight >= 7680) {
            auto vDecSample = make_shared<VDecAPI11Sample>();
            vDecSample->INP_DIR = INP_DIR_4320_7680_10BIT_HDR_265;
            vDecSample->DEFAULT_WIDTH = 4320;
            vDecSample->DEFAULT_HEIGHT = 7680;
            vDecSample->DEFAULT_FRAME_RATE = 30;
            vDecSample->SF_OUTPUT = false;
            vDecSample->outputYuvFlag = false;
            vDecSample->enbleSyncMode = 1;
            vDecSample->is8bitYuv = false;
            vDecSample->NocaleHash = false;
            ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameHEVC));
            ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
            ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
            vDecSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
            ASSERT_EQ(5, vDecSample->outFrameCount);
        }
    }
}

/**
 * @tc.number    : VIDEO_DECODE_8K_0300
 * @tc.name      : h264解码8k，异步surface，10bit
 * @tc.desc      : function test
 */
HWTEST_F(HwdecFunc3NdkTest, VIDEO_DECODE_8K_0300, TestSize.Level2)
{
    if (cap_hevc != nullptr) {
        int32_t maxWidth = GetVideoMaxWidthForHeight(cap_hevc, 7680);
        int32_t maxHeight = GetVideoMaxHeightForWidth(cap_hevc, 4320);
        if (maxWidth >= 4320 && maxHeight >= 7680) {
            auto vDecSample = make_shared<VDecAPI11Sample>();
            vDecSample->INP_DIR = INP_DIR_4320_7680_10BIT_HDR_265;
            vDecSample->DEFAULT_WIDTH = 4320;
            vDecSample->DEFAULT_HEIGHT = 7680;
            vDecSample->DEFAULT_FRAME_RATE = 30;
            vDecSample->SF_OUTPUT = true;
            vDecSample->outputYuvFlag = false;
            vDecSample->is8bitYuv = false;
            ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameHEVC));
            ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
            ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
            ASSERT_EQ(AV_ERR_OK, vDecSample->DecodeSetSurface());
            ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
            vDecSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
            ASSERT_EQ(5, vDecSample->outFrameCount);
        }
    }
}

/**
 * @tc.number    : VIDEO_DECODE_8K_0310
 * @tc.name      : h265解码8k，同步surface，10bit
 * @tc.desc      : function test
 */
HWTEST_F(HwdecFunc3NdkTest, VIDEO_DECODE_8K_0310, TestSize.Level2)
{
    if (cap_hevc != nullptr) {
        int32_t maxWidth = GetVideoMaxWidthForHeight(cap_hevc, 8192);
        int32_t maxHeight = GetVideoMaxHeightForWidth(cap_hevc, 8192);
        if (maxWidth >= 8192 && maxHeight >= 8192) {
            auto vDecSample = make_shared<VDecAPI11Sample>();
            vDecSample->INP_DIR = INP_DIR_8192_8192_10BIT_265;
            vDecSample->DEFAULT_WIDTH = 8192;
            vDecSample->DEFAULT_HEIGHT = 8192;
            vDecSample->DEFAULT_FRAME_RATE = 30;
            vDecSample->SF_OUTPUT = true;
            vDecSample->outputYuvFlag = false;
            vDecSample->enbleSyncMode = 1;
            vDecSample->is8bitYuv = false;
            ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameHEVC));
            ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
            ASSERT_EQ(AV_ERR_OK, vDecSample->DecodeSetSurface());
            ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
            vDecSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
            ASSERT_EQ(5, vDecSample->outFrameCount);
        }
    }
}

/**
 * @tc.number    : VIDEO_DECODE_8K_0320
 * @tc.name      : h265解码8k，异步buffer，10bit
 * @tc.desc      : function test
 */
HWTEST_F(HwdecFunc3NdkTest, VIDEO_DECODE_8K_0320, TestSize.Level2)
{
    if (cap_hevc != nullptr) {
        int32_t maxWidth = GetVideoMaxWidthForHeight(cap_hevc, 8192);
        int32_t maxHeight = GetVideoMaxHeightForWidth(cap_hevc, 8192);
        if (maxWidth >= 8192 && maxHeight >= 8192) {
            auto vDecSample = make_shared<VDecAPI11Sample>();
            vDecSample->INP_DIR = INP_DIR_8192_8192_10BIT_265;
            vDecSample->DEFAULT_WIDTH = 8192;
            vDecSample->DEFAULT_HEIGHT = 8192;
            vDecSample->DEFAULT_FRAME_RATE = 30;
            vDecSample->SF_OUTPUT = false;
            vDecSample->outputYuvFlag = false;
            vDecSample->is8bitYuv = false;
            vDecSample->NocaleHash = false;
            ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameHEVC));
            ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
            ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
            ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
            vDecSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
            ASSERT_EQ(5, vDecSample->outFrameCount);
        }
    }
}

/**
 * @tc.number    : VIDEO_DECODE_8K_0330
 * @tc.name      : h264解码8k，同步buffer，8bit, lowlatency
 * @tc.desc      : function test
 */
HWTEST_F(HwdecFunc3NdkTest, VIDEO_DECODE_8K_0330, TestSize.Level2)
{
    if (cap != nullptr) {
        int32_t maxWidth = GetVideoMaxWidthForHeight(cap, 4320);
        int32_t maxHeight = GetVideoMaxHeightForWidth(cap, 8192);
        if (maxWidth >= 8190 && maxHeight >= 4320) {
            auto vDecSample = make_shared<VDecAPI11Sample>();
            vDecSample->INP_DIR = INP_DIR_8192_4320_8BIT_264;
            vDecSample->DEFAULT_WIDTH = 8192;
            vDecSample->DEFAULT_HEIGHT = 4320;
            vDecSample->DEFAULT_FRAME_RATE = 30;
            vDecSample->SF_OUTPUT = false;
            vDecSample->outputYuvFlag = false;
            vDecSample->enbleSyncMode = 1;
            vDecSample->enableLowLatency = true;
            vDecSample->NocaleHash = false;
            vDecSample->isCheckLowLatency = true;
            ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecName));
            ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
            ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
            vDecSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
            ASSERT_EQ(5, vDecSample->outFrameCount);
        }
    }
}

/**
 * @tc.number    : VIDEO_DECODE_8K_0340
 * @tc.name      : h265解码8k，异步buffer，8bit, lowlatency
 * @tc.desc      : function test
 */
HWTEST_F(HwdecFunc3NdkTest, VIDEO_DECODE_8K_0340, TestSize.Level2)
{
    if (cap_hevc != nullptr) {
        int32_t maxWidth = GetVideoMaxWidthForHeight(cap_hevc, 4992);
        int32_t maxHeight = GetVideoMaxHeightForWidth(cap_hevc, 8192);
        if (maxWidth >= 8192 && maxHeight >= 4992) {
            auto vDecSample = make_shared<VDecAPI11Sample>();
            vDecSample->INP_DIR = INP_DIR_8192_4992_8BIT_265;
            vDecSample->DEFAULT_WIDTH = 8192;
            vDecSample->DEFAULT_HEIGHT = 4992;
            vDecSample->DEFAULT_FRAME_RATE = 30;
            vDecSample->SF_OUTPUT = false;
            vDecSample->outputYuvFlag = false;
            vDecSample->enableLowLatency = true;
            vDecSample->NocaleHash = false;
            vDecSample->isCheckLowLatency = true;
            ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameHEVC));
            ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
            ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
            ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
            vDecSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
            ASSERT_EQ(5, vDecSample->outFrameCount);
        }
    }
}

/**
 * @tc.number    : VIDEO_DECODE_8K_0350
 * @tc.name      : h264解码8k，同步buffer，8bit, 变分辨率
 * @tc.desc      : function test
 */
HWTEST_F(HwdecFunc3NdkTest, VIDEO_DECODE_8K_0350, TestSize.Level2)
{
    if (cap != nullptr) {
        int32_t maxWidth = GetVideoMaxWidthForHeight(cap, 4320);
        int32_t maxHeight = GetVideoMaxHeightForWidth(cap, 8192);
        if (maxWidth >= 8192 && maxHeight >= 4320) {
            auto vDecSample = make_shared<VDecAPI11Sample>();
            vDecSample->INP_DIR = INP_DIR_RES_8K_264;
            vDecSample->DEFAULT_WIDTH = 8192;
            vDecSample->DEFAULT_HEIGHT = 4320;
            vDecSample->DEFAULT_FRAME_RATE = 30;
            vDecSample->SF_OUTPUT = false;
            vDecSample->outputYuvFlag = false;
            vDecSample->enbleSyncMode = 1;
            vDecSample->NocaleHash = true;
            ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecName));
            ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
            ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
            vDecSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
        }
    }
}

/**
 * @tc.number    : VIDEO_DECODE_8K_0360
 * @tc.name      : h265解码8k，异步surface，10bit, 变分辨率
 * @tc.desc      : function test
 */
HWTEST_F(HwdecFunc3NdkTest, VIDEO_DECODE_8K_0360, TestSize.Level2)
{
    if (cap_hevc != nullptr) {
        int32_t maxWidth = GetVideoMaxWidthForHeight(cap_hevc, 4992);
        int32_t maxHeight = GetVideoMaxHeightForWidth(cap_hevc, 8192);
        if (maxWidth >= 8192 && maxHeight >= 4992) {
            auto vDecSample = make_shared<VDecAPI11Sample>();
            vDecSample->INP_DIR = INP_DIR_RES_8K_265;
            vDecSample->DEFAULT_WIDTH = 8192;
            vDecSample->DEFAULT_HEIGHT = 4992;
            vDecSample->DEFAULT_FRAME_RATE = 30;
            vDecSample->SF_OUTPUT = true;
            vDecSample->outputYuvFlag = false;
            vDecSample->NocaleHash = true;
            vDecSample->is8bitYuv = false;
            ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameHEVC));
            ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
            ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
            ASSERT_EQ(AV_ERR_OK, vDecSample->DecodeSetSurface());
            ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
            vDecSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
        }
    }
}

/**
 * @tc.number    : VIDEO_DECODE_8K_0370
 * @tc.name      : h265解码8k，同步surface, OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE
 * @tc.desc      : function test
 */
HWTEST_F(HwdecFunc3NdkTest, VIDEO_DECODE_8K_0370, TestSize.Level2)
{
    if (cap_hevc != nullptr) {
        int32_t maxWidth = GetVideoMaxWidthForHeight(cap_hevc, 4320);
        int32_t maxHeight = GetVideoMaxHeightForWidth(cap_hevc, 8192);
        if (maxWidth >= 7680 && maxHeight >= 4320) {
            shared_ptr<VDecAPI11Sample> vDecSample = make_shared<VDecAPI11Sample>();
            vDecSample->INP_DIR = INP_DIR_7680_4320_10BIT_HDR_265;
            vDecSample->DEFAULT_WIDTH = 7680;
            vDecSample->DEFAULT_HEIGHT = 4320;
            vDecSample->SF_OUTPUT = true;
            vDecSample->TRANSFER_FLAG = true;
            vDecSample->enbleSyncMode = 1;
            if (ONLY_FOR_FLAGSHIP_CHIP) {
                ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameHEVC));
                ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
                ASSERT_EQ(AV_ERR_OK, vDecSample->DecodeSetSurface());
                ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
                vDecSample->WaitForEOS();
                ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
            }
            else {
                ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameHEVC));
                ASSERT_EQ(AV_ERR_UNSUPPORT, vDecSample->ConfigureVideoDecoder());
                vDecSample->WaitForEOS();
                ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
            }
        }
    }
}

/**
 * @tc.number    : VIDEO_DECODE_8K_0380
 * @tc.name      : H264解码8k, 异步surface, config transform
 * @tc.desc      : function test
 */
HWTEST_F(HwdecFunc3NdkTest, VIDEO_DECODE_8K_0380, TestSize.Level2)
{
    if (cap != nullptr) {
        int32_t maxWidth = GetVideoMaxWidthForHeight(cap, 4320);
        int32_t maxHeight = GetVideoMaxHeightForWidth(cap, 8192);
        if (maxWidth >= 8192 && maxHeight >= 4320) {
            auto vDecSample = make_shared<VDecAPI11Sample>();
            vDecSample->INP_DIR = INP_DIR_8192_4320_8BIT_264;
            vDecSample->DEFAULT_WIDTH = 8192;
            vDecSample->DEFAULT_HEIGHT = 4320;
            vDecSample->DEFAULT_FRAME_RATE = 30;
            ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecName));
            ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
            vDecSample->SF_OUTPUT = true;
            vDecSample->CreateSurface();
            ASSERT_EQ(AV_ERR_OK, vDecSample->SetConfigTransform());
            ASSERT_EQ(0, vDecSample->GetSurfaceTransform(0));
            ASSERT_EQ(AV_ERR_OK, vDecSample->Reset());
            vDecSample->setTransform = true;
            vDecSample->DEFAULT_TRANSFORM = -1;
            ASSERT_EQ(AV_ERR_INVALID_VAL, vDecSample->SetConfigTransform());
            ASSERT_EQ(0, vDecSample->GetSurfaceTransform(0));
            ASSERT_EQ(AV_ERR_OK, vDecSample->Reset());
            for (const auto& transformtype : transfromTypes) {
                vDecSample->DEFAULT_TRANSFORM = transformtype;
                ASSERT_EQ(AV_ERR_OK, vDecSample->SetConfigTransform());
                ASSERT_EQ(vDecSample->DEFAULT_TRANSFORM, vDecSample->GetSurfaceTransform(0));
                ASSERT_EQ(AV_ERR_OK, vDecSample->Reset());
            }
            ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
            ASSERT_EQ(AV_ERR_OK, vDecSample->SetSurface());
            ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
            ASSERT_EQ(vDecSample->DEFAULT_TRANSFORM, vDecSample->GetSurfaceTransform(0));
            vDecSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
            ASSERT_EQ(5, vDecSample->outFrameCount);
        }
    }
}

/**
 * @tc.number    : VIDEO_DECODE_8K_0390
 * @tc.name      : H264解码8k, 异步buffer, decode to NV12 format, enable VRR
 * @tc.desc      : function test
 */
HWTEST_F(HwdecFunc3NdkTest, VIDEO_DECODE_8K_0390, TestSize.Level2)
{
    if (cap != nullptr) {
        int32_t maxWidth = GetVideoMaxWidthForHeight(cap, 4320);
        int32_t maxHeight = GetVideoMaxHeightForWidth(cap, 8192);
        if (maxWidth >= 8192 && maxHeight >= 4320) {
            auto vDecSample = make_shared<VDecAPI11Sample>();
            vDecSample->INP_DIR = INP_DIR_8192_4320_8BIT_264;
            vDecSample->DEFAULT_FRAME_RATE = 30;
            vDecSample->DEFAULT_WIDTH = 8192;
            vDecSample->DEFAULT_HEIGHT = 4320;
            vDecSample->enableVRR = true;
            vDecSample->SF_OUTPUT = false;
            vDecSample->outputYuvFlag = false;
            vDecSample->NocaleHash = false;
            vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
            ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecName));
            ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
            ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
            ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
            vDecSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
            ASSERT_EQ(5, vDecSample->outFrameCount);
        }
    }
}

/**
 * @tc.number    : VIDEO_DECODE_8K_0400
 * @tc.name      : H265解码8k, 异步surface, config OH_MD_KEY_VIDEO_DECODER_BLANK_FRAME_ON_SHUTDOWN
 * @tc.desc      : function test
 */
HWTEST_F(HwdecFunc3NdkTest, VIDEO_DECODE_8K_0400, TestSize.Level2)
{
    if (cap_hevc != nullptr) {
        int32_t maxWidth = GetVideoMaxWidthForHeight(cap_hevc, 4992);
        int32_t maxHeight = GetVideoMaxHeightForWidth(cap_hevc, 8192);
        if (maxWidth >= 8192 && maxHeight >= 4992) {
            auto vDecSample = make_shared<VDecAPI11Sample>();
            vDecSample->INP_DIR = INP_DIR_8192_4992_8BIT_265;
            vDecSample->DEFAULT_WIDTH = 8192;
            vDecSample->DEFAULT_HEIGHT = 4992;
            vDecSample->DEFAULT_FRAME_RATE = 30;
            vDecSample->SF_OUTPUT = true;
            vDecSample->enbleBlankFrame = 1;
            ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameHEVC));
            ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
            ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
            ASSERT_EQ(AV_ERR_OK, vDecSample->DecodeSetSurface());
            ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
            vDecSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
            ASSERT_EQ(5, vDecSample->outFrameCount);
        }
    }
}

/**
 * @tc.number    : VIDEO_DECODE_8K_0410
 * @tc.name      : H265解码8k, 异步surface切换
 * @tc.desc      : function test
 */
HWTEST_F(HwdecFunc3NdkTest, VIDEO_DECODE_8K_0410, TestSize.Level2)
{
    if (cap_hevc != nullptr) {
        int32_t maxWidth = GetVideoMaxWidthForHeight(cap_hevc, 4992);
        int32_t maxHeight = GetVideoMaxHeightForWidth(cap_hevc, 8192);
        if (maxWidth >= 8192 && maxHeight >= 4992) {
            auto vDecSample = make_shared<VDecAPI11Sample>();
            vDecSample->INP_DIR = INP_DIR_8192_4992_8BIT_265;
            vDecSample->DEFAULT_WIDTH = 8192;
            vDecSample->DEFAULT_HEIGHT = 4992;
            vDecSample->DEFAULT_FRAME_RATE = 30;
            vDecSample->SF_OUTPUT = true;
            vDecSample->autoSwitchSurface = true;
            vDecSample->AFTER_EOS_DESTORY_CODEC = false;
            vDecSample->sleepOnFPS = true;
            vDecSample->CreateSurface();
            ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameHEVC));
            ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
            ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
            ASSERT_EQ(AV_ERR_OK, vDecSample->SetSurface());
            ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
            vDecSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
            ASSERT_EQ(5, vDecSample->outFrameCount);
            ASSERT_EQ(AV_ERR_OK, vDecSample->Reset());
            ASSERT_EQ(AV_ERR_INVALID_STATE, vDecSample->SwitchSurface());
            ASSERT_EQ(AV_ERR_OK, vDecSample->Release());
        }
    }
}

/**
 * @tc.number    : VIDEO_DECODE_DESTROY_SURFACE
 * @tc.name      : 创建surface-销毁surface-设置surface
 * @tc.desc      : function test
 */
HWTEST_F(HwdecFunc3NdkTest, VIDEO_DECODE_DESTROY_SURFACE_0010, TestSize.Level0)
{
    if (cap != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_1080_30;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = true;
        vDecSample->outputYuvFlag = false;
        vDecSample->needDestroySurface = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecName));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_INVALID_VAL, vDecSample->DecodeSetSurface());
    }
}
}