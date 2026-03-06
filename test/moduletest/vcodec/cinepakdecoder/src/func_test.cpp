/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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
#include <fcntl.h>
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
#include "videoenc_sample.h"

#define MAX_THREAD 16

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;

namespace {
OH_AVCapability *g_cap = nullptr;
static constexpr uint32_t CODEC_NAME_SIZE = 128;
char g_codecName[CODEC_NAME_SIZE] = {};
} // namespace
namespace OHOS {
namespace Media {
class CinepakdecFuncNdkTest : public testing::Test {
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
    const char *INP_DIR_1 = "/data/test/media/cinepak.cinepak";
    const char *INP_DIR_2 = "/data/test/media/cinepak.mkv";
    const char *INP_DIR_3 = "/data/test/media/cinepak.mov";
    const char *INP_DIR_4 = "/data/test/media/cinepak.asf";
    const char *INP_DIR_5 = "/data/test/media/cinepak.avi";
    const char *INP_DIR_6 = "/data/test/media/cinepak_4_4_30.mkv";
    const char *INP_DIR_7 = "/data/test/media/cinepak_480_640_30.mkv";
    const char *INP_DIR_8 = "/data/test/media/cinepak_600_800_30.mkv";
    const char *INP_DIR_9 = "/data/test/media/cinepak_720_480_25.mkv";
    const char *INP_DIR_10 = "/data/test/media/cinepak_1080_1920_30.mkv";
    const char *INP_DIR_11 = "/data/test/media/cinepak_1280_720_30.mkv";
    const char *INP_DIR_12 = "/data/test/media/cinepak_1920_1080_30.mkv";
    const char *INP_DIR_13 = "/data/test/media/cinepak_2560_1440_30.mkv";
    const char *INP_DIR_14 = "/data/test/media/cinepak_3840_1600_30.mkv";
    const char *INP_DIR_15 = "/data/test/media/cinepak_3840_2160_30.mkv";
    const char *INP_DIR_16 = "/data/test/media/cinepak_4096_4096.mkv";
};

static OH_AVCapability *cap_cinepak = nullptr;
static string g_codecNameCinepak = "";
static constexpr uint32_t FRAMESIZE5 = 5;
static constexpr uint32_t FRAMESIZE20 = 20;
static constexpr uint32_t DEFAULTWIDTH = 720;
static constexpr uint32_t DEFAULTHEIGHT = 480;
void CinepakdecFuncNdkTest::SetUpTestCase()
{
    cap_cinepak = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_CINEPAK, false, SOFTWARE);
    g_codecNameCinepak = OH_AVCapability_GetName(cap_cinepak);
    cout << "g_codecNameCinepak: " << g_codecNameCinepak << endl;
    g_cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_AVC, true, HARDWARE);
    const char *tmpCodecName = OH_AVCapability_GetName(g_cap);
    if (memcpy_s(g_codecName, sizeof(g_codecName), tmpCodecName, strlen(tmpCodecName)) != 0)
        cout << "memcpy failed" << endl;
    cout << "codecname: " << g_codecName << endl;
}

void CinepakdecFuncNdkTest::TearDownTestCase() {}
void CinepakdecFuncNdkTest::SetUp() {}
void CinepakdecFuncNdkTest::TearDown() {}
} // namespace Media
} // namespace OHOS

