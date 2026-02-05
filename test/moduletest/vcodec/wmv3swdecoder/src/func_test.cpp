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

#include "avcodec_codec_name.h"
#include "gtest/gtest.h"
#include "native_avcapability.h"
#include "native_avcodec_base.h"
#include "native_avcodec_videodecoder.h"
#include "native_averrors.h"
#include "native_avformat.h"
#include "videodec_api11_sample.h"

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
class Wmv3SwdecFuncNdkTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp() override;
    void TearDown() override;
    void InputFunc();
    void OutputFunc();
    void Release();
    int32_t Stop();
};
}  // namespace Media
}  // namespace OHOS

namespace {
static OH_AVCapability *cap_wmv3 = nullptr;
static string g_codecNameWmv3 = "";
static OH_AVCapability *cap_264 = nullptr;
static string g_codecName264 = "";
constexpr uint32_t FRAMESIZE61 = 61;
constexpr uint32_t FRAMESIZE90 = 90;
constexpr uint32_t FRAMESIZE180 = 180;

}  // namespace

void Wmv3SwdecFuncNdkTest::SetUpTestCase()
{
    cap_wmv3 = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV3, false, SOFTWARE);
    g_codecNameWmv3 = OH_AVCapability_GetName(cap_wmv3);
    cout << "g_codecNameWmv3: " << g_codecNameWmv3 << endl;
    cap_264 = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_AVC, false, SOFTWARE);
    g_codecName264 = OH_AVCapability_GetName(cap_264);
    cout << "g_codecName264: " << g_codecName264 << endl;
}

void Wmv3SwdecFuncNdkTest::TearDownTestCase()
{
}
void Wmv3SwdecFuncNdkTest::SetUp()
{
}
void Wmv3SwdecFuncNdkTest::TearDown()
{
}

