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
#include "native_avformat.h"
#include "openssl/sha.h"

#ifdef SUPPORT_DRM
#include "native_mediakeysession.h"
#include "native_mediakeysystem.h"
#endif

#define MAX_THREAD 16

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;

namespace OHOS {
namespace Media {
class Mpeg2SwdecFuncNdkTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp() override;
    void TearDown() override;
    void InputFUNCTION();
    void OutputFUNCTION();
    void Release();
    int32_t Stop();

protected:
    const string CODEC_NAME = "avdec_mpeg2";
    const char *INP_DIR_1 = "/data/test/media/simple@low_level_352_288_30.m2v";
    const char *INP_DIR_2 = "/data/test/media/simple@main_level_640_480_30.m2v";
    const char *INP_DIR_3 = "/data/test/media/main@low_level_352_288_30.m2v";
    const char *INP_DIR_4 = "/data/test/media/main@main_level_640_480_30.m2v";
    const char *INP_DIR_5 = "/data/test/media/main@high14_level_1280_720_60.m2v";
    const char *INP_DIR_6 = "/data/test/media/main@high_level_1920_1080_60.m2v";
    const char *INP_DIR_7 = "/data/test/media/snr_scalable@low_level_352_288_30.m2v";
    const char *INP_DIR_8 = "/data/test/media/snr_scalable@main_level_640_480_30.m2v";
    const char *INP_DIR_9 = "/data/test/media/snr_scalable@high_level_1920_1080_60.m2v";
    const char *INP_DIR_10 = "/data/test/media/spatially_scalable@low_level_352_288_30.m2v";
    const char *INP_DIR_11 = "/data/test/media/spatially_scalable@main_level_640_480_30.m2v";
    const char *INP_DIR_12 = "/data/test/media/spatially_scalable@high14_level_1280_720_60.m2v";
    const char *INP_DIR_13 = "/data/test/media/spatially_scalable@high_level_1920_1080_60.m2v";
    const char *INP_DIR_14 = "/data/test/media/high@low_level_352_288_30.m2v";
    const char *INP_DIR_15 = "/data/test/media/high@main_level_640_480_30.m2v";
    const char *INP_DIR_16 = "/data/test/media/high@high14_level_1280_720_60.m2v";
    const char *INP_DIR_17 = "/data/test/media/high@high_level_1920_1080_60.m2v";
    const char *INP_DIR_18 = "/data/test/media/422P@main_level_640_480_30.m2v";
    const char *INP_DIR_19 = "/data/test/media/422P@high14_level_1280_720_60.m2v";
    const char *INP_DIR_20 = "/data/test/media/422P@high_level_1920_1080_60.m2v";
};
} // namespace Media
} // namespace OHOS

namespace {
static OH_AVCapability *cap_mpeg2 = nullptr;
static string g_codecNameMpeg2 = "";
} // namespace

void Mpeg2SwdecFuncNdkTest::SetUpTestCase()
{
    cap_mpeg2 = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_MPEG2, false, SOFTWARE);
    g_codecNameMpeg2 = OH_AVCapability_GetName(cap_mpeg2);
    cout << "g_codecNameMpeg2: " << g_codecNameMpeg2 << endl;
}
void Mpeg2SwdecFuncNdkTest::TearDownTestCase() {}
void Mpeg2SwdecFuncNdkTest::SetUp() {}
void Mpeg2SwdecFuncNdkTest::TearDown() {}

struct DecInfo {
    const char *inpDir;
    uint32_t defaultWidth;
    uint32_t defaultHeight;
    uint32_t defaultFrameRate;
};

static void RunDec(DecInfo decinfo)
{
    auto vDecSample = make_shared<VDecNdkSample>();
    vDecSample->INP_DIR = decinfo.inpDir;
    vDecSample->DEFAULT_WIDTH = decinfo.defaultWidth;
    vDecSample->DEFAULT_HEIGHT = decinfo.defaultHeight;
    vDecSample->DEFAULT_FRAME_RATE = decinfo.defaultFrameRate;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameMpeg2));
    vDecSample->WaitForEOS();
}

