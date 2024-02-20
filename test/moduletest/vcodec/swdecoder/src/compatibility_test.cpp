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
using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;
namespace OHOS {
namespace Media {
class SwdecCompNdkTest : public testing::Test {
public:
    // SetUpTestCase: Called before all test cases
    static void SetUpTestCase(void);
    // TearDownTestCase: Called after all test case
    static void TearDownTestCase(void);
    // SetUp: Called before each test cases
    void SetUp(void);
    // TearDown: Called after each test cases
    void TearDown(void);

protected:
    const ::testing::TestInfo *testInfo_ = nullptr;
    bool createCodecSuccess_ = false;
};

void SwdecCompNdkTest::SetUpTestCase(void) {}
void SwdecCompNdkTest::TearDownTestCase(void) {}
void SwdecCompNdkTest::SetUp(void) {}
void SwdecCompNdkTest::TearDown(void) {}
} // namespace Media
} // namespace OHOS

namespace {
/**
 * @tc.number    : VIDEO_SWDEC_H264_IPB_0100
 * @tc.name      : software decode all idr frame
 * @tc.desc      : function test
 */
HWTEST_F(SwdecCompNdkTest, VIDEO_SWDEC_H264_IPB_0100, TestSize.Level0)
{
    VDecNdkSample *vDecSample = new VDecNdkSample();

    int32_t ret = vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.AVC");
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->SetVideoDecoderCallback();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->ConfigureVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample->INP_DIR = "/data/test/media/1920x1080I.h264";
    ret = vDecSample->StartVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    vDecSample->Stop();
    vDecSample->Release();
}

/**
 * @tc.number    : VIDEO_SWDEC_H264_IPB_0200
 * @tc.name      : software decode single idr frame
 * @tc.desc      : function test
 */
HWTEST_F(SwdecCompNdkTest, VIDEO_SWDEC_H264_IPB_0200, TestSize.Level0)
{
    VDecNdkSample *vDecSample = new VDecNdkSample();

    int32_t ret = vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.AVC");
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->SetVideoDecoderCallback();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->ConfigureVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample->INP_DIR = "/data/test/media/1920x1080IP.h264";
    ret = vDecSample->StartVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    vDecSample->Stop();
    vDecSample->Release();
}

/**
 * @tc.number    : VIDEO_SWDEC_H264_IPB_0300
 * @tc.name      : software decode all idr frame
 * @tc.desc      : function test
 */
HWTEST_F(SwdecCompNdkTest, VIDEO_SWDEC_H264_IPB_0300, TestSize.Level0)
{
    VDecNdkSample *vDecSample = new VDecNdkSample();

    int32_t ret = vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.AVC");
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->SetVideoDecoderCallback();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->ConfigureVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample->INP_DIR = "/data/test/media/1920x1080IPB.h264";
    ret = vDecSample->StartVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    vDecSample->Stop();
    vDecSample->Release();
}

/**
 * @tc.number    : VIDEO_SWDEC_H264_SVC_0100
 * @tc.name      : software decode all type frame
 * @tc.desc      : function test
 */
HWTEST_F(SwdecCompNdkTest, VIDEO_SWDEC_H264_SVC_0100, TestSize.Level0)
{
    VDecNdkSample *vDecSample = new VDecNdkSample();

    int32_t ret = vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.AVC");
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->SetVideoDecoderCallback();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->ConfigureVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample->INP_DIR = "/data/test/media/1920x1080IPB.h264";
    ret = vDecSample->StartVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    vDecSample->Stop();
    vDecSample->Release();
}

/**
 * @tc.number    : VIDEO_SWDEC_H264_01_0200
 * @tc.name      : software decode frame
 * @tc.desc      : function test
 */
HWTEST_F(SwdecCompNdkTest, VIDEO_SWDEC_H264_01_0200, TestSize.Level0)
{
    VDecNdkSample *vDecSample = new VDecNdkSample();

    int32_t ret = vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.AVC");
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->SetVideoDecoderCallback();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->ConfigureVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample->INP_DIR = "/data/test/media/3840x2160_60_10Mb.h264";
    ret = vDecSample->StartVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    vDecSample->Stop();
    vDecSample->Release();
}

/**
 * @tc.number    : VIDEO_SWDEC_H264_01_0300
 * @tc.name      : software decode frame
 * @tc.desc      : function test
 */
HWTEST_F(SwdecCompNdkTest, VIDEO_SWDEC_H264_01_0300, TestSize.Level0)
{
    VDecNdkSample *vDecSample = new VDecNdkSample();

    int32_t ret = vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.AVC");
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->SetVideoDecoderCallback();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->ConfigureVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample->INP_DIR = "/data/test/media/1920x1080_60_10Mb.h264";
    ret = vDecSample->StartVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    vDecSample->Stop();
    vDecSample->Release();
}

/**
 * @tc.number    : VIDEO_SWDEC_H264_01_0400
 * @tc.name      : software decode frame
 * @tc.desc      : function test
 */
HWTEST_F(SwdecCompNdkTest, VIDEO_SWDEC_H264_01_0400, TestSize.Level0)
{
    VDecNdkSample *vDecSample = new VDecNdkSample();

    int32_t ret = vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.AVC");
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->SetVideoDecoderCallback();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->ConfigureVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample->INP_DIR = "/data/test/media/1504x720_60_10Mb.h264";
    ret = vDecSample->StartVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    vDecSample->Stop();
    vDecSample->Release();
}

/**
 * @tc.number    : VIDEO_SWDEC_H264_01_0500
 * @tc.name      : software decode frame
 * @tc.desc      : function test
 */
HWTEST_F(SwdecCompNdkTest, VIDEO_SWDEC_H264_01_0500, TestSize.Level0)
{
    VDecNdkSample *vDecSample = new VDecNdkSample();

    int32_t ret = vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.AVC");
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->SetVideoDecoderCallback();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->ConfigureVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample->INP_DIR = "/data/test/media/1440x1080_60_10Mb.h264";
    ret = vDecSample->StartVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    vDecSample->Stop();
    vDecSample->Release();
}

/**
 * @tc.number    : VIDEO_SWDEC_H264_01_0600
 * @tc.name      : software decode frame
 * @tc.desc      : function test
 */
HWTEST_F(SwdecCompNdkTest, VIDEO_SWDEC_H264_01_0600, TestSize.Level0)
{
    VDecNdkSample *vDecSample = new VDecNdkSample();

    int32_t ret = vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.AVC");
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->SetVideoDecoderCallback();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->ConfigureVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample->INP_DIR = "/data/test/media/1280x720_60_10Mb.h264";
    ret = vDecSample->StartVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    vDecSample->Stop();
    vDecSample->Release();
}

/**
 * @tc.number    : VIDEO_SWDEC_H264_01_0700
 * @tc.name      : software decode frame
 * @tc.desc      : function test
 */
HWTEST_F(SwdecCompNdkTest, VIDEO_SWDEC_H264_01_0700, TestSize.Level0)
{
    VDecNdkSample *vDecSample = new VDecNdkSample();

    int32_t ret = vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.AVC");
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->SetVideoDecoderCallback();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->ConfigureVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample->INP_DIR = "/data/test/media/1232x768_60_10Mb.h264";
    ret = vDecSample->StartVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    vDecSample->Stop();
    vDecSample->Release();
}

/**
 * @tc.number    : VIDEO_SWDEC_H264_01_0800
 * @tc.name      : software decode frame
 * @tc.desc      : function test
 */
HWTEST_F(SwdecCompNdkTest, VIDEO_SWDEC_H264_01_0800, TestSize.Level0)
{
    VDecNdkSample *vDecSample = new VDecNdkSample();

    int32_t ret = vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.AVC");
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->SetVideoDecoderCallback();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->ConfigureVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample->INP_DIR = "/data/test/media/1152x720_60_10Mb.h264";
    ret = vDecSample->StartVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    vDecSample->Stop();
    vDecSample->Release();
}

/**
 * @tc.number    : VIDEO_SWDEC_H264_01_0900
 * @tc.name      : software decode frame
 * @tc.desc      : function test
 */
HWTEST_F(SwdecCompNdkTest, VIDEO_SWDEC_H264_01_0900, TestSize.Level0)
{
    VDecNdkSample *vDecSample = new VDecNdkSample();

    int32_t ret = vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.AVC");
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->SetVideoDecoderCallback();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->ConfigureVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample->INP_DIR = "/data/test/media/960x720_60_10Mb.h264";
    ret = vDecSample->StartVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    vDecSample->Stop();
    vDecSample->Release();
}

/**
 * @tc.number    : VIDEO_SWDEC_H264_01_1000
 * @tc.name      : software decode frame
 * @tc.desc      : function test
 */
HWTEST_F(SwdecCompNdkTest, VIDEO_SWDEC_H264_01_1000, TestSize.Level0)
{
    VDecNdkSample *vDecSample = new VDecNdkSample();

    int32_t ret = vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.AVC");
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->SetVideoDecoderCallback();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->ConfigureVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample->INP_DIR = "/data/test/media/960x544_60_10Mb.h264";
    ret = vDecSample->StartVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    vDecSample->Stop();
    vDecSample->Release();
}

/**
 * @tc.number    : VIDEO_SWDEC_H264_01_1100
 * @tc.name      : software decode frame
 * @tc.desc      : function test
 */
HWTEST_F(SwdecCompNdkTest, VIDEO_SWDEC_H264_01_1100, TestSize.Level0)
{
    VDecNdkSample *vDecSample = new VDecNdkSample();

    int32_t ret = vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.AVC");
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->SetVideoDecoderCallback();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->ConfigureVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample->INP_DIR = "/data/test/media/880x720_60_10Mb.h264";
    ret = vDecSample->StartVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    vDecSample->Stop();
    vDecSample->Release();
}

/**
 * @tc.number    : VIDEO_SWDEC_H264_01_1200
 * @tc.name      : software decode frame
 * @tc.desc      : function test
 */
HWTEST_F(SwdecCompNdkTest, VIDEO_SWDEC_H264_01_1200, TestSize.Level0)
{
    VDecNdkSample *vDecSample = new VDecNdkSample();

    int32_t ret = vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.AVC");
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->SetVideoDecoderCallback();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->ConfigureVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample->INP_DIR = "/data/test/media/720x720_60_10Mb.h264";
    ret = vDecSample->StartVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    vDecSample->Stop();
    vDecSample->Release();
}

/**
 * @tc.number    : VIDEO_SWDEC_H264_01_1300
 * @tc.name      : software decode frame
 * @tc.desc      : function test
 */
HWTEST_F(SwdecCompNdkTest, VIDEO_SWDEC_H264_01_1300, TestSize.Level0)
{
    VDecNdkSample *vDecSample = new VDecNdkSample();

    int32_t ret = vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.AVC");
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->SetVideoDecoderCallback();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->ConfigureVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample->INP_DIR = "/data/test/media/720x480_60_10Mb.h264";
    ret = vDecSample->StartVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    vDecSample->Stop();
    vDecSample->Release();
}

/**
 * @tc.number    : VIDEO_SWDEC_H264_01_1400
 * @tc.name      : software decode frame
 * @tc.desc      : function test
 */
HWTEST_F(SwdecCompNdkTest, VIDEO_SWDEC_H264_01_1400, TestSize.Level0)
{
    VDecNdkSample *vDecSample = new VDecNdkSample();

    int32_t ret = vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.AVC");
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->SetVideoDecoderCallback();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->ConfigureVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample->INP_DIR = "/data/test/media/640x480_60_10Mb.h264";
    ret = vDecSample->StartVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    vDecSample->Stop();
    vDecSample->Release();
}

/**
 * @tc.number    : VIDEO_SWDEC_H264_01_1500
 * @tc.name      : software decode frame
 * @tc.desc      : function test
 */
HWTEST_F(SwdecCompNdkTest, VIDEO_SWDEC_H264_01_1500, TestSize.Level0)
{
    VDecNdkSample *vDecSample = new VDecNdkSample();

    int32_t ret = vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.AVC");
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->SetVideoDecoderCallback();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->ConfigureVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample->INP_DIR = "/data/test/media/320x240_60_10Mb.h264";
    ret = vDecSample->StartVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    vDecSample->Stop();
    vDecSample->Release();
}

/**
 * @tc.number    : VIDEO_SWDEC_H264_01_1600
 * @tc.name      : software decode frame
 * @tc.desc      : function test
 */
HWTEST_F(SwdecCompNdkTest, VIDEO_SWDEC_H264_01_1600, TestSize.Level0)
{
    VDecNdkSample *vDecSample = new VDecNdkSample();

    int32_t ret = vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.AVC");
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->SetVideoDecoderCallback();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->ConfigureVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample->INP_DIR = "/data/test/media/352x288_60_10Mb.h264";
    ret = vDecSample->StartVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    vDecSample->Stop();
    vDecSample->Release();
}

/**
 * @tc.number    : VIDEO_SWDEC_H264_02_0100
 * @tc.name      : software decode frame
 * @tc.desc      : function test
 */
HWTEST_F(SwdecCompNdkTest, VIDEO_SWDEC_H264_02_0100, TestSize.Level0)
{
    VDecNdkSample *vDecSample = new VDecNdkSample();

    int32_t ret = vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.AVC");
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->SetVideoDecoderCallback();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->ConfigureVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample->INP_DIR = "/data/test/media/1920x1080_60_30Mb.h264";
    ret = vDecSample->StartVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    vDecSample->Stop();
    vDecSample->Release();
}

/**
 * @tc.number    : VIDEO_SWDEC_H264_02_0200
 * @tc.name      : software decode frame
 * @tc.desc      : function test
 */
HWTEST_F(SwdecCompNdkTest, VIDEO_SWDEC_H264_02_0200, TestSize.Level0)
{
    VDecNdkSample *vDecSample = new VDecNdkSample();

    int32_t ret = vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.AVC");
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->SetVideoDecoderCallback();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->ConfigureVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample->INP_DIR = "/data/test/media/1920x1080_30_30Mb.h264";
    ret = vDecSample->StartVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    vDecSample->Stop();
    vDecSample->Release();
}

/**
 * @tc.number    : VIDEO_SWDEC_H264_02_0300
 * @tc.name      : software decode frame
 * @tc.desc      : function test
 */
HWTEST_F(SwdecCompNdkTest, VIDEO_SWDEC_H264_02_0300, TestSize.Level0)
{
    VDecNdkSample *vDecSample = new VDecNdkSample();

    int32_t ret = vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.AVC");
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->SetVideoDecoderCallback();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->ConfigureVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample->INP_DIR = "/data/test/media/1920x1080_10_30Mb.h264";
    ret = vDecSample->StartVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    vDecSample->Stop();
    vDecSample->Release();
}

/**
 * @tc.number    : VIDEO_SWDEC_H264_03_0100
 * @tc.name      : software decode frame
 * @tc.desc      : function test
 */
HWTEST_F(SwdecCompNdkTest, VIDEO_SWDEC_H264_03_0100, TestSize.Level0)
{
    VDecNdkSample *vDecSample = new VDecNdkSample();

    int32_t ret = vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.AVC");
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->SetVideoDecoderCallback();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->ConfigureVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample->INP_DIR = "/data/test/media/1920x1080_60_30Mb.h264";
    ret = vDecSample->StartVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    vDecSample->Stop();
    vDecSample->Release();
}

/**
 * @tc.number    : VIDEO_SWDEC_H264_03_0200
 * @tc.name      : software decode frame
 * @tc.desc      : function test
 */
HWTEST_F(SwdecCompNdkTest, VIDEO_SWDEC_H264_03_0200, TestSize.Level0)
{
    VDecNdkSample *vDecSample = new VDecNdkSample();

    int32_t ret = vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.AVC");
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->SetVideoDecoderCallback();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->ConfigureVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample->INP_DIR = "/data/test/media/1920x1080_60_30Mb.h264";
    ret = vDecSample->StartVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    vDecSample->Stop();
    vDecSample->Release();
}

/**
 * @tc.number    : VIDEO_SWDEC_H264_03_0300
 * @tc.name      : software decode frame
 * @tc.desc      : function test
 */
HWTEST_F(SwdecCompNdkTest, VIDEO_SWDEC_H264_03_0300, TestSize.Level0)
{
    VDecNdkSample *vDecSample = new VDecNdkSample();

    int32_t ret = vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.AVC");
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->SetVideoDecoderCallback();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->ConfigureVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample->INP_DIR = "/data/test/media/1920x1080_60_20Mb.h264";
    ret = vDecSample->StartVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    vDecSample->Stop();
    vDecSample->Release();
}

/**
 * @tc.number    : VIDEO_SWDEC_H264_03_0400
 * @tc.name      : software decode frame
 * @tc.desc      : function test
 */
HWTEST_F(SwdecCompNdkTest, VIDEO_SWDEC_H264_03_0400, TestSize.Level0)
{
    VDecNdkSample *vDecSample = new VDecNdkSample();

    int32_t ret = vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.AVC");
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->SetVideoDecoderCallback();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->ConfigureVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample->INP_DIR = "/data/test/media/1920x1080_60_3Mb.h264";
    ret = vDecSample->StartVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    vDecSample->Stop();
    vDecSample->Release();
}

/**
 * @tc.number    : VIDEO_SWDEC_H264_03_0500
 * @tc.name      : software decode frame
 * @tc.desc      : function test
 */
HWTEST_F(SwdecCompNdkTest, VIDEO_SWDEC_H264_03_0500, TestSize.Level0)
{
    VDecNdkSample *vDecSample = new VDecNdkSample();

    int32_t ret = vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.AVC");
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->SetVideoDecoderCallback();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->ConfigureVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample->INP_DIR = "/data/test/media/1920x1080_60_2Mb.h264";
    ret = vDecSample->StartVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    vDecSample->Stop();
    vDecSample->Release();
}

/**
 * @tc.number    : VIDEO_SWDEC_H264_03_0600
 * @tc.name      : software decode frame
 * @tc.desc      : function test
 */
HWTEST_F(SwdecCompNdkTest, VIDEO_SWDEC_H264_03_0600, TestSize.Level0)
{
    VDecNdkSample *vDecSample = new VDecNdkSample();

    int32_t ret = vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.AVC");
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->SetVideoDecoderCallback();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->ConfigureVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample->INP_DIR = "/data/test/media/1920x1080_60_1Mb.h264";
    ret = vDecSample->StartVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample->WaitForEOS();
    ASSERT_EQ(0, vDecSample->errCount);
    vDecSample->Stop();
    vDecSample->Release();
}
} // namespace