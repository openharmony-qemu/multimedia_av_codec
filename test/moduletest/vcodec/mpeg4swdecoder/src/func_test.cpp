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
class Mpeg4SwdecFuncNdkTest : public testing::Test {
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
    const char *INP_DIR_720_30 = "/data/test/media/mpeg4_simple@level6_1280x720_30.m4v";
    const char *INP_DIR_1080_30 = "/data/test/media/mpeg4_main@level4_1920x1080_30.m4v";
};
} // namespace Media
} // namespace OHOS

namespace {
static OH_AVCapability *cap_mpeg4 = nullptr;
static string g_codecNameMpeg4 = "";
} // namespace

void Mpeg4SwdecFuncNdkTest::SetUpTestCase()
{
    cap_mpeg4 = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_MPEG4_PART2, false, SOFTWARE);
    g_codecNameMpeg4 = OH_AVCapability_GetName(cap_mpeg4);
    cout << "g_codecNameMpeg4: " << g_codecNameMpeg4 << endl;
}

void Mpeg4SwdecFuncNdkTest::TearDownTestCase() {}
void Mpeg4SwdecFuncNdkTest::SetUp() {}
void Mpeg4SwdecFuncNdkTest::TearDown() {}

namespace {
/**
 * @tc.number    : VIDEO_MPEG4SWDEC_FUNC_0100
 * @tc.name      : decode mpeg4 stream, buffer mode, output pixel format is nv12
 * @tc.desc      : function test
 */
HWTEST_F(Mpeg4SwdecFuncNdkTest, VIDEO_MPEG4SWDEC_FUNC_0100, TestSize.Level2)
{
    if (cap_mpeg4 != nullptr) {
        auto vDecSample = make_shared<VDecNdkSample>();
        vDecSample->INP_DIR = INP_DIR_1080_30;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SURFACE_OUTPUT = false;
        vDecSample->checkOutPut = true;
        vDecSample->DEFAULT_PIXEL_FORMAT = AV_PIXEL_FORMAT_NV12;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameMpeg4));
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_MPEG4SWDEC_FUNC_0200
 * @tc.name      : decode mpeg4 stream, buffer mode, output pixel format is nv21
 * @tc.desc      : function test
 */
HWTEST_F(Mpeg4SwdecFuncNdkTest, VIDEO_MPEG4SWDEC_FUNC_0200, TestSize.Level2)
{
    if (cap_mpeg4 != nullptr) {
        auto vDecSample = make_shared<VDecNdkSample>();
        vDecSample->INP_DIR = INP_DIR_1080_30;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SURFACE_OUTPUT = false;
        vDecSample->checkOutPut = true;
        vDecSample->DEFAULT_PIXEL_FORMAT = AV_PIXEL_FORMAT_NV21;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameMpeg4));
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_MPEG4SWDEC_FUNC_0300
 * @tc.name      : decode mpeg4 stream, buffer mode, output pixel format is YUVI420
 * @tc.desc      : function test
 */
HWTEST_F(Mpeg4SwdecFuncNdkTest, VIDEO_MPEG4SWDEC_FUNC_0300, TestSize.Level2)
{
    if (cap_mpeg4 != nullptr) {
        auto vDecSample = make_shared<VDecNdkSample>();
        vDecSample->INP_DIR = INP_DIR_1080_30;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SURFACE_OUTPUT = false;
        vDecSample->checkOutPut = true;
        vDecSample->DEFAULT_PIXEL_FORMAT = AV_PIXEL_FORMAT_YUVI420;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameMpeg4));
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_MPEG4SWDEC_FUNC_0400
 * @tc.name      : decode mpeg4 stream, buffer mode, output pixel format is RGBA
 * @tc.desc      : function test
 */
HWTEST_F(Mpeg4SwdecFuncNdkTest, VIDEO_MPEG4SWDEC_FUNC_0400, TestSize.Level2)
{
    if (cap_mpeg4 != nullptr) {
        auto vDecSample = make_shared<VDecNdkSample>();
        vDecSample->INP_DIR = INP_DIR_1080_30;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SURFACE_OUTPUT = false;
        vDecSample->checkOutPut = false;
        vDecSample->DEFAULT_PIXEL_FORMAT = AV_PIXEL_FORMAT_RGBA;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameMpeg4));
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_MPEG4SWDEC_FUNC_0500
 * @tc.name      : decode mpeg4 stream, surface mode, output pixel format is NV12
 * @tc.desc      : function test
 */
HWTEST_F(Mpeg4SwdecFuncNdkTest, VIDEO_MPEG4SWDEC_FUNC_0500, TestSize.Level2)
{
    if (cap_mpeg4 != nullptr) {
        auto vDecSample = make_shared<VDecNdkSample>();
        vDecSample->INP_DIR = INP_DIR_1080_30;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SURFACE_OUTPUT = true;
        vDecSample->checkOutPut = true;
        vDecSample->DEFAULT_PIXEL_FORMAT = AV_PIXEL_FORMAT_NV12;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameMpeg4));
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_MPEG4SWDEC_FUNC_0600
 * @tc.name      : decode mpeg4 stream, surface mode, output pixel format is NV21
 * @tc.desc      : function test
 */
HWTEST_F(Mpeg4SwdecFuncNdkTest, VIDEO_MPEG4SWDEC_FUNC_0600, TestSize.Level2)
{
    if (cap_mpeg4 != nullptr) {
        auto vDecSample = make_shared<VDecNdkSample>();
        vDecSample->INP_DIR = INP_DIR_1080_30;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SURFACE_OUTPUT = true;
        vDecSample->checkOutPut = true;
        vDecSample->DEFAULT_PIXEL_FORMAT = AV_PIXEL_FORMAT_NV21;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameMpeg4));
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_MPEG4SWDEC_FUNC_0700
 * @tc.name      : decode mpeg4 stream, surface mode, output pixel format is YUVI420
 * @tc.desc      : function test
 */
HWTEST_F(Mpeg4SwdecFuncNdkTest, VIDEO_MPEG4SWDEC_FUNC_0700, TestSize.Level2)
{
    if (cap_mpeg4 != nullptr) {
        auto vDecSample = make_shared<VDecNdkSample>();
        vDecSample->INP_DIR = INP_DIR_1080_30;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SURFACE_OUTPUT = true;
        vDecSample->checkOutPut = true;
        vDecSample->DEFAULT_PIXEL_FORMAT = AV_PIXEL_FORMAT_YUVI420;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameMpeg4));
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_MPEG4SWDEC_FUNC_0800
 * @tc.name      : decode mpeg4 stream, surface mode, output pixel format is RGBA
 * @tc.desc      : function test
 */
HWTEST_F(Mpeg4SwdecFuncNdkTest, VIDEO_MPEG4SWDEC_FUNC_0800, TestSize.Level2)
{
    if (cap_mpeg4 != nullptr) {
        auto vDecSample = make_shared<VDecNdkSample>();
        vDecSample->INP_DIR = INP_DIR_1080_30;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SURFACE_OUTPUT = true;
        vDecSample->checkOutPut = false;
        vDecSample->DEFAULT_PIXEL_FORMAT = AV_PIXEL_FORMAT_RGBA;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameMpeg4));
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_MPEG4SWDEC_FUNC_0900
 * @tc.name      : software decode frame
 * @tc.desc      : function test
 */
HWTEST_F(Mpeg4SwdecFuncNdkTest, VIDEO_MPEG4SWDEC_FUNC_0900, TestSize.Level0)
{
    if (cap_mpeg4 != nullptr) {
        VDecNdkSample *vDecSample = new VDecNdkSample();

        int32_t ret = vDecSample->CreateVideoDecoder(g_codecNameMpeg4);
        ASSERT_EQ(AV_ERR_OK, ret);
        ret = vDecSample->SetVideoDecoderCallback();
        ASSERT_EQ(AV_ERR_OK, ret);
        ret = vDecSample->ConfigureVideoDecoder();
        ASSERT_EQ(AV_ERR_OK, ret);
        vDecSample->INP_DIR = "/data/test/media/mpeg4_simple@level6_1280x720_30.m4v";
        ret = vDecSample->StartVideoDecoder();
        ASSERT_EQ(AV_ERR_OK, ret);
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_MPEG4SWDEC_FUNC_1000
 * @tc.name      : software decode frame
 * @tc.desc      : function test
 */
HWTEST_F(Mpeg4SwdecFuncNdkTest, VIDEO_MPEG4SWDEC_FUNC_1000, TestSize.Level2)
{
    if (cap_mpeg4 != nullptr) {
        VDecNdkSample *vDecSample = new VDecNdkSample();

        int32_t ret = vDecSample->CreateVideoDecoder(g_codecNameMpeg4);
        ASSERT_EQ(AV_ERR_OK, ret);
        ret = vDecSample->SetVideoDecoderCallback();
        ASSERT_EQ(AV_ERR_OK, ret);
        ret = vDecSample->ConfigureVideoDecoder();
        ASSERT_EQ(AV_ERR_OK, ret);
        vDecSample->INP_DIR = "/data/test/media/mpeg4_simple_scalable@level2_352x288_60.m4v";
        ret = vDecSample->StartVideoDecoder();
        ASSERT_EQ(AV_ERR_OK, ret);
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_MPEG4SWDEC_FUNC_1100
 * @tc.name      : software decode frame
 * @tc.desc      : function test
 */
HWTEST_F(Mpeg4SwdecFuncNdkTest, VIDEO_MPEG4SWDEC_FUNC_1100, TestSize.Level2)
{
    if (cap_mpeg4 != nullptr) {
        VDecNdkSample *vDecSample = new VDecNdkSample();

        int32_t ret = vDecSample->CreateVideoDecoder(g_codecNameMpeg4);
        ASSERT_EQ(AV_ERR_OK, ret);
        ret = vDecSample->SetVideoDecoderCallback();
        ASSERT_EQ(AV_ERR_OK, ret);
        ret = vDecSample->ConfigureVideoDecoder();
        ASSERT_EQ(AV_ERR_OK, ret);
        vDecSample->INP_DIR = "/data/test/media/mpeg4_core@level1_176x144_30.m4v";
        ret = vDecSample->StartVideoDecoder();
        ASSERT_EQ(AV_ERR_OK, ret);
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_MPEG4SWDEC_FUNC_1200
 * @tc.name      : software decode frame
 * @tc.desc      : function test
 */
HWTEST_F(Mpeg4SwdecFuncNdkTest, VIDEO_MPEG4SWDEC_FUNC_1200, TestSize.Level0)
{
    if (cap_mpeg4 != nullptr) {
        VDecNdkSample *vDecSample = new VDecNdkSample();

        int32_t ret = vDecSample->CreateVideoDecoder(g_codecNameMpeg4);
        ASSERT_EQ(AV_ERR_OK, ret);
        ret = vDecSample->SetVideoDecoderCallback();
        ASSERT_EQ(AV_ERR_OK, ret);
        ret = vDecSample->ConfigureVideoDecoder();
        ASSERT_EQ(AV_ERR_OK, ret);
        vDecSample->INP_DIR = "/data/test/media/mpeg4_main@level4_1920x1080_30.m4v";
        ret = vDecSample->StartVideoDecoder();
        ASSERT_EQ(AV_ERR_OK, ret);
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_MPEG4SWDEC_FUNC_1300
 * @tc.name      : software decode frame
 * @tc.desc      : function test
 */
HWTEST_F(Mpeg4SwdecFuncNdkTest, VIDEO_MPEG4SWDEC_FUNC_1300, TestSize.Level2)
{
    if (cap_mpeg4 != nullptr) {
        VDecNdkSample *vDecSample = new VDecNdkSample();

        int32_t ret = vDecSample->CreateVideoDecoder(g_codecNameMpeg4);
        ASSERT_EQ(AV_ERR_OK, ret);
        ret = vDecSample->SetVideoDecoderCallback();
        ASSERT_EQ(AV_ERR_OK, ret);
        ret = vDecSample->ConfigureVideoDecoder();
        ASSERT_EQ(AV_ERR_OK, ret);
        vDecSample->INP_DIR = "/data/test/media/mpeg4_N_bit@level2_640x480_60.m4v";
        ret = vDecSample->StartVideoDecoder();
        ASSERT_EQ(AV_ERR_OK, ret);
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_MPEG4SWDEC_FUNC_1400
 * @tc.name      : software decode frame
 * @tc.desc      : function test
 */
HWTEST_F(Mpeg4SwdecFuncNdkTest, VIDEO_MPEG4SWDEC_FUNC_1400, TestSize.Level2)
{
    if (cap_mpeg4 != nullptr) {
        VDecNdkSample *vDecSample = new VDecNdkSample();

        int32_t ret = vDecSample->CreateVideoDecoder(g_codecNameMpeg4);
        ASSERT_EQ(AV_ERR_OK, ret);
        ret = vDecSample->SetVideoDecoderCallback();
        ASSERT_EQ(AV_ERR_OK, ret);
        ret = vDecSample->ConfigureVideoDecoder();
        ASSERT_EQ(AV_ERR_OK, ret);
        vDecSample->INP_DIR = "/data/test/media/mpeg4_Hybrid@level2_720x480_30.m4v";
        ret = vDecSample->StartVideoDecoder();
        ASSERT_EQ(AV_ERR_OK, ret);
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_MPEG4SWDEC_FUNC_1500
 * @tc.name      : software decode frame
 * @tc.desc      : function test
 */
HWTEST_F(Mpeg4SwdecFuncNdkTest, VIDEO_MPEG4SWDEC_FUNC_1500, TestSize.Level2)
{
    if (cap_mpeg4 != nullptr) {
        VDecNdkSample *vDecSample = new VDecNdkSample();

        int32_t ret = vDecSample->CreateVideoDecoder(g_codecNameMpeg4);
        ASSERT_EQ(AV_ERR_OK, ret);
        ret = vDecSample->SetVideoDecoderCallback();
        ASSERT_EQ(AV_ERR_OK, ret);
        ret = vDecSample->ConfigureVideoDecoder();
        ASSERT_EQ(AV_ERR_OK, ret);
        vDecSample->INP_DIR = "/data/test/media/mpeg4_Basic_Animated_Texture@level2_720x576_30.m4v";
        ret = vDecSample->StartVideoDecoder();
        ASSERT_EQ(AV_ERR_OK, ret);
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_MPEG4SWDEC_FUNC_1600
 * @tc.name      : software decode frame
 * @tc.desc      : function test
 */
HWTEST_F(Mpeg4SwdecFuncNdkTest, VIDEO_MPEG4SWDEC_FUNC_1600, TestSize.Level2)
{
    if (cap_mpeg4 != nullptr) {
        VDecNdkSample *vDecSample = new VDecNdkSample();

        int32_t ret = vDecSample->CreateVideoDecoder(g_codecNameMpeg4);
        ASSERT_EQ(AV_ERR_OK, ret);
        ret = vDecSample->SetVideoDecoderCallback();
        ASSERT_EQ(AV_ERR_OK, ret);
        ret = vDecSample->ConfigureVideoDecoder();
        ASSERT_EQ(AV_ERR_OK, ret);
        vDecSample->INP_DIR = "/data/test/media/mpeg4_Scalable_Texture@level1_1600x900_60.m4v";
        ret = vDecSample->StartVideoDecoder();
        ASSERT_EQ(AV_ERR_OK, ret);
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_MPEG4SWDEC_FUNC_1700
 * @tc.name      : software decode frame
 * @tc.desc      : function test
 */
HWTEST_F(Mpeg4SwdecFuncNdkTest, VIDEO_MPEG4SWDEC_FUNC_1700, TestSize.Level0)
{
    if (cap_mpeg4 != nullptr) {
        VDecNdkSample *vDecSample = new VDecNdkSample();

        int32_t ret = vDecSample->CreateVideoDecoder(g_codecNameMpeg4);
        ASSERT_EQ(AV_ERR_OK, ret);
        ret = vDecSample->SetVideoDecoderCallback();
        ASSERT_EQ(AV_ERR_OK, ret);
        ret = vDecSample->ConfigureVideoDecoder();
        ASSERT_EQ(AV_ERR_OK, ret);
        vDecSample->INP_DIR = "/data/test/media/mpeg4_Simple_FA@level2_1024x768_30.m4v";
        ret = vDecSample->StartVideoDecoder();
        ASSERT_EQ(AV_ERR_OK, ret);
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_MPEG4SWDEC_FUNC_1800
 * @tc.name      : software decode frame
 * @tc.desc      : function test
 */
HWTEST_F(Mpeg4SwdecFuncNdkTest, VIDEO_MPEG4SWDEC_FUNC_1800, TestSize.Level2)
{
    if (cap_mpeg4 != nullptr) {
        VDecNdkSample *vDecSample = new VDecNdkSample();

        int32_t ret = vDecSample->CreateVideoDecoder(g_codecNameMpeg4);
        ASSERT_EQ(AV_ERR_OK, ret);
        ret = vDecSample->SetVideoDecoderCallback();
        ASSERT_EQ(AV_ERR_OK, ret);
        ret = vDecSample->ConfigureVideoDecoder();
        ASSERT_EQ(AV_ERR_OK, ret);
        vDecSample->INP_DIR = "/data/test/media/mpeg4_Advanced_Real_Time_Simple@level4_1440x1080_60.m4v";
        ret = vDecSample->StartVideoDecoder();
        ASSERT_EQ(AV_ERR_OK, ret);
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_MPEG4SWDEC_FUNC_1900
 * @tc.name      : software decode frame
 * @tc.desc      : function test
 */
HWTEST_F(Mpeg4SwdecFuncNdkTest, VIDEO_MPEG4SWDEC_FUNC_1900, TestSize.Level0)
{
    if (cap_mpeg4 != nullptr) {
        VDecNdkSample *vDecSample = new VDecNdkSample();

        int32_t ret = vDecSample->CreateVideoDecoder(g_codecNameMpeg4);
        ASSERT_EQ(AV_ERR_OK, ret);
        ret = vDecSample->SetVideoDecoderCallback();
        ASSERT_EQ(AV_ERR_OK, ret);
        ret = vDecSample->ConfigureVideoDecoder();
        ASSERT_EQ(AV_ERR_OK, ret);
        vDecSample->INP_DIR = "/data/test/media/mpeg4_Core_Scalable@level3_320x240_30.m4v";
        ret = vDecSample->StartVideoDecoder();
        ASSERT_EQ(AV_ERR_OK, ret);
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_MPEG4SWDEC_FUNC_2000
 * @tc.name      : software decode frame
 * @tc.desc      : function test
 */
HWTEST_F(Mpeg4SwdecFuncNdkTest, VIDEO_MPEG4SWDEC_FUNC_2000, TestSize.Level2)
{
    if (cap_mpeg4 != nullptr) {
        VDecNdkSample *vDecSample = new VDecNdkSample();

        int32_t ret = vDecSample->CreateVideoDecoder(g_codecNameMpeg4);
        ASSERT_EQ(AV_ERR_OK, ret);
        ret = vDecSample->SetVideoDecoderCallback();
        ASSERT_EQ(AV_ERR_OK, ret);
        ret = vDecSample->ConfigureVideoDecoder();
        ASSERT_EQ(AV_ERR_OK, ret);
        vDecSample->INP_DIR = "/data/test/media/mpeg4_Advanced_Coding_Efficiency@level6_1600x1200_60.m4v";
        ret = vDecSample->StartVideoDecoder();
        ASSERT_EQ(AV_ERR_OK, ret);
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_MPEG4SWDEC_FUNC_2100
 * @tc.name      : software decode frame
 * @tc.desc      : function test
 */
HWTEST_F(Mpeg4SwdecFuncNdkTest, VIDEO_MPEG4SWDEC_FUNC_2100, TestSize.Level0)
{
    if (cap_mpeg4 != nullptr) {
        VDecNdkSample *vDecSample = new VDecNdkSample();

        int32_t ret = vDecSample->CreateVideoDecoder(g_codecNameMpeg4);
        ASSERT_EQ(AV_ERR_OK, ret);
        ret = vDecSample->SetVideoDecoderCallback();
        ASSERT_EQ(AV_ERR_OK, ret);
        ret = vDecSample->ConfigureVideoDecoder();
        ASSERT_EQ(AV_ERR_OK, ret);
        vDecSample->INP_DIR = "/data/test/media/mpeg4_Advanced_Core@level2_800x600_30.m4v";
        ret = vDecSample->StartVideoDecoder();
        ASSERT_EQ(AV_ERR_OK, ret);
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_MPEG4SWDEC_FUNC_2200
 * @tc.name      : software decode frame
 * @tc.desc      : function test
 */
HWTEST_F(Mpeg4SwdecFuncNdkTest, VIDEO_MPEG4SWDEC_FUNC_2200, TestSize.Level2)
{
    if (cap_mpeg4 != nullptr) {
        VDecNdkSample *vDecSample = new VDecNdkSample();

        int32_t ret = vDecSample->CreateVideoDecoder(g_codecNameMpeg4);
        ASSERT_EQ(AV_ERR_OK, ret);
        ret = vDecSample->SetVideoDecoderCallback();
        ASSERT_EQ(AV_ERR_OK, ret);
        ret = vDecSample->ConfigureVideoDecoder();
        ASSERT_EQ(AV_ERR_OK, ret);
        vDecSample->INP_DIR = "/data/test/media/mpeg4_Advanced_Scalable_Texture@level3_1024x576_60.m4v";
        ret = vDecSample->StartVideoDecoder();
        ASSERT_EQ(AV_ERR_OK, ret);
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_MPEG4SWDEC_FUNC_2300
 * @tc.name      : software decode frame
 * @tc.desc      : function test
 */
HWTEST_F(Mpeg4SwdecFuncNdkTest, VIDEO_MPEG4SWDEC_FUNC_2300, TestSize.Level2)
{
    if (cap_mpeg4 != nullptr) {
        VDecNdkSample *vDecSample = new VDecNdkSample();

        int32_t ret = vDecSample->CreateVideoDecoder(g_codecNameMpeg4);
        ASSERT_EQ(AV_ERR_OK, ret);
        ret = vDecSample->SetVideoDecoderCallback();
        ASSERT_EQ(AV_ERR_OK, ret);
        ret = vDecSample->ConfigureVideoDecoder();
        ASSERT_EQ(AV_ERR_OK, ret);
        vDecSample->INP_DIR = "/data/test/media/mpeg4_Advanced_Simple@level5_320x240_30.m4v";
        ret = vDecSample->StartVideoDecoder();
        ASSERT_EQ(AV_ERR_OK, ret);
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_MPEG4SWDEC_FUNC_2400
 * @tc.name      : software decode odd res stream
 * @tc.desc      : function test
 */
HWTEST_F(Mpeg4SwdecFuncNdkTest, VIDEO_MPEG4SWDEC_FUNC_2400, TestSize.Level2)
{
    if (cap_mpeg4 != nullptr) {
        VDecNdkSample *vDecSample = new VDecNdkSample();

        int32_t ret = vDecSample->CreateVideoDecoder(g_codecNameMpeg4);
        ASSERT_EQ(AV_ERR_OK, ret);
        ret = vDecSample->SetVideoDecoderCallback();
        ASSERT_EQ(AV_ERR_OK, ret);
        ret = vDecSample->ConfigureVideoDecoder();
        ASSERT_EQ(AV_ERR_OK, ret);
        vDecSample->INP_DIR = "/data/test/media/mpeg4_odd_res_1281x721.m4v";
        ret = vDecSample->StartVideoDecoder();
        ASSERT_EQ(AV_ERR_OK, ret);
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_MPEG4SWDEC_FUNC_2500
 * @tc.name      : software decode res change video
 * @tc.desc      : function test
 */
HWTEST_F(Mpeg4SwdecFuncNdkTest, VIDEO_MPEG4SWDEC_FUNC_2500, TestSize.Level3)
{
    if (cap_mpeg4 != nullptr) {
        auto vDecSample = make_shared<VDecNdkSample>();
        vDecSample->INP_DIR = "/data/test/media/mpeg4_res_change.m4v";
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SURFACE_OUTPUT = false;
        vDecSample->checkOutPut = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMpeg4));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_MPEG4SWDEC_FUNC_2600
 * @tc.name      : software decode err resolution video
 * @tc.desc      : function test
 */
HWTEST_F(Mpeg4SwdecFuncNdkTest, VIDEO_MPEG4SWDEC_FUNC_2600, TestSize.Level3)
{
    if (cap_mpeg4 != nullptr) {
        auto vDecSample = make_shared<VDecNdkSample>();
        vDecSample->INP_DIR = "/data/test/media/mpeg4_err_res.m4v";
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SURFACE_OUTPUT = false;
        vDecSample->checkOutPut = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMpeg4));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_MPEG4SWDEC_FUNC_2700
 * @tc.name      : software decode rotation 90
 * @tc.desc      : function test
 */
HWTEST_F(Mpeg4SwdecFuncNdkTest, VIDEO_MPEG4SWDEC_FUNC_2700, TestSize.Level2)
{
    if (cap_mpeg4 != nullptr) {
        auto vDecSample = make_shared<VDecNdkSample>();
        vDecSample->INP_DIR = "/data/test/media/mpeg4_simple@level6_1280x720_30.m4v";
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SURFACE_OUTPUT = true;
        vDecSample->checkOutPut = false;
        vDecSample->DEFAULT_ROTATION = 90;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameMpeg4));
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_MPEG4SWDEC_FUNC_2800
 * @tc.name      : software decode rotation 180
 * @tc.desc      : function test
 */
HWTEST_F(Mpeg4SwdecFuncNdkTest, VIDEO_MPEG4SWDEC_FUNC_2800, TestSize.Level2)
{
    if (cap_mpeg4 != nullptr) {
        auto vDecSample = make_shared<VDecNdkSample>();
        vDecSample->INP_DIR = "/data/test/media/mpeg4_simple@level6_1280x720_30.m4v";
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SURFACE_OUTPUT = true;
        vDecSample->checkOutPut = false;
        vDecSample->DEFAULT_ROTATION = 180;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameMpeg4));
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_MPEG4SWDEC_FUNC_2900
 * @tc.name      : software decode rotation 270
 * @tc.desc      : function test
 */
HWTEST_F(Mpeg4SwdecFuncNdkTest, VIDEO_MPEG4SWDEC_FUNC_2900, TestSize.Level2)
{
    if (cap_mpeg4 != nullptr) {
        auto vDecSample = make_shared<VDecNdkSample>();
        vDecSample->INP_DIR = "/data/test/media/mpeg4_simple@level6_1280x720_30.m4v";
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SURFACE_OUTPUT = true;
        vDecSample->checkOutPut = false;
        vDecSample->DEFAULT_ROTATION = 270;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameMpeg4));
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_MPEG4SWDEC_FUNC_3000
 * @tc.name      : software decode mpeg2 stream
 * @tc.desc      : function test
 */
HWTEST_F(Mpeg4SwdecFuncNdkTest, VIDEO_MPEG4SWDEC_FUNC_3000, TestSize.Level3)
{
    if (cap_mpeg4 != nullptr) {
        VDecNdkSample *vDecSample = new VDecNdkSample();

        int32_t ret = vDecSample->CreateVideoDecoder(g_codecNameMpeg4);
        ASSERT_EQ(AV_ERR_OK, ret);
        ret = vDecSample->SetVideoDecoderCallback();
        ASSERT_EQ(AV_ERR_OK, ret);
        ret = vDecSample->ConfigureVideoDecoder();
        ASSERT_EQ(AV_ERR_OK, ret);
        vDecSample->INP_DIR = "/data/test/media/mpeg2.m2v";
        ret = vDecSample->StartVideoDecoder();
        ASSERT_EQ(AV_ERR_OK, ret);
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_MPEG4_IPB_0100
 * @tc.name      : software decode all idr frame
 * @tc.desc      : function test
 */
HWTEST_F(Mpeg4SwdecFuncNdkTest, VIDEO_SWDEC_MPEG4_IPB_0100, TestSize.Level2)
{
    if (cap_mpeg4 != nullptr) {
        VDecNdkSample *vDecSample = new VDecNdkSample();

        int32_t ret = vDecSample->CreateVideoDecoder(g_codecNameMpeg4);
        ASSERT_EQ(AV_ERR_OK, ret);
        ret = vDecSample->SetVideoDecoderCallback();
        ASSERT_EQ(AV_ERR_OK, ret);
        ret = vDecSample->ConfigureVideoDecoder();
        ASSERT_EQ(AV_ERR_OK, ret);
        vDecSample->INP_DIR = "/data/test/media/mpeg4_1280x720_I.m4v";
        ret = vDecSample->StartVideoDecoder();
        ASSERT_EQ(AV_ERR_OK, ret);
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_MPEG4SWDEC_IPB_0200
 * @tc.name      : software decode single idr frame
 * @tc.desc      : function test
 */
HWTEST_F(Mpeg4SwdecFuncNdkTest, VIDEO_MPEG4SWDEC_IPB_0200, TestSize.Level2)
{
    if (cap_mpeg4 != nullptr) {
        VDecNdkSample *vDecSample = new VDecNdkSample();

        int32_t ret = vDecSample->CreateVideoDecoder(g_codecNameMpeg4);
        ASSERT_EQ(AV_ERR_OK, ret);
        ret = vDecSample->SetVideoDecoderCallback();
        ASSERT_EQ(AV_ERR_OK, ret);
        ret = vDecSample->ConfigureVideoDecoder();
        ASSERT_EQ(AV_ERR_OK, ret);
        vDecSample->INP_DIR = "/data/test/media/mpeg4_1280x720_IP.m4v";
        ret = vDecSample->StartVideoDecoder();
        ASSERT_EQ(AV_ERR_OK, ret);
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_MPEG4SWDEC_IPB_0300
 * @tc.name      : software decode all idr frame
 * @tc.desc      : function test
 */
HWTEST_F(Mpeg4SwdecFuncNdkTest, VIDEO_MPEG4SWDEC_IPB_0300, TestSize.Level2)
{
    if (cap_mpeg4 != nullptr) {
        VDecNdkSample *vDecSample = new VDecNdkSample();

        int32_t ret = vDecSample->CreateVideoDecoder(g_codecNameMpeg4);
        ASSERT_EQ(AV_ERR_OK, ret);
        ret = vDecSample->SetVideoDecoderCallback();
        ASSERT_EQ(AV_ERR_OK, ret);
        ret = vDecSample->ConfigureVideoDecoder();
        ASSERT_EQ(AV_ERR_OK, ret);
        vDecSample->INP_DIR = "/data/test/media/mpeg4_1280x720_IPB.m4v";
        ret = vDecSample->StartVideoDecoder();
        ASSERT_EQ(AV_ERR_OK, ret);
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_MPEG4SWDEC_PARA_0100
 * @tc.name      : width set -1 height set -1
 * @tc.desc      : function test
 */
HWTEST_F(Mpeg4SwdecFuncNdkTest, VIDEO_MPEG4SWDEC_PARA_0100, TestSize.Level2)
{
    if (cap_mpeg4 != nullptr) {
        auto vDecSample = make_shared<VDecNdkSample>();
        vDecSample->INP_DIR = INP_DIR_1080_30;
        vDecSample->DEFAULT_WIDTH = -1;
        vDecSample->DEFAULT_HEIGHT = -1;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SURFACE_OUTPUT = false;
        vDecSample->checkOutPut = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMpeg4));
        ASSERT_EQ(AV_ERR_INVALID_VAL, vDecSample->ConfigureVideoDecoder());
    }
}

/**
 * @tc.number    : VIDEO_MPEG4SWDEC_PARA_0200
 * @tc.name      : width set 0 height set 0
 * @tc.desc      : function test
 */
HWTEST_F(Mpeg4SwdecFuncNdkTest, VIDEO_MPEG4SWDEC_PARA_0200, TestSize.Level2)
{
    if (cap_mpeg4 != nullptr) {
        auto vDecSample = make_shared<VDecNdkSample>();
        vDecSample->INP_DIR = INP_DIR_1080_30;
        vDecSample->DEFAULT_WIDTH = 0;
        vDecSample->DEFAULT_HEIGHT = 0;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SURFACE_OUTPUT = false;
        vDecSample->checkOutPut = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMpeg4));
        ASSERT_EQ(AV_ERR_INVALID_VAL, vDecSample->ConfigureVideoDecoder());
    }
}

/**
 * @tc.number    : VIDEO_MPEG4SWDEC_PARA_0300
 * @tc.name      : width set 1 height set 1
 * @tc.desc      : function test
 */
HWTEST_F(Mpeg4SwdecFuncNdkTest, VIDEO_MPEG4SWDEC_PARA_0300, TestSize.Level2)
{
    if (cap_mpeg4 != nullptr) {
        auto vDecSample = make_shared<VDecNdkSample>();
        vDecSample->INP_DIR = INP_DIR_1080_30;
        vDecSample->DEFAULT_WIDTH = 1;
        vDecSample->DEFAULT_HEIGHT = 1;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SURFACE_OUTPUT = false;
        vDecSample->checkOutPut = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMpeg4));
        ASSERT_EQ(AV_ERR_INVALID_VAL, vDecSample->ConfigureVideoDecoder());
    }
}

/**
 * @tc.number    : VIDEO_MPEG4SWDEC_PARA_0400
 * @tc.name      : width set 4097 height set 4097
 * @tc.desc      : function test
 */
HWTEST_F(Mpeg4SwdecFuncNdkTest, VIDEO_MPEG4SWDEC_PARA_0400, TestSize.Level2)
{
    if (cap_mpeg4 != nullptr) {
        auto vDecSample = make_shared<VDecNdkSample>();
        vDecSample->INP_DIR = INP_DIR_1080_30;
        vDecSample->DEFAULT_WIDTH = 4097;
        vDecSample->DEFAULT_HEIGHT = 4097;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SURFACE_OUTPUT = false;
        vDecSample->checkOutPut = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMpeg4));
        ASSERT_EQ(AV_ERR_INVALID_VAL, vDecSample->ConfigureVideoDecoder());
    }
}

/**
 * @tc.number    : VIDEO_MPEG4SWDEC_PARA_0500
 * @tc.name      : width set 10000 height set 10000
 * @tc.desc      : function test
 */
HWTEST_F(Mpeg4SwdecFuncNdkTest, VIDEO_MPEG4SWDEC_PARA_0500, TestSize.Level2)
{
    if (cap_mpeg4 != nullptr) {
        auto vDecSample = make_shared<VDecNdkSample>();
        vDecSample->INP_DIR = INP_DIR_1080_30;
        vDecSample->DEFAULT_WIDTH = 10000;
        vDecSample->DEFAULT_HEIGHT = 10000;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SURFACE_OUTPUT = false;
        vDecSample->checkOutPut = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMpeg4));
        ASSERT_EQ(AV_ERR_INVALID_VAL, vDecSample->ConfigureVideoDecoder());
    }
}

/**
 * @tc.number    : VIDEO_MPEG4SWDEC_PARA_0600
 * @tc.name      : width set 64 height set 64
 * @tc.desc      : function test
 */
HWTEST_F(Mpeg4SwdecFuncNdkTest, VIDEO_MPEG4SWDEC_PARA_0600, TestSize.Level2)
{
    if (cap_mpeg4 != nullptr) {
        auto vDecSample = make_shared<VDecNdkSample>();
        vDecSample->INP_DIR = INP_DIR_1080_30;
        vDecSample->DEFAULT_WIDTH = 64;
        vDecSample->DEFAULT_HEIGHT = 64;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SURFACE_OUTPUT = false;
        vDecSample->checkOutPut = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMpeg4));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_MPEG4SWDEC_PARA_0700
 * @tc.name      : test mpeg4 decode buffer framerate -1
 * @tc.desc      : function test
 */
HWTEST_F(Mpeg4SwdecFuncNdkTest, VIDEO_MPEG4SWDEC_PARA_0700, TestSize.Level2)
{
    if (cap_mpeg4 != nullptr) {
        if (!access("/system/lib64/media/", 0)) {
            auto vDecSample = make_shared<VDecAPI11Sample>();
            vDecSample->INP_DIR = INP_DIR_1080_30;
            vDecSample->DEFAULT_WIDTH = 1920;
            vDecSample->DEFAULT_HEIGHT = 1080;
            vDecSample->DEFAULT_FRAME_RATE = -1;
            vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
            vDecSample->SF_OUTPUT = false;
            ASSERT_EQ(AV_ERR_INVALID_VAL, vDecSample->RunVideoDec(g_codecNameMpeg4));
            vDecSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
        }
    }
}

/**
 * @tc.number    : VIDEO_MPEG4SWDEC_PARA_0800
 * @tc.name      : test mpeg4 decode buffer framerate 0
 * @tc.desc      : function test
 */
HWTEST_F(Mpeg4SwdecFuncNdkTest, VIDEO_MPEG4SWDEC_PARA_0800, TestSize.Level2)
{
    if (cap_mpeg4 != nullptr) {
        if (!access("/system/lib64/media/", 0)) {
            auto vDecSample = make_shared<VDecAPI11Sample>();
            vDecSample->INP_DIR = INP_DIR_1080_30;
            vDecSample->DEFAULT_WIDTH = 1920;
            vDecSample->DEFAULT_HEIGHT = 1080;
            vDecSample->DEFAULT_FRAME_RATE = 0;
            vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
            vDecSample->SF_OUTPUT = false;
            ASSERT_EQ(AV_ERR_INVALID_VAL, vDecSample->RunVideoDec(g_codecNameMpeg4));
            vDecSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
        }
    }
}

/**
 * @tc.number    : VIDEO_MPEG4SWDEC_PARA_0900
 * @tc.name      : test mpeg4 decode buffer framerate 0.1
 * @tc.desc      : function test
 */
HWTEST_F(Mpeg4SwdecFuncNdkTest, VIDEO_MPEG4SWDEC_PARA_0900, TestSize.Level2)
{
    if (cap_mpeg4 != nullptr) {
        if (!access("/system/lib64/media/", 0)) {
            auto vDecSample = make_shared<VDecAPI11Sample>();
            vDecSample->INP_DIR = INP_DIR_1080_30;
            vDecSample->DEFAULT_WIDTH = 1920;
            vDecSample->DEFAULT_HEIGHT = 1080;
            vDecSample->DEFAULT_FRAME_RATE = 0.1;
            vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
            vDecSample->SF_OUTPUT = false;
            ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameMpeg4));
            vDecSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
        }
    }
}

/**
 * @tc.number    : VIDEO_MPEG4SWDEC_PARA_1000
 * @tc.name      : test mpeg4 decode buffer framerate 10000
 * @tc.desc      : function test
 */
HWTEST_F(Mpeg4SwdecFuncNdkTest, VIDEO_MPEG4SWDEC_PARA_1000, TestSize.Level2)
{
    if (cap_mpeg4 != nullptr) {
        if (!access("/system/lib64/media/", 0)) {
            auto vDecSample = make_shared<VDecAPI11Sample>();
            vDecSample->INP_DIR = INP_DIR_1080_30;
            vDecSample->DEFAULT_WIDTH = 1920;
            vDecSample->DEFAULT_HEIGHT = 1080;
            vDecSample->DEFAULT_FRAME_RATE = 10000;
            vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
            vDecSample->SF_OUTPUT = false;
            ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameMpeg4));
            vDecSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
        }
    }
}

/**
 * @tc.number    : VIDEO_MPEG4SWDEC_SURF_CHANGE_0100
 * @tc.name      : surf model change in normal state
 * @tc.desc      : function test
 */
HWTEST_F(Mpeg4SwdecFuncNdkTest, VIDEO_MPEG4SWDEC_SURF_CHANGE_0100, TestSize.Level2)
{
    if (cap_mpeg4 != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_1080_30;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SURFACE_OUTPUT = true;
        vDecSample->autoSwitchSurface = true;
        vDecSample->AFTER_EOS_DESTORY_CODEC = false;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameMpeg4));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->Reset());
        ASSERT_EQ(AV_ERR_INVALID_STATE, vDecSample->SwitchSurface());
        ASSERT_EQ(AV_ERR_OK, vDecSample->Release());
    }
}

/**
 * @tc.number    : VIDEO_MPEG4SWDEC_SURF_CHANGE_0200
 * @tc.name      : surf model change in flushed state
 * @tc.desc      : function test
 */
HWTEST_F(Mpeg4SwdecFuncNdkTest, VIDEO_MPEG4SWDEC_SURF_CHANGE_0200, TestSize.Level2)
{
    if (cap_mpeg4 != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_1080_30;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SURFACE_OUTPUT = true;
        vDecSample->autoSwitchSurface = true;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameMpeg4));
        ASSERT_EQ(AV_ERR_OK, vDecSample->Flush());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
        ASSERT_EQ(AV_ERR_OK, vDecSample->Stop());
    }
}

/**
 * @tc.number    : VIDEO_MPEG4SWDEC_SURF_CHANGE_0300
 * @tc.name      : surf model change in runing state
 * @tc.desc      : function test
 */
HWTEST_F(Mpeg4SwdecFuncNdkTest, VIDEO_MPEG4SWDEC_SURF_CHANGE_0300, TestSize.Level2)
{
    if (cap_mpeg4 != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_1080_30;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SURFACE_OUTPUT = false;
        vDecSample->autoSwitchSurface = false;
        vDecSample->CreateSurface();
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameMpeg4));
        ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, vDecSample->SwitchSurface());
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_MPEG4SWDEC_SURF_CHANGE_0400
 * @tc.name      : repeat call setSurface fastly
 * @tc.desc      : function test
 */
HWTEST_F(Mpeg4SwdecFuncNdkTest, VIDEO_MPEG4SWDEC_SURF_CHANGE_0400, TestSize.Level2)
{
    if (cap_mpeg4 != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_1080_30;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SURFACE_OUTPUT = true;
        vDecSample->autoSwitchSurface = true;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameMpeg4));
        ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
        vDecSample->WaitForEOS();
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_RESOLUTION_PROFILE_0010
 * @tc.name      : Resolution and profile change
 * @tc.desc      : function test
 */
HWTEST_F(Mpeg4SwdecFuncNdkTest, VIDEO_SWDEC_RESOLUTION_PROFILE_0010, TestSize.Level2)
{
    if (cap_mpeg4 != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = "/data/test/media/profResoChange.m4v";
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SURFACE_OUTPUT = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMpeg4));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(2110, vDecSample->outFrameCount);
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_SYNC_SWMPEG4_FUNC_0010
 * @tc.name      : mpeg4同步软解输出nv12
 * @tc.desc      : function test
 */
HWTEST_F(Mpeg4SwdecFuncNdkTest, VIDEO_DECODE_SYNC_SWMPEG4_FUNC_0010, TestSize.Level1)
{
    if (cap_mpeg4 != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_1080_30;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->enbleSyncMode = 1;
        vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMpeg4));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartSyncVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_SYNC_SWMPEG4_FUNC_0020
 * @tc.name      : mpeg4同步软解输出nv21
 * @tc.desc      : function test
 */
HWTEST_F(Mpeg4SwdecFuncNdkTest, VIDEO_DECODE_SYNC_SWMPEG4_FUNC_0020, TestSize.Level0)
{
    if (cap_mpeg4 != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_1080_30;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->enbleSyncMode = 1;
        vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV21;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMpeg4));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartSyncVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_SYNC_SWMPEG4_FUNC_0030
 * @tc.name      : mpeg4同步软解输出surface
 * @tc.desc      : function test
 */
HWTEST_F(Mpeg4SwdecFuncNdkTest, VIDEO_DECODE_SYNC_SWMPEG4_FUNC_0030, TestSize.Level1)
{
    if (cap_mpeg4 != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_1080_30;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SURFACE_OUTPUT = true;
        vDecSample->enbleSyncMode = 1;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMpeg4));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->DecodeSetSurface());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartSyncVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_DECODE_SYNC_SWMPEG4_FUNC_0040
 * @tc.name      : 264同步软解输出rgba
 * @tc.desc      : function test
 */
HWTEST_F(Mpeg4SwdecFuncNdkTest, VIDEO_DECODE_SYNC_SWMPEG4_FUNC_0040, TestSize.Level1)
{
    if (cap_mpeg4 != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_1080_30;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->enbleSyncMode = 1;
        vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_RGBA;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMpeg4));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartSyncVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_MPEG4SWDEC_BLANK_FRAME_0010
 * @tc.name      : config OH_MD_KEY_VIDEO_DECODER_BLANK_FRAME_ON_SHUTDOWN, decoder mpeg4
 * @tc.desc      : function test
 */
HWTEST_F(Mpeg4SwdecFuncNdkTest, VIDEO_MPEG4SWDEC_BLANK_FRAME_0010, TestSize.Level2)
{
    if (cap_mpeg4 != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_1080_30;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->enbleBlankFrame = 1;
        vDecSample->SF_OUTPUT = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameMpeg4));
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_MPEG4SWDEC_BLANK_FRAME_0020
 * @tc.name      :  config OH_MD_KEY_VIDEO_DECODER_BLANK_FRAME_ON_SHUTDOWN, decoder mpeg4, surface
 * @tc.desc      : function test
 */
HWTEST_F(Mpeg4SwdecFuncNdkTest, VIDEO_MPEG4SWDEC_BLANK_FRAME_0020, TestSize.Level2)
{
    if (cap_mpeg4 != nullptr) {
            auto vDecSample = make_shared<VDecAPI11Sample>();
            vDecSample->INP_DIR = INP_DIR_1080_30;
            vDecSample->DEFAULT_WIDTH = 1920;
            vDecSample->DEFAULT_HEIGHT = 1080;
            vDecSample->DEFAULT_FRAME_RATE = 30;
            vDecSample->enbleBlankFrame = 1;
            vDecSample->SF_OUTPUT = true;
            ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMpeg4));
            ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
            ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
            ASSERT_EQ(AV_ERR_OK, vDecSample->DecodeSetSurface());
            ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
            vDecSample->WaitForEOS();
            ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_MPEG4_FLUSH_0010
 * @tc.name      : create-start-eos-flush-start-eos
 * @tc.desc      : function test
 */
HWTEST_F(Mpeg4SwdecFuncNdkTest, VIDEO_SWDEC_MPEG4_FLUSH_0010, TestSize.Level1)
{
    if (cap_mpeg4 != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_1080_30;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameMpeg4));
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
 * @tc.number    : VIDEO_SWDEC_MPEG4_FLUSH_0020
 * @tc.name      : create-start-eos-flush-start-eos,surface
 * @tc.desc      : function test
 */
HWTEST_F(Mpeg4SwdecFuncNdkTest, VIDEO_SWDEC_MPEG4_FLUSH_0020, TestSize.Level1)
{
    if (cap_mpeg4 != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_1080_30;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameMpeg4));
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
 * @tc.number    : VIDEO_SWDEC_PIXE_FORMAT_MPEG4_0010
 * @tc.name      : mpeg4 pixelformat query key
 * @tc.desc      : function test
 */
HWTEST_F(Mpeg4SwdecFuncNdkTest, VIDEO_SWDEC_PIXE_FORMAT_MPEG4_0010, TestSize.Level2)
{
    if (cap_mpeg4 != nullptr) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_1080_30;
        vDecSample->isGetVideoSupportedPixelFormats = true;
        vDecSample->isGetFormatKey = true;
        vDecSample->avcodecMimeType = OH_AVCODEC_MIMETYPE_VIDEO_MPEG4;
        vDecSample->isEncoder = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMpeg4));
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