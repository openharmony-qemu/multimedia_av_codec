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
class MsVideo1decFuncNdkTest : public testing::Test {
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
    const char *INP_DIR_1 = "/data/test/media/msvideo1_720x480.avi";
    const char *INP_DIR_2 = "/data/test/media/msvideo1_4x4.avi";
    const char *INP_DIR_3 = "/data/test/media/msvideo1_4096x4096.avi";
    const char *INP_DIR_4 = "/data/test/media/msvideo1_mkv.mkv";
    const char *INP_DIR_5 = "/data/test/media/msvideo1_mov.mov";
    const char *INP_DIR_6 = "/data/test/media/msvideo1_64_aligned.avi";
    const char *INP_DIR_7 = "/data/test/media/msvideo1_16_aligned.avi";
    const char *INP_DIR_8 = "/data/test/media/msvideo1_30fps.avi";
    const char *INP_DIR_9 = "/data/test/media/msvideo1_max_fps.avi";
    const char *INP_DIR_10 = "/data/test/media/msvideo1_1280x720.avi";
    const char *INP_DIR_11 = "/data/test/media/msvideo1_1920x1080.avi";
    const char *INP_DIR_12 = "/data/test/media/msvideo1_2560x1440.avi";
    const char *INP_DIR_13 = "/data/test/media/msvideo1_720x1280.avi";
};

static OH_AVCapability *cap_msvideo1 = nullptr;
static string g_codecNameMsVideo1 = "";
constexpr uint32_t FRAMESIZE98 = 98;
constexpr uint32_t FRAMESIZE8 = 8;
constexpr uint32_t DEFAULT_WIDTH = 720;
constexpr uint32_t DEFAULT_HEIGHT = 480;
void MsVideo1decFuncNdkTest::SetUpTestCase()
{
    cap_msvideo1 = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_MSVIDEO1, false, SOFTWARE);
    g_codecNameMsVideo1 = OH_AVCapability_GetName(cap_msvideo1);
    cout << "g_codecNameMsVideo1: " << g_codecNameMsVideo1 << endl;
    cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_AVC, true, HARDWARE);
    const char *tmpCodecName = OH_AVCapability_GetName(cap);
    if (memcpy_s(g_codecName, sizeof(g_codecName), tmpCodecName, strlen(tmpCodecName)) != 0)
        cout << "memcpy failed" << endl;
    cout << "codecname: " << g_codecName << endl;
}

void MsVideo1decFuncNdkTest::TearDownTestCase() {}
void MsVideo1decFuncNdkTest::SetUp() {}
void MsVideo1decFuncNdkTest::TearDown() {}
} // namespace Media
} // namespace OHOS