static void getVideoSizeRange(OH_AVRange *widthRange, OH_AVRange *heightRange)
{
    memset_s(widthRange, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    memset_s(heightRange, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_CINEPAK, false, SOFTWARE);
    OH_AVCapability_GetVideoHeightRangeForWidth(capability, DEFAULTWIDTH, heightRange);
    cout << "minval=" << heightRange->minVal << "  maxval=" << heightRange->maxVal << endl;
    OH_AVCapability_GetVideoWidthRangeForHeight(capability, DEFAULTHEIGHT, widthRange);
    cout << "minval=" << widthRange->minVal << "  maxval=" << widthRange->maxVal << endl;
}


namespace {
/**
 * @tc.number    : VIDEO_CINEPAKDEC_FUNCTION_0001
 * @tc.name      : Decode Cinepak buffer, 720x480, NV12
 * @tc.desc      : function test
 */
HWTEST_F(CinepakdecFuncNdkTest, VIDEO_CINEPAKDEC_FUNCTION_0001, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameCinepak.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE20, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_CINEPAKDEC_FUNCTION_0002
 * @tc.name      : Decode Cinepak buffer with 90-degree rotation
 * @tc.desc      : function test
 */
HWTEST_F(CinepakdecFuncNdkTest, VIDEO_CINEPAKDEC_FUNCTION_0002, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = INP_DIR_1;
    vDecSample->getFormat(INP_DIR_2);
    int32_t angle = 90;
    (void)OH_AVFormat_SetIntValue(vDecSample->trackFormat, OH_MD_KEY_ROTATION, angle);
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameCinepak.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE20, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_CINEPAKDEC_FUNCTION_0003
 * @tc.name      : Decode Cinepak buffer with 180-degree rotation
 * @tc.desc      : function test
 */
HWTEST_F(CinepakdecFuncNdkTest, VIDEO_CINEPAKDEC_FUNCTION_0003, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = INP_DIR_1;
    vDecSample->getFormat(INP_DIR_2);
    int32_t angle = 180;
    (void)OH_AVFormat_SetIntValue(vDecSample->trackFormat, OH_MD_KEY_ROTATION, angle);
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameCinepak.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE20, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_CINEPAKDEC_FUNCTION_0004
 * @tc.name      : Decode Cinepak buffer with 270-degree rotation
 * @tc.desc      : function test
 */
HWTEST_F(CinepakdecFuncNdkTest, VIDEO_CINEPAKDEC_FUNCTION_0004, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = INP_DIR_1;
    vDecSample->getFormat(INP_DIR_2);
    int32_t angle = 270;
    (void)OH_AVFormat_SetIntValue(vDecSample->trackFormat, OH_MD_KEY_ROTATION, angle);
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameCinepak.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE20, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_CINEPAKDEC_FUNCTION_0005
 * @tc.name      : Decode H.263 buffer with NV12 pixel format
 * @tc.desc      : function test
 */
HWTEST_F(CinepakdecFuncNdkTest, VIDEO_CINEPAKDEC_FUNCTION_0005, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->getFormat(INP_DIR_2);
    vDecSample->inputDir = "/data/test/media/profile0_level10_I_128x96.h263";
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameCinepak.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderFor263());
    vDecSample->WaitForEOS();
    cout << "outFrameCount" << vDecSample->outFrameCount << endl;
    ASSERT_EQ(false, vDecSample->isonError);
}

/**
 * @tc.number    : VIDEO_CINEPAKDEC_FUNCTION_0006
 * @tc.name      : Decode Cinepak buffer with invalid resolution below minimum
 * @tc.desc      : function test
 */
HWTEST_F(CinepakdecFuncNdkTest, VIDEO_CINEPAKDEC_FUNCTION_0006, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = INP_DIR_2;
    vDecSample->outputYuvFlag = true;
    OH_AVRange widthRange;
    OH_AVRange heightRange;
    getVideoSizeRange(&widthRange, &heightRange);
    vDecSample->defaultWidth = widthRange.minVal - 1;
    vDecSample->defaultHeight = heightRange.minVal - 1;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameCinepak.c_str()));
    ASSERT_EQ(AV_ERR_INVALID_VAL, vDecSample->ConfigureVideoDecoder());
}

/**
 * @tc.number    : VIDEO_CINEPAKDEC_FUNCTION_0007
 * @tc.name      : Decode Cinepak buffer with invalid resolution above maximum
 * @tc.desc      : function test
 */
HWTEST_F(CinepakdecFuncNdkTest, VIDEO_CINEPAKDEC_FUNCTION_0007, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    OH_AVRange widthRange;
    OH_AVRange heightRange;
    getVideoSizeRange(&widthRange, &heightRange);
    vDecSample->defaultWidth = widthRange.maxVal + 1;
    vDecSample->defaultHeight = heightRange.maxVal + 1;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameCinepak.c_str()));
    ASSERT_EQ(AV_ERR_INVALID_VAL, vDecSample->ConfigureVideoDecoder());
}

/**
 * @tc.number    : VIDEO_CINEPAKDEC_FUNCTION_0008
 * @tc.name      : Decode Cinepak buffer with extremely large resolution
 * @tc.desc      : function test
 */
HWTEST_F(CinepakdecFuncNdkTest, VIDEO_CINEPAKDEC_FUNCTION_0008, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    int32_t max = 2147483647;
    vDecSample->defaultWidth = max;
    vDecSample->defaultHeight = max;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameCinepak.c_str()));
    ASSERT_EQ(AV_ERR_INVALID_VAL, vDecSample->ConfigureVideoDecoder());
}

