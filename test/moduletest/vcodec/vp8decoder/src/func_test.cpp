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
class Vp8decFuncNdkTest : public testing::Test {
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
    const char *INP_DIR_1 = "/data/test/media/vp8_720x480_30fps.ivf";
    const char *INP_DIR_2 = "/data/test/media/vp8_720x480_60fps.ivf";
    const char *INP_DIR_3 = "/data/test/media/vp8_IPFrames_only.ivf";
    const char *INP_DIR_4 = "/data/test/media/vp8_IFrames_only.ivf";
    const char *INP_DIR_5 = "/data/test/media/vp8_ogg.ogg";
    const char *INP_DIR_6 = "/data/test/media/vp8_mkv.mkv";
    const char *INP_DIR_7 = "/data/test/media/vp8_webm.webm";
    const char *INP_DIR_8 = "/data/test/media/vp8_257x129.ivf";
    const char *INP_DIR_9 = "/data/test/media/vp8_16x16.ivf";
    const char *INP_DIR_10 = "/data/test/media/vp8_720p.ivf";
    const char *INP_DIR_11 = "/data/test/media/vp8_1080p.ivf";
    const char *INP_DIR_12 = "/data/test/media/vp8_600x800.ivf";
    const char *INP_DIR_13 = "/data/test/media/vp8_480x640.ivf";
    const char *INP_DIR_14 = "/data/test/media/vp8_4x4.ivf";
    const char *INP_DIR_15 = "/data/test/media/vp8_1080x1920.ivf";
};

static OH_AVCapability *cap_vp8 = nullptr;
static string g_codecNameVp8 = "";
constexpr uint32_t FRAMESIZE100 = 100;
constexpr uint32_t FRAMESIZE52 = 52;
constexpr uint32_t FRAMESIZE602 = 602;
void Vp8decFuncNdkTest::SetUpTestCase()
{
    cap_vp8 = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_VP8, false, SOFTWARE);
    g_codecNameVp8 = OH_AVCapability_GetName(cap_vp8);
    cout << "g_codecNameVp8: " << g_codecNameVp8 << endl;
    cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_AVC, true, HARDWARE);
    const char *tmpCodecName = OH_AVCapability_GetName(cap);
    if (memcpy_s(g_codecName, sizeof(g_codecName), tmpCodecName, strlen(tmpCodecName)) != 0)
        cout << "memcpy failed" << endl;
    cout << "codecname: " << g_codecName << endl;
}

void Vp8decFuncNdkTest::TearDownTestCase() {}
void Vp8decFuncNdkTest::SetUp() {}
void Vp8decFuncNdkTest::TearDown() {}
} // namespace Media
} // namespace OHOS

