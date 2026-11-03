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
class Wmv1SwdecFuncNdkTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp() override;
    void TearDown() override;
};
}  // namespace Media
}  // namespace OHOS

namespace {
static OH_AVCapability *cap_wmv1 = nullptr;
static string g_codecNameWmv1 = "";

/** WMV1 码流信息：分辨率、帧率以及逐帧切分表中的总帧数 */
struct Wmv1StreamInfo {
    const char *path;
    uint32_t width;
    uint32_t height;
    uint32_t frameRate;
    uint32_t frameCount;
};

// 720x480 25fps 作为默认码流
static const Wmv1StreamInfo WMV1_STREAM_720X480_25 = {
    "/data/test/media/wmv1_720x480_25.wmv1", 720, 480, 25, 250};
static const Wmv1StreamInfo WMV1_STREAM_176X144_25 = {
    "/data/test/media/wmv1_176x144_25.wmv1", 176, 144, 25, 250};
static const Wmv1StreamInfo WMV1_STREAM_320X240_25 = {
    "/data/test/media/wmv1_320x240_25.wmv1", 320, 240, 25, 250};
static const Wmv1StreamInfo WMV1_STREAM_352X288_25 = {
    "/data/test/media/wmv1_352x288_25.wmv1", 352, 288, 25, 250};
static const Wmv1StreamInfo WMV1_STREAM_640X480_25 = {
    "/data/test/media/wmv1_640x480_25.wmv1", 640, 480, 25, 250};
static const Wmv1StreamInfo WMV1_STREAM_720X576_25 = {
    "/data/test/media/wmv1_720x576_25.wmv1", 720, 576, 25, 250};
static const Wmv1StreamInfo WMV1_STREAM_1280X720_25 = {
    "/data/test/media/wmv1_1280x720_25.wmv1", 1280, 720, 25, 250};
static const Wmv1StreamInfo WMV1_STREAM_1280X720_60 = {
    "/data/test/media/wmv1_1280x720_60.wmv1", 1280, 720, 60, 600};
static const Wmv1StreamInfo WMV1_STREAM_1920X1080_25 = {
    "/data/test/media/wmv1_1920x1080_25.wmv1", 1920, 1080, 25, 250};

void ApplyStream(const shared_ptr<VDecAPI11Sample> &sample, const Wmv1StreamInfo &stream)
{
    sample->INP_DIR = stream.path;
    sample->DEFAULT_WIDTH = stream.width;
    sample->DEFAULT_HEIGHT = stream.height;
    sample->DEFAULT_FRAME_RATE = stream.frameRate;
}
}  // namespace

void Wmv1SwdecFuncNdkTest::SetUpTestCase()
{
    cap_wmv1 = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_WMV1, false, SOFTWARE);
    if (cap_wmv1 == nullptr) {
        cout << "wmv1 decoder capability is null" << endl;
        return;
    }
    const char *codecName = OH_AVCapability_GetName(cap_wmv1);
    if (codecName != nullptr) {
        g_codecNameWmv1 = codecName;
    }
    cout << "g_codecNameWmv1: " << g_codecNameWmv1 << endl;
}

void Wmv1SwdecFuncNdkTest::TearDownTestCase()
{
}
void Wmv1SwdecFuncNdkTest::SetUp()
{
}
void Wmv1SwdecFuncNdkTest::TearDown()
{
}

