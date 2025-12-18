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

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;

namespace OHOS {
namespace Media {
class DVVideoSwdecFuncNdkTest : public testing::Test {
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
    const char *INP_DIR_0 = "/data/test/media/dvvideo_720x576_yuv420p_25_25frames.avi";
    const char *INP_DIR_1 = "/data/test/media/dvvideo_720x576_yuv422p_25_25frames.avi";
    const char *INP_DIR_2 = "/data/test/media/dvvideo_720x576_yuv411p_25_25frames.avi";
    const char *INP_DIR_3 = "/data/test/media/dvvideo_720x480_yuv422p_30_25frames.avi";
    const char *INP_DIR_4 = "/data/test/media/dvvideo_720x480_yuv411p_30_25frames.avi";
    const char *INP_DIR_5 = "/data/test/media/dvvideo_960x720_yuv422p_50_25frames.avi";
    const char *INP_DIR_6 = "/data/test/media/dvvideo_960x720_yuv422p_60_25frames.avi";
    const char *INP_DIR_7 = "/data/test/media/dvvideo_1280x1080_yuv422p_30_25frames.avi";
    const char *INP_DIR_8 = "/data/test/media/dvvideo_1440x1080_yuv422p_25_25frames.avi";
    const char *INP_DIR_9 = "/data/test/media/dvvideo_720x480_yuv411p_30_25frames.mkv";
    const char *INP_DIR_10 = "/data/test/media/dvvideo_720x480_yuv411p_30_25frames.mov";
};
}  // namespace Media
}  // namespace OHOS

namespace {
static OH_AVCapability *cap_dvvideo = nullptr;
static string g_codecNameDVVideo = "";
static OH_AVCapability *cap_264 = nullptr;
static string g_codecName264 = "";
constexpr uint32_t FRAMESIZE25 = 25;

}  // namespace

void DVVideoSwdecFuncNdkTest::SetUpTestCase()
{
    cap_dvvideo = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_DVVIDEO, false, SOFTWARE);
    g_codecNameDVVideo = OH_AVCapability_GetName(cap_dvvideo);
    cout << "g_codecNameDVVideo: " << g_codecNameDVVideo << endl;
    cap_264 = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_AVC, false, SOFTWARE);
    g_codecName264 = OH_AVCapability_GetName(cap_264);
    cout << "g_codecName264: " << g_codecName264 << endl;
}

void DVVideoSwdecFuncNdkTest::TearDownTestCase()
{
}
void DVVideoSwdecFuncNdkTest::SetUp()
{
}
void DVVideoSwdecFuncNdkTest::TearDown()
{
}

