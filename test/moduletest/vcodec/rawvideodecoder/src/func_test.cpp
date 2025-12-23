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
#include <fcntl.h>
#include "libavutil/pixfmt.h"
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
    OH_AVCapability *cap = nullptr;
    constexpr uint32_t CODEC_NAME_SIZE = 128;
    char g_codecName[CODEC_NAME_SIZE] = {};
} // namespace
namespace OHOS {
namespace Media {
class RawVideodecFuncNdkTest : public testing::Test {
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
    const char *INP_DIR_1 = "/data/test/media/rawvideo_nv12.avi";
    const char *INP_DIR_2 = "/data/test/media/rawvideo_nv21.avi";
    const char *INP_DIR_3 = "/data/test/media/rawvideo_rgba.avi";
    const char *INP_DIR_4 = "/data/test/media/rawvideo_yuv420p.avi";
    const char *INP_DIR_5 = "/data/test/media/rawvideo_720_480_60.avi";
    const char *INP_DIR_6 = "/data/test/media/rawvideo.mkv";
    const char *INP_DIR_7 = "/data/test/media/rawvideo_600_800_30.avi";
    const char *INP_DIR_8 = "/data/test/media/rawvideo_4_4_30.avi";
};

static OH_AVCapability *cap_rawvideo = nullptr;
static string g_codecNameRawVideo = "";
static constexpr uint32_t FRAMESIZE5 = 5;
static constexpr uint32_t FRAMESIZE7 = 7;
static constexpr uint32_t FRAMESIZE10 = 10;
static constexpr uint32_t DEFAULTWIDTH = 720;
static constexpr uint32_t DEFAULTHEIGHT = 480;
void RawVideodecFuncNdkTest::SetUpTestCase()
{
    cap_rawvideo = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_RAWVIDEO, false, SOFTWARE);
    g_codecNameRawVideo = OH_AVCapability_GetName(cap_rawvideo);
    cout << "g_codecNameRawVideo: " << g_codecNameRawVideo << endl;
    cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_AVC, true, HARDWARE);
    const char *tmpCodecName = OH_AVCapability_GetName(cap);
    if (memcpy_s(g_codecName, sizeof(g_codecName), tmpCodecName, strlen(tmpCodecName)) != 0)
        cout << "memcpy failed" << endl;
    cout << "codecname: " << g_codecName << endl;
}

void RawVideodecFuncNdkTest::TearDownTestCase() {}
void RawVideodecFuncNdkTest::SetUp() {}
void RawVideodecFuncNdkTest::TearDown() {}
} // namespace Media
} // namespace OHOS