namespace {
/**
 * @tc.number    : VIDEO_WMV1SWDEC_FUNCTION_0010
 * @tc.name      : decode WMV1 buffer,PixelFormat,NV12
 * @tc.desc      : function test
 */
HWTEST_F(Wmv1SwdecFuncNdkTest, VIDEO_WMV1SWDEC_FUNCTION_0010, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    ApplyStream(vDecSample, WMV1_STREAM_720X480_25);
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
    vDecSample->needMdCompare = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.WMV1"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(WMV1_STREAM_720X480_25.frameCount, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_WMV1SWDEC_FUNCTION_0020
 * @tc.name      : decode WMV1 buffer,PixelFormat,NV21
 * @tc.desc      : function test
 */
HWTEST_F(Wmv1SwdecFuncNdkTest, VIDEO_WMV1SWDEC_FUNCTION_0020, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    ApplyStream(vDecSample, WMV1_STREAM_720X480_25);
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV21;
    vDecSample->needMdCompare = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.WMV1"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(WMV1_STREAM_720X480_25.frameCount, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_WMV1SWDEC_FUNCTION_0030
 * @tc.name      : decode WMV1 buffer,PixelFormat,I420
 * @tc.desc      : function test
 */
HWTEST_F(Wmv1SwdecFuncNdkTest, VIDEO_WMV1SWDEC_FUNCTION_0030, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    ApplyStream(vDecSample, WMV1_STREAM_720X480_25);
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_YUVI420;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.WMV1"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(WMV1_STREAM_720X480_25.frameCount, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_WMV1SWDEC_FUNCTION_0040
 * @tc.name      : decode WMV1 buffer,PixelFormat,RGBA
 * @tc.desc      : function test
 */
HWTEST_F(Wmv1SwdecFuncNdkTest, VIDEO_WMV1SWDEC_FUNCTION_0040, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    ApplyStream(vDecSample, WMV1_STREAM_720X480_25);
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_RGBA;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.WMV1"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(WMV1_STREAM_720X480_25.frameCount, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_WMV1SWDEC_FUNCTION_0050
 * @tc.name      : decode WMV1 buffer without dump, 720x480 25fps
 * @tc.desc      : function test
 */
HWTEST_F(Wmv1SwdecFuncNdkTest, VIDEO_WMV1SWDEC_FUNCTION_0050, TestSize.Level1)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    ApplyStream(vDecSample, WMV1_STREAM_720X480_25);
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameWmv1));
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(WMV1_STREAM_720X480_25.frameCount, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_WMV1SWDEC_FUNCTION_0060
 * @tc.name      : decode WMV1 surface, 720x480 25fps
 * @tc.desc      : function test
 */
HWTEST_F(Wmv1SwdecFuncNdkTest, VIDEO_WMV1SWDEC_FUNCTION_0060, TestSize.Level1)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    ApplyStream(vDecSample, WMV1_STREAM_720X480_25);
    vDecSample->SF_OUTPUT = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameWmv1));
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(WMV1_STREAM_720X480_25.frameCount, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_WMV1SWDEC_FUNCTION_0070
 * @tc.name      : decode WMV1 buffer,input h263 error
 * @tc.desc      : function test
 */
HWTEST_F(Wmv1SwdecFuncNdkTest, VIDEO_WMV1SWDEC_FUNCTION_0070, TestSize.Level3)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = "/data/test/media/profile2_1920x1080.h263";
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    vDecSample->DEFAULT_FRAME_RATE = 24;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.WMV1"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_UNKNOWN, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(false, vDecSample->isonError);
}

/**
 * @tc.number    : VIDEO_WMV1SWDEC_FUNCTION_0080
 * @tc.name      : decode WMV1 surface,nv12, 720x480 25fps
 * @tc.desc      : function test
 */
HWTEST_F(Wmv1SwdecFuncNdkTest, VIDEO_WMV1SWDEC_FUNCTION_0080, TestSize.Level3)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    ApplyStream(vDecSample, WMV1_STREAM_720X480_25);
    vDecSample->SF_OUTPUT = true;
    vDecSample->outputYuvSurface = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameWmv1));
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(WMV1_STREAM_720X480_25.frameCount, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_WMV1SWDEC_FUNCTION_0090
 * @tc.name      : decode WMV1 surface,nv21, 720x480 25fps
 * @tc.desc      : function test
 */
HWTEST_F(Wmv1SwdecFuncNdkTest, VIDEO_WMV1SWDEC_FUNCTION_0090, TestSize.Level3)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    ApplyStream(vDecSample, WMV1_STREAM_720X480_25);
    vDecSample->SF_OUTPUT = true;
    vDecSample->outputYuvSurface = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV21;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameWmv1));
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(WMV1_STREAM_720X480_25.frameCount, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_WMV1SWDEC_FUNCTION_0100
 * @tc.name      : decode WMV1 surface, default pixel format, 720x480 25fps
 * @tc.desc      : function test
 */
HWTEST_F(Wmv1SwdecFuncNdkTest, VIDEO_WMV1SWDEC_FUNCTION_0100, TestSize.Level3)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    ApplyStream(vDecSample, WMV1_STREAM_720X480_25);
    vDecSample->SF_OUTPUT = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameWmv1));
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(WMV1_STREAM_720X480_25.frameCount, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_WMV1SWDEC_FUNCTION_0110
 * @tc.name      : decode WMV1 buffer, eos before all frames pushed
 * @tc.desc      : function test
 */
HWTEST_F(Wmv1SwdecFuncNdkTest, VIDEO_WMV1SWDEC_FUNCTION_0110, TestSize.Level3)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    ApplyStream(vDecSample, WMV1_STREAM_720X480_25);
    vDecSample->BEFORE_EOS_INPUT = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameWmv1));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_TRUE(vDecSample->outFrameCount > 0);
    ASSERT_TRUE(vDecSample->outFrameCount < WMV1_STREAM_720X480_25.frameCount);
}