namespace {

/**
 * @tc.number    : VIDEO_SWDEC_DVVIDEO_FUNCTION_0010
 * @tc.name      : decoder dvvideo,buffer,AsyncMode,dvvideo_720x576_yuv420p_25_25frames OutputPixelFormat:yuv420->NV12
 * @tc.desc      : function test
 */
HWTEST_F(DVVideoSwdecFuncNdkTest, VIDEO_SWDEC_DVVIDEO_FUNCTION_0010, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->DEFAULT_WIDTH = 720;
    vDecSample->DEFAULT_HEIGHT = 576;
    vDecSample->DEFAULT_FRAME_RATE = 25;
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
    vDecSample->OUT_DIR = "/data/test/media/dv_func_0010_720x576_25fps_nv12.yuv";
    ASSERT_EQ(AV_ERR_OK, vDecSample->LoadInputFile(INP_DIR_0));
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.DVVIDEO"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE25, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_SWDEC_DVVIDEO_FUNCTION_0020
 * @tc.name      : decoder dvvideo,buffer,AsyncMode,dvvideo_720x576_yuv420p_25_25frames OutputPixelFormat:yuv420->NV21
 * @tc.desc      : function test
 */
HWTEST_F(DVVideoSwdecFuncNdkTest, VIDEO_SWDEC_DVVIDEO_FUNCTION_0020, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->DEFAULT_WIDTH = 720;
    vDecSample->DEFAULT_HEIGHT = 576;
    vDecSample->DEFAULT_FRAME_RATE = 25;
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV21;
    vDecSample->OUT_DIR = "/data/test/media/dv_func_0020_720x576_25fps_nv21.yuv";
    ASSERT_EQ(AV_ERR_OK, vDecSample->LoadInputFile(INP_DIR_0));
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.DVVIDEO"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE25, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_SWDEC_DVVIDEO_FUNCTION_0030
 * @tc.name      : decoder dvvideo,buffer,AsyncMode,dvvideo_720x576_yuv420p_25_25frames OutputPixelFormat:yuv420->I420
 * @tc.desc      : function test
 */
HWTEST_F(DVVideoSwdecFuncNdkTest, VIDEO_SWDEC_DVVIDEO_FUNCTION_0030, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->DEFAULT_WIDTH = 720;
    vDecSample->DEFAULT_HEIGHT = 576;
    vDecSample->DEFAULT_FRAME_RATE = 25;
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_YUVI420;
    vDecSample->OUT_DIR = "/data/test/media/dv_func_0030_720x576_25fps_I420.yuv";
    ASSERT_EQ(AV_ERR_OK, vDecSample->LoadInputFile(INP_DIR_0));
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.DVVIDEO"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE25, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_SWDEC_DVVIDEO_FUNCTION_0040
 * @tc.name      : decoder dvvideo,buffer,AsyncMode,dvvideo_720x576_yuv420p_25_25frames OutputPixelFormat:yuv420->RGBA
 * @tc.desc      : function test
 */
HWTEST_F(DVVideoSwdecFuncNdkTest, VIDEO_SWDEC_DVVIDEO_FUNCTION_0040, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->DEFAULT_WIDTH = 720;
    vDecSample->DEFAULT_HEIGHT = 576;
    vDecSample->DEFAULT_FRAME_RATE = 25;
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_RGBA;
    vDecSample->OUT_DIR = "/data/test/media/dv_func_0040_720x576_25fps_rgba.rgba";
    ASSERT_EQ(AV_ERR_OK, vDecSample->LoadInputFile(INP_DIR_0));
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.DVVIDEO"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE25, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_SWDEC_DVVIDEO_FUNCTION_0050
 * @tc.name      : decoder dvvideo,buffer,AsyncMode,dvvideo_720x576_yuv422p_25_25frames OutputPixelFormat:yuv422p->NV12
 * @tc.desc      : function test
 */
HWTEST_F(DVVideoSwdecFuncNdkTest, VIDEO_SWDEC_DVVIDEO_FUNCTION_0050, TestSize.Level1)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->DEFAULT_WIDTH = 720;
    vDecSample->DEFAULT_HEIGHT = 576;
    vDecSample->DEFAULT_FRAME_RATE = 25;
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
    vDecSample->OUT_DIR = "/data/test/media/dv_func_0050_720x576_25fps_nv12.yuv";
    ASSERT_EQ(AV_ERR_OK, vDecSample->LoadInputFile(INP_DIR_1));
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.DVVIDEO"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE25, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_SWDEC_DVVIDEO_FUNCTION_0060
 * @tc.name      : decode dvvideo buffer,nv21,dvvideo_720x576_yuv411p_25_25frames
 * @tc.desc      : function test
 */
HWTEST_F(DVVideoSwdecFuncNdkTest, VIDEO_SWDEC_DVVIDEO_FUNCTION_0060, TestSize.Level1)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->DEFAULT_WIDTH = 720;
    vDecSample->DEFAULT_HEIGHT = 576;
    vDecSample->DEFAULT_FRAME_RATE = 25;
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV21;
    vDecSample->OUT_DIR = "/data/test/media/dv_func_0060_720x576_25fps_nv21.yuv";
    ASSERT_EQ(AV_ERR_OK, vDecSample->LoadInputFile(INP_DIR_2));
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.DVVIDEO"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE25, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_SWDEC_DVVIDEO_FUNCTION_0070
 * @tc.name      : decoder dvvideo,buffer,AsyncMode,dvvideo_720x480_yuv422p_30_25frames OutputPixelFormat:yuv422p->NV12
 * @tc.desc      : function test
 */
HWTEST_F(DVVideoSwdecFuncNdkTest, VIDEO_SWDEC_DVVIDEO_FUNCTION_0070, TestSize.Level1)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->DEFAULT_WIDTH = 720;
    vDecSample->DEFAULT_HEIGHT = 480;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
    vDecSample->OUT_DIR = "/data/test/media/dv_func_0070_720x480_30fps_nv12.yuv";
    ASSERT_EQ(AV_ERR_OK, vDecSample->LoadInputFile(INP_DIR_3));
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.DVVIDEO"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE25, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_SWDEC_DVVIDEO_FUNCTION_0080
 * @tc.name      : decoder dvvideo,buffer,AsyncMode,dvvideo_720x480_yuv411p_30_25frames  OutputPixelFormat:yuv411p->NV12
 * @tc.desc      : function test
 */
HWTEST_F(DVVideoSwdecFuncNdkTest, VIDEO_SWDEC_DVVIDEO_FUNCTION_0080, TestSize.Level1)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->DEFAULT_WIDTH = 720;
    vDecSample->DEFAULT_HEIGHT = 480;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
    vDecSample->OUT_DIR = "/data/test/media/dv_func_0080_720x480_30fps_nv12.yuv";
    ASSERT_EQ(AV_ERR_OK, vDecSample->LoadInputFile(INP_DIR_4));
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.DVVIDEO"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE25, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_SWDEC_DVVIDEO_FUNCTION_0090
 * @tc.name      : decoder dvvideo,buffer,AsyncMode,dvvideo_960x720_yuv422p_50_25frames OutputPixelFormat:yuv422p->NV12
 * @tc.desc      : function test
 */
HWTEST_F(DVVideoSwdecFuncNdkTest, VIDEO_SWDEC_DVVIDEO_FUNCTION_0090, TestSize.Level1)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->DEFAULT_WIDTH = 960;
    vDecSample->DEFAULT_HEIGHT = 720;
    vDecSample->DEFAULT_FRAME_RATE = 50;
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
    vDecSample->OUT_DIR = "/data/test/media/dv_func_0090_960x720_50fps_nv12.yuv";
    ASSERT_EQ(AV_ERR_OK, vDecSample->LoadInputFile(INP_DIR_5));
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.DVVIDEO"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE25, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_SWDEC_DVVIDEO_FUNCTION_0100
 * @tc.name      : decoder dvvideo,buffer,AsyncMode,dvvideo_960x720_yuv422p_60_25frames  OutputPixelFormat:yuv422p->NV12
 * @tc.desc      : function test
 */
HWTEST_F(DVVideoSwdecFuncNdkTest, VIDEO_SWDEC_DVVIDEO_FUNCTION_0100, TestSize.Level1)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->DEFAULT_WIDTH = 960;
    vDecSample->DEFAULT_HEIGHT = 720;
    vDecSample->DEFAULT_FRAME_RATE = 60;
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
    vDecSample->OUT_DIR = "/data/test/media/dv_func_0100_960x720_60fps_nv12.yuv";
    ASSERT_EQ(AV_ERR_OK, vDecSample->LoadInputFile(INP_DIR_6));
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.DVVIDEO"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE25, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_SWDEC_DVVIDEO_FUNCTION_0110
 * @tc.name      : decoder dvvideo,buffer,AsyncMode,dvvideo_1280_1080_yuv422p_25frames OutputPixelFormat:yuv422p->NV12
 * @tc.desc      : function test
 */
HWTEST_F(DVVideoSwdecFuncNdkTest, VIDEO_SWDEC_DVVIDEO_FUNCTION_0110, TestSize.Level1)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->DEFAULT_WIDTH = 1280;
    vDecSample->DEFAULT_HEIGHT = 1080;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
    vDecSample->OUT_DIR = "/data/test/media/dv_func_0110_1280x1080_30fps_nv12.yuv";
    ASSERT_EQ(AV_ERR_OK, vDecSample->LoadInputFile(INP_DIR_7));
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.DVVIDEO"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE25, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_SWDEC_DVVIDEO_FUNCTION_0120
 * @tc.name      : decoder dvvideo,buffer,AsyncMode,dvvideo_1440_1080_yuv422p_25frames OutputPixelFormat:yuv422p->NV12
 * @tc.desc      : function test
 */
HWTEST_F(DVVideoSwdecFuncNdkTest, VIDEO_SWDEC_DVVIDEO_FUNCTION_0120, TestSize.Level1)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->DEFAULT_WIDTH = 1440;
    vDecSample->DEFAULT_HEIGHT = 1080;
    vDecSample->DEFAULT_FRAME_RATE = 25;
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
    vDecSample->OUT_DIR = "/data/test/media/dv_func_0120_1440x1080_25fps_nv12.yuv";
    ASSERT_EQ(AV_ERR_OK, vDecSample->LoadInputFile(INP_DIR_8));
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.DVVIDEO"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE25, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_SWDEC_DVVIDEO_FUNCTION_0130
 * @tc.name      : decoder dvvideo,surface,AsyncMode,dvvideo_720_480_yuv411p_29fps OutputPixelFormat yuv411p->NV12
 * @tc.desc      : function test
 */
HWTEST_F(DVVideoSwdecFuncNdkTest, VIDEO_SWDEC_DVVIDEO_FUNCTION_0130, TestSize.Level1)
{
    if (cap_dvvideo != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->DEFAULT_WIDTH = 720;
        vDecSample->DEFAULT_HEIGHT = 480;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->outputYuvSurface = true;
        vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
        vDecSample->OUT_DIR = "/data/test/media/dv_func_surf_0130_720x480_30fps_nv12.yuv";
        ASSERT_EQ(AV_ERR_OK, vDecSample->LoadInputFile(INP_DIR_4));
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameDVVideo));
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
        ASSERT_EQ(FRAMESIZE25, vDecSample->outFrameCount);
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_DVVIDEO_FUNCTION_0140
 * @tc.name      : decode dvvideo surface,AsyncMode,dvvideo_720_576_yuv422p_25fps OutputPixelFormat yuv422p-> NV12
 * @tc.desc      : function test
 */
HWTEST_F(DVVideoSwdecFuncNdkTest, VIDEO_SWDEC_DVVIDEO_FUNCTION_0140, TestSize.Level1)
{
    if (cap_dvvideo != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->DEFAULT_WIDTH = 720;
        vDecSample->DEFAULT_HEIGHT = 576;
        vDecSample->DEFAULT_FRAME_RATE = 25;
        vDecSample->outputYuvSurface = true;
        vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
        vDecSample->OUT_DIR = "/data/test/media/dv_func_surf_0140_720x576_25fps_nv12.yuv";
        ASSERT_EQ(AV_ERR_OK, vDecSample->LoadInputFile(INP_DIR_1));
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameDVVideo));
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
        ASSERT_EQ(FRAMESIZE25, vDecSample->outFrameCount);
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_DVVIDEO_FUNCTION_0150
 * @tc.name      : decode dvvideo surface,AsyncMode,dvvideo_960_720_yuv422p_59fps OutputPixelFormat yuv422p-> NV12
 * @tc.desc      : function test
 */
HWTEST_F(DVVideoSwdecFuncNdkTest, VIDEO_SWDEC_DVVIDEO_FUNCTION_0150, TestSize.Level1)
{
    if (cap_dvvideo != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->DEFAULT_WIDTH = 960;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 60;
        vDecSample->outputYuvSurface = true;
        vDecSample->sleepOnFPS = true;
        vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
        vDecSample->OUT_DIR = "/data/test/media/dv_func_surf_0150_960x720_60fps_nv12.yuv";
        ASSERT_EQ(AV_ERR_OK, vDecSample->LoadInputFile(INP_DIR_6));
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameDVVideo));
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
        ASSERT_EQ(FRAMESIZE25, vDecSample->outFrameCount);
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_DVVIDEO_FUNCTION_0160
 * @tc.name      : decode dvvideo surface,AsyncMode,dvvideo_1280_1080_yuv422p_30fps OutputPixelFormat yuv422p-> NV12
 * @tc.desc      : function test
 */
HWTEST_F(DVVideoSwdecFuncNdkTest, VIDEO_SWDEC_DVVIDEO_FUNCTION_0160, TestSize.Level1)
{
    if (cap_dvvideo != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->outputYuvSurface = true;
        vDecSample->sleepOnFPS = true;
        vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
        vDecSample->OUT_DIR = "/data/test/media/dv_func_surf_0160_1280x1080_30fps_nv12.yuv";
        ASSERT_EQ(AV_ERR_OK, vDecSample->LoadInputFile(INP_DIR_7));
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameDVVideo));
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
        ASSERT_EQ(FRAMESIZE25, vDecSample->outFrameCount);
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_DVVIDEO_FUNCTION_0170
 * @tc.name      : decode dvvideo surface,AsyncMode,dvvideo_1440_1080_yuv422p_30fps OutputPixelFormat yuv422p->NV12
 * @tc.desc      : function test
 */
HWTEST_F(DVVideoSwdecFuncNdkTest, VIDEO_SWDEC_DVVIDEO_FUNCTION_0170, TestSize.Level1)
{
    if (cap_dvvideo != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->DEFAULT_WIDTH = 1440;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 25;
        vDecSample->outputYuvSurface = true;
        vDecSample->sleepOnFPS = true;
        vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
        vDecSample->OUT_DIR = "/data/test/media/dv_func_surf_0170_1440x1080_25fps_nv12.yuv";
        ASSERT_EQ(AV_ERR_OK, vDecSample->LoadInputFile(INP_DIR_8));
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameDVVideo));
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
        ASSERT_EQ(FRAMESIZE25, vDecSample->outFrameCount);
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_DVVIDEO_FUNCTION_0180
 * @tc.name      : decode dvvideo buffer,PixelFormat,NV12,mkv
 * @tc.desc      : function test
 */
HWTEST_F(DVVideoSwdecFuncNdkTest, VIDEO_SWDEC_DVVIDEO_FUNCTION_0180, TestSize.Level1)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->DEFAULT_WIDTH = 720;
    vDecSample->DEFAULT_HEIGHT = 480;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
    vDecSample->OUT_DIR = "/data/test/media/dv_func_0180_720x480_30fps_nv12.yuv";
    ASSERT_EQ(AV_ERR_OK, vDecSample->LoadInputFile(INP_DIR_9));
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.DVVIDEO"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE25, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_SWDEC_DVVIDEO_FUNCTION_0190
 * @tc.name      : decode dvvideo buffer,PixelFormat,NV21,mov
 * @tc.desc      : function test
 */
HWTEST_F(DVVideoSwdecFuncNdkTest, VIDEO_SWDEC_DVVIDEO_FUNCTION_0190, TestSize.Level1)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->DEFAULT_WIDTH = 720;
    vDecSample->DEFAULT_HEIGHT = 480;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV21;
    vDecSample->OUT_DIR = "/data/test/media/dv_func_0190_720x480_30fps_nv21.yuv";
    ASSERT_EQ(AV_ERR_OK, vDecSample->LoadInputFile(INP_DIR_10));
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.DVVIDEO"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE25, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_SWDEC_DVVIDEO_FUNCTION_0200
 * @tc.name      : decode dvvideo buffer,PixelFormat,NV21,avi
 * @tc.desc      : function test
 */
HWTEST_F(DVVideoSwdecFuncNdkTest, VIDEO_SWDEC_DVVIDEO_FUNCTION_0200, TestSize.Level1)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->DEFAULT_WIDTH = 720;
    vDecSample->DEFAULT_HEIGHT = 480;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV21;
    vDecSample->OUT_DIR = "/data/test/media/dv_func_0200_720x480_30fps_nv21.yuv";
    ASSERT_EQ(AV_ERR_OK, vDecSample->LoadInputFile(INP_DIR_4));
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.DVVIDEO"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE25, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_SWDEC_DVVIDEO_FUNCTION_0210
 * @tc.name      : decode H263 buffer and handle error
 * @tc.desc      : function test
 */
HWTEST_F(DVVideoSwdecFuncNdkTest, VIDEO_SWDEC_DVVIDEO_FUNCTION_0210, TestSize.Level2)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->DEFAULT_WIDTH = 1280;
    vDecSample->DEFAULT_HEIGHT = 720;
    vDecSample->DEFAULT_FRAME_RATE = 60;
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV21;
    vDecSample->IN_DIR = "/data/test/media/profile2_level50_gop60_1280x720.h263";
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.DVVIDEO"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderForH263());
    vDecSample->WaitForEOS();
    cout << "outFrameCount" << vDecSample->outFrameCount << endl;
    ASSERT_EQ(true, vDecSample->isonError);
}

/**
 * @tc.number    : VIDEO_SWDEC_DVVIDEO_RESOLUTION_0010
 * @tc.name      : dvvideo Resolution Change，buffer, nv12, INP_DIR_6
 * @tc.desc      : function test
 */
HWTEST_F(DVVideoSwdecFuncNdkTest, VIDEO_SWDEC_DVVIDEO_RESOLUTION_0010, TestSize.Level3)
{
    if (g_codecNameDVVideo.find("DVVIDEO") != string::npos) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->DEFAULT_WIDTH = 960;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 60;
        vDecSample->outputYuvFlag = true;
        vDecSample->isDVVideoChange = true;
        vDecSample->SF_OUTPUT = false;
        vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
        vDecSample->OUT_DIR = "/data/test/media/dv_resolution_0010_960x720_60fps_nv12.yuv";
        ASSERT_EQ(AV_ERR_OK, vDecSample->LoadInputFile(INP_DIR_6));
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameDVVideo));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
        ASSERT_EQ(FRAMESIZE25, vDecSample->outFrameCount);
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_DVVIDEO_RESOLUTION_0020
 * @tc.name      : dvvideo Resolution Change，buffer, nv21, INP_DIR_6
 * @tc.desc      : function test
 */
HWTEST_F(DVVideoSwdecFuncNdkTest, VIDEO_SWDEC_DVVIDEO_RESOLUTION_0020, TestSize.Level3)
{
    if (g_codecNameDVVideo.find("DVVIDEO") != string::npos) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->DEFAULT_WIDTH = 960;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 60;
        vDecSample->outputYuvFlag = true;
        vDecSample->isDVVideoChange = true;
        vDecSample->SF_OUTPUT = false;
        vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV21;
        vDecSample->OUT_DIR = "/data/test/media/dv_resolution_0020_960x720_60fps_nv21.yuv";
        ASSERT_EQ(AV_ERR_OK, vDecSample->LoadInputFile(INP_DIR_6));
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameDVVideo));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
        ASSERT_EQ(FRAMESIZE25, vDecSample->outFrameCount);
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_DVVIDEO_RESOLUTION_0030
 * @tc.name      : dvvideo Resolution Change，buffer, nv12, INP_DIR_6
 * @tc.desc      : function test
 */
HWTEST_F(DVVideoSwdecFuncNdkTest, VIDEO_SWDEC_DVVIDEO_RESOLUTION_0030, TestSize.Level3)
{
    if (g_codecNameDVVideo.find("DVVIDEO") != string::npos) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->DEFAULT_WIDTH = 960;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 60;
        vDecSample->outputYuvFlag = true;
        vDecSample->isDVVideoChange = true;
        vDecSample->SF_OUTPUT = false;
        vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_YUVI420;
        vDecSample->OUT_DIR = "/data/test/media/dv_resolution_0030_960x720_60fps_i420.yuv";
        ASSERT_EQ(AV_ERR_OK, vDecSample->LoadInputFile(INP_DIR_6));
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameDVVideo));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
        ASSERT_EQ(FRAMESIZE25, vDecSample->outFrameCount);
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_DVVIDEO_RESOLUTION_0040
 * @tc.name      : dvvideo Resolution Change，surface, nv21, INP_DIR_6
 * @tc.desc      : function test
 */
HWTEST_F(DVVideoSwdecFuncNdkTest, VIDEO_SWDEC_DVVIDEO_RESOLUTION_0040, TestSize.Level3)
{
    if (g_codecNameDVVideo.find("DVVIDEO") != string::npos) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->DEFAULT_WIDTH = 960;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 60;
        vDecSample->outputYuvSurface = true;
        vDecSample->SF_OUTPUT = true;
        vDecSample->isDVVideoChange = true;
        vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV21;
        vDecSample->OUT_DIR = "/data/test/media/dv_resolution_0040_960x720_60fps_nv21.yuv";
        ASSERT_EQ(AV_ERR_OK, vDecSample->LoadInputFile(INP_DIR_6));
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameDVVideo));
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
        ASSERT_EQ(FRAMESIZE25, vDecSample->outFrameCount);
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_DVVIDEO_RESOLUTION_0050
 * @tc.name      : dvvideo Resolution Change，surface, nv21, profile1_1440x1080_24
 * @tc.desc      : function test
 */