/**
 * @tc.number    : VIDEO_CINEPAKDEC_FUNCTION_0011
 * @tc.name      : Decode Cinepak buffer with negative resolution
 * @tc.desc      : function test
 */
HWTEST_F(CinepakdecFuncNdkTest, VIDEO_CINEPAKDEC_FUNCTION_0011, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    vDecSample->defaultWidth = -1;
    vDecSample->defaultHeight = -1;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameCinepak.c_str()));
    ASSERT_EQ(AV_ERR_INVALID_VAL, vDecSample->ConfigureVideoDecoder());
}

/**
 * @tc.number    : VIDEO_CINEPAKDEC_FUNCTION_0012
 * @tc.name      : Decode Cinepak buffer with unsupported pixel format (-1)
 * @tc.desc      : function test
 */
HWTEST_F(CinepakdecFuncNdkTest, VIDEO_CINEPAKDEC_FUNCTION_0012, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    vDecSample->defaultPixelFormat = -1;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameCinepak.c_str()));
    ASSERT_EQ(AV_ERR_UNSUPPORT, vDecSample->ConfigureVideoDecoder());
}

/**
 * @tc.number    : VIDEO_CINEPAKDEC_FUNCTION_0013
 * @tc.name      : Decode Cinepak buffer with unsupported pixel format (0)
 * @tc.desc      : function test
 */
HWTEST_F(CinepakdecFuncNdkTest, VIDEO_CINEPAKDEC_FUNCTION_0013, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    vDecSample->defaultPixelFormat = 0;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameCinepak.c_str()));
    ASSERT_EQ(AV_ERR_UNSUPPORT, vDecSample->ConfigureVideoDecoder());
}

/**
 * @tc.number    : VIDEO_CINEPAKDEC_FUNCTION_0014
 * @tc.name      : Decode Cinepak buffer with supported pixel format (3)
 * @tc.desc      : function test
 */
HWTEST_F(CinepakdecFuncNdkTest, VIDEO_CINEPAKDEC_FUNCTION_0014, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = INP_DIR_1;
    vDecSample->getFormat(INP_DIR_2);
    vDecSample->outputYuvFlag = true;
    vDecSample->defaultPixelFormat = 3;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameCinepak.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE20, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_CINEPAKDEC_FUNCTION_0015
 * @tc.name      : Decode Cinepak buffer with unsupported pixel format (max int)
 * @tc.desc      : function test
 */
HWTEST_F(CinepakdecFuncNdkTest, VIDEO_CINEPAKDEC_FUNCTION_0015, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    int32_t max = 2147483647;
    vDecSample->defaultPixelFormat = max;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameCinepak.c_str()));
    ASSERT_EQ(AV_ERR_UNSUPPORT, vDecSample->ConfigureVideoDecoder());
}

/**
 * @tc.number    : VIDEO_CINEPAKDEC_FUNCTION_0017
 * @tc.name      : Decode Cinepak buffer with NV21 pixel format
 * @tc.desc      : function test
 */
