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
constexpr uint32_t CODEC_NAME_SIZE = 128;
char g_codecName[CODEC_NAME_SIZE] = {};
char g_codecNameHEVC[CODEC_NAME_SIZE] = {};
OH_AVFormat *format;
constexpr uint32_t DEFAULT_WIDTH = 1280;
constexpr uint32_t DEFAULT_HEIGHT = 720;
constexpr uint32_t DEFAULT_KEY_FRAME_INTERVAL = 1000;
constexpr uint32_t DEFAULT_BITRATE = 10000000;
constexpr double DEFAULT_FRAME_RATE = 30.0;
OH_AVPixelFormat DEFAULT_PIX_FMT = AV_PIXEL_FORMAT_NV12;
const char *INP_DIR_720 = "/data/test/media/1280_720_nv.yuv";
} // namespace
namespace OHOS {
namespace Media {
class HwEncTemporalNdkTest : public testing::Test {
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
    const char *inpDir720 = "/data/test/media/1280_720_nv.yuv";
    const char *inpDir720Array[16] = {"/data/test/media/1280_720_nv.yuv",    "/data/test/media/1280_720_nv_1.yuv",
                                      "/data/test/media/1280_720_nv_2.yuv",  "/data/test/media/1280_720_nv_3.yuv",
                                      "/data/test/media/1280_720_nv_7.yuv",  "/data/test/media/1280_720_nv_10.yuv",
                                      "/data/test/media/1280_720_nv_13.yuv", "/data/test/media/1280_720_nv_4.yuv",
                                      "/data/test/media/1280_720_nv_8.yuv",  "/data/test/media/1280_720_nv_11.yuv",
                                      "/data/test/media/1280_720_nv_14.yuv", "/data/test/media/1280_720_nv_5.yuv",
                                      "/data/test/media/1280_720_nv_9.yuv",  "/data/test/media/1280_720_nv_12.yuv",
                                      "/data/test/media/1280_720_nv_15.yuv", "/data/test/media/1280_720_nv_6.yuv"};
};
} // namespace Media
} // namespace OHOS

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;

void HwEncTemporalNdkTest::SetUpTestCase()
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
void HwEncTemporalNdkTest::TearDownTestCase() {}
void HwEncTemporalNdkTest::SetUp() {}
void HwEncTemporalNdkTest::TearDown()
{
    if (venc_ != NULL) {
        OH_VideoEncoder_Destroy(venc_);
        venc_ = nullptr;
    }
    if (format != nullptr) {
        OH_AVFormat_Destroy(format);
        format = nullptr;
    }
}
namespace {
/**
 * @tc.number    : VIDEO_TEMPORAL_ENCODE_API_0020
 * @tc.name      : OOH_MD_KEY_VIDEO_ENCODER_TEMPORAL_GOP_REFERENCE_MODE error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncTemporalNdkTest, VIDEO_TEMPORAL_ENCODE_API_0020, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    venc_ = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, venc_);
    format = OH_AVFormat_Create();
    ASSERT_NE(nullptr, format);

    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, DEFAULT_PIX_FMT);
    (void)OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_FRAME_RATE, DEFAULT_FRAME_RATE);
    (void)OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, DEFAULT_BITRATE);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_I_FRAME_INTERVAL, DEFAULT_KEY_FRAME_INTERVAL);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_ENABLE_TEMPORAL_LEVEL_SCALE, 1);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_TEMPORAL_GOP_REFERENCE_MODE, 3);
    ret = OH_VideoEncoder_Configure(venc_, format);
    ASSERT_EQ(ret, AV_ERR_INVALID_VAL);
}

/**
 * @tc.number    : VIDEO_TEMPORAL_ENCODE_API_0030
 * @tc.name      : OH_MD_KEY_VIDEO_ENCODER_TEMPORAL_GOP_SIZE para error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncTemporalNdkTest, VIDEO_TEMPORAL_ENCODE_API_0030, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    venc_ = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, venc_);
    format = OH_AVFormat_Create();
    ASSERT_NE(nullptr, format);

    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, DEFAULT_PIX_FMT);
    (void)OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_FRAME_RATE, DEFAULT_FRAME_RATE);
    (void)OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, DEFAULT_BITRATE);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_I_FRAME_INTERVAL, DEFAULT_KEY_FRAME_INTERVAL);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_ENABLE_TEMPORAL_LEVEL_SCALE, 1);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_TEMPORAL_GOP_SIZE, 30);
    ret = OH_VideoEncoder_Configure(venc_, format);
    ASSERT_EQ(ret, AV_ERR_INVALID_VAL);
}

/**
 * @tc.number    : VIDEO_TEMPORAL_ENCODE_FUNCTION_0010
 * @tc.name      : 调用使能接口
 * @tc.desc      : api test
 */
HWTEST_F(HwEncTemporalNdkTest, VIDEO_TEMPORAL_ENCODE_FUNCTION_0010, TestSize.Level1)
{
    int32_t temporal_gop_size = 2;
    if (!access("/system/lib64/media/", 0)) {
        auto vEncSample = make_unique<VEncNdkSample>();
        cout << "running on phone=========="<< endl;
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
        vEncSample->TEMPORAL_ENABLE = true;
        ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder_Temporal(temporal_gop_size));
    } else {
        auto vEncSample = make_unique<VEncNdkSample>();
        cout << "running on rk=========="<< endl;
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
        vEncSample->TEMPORAL_ENABLE = true;
        ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder_Temporal(temporal_gop_size));
    }
}