static void getVideoSizeRange(OH_AVRange *widthRange, OH_AVRange *heightRange)
{
    memset_s(widthRange, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    memset_s(heightRange, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_VP8, false, SOFTWARE);
    OH_AVCapability_GetVideoHeightRange(capability, heightRange);
    cout << "minval=" << heightRange->minVal << "  maxval=" << heightRange->maxVal << endl;
    OH_AVCapability_GetVideoWidthRange(capability, widthRange);
    cout << "minval=" << widthRange->minVal << "  maxval=" << widthRange->maxVal << endl;
}


namespace {
/**
 * @tc.number    : VIDEO_VP8DEC_FUNCTION_0001
 * @tc.name      : Decode VP8 buffer with default settings
 * @tc.desc      : function test
 */
HWTEST_F(Vp8decFuncNdkTest, VIDEO_VP8DEC_FUNCTION_0001, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    vDecSample->needCheckHash = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP8"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE52, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP8DEC_FUNCTION_0002
 * @tc.name      : Decode VP8 buffer with 90-degree rotation
 * @tc.desc      : function test
 */
HWTEST_F(Vp8decFuncNdkTest, VIDEO_VP8DEC_FUNCTION_0002, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    int32_t angle = 90;
    vDecSample->trackFormat = OH_AVFormat_Create();
    (void)OH_AVFormat_SetIntValue(vDecSample->trackFormat, OH_MD_KEY_ROTATION, angle);
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP8"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE52, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP8DEC_FUNCTION_0003
 * @tc.name      : Decode VP8 buffer with 180-degree rotation
 * @tc.desc      : function test
 */
HWTEST_F(Vp8decFuncNdkTest, VIDEO_VP8DEC_FUNCTION_0003, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    int32_t angle = 180;
    vDecSample->trackFormat = OH_AVFormat_Create();
    (void)OH_AVFormat_SetIntValue(vDecSample->trackFormat, OH_MD_KEY_ROTATION, angle);
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP8"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE52, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP8DEC_FUNCTION_0004
 * @tc.name      : Decode VP8 buffer with 270-degree rotation
 * @tc.desc      : function test
 */
HWTEST_F(Vp8decFuncNdkTest, VIDEO_VP8DEC_FUNCTION_0004, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    int32_t angle = 270;
    vDecSample->trackFormat = OH_AVFormat_Create();
    (void)OH_AVFormat_SetIntValue(vDecSample->trackFormat, OH_MD_KEY_ROTATION, angle);
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP8"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE52, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP8DEC_FUNCTION_0005
 * @tc.name      : Decode H.263 buffer with VP8 decoder
 * @tc.desc      : function test
 */
HWTEST_F(Vp8decFuncNdkTest, VIDEO_VP8DEC_FUNCTION_0005, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->INP_DIR = "/data/test/media/profile0_level10_I_128x96.h263";
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP8"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderFor263());
    vDecSample->WaitForEOS();
    ASSERT_EQ(false, vDecSample->isonError);
}

/**
 * @tc.number    : VIDEO_VP8DEC_FUNCTION_0006
 * @tc.name      : Decode VP8 buffer with invalid resolution below minimum
 * @tc.desc      : function test
 */
HWTEST_F(Vp8decFuncNdkTest, VIDEO_VP8DEC_FUNCTION_0006, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    OH_AVRange widthRange;
    OH_AVRange heightRange;
    getVideoSizeRange(&widthRange, &heightRange);
    vDecSample->DEFAULT_WIDTH = widthRange.minVal - 1;
    vDecSample->DEFAULT_HEIGHT = heightRange.minVal - 1;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP8"));
    ASSERT_EQ(AV_ERR_INVALID_VAL, vDecSample->ConfigureVideoDecoder());
}

/**
 * @tc.number    : VIDEO_VP8DEC_FUNCTION_0007
 * @tc.name      : Decode VP8 buffer with invalid resolution above maximum
 * @tc.desc      : function test
 */
HWTEST_F(Vp8decFuncNdkTest, VIDEO_VP8DEC_FUNCTION_0007, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    OH_AVRange widthRange;
    OH_AVRange heightRange;
    getVideoSizeRange(&widthRange, &heightRange);
    vDecSample->DEFAULT_WIDTH = widthRange.maxVal + 1;
    vDecSample->DEFAULT_HEIGHT = heightRange.maxVal + 1;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP8"));
    ASSERT_EQ(AV_ERR_INVALID_VAL, vDecSample->ConfigureVideoDecoder());
}

/**
 * @tc.number    : VIDEO_VP8DEC_FUNCTION_0008
 * @tc.name      : Decode VP8 buffer with extremely large resolution
 * @tc.desc      : function test
 */
HWTEST_F(Vp8decFuncNdkTest, VIDEO_VP8DEC_FUNCTION_0008, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    int32_t max = 2147483647;
    vDecSample->DEFAULT_WIDTH = max;
    vDecSample->DEFAULT_HEIGHT = max;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP8"));
    ASSERT_EQ(AV_ERR_INVALID_VAL, vDecSample->ConfigureVideoDecoder());
}

/**
 * @tc.number    : VIDEO_VP8DEC_FUNCTION_0009
 * @tc.name      : Decode VP8 buffer with minimum valid resolution
 * @tc.desc      : function test
 */
HWTEST_F(Vp8decFuncNdkTest, VIDEO_VP8DEC_FUNCTION_0009, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    OH_AVRange widthRange;
    OH_AVRange heightRange;
    getVideoSizeRange(&widthRange, &heightRange);
    vDecSample->DEFAULT_WIDTH = widthRange.minVal;
    vDecSample->DEFAULT_HEIGHT = heightRange.minVal;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP8"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE52, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP8DEC_FUNCTION_0010
 * @tc.name      : Decode VP8 buffer with maximum valid resolution
 * @tc.desc      : function test
 */
HWTEST_F(Vp8decFuncNdkTest, VIDEO_VP8DEC_FUNCTION_0010, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP8"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE52, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP8DEC_FUNCTION_0011
 * @tc.name      : Decode VP8 buffer with invalid video size
 * @tc.desc      : function test
 */
HWTEST_F(Vp8decFuncNdkTest, VIDEO_VP8DEC_FUNCTION_0011, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = -1;
    vDecSample->DEFAULT_HEIGHT = -1;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP8"));
    ASSERT_EQ(AV_ERR_INVALID_VAL, vDecSample->ConfigureVideoDecoder());
}

/**
 * @tc.number    : VIDEO_VP8DEC_FUNCTION_0012
 * @tc.name      : Decode VP8 buffer with unsupported pixel format
 * @tc.desc      : function test
 */
HWTEST_F(Vp8decFuncNdkTest, VIDEO_VP8DEC_FUNCTION_0012, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = -1;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP8"));
    ASSERT_EQ(AV_ERR_UNSUPPORT, vDecSample->ConfigureVideoDecoder());
}

/**
 * @tc.number    : VIDEO_VP8DEC_FUNCTION_0013
 * @tc.name      : Decode VP8 buffer with pixel format 0
 * @tc.desc      : function test
 */
HWTEST_F(Vp8decFuncNdkTest, VIDEO_VP8DEC_FUNCTION_0013, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = 0;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP8"));
    ASSERT_EQ(AV_ERR_UNSUPPORT, vDecSample->ConfigureVideoDecoder());
}

/**
 * @tc.number    : VIDEO_VP8DEC_FUNCTION_0014
 * @tc.name      : Decode VP8 buffer with pixel format 1
 * @tc.desc      : function test
 */
HWTEST_F(Vp8decFuncNdkTest, VIDEO_VP8DEC_FUNCTION_0014, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = 1;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP8"));
    ASSERT_EQ(AV_ERR_UNSUPPORT, vDecSample->ConfigureVideoDecoder());
}

/**
 * @tc.number    : VIDEO_VP8DEC_FUNCTION_0015
 * @tc.name      : Decode VP8 buffer with pixel format 3
 * @tc.desc      : function test
 */
HWTEST_F(Vp8decFuncNdkTest, VIDEO_VP8DEC_FUNCTION_0015, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = 3;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP8"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE52, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP8DEC_FUNCTION_0016
 * @tc.name      : Decode VP8 buffer with unsupported pixel format 5
 * @tc.desc      : function test
 */
HWTEST_F(Vp8decFuncNdkTest, VIDEO_VP8DEC_FUNCTION_0016, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = 5;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP8"));
    ASSERT_EQ(AV_ERR_UNSUPPORT, vDecSample->ConfigureVideoDecoder());
}

/**
 * @tc.number    : VIDEO_VP8DEC_FUNCTION_0017
 * @tc.name      : Decode VP8 buffer with maximum int value
 * @tc.desc      : function test
 */
HWTEST_F(Vp8decFuncNdkTest, VIDEO_VP8DEC_FUNCTION_0017, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    int32_t max = 2147483647;
    vDecSample->defualtPixelFormat = max;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP8"));
    ASSERT_EQ(AV_ERR_UNSUPPORT, vDecSample->ConfigureVideoDecoder());
}

/**
 * @tc.number    : VIDEO_VP8DEC_FUNCTION_0018
 * @tc.name      : Decode VP8 buffer with NV12 pixel format
 * @tc.desc      : function test
 */
HWTEST_F(Vp8decFuncNdkTest, VIDEO_VP8DEC_FUNCTION_0018, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
    vDecSample->needCheckHash = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP8"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE52, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP8DEC_FUNCTION_0019
 * @tc.name      : Decode VP8 buffer with NV21 pixel format
 * @tc.desc      : function test
 */
HWTEST_F(Vp8decFuncNdkTest, VIDEO_VP8DEC_FUNCTION_0019, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV21;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP8"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE52, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP8DEC_FUNCTION_0020
 * @tc.name      : Decode VP8 buffer with YUVI420 pixel format
 * @tc.desc      : function test
 */
HWTEST_F(Vp8decFuncNdkTest, VIDEO_VP8DEC_FUNCTION_0020, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_YUVI420;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP8"));
    ASSERT_EQ(AV_ERR_UNSUPPORT, vDecSample->ConfigureVideoDecoder());
}

/**
 * @tc.number    : VIDEO_VP8DEC_FUNCTION_0021
 * @tc.name      : Decode VP8 surface with NV12 pixel format
 * @tc.desc      : function test
 */
HWTEST_F(Vp8decFuncNdkTest, VIDEO_VP8DEC_FUNCTION_0021, TestSize.Level3)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->SF_OUTPUT = true;
    vDecSample->outputYuvSurface = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface("OH.Media.Codec.Decoder.Video.VP8"));
    vDecSample->WaitForEOS();
    ASSERT_EQ(FRAMESIZE52, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP8DEC_FUNCTION_0022
 * @tc.name      : Decode VP8 surface with NV21 pixel format
 * @tc.desc      : function test
 */
HWTEST_F(Vp8decFuncNdkTest, VIDEO_VP8DEC_FUNCTION_0022, TestSize.Level3)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->SF_OUTPUT = true;
    vDecSample->outputYuvSurface = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV21;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface("OH.Media.Codec.Decoder.Video.VP8"));
    vDecSample->WaitForEOS();
    ASSERT_EQ(FRAMESIZE52, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP8DEC_FUNCTION_0023
 * @tc.name      : Surface model change in normal state
 * @tc.desc      : function test
 */
HWTEST_F(Vp8decFuncNdkTest, VIDEO_VP8DEC_FUNCTION_0023, TestSize.Level2)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->SF_OUTPUT = true;
    vDecSample->autoSwitchSurface = true;
    vDecSample->AFTER_EOS_DESTORY_CODEC = false;
    vDecSample->sleepOnFPS = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface("OH.Media.Codec.Decoder.Video.VP8"));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->Reset());
    ASSERT_EQ(AV_ERR_INVALID_STATE, vDecSample->SwitchSurface());
    ASSERT_EQ(AV_ERR_OK, vDecSample->Release());
}

/**
 * @tc.number    : VIDEO_VP8DEC_FUNCTION_0024
 * @tc.name      : Surface model change in flushed state
 * @tc.desc      : function test
 */
HWTEST_F(Vp8decFuncNdkTest, VIDEO_VP8DEC_FUNCTION_0024, TestSize.Level2)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->SF_OUTPUT = true;
    vDecSample->autoSwitchSurface = true;
    vDecSample->sleepOnFPS = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface("OH.Media.Codec.Decoder.Video.VP8"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->Flush());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
    ASSERT_EQ(AV_ERR_OK, vDecSample->Stop());
}

/**
 * @tc.number    : VIDEO_VP8DEC_FUNCTION_0025
 * @tc.name      : Surface model change in running state
 * @tc.desc      : function test
 */
HWTEST_F(Vp8decFuncNdkTest, VIDEO_VP8DEC_FUNCTION_0025, TestSize.Level2)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->SF_OUTPUT = false;
    vDecSample->autoSwitchSurface = false;
    vDecSample->CreateSurface();
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec("OH.Media.Codec.Decoder.Video.VP8"));
    ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, vDecSample->SwitchSurface());
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_VP8DEC_FUNCTION_0026
 * @tc.name      : Repeat call setSurface quickly
 * @tc.desc      : function test
 */
HWTEST_F(Vp8decFuncNdkTest, VIDEO_VP8DEC_FUNCTION_0026, TestSize.Level2)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->SF_OUTPUT = true;
    vDecSample->autoSwitchSurface = true;
    vDecSample->sleepOnFPS = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface("OH.Media.Codec.Decoder.Video.VP8"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
    vDecSample->WaitForEOS();
}

/**
 * @tc.number    : VIDEO_VP8DEC_FUNCTION_0027
 * @tc.name      : Surface model change from flush to running state
 * @tc.desc      : function test
 */
HWTEST_F(Vp8decFuncNdkTest, VIDEO_VP8DEC_FUNCTION_0027, TestSize.Level2)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->SF_OUTPUT = true;
    vDecSample->autoSwitchSurface = true;
    vDecSample->AFTER_EOS_DESTORY_CODEC = false;
    vDecSample->sleepOnFPS = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface("OH.Media.Codec.Decoder.Video.VP8"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->Flush());
    ASSERT_EQ(AV_ERR_OK, vDecSample->Start());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
}

/**
 * @tc.number    : VIDEO_VP8DEC_FUNCTION_0028
 * @tc.name      : Surface model change after decoder finishes to End-of-Stream state
 * @tc.desc      : function test
 */
HWTEST_F(Vp8decFuncNdkTest, VIDEO_VP8DEC_FUNCTION_0028, TestSize.Level2)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->SF_OUTPUT = true;
    vDecSample->autoSwitchSurface = true;
    vDecSample->AFTER_EOS_DESTORY_CODEC = false;
    vDecSample->sleepOnFPS = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface("OH.Media.Codec.Decoder.Video.VP8"));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
}

/**
 * @tc.number    : VIDEO_VP8DEC_FUNCTION_0029
 * @tc.name      : Buffer model change after decoder finishes to End-of-Stream state
 * @tc.desc      : function test
 */
HWTEST_F(Vp8decFuncNdkTest, VIDEO_VP8DEC_FUNCTION_0029, TestSize.Level2)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->SF_OUTPUT = false;
    vDecSample->autoSwitchSurface = false;
    vDecSample->CreateSurface();
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec("OH.Media.Codec.Decoder.Video.VP8"));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, vDecSample->SwitchSurface());
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_VP8DEC_FUNCTION_0030
 * @tc.name      : Buffer model change from running to flushed state
 * @tc.desc      : function test
 */