static void getVideoSizeRange(OH_AVRange *widthRange, OH_AVRange *heightRange)
{
    memset_s(widthRange, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    memset_s(heightRange, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_MSVIDEO1, false, SOFTWARE);
    OH_AVCapability_GetVideoHeightRangeForWidth(capability, DEFAULT_WIDTH, heightRange);
    cout << "minval=" << heightRange->minVal << "  maxval=" << heightRange->maxVal << endl;
    OH_AVCapability_GetVideoWidthRangeForHeight(capability, DEFAULT_HEIGHT, widthRange);
    cout << "minval=" << widthRange->minVal << "  maxval=" << widthRange->maxVal << endl;
}


namespace {
/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_FUNCTION_0001
 * @tc.name      : Decode MsVideo1 buffer, 1280x720, NV12
 * @tc.desc      : function test
 */
HWTEST_F(MsVideo1decFuncNdkTest, VIDEO_MSVIDEO1DEC_FUNCTION_0001, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMsVideo1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE98, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_FUNCTION_0002
 * @tc.name      : Decode MsVideo1 buffer with 90-degree rotation
 * @tc.desc      : function test
 */
HWTEST_F(MsVideo1decFuncNdkTest, VIDEO_MSVIDEO1DEC_FUNCTION_0002, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    int32_t angle = 90;
    vDecSample->trackFormat = OH_AVFormat_Create();
    (void)OH_AVFormat_SetIntValue(vDecSample->trackFormat, OH_MD_KEY_ROTATION, angle);
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMsVideo1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE98, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_FUNCTION_0003
 * @tc.name      : Decode MsVideo1 buffer with 180-degree rotation
 * @tc.desc      : function test
 */
HWTEST_F(MsVideo1decFuncNdkTest, VIDEO_MSVIDEO1DEC_FUNCTION_0003, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    int32_t angle = 180;
    vDecSample->trackFormat = OH_AVFormat_Create();
    (void)OH_AVFormat_SetIntValue(vDecSample->trackFormat, OH_MD_KEY_ROTATION, angle);
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMsVideo1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE98, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_FUNCTION_0004
 * @tc.name      : Decode MsVideo1 buffer with 270-degree rotation
 * @tc.desc      : function test
 */
HWTEST_F(MsVideo1decFuncNdkTest, VIDEO_MSVIDEO1DEC_FUNCTION_0004, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    int32_t angle = 270;
    vDecSample->trackFormat = OH_AVFormat_Create();
    (void)OH_AVFormat_SetIntValue(vDecSample->trackFormat, OH_MD_KEY_ROTATION, angle);
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMsVideo1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE98, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_FUNCTION_0005
 * @tc.name      : Decode H.263 buffer with NV12 pixel format
 * @tc.desc      : function test
 */
HWTEST_F(MsVideo1decFuncNdkTest, VIDEO_MSVIDEO1DEC_FUNCTION_0005, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = "/data/test/media/profile0_level10_I_128x96.h263";
    vDecSample->outputYuvFlag = true;
    vDecSample->NocaleHash = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMsVideo1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderFor263());
    vDecSample->WaitForEOS();
    cout << "outFrameCount" << vDecSample->outFrameCount << endl;
    ASSERT_EQ(false, vDecSample->isonError);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_FUNCTION_0006
 * @tc.name      : Decode MsVideo1 buffer with invalid resolution below minimum
 * @tc.desc      : function test
 */
HWTEST_F(MsVideo1decFuncNdkTest, VIDEO_MSVIDEO1DEC_FUNCTION_0006, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    OH_AVRange widthRange;
    OH_AVRange heightRange;
    getVideoSizeRange(&widthRange, &heightRange);
    vDecSample->DEFAULT_WIDTH = widthRange.minVal - 1;
    vDecSample->DEFAULT_HEIGHT = heightRange.minVal - 1;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMsVideo1.c_str()));
    ASSERT_EQ(AV_ERR_INVALID_VAL, vDecSample->ConfigureVideoDecoder());
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_FUNCTION_0007
 * @tc.name      : Decode MsVideo1 buffer with invalid resolution above maximum
 * @tc.desc      : function test
 */
HWTEST_F(MsVideo1decFuncNdkTest, VIDEO_MSVIDEO1DEC_FUNCTION_0007, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    OH_AVRange widthRange;
    OH_AVRange heightRange;
    getVideoSizeRange(&widthRange, &heightRange);
    vDecSample->DEFAULT_WIDTH = widthRange.maxVal + 1;
    vDecSample->DEFAULT_HEIGHT = heightRange.maxVal + 1;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMsVideo1.c_str()));
    ASSERT_EQ(AV_ERR_INVALID_VAL, vDecSample->ConfigureVideoDecoder());
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_FUNCTION_0008
 * @tc.name      : Decode MsVideo1 buffer with extremely large resolution
 * @tc.desc      : function test
 */
HWTEST_F(MsVideo1decFuncNdkTest, VIDEO_MSVIDEO1DEC_FUNCTION_0008, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    int32_t max = 2147483647;
    vDecSample->DEFAULT_WIDTH = max;
    vDecSample->DEFAULT_HEIGHT = max;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMsVideo1.c_str()));
    ASSERT_EQ(AV_ERR_INVALID_VAL, vDecSample->ConfigureVideoDecoder());
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_FUNCTION_0009
 * @tc.name      : Decode MsVideo1 buffer with minimum valid resolution
 * @tc.desc      : function test
 */
HWTEST_F(MsVideo1decFuncNdkTest, VIDEO_MSVIDEO1DEC_FUNCTION_0009, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_2;
    vDecSample->outputYuvFlag = true;
    OH_AVRange widthRange;
    OH_AVRange heightRange;
    getVideoSizeRange(&widthRange, &heightRange);
    vDecSample->DEFAULT_WIDTH = widthRange.minVal;
    vDecSample->DEFAULT_HEIGHT = heightRange.minVal;
    vDecSample->NocaleHash = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMsVideo1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE98, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_FUNCTION_0010
 * @tc.name      : Decode MsVideo1 buffer with maximum valid resolution
 * @tc.desc      : function test
 */
HWTEST_F(MsVideo1decFuncNdkTest, VIDEO_MSVIDEO1DEC_FUNCTION_0010, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_3;
    vDecSample->outputYuvFlag = true;
    OH_AVRange widthRange;
    OH_AVRange heightRange;
    getVideoSizeRange(&widthRange, &heightRange);
    vDecSample->DEFAULT_WIDTH = widthRange.maxVal;
    vDecSample->DEFAULT_HEIGHT = heightRange.maxVal;
    vDecSample->NocaleHash = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMsVideo1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE8, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_FUNCTION_0011
 * @tc.name      : Decode MsVideo1 buffer with negative resolution
 * @tc.desc      : function test
 */
HWTEST_F(MsVideo1decFuncNdkTest, VIDEO_MSVIDEO1DEC_FUNCTION_0011, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = -1;
    vDecSample->DEFAULT_HEIGHT = -1;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMsVideo1.c_str()));
    ASSERT_EQ(AV_ERR_INVALID_VAL, vDecSample->ConfigureVideoDecoder());
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_FUNCTION_0012
 * @tc.name      : Decode MsVideo1 buffer with unsupported pixel format (-1)
 * @tc.desc      : function test
 */
HWTEST_F(MsVideo1decFuncNdkTest, VIDEO_MSVIDEO1DEC_FUNCTION_0012, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = -1;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMsVideo1.c_str()));
    ASSERT_EQ(AV_ERR_UNSUPPORT, vDecSample->ConfigureVideoDecoder());
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_FUNCTION_0013
 * @tc.name      : Decode MsVideo1 buffer with unsupported pixel format (0)
 * @tc.desc      : function test
 */
HWTEST_F(MsVideo1decFuncNdkTest, VIDEO_MSVIDEO1DEC_FUNCTION_0013, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = 0;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMsVideo1.c_str()));
    ASSERT_EQ(AV_ERR_UNSUPPORT, vDecSample->ConfigureVideoDecoder());
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_FUNCTION_0014
 * @tc.name      : Decode MsVideo1 buffer with supported pixel format (3)
 * @tc.desc      : function test
 */
HWTEST_F(MsVideo1decFuncNdkTest, VIDEO_MSVIDEO1DEC_FUNCTION_0014, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = 3;
    vDecSample->NocaleHash = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMsVideo1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE98, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_FUNCTION_0015
 * @tc.name      : Decode MsVideo1 buffer with unsupported pixel format (max int)
 * @tc.desc      : function test
 */
HWTEST_F(MsVideo1decFuncNdkTest, VIDEO_MSVIDEO1DEC_FUNCTION_0015, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    int32_t max = 2147483647;
    vDecSample->defualtPixelFormat = max;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMsVideo1.c_str()));
    ASSERT_EQ(AV_ERR_UNSUPPORT, vDecSample->ConfigureVideoDecoder());
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_FUNCTION_0016
 * @tc.name      : Decode MsVideo1 buffer with NV12 pixel format
 * @tc.desc      : function test
 */
HWTEST_F(MsVideo1decFuncNdkTest, VIDEO_MSVIDEO1DEC_FUNCTION_0016, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMsVideo1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE98, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_FUNCTION_0017
 * @tc.name      : Decode MsVideo1 buffer with NV21 pixel format
 * @tc.desc      : function test
 */
HWTEST_F(MsVideo1decFuncNdkTest, VIDEO_MSVIDEO1DEC_FUNCTION_0017, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV21;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMsVideo1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE98, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_FUNCTION_0018
 * @tc.name      : Decode MsVideo1 surface with NV12 pixel format
 * @tc.desc      : function test
 */
HWTEST_F(MsVideo1decFuncNdkTest, VIDEO_MSVIDEO1DEC_FUNCTION_0018, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->SF_OUTPUT = true;
    vDecSample->outputYuvSurface = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameMsVideo1.c_str()));
    vDecSample->WaitForEOS();
    ASSERT_EQ(FRAMESIZE98, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_FUNCTION_0019
 * @tc.name      : Decode MsVideo1 surface with NV21 pixel format
 * @tc.desc      : function test
 */
HWTEST_F(MsVideo1decFuncNdkTest, VIDEO_MSVIDEO1DEC_FUNCTION_0019, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->SF_OUTPUT = true;
    vDecSample->outputYuvSurface = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV21;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameMsVideo1.c_str()));
    vDecSample->WaitForEOS();
    ASSERT_EQ(FRAMESIZE98, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_FUNCTION_0020
 * @tc.name      : Surface model change in normal state
 * @tc.desc      : function test
 */
HWTEST_F(MsVideo1decFuncNdkTest, VIDEO_MSVIDEO1DEC_FUNCTION_0020, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->SURFACE_OUTPUT = true;
    vDecSample->autoSwitchSurface = true;
    vDecSample->AFTER_EOS_DESTORY_CODEC = false;
    vDecSample->sleepOnFPS = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameMsVideo1.c_str()));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->Reset());
    ASSERT_EQ(AV_ERR_INVALID_STATE, vDecSample->SwitchSurface());
    ASSERT_EQ(AV_ERR_OK, vDecSample->Release());
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_FUNCTION_0021
 * @tc.name      : Surface model change in flushed state
 * @tc.desc      : function test
 */
HWTEST_F(MsVideo1decFuncNdkTest, VIDEO_MSVIDEO1DEC_FUNCTION_0021, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->SURFACE_OUTPUT = true;
    vDecSample->autoSwitchSurface = true;
    vDecSample->sleepOnFPS = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameMsVideo1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->Flush());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
    ASSERT_EQ(AV_ERR_OK, vDecSample->Stop());
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_FUNCTION_0022
 * @tc.name      : Surface model change in running state
 * @tc.desc      : function test
 */
HWTEST_F(MsVideo1decFuncNdkTest, VIDEO_MSVIDEO1DEC_FUNCTION_0022, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->SURFACE_OUTPUT = false;
    vDecSample->autoSwitchSurface = false;
    vDecSample->CreateSurface();
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameMsVideo1.c_str()));
    ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, vDecSample->SwitchSurface());
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_FUNCTION_0023
 * @tc.name      : Repeat call setSurface quickly
 * @tc.desc      : function test
 */