namespace {
/**
 * @tc.number    : VIDEO_WMV3SWDEC_FUNCTION_0010
 * @tc.name      : decode WMV3 buffer,PixelFormat,NV12
 * @tc.desc      : function test
 */
HWTEST_F(Wmv3SwdecFuncNdkTest, VIDEO_WMV3SWDEC_FUNCTION_0010, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = "/data/test/media/profile0_128x96_10.wmv3";
    vDecSample->DEFAULT_WIDTH = 128;
    vDecSample->DEFAULT_HEIGHT = 96;
    vDecSample->DEFAULT_FRAME_RATE = 10;
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.WMV3"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE90, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_WMV3SWDEC_FUNCTION_0020
 * @tc.name      : decode WMV3 buffer,PixelFormat,NV21
 * @tc.desc      : function test
 */
HWTEST_F(Wmv3SwdecFuncNdkTest, VIDEO_WMV3SWDEC_FUNCTION_0020, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = "/data/test/media/profile0_128x96_10.wmv3";
    vDecSample->DEFAULT_WIDTH = 128;
    vDecSample->DEFAULT_HEIGHT = 96;
    vDecSample->DEFAULT_FRAME_RATE = 10;
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV21;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.WMV3"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE90, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_WMV3SWDEC_FUNCTION_0030
 * @tc.name      : decode WMV3 buffer,PixelFormat,I420
 * @tc.desc      : function test
 */
HWTEST_F(Wmv3SwdecFuncNdkTest, VIDEO_WMV3SWDEC_FUNCTION_0030, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = "/data/test/media/profile0_128x96_10.wmv3";
    vDecSample->DEFAULT_WIDTH = 128;
    vDecSample->DEFAULT_HEIGHT = 96;
    vDecSample->DEFAULT_FRAME_RATE = 10;
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_YUVI420;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.WMV3"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE90, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_WMV3SWDEC_FUNCTION_0040
 * @tc.name      : decode WMV3 buffer,PixelFormat,RGBA
 * @tc.desc      : function test
 */
HWTEST_F(Wmv3SwdecFuncNdkTest, VIDEO_WMV3SWDEC_FUNCTION_0040, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = "/data/test/media/profile1_352_288_10.wmv3";
    vDecSample->DEFAULT_WIDTH = 352;
    vDecSample->DEFAULT_HEIGHT = 288;
    vDecSample->DEFAULT_FRAME_RATE = 10;
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_RGBA;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.WMV3"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE61, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_WMV3SWDEC_FUNCTION_0050
 * @tc.name      : decode WMV3 buffer,profile0_128x96_10
 * @tc.desc      : function test
 */
HWTEST_F(Wmv3SwdecFuncNdkTest, VIDEO_WMV3SWDEC_FUNCTION_0050, TestSize.Level1)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = "/data/test/media/profile0_128x96_10.wmv3";
    vDecSample->DEFAULT_WIDTH = 128;
    vDecSample->DEFAULT_HEIGHT = 96;
    vDecSample->DEFAULT_FRAME_RATE = 10;
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.WMV3"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE90, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_WMV3SWDEC_FUNCTION_0060
 * @tc.name      : decode WMV3 buffer,profile1_1920x1080_24
 * @tc.desc      : function test
 */
HWTEST_F(Wmv3SwdecFuncNdkTest, VIDEO_WMV3SWDEC_FUNCTION_0060, TestSize.Level1)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = "/data/test/media/profile1_1920x1080_24.wmv3";
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    vDecSample->DEFAULT_FRAME_RATE = 24;
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.WMV3"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE180, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_WMV3SWDEC_FUNCTION_0070
 * @tc.name      : decode WMV3 buffer,profile1_1440x1080_24
 * @tc.desc      : function test
 */
HWTEST_F(Wmv3SwdecFuncNdkTest, VIDEO_WMV3SWDEC_FUNCTION_0070, TestSize.Level1)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = "/data/test/media/profile1_1440x1080_24.wmv3";
    vDecSample->DEFAULT_WIDTH = 1440;
    vDecSample->DEFAULT_HEIGHT = 1080;
    vDecSample->DEFAULT_FRAME_RATE = 24;
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.WMV3"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE180, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_WMV3SWDEC_FUNCTION_0080
 * @tc.name      : decode WMV3 buffer,input h263 error
 * @tc.desc      : function test
 */
HWTEST_F(Wmv3SwdecFuncNdkTest, VIDEO_WMV3SWDEC_FUNCTION_0080, TestSize.Level3)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = "/data/test/media/profile2_1920x1080.h263";
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    vDecSample->DEFAULT_FRAME_RATE = 24;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.WMV3"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_UNKNOWN, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(false, vDecSample->isonError);
}

/**
 * @tc.number    : VIDEO_WMV3SWDEC_FUNCTION_0090
 * @tc.name      : decode WMV3 surface,profile1_1440x1080_24
 * @tc.desc      : function test
 */
HWTEST_F(Wmv3SwdecFuncNdkTest, VIDEO_WMV3SWDEC_FUNCTION_0090, TestSize.Level3)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = "/data/test/media/profile1_1440x1080_24.wmv3";
    vDecSample->DEFAULT_WIDTH = 1440;
    vDecSample->DEFAULT_HEIGHT = 1080;
    vDecSample->DEFAULT_FRAME_RATE = 24;
    vDecSample->SF_OUTPUT = true;
    vDecSample->outputYuvSurface = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameWmv3));
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE180, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_WMV3SWDEC_FUNCTION_0100
 * @tc.name      : decode WMV3 surface,profile0_128x96_10
 * @tc.desc      : function test
 */
HWTEST_F(Wmv3SwdecFuncNdkTest, VIDEO_WMV3SWDEC_FUNCTION_0100, TestSize.Level3)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = "/data/test/media/profile0_128x96_10.wmv3";
    vDecSample->DEFAULT_WIDTH = 128;
    vDecSample->DEFAULT_HEIGHT = 96;
    vDecSample->DEFAULT_FRAME_RATE = 10;
    vDecSample->SF_OUTPUT = true;
    vDecSample->outputYuvSurface = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV21;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameWmv3));
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE90, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_WMV3SWDEC_FUNCTION_0110
 * @tc.name      : decode WMV3 surface, main profile1_352_288_10
 * @tc.desc      : function test
 */
