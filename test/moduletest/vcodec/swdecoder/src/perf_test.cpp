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
#include "videodec_ndk_sample.h"
#include "native_avcodec_videodecoder.h"
#include "native_avcodec_base.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;

namespace OHOS {
namespace Media {
class SwdecPerfNdkTest : public testing::Test {
public:
    static void SetUpTestCase();    // 第一个测试用例执行前
    static void TearDownTestCase(); // 最后一个测试用例执行后
    void SetUp() override;          // 每个测试用例执行前
    void TearDown() override;       // 每个测试用例执行后
    void InputFunc();
    void OutputFunc();
    void Release();
    int64_t GetSystemTimeUs();
    int32_t Stop();

protected:
    OH_AVCodec *vdec_;
    bool createCodecSuccess_ = false;
    const string CODEC_NAME = "video_decoder.avc";
    const char *INP_DIR = "/data/test/media/1920_1080_20M_30.h264";

    const char *INP_DIR_720_30 = "/data/test/media/1280_720_10M_30.h264";
    const char *INP_DIR_720_60 = "/data/test/media/1280_720_10M_60.h264";
    const char *INP_DIR_1080_30 = "/data/test/media/1920_1080_20M_30.h264";
    const char *INP_DIR_1080_60 = "/data/test/media/1920_1080_20M_60.h264";
    const char *INP_DIR_2160_30 = "/data/test/media/3840_2160_30M_30.h264";
    const char *INP_DIR_2160_60 = "/data/test/media/3840_2160_30M_60.h264";

    const char *INP_DIR_720_30_1 = "/data/test/media/1280_720_10M_30_1.h264";
    const char *INP_DIR_720_60_1 = "/data/test/media/1280_720_10M_60_1.h264";
    const char *INP_DIR_1080_30_1 = "/data/test/media/1920_1080_20M_30_1.h264";
    const char *INP_DIR_1080_60_1 = "/data/test/media/1920_1080_20M_60_1.h264";
    const char *INP_DIR_2160_30_1 = "/data/test/media/3840_2160_30M_30_1.h264";
    const char *INP_DIR_2160_60_1 = "/data/test/media/3840_2160_30M_60_1.h264";
    int64_t nanosInSecond = 1000000000L;
    int64_t nanosInMicro = 1000L;
};
} // namespace Media
} // namespace OHOS
void SwdecPerfNdkTest::SetUpTestCase() {}
void SwdecPerfNdkTest::TearDownTestCase() {}
void SwdecPerfNdkTest::SetUp() {}

void SwdecPerfNdkTest::TearDown() {}

int64_t SwdecPerfNdkTest::GetSystemTimeUs()
{
    struct timespec now;
    (void)clock_gettime(CLOCK_BOOTTIME, &now);
    int64_t nanoTime = (int64_t)now.tv_sec * nanosInSecond + now.tv_nsec;
    return nanoTime / nanosInMicro;
}