HWTEST_F(CinepakdecFuncNdkTest, VIDEO_CINEPAKDEC_FUNCTION_0017, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->getFormat(INP_DIR_2);
    vDecSample->outputYuvFlag = true;
    vDecSample->needCheckHash = true;
    vDecSample->defaultWidth = 720;
    vDecSample->defaultHeight = 480;
    vDecSample->defaultFrameRate = 30;
    vDecSample->defaultPixelFormat = AV_PIXEL_FORMAT_NV21;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameCinepak.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE20, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_CINEPAKDEC_FUNCTION_0018
 * @tc.name      : Decode Cinepak surface with NV12 pixel format
 * @tc.desc      : function test
 */
HWTEST_F(CinepakdecFuncNdkTest, VIDEO_CINEPAKDEC_FUNCTION_0018, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->getFormat(INP_DIR_2);
    vDecSample->sfOutput = true;
    vDecSample->outputYuvSurface = true;
    vDecSample->defaultPixelFormat = AV_PIXEL_FORMAT_NV12;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameCinepak.c_str()));
    vDecSample->WaitForEOS();
    ASSERT_EQ(FRAMESIZE20, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_CINEPAKDEC_FUNCTION_0019
 * @tc.name      : Decode Cinepak surface with NV21 pixel format
 * @tc.desc      : function test
 */
HWTEST_F(CinepakdecFuncNdkTest, VIDEO_CINEPAKDEC_FUNCTION_0019, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->getFormat(INP_DIR_2);
    vDecSample->sfOutput = true;
    vDecSample->outputYuvSurface = true;
    vDecSample->defaultPixelFormat = AV_PIXEL_FORMAT_NV21;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameCinepak.c_str()));
    vDecSample->WaitForEOS();
    ASSERT_EQ(FRAMESIZE20, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_CINEPAKDEC_FUNCTION_0020
 * @tc.name      : Surface model change in normal state
 * @tc.desc      : function test
 */
HWTEST_F(CinepakdecFuncNdkTest, VIDEO_CINEPAKDEC_FUNCTION_0020, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->getFormat(INP_DIR_2);
    vDecSample->sfOutput = true;
    vDecSample->autoSwitchSurface = true;
    vDecSample->afterEosDestroyCodec = false;
    vDecSample->sleepOnFPS = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameCinepak.c_str()));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->Reset());
    ASSERT_EQ(AV_ERR_INVALID_STATE, vDecSample->SwitchSurface());
    ASSERT_EQ(AV_ERR_OK, vDecSample->Release());
}

/**
 * @tc.number    : VIDEO_CINEPAKDEC_FUNCTION_0021
 * @tc.name      : Surface model change in flushed state
 * @tc.desc      : function test
 */
HWTEST_F(CinepakdecFuncNdkTest, VIDEO_CINEPAKDEC_FUNCTION_0021, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->getFormat(INP_DIR_2);
    vDecSample->sfOutput = true;
    vDecSample->autoSwitchSurface = true;
    vDecSample->sleepOnFPS = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameCinepak.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->Flush());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
    ASSERT_EQ(AV_ERR_OK, vDecSample->Stop());
}

/**
 * @tc.number    : VIDEO_CINEPAKDEC_FUNCTION_0022
 * @tc.name      : Surface model change in running state
 * @tc.desc      : function test
 */
HWTEST_F(CinepakdecFuncNdkTest, VIDEO_CINEPAKDEC_FUNCTION_0022, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->getFormat(INP_DIR_2);
    vDecSample->sfOutput = false;
    vDecSample->autoSwitchSurface = false;
    vDecSample->CreateSurface();
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameCinepak.c_str()));
    ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, vDecSample->SwitchSurface());
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_CINEPAKDEC_FUNCTION_0023
 * @tc.name      : Repeat call setSurface quickly
 * @tc.desc      : function test
 */
HWTEST_F(CinepakdecFuncNdkTest, VIDEO_CINEPAKDEC_FUNCTION_0023, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->getFormat(INP_DIR_2);
    vDecSample->sfOutput = true;
    vDecSample->autoSwitchSurface = true;
    vDecSample->sleepOnFPS = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameCinepak.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
    vDecSample->WaitForEOS();
}

/**
 * @tc.number    : VIDEO_CINEPAKDEC_FUNCTION_0024
 * @tc.name      : Surface model change in flush to running state
 * @tc.desc      : function test
 */
HWTEST_F(CinepakdecFuncNdkTest, VIDEO_CINEPAKDEC_FUNCTION_0024, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->getFormat(INP_DIR_2);
    vDecSample->sfOutput = true;
    vDecSample->autoSwitchSurface = true;
    vDecSample->afterEosDestroyCodec = false;
    vDecSample->sleepOnFPS = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameCinepak.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->Flush());
    ASSERT_EQ(AV_ERR_OK, vDecSample->Start());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
}

