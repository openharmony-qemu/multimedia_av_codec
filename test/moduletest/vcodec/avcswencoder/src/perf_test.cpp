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
#include "native_avcodec_videoencoder.h"
#include "native_averrors.h"
#include "videoenc_sample.h"
#include "native_avcodec_base.h"
#include "avcodec_codec_name.h"
#include "native_avcapability.h"

#define MAX_THREAD 16

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;

namespace {
    OH_AVCodec *venc_ = NULL;
    OH_AVCapability *cap = nullptr;
    constexpr uint32_t CODEC_NAME_SIZE = 128;
    char g_codecName[CODEC_NAME_SIZE] = {};
    const char *INP_DIR_720 = "/data/test/media/1280_720_nv.yuv";
    const char *INP_DIR_1080 = "/data/test/media/1920_1080_nv21.yuv";
}

namespace OHOS {
namespace Media {
class AvcSwEncPerfNdkTest : public testing::Test {
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

void AvcSwEncPerfNdkTest::SetUpTestCase()
{
    cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_AVC, true, SOFTWARE);
    const char *tmpCodecName = OH_AVCapability_GetName(cap);
    if (memcpy_s(g_codecName, sizeof(g_codecName), tmpCodecName, strlen(tmpCodecName)) != 0)
        cout << "memcpy failed" << endl;
    cout << "codecname: " << g_codecName << endl;
}
void AvcSwEncPerfNdkTest::TearDownTestCase() {}
void AvcSwEncPerfNdkTest::SetUp() {}
void AvcSwEncPerfNdkTest::TearDown() {}

namespace {
/**
 * @tc.number    : VIDEO_ENCODE_PERF_0100
 * @tc.name      : OH_VideoEncoder_CreateByMime
 * @tc.desc      : performance test
 */
HWTEST_F(AvcSwEncPerfNdkTest, VIDEO_ENCODE_PERF_BUFFER_0100, TestSize.Level1)
{
    if (strlen(g_codecName) == 0) {
        return;
    }
    auto vEncSample = make_unique<VEncNdkSample>();
    vEncSample->INP_DIR = INP_DIR_720;
    vEncSample->DEFAULT_WIDTH = 1280;
    vEncSample->DEFAULT_HEIGHT = 720;
    vEncSample->DEFAULT_FRAME_RATE = 30;
    vEncSample->DEFAULT_BITRATE = 10000000;
    vEncSample->OUT_DIR = "/data/test/media/1280_720_buffer.h264";
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

/**
 * @tc.number    : VIDEO_ENCODE_PERF_BUFFER_0200
 * @tc.name      : perf time,1080P buffer
 * @tc.desc      : performance test
 */
HWTEST_F(AvcSwEncPerfNdkTest, VIDEO_ENCODE_PERF_BUFFER_0200, TestSize.Level1)
{
    if (strlen(g_codecName) == 0) {
        return;
    }
    auto vEncSample = make_unique<VEncNdkSample>();
    vEncSample->INP_DIR = INP_DIR_1080;
    vEncSample->DEFAULT_WIDTH = 1920;
    vEncSample->DEFAULT_HEIGHT = 1088;
    vEncSample->DEFAULT_FRAME_RATE = 30;
    vEncSample->DEFAULT_BITRATE = 20000000;
    vEncSample->OUT_DIR = "/data/test/media/1920_1080_buffer.h264";
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

/**
 * @tc.number    : VIDEO_ENCODE_PERF_BUFFER_0300
 * @tc.name      : perf time,720P buffer
 * @tc.desc      : performance test
 */
HWTEST_F(AvcSwEncPerfNdkTest, VIDEO_ENCODE_PERF_BUFFER_0300, TestSize.Level1)
{
    if (strlen(g_codecName) == 0) {
        return;
    }
    auto vEncSample = make_unique<VEncNdkSample>();
    vEncSample->INP_DIR = INP_DIR_720;
    vEncSample->DEFAULT_WIDTH = 1280;
    vEncSample->DEFAULT_HEIGHT = 720;
    vEncSample->DEFAULT_FRAME_RATE = 60;
    vEncSample->DEFAULT_BITRATE = 10000000;
    vEncSample->OUT_DIR = "/data/test/media/1280_720_buffer.h264";
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

/**
 * @tc.number    : VIDEO_ENCODE_PERF_BUFFER_0400
 * @tc.name      : perf mmeory,1080P buffer
 * @tc.desc      : performance test
 */
HWTEST_F(AvcSwEncPerfNdkTest, VIDEO_ENCODE_PERF_BUFFER_0400, TestSize.Level1)
{
    if (strlen(g_codecName) == 0) {
        return;
    }
    auto vEncSample = make_unique<VEncNdkSample>();
    vEncSample->INP_DIR = INP_DIR_1080;
    vEncSample->DEFAULT_WIDTH = 1920;
    vEncSample->DEFAULT_HEIGHT = 1088;
    vEncSample->DEFAULT_FRAME_RATE = 60;
    vEncSample->DEFAULT_BITRATE = 20000000;
    vEncSample->OUT_DIR = "/data/test/media/1920_1080_buffer.h264";
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

/**
 * @tc.number    : VIDEO_ENCODE_PERF_SURFACE_0100
 * @tc.name      : perf time,720P surface
 * @tc.desc      : performance test
 */
HWTEST_F(AvcSwEncPerfNdkTest, VIDEO_ENCODE_PERF_SURFACE_0100, TestSize.Level1)
{
    if (strlen(g_codecName) == 0) {
        return;
    }
    auto vEncSample = make_unique<VEncNdkSample>();
    vEncSample->INP_DIR = INP_DIR_720;
    vEncSample->DEFAULT_WIDTH = 1280;
    vEncSample->DEFAULT_HEIGHT = 720;
    vEncSample->DEFAULT_FRAME_RATE = 30;
    vEncSample->DEFAULT_BITRATE = 10000000;
    vEncSample->SURF_INPUT = true;
    vEncSample->OUT_DIR = "/data/test/media/1280_720.h264";
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

/**
 * @tc.number    : VIDEO_ENCODE_PERF_SURFACE_0200
 * @tc.name      : perf time,1080P surface
 * @tc.desc      : performance test
 */
HWTEST_F(AvcSwEncPerfNdkTest, VIDEO_ENCODE_PERF_SURFACE_0200, TestSize.Level1)
{
    if (strlen(g_codecName) == 0) {
        return;
    }
    auto vEncSample = make_unique<VEncNdkSample>();
    vEncSample->INP_DIR = INP_DIR_1080;
    vEncSample->DEFAULT_WIDTH = 1920;
    vEncSample->DEFAULT_HEIGHT = 1088;
    vEncSample->DEFAULT_FRAME_RATE = 30;
    vEncSample->DEFAULT_BITRATE = 20000000;
    vEncSample->SURF_INPUT = true;
    vEncSample->OUT_DIR = "/data/test/media/1920_1080.h264";
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

/**
 * @tc.number    : VIDEO_ENCODE_PERF_SURFACE_0300
 * @tc.name      : perf time,720P surface
 * @tc.desc      : performance test
 */
HWTEST_F(AvcSwEncPerfNdkTest, VIDEO_ENCODE_PERF_SURFACE_0300, TestSize.Level1)
{
    if (strlen(g_codecName) == 0) {
        return;
    }
    auto vEncSample = make_unique<VEncNdkSample>();
    vEncSample->INP_DIR = INP_DIR_720;
    vEncSample->DEFAULT_WIDTH = 1280;
    vEncSample->DEFAULT_HEIGHT = 720;
    vEncSample->DEFAULT_FRAME_RATE = 60;
    vEncSample->DEFAULT_BITRATE = 10000000;
    vEncSample->SURF_INPUT = true;
    vEncSample->OUT_DIR = "/data/test/media/1280_720.h264";
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

/**
 * @tc.number    : VIDEO_ENCODE_PERF_SURFACE_0400
 * @tc.name      : perf time,1080P surface
 * @tc.desc      : performance test
 */
HWTEST_F(AvcSwEncPerfNdkTest, VIDEO_ENCODE_PERF_SURFACE_0400, TestSize.Level1)
{
    if (strlen(g_codecName) == 0) {
        return;
    }
    auto vEncSample = make_unique<VEncNdkSample>();
    vEncSample->INP_DIR = INP_DIR_1080;
    vEncSample->DEFAULT_WIDTH = 1920;
    vEncSample->DEFAULT_HEIGHT = 1088;
    vEncSample->DEFAULT_FRAME_RATE = 60;
    vEncSample->DEFAULT_BITRATE = 20000000;
    vEncSample->SURF_INPUT = true;
    vEncSample->OUT_DIR = "/data/test/media/1920_1080.h264";
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

/**
 * @tc.number    : VIDEO_ENCODE_PERF_0100
 * @tc.name      : OH_VideoEncoder_CreateByMime
 * @tc.desc      : performance test
 */
HWTEST_F(AvcSwEncPerfNdkTest, VIDEO_ENCODE_PERF_0100, TestSize.Level1)
{
    if (strlen(g_codecName) == 0) {
        return;
    }
    for (int i = 0; i < 2000; i++) {
        venc_ = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
        ASSERT_NE(nullptr, venc_);
        ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(venc_));
        venc_ = nullptr;
    }
}
} // namespace