HWTEST_F(MsVideo1decFuncNdkTest, VIDEO_MSVIDEO1DEC_FUNCTION_0023, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->SURFACE_OUTPUT = true;
    vDecSample->autoSwitchSurface = true;
    vDecSample->sleepOnFPS = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameMsVideo1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
    vDecSample->WaitForEOS();
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_FUNCTION_0024
 * @tc.name      : Surface model change in flush to running state
 * @tc.desc      : function test
 */
HWTEST_F(MsVideo1decFuncNdkTest, VIDEO_MSVIDEO1DEC_FUNCTION_0024, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->SURFACE_OUTPUT = true;
    vDecSample->autoSwitchSurface = true;
    vDecSample->AFTER_EOS_DESTORY_CODEC = false;
    vDecSample->sleepOnFPS = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameMsVideo1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->Flush());
    ASSERT_EQ(AV_ERR_OK, vDecSample->Start());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_FUNCTION_0025
 * @tc.name      : Surface model change in decoder finish to End-of-Stream state
 * @tc.desc      : function test
 */
HWTEST_F(MsVideo1decFuncNdkTest, VIDEO_MSVIDEO1DEC_FUNCTION_0025, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->SURFACE_OUTPUT = true;
    vDecSample->autoSwitchSurface = true;
    vDecSample->AFTER_EOS_DESTORY_CODEC = false;
    vDecSample->sleepOnFPS = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameMsVideo1.c_str()));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_FUNCTION_0026
 * @tc.name      : Buffer model change in decoder finish to End-of-Stream state
 * @tc.desc      : function test
 */