static void getVideoSizeRange(OH_AVRange *widthRange, OH_AVRange *heightRange)
{
    memset_s(widthRange, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    memset_s(heightRange, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_RAWVIDEO, false, SOFTWARE);
    OH_AVCapability_GetVideoHeightRangeForWidth(capability, DEFAULTWIDTH, heightRange);
    cout << "minval=" << heightRange->minVal << "  maxval=" << heightRange->maxVal << endl;
    OH_AVCapability_GetVideoWidthRangeForHeight(capability, DEFAULTHEIGHT, widthRange);
    cout << "minval=" << widthRange->minVal << "  maxval=" << widthRange->maxVal << endl;
}


namespace {
/**
 * @tc.number    : VIDEO_RAWVIDEODEC_FUNCTION_0001
 * @tc.name      : Decode RawVideo buffer, 1280x720, NV12
 * @tc.desc      : function test
 */
HWTEST_F(RawVideodecFuncNdkTest, VIDEO_RAWVIDEODEC_FUNCTION_0001, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    vDecSample->needCheckHash = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameRawVideo.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE10, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_FUNCTION_0002
 * @tc.name      : Decode RawVideo buffer with 90-degree rotation
 * @tc.desc      : function test
 */
HWTEST_F(RawVideodecFuncNdkTest, VIDEO_RAWVIDEODEC_FUNCTION_0002, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = INP_DIR_1;
    int32_t angle = 90;
    vDecSample->trackFormat = OH_AVFormat_Create();
    (void)OH_AVFormat_SetIntValue(vDecSample->trackFormat, OH_MD_KEY_ROTATION, angle);
    vDecSample->outputYuvFlag = true;
    vDecSample->needCheckHash = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameRawVideo.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE10, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_FUNCTION_0003
 * @tc.name      : Decode RawVideo buffer with 180-degree rotation
 * @tc.desc      : function test
 */
HWTEST_F(RawVideodecFuncNdkTest, VIDEO_RAWVIDEODEC_FUNCTION_0003, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = INP_DIR_1;
    int32_t angle = 180;
    vDecSample->trackFormat = OH_AVFormat_Create();
    (void)OH_AVFormat_SetIntValue(vDecSample->trackFormat, OH_MD_KEY_ROTATION, angle);
    vDecSample->outputYuvFlag = true;
    vDecSample->needCheckHash = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameRawVideo.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE10, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_FUNCTION_0004
 * @tc.name      : Decode RawVideo buffer with 270-degree rotation
 * @tc.desc      : function test
 */
HWTEST_F(RawVideodecFuncNdkTest, VIDEO_RAWVIDEODEC_FUNCTION_0004, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = INP_DIR_1;
    int32_t angle = 270;
    vDecSample->trackFormat = OH_AVFormat_Create();
    (void)OH_AVFormat_SetIntValue(vDecSample->trackFormat, OH_MD_KEY_ROTATION, angle);
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameRawVideo.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE10, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_FUNCTION_0005
 * @tc.name      : Decode H.263 buffer with NV12 pixel format
 * @tc.desc      : function test
 */
HWTEST_F(RawVideodecFuncNdkTest, VIDEO_RAWVIDEODEC_FUNCTION_0005, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = "/data/test/media/profile0_level10_I_128x96.h263";
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameRawVideo.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderFor263());
    vDecSample->WaitForEOS();
    cout << "outFrameCount" << vDecSample->outFrameCount << endl;
    ASSERT_EQ(true, vDecSample->isonError);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_FUNCTION_0006
 * @tc.name      : Decode RawVideo buffer with invalid resolution below minimum
 * @tc.desc      : function test
 */
HWTEST_F(RawVideodecFuncNdkTest, VIDEO_RAWVIDEODEC_FUNCTION_0006, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    OH_AVRange widthRange;
    OH_AVRange heightRange;
    getVideoSizeRange(&widthRange, &heightRange);
    vDecSample->defaultWidth = widthRange.minVal - 1;
    vDecSample->defaultHeight = heightRange.minVal - 1;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameRawVideo.c_str()));
    ASSERT_EQ(AV_ERR_INVALID_VAL, vDecSample->ConfigureVideoDecoder());
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_FUNCTION_0007
 * @tc.name      : Decode RawVideo buffer with invalid resolution above maximum
 * @tc.desc      : function test
 */
HWTEST_F(RawVideodecFuncNdkTest, VIDEO_RAWVIDEODEC_FUNCTION_0007, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    OH_AVRange widthRange;
    OH_AVRange heightRange;
    getVideoSizeRange(&widthRange, &heightRange);
    vDecSample->defaultWidth = widthRange.maxVal + 1;
    vDecSample->defaultHeight = heightRange.maxVal + 1;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameRawVideo.c_str()));
    ASSERT_EQ(AV_ERR_INVALID_VAL, vDecSample->ConfigureVideoDecoder());
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_FUNCTION_0008
 * @tc.name      : Decode RawVideo buffer with extremely large resolution
 * @tc.desc      : function test
 */
HWTEST_F(RawVideodecFuncNdkTest, VIDEO_RAWVIDEODEC_FUNCTION_0008, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    int32_t max = 2147483647;
    vDecSample->defaultWidth = max;
    vDecSample->defaultHeight = max;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameRawVideo.c_str()));
    ASSERT_EQ(AV_ERR_INVALID_VAL, vDecSample->ConfigureVideoDecoder());
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_FUNCTION_0009
 * @tc.name      : Decode RawVideo buffer with minimum valid resolution
 * @tc.desc      : function test
 */
HWTEST_F(RawVideodecFuncNdkTest, VIDEO_RAWVIDEODEC_FUNCTION_0009, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = INP_DIR_2;
    vDecSample->outputYuvFlag = true;
    OH_AVRange widthRange;
    OH_AVRange heightRange;
    getVideoSizeRange(&widthRange, &heightRange);
    vDecSample->defaultWidth = widthRange.minVal;
    vDecSample->defaultHeight = heightRange.minVal;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameRawVideo.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE10, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_FUNCTION_0011
 * @tc.name      : Decode RawVideo buffer with negative resolution
 * @tc.desc      : function test
 */
HWTEST_F(RawVideodecFuncNdkTest, VIDEO_RAWVIDEODEC_FUNCTION_0011, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    vDecSample->defaultWidth = -1;
    vDecSample->defaultHeight = -1;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameRawVideo.c_str()));
    ASSERT_EQ(AV_ERR_INVALID_VAL, vDecSample->ConfigureVideoDecoder());
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_FUNCTION_0012
 * @tc.name      : Decode RawVideo buffer with unsupported pixel format (-1)
 * @tc.desc      : function test
 */
HWTEST_F(RawVideodecFuncNdkTest, VIDEO_RAWVIDEODEC_FUNCTION_0012, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = -1;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameRawVideo.c_str()));
    ASSERT_EQ(AV_ERR_UNSUPPORT, vDecSample->ConfigureVideoDecoder());
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_FUNCTION_0013
 * @tc.name      : Decode RawVideo buffer with unsupported pixel format (0)
 * @tc.desc      : function test
 */
HWTEST_F(RawVideodecFuncNdkTest, VIDEO_RAWVIDEODEC_FUNCTION_0013, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = 0;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameRawVideo.c_str()));
    ASSERT_EQ(AV_ERR_UNSUPPORT, vDecSample->ConfigureVideoDecoder());
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_FUNCTION_0014
 * @tc.name      : Decode RawVideo buffer with supported pixel format (3)
 * @tc.desc      : function test
 */
HWTEST_F(RawVideodecFuncNdkTest, VIDEO_RAWVIDEODEC_FUNCTION_0014, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = 3;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameRawVideo.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE10, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_FUNCTION_0015
 * @tc.name      : Decode RawVideo buffer with unsupported pixel format (max int)
 * @tc.desc      : function test
 */
HWTEST_F(RawVideodecFuncNdkTest, VIDEO_RAWVIDEODEC_FUNCTION_0015, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    int32_t max = 2147483647;
    vDecSample->defualtPixelFormat = max;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameRawVideo.c_str()));
    ASSERT_EQ(AV_ERR_UNSUPPORT, vDecSample->ConfigureVideoDecoder());
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_FUNCTION_0016
 * @tc.name      : Decode RawVideo buffer with NV12 pixel format
 * @tc.desc      : function test
 */
HWTEST_F(RawVideodecFuncNdkTest, VIDEO_RAWVIDEODEC_FUNCTION_0016, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    vDecSample->needCheckHash = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
    vDecSample->pixFmt = AV_PIX_FMT_NV12;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameRawVideo.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE10, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_FUNCTION_0017
 * @tc.name      : Decode RawVideo buffer NV12→NV21
 * @tc.desc      : function test
 */
HWTEST_F(RawVideodecFuncNdkTest, VIDEO_RAWVIDEODEC_FUNCTION_0017, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV21;
    vDecSample->pixFmt = AV_PIX_FMT_NV12;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameRawVideo.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE10, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_FUNCTION_0019
 * @tc.name      : Decode RawVideo buffer NV12→RGBA
 * @tc.desc      : function test
 */
HWTEST_F(RawVideodecFuncNdkTest, VIDEO_RAWVIDEODEC_FUNCTION_0019, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = INP_DIR_1;
    vDecSample->outputYuvFlag = true;

    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_RGBA;
    vDecSample->pixFmt = AV_PIX_FMT_NV12;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameRawVideo.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE10, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_FUNCTION_0021
 * @tc.name      : Decode RawVideo buffer NV12→SURFACE_FORMAT
 * @tc.desc      : function test
 */
HWTEST_F(RawVideodecFuncNdkTest, VIDEO_RAWVIDEODEC_FUNCTION_0021, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_SURFACE_FORMAT;
    vDecSample->pixFmt = AV_PIX_FMT_NV12;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameRawVideo.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE10, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_FUNCTION_0022
 * @tc.name      : Decode RawVideo buffer NV21→NV12
 * @tc.desc      : function test
 */
HWTEST_F(RawVideodecFuncNdkTest, VIDEO_RAWVIDEODEC_FUNCTION_0022, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = INP_DIR_2;
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
    vDecSample->pixFmt = AV_PIX_FMT_NV21;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameRawVideo.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE10, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_FUNCTION_0023
 * @tc.name      : Decode RawVideo buffer NV21→NV21
 * @tc.desc      : function test
 */
HWTEST_F(RawVideodecFuncNdkTest, VIDEO_RAWVIDEODEC_FUNCTION_0023, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = INP_DIR_2;
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV21;
    vDecSample->pixFmt = AV_PIX_FMT_NV21;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameRawVideo.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE10, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_FUNCTION_0025
 * @tc.name      : Decode RawVideo buffer NV21→RGBA
 * @tc.desc      : function test
 */
HWTEST_F(RawVideodecFuncNdkTest, VIDEO_RAWVIDEODEC_FUNCTION_0025, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = INP_DIR_2;
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_RGBA;
    vDecSample->pixFmt = AV_PIX_FMT_NV21;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameRawVideo.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE10, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_FUNCTION_0027
 * @tc.name      : Decode RawVideo buffer NV21→SURFACE_FORMAT
 * @tc.desc      : function test
 */
HWTEST_F(RawVideodecFuncNdkTest, VIDEO_RAWVIDEODEC_FUNCTION_0027, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = INP_DIR_2;
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_SURFACE_FORMAT;
    vDecSample->pixFmt = AV_PIX_FMT_NV21;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameRawVideo.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE10, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_FUNCTION_0028
 * @tc.name      : Decode RawVideo buffer RGBA→NV12
 * @tc.desc      : function test
 */
HWTEST_F(RawVideodecFuncNdkTest, VIDEO_RAWVIDEODEC_FUNCTION_0028, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = INP_DIR_3;
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
    vDecSample->pixFmt = AV_PIX_FMT_RGBA;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameRawVideo.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE5, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_FUNCTION_0029
 * @tc.name      : Decode RawVideo buffer RGBA→NV21
 * @tc.desc      : function test
 */
HWTEST_F(RawVideodecFuncNdkTest, VIDEO_RAWVIDEODEC_FUNCTION_0029, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = INP_DIR_3;
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV21;
    vDecSample->pixFmt = AV_PIX_FMT_RGBA;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameRawVideo.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE5, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_FUNCTION_0031
 * @tc.name      : Decode RawVideo buffer RGBA→RGBA
 * @tc.desc      : function test
 */
HWTEST_F(RawVideodecFuncNdkTest, VIDEO_RAWVIDEODEC_FUNCTION_0031, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = INP_DIR_3;
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_RGBA;
    vDecSample->pixFmt = AV_PIX_FMT_RGBA;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameRawVideo.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE5, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_FUNCTION_0033
 * @tc.name      : Decode RawVideo buffer RGBA→SURFACE_FORMAT
 * @tc.desc      : function test
 */
HWTEST_F(RawVideodecFuncNdkTest, VIDEO_RAWVIDEODEC_FUNCTION_0033, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = INP_DIR_3;
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_SURFACE_FORMAT;
    vDecSample->pixFmt = AV_PIX_FMT_RGBA;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameRawVideo.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE5, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_FUNCTION_0034
 * @tc.name      : Decode RawVideo buffer YUV420P→NV12
 * @tc.desc      : function test
 */
HWTEST_F(RawVideodecFuncNdkTest, VIDEO_RAWVIDEODEC_FUNCTION_0034, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = INP_DIR_4;
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
    vDecSample->pixFmt = AV_PIX_FMT_YUV420P;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameRawVideo.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE10, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_FUNCTION_0035
 * @tc.name      : Decode RawVideo buffer YUV420P→NV21
 * @tc.desc      : function test
 */
HWTEST_F(RawVideodecFuncNdkTest, VIDEO_RAWVIDEODEC_FUNCTION_0035, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = INP_DIR_4;
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV21;
    vDecSample->pixFmt = AV_PIX_FMT_YUV420P;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameRawVideo.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE10, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_FUNCTION_0036
 * @tc.name      : Decode RawVideo buffer YUV420P→YUVI420
 * @tc.desc      : function test
 */
HWTEST_F(RawVideodecFuncNdkTest, VIDEO_RAWVIDEODEC_FUNCTION_0036, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = INP_DIR_4;
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = static_cast<uint32_t>(OHOS::MediaAVCodec::VideoPixelFormat::YUVI420);;
    vDecSample->pixFmt = AV_PIX_FMT_YUV420P;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameRawVideo.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE10, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_FUNCTION_0037
 * @tc.name      : Decode RawVideo buffer YUV420P→RGBA
 * @tc.desc      : function test
 */
HWTEST_F(RawVideodecFuncNdkTest, VIDEO_RAWVIDEODEC_FUNCTION_0037, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = INP_DIR_4;
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_RGBA;
    vDecSample->pixFmt = AV_PIX_FMT_YUV420P;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameRawVideo.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE10, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_FUNCTION_0039
 * @tc.name      : Decode RawVideo buffer YUV420P→SURFACE_FORMAT
 * @tc.desc      : function test
 */
HWTEST_F(RawVideodecFuncNdkTest, VIDEO_RAWVIDEODEC_FUNCTION_0039, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = INP_DIR_4;
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_SURFACE_FORMAT;
    vDecSample->pixFmt = AV_PIX_FMT_YUV420P;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameRawVideo.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE10, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_FUNCTION_0040
 * @tc.name      : Decode RawVideo buffer with NV12 pixel format
 * @tc.desc      : function test
 */
HWTEST_F(RawVideodecFuncNdkTest, VIDEO_RAWVIDEODEC_FUNCTION_0040, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameRawVideo.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE10, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_FUNCTION_0041
 * @tc.name      : Decode RawVideo buffer with NV21 pixel format
 * @tc.desc      : function test
 */
HWTEST_F(RawVideodecFuncNdkTest, VIDEO_RAWVIDEODEC_FUNCTION_0041, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV21;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameRawVideo.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE10, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_FUNCTION_0042
 * @tc.name      : Decode RawVideo surface with NV12 pixel format
 * @tc.desc      : function test
 */
HWTEST_F(RawVideodecFuncNdkTest, VIDEO_RAWVIDEODEC_FUNCTION_0042, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = INP_DIR_1;
    vDecSample->sfOutput = true;
    vDecSample->outputYuvSurface = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameRawVideo.c_str()));
    vDecSample->WaitForEOS();
    ASSERT_EQ(FRAMESIZE10, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_FUNCTION_0043
 * @tc.name      : Decode RawVideo surface with NV21 pixel format
 * @tc.desc      : function test
 */
HWTEST_F(RawVideodecFuncNdkTest, VIDEO_RAWVIDEODEC_FUNCTION_0043, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = INP_DIR_1;
    vDecSample->sfOutput = true;
    vDecSample->outputYuvSurface = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV21;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameRawVideo.c_str()));
    vDecSample->WaitForEOS();
    ASSERT_EQ(FRAMESIZE10, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_FUNCTION_0044
 * @tc.name      : Surface model change in normal state
 * @tc.desc      : function test
 */
HWTEST_F(RawVideodecFuncNdkTest, VIDEO_RAWVIDEODEC_FUNCTION_0044, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    
    vDecSample->inputDir = INP_DIR_1;
    vDecSample->sfOutput = true;
    vDecSample->autoSwitchSurface = true;
    vDecSample->afterEosDestroyCodec = false;
    vDecSample->sleepOnFPS = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameRawVideo.c_str()));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->Reset());
    ASSERT_EQ(AV_ERR_INVALID_STATE, vDecSample->SwitchSurface());
    ASSERT_EQ(AV_ERR_OK, vDecSample->Release());
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_FUNCTION_0045
 * @tc.name      : Surface model change in flushed state
 * @tc.desc      : function test
 */
HWTEST_F(RawVideodecFuncNdkTest, VIDEO_RAWVIDEODEC_FUNCTION_0045, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    
    vDecSample->inputDir = INP_DIR_1;
    vDecSample->sfOutput = true;
    vDecSample->autoSwitchSurface = true;
    vDecSample->sleepOnFPS = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameRawVideo.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->Flush());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
    ASSERT_EQ(AV_ERR_OK, vDecSample->Stop());
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_FUNCTION_0046
 * @tc.name      : Surface model change in running state
 * @tc.desc      : function test
 */
HWTEST_F(RawVideodecFuncNdkTest, VIDEO_RAWVIDEODEC_FUNCTION_0046, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    
    vDecSample->inputDir = INP_DIR_1;
    vDecSample->sfOutput = false;
    vDecSample->autoSwitchSurface = false;
    vDecSample->CreateSurface();
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameRawVideo.c_str()));
    ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, vDecSample->SwitchSurface());
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_FUNCTION_0047
 * @tc.name      : Repeat call setSurface quickly
 * @tc.desc      : function test
 */
HWTEST_F(RawVideodecFuncNdkTest, VIDEO_RAWVIDEODEC_FUNCTION_0047, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = INP_DIR_1;
    vDecSample->sfOutput = true;
    vDecSample->autoSwitchSurface = true;
    vDecSample->sleepOnFPS = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameRawVideo.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
    vDecSample->WaitForEOS();
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_FUNCTION_0048
 * @tc.name      : Surface model change in flush to running state
 * @tc.desc      : function test
 */
HWTEST_F(RawVideodecFuncNdkTest, VIDEO_RAWVIDEODEC_FUNCTION_0048, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = INP_DIR_1;
    vDecSample->sfOutput = true;
    vDecSample->autoSwitchSurface = true;
    vDecSample->afterEosDestroyCodec = false;
    vDecSample->sleepOnFPS = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameRawVideo.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->Flush());
    ASSERT_EQ(AV_ERR_OK, vDecSample->Start());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_FUNCTION_0049
 * @tc.name      : Surface model change in decoder finish to End-of-Stream state
 * @tc.desc      : function test
 */
HWTEST_F(RawVideodecFuncNdkTest, VIDEO_RAWVIDEODEC_FUNCTION_0049, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = INP_DIR_1;
    vDecSample->sfOutput = true;
    vDecSample->autoSwitchSurface = true;
    vDecSample->afterEosDestroyCodec = false;
    vDecSample->sleepOnFPS = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameRawVideo.c_str()));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_FUNCTION_0050
 * @tc.name      : Buffer model change in decoder finish to End-of-Stream state
 * @tc.desc      : function test
 */
HWTEST_F(RawVideodecFuncNdkTest, VIDEO_RAWVIDEODEC_FUNCTION_0050, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = INP_DIR_1;
    vDecSample->sfOutput = false;
    vDecSample->autoSwitchSurface = false;
    vDecSample->CreateSurface();
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameRawVideo.c_str()));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, vDecSample->SwitchSurface());
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_FUNCTION_0051
 * @tc.name      : Buffer model change in running to flushed state
 * @tc.desc      : function test
 */
HWTEST_F(RawVideodecFuncNdkTest, VIDEO_RAWVIDEODEC_FUNCTION_0051, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = INP_DIR_1;
    vDecSample->sfOutput = false;
    vDecSample->autoSwitchSurface = false;
    vDecSample->CreateSurface();
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameRawVideo.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->Flush());
    ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, vDecSample->SwitchSurface());
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_FUNCTION_0052
 * @tc.name      : Buffer model change in flushed to running state
 * @tc.desc      : function test
 */
HWTEST_F(RawVideodecFuncNdkTest, VIDEO_RAWVIDEODEC_FUNCTION_0052, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = INP_DIR_1;
    vDecSample->sfOutput = false;
    vDecSample->autoSwitchSurface = false;
    vDecSample->CreateSurface();
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameRawVideo.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->Flush());
    ASSERT_EQ(AV_ERR_OK, vDecSample->Start());
    ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, vDecSample->SwitchSurface());
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_FUNCTION_0053
 * @tc.name      : Buffer model change in normal state
 * @tc.desc      : function test
 */
HWTEST_F(RawVideodecFuncNdkTest, VIDEO_RAWVIDEODEC_FUNCTION_0053, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = INP_DIR_1;
    vDecSample->sfOutput = false;
    vDecSample->autoSwitchSurface = false;
    vDecSample->CreateSurface();
    vDecSample->sfOutput = false;
    vDecSample->autoSwitchSurface = false;
    vDecSample->CreateSurface();
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameRawVideo.c_str()));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->Reset());
    ASSERT_EQ(AV_ERR_INVALID_STATE, vDecSample->SwitchSurface());
    ASSERT_EQ(AV_ERR_OK, vDecSample->Release());
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_FUNCTION_0054
 * @tc.name      : Surface model change in config state
 * @tc.desc      : function test
 */
HWTEST_F(RawVideodecFuncNdkTest, VIDEO_RAWVIDEODEC_FUNCTION_0054, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = INP_DIR_1;
    vDecSample->sfOutput = false;
    vDecSample->autoSwitchSurface = false;
    vDecSample->CreateSurface();
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameRawVideo.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_FUNCTION_0055
 * @tc.name      : Surface model change in create state
 * @tc.desc      : function test
 */
HWTEST_F(RawVideodecFuncNdkTest, VIDEO_RAWVIDEODEC_FUNCTION_0055, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = INP_DIR_1;
    vDecSample->sfOutput = false;
    vDecSample->autoSwitchSurface = false;
    vDecSample->CreateSurface();
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameRawVideo.c_str()));;
    ASSERT_EQ(AV_ERR_INVALID_STATE, vDecSample->SwitchSurface());
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_FUNCTION_0056
 * @tc.name      : Two object repeat call setSurface quickly
 * @tc.desc      : function test
 */
HWTEST_F(RawVideodecFuncNdkTest, VIDEO_RAWVIDEODEC_FUNCTION_0056, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = INP_DIR_1;
    vDecSample->sfOutput = true;
    vDecSample->autoSwitchSurface = true;
    vDecSample->sleepOnFPS = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameRawVideo.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
    vDecSample->WaitForEOS();
    vDecSample->ReleaseInFile();

    auto vDecSample_1 = make_shared<VDecAPI11Sample>();
    vDecSample_1->inputDir = INP_DIR_1;
    vDecSample_1->sfOutput = true;
    vDecSample_1->autoSwitchSurface = true;
    vDecSample_1->sleepOnFPS = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample_1->RunVideoDec_Surface(g_codecNameRawVideo.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample_1->SwitchSurface());
    vDecSample_1->WaitForEOS();
    vDecSample_1->ReleaseInFile();
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_FUNCTION_0057
 * @tc.name      : Repeat call setSurface quickly 2 times
 * @tc.desc      : function test
 */
HWTEST_F(RawVideodecFuncNdkTest, VIDEO_RAWVIDEODEC_FUNCTION_0057, TestSize.Level0)
{
    for (int i = 0; i < 2; i++) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->inputDir = INP_DIR_1;
        vDecSample->sfOutput = true;
        vDecSample->autoSwitchSurface = true;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameRawVideo.c_str()));
        ASSERT_EQ(AV_ERR_OK, vDecSample->RepeatCallSetSurface());
        vDecSample->WaitForEOS();
        vDecSample->ReleaseInFile();
    }
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_FUNCTION_0058
 * @tc.name      : Decode RawVideo buffer in avi format
 * @tc.desc      : function test
 */
HWTEST_F(RawVideodecFuncNdkTest, VIDEO_RAWVIDEODEC_FUNCTION_0058, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    const char *file = INP_DIR_1;
    vDecSample->getFormat(file);
    vDecSample->outputYuvFlag = true;
    vDecSample->pixFmt = AV_PIX_FMT_NV12;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameRawVideo.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE10, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_FUNCTION_0059
 * @tc.name      : Decode RawVideo buffer in MKV format
 * @tc.desc      : function test
 */
HWTEST_F(RawVideodecFuncNdkTest, VIDEO_RAWVIDEODEC_FUNCTION_0059, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->getFormat(INP_DIR_6);
    vDecSample->outputYuvFlag = true;
    vDecSample->pixFmt = AV_PIX_FMT_YUV420P;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameRawVideo.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE10, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_FUNCTION_0060
 * @tc.name      : Decode RawVideo buffer and encode to H.264
 * @tc.desc      : function test
 */
HWTEST_F(RawVideodecFuncNdkTest, VIDEO_RAWVIDEODEC_FUNCTION_0060, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameRawVideo.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE10, vDecSample->outFrameCount);

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
 * @tc.number    : VIDEO_RAWVIDEODEC_FUNCTION_0061
 * @tc.name      : Decode RawVideo buffer with 64-aligned resolution
 * @tc.desc      : function test
 */
HWTEST_F(RawVideodecFuncNdkTest, VIDEO_RAWVIDEODEC_FUNCTION_0061, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = INP_DIR_5;
    vDecSample->outputYuvFlag = true;
    vDecSample->defaultWidth = 720;
    vDecSample->defaultHeight = 480;
    vDecSample->pixFmt = AV_PIX_FMT_YUV420P;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameRawVideo.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE10, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_FUNCTION_0062
 * @tc.name      : Decode RawVideo buffer with 16-aligned resolution
 * @tc.desc      : function test
 */
HWTEST_F(RawVideodecFuncNdkTest, VIDEO_RAWVIDEODEC_FUNCTION_0062, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = INP_DIR_7;
    vDecSample->outputYuvFlag = true;
    vDecSample->defaultWidth = 720;
    vDecSample->defaultHeight = 480;
    vDecSample->pixFmt = AV_PIX_FMT_YUV420P;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameRawVideo.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE7, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_FUNCTION_0063
 * @tc.name      : Decode RawVideo buffer with 30fps
 * @tc.desc      : function test
 */
HWTEST_F(RawVideodecFuncNdkTest, VIDEO_RAWVIDEODEC_FUNCTION_0063, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = INP_DIR_7;
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameRawVideo.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE7, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_FUNCTION_0064
 * @tc.name      : Decode RawVideo buffer with 60fps
 * @tc.desc      : function test
 */
HWTEST_F(RawVideodecFuncNdkTest, VIDEO_RAWVIDEODEC_FUNCTION_0064, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = INP_DIR_5;
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameRawVideo.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE10, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_FUNCTION_0065
 * @tc.name      : Decode RawVideo buffer with resolution 720x480
 * @tc.desc      : function test
 */
HWTEST_F(RawVideodecFuncNdkTest, VIDEO_RAWVIDEODEC_FUNCTION_0065, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = INP_DIR_5;
    vDecSample->outputYuvFlag = true;
    vDecSample->defaultWidth = 720;
    vDecSample->defaultHeight = 480;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameRawVideo.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE10, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_FUNCTION_0066
 * @tc.name      : RawVideo synchronous software decoding with NV12 output
 * @tc.desc      : function test
 */
HWTEST_F(RawVideodecFuncNdkTest, VIDEO_RAWVIDEODEC_FUNCTION_0066, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = INP_DIR_1;
    vDecSample->enbleSyncMode = 1;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameRawVideo.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartSyncVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE10, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_FUNCTION_0067
 * @tc.name      : RawVideo synchronous software decoding with NV21 output
 * @tc.desc      : function test
 */
HWTEST_F(RawVideodecFuncNdkTest, VIDEO_RAWVIDEODEC_FUNCTION_0067, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = INP_DIR_1;
    vDecSample->enbleSyncMode = 1;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV21;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameRawVideo.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartSyncVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE10, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_FUNCTION_0068
 * @tc.name      : RawVideo synchronous software decoding with surface output
 * @tc.desc      : function test
 */
HWTEST_F(RawVideodecFuncNdkTest, VIDEO_RAWVIDEODEC_FUNCTION_0068, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = INP_DIR_1;
    vDecSample->sfOutput = true;
    vDecSample->enbleSyncMode = 1;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameRawVideo.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->DecodeSetSurface());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartSyncVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(FRAMESIZE10, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_FUNCTION_0069
 * @tc.name      : Decode RawVideo buffer with resolution 4x4
 * @tc.desc      : function test
 */
HWTEST_F(RawVideodecFuncNdkTest, VIDEO_RAWVIDEODEC_FUNCTION_0069, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = INP_DIR_8;
    vDecSample->outputYuvFlag = true;
    vDecSample->defaultWidth = 4;
    vDecSample->defaultHeight = 4;
    vDecSample->pixFmt = AV_PIX_FMT_YUV420P;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameRawVideo.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE7, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_RAWVIDEODEC_FUNCTION_0070
 * @tc.name      : Decode RawVideo buffer with UNKNOWN pixel format
 * @tc.desc      : function test
 */
HWTEST_F(RawVideodecFuncNdkTest, VIDEO_RAWVIDEODEC_FUNCTION_0070, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->inputDir = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    vDecSample->needCheckHash = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameRawVideo.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoderNoPixelFormat());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE10, vDecSample->outFrameCount);
}
} // namespace