namespace {
/**
 * @tc.number    : VIDEO_SWDEC_PERFORMANCE_0300
 * @tc.name      : surface API time test
 * @tc.desc      : perf test
 */
HWTEST_F(SwdecPerfNdkTest, VIDEO_SWDEC_PERFORMANCE_0300, TestSize.Level3)
{
    shared_ptr<VDecNdkSample> vDecSample = make_shared<VDecNdkSample>();
    vDecSample->INP_DIR = INP_DIR_1080_60;
    vDecSample->OUT_DIR = "/data/test/media/1920_1080_60_out.rgba";
    vDecSample->SURFACE_OUTPUT = true;
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    vDecSample->DEFAULT_FRAME_RATE = 60;
    vDecSample->sleepOnFPS = false;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface("OH.Media.Codec.Decoder.Video.AVC"));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_SWDEC_PERFORMANCE_0400
 * @tc.name      : create 1 decoder(1920*1080 60fps)+2 decoder(1280*720 60fps)
 * @tc.desc      : perf test
 */
HWTEST_F(SwdecPerfNdkTest, VIDEO_SWDEC_PERFORMANCE_0400, TestSize.Level3)
{
    shared_ptr<VDecNdkSample> vDecSample = make_shared<VDecNdkSample>();
    vDecSample->INP_DIR = INP_DIR_1080_60;
    vDecSample->SURFACE_OUTPUT = false;
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    vDecSample->DEFAULT_FRAME_RATE = 60;
    vDecSample->sleepOnFPS = false;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.AVC"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());

    shared_ptr<VDecNdkSample> vDecSample1 = make_shared<VDecNdkSample>();
    vDecSample1->INP_DIR = INP_DIR_720_60;
    vDecSample1->SURFACE_OUTPUT = false;
    vDecSample1->DEFAULT_WIDTH = 1280;
    vDecSample1->DEFAULT_HEIGHT = 720;
    vDecSample1->DEFAULT_FRAME_RATE = 60;
    vDecSample1->sleepOnFPS = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample1->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.AVC"));
    ASSERT_EQ(AV_ERR_OK, vDecSample1->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample1->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample1->StartVideoDecoder());

    shared_ptr<VDecNdkSample> vDecSample2 = make_shared<VDecNdkSample>();
    vDecSample2->SURFACE_OUTPUT = false;
    vDecSample2->INP_DIR = INP_DIR_720_30;
    vDecSample2->DEFAULT_WIDTH = 1280;
    vDecSample2->DEFAULT_HEIGHT = 720;
    vDecSample2->DEFAULT_FRAME_RATE = 30;
    vDecSample2->sleepOnFPS = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample2->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.AVC"));
    ASSERT_EQ(AV_ERR_OK, vDecSample2->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample2->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample2->StartVideoDecoder());

    vDecSample->WaitForEOS();
    vDecSample1->WaitForEOS();
    vDecSample2->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    ASSERT_EQ(AV_ERR_OK, vDecSample1->errCount);
    ASSERT_EQ(AV_ERR_OK, vDecSample2->errCount);
}

/**
 * @tc.number    : VIDEO_SWDEC_PERFORMANCE_0500
 * @tc.name      : decode YUV time 1280*720 30fps 10M
 * @tc.desc      : perf test
 */
HWTEST_F(SwdecPerfNdkTest, VIDEO_SWDEC_PERFORMANCE_0500, TestSize.Level3)
{
    shared_ptr<VDecNdkSample> vDecSample = make_shared<VDecNdkSample>();
    vDecSample->SURFACE_OUTPUT = false;
    vDecSample->INP_DIR = INP_DIR_720_30;
    vDecSample->DEFAULT_WIDTH = 1280;
    vDecSample->DEFAULT_HEIGHT = 720;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->sleepOnFPS = false;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.AVC"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_SWDEC_PERFORMANCE_0600
 * @tc.name      : decode Surface time 1280*720 30fps 10M
 * @tc.desc      : perf test
 */
HWTEST_F(SwdecPerfNdkTest, VIDEO_SWDEC_PERFORMANCE_0600, TestSize.Level3)
{
    shared_ptr<VDecNdkSample> vDecSample = make_shared<VDecNdkSample>();
    vDecSample->INP_DIR = INP_DIR_720_30;
    vDecSample->SURFACE_OUTPUT = true;
    vDecSample->DEFAULT_WIDTH = 1280;
    vDecSample->DEFAULT_HEIGHT = 720;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->sleepOnFPS = false;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface("OH.Media.Codec.Decoder.Video.AVC"));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_SWDEC_PERFORMANCE_0700
 * @tc.name      : decode YUV time 1280*720 60fps 10M
 * @tc.desc      : perf test
 */
HWTEST_F(SwdecPerfNdkTest, VIDEO_SWDEC_PERFORMANCE_0700, TestSize.Level3)
{
    shared_ptr<VDecNdkSample> vDecSample = make_shared<VDecNdkSample>();
    vDecSample->INP_DIR = INP_DIR_720_60;
    vDecSample->SURFACE_OUTPUT = false;
    vDecSample->DEFAULT_WIDTH = 1280;
    vDecSample->DEFAULT_HEIGHT = 720;
    vDecSample->DEFAULT_FRAME_RATE = 60;
    vDecSample->sleepOnFPS = false;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.AVC"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_SWDEC_PERFORMANCE_0800
 * @tc.name      : decode Surface time 1280*720 60fps 10M
 * @tc.desc      : perf test
 */
HWTEST_F(SwdecPerfNdkTest, VIDEO_SWDEC_PERFORMANCE_0800, TestSize.Level3)
{
    shared_ptr<VDecNdkSample> vDecSample = make_shared<VDecNdkSample>();
    vDecSample->INP_DIR = INP_DIR_720_60;
    vDecSample->SURFACE_OUTPUT = true;
    vDecSample->DEFAULT_WIDTH = 1280;
    vDecSample->DEFAULT_HEIGHT = 720;
    vDecSample->DEFAULT_FRAME_RATE = 60;
    vDecSample->sleepOnFPS = false;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface("OH.Media.Codec.Decoder.Video.AVC"));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_SWDEC_PERFORMANCE_0900
 * @tc.name      : decode YUV time 1920*1080 30fps 20M
 * @tc.desc      : perf test
 */
HWTEST_F(SwdecPerfNdkTest, VIDEO_SWDEC_PERFORMANCE_0900, TestSize.Level3)
{
    shared_ptr<VDecNdkSample> vDecSample = make_shared<VDecNdkSample>();
    vDecSample->INP_DIR = INP_DIR_1080_30;
    vDecSample->SURFACE_OUTPUT = false;
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->sleepOnFPS = false;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.AVC"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_SWDEC_PERFORMANCE_1000
 * @tc.name      : decode Surface time 1920*1080 30fps 20M
 * @tc.desc      : perf test
 */
HWTEST_F(SwdecPerfNdkTest, VIDEO_SWDEC_PERFORMANCE_1000, TestSize.Level3)
{
    shared_ptr<VDecNdkSample> vDecSample = make_shared<VDecNdkSample>();
    vDecSample->INP_DIR = INP_DIR_1080_30;
    vDecSample->SURFACE_OUTPUT = true;
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->sleepOnFPS = false;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface("OH.Media.Codec.Decoder.Video.AVC"));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_SWDEC_PERFORMANCE_1100
 * @tc.name      : decode YUV time 1920*1080 60fps 20M
 * @tc.desc      : perf test
 */
HWTEST_F(SwdecPerfNdkTest, VIDEO_SWDEC_PERFORMANCE_1100, TestSize.Level3)
{
    shared_ptr<VDecNdkSample> vDecSample = make_shared<VDecNdkSample>();
    vDecSample->INP_DIR = INP_DIR_1080_60;
    vDecSample->SURFACE_OUTPUT = false;
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    vDecSample->DEFAULT_FRAME_RATE = 60;
    vDecSample->sleepOnFPS = false;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.AVC"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_SWDEC_PERFORMANCE_1200
 * @tc.name      : decode Surface time 1920*1080 60fps 20M
 * @tc.desc      : perf test
 */
HWTEST_F(SwdecPerfNdkTest, VIDEO_SWDEC_PERFORMANCE_1200, TestSize.Level3)
{
    shared_ptr<VDecNdkSample> vDecSample = make_shared<VDecNdkSample>();
    vDecSample->INP_DIR = INP_DIR_1080_60;
    vDecSample->SURFACE_OUTPUT = true;
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    vDecSample->DEFAULT_FRAME_RATE = 60;
    vDecSample->sleepOnFPS = false;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface("OH.Media.Codec.Decoder.Video.AVC"));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_SWDEC_PERFORMANCE_1300
 * @tc.name      : decode YUV time 3840*2160 30fps 50M
 * @tc.desc      : perf test
 */
HWTEST_F(SwdecPerfNdkTest, VIDEO_SWDEC_PERFORMANCE_1300, TestSize.Level3)
{
    shared_ptr<VDecNdkSample> vDecSample = make_shared<VDecNdkSample>();
    vDecSample->INP_DIR = INP_DIR_2160_30;
    vDecSample->SURFACE_OUTPUT = false;
    vDecSample->DEFAULT_WIDTH = 3840;
    vDecSample->DEFAULT_HEIGHT = 2160;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->sleepOnFPS = false;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.AVC"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_SWDEC_PERFORMANCE_1400
 * @tc.name      : decode Surface time 3840*2160 30fps 50M
 * @tc.desc      : perf test
 */
HWTEST_F(SwdecPerfNdkTest, VIDEO_SWDEC_PERFORMANCE_1400, TestSize.Level3)
{
    shared_ptr<VDecNdkSample> vDecSample = make_shared<VDecNdkSample>();
    vDecSample->INP_DIR = INP_DIR_2160_30;
    vDecSample->SURFACE_OUTPUT = true;
    vDecSample->DEFAULT_WIDTH = 3840;
    vDecSample->DEFAULT_HEIGHT = 2160;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->sleepOnFPS = false;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface("OH.Media.Codec.Decoder.Video.AVC"));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_SWDEC_PERFORMANCE_1500
 * @tc.name      : decode YUV time 3840*2160 60fps 50M
 * @tc.desc      : perf test
 */
HWTEST_F(SwdecPerfNdkTest, VIDEO_SWDEC_PERFORMANCE_1500, TestSize.Level3)
{
    shared_ptr<VDecNdkSample> vDecSample = make_shared<VDecNdkSample>();
    vDecSample->INP_DIR = INP_DIR_2160_60;
    vDecSample->SURFACE_OUTPUT = false;
    vDecSample->DEFAULT_WIDTH = 3840;
    vDecSample->DEFAULT_HEIGHT = 2160;
    vDecSample->DEFAULT_FRAME_RATE = 60;
    vDecSample->sleepOnFPS = false;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.AVC"));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_SWDEC_PERFORMANCE_1600
 * @tc.name      : decode Surface time 3840*2160 60fps 50M
 * @tc.desc      : perf test
 */
HWTEST_F(SwdecPerfNdkTest, VIDEO_SWDEC_PERFORMANCE_1600, TestSize.Level3)
{
    shared_ptr<VDecNdkSample> vDecSample = make_shared<VDecNdkSample>();
    vDecSample->INP_DIR = INP_DIR_2160_60;
    vDecSample->SURFACE_OUTPUT = true;
    vDecSample->DEFAULT_WIDTH = 3840;
    vDecSample->DEFAULT_HEIGHT = 2160;
    vDecSample->DEFAULT_FRAME_RATE = 60;
    vDecSample->sleepOnFPS = false;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface("OH.Media.Codec.Decoder.Video.AVC"));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_SWDEC_PERFORMANCE_MORE_0500
 * @tc.name      : decode YUV time 1280*720 30fps 10M
 * @tc.desc      : perf test
 */
HWTEST_F(SwdecPerfNdkTest, VIDEO_SWDEC_PERFORMANCE_MORE_0500, TestSize.Level3)
{
    for (int i = 0; i < 2; i++) {
        VDecNdkSample *vDecSample = new VDecNdkSample();
        vDecSample->SURFACE_OUTPUT = false;
        if (i == 1) {
            vDecSample->INP_DIR = INP_DIR_720_30_1;
        } else {
            vDecSample->INP_DIR = INP_DIR_720_30;
        }
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->sleepOnFPS = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.AVC"));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        if (i == 1) {
            vDecSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
        }
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_PERFORMANCE_MORE_0600
 * @tc.name      : decode Surface time 1280*720 30fps 10M
 * @tc.desc      : perf test
 */
HWTEST_F(SwdecPerfNdkTest, VIDEO_SWDEC_PERFORMANCE_MORE_0600, TestSize.Level3)
{
    for (int i = 0; i < 2; i++) {
        VDecNdkSample *vDecSample = new VDecNdkSample();
        if (i == 1) {
            vDecSample->INP_DIR = INP_DIR_720_30;
        } else {
            vDecSample->INP_DIR = INP_DIR_720_30_1;
        }
        vDecSample->SURFACE_OUTPUT = true;
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->sleepOnFPS = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface("OH.Media.Codec.Decoder.Video.AVC"));
        if (i == 1) {
            vDecSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
        }
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_PERFORMANCE_MORE_0700
 * @tc.name      : decode YUV time 1280*720 60fps 10M
 * @tc.desc      : perf test
 */
HWTEST_F(SwdecPerfNdkTest, VIDEO_SWDEC_PERFORMANCE_MORE_0700, TestSize.Level3)
{
    for (int i = 0; i < 2; i++) {
        VDecNdkSample *vDecSample = new VDecNdkSample();
        vDecSample->SURFACE_OUTPUT = false;
        if (i == 1) {
            vDecSample->INP_DIR = INP_DIR_720_60_1;
        } else {
            vDecSample->INP_DIR = INP_DIR_720_60;
        }
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->sleepOnFPS = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.AVC"));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        if (i == 1) {
            vDecSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
        }
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_PERFORMANCE_MORE_0800
 * @tc.name      : decode Surface time 1280*720 60fps 10M
 * @tc.desc      : perf test
 */
HWTEST_F(SwdecPerfNdkTest, VIDEO_SWDEC_PERFORMANCE_MORE_0800, TestSize.Level3)
{
    for (int i = 0; i < 2; i++) {
        VDecNdkSample *vDecSample = new VDecNdkSample();
        if (i == 1) {
            vDecSample->INP_DIR = INP_DIR_720_60;
        } else {
            vDecSample->INP_DIR = INP_DIR_720_60_1;
        }
        vDecSample->SURFACE_OUTPUT = true;
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->sleepOnFPS = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface("OH.Media.Codec.Decoder.Video.AVC"));
        if (i == 1) {
            vDecSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
        }
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_PERFORMANCE_MORE_0900
 * @tc.name      : decode YUV time 1920*1080 30fps 20M
 * @tc.desc      : perf test
 */
HWTEST_F(SwdecPerfNdkTest, VIDEO_SWDEC_PERFORMANCE_MORE_0900, TestSize.Level3)
{
    for (int i = 0; i < 2; i++) {
        VDecNdkSample *vDecSample = new VDecNdkSample();
        vDecSample->SURFACE_OUTPUT = false;
        if (i == 1) {
            vDecSample->INP_DIR = INP_DIR_1080_30_1;
        } else {
            vDecSample->INP_DIR = INP_DIR_1080_30;
        }
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->sleepOnFPS = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.AVC"));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        if (i == 1) {
            vDecSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
        }
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_PERFORMANCE_MORE_1000
 * @tc.name      : decode Surface time 1920*1080 30fps 20M
 * @tc.desc      : perf test
 */
HWTEST_F(SwdecPerfNdkTest, VIDEO_SWDEC_PERFORMANCE_MORE_1000, TestSize.Level3)
{
    for (int i = 0; i < 2; i++) {
        VDecNdkSample *vDecSample = new VDecNdkSample();
        if (i == 1) {
            vDecSample->INP_DIR = INP_DIR_1080_30;
        } else {
            vDecSample->INP_DIR = INP_DIR_1080_30_1;
        }
        vDecSample->SURFACE_OUTPUT = true;
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->sleepOnFPS = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface("OH.Media.Codec.Decoder.Video.AVC"));
        if (i == 1) {
            vDecSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
        }
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_PERFORMANCE_MORE_1100
 * @tc.name      : decode YUV time 1920*1080 60fps 20M
 * @tc.desc      : perf test
 */
HWTEST_F(SwdecPerfNdkTest, VIDEO_SWDEC_PERFORMANCE_MORE_1100, TestSize.Level3)
{
    for (int i = 0; i < 2; i++) {
        VDecNdkSample *vDecSample = new VDecNdkSample();
        vDecSample->SURFACE_OUTPUT = false;
        if (i == 1) {
            vDecSample->INP_DIR = INP_DIR_1080_60_1;
        } else {
            vDecSample->INP_DIR = INP_DIR_1080_60;
        }
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->sleepOnFPS = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.AVC"));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        if (i == 1) {
            vDecSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
        }
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_PERFORMANCE_MORE_1200
 * @tc.name      : decode Surface time 1920*1080 60fps 20M
 * @tc.desc      : perf test
 */
HWTEST_F(SwdecPerfNdkTest, VIDEO_SWDEC_PERFORMANCE_MORE_1200, TestSize.Level3)
{
    for (int i = 0; i < 2; i++) {
        VDecNdkSample *vDecSample = new VDecNdkSample();
        if (i == 1) {
            vDecSample->INP_DIR = INP_DIR_1080_60;
        } else {
            vDecSample->INP_DIR = INP_DIR_1080_60_1;
        }
        vDecSample->SURFACE_OUTPUT = true;
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->sleepOnFPS = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface("OH.Media.Codec.Decoder.Video.AVC"));
        if (i == 1) {
            vDecSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
        }
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_PERFORMANCE_MORE_1300
 * @tc.name      : decode YUV time 3840*2160 30fps 50M
 * @tc.desc      : perf test
 */
HWTEST_F(SwdecPerfNdkTest, VIDEO_SWDEC_PERFORMANCE_MORE_1300, TestSize.Level3)
{
    for (int i = 0; i < 2; i++) {
        VDecNdkSample *vDecSample = new VDecNdkSample();
        vDecSample->SURFACE_OUTPUT = false;
        if (i == 1) {
            vDecSample->INP_DIR = INP_DIR_2160_30_1;
        } else {
            vDecSample->INP_DIR = INP_DIR_2160_30;
        }
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->sleepOnFPS = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.AVC"));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        if (i == 1) {
            vDecSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
        }
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_PERFORMANCE_MORE_1400
 * @tc.name      : decode Surface time 3840*2160 30fps 50M
 * @tc.desc      : perf test
 */
HWTEST_F(SwdecPerfNdkTest, VIDEO_SWDEC_PERFORMANCE_MORE_1400, TestSize.Level3)
{
    for (int i = 0; i < 2; i++) {
        VDecNdkSample *vDecSample = new VDecNdkSample();
        if (i == 1) {
            vDecSample->INP_DIR = INP_DIR_2160_30;
        } else {
            vDecSample->INP_DIR = INP_DIR_2160_30_1;
        }
        vDecSample->SURFACE_OUTPUT = true;
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->sleepOnFPS = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface("OH.Media.Codec.Decoder.Video.AVC"));
        if (i == 1) {
            vDecSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
        }
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_PERFORMANCE_MORE_1500
 * @tc.name      : decode YUV time 3840*2160 60fps 50M
 * @tc.desc      : perf test
 */
HWTEST_F(SwdecPerfNdkTest, VIDEO_SWDEC_PERFORMANCE_MORE_1500, TestSize.Level3)
{
    for (int i = 0; i < 2; i++) {
        VDecNdkSample *vDecSample = new VDecNdkSample();
        vDecSample->SURFACE_OUTPUT = false;
        if (i == 1) {
            vDecSample->INP_DIR = INP_DIR_2160_60_1;
        } else {
            vDecSample->INP_DIR = INP_DIR_2160_60;
        }
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->sleepOnFPS = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.AVC"));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        if (i == 1) {
            vDecSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
        }
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_PERFORMANCE_MORE_1600
 * @tc.name      : decode Surface time 3840*2160 60fps 50M
 * @tc.desc      : perf test
 */
HWTEST_F(SwdecPerfNdkTest, VIDEO_SWDEC_PERFORMANCE_MORE_1600, TestSize.Level3)
{
    for (int i = 0; i < 2; i++) {
        VDecNdkSample *vDecSample = new VDecNdkSample();
        if (i == 1) {
            vDecSample->INP_DIR = INP_DIR_2160_60;
        } else {
            vDecSample->INP_DIR = INP_DIR_2160_60_1;
        }
        vDecSample->SURFACE_OUTPUT = true;
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->sleepOnFPS = false;
        ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec_Surface("OH.Media.Codec.Decoder.Video.AVC"));
        if (i == 1) {
            vDecSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
        }
    }
}
} // namespace