/**
 * @tc.number    : VIDEO_DECODE_WMV1_RESOLUTION_0010
 * @tc.name      : wmv1分辨率176x144，buffer, nv12
 * @tc.desc      : function test
 */
HWTEST_F(Wmv1SwdecFuncNdkTest, VIDEO_DECODE_WMV1_RESOLUTION_0010, TestSize.Level1)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    ApplyStream(vDecSample, WMV1_STREAM_176X144_25);
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
    vDecSample->needMdCompare = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameWmv1));
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(WMV1_STREAM_176X144_25.frameCount, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_DECODE_WMV1_RESOLUTION_0020
 * @tc.name      : wmv1分辨率320x240，buffer, nv21
 * @tc.desc      : function test
 */
HWTEST_F(Wmv1SwdecFuncNdkTest, VIDEO_DECODE_WMV1_RESOLUTION_0020, TestSize.Level1)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    ApplyStream(vDecSample, WMV1_STREAM_320X240_25);
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV21;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameWmv1));
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(WMV1_STREAM_320X240_25.frameCount, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_DECODE_WMV1_RESOLUTION_0030
 * @tc.name      : wmv1分辨率352x288，surface, nv12
 * @tc.desc      : function test
 */
HWTEST_F(Wmv1SwdecFuncNdkTest, VIDEO_DECODE_WMV1_RESOLUTION_0030, TestSize.Level1)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    ApplyStream(vDecSample, WMV1_STREAM_352X288_25);
    vDecSample->SF_OUTPUT = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameWmv1));
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(WMV1_STREAM_352X288_25.frameCount, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_DECODE_WMV1_RESOLUTION_0040
 * @tc.name      : wmv1分辨率640x480，buffer, i420
 * @tc.desc      : function test
 */
HWTEST_F(Wmv1SwdecFuncNdkTest, VIDEO_DECODE_WMV1_RESOLUTION_0040, TestSize.Level1)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    ApplyStream(vDecSample, WMV1_STREAM_640X480_25);
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_YUVI420;
    vDecSample->needMdCompare = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameWmv1));
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(WMV1_STREAM_640X480_25.frameCount, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_DECODE_WMV1_RESOLUTION_0050
 * @tc.name      : wmv1分辨率720x576，buffer, nv12
 * @tc.desc      : function test
 */
HWTEST_F(Wmv1SwdecFuncNdkTest, VIDEO_DECODE_WMV1_RESOLUTION_0050, TestSize.Level1)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    ApplyStream(vDecSample, WMV1_STREAM_720X576_25);
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameWmv1));
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(WMV1_STREAM_720X576_25.frameCount, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_DECODE_WMV1_RESOLUTION_0060
 * @tc.name      : wmv1分辨率1280x720，buffer, nv12
 * @tc.desc      : function test
 */
HWTEST_F(Wmv1SwdecFuncNdkTest, VIDEO_DECODE_WMV1_RESOLUTION_0060, TestSize.Level2)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    ApplyStream(vDecSample, WMV1_STREAM_1280X720_25);
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameWmv1));
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(WMV1_STREAM_1280X720_25.frameCount, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_DECODE_WMV1_RESOLUTION_0070
 * @tc.name      : wmv1分辨率1280x720@60fps，buffer, nv12
 * @tc.desc      : function test
 */
HWTEST_F(Wmv1SwdecFuncNdkTest, VIDEO_DECODE_WMV1_RESOLUTION_0070, TestSize.Level2)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    ApplyStream(vDecSample, WMV1_STREAM_1280X720_60);
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameWmv1));
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(WMV1_STREAM_1280X720_60.frameCount, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_DECODE_WMV1_RESOLUTION_0080
 * @tc.name      : wmv1分辨率1920x1080，buffer, nv12
 * @tc.desc      : function test
 */
HWTEST_F(Wmv1SwdecFuncNdkTest, VIDEO_DECODE_WMV1_RESOLUTION_0080, TestSize.Level2)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    ApplyStream(vDecSample, WMV1_STREAM_1920X1080_25);
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameWmv1));
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(WMV1_STREAM_1920X1080_25.frameCount, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_DECODE_WMV1_RESOLUTION_0090
 * @tc.name      : wmv1分辨率1920x1080，surface, nv21
 * @tc.desc      : function test
 */
HWTEST_F(Wmv1SwdecFuncNdkTest, VIDEO_DECODE_WMV1_RESOLUTION_0090, TestSize.Level2)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    ApplyStream(vDecSample, WMV1_STREAM_1920X1080_25);
    vDecSample->SF_OUTPUT = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV21;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameWmv1));
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(WMV1_STREAM_1920X1080_25.frameCount, vDecSample->outFrameCount);
}

