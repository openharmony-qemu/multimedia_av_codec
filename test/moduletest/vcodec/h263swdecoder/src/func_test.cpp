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
class H263SwdecFuncNdkTest : public testing::Test {
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
} // namespace Media
} // namespace OHOS

namespace {
static OH_AVCapability *cap_263 = nullptr;
static string g_codecName263 = "";
static OH_AVCapability *cap_264 = nullptr;
static string g_codecName264 = "";
constexpr uint32_t FRAMESIZE90 = 90;
constexpr uint32_t FRAMESIZE75 = 75;
constexpr uint32_t FRAMESIZE180 = 180;
constexpr uint32_t FRAMESIZE360 = 360;
constexpr uint32_t FRAMESIZE540 = 540;

} // namespace

void H263SwdecFuncNdkTest::SetUpTestCase()
{
    cap_263 = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_H263, false, SOFTWARE);
    g_codecName263 = OH_AVCapability_GetName(cap_263);
    cout << "g_codecName263: " << g_codecName263 << endl;
    cap_264 = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_AVC, false, SOFTWARE);
    g_codecName264 = OH_AVCapability_GetName(cap_264);
    cout << "g_codecName264: " << g_codecName264 << endl;
}

void H263SwdecFuncNdkTest::TearDownTestCase() {}
void H263SwdecFuncNdkTest::SetUp() {}
void H263SwdecFuncNdkTest::TearDown() {}