HWTEST_F(DVVideoSwdecFuncNdkTest, VIDEO_SWDEC_DVVIDEO_RESOLUTION_0050, TestSize.Level3)
{
    if (g_codecNameDVVideo.find("DVVIDEO") != string::npos) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->outputYuvSurface = true;
        vDecSample->isDVVideoChange = true;
        vDecSample->SF_OUTPUT = true;
        vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV21;
        vDecSample->OUT_DIR = "/data/test/media/dv_resolution_0050_1280x1080_30fps_nv21.yuv";
        ASSERT_EQ(AV_ERR_OK, vDecSample->LoadInputFile(INP_DIR_7));
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameDVVideo));
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
        ASSERT_EQ(FRAMESIZE25, vDecSample->outFrameCount);
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_DVVIDEO_RESOLUTION_0060
 * @tc.name      : dvvideo Resolution Change，surface, nv21, profile1_1920x1080_24
 * @tc.desc      : function test
 */
HWTEST_F(DVVideoSwdecFuncNdkTest, VIDEO_SWDEC_DVVIDEO_RESOLUTION_0060, TestSize.Level3)
{
    if (g_codecNameDVVideo.find("DVVIDEO") != string::npos) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->outputYuvSurface = true;
        vDecSample->isDVVideoChange = true;
        vDecSample->SF_OUTPUT = true;
        vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
        vDecSample->OUT_DIR = "/data/test/media/dv_resolution_0060_1280x1080_30fps_nv12.yuv";
        ASSERT_EQ(AV_ERR_OK, vDecSample->LoadInputFile(INP_DIR_7));
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameDVVideo));
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
        ASSERT_EQ(FRAMESIZE25, vDecSample->outFrameCount);
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_DVVIDEO_API11_ROTATION_ANGLE_FUNC_001
 * @tc.name      : decoder dvvideo,func,surface settransform rotate 90 degree,INP_DIR_6
 * @tc.desc      : function test
 */