/**
 * @tc.number    : VIDEO_TEMPORAL_ENCODE_FUNCTION_0020
 * @tc.name      : 未使能，配置相邻模式分层编码,h264 buffer
 * @tc.desc      : api test
 */
HWTEST_F(HwEncTemporalNdkTest, VIDEO_TEMPORAL_ENCODE_FUNCTION_0020, TestSize.Level1)
{
    auto vEncSample = make_unique<VEncNdkSample>();
    vEncSample->OUT_DIR = "/data/test/media/VIDEO_TEMPORAL_ENCODE_FUNCTION_0020.h264";
    vEncSample->INP_DIR = INP_DIR_720;
    vEncSample->DEFAULT_WIDTH = 1280;
    vEncSample->DEFAULT_HEIGHT = 720;
    vEncSample->DEFAULT_FRAME_RATE = 30;
    vEncSample->TEMPORAL_ENABLE = false;
    vEncSample->TEMPORAL_CONFIG = true;
    int32_t temporal_gop_size = 2;
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder_Temporal(temporal_gop_size));
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

/**
 * @tc.number    : VIDEO_TEMPORAL_ENCODE_FUNCTION_0030
 * @tc.name      : 未使能，配置相邻模式分层编码,h265 surface
 * @tc.desc      : api test
 */
HWTEST_F(HwEncTemporalNdkTest, VIDEO_TEMPORAL_ENCODE_FUNCTION_0030, TestSize.Level1)
{
    auto vEncSample = make_unique<VEncNdkSample>();
    vEncSample->OUT_DIR = "/data/test/media/VIDEO_TEMPORAL_ENCODE_FUNCTION_0030.h265";
    vEncSample->INP_DIR = INP_DIR_720;
    vEncSample->DEFAULT_WIDTH = 1280;
    vEncSample->DEFAULT_HEIGHT = 720;
    vEncSample->DEFAULT_FRAME_RATE = 30;
    vEncSample->SURFACE_INPUT = true;
    vEncSample->TEMPORAL_ENABLE = false;
    vEncSample->TEMPORAL_CONFIG = true;
    int32_t temporal_gop_size = 2;
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder_Temporal(temporal_gop_size));
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

/**
 * @tc.number    : VIDEO_TEMPORAL_ENCODE_FUNCTION_0040
 * @tc.name      : 配置跨帧模式分层编码,默认gopsize,h265 surface
 * @tc.desc      : api test
 */
HWTEST_F(HwEncTemporalNdkTest, VIDEO_TEMPORAL_ENCODE_FUNCTION_0040, TestSize.Level1)
{
    auto vEncSample = make_unique<VEncNdkSample>();
    vEncSample->OUT_DIR = "/data/test/media/VIDEO_TEMPORAL_ENCODE_FUNCTION_0040.h265";
    vEncSample->INP_DIR = INP_DIR_720;
    vEncSample->DEFAULT_WIDTH = 1280;
    vEncSample->DEFAULT_HEIGHT = 720;
    vEncSample->DEFAULT_FRAME_RATE = 30;
    vEncSample->SURFACE_INPUT = true;
    vEncSample->TEMPORAL_ENABLE = true;
    vEncSample->TEMPORAL_DEFAULT = true;
    vEncSample->TEMPORAL_JUMP_MODE = true;
    int32_t temporal_gop_size = 0;
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder_Temporal(temporal_gop_size));
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

