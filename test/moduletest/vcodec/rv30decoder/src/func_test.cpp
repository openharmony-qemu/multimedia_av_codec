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
class Rv30decFuncNdkTest : public testing::Test {
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
    const char *INP_DIR_1 = "/data/test/media/rv30_track.rv30";
    const char *INP_DIR_2 = "/data/test/media/160x120_15.rm";
};

static OH_AVCapability *cap_rv30 = nullptr;
static string g_codecNameRv30 = "";
constexpr uint32_t FRAMESIZE688 = 688;
constexpr uint32_t DEFAULT_WIDTH = 160;
constexpr uint32_t DEFAULT_HEIGHT = 120;
void Rv30decFuncNdkTest::SetUpTestCase()
{
    cap_rv30 = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_RV30, false, SOFTWARE);
    g_codecNameRv30 = OH_AVCapability_GetName(cap_rv30);
    cout << "g_codecNameRv30: " << g_codecNameRv30 << endl;
    cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_AVC, true, HARDWARE);
    const char *tmpCodecName = OH_AVCapability_GetName(cap);
    if (memcpy_s(g_codecName, sizeof(g_codecName), tmpCodecName, strlen(tmpCodecName)) != 0)
        cout << "memcpy failed" << endl;
    cout << "codecname: " << g_codecName << endl;
}

void Rv30decFuncNdkTest::TearDownTestCase() {}
void Rv30decFuncNdkTest::SetUp() {}
void Rv30decFuncNdkTest::TearDown() {}
} // namespace Media
} // namespace OHOS

