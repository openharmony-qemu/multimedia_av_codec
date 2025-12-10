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
}