/**
 * @tc.number    : VIDEO_TEMPORAL_ENCODE_FUNCTION_0440
 * @tc.name      : 配置相邻模式分层编码,默认gopsize,h265 surface
 * @tc.desc      : api test
 */
HWTEST_F(HwEncTemporalNdkTest, VIDEO_TEMPORAL_ENCODE_FUNCTION_0440, TestSize.Level1)
{
    auto vEncSample = make_unique<VEncNdkSample>();
    vEncSample->OUT_DIR = "/data/test/media/VIDEO_TEMPORAL_ENCODE_FUNCTION_0440.h265";
    vEncSample->INP_DIR = INP_DIR_720;
    vEncSample->DEFAULT_WIDTH = 1280;
    vEncSample->DEFAULT_HEIGHT = 720;
    vEncSample->DEFAULT_FRAME_RATE = 30;
    vEncSample->SURFACE_INPUT = true;
    vEncSample->TEMPORAL_ENABLE = true;
    vEncSample->TEMPORAL_DEFAULT = true;
    int32_t temporal_gop_size = 0;
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder_Temporal(temporal_gop_size));
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

/**
 * @tc.number    : VIDEO_TEMPORAL_ENCODE_FUNCTION_0050
 * @tc.name      : h265，surface相邻参考模式分层编码。时域gop size为2
 * @tc.desc      : function test
 */
HWTEST_F(HwEncTemporalNdkTest, VIDEO_TEMPORAL_ENCODE_FUNCTION_0050, TestSize.Level1)
{
    auto vEncSample = make_unique<VEncNdkSample>();
    vEncSample->OUT_DIR = "/data/test/media/VIDEO_TEMPORAL_ENCODE_FUNCTION_0050.h265";
    vEncSample->INP_DIR = INP_DIR_720;
    vEncSample->DEFAULT_WIDTH = 1280;
    vEncSample->DEFAULT_HEIGHT = 720;
    vEncSample->DEFAULT_FRAME_RATE = 30;
    vEncSample->SURFACE_INPUT = true;
    vEncSample->TEMPORAL_ENABLE = true;
    vEncSample->TEMPORAL_CONFIG = true;
    int32_t temporal_gop_size = 2;
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder_Temporal(temporal_gop_size));
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

/**
 * @tc.number    : VIDEO_TEMPORAL_ENCODE_FUNCTION_0060
 * @tc.name      : h265，surface相邻参考模式分层编码。时域gop size为I帧间隔-1
 * @tc.desc      : function test
 */
HWTEST_F(HwEncTemporalNdkTest, VIDEO_TEMPORAL_ENCODE_FUNCTION_0060, TestSize.Level1)
{
    auto vEncSample = make_unique<VEncNdkSample>();
    vEncSample->OUT_DIR = "/data/test/media/VIDEO_TEMPORAL_ENCODE_FUNCTION_0060.h265";
    vEncSample->INP_DIR = INP_DIR_720;
    vEncSample->DEFAULT_WIDTH = 1280;
    vEncSample->DEFAULT_HEIGHT = 720;
    vEncSample->DEFAULT_FRAME_RATE = 30;
    vEncSample->SURFACE_INPUT = true;
    vEncSample->TEMPORAL_ENABLE = true;
    vEncSample->TEMPORAL_CONFIG = true;

    int32_t temporal_gop_size = vEncSample->DEFAULT_FRAME_RATE-1;
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder_Temporal(temporal_gop_size));
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

/**
 * @tc.number    : VIDEO_TEMPORAL_ENCODE_FUNCTION_0070
 * @tc.name      : h265，surface跨帧参考模式分层编码。时域gop size为2
 * @tc.desc      : function test
 */
HWTEST_F(HwEncTemporalNdkTest, VIDEO_TEMPORAL_ENCODE_FUNCTION_0070, TestSize.Level1)
{
    auto vEncSample = make_unique<VEncNdkSample>();
    vEncSample->OUT_DIR = "/data/test/media/VIDEO_TEMPORAL_ENCODE_FUNCTION_0070.h265";
    vEncSample->INP_DIR = INP_DIR_720;
    vEncSample->DEFAULT_WIDTH = 1280;
    vEncSample->DEFAULT_HEIGHT = 720;
    vEncSample->DEFAULT_FRAME_RATE = 30;
    vEncSample->SURFACE_INPUT = true;
    vEncSample->TEMPORAL_ENABLE = true;
    vEncSample->TEMPORAL_CONFIG = true;
    vEncSample->TEMPORAL_JUMP_MODE = true;
    int32_t temporal_gop_size = 2;
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder_Temporal(temporal_gop_size));
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