/**
 * @tc.number    : VIDEO_CINEPAKDEC_FUNCTION_0025
 * @tc.name      : Surface model change in decoder finish to End-of-Stream state
 * @tc.desc      : function test
 */
HWTEST_F(CinepakdecFuncNdkTest, VIDEO_CINEPAKDEC_FUNCTION_0025, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->getFormat(INP_DIR_2);
    vDecSample->sfOutput = true;
    vDecSample->autoSwitchSurface = true;
    vDecSample->afterEosDestroyCodec = false;
    vDecSample->sleepOnFPS = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameCinepak.c_str()));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
}

/**
 * @tc.number    : VIDEO_CINEPAKDEC_FUNCTION_0026
 * @tc.name      : Buffer model change in decoder finish to End-of-Stream state
 * @tc.desc      : function test
 */
HWTEST_F(CinepakdecFuncNdkTest, VIDEO_CINEPAKDEC_FUNCTION_0026, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->getFormat(INP_DIR_2);
    vDecSample->sfOutput = false;
    vDecSample->autoSwitchSurface = false;
    vDecSample->CreateSurface();
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameCinepak.c_str()));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, vDecSample->SwitchSurface());
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_CINEPAKDEC_FUNCTION_0027
 * @tc.name      : Buffer model change in running to flushed state
 * @tc.desc      : function test
 */
HWTEST_F(CinepakdecFuncNdkTest, VIDEO_CINEPAKDEC_FUNCTION_0027, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->getFormat(INP_DIR_2);
    vDecSample->sfOutput = false;
    vDecSample->autoSwitchSurface = false;
    vDecSample->CreateSurface();
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameCinepak.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->Flush());
    ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, vDecSample->SwitchSurface());
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_CINEPAKDEC_FUNCTION_0028
 * @tc.name      : Buffer model change in flushed to running state
 * @tc.desc      : function test
 */
HWTEST_F(CinepakdecFuncNdkTest, VIDEO_CINEPAKDEC_FUNCTION_0028, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->getFormat(INP_DIR_2);
    vDecSample->sfOutput = false;
    vDecSample->autoSwitchSurface = false;
    vDecSample->CreateSurface();
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameCinepak.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->Flush());
    ASSERT_EQ(AV_ERR_OK, vDecSample->Start());
    ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, vDecSample->SwitchSurface());
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_CINEPAKDEC_FUNCTION_0029
 * @tc.name      : Buffer model change in normal state
 * @tc.desc      : function test
 */
HWTEST_F(CinepakdecFuncNdkTest, VIDEO_CINEPAKDEC_FUNCTION_0029, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->sfOutput = false;
    vDecSample->autoSwitchSurface = false;
    vDecSample->CreateSurface();
    vDecSample->sfOutput = false;
    vDecSample->autoSwitchSurface = false;
    vDecSample->CreateSurface();
    
    vDecSample->getFormat(INP_DIR_2);
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameCinepak.c_str()));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->Reset());
    ASSERT_EQ(AV_ERR_INVALID_STATE, vDecSample->SwitchSurface());
    ASSERT_EQ(AV_ERR_OK, vDecSample->Release());
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_CINEPAKDEC_FUNCTION_0030
 * @tc.name      : Surface model change in config state
 * @tc.desc      : function test
 */
HWTEST_F(CinepakdecFuncNdkTest, VIDEO_CINEPAKDEC_FUNCTION_0030, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->getFormat(INP_DIR_2);
    vDecSample->sfOutput = false;
    vDecSample->autoSwitchSurface = false;
    vDecSample->CreateSurface();
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameCinepak.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_CINEPAKDEC_FUNCTION_0031
 * @tc.name      : Surface model change in create state
 * @tc.desc      : function test
 */
HWTEST_F(CinepakdecFuncNdkTest, VIDEO_CINEPAKDEC_FUNCTION_0031, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    
    vDecSample->inputDir = INP_DIR_1;
    vDecSample->getFormat(INP_DIR_2);
    vDecSample->sfOutput = false;
    vDecSample->autoSwitchSurface = false;
    vDecSample->CreateSurface();
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameCinepak.c_str()));;
    ASSERT_EQ(AV_ERR_INVALID_STATE, vDecSample->SwitchSurface());
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_CINEPAKDEC_FUNCTION_0032
 * @tc.name      : Two object repeat call setSurface quickly
 * @tc.desc      : function test
 */