HWTEST_F(Wmv3SwdecFuncNdkTest, VIDEO_WMV3SWDEC_FUNCTION_0110, TestSize.Level3)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = "/data/test/media/profile1_352_288_10.wmv3";
    vDecSample->DEFAULT_WIDTH = 352;
    vDecSample->DEFAULT_HEIGHT = 288;
    vDecSample->DEFAULT_FRAME_RATE = 10;
    vDecSample->SF_OUTPUT = true;
    vDecSample->outputYuvSurface = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV21;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameWmv3));
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE61, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_DECODE_WMV3_RESOLUTION_0010
 * @tc.name      : wmv3变分辨率，buffer, nv12, profile1_1440x1080_24
 * @tc.desc      : function test
 */
HWTEST_F(Wmv3SwdecFuncNdkTest, VIDEO_DECODE_WMV3_RESOLUTION_0010, TestSize.Level3)
{
    if (g_codecNameWmv3.find("WMV3") != string::npos) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/profile1_1440x1080_24.wmv3";
        vDecSample->DEFAULT_WIDTH = 1440;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 24;
        vDecSample->SF_OUTPUT = false;
        vDecSample->outputYuvFlag = true;
        vDecSample->isWmv3Change = true;
        vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameWmv3));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
        ASSERT_EQ(FRAMESIZE180, vDecSample->outFrameCount);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_WMV3_RESOLUTION_0020
 * @tc.name      : wmv3变分辨率，buffer, nv21, profile1_1440x1080_24
 * @tc.desc      : function test
 */
HWTEST_F(Wmv3SwdecFuncNdkTest, VIDEO_DECODE_WMV3_RESOLUTION_0020, TestSize.Level3)
{
    if (g_codecNameWmv3.find("WMV3") != string::npos) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/profile1_1440x1080_24.wmv3";
        vDecSample->DEFAULT_WIDTH = 1440;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 24;
        vDecSample->SF_OUTPUT = false;
        vDecSample->outputYuvFlag = true;
        vDecSample->isWmv3Change = true;
        vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV21;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameWmv3));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
        ASSERT_EQ(FRAMESIZE180, vDecSample->outFrameCount);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_WMV3_RESOLUTION_0030
 * @tc.name      : wmv3变分辨率，buffer, nv12, profile1_1440x1080_24
 * @tc.desc      : function test
 */
HWTEST_F(Wmv3SwdecFuncNdkTest, VIDEO_DECODE_WMV3_RESOLUTION_0030, TestSize.Level3)
{
    if (g_codecNameWmv3.find("WMV3") != string::npos) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/profile1_1440x1080_24.wmv3";
        vDecSample->DEFAULT_WIDTH = 1440;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 24;
        vDecSample->SF_OUTPUT = false;
        vDecSample->outputYuvFlag = true;
        vDecSample->isWmv3Change = true;
        vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameWmv3));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
        ASSERT_EQ(FRAMESIZE180, vDecSample->outFrameCount);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_WMV3_RESOLUTION_0040
 * @tc.name      : wmv3变分辨率，surface, nv21, profile1_1440x1080_24
 * @tc.desc      : function test
 */
HWTEST_F(Wmv3SwdecFuncNdkTest, VIDEO_DECODE_WMV3_RESOLUTION_0040, TestSize.Level3)
{
    if (g_codecNameWmv3.find("WMV3") != string::npos) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/profile1_1440x1080_24.wmv3";
        vDecSample->DEFAULT_WIDTH = 1440;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 24;
        vDecSample->SF_OUTPUT = true;
        vDecSample->isWmv3Change = true;
        vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV21;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameWmv3));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
        ASSERT_EQ(FRAMESIZE180, vDecSample->outFrameCount);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_WMV3_RESOLUTION_0050
 * @tc.name      : wmv3变分辨率，surface, nv21, profile1_1440x1080_24
 * @tc.desc      : function test
 */