static void getVideoSizeRange(OH_AVRange *widthRange, OH_AVRange *heightRange)
{
    memset_s(widthRange, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    memset_s(heightRange, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_RV30, false, SOFTWARE);
    OH_AVCapability_GetVideoHeightRangeForWidth(capability, DEFAULT_WIDTH, heightRange);
    cout << "minval=" << heightRange->minVal << "  maxval=" << heightRange->maxVal << endl;
    OH_AVCapability_GetVideoWidthRangeForHeight(capability, DEFAULT_HEIGHT, widthRange);
    cout << "minval=" << widthRange->minVal << "  maxval=" << widthRange->maxVal << endl;
}


namespace {
/**
 * @tc.number    : VIDEO_RV30DEC_FUNCTION_0001
 * @tc.name      : Decode Rv30 buffer, 160x120, NV12
 * @tc.desc      : function test
 */
HWTEST_F(Rv30decFuncNdkTest, VIDEO_RV30DEC_FUNCTION_0001, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->getFormat(INP_DIR_2);
    vDecSample->outputYuvFlag = true;
    vDecSample->NocaleHash = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameRv30.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE688, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_RV30DEC_FUNCTION_0002
 * @tc.name      : Decode Rv30 buffer with 90-degree rotation
 * @tc.desc      : function test
 */
HWTEST_F(Rv30decFuncNdkTest, VIDEO_RV30DEC_FUNCTION_0002, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->getFormat(INP_DIR_2);
    int32_t angle = 90;
    vDecSample->NocaleHash = true;
    (void)OH_AVFormat_SetIntValue(vDecSample->trackFormat, OH_MD_KEY_ROTATION, angle);
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameRv30.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE688, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_RV30DEC_FUNCTION_0003
 * @tc.name      : Decode Rv30 buffer with 180-degree rotation
 * @tc.desc      : function test
 */
HWTEST_F(Rv30decFuncNdkTest, VIDEO_RV30DEC_FUNCTION_0003, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->getFormat(INP_DIR_2);
    int32_t angle = 180;
    vDecSample->NocaleHash = true;
    (void)OH_AVFormat_SetIntValue(vDecSample->trackFormat, OH_MD_KEY_ROTATION, angle);
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameRv30.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE688, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_RV30DEC_FUNCTION_0004
 * @tc.name      : Decode Rv30 buffer with 270-degree rotation
 * @tc.desc      : function test
 */
HWTEST_F(Rv30decFuncNdkTest, VIDEO_RV30DEC_FUNCTION_0004, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->getFormat(INP_DIR_2);
    int32_t angle = 270;
    vDecSample->NocaleHash = true;
    (void)OH_AVFormat_SetIntValue(vDecSample->trackFormat, OH_MD_KEY_ROTATION, angle);
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameRv30.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE688, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_RV30DEC_FUNCTION_0005
 * @tc.name      : Decode H.263 buffer with NV12 pixel format
 * @tc.desc      : function test
 */
HWTEST_F(Rv30decFuncNdkTest, VIDEO_RV30DEC_FUNCTION_0005, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->getFormat(INP_DIR_2);
    vDecSample->INP_DIR = "/data/test/media/profile0_level10_I_128x96.h263";
    vDecSample->outputYuvFlag = true;
    vDecSample->NocaleHash = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameRv30.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderFor263());
    vDecSample->WaitForEOS();
    cout << "outFrameCount" << vDecSample->outFrameCount << endl;
    ASSERT_EQ(false, vDecSample->isonError);
}

/**
 * @tc.number    : VIDEO_RV30DEC_FUNCTION_0006
 * @tc.name      : Decode Rv30 buffer with invalid resolution below minimum
 * @tc.desc      : function test
 */
HWTEST_F(Rv30decFuncNdkTest, VIDEO_RV30DEC_FUNCTION_0006, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_2;
    vDecSample->outputYuvFlag = true;
    OH_AVRange widthRange;
    OH_AVRange heightRange;
    getVideoSizeRange(&widthRange, &heightRange);
    vDecSample->DEFAULT_WIDTH = widthRange.minVal - 1;
    vDecSample->DEFAULT_HEIGHT = heightRange.minVal - 1;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameRv30.c_str()));
    ASSERT_EQ(AV_ERR_INVALID_VAL, vDecSample->ConfigureVideoDecoder());
}

/**
 * @tc.number    : VIDEO_RV30DEC_FUNCTION_0007
 * @tc.name      : Decode Rv30 buffer with invalid resolution above maximum
 * @tc.desc      : function test
 */
HWTEST_F(Rv30decFuncNdkTest, VIDEO_RV30DEC_FUNCTION_0007, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    OH_AVRange widthRange;
    OH_AVRange heightRange;
    getVideoSizeRange(&widthRange, &heightRange);
    vDecSample->DEFAULT_WIDTH = widthRange.maxVal + 1;
    vDecSample->DEFAULT_HEIGHT = heightRange.maxVal + 1;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameRv30.c_str()));
    ASSERT_EQ(AV_ERR_INVALID_VAL, vDecSample->ConfigureVideoDecoder());
}

/**
 * @tc.number    : VIDEO_RV30DEC_FUNCTION_0008
 * @tc.name      : Decode Rv30 buffer with extremely large resolution
 * @tc.desc      : function test
 */
HWTEST_F(Rv30decFuncNdkTest, VIDEO_RV30DEC_FUNCTION_0008, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    int32_t max = 2147483647;
    vDecSample->DEFAULT_WIDTH = max;
    vDecSample->DEFAULT_HEIGHT = max;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameRv30.c_str()));
    ASSERT_EQ(AV_ERR_INVALID_VAL, vDecSample->ConfigureVideoDecoder());
}

/**
 * @tc.number    : VIDEO_RV30DEC_FUNCTION_0011
 * @tc.name      : Decode Rv30 buffer with negative resolution
 * @tc.desc      : function test
 */
HWTEST_F(Rv30decFuncNdkTest, VIDEO_RV30DEC_FUNCTION_0011, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    vDecSample->DEFAULT_WIDTH = -1;
    vDecSample->DEFAULT_HEIGHT = -1;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameRv30.c_str()));
    ASSERT_EQ(AV_ERR_INVALID_VAL, vDecSample->ConfigureVideoDecoder());
}

/**
 * @tc.number    : VIDEO_RV30DEC_FUNCTION_0012
 * @tc.name      : Decode Rv30 buffer with unsupported pixel format (-1)
 * @tc.desc      : function test
 */
HWTEST_F(Rv30decFuncNdkTest, VIDEO_RV30DEC_FUNCTION_0012, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = -1;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameRv30.c_str()));
    ASSERT_EQ(AV_ERR_UNSUPPORT, vDecSample->ConfigureVideoDecoder());
}

