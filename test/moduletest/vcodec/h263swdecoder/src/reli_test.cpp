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
class H263SwdecReliNdkTest : public testing::Test {
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
    const char *INP_DIR_1920x1080_30_ARRAY[16] = {
        "/data/test/media/profile2_level60_gop60_1920x1080.h263",    "/data/test/media/profile2_level60_gop60_1920x1080_1.h263",
        "/data/test/media/profile2_level60_gop60_1920x1080_2.h263",  "/data/test/media/profile2_level60_gop60_1920x1080_3.h263",
        "/data/test/media/profile2_level60_gop60_1920x1080_8.h263",  "/data/test/media/profile2_level60_gop60_1920x1080_12.h263",
        "/data/test/media/profile2_level60_gop60_1920x1080_4.h263",  "/data/test/media/profile2_level60_gop60_1920x1080_9.h263",
        "/data/test/media/profile2_level60_gop60_1920x1080_13.h263", "/data/test/media/profile2_level60_gop60_1920x1080_5.h263",
        "/data/test/media/profile2_level60_gop60_1920x1080_10.h263", "/data/test/media/profile2_level60_gop60_1920x1080_14.h263",
        "/data/test/media/profile2_level60_gop60_1920x1080_6.h263",  "/data/test/media/profile2_level60_gop60_1920x1080_11.h263",
        "/data/test/media/profile2_level60_gop60_1920x1080_15.h263", "/data/test/media/profile2_level60_gop60_1920x1080_7.h263"};
};
} // namespace Media
} // namespace OHOS

void H263SwdecReliNdkTest::SetUpTestCase() {}
void H263SwdecReliNdkTest::TearDownTestCase() {}
void H263SwdecReliNdkTest::SetUp() {}
void H263SwdecReliNdkTest::TearDown() {}

namespace {
/**
 * @tc.number    : VIDEO_H263SWDEC_RELI_WHILE_0100
 * @tc.name      : 16 instances
 * @tc.desc      : reliable test
 */
HWTEST_F(H263SwdecReliNdkTest, VIDEO_H263SWDEC_RELI_WHILE_0100, TestSize.Level3)
{
    for (int i = 0; i < 16; i++) {
        auto vDecSample = make_shared<VDecAPI11Sample>();
        vDecSample->SURFACE_OUTPUT = false;
        vDecSample->INP_DIR = INP_DIR_1920x1080_30_ARRAY[i];
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.H263"));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        if (i == 15) {
            vDecSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
        }
    }
}


/**
 * @tc.number    : VIDEO_H263SWDEC_RELI_WHILE_0200
 * @tc.name      : create Destroy
 * @tc.desc      : reliable test
 */
HWTEST_F(H263SwdecReliNdkTest, VIDEO_H263SWDEC_RELI_WHILE_0200, TestSize.Level4)
{
    for (int i = 0; i < 1000; i++) {
        OH_AVCodec *vdec_ = OH_VideoDecoder_CreateByName("OH.Media.Codec.Decoder.Video.H263");
        ASSERT_NE(nullptr, vdec_);
        OH_AVFormat *format = OH_AVFormat_Create();
        ASSERT_NE(nullptr, format);
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, 1920);
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, 1080);
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_FRAME_RATE, 30);
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_NV12);
        ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(vdec_, format));
        OH_AVFormat_Destroy(format);
        ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Start(vdec_));
        OH_VideoDecoder_Stop(vdec_);
        OH_VideoDecoder_Destroy(vdec_);
    }
}
} // namespace