/**
 * @tc.number    : API11_SURF_CHANGE_FUNC_001
 * @tc.name      : surf model change in normal state
 * @tc.desc      : function test
 */
HWTEST_F(Wmv1SwdecFuncNdkTest, API11_SURF_CHANGE_FUNC_001, TestSize.Level2)
{
    if (ONLY_FOR_FLAGSHIP_CHIP) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        ApplyStream(vDecSample, WMV1_STREAM_720X480_25);
        vDecSample->SURFACE_OUTPUT = true;
        vDecSample->autoSwitchSurface = true;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameWmv1));
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
HWTEST_F(Wmv1SwdecFuncNdkTest, API11_SURF_CHANGE_FUNC_002, TestSize.Level2)
{
    if (ONLY_FOR_FLAGSHIP_CHIP) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        ApplyStream(vDecSample, WMV1_STREAM_720X480_25);
        vDecSample->SURFACE_OUTPUT = true;
        vDecSample->autoSwitchSurface = true;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameWmv1));
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
HWTEST_F(Wmv1SwdecFuncNdkTest, API11_SURF_CHANGE_FUNC_003, TestSize.Level2)
{
    if (ONLY_FOR_FLAGSHIP_CHIP) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        ApplyStream(vDecSample, WMV1_STREAM_720X480_25);
        vDecSample->SURFACE_OUTPUT = false;
        vDecSample->autoSwitchSurface = false;
        vDecSample->CreateSurface();
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameWmv1));
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
HWTEST_F(Wmv1SwdecFuncNdkTest, API11_SURF_CHANGE_FUNC_004, TestSize.Level2)
{
    if (ONLY_FOR_FLAGSHIP_CHIP) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        ApplyStream(vDecSample, WMV1_STREAM_720X480_25);
        vDecSample->SURFACE_OUTPUT = true;
        vDecSample->autoSwitchSurface = true;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameWmv1));
        ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
        vDecSample->WaitForEOS();
    }
}