namespace {
/**
 * @tc.number    : VIDEO_MPEG2SWDEC_FUNCTION_0100
 * @tc.name      : decode mpeg2 stream in buffer mode ,output pixel format is NV12
 * @tc.desc      : FUNCTION test
 */
HWTEST_F(Mpeg2SwdecFuncNdkTest, VIDEO_MPEG2SWDEC_FUNCTION_0100, TestSize.Level2)
{
    if (cap_mpeg2 != nullptr) {
        auto vDecSample = make_shared<VDecNdkSample>();
        vDecSample->INP_DIR = INP_DIR_2;
        vDecSample->DEFAULT_WIDTH = 640;
        vDecSample->DEFAULT_HEIGHT = 480;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->DEFAULT_PIXEL_FORMAT = AV_PIXEL_FORMAT_NV12;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameMpeg2));
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_MPEG2SWDEC_FUNCTION_0200
 * @tc.name      : decode mpeg2 stream in buffer mode ,output pixel format is NV21
 * @tc.desc      : FUNCTION test
 */
HWTEST_F(Mpeg2SwdecFuncNdkTest, VIDEO_MPEG2SWDEC_FUNCTION_0200, TestSize.Level2)
{
    if (cap_mpeg2 != nullptr) {
        auto vDecSample = make_shared<VDecNdkSample>();
        vDecSample->INP_DIR = INP_DIR_2;
        vDecSample->DEFAULT_WIDTH = 640;
        vDecSample->DEFAULT_HEIGHT = 480;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->DEFAULT_PIXEL_FORMAT = AV_PIXEL_FORMAT_NV21;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameMpeg2));
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_MPEG2SWDEC_FUNCTION_0300
 * @tc.name      : decode mpeg2 stream in buffer mode ,output pixel format is YUV420
 * @tc.desc      : FUNCTION test
 */
HWTEST_F(Mpeg2SwdecFuncNdkTest, VIDEO_MPEG2SWDEC_FUNCTION_0300, TestSize.Level2)
{
    if (cap_mpeg2 != nullptr) {
        auto vDecSample = make_shared<VDecNdkSample>();
        vDecSample->INP_DIR = INP_DIR_2;
        vDecSample->DEFAULT_WIDTH = 640;
        vDecSample->DEFAULT_HEIGHT = 480;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->DEFAULT_PIXEL_FORMAT = AV_PIXEL_FORMAT_YUVI420;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameMpeg2));
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_MPEG2SWDEC_FUNCTION_0400
 * @tc.name      : decode mpeg2 stream in buffer mode ,output pixel format is RGBa
 * @tc.desc      : FUNCTION test
 */
HWTEST_F(Mpeg2SwdecFuncNdkTest, VIDEO_MPEG2SWDEC_FUNCTION_0400, TestSize.Level2)
{
    if (cap_mpeg2 != nullptr) {
        auto vDecSample = make_shared<VDecNdkSample>();
        vDecSample->INP_DIR = INP_DIR_2;
        vDecSample->DEFAULT_WIDTH = 640;
        vDecSample->DEFAULT_HEIGHT = 480;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->checkOutPut = false;
        vDecSample->DEFAULT_PIXEL_FORMAT = AV_PIXEL_FORMAT_RGBA;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameMpeg2));
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_MPEG2SWDEC_FUNCTION_0500
 * @tc.name      :  decode mpeg2 stream in surface mode ,output pixel format is NV12
 * @tc.desc      : FUNCTION test
 */
HWTEST_F(Mpeg2SwdecFuncNdkTest, VIDEO_MPEG2SWDEC_FUNCTION_0500, TestSize.Level2)
{
    if (cap_mpeg2 != nullptr) {
        auto vDecSample = make_shared<VDecNdkSample>();
        vDecSample->INP_DIR = INP_DIR_2;
        vDecSample->DEFAULT_WIDTH = 640;
        vDecSample->DEFAULT_HEIGHT = 480;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->DEFAULT_PIXEL_FORMAT = AV_PIXEL_FORMAT_NV12;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameMpeg2));
        vDecSample->WaitForEOS();
        bool isVaild = false;
        OH_VideoDecoder_IsValid(vDecSample->vdec_, &isVaild);
        ASSERT_EQ(false, isVaild);
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_MPEG2SWDEC_FUNCTION_0600
 * @tc.name      : decode mpeg2 stream in surface mode ,output pixel format is NV21
 * @tc.desc      : FUNCTION test
 */
HWTEST_F(Mpeg2SwdecFuncNdkTest, VIDEO_MPEG2SWDEC_FUNCTION_0600, TestSize.Level2)
{
    if (cap_mpeg2 != nullptr) {
        auto vDecSample = make_shared<VDecNdkSample>();
        vDecSample->INP_DIR = INP_DIR_2;
        vDecSample->DEFAULT_WIDTH = 640;
        vDecSample->DEFAULT_HEIGHT = 480;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->DEFAULT_PIXEL_FORMAT = AV_PIXEL_FORMAT_NV21;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameMpeg2));
        vDecSample->WaitForEOS();
        bool isVaild = false;
        OH_VideoDecoder_IsValid(vDecSample->vdec_, &isVaild);
        ASSERT_EQ(false, isVaild);
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_MPEG2SWDEC_FUNCTION_0700
 * @tc.name      : decode mpeg2 stream in surface mode ,output pixel format is YUV420
 * @tc.desc      : FUNCTION test
 */
HWTEST_F(Mpeg2SwdecFuncNdkTest, VIDEO_MPEG2SWDEC_FUNCTION_0700, TestSize.Level2)
{
    if (cap_mpeg2 != nullptr) {
        auto vDecSample = make_shared<VDecNdkSample>();
        vDecSample->INP_DIR = INP_DIR_2;
        vDecSample->DEFAULT_WIDTH = 640;
        vDecSample->DEFAULT_HEIGHT = 480;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->DEFAULT_PIXEL_FORMAT = AV_PIXEL_FORMAT_YUVI420;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameMpeg2));
        vDecSample->WaitForEOS();
        bool isVaild = false;
        OH_VideoDecoder_IsValid(vDecSample->vdec_, &isVaild);
        ASSERT_EQ(false, isVaild);
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_MPEG2SWDEC_FUNCTION_0800
 * @tc.name      : decode mpeg2 stream in surface mode ,output pixel format is RGBa
 * @tc.desc      : FUNCTION test
 */
HWTEST_F(Mpeg2SwdecFuncNdkTest, VIDEO_MPEG2SWDEC_FUNCTION_0800, TestSize.Level2)
{
    if (cap_mpeg2 != nullptr) {
        auto vDecSample = make_shared<VDecNdkSample>();
        vDecSample->INP_DIR = INP_DIR_2;
        vDecSample->DEFAULT_WIDTH = 640;
        vDecSample->DEFAULT_HEIGHT = 480;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->DEFAULT_PIXEL_FORMAT = AV_PIXEL_FORMAT_RGBA;
        vDecSample->checkOutPut = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameMpeg2));
        vDecSample->WaitForEOS();
        bool isVaild = false;
        OH_VideoDecoder_IsValid(vDecSample->vdec_, &isVaild);
        ASSERT_EQ(false, isVaild);
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_MPEG2SWDEC_FUNCTION_0900
 * @tc.name      : decode mpeg2 stream ,under buffer mode
 * @tc.desc      : FUNCTION test
 */
HWTEST_F(Mpeg2SwdecFuncNdkTest, VIDEO_MPEG2SWDEC_FUNCTION_0900, TestSize.Level2)
{
    if (cap_mpeg2 != nullptr) {
        DecInfo fileTest1{INP_DIR_1, 352, 288, 30};
        RunDec(fileTest1);
        DecInfo fileTest2{INP_DIR_2, 640, 480, 30};
        RunDec(fileTest2);
        DecInfo fileTest3{INP_DIR_3, 352, 288, 30};
        RunDec(fileTest3);
        DecInfo fileTest4{INP_DIR_4, 640, 480, 30};
        RunDec(fileTest4);
        DecInfo fileTest5{INP_DIR_5, 1280, 720, 60};
        RunDec(fileTest5);
        DecInfo fileTest6{INP_DIR_6, 1920, 1080, 60};
        RunDec(fileTest6);
        DecInfo fileTest7{INP_DIR_7, 352, 288, 30};
        RunDec(fileTest7);
        DecInfo fileTest8{INP_DIR_8, 640, 480, 30};
        RunDec(fileTest8);
        DecInfo fileTest9{INP_DIR_9, 1920, 1080, 60};
        RunDec(fileTest9);
        DecInfo fileTest10{INP_DIR_10, 352, 288, 30};
        RunDec(fileTest10);
        DecInfo fileTest11{INP_DIR_11, 640, 480, 30};
        RunDec(fileTest11);
        DecInfo fileTest12{INP_DIR_12, 1280, 720, 60};
        RunDec(fileTest12);
        DecInfo fileTest13{INP_DIR_13, 1920, 1080, 60};
        RunDec(fileTest13);
        DecInfo fileTest14{INP_DIR_14, 352, 288, 30};
        RunDec(fileTest14);
        DecInfo fileTest15{INP_DIR_15, 640, 480, 30};
        RunDec(fileTest15);
        DecInfo fileTest16{INP_DIR_16, 1280, 720, 60};
        RunDec(fileTest16);
        DecInfo fileTest17{INP_DIR_17, 1920, 1080, 60};
        RunDec(fileTest17);
        DecInfo fileTest18{INP_DIR_18, 640, 480, 30};
        RunDec(fileTest18);
        DecInfo fileTest19{INP_DIR_19, 1280, 720, 60};
        RunDec(fileTest19);
        DecInfo fileTest20{INP_DIR_20, 1920, 1080, 60};
        RunDec(fileTest20);
    }
}

/**
 * @tc.number    : VIDEO_MPEG2SWDEC_FUNCTION_1000
 * @tc.name      : odd resolution of 641 * 481
 * @tc.desc      : FUNCTION test
 */
HWTEST_F(Mpeg2SwdecFuncNdkTest, VIDEO_MPEG2SWDEC_FUNCTION_1000, TestSize.Level2)
{
    if (cap_mpeg2 != nullptr) {
        auto vDecSample = make_shared<VDecNdkSample>();
        vDecSample->INP_DIR = "/data/test/media/simple@main_level_641_481_30.m2v";
        vDecSample->DEFAULT_WIDTH = 641;
        vDecSample->DEFAULT_HEIGHT = 481;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SURFACE_OUTPUT = false;
        vDecSample->checkOutPut = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMpeg2));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_MPEG2SWDEC_FUNCTION_1100
 * @tc.name      : odd resolution of 1281 * 721
 * @tc.desc      : FUNCTION test
 */
HWTEST_F(Mpeg2SwdecFuncNdkTest, VIDEO_MPEG2SWDEC_FUNCTION_1100, TestSize.Level2)
{
    if (cap_mpeg2 != nullptr) {
        auto vDecSample = make_shared<VDecNdkSample>();
        vDecSample->INP_DIR = "/data/test/media/main@high14_level_1281_721_60.m2v";
        vDecSample->DEFAULT_WIDTH = 1281;
        vDecSample->DEFAULT_HEIGHT = 721;
        vDecSample->DEFAULT_FRAME_RATE = 60;
        vDecSample->SURFACE_OUTPUT = false;
        vDecSample->checkOutPut = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMpeg2));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_MPEG2SWDEC_FUNCTION_1200
 * @tc.name      : mpeg2 decode res change video
 * @tc.desc      : FUNCTION test
 */
HWTEST_F(Mpeg2SwdecFuncNdkTest, VIDEO_MPEG2SWDEC_FUNCTION_1200, TestSize.Level2)
{
    if (cap_mpeg2 != nullptr) {
        auto vDecSample = make_shared<VDecNdkSample>();
        vDecSample->INP_DIR = "/data/test/media/mpeg2_res_change.m2v";
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SURFACE_OUTPUT = false;
        vDecSample->checkOutPut = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMpeg2));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_MPEG2SWDEC_FUNCTION_1300
 * @tc.name      : rotation is 90
 * @tc.desc      : FUNCTION test
 */
HWTEST_F(Mpeg2SwdecFuncNdkTest, VIDEO_MPEG2SWDEC_FUNCTION_1300, TestSize.Level2)
{
    if (cap_mpeg2 != nullptr) {
        auto vDecSample = make_shared<VDecNdkSample>();
        vDecSample->INP_DIR = "/data/test/media/simple@low_level_352_288_30_rotation_90.m2v";
        vDecSample->DEFAULT_WIDTH = 352;
        vDecSample->DEFAULT_HEIGHT = 288;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->DEFAULT_ROTATION = 90;
        vDecSample->SURFACE_OUTPUT = false;
        vDecSample->checkOutPut = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMpeg2));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_MPEG2SWDEC_FUNCTION_1400
 * @tc.name      : rotation is 180
 * @tc.desc      : FUNCTION test
 */
HWTEST_F(Mpeg2SwdecFuncNdkTest, VIDEO_MPEG2SWDEC_FUNCTION_1400, TestSize.Level2)
{
    if (cap_mpeg2 != nullptr) {
        auto vDecSample = make_shared<VDecNdkSample>();
        vDecSample->INP_DIR = "/data/test/media/simple@low_level_352_288_30_rotation_180.m2v";
        vDecSample->DEFAULT_WIDTH = 352;
        vDecSample->DEFAULT_HEIGHT = 288;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->DEFAULT_ROTATION = 180;
        vDecSample->SURFACE_OUTPUT = false;
        vDecSample->checkOutPut = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMpeg2));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_MPEG2SWDEC_FUNCTION_1500
 * @tc.name      : rotation is 270
 * @tc.desc      : FUNCTION test
 */
HWTEST_F(Mpeg2SwdecFuncNdkTest, VIDEO_MPEG2SWDEC_FUNCTION_1500, TestSize.Level2)
{
    if (cap_mpeg2 != nullptr) {
        auto vDecSample = make_shared<VDecNdkSample>();
        vDecSample->INP_DIR = "/data/test/media/simple@low_level_352_288_30_rotation_270.m2v";
        vDecSample->DEFAULT_WIDTH = 352;
        vDecSample->DEFAULT_HEIGHT = 288;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->DEFAULT_ROTATION = 270;
        vDecSample->SURFACE_OUTPUT = false;
        vDecSample->checkOutPut = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMpeg2));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_MPEG2SWDEC_FUNCTION_1600
 * @tc.name      : unaligned byte test
 * @tc.desc      : FUNCTION test
 */
HWTEST_F(Mpeg2SwdecFuncNdkTest, VIDEO_MPEG2SWDEC_FUNCTION_1600, TestSize.Level2)
{
    if (cap_mpeg2 != nullptr) {
        auto vDecSample = make_shared<VDecNdkSample>();
        vDecSample->INP_DIR = "/data/test/media/simple@low_level_353_289_30.m2v";
        vDecSample->DEFAULT_WIDTH = 353;
        vDecSample->DEFAULT_HEIGHT = 289;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SURFACE_OUTPUT = false;
        vDecSample->checkOutPut = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMpeg2));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_MPEG2SWDEC_FUNCTION_1700
 * @tc.name      : mpeg2 decode err resolution
 * @tc.desc      : FUNCTION test
 */
HWTEST_F(Mpeg2SwdecFuncNdkTest, VIDEO_MPEG2SWDEC_FUNCTION_1700, TestSize.Level2)
{
    if (cap_mpeg2 != nullptr) {
        auto vDecSample = make_shared<VDecNdkSample>();
        vDecSample->INP_DIR = "/data/test/media/mpeg2_err_res.m2v";
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SURFACE_OUTPUT = false;
        vDecSample->checkOutPut = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMpeg2));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_MPEG2VIDEO_SURF_CHANGE_0100
 * @tc.name      : 1080p ,surf model change in normal state
 * @tc.desc      : FUNCTION test
 */
HWTEST_F(Mpeg2SwdecFuncNdkTest, VIDEO_MPEG2VIDEO_SURF_CHANGE_0100, TestSize.Level2)
{
    if (cap_mpeg2 != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_6;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 60;
        vDecSample->SURFACE_OUTPUT = true;
        vDecSample->autoSwitchSurface = true;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameMpeg2));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->Reset());
        ASSERT_EQ(AV_ERR_INVALID_STATE, vDecSample->SwitchSurface());
        ASSERT_EQ(AV_ERR_OK, vDecSample->Release());
    }
}