HWTEST_F(Wmv3SwdecFuncNdkTest, VIDEO_DECODE_WMV3_RESOLUTION_0050, TestSize.Level3)
{
    if (g_codecNameWmv3.find("WMV3") != string::npos) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/profile1_1440x1080_24.wmv3";
        vDecSample->DEFAULT_WIDTH = 1440;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 24;
        vDecSample->SF_OUTPUT = true;
        vDecSample->isWmv3Change = true;
        vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV21;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameWmv3));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
        ASSERT_EQ(FRAMESIZE180, vDecSample->outFrameCount);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_WMV3_RESOLUTION_0060
 * @tc.name      : wmv3变分辨率，surface, nv21, profile1_1920x1080_24
 * @tc.desc      : function test
 */
HWTEST_F(Wmv3SwdecFuncNdkTest, VIDEO_DECODE_WMV3_RESOLUTION_0060, TestSize.Level3)
{
    if (g_codecNameWmv3.find("WMV3") != string::npos) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/profile1_1920x1080_24.wmv3";
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 24;
        vDecSample->SF_OUTPUT = true;
        vDecSample->isWmv3Change = true;
        vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV21;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameWmv3));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
        ASSERT_EQ(FRAMESIZE180, vDecSample->outFrameCount);
    }
}
/**
 * @tc.number    : API11_SURF_CHANGE_FUNC_001
 * @tc.name      : surf model change in normal state
 * @tc.desc      : function test
 */
HWTEST_F(Wmv3SwdecFuncNdkTest, API11_SURF_CHANGE_FUNC_001, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/profile1_352_288_10.wmv3";
        vDecSample->DEFAULT_WIDTH = 352;
        vDecSample->DEFAULT_HEIGHT = 288;
        vDecSample->DEFAULT_FRAME_RATE = 10;
        vDecSample->SURFACE_OUTPUT = true;
        vDecSample->autoSwitchSurface = true;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameWmv3));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->Reset());
        ASSERT_EQ(AV_ERR_INVALID_STATE, vDecSample->SwitchSurface());
        ASSERT_EQ(AV_ERR_OK, vDecSample->Release());
    }
}

/**
 * @tc.number    : API11_SURF_CHANGE_FUNC_002
 * @tc.name      : surf model change in flushed state
 * @tc.desc      : function test
 */
HWTEST_F(Wmv3SwdecFuncNdkTest, API11_SURF_CHANGE_FUNC_002, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/profile1_352_288_10.wmv3";
        vDecSample->DEFAULT_WIDTH = 352;
        vDecSample->DEFAULT_HEIGHT = 288;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SURFACE_OUTPUT = true;
        vDecSample->autoSwitchSurface = true;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameWmv3));
        ASSERT_EQ(AV_ERR_OK, vDecSample->Flush());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
        ASSERT_EQ(AV_ERR_OK, vDecSample->Stop());
    }
}

/**
 * @tc.number    : API11_SURF_CHANGE_FUNC_003
 * @tc.name      : surf model change in runing state
 * @tc.desc      : function test
 */
HWTEST_F(Wmv3SwdecFuncNdkTest, API11_SURF_CHANGE_FUNC_003, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/profile1_352_288_10.wmv3";
        vDecSample->DEFAULT_WIDTH = 352;
        vDecSample->DEFAULT_HEIGHT = 288;
        vDecSample->DEFAULT_FRAME_RATE = 10;
        vDecSample->SURFACE_OUTPUT = false;
        vDecSample->autoSwitchSurface = false;
        vDecSample->CreateSurface();
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameWmv3));
        ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, vDecSample->SwitchSurface());
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : API11_SURF_CHANGE_FUNC_004
 * @tc.name      : repeat call setSurface fastly
 * @tc.desc      : function test
 */