HWTEST_F(DVVideoSwdecFuncNdkTest, VIDEO_SWDEC_DVVIDEO_API11_ROTATION_ANGLE_FUNC_001, TestSize.Level3)
{
    if (cap_dvvideo != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->DEFAULT_WIDTH = 960;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 60;
        vDecSample->outputYuvSurface = true;
        vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
        vDecSample->OUT_DIR = "/data/test/media/dv_rotation_angle_001_960x720_60fps_nv12.yuv";
        ASSERT_EQ(AV_ERR_OK, vDecSample->LoadInputFile(INP_DIR_6));
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameDVVideo));
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetRotationAngle(90));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
        ASSERT_EQ(FRAMESIZE25, vDecSample->outFrameCount);
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_DVVIDEO_API11_ROTATION_ANGLE_FUNC_002
 * @tc.name      : decoder dvvideo,func,surface settransform rotate 180 degree,INP_DIR_6
 * @tc.desc      : function test
 */
HWTEST_F(DVVideoSwdecFuncNdkTest, VIDEO_SWDEC_DVVIDEO_API11_ROTATION_ANGLE_FUNC_002, TestSize.Level3)
{
    if (cap_dvvideo != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->DEFAULT_WIDTH = 960;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 60;
        vDecSample->outputYuvSurface = true;
        vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
        vDecSample->OUT_DIR = "/data/test/media/dv_rotation_angle_002_960x720_60fps_nv12.yuv";
        ASSERT_EQ(AV_ERR_OK, vDecSample->LoadInputFile(INP_DIR_6));
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameDVVideo));
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetRotationAngle(180));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
        ASSERT_EQ(FRAMESIZE25, vDecSample->outFrameCount);
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_DVVIDEO_API11_ROTATION_ANGLE_FUNC_003
 * @tc.name      : decoder dvvideo,func,surface settransform rotate 270 degree,INP_DIR_6
 * @tc.desc      : function test
 */
