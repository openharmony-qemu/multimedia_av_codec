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
class Vc1decFuncNdkTest : public testing::Test {
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
    const char *INP_DIR_1 = "/data/test/media/vc1_advanced_L1_720x480.avi";
    const char *INP_DIR_2 = "/data/test/media/vc1_advanced_L2_1280x720.avi";
    const char *INP_DIR_3 = "/data/test/media/vc1_advanced_L3_1920x1080_30fps.avi";
    const char *INP_DIR_4 = "/data/test/media/vc1_advanced_L4_1920x1080_60fps.avi";
    const char *INP_DIR_5 = "/data/test/media/vc1_176x144.avi";
    const char *INP_DIR_6 = "/data/test/media/vc1_advanced_L0_352x288.avi";
    const char *INP_DIR_7 = "/data/test/media/vc1_advanced_L3_720x1280.avi";
    const char *INP_DIR_8 = "/data/test/media/vc1_advanced_L4_2048x2048.avi";
    const char *INP_DIR_9 = "/data/test/media/vc1_advanced_L3_IPBFrame.avi";
    const char *INP_DIR_10 = "/data/test/media/vc1_advanced_L3_IFrame_only.avi";
    const char *INP_DIR_11 = "/data/test/media/vc1_advanced_L3_IPFrame_only.avi";
    const char *INP_DIR_12 = "/data/test/media/vc1_fmp4.mp4";
    const char *INP_DIR_13 = "/data/test/media/vc1_mkv.mkv";
    const char *INP_DIR_14 = "/data/test/media/vc1_mov.mov";
    const char *INP_DIR_15 = "/data/test/media/vc1_mp4.mp4";
};

static OH_AVCapability *cap_vc1 = nullptr;
static string g_codecNameVc1 = "";
constexpr uint32_t FRAMESIZE60 = 60;
constexpr uint32_t FRAMESIZE26 = 26;
constexpr uint32_t FRAMESIZE9 = 9;
constexpr uint32_t FRAMESIZE4 = 4;
constexpr uint32_t FRAMESIZE354 = 354;
constexpr uint32_t FRAMESIZE88 = 88;
constexpr uint32_t FRAMESIZE2 = 2;
void Vc1decFuncNdkTest::SetUpTestCase()
{
    cap_vc1 = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_VC1, false, SOFTWARE);
    g_codecNameVc1 = OH_AVCapability_GetName(cap_vc1);
    cout << "g_codecNameVc1: " << g_codecNameVc1 << endl;
    cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_AVC, true, HARDWARE);
    const char *tmpCodecName = OH_AVCapability_GetName(cap);
    if (memcpy_s(g_codecName, sizeof(g_codecName), tmpCodecName, strlen(tmpCodecName)) != 0)
        cout << "memcpy failed" << endl;
    cout << "codecname: " << g_codecName << endl;
}

void Vc1decFuncNdkTest::TearDownTestCase() {}
void Vc1decFuncNdkTest::SetUp() {}
void Vc1decFuncNdkTest::TearDown() {}
} // namespace Media
} // namespace OHOS