namespace {
/**
 * @tc.number    : VIDEO_H263SWDEC_FUNCTION_0010
 * @tc.name      : decode H263 buffer,PixelFormat,NV12
 * @tc.desc      : function test
 */
HWTEST_F(H263SwdecFuncNdkTest, VIDEO_H263SWDEC_FUNCTION_0010, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = "/data/test/media/20x20.h263";
    vDecSample->DEFAULT_WIDTH = 20;
    vDecSample->DEFAULT_HEIGHT = 20;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.H263"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE75, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_H263SWDEC_FUNCTION_0020
 * @tc.name      : decode H263 buffer,PixelFormat,NV21
 * @tc.desc      : function test
 */
HWTEST_F(H263SwdecFuncNdkTest, VIDEO_H263SWDEC_FUNCTION_0020, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = "/data/test/media/profile0_level10_I_128x96.h263";
    vDecSample->DEFAULT_WIDTH = 128;
    vDecSample->DEFAULT_HEIGHT = 96;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV21;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.H263"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE90, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_H263SWDEC_FUNCTION_0030
 * @tc.name      : decode H263 buffer,PixelFormat,I420
 * @tc.desc      : function test
 */
HWTEST_F(H263SwdecFuncNdkTest, VIDEO_H263SWDEC_FUNCTION_0030, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = "/data/test/media/profile0_level20_176x144.h263";
    vDecSample->DEFAULT_WIDTH = 176;
    vDecSample->DEFAULT_HEIGHT = 144;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_YUVI420;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.H263"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE90, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_H263SWDEC_FUNCTION_0040
 * @tc.name      : decode H263 buffer,PixelFormat,RGBA
 * @tc.desc      : function test
 */
HWTEST_F(H263SwdecFuncNdkTest, VIDEO_H263SWDEC_FUNCTION_0040, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = "/data/test/media/profile0_level30_352x288.h263";
    vDecSample->DEFAULT_WIDTH = 352;
    vDecSample->DEFAULT_HEIGHT = 288;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_RGBA;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.H263"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE90, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_H263SWDEC_FUNCTION_0050
 * @tc.name      : decode H263 buffer,profile0_level40
 * @tc.desc      : function test
 */
HWTEST_F(H263SwdecFuncNdkTest, VIDEO_H263SWDEC_FUNCTION_0050, TestSize.Level1)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = "/data/test/media/profile0_level40_I_352x288.h263";
    vDecSample->DEFAULT_WIDTH = 352;
    vDecSample->DEFAULT_HEIGHT = 288;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.H263"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE90, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_H263SWDEC_FUNCTION_0060
 * @tc.name      : decode H263 buffer,profile0_level50
 * @tc.desc      : function test
 */
HWTEST_F(H263SwdecFuncNdkTest, VIDEO_H263SWDEC_FUNCTION_0060, TestSize.Level1)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = "/data/test/media/profile0_level50_352x288.h263";
    vDecSample->DEFAULT_WIDTH = 352;
    vDecSample->DEFAULT_HEIGHT = 288;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.H263"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE90, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_H263SWDEC_FUNCTION_0070
 * @tc.name      : decode H263 buffer,profile0_level60
 * @tc.desc      : function test
 */
HWTEST_F(H263SwdecFuncNdkTest, VIDEO_H263SWDEC_FUNCTION_0070, TestSize.Level1)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = "/data/test/media/profile0_level60_704x576.h263";
    vDecSample->DEFAULT_WIDTH = 704;
    vDecSample->DEFAULT_HEIGHT = 576;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.H263"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE90, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_H263SWDEC_FUNCTION_0080
 * @tc.name      : decode H263 buffer,profile0_level70
 * @tc.desc      : function test
 */
HWTEST_F(H263SwdecFuncNdkTest, VIDEO_H263SWDEC_FUNCTION_0080, TestSize.Level1)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = "/data/test/media/profile0_level70_1408x1152.h263";
    vDecSample->DEFAULT_WIDTH = 1408;
    vDecSample->DEFAULT_HEIGHT = 1152;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.H263"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE90, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_H263SWDEC_FUNCTION_0090
 * @tc.name      : decode H263 buffer,profile2_level10
 * @tc.desc      : function test
 */
HWTEST_F(H263SwdecFuncNdkTest, VIDEO_H263SWDEC_FUNCTION_0090, TestSize.Level1)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = "/data/test/media/profile2_level10_128x96.h263";
    vDecSample->DEFAULT_WIDTH = 128;
    vDecSample->DEFAULT_HEIGHT = 96;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.H263"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE75, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_H263SWDEC_FUNCTION_0100
 * @tc.name      : decode H263 buffer,profile2_level20
 * @tc.desc      : function test
 */
HWTEST_F(H263SwdecFuncNdkTest, VIDEO_H263SWDEC_FUNCTION_0100, TestSize.Level1)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = "/data/test/media/profile2_level20_176x144.h263";
    vDecSample->DEFAULT_WIDTH = 176;
    vDecSample->DEFAULT_HEIGHT = 144;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.H263"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE75, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_H263SWDEC_FUNCTION_0110
 * @tc.name      : decode H263 buffer,profile2_level30
 * @tc.desc      : function test
 */
HWTEST_F(H263SwdecFuncNdkTest, VIDEO_H263SWDEC_FUNCTION_0110, TestSize.Level1)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = "/data/test/media/profile2_level30_I_352x288.h263";
    vDecSample->DEFAULT_WIDTH = 352;
    vDecSample->DEFAULT_HEIGHT = 288;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.H263"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE75, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_H263SWDEC_FUNCTION_0120
 * @tc.name      : decode H263 buffer,profile2_level40
 * @tc.desc      : function test
 */
HWTEST_F(H263SwdecFuncNdkTest, VIDEO_H263SWDEC_FUNCTION_0120, TestSize.Level1)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = "/data/test/media/profile2_level40_352x288.h263";
    vDecSample->DEFAULT_WIDTH = 352;
    vDecSample->DEFAULT_HEIGHT = 288;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.H263"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE75, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_H263SWDEC_FUNCTION_0130
 * @tc.name      : decode H263 buffer,profile2_level50
 * @tc.desc      : function test
 */
HWTEST_F(H263SwdecFuncNdkTest, VIDEO_H263SWDEC_FUNCTION_0130, TestSize.Level1)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = "/data/test/media/profile2_level50_gop60_1280x720.h263";
    vDecSample->DEFAULT_WIDTH = 1280;
    vDecSample->DEFAULT_HEIGHT = 720;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.H263"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE180, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_H263SWDEC_FUNCTION_0140
 * @tc.name      : decode H263 buffer,profile2_level60
 * @tc.desc      : function test
 */
HWTEST_F(H263SwdecFuncNdkTest, VIDEO_H263SWDEC_FUNCTION_0140, TestSize.Level1)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = "/data/test/media/profile2_level60_gop60_1920x1080.h263";
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.H263"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE180, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_H263SWDEC_FUNCTION_0150
 * @tc.name      : decode H263 buffer,profile2_level70
 * @tc.desc      : function test
 */
HWTEST_F(H263SwdecFuncNdkTest, VIDEO_H263SWDEC_FUNCTION_0150, TestSize.Level1)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = "/data/test/media/profile2_level70_gop60_2048x1152.h263";
    vDecSample->DEFAULT_WIDTH = 2048;
    vDecSample->DEFAULT_HEIGHT = 1152;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.H263"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE180, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_H263SWDEC_FUNCTION_0160
 * @tc.name      : decode H263 buffer,input h264
 * @tc.desc      : function test
 */
HWTEST_F(H263SwdecFuncNdkTest, VIDEO_H263SWDEC_FUNCTION_0160, TestSize.Level3)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = "/data/test/media/1920x1080.h264";
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.H263"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(true, vDecSample->isonError);
}

/**
 * @tc.number    : VIDEO_H263SWDEC_FUNCTION_0170
 * @tc.name      : decode H263 surface,profile2_level70
 * @tc.desc      : function test
 */
HWTEST_F(H263SwdecFuncNdkTest, VIDEO_H263SWDEC_FUNCTION_0170, TestSize.Level3)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = "/data/test/media/profile2_level70_gop60_2048x1152.h263";
    vDecSample->DEFAULT_WIDTH = 2048;
    vDecSample->DEFAULT_HEIGHT = 1152;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->SF_OUTPUT = true;
    vDecSample->outputYuvSurface = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecName263));
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE180, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_H263SWDEC_FUNCTION_0180
 * @tc.name      : decode H263 surface,profile0_level10
 * @tc.desc      : function test
 */
HWTEST_F(H263SwdecFuncNdkTest, VIDEO_H263SWDEC_FUNCTION_0180, TestSize.Level3)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = "/data/test/media/profile0_level10_I_128x96.h263";
    vDecSample->DEFAULT_WIDTH = 128;
    vDecSample->DEFAULT_HEIGHT = 96;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->SF_OUTPUT = true;
    vDecSample->outputYuvSurface = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV21;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecName263));
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE90, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_H263SWDEC_FUNCTION_0190
 * @tc.name      : decode H263 surface,profile0_level30
 * @tc.desc      : function test
 */