HWTEST_F(DVVideoSwdecFuncNdkTest, VIDEO_SWDEC_DVVIDEO_API11_ROTATION_ANGLE_FUNC_003, TestSize.Level3)
{
    if (cap_dvvideo != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->DEFAULT_WIDTH = 960;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 60;
        vDecSample->outputYuvSurface = true;
        vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
        vDecSample->OUT_DIR = "/data/test/media/dv_rotation_angle_003_960x720_60fps_nv12.yuv";
        ASSERT_EQ(AV_ERR_OK, vDecSample->LoadInputFile(INP_DIR_6));
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameDVVideo));
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetRotationAngle(270));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
        ASSERT_EQ(FRAMESIZE25, vDecSample->outFrameCount);
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_DVVIDEO_API11_SURF_CHANGE_FUNC_001
 * @tc.name      : surf model change in normal state
 * @tc.desc      : function test
 */
HWTEST_F(DVVideoSwdecFuncNdkTest, VIDEO_SWDEC_DVVIDEO_API11_SURF_CHANGE_FUNC_001, TestSize.Level2)
{
    if (cap_dvvideo != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->DEFAULT_WIDTH = 960;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 60;
        vDecSample->outputYuvSurface = true;
        vDecSample->autoSwitchSurface = true;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        vDecSample->sleepOnFPS = true;
        vDecSample->OUT_DIR = "/data/test/media/dv_surf_001_960x720_60fps_nv12_1.yuv";
        vDecSample->OUT_DIR2 = "/data/test/media/dv_surf_001_960x720_60fps_nv12_2.yuv";
        ASSERT_EQ(AV_ERR_OK, vDecSample->LoadInputFile(INP_DIR_6));
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameDVVideo));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->Reset());
        ASSERT_EQ(AV_ERR_INVALID_STATE, vDecSample->SwitchSurface());
        ASSERT_EQ(AV_ERR_OK, vDecSample->Release());
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_DVVIDEO_API11_SURF_CHANGE_FUNC_002
 * @tc.name      : surf model change in flushed state
 * @tc.desc      : function test
 */
HWTEST_F(DVVideoSwdecFuncNdkTest, VIDEO_SWDEC_DVVIDEO_API11_SURF_CHANGE_FUNC_002, TestSize.Level2)
{
    if (cap_dvvideo != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->DEFAULT_WIDTH = 960;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 60;
        vDecSample->outputYuvSurface = true;
        vDecSample->autoSwitchSurface = true;
        vDecSample->sleepOnFPS = true;
        vDecSample->OUT_DIR = "/data/test/media/dv_surf_002_960x720_60fps_nv12_1.yuv";
        vDecSample->OUT_DIR2 = "/data/test/media/dv_surf_002_960x720_60fps_nv12_2.yuv";
        ASSERT_EQ(AV_ERR_OK, vDecSample->LoadInputFile(INP_DIR_6));
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameDVVideo));
        ASSERT_EQ(AV_ERR_OK, vDecSample->Flush());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
        ASSERT_EQ(AV_ERR_OK, vDecSample->Stop());
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_DVVIDEO_API11_SURF_CHANGE_FUNC_003
 * @tc.name      : surf model change in runing state
 * @tc.desc      : function test
 */
HWTEST_F(DVVideoSwdecFuncNdkTest, VIDEO_SWDEC_DVVIDEO_API11_SURF_CHANGE_FUNC_003, TestSize.Level2)
{
    if (cap_dvvideo != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->DEFAULT_WIDTH = 960;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 60;
        vDecSample->outputYuvSurface = true;
        vDecSample->autoSwitchSurface = false;
        vDecSample->SF_OUTPUT = true;
        vDecSample->OUT_DIR = "/data/test/media/dv_surf_003_960x720_60fps_nv12.yuv";
        ASSERT_EQ(AV_ERR_OK, vDecSample->LoadInputFile(INP_DIR_6));
        vDecSample->CreateSurface();
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameDVVideo));
        ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, vDecSample->SwitchSurface());
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_DVVIDEO_API11_SURF_CHANGE_FUNC_004
 * @tc.name      : repeat call setSurface fastly
 * @tc.desc      : function test
 */