HWTEST_F(MsVideo1decFuncNdkTest, VIDEO_MSVIDEO1DEC_FUNCTION_0026, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->SURFACE_OUTPUT = false;
    vDecSample->autoSwitchSurface = false;
    vDecSample->CreateSurface();
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameMsVideo1.c_str()));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, vDecSample->SwitchSurface());
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_FUNCTION_0027
 * @tc.name      : Buffer model change in running to flushed state
 * @tc.desc      : function test
 */
HWTEST_F(MsVideo1decFuncNdkTest, VIDEO_MSVIDEO1DEC_FUNCTION_0027, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->SURFACE_OUTPUT = false;
    vDecSample->autoSwitchSurface = false;
    vDecSample->CreateSurface();
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameMsVideo1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->Flush());
    ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, vDecSample->SwitchSurface());
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_FUNCTION_0028
 * @tc.name      : Buffer model change in flushed to running state
 * @tc.desc      : function test
 */
HWTEST_F(MsVideo1decFuncNdkTest, VIDEO_MSVIDEO1DEC_FUNCTION_0028, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->SURFACE_OUTPUT = false;
    vDecSample->autoSwitchSurface = false;
    vDecSample->CreateSurface();
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameMsVideo1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->Flush());
    ASSERT_EQ(AV_ERR_OK, vDecSample->Start());
    ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, vDecSample->SwitchSurface());
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_FUNCTION_0029
 * @tc.name      : Buffer model change in normal state
 * @tc.desc      : function test
 */