/**
 * @tc.number    : VIDEO_RV30DEC_FUNCTION_0013
 * @tc.name      : Decode Rv30 buffer with unsupported pixel format (0)
 * @tc.desc      : function test
 */
HWTEST_F(Rv30decFuncNdkTest, VIDEO_RV30DEC_FUNCTION_0013, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = 0;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameRv30.c_str()));
    ASSERT_EQ(AV_ERR_UNSUPPORT, vDecSample->ConfigureVideoDecoder());
}

/**
 * @tc.number    : VIDEO_RV30DEC_FUNCTION_0014
 * @tc.name      : Decode Rv30 buffer with supported pixel format (3)
 * @tc.desc      : function test
 */
HWTEST_F(Rv30decFuncNdkTest, VIDEO_RV30DEC_FUNCTION_0014, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->getFormat(INP_DIR_2);
    vDecSample->NocaleHash = true;
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = 3;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameRv30.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE688, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_RV30DEC_FUNCTION_0015
 * @tc.name      : Decode Rv30 buffer with unsupported pixel format (max int)
 * @tc.desc      : function test
 */
HWTEST_F(Rv30decFuncNdkTest, VIDEO_RV30DEC_FUNCTION_0015, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->outputYuvFlag = true;
    int32_t max = 2147483647;
    vDecSample->defualtPixelFormat = max;

    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameRv30.c_str()));
    ASSERT_EQ(AV_ERR_UNSUPPORT, vDecSample->ConfigureVideoDecoder());
}

/**
 * @tc.number    : VIDEO_RV30DEC_FUNCTION_0016
 * @tc.name      : Decode Rv30 buffer with NV12 pixel format
 * @tc.desc      : function test
 */
HWTEST_F(Rv30decFuncNdkTest, VIDEO_RV30DEC_FUNCTION_0016, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->getFormat(INP_DIR_2);
    vDecSample->NocaleHash = true;
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameRv30.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE688, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_RV30DEC_FUNCTION_0017
 * @tc.name      : Decode Rv30 buffer with NV21 pixel format
 * @tc.desc      : function test
 */
HWTEST_F(Rv30decFuncNdkTest, VIDEO_RV30DEC_FUNCTION_0017, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->getFormat(INP_DIR_2);
    vDecSample->outputYuvFlag = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV21;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameRv30.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE688, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_RV30DEC_FUNCTION_0018
 * @tc.name      : Decode Rv30 surface with NV12 pixel format
 * @tc.desc      : function test
 */
HWTEST_F(Rv30decFuncNdkTest, VIDEO_RV30DEC_FUNCTION_0018, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->getFormat(INP_DIR_2);
    vDecSample->SF_OUTPUT = true;
    vDecSample->outputYuvSurface = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameRv30.c_str()));
    vDecSample->WaitForEOS();
    ASSERT_EQ(FRAMESIZE688, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_RV30DEC_FUNCTION_0019
 * @tc.name      : Decode Rv30 surface with NV21 pixel format
 * @tc.desc      : function test
 */
HWTEST_F(Rv30decFuncNdkTest, VIDEO_RV30DEC_FUNCTION_0019, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->getFormat(INP_DIR_2);
    vDecSample->SF_OUTPUT = true;
    vDecSample->outputYuvSurface = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV21;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameRv30.c_str()));
    vDecSample->WaitForEOS();
    ASSERT_EQ(FRAMESIZE688, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_RV30DEC_FUNCTION_0020
 * @tc.name      : Surface model change in normal state
 * @tc.desc      : function test
 */
HWTEST_F(Rv30decFuncNdkTest, VIDEO_RV30DEC_FUNCTION_0020, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();

    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->getFormat(INP_DIR_2);
    vDecSample->SURFACE_OUTPUT = true;
    vDecSample->autoSwitchSurface = true;
    vDecSample->AFTER_EOS_DESTORY_CODEC = false;
    vDecSample->sleepOnFPS = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameRv30.c_str()));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->Reset());
    ASSERT_EQ(AV_ERR_INVALID_STATE, vDecSample->SwitchSurface());
    ASSERT_EQ(AV_ERR_OK, vDecSample->Release());
}