/**
 * @tc.number    : VIDEO_TEMPORAL_ENCODE_FUNCTION_0080
 * @tc.name      : h265，surface跨帧参考模式分层编码  时域gop size为I帧间隔-1
 * @tc.desc      : function test
 */
HWTEST_F(HwEncTemporalNdkTest, VIDEO_TEMPORAL_ENCODE_FUNCTION_0080, TestSize.Level1)
{
    auto vEncSample = make_unique<VEncNdkSample>();
    vEncSample->OUT_DIR = "/data/test/media/VIDEO_TEMPORAL_ENCODE_FUNCTION_0080.h265";
    vEncSample->INP_DIR = INP_DIR_720;
    vEncSample->DEFAULT_WIDTH = 1280;
    vEncSample->DEFAULT_HEIGHT = 720;
    vEncSample->DEFAULT_FRAME_RATE = 30;
    vEncSample->SURFACE_INPUT = true;
    vEncSample->TEMPORAL_ENABLE = true;
    vEncSample->TEMPORAL_CONFIG = true;
    vEncSample->TEMPORAL_JUMP_MODE = true;
    int32_t temporal_gop_size = vEncSample->DEFAULT_FRAME_RATE-1;
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder_Temporal(temporal_gop_size));
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

/**
 * @tc.number    : VIDEO_TEMPORAL_ENCODE_FUNCTION_0090
 * @tc.name      : h264 buffer 相邻参考模式分层编码 时域gop size为任意合法值
 * @tc.desc      : function test
 */
HWTEST_F(HwEncTemporalNdkTest, VIDEO_TEMPORAL_ENCODE_FUNCTION_0090, TestSize.Level1)
{
    auto vEncSample = make_unique<VEncNdkSample>();
    vEncSample->OUT_DIR = "/data/test/media/VIDEO_TEMPORAL_ENCODE_FUNCTION_0090.h264";
    vEncSample->INP_DIR = INP_DIR_720;
    vEncSample->DEFAULT_WIDTH = 1280;
    vEncSample->DEFAULT_HEIGHT = 720;
    vEncSample->DEFAULT_FRAME_RATE = 30;
    vEncSample->TEMPORAL_ENABLE = true;
    vEncSample->TEMPORAL_CONFIG = true;
    int32_t temporal_gop_size = 3;
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder_Temporal(temporal_gop_size));
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

/**
 * @tc.number    : VIDEO_TEMPORAL_ENCODE_FUNCTION_0100
 * @tc.name      : h264 buffer 跨帧参考模式分层编码 时域gop size为任意合法值
 * @tc.desc      : function test
 */
HWTEST_F(HwEncTemporalNdkTest, VIDEO_TEMPORAL_ENCODE_FUNCTION_0100, TestSize.Level1)
{
    auto vEncSample = make_unique<VEncNdkSample>();
    vEncSample->OUT_DIR = "/data/test/media/VIDEO_TEMPORAL_ENCODE_FUNCTION_0100.h264";
    vEncSample->INP_DIR = INP_DIR_720;
    vEncSample->DEFAULT_WIDTH = 1280;
    vEncSample->DEFAULT_HEIGHT = 720;
    vEncSample->DEFAULT_FRAME_RATE = 30;
    vEncSample->TEMPORAL_ENABLE = true;
    vEncSample->TEMPORAL_CONFIG = true;
    vEncSample->TEMPORAL_JUMP_MODE = true;
    int32_t temporal_gop_size = 3;
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder_Temporal(temporal_gop_size));
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

/**
 * @tc.number    : VIDEO_TEMPORAL_ENCODE_FUNCTION_0110
 * @tc.name      : h264 surface 相邻参考模式分层编码 时域gop size为任意合法值
 * @tc.desc      : function test
 */