HWTEST_F(Wmv3SwdecFuncNdkTest, API11_SURF_CHANGE_FUNC_004, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/profile1_352_288_10.wmv3";
        vDecSample->DEFAULT_WIDTH = 352;
        vDecSample->DEFAULT_HEIGHT = 288;
        vDecSample->DEFAULT_FRAME_RATE = 10;
        vDecSample->SURFACE_OUTPUT = true;
        vDecSample->autoSwitchSurface = true;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameWmv3));
        ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
        vDecSample->WaitForEOS();
    }
}

/**
 * @tc.number    : API11_SURF_CHANGE_FUNC_005
 * @tc.name      : surf model change in flush to runnig state
 * @tc.desc      : function test
 */
HWTEST_F(Wmv3SwdecFuncNdkTest, API11_SURF_CHANGE_FUNC_005, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/profile1_352_288_10.wmv3";
        vDecSample->DEFAULT_WIDTH = 352;
        vDecSample->DEFAULT_HEIGHT = 288;
        vDecSample->DEFAULT_FRAME_RATE = 10;
        vDecSample->SURFACE_OUTPUT = true;
        vDecSample->autoSwitchSurface = true;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameWmv3));
        ASSERT_EQ(AV_ERR_OK, vDecSample->Flush());
        ASSERT_EQ(AV_ERR_OK, vDecSample->Start());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
    }
}

/**
 * @tc.number    : API11_SURF_CHANGE_FUNC_006
 * @tc.name      : surf model change in decoder finish to End-of-Stream state
 * @tc.desc      : function test
 */
HWTEST_F(Wmv3SwdecFuncNdkTest, API11_SURF_CHANGE_FUNC_006, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/profile1_352_288_10.wmv3";
        vDecSample->DEFAULT_WIDTH = 352;
        vDecSample->DEFAULT_HEIGHT = 288;
        vDecSample->DEFAULT_FRAME_RATE = 10;
        vDecSample->SURFACE_OUTPUT = true;
        vDecSample->autoSwitchSurface = true;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameWmv3));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
    }
}

/**
 * @tc.number    : API11_SURF_CHANGE_FUNC_007
 * @tc.name      : buffer model change in decoder finish to End-of-Stream state
 * @tc.desc      : function test
 */
HWTEST_F(Wmv3SwdecFuncNdkTest, API11_SURF_CHANGE_FUNC_007, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/profile1_352_288_10.wmv3";
        vDecSample->DEFAULT_WIDTH = 352;
        vDecSample->DEFAULT_HEIGHT = 288;
        vDecSample->DEFAULT_FRAME_RATE = 10;
        vDecSample->SURFACE_OUTPUT = false;
        vDecSample->autoSwitchSurface = false;
        vDecSample->CreateSurface();
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameWmv3));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, vDecSample->SwitchSurface());
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : API11_SURF_CHANGE_FUNC_008
 * @tc.name      : buffer model change in runing to flushed state
 * @tc.desc      : function test
 */
HWTEST_F(Wmv3SwdecFuncNdkTest, API11_SURF_CHANGE_FUNC_008, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/profile1_352_288_10.wmv3";
        vDecSample->DEFAULT_WIDTH = 352;
        vDecSample->DEFAULT_HEIGHT = 288;
        vDecSample->DEFAULT_FRAME_RATE = 10;
        vDecSample->SURFACE_OUTPUT = false;
        vDecSample->autoSwitchSurface = false;
        vDecSample->CreateSurface();
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameWmv3));
        ASSERT_EQ(AV_ERR_OK, vDecSample->Flush());
        ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, vDecSample->SwitchSurface());
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : API11_SURF_CHANGE_FUNC_009
 * @tc.name      : buffer model change in flushed to runing state
 * @tc.desc      : function test
 */