/**
 * @tc.number    : API11_SURF_CHANGE_FUNC_005
 * @tc.name      : surf model change in flush to runnig state
 * @tc.desc      : function test
 */
HWTEST_F(Wmv1SwdecFuncNdkTest, API11_SURF_CHANGE_FUNC_005, TestSize.Level2)
{
    if (ONLY_FOR_FLAGSHIP_CHIP) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        ApplyStream(vDecSample, WMV1_STREAM_720X480_25);
        vDecSample->SURFACE_OUTPUT = true;
        vDecSample->autoSwitchSurface = true;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameWmv1));
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
HWTEST_F(Wmv1SwdecFuncNdkTest, API11_SURF_CHANGE_FUNC_006, TestSize.Level2)
{
    if (ONLY_FOR_FLAGSHIP_CHIP) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        ApplyStream(vDecSample, WMV1_STREAM_720X480_25);
        vDecSample->SURFACE_OUTPUT = true;
        vDecSample->autoSwitchSurface = true;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameWmv1));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
    }
}

/**
 * @tc.number    : API11_SURF_CHANGE_FUNC_007
 * @tc.name      : buffer model change in decoder finish to End-of-Stream state
 * @tc.desc      : function test
 */
HWTEST_F(Wmv1SwdecFuncNdkTest, API11_SURF_CHANGE_FUNC_007, TestSize.Level2)
{
    if (ONLY_FOR_FLAGSHIP_CHIP) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        ApplyStream(vDecSample, WMV1_STREAM_720X480_25);
        vDecSample->SURFACE_OUTPUT = false;
        vDecSample->autoSwitchSurface = false;
        vDecSample->CreateSurface();
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameWmv1));
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
HWTEST_F(Wmv1SwdecFuncNdkTest, API11_SURF_CHANGE_FUNC_008, TestSize.Level2)
{
    if (ONLY_FOR_FLAGSHIP_CHIP) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        ApplyStream(vDecSample, WMV1_STREAM_720X480_25);
        vDecSample->SURFACE_OUTPUT = false;
        vDecSample->autoSwitchSurface = false;
        vDecSample->CreateSurface();
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameWmv1));
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
HWTEST_F(Wmv1SwdecFuncNdkTest, API11_SURF_CHANGE_FUNC_009, TestSize.Level2)
{
    if (ONLY_FOR_FLAGSHIP_CHIP) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        ApplyStream(vDecSample, WMV1_STREAM_720X480_25);
        vDecSample->SURFACE_OUTPUT = false;
        vDecSample->autoSwitchSurface = false;
        vDecSample->CreateSurface();
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameWmv1));
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
HWTEST_F(Wmv1SwdecFuncNdkTest, API11_SURF_CHANGE_FUNC_010, TestSize.Level2)
{
    if (ONLY_FOR_FLAGSHIP_CHIP) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        ApplyStream(vDecSample, WMV1_STREAM_720X480_25);
        vDecSample->SURFACE_OUTPUT = false;
        vDecSample->autoSwitchSurface = false;
        vDecSample->CreateSurface();
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameWmv1));
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
HWTEST_F(Wmv1SwdecFuncNdkTest, API11_SURF_CHANGE_FUNC_011, TestSize.Level1)
{
    if (ONLY_FOR_FLAGSHIP_CHIP) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        ApplyStream(vDecSample, WMV1_STREAM_720X480_25);
        vDecSample->SURFACE_OUTPUT = false;
        vDecSample->autoSwitchSurface = false;
        vDecSample->CreateSurface();
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameWmv1));
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
HWTEST_F(Wmv1SwdecFuncNdkTest, API11_SURF_CHANGE_FUNC_012, TestSize.Level1)
{
    if (ONLY_FOR_FLAGSHIP_CHIP) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        ApplyStream(vDecSample, WMV1_STREAM_720X480_25);
        vDecSample->SURFACE_OUTPUT = false;
        vDecSample->autoSwitchSurface = false;
        vDecSample->CreateSurface();
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameWmv1));
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
HWTEST_F(Wmv1SwdecFuncNdkTest, API11_SURF_CHANGE_FUNC_013, TestSize.Level2)
{
    if (ONLY_FOR_FLAGSHIP_CHIP) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        ApplyStream(vDecSample, WMV1_STREAM_720X480_25);
        vDecSample->SURFACE_OUTPUT = true;
        vDecSample->autoSwitchSurface = true;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameWmv1));
        ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
        vDecSample->WaitForEOS();

        auto vDecSample_1 = make_shared<VDecAPI11Sample>();
        ApplyStream(vDecSample_1, WMV1_STREAM_720X480_25);
        vDecSample_1->SURFACE_OUTPUT = true;
        vDecSample_1->autoSwitchSurface = true;
        vDecSample_1->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample_1->RunVideoDec_Surface(g_codecNameWmv1));
        ASSERT_EQ(AV_ERR_OK, vDecSample_1->SwitchSurface());
        vDecSample_1->WaitForEOS();
    }
}

