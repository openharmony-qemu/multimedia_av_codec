/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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
#include "native_avcodec_videoencoder.h"
#include "native_averrors.h"
#include "videoenc_ndk_sample.h"
#include "native_avcodec_base.h"
#include "avcodec_codec_name.h"
#include "native_avcapability.h"

namespace {
OH_AVCodec *venc_ = NULL;
OH_AVCapability *cap = nullptr;
OH_AVCapability *cap_hevc = nullptr;
const char *g_codecMime = "video/avc";
const char *g_codecMime_HEVC = "video/hevc";
constexpr uint32_t CODEC_NAME_SIZE = 128;
char g_codecName[CODEC_NAME_SIZE] = {};
char g_codecNameHEVC[CODEC_NAME_SIZE] = {};
const char *INP_DIR_1080 = "/data/test/media/1920_1080_nv.yuv";
const char *INP_DIR_720 = "/data/test/media/1280_720_nv.yuv";
constexpr uint32_t SECOND = 1000;
constexpr uint32_t DEFAULT_WIDTH = 1920;
constexpr uint32_t DEFAULT_HEIGHT = 1080;
} // namespace
namespace OHOS {
namespace Media {
class EncoderFuncNdkTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp() override;
    void TearDown() override;
    void InputFunc();
    void OutputFunc();
    void Release();
    int32_t Stop();
};
} // namespace Media
} // namespace OHOS

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;

void EncoderFuncNdkTest::SetUpTestCase()
{
    cap = OH_AVCodec_GetCapabilityByCategory(g_codecMime, true, HARDWARE);
    const char *TMP_CODEC_NAME = OH_AVCapability_GetName(cap);
    if (memcpy_s(g_codecName, sizeof(g_codecName), TMP_CODEC_NAME, strlen(TMP_CODEC_NAME)) != 0)
        cout << "memcpy failed" << endl;
    cout << "codecname: " << g_codecName << endl;
    cap_hevc = OH_AVCodec_GetCapabilityByCategory(g_codecMime_HEVC, true, HARDWARE);
    const char *TMP_CODEC_NAME_HEVC = OH_AVCapability_GetName(cap_hevc);
    if (memcpy_s(g_codecNameHEVC, sizeof(g_codecNameHEVC), TMP_CODEC_NAME_HEVC, strlen(TMP_CODEC_NAME_HEVC)) != 0)
        cout << "memcpy failed" << endl;
    cout << "codecname_hevc: " << g_codecNameHEVC << endl;
}
void EncoderFuncNdkTest::TearDownTestCase() {}
void EncoderFuncNdkTest::SetUp() {}
void EncoderFuncNdkTest::TearDown()
{
    if (venc_ != NULL) {
        OH_VideoEncoder_Destroy(venc_);
        venc_ = nullptr;
    }
}
namespace {
/**
 * @tc.number    : VIDEO_ENCODE_FUNCTION_0100
 * @tc.name      : create by mime
 * @tc.desc      : function test
 */
HWTEST_F(EncoderFuncNdkTest, VIDEO_ENCODE_FUNCTION_0100, TestSize.Level1)
{
    venc_ = OH_VideoEncoder_CreateByMime(g_codecMime);
    ASSERT_NE(nullptr, venc_);
}

/**
 * @tc.number    : VIDEO_ENCODE_FUNCTION_0200
 * @tc.name      : create by name
 * @tc.desc      : function test
 */
HWTEST_F(EncoderFuncNdkTest, VIDEO_ENCODE_FUNCTION_0200, TestSize.Level1)
{
    venc_ = OH_VideoEncoder_CreateByName(g_codecName);
    ASSERT_NE(nullptr, venc_);
}

/**
 * @tc.number    : VIDEO_ENCODE_FUNCTION_0300
 * @tc.name      : create no exist encoder
 * @tc.desc      : function test
 */
HWTEST_F(EncoderFuncNdkTest, VIDEO_ENCODE_FUNCTION_0300, TestSize.Level1)
{
    venc_ = OH_VideoEncoder_CreateByName("aabbccdd");
    ASSERT_EQ(nullptr, venc_);
}

/**
 * @tc.number    : VIDEO_ENCODE_FUNCTION_0400
 * @tc.name      : test encode buffer
 * @tc.desc      : function test
 */
HWTEST_F(EncoderFuncNdkTest, VIDEO_ENCODE_FUNCTION_0400, TestSize.Level1)
{
    auto vEncSample = make_unique<VEncNdkSample>();
    vEncSample->INP_DIR = INP_DIR_1080;
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
 * @tc.number    : VIDEO_ENCODE_FUNCTION_0500
 * @tc.name      : test encode surface
 * @tc.desc      : function test
 */
HWTEST_F(EncoderFuncNdkTest, VIDEO_ENCODE_FUNCTION_0500, TestSize.Level1)
{
    auto vEncSample = make_unique<VEncNdkSample>();
    vEncSample->INP_DIR = INP_DIR_1080;
    vEncSample->DEFAULT_WIDTH = 1920;
    vEncSample->DEFAULT_HEIGHT = 1080;
    vEncSample->DEFAULT_FRAME_RATE = 30;
    vEncSample->SURFACE_INPUT = true;
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

/**
 * @tc.number    : VIDEO_ENCODE_FUNCTION_0600
 * @tc.name      : set force IDR when encoding
 * @tc.desc      : function test
 */
HWTEST_F(EncoderFuncNdkTest, VIDEO_ENCODE_FUNCTION_0600, TestSize.Level1)
{
    auto vEncSample = make_unique<VEncNdkSample>();
    vEncSample->INP_DIR = INP_DIR_1080;
    vEncSample->DEFAULT_WIDTH = 1920;
    vEncSample->DEFAULT_HEIGHT = 1080;
    vEncSample->DEFAULT_FRAME_RATE = 30;
    vEncSample->enableForceIDR = true;
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

/**
 * @tc.number    : VIDEO_ENCODE_FUNCTION_0700
 * @tc.name      : set color format
 * @tc.desc      : function test
 */
HWTEST_F(EncoderFuncNdkTest, VIDEO_ENCODE_FUNCTION_0700, TestSize.Level1)
{
    venc_ = OH_VideoEncoder_CreateByMime(g_codecMime);
    ASSERT_NE(nullptr, venc_);
    OH_AVFormat *format = OH_AVFormat_Create();
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(format, OH_MD_KEY_RANGE_FLAG, 1));
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(format, OH_MD_KEY_COLOR_PRIMARIES, OH_ColorPrimary::COLOR_PRIMARY_BT709));
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(format, OH_MD_KEY_TRANSFER_CHARACTERISTICS,
                                            OH_TransferCharacteristic::TRANSFER_CHARACTERISTIC_LINEAR));
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(format, OH_MD_KEY_MATRIX_COEFFICIENTS,
                                            OH_MatrixCoefficient::MATRIX_COEFFICIENT_YCGCO));
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_YUVI420);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(venc_, format));
    OH_AVFormat_Destroy(format);
}