HWTEST_F(Vp8decFuncNdkTest, VIDEO_VP8DEC_FUNCTION_0030, TestSize.Level2)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->SF_OUTPUT = false;
    vDecSample->autoSwitchSurface = false;
    vDecSample->CreateSurface();
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec("OH.Media.Codec.Decoder.Video.VP8"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->Flush());
    ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, vDecSample->SwitchSurface());
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_VP8DEC_FUNCTION_0031
 * @tc.name      : Buffer model change from flushed to running state
 * @tc.desc      : function test
 */
HWTEST_F(Vp8decFuncNdkTest, VIDEO_VP8DEC_FUNCTION_0031, TestSize.Level2)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->SF_OUTPUT = false;
    vDecSample->autoSwitchSurface = false;
    vDecSample->CreateSurface();
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec("OH.Media.Codec.Decoder.Video.VP8"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->Flush());
    ASSERT_EQ(AV_ERR_OK, vDecSample->Start());
    ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, vDecSample->SwitchSurface());
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_VP8DEC_FUNCTION_0032
 * @tc.name      : Buffer model change in normal state
 * @tc.desc      : function test
 */
HWTEST_F(Vp8decFuncNdkTest, VIDEO_VP8DEC_FUNCTION_0032, TestSize.Level2)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->SF_OUTPUT = false;
    vDecSample->autoSwitchSurface = false;
    vDecSample->CreateSurface();
    vDecSample->SF_OUTPUT = false;
    vDecSample->autoSwitchSurface = false;
    vDecSample->CreateSurface();
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec("OH.Media.Codec.Decoder.Video.VP8"));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->Reset());
    ASSERT_EQ(AV_ERR_INVALID_STATE, vDecSample->SwitchSurface());
    ASSERT_EQ(AV_ERR_OK, vDecSample->Release());
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_VP8DEC_FUNCTION_0033
 * @tc.name      : Surface model change in config state
 * @tc.desc      : function test
 */
