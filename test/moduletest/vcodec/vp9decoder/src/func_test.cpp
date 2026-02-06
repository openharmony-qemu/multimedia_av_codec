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
class Vp9decFuncNdkTest : public testing::Test {
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
    const char *INP_DIR_1 = "/data/test/media/vp9_720x480.ivf";
    const char *INP_DIR_2 = "/data/test/media/vp9_4x4.ivf";
    const char *INP_DIR_3 = "/data/test/media/vp9_641x481.ivf";
    const char *INP_DIR_4 = "/data/test/media/vp9_1080x1920.ivf";
    const char *INP_DIR_5 = "/data/test/media/vp9_1280x720.ivf";
    const char *INP_DIR_6 = "/data/test/media/vp9_1920x1080.ivf";
    const char *INP_DIR_7 = "/data/test/media/vp9_2560x1440.ivf";
    const char *INP_DIR_8 = "/data/test/media/vp9_3840x2160.ivf";
    const char *INP_DIR_9 = "/data/test/media/vp9_7680x4320.ivf";
    const char *INP_DIR_10 = "/data/test/media/vp9_all_iframes.ivf";
    const char *INP_DIR_11 = "/data/test/media/vp9_avi.avi";
    const char *INP_DIR_12 = "/data/test/media/vp9_fmp4.mp4";
    const char *INP_DIR_13 = "/data/test/media/vp9_mkv.mkv";
    const char *INP_DIR_14 = "/data/test/media/test_vp9.mp4";
    const char *INP_DIR_15 = "/data/test/media/vp9_720x480@130.ivf";
    const char *inpDirL1P0 = "/data/test/media/vp9_1_0_p0_256x144@15fps.ivf";
    const char *inpDirL1P1 = "/data/test/media/vp9_1_0_p1_256x144@15fps.ivf";
    const char *inpDirL1P2 = "/data/test/media/vp9_1_0_p2_256x144@15fps.ivf";
    const char *inpDirL1P3 = "/data/test/media/vp9_1_0_p3_256x144@15fps.ivf";
    const char *inpDirL11P0 = "/data/test/media/vp9_1_1_p0_384x192@30fps.ivf";
    const char *inpDirL11P1 = "/data/test/media/vp9_1_1_p1_384x192@30fps.ivf";
    const char *inpDirL11P2 = "/data/test/media/vp9_1_1_p2_384x192@30fps.ivf";
    const char *inpDirL11P3 = "/data/test/media/vp9_1_1_p3_384x192@30fps.ivf";
    const char *inpDirL2P0 = "/data/test/media/vp9_2_0_p0_480x256@30fps.ivf";
    const char *inpDirL2P1 = "/data/test/media/vp9_2_0_p1_480x256@30fps.ivf";
    const char *inpDirL2P2 = "/data/test/media/vp9_2_0_p2_480x256@30fps.ivf";
    const char *inpDirL2P3 = "/data/test/media/vp9_2_0_p3_480x256@30fps.ivf";
    const char *inpDirL21P0 = "/data/test/media/vp9_2_1_p0_640x384@30fps.ivf";
    const char *inpDirL21P1 = "/data/test/media/vp9_2_1_p1_640x384@30fps.ivf";
    const char *inpDirL21P2 = "/data/test/media/vp9_2_1_p2_640x384@30fps.ivf";
    const char *inpDirL21P3 = "/data/test/media/vp9_2_1_p3_640x384@30fps.ivf";
    const char *inpDirL3P0 = "/data/test/media/vp9_3_0_p0_1080x512@30fps.ivf";
    const char *inpDirL3P1 = "/data/test/media/vp9_3_0_p1_1080x512@30fps.ivf";
    const char *inpDirL3P2 = "/data/test/media/vp9_3_0_p2_1080x512@30fps.ivf";
    const char *inpDirL3P3 = "/data/test/media/vp9_3_0_p3_1080x512@30fps.ivf";
    const char *inpDirL31P0 = "/data/test/media/vp9_3_1_p0_1280x768@30fps.ivf";
    const char *inpDirL31P1 = "/data/test/media/vp9_3_1_p1_1280x768@30fps.ivf";
    const char *inpDirL31P2 = "/data/test/media/vp9_3_1_p2_1280x768@30fps.ivf";
    const char *inpDirL31P3 = "/data/test/media/vp9_3_1_p3_1280x768@30fps.ivf";
    const char *inpDirL4P0 = "/data/test/media/vp9_4_0_p0_2048x1088@30fps.ivf";
    const char *inpDirL4P1 = "/data/test/media/vp9_4_0_p1_2048x1088@30fps.ivf";
    const char *inpDirL4P2 = "/data/test/media/vp9_4_0_p2_2048x1088@30fps.ivf";
    const char *inpDirL4P3 = "/data/test/media/vp9_4_0_p3_2048x1088@30fps.ivf";
    const char *inpDirL41P0 = "/data/test/media/vp9_4_1_p0_2048x1088@60fps.ivf";
    const char *inpDirL41P1 = "/data/test/media/vp9_4_1_p1_2048x1088@60fps.ivf";
    const char *inpDirL41P2 = "/data/test/media/vp9_4_1_p2_2048x1088@60fps.ivf";
    const char *inpDirL41P3 = "/data/test/media/vp9_4_1_p3_2048x1088@60fps.ivf";
    const char *inpDirL5P0 = "/data/test/media/vp9_5_0_p0_4096x2176@30fps.ivf";
    const char *inpDirL5P1 = "/data/test/media/vp9_5_0_p1_4096x2176@30fps.ivf";
    const char *inpDirL5P2 = "/data/test/media/vp9_5_0_p2_4096x2176@30fps.ivf";
    const char *inpDirL5P3 = "/data/test/media/vp9_5_0_p3_4096x2176@30fps.ivf";
    const char *inpDirL51P0 = "/data/test/media/vp9_5_1_p0_4096x2176@60fps.ivf";
    const char *inpDirL51P1 = "/data/test/media/vp9_5_1_p1_4096x2176@60fps.ivf";
    const char *inpDirL51P2 = "/data/test/media/vp9_5_1_p2_4096x2176@60fps.ivf";
    const char *inpDirL51P3 = "/data/test/media/vp9_5_1_p3_4096x2176@60fps.ivf";
    const char *inpDirL52P0 = "/data/test/media/vp9_5_2_p0_4096x2176@120fps.ivf";
    const char *inpDirL52P1 = "/data/test/media/vp9_5_2_p1_4096x2176@120fps.ivf";
    const char *inpDirL52P2 = "/data/test/media/vp9_5_2_p2_4096x2176@120fps.ivf";
    const char *inpDirL52P3 = "/data/test/media/vp9_5_2_p3_4096x2176@120fps.ivf";
    const char *inpDirL6P0 = "/data/test/media/vp9_6_0_p0_8192x4352@30fps.ivf";
    const char *inpDirL6P1 = "/data/test/media/vp9_6_0_p1_8192x4352@30fps.ivf";
    const char *inpDirL6P2 = "/data/test/media/vp9_6_0_p2_8192x4352@30fps.ivf";
    const char *inpDirL6P3 = "/data/test/media/vp9_6_0_p3_8192x4352@30fps.ivf";
    const char *inpDirL61P0 = "/data/test/media/vp9_6_1_p0_8192x4352@60fps.ivf";
    const char *inpDirL61P1 = "/data/test/media/vp9_6_1_p1_8192x4352@60fps.ivf";
    const char *inpDirL61P2 = "/data/test/media/vp9_6_1_p2_8192x4352@60fps.ivf";
    const char *inpDirL61P3 = "/data/test/media/vp9_6_1_p3_8192x4352@60fps.ivf";
    const char *inpDirL62P0 = "/data/test/media/vp9_6_2_p0_8192x4352@120fps.ivf";
    const char *inpDirL62P1 = "/data/test/media/vp9_6_2_p1_8192x4352@120fps.ivf";
    const char *inpDirL62P2 = "/data/test/media/vp9_6_2_p2_8192x4352@120fps.ivf";
    const char *inpDirL62P3 = "/data/test/media/vp9_6_2_p3_8192x4352@120fps.ivf";
};