HWTEST_F(H263SwdecFuncNdkTest, VIDEO_H263SWDEC_FUNCTION_0190, TestSize.Level3)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = "/data/test/media/profile0_level30_352x288.h263";
    vDecSample->DEFAULT_WIDTH = 352;
    vDecSample->DEFAULT_HEIGHT = 288;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->SF_OUTPUT = true;
    vDecSample->outputYuvSurface = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV21;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecName263));
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE90, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_DECODE_H263_RESOLUTION_0010
 * @tc.name      : h263变分辨率，buffer, nv12
 * @tc.desc      : function test
 */
HWTEST_F(H263SwdecFuncNdkTest, VIDEO_DECODE_H263_RESOLUTION_0010, TestSize.Level3)
{
    if (g_codecName263.find("H263") != string::npos) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/profile0_0_0.h263";
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        vDecSample->outputYuvFlag = true;
        vDecSample->isH263Change = true;
        vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecName263));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
        ASSERT_EQ(FRAMESIZE360, vDecSample->outFrameCount);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_H263_RESOLUTION_0020
 * @tc.name      : h263变分辨率，buffer, nv21
 * @tc.desc      : function test
 */
HWTEST_F(H263SwdecFuncNdkTest, VIDEO_DECODE_H263_RESOLUTION_0020, TestSize.Level3)
{
    if (g_codecName263.find("H263") != string::npos) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/profile2_2_2.h263";
        vDecSample->DEFAULT_WIDTH = 20;
        vDecSample->DEFAULT_HEIGHT = 20;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        vDecSample->outputYuvFlag = true;
        vDecSample->isH263Change = true;
        vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV21;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecName263));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
        ASSERT_EQ(FRAMESIZE540, vDecSample->outFrameCount);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_H263_RESOLUTION_0030
 * @tc.name      : h263变分辨率，buffer, nv12
 * @tc.desc      : function test
 */
