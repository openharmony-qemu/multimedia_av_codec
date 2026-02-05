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
#include "videodec_api11_sample.h"
#include "native_avcodec_base.h"
#include "avcodec_codec_name.h"
#include "native_avcapability.h"
#include "native_avformat.h"
#include "native_avdemuxer.h"
#include "native_avsource.h"
#include "native_avmemory.h"
#include <fcntl.h>
#include "videoenc_sample.h"


#define MAX_THREAD 16

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;

namespace {
    OH_AVCapability *cap = nullptr;
    constexpr uint32_t CODEC_NAME_SIZE = 128;
    char g_codecName[CODEC_NAME_SIZE] = {};
} // namespace
namespace OHOS {
namespace Media {
class Av1decFuncNdkTest : public testing::Test {
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
    const char *INP_DIR_0 = "/data/test/media/test_av1.ivf";
    const char *INP_DIR_1 = "/data/test/media/av1_main_L2.0_426x240.ivf";
    const char *INP_DIR_2 = "/data/test/media/av1_main_L2.1_640x360.ivf";
    const char *INP_DIR_3 = "/data/test/media/av1_main_L3.0_720x480.ivf";
    const char *INP_DIR_4 = "/data/test/media/av1_main_L3.1_1280x720.ivf";
    const char *INP_DIR_5 = "/data/test/media/av1_main_L4.0_1920x1080.ivf";
    const char *INP_DIR_6 = "/data/test/media/av1_main_L4.1_1920x1080_60fps.ivf";
    const char *INP_DIR_7 = "/data/test/media/av1_main_L5.0_3840x1600.ivf";
    const char *INP_DIR_8 = "/data/test/media/av1_main_L5.1_2560x1440_60fps.ivf";
    const char *INP_DIR_9 = "/data/test/media/av1_main_L5.2_3840x2160_120fps.ivf";
    const char *INP_DIR_10 = "/data/test/media/av1_main_L5.3_3840x2160_120fps.ivf";
    const char *INP_DIR_11 = "/data/test/media/av1_main_L6.0_7680x4320.ivf";
    const char *INP_DIR_12 = "/data/test/media/av1_main_L6.1_7680x4320_60fps.ivf";
    const char *INP_DIR_13 = "/data/test/media/av1_main_L6.2_7680x4320_120fps.ivf";
    const char *INP_DIR_14 = "/data/test/media/av1_main_L6.3_7680x4320_120fps.ivf";
    const char *INP_DIR_15 = "/data/test/media/av1_480x640.ivf";
    const char *INP_DIR_16 = "/data/test/media/av1_600x800.ivf";
    const char *INP_DIR_17 = "/data/test/media/av1_1080x1920.ivf";
    const char *INP_DIR_18 = "/data/test/media/av1_4x4.ivf";
    const char *INP_DIR_19 = "/data/test/media/av1_8192x4352.ivf";
    const char *INP_DIR_20 = "/data/test/media/av1_IFrame_only.ivf";
    const char *INP_DIR_21 = "/data/test/media/av1_IPFrame_only.ivf";
    const char *INP_DIR_22 = "/data/test/media/av1_1279x719.ivf";
    const char *INP_DIR_23 = "/data/test/media/av1_high_L2.0_426x240.ivf";
    const char *INP_DIR_24 = "/data/test/media/av1_high_L2.1_640x360.ivf";
    const char *INP_DIR_25 = "/data/test/media/av1_high_L3.0_854x480.ivf";
    const char *INP_DIR_26 = "/data/test/media/av1_high_L3.1_1280x720.ivf";
    const char *INP_DIR_27 = "/data/test/media/av1_high_L4.0_1920x1080.ivf";
    const char *INP_DIR_28 = "/data/test/media/av1_high_L4.1_1920x1080_60fps.ivf";
    const char *INP_DIR_29 = "/data/test/media/av1_high_L5.0_3840x2160.ivf";
    const char *INP_DIR_30 = "/data/test/media/av1_high_L5.1_3840x2160_60fps.ivf";
    const char *INP_DIR_31 = "/data/test/media/av1_high_L5.2_3840x2160_120fps.ivf";
    const char *INP_DIR_32 = "/data/test/media/av1_high_L5.3_3840x2160_120fps.ivf";
    const char *INP_DIR_33 = "/data/test/media/av1_high_L6.0_7680x4320.ivf";
    const char *INP_DIR_34 = "/data/test/media/av1_high_L6.1_7680x4320_60fps.ivf";
    const char *INP_DIR_35 = "/data/test/media/av1_high_L6.2_7680x4320_120fps.ivf";
    const char *INP_DIR_36 = "/data/test/media/av1_high_L6.3_7680x4320_120fps.ivf";
    const char *INP_DIR_37 = "/data/test/media/av1_pro_L2.0_426x240.ivf";
    const char *INP_DIR_38 = "/data/test/media/av1_pro_L2.1_640x360.ivf";
    const char *INP_DIR_39 = "/data/test/media/av1_pro_L3.0_854x480.ivf";
    const char *INP_DIR_40 = "/data/test/media/av1_pro_L3.1_1280x720.ivf";
    const char *INP_DIR_41 = "/data/test/media/av1_pro_L4.0_1920x1080.ivf";
    const char *INP_DIR_42 = "/data/test/media/av1_pro_L4.1_1920x1080_60fps.ivf";
    const char *INP_DIR_43 = "/data/test/media/av1_pro_L5.0_3840x2160.ivf";
    const char *INP_DIR_44 = "/data/test/media/av1_pro_L5.1_3840x2160_60fps.ivf";
    const char *INP_DIR_45 = "/data/test/media/av1_pro_L5.2_3840x2160_120fps.ivf";
    const char *INP_DIR_46 = "/data/test/media/av1_pro_L5.3_3840x2160_120fps.ivf";
    const char *INP_DIR_47 = "/data/test/media/av1_pro_L6.0_7680x4320.ivf";
    const char *INP_DIR_48 = "/data/test/media/av1_pro_L6.1_7680x4320_60fps.ivf";
    const char *INP_DIR_49 = "/data/test/media/av1_pro_L6.2_7680x4320_120fps.ivf";
    const char *INP_DIR_50 = "/data/test/media/av1_pro_L6.3_7680x4320_120fps.ivf";
    const char *INP_DIR_51 = "/data/test/media/av1_300fps.ivf";
};

static OH_AVCapability *cap_av1 = nullptr;
static string g_codecNameAv1 = "";
constexpr uint32_t FRAMESIZE180 = 180;
constexpr uint32_t FRAMESIZE300 = 300;
constexpr uint32_t FRAMESIZE45 = 45;
constexpr uint32_t FRAMESIZE30 = 30;
constexpr uint32_t FRAMESIZE90 = 90;
constexpr uint32_t FRAMESIZE120 = 120;
constexpr uint32_t FRAMESIZE60 = 60;
void Av1decFuncNdkTest::SetUpTestCase()
{
    cap_av1 = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_AV1, false, SOFTWARE);
    g_codecNameAv1 = OH_AVCapability_GetName(cap_av1);
    cout << "g_codecNameAv1: " << g_codecNameAv1 << endl;
    cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_AVC, true, HARDWARE);
    const char *tmpCodecName = OH_AVCapability_GetName(cap);
    if (memcpy_s(g_codecName, sizeof(g_codecName), tmpCodecName, strlen(tmpCodecName)) != 0)
        cout << "memcpy failed" << endl;
    cout << "codecname: " << g_codecName << endl;
}

