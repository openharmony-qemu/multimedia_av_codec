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
constexpr uint32_t DEFAULT_BITRATE = 1000000;
char g_codecName[CODEC_NAME_SIZE] = {};
const char *INP_DIR_720 = "/data/test/media/1280_720_nv.yuv";
constexpr uint32_t DEFAULT_WIDTH = 1280;
constexpr uint32_t DEFAULT_HEIGHT = 720;
} // namespace
namespace OHOS {
namespace Media {
class AvcSwEncGetParamNdkTest : public testing::Test {
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

void AvcSwEncGetParamNdkTest::SetUpTestCase()
{
    cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_AVC, true, SOFTWARE);
    const char *tmpCodecName = OH_AVCapability_GetName(cap);
    if (memcpy_s(g_codecName, sizeof(g_codecName), tmpCodecName, strlen(tmpCodecName)) != 0)
        cout << "memcpy failed" << endl;
    cout << "codecname: " << g_codecName << endl;
}
void AvcSwEncGetParamNdkTest::TearDownTestCase() {}
void AvcSwEncGetParamNdkTest::SetUp() {}
void AvcSwEncGetParamNdkTest::TearDown()
{
    if (venc_ != NULL) {
        OH_VideoEncoder_Destroy(venc_);
        venc_ = nullptr;
    }
}
namespace {

/**
 * @tc.number    : VIDEO_ENCODE_MSE_QP_0100
 * @tc.name      : encode Avbuffer h264 buffer,get QP and MSE
 * @tc.desc      : function test
 */
HWTEST_F(AvcSwEncGetParamNdkTest, VIDEO_ENCODE_MSE_QP_0100, TestSize.Level0)
{
    if (strlen(g_codecName) == 0) {
        return;
    }
    auto vEncSample = make_unique<VEncAPI11Sample>();
    vEncSample->INP_DIR = INP_DIR_720;
    vEncSample->DEFAULT_WIDTH = DEFAULT_WIDTH;
    vEncSample->DEFAULT_HEIGHT = DEFAULT_HEIGHT;
    vEncSample->DEFAULT_FRAME_RATE = 30;
    vEncSample->DEFAULT_BITRATE = DEFAULT_BITRATE;
    vEncSample->getQpMse = true;
    vEncSample->SURF_INPUT = false;
    vEncSample->OUT_DIR = "/data/test/media/qp_mse_0300.h264";
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

/**
 * @tc.number    : VIDEO_ENCODE_MSE_QP_0200
 * @tc.name      : encode Avbuffer h264 surf,get QP and MSE
 * @tc.desc      : function test
 */
HWTEST_F(AvcSwEncGetParamNdkTest, VIDEO_ENCODE_MSE_QP_0200, TestSize.Level0)
{
    if (strlen(g_codecName) == 0) {
        return;
    }
    auto vEncSample = make_unique<VEncAPI11Sample>();
    vEncSample->INP_DIR = INP_DIR_720;
    vEncSample->DEFAULT_WIDTH = DEFAULT_WIDTH;
    vEncSample->DEFAULT_HEIGHT = DEFAULT_HEIGHT;
    vEncSample->DEFAULT_FRAME_RATE = 30;
    vEncSample->DEFAULT_BITRATE = DEFAULT_BITRATE;
    vEncSample->getQpMse = true;
    vEncSample->SURF_INPUT = true;
    vEncSample->OUT_DIR = "/data/test/media/qp_mse_0400.h264";
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}
}