/**
 * @tc.number    : VIDEO_MPEG2VIDEO_SURF_CHANGE_0200
 * @tc.name      : 1080p ,surf model change in flushed state
 * @tc.desc      : FUNCTION test
 */
HWTEST_F(Mpeg2SwdecFuncNdkTest, VIDEO_MPEG2VIDEO_SURF_CHANGE_0200, TestSize.Level2)
{
    if (cap_mpeg2 != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_6;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 60;
        vDecSample->SURFACE_OUTPUT = true;
        vDecSample->autoSwitchSurface = true;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameMpeg2));
        ASSERT_EQ(AV_ERR_OK, vDecSample->Flush());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
        ASSERT_EQ(AV_ERR_OK, vDecSample->Stop());
    }
}

/**
 * @tc.number    : VIDEO_MPEG2VIDEO_SURF_CHANGE_0300
 * @tc.name      : 1080p ,surf model change in runing state
 * @tc.desc      : FUNCTION test
 */
HWTEST_F(Mpeg2SwdecFuncNdkTest, VIDEO_MPEG2VIDEO_SURF_CHANGE_0300, TestSize.Level2)
{
    if (cap_mpeg2 != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_6;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 60;
        vDecSample->SURFACE_OUTPUT = false;
        vDecSample->autoSwitchSurface = false;
        vDecSample->CreateSurface();
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameMpeg2));
        ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, vDecSample->SwitchSurface());
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_MPEG2VIDEO_SURF_CHANGE_0400
 * @tc.name      : 1080p ,repeat call setSurface fastly
 * @tc.desc      : FUNCTION test
 */
