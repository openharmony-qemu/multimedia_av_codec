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
#include "videodec_ndk_sample.h"
using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;

namespace OHOS {
namespace Media {
class SwdecReliNdkTest : public testing::Test {
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
    const char *INP_DIR_720_30 = "/data/test/media/1280x720_30_10M.h264";
    const char *INP_DIR_1080_30 = "/data/test/media/1920_1080_10_30Mb.h264";
    const char *INP_DIR_720_30_ARRAY[16] = {
        "/data/test/media/1280x720_30_10M.h264",    "/data/test/media/1280x720_30_10M_1.h264",
        "/data/test/media/1280x720_30_10M_2.h264",  "/data/test/media/1280x720_30_10M_3.h264",
        "/data/test/media/1280x720_30_10M_8.h264",  "/data/test/media/1280x720_30_10M_12.h264",
        "/data/test/media/1280x720_30_10M_4.h264",  "/data/test/media/1280x720_30_10M_9.h264",
        "/data/test/media/1280x720_30_10M_13.h264", "/data/test/media/1280x720_30_10M_5.h264",
        "/data/test/media/1280x720_30_10M_10.h264", "/data/test/media/1280x720_30_10M_14.h264",
        "/data/test/media/1280x720_30_10M_6.h264",  "/data/test/media/1280x720_30_10M_11.h264",
        "/data/test/media/1280x720_30_10M_15.h264", "/data/test/media/1280x720_30_10M_7.h264"};
};
} // namespace Media
} // namespace OHOS

void SwdecReliNdkTest::SetUpTestCase() {}
void SwdecReliNdkTest::TearDownTestCase() {}
void SwdecReliNdkTest::SetUp() {}
void SwdecReliNdkTest::TearDown() {}

namespace {
constexpr uint32_t MAX_THREAD = 16;
/**
 * @tc.number    : VIDEO_SWDEC_RELI_0200
 * @tc.name      : confige-start-flush-start-reset 1000 times
 * @tc.desc      : reliable test
 */
HWTEST_F(SwdecReliNdkTest, VIDEO_SWDEC_RELI_0200, TestSize.Level4)
{
    vdec_ = OH_VideoDecoder_CreateByName("OH.Media.Codec.Decoder.Video.AVC");
    for (int i = 0; i < 1000; i++) {
        ASSERT_NE(nullptr, vdec_);
        OH_AVFormat *format = OH_AVFormat_Create();
        ASSERT_NE(nullptr, format);
        string widthStr = "width";
        string heightStr = "height";
        string frameRateStr = "frame_rate";
        (void)OH_AVFormat_SetIntValue(format, widthStr.c_str(), 1920);
        (void)OH_AVFormat_SetIntValue(format, heightStr.c_str(), 1080);
        (void)OH_AVFormat_SetIntValue(format, frameRateStr.c_str(), 30);
        ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(vdec_, format));
        OH_AVFormat_Destroy(format);
        ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Start(vdec_));
        ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Flush(vdec_));
        ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Start(vdec_));
        ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Reset(vdec_));
    }
    OH_VideoDecoder_Stop(vdec_);
    OH_VideoDecoder_Destroy(vdec_);
}

/**
 * @tc.number    : VIDEO_SWDEC_RELI_0400
 * @tc.name      : SetParameter 1000 times
 * @tc.desc      : reliable test
 */
HWTEST_F(SwdecReliNdkTest, VIDEO_SWDEC_RELI_0400, TestSize.Level4)
{
    vdec_ = OH_VideoDecoder_CreateByName("OH.Media.Codec.Decoder.Video.AVC");
    ASSERT_NE(nullptr, vdec_);
    OH_AVFormat *format = OH_AVFormat_Create();
    ASSERT_NE(nullptr, format);
    string widthStr = "width";
    string heightStr = "height";
    string frameRateStr = "frame_rate";
    int64_t widht = 1920;
    (void)OH_AVFormat_SetIntValue(format, widthStr.c_str(), widht);
    (void)OH_AVFormat_SetIntValue(format, heightStr.c_str(), 1080);
    (void)OH_AVFormat_SetIntValue(format, frameRateStr.c_str(), 30);
    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(vdec_, format));
    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Start(vdec_));
    for (int i = 0; i < 1000; i++) {
        widht++;
        (void)OH_AVFormat_SetIntValue(format, widthStr.c_str(), widht);
        ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_SetParameter(vdec_, format));
    }
    OH_AVFormat_Destroy(format);
    OH_VideoDecoder_Destroy(vdec_);
}

/**
 * @tc.number    : VIDEO_SWDEC_RELI_WHILE_0100
 * @tc.name      : 16 instances
 * @tc.desc      : reliable test
 */