HWTEST_F(MsVideo1decFuncNdkTest, VIDEO_MSVIDEO1DEC_FUNCTION_0029, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->SURFACE_OUTPUT = false;
    vDecSample->autoSwitchSurface = false;
    vDecSample->CreateSurface();
    vDecSample->SURFACE_OUTPUT = false;
    vDecSample->autoSwitchSurface = false;
    vDecSample->CreateSurface();
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameMsVideo1.c_str()));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->Reset());
    ASSERT_EQ(AV_ERR_INVALID_STATE, vDecSample->SwitchSurface());
    ASSERT_EQ(AV_ERR_OK, vDecSample->Release());
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_FUNCTION_0030
 * @tc.name      : Surface model change in config state
 * @tc.desc      : function test
 */
HWTEST_F(MsVideo1decFuncNdkTest, VIDEO_MSVIDEO1DEC_FUNCTION_0030, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->SURFACE_OUTPUT = false;
    vDecSample->autoSwitchSurface = false;
    vDecSample->CreateSurface();
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMsVideo1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_FUNCTION_0031
 * @tc.name      : Surface model change in create state
 * @tc.desc      : function test
 */
HWTEST_F(MsVideo1decFuncNdkTest, VIDEO_MSVIDEO1DEC_FUNCTION_0031, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->SURFACE_OUTPUT = false;
    vDecSample->autoSwitchSurface = false;
    vDecSample->CreateSurface();
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMsVideo1.c_str()));;
    ASSERT_EQ(AV_ERR_INVALID_STATE, vDecSample->SwitchSurface());
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_FUNCTION_0032
 * @tc.name      : Two object repeat call setSurface quickly
 * @tc.desc      : function test
 */