/**
 * @tc.number    : VIDEO_RV30DEC_FUNCTION_0021
 * @tc.name      : Surface model change in flushed state
 * @tc.desc      : function test
 */
HWTEST_F(Rv30decFuncNdkTest, VIDEO_RV30DEC_FUNCTION_0021, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();

    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->getFormat(INP_DIR_2);
    vDecSample->SURFACE_OUTPUT = true;
    vDecSample->autoSwitchSurface = true;
    vDecSample->sleepOnFPS = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameRv30.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->Flush());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
    ASSERT_EQ(AV_ERR_OK, vDecSample->Stop());
}

/**
 * @tc.number    : VIDEO_RV30DEC_FUNCTION_0022
 * @tc.name      : Surface model change in running state
 * @tc.desc      : function test
 */
HWTEST_F(Rv30decFuncNdkTest, VIDEO_RV30DEC_FUNCTION_0022, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();

    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->getFormat(INP_DIR_2);
    vDecSample->SURFACE_OUTPUT = false;
    vDecSample->autoSwitchSurface = false;
    vDecSample->CreateSurface();
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameRv30.c_str()));
    ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, vDecSample->SwitchSurface());
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_RV30DEC_FUNCTION_0023
 * @tc.name      : Repeat call setSurface quickly
 * @tc.desc      : function test
 */
HWTEST_F(Rv30decFuncNdkTest, VIDEO_RV30DEC_FUNCTION_0023, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();

    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->getFormat(INP_DIR_2);
    vDecSample->SURFACE_OUTPUT = true;
    vDecSample->autoSwitchSurface = true;
    vDecSample->sleepOnFPS = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameRv30.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
    vDecSample->WaitForEOS();
}

/**
 * @tc.number    : VIDEO_RV30DEC_FUNCTION_0024
 * @tc.name      : Surface model change in flush to running state
 * @tc.desc      : function test
 */
HWTEST_F(Rv30decFuncNdkTest, VIDEO_RV30DEC_FUNCTION_0024, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();

    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->getFormat(INP_DIR_2);
    vDecSample->SURFACE_OUTPUT = true;
    vDecSample->autoSwitchSurface = true;
    vDecSample->AFTER_EOS_DESTORY_CODEC = false;
    vDecSample->sleepOnFPS = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameRv30.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->Flush());
    ASSERT_EQ(AV_ERR_OK, vDecSample->Start());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
}

/**
 * @tc.number    : VIDEO_RV30DEC_FUNCTION_0025
 * @tc.name      : Surface model change in decoder finish to End-of-Stream state
 * @tc.desc      : function test
 */
HWTEST_F(Rv30decFuncNdkTest, VIDEO_RV30DEC_FUNCTION_0025, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();

    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->getFormat(INP_DIR_2);
    vDecSample->SURFACE_OUTPUT = true;
    vDecSample->autoSwitchSurface = true;
    vDecSample->AFTER_EOS_DESTORY_CODEC = false;
    vDecSample->sleepOnFPS = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameRv30.c_str()));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
}

/**
 * @tc.number    : VIDEO_RV30DEC_FUNCTION_0026
 * @tc.name      : Buffer model change in decoder finish to End-of-Stream state
 * @tc.desc      : function test
 */
HWTEST_F(Rv30decFuncNdkTest, VIDEO_RV30DEC_FUNCTION_0026, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();

    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->getFormat(INP_DIR_2);
    vDecSample->SURFACE_OUTPUT = false;
    vDecSample->autoSwitchSurface = false;
    vDecSample->CreateSurface();
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameRv30.c_str()));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, vDecSample->SwitchSurface());
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_RV30DEC_FUNCTION_0027
 * @tc.name      : Buffer model change in running to flushed state
 * @tc.desc      : function test
 */
HWTEST_F(Rv30decFuncNdkTest, VIDEO_RV30DEC_FUNCTION_0027, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();

    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->getFormat(INP_DIR_2);
    vDecSample->SURFACE_OUTPUT = false;
    vDecSample->autoSwitchSurface = false;
    vDecSample->CreateSurface();
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameRv30.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->Flush());
    ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, vDecSample->SwitchSurface());
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_RV30DEC_FUNCTION_0028
 * @tc.name      : Buffer model change in flushed to running state
 * @tc.desc      : function test
 */