/**
 * @tc.number    : VIDEO_ENCODE_FUNCTION_0800
 * @tc.name      : set key frame interval
 * @tc.desc      : function test
 */
HWTEST_F(EncoderFuncNdkTest, VIDEO_ENCODE_FUNCTION_0800, TestSize.Level1)
{
    venc_ = OH_VideoEncoder_CreateByMime(g_codecMime);
    ASSERT_NE(nullptr, venc_);
    OH_AVFormat *format = OH_AVFormat_Create();

    ASSERT_EQ(true, OH_AVFormat_SetIntValue(format, OH_MD_KEY_I_FRAME_INTERVAL, SECOND));
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_YUVI420);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(venc_, format));
    OH_AVFormat_Destroy(format);
}

/**
 * @tc.number    : VIDEO_ENCODE_FUNCTION_0900
 * @tc.name      : set profile level
 * @tc.desc      : function test
 */
HWTEST_F(EncoderFuncNdkTest, VIDEO_ENCODE_FUNCTION_0900, TestSize.Level1)
{
    venc_ = OH_VideoEncoder_CreateByMime(g_codecMime);
    ASSERT_NE(nullptr, venc_);
    OH_AVFormat *format = OH_AVFormat_Create();

    ASSERT_EQ(true, OH_AVFormat_SetIntValue(format, OH_MD_KEY_PROFILE, AVC_PROFILE_BASELINE));
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_YUVI420);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(venc_, format));
    OH_AVFormat_Destroy(format);
}

/**
 * @tc.number    : VIDEO_ENCODE_FUNCTION_1000
 * @tc.name      : set bitrate mode
 * @tc.desc      : function test
 */