HWTEST_F(DVVideoSwdecFuncNdkTest, VIDEO_SWDEC_DVVIDEO_API11_SURF_CHANGE_FUNC_004, TestSize.Level2)
{
    if (cap_dvvideo != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->DEFAULT_WIDTH = 960;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 60;
        vDecSample->outputYuvSurface = true;
        vDecSample->autoSwitchSurface = true;
        vDecSample->sleepOnFPS = true;
        vDecSample->OUT_DIR = "/data/test/media/dv_surf_004_960x720_60fps_nv12_1.yuv";
        vDecSample->OUT_DIR2 = "/data/test/media/dv_surf_004_960x720_60fps_nv12_2.yuv";
        ASSERT_EQ(AV_ERR_OK, vDecSample->LoadInputFile(INP_DIR_6));
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameDVVideo));
        ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
        vDecSample->WaitForEOS();
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_DVVIDEO_API11_SURF_CHANGE_FUNC_005
 * @tc.name      : surf model change in flush to runnig state
 * @tc.desc      : function test
 */
HWTEST_F(DVVideoSwdecFuncNdkTest, VIDEO_SWDEC_DVVIDEO_API11_SURF_CHANGE_FUNC_005, TestSize.Level2)
{
    if (cap_dvvideo != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->DEFAULT_WIDTH = 960;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 60;
        vDecSample->outputYuvSurface = true;
        vDecSample->autoSwitchSurface = true;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        vDecSample->sleepOnFPS = true;
        vDecSample->OUT_DIR = "/data/test/media/dv_surf_005_960x720_60fps_nv12_1.yuv";
        vDecSample->OUT_DIR2 = "/data/test/media/dv_surf_005_960x720_60fps_nv12_2.yuv";
        ASSERT_EQ(AV_ERR_OK, vDecSample->LoadInputFile(INP_DIR_6));
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameDVVideo));
        ASSERT_EQ(AV_ERR_OK, vDecSample->Flush());
        ASSERT_EQ(AV_ERR_OK, vDecSample->Start());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_DVVIDEO_API11_SURF_CHANGE_FUNC_006
 * @tc.name      : surf model change in decoder finish to End-of-Stream state
 * @tc.desc      : function test
 */
HWTEST_F(DVVideoSwdecFuncNdkTest, VIDEO_SWDEC_DVVIDEO_API11_SURF_CHANGE_FUNC_006, TestSize.Level2)
{
    if (cap_dvvideo != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->DEFAULT_WIDTH = 960;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 60;
        vDecSample->outputYuvSurface = true;
        vDecSample->autoSwitchSurface = true;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        vDecSample->sleepOnFPS = true;
        vDecSample->OUT_DIR = "/data/test/media/dv_surf_006_960x720_60fps_nv12_1.yuv";
        vDecSample->OUT_DIR2 = "/data/test/media/dv_surf_006_960x720_60fps_nv12_2.yuv";
        ASSERT_EQ(AV_ERR_OK, vDecSample->LoadInputFile(INP_DIR_6));
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameDVVideo));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_DVVIDEO_API11_SURF_CHANGE_FUNC_007
 * @tc.name      : buffer model change in decoder finish to End-of-Stream state
 * @tc.desc      : function test
 */
HWTEST_F(DVVideoSwdecFuncNdkTest, VIDEO_SWDEC_DVVIDEO_API11_SURF_CHANGE_FUNC_007, TestSize.Level2)
{
    if (cap_dvvideo != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->DEFAULT_WIDTH = 960;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 60;
        vDecSample->outputYuvFlag = true;
        vDecSample->outputYuvSurface = true;
        vDecSample->autoSwitchSurface = false;
        vDecSample->SF_OUTPUT = true;
        vDecSample->OUT_DIR = "/data/test/media/dv_surf_007_960x720_60fps_nv12_1.yuv";
        vDecSample->OUT_DIR = "/data/test/media/dv_surf_007_960x720_60fps_nv12_2.yuv";
        ASSERT_EQ(AV_ERR_OK, vDecSample->LoadInputFile(INP_DIR_6));
        vDecSample->CreateSurface();
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameDVVideo));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, vDecSample->SwitchSurface());
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_DVVIDEO_API11_SURF_CHANGE_FUNC_008
 * @tc.name      : buffer model change in runing to flushed state
 * @tc.desc      : function test
 */
HWTEST_F(DVVideoSwdecFuncNdkTest, VIDEO_SWDEC_DVVIDEO_API11_SURF_CHANGE_FUNC_008, TestSize.Level2)
{
    if (cap_dvvideo != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->DEFAULT_WIDTH = 960;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 60;
        vDecSample->outputYuvFlag = true;
        vDecSample->outputYuvSurface = true;
        vDecSample->autoSwitchSurface = false;
        vDecSample->SF_OUTPUT = true;
        vDecSample->OUT_DIR = "/data/test/media/dv_surf_008_960x720_60fps_nv12.yuv";
        ASSERT_EQ(AV_ERR_OK, vDecSample->LoadInputFile(INP_DIR_6));
        vDecSample->CreateSurface();
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameDVVideo));
        ASSERT_EQ(AV_ERR_OK, vDecSample->Flush());
        ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, vDecSample->SwitchSurface());
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_DVVIDEO_API11_SURF_CHANGE_FUNC_009
 * @tc.name      : buffer model change in flushed to runing state
 * @tc.desc      : function test
 */
HWTEST_F(DVVideoSwdecFuncNdkTest, VIDEO_SWDEC_DVVIDEO_API11_SURF_CHANGE_FUNC_009, TestSize.Level2)
{
    if (cap_dvvideo != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->DEFAULT_WIDTH = 960;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 60;
        vDecSample->outputYuvFlag = true;
        vDecSample->outputYuvSurface = true;
        vDecSample->autoSwitchSurface = false;
        vDecSample->SF_OUTPUT = true;
        vDecSample->OUT_DIR = "/data/test/media/dv_surf_008_960x720_60fps_nv12.yuv";
        ASSERT_EQ(AV_ERR_OK, vDecSample->LoadInputFile(INP_DIR_6));
        vDecSample->CreateSurface();
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameDVVideo));
        ASSERT_EQ(AV_ERR_OK, vDecSample->Flush());
        ASSERT_EQ(AV_ERR_OK, vDecSample->Start());
        ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, vDecSample->SwitchSurface());
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_DVVIDEO_API11_SURF_CHANGE_FUNC_010
 * @tc.name      : buffer model change in normal state
 * @tc.desc      : function test
 */
