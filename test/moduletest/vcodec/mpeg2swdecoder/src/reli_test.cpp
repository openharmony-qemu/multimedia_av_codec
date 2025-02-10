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
class Mpeg2SwdecReliNdkTest : public testing::Test {
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
};
} // namespace Media
} // namespace OHOS

void Mpeg2SwdecReliNdkTest::SetUpTestCase() {}
void Mpeg2SwdecReliNdkTest::TearDownTestCase() {}
void Mpeg2SwdecReliNdkTest::SetUp() {}
void Mpeg2SwdecReliNdkTest::TearDown() {}

namespace {
/**
 * @tc.number    : VIDEO_MPEG2SWDEC_RELI_0100
 * @tc.name      : repeat create and destroy decoder in surface mode ,MPEG2
 * @tc.desc      : reliable test
 */
HWTEST_F(Mpeg2SwdecReliNdkTest, VIDEO_MPEG2SWDEC_RELI_0100, TestSize.Level3)
{
    shared_ptr<VDecNdkSample> vDecSample = make_shared<VDecNdkSample>();
    const char *INP_DIR_1 = "/data/test/media/main@high14_level_1280_720_60.m2v";
    vDecSample->SURFACE_OUTPUT = true;
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->DEFAULT_WIDTH = 1280;
    vDecSample->DEFAULT_HEIGHT = 720;
    vDecSample->DEFAULT_FRAME_RATE = 60;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface("OH.Media.Codec.Decoder.Video.MPEG2"));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_MPEG2SWDEC_RELI_0200
 * @tc.name      : repeat create and destroy decoder in buffer mode ,MPEG2
 * @tc.desc      : reliable test
 */
HWTEST_F(Mpeg2SwdecReliNdkTest, VIDEO_MPEG2SWDEC_RELI_0200, TestSize.Level3)
{
    shared_ptr<VDecNdkSample> vDecSample = make_shared<VDecNdkSample>();
    const char *INP_DIR_1 = "/data/test/media/main@high14_level_1280_720_60.m2v";
    vDecSample->SURFACE_OUTPUT = false;
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->DEFAULT_WIDTH = 1280;
    vDecSample->DEFAULT_HEIGHT = 720;
    vDecSample->DEFAULT_FRAME_RATE = 60;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.MPEG2"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_MPEG2SWDEC_RELI_0300
 * @tc.name      : long decode in surface mode ,MPEG2
 * @tc.desc      : reliable test
 */
HWTEST_F(Mpeg2SwdecReliNdkTest, VIDEO_MPEG2SWDEC_RELI_0300, TestSize.Level3)
{
    shared_ptr<VDecNdkSample> vDecSample = make_shared<VDecNdkSample>();
    const char *INP_DIR_1 = "/data/test/media/main@high14_level_1280_720_60.m2v";
    vDecSample->SURFACE_OUTPUT = true;
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->DEFAULT_WIDTH = 1280;
    vDecSample->DEFAULT_HEIGHT = 720;
    vDecSample->DEFAULT_FRAME_RATE = 60;
    vDecSample->repeatRun = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface("OH.Media.Codec.Decoder.Video.MPEG2"));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_MPEG2SWDEC_RELI_0400
 * @tc.name      : long decode in buffer mode ,MPEG2
 * @tc.desc      : reliable test
 */
HWTEST_F(Mpeg2SwdecReliNdkTest, VIDEO_MPEG2SWDEC_RELI_0400, TestSize.Level3)
{
    shared_ptr<VDecNdkSample> vDecSample = make_shared<VDecNdkSample>();
    const char *INP_DIR_1 = "/data/test/media/main@high14_level_1280_720_60.m2v";
    vDecSample->SURFACE_OUTPUT = false;
    vDecSample->INP_DIR = INP_DIR_1;
    vDecSample->DEFAULT_WIDTH = 1280;
    vDecSample->DEFAULT_HEIGHT = 720;
    vDecSample->DEFAULT_FRAME_RATE = 60;
    vDecSample->repeatRun = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.MPEG2"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}
} // namespace