HWTEST_F(EncoderFuncNdkTest, VIDEO_ENCODE_FUNCTION_1000, TestSize.Level1)
{
    venc_ = OH_VideoEncoder_CreateByMime(g_codecMime);
    ASSERT_NE(nullptr, venc_);
    OH_AVFormat *format = OH_AVFormat_Create();

    ASSERT_EQ(true, OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODE_BITRATE_MODE, CBR));
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_YUVI420);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(venc_, format));
    OH_AVFormat_Destroy(format);
}

/**
 * @tc.number    : VIDEO_ENCODE_FUNCTION_1100
 * @tc.name      : set bitrate value
 * @tc.desc      : function test
 */
HWTEST_F(EncoderFuncNdkTest, VIDEO_ENCODE_FUNCTION_1100, TestSize.Level1)
{
    venc_ = OH_VideoEncoder_CreateByMime(g_codecMime);
    ASSERT_NE(nullptr, venc_);
    OH_AVFormat *format = OH_AVFormat_Create();
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODE_BITRATE_MODE, CBR));
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITRATE, 1000));
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_YUVI420);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(venc_, format));
    OH_AVFormat_Destroy(format);
}

/**
 * @tc.number    : VIDEO_ENCODE_FUNCTION_1400
 * @tc.name      : set framerate
 * @tc.desc      : function test
 */
HWTEST_F(EncoderFuncNdkTest, VIDEO_ENCODE_FUNCTION_1400, TestSize.Level1)
{
    venc_ = OH_VideoEncoder_CreateByMime(g_codecMime);
    ASSERT_NE(nullptr, venc_);
    OH_AVFormat *format = OH_AVFormat_Create();

    ASSERT_EQ(true, OH_AVFormat_SetIntValue(format, OH_MD_KEY_FRAME_RATE, 60));
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_YUVI420);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(venc_, format));
    OH_AVFormat_Destroy(format);
}

/**
 * @tc.number    : VIDEO_ENCODE_FUNCTION_1600
 * @tc.name      : set quality
 * @tc.desc      : function test
 */
HWTEST_F(EncoderFuncNdkTest, VIDEO_ENCODE_FUNCTION_1600, TestSize.Level1)
{
    venc_ = OH_VideoEncoder_CreateByMime(g_codecMime);
    ASSERT_NE(nullptr, venc_);
    OH_AVFormat *format = OH_AVFormat_Create();
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODE_BITRATE_MODE, CQ));
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(format, OH_MD_KEY_QUALITY, 60));
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_YUVI420);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(venc_, format));
    OH_AVFormat_Destroy(format);
}

/**
 * @tc.number    : VIDEO_ENCODE_FUNCTION_1700
 * @tc.name      : input frame after EOS
 * @tc.desc      : function test
 */
HWTEST_F(EncoderFuncNdkTest, VIDEO_ENCODE_FUNCTION_1700, TestSize.Level1)
{
    auto vEncSample = make_unique<VEncNdkSample>();
    vEncSample->INP_DIR = INP_DIR_1080;
    vEncSample->DEFAULT_WIDTH = 1920;
    vEncSample->DEFAULT_HEIGHT = 1080;
    vEncSample->DEFAULT_FRAME_RATE = 30;
    vEncSample->enable_random_eos = true;
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());

    vEncSample->WaitForEOS();

    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

/**
 * @tc.number    : VIDEO_ENCODE_FUNCTION_1800
 * @tc.name      : encode h265 buffer
 * @tc.desc      : function test
 */
HWTEST_F(EncoderFuncNdkTest, VIDEO_ENCODE_FUNCTION_1800, TestSize.Level1)
{
    auto vEncSample = make_unique<VEncNdkSample>();
    vEncSample->INP_DIR = INP_DIR_720;
    vEncSample->DEFAULT_WIDTH = 1280;
    vEncSample->DEFAULT_HEIGHT = 720;
    vEncSample->DEFAULT_FRAME_RATE = 30;
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());

    vEncSample->WaitForEOS();

    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

/**
 * @tc.number    : VIDEO_ENCODE_FUNCTION_1900
 * @tc.name      : encode h265 surface
 * @tc.desc      : function test
 */
HWTEST_F(EncoderFuncNdkTest, VIDEO_ENCODE_FUNCTION_1900, TestSize.Level1)
{
    auto vEncSample = make_unique<VEncNdkSample>();
    vEncSample->INP_DIR = INP_DIR_1080;
    vEncSample->DEFAULT_WIDTH = 1920;
    vEncSample->DEFAULT_HEIGHT = 1080;
    vEncSample->DEFAULT_FRAME_RATE = 30;
    vEncSample->SURFACE_INPUT = true;
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}
} // namespace