HWTEST_F(DVVideoSwdecFuncNdkTest, VIDEO_SWDEC_DVVIDEO_API11_SURF_CHANGE_FUNC_010, TestSize.Level2)
{
    if (cap_dvvideo != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->DEFAULT_WIDTH = 960;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 60;
        vDecSample->outputYuvFlag = true;
        vDecSample->outputYuvSurface = true;
        vDecSample->autoSwitchSurface = false;
        vDecSample->OUT_DIR = "/data/test/media/dv_surf_010_960x720_60fps_nv12.yuv";
        ASSERT_EQ(AV_ERR_OK, vDecSample->LoadInputFile(INP_DIR_6));
        vDecSample->CreateSurface();
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameDVVideo));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->Reset());
        ASSERT_EQ(AV_ERR_INVALID_STATE, vDecSample->SwitchSurface());
        ASSERT_EQ(AV_ERR_OK, vDecSample->Release());
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_DVVIDEO_API11_SURF_CHANGE_FUNC_011
 * @tc.name      : surf model change in config state
 * @tc.desc      : function test
 */
HWTEST_F(DVVideoSwdecFuncNdkTest, VIDEO_SWDEC_DVVIDEO_API11_SURF_CHANGE_FUNC_011, TestSize.Level1)
{
    if (cap_dvvideo != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->DEFAULT_WIDTH = 960;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 60;
        vDecSample->outputYuvSurface = true;
        vDecSample->autoSwitchSurface = false;
        vDecSample->OUT_DIR = "/data/test/media/dv_surf_011_960x720_60fps_nv12.yuv";
        ASSERT_EQ(AV_ERR_OK, vDecSample->LoadInputFile(INP_DIR_6));
        vDecSample->CreateSurface();
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameDVVideo));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_DVVIDEO_API11_SURF_CHANGE_FUNC_012
 * @tc.name      : surf model change in config state
 * @tc.desc      : function test
 */
HWTEST_F(DVVideoSwdecFuncNdkTest, VIDEO_SWDEC_DVVIDEO_API11_SURF_CHANGE_FUNC_012, TestSize.Level1)
{
    if (cap_dvvideo != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->DEFAULT_WIDTH = 960;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 60;
        vDecSample->outputYuvFlag = true;
        vDecSample->outputYuvSurface = true;
        vDecSample->autoSwitchSurface = false;
        vDecSample->OUT_DIR = "/data/test/media/dv_surf_012_960x720_60fps_nv12.yuv";
        ASSERT_EQ(AV_ERR_OK, vDecSample->LoadInputFile(INP_DIR_6));
        vDecSample->CreateSurface();
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameDVVideo));
        ASSERT_EQ(AV_ERR_INVALID_STATE, vDecSample->SwitchSurface());
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_DVVIDEO_API11_SURF_CHANGE_FUNC_013
 * @tc.name      : Two object repeat call setSurface fastly
 * @tc.desc      : function test
 */
HWTEST_F(DVVideoSwdecFuncNdkTest, VIDEO_SWDEC_DVVIDEO_API11_SURF_CHANGE_FUNC_013, TestSize.Level2)
{
    if (cap_dvvideo != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->DEFAULT_WIDTH = 960;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 60;
        vDecSample->outputYuvSurface = true;
        vDecSample->autoSwitchSurface = true;
        vDecSample->sleepOnFPS = true;
        vDecSample->OUT_DIR = "/data/test/media/dv_surf_013_960x720_60fps_nv12_1.yuv";
        vDecSample->OUT_DIR2 = "/data/test/media/dv_surf_013_960x720_60fps_nv12_2.yuv";
        ASSERT_EQ(AV_ERR_OK, vDecSample->LoadInputFile(INP_DIR_6));
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameDVVideo));
        ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
        vDecSample->WaitForEOS();

        auto vDecSample_1 = make_shared<VDecAPI11Sample>();
        vDecSample_1->DEFAULT_WIDTH = 720;
        vDecSample_1->DEFAULT_HEIGHT = 480;
        vDecSample_1->DEFAULT_FRAME_RATE = 30;
        vDecSample_1->outputYuvSurface = true;
        vDecSample_1->autoSwitchSurface = true;
        vDecSample_1->sleepOnFPS = true;
        vDecSample_1->OUT_DIR = "/data/test/media/dv_surf_013_720x480_30fps_nv12_1.yuv";
        vDecSample_1->OUT_DIR2 = "/data/test/media/dv_surf_013_720x480_30fps_nv12_2.yuv";
        ASSERT_EQ(AV_ERR_OK, vDecSample_1->LoadInputFile(INP_DIR_10));
        ASSERT_EQ(AV_ERR_OK, vDecSample_1->RunVideoDec_Surface(g_codecNameDVVideo));
        ASSERT_EQ(AV_ERR_OK, vDecSample_1->SwitchSurface());
        vDecSample_1->WaitForEOS();
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_DVVIDEO_API11_SURF_CHANGE_FUNC_014
 * @tc.name      : repeat call setSurface fastly 2 time
 * @tc.desc      : function test
 */
HWTEST_F(DVVideoSwdecFuncNdkTest, VIDEO_SWDEC_DVVIDEO_API11_SURF_CHANGE_FUNC_014, TestSize.Level2)
{
    if (cap_dvvideo != nullptr) {
        for (int i = 0; i < 2; i++) {
            auto vDecSample = make_shared<VDecAPI11Sample>();
            vDecSample->DEFAULT_WIDTH = 960;
            vDecSample->DEFAULT_HEIGHT = 720;
            vDecSample->DEFAULT_FRAME_RATE = 60;
            vDecSample->autoSwitchSurface = true;
            vDecSample->sleepOnFPS = true;
            vDecSample->outputYuvSurface = true;
            vDecSample->OUT_DIR = std::string("/data/test/media/dv_surf_func_014_960x720_60fps_"
                + std::to_string(i) + "_1.yuv").c_str();
            vDecSample->OUT_DIR2 = std::string("/data/test/media/dv_surf_func_014_960x720_60fps_"
                + std::to_string(i) + "_2.yuv").c_str();
            ASSERT_EQ(AV_ERR_OK, vDecSample->LoadInputFile(INP_DIR_6));
            ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameDVVideo));
            ASSERT_EQ(AV_ERR_OK, vDecSample->RepeatCallSetSurface());
            vDecSample->WaitForEOS();
        }
    }
}

/**
 * @tc.number    : VIDEO_DECODE_SYNC_SWDVVIDEO_FUNC_0010
 * @tc.name      : SyncMode OutputPixelFormat:yuv420p->nv12
 * @tc.desc      : function test
 */