HWTEST_F(H263SwdecFuncNdkTest, VIDEO_DECODE_H263_RESOLUTION_0030, TestSize.Level3)
{
    if (g_codecName263.find("H263") != string::npos) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/profile0_0_2.h263";
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        vDecSample->outputYuvFlag = true;
        vDecSample->isH263Change = true;
        vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecName263));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
        ASSERT_EQ(FRAMESIZE360, vDecSample->outFrameCount);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_H263_RESOLUTION_0040
 * @tc.name      : h263变分辨率，surface, nv21
 * @tc.desc      : function test
 */
HWTEST_F(H263SwdecFuncNdkTest, VIDEO_DECODE_H263_RESOLUTION_0040, TestSize.Level3)
{
    if (g_codecName263.find("H263") != string::npos) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/profile2_2_2.h263";
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = true;
        vDecSample->isH263Change = true;
        vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV21;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecName263));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
        ASSERT_EQ(FRAMESIZE540, vDecSample->outFrameCount);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_H263_RESOLUTION_0050
 * @tc.name      : h263变分辨率，surface, nv21
 * @tc.desc      : function test
 */
HWTEST_F(H263SwdecFuncNdkTest, VIDEO_DECODE_H263_RESOLUTION_0050, TestSize.Level3)
{
    if (g_codecName263.find("H263") != string::npos) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/profile0_0_2.h263";
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = true;
        vDecSample->isH263Change = true;
        vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV21;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecName263));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
        ASSERT_EQ(FRAMESIZE360, vDecSample->outFrameCount);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_H263_RESOLUTION_0060
 * @tc.name      : h263变分辨率，surface, nv21
 * @tc.desc      : function test
 */
HWTEST_F(H263SwdecFuncNdkTest, VIDEO_DECODE_H263_RESOLUTION_0060, TestSize.Level3)
{
    if (g_codecName263.find("H263") != string::npos) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/profile0_0_0.h263";
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = true;
        vDecSample->isH263Change = true;
        vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV21;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecName263));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
        ASSERT_EQ(FRAMESIZE360, vDecSample->outFrameCount);
    }
}
/**
 * @tc.number    : API11_SURF_CHANGE_FUNC_001
 * @tc.name      : surf model change in normal state
 * @tc.desc      : function test
 */
HWTEST_F(H263SwdecFuncNdkTest, API11_SURF_CHANGE_FUNC_001, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/profile0_level30_352x288.h263";
        vDecSample->DEFAULT_WIDTH = 352;
        vDecSample->DEFAULT_HEIGHT = 288;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SURFACE_OUTPUT = true;
        vDecSample->autoSwitchSurface = true;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecName263));
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
HWTEST_F(H263SwdecFuncNdkTest, API11_SURF_CHANGE_FUNC_002, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/profile0_level30_352x288.h263";
        vDecSample->DEFAULT_WIDTH = 352;
        vDecSample->DEFAULT_HEIGHT = 288;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SURFACE_OUTPUT = true;
        vDecSample->autoSwitchSurface = true;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecName263));
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
HWTEST_F(H263SwdecFuncNdkTest, API11_SURF_CHANGE_FUNC_003, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/profile0_level30_352x288.h263";
        vDecSample->DEFAULT_WIDTH = 352;
        vDecSample->DEFAULT_HEIGHT = 288;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SURFACE_OUTPUT = false;
        vDecSample->autoSwitchSurface = false;
        vDecSample->CreateSurface();
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecName263));
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
HWTEST_F(H263SwdecFuncNdkTest, API11_SURF_CHANGE_FUNC_004, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/profile0_level30_352x288.h263";
        vDecSample->DEFAULT_WIDTH = 352;
        vDecSample->DEFAULT_HEIGHT = 288;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SURFACE_OUTPUT = true;
        vDecSample->autoSwitchSurface = true;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecName263));
        ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
        vDecSample->WaitForEOS();
    }
}