HWTEST_F(CinepakdecFuncNdkTest, VIDEO_CINEPAKDEC_FUNCTION_0032, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->getFormat(INP_DIR_2);
    vDecSample->sfOutput = true;
    vDecSample->autoSwitchSurface = true;
    vDecSample->sleepOnFPS = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameCinepak.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
    vDecSample->WaitForEOS();
    vDecSample->ReleaseInFile();

    auto vDecSample_1 = make_shared<VDecAPI11Sample>();
    vDecSample_1->getFormat(INP_DIR_2);
    vDecSample_1->sfOutput = true;
    vDecSample_1->autoSwitchSurface = true;
    vDecSample_1->sleepOnFPS = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample_1->RunVideoDec_Surface(g_codecNameCinepak.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample_1->SwitchSurface());
    vDecSample_1->WaitForEOS();
    vDecSample_1->ReleaseInFile();
}

/**
 * @tc.number    : VIDEO_CINEPAKDEC_FUNCTION_0033
 * @tc.name      : Repeat call setSurface quickly 2 times
 * @tc.desc      : function test
 */
HWTEST_F(CinepakdecFuncNdkTest, VIDEO_CINEPAKDEC_FUNCTION_0033, TestSize.Level0)
{
    for (int i = 0; i < 2; i++) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->getFormat(INP_DIR_2);
        vDecSample->sfOutput = true;
        vDecSample->autoSwitchSurface = true;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameCinepak.c_str()));
        ASSERT_EQ(AV_ERR_OK, vDecSample->RepeatCallSetSurface());
        vDecSample->WaitForEOS();
        vDecSample->ReleaseInFile();
    }
}

/**
 * @tc.number    : VIDEO_CINEPAKDEC_FUNCTION_0034
 * @tc.name      : Decode cinepak buffer with demuxer mov format
 * @tc.desc      : function test
 */