HWTEST_F(MsVideo1decFuncNdkTest, VIDEO_MSVIDEO1DEC_FUNCTION_0032, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->SURFACE_OUTPUT = true;
    vDecSample->autoSwitchSurface = true;
    vDecSample->sleepOnFPS = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameMsVideo1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
    vDecSample->WaitForEOS();
    vDecSample->ReleaseInFile();

    auto vDecSample_1 = make_shared<VDecAPI11Sample>();
    vDecSample_1->INP_DIR = INP_DIR_1;
    vDecSample_1->SURFACE_OUTPUT = true;
    vDecSample_1->autoSwitchSurface = true;
    vDecSample_1->sleepOnFPS = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample_1->RunVideoDec_Surface(g_codecNameMsVideo1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample_1->SwitchSurface());
    vDecSample_1->WaitForEOS();
    vDecSample_1->ReleaseInFile();
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_FUNCTION_0033
 * @tc.name      : Repeat call setSurface quickly 2 times
 * @tc.desc      : function test
 */
HWTEST_F(MsVideo1decFuncNdkTest, VIDEO_MSVIDEO1DEC_FUNCTION_0033, TestSize.Level0)
{
    for (int i = 0; i < 2; i++) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_1;
        vDecSample->SURFACE_OUTPUT = true;
        vDecSample->autoSwitchSurface = true;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameMsVideo1.c_str()));
        ASSERT_EQ(AV_ERR_OK, vDecSample->RepeatCallSetSurface());
        vDecSample->WaitForEOS();
        vDecSample->ReleaseInFile();
    }
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_FUNCTION_0034
 * @tc.name      : Decode MsVideo1 buffer in avi format
 * @tc.desc      : function test
 */