HWTEST_F(Rv30decFuncNdkTest, VIDEO_RV30DEC_FUNCTION_0028, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();

    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->getFormat(INP_DIR_2);
    vDecSample->SURFACE_OUTPUT = false;
    vDecSample->autoSwitchSurface = false;
    vDecSample->CreateSurface();
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameRv30.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->Flush());
    ASSERT_EQ(AV_ERR_OK, vDecSample->Start());
    ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, vDecSample->SwitchSurface());
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_RV30DEC_FUNCTION_0029
 * @tc.name      : Buffer model change in normal state
 * @tc.desc      : function test
 */
HWTEST_F(Rv30decFuncNdkTest, VIDEO_RV30DEC_FUNCTION_0029, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();

    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->SURFACE_OUTPUT = false;
    vDecSample->autoSwitchSurface = false;
    vDecSample->CreateSurface();
    vDecSample->SURFACE_OUTPUT = false;
    vDecSample->autoSwitchSurface = false;
    vDecSample->CreateSurface();

    vDecSample->getFormat(INP_DIR_2);
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecNameRv30.c_str()));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->Reset());
    ASSERT_EQ(AV_ERR_INVALID_STATE, vDecSample->SwitchSurface());
    ASSERT_EQ(AV_ERR_OK, vDecSample->Release());
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_RV30DEC_FUNCTION_0030
 * @tc.name      : Surface model change in config state
 * @tc.desc      : function test
 */
HWTEST_F(Rv30decFuncNdkTest, VIDEO_RV30DEC_FUNCTION_0030, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();

    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->getFormat(INP_DIR_2);
    vDecSample->SURFACE_OUTPUT = false;
    vDecSample->autoSwitchSurface = false;
    vDecSample->CreateSurface();
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameRv30.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_RV30DEC_FUNCTION_0031
 * @tc.name      : Surface model change in create state
 * @tc.desc      : function test
 */
HWTEST_F(Rv30decFuncNdkTest, VIDEO_RV30DEC_FUNCTION_0031, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();

    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->getFormat(INP_DIR_2);
    vDecSample->SURFACE_OUTPUT = false;
    vDecSample->autoSwitchSurface = false;
    vDecSample->CreateSurface();
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameRv30.c_str()));;
    ASSERT_EQ(AV_ERR_INVALID_STATE, vDecSample->SwitchSurface());
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_RV30DEC_FUNCTION_0032
 * @tc.name      : Two object repeat call setSurface quickly
 * @tc.desc      : function test
 */
HWTEST_F(Rv30decFuncNdkTest, VIDEO_RV30DEC_FUNCTION_0032, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->getFormat(INP_DIR_2);
    vDecSample->SURFACE_OUTPUT = true;
    vDecSample->autoSwitchSurface = true;
    vDecSample->sleepOnFPS = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameRv30.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->SwitchSurface());
    vDecSample->WaitForEOS();
    vDecSample->ReleaseInFile();

    auto vDecSample_1 = make_shared<VDecAPI11Sample>();
    vDecSample_1->INP_DIR = INP_DIR_1;
    vDecSample_1->getFormat(INP_DIR_2);
    vDecSample_1->SURFACE_OUTPUT = true;
    vDecSample_1->autoSwitchSurface = true;
    vDecSample_1->sleepOnFPS = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample_1->RunVideoDec_Surface(g_codecNameRv30.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample_1->SwitchSurface());
    vDecSample_1->WaitForEOS();
    vDecSample_1->ReleaseInFile();
}

/**
 * @tc.number    : VIDEO_RV30DEC_FUNCTION_0033
 * @tc.name      : Repeat call setSurface quickly 2 times
 * @tc.desc      : function test
 */