HWTEST_F(Mpeg2SwdecFuncNdkTest, VIDEO_MPEG2VIDEO_SURF_CHANGE_0400, TestSize.Level2)
{
    if (cap_mpeg2 != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_6;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 60;
        vDecSample->SURFACE_OUTPUT = true;
        vDecSample->autoSwitchSurface = true;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameMpeg2));
        ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
        vDecSample->WaitForEOS();
    }
}

/**
 * @tc.number    : VIDEO_MPEG2VIDEO_SURF_CHANGE_0500
 * @tc.name      : 1080p ,surf model change in flush to runnig state
 * @tc.desc      : FUNCTION test
 */
HWTEST_F(Mpeg2SwdecFuncNdkTest, VIDEO_MPEG2VIDEO_SURF_CHANGE_0500, TestSize.Level2)
{
    if (cap_mpeg2 != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_6;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 60;
        vDecSample->SURFACE_OUTPUT = true;
        vDecSample->autoSwitchSurface = true;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameMpeg2));
        ASSERT_EQ(AV_ERR_OK, vDecSample->Flush());
        ASSERT_EQ(AV_ERR_OK, vDecSample->Start());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
    }
}

/**
 * @tc.number    : VIDEO_MPEG2VIDEO_SURF_CHANGE_0600
 * @tc.name      : 1080p ,surf model change in decoder finish to End-of-Stream state
 * @tc.desc      : FUNCTION test
 */