HWTEST_F(Wmv3SwdecFuncNdkTest, API11_SURF_CHANGE_FUNC_009, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/profile1_352_288_10.wmv3";
        vDecSample->DEFAULT_WIDTH = 352;
        vDecSample->DEFAULT_HEIGHT = 288;
        vDecSample->DEFAULT_FRAME_RATE = 10;
        vDecSample->SURFACE_OUTPUT = false;
        vDecSample->autoSwitchSurface = false;
        vDecSample->CreateSurface();
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameWmv3));
        ASSERT_EQ(AV_ERR_OK, vDecSample->Flush());
        ASSERT_EQ(AV_ERR_OK, vDecSample->Start());
        ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, vDecSample->SwitchSurface());
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : API11_SURF_CHANGE_FUNC_010
 * @tc.name      : buffer model change in normal state
 * @tc.desc      : function test
 */
HWTEST_F(Wmv3SwdecFuncNdkTest, API11_SURF_CHANGE_FUNC_010, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/profile1_352_288_10.wmv3";
        vDecSample->DEFAULT_WIDTH = 352;
        vDecSample->DEFAULT_HEIGHT = 288;
        vDecSample->DEFAULT_FRAME_RATE = 10;
        vDecSample->SURFACE_OUTPUT = false;
        vDecSample->autoSwitchSurface = false;
        vDecSample->CreateSurface();
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameWmv3));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->Reset());
        ASSERT_EQ(AV_ERR_INVALID_STATE, vDecSample->SwitchSurface());
        ASSERT_EQ(AV_ERR_OK, vDecSample->Release());
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : API11_SURF_CHANGE_FUNC_011
 * @tc.name      : surf model change in config state
 * @tc.desc      : function test
 */
HWTEST_F(Wmv3SwdecFuncNdkTest, API11_SURF_CHANGE_FUNC_011, TestSize.Level1)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/profile1_352_288_10.wmv3";
        vDecSample->DEFAULT_WIDTH = 352;
        vDecSample->DEFAULT_HEIGHT = 288;
        vDecSample->DEFAULT_FRAME_RATE = 10;
        vDecSample->SURFACE_OUTPUT = false;
        vDecSample->autoSwitchSurface = false;
        vDecSample->CreateSurface();
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameWmv3));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : API11_SURF_CHANGE_FUNC_012
 * @tc.name      : surf model change in config state
 * @tc.desc      : function test
 */
HWTEST_F(Wmv3SwdecFuncNdkTest, API11_SURF_CHANGE_FUNC_012, TestSize.Level1)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/profile1_352_288_10.wmv3";
        vDecSample->DEFAULT_WIDTH = 352;
        vDecSample->DEFAULT_HEIGHT = 288;
        vDecSample->DEFAULT_FRAME_RATE = 10;
        vDecSample->SURFACE_OUTPUT = false;
        vDecSample->autoSwitchSurface = false;
        vDecSample->CreateSurface();
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameWmv3));
        ASSERT_EQ(AV_ERR_INVALID_STATE, vDecSample->SwitchSurface());
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : API11_SURF_CHANGE_FUNC_013
 * @tc.name      : Two object repeat call setSurface fastly
 * @tc.desc      : function test
 */
HWTEST_F(Wmv3SwdecFuncNdkTest, API11_SURF_CHANGE_FUNC_013, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/profile1_352_288_10.wmv3";
        vDecSample->DEFAULT_WIDTH = 352;
        vDecSample->DEFAULT_HEIGHT = 288;
        vDecSample->DEFAULT_FRAME_RATE = 10;
        vDecSample->SURFACE_OUTPUT = true;
        vDecSample->autoSwitchSurface = true;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameWmv3));
        ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
        vDecSample->WaitForEOS();

        auto vDecSample_1 = make_shared<VDecAPI11Sample>();
        vDecSample_1->INP_DIR = "/data/test/media/profile1_352_288_10.wmv3";
        vDecSample_1->DEFAULT_WIDTH = 352;
        vDecSample_1->DEFAULT_HEIGHT = 288;
        vDecSample_1->DEFAULT_FRAME_RATE = 10;
        vDecSample_1->SURFACE_OUTPUT = true;
        vDecSample_1->autoSwitchSurface = true;
        vDecSample_1->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample_1->RunVideoDec_Surface(g_codecNameWmv3));
        ASSERT_EQ(AV_ERR_OK, vDecSample_1->SwitchSurface());
        vDecSample_1->WaitForEOS();
    }
}