static void getVideoSizeRange(OH_AVRange *widthRange, OH_AVRange *heightRange)
{
    memset_s(widthRange, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    memset_s(heightRange, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_VC1, false, SOFTWARE);
    OH_AVCapability_GetVideoHeightRange(capability, heightRange);
    cout << "minval=" << heightRange->minVal << "  maxval=" << heightRange->maxVal << endl;
    OH_AVCapability_GetVideoWidthRange(capability, widthRange);
    cout << "minval=" << widthRange->minVal << "  maxval=" << widthRange->maxVal << endl;
}


namespace {
/**
 * @tc.number    : VIDEO_VC1DEC_FUNCTION_0001
 * @tc.name      : decode Vc1 buffer with default settings
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decFuncNdkTest, VIDEO_VC1DEC_FUNCTION_0001, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    const char *file = "/data/test/media/test_vc1.avi";
    vDecSample->getFormat(file);
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VC1"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE60, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VC1DEC_FUNCTION_0002
 * @tc.name      : decode Vc1 buffer with profile VC1_PROFILE_SIMPLE
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decFuncNdkTest, VIDEO_VC1DEC_FUNCTION_0002, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    const char *file = "/data/test/media/test_vc1.avi";
    vDecSample->getFormat(file);
    vDecSample->outputYuvFlag = true;
    OH_AVFormat_SetIntValue(vDecSample->trackFormat, OH_MD_KEY_PROFILE, VC1_PROFILE_SIMPLE);
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VC1"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE60, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VC1DEC_FUNCTION_0003
 * @tc.name      : decode Vc1 buffer with profile VC1_PROFILE_MAIN
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decFuncNdkTest, VIDEO_VC1DEC_FUNCTION_0003, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    const char *file = "/data/test/media/test_vc1.avi";
    vDecSample->getFormat(file);
    vDecSample->outputYuvFlag = true;
    OH_AVFormat_SetIntValue(vDecSample->trackFormat, OH_MD_KEY_PROFILE, VC1_PROFILE_MAIN);
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VC1"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE60, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VC1DEC_FUNCTION_0004
 * @tc.name      : decode Vc1 buffer with profile VC1_PROFILE_ADVANCED
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decFuncNdkTest, VIDEO_VC1DEC_FUNCTION_0004, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    const char *file = "/data/test/media/test_vc1.avi";
    vDecSample->getFormat(file);
    vDecSample->outputYuvFlag = true;
    OH_AVFormat_SetIntValue(vDecSample->trackFormat, OH_MD_KEY_PROFILE, VC1_PROFILE_ADVANCED);
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VC1"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE60, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VC1DEC_FUNCTION_0005
 * @tc.name      : decode Vc1 buffer with rotation angle 90
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decFuncNdkTest, VIDEO_VC1DEC_FUNCTION_0005, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    const char *file = "/data/test/media/test_vc1.avi";
    vDecSample->getFormat(file);
    int32_t angle = 90;
    (void)OH_AVFormat_SetIntValue(vDecSample->trackFormat, OH_MD_KEY_ROTATION, angle);
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VC1"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE60, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VC1DEC_FUNCTION_0006
 * @tc.name      : decode Vc1 buffer with rotation angle 180
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decFuncNdkTest, VIDEO_VC1DEC_FUNCTION_0006, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    const char *file = "/data/test/media/test_vc1.avi";
    vDecSample->getFormat(file);
    int32_t angle = 180;
    (void)OH_AVFormat_SetIntValue(vDecSample->trackFormat, OH_MD_KEY_ROTATION, angle);
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VC1"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE60, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VC1DEC_FUNCTION_0007
 * @tc.name      : decode Vc1 buffer with rotation angle 270
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decFuncNdkTest, VIDEO_VC1DEC_FUNCTION_0007, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    const char *file = "/data/test/media/test_vc1.avi";
    vDecSample->getFormat(file);
    int32_t angle = 270;
    (void)OH_AVFormat_SetIntValue(vDecSample->trackFormat, OH_MD_KEY_ROTATION, angle);
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VC1"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE60, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VC1DEC_FUNCTION_0008
 * @tc.name      : decode H263 buffer and handle error
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decFuncNdkTest, VIDEO_VC1DEC_FUNCTION_0008, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    const char *file = "/data/test/media/test_vc1.avi";
    vDecSample->getFormat(file);
    vDecSample->INP_DIR = "/data/test/media/profile0_level10_I_128x96.h263";
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VC1"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderFor263());
    vDecSample->WaitForEOS();
    cout << "outFrameCount" << vDecSample->outFrameCount << endl;
    ASSERT_EQ(true, vDecSample->isonError);
}

/**
 * @tc.number    : VIDEO_VC1DEC_FUNCTION_0009
 * @tc.name      : decode Vc1 buffer with invalid width and height (below range)
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decFuncNdkTest, VIDEO_VC1DEC_FUNCTION_0009, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    const char *file = "/data/test/media/test_vc1.avi";
    vDecSample->getFormat(file);
    vDecSample->outputYuvFlag = true;
    OH_AVRange widthRange;
    OH_AVRange heightRange;
    getVideoSizeRange(&widthRange, &heightRange);
    vDecSample->DEFAULT_WIDTH = widthRange.minVal - 1;
    vDecSample->DEFAULT_HEIGHT = heightRange.minVal - 1;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VC1"));
    ASSERT_EQ(AV_ERR_INVALID_VAL, vDecSample->ConfigureVideoDecoder());
}

/**
 * @tc.number    : VIDEO_VC1DEC_FUNCTION_0010
 * @tc.name      : decode Vc1 buffer with invalid width and height (above range)
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decFuncNdkTest, VIDEO_VC1DEC_FUNCTION_0010, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    const char *file = "/data/test/media/test_vc1.avi";
    vDecSample->getFormat(file);
    vDecSample->outputYuvFlag = true;
    OH_AVRange widthRange;
    OH_AVRange heightRange;
    getVideoSizeRange(&widthRange, &heightRange);
    vDecSample->DEFAULT_WIDTH = widthRange.maxVal + 1;
    vDecSample->DEFAULT_HEIGHT = heightRange.maxVal + 1;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VC1"));
    ASSERT_EQ(AV_ERR_INVALID_VAL, vDecSample->ConfigureVideoDecoder());
}

/**
 * @tc.number    : VIDEO_VC1DEC_FUNCTION_0011
 * @tc.name      : decode Vc1 buffer with maximum width and height
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decFuncNdkTest, VIDEO_VC1DEC_FUNCTION_0011, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    const char *file = "/data/test/media/test_vc1.avi";
    vDecSample->getFormat(file);
    vDecSample->outputYuvFlag = true;
    int32_t max = 2147483647;
    vDecSample->DEFAULT_WIDTH = max;
    vDecSample->DEFAULT_HEIGHT = max;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VC1"));
    ASSERT_EQ(AV_ERR_INVALID_VAL, vDecSample->ConfigureVideoDecoder());
}

/**
 * @tc.number    : VIDEO_VC1DEC_FUNCTION_0012
 * @tc.name      : decode Vc1 buffer with minimum valid width and height
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decFuncNdkTest, VIDEO_VC1DEC_FUNCTION_0012, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    const char *file = "/data/test/media/test_vc1.avi";
    vDecSample->getFormat(file);
    vDecSample->outputYuvFlag = true;
    OH_AVRange widthRange;
    OH_AVRange heightRange;
    getVideoSizeRange(&widthRange, &heightRange);
    vDecSample->DEFAULT_WIDTH = widthRange.minVal;
    vDecSample->DEFAULT_HEIGHT = heightRange.minVal;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VC1"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE60, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VC1DEC_FUNCTION_0013
 * @tc.name      : decode Vc1 buffer with maximum valid width and height
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decFuncNdkTest, VIDEO_VC1DEC_FUNCTION_0013, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    const char *file = "/data/test/media/test_vc1.avi";
    vDecSample->getFormat(file);
    vDecSample->outputYuvFlag = true;
    OH_AVRange widthRange;
    OH_AVRange heightRange;
    getVideoSizeRange(&widthRange, &heightRange);
    vDecSample->DEFAULT_WIDTH = widthRange.maxVal;
    vDecSample->DEFAULT_HEIGHT = heightRange.maxVal;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VC1"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE60, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VC1DEC_FUNCTION_0014
 * @tc.name      : decode Vc1 buffer with negative width and height
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decFuncNdkTest, VIDEO_VC1DEC_FUNCTION_0014, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    const char *file = "/data/test/media/test_vc1.avi";
    vDecSample->getFormat(file);
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = -1;
    vDecSample->DEFAULT_HEIGHT = -1;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VC1"));
    ASSERT_EQ(AV_ERR_INVALID_VAL, vDecSample->ConfigureVideoDecoder());
}

/**
 * @tc.number    : VIDEO_VC1DEC_FUNCTION_0015
 * @tc.name      : decode Vc1 buffer with unsupported pixel format (-1)
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decFuncNdkTest, VIDEO_VC1DEC_FUNCTION_0015, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    const char *file = "/data/test/media/test_vc1.avi";
    vDecSample->getFormat(file);
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = -1;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VC1"));
    ASSERT_EQ(AV_ERR_UNSUPPORT, vDecSample->ConfigureVideoDecoder());
}

/**
 * @tc.number    : VIDEO_VC1DEC_FUNCTION_0016
 * @tc.name      : decode Vc1 buffer with unsupported pixel format (0)
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decFuncNdkTest, VIDEO_VC1DEC_FUNCTION_0016, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    const char *file = "/data/test/media/test_vc1.avi";
    vDecSample->getFormat(file);
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = 0;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VC1"));
    ASSERT_EQ(AV_ERR_UNSUPPORT, vDecSample->ConfigureVideoDecoder());
}

/**
 * @tc.number    : VIDEO_VC1DEC_FUNCTION_0017
 * @tc.name      : decode Vc1 buffer with supported pixel format (1)
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decFuncNdkTest, VIDEO_VC1DEC_FUNCTION_0017, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    const char *file = "/data/test/media/test_vc1.avi";
    vDecSample->getFormat(file);
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = 1;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VC1"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE60, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VC1DEC_FUNCTION_0018
 * @tc.name      : decode Vc1 buffer with supported pixel format (3)
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decFuncNdkTest, VIDEO_VC1DEC_FUNCTION_0018, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    const char *file = "/data/test/media/test_vc1.avi";
    vDecSample->getFormat(file);
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = 3;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VC1"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE60, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VC1DEC_FUNCTION_0019
 * @tc.name      : decode Vc1 buffer with unsupported pixel format (5)
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decFuncNdkTest, VIDEO_VC1DEC_FUNCTION_0019, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    const char *file = "/data/test/media/test_vc1.avi";
    vDecSample->getFormat(file);
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = 5;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VC1"));
    ASSERT_EQ(AV_ERR_UNSUPPORT, vDecSample->ConfigureVideoDecoder());
}

/**
 * @tc.number    : VIDEO_VC1DEC_FUNCTION_0020
 * @tc.name      : decode Vc1 buffer with unsupported pixel format (maximum value)
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decFuncNdkTest, VIDEO_VC1DEC_FUNCTION_0020, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    const char *file = "/data/test/media/test_vc1.avi";
    vDecSample->getFormat(file);
    vDecSample->outputYuvFlag = true;
    int32_t max = 2147483647;
    vDecSample->defualtPixelFormat = max;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VC1"));
    ASSERT_EQ(AV_ERR_UNSUPPORT, vDecSample->ConfigureVideoDecoder());
}

/**
 * @tc.number    : VIDEO_VC1DEC_FUNCTION_0021
 * @tc.name      : decode Vc1 buffer with pixel format NV12
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decFuncNdkTest, VIDEO_VC1DEC_FUNCTION_0021, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    const char *file = "/data/test/media/test_vc1.avi";
    vDecSample->getFormat(file);
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VC1"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE60, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VC1DEC_FUNCTION_0022
 * @tc.name      : decode Vc1 buffer with pixel format NV21
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decFuncNdkTest, VIDEO_VC1DEC_FUNCTION_0022, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    const char *file = "/data/test/media/test_vc1.avi";
    vDecSample->getFormat(file);
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV21;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VC1"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE60, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VC1DEC_FUNCTION_0023
 * @tc.name      : decode Vc1 buffer with pixel format YUVI420
 * @tc.desc      : Test decoding VC1 buffer with YUVI420 pixel format and verify output frame count.
 */
HWTEST_F(Vc1decFuncNdkTest, VIDEO_VC1DEC_FUNCTION_0023, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    const char *file = "/data/test/media/test_vc1.avi";
    
    vDecSample->getFormat(file);
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_YUVI420;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VC1"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE60, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VC1DEC_FUNCTION_0024
 * @tc.name      : decode Vc1 surface with pixel format NV12
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decFuncNdkTest, VIDEO_VC1DEC_FUNCTION_0024, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    const char *file = "/data/test/media/test_vc1.avi";
    vDecSample->getFormat(file);
    vDecSample->SF_OUTPUT = true;
    vDecSample->outputYuvSurface = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface("OH.Media.Codec.Decoder.Video.VC1"));
    vDecSample->WaitForEOS();
    ASSERT_EQ(FRAMESIZE60, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VC1DEC_FUNCTION_0025
 * @tc.name      : decode Vc1 surface with pixel format NV21
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decFuncNdkTest, VIDEO_VC1DEC_FUNCTION_0025, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    const char *file = "/data/test/media/test_vc1.avi";
    vDecSample->getFormat(file);
    vDecSample->SF_OUTPUT = true;
    vDecSample->outputYuvSurface = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV21;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface("OH.Media.Codec.Decoder.Video.VC1"));
    vDecSample->WaitForEOS();
    ASSERT_EQ(FRAMESIZE60, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VC1DEC_FUNCTION_0026
 * @tc.name      : switch surface model in normal state
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decFuncNdkTest, VIDEO_VC1DEC_FUNCTION_0026, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    const char *file = "/data/test/media/test_vc1.avi";
    
    vDecSample->getFormat(file);
    vDecSample->SURFACE_OUTPUT = true;
    vDecSample->autoSwitchSurface = true;
    vDecSample->AFTER_EOS_DESTORY_CODEC = false;
    vDecSample->sleepOnFPS = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface("OH.Media.Codec.Decoder.Video.VC1"));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->Reset());
    ASSERT_EQ(AV_ERR_INVALID_STATE, vDecSample->SwitchSurface());
    ASSERT_EQ(AV_ERR_OK, vDecSample->Release());
}