HWTEST_F(Mpeg2SwdecFuncNdkTest, VIDEO_MPEG2VIDEO_SURF_CHANGE_0600, TestSize.Level2)
{
    if (cap_mpeg2 != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_6;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 60;
        vDecSample->autoSwitchSurface = true;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameMpeg2));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
    }
}

/**
 * @tc.number    : VIDEO_MPEG2VIDEO_SURF_CHANGE_0700
 * @tc.name      : 1080p ,surf model change in config state
 * @tc.desc      : FUNCTION test
 */
HWTEST_F(Mpeg2SwdecFuncNdkTest, VIDEO_MPEG2VIDEO_SURF_CHANGE_0700, TestSize.Level2)
{
    if (cap_mpeg2 != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_6;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 60;
        vDecSample->SURFACE_OUTPUT = false;
        vDecSample->autoSwitchSurface = false;
        vDecSample->CreateSurface();
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMpeg2));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_MPEG2VIDEO_SURF_CHANGE_0800
 * @tc.name      : Two object repeat call setSurface fastly
 * @tc.desc      : FUNCTION test
 */
HWTEST_F(Mpeg2SwdecFuncNdkTest, VIDEO_MPEG2VIDEO_SURF_CHANGE_0800, TestSize.Level2)
{
    if (cap_mpeg2 != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_6;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 60;
        vDecSample->autoSwitchSurface = true;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameMpeg2));
        ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
        vDecSample->WaitForEOS();
        auto vDecSample_1 = make_shared<VDecAPI11Sample>();
        vDecSample_1->INP_DIR = INP_DIR_6;
        vDecSample_1->DEFAULT_WIDTH = 1920;
        vDecSample_1->DEFAULT_HEIGHT = 1080;
        vDecSample_1->DEFAULT_FRAME_RATE = 60;
        vDecSample_1->autoSwitchSurface = true;
        vDecSample_1->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample_1->RunVideoDec_Surface(g_codecNameMpeg2));
        ASSERT_EQ(AV_ERR_OK, vDecSample_1->SwitchSurface());
        vDecSample_1->WaitForEOS();
    }
}