HWTEST_F(MsVideo1decFuncNdkTest, VIDEO_MSVIDEO1DEC_FUNCTION_0034, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    const char *file = INP_DIR_1;
    vDecSample->getFormat(file);
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMsVideo1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE98, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_FUNCTION_0035
 * @tc.name      : Decode MsVideo1 buffer in MKV format
 * @tc.desc      : function test
 */
HWTEST_F(MsVideo1decFuncNdkTest, VIDEO_MSVIDEO1DEC_FUNCTION_0035, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->getFormat(INP_DIR_4);
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMsVideo1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE98, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_FUNCTION_0036
 * @tc.name      : Decode MsVideo1 buffer in MOV format
 * @tc.desc      : function test
 */
HWTEST_F(MsVideo1decFuncNdkTest, VIDEO_MSVIDEO1DEC_FUNCTION_0036, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    const char *file = INP_DIR_5;
    vDecSample->getFormat(file);
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMsVideo1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE98, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_FUNCTION_0038
 * @tc.name      : Decode MsVideo1 buffer and encode to H.264
 * @tc.desc      : function test
 */
HWTEST_F(MsVideo1decFuncNdkTest, VIDEO_MSVIDEO1DEC_FUNCTION_0038, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMsVideo1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE98, vDecSample->outFrameCount);

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
 * @tc.number    : VIDEO_MSVIDEO1DEC_FUNCTION_0040
 * @tc.name      : Decode MsVideo1 buffer with 64-aligned resolution
 * @tc.desc      : function test
 */
HWTEST_F(MsVideo1decFuncNdkTest, VIDEO_MSVIDEO1DEC_FUNCTION_0040, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_6;
    vDecSample->outputYuvFlag = true;
    vDecSample->NocaleHash = true;
    vDecSample->DEFAULT_WIDTH = 320;
    vDecSample->DEFAULT_HEIGHT = 256;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMsVideo1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE98, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_FUNCTION_0041
 * @tc.name      : Decode MsVideo1 buffer with 16-aligned resolution
 * @tc.desc      : function test
 */
HWTEST_F(MsVideo1decFuncNdkTest, VIDEO_MSVIDEO1DEC_FUNCTION_0041, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_7;
    vDecSample->outputYuvFlag = true;
    vDecSample->NocaleHash = true;
    vDecSample->DEFAULT_WIDTH = 320;
    vDecSample->DEFAULT_HEIGHT = 240;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMsVideo1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE98, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_FUNCTION_0042
 * @tc.name      : Decode MsVideo1 buffer with 30fps
 * @tc.desc      : function test
 */
HWTEST_F(MsVideo1decFuncNdkTest, VIDEO_MSVIDEO1DEC_FUNCTION_0042, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_8;
    vDecSample->outputYuvFlag = true;
    vDecSample->NocaleHash = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMsVideo1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(52, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_FUNCTION_0043
 * @tc.name      : Decode MsVideo1 buffer with 60fps
 * @tc.desc      : function test
 */
HWTEST_F(MsVideo1decFuncNdkTest, VIDEO_MSVIDEO1DEC_FUNCTION_0043, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_9;
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMsVideo1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE98, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_FUNCTION_0044
 * @tc.name      : Decode MsVideo1 buffer with resolution 1280x720
 * @tc.desc      : function test
 */
HWTEST_F(MsVideo1decFuncNdkTest, VIDEO_MSVIDEO1DEC_FUNCTION_0044, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_10;
    vDecSample->outputYuvFlag = true;
    vDecSample->NocaleHash = true;
    vDecSample->DEFAULT_WIDTH = 1280;
    vDecSample->DEFAULT_HEIGHT = 720;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMsVideo1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE98, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_FUNCTION_0144
 * @tc.name      : Decode MsVideo1 buffer with resolution 720x1280
 * @tc.desc      : function test
 */
HWTEST_F(MsVideo1decFuncNdkTest, VIDEO_MSVIDEO1DEC_FUNCTION_0144, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_13;
    vDecSample->outputYuvFlag = true;
    vDecSample->NocaleHash = true;
    vDecSample->DEFAULT_WIDTH = 720;
    vDecSample->DEFAULT_HEIGHT = 1280;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMsVideo1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE98, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_FUNCTION_0045
 * @tc.name      : Decode MsVideo1 buffer with resolution 1920x1080
 * @tc.desc      : function test
 */
HWTEST_F(MsVideo1decFuncNdkTest, VIDEO_MSVIDEO1DEC_FUNCTION_0045, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_11;
    vDecSample->outputYuvFlag = true;
    vDecSample->NocaleHash = true;
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMsVideo1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    cout << "in frameCount_ = " << vDecSample->frameCount_ << endl;
    ASSERT_EQ(FRAMESIZE98, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_FUNCTION_0046
 * @tc.name      : Decode MsVideo1 buffer with resolution 2560x1440
 * @tc.desc      : function test
 */
HWTEST_F(MsVideo1decFuncNdkTest, VIDEO_MSVIDEO1DEC_FUNCTION_0046, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_12;
    vDecSample->outputYuvFlag = true;
    vDecSample->NocaleHash = true;
    vDecSample->DEFAULT_WIDTH = 2560;
    vDecSample->DEFAULT_HEIGHT = 1440;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMsVideo1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE98, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_FUNCTION_0047
 * @tc.name      : MsVideo1 synchronous software decoding with NV12 output
 * @tc.desc      : function test
 */
HWTEST_F(MsVideo1decFuncNdkTest, VIDEO_MSVIDEO1DEC_FUNCTION_0047, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->enbleSyncMode = 1;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMsVideo1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartSyncVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE98, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_FUNCTION_0048
 * @tc.name      : MsVideo1 synchronous software decoding with NV21 output
 * @tc.desc      : function test
 */
HWTEST_F(MsVideo1decFuncNdkTest, VIDEO_MSVIDEO1DEC_FUNCTION_0048, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->enbleSyncMode = 1;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV21;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMsVideo1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartSyncVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE98, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_MSVIDEO1DEC_FUNCTION_0049
 * @tc.name      : MsVideo1 synchronous software decoding with surface output
 * @tc.desc      : function test
 */
HWTEST_F(MsVideo1decFuncNdkTest, VIDEO_MSVIDEO1DEC_FUNCTION_0049, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->SF_OUTPUT = true;
    vDecSample->enbleSyncMode = 1;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameMsVideo1.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->DecodeSetSurface());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartSyncVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(FRAMESIZE98, vDecSample->outFrameCount);
}
} // namespace