void Av1decFuncNdkTest::TearDownTestCase() {}
void Av1decFuncNdkTest::SetUp() {}
void Av1decFuncNdkTest::TearDown() {}
} // namespace Media
} // namespace OHOS

static void getVideoSizeRange(OH_AVRange *widthRange, OH_AVRange *heightRange)
{
    memset_s(widthRange, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    memset_s(heightRange, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_AV1, false, SOFTWARE);
    OH_AVCapability_GetVideoHeightRange(capability, heightRange);
    cout << "minval=" << heightRange->minVal << "  maxval=" << heightRange->maxVal << endl;
    OH_AVCapability_GetVideoWidthRange(capability, widthRange);
    cout << "minval=" << widthRange->minVal << "  maxval=" << widthRange->maxVal << endl;
}


namespace {
/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0001
 * @tc.name      : decode Av1 buffer with default settings
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0001, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_0;
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForAV1());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE180, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0002
 * @tc.name      : decode Av1 buffer with profile AV1_PROFILE_MAIN
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0002, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_0;
    vDecSample->outputYuvFlag = true;
    vDecSample->trackFormat = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(vDecSample->trackFormat, OH_MD_KEY_PROFILE, AV1_PROFILE_MAIN);
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForAV1());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE180, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0003
 * @tc.name      : decode Av1 buffer with profile AV1_PROFILE_HIGH
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0003, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_0;
    vDecSample->outputYuvFlag = true;
    vDecSample->trackFormat = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(vDecSample->trackFormat, OH_MD_KEY_PROFILE, AV1_PROFILE_HIGH);
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForAV1());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE180, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0004
 * @tc.name      : decode Av1 buffer with profile AV1_PROFILE_PROFESSIONAL
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0004, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_0;
    vDecSample->outputYuvFlag = true;
    vDecSample->trackFormat = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(vDecSample->trackFormat, OH_MD_KEY_PROFILE, AV1_PROFILE_PROFESSIONAL);
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForAV1());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE180, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0005
 * @tc.name      : decode Av1 buffer with rotation angle 90
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0005, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_0;
    int32_t angle = 90;
    vDecSample->trackFormat = OH_AVFormat_Create();
    (void)OH_AVFormat_SetIntValue(vDecSample->trackFormat, OH_MD_KEY_ROTATION, angle);
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForAV1());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE180, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0006
 * @tc.name      : decode Av1 buffer with rotation angle 180
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0006, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_0;
    int32_t angle = 180;
    vDecSample->trackFormat = OH_AVFormat_Create();
    (void)OH_AVFormat_SetIntValue(vDecSample->trackFormat, OH_MD_KEY_ROTATION, angle);
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForAV1());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE180, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0007
 * @tc.name      : decode Av1 buffer with rotation angle 270
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0007, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_0;
    int32_t angle = 270;
    vDecSample->trackFormat = OH_AVFormat_Create();
    (void)OH_AVFormat_SetIntValue(vDecSample->trackFormat, OH_MD_KEY_ROTATION, angle);
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForAV1());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE180, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0008
 * @tc.name      : decode H263 buffer and handle error
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0008, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = "/data/test/media/profile0_level10_I_128x96.h263";
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderFor263());
    vDecSample->WaitForEOS();
    cout << "outFrameCount" << vDecSample->outFrameCount << endl;
    ASSERT_EQ(false, vDecSample->isonError);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0009
 * @tc.name      : decode Av1 buffer with invalid width and height (below range)
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0009, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_0;
    vDecSample->outputYuvFlag = true;
    OH_AVRange widthRange;
    OH_AVRange heightRange;
    getVideoSizeRange(&widthRange, &heightRange);
    vDecSample->DEFAULT_WIDTH = widthRange.minVal - 1;
    vDecSample->DEFAULT_HEIGHT = heightRange.minVal - 1;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_INVALID_VAL, vDecSample->ConfigureVideoDecoder());
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0010
 * @tc.name      : decode Av1 buffer with invalid width and height (above range)
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0010, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_0;
    vDecSample->outputYuvFlag = true;
    OH_AVRange widthRange;
    OH_AVRange heightRange;
    getVideoSizeRange(&widthRange, &heightRange);
    vDecSample->DEFAULT_WIDTH = widthRange.maxVal + 1;
    vDecSample->DEFAULT_HEIGHT = heightRange.maxVal + 1;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_INVALID_VAL, vDecSample->ConfigureVideoDecoder());
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0011
 * @tc.name      : decode Av1 buffer with maximum width and height
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0011, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_0;
    vDecSample->outputYuvFlag = true;
    int32_t max = 2147483647;
    vDecSample->DEFAULT_WIDTH = max;
    vDecSample->DEFAULT_HEIGHT = max;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_INVALID_VAL, vDecSample->ConfigureVideoDecoder());
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0012
 * @tc.name      : decode Av1 buffer with minimum valid width and height
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0012, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_0;
    vDecSample->outputYuvFlag = true;
    OH_AVRange widthRange;
    OH_AVRange heightRange;
    getVideoSizeRange(&widthRange, &heightRange);
    vDecSample->DEFAULT_WIDTH = widthRange.minVal;
    vDecSample->DEFAULT_HEIGHT = heightRange.minVal;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForAV1());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE180, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0013
 * @tc.name      : decode Av1 buffer with maximum valid width and height
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0013, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_0;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 8192;
    vDecSample->DEFAULT_HEIGHT = 4352;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForAV1());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE180, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0014
 * @tc.name      : decode Av1 buffer with negative width and height
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0014, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_0;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = -1;
    vDecSample->DEFAULT_HEIGHT = -1;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_INVALID_VAL, vDecSample->ConfigureVideoDecoder());
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0015
 * @tc.name      : decode Av1 buffer with unsupported pixel format (-1)
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0015, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_0;
    vDecSample->outputYuvFlag = true;
    vDecSample->defaultPixelFormat = -1;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_UNSUPPORT, vDecSample->ConfigureVideoDecoder());
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0016
 * @tc.name      : decode Av1 buffer with unsupported pixel format (0)
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0016, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_0;
    vDecSample->outputYuvFlag = true;
    vDecSample->defaultPixelFormat = 0;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_UNSUPPORT, vDecSample->ConfigureVideoDecoder());
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0017
 * @tc.name      : decode Av1 buffer with supported pixel format (2)
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0017, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_0;
    vDecSample->outputYuvFlag = true;
    vDecSample->defaultPixelFormat = 2;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForAV1());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE180, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0018
 * @tc.name      : decode Av1 buffer with supported pixel format (3)
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0018, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_0;
    vDecSample->outputYuvFlag = true;
    vDecSample->defaultPixelFormat = 3;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForAV1());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE180, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0019
 * @tc.name      : decode Av1 buffer with unsupported pixel format (5)
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0019, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_0;
    vDecSample->outputYuvFlag = true;
    vDecSample->defaultPixelFormat = 5;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_UNSUPPORT, vDecSample->ConfigureVideoDecoder());
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0020
 * @tc.name      : decode Av1 buffer with unsupported pixel format (maximum value)
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0020, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_0;
    vDecSample->outputYuvFlag = true;
    int32_t max = 2147483647;
    vDecSample->defaultPixelFormat = max;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_UNSUPPORT, vDecSample->ConfigureVideoDecoder());
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0021
 * @tc.name      : decode Av1 buffer with pixel format NV12
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0021, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_5;
    vDecSample->outputYuvFlag = true;
    vDecSample->needCheckHash = true;
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    vDecSample->defaultPixelFormat = AV_PIXEL_FORMAT_NV12;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForAV1());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE180, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0022
 * @tc.name      : decode Av1 buffer with pixel format NV21
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0022, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_5;
    vDecSample->outputYuvFlag = true;
    vDecSample->needCheckHash = true;
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    vDecSample->defaultPixelFormat = AV_PIXEL_FORMAT_NV21;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForAV1());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE180, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0023
 * @tc.name      : decode Av1 buffer with unsupported pixel format (1)
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0023, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();

    vDecSample->INP_DIR = INP_DIR_0;
    vDecSample->outputYuvFlag = true;
    vDecSample->defaultPixelFormat = 1;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_UNSUPPORT, vDecSample->ConfigureVideoDecoder());
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0024
 * @tc.name      : decode Av1 surface with pixel format NV12
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0024, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_0;
    vDecSample->SF_OUTPUT = true;
    vDecSample->outputYuvSurface = true;
    vDecSample->defaultPixelFormat = AV_PIXEL_FORMAT_NV12;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_SurfaceForAv1(g_codecNameAv1.c_str()));
    vDecSample->WaitForEOS();
    ASSERT_EQ(FRAMESIZE180, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0025
 * @tc.name      : decode Av1 surface with pixel format NV21
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0025, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_0;
    vDecSample->SF_OUTPUT = true;
    vDecSample->outputYuvSurface = true;
    vDecSample->defaultPixelFormat = AV_PIXEL_FORMAT_NV21;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_SurfaceForAv1(g_codecNameAv1.c_str()));
    vDecSample->WaitForEOS();
    ASSERT_EQ(FRAMESIZE180, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0026
 * @tc.name      : switch surface model in normal state
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0026, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_0;
    vDecSample->SF_OUTPUT = true;
    vDecSample->autoSwitchSurface = true;
    vDecSample->AFTER_EOS_DESTORY_CODEC = false;
    vDecSample->sleepOnFPS = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_SurfaceForAv1(g_codecNameAv1.c_str()));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->Reset());
    ASSERT_EQ(AV_ERR_INVALID_STATE, vDecSample->SwitchSurface());
    ASSERT_EQ(AV_ERR_OK, vDecSample->Release());
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0027
 * @tc.name      : switch surface model in flushed state
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0027, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_0;
    vDecSample->SF_OUTPUT = true;
    vDecSample->autoSwitchSurface = true;
    vDecSample->sleepOnFPS = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_SurfaceForAv1(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->Flush());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
    ASSERT_EQ(AV_ERR_OK, vDecSample->Stop());
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0028
 * @tc.name      : switch surface model in running state
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0028, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_0;
    vDecSample->SF_OUTPUT = false;
    vDecSample->autoSwitchSurface = false;
    vDecSample->CreateSurface();
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDecForAv1(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, vDecSample->SwitchSurface());
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0029
 * @tc.name      : repeat setSurface calls quickly
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0029, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_0;
    vDecSample->SF_OUTPUT = true;
    vDecSample->autoSwitchSurface = true;
    vDecSample->sleepOnFPS = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_SurfaceForAv1(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
    vDecSample->WaitForEOS();
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0030
 * @tc.name      : switch surface model from flush to running state
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0030, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_0;
    vDecSample->SF_OUTPUT = true;
    vDecSample->autoSwitchSurface = true;
    vDecSample->AFTER_EOS_DESTORY_CODEC = false;
    vDecSample->sleepOnFPS = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_SurfaceForAv1(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->Flush());
    ASSERT_EQ(AV_ERR_OK, vDecSample->Start());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0031
 * @tc.name      : switch surface model after decoder finishes to EOS state
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0031, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_0;
    vDecSample->SF_OUTPUT = true;
    vDecSample->autoSwitchSurface = true;
    vDecSample->AFTER_EOS_DESTORY_CODEC = false;
    vDecSample->sleepOnFPS = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_SurfaceForAv1(g_codecNameAv1.c_str()));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0032
 * @tc.name      : buffer model change after decoder finishes to EOS state
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0032, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_0;
    vDecSample->SF_OUTPUT = false;
    vDecSample->autoSwitchSurface = false;
    vDecSample->CreateSurface();
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDecForAv1(g_codecNameAv1.c_str()));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, vDecSample->SwitchSurface());
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0033
 * @tc.name      : buffer model change from running to flushed state
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0033, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_0;
    vDecSample->SF_OUTPUT = false;
    vDecSample->autoSwitchSurface = false;
    vDecSample->CreateSurface();
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDecForAv1(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->Flush());
    ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, vDecSample->SwitchSurface());
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0034
 * @tc.name      : buffer model change from flushed to running state
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0034, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_0;
    vDecSample->SF_OUTPUT = false;
    vDecSample->autoSwitchSurface = false;
    vDecSample->CreateSurface();
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDecForAv1(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->Flush());
    ASSERT_EQ(AV_ERR_OK, vDecSample->Start());
    ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, vDecSample->SwitchSurface());
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0035
 * @tc.name      : buffer model change in normal state
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0035, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_0;
    vDecSample->SF_OUTPUT = false;
    vDecSample->autoSwitchSurface = false;
    vDecSample->CreateSurface();
    vDecSample->SF_OUTPUT = false;
    vDecSample->autoSwitchSurface = false;
    vDecSample->CreateSurface();
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDecForAv1(g_codecNameAv1.c_str()));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->Reset());
    ASSERT_EQ(AV_ERR_INVALID_STATE, vDecSample->SwitchSurface());
    ASSERT_EQ(AV_ERR_OK, vDecSample->Release());
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0036
 * @tc.name      : switch surface model in config state
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0036, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_0;
    vDecSample->SF_OUTPUT = false;
    vDecSample->autoSwitchSurface = false;
    vDecSample->CreateSurface();
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0037
 * @tc.name      : invalid surface switch in config state
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0037, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_0;
    vDecSample->SF_OUTPUT = false;
    vDecSample->autoSwitchSurface = false;
    vDecSample->CreateSurface();
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_INVALID_STATE, vDecSample->SwitchSurface());
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0038
 * @tc.name      : two objects repeat setSurface calls quickly
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0038, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_0;
    vDecSample->SF_OUTPUT = true;
    vDecSample->autoSwitchSurface = true;
    vDecSample->sleepOnFPS = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_SurfaceForAv1(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
    vDecSample->WaitForEOS();
    vDecSample->ReleaseInFile();

    auto vDecSample_1 = make_shared<VDecAPI11Sample>();
    vDecSample_1->INP_DIR = INP_DIR_0;
    vDecSample_1->SF_OUTPUT = true;
    vDecSample_1->autoSwitchSurface = true;
    vDecSample_1->sleepOnFPS = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample_1->RunVideoDec_SurfaceForAv1(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample_1->SwitchSurface());
    vDecSample_1->WaitForEOS();
    vDecSample->ReleaseInFile();
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0039
 * @tc.name      : repeat setSurface calls quickly two times
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0039, TestSize.Level0)
{
    for (int i = 0; i < 2; i++) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_0;
        vDecSample->SF_OUTPUT = true;
        vDecSample->autoSwitchSurface = true;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_SurfaceForAv1(g_codecNameAv1.c_str()));
        ASSERT_EQ(AV_ERR_OK, vDecSample->RepeatCallSetSurface());
        vDecSample->WaitForEOS();
        vDecSample->ReleaseInFile();
    }
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0040
 * @tc.name      : decode Av1 buffer from FMP4 file
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0040, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    const char *file = "/data/test/media/av1_fmp4.mp4";
    
    vDecSample->getFormat(file);
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE180, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0041
 * @tc.name      : decode Av1 buffer from MKV file
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0041, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    const char *file = "/data/test/media/av1_mkv.mkv";
    
    vDecSample->getFormat(file);
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE180, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0042
 * @tc.name      : decode Av1 buffer from MP4 file
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0042, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    const char *file = "/data/test/media/av1_mp4.mp4";
    
    vDecSample->getFormat(file);
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE180, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0043
 * @tc.name      : decode Av1 buffer, max frame rate 300fps
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0043, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_51;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    vDecSample->DEFAULT_FRAME_RATE = 300;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForAV1());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE300, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0044
 * @tc.name      : decode Av1 buffer from AVI file
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0044, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    const char *file = "/data/test/media/av1_avi.avi";
    
    vDecSample->getFormat(file);
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE180, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0045
 * @tc.name      : decode Av1 buffer and encode to H264
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0045, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_0;
    
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForAV1());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE180, vDecSample->outFrameCount);

    auto vEncSample = make_unique<VEncNdkSample>();
    vEncSample->INP_DIR = vDecSample->OUT_DIR;
    vEncSample->DEFAULT_WIDTH = 1920;
    vEncSample->DEFAULT_HEIGHT = 1080;
    vEncSample->DEFAULT_FRAME_RATE = 30;
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0046
 * @tc.name      : decode Av1 buffer, main profile L0 426x240
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0046, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 426;
    vDecSample->DEFAULT_HEIGHT = 240;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForAV1());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE180, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0047
 * @tc.name      : decode Av1 buffer, main profile L1 640x360
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0047, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_2;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 640;
    vDecSample->DEFAULT_HEIGHT = 360;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForAV1());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE180, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0048
 * @tc.name      : decode Av1 buffer, main profile L4 720x480
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0048, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_3;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 720;
    vDecSample->DEFAULT_HEIGHT = 480;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForAV1());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE180, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0049
 * @tc.name      : decode Av1 buffer, main profile L5 1280x720
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0049, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_4;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 1280;
    vDecSample->DEFAULT_HEIGHT = 720;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForAV1());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE180, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0050
 * @tc.name      : decode Av1 buffer, main profile L8 1920x1080
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0050, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_5;
    vDecSample->outputYuvFlag = true;
    vDecSample->needCheckHash = true;
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForAV1());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE180, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0051
 * @tc.name      : decode Av1 buffer, main profile L9 1920x1080 60fps
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0051, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_6;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    vDecSample->DEFAULT_FRAME_RATE = 60;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForAV1());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE90, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0052
 * @tc.name      : decode Av1 buffer, main profile L12 3840x1600
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0052, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_7;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 3840;
    vDecSample->DEFAULT_HEIGHT = 1600;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForAV1());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE45, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0053
 * @tc.name      : decode Av1 buffer, main profile L13 2560x1440 60fps
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0053, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_8;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 2560;
    vDecSample->DEFAULT_HEIGHT = 1440;
    vDecSample->DEFAULT_FRAME_RATE = 60;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForAV1());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE90, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0054
 * @tc.name      : decode Av1 buffer, main profile L14 3840x2160 120fps
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0054, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_9;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 3840;
    vDecSample->DEFAULT_HEIGHT = 2160;
    vDecSample->DEFAULT_FRAME_RATE = 120;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForAV1());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE180, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0055
 * @tc.name      : decode Av1 buffer, main profile L15 3840x2160 120fps
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0055, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_10;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 3840;
    vDecSample->DEFAULT_HEIGHT = 2160;
    vDecSample->DEFAULT_FRAME_RATE = 120;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForAV1());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE180, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0056
 * @tc.name      : decode Av1 buffer, main profile L16 7680x4320
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0056, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_11;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 7680;
    vDecSample->DEFAULT_HEIGHT = 4320;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForAV1());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE45, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0057
 * @tc.name      : decode Av1 buffer, main profile L17 7680x4320 60fps
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0057, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_12;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 7680;
    vDecSample->DEFAULT_HEIGHT = 4320;
    vDecSample->DEFAULT_FRAME_RATE = 60;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForAV1());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE90, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0058
 * @tc.name      : decode Av1 buffer, main profile L18 7680x4320 120fps
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0058, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_13;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 7680;
    vDecSample->DEFAULT_HEIGHT = 4320;
    vDecSample->DEFAULT_FRAME_RATE = 120;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForAV1());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE120, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0059
 * @tc.name      : decode Av1 buffer, main profile L19 7680x4320 120fps
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0059, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_14;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 7680;
    vDecSample->DEFAULT_HEIGHT = 4320;
    vDecSample->DEFAULT_FRAME_RATE = 120;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForAV1());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE120, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0060
 * @tc.name      : decode Av1 buffer, 480x640
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0060, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_15;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 480;
    vDecSample->DEFAULT_HEIGHT = 640;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForAV1());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE180, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0061
 * @tc.name      : decode Av1 buffer, 600x800
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0061, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_16;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 600;
    vDecSample->DEFAULT_HEIGHT = 800;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForAV1());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE180, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0062
 * @tc.name      : decode Av1 buffer, 1080x1920
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0062, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_17;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 1080;
    vDecSample->DEFAULT_HEIGHT = 1920;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForAV1());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE180, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0063
 * @tc.name      : Decode av1 buffer with minimum valid resolution
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0063, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_18;
    vDecSample->outputYuvFlag = true;
    OH_AVRange widthRange;
    OH_AVRange heightRange;
    getVideoSizeRange(&widthRange, &heightRange);
    vDecSample->DEFAULT_WIDTH = widthRange.minVal;
    vDecSample->DEFAULT_HEIGHT = heightRange.minVal;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForAV1());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE180, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0064
 * @tc.name      : Decode av1 buffer with maximum valid resolution 8192x4352
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0064, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_19;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 8192;
    vDecSample->DEFAULT_HEIGHT = 4352;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForAV1());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE45, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0065
 * @tc.name      : decode Av1 buffer, I frame only
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0065, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_20;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 720;
    vDecSample->DEFAULT_HEIGHT = 480;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForAV1());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE45, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0066
 * @tc.name      : decode Av1 buffer, IP frame only
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0066, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_21;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 720;
    vDecSample->DEFAULT_HEIGHT = 480;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForAV1());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE180, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0067
 * @tc.name      : decode Av1 buffer, 1279x719
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0067, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_22;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 1279;
    vDecSample->DEFAULT_HEIGHT = 719;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForAV1());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE180, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0068
 * @tc.name      : decode Av1 buffer, high profile L0 426x240
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0068, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_23;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 426;
    vDecSample->DEFAULT_HEIGHT = 240;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForAV1());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE45, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0069
 * @tc.name      : decode Av1 buffer, high profile L1 640x360
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0069, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_24;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 640;
    vDecSample->DEFAULT_HEIGHT = 360;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForAV1());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE45, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0070
 * @tc.name      : decode Av1 buffer, high profile L4 854x480
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0070, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_25;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 854;
    vDecSample->DEFAULT_HEIGHT = 480;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForAV1());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE45, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0071
 * @tc.name      : decode Av1 buffer, high profile L5 1280x720
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0071, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_26;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 1280;
    vDecSample->DEFAULT_HEIGHT = 720;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForAV1());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE45, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0072
 * @tc.name      : decode Av1 buffer, high profile L8 1920x1080
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0072, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_27;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForAV1());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE45, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0073
 * @tc.name      : decode Av1 buffer, high profile L9 1920x1080 60fps
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0073, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_28;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    vDecSample->DEFAULT_FRAME_RATE = 60;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForAV1());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE90, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0074
 * @tc.name      : decode Av1 buffer, high profile L12 3840x2160
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0074, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_29;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 3840;
    vDecSample->DEFAULT_HEIGHT = 2160;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForAV1());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE45, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0075
 * @tc.name      : decode Av1 buffer, high profile L13 3840x2160 60fps
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0075, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_30;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 3840;
    vDecSample->DEFAULT_HEIGHT = 2160;
    vDecSample->DEFAULT_FRAME_RATE = 60;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForAV1());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE90, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0076
 * @tc.name      : decode Av1 buffer, high profile L14 3840x2160 120fps
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0076, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_31;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 3840;
    vDecSample->DEFAULT_HEIGHT = 2160;
    vDecSample->DEFAULT_FRAME_RATE = 120;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForAV1());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE180, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0077
 * @tc.name      : decode Av1 buffer, high profile L15 3840x2160 120fps
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0077, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_32;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 3840;
    vDecSample->DEFAULT_HEIGHT = 2160;
    vDecSample->DEFAULT_FRAME_RATE = 120;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForAV1());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE180, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0078
 * @tc.name      : decode Av1 buffer, high profile L16 7680x4320
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0078, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_33;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 7680;
    vDecSample->DEFAULT_HEIGHT = 4320;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForAV1());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE45, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0079
 * @tc.name      : decode Av1 buffer, high profile L17 7680x4320 60fps
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0079, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_34;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 7680;
    vDecSample->DEFAULT_HEIGHT = 4320;
    vDecSample->DEFAULT_FRAME_RATE = 60;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForAV1());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE60, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0080
 * @tc.name      : decode Av1 buffer, high profile L18 7680x4320 120fps
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0080, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_35;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 7680;
    vDecSample->DEFAULT_HEIGHT = 4320;
    vDecSample->DEFAULT_FRAME_RATE = 120;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForAV1());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE120, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0081
 * @tc.name      : decode Av1 buffer, high profile L19 7680x4320 120fps
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0081, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_36;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 7680;
    vDecSample->DEFAULT_HEIGHT = 4320;
    vDecSample->DEFAULT_FRAME_RATE = 120;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForAV1());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE120, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0082
 * @tc.name      : decode Av1 buffer from FLV file
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0082, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    const char *file = "/data/test/media/av1_flv.flv";
    
    vDecSample->getFormat(file);
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE180, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0083
 * @tc.name      : decode Av1 buffer from WEBM file
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0083, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    const char *file = "/data/test/media/av1_webm.webm";
    
    vDecSample->getFormat(file);
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE180, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0084
 * @tc.name      : decode Av1 buffer, Professional profile L0 426x240
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0084, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_37;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 426;
    vDecSample->DEFAULT_HEIGHT = 240;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForAV1());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE45, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0085
 * @tc.name      : decode Av1 buffer, Professional profile L1 640x360
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0085, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_38;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 640;
    vDecSample->DEFAULT_HEIGHT = 360;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForAV1());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE45, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0086
 * @tc.name      : decode Av1 buffer, Professional profile L4 854x480
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0086, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_39;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 854;
    vDecSample->DEFAULT_HEIGHT = 480;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForAV1());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE45, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0087
 * @tc.name      : decode Av1 buffer, Professional profile L5 1280x720
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0087, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_40;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 1280;
    vDecSample->DEFAULT_HEIGHT = 720;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForAV1());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE45, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0088
 * @tc.name      : decode Av1 buffer, Professional profile L8 1920x1080
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0088, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_41;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForAV1());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE45, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0089
 * @tc.name      : decode Av1 buffer, Professional profile L9 1920x1080 60fps
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0089, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_42;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    vDecSample->DEFAULT_FRAME_RATE = 60;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForAV1());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE90, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0090
 * @tc.name      : decode Av1 buffer, Professional profile L12 3840x2160
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0090, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_43;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 3840;
    vDecSample->DEFAULT_HEIGHT = 2160;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForAV1());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE30, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0091
 * @tc.name      : decode Av1 buffer, Professional profile L13 3840x2160 60fps
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0091, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_44;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 3840;
    vDecSample->DEFAULT_HEIGHT = 2160;
    vDecSample->DEFAULT_FRAME_RATE = 60;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForAV1());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE60, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0092
 * @tc.name      : decode Av1 buffer, Professional profile L14 3840x2160 120fps
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0092, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_45;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 3840;
    vDecSample->DEFAULT_HEIGHT = 2160;
    vDecSample->DEFAULT_FRAME_RATE = 120;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForAV1());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE120, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0093
 * @tc.name      : decode Av1 buffer, Professional profile L15 3840x2160 120fps
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0093, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_46;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 3840;
    vDecSample->DEFAULT_HEIGHT = 2160;
    vDecSample->DEFAULT_FRAME_RATE = 120;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForAV1());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE120, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0094
 * @tc.name      : decode Av1 buffer, Professional profile L16 7680x4320
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0094, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_47;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 7680;
    vDecSample->DEFAULT_HEIGHT = 4320;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForAV1());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE30, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0095
 * @tc.name      : decode Av1 buffer, Professional profile L17 7680x4320 60fps
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0095, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_48;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 7680;
    vDecSample->DEFAULT_HEIGHT = 4320;
    vDecSample->DEFAULT_FRAME_RATE = 60;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForAV1());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE60, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0096
 * @tc.name      : decode Av1 buffer, Professional profile L18 7680x4320 120fps
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0096, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_49;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 7680;
    vDecSample->DEFAULT_HEIGHT = 4320;
    vDecSample->DEFAULT_FRAME_RATE = 120;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForAV1());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE120, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0097
 * @tc.name      : decode Av1 buffer, Professional profile L19 7680x4320 120fps
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0097, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_50;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 7680;
    vDecSample->DEFAULT_HEIGHT = 4320;
    vDecSample->DEFAULT_FRAME_RATE = 120;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForAV1());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE120, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0098
 * @tc.name      : decode Av1 buffer No PixelFormat
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0098, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_5;
    vDecSample->outputYuvFlag = true;
    vDecSample->needCheckHash = true;
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoderNoPixelFormat());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForAV1());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE180, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_DECODE_SYNC_AV1_FUNC_0001
 * @tc.name      : decode Av1 buffer synchronously with NV12 output
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_DECODE_SYNC_AV1_FUNC_0001, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_0;
    vDecSample->enbleSyncMode = 1;
    vDecSample->defaultPixelFormat = AV_PIXEL_FORMAT_NV12;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartSyncVideoDecoderForAv1());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE180, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_DECODE_SYNC_AV1_FUNC_0002
 * @tc.name      : decode Av1 buffer synchronously with NV21 output
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_DECODE_SYNC_AV1_FUNC_0002, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_0;
    vDecSample->enbleSyncMode = 1;
    vDecSample->defaultPixelFormat = AV_PIXEL_FORMAT_NV21;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartSyncVideoDecoderForAv1());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE180, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_DECODE_SYNC_AV1_FUNC_0003
 * @tc.name      : decode Av1 buffer synchronously with surface output
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_DECODE_SYNC_AV1_FUNC_0003, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_0;

    vDecSample->SF_OUTPUT = true;
    vDecSample->enbleSyncMode = 1;
    vDecSample->defaultPixelFormat = AV_PIXEL_FORMAT_NV12;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->DecodeSetSurface());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartSyncVideoDecoderForAv1());
    vDecSample->WaitForEOS();
    ASSERT_EQ(FRAMESIZE180, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_AV1DEC_FUNCTION_0100
 * @tc.name      : graph pixel format
 * @tc.desc      : function test
 */
HWTEST_F(Av1decFuncNdkTest, VIDEO_AV1DEC_FUNCTION_0100, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    int32_t pixfmt[4] = {24, 25, 35, 36};
    vDecSample->INP_DIR = INP_DIR_50;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 7680;
    vDecSample->DEFAULT_HEIGHT = 4320;
    vDecSample->DEFAULT_FRAME_RATE = 120;
    vDecSample->isGetVideoSupportedPixelFormats = true;
    vDecSample->isGetFormatKey = true;
    vDecSample->avcodecMimeType = OH_AVCODEC_MIMETYPE_VIDEO_AV1;
    vDecSample->isEncoder = false;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameAv1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForAV1());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(4, vDecSample->pixlFormatNum);
    for (int i = 0; i < vDecSample->pixlFormatNum; i++) {
        ASSERT_EQ(vDecSample->pixlFormats[i], pixfmt[i]);
    }
    ASSERT_EQ(FRAMESIZE120, vDecSample->outFrameCount);
}
} // namespace