HWTEST_F(HwEncTemporalNdkTest, VIDEO_TEMPORAL_ENCODE_FUNCTION_0110, TestSize.Level1)
{
    auto vEncSample = make_unique<VEncNdkSample>();
    vEncSample->OUT_DIR = "/data/test/media/VIDEO_TEMPORAL_ENCODE_FUNCTION_0110.h264";
    vEncSample->INP_DIR = INP_DIR_720;
    vEncSample->DEFAULT_WIDTH = 1280;
    vEncSample->DEFAULT_HEIGHT = 720;
    vEncSample->DEFAULT_FRAME_RATE = 30;
    vEncSample->SURFACE_INPUT = true;
    vEncSample->TEMPORAL_ENABLE = true;
    vEncSample->TEMPORAL_CONFIG = true;
    int32_t temporal_gop_size = 5;
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder_Temporal(temporal_gop_size));
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

/**
 * @tc.number    : VIDEO_TEMPORAL_ENCODE_FUNCTION_0120
 * @tc.name      : h264 surface 跨帧参考模式分层编码 时域gop size为5
 * @tc.desc      : function test
 */
HWTEST_F(HwEncTemporalNdkTest, VIDEO_TEMPORAL_ENCODE_FUNCTION_0120, TestSize.Level1)
{
    auto vEncSample = make_unique<VEncNdkSample>();
    vEncSample->OUT_DIR = "/data/test/media/VIDEO_TEMPORAL_ENCODE_FUNCTION_0120.h264";
    vEncSample->INP_DIR = INP_DIR_720;
    vEncSample->DEFAULT_WIDTH = 1280;
    vEncSample->DEFAULT_HEIGHT = 720;
    vEncSample->DEFAULT_FRAME_RATE = 30;
    vEncSample->SURFACE_INPUT = true;
    vEncSample->TEMPORAL_ENABLE = true;
    vEncSample->TEMPORAL_CONFIG = true;
    vEncSample->TEMPORAL_JUMP_MODE = true;
    int32_t temporal_gop_size = 5;
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder_Temporal(temporal_gop_size));
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

/**
 * @tc.number    : VIDEO_TEMPORAL_ENCODE_FUNCTION_0130
 * @tc.name      : h265 buffer 相邻参考模式分层编码 时域gop size为任意合法值
 * @tc.desc      : function test
 */
HWTEST_F(HwEncTemporalNdkTest, VIDEO_TEMPORAL_ENCODE_FUNCTION_0130, TestSize.Level1)
{
    auto vEncSample = make_unique<VEncNdkSample>();
    vEncSample->OUT_DIR = "/data/test/media/VIDEO_TEMPORAL_ENCODE_FUNCTION_0130.h265";
    vEncSample->INP_DIR = INP_DIR_720;
    vEncSample->DEFAULT_WIDTH = 1280;
    vEncSample->DEFAULT_HEIGHT = 720;
    vEncSample->DEFAULT_FRAME_RATE = 30;
    vEncSample->TEMPORAL_ENABLE = true;
    vEncSample->TEMPORAL_CONFIG = true;
    int32_t temporal_gop_size = 6;
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder_Temporal(temporal_gop_size));
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

/**
 * @tc.number    : VIDEO_TEMPORAL_ENCODE_FUNCTION_0140
 * @tc.name      : h265 buffer 跨帧参考模式分层编码 时域gop size为8
 * @tc.desc      : function test
 */
HWTEST_F(HwEncTemporalNdkTest, VIDEO_TEMPORAL_ENCODE_FUNCTION_0140, TestSize.Level1)
{
    auto vEncSample = make_unique<VEncNdkSample>();
    vEncSample->OUT_DIR = "/data/test/media/VIDEO_TEMPORAL_ENCODE_FUNCTION_0140.h265";
    vEncSample->INP_DIR = INP_DIR_720;
    vEncSample->DEFAULT_WIDTH = 1280;
    vEncSample->DEFAULT_HEIGHT = 720;
    vEncSample->DEFAULT_FRAME_RATE = 30;
    vEncSample->TEMPORAL_ENABLE = true;
    vEncSample->TEMPORAL_CONFIG = true;
    vEncSample->TEMPORAL_JUMP_MODE = true;
    int32_t temporal_gop_size = 8;
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder_Temporal(temporal_gop_size));
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

/**
 * @tc.number    : VIDEO_TEMPORAL_ENCODE_FUNCTION_0150
 * @tc.name      : h265 surface 相邻参考模式分层编码 时域gop size为任意合法值
 * @tc.desc      : function test
 */