/**
 * @tc.number    : API11_SURF_CHANGE_FUNC_005
 * @tc.name      : surf model change in flush to runnig state
 * @tc.desc      : function test
 */
HWTEST_F(H263SwdecFuncNdkTest, API11_SURF_CHANGE_FUNC_005, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/profile0_level30_352x288.h263";
        vDecSample->DEFAULT_WIDTH = 352;
        vDecSample->DEFAULT_HEIGHT = 288;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SURFACE_OUTPUT = true;
        vDecSample->autoSwitchSurface = true;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecName263));
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
HWTEST_F(H263SwdecFuncNdkTest, API11_SURF_CHANGE_FUNC_006, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/profile0_level30_352x288.h263";
        vDecSample->DEFAULT_WIDTH = 352;
        vDecSample->DEFAULT_HEIGHT = 288;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SURFACE_OUTPUT = true;
        vDecSample->autoSwitchSurface = true;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecName263));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
    }
}

/**
 * @tc.number    : API11_SURF_CHANGE_FUNC_007
 * @tc.name      : buffer model change in decoder finish to End-of-Stream state
 * @tc.desc      : function test
 */
HWTEST_F(H263SwdecFuncNdkTest, API11_SURF_CHANGE_FUNC_007, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/profile0_level30_352x288.h263";
        vDecSample->DEFAULT_WIDTH = 352;
        vDecSample->DEFAULT_HEIGHT = 288;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SURFACE_OUTPUT = false;
        vDecSample->autoSwitchSurface = false;
        vDecSample->CreateSurface();
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecName263));
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
HWTEST_F(H263SwdecFuncNdkTest, API11_SURF_CHANGE_FUNC_008, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/profile0_level30_352x288.h263";
        vDecSample->DEFAULT_WIDTH = 352;
        vDecSample->DEFAULT_HEIGHT = 288;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SURFACE_OUTPUT = false;
        vDecSample->autoSwitchSurface = false;
        vDecSample->CreateSurface();
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecName263));
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
HWTEST_F(H263SwdecFuncNdkTest, API11_SURF_CHANGE_FUNC_009, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/profile0_level30_352x288.h263";
        vDecSample->DEFAULT_WIDTH = 352;
        vDecSample->DEFAULT_HEIGHT = 288;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SURFACE_OUTPUT = false;
        vDecSample->autoSwitchSurface = false;
        vDecSample->CreateSurface();
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecName263));
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
HWTEST_F(H263SwdecFuncNdkTest, API11_SURF_CHANGE_FUNC_010, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/profile0_level30_352x288.h263";
        vDecSample->DEFAULT_WIDTH = 352;
        vDecSample->DEFAULT_HEIGHT = 288;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SURFACE_OUTPUT = false;
        vDecSample->autoSwitchSurface = false;
        vDecSample->CreateSurface();
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecName263));
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
HWTEST_F(H263SwdecFuncNdkTest, API11_SURF_CHANGE_FUNC_011, TestSize.Level1)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/profile0_level30_352x288.h263";
        vDecSample->DEFAULT_WIDTH = 352;
        vDecSample->DEFAULT_HEIGHT = 288;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SURFACE_OUTPUT = false;
        vDecSample->autoSwitchSurface = false;
        vDecSample->CreateSurface();
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecName263));
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
HWTEST_F(H263SwdecFuncNdkTest, API11_SURF_CHANGE_FUNC_012, TestSize.Level1)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/profile0_level30_352x288.h263";
        vDecSample->DEFAULT_WIDTH = 352;
        vDecSample->DEFAULT_HEIGHT = 288;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SURFACE_OUTPUT = false;
        vDecSample->autoSwitchSurface = false;
        vDecSample->CreateSurface();
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecName263));;
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
HWTEST_F(H263SwdecFuncNdkTest, API11_SURF_CHANGE_FUNC_013, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0)) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/profile0_level30_352x288.h263";
        vDecSample->DEFAULT_WIDTH = 352;
        vDecSample->DEFAULT_HEIGHT = 288;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SURFACE_OUTPUT = true;
        vDecSample->autoSwitchSurface = true;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecName263));
        ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
        vDecSample->WaitForEOS();

        auto vDecSample_1 = make_shared<VDecAPI11Sample>();
        vDecSample_1->INP_DIR = "/data/test/media/profile0_level30_352x288.h263";
        vDecSample_1->DEFAULT_WIDTH = 352;
        vDecSample_1->DEFAULT_HEIGHT = 288;
        vDecSample_1->DEFAULT_FRAME_RATE = 30;
        vDecSample_1->SURFACE_OUTPUT = true;
        vDecSample_1->autoSwitchSurface = true;
        vDecSample_1->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample_1->RunVideoDec_Surface(g_codecName263));
        ASSERT_EQ(AV_ERR_OK, vDecSample_1->SwitchSurface());
        vDecSample_1->WaitForEOS();
    }
}