HWTEST_F(SwdecReliNdkTest, VIDEO_SWDEC_RELI_WHILE_0100, TestSize.Level3)
{
    for (int i = 0; i < 16; i++) {
        VDecNdkSample *vDecSample = new VDecNdkSample();
        vDecSample->SURFACE_OUTPUT = false;
        vDecSample->INP_DIR = INP_DIR_720_30_ARRAY[i];
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->sleepOnFPS = true;
        vDecSample->repeatRun = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.AVC"));
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
 * @tc.number    : VIDEO_SWDEC_RELI_WHILE_0200
 * @tc.name      : 16 instances while true
 * @tc.desc      : reliable test
 */
HWTEST_F(SwdecReliNdkTest, VIDEO_SWDEC_RELI_WHILE_0200, TestSize.Level3)
{
    VDecNdkSample *vDecList[16] = {};
    while (true) {
        for (int i = 0; i < 16; i++) {
            VDecNdkSample *vDecSample = new VDecNdkSample();
            vDecList[i] = vDecSample;
            vDecSample->SURFACE_OUTPUT = false;
            vDecSample->INP_DIR = INP_DIR_720_30_ARRAY[i];
            vDecSample->DEFAULT_WIDTH = 1280;
            vDecSample->DEFAULT_HEIGHT = 720;
            vDecSample->DEFAULT_FRAME_RATE = 30;
            vDecSample->sleepOnFPS = true;
            ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.AVC"));
            ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
            ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
            ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        }
        for (int i = 0; i < 16; i++) {
            vDecList[i]->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vDecList[i]->errCount);
            delete vDecList[i];
            vDecList[i] = nullptr;
        }
        usleep(5000000);
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_RELI_WHILE_0300
 * @tc.name      : long decode
 * @tc.desc      : reliable test
 */
HWTEST_F(SwdecReliNdkTest, VIDEO_SWDEC_RELI_WHILE_0300, TestSize.Level3)
{
    shared_ptr<VDecNdkSample> vDecSample = make_shared<VDecNdkSample>();
    vDecSample->SURFACE_OUTPUT = false;
    vDecSample->INP_DIR = INP_DIR_720_30;
    vDecSample->DEFAULT_WIDTH = 1280;
    vDecSample->DEFAULT_HEIGHT = 720;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->repeatRun = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.AVC"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_SWDEC_RELI_WHILE_0300
 * @tc.name      : confige-start-flush-start-reset while true
 * @tc.desc      : reli test
 */
HWTEST_F(SwdecReliNdkTest, VIDEO_SWDEC_RELI_WHILE_0400, TestSize.Level3)
{
    while (true) {
        auto vDecSample = make_shared<VDecNdkSample>();
        vDecSample->SURFACE_OUTPUT = false;
        vDecSample->INP_DIR = INP_DIR_720_30;
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.AVC"));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_MULTIINSTANCE_0100
 * @tc.name      : create 16 decoder (320*240)
 * @tc.desc      : reliable test
 */

HWTEST_F(SwdecReliNdkTest, VIDEO_SWDEC_MULTIINSTANCE_0100, TestSize.Level3)
{
    vector<shared_ptr<VDecNdkSample>> decVec;
    for (int i = 0; i < MAX_THREAD; i++) {
        auto vDecSample = make_shared<VDecNdkSample>();
        decVec.push_back(vDecSample);
        vDecSample->INP_DIR = INP_DIR_1080_30;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SURFACE_OUTPUT = false;
        cout << i << " ";
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.AVC"));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->Start());
    }
}

/*
 * @tc.number    : VIDEO_SWDEC_MULTIINSTANCE_0100
 * @tc.name      : create 17 decoder
 * @tc.desc      : reliable test
 */
HWTEST_F(SwdecReliNdkTest, VIDEO_SWDEC_MULTIINSTANCE_0200, TestSize.Level3)
{
    vector<shared_ptr<VDecNdkSample>> decVec;
    for (int i = 0; i < MAX_THREAD; i++) {
        auto vDecSample = make_shared<VDecNdkSample>();
        decVec.push_back(vDecSample);
        vDecSample->INP_DIR = INP_DIR_1080_30;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SURFACE_OUTPUT = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.AVC"));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->Start());
    }
    auto vDecSampleExtra = make_shared<VDecNdkSample>();
    vDecSampleExtra->INP_DIR = INP_DIR_1080_30;
    vDecSampleExtra->DEFAULT_WIDTH = 1920;
    vDecSampleExtra->DEFAULT_HEIGHT = 1080;
    vDecSampleExtra->DEFAULT_FRAME_RATE = 30;
    vDecSampleExtra->SURFACE_OUTPUT = false;
    ASSERT_EQ(AV_ERR_UNKNOWN, vDecSampleExtra->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.AVC"));
}
} // namespace