/**
 * @tc.number    : API11_SURF_CHANGE_FUNC_014
 * @tc.name      : repeat call setSurface fastly 2 time
 * @tc.desc      : function test
 */
HWTEST_F(Wmv1SwdecFuncNdkTest, API11_SURF_CHANGE_FUNC_014, TestSize.Level2)
{
    if (ONLY_FOR_FLAGSHIP_CHIP) {
        for (int i = 0; i < 2; i++) {
            auto vDecSample = make_shared<VDecAPI11Sample>();
            ApplyStream(vDecSample, WMV1_STREAM_720X480_25);
            vDecSample->SURFACE_OUTPUT = true;
            vDecSample->autoSwitchSurface = true;
            vDecSample->sleepOnFPS = true;
            ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameWmv1));
            ASSERT_EQ(AV_ERR_OK, vDecSample->RepeatCallSetSurface());
            vDecSample->WaitForEOS();
        }
    }
}

/**
 * @tc.number    : VIDEO_DECODE_SYNC_SWWMV1_FUNC_0010
 * @tc.name      : wmv1同步软解输出nv12
 * @tc.desc      : function test
 */
HWTEST_F(Wmv1SwdecFuncNdkTest, VIDEO_DECODE_SYNC_SWWMV1_FUNC_0010, TestSize.Level1)
{
    if (cap_wmv1 != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        ApplyStream(vDecSample, WMV1_STREAM_720X480_25);
        vDecSample->enbleSyncMode = 1;
        vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameWmv1));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartSyncVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
        ASSERT_EQ(WMV1_STREAM_720X480_25.frameCount, vDecSample->outFrameCount);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_SYNC_SWWMV1_FUNC_0020
 * @tc.name      : wmv1同步软解输出nv21
 * @tc.desc      : function test
 */
HWTEST_F(Wmv1SwdecFuncNdkTest, VIDEO_DECODE_SYNC_SWWMV1_FUNC_0020, TestSize.Level0)
{
    if (cap_wmv1 != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        ApplyStream(vDecSample, WMV1_STREAM_720X480_25);
        vDecSample->enbleSyncMode = 1;
        vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV21;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameWmv1));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartSyncVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
        ASSERT_EQ(WMV1_STREAM_720X480_25.frameCount, vDecSample->outFrameCount);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_SYNC_SWWMV1_FUNC_0030
 * @tc.name      : wmv1同步软解输出surface
 * @tc.desc      : function test
 */
