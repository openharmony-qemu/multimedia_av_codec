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

#include <iostream>
#include <cstdio>
#include <atomic>
#include <fstream>
#include <thread>
#include <mutex>
#include <queue>
#include <string>

#include "gtest/gtest.h"
#include "native_avcodec_videodecoder.h"
#include "native_avcodec_base.h"
#include "videodec_sample.h"
#include "videodec_api11_sample.h"
using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;

namespace OHOS {
namespace Media {
class Mpeg4SwdecReliNdkTest : public testing::Test {
public:
    static void SetUpTestCase();    // 第一个测试用例执行前
    static void TearDownTestCase(); // 最后一个测试用例执行后
    void SetUp() override;          // 每个测试用例执行前
    void TearDown() override;       // 每个测试用例执行后
    void InputFunc();
    void OutputFunc();
    void Release();
    int32_t Stop();

protected:
    bool createCodecSuccess_ = false;
    OH_AVCodec *vdec_;
    static string gCodecNameMpeg4 = "";
    const char *INP_DIR_720_30 = "/data/test/media/mpeg4_simple@level6_1280x720_30.m4v";
    const char *INP_DIR_1080_30 = "/data/test/media/mpeg4_main@level4_1920x1080_30.m4v";
};
} // namespace Media
} // namespace OHOS

void Mpeg4SwdecReliNdkTest::SetUpTestCase()
{
    gCodecNameMpeg4 = "OH.Media.Codec.Decoder.Video.MPEG4";
}
void Mpeg4SwdecReliNdkTest::TearDownTestCase() {}
void Mpeg4SwdecReliNdkTest::SetUp() {}
void Mpeg4SwdecReliNdkTest::TearDown() {}

namespace {
/**
 * @tc.number    : VIDEO_MPEG4SWDEC_RELI_0100
 * @tc.name      : repeat creat and release decoder in buffer mode
 * @tc.desc      : reliable test
 */
HWTEST_F(Mpeg4SwdecReliNdkTest, VIDEO_MPEG4SWDEC_RELI_0100, TestSize.Level3)
{
    for (int i = 0; i < 50; i++) {
        VDecNdkSample *vDecSample = new VDecNdkSample();
        vDecSample->SURFACE_OUTPUT = false;
        vDecSample->INP_DIR = INP_DIR_1080_30;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(gCodecNameMpeg4));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_MPEG4SWDEC_RELI_0200
 * @tc.name      : repeat creat and release decoder in surface mode
 * @tc.desc      : reliable test
 */
HWTEST_F(Mpeg4SwdecReliNdkTest, VIDEO_MPEG4SWDEC_RELI_0200, TestSize.Level3)
{
    for (int i = 0; i < 50; i++) {
        VDecNdkSample *vDecSample = new VDecNdkSample();
        vDecSample->SURFACE_OUTPUT = true;
        vDecSample->INP_DIR = INP_DIR_1080_30;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(gCodecNameMpeg4));
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_MPEG4SWDEC_RELI_0300
 * @tc.name      : long decode in buffre mode
 * @tc.desc      : reliable test
 */
HWTEST_F(Mpeg4SwdecReliNdkTest, VIDEO_MPEG4SWDEC_RELI_0300, TestSize.Level3)
{
    shared_ptr<VDecNdkSample> vDecSample = make_shared<VDecNdkSample>();
    vDecSample->SURFACE_OUTPUT = false;
    vDecSample->INP_DIR = INP_DIR_720_30;
    vDecSample->DEFAULT_WIDTH = 1280;
    vDecSample->DEFAULT_HEIGHT = 720;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->repeatRun = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(gCodecNameMpeg4));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_MPEG4SWDEC_RELI_0400
 * @tc.name      : long decode in surface mode
 * @tc.desc      : reliable test
 */
HWTEST_F(Mpeg4SwdecReliNdkTest, VIDEO_MPEG4SWDEC_RELI_0400, TestSize.Level3)
{
    shared_ptr<VDecNdkSample> vDecSample = make_shared<VDecNdkSample>();
    vDecSample->SURFACE_OUTPUT = true;
    vDecSample->INP_DIR = INP_DIR_720_30;
    vDecSample->DEFAULT_WIDTH = 1280;
    vDecSample->DEFAULT_HEIGHT = 720;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->repeatRun = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface(gCodecNameMpeg4));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}


} // namespace