/**
 * @tc.number    : VIDEO_MPEG2VIDEO_SURF_CHANGE_0900
 * @tc.name      : repeat call setSurface fastly 2 time
 * @tc.desc      : FUNCTION test
 */
HWTEST_F(Mpeg2SwdecFuncNdkTest, VIDEO_MPEG2VIDEO_SURF_CHANGE_0900, TestSize.Level2)
{
    if (cap_mpeg2 != nullptr) {
        for (int i = 0; i < 2; i++) {
            auto vDecSample = make_shared<VDecAPI11Sample>();
            vDecSample->INP_DIR = INP_DIR_6;
            vDecSample->DEFAULT_WIDTH = 1920;
            vDecSample->DEFAULT_HEIGHT = 1080;
            vDecSample->DEFAULT_FRAME_RATE = 60;
            vDecSample->autoSwitchSurface = true;
            vDecSample->sleepOnFPS = true;
            ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameMpeg2));
            ASSERT_EQ(AV_ERR_OK, vDecSample->RepeatCallSetSurface());
            vDecSample->WaitForEOS();
        }
    }
}

/**
 * @tc.number    : VIDEO_MPEG2VIDEO_PARA_0100
 * @tc.name      : width is -1 ,height is -1
 * @tc.desc      : FUNCTION test
 */