HWTEST_F(Rv30decFuncNdkTest, VIDEO_RV30DEC_FUNCTION_0033, TestSize.Level0)
{
    for (int i = 0; i < 2; i++) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->INP_DIR = INP_DIR_1;
        vDecSample->getFormat(INP_DIR_2);
        vDecSample->SURFACE_OUTPUT = true;
        vDecSample->autoSwitchSurface = true;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(g_codecNameRv30.c_str()));
        ASSERT_EQ(AV_ERR_OK, vDecSample->RepeatCallSetSurface());
        vDecSample->WaitForEOS();
        vDecSample->ReleaseInFile();
    }
}

/**
 * @tc.number    : VIDEO_RV30DEC_FUNCTION_0038
 * @tc.name      : Decode Rv30 buffer and encode to H.264
 * @tc.desc      : function test
 */
HWTEST_F(Rv30decFuncNdkTest, VIDEO_RV30DEC_FUNCTION_0038, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->getFormat(INP_DIR_2);
    vDecSample->NocaleHash = true;
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameRv30.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE688, vDecSample->outFrameCount);

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
 * @tc.number    : VIDEO_RV30DEC_FUNCTION_0047
 * @tc.name      : Rv30 synchronous software decoding with NV12 output
 * @tc.desc      : function test
 */
HWTEST_F(Rv30decFuncNdkTest, VIDEO_RV30DEC_FUNCTION_0047, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->getFormat(INP_DIR_2);
    vDecSample->enbleSyncMode = 1;
    vDecSample->NocaleHash = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameRv30.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartSyncVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE688, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_RV30DEC_FUNCTION_0048
 * @tc.name      : Rv30 synchronous software decoding with NV21 output
 * @tc.desc      : function test
 */
HWTEST_F(Rv30decFuncNdkTest, VIDEO_RV30DEC_FUNCTION_0048, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->getFormat(INP_DIR_2);
    vDecSample->enbleSyncMode = 1;
    vDecSample->NocaleHash = true;
    vDecSample->defualtPixelFormat = AV_PIXEL_FORMAT_NV21;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameRv30.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartSyncVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE688, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_RV30DEC_FUNCTION_0049
 * @tc.name      : Rv30 synchronous software decoding with surface output
 * @tc.desc      : function test
 */
HWTEST_F(Rv30decFuncNdkTest, VIDEO_RV30DEC_FUNCTION_0049, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->getFormat(INP_DIR_2);
    vDecSample->SF_OUTPUT = true;
    vDecSample->enbleSyncMode = 1;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameRv30.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->DecodeSetSurface());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartSyncVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(FRAMESIZE688, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_RV30DEC_FUNCTION_0050
 * @tc.name      : Decode Rv30 buffer No PixelFormat
 * @tc.desc      : function test
 */
HWTEST_F(Rv30decFuncNdkTest, VIDEO_RV30DEC_FUNCTION_0050, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->getFormat(INP_DIR_2);
    vDecSample->outputYuvFlag = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameRv30.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoderNoPixelFormat());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(FRAMESIZE688, vDecSample->outFrameCount);
}

/**
 * @tc.number    : VIDEO_RV30DEC_FUNCTION_0051
 * @tc.name      : Decode Rv30 buffer graph pixel format
 * @tc.desc      : function test
 */
HWTEST_F(Rv30decFuncNdkTest, VIDEO_RV30DEC_FUNCTION_0051, TestSize.Level0)
{
    auto vDecSample = make_shared<VDecAPI11Sample>();
    int32_t pixfmt[4] = {28, 24, 25, 12};
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->getFormat(INP_DIR_2);
    vDecSample->outputYuvFlag = true;
    vDecSample->NocaleHash = true;
    vDecSample->isGetVideoSupportedPixelFormats = true;
    vDecSample->isGetFormatKey = true;
    vDecSample->avcodecMimeType = OH_AVCODEC_MIMETYPE_VIDEO_RV30;
    vDecSample->isEncoder = false;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecNameRv30.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoderReadStream());
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    ASSERT_EQ(4, vDecSample->pixlFormatNum);
    for(int i = 0; i < vDecSample->pixlFormatNum; i++) {
        ASSERT_EQ(vDecSample->pixlFormats[i], pixfmt[i]);
    }
    ASSERT_EQ(FRAMESIZE688, vDecSample->outFrameCount);
}
} // namespace