/**
 * @tc.number    : VIDEO_VC1DEC_FUNCTION_0027
 * @tc.name      : switch surface model in flushed state
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decFuncNdkTest, VIDEO_VC1DEC_FUNCTION_0027, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    const char *file = "/data/test/media/test_vc1.avi";
    
    vDecSample->getFormat(file);
    vDecSample->SURFACE_OUTPUT = true;
    vDecSample->autoSwitchSurface = true;
    vDecSample->sleepOnFPS = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface("OH.Media.Codec.Decoder.Video.VC1"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->Flush());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
    ASSERT_EQ(AV_ERR_OK, vDecSample->Stop());
}

/**
 * @tc.number    : VIDEO_VC1DEC_FUNCTION_0028
 * @tc.name      : switch surface model in running state
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decFuncNdkTest, VIDEO_VC1DEC_FUNCTION_0028, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    const char *file = "/data/test/media/test_vc1.avi";
    
    vDecSample->getFormat(file);
    vDecSample->SURFACE_OUTPUT = false;
    vDecSample->autoSwitchSurface = false;
    vDecSample->CreateSurface();
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec("OH.Media.Codec.Decoder.Video.VC1"));
    ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, vDecSample->SwitchSurface());
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_VC1DEC_FUNCTION_0029
 * @tc.name      : repeat setSurface calls quickly
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decFuncNdkTest, VIDEO_VC1DEC_FUNCTION_0029, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    const char *file = "/data/test/media/test_vc1.avi";
    
    vDecSample->getFormat(file);
    vDecSample->SURFACE_OUTPUT = true;
    vDecSample->autoSwitchSurface = true;
    vDecSample->sleepOnFPS = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface("OH.Media.Codec.Decoder.Video.VC1"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
    vDecSample->WaitForEOS();
}

/**
 * @tc.number    : VIDEO_VC1DEC_FUNCTION_0030
 * @tc.name      : switch surface model from flush to running state
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decFuncNdkTest, VIDEO_VC1DEC_FUNCTION_0030, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    const char *file = "/data/test/media/test_vc1.avi";
    
    vDecSample->getFormat(file);
    vDecSample->SURFACE_OUTPUT = true;
    vDecSample->autoSwitchSurface = true;
    vDecSample->AFTER_EOS_DESTORY_CODEC = false;
    vDecSample->sleepOnFPS = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface("OH.Media.Codec.Decoder.Video.VC1"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->Flush());
    ASSERT_EQ(AV_ERR_OK, vDecSample->Start());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
}

/**
 * @tc.number    : VIDEO_VC1DEC_FUNCTION_0031
 * @tc.name      : switch surface model after decoder finishes to EOS state
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decFuncNdkTest, VIDEO_VC1DEC_FUNCTION_0031, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    const char *file = "/data/test/media/test_vc1.avi";
    
    vDecSample->getFormat(file);
    vDecSample->SURFACE_OUTPUT = true;
    vDecSample->autoSwitchSurface = true;
    vDecSample->AFTER_EOS_DESTORY_CODEC = false;
    vDecSample->sleepOnFPS = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface("OH.Media.Codec.Decoder.Video.VC1"));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
}

/**
 * @tc.number    : VIDEO_VC1DEC_FUNCTION_0032
 * @tc.name      : buffer model change after decoder finishes to EOS state
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decFuncNdkTest, VIDEO_VC1DEC_FUNCTION_0032, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    const char *file = "/data/test/media/test_vc1.avi";
    
    vDecSample->getFormat(file);
    vDecSample->SURFACE_OUTPUT = false;
    vDecSample->autoSwitchSurface = false;
    vDecSample->CreateSurface();
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec("OH.Media.Codec.Decoder.Video.VC1"));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, vDecSample->SwitchSurface());
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_VC1DEC_FUNCTION_0033
 * @tc.name      : buffer model change from running to flushed state
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decFuncNdkTest, VIDEO_VC1DEC_FUNCTION_0033, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    const char *file = "/data/test/media/test_vc1.avi";
    
    vDecSample->getFormat(file);
    vDecSample->SURFACE_OUTPUT = false;
    vDecSample->autoSwitchSurface = false;
    vDecSample->CreateSurface();
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec("OH.Media.Codec.Decoder.Video.VC1"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->Flush());
    ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, vDecSample->SwitchSurface());
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_VC1DEC_FUNCTION_0034
 * @tc.name      : buffer model change from flushed to running state
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decFuncNdkTest, VIDEO_VC1DEC_FUNCTION_0034, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    const char *file = "/data/test/media/test_vc1.avi";
    
    vDecSample->getFormat(file);
    vDecSample->SURFACE_OUTPUT = false;
    vDecSample->autoSwitchSurface = false;
    vDecSample->CreateSurface();
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec("OH.Media.Codec.Decoder.Video.VC1"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->Flush());
    ASSERT_EQ(AV_ERR_OK, vDecSample->Start());
    ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, vDecSample->SwitchSurface());
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_VC1DEC_FUNCTION_0035
 * @tc.name      : buffer model change in normal state
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decFuncNdkTest, VIDEO_VC1DEC_FUNCTION_0035, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    const char *file = "/data/test/media/test_vc1.avi";
    
    vDecSample->getFormat(file);
    vDecSample->SURFACE_OUTPUT = false;
    vDecSample->autoSwitchSurface = false;
    vDecSample->CreateSurface();
    vDecSample->SURFACE_OUTPUT = false;
    vDecSample->autoSwitchSurface = false;
    vDecSample->CreateSurface();
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec("OH.Media.Codec.Decoder.Video.VC1"));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->Reset());
    ASSERT_EQ(AV_ERR_INVALID_STATE, vDecSample->SwitchSurface());
    ASSERT_EQ(AV_ERR_OK, vDecSample->Release());
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_VC1DEC_FUNCTION_0036
 * @tc.name      : switch surface model in config state
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decFuncNdkTest, VIDEO_VC1DEC_FUNCTION_0036, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    const char *file = "/data/test/media/test_vc1.avi";
    
    vDecSample->getFormat(file);
    vDecSample->SURFACE_OUTPUT = false;
    vDecSample->autoSwitchSurface = false;
    vDecSample->CreateSurface();
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VC1"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_VC1DEC_FUNCTION_0037
 * @tc.name      : invalid surface switch in config state
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decFuncNdkTest, VIDEO_VC1DEC_FUNCTION_0037, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    const char *file = "/data/test/media/test_vc1.avi";
    
    vDecSample->getFormat(file);
    vDecSample->SURFACE_OUTPUT = false;
    vDecSample->autoSwitchSurface = false;
    vDecSample->CreateSurface();
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VC1"));;
    ASSERT_EQ(AV_ERR_INVALID_STATE, vDecSample->SwitchSurface());
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_VC1DEC_FUNCTION_0038
 * @tc.name      : two objects repeat setSurface calls quickly
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decFuncNdkTest, VIDEO_VC1DEC_FUNCTION_0038, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    const char *file = "/data/test/media/test_vc1.avi";
    
    vDecSample->getFormat(file);
    vDecSample->SURFACE_OUTPUT = true;
    vDecSample->autoSwitchSurface = true;
    vDecSample->sleepOnFPS = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface("OH.Media.Codec.Decoder.Video.VC1"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
    vDecSample->WaitForEOS();
    close(vDecSample->g_fd);

    auto vDecSample_1 = make_shared<VDecAPI11Sample>();
    vDecSample_1->getFormat(file);
    vDecSample_1->SURFACE_OUTPUT = true;
    vDecSample_1->autoSwitchSurface = true;
    vDecSample_1->sleepOnFPS = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample_1->RunVideoDec_Surface("OH.Media.Codec.Decoder.Video.VC1"));
    ASSERT_EQ(AV_ERR_OK, vDecSample_1->SwitchSurface());
    vDecSample_1->WaitForEOS();
}

/**
 * @tc.number    : VIDEO_VC1DEC_FUNCTION_0039
 * @tc.name      : repeat setSurface calls quickly two times
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decFuncNdkTest, VIDEO_VC1DEC_FUNCTION_0039, TestSize.Level0)
{
    for (int i = 0; i < 2; i++) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        const char *file = "/data/test/media/test_vc1.avi";
        
        vDecSample->getFormat(file);
        vDecSample->SURFACE_OUTPUT = true;
        vDecSample->autoSwitchSurface = true;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface("OH.Media.Codec.Decoder.Video.VC1"));
        ASSERT_EQ(AV_ERR_OK, vDecSample->RepeatCallSetSurface());
        vDecSample->WaitForEOS();
        close(vDecSample->g_fd);
    }
}

/**
 * @tc.number    : VIDEO_VC1DEC_FUNCTION_0040
 * @tc.name      : decode Vc1 buffer from MP4 file
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decFuncNdkTest, VIDEO_VC1DEC_FUNCTION_0040, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    const char *file = "/data/test/media/vc1_fmp4.mp4";
    
    vDecSample->getFormat(file);
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VC1"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE60, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VC1DEC_FUNCTION_0041
 * @tc.name      : decode Vc1 buffer from MKV file
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decFuncNdkTest, VIDEO_VC1DEC_FUNCTION_0041, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    const char *file = "/data/test/media/vc1_mkv.mkv";
    
    vDecSample->getFormat(file);
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VC1"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE60, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VC1DEC_FUNCTION_0042
 * @tc.name      : decode Vc1 buffer from MP4 file
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decFuncNdkTest, VIDEO_VC1DEC_FUNCTION_0042, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    const char *file = "/data/test/media/vc1_mp4.mp4";
    
    vDecSample->getFormat(file);
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VC1"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE60, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VC1DEC_FUNCTION_0044
 * @tc.name      : decode Vc1 buffer from MOV file
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decFuncNdkTest, VIDEO_VC1DEC_FUNCTION_0044, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    const char *file = "/data/test/media/vc1_mov.mov";
    
    vDecSample->getFormat(file);
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VC1"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE60, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VC1DEC_FUNCTION_0045
 * @tc.name      : decode Vc1 buffer and encode to H264
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decFuncNdkTest, VIDEO_VC1DEC_FUNCTION_0045, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    const char *file = "/data/test/media/test_vc1.avi";
    vDecSample->getFormat(file);
    
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VC1"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE60, vDecSample->outFrameCount);

    auto vEncSample = make_unique<VEncNdkSample>();
    vEncSample->INP_DIR = vDecSample->OUT_DIR;
    vEncSample->DEFAULT_WIDTH = 1920;
    vEncSample->DEFAULT_HEIGHT = 1080;
    vEncSample->DEFAULT_FRAME_RATE = 30;
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

/**
 * @tc.number    : VIDEO_VC1DEC_FUNCTION_0046
 * @tc.name      : decode Vc1 buffer, videosize 720x480
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decFuncNdkTest, VIDEO_VC1DEC_FUNCTION_0046, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->getFormat(INP_DIR_1);
    vDecSample->NocaleHash = true;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = 720;
    vDecSample->DEFAULT_HEIGHT = 480;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VC1"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE26, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VC1DEC_FUNCTION_0047
 * @tc.name      : decode Vc1 buffer, videosize 1280x720
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decFuncNdkTest, VIDEO_VC1DEC_FUNCTION_0047, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->getFormat(INP_DIR_2);
    vDecSample->outputYuvFlag = true;
    vDecSample->NocaleHash = true;
    vDecSample->DEFAULT_WIDTH = 1280;
    vDecSample->DEFAULT_HEIGHT = 720;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VC1"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE9, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VC1DEC_FUNCTION_0048
 * @tc.name      : decode Vc1 buffer, 30fps
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decFuncNdkTest, VIDEO_VC1DEC_FUNCTION_0048, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->getFormat(INP_DIR_3);
    vDecSample->outputYuvFlag = true;
    vDecSample->NocaleHash = true;
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VC1"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE4, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VC1DEC_FUNCTION_0049
 * @tc.name      : decode Vc1 buffer, 60fps
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decFuncNdkTest, VIDEO_VC1DEC_FUNCTION_0049, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->getFormat(INP_DIR_4);
    vDecSample->outputYuvFlag = true;
    vDecSample->NocaleHash = true;
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VC1"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE4, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VC1DEC_FUNCTION_0050
 * @tc.name      : decode Vc1 buffer, 176x144
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decFuncNdkTest, VIDEO_VC1DEC_FUNCTION_0050, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->getFormat(INP_DIR_5);
    vDecSample->outputYuvFlag = true;
    vDecSample->NocaleHash = true;
    vDecSample->DEFAULT_WIDTH = 176;
    vDecSample->DEFAULT_HEIGHT = 144;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VC1"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE354, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VC1DEC_FUNCTION_0051
 * @tc.name      : decode Vc1 buffer, advanced profile L0
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decFuncNdkTest, VIDEO_VC1DEC_FUNCTION_0051, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->getFormat(INP_DIR_6);
    vDecSample->outputYuvFlag = true;
    vDecSample->NocaleHash = true;
    vDecSample->DEFAULT_WIDTH = 352;
    vDecSample->DEFAULT_HEIGHT = 288;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VC1"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE88, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VC1DEC_FUNCTION_0052
 * @tc.name      : decode Vc1 buffer, 720x1280
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decFuncNdkTest, VIDEO_VC1DEC_FUNCTION_0052, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->getFormat(INP_DIR_7);
    vDecSample->outputYuvFlag = true;
    vDecSample->NocaleHash = true;
    vDecSample->DEFAULT_WIDTH = 720;
    vDecSample->DEFAULT_HEIGHT = 1280;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VC1"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE9, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VC1DEC_FUNCTION_0053
 * @tc.name      : decode Vc1 buffer, advanced profile L4,2048x2048
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decFuncNdkTest, VIDEO_VC1DEC_FUNCTION_0053, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->getFormat(INP_DIR_8);
    vDecSample->outputYuvFlag = true;
    vDecSample->NocaleHash = true;
    vDecSample->DEFAULT_WIDTH = 2048;
    vDecSample->DEFAULT_HEIGHT = 2048;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VC1"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE2, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VC1DEC_FUNCTION_0054
 * @tc.name      : decode Vc1 buffer, IPB frames
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decFuncNdkTest, VIDEO_VC1DEC_FUNCTION_0054, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->getFormat(INP_DIR_9);
    vDecSample->outputYuvFlag = true;
    vDecSample->NocaleHash = true;
    vDecSample->DEFAULT_WIDTH = 1280;
    vDecSample->DEFAULT_HEIGHT = 720;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VC1"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE9, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VC1DEC_FUNCTION_0055
 * @tc.name      : decode Vc1 buffer, IP frame only
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decFuncNdkTest, VIDEO_VC1DEC_FUNCTION_0055, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->getFormat(INP_DIR_10);
    vDecSample->outputYuvFlag = true;
    vDecSample->NocaleHash = true;
    vDecSample->DEFAULT_WIDTH = 1280;
    vDecSample->DEFAULT_HEIGHT = 720;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VC1"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE9, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_VC1DEC_FUNCTION_0056
 * @tc.name      : decode Vc1 buffer, I frame only
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decFuncNdkTest, VIDEO_VC1DEC_FUNCTION_0056, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->getFormat(INP_DIR_11);
    vDecSample->outputYuvFlag = true;
    vDecSample->NocaleHash = true;
    vDecSample->DEFAULT_WIDTH = 1280;
    vDecSample->DEFAULT_HEIGHT = 720;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VC1"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE9, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_DECODE_SYNC_VC1_FUNC_0001
 * @tc.name      : decode Vc1 buffer synchronously with NV12 output
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decFuncNdkTest, VIDEO_DECODE_SYNC_VC1_FUNC_0001, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    const char *file = "/data/test/media/test_vc1.avi";
    vDecSample->getFormat(file);
    vDecSample->enbleSyncMode = 1;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VC1"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartSyncVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE60, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_DECODE_SYNC_VC1_FUNC_0002
 * @tc.name      : decode Vc1 buffer synchronously with NV21 output
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decFuncNdkTest, VIDEO_DECODE_SYNC_VC1_FUNC_0002, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    const char *file = "/data/test/media/test_vc1.avi";
    vDecSample->getFormat(file);
    vDecSample->enbleSyncMode = 1;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV21;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VC1"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartSyncVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE60, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_DECODE_SYNC_VC1_FUNC_0003
 * @tc.name      : decode Vc1 buffer synchronously with surface output
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decFuncNdkTest, VIDEO_DECODE_SYNC_VC1_FUNC_0003, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    const char *file = "/data/test/media/test_vc1.avi";
    vDecSample->getFormat(file);
    
    vDecSample->SF_OUTPUT = true;
    vDecSample->enbleSyncMode = 1;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VC1"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->DecodeSetSurface());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartSyncVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(FRAMESIZE60, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_DECODE_SYNC_VC1_FUNC_0004
 * @tc.name      : decode Vc1 buffer synchronously with YUVI420 output
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decFuncNdkTest, VIDEO_DECODE_SYNC_VC1_FUNC_0004, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    const char *file = "/data/test/media/test_vc1.avi";
    vDecSample->getFormat(file);
    vDecSample->enbleSyncMode = 1;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_YUVI420;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.VC1"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartSyncVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE60, vDecSample->outFrameCount);
}

} // namespace