/**
 * @tc.number    : API11_SURF_CHANGE_FUNC_014
 * @tc.name      : repeat call setSurface fastly 2 time
 * @tc.desc      : function test
 */
HWTEST_F(H263SwdecFuncNdkTest, API11_SURF_CHANGE_FUNC_014, TestSize.Level2)
{
    if (!access("/system/lib64/media/", 0)) {
        for (int i = 0; i < 2; i++) {
            auto vDecSample = make_shared<VDecAPI11Sample>();
            vDecSample->INP_DIR = "/data/test/media/profile0_level30_352x288.h263";
            vDecSample->DEFAULT_WIDTH = 352;
            vDecSample->DEFAULT_HEIGHT = 288;
            vDecSample->DEFAULT_FRAME_RATE = 30;
            vDecSample->SURFACE_OUTPUT = true;
            vDecSample->autoSwitchSurface = true;
            vDecSample->sleepOnFPS = true;
            ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecName263));
            ASSERT_EQ(AV_ERR_OK, vDecSample->RepeatCallSetSurface());
            vDecSample->WaitForEOS();
        }
    }
}

/**
 * @tc.number    : VIDEO_DECODE_SYNC_SW263_FUNC_0010
 * @tc.name      : 263同步软解输出nv12
 * @tc.desc      : function test
 */
HWTEST_F(H263SwdecFuncNdkTest, VIDEO_DECODE_SYNC_SW263_FUNC_0010, TestSize.Level1)
{
    if (cap_263 != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/profile0_level10_I_128x96.h263";
        vDecSample->DEFAULT_WIDTH = 128;
        vDecSample->DEFAULT_HEIGHT = 96;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->enbleSyncMode = 1;
        vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecName263));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartSyncVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
        ASSERT_EQ(FRAMESIZE90, vDecSample->outFrameCount);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_SYNC_SW263_FUNC_0020
 * @tc.name      : 263同步软解输出nv21
 * @tc.desc      : function test
 */
HWTEST_F(H263SwdecFuncNdkTest, VIDEO_DECODE_SYNC_SW263_FUNC_0020, TestSize.Level0)
{
    if (cap_263 != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/profile0_level10_I_128x96.h263";
        vDecSample->DEFAULT_WIDTH = 128;
        vDecSample->DEFAULT_HEIGHT = 96;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->enbleSyncMode = 1;
        vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV21;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecName263));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartSyncVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
        ASSERT_EQ(FRAMESIZE90, vDecSample->outFrameCount);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_SYNC_SW263_FUNC_0030
 * @tc.name      : 263同步软解输出surface
 * @tc.desc      : function test
 */
HWTEST_F(H263SwdecFuncNdkTest, VIDEO_DECODE_SYNC_SW263_FUNC_0030, TestSize.Level1)
{
    if (cap_263 != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/profile0_level10_I_128x96.h263";
        vDecSample->DEFAULT_WIDTH = 128;
        vDecSample->DEFAULT_HEIGHT = 96;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = true;
        vDecSample->enbleSyncMode = 1;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecName263));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->DecodeSetSurface());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartSyncVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
        ASSERT_EQ(FRAMESIZE90, vDecSample->outFrameCount);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_SYNC_SW263_FUNC_0040
 * @tc.name      : 263同步软解输出rgba
 * @tc.desc      : function test
 */
