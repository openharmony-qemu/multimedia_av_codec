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
#include <limits>
#include "gtest/gtest.h"
#include "native_avcodec_videoencoder.h"
#include "native_averrors.h"
#include "videoenc_api11_sample.h"
#include "native_avcodec_base.h"
#include "avcodec_codec_name.h"
#include "native_avcapability.h"
namespace {
OH_AVCodec *venc_ = NULL;
OH_AVCapability *cap = nullptr;
constexpr uint32_t CODEC_NAME_SIZE = 128;
char g_codecName[CODEC_NAME_SIZE] = {};
const char *INP_DIR_720 = "/data/test/media/1280_720_nv.yuv";
constexpr uint32_t DEFAULT_WIDTH = 1280;
constexpr uint32_t DEFAULT_HEIGHT = 720;
} // namespace
namespace OHOS {
namespace Media {
class AvcSwEncSetParamNdkTest : public testing::Test {
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

void AvcSwEncSetParamNdkTest::SetUpTestCase()
{
    cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_AVC, true, SOFTWARE);
    const char *tmpCodecName = OH_AVCapability_GetName(cap);
    if (memcpy_s(g_codecName, sizeof(g_codecName), tmpCodecName, strlen(tmpCodecName)) != 0)
        cout << "memcpy failed" << endl;
    cout << "codecname: " << g_codecName << endl;
}
void AvcSwEncSetParamNdkTest::TearDownTestCase() {}
void AvcSwEncSetParamNdkTest::SetUp() {}
void AvcSwEncSetParamNdkTest::TearDown()
{
    if (venc_ != NULL) {
        OH_VideoEncoder_Destroy(venc_);
        venc_ = nullptr;
    }
}
namespace {
/**
 * @tc.number    : RESET_BITRATE_001
 * @tc.name      : reset bitrate use illegal value
 * @tc.desc      : function test
 */
HWTEST_F(AvcSwEncSetParamNdkTest, RESET_BITRATE_001, TestSize.Level1)
{
    if (strlen(g_codecName) == 0) {
        return;
    }
    auto vEncSample = make_unique<VEncAPI11Sample>();
    vEncSample->INP_DIR = INP_DIR_720;
    vEncSample->DEFAULT_WIDTH = DEFAULT_WIDTH;
    vEncSample->DEFAULT_HEIGHT = DEFAULT_HEIGHT;
    vEncSample->DEFAULT_FRAME_RATE = 30;
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    OH_AVFormat *format = OH_AVFormat_Create();
    (void)OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, -1);
    EXPECT_EQ(AV_ERR_INVALID_VAL, vEncSample->SetParameter(format));
    (void)OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, LONG_MAX);
    EXPECT_EQ(AV_ERR_INVALID_VAL, vEncSample->SetParameter(format));
    OH_AVFormat_Destroy(format);
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}


/**
 * @tc.number    : RESET_BITRATE_002
 * @tc.name      : reset bitrate in CBR mode ,gop size -1
 * @tc.desc      : function test
 */
HWTEST_F(AvcSwEncSetParamNdkTest, RESET_BITRATE_002, TestSize.Level0)
{
    if (strlen(g_codecName) == 0) {
        return;
    }
    auto vEncSample = make_unique<VEncAPI11Sample>();
    vEncSample->INP_DIR = INP_DIR_720;
    vEncSample->DEFAULT_WIDTH = DEFAULT_WIDTH;
    vEncSample->DEFAULT_HEIGHT = DEFAULT_HEIGHT;
    vEncSample->DEFAULT_FRAME_RATE = 30;
    vEncSample->DEFAULT_BITRATE_MODE = CBR;
    vEncSample->DEFAULT_KEY_FRAME_INTERVAL = -1;
    vEncSample->enableAutoSwitchParam = true;
    vEncSample->needResetBitrate = true;
    vEncSample->OUT_DIR = "/data/test/media/cbr_-1_.h264";
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

/**
 * @tc.number    : RESET_BITRATE_003
 * @tc.name      : reset bitrate in CBR mode ,gop size 0
 * @tc.desc      : function test
 */
HWTEST_F(AvcSwEncSetParamNdkTest, RESET_BITRATE_003, TestSize.Level0)
{
    if (strlen(g_codecName) == 0) {
        return;
    }
    auto vEncSample = make_unique<VEncAPI11Sample>();
    vEncSample->INP_DIR = INP_DIR_720;
    vEncSample->DEFAULT_WIDTH = DEFAULT_WIDTH;
    vEncSample->DEFAULT_HEIGHT = DEFAULT_HEIGHT;
    vEncSample->DEFAULT_FRAME_RATE = 30;
    vEncSample->DEFAULT_BITRATE_MODE = CBR;
    vEncSample->DEFAULT_KEY_FRAME_INTERVAL = 0;
    vEncSample->enableAutoSwitchParam = true;
    vEncSample->needResetBitrate = true;
    vEncSample->OUT_DIR = "/data/test/media/cbr_0_.h264";
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

/**
 * @tc.number    : RESET_BITRATE_004
 * @tc.name      : reset bitrate in CBR mode ,gop size 1s
 * @tc.desc      : function test
 */
HWTEST_F(AvcSwEncSetParamNdkTest, RESET_BITRATE_004, TestSize.Level0)
{
    if (strlen(g_codecName) == 0) {
        return;
    }
    auto vEncSample = make_unique<VEncAPI11Sample>();
    vEncSample->INP_DIR = INP_DIR_720;
    vEncSample->DEFAULT_WIDTH = DEFAULT_WIDTH;
    vEncSample->DEFAULT_HEIGHT = DEFAULT_HEIGHT;
    vEncSample->DEFAULT_FRAME_RATE = 30;
    vEncSample->DEFAULT_BITRATE_MODE = CBR;
    vEncSample->enableAutoSwitchParam = true;
    vEncSample->needResetBitrate = true;
    vEncSample->OUT_DIR = "/data/test/media/cbr_1s_.h264";
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}


/**
 * @tc.number    : RESET_BITRATE_005
 * @tc.name      : reset bitrate in VBR mode
 * @tc.desc      : function test
 */
HWTEST_F(AvcSwEncSetParamNdkTest, RESET_BITRATE_005, TestSize.Level0)
{
    if (strlen(g_codecName) == 0) {
        return;
    }
    auto vEncSample = make_unique<VEncAPI11Sample>();
    vEncSample->INP_DIR = INP_DIR_720;
    vEncSample->DEFAULT_WIDTH = DEFAULT_WIDTH;
    vEncSample->DEFAULT_HEIGHT = DEFAULT_HEIGHT;
    vEncSample->DEFAULT_FRAME_RATE = 30;
    vEncSample->DEFAULT_BITRATE_MODE = VBR;
    vEncSample->enableAutoSwitchParam = true;
    vEncSample->needResetBitrate = true;
    vEncSample->OUT_DIR = "/data/test/media/vbr_1s_.h264";
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

/**
 * @tc.number    : SET_PROFILE_001
 * @tc.name      : set profile main
 * @tc.desc      : function test
 */
HWTEST_F(AvcSwEncSetParamNdkTest, SET_PROFILE_001, TestSize.Level1)
{
    if (strlen(g_codecName) == 0) {
        return;
    }
    auto vEncSample = make_unique<VEncAPI11Sample>();
    vEncSample->INP_DIR = INP_DIR_720;
    vEncSample->DEFAULT_WIDTH = DEFAULT_WIDTH;
    vEncSample->DEFAULT_HEIGHT = DEFAULT_HEIGHT;
    vEncSample->DEFAULT_BITRATE_MODE = VBR;
    vEncSample->DEFAULT_PROFILE = AVC_PROFILE_MAIN;
    vEncSample->OUT_DIR = "/data/test/media/1280_720_30_10Mb.h264";
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}
/**
 * @tc.number    : SET_PROFILE_002
 * @tc.name      : set profile base
 * @tc.desc      : function test
 */
HWTEST_F(AvcSwEncSetParamNdkTest, SET_PROFILE_002, TestSize.Level1)
{
    if (strlen(g_codecName) == 0) {
        return;
    }
    auto vEncSample = make_unique<VEncAPI11Sample>();
    vEncSample->INP_DIR = INP_DIR_720;
    vEncSample->DEFAULT_WIDTH = DEFAULT_WIDTH;
    vEncSample->DEFAULT_HEIGHT = DEFAULT_HEIGHT;
    vEncSample->DEFAULT_BITRATE_MODE = VBR;
    vEncSample->DEFAULT_PROFILE = AVC_PROFILE_BASELINE;
    vEncSample->OUT_DIR = "/data/test/media/1280_720_30_10Mb.h264";
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

/**
 * @tc.number    : SET_RANGE_FLAG_001
 * @tc.name      : set range flag true
 * @tc.desc      : function test
 */
HWTEST_F(AvcSwEncSetParamNdkTest, SET_RANGE_FLAG_001, TestSize.Level1)
{
    if (strlen(g_codecName) == 0) {
        return;
    }
    auto vEncSample = make_unique<VEncAPI11Sample>();
    vEncSample->INP_DIR = INP_DIR_720;
    vEncSample->DEFAULT_WIDTH = DEFAULT_WIDTH;
    vEncSample->DEFAULT_HEIGHT = DEFAULT_HEIGHT;
    vEncSample->DEFAULT_BITRATE_MODE = VBR;
    vEncSample->DEFAULT_RANGE_FLAG = 1;
    vEncSample->OUT_DIR = "/data/test/media/vbr_fullrange.h264";
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

/**
 * @tc.number    : SET_COLORSPACE_001
 * @tc.name      : set color space parameter
 * @tc.desc      : function test
 */
HWTEST_F(AvcSwEncSetParamNdkTest, SET_COLORSPACE_001, TestSize.Level1)
{
    if (strlen(g_codecName) == 0) {
        return;
    }
    auto vEncSample = make_unique<VEncAPI11Sample>();
    vEncSample->INP_DIR = INP_DIR_720;
    vEncSample->DEFAULT_WIDTH = DEFAULT_WIDTH;
    vEncSample->DEFAULT_HEIGHT = DEFAULT_HEIGHT;
    vEncSample->DEFAULT_BITRATE_MODE = VBR;
    vEncSample->DEFAULT_RANGE_FLAG = 1;
    vEncSample->enableColorSpaceParams = true;
    vEncSample->OUT_DIR = "/data/test/media/vbr_cs.h264";
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

/**
 * @tc.number    : SET_FORCE_IDR_001
 * @tc.name      : request i frame
 * @tc.desc      : function test
 */
HWTEST_F(AvcSwEncSetParamNdkTest, SET_FORCE_IDR_001, TestSize.Level1)
{
    if (strlen(g_codecName) == 0) {
        return;
    }
    auto vEncSample = make_unique<VEncAPI11Sample>();
    vEncSample->INP_DIR = INP_DIR_720;
    vEncSample->DEFAULT_WIDTH = DEFAULT_WIDTH;
    vEncSample->DEFAULT_HEIGHT = DEFAULT_HEIGHT;
    vEncSample->DEFAULT_BITRATE_MODE = VBR;
    vEncSample->DEFAULT_RANGE_FLAG = 1;
    vEncSample->enableForceIDR = true;
    vEncSample->OUT_DIR = "/data/test/media/vbr_i.h264";
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}
/**
 * @tc.number    : COLORSPACE_CONFIG_001
 * @tc.name      : COLORSPACE
 * @tc.desc      : function test
 */
HWTEST_F(AvcSwEncSetParamNdkTest, COLORSPACE_CONFIG_001, TestSize.Level1)
{
    if (strlen(g_codecName) == 0) {
        return;
    }
    auto vEncSample = make_unique<VEncAPI11Sample>();
    vEncSample->INP_DIR = INP_DIR_720;
    vEncSample->DEFAULT_WIDTH = DEFAULT_WIDTH;
    vEncSample->DEFAULT_HEIGHT = DEFAULT_HEIGHT;
    vEncSample->DEFAULT_BITRATE_MODE = VBR;
    vEncSample->DEFAULT_RANGE_FLAG = 1;
    vEncSample->enableColorSpaceParams = true;
    vEncSample->DEFAULT_COLOR_PRIMARIES = 100;
    vEncSample->DEFAULT_TRANSFER_CHARACTERISTICS = 10000;
    vEncSample->DEFAULT_MATRIX_COEFFICIENTS = 10000;
    vEncSample->OUT_DIR = "/data/test/media/vbr_i.h264";
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_INVALID_VAL, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_INVALID_STATE, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}
/**
 * @tc.number    : COLORSPACE_CONFIG_002
 * @tc.name      : COLORSPACE 264
 * @tc.desc      : function test
 */
HWTEST_F(AvcSwEncSetParamNdkTest, COLORSPACE_CONFIG_002, TestSize.Level1)
{
    if (strlen(g_codecName) == 0) {
        return;
    }
    auto vEncSample = make_unique<VEncAPI11Sample>();
    vEncSample->INP_DIR = INP_DIR_720;
    vEncSample->DEFAULT_WIDTH = DEFAULT_WIDTH;
    vEncSample->DEFAULT_HEIGHT = DEFAULT_HEIGHT;
    vEncSample->DEFAULT_BITRATE_MODE = VBR;
    vEncSample->DEFAULT_RANGE_FLAG = 1;
    vEncSample->enableColorSpaceParams = true;
    vEncSample->DEFAULT_COLOR_PRIMARIES = COLOR_PRIMARY_BT709;
    vEncSample->DEFAULT_TRANSFER_CHARACTERISTICS = TRANSFER_CHARACTERISTIC_BT709;
    vEncSample->DEFAULT_MATRIX_COEFFICIENTS = MATRIX_COEFFICIENT_BT709;
    vEncSample->OUT_DIR = "/data/test/media/bt_709_h264.h264";
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}
/**
 * @tc.number    : FRAMENUM_JUDGMENT_001
 * @tc.name      : Increase frame rate judgment
 * @tc.desc      : function test
 */
HWTEST_F(AvcSwEncSetParamNdkTest, FRAMENUM_JUDGMENT_001, TestSize.Level0)
{
    if (strlen(g_codecName) == 0) {
        return;
    }
    auto vEncSample = make_unique<VEncAPI11Sample>();
    vEncSample->INP_DIR = INP_DIR_720;
    vEncSample->OUT_DIR = "/data/test/media/1280_720_nv.h264";
    vEncSample->DEFAULT_WIDTH = DEFAULT_WIDTH;
    vEncSample->DEFAULT_HEIGHT = DEFAULT_HEIGHT;
    vEncSample->DEFAULT_FRAME_RATE = 30;
    vEncSample->DEFAULT_BITRATE_MODE = VBR;
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(25, vEncSample->outCount);
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}
}