/**
 * @tc.number    : API11_SURF_CHANGE_FUNC_014
 * @tc.name      : repeat call setSurface fastly 2 time
 * @tc.desc      : function test
 */
HWTEST_F(Wmv3SwdecFuncNdkTest, API11_SURF_CHANGE_FUNC_014, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0)) {
        for (int i = 0; i < 2; i++) {
            auto vDecSample = make_shared<VDecAPI11Sample>();
            vDecSample->INP_DIR = "/data/test/media/profile1_352_288_10.wmv3";
            vDecSample->DEFAULT_WIDTH = 352;
            vDecSample->DEFAULT_HEIGHT = 288;
            vDecSample->DEFAULT_FRAME_RATE = 10;
            vDecSample->SURFACE_OUTPUT = true;
            vDecSample->autoSwitchSurface = true;
            vDecSample->sleepOnFPS = true;
            ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameWmv3));
            ASSERT_EQ(AV_ERR_OK, vDecSample->RepeatCallSetSurface());
            vDecSample->WaitForEOS();
        }
    }
}

/**
 * @tc.number    : VIDEO_DECODE_SYNC_SWWMV3_FUNC_0010
 * @tc.name      : wmv3同步软解输出nv12
 * @tc.desc      : function test
 */
HWTEST_F(Wmv3SwdecFuncNdkTest, VIDEO_DECODE_SYNC_SWWMV3_FUNC_0010, TestSize.Level1)
{
    if (cap_wmv3 != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/profile0_128x96_10.wmv3";
        vDecSample->DEFAULT_WIDTH = 128;
        vDecSample->DEFAULT_HEIGHT = 96;
        vDecSample->DEFAULT_FRAME_RATE = 10;
        vDecSample->enbleSyncMode = 1;
        vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameWmv3));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartSyncVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
        ASSERT_EQ(FRAMESIZE90, vDecSample->outFrameCount);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_SYNC_SWWMV3_FUNC_0020
 * @tc.name      : wmv3同步软解输出nv21
 * @tc.desc      : function test
 */
HWTEST_F(Wmv3SwdecFuncNdkTest, VIDEO_DECODE_SYNC_SWWMV3_FUNC_0020, TestSize.Level0)
{
    if (cap_wmv3 != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/profile0_128x96_10.wmv3";
        vDecSample->DEFAULT_WIDTH = 128;
        vDecSample->DEFAULT_HEIGHT = 96;
        vDecSample->DEFAULT_FRAME_RATE = 10;
        vDecSample->enbleSyncMode = 1;
        vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV21;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameWmv3));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartSyncVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
        ASSERT_EQ(FRAMESIZE90, vDecSample->outFrameCount);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_SYNC_SWWMV3_FUNC_0030
 * @tc.name      : wmv3同步软解输出surface
 * @tc.desc      : function test
 */
HWTEST_F(Wmv3SwdecFuncNdkTest, VIDEO_DECODE_SYNC_SWWMV3_FUNC_0030, TestSize.Level1)
{
    if (cap_wmv3 != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/profile0_128x96_10.wmv3";
        vDecSample->DEFAULT_WIDTH = 128;
        vDecSample->DEFAULT_HEIGHT = 96;
        vDecSample->DEFAULT_FRAME_RATE = 10;
        vDecSample->SF_OUTPUT = true;
        vDecSample->enbleSyncMode = 1;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameWmv3));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->DecodeSetSurface());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartSyncVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
        ASSERT_EQ(FRAMESIZE90, vDecSample->outFrameCount);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_SYNC_SWWMV3_FUNC_0040
 * @tc.name      : wmv3同步软解输出rgba
 * @tc.desc      : function test
 */