HWTEST_F(DVVideoSwdecFuncNdkTest, VIDEO_DECODE_SYNC_SWDVVIDEO_FUNC_0010, TestSize.Level0)
{
    if (cap_dvvideo != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->DEFAULT_WIDTH = 720;
        vDecSample->DEFAULT_HEIGHT = 576;
        vDecSample->DEFAULT_FRAME_RATE = 25;
        vDecSample->outputYuvFlag = true;
        vDecSample->enbleSyncMode = 1;
        vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
        vDecSample->OUT_DIR = "/data/test/media/dv_sync_func_0010_720x576_25fps_nv12.yuv";
        ASSERT_EQ(AV_ERR_OK, vDecSample->LoadInputFile(INP_DIR_0));
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameDVVideo));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartSyncVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
        ASSERT_EQ(FRAMESIZE25, vDecSample->outFrameCount);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_SYNC_SWDVVIDEO_FUNC_0020
 * @tc.name      : SyncMode OutputPixelFormat:yuv422p->nv21
 * @tc.desc      : function test
 */
HWTEST_F(DVVideoSwdecFuncNdkTest, VIDEO_DECODE_SYNC_SWDVVIDEO_FUNC_0020, TestSize.Level0)
{
    if (cap_dvvideo != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->DEFAULT_WIDTH = 720;
        vDecSample->DEFAULT_HEIGHT = 576;
        vDecSample->DEFAULT_FRAME_RATE = 25;
        vDecSample->outputYuvFlag = true;
        vDecSample->enbleSyncMode = 1;
        vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV21;
        vDecSample->OUT_DIR = "/data/test/media/dv_sync_func_0020_720x576_25fps_nv21.yuv";
        ASSERT_EQ(AV_ERR_OK, vDecSample->LoadInputFile(INP_DIR_0));
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameDVVideo));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartSyncVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
        ASSERT_EQ(FRAMESIZE25, vDecSample->outFrameCount);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_SYNC_SWDVVIDEO_FUNC_0030
 * @tc.name      : SyncMode Surface output
 * @tc.desc      : function test
 */
HWTEST_F(DVVideoSwdecFuncNdkTest, VIDEO_DECODE_SYNC_SWDVVIDEO_FUNC_0030, TestSize.Level0)
{
    if (cap_dvvideo != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->DEFAULT_WIDTH = 720;
        vDecSample->DEFAULT_HEIGHT = 576;
        vDecSample->DEFAULT_FRAME_RATE = 25;
        vDecSample->enbleSyncMode = 1;
        vDecSample->outputYuvFlag = true;
        vDecSample->outputYuvSurface = true;
        vDecSample->autoSwitchSurface = false;
        vDecSample->SF_OUTPUT = true;
        vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV21;
        vDecSample->OUT_DIR = "/data/test/media/dv_sync_surf_func_0030_720x576_25fps_nv21.yuv";
        ASSERT_EQ(AV_ERR_OK, vDecSample->LoadInputFile(INP_DIR_0));
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameDVVideo));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->DecodeSetSurface());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartSyncVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
        ASSERT_EQ(FRAMESIZE25, vDecSample->outFrameCount);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_SYNC_SWDVVIDEO_FUNC_0040
 * @tc.name      : SyncMode OutputPixelFormat:yuv420p->RGBA
 * @tc.desc      : function test
 */
HWTEST_F(DVVideoSwdecFuncNdkTest, VIDEO_DECODE_SYNC_SWDVVIDEO_FUNC_0040, TestSize.Level1)
{
    if (cap_dvvideo != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->DEFAULT_WIDTH = 720;
        vDecSample->DEFAULT_HEIGHT = 576;
        vDecSample->DEFAULT_FRAME_RATE = 25;
        vDecSample->outputYuvFlag = true;
        vDecSample->enbleSyncMode = 1;
        vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_RGBA;
        vDecSample->OUT_DIR = "/data/test/media/dv_sync_func_0040_720x576_25fps_rgba.rgba";
        ASSERT_EQ(AV_ERR_OK, vDecSample->LoadInputFile(INP_DIR_0));
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameDVVideo));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartSyncVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
        ASSERT_EQ(FRAMESIZE25, vDecSample->outFrameCount);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_SWDVVIDEO_BLANK_FRAME_0010
 * @tc.name      : config OH_MD_KEY_VIDEO_DECODER_BLANK_FRAME_ON_SHUTDOWN,buffer
 * @tc.desc      : function test
 */
HWTEST_F(DVVideoSwdecFuncNdkTest, VIDEO_DECODE_SWDVVIDEO_BLANK_FRAME_0010, TestSize.Level2)
{
    if (cap_dvvideo != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->DEFAULT_WIDTH = 720;
        vDecSample->DEFAULT_HEIGHT = 576;
        vDecSample->DEFAULT_FRAME_RATE = 25;
        vDecSample->enbleBlankFrame = 1;
        vDecSample->outputYuvFlag = true;
        vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
        vDecSample->OUT_DIR = "/data/test/media/dv_blank_frame_buffer_0010_720x576_25fps_nv12.yuv";
        ASSERT_EQ(AV_ERR_OK, vDecSample->LoadInputFile(INP_DIR_0));
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameDVVideo));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
        ASSERT_EQ(FRAMESIZE25, vDecSample->outFrameCount);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_SWDVVIDEO_BLANK_FRAME_0020
 * @tc.name      : config OH_MD_KEY_VIDEO_DECODER_BLANK_FRAME_ON_SHUTDOWN, surface
 * @tc.desc      : function test
 */
HWTEST_F(DVVideoSwdecFuncNdkTest, VIDEO_DECODE_SWDVVIDEO_BLANK_FRAME_0020, TestSize.Level2)
{
    if (cap_dvvideo != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->DEFAULT_WIDTH = 720;
        vDecSample->DEFAULT_HEIGHT = 576;
        vDecSample->DEFAULT_FRAME_RATE = 25;
        vDecSample->outputYuvSurface = true;
        vDecSample->autoSwitchSurface = false;
        vDecSample->enbleBlankFrame = 1;
        vDecSample->sleepOnFPS = true;
        vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
        vDecSample->OUT_DIR = "/data/test/media/dv_blank_frame_surface_0020_720x576_25fps_nv12.yuv";
        ASSERT_EQ(AV_ERR_OK, vDecSample->LoadInputFile(INP_DIR_0));
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameDVVideo));
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
        ASSERT_EQ(FRAMESIZE25, vDecSample->outFrameCount);
    }
}

}  // namespace