static OH_AVCapability *cap_vp9 = nullptr;
static string g_codecNameVp9 = "";
constexpr uint32_t FRAMESIZE100 = 100;
constexpr uint32_t FRAMESIZE52 = 52;
constexpr uint32_t FRAMESIZE602 = 602;
void Vp9decFuncNdkTest::SetUpTestCase()
{
    cap_vp9 = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_VP9, false, SOFTWARE);
    g_codecNameVp9 = OH_AVCapability_GetName(cap_vp9);
    cout << "g_codecNameVp9: " << g_codecNameVp9 << endl;
    cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_AVC, true, HARDWARE);
    const char *tmpCodecName = OH_AVCapability_GetName(cap);
    if (memcpy_s(g_codecName, sizeof(g_codecName), tmpCodecName, strlen(tmpCodecName)) != 0)
        cout << "memcpy failed" << endl;
    cout << "codecname: " << g_codecName << endl;
}

void Vp9decFuncNdkTest::TearDownTestCase() {}
void Vp9decFuncNdkTest::SetUp() {}
void Vp9decFuncNdkTest::TearDown() {}
} // namespace Media
} // namespace OHOS

static void getVideoSizeRange(OH_AVRange *widthRange, OH_AVRange *heightRange)
{
    memset_s(widthRange, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    memset_s(heightRange, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_VP9, false, SOFTWARE);
    OH_AVCapability_GetVideoHeightRange(capability, heightRange);
    cout << "minval=" << heightRange->minVal << "  maxval=" << heightRange->maxVal << endl;
    OH_AVCapability_GetVideoWidthRange(capability, widthRange);
    cout << "minval=" << widthRange->minVal << "  maxval=" << widthRange->maxVal << endl;
}


namespace {
/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0001
 * @tc.name      : Decode VP9 buffer with default settings
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0001, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0002
 * @tc.name      : Decode VP9 buffer with 90-degree rotation
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0002, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    int32_t angle = 90;
    vDecSample->trackFormat = OH_AVFormat_Create();
    (void)OH_AVFormat_SetIntValue(vDecSample->trackFormat, OH_MD_KEY_ROTATION, angle);
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0003
 * @tc.name      : Decode VP9 buffer with 180-degree rotation
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0003, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    int32_t angle = 180;
    vDecSample->trackFormat = OH_AVFormat_Create();
    (void)OH_AVFormat_SetIntValue(vDecSample->trackFormat, OH_MD_KEY_ROTATION, angle);
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0004
 * @tc.name      : Decode VP9 buffer with 270-degree rotation
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0004, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    int32_t angle = 270;
    vDecSample->trackFormat = OH_AVFormat_Create();
    (void)OH_AVFormat_SetIntValue(vDecSample->trackFormat, OH_MD_KEY_ROTATION, angle);
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0005
 * @tc.name      : Decode H.263 buffer with VP9 decoder
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0005, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->INP_DIR = "/data/test/media/profile0_level10_I_128x96.h263";
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderFor263());
    vDecSample->WaitForEOS();
    ASSERT_EQ(false, vDecSample->isonError);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0006
 * @tc.name      : Decode VP9 buffer with invalid resolution below minimum
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0006, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    OH_AVRange widthRange;
    OH_AVRange heightRange;
    getVideoSizeRange(&widthRange, &heightRange);
    vDecSample->DEFAULT_WIDTH = widthRange.minVal - 1;
    vDecSample->DEFAULT_HEIGHT = heightRange.minVal - 1;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_INVALID_VAL, vDecSample->ConfigureVideoDecoder());
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0007
 * @tc.name      : Decode VP9 buffer with invalid resolution above maximum
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0007, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    OH_AVRange widthRange;
    OH_AVRange heightRange;
    getVideoSizeRange(&widthRange, &heightRange);
    vDecSample->DEFAULT_WIDTH = widthRange.maxVal + 1;
    vDecSample->DEFAULT_HEIGHT = heightRange.maxVal + 1;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_INVALID_VAL, vDecSample->ConfigureVideoDecoder());
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0008
 * @tc.name      : Decode VP9 buffer with extremely large resolution
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0008, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    int32_t max = 2147483647;
    vDecSample->DEFAULT_WIDTH = max;
    vDecSample->DEFAULT_HEIGHT = max;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_INVALID_VAL, vDecSample->ConfigureVideoDecoder());
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0009
 * @tc.name      : Decode VP9 buffer with minimum valid resolution
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0009, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    OH_AVRange widthRange;
    OH_AVRange heightRange;
    getVideoSizeRange(&widthRange, &heightRange);
    vDecSample->DEFAULT_WIDTH = widthRange.minVal;
    vDecSample->DEFAULT_HEIGHT = heightRange.minVal;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0010
 * @tc.name      : Decode VP9 buffer with maximum valid resolution
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0010, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 8192;
    vDecSample->DEFAULT_HEIGHT = 4352;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0011
 * @tc.name      : Decode VP9 buffer with invalid video size
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0011, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = -1;
    vDecSample->DEFAULT_HEIGHT = -1;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_INVALID_VAL, vDecSample->ConfigureVideoDecoder());
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0012
 * @tc.name      : Decode VP9 buffer with unsupported pixel format
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0012, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = -1;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_UNSUPPORT, vDecSample->ConfigureVideoDecoder());
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0013
 * @tc.name      : Decode VP9 buffer with pixel format 0
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0013, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = 0;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_UNSUPPORT, vDecSample->ConfigureVideoDecoder());
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0014
 * @tc.name      : Decode VP9 buffer with pixel format 1
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0014, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = 1;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_UNSUPPORT, vDecSample->ConfigureVideoDecoder());
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0015
 * @tc.name      : Decode VP9 buffer with pixel format 3
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0015, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = 3;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0016
 * @tc.name      : Decode VP9 buffer with unsupported pixel format 5
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0016, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = 5;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_UNSUPPORT, vDecSample->ConfigureVideoDecoder());
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0017
 * @tc.name      : Decode VP9 buffer with maximum int value
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0017, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    int32_t max = 2147483647;
    vDecSample->defualtPixelFormat = max;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_UNSUPPORT, vDecSample->ConfigureVideoDecoder());
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0018
 * @tc.name      : Decode VP9 buffer with NV12 pixel format
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0018, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0019
 * @tc.name      : Decode VP9 buffer with NV21 pixel format
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0019, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV21;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0020
 * @tc.name      : Decode VP9 buffer with YUVI420 pixel format
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0020, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_YUVI420;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_UNSUPPORT, vDecSample->ConfigureVideoDecoder());
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0021
 * @tc.name      : Decode VP9 surface with NV12 pixel format
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0021, TestSize.Level3)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->SF_OUTPUT = true;
    vDecSample->outputYuvSurface = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface("OH.Media.Codec.Decoder.Video.VP9"));
    vDecSample->WaitForEOS();
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0022
 * @tc.name      : Decode VP9 surface with NV21 pixel format
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0022, TestSize.Level3)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->SF_OUTPUT = true;
    vDecSample->outputYuvSurface = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV21;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface("OH.Media.Codec.Decoder.Video.VP9"));
    vDecSample->WaitForEOS();
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0023
 * @tc.name      : Surface model change in normal state
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0023, TestSize.Level2)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->SF_OUTPUT = true;
    vDecSample->autoSwitchSurface = true;
    vDecSample->AFTER_EOS_DESTORY_CODEC = false;
    vDecSample->sleepOnFPS = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface("OH.Media.Codec.Decoder.Video.VP9"));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->Reset());
    ASSERT_EQ(AV_ERR_INVALID_STATE, vDecSample->SwitchSurface());
    ASSERT_EQ(AV_ERR_OK, vDecSample->Release());
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0024
 * @tc.name      : Surface model change in flushed state
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0024, TestSize.Level2)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->SF_OUTPUT = true;
    vDecSample->autoSwitchSurface = true;
    vDecSample->sleepOnFPS = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->Flush());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
    ASSERT_EQ(AV_ERR_OK, vDecSample->Stop());
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0025
 * @tc.name      : Surface model change in running state
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0025, TestSize.Level2)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->SF_OUTPUT = false;
    vDecSample->autoSwitchSurface = false;
    vDecSample->CreateSurface();
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, vDecSample->SwitchSurface());
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0026
 * @tc.name      : Repeat call setSurface quickly
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0026, TestSize.Level2)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->SF_OUTPUT = true;
    vDecSample->autoSwitchSurface = true;
    vDecSample->sleepOnFPS = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
    vDecSample->WaitForEOS();
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0027
 * @tc.name      : Surface model change from flush to running state
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0027, TestSize.Level2)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->SF_OUTPUT = true;
    vDecSample->autoSwitchSurface = true;
    vDecSample->AFTER_EOS_DESTORY_CODEC = false;
    vDecSample->sleepOnFPS = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->Flush());
    ASSERT_EQ(AV_ERR_OK, vDecSample->Start());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0028
 * @tc.name      : Surface model change after decoder finishes to End-of-Stream state
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0028, TestSize.Level2)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->SF_OUTPUT = true;
    vDecSample->autoSwitchSurface = true;
    vDecSample->AFTER_EOS_DESTORY_CODEC = false;
    vDecSample->sleepOnFPS = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface("OH.Media.Codec.Decoder.Video.VP9"));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0029
 * @tc.name      : Buffer model change after decoder finishes to End-of-Stream state
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0029, TestSize.Level2)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->SF_OUTPUT = false;
    vDecSample->autoSwitchSurface = false;
    vDecSample->CreateSurface();
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec("OH.Media.Codec.Decoder.Video.VP9"));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, vDecSample->SwitchSurface());
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0030
 * @tc.name      : Buffer model change from running to flushed state
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0030, TestSize.Level2)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->SF_OUTPUT = false;
    vDecSample->autoSwitchSurface = false;
    vDecSample->CreateSurface();
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->Flush());
    ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, vDecSample->SwitchSurface());
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0031
 * @tc.name      : Buffer model change from flushed to running state
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0031, TestSize.Level2)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->SF_OUTPUT = false;
    vDecSample->autoSwitchSurface = false;
    vDecSample->CreateSurface();
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->Flush());
    ASSERT_EQ(AV_ERR_OK, vDecSample->Start());
    ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, vDecSample->SwitchSurface());
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0032
 * @tc.name      : Buffer model change in normal state
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0032, TestSize.Level2)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->SF_OUTPUT = false;
    vDecSample->autoSwitchSurface = false;
    vDecSample->CreateSurface();
    vDecSample->SF_OUTPUT = false;
    vDecSample->autoSwitchSurface = false;
    vDecSample->CreateSurface();
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec("OH.Media.Codec.Decoder.Video.VP9"));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->Reset());
    ASSERT_EQ(AV_ERR_INVALID_STATE, vDecSample->SwitchSurface());
    ASSERT_EQ(AV_ERR_OK, vDecSample->Release());
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0033
 * @tc.name      : Surface model change in config state
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0033, TestSize.Level1)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->SF_OUTPUT = false;
    vDecSample->autoSwitchSurface = false;
    vDecSample->CreateSurface();
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0034
 * @tc.name      : Surface model change in config state
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0034, TestSize.Level1)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->SF_OUTPUT = false;
    vDecSample->autoSwitchSurface = false;
    vDecSample->CreateSurface();
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));;
    ASSERT_EQ(AV_ERR_INVALID_STATE, vDecSample->SwitchSurface());
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0035
 * @tc.name      : Two objects repeat call setSurface quickly
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0035, TestSize.Level2)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->SF_OUTPUT = true;
    vDecSample->autoSwitchSurface = true;
    vDecSample->sleepOnFPS = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
    vDecSample->WaitForEOS();
    vDecSample->ReleaseInFile();

    auto vDecSample_1 = make_shared<VDecAPI11Sample>();
    vDecSample_1->INP_DIR = INP_DIR_1;
    vDecSample_1->SF_OUTPUT = true;
    vDecSample_1->autoSwitchSurface = true;
    vDecSample_1->sleepOnFPS = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample_1->RunVideoDec_Surface("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample_1->SwitchSurface());
    vDecSample_1->WaitForEOS();
    vDecSample_1->ReleaseInFile();
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0036
 * @tc.name      : Repeat call setSurface quickly 2 times
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0036, TestSize.Level2)
{
    for (int i = 0; i < 2; i++) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_1;
        vDecSample->SF_OUTPUT = true;
        vDecSample->autoSwitchSurface = true;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface("OH.Media.Codec.Decoder.Video.VP9"));
        ASSERT_EQ(AV_ERR_OK, vDecSample->RepeatCallSetSurface());
        vDecSample->WaitForEOS();
        vDecSample->ReleaseInFile();
    }
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0037
 * @tc.name      : Decode VP9 buffer with demuxer avi format
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0037, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->getFormat(INP_DIR_11);
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE602, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0038
 * @tc.name      : Decode VP9 buffer with demuxer MKV format
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0038, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->getFormat(INP_DIR_13);
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE602, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0039
 * @tc.name      : Decode VP9 buffer and encode to H.264
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0039, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);

    auto vEncSample = make_unique<VEncNdkSample>();
    vEncSample->INP_DIR = vDecSample->OUT_DIR;
    vEncSample->DEFAULT_WIDTH = 720;
    vEncSample->DEFAULT_HEIGHT = 480;
    vEncSample->DEFAULT_FRAME_RATE = 30;
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0040
 * @tc.name      : Decode VP9 buffer with minimum resolution (4x4)
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0040, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_2;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 4;
    vDecSample->DEFAULT_HEIGHT = 4;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0041
 * @tc.name      : Decode VP9 buffer with odd resolution (641x481)
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0041, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_3;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 641;
    vDecSample->DEFAULT_HEIGHT = 481;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0042
 * @tc.name      : Decode VP9 buffer with resolution (2560x1440)
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0042, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_7;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 2560;
    vDecSample->DEFAULT_HEIGHT = 1440;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0043
 * @tc.name      : Decode VP9 buffer with resolution (3840x2160)
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0043, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_8;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 3840;
    vDecSample->DEFAULT_HEIGHT = 2160;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0044
 * @tc.name      : Decode VP9 buffer with resolution (7680x4320)
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0044, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_9;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 7680;
    vDecSample->DEFAULT_HEIGHT = 4320;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0045
 * @tc.name      : Decode VP9 buffer with 1280x720 resolution
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0045, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_5;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 1280;
    vDecSample->DEFAULT_HEIGHT = 720;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0046
 * @tc.name      : Decode VP9 buffer with 1920x1080 resolution
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0046, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_6;
    vDecSample->outputYuvFlag = true;
    vDecSample->needCheckHash = true;
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0047
 * @tc.name      : Decode VP9 buffer containing only I frames
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0047, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_10;
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0048
 * @tc.name      : Decode VP9 buffer with demuxer mp4 format
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0048, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->getFormat(INP_DIR_14);
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE602, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0049
 * @tc.name      : Decode VP9 buffer with demuxer fmp4 format
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0049, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->getFormat(INP_DIR_12);
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE602, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0050
 * @tc.name      : Decode VP9 buffer with 1080x1920 resolution
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0050, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_4;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 1080;
    vDecSample->DEFAULT_HEIGHT = 1920;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0051
 * @tc.name      : Decode VP9 buffer with level1, profile0
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0051, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = inpDirL1P0;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 256;
    vDecSample->DEFAULT_HEIGHT = 144;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE52, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0052
 * @tc.name      : Decode VP9 buffer with level1, profile1
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0052, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = inpDirL1P1;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 256;
    vDecSample->DEFAULT_HEIGHT = 144;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE52, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0053
 * @tc.name      : Decode VP9 buffer with level1, profile2
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0053, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = inpDirL1P2;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 256;
    vDecSample->DEFAULT_HEIGHT = 144;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE52, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0054
 * @tc.name      : Decode VP9 buffer with level1, profile3
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0054, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = inpDirL1P3;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 256;
    vDecSample->DEFAULT_HEIGHT = 144;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE52, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0055
 * @tc.name      : Decode VP9 buffer with level1.1, profile0
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0055, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = inpDirL11P0;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 384;
    vDecSample->DEFAULT_HEIGHT = 192;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0056
 * @tc.name      : Decode VP9 buffer with level1.1, profile1
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0056, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = inpDirL11P1;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 384;
    vDecSample->DEFAULT_HEIGHT = 192;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0057
 * @tc.name      : Decode VP9 buffer with level1.1, profile2
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0057, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = inpDirL11P2;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 384;
    vDecSample->DEFAULT_HEIGHT = 192;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0058
 * @tc.name      : Decode VP9 buffer with level1.1, profile3
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0058, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = inpDirL11P3;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 384;
    vDecSample->DEFAULT_HEIGHT = 192;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0059
 * @tc.name      : Decode VP9 buffer with level2.0, profile0
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0059, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = inpDirL2P0;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 480;
    vDecSample->DEFAULT_HEIGHT = 256;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0060
 * @tc.name      : Decode VP9 buffer with level2.0, profile1
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0060, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = inpDirL2P1;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 480;
    vDecSample->DEFAULT_HEIGHT = 256;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0061
 * @tc.name      : Decode VP9 buffer with level2.0, profile2
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0061, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = inpDirL2P2;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 480;
    vDecSample->DEFAULT_HEIGHT = 256;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0062
 * @tc.name      : Decode VP9 buffer with level2.0, profile3
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0062, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = inpDirL2P3;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 480;
    vDecSample->DEFAULT_HEIGHT = 256;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0063
 * @tc.name      : Decode VP9 buffer with level2.1, profile0
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0063, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = inpDirL21P0;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 640;
    vDecSample->DEFAULT_HEIGHT = 384;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0064
 * @tc.name      : Decode VP9 buffer with level2.1, profile1
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0064, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = inpDirL21P1;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 640;
    vDecSample->DEFAULT_HEIGHT = 384;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0065
 * @tc.name      : Decode VP9 buffer with level2.1, profile2
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0065, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = inpDirL21P2;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 640;
    vDecSample->DEFAULT_HEIGHT = 384;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0066
 * @tc.name      : Decode VP9 buffer with level2.1, profile3
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0066, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = inpDirL21P3;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 640;
    vDecSample->DEFAULT_HEIGHT = 384;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0067
 * @tc.name      : Decode VP9 buffer with level3.0, profile0
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0067, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = inpDirL3P0;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 1080;
    vDecSample->DEFAULT_HEIGHT = 512;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0068
 * @tc.name      : Decode VP9 buffer with level3.0, profile1
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0068, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = inpDirL3P1;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 1080;
    vDecSample->DEFAULT_HEIGHT = 512;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0069
 * @tc.name      : Decode VP9 buffer with level3.0, profile2
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0069, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = inpDirL3P2;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 1080;
    vDecSample->DEFAULT_HEIGHT = 512;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0070
 * @tc.name      : Decode VP9 buffer with level3.0, profile3
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0070, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = inpDirL3P3;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 1080;
    vDecSample->DEFAULT_HEIGHT = 512;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0071
 * @tc.name      : Decode VP9 buffer with level3.1, profile0
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0071, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = inpDirL31P0;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 1280;
    vDecSample->DEFAULT_HEIGHT = 768;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0072
 * @tc.name      : Decode VP9 buffer with level3.1, profile1
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0072, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = inpDirL31P1;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 1280;
    vDecSample->DEFAULT_HEIGHT = 768;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0073
 * @tc.name      : Decode VP9 buffer with level3.1, profile2
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0073, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = inpDirL31P2;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 1280;
    vDecSample->DEFAULT_HEIGHT = 768;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0074
 * @tc.name      : Decode VP9 buffer with level3.1, profile3
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0074, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = inpDirL31P3;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 1280;
    vDecSample->DEFAULT_HEIGHT = 768;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0075
 * @tc.name      : Decode VP9 buffer with level4.0, profile0
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0075, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = inpDirL4P0;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 2048;
    vDecSample->DEFAULT_HEIGHT = 1088;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0076
 * @tc.name      : Decode VP9 buffer with level4.0, profile1
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0076, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = inpDirL4P1;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 2048;
    vDecSample->DEFAULT_HEIGHT = 1088;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0077
 * @tc.name      : Decode VP9 buffer with level4.0, profile2
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0077, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = inpDirL4P2;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 2048;
    vDecSample->DEFAULT_HEIGHT = 1088;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0078
 * @tc.name      : Decode VP9 buffer with level4.0, profile3
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0078, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = inpDirL4P3;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 2048;
    vDecSample->DEFAULT_HEIGHT = 1088;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0079
 * @tc.name      : Decode VP9 buffer with level4.1, profile0
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0079, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = inpDirL41P0;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 2048;
    vDecSample->DEFAULT_HEIGHT = 1088;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0080
 * @tc.name      : Decode VP9 buffer with level4.1, profile1
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0080, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = inpDirL41P1;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 2048;
    vDecSample->DEFAULT_HEIGHT = 1088;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0081
 * @tc.name      : Decode VP9 buffer with level4.1, profile2
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0081, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = inpDirL41P2;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 2048;
    vDecSample->DEFAULT_HEIGHT = 1088;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0082
 * @tc.name      : Decode VP9 buffer with level4.1, profile3
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0082, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = inpDirL41P3;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 2048;
    vDecSample->DEFAULT_HEIGHT = 1088;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0083
 * @tc.name      : Decode VP9 buffer with level5.0, profile0
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0083, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = inpDirL5P0;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 4096;
    vDecSample->DEFAULT_HEIGHT = 2176;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0084
 * @tc.name      : Decode VP9 buffer with level5.0, profile1
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0084, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = inpDirL5P1;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 4096;
    vDecSample->DEFAULT_HEIGHT = 2176;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0085
 * @tc.name      : Decode VP9 buffer with level5.0, profile2
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0085, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = inpDirL5P2;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 4096;
    vDecSample->DEFAULT_HEIGHT = 2176;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0086
 * @tc.name      : Decode VP9 buffer with level5.0, profile3
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0086, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = inpDirL5P3;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 4096;
    vDecSample->DEFAULT_HEIGHT = 2176;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0087
 * @tc.name      : Decode VP9 buffer with level5.1, profile0
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0087, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = inpDirL51P0;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 4096;
    vDecSample->DEFAULT_HEIGHT = 2176;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0088
 * @tc.name      : Decode VP9 buffer with level5.1, profile1
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0088, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = inpDirL51P1;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 4096;
    vDecSample->DEFAULT_HEIGHT = 2176;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0089
 * @tc.name      : Decode VP9 buffer with level5.1, profile2
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0089, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = inpDirL51P2;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 4096;
    vDecSample->DEFAULT_HEIGHT = 2176;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0090
 * @tc.name      : Decode VP9 buffer with level5.1, profile3
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0090, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = inpDirL51P3;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 4096;
    vDecSample->DEFAULT_HEIGHT = 2176;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0091
 * @tc.name      : Decode VP9 buffer with level5.2, profile0
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0091, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = inpDirL52P0;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 4096;
    vDecSample->DEFAULT_HEIGHT = 2176;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0092
 * @tc.name      : Decode VP9 buffer with level5.2, profile1
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0092, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = inpDirL52P1;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 4096;
    vDecSample->DEFAULT_HEIGHT = 2176;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0093
 * @tc.name      : Decode VP9 buffer with level5.2, profile2
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0093, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = inpDirL52P2;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 4096;
    vDecSample->DEFAULT_HEIGHT = 2176;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0094
 * @tc.name      : Decode VP9 buffer with level5.2, profile3
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0094, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = inpDirL52P3;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 4096;
    vDecSample->DEFAULT_HEIGHT = 2176;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0095
 * @tc.name      : Decode VP9 buffer with level6.0, profile0
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0095, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = inpDirL6P0;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 8192;
    vDecSample->DEFAULT_HEIGHT = 4352;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0096
 * @tc.name      : Decode VP9 buffer with level6.0, profile1
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0096, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = inpDirL6P1;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 8192;
    vDecSample->DEFAULT_HEIGHT = 4352;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0097
 * @tc.name      : Decode VP9 buffer with level6.0, profile2
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0097, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = inpDirL6P2;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 8192;
    vDecSample->DEFAULT_HEIGHT = 4352;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0098
 * @tc.name      : Decode VP9 buffer with level6.0, profile3
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0098, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = inpDirL6P3;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 8192;
    vDecSample->DEFAULT_HEIGHT = 4352;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0099
 * @tc.name      : Decode VP9 buffer with level6.1, profile0
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0099, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = inpDirL61P0;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 8192;
    vDecSample->DEFAULT_HEIGHT = 4352;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0100
 * @tc.name      : Decode VP9 buffer with level6.1, profile1
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0100, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = inpDirL61P1;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 8192;
    vDecSample->DEFAULT_HEIGHT = 4352;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0101
 * @tc.name      : Decode VP9 buffer with level6.1, profile2
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0101, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = inpDirL61P2;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 8192;
    vDecSample->DEFAULT_HEIGHT = 4352;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0102
 * @tc.name      : Decode VP9 buffer with level6.1, profile3
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0102, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = inpDirL61P3;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 8192;
    vDecSample->DEFAULT_HEIGHT = 4352;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0103
 * @tc.name      : Decode VP9 buffer with level6.2, profile0
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0103, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = inpDirL62P0;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 8192;
    vDecSample->DEFAULT_HEIGHT = 4352;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0104
 * @tc.name      : Decode VP9 buffer with level6.2, profile1
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0104, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = inpDirL62P1;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 8192;
    vDecSample->DEFAULT_HEIGHT = 4352;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0105
 * @tc.name      : Decode VP9 buffer with level6.2, profile2
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0105, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = inpDirL62P2;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 8192;
    vDecSample->DEFAULT_HEIGHT = 4352;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0106
 * @tc.name      : Decode VP9 buffer with level6.2, profile3
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0106, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = inpDirL62P3;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 8192;
    vDecSample->DEFAULT_HEIGHT = 4352;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0107
 * @tc.name      : Decode VP9 buffer with 720x480@130
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0107, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_15;
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0108
 * @tc.name      : Decode VP9 buffer with unknown pixelformat
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0108, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_6;
    vDecSample->outputYuvFlag = true;
    vDecSample->needCheckHash = true;
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoderNoPixelFormat());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_DECODE_SYNC_VP9_FUNC_0001
 * @tc.name      : VP9 synchronous decoding with NV12 output
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_DECODE_SYNC_VP9_FUNC_0001, TestSize.Level1)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->enbleSyncMode = 1;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartSyncVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_DECODE_SYNC_VP9_FUNC_0002
 * @tc.name      : VP9 synchronous decoding with NV21 output
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_DECODE_SYNC_VP9_FUNC_0002, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->enbleSyncMode = 1;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV21;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartSyncVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_DECODE_SYNC_VP9_FUNC_0003
 * @tc.name      : VP9 synchronous decoding with surface output
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_DECODE_SYNC_VP9_FUNC_0003, TestSize.Level1)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->SF_OUTPUT = true;
    vDecSample->enbleSyncMode = 1;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->DecodeSetSurface());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartSyncVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP9DEC_FUNCTION_0109
 * @tc.name      : Decode VP9 buffer with default settings
 * @tc.desc      : function test
 */
HWTEST_F(Vp9decFuncNdkTest, VIDEO_VP9DEC_FUNCTION_0109, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    int32_t pixfmt[4] = {24, 25, 35, 36};
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    vDecSample->isGetVideoSupportedPixelFormats = true;
    vDecSample->isGetFormatKey = true;
    vDecSample->avcodecMimeType = OH_AVCODEC_MIMETYPE_VIDEO_VP9;
    vDecSample->isEncoder = false;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP9"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(4, vDecSample->pixlFormatNum);
    for (int i = 0; i < vDecSample->pixlFormatNum; i++) {
        ASSERT_EQ(vDecSample->pixlFormats[i], pixfmt[i]);
    }
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}
} // namespace