HWTEST_F(Vp8decFuncNdkTest, VIDEO_VP8DEC_FUNCTION_0033, TestSize.Level1)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->SF_OUTPUT = false;
    vDecSample->autoSwitchSurface = false;
    vDecSample->CreateSurface();
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP8"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_VP8DEC_FUNCTION_0034
 * @tc.name      : Surface model change in config state
 * @tc.desc      : function test
 */
HWTEST_F(Vp8decFuncNdkTest, VIDEO_VP8DEC_FUNCTION_0034, TestSize.Level1)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->SF_OUTPUT = false;
    vDecSample->autoSwitchSurface = false;
    vDecSample->CreateSurface();
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP8"));;
    ASSERT_EQ(AV_ERR_INVALID_STATE, vDecSample->SwitchSurface());
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_VP8DEC_FUNCTION_0035
 * @tc.name      : Two objects repeat call setSurface quickly
 * @tc.desc      : function test
 */
HWTEST_F(Vp8decFuncNdkTest, VIDEO_VP8DEC_FUNCTION_0035, TestSize.Level2)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->SF_OUTPUT = true;
    vDecSample->autoSwitchSurface = true;
    vDecSample->sleepOnFPS = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface("OH.Media.Codec.Decoder.Video.VP8"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
    vDecSample->WaitForEOS();
    vDecSample->ReleaseInFile();

    auto vDecSample_1 = make_shared<VDecAPI11Sample>();
    vDecSample_1->INP_DIR = INP_DIR_1;
    vDecSample_1->SF_OUTPUT = true;
    vDecSample_1->autoSwitchSurface = true;
    vDecSample_1->sleepOnFPS = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample_1->RunVideoDec_Surface("OH.Media.Codec.Decoder.Video.VP8"));
    ASSERT_EQ(AV_ERR_OK, vDecSample_1->SwitchSurface());
    vDecSample_1->WaitForEOS();
    vDecSample_1->ReleaseInFile();
}