HWTEST_F(CinepakdecFuncNdkTest, VIDEO_CINEPAKDEC_FUNCTION_0034, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->getFormat(INP_DIR_3);
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameCinepak.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE20, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_CINEPAKDEC_FUNCTION_0035
 * @tc.name      : Decode cinepak buffer with demuxer mp4 format
 * @tc.desc      : function test
 */
HWTEST_F(CinepakdecFuncNdkTest, VIDEO_CINEPAKDEC_FUNCTION_0035, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->getFormat(INP_DIR_4);
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameCinepak.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE20, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_CINEPAKDEC_FUNCTION_0036
 * @tc.name      : Decode cinepak buffer with demuxer avi format
 * @tc.desc      : function test
 */
HWTEST_F(CinepakdecFuncNdkTest, VIDEO_CINEPAKDEC_FUNCTION_0036, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->getFormat(INP_DIR_5);
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameCinepak.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE20, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_CINEPAKDEC_FUNCTION_0038
 * @tc.name      : Decode Cinepak buffer and encode to H.264
 * @tc.desc      : function test
 */
HWTEST_F(CinepakdecFuncNdkTest, VIDEO_CINEPAKDEC_FUNCTION_0038, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = INP_DIR_1;
    vDecSample->getFormat(INP_DIR_2);
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameCinepak.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE20, vDecSample->outFrameCount);

    auto vEncSample = make_unique<VEncNdkSample>();
    vEncSample->inputDir = vDecSample->outputDir;
    vEncSample->defaultWidth = 720;
    vEncSample->defaultHeight = 480;
    vEncSample->defaultFrameRate = 30;
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

/**
 * @tc.number    : VIDEO_CINEPAKDEC_FUNCTION_0047
 * @tc.name      : Cinepak synchronous software decoding with NV12 output
 * @tc.desc      : function test
 */
HWTEST_F(CinepakdecFuncNdkTest, VIDEO_CINEPAKDEC_FUNCTION_0047, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = INP_DIR_1;
    vDecSample->getFormat(INP_DIR_2);
    vDecSample->enbleSyncMode = 1;
    vDecSample->defaultPixelFormat = AV_PIXEL_FORMAT_NV12;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameCinepak.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartSyncVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE20, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_CINEPAKDEC_FUNCTION_0048
 * @tc.name      : Cinepak synchronous software decoding with NV21 output
 * @tc.desc      : function test
 */
HWTEST_F(CinepakdecFuncNdkTest, VIDEO_CINEPAKDEC_FUNCTION_0048, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = INP_DIR_1;
    vDecSample->getFormat(INP_DIR_2);
    vDecSample->enbleSyncMode = 1;
    vDecSample->defaultPixelFormat = AV_PIXEL_FORMAT_NV21;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameCinepak.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartSyncVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE20, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_CINEPAKDEC_FUNCTION_0049
 * @tc.name      : Cinepak synchronous software decoding with surface output
 * @tc.desc      : function test
 */
HWTEST_F(CinepakdecFuncNdkTest, VIDEO_CINEPAKDEC_FUNCTION_0049, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = INP_DIR_1;
    vDecSample->getFormat(INP_DIR_2);
    vDecSample->sfOutput = true;
    vDecSample->enbleSyncMode = 1;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameCinepak.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->DecodeSetSurface());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartSyncVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(FRAMESIZE20, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_CINEPAKDEC_FUNCTION_0050
 * @tc.name      : Decode Cinepak buffer No PixelFormat
 * @tc.desc      : function test
 */
HWTEST_F(CinepakdecFuncNdkTest, VIDEO_CINEPAKDEC_FUNCTION_0050, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->getFormat(INP_DIR_2);
    vDecSample->outputYuvFlag = true;
    vDecSample->needCheckHash = true;
    vDecSample->defaultWidth = 720;
    vDecSample->defaultHeight = 480;
    vDecSample->defaultFrameRate = 30;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameCinepak.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoderNoPixelFormat());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE20, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_CINEPAKDEC_FUNCTION_0051
 * @tc.name      : Decode cinepak buffer with demuxer mkv format
 * @tc.desc      : function test
 */
HWTEST_F(CinepakdecFuncNdkTest, VIDEO_CINEPAKDEC_FUNCTION_0051, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->getFormat(INP_DIR_2);
    vDecSample->outputYuvFlag = true;
    vDecSample->needCheckHash = true;
    vDecSample->defaultWidth = 720;
    vDecSample->defaultHeight = 480;
    vDecSample->defaultFrameRate = 30;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameCinepak.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE20, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_CINEPAKDEC_FUNCTION_0052
 * @tc.name      : Decode cinepak buffer with 4x4
 * @tc.desc      : function test
 */
HWTEST_F(CinepakdecFuncNdkTest, VIDEO_CINEPAKDEC_FUNCTION_0052, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->getFormat(INP_DIR_6);
    vDecSample->outputYuvFlag = true;
    vDecSample->defaultWidth = 4;
    vDecSample->defaultHeight = 4;
    vDecSample->defaultFrameRate = 30;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameCinepak.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE20, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_CINEPAKDEC_FUNCTION_0053
 * @tc.name      : Decode cinepak buffer with 480x640
 * @tc.desc      : function test
 */
HWTEST_F(CinepakdecFuncNdkTest, VIDEO_CINEPAKDEC_FUNCTION_0053, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->getFormat(INP_DIR_7);
    vDecSample->outputYuvFlag = true;
    vDecSample->defaultWidth = 480;
    vDecSample->defaultHeight = 640;
    vDecSample->defaultFrameRate = 30;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameCinepak.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE20, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_CINEPAKDEC_FUNCTION_0054
 * @tc.name      : Decode cinepak buffer with 600x800
 * @tc.desc      : function test
 */
HWTEST_F(CinepakdecFuncNdkTest, VIDEO_CINEPAKDEC_FUNCTION_0054, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->getFormat(INP_DIR_8);
    vDecSample->outputYuvFlag = true;
    vDecSample->defaultWidth = 600;
    vDecSample->defaultHeight = 800;
    vDecSample->defaultFrameRate = 30;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameCinepak.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE20, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_CINEPAKDEC_FUNCTION_0055
 * @tc.name      : Decode cinepak buffer with 720x480
 * @tc.desc      : function test
 */
HWTEST_F(CinepakdecFuncNdkTest, VIDEO_CINEPAKDEC_FUNCTION_0055, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->getFormat(INP_DIR_9);
    vDecSample->outputYuvFlag = true;
    vDecSample->defaultWidth = 720;
    vDecSample->defaultHeight = 480;
    vDecSample->defaultFrameRate = 25;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameCinepak.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE5, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_CINEPAKDEC_FUNCTION_0056
 * @tc.name      : Decode cinepak buffer with 1080x1920
 * @tc.desc      : function test
 */
HWTEST_F(CinepakdecFuncNdkTest, VIDEO_CINEPAKDEC_FUNCTION_0056, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->getFormat(INP_DIR_10);
    vDecSample->outputYuvFlag = true;
    vDecSample->defaultWidth = 1080;
    vDecSample->defaultHeight = 1920;
    vDecSample->defaultFrameRate = 30;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameCinepak.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE5, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_CINEPAKDEC_FUNCTION_0057
 * @tc.name      : Decode cinepak buffer with 1280x720
 * @tc.desc      : function test
 */
HWTEST_F(CinepakdecFuncNdkTest, VIDEO_CINEPAKDEC_FUNCTION_0057, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->getFormat(INP_DIR_11);
    vDecSample->outputYuvFlag = true;
    vDecSample->defaultWidth = 1280;
    vDecSample->defaultHeight = 720;
    vDecSample->defaultFrameRate = 30;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameCinepak.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE20, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_CINEPAKDEC_FUNCTION_0058
 * @tc.name      : Decode cinepak buffer with 1920x1080
 * @tc.desc      : function test
 */
HWTEST_F(CinepakdecFuncNdkTest, VIDEO_CINEPAKDEC_FUNCTION_0058, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->getFormat(INP_DIR_12);
    vDecSample->outputYuvFlag = true;
    vDecSample->defaultWidth = 1920;
    vDecSample->defaultHeight = 1080;
    vDecSample->defaultFrameRate = 30;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameCinepak.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE5, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_CINEPAKDEC_FUNCTION_0059
 * @tc.name      : Decode cinepak buffer with 2560x1440
 * @tc.desc      : function test
 */
HWTEST_F(CinepakdecFuncNdkTest, VIDEO_CINEPAKDEC_FUNCTION_0059, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->getFormat(INP_DIR_13);
    vDecSample->outputYuvFlag = true;
    vDecSample->defaultWidth = 2560;
    vDecSample->defaultHeight = 1440;
    vDecSample->defaultFrameRate = 30;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameCinepak.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE5, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_CINEPAKDEC_FUNCTION_0060
 * @tc.name      : Decode cinepak buffer with 3840x1600
 * @tc.desc      : function test
 */
HWTEST_F(CinepakdecFuncNdkTest, VIDEO_CINEPAKDEC_FUNCTION_0060, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->getFormat(INP_DIR_14);
    vDecSample->outputYuvFlag = true;
    vDecSample->defaultWidth = 3840;
    vDecSample->defaultHeight = 1600;
    vDecSample->defaultFrameRate = 30;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameCinepak.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE5, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_CINEPAKDEC_FUNCTION_0061
 * @tc.name      : Decode cinepak buffer with 3840x2160
 * @tc.desc      : function test
 */
HWTEST_F(CinepakdecFuncNdkTest, VIDEO_CINEPAKDEC_FUNCTION_0061, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->getFormat(INP_DIR_15);
    vDecSample->outputYuvFlag = true;
    vDecSample->defaultWidth = 3840;
    vDecSample->defaultHeight = 2160;
    vDecSample->defaultFrameRate = 30;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameCinepak.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE5, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_CINEPAKDEC_FUNCTION_0062
 * @tc.name      : Decode cinepak buffer with 4096x4096
 * @tc.desc      : function test
 */
HWTEST_F(CinepakdecFuncNdkTest, VIDEO_CINEPAKDEC_FUNCTION_0062, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->getFormat(INP_DIR_16);
    vDecSample->outputYuvFlag = true;
    vDecSample->defaultWidth = 4096;
    vDecSample->defaultHeight = 4096;
    vDecSample->defaultFrameRate = 30;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameCinepak.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE5, vDecSample->outFrameCount);
}
} // namespace