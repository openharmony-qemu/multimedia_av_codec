/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
OH_AVCapability *cap_hevc = nullptr;
constexpr uint32_t CODEC_NAME_SIZE = 128;
constexpr uint32_t DEFAULT_BITRATE = 1000000;
char g_codecName[CODEC_NAME_SIZE] = {};
char g_codecNameHEVC[CODEC_NAME_SIZE] = {};
const char *INP_DIR_720 = "/data/test/media/1280_720_nv.yuv";
const char *INP_DIR_1920_30 = "/data/test/media/1920_1080_nv21.yuv";
const char *INP_DIR_1080_30 = "/data/test/media/1080_1920_nv12.yuv";
const char *OUT_DIR = "/data/test/media/VIDEO_ENCODE_FILE_ROI.h265";
constexpr uint32_t DEFAULT_WIDTH = 1280;
constexpr uint32_t DEFAULT_HEIGHT = 720;
} // namespace
namespace OHOS {
namespace Media {
class HwEncGetParamNdkTest : public testing::Test {
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

void HwEncGetParamNdkTest::SetUpTestCase()
{
    cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_AVC, true, HARDWARE);
    const char *tmpCodecName = OH_AVCapability_GetName(cap);
    if (memcpy_s(g_codecName, sizeof(g_codecName), tmpCodecName, strlen(tmpCodecName)) != 0)
        cout << "memcpy failed" << endl;
    cout << "codecname: " << g_codecName << endl;
    cap_hevc = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_HEVC, true, HARDWARE);
    const char *tmpCodecNameHevc = OH_AVCapability_GetName(cap_hevc);
    if (memcpy_s(g_codecNameHEVC, sizeof(g_codecNameHEVC), tmpCodecNameHevc, strlen(tmpCodecNameHevc)) != 0)
        cout << "memcpy failed" << endl;
    cout << "codecname_hevc: " << g_codecNameHEVC << endl;
}
void HwEncGetParamNdkTest::TearDownTestCase() {}
void HwEncGetParamNdkTest::SetUp() {}
void HwEncGetParamNdkTest::TearDown()
{
    if (venc_ != NULL) {
        OH_VideoEncoder_Destroy(venc_);
        venc_ = nullptr;
    }
}
namespace {
/**
 * @tc.number    : VIDEO_ENCODE_MSE_QP_0100
 * @tc.name      : encode Avbuffer h265 buffer,get QP and MSE
 * @tc.desc      : function test
 */
HWTEST_F(HwEncGetParamNdkTest, VIDEO_ENCODE_MSE_QP_0100, TestSize.Level0)
{
    auto vEncSample = make_unique<VEncAPI11Sample>();
    vEncSample->INP_DIR = INP_DIR_720;
    vEncSample->DEFAULT_WIDTH = DEFAULT_WIDTH;
    vEncSample->DEFAULT_HEIGHT = DEFAULT_HEIGHT;
    vEncSample->DEFAULT_FRAME_RATE = 30;
    vEncSample->DEFAULT_BITRATE = DEFAULT_BITRATE;
    vEncSample->getQpMse = true;
    vEncSample->SURF_INPUT = false;
    vEncSample->OUT_DIR = "/data/test/media/qp_mse_0100.h265";
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

/**
 * @tc.number    : VIDEO_ENCODE_MSE_QP_0200
 * @tc.name      : encode Avbuffer h265 surf,get QP and MSE
 * @tc.desc      : function test
 */
HWTEST_F(HwEncGetParamNdkTest, VIDEO_ENCODE_MSE_QP_0200, TestSize.Level0)
{
    auto vEncSample = make_unique<VEncAPI11Sample>();
    vEncSample->INP_DIR = INP_DIR_720;
    vEncSample->DEFAULT_WIDTH = DEFAULT_WIDTH;
    vEncSample->DEFAULT_HEIGHT = DEFAULT_HEIGHT;
    vEncSample->DEFAULT_FRAME_RATE = 30;
    vEncSample->DEFAULT_BITRATE = DEFAULT_BITRATE;
    vEncSample->getQpMse = true;
    vEncSample->SURF_INPUT = true;
    vEncSample->OUT_DIR = "/data/test/media/qp_mse_0200.h265";
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

/**
 * @tc.number    : VIDEO_ENCODE_MSE_QP_0300
 * @tc.name      : encode Avbuffer h264 buffer,get QP and MSE
 * @tc.desc      : function test
 */
HWTEST_F(HwEncGetParamNdkTest, VIDEO_ENCODE_MSE_QP_0300, TestSize.Level0)
{
    auto vEncSample = make_unique<VEncAPI11Sample>();
    vEncSample->INP_DIR = INP_DIR_720;
    vEncSample->DEFAULT_WIDTH = DEFAULT_WIDTH;
    vEncSample->DEFAULT_HEIGHT = DEFAULT_HEIGHT;
    vEncSample->DEFAULT_FRAME_RATE = 30;
    vEncSample->DEFAULT_BITRATE = DEFAULT_BITRATE;
    vEncSample->getQpMse = true;
    vEncSample->SURF_INPUT = false;
    vEncSample->OUT_DIR = "/data/test/media/qp_mse_0300.h265";
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

/**
 * @tc.number    : VIDEO_ENCODE_MSE_QP_0400
 * @tc.name      : encode Avbuffer h264 surf,get QP and MSE
 * @tc.desc      : function test
 */
HWTEST_F(HwEncGetParamNdkTest, VIDEO_ENCODE_MSE_QP_0400, TestSize.Level0)
{
    auto vEncSample = make_unique<VEncAPI11Sample>();
    vEncSample->INP_DIR = INP_DIR_720;
    vEncSample->DEFAULT_WIDTH = DEFAULT_WIDTH;
    vEncSample->DEFAULT_HEIGHT = DEFAULT_HEIGHT;
    vEncSample->DEFAULT_FRAME_RATE = 30;
    vEncSample->DEFAULT_BITRATE = DEFAULT_BITRATE;
    vEncSample->getQpMse = true;
    vEncSample->SURF_INPUT = true;
    vEncSample->OUT_DIR = "/data/test/media/qp_mse_0400.h265";
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

/**
 * @tc.number    : VIDEO_HW_ENCODE_ROI_FUNC_001
 * @tc.name      : hardware encoder roi CBR NV12
 * @tc.desc      : function test
 */
HWTEST_F(HwEncGetParamNdkTest, VIDEO_HW_ENCODE_ROI_FUNC_001, TestSize.Level1)
{
    bool isSupported = OH_AVCapability_IsEncoderBitrateModeSupported(cap_hevc, BITRATE_MODE_CBR);
    if (!isSupported) {
        return;
    }
    auto vEncSample = make_unique<VEncAPI11Sample>();
    vEncSample->DEFAULT_WIDTH = 1920;
    vEncSample->DEFAULT_HEIGHT = 1080;
    vEncSample->roiInfo = "10,10-100,100=-8;100,1-200,200=-4;"
                      "300,350-430,440=-4;510,54050,690=-4;"
                      "700,710-820,1000=-4";
    vEncSample->OUT_DIR = OUT_DIR;
    vEncSample->DEFAULT_PIX_FMT = AV_PIXEL_FORMAT_NV12;
    vEncSample->DEFAULT_BITRATE_MODE = CBR;
    vEncSample->INP_DIR = INP_DIR_1920_30;
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

/**
 * @tc.number    : VIDEO_HW_ENCODE_ROI_FUNC_002
 * @tc.name      : hardware encoder roi VBR NV12
 * @tc.desc      : function test
 */
HWTEST_F(HwEncGetParamNdkTest, VIDEO_HW_ENCODE_ROI_FUNC_002, TestSize.Level1)
{
    bool isSupported = OH_AVCapability_IsEncoderBitrateModeSupported(cap_hevc, BITRATE_MODE_VBR);
    if (!isSupported) {
        return;
    }
    auto vEncSample = make_unique<VEncAPI11Sample>();
    vEncSample->DEFAULT_WIDTH = 1920;
    vEncSample->DEFAULT_HEIGHT = 1080;
    vEncSample->roiInfo = "440,1272-1080,1920=-51";
    vEncSample->OUT_DIR = OUT_DIR;
    vEncSample->DEFAULT_PIX_FMT = AV_PIXEL_FORMAT_NV12;
    vEncSample->DEFAULT_BITRATE_MODE = VBR;
    vEncSample->INP_DIR = INP_DIR_1920_30;
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

/**
 * @tc.number    : VIDEO_HW_ENCODE_ROI_FUNC_003
 * @tc.name      : hardware encoder roi CBR NV12
 * @tc.desc      : function test
 */
HWTEST_F(HwEncGetParamNdkTest, VIDEO_HW_ENCODE_ROI_FUNC_003, TestSize.Level1)
{
    bool isSupported = OH_AVCapability_IsEncoderBitrateModeSupported(cap_hevc, BITRATE_MODE_CBR);
    if (!isSupported) {
        return;
    }
    auto vEncSample = make_unique<VEncAPI11Sample>();
    vEncSample->DEFAULT_WIDTH = 1080;
    vEncSample->DEFAULT_HEIGHT = 1920;
    vEncSample->roiInfo = "10,10-100,100=-4;100,100-200,200=-4;350,300-400,430=-4;540,510-690,650=51";
    vEncSample->OUT_DIR = OUT_DIR;
    vEncSample->DEFAULT_PIX_FMT = AV_PIXEL_FORMAT_NV12;
    vEncSample->INP_DIR = INP_DIR_1080_30;
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

/**
 * @tc.number    : VIDEO_HW_ENCODE_ROI_FUNC_004
 * @tc.name      : hardware encoder roi SQR NV12
 * @tc.desc      : function test
 */
HWTEST_F(HwEncGetParamNdkTest, VIDEO_HW_ENCODE_ROI_FUNC_004, TestSize.Level1)
{
    bool isSupported = OH_AVCapability_IsEncoderBitrateModeSupported(cap_hevc, BITRATE_MODE_SQR);
    if (!isSupported) {
        return;
    }
    auto vEncSample = make_unique<VEncAPI11Sample>();
    vEncSample->DEFAULT_WIDTH = 1080;
    vEncSample->DEFAULT_HEIGHT = 1920;
    vEncSample->roiInfo = "0,0-500,500;500,500-600,600;600,500-700,600";
    vEncSample->OUT_DIR = OUT_DIR;
    vEncSample->DEFAULT_BITRATE_MODE = SQR;
    vEncSample->DEFAULT_PIX_FMT = AV_PIXEL_FORMAT_NV12;
    vEncSample->INP_DIR = INP_DIR_1080_30;
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

/**
 * @tc.number    : VIDEO_HW_ENCODE_ROI_FUNC_005
 * @tc.name      : hardware encoder roi SQR NV12
 * @tc.desc      : function test
 */
HWTEST_F(HwEncGetParamNdkTest, VIDEO_HW_ENCODE_ROI_FUNC_005, TestSize.Level1)
{
    bool isSupported = OH_AVCapability_IsEncoderBitrateModeSupported(cap_hevc, BITRATE_MODE_SQR);
    if (!isSupported) {
        return;
    }
    auto vEncSample = make_unique<VEncAPI11Sample>();
    vEncSample->DEFAULT_WIDTH = 1920;
    vEncSample->DEFAULT_HEIGHT = 1080;
    vEncSample->roiInfo = "123456789123,123456789123-123456789-12345678912=-8";
    vEncSample->OUT_DIR = OUT_DIR;
    vEncSample->DEFAULT_BITRATE_MODE = SQR;
    vEncSample->DEFAULT_PIX_FMT = AV_PIXEL_FORMAT_NV12;
    vEncSample->INP_DIR = INP_DIR_1920_30;
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

/**
 * @tc.number    : VIDEO_HW_ENCODE_ROI_FUNC_006
 * @tc.name      : hardware encoder roi CBR NV21
 * @tc.desc      : function test
 */
HWTEST_F(HwEncGetParamNdkTest, VIDEO_HW_ENCODE_ROI_FUNC_006, TestSize.Level1)
{
    bool isSupported = OH_AVCapability_IsEncoderBitrateModeSupported(cap_hevc, BITRATE_MODE_CBR);
    if (!isSupported) {
        return;
    }
    auto vEncSample = make_unique<VEncAPI11Sample>();
    vEncSample->DEFAULT_WIDTH = 1080;
    vEncSample->DEFAULT_HEIGHT = 1920;
    vEncSample->roiInfo = "10,10-100,100=-8;100,10-200,200=-4;"
                        "300,350-430,440=-4;510,540650,690=-4;"
                        "700,710-80,1000=-4";
    vEncSample->OUT_DIR = OUT_DIR;
    vEncSample->SURF_INPUT = true;
    vEncSample->enableSeekEos = true;
    vEncSample->enableRepeat = true;
    vEncSample->setMaxCount = true;
    vEncSample->DEFAULT_BITRATE_MODE = CBR;
    vEncSample->INP_DIR = INP_DIR_1080_30;
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

/**
 * @tc.number    : VIDEO_HW_ENCODE_ROI_FUNC_007
 * @tc.name      : hardware encoder roi VBR NV21
 * @tc.desc      : function test
 */
HWTEST_F(HwEncGetParamNdkTest, VIDEO_HW_ENCODE_ROI_FUNC_007, TestSize.Level1)
{
    bool isSupported = OH_AVCapability_IsEncoderBitrateModeSupported(cap_hevc, BITRATE_MODE_VBR);
    if (!isSupported) {
        return;
    }
    auto vEncSample = make_unique<VEncAPI11Sample>();
    vEncSample->DEFAULT_WIDTH = 1080;
    vEncSample->DEFAULT_HEIGHT = 1920;
    vEncSample->roiInfo = "400,1272-1080,1920=-51";
    vEncSample->OUT_DIR = OUT_DIR;
    vEncSample->SURF_INPUT = true;
    vEncSample->enableSeekEos = true;
    vEncSample->enableRepeat = true;
    vEncSample->setMaxCount = true;
    vEncSample->DEFAULT_BITRATE_MODE = VBR;
    vEncSample->INP_DIR = INP_DIR_1080_30;
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

/**
 * @tc.number    : VIDEO_HW_ENCODE_ROI_FUNC_008
 * @tc.name      : hardware encoder roi CBR NV21
 * @tc.desc      : function test
 */
HWTEST_F(HwEncGetParamNdkTest, VIDEO_HW_ENCODE_ROI_FUNC_008, TestSize.Level1)
{
    bool isSupported = OH_AVCapability_IsEncoderBitrateModeSupported(cap_hevc, BITRATE_MODE_CBR);
    if (!isSupported) {
        return;
    }
    auto vEncSample = make_unique<VEncAPI11Sample>();
    vEncSample->DEFAULT_WIDTH = 1080;
    vEncSample->DEFAULT_HEIGHT = 1920;
    vEncSample->roiInfo = "10,10-100,100=-4;100,100-200,200=-4;350,300-400,430=-4;540,510-690,650=51";
    vEncSample->OUT_DIR = OUT_DIR;
    vEncSample->SURF_INPUT = true;
    vEncSample->enableSeekEos = true;
    vEncSample->enableRepeat = true;
    vEncSample->setMaxCount = true;
    vEncSample->DEFAULT_BITRATE_MODE = CBR;
    vEncSample->INP_DIR = INP_DIR_1080_30;
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

/**
 * @tc.number    : VIDEO_HW_ENCODE_ROI_FUNC_009
 * @tc.name      : hardware encoder roi CBR NV12
 * @tc.desc      : function test
 */
HWTEST_F(HwEncGetParamNdkTest, VIDEO_HW_ENCODE_ROI_FUNC_009, TestSize.Level1)
{
    bool isSupported = OH_AVCapability_IsEncoderBitrateModeSupported(cap_hevc, BITRATE_MODE_SQR);
    if (!isSupported) {
        return;
    }
    auto vEncSample = make_unique<VEncAPI11Sample>();
    vEncSample->DEFAULT_WIDTH = 1080;
    vEncSample->DEFAULT_HEIGHT = 1920;
    vEncSample->roiInfo = "0,0-500,500;500,500-600,600;600,500-700,600";
    vEncSample->OUT_DIR = OUT_DIR;
    vEncSample->SURF_INPUT = true;
    vEncSample->enableSeekEos = true;
    vEncSample->enableRepeat = true;
    vEncSample->setMaxCount = true;
    vEncSample->DEFAULT_BITRATE_MODE = SQR;
    vEncSample->DEFAULT_PIX_FMT = AV_PIXEL_FORMAT_NV12;
    vEncSample->INP_DIR = INP_DIR_1080_30;
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

/**
 * @tc.number    : VIDEO_HW_ENCODE_ROI_FUNC_010
 * @tc.name      : hardware encoder roi CBR NV12
 * @tc.desc      : function test
 */
HWTEST_F(HwEncGetParamNdkTest, VIDEO_HW_ENCODE_ROI_FUNC_010, TestSize.Level1)
{
    bool isSupported = OH_AVCapability_IsEncoderBitrateModeSupported(cap_hevc, BITRATE_MODE_SQR);
    if (!isSupported) {
        return;
    }
    auto vEncSample = make_unique<VEncAPI11Sample>();
    vEncSample->DEFAULT_WIDTH = 1080;
    vEncSample->DEFAULT_HEIGHT = 1920;
    vEncSample->roiInfo = "123456789123,123456789123-123456789-12345678912=-8";
    vEncSample->OUT_DIR = OUT_DIR;
    vEncSample->SURF_INPUT = true;
    vEncSample->enableSeekEos = true;
    vEncSample->enableRepeat = true;
    vEncSample->setMaxCount = true;
    vEncSample->DEFAULT_BITRATE_MODE = SQR;
    vEncSample->DEFAULT_PIX_FMT = AV_PIXEL_FORMAT_NV12;
    vEncSample->INP_DIR = INP_DIR_1080_30;
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}
}