HWTEST_F(HwEncTemporalNdkTest, VIDEO_TEMPORAL_ENCODE_FUNCTION_0150, TestSize.Level1)
{
    auto vEncSample = make_unique<VEncNdkSample>();
    vEncSample->OUT_DIR = "/data/test/media/VIDEO_TEMPORAL_ENCODE_FUNCTION_0150.h265";
    vEncSample->INP_DIR = INP_DIR_720;
    vEncSample->DEFAULT_WIDTH = 1280;
    vEncSample->DEFAULT_HEIGHT = 720;
    vEncSample->DEFAULT_FRAME_RATE = 30;
    vEncSample->SURFACE_INPUT = true;
    vEncSample->TEMPORAL_ENABLE = true;
    vEncSample->TEMPORAL_CONFIG = true;
    int32_t temporal_gop_size = 9;
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder_Temporal(temporal_gop_size));
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

/**
 * @tc.number    : VIDEO_TEMPORAL_ENCODE_FUNCTION_0160
 * @tc.name      : h265 surface 跨帧参考模式分层编码 时域gop size为12
 * @tc.desc      : function test
 */
HWTEST_F(HwEncTemporalNdkTest, VIDEO_TEMPORAL_ENCODE_FUNCTION_0160, TestSize.Level1)
{
    auto vEncSample = make_unique<VEncNdkSample>();
    vEncSample->OUT_DIR = "/data/test/media/VIDEO_TEMPORAL_ENCODE_FUNCTION_0160.h265";
    vEncSample->INP_DIR = INP_DIR_720;
    vEncSample->DEFAULT_WIDTH = 1280;
    vEncSample->DEFAULT_HEIGHT = 720;
    vEncSample->DEFAULT_FRAME_RATE = 30;
    vEncSample->SURFACE_INPUT = true;
    vEncSample->TEMPORAL_ENABLE = true;
    vEncSample->TEMPORAL_CONFIG = true;
    vEncSample->TEMPORAL_JUMP_MODE = true;
    int32_t temporal_gop_size = 12;
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder_Temporal(temporal_gop_size));
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

/**
 * @tc.number    : VIDEO_TEMPORAL_ENCODE_FUNCTION_0170
 * @tc.name      : h265 surface分层编码过程中强制I帧，检查相邻分层结构是否刷新
 * @tc.desc      : function test
 */
HWTEST_F(HwEncTemporalNdkTest, VIDEO_TEMPORAL_ENCODE_FUNCTION_0170, TestSize.Level1)
{
    auto vEncSample = make_unique<VEncNdkSample>();
    vEncSample->OUT_DIR = "/data/test/media/VIDEO_TEMPORAL_ENCODE_FUNCTION_0170.h265";
    vEncSample->INP_DIR = INP_DIR_720;
    vEncSample->DEFAULT_WIDTH = 1280;
    vEncSample->DEFAULT_HEIGHT = 720;
    vEncSample->DEFAULT_FRAME_RATE = 30;
    vEncSample->SURFACE_INPUT = true;
    vEncSample->TEMPORAL_ENABLE = true;
    vEncSample->TEMPORAL_CONFIG = true;
    int32_t temporal_gop_size = 3;
    vEncSample->enableForceIDR = true;
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder_Temporal(temporal_gop_size));
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

/**
 * @tc.number    : VIDEO_TEMPORAL_ENCODE_FUNCTION_0180
 * @tc.name      : h265 surface分层编码过程中强制I帧，检查跨帧分层结构是否刷新
 * @tc.desc      : function test
 */
HWTEST_F(HwEncTemporalNdkTest, VIDEO_TEMPORAL_ENCODE_FUNCTION_0180, TestSize.Level1)
{
    auto vEncSample = make_unique<VEncNdkSample>();
    vEncSample->OUT_DIR = "/data/test/media/VIDEO_TEMPORAL_ENCODE_FUNCTION_0180.h265";
    vEncSample->INP_DIR = INP_DIR_720;
    vEncSample->DEFAULT_WIDTH = 1280;
    vEncSample->DEFAULT_HEIGHT = 720;
    vEncSample->DEFAULT_FRAME_RATE = 30;
    vEncSample->SURFACE_INPUT = true;
    vEncSample->TEMPORAL_ENABLE = true;
    vEncSample->TEMPORAL_CONFIG = true;
    vEncSample->TEMPORAL_JUMP_MODE = true;
    int32_t temporal_gop_size = 4;
    vEncSample->enableForceIDR = true;
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder_Temporal(temporal_gop_size));
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}
} // namespace