HWTEST_F(H263SwdecFuncNdkTest, VIDEO_DECODE_SYNC_SW263_FUNC_0040, TestSize.Level1)
{
    if (cap_263 != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/profile0_level10_I_128x96.h263";
        vDecSample->DEFAULT_WIDTH = 128;
        vDecSample->DEFAULT_HEIGHT = 96;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->enbleSyncMode = 1;
        vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_RGBA;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecName263));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartSyncVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
        ASSERT_EQ(FRAMESIZE90, vDecSample->outFrameCount);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_SW263_BLANK_FRAME_0010
 * @tc.name      : config OH_MD_KEY_VIDEO_DECODER_BLANK_FRAME_ON_SHUTDOWN, decoder h263
 * @tc.desc      : function test
 */
HWTEST_F(H263SwdecFuncNdkTest, VIDEO_DECODE_SW263_BLANK_FRAME_0010, TestSize.Level2)
{
    if (cap_263 != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/20x20.h263";
        vDecSample->DEFAULT_WIDTH = 20;
        vDecSample->DEFAULT_HEIGHT = 20;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->enbleBlankFrame = 1;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.H263"));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
        ASSERT_EQ(FRAMESIZE75, vDecSample->outFrameCount);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_SW263_BLANK_FRAME_0020
 * @tc.name      : config OH_MD_KEY_VIDEO_DECODER_BLANK_FRAME_ON_SHUTDOWN, decoder h263, surface
 * @tc.desc      : function test
 */
HWTEST_F(H263SwdecFuncNdkTest, VIDEO_DECODE_SW263_BLANK_FRAME_0020, TestSize.Level2)
{
    if (cap_263 != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/20x20.h263";
        vDecSample->DEFAULT_WIDTH = 20;
        vDecSample->DEFAULT_HEIGHT = 20;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SURFACE_OUTPUT = true;
        vDecSample->enbleBlankFrame = 1;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.H263"));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->DecodeSetSurface());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
        ASSERT_EQ(FRAMESIZE75, vDecSample->outFrameCount);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_H263_FLUSH_0010
 * @tc.name      : create-start-eos-flush-start-eos
 * @tc.desc      : function test
 */
HWTEST_F(H263SwdecFuncNdkTest, VIDEO_DECODE_H263_FLUSH_0010, TestSize.Level1)
{
    if (g_codecName263.find("H263") != string::npos) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/profile0_0_0.h263";
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV21;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecName263));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
        ASSERT_EQ(AV_ERR_OK, vDecSample->Flush());
        vDecSample->FlushStatus();
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_H263_FLUSH_0020
 * @tc.name      : create-start-eos-flush-start-eos,surface
 * @tc.desc      : function test
 */
HWTEST_F(H263SwdecFuncNdkTest, VIDEO_DECODE_H263_FLUSH_0020, TestSize.Level1)
{
    if (g_codecName263.find("H263") != string::npos) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/profile0_0_0.h263";
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = true;
        vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV21;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecName263));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
        ASSERT_EQ(AV_ERR_OK, vDecSample->Flush());
        vDecSample->FlushStatus();
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_PIXE_FORMAT_0010
 * @tc.name      : h263 pixelformat query key
 * @tc.desc      : function test
 */
HWTEST_F(H263SwdecFuncNdkTest, VIDEO_DECODE_PIXE_FORMAT_0010, TestSize.Level2)
{
    if (cap_263 != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/20x20.h263";
        vDecSample->isGetVideoSupportedPixelFormats = true;
        vDecSample->isGetFormatKey = true;
        vDecSample->avcodecMimeType = OH_AVCODEC_MIMETYPE_VIDEO_H263;
        vDecSample->isEncoder = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecName263));
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_LT(1, vDecSample->pixlFormatNum);
        ASSERT_EQ(24, vDecSample->firstCallBackKey);
        ASSERT_EQ(24, vDecSample->onStreamChangedKey);
    }
}
} // namespace