HWTEST_F(Wmv1SwdecFuncNdkTest, VIDEO_DECODE_SYNC_SWWMV1_FUNC_0030, TestSize.Level1)
{
    if (cap_wmv1 != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        ApplyStream(vDecSample, WMV1_STREAM_1280X720_25);
        vDecSample->SF_OUTPUT = true;
        vDecSample->enbleSyncMode = 1;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameWmv1));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->DecodeSetSurface());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartSyncVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
        ASSERT_EQ(WMV1_STREAM_1280X720_25.frameCount, vDecSample->outFrameCount);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_SYNC_SWWMV1_FUNC_0040
 * @tc.name      : wmv1同步软解输出rgba
 * @tc.desc      : function test
 */
HWTEST_F(Wmv1SwdecFuncNdkTest, VIDEO_DECODE_SYNC_SWWMV1_FUNC_0040, TestSize.Level1)
{
    if (cap_wmv1 != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        ApplyStream(vDecSample, WMV1_STREAM_720X480_25);
        vDecSample->enbleSyncMode = 1;
        vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_RGBA;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameWmv1));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartSyncVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
        ASSERT_EQ(WMV1_STREAM_720X480_25.frameCount, vDecSample->outFrameCount);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_SWWMV1_BLANK_FRAME_0010
 * @tc.name      : config OH_MD_KEY_VIDEO_DECODER_BLANK_FRAME_ON_SHUTDOWN, decoder wmv1
 * @tc.desc      : function test
 */
HWTEST_F(Wmv1SwdecFuncNdkTest, VIDEO_DECODE_SWWMV1_BLANK_FRAME_0010, TestSize.Level2)
{
    if (cap_wmv1 != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        ApplyStream(vDecSample, WMV1_STREAM_720X480_25);
        vDecSample->enbleBlankFrame = 1;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.WMV1"));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
        ASSERT_EQ(WMV1_STREAM_720X480_25.frameCount, vDecSample->outFrameCount);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_SWWMV1_BLANK_FRAME_0020
 * @tc.name      : config OH_MD_KEY_VIDEO_DECODER_BLANK_FRAME_ON_SHUTDOWN, decoder wmv1, surface
 * @tc.desc      : function test
 */
HWTEST_F(Wmv1SwdecFuncNdkTest, VIDEO_DECODE_SWWMV1_BLANK_FRAME_0020, TestSize.Level2)
{
    if (cap_wmv1 != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        ApplyStream(vDecSample, WMV1_STREAM_720X480_25);
        vDecSample->SURFACE_OUTPUT = true;
        vDecSample->enbleBlankFrame = 1;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.WMV1"));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->DecodeSetSurface());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
        ASSERT_EQ(WMV1_STREAM_720X480_25.frameCount, vDecSample->outFrameCount);
    }
}

/**
 * @tc.number    : VIDEO_WMV1SWDEC_FUNCTION_GRAPH_0010
 * @tc.name      : decode WMV1 buffer,graph PixelFormat
 * @tc.desc      : function test
 */
HWTEST_F(Wmv1SwdecFuncNdkTest, VIDEO_WMV1SWDEC_FUNCTION_GRAPH_0010, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    int32_t pixfmt[4] = {28, 24, 25, 12};
    ApplyStream(vDecSample, WMV1_STREAM_720X480_25);
    vDecSample->isGetVideoSupportedPixelFormats = true;
    vDecSample->isGetFormatKey = true;
    vDecSample->avcodecMimeType = OH_AVCODEC_MIMETYPE_VIDEO_WMV1;
    vDecSample->isEncoder = false;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.WMV1"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(4, vDecSample->pixlFormatNum);
    for (int i = 0; i < vDecSample->pixlFormatNum; i++) {
        ASSERT_EQ(vDecSample->pixlFormats[i], pixfmt[i]);
    }
    ASSERT_EQ(WMV1_STREAM_720X480_25.frameCount, vDecSample->outFrameCount);
}

}  // namespace