HWTEST_F(Wmv3SwdecFuncNdkTest, VIDEO_DECODE_SYNC_SWWMV3_FUNC_0040, TestSize.Level1)
{
    if (cap_wmv3 != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/profile0_128x96_10.wmv3";
        vDecSample->DEFAULT_WIDTH = 128;
        vDecSample->DEFAULT_HEIGHT = 96;
        vDecSample->DEFAULT_FRAME_RATE = 10;
        vDecSample->enbleSyncMode = 1;
        vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_RGBA;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameWmv3));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartSyncVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
        ASSERT_EQ(FRAMESIZE90, vDecSample->outFrameCount);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_SWWMV3_BLANK_FRAME_0010
 * @tc.name      : config OH_MD_KEY_VIDEO_DECODER_BLANK_FRAME_ON_SHUTDOWN, decoder wmv3
 * @tc.desc      : function test
 */
HWTEST_F(Wmv3SwdecFuncNdkTest, VIDEO_DECODE_SWWMV3_BLANK_FRAME_0010, TestSize.Level2)
{
    if (cap_wmv3 != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/profile0_128x96_10.wmv3";
        vDecSample->DEFAULT_WIDTH = 128;
        vDecSample->DEFAULT_HEIGHT = 96;
        vDecSample->DEFAULT_FRAME_RATE = 10;
        vDecSample->enbleBlankFrame = 1;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.WMV3"));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
        ASSERT_EQ(FRAMESIZE90, vDecSample->outFrameCount);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_SWWMV3_BLANK_FRAME_0020
 * @tc.name      : config OH_MD_KEY_VIDEO_DECODER_BLANK_FRAME_ON_SHUTDOWN, decoder wmv3, surface
 * @tc.desc      : function test
 */
HWTEST_F(Wmv3SwdecFuncNdkTest, VIDEO_DECODE_SWWMV3_BLANK_FRAME_0020, TestSize.Level2)
{
    if (cap_wmv3 != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/profile0_128x96_10.wmv3";
        vDecSample->DEFAULT_WIDTH = 128;
        vDecSample->DEFAULT_HEIGHT = 96;
        vDecSample->DEFAULT_FRAME_RATE = 10;
        vDecSample->SURFACE_OUTPUT = true;
        vDecSample->enbleBlankFrame = 1;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.WMV3"));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->DecodeSetSurface());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
        ASSERT_EQ(FRAMESIZE90, vDecSample->outFrameCount);
    }
}

/**
 * @tc.number    : VIDEO_WMV3SWDEC_FUNCTION_GRAPH_0010
 * @tc.name      : decode WMV3 buffer,graph PixelFormat
 * @tc.desc      : function test
 */
HWTEST_F(Wmv3SwdecFuncNdkTest, VIDEO_WMV3SWDEC_FUNCTION_GRAPH_0010, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    int32_t pixfmt[4] = {28, 24, 25, 12};
    vDecSample->INP_DIR = "/data/test/media/profile0_128x96_10.wmv3";
    vDecSample->DEFAULT_WIDTH = 128;
    vDecSample->DEFAULT_HEIGHT = 96;
    vDecSample->DEFAULT_FRAME_RATE = 10;
    vDecSample->outputYuvFlag = true;
    vDecSample->isGetVideoSupportedPixelFormats = true;
    vDecSample->isGetFormatKey = true;
    vDecSample->avcodecMimeType = OH_AVCODEC_MIMETYPE_VIDEO_WMV3;
    vDecSample->isEncoder = false;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.WMV3"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(4, vDecSample->pixlFormatNum);
    for (int i = 0; i < vDecSample->pixlFormatNum; i++) {
        ASSERT_EQ(vDecSample->pixlFormats[i], pixfmt[i]);
    }
    ASSERT_EQ(FRAMESIZE90, vDecSample->outFrameCount);
}

}  // namespace