HWTEST_F(Mpeg2SwdecFuncNdkTest, VIDEO_MPEG2VIDEO_PARA_0100, TestSize.Level2)
{
    if (cap_mpeg2 != nullptr) {
        auto vDecSample = make_shared<VDecNdkSample>();
        vDecSample->INP_DIR = INP_DIR_2;
        vDecSample->DEFAULT_WIDTH = -1;
        vDecSample->DEFAULT_HEIGHT = -1;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMpeg2));
        ASSERT_NE(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_MPEG2VIDEO_PARA_0200
 * @tc.name      : width is 0 ,height is 0
 * @tc.desc      : FUNCTION test
 */
HWTEST_F(Mpeg2SwdecFuncNdkTest, VIDEO_MPEG2VIDEO_PARA_0200, TestSize.Level2)
{
    if (cap_mpeg2 != nullptr) {
        auto vDecSample = make_shared<VDecNdkSample>();
        vDecSample->INP_DIR = INP_DIR_2;
        vDecSample->DEFAULT_WIDTH = 0;
        vDecSample->DEFAULT_HEIGHT = 0;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMpeg2));
        ASSERT_NE(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_MPEG2VIDEO_PARA_0300
 * @tc.name      : width is 1 ,height is 1
 * @tc.desc      : FUNCTION test
 */
HWTEST_F(Mpeg2SwdecFuncNdkTest, VIDEO_MPEG2VIDEO_PARA_0300, TestSize.Level2)
{
    if (cap_mpeg2 != nullptr) {
        auto vDecSample = make_shared<VDecNdkSample>();
        vDecSample->INP_DIR = INP_DIR_2;
        vDecSample->DEFAULT_WIDTH = 1;
        vDecSample->DEFAULT_HEIGHT = 1;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMpeg2));
        ASSERT_NE(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_MPEG2VIDEO_PARA_0400
 * @tc.name      : width is 1920 ,height is 1080
 * @tc.desc      : FUNCTION test
 */
HWTEST_F(Mpeg2SwdecFuncNdkTest, VIDEO_MPEG2VIDEO_PARA_0400, TestSize.Level2)
{
    if (cap_mpeg2 != nullptr) {
        auto vDecSample = make_shared<VDecNdkSample>();
        vDecSample->INP_DIR = INP_DIR_2;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMpeg2));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_MPEG2VIDEO_PARA_0500
 * @tc.name      : width is 4097 ,height is 4097
 * @tc.desc      : FUNCTION test
 */
HWTEST_F(Mpeg2SwdecFuncNdkTest, VIDEO_MPEG2VIDEO_PARA_0500, TestSize.Level2)
{
    if (cap_mpeg2 != nullptr) {
        auto vDecSample = make_shared<VDecNdkSample>();
        vDecSample->INP_DIR = INP_DIR_2;
        vDecSample->DEFAULT_WIDTH = 4097;
        vDecSample->DEFAULT_HEIGHT = 4097;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMpeg2));
        ASSERT_NE(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_MPEG2VIDEO_PARA_0600
 * @tc.name      : width is 10000 ,height is 10000
 * @tc.desc      : FUNCTION test
 */
HWTEST_F(Mpeg2SwdecFuncNdkTest, VIDEO_MPEG2VIDEO_PARA_0600, TestSize.Level2)
{
    if (cap_mpeg2 != nullptr) {
        auto vDecSample = make_shared<VDecNdkSample>();
        vDecSample->INP_DIR = INP_DIR_2;
        vDecSample->DEFAULT_WIDTH = 10000;
        vDecSample->DEFAULT_HEIGHT = 10000;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMpeg2));
        ASSERT_NE(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_MPEG2VIDEO_PARA_0700
 * @tc.name      : framerate is -1
 * @tc.desc      : FUNCTION test
 */
HWTEST_F(Mpeg2SwdecFuncNdkTest, VIDEO_MPEG2VIDEO_PARA_0700, TestSize.Level2)
{
    if (cap_mpeg2 != nullptr) {
        auto vDecSample = make_shared<VDecNdkSample>();
        vDecSample->INP_DIR = INP_DIR_2;
        vDecSample->DEFAULT_WIDTH = 640;
        vDecSample->DEFAULT_HEIGHT = 480;
        vDecSample->DEFAULT_FRAME_RATE = -1;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMpeg2));
        ASSERT_NE(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_MPEG2VIDEO_PARA_0800
 * @tc.name      : framerate is 0
 * @tc.desc      : FUNCTION test
 */
HWTEST_F(Mpeg2SwdecFuncNdkTest, VIDEO_MPEG2VIDEO_PARA_0800, TestSize.Level2)
{
    if (cap_mpeg2 != nullptr) {
        auto vDecSample = make_shared<VDecNdkSample>();
        vDecSample->INP_DIR = INP_DIR_2;
        vDecSample->DEFAULT_WIDTH = 640;
        vDecSample->DEFAULT_HEIGHT = 480;
        vDecSample->DEFAULT_FRAME_RATE = 0;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMpeg2));
        ASSERT_NE(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_MPEG2VIDEO_PARA_0900
 * @tc.name      : framerate is 10000
 * @tc.desc      : FUNCTION test
 */
HWTEST_F(Mpeg2SwdecFuncNdkTest, VIDEO_MPEG2VIDEO_PARA_0900, TestSize.Level2)
{
    if (cap_mpeg2 != nullptr) {
        auto vDecSample = make_shared<VDecNdkSample>();
        vDecSample->INP_DIR = INP_DIR_2;
        vDecSample->DEFAULT_WIDTH = 640;
        vDecSample->DEFAULT_HEIGHT = 480;
        vDecSample->DEFAULT_FRAME_RATE = 10000;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMpeg2));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_MPEG2VIDEO_PARA_1000
 * @tc.name      : MPEG2 decoder, MPEG4 input
 * @tc.desc      : FUNCTION test
 */
HWTEST_F(Mpeg2SwdecFuncNdkTest, VIDEO_MPEG2VIDEO_PARA_1000, TestSize.Level2)
{
    if (cap_mpeg2 != nullptr) {
        auto vDecSample = make_shared<VDecNdkSample>();
        vDecSample->INP_DIR = "/data/test/media/mpeg4.m4v";
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMpeg2));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_SYNC_SWMPEG2_FUNC_0010
 * @tc.name      : mpeg2同步软解输出nv12
 * @tc.desc      : function test
 */
HWTEST_F(Mpeg2SwdecFuncNdkTest, VIDEO_DECODE_SYNC_SWMPEG2_FUNC_0010, TestSize.Level1)
{
    if (cap_mpeg2 != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_6;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->enbleSyncMode = 1;
        vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMpeg2));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartSyncVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_SYNC_SWMPEG2_FUNC_0020
 * @tc.name      : mpeg2同步软解输出nv21
 * @tc.desc      : function test
 */
HWTEST_F(Mpeg2SwdecFuncNdkTest, VIDEO_DECODE_SYNC_SWMPEG2_FUNC_0020, TestSize.Level0)
{
    if (cap_mpeg2 != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_6;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->enbleSyncMode = 1;
        vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV21;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMpeg2));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartSyncVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_SYNC_SWMPEG2_FUNC_0030
 * @tc.name      : mpeg2同步软解输出surface
 * @tc.desc      : function test
 */
HWTEST_F(Mpeg2SwdecFuncNdkTest, VIDEO_DECODE_SYNC_SWMPEG2_FUNC_0030, TestSize.Level1)
{
    if (cap_mpeg2 != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_6;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SURFACE_OUTPUT = true;
        vDecSample->enbleSyncMode = 1;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMpeg2));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->DecodeSetSurface());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartSyncVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_SYNC_SWMPEG2_FUNC_0040
 * @tc.name      : 264同步软解输出rgba
 * @tc.desc      : function test
 */
HWTEST_F(Mpeg2SwdecFuncNdkTest, VIDEO_DECODE_SYNC_SWMPEG2_FUNC_0040, TestSize.Level1)
{
    if (cap_mpeg2 != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_6;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->enbleSyncMode = 1;
        vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_RGBA;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMpeg2));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartSyncVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_BLANK_FRAME_0010
 * @tc.name      : config OH_MD_KEY_VIDEO_DECODER_BLANK_FRAME_ON_SHUTDOWN, decoder mpeg2
 * @tc.desc      : function test
 */
HWTEST_F(Mpeg2SwdecFuncNdkTest, VIDEO_DECODE_BLANK_FRAME_0010, TestSize.Level2)
{
    if (cap_mpeg2 != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_6;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->enbleBlankFrame = 1;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameMpeg2));
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_BLANK_FRAME_0020
 * @tc.name      : config OH_MD_KEY_VIDEO_DECODER_BLANK_FRAME_ON_SHUTDOWN, decoder mpeg2，surface
 * @tc.desc      : function test
 */
HWTEST_F(Mpeg2SwdecFuncNdkTest, VIDEO_DECODE_BLANK_FRAME_0020, TestSize.Level2)
{
    if (cap_mpeg2 != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_6;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->enbleBlankFrame = 1;
        vDecSample->SURFACE_OUTPUT = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMpeg2));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->DecodeSetSurface());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_MPEG2_FLUSH_0010
 * @tc.name      : create-start-eos-flush-start-eos
 * @tc.desc      : function test
 */
HWTEST_F(Mpeg2SwdecFuncNdkTest, VIDEO_SWDEC_MPEG2_FLUSH_0010, TestSize.Level1)
{
    if (cap_mpeg2 != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_6;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameMpeg2));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
        ASSERT_EQ(AV_ERR_OK, vDecSample->Flush());
        vDecSample->FlushStatus();
        vDecSample->finishLastPush = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_MPEG2_FLUSH_0020
 * @tc.name      : create-start-eos-flush-start-eos,surface
 * @tc.desc      : function test
 */
HWTEST_F(Mpeg2SwdecFuncNdkTest, VIDEO_SWDEC_MPEG2_FLUSH_0020, TestSize.Level1)
{
    if (cap_mpeg2 != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_6;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameMpeg2));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
        ASSERT_EQ(AV_ERR_OK, vDecSample->Flush());
        vDecSample->FlushStatus();
        vDecSample->finishLastPush = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_PIXE_FORMAT_MPEG2_0010
 * @tc.name      : mpeg2 pixelformat query key
 * @tc.desc      : function test
 */
HWTEST_F(Mpeg2SwdecFuncNdkTest, VIDEO_SWDEC_PIXE_FORMAT_MPEG2_0010, TestSize.Level2)
{
    if (cap_mpeg2 != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_6;
        vDecSample->isGetVideoSupportedPixelFormats = true;
        vDecSample->isGetFormatKey = true;
        vDecSample->avcodecMimeType = OH_AVCODEC_MIMETYPE_VIDEO_MPEG2;
        vDecSample->isEncoder = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMpeg2));
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_LT(1, vDecSample->pixlFormatNum);
        ASSERT_EQ(24, vDecSample->firstCallBackKey);
        ASSERT_EQ(24, vDecSample->onStreamChangedKey);
    }
}
}