/**
 * @tc.number    : VIDEO_VP8DEC_FUNCTION_0036
 * @tc.name      : Repeat call setSurface quickly 2 times
 * @tc.desc      : function test
 */
HWTEST_F(Vp8decFuncNdkTest, VIDEO_VP8DEC_FUNCTION_0036, TestSize.Level2)
{
    for (int i = 0; i < 2; i++) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_1;
        vDecSample->SF_OUTPUT = true;
        vDecSample->autoSwitchSurface = true;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface("OH.Media.Codec.Decoder.Video.VP8"));
        ASSERT_EQ(AV_ERR_OK, vDecSample->RepeatCallSetSurface());
        vDecSample->WaitForEOS();
        vDecSample->ReleaseInFile();
    }
}

/**
 * @tc.number    : VIDEO_VP8DEC_FUNCTION_0037
 * @tc.name      : Decode VP8 buffer with demuxer OGG format
 * @tc.desc      : function test
 */
HWTEST_F(Vp8decFuncNdkTest, VIDEO_VP8DEC_FUNCTION_0037, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->getFormat(INP_DIR_5);
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP8"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE602, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP8DEC_FUNCTION_0038
 * @tc.name      : Decode VP8 buffer with demuxer MKV format
 * @tc.desc      : function test
 */
HWTEST_F(Vp8decFuncNdkTest, VIDEO_VP8DEC_FUNCTION_0038, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->getFormat(INP_DIR_6);
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP8"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE602, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP8DEC_FUNCTION_0039
 * @tc.name      : Decode VP8 buffer and encode to H.264
 * @tc.desc      : function test
 */
HWTEST_F(Vp8decFuncNdkTest, VIDEO_VP8DEC_FUNCTION_0039, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP8"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE52, vDecSample->outFrameCount);

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
 * @tc.number    : VIDEO_VP8DEC_FUNCTION_0040
 * @tc.name      : Decode VP8 buffer with minimum resolution (16x16)
 * @tc.desc      : function test
 */
HWTEST_F(Vp8decFuncNdkTest, VIDEO_VP8DEC_FUNCTION_0040, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_9;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 16;
    vDecSample->DEFAULT_HEIGHT = 16;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP8"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP8DEC_FUNCTION_0041
 * @tc.name      : Decode VP8 buffer with odd resolution (257x129)
 * @tc.desc      : function test
 */
HWTEST_F(Vp8decFuncNdkTest, VIDEO_VP8DEC_FUNCTION_0041, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_8;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 257;
    vDecSample->DEFAULT_HEIGHT = 129;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP8"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP8DEC_FUNCTION_0042
 * @tc.name      : Decode VP8 buffer with non 16-aligned resolution (600x800)
 * @tc.desc      : function test
 */
HWTEST_F(Vp8decFuncNdkTest, VIDEO_VP8DEC_FUNCTION_0042, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_12;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 600;;
    vDecSample->DEFAULT_HEIGHT = 800;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP8"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE52, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP8DEC_FUNCTION_0043
 * @tc.name      : Decode VP8 buffer with 16-aligned resolution (480x640)
 * @tc.desc      : function test
 */
HWTEST_F(Vp8decFuncNdkTest, VIDEO_VP8DEC_FUNCTION_0043, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_13;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 480;
    vDecSample->DEFAULT_HEIGHT = 640;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP8"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE52, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP8DEC_FUNCTION_0044
 * @tc.name      : Decode VP8 buffer with 60fps
 * @tc.desc      : function test
 */
HWTEST_F(Vp8decFuncNdkTest, VIDEO_VP8DEC_FUNCTION_0044, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_2;
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP8"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP8DEC_FUNCTION_0045
 * @tc.name      : Decode VP8 buffer with 1280x720 resolution
 * @tc.desc      : function test
 */
HWTEST_F(Vp8decFuncNdkTest, VIDEO_VP8DEC_FUNCTION_0045, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_10;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 1280;
    vDecSample->DEFAULT_HEIGHT = 720;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP8"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP8DEC_FUNCTION_0046
 * @tc.name      : Decode VP8 buffer with 1920x1080 resolution
 * @tc.desc      : function test
 */
HWTEST_F(Vp8decFuncNdkTest, VIDEO_VP8DEC_FUNCTION_0046, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_11;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP8"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP8DEC_FUNCTION_0047
 * @tc.name      : Decode VP8 buffer containing only IP frames
 * @tc.desc      : function test
 */
HWTEST_F(Vp8decFuncNdkTest, VIDEO_VP8DEC_FUNCTION_0047, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_3;
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP8"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP8DEC_FUNCTION_0048
 * @tc.name      : Decode VP8 buffer containing only I frames
 * @tc.desc      : function test
 */
HWTEST_F(Vp8decFuncNdkTest, VIDEO_VP8DEC_FUNCTION_0048, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_4;
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP8"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE100, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP8DEC_FUNCTION_0049
 * @tc.name      : Decode VP8 buffer with demuxer
 * @tc.desc      : function test
 */
HWTEST_F(Vp8decFuncNdkTest, VIDEO_VP8DEC_FUNCTION_0049, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->getFormat(INP_DIR_7);
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP8"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE602, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP8DEC_FUNCTION_0050
 * @tc.name      : Decode VP8 buffer with 4x4 resolution
 * @tc.desc      : function test
 */
HWTEST_F(Vp8decFuncNdkTest, VIDEO_VP8DEC_FUNCTION_0050, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_14;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 4;
    vDecSample->DEFAULT_HEIGHT = 4;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP8"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE52, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VP8DEC_FUNCTION_0051
 * @tc.name      : Decode VP8 buffer with unknown pixelformat
 * @tc.desc      : function test
 */
HWTEST_F(Vp8decFuncNdkTest, VIDEO_VP8DEC_FUNCTION_0051, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    vDecSample->needCheckHash = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP8"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoderNoPixelFormat());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE52, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_DECODE_SYNC_VP8_FUNC_0001
 * @tc.name      : VP8 synchronous decoding with NV12 output
 * @tc.desc      : function test
 */
HWTEST_F(Vp8decFuncNdkTest, VIDEO_DECODE_SYNC_VP8_FUNC_0001, TestSize.Level1)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->enbleSyncMode = 1;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
    vDecSample->needCheckHash = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP8"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartSyncVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE52, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_DECODE_SYNC_VP8_FUNC_0002
 * @tc.name      : VP8 synchronous decoding with NV21 output
 * @tc.desc      : function test
 */
HWTEST_F(Vp8decFuncNdkTest, VIDEO_DECODE_SYNC_VP8_FUNC_0002, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->enbleSyncMode = 1;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV21;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP8"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartSyncVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE52, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_DECODE_SYNC_VP8_FUNC_0003
 * @tc.name      : VP8 synchronous decoding with surface output
 * @tc.desc      : function test
 */
HWTEST_F(Vp8decFuncNdkTest, VIDEO_DECODE_SYNC_VP8_FUNC_0003, TestSize.Level1)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->SF_OUTPUT = true;
    vDecSample->enbleSyncMode = 1;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VP8"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->DecodeSetSurface());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartSyncVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(FRAMESIZE52, vDecSample->outFrameCount);
}

} // namespace