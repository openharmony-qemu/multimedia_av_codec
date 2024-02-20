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
#include "avcodec_codec_name.h"
#include "videodec_ndk_sample.h"
#include "native_avcapability.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;
namespace OHOS {
namespace Media {
class HwdecNdkTest : public testing::Test {
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

string g_codecName;
OH_AVCapability *cap = nullptr;

void HwdecNdkTest::SetUpTestCase(void)
{
    cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_AVC, false, HARDWARE);
    g_codecName = OH_AVCapability_GetName(cap);
}
void HwdecNdkTest::TearDownTestCase(void) {}
void HwdecNdkTest::SetUp(void) {}
void HwdecNdkTest::TearDown(void) {}
} // namespace Media
} // namespace OHOS

namespace {
/**
 * @tc.number    : VIDEO_HWDEC_H264_IPB_0100
 * @tc.name      : hardware decode all idr frame
 * @tc.desc      : function test
 */
HWTEST_F(HwdecNdkTest, VIDEO_HWDEC_H264_IPB_0100, TestSize.Level0)
{
    shared_ptr<VDecNdkSample> vDecSample = make_shared<VDecNdkSample>();
    vDecSample->INP_DIR = "/data/test/media/1920_1080I.h264";
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->SF_OUTPUT = false;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecName));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_HWDEC_H264_IPB_0200
 * @tc.name      : hardware decode single idr frame
 * @tc.desc      : function test
 */
HWTEST_F(HwdecNdkTest, VIDEO_HWDEC_H264_IPB_0200, TestSize.Level0)
{
    shared_ptr<VDecNdkSample> vDecSample = make_shared<VDecNdkSample>();
    vDecSample->INP_DIR = "/data/test/media/1920_1080IP.h264";
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->SF_OUTPUT = false;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecName));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_HWDEC_H264_IPB_0300
 * @tc.name      : hardware decode all type frame
 * @tc.desc      : function test
 */
HWTEST_F(HwdecNdkTest, VIDEO_HWDEC_H264_IPB_0300, TestSize.Level0)
{
    shared_ptr<VDecNdkSample> vDecSample = make_shared<VDecNdkSample>();
    vDecSample->INP_DIR = "/data/test/media/1920_1080IPB.h264";
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->SF_OUTPUT = false;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecName));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_HWDEC_H264_SVC_0100
 * @tc.name      : hardware decode svc frame
 * @tc.desc      : function test
 */
HWTEST_F(HwdecNdkTest, VIDEO_HWDEC_H264_SVC_0100, TestSize.Level0)
{
    shared_ptr<VDecNdkSample> vDecSample = make_shared<VDecNdkSample>();
    vDecSample->INP_DIR = "/data/test/media/1920_1080_svc_avcc.h264";
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->SF_OUTPUT = false;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecName));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_HWDEC_H264_01_0100
 * @tc.name      : hardware decode 4k frame
 * @tc.desc      : function test
 */
HWTEST_F(HwdecNdkTest, VIDEO_HWDEC_H264_01_0100, TestSize.Level0)
{
    shared_ptr<VDecNdkSample> vDecSample = make_shared<VDecNdkSample>();
    vDecSample->INP_DIR = "/data/test/media/4096_2160_60_30Mb.h264";
    vDecSample->DEFAULT_WIDTH = 3840;
    vDecSample->DEFAULT_HEIGHT = 2160;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->SF_OUTPUT = false;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecName));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_HWDEC_H264_01_0200
 * @tc.name      : hardware decode 4k frame
 * @tc.desc      : function test
 */
HWTEST_F(HwdecNdkTest, VIDEO_HWDEC_H264_01_0200, TestSize.Level0)
{
    shared_ptr<VDecNdkSample> vDecSample = make_shared<VDecNdkSample>();
    vDecSample->INP_DIR = "/data/test/media/3840_2160_60_10M.h264";
    vDecSample->DEFAULT_WIDTH = 3840;
    vDecSample->DEFAULT_HEIGHT = 2160;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->SF_OUTPUT = false;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecName));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_HWDEC_H264_01_0300
 * @tc.name      : hardware decode 1080p frame
 * @tc.desc      : function test
 */
HWTEST_F(HwdecNdkTest, VIDEO_HWDEC_H264_01_0300, TestSize.Level0)
{
    shared_ptr<VDecNdkSample> vDecSample = make_shared<VDecNdkSample>();
    vDecSample->INP_DIR = "/data/test/media/1920_1080_60_10M_IP.h264";
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->SF_OUTPUT = false;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecName));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_HWDEC_H264_01_0400
 * @tc.name      : hardware decode non-standard resolution frame
 * @tc.desc      : function test
 */
HWTEST_F(HwdecNdkTest, VIDEO_HWDEC_H264_01_0400, TestSize.Level0)
{
    shared_ptr<VDecNdkSample> vDecSample = make_shared<VDecNdkSample>();
    vDecSample->INP_DIR = "/data/test/media/1504_720_60_10Mb.h264";
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->SF_OUTPUT = false;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecName));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_HWDEC_H264_01_0500
 * @tc.name      : hardware decode non-standard resolution frame
 * @tc.desc      : function test
 */
HWTEST_F(HwdecNdkTest, VIDEO_HWDEC_H264_01_0500, TestSize.Level0)
{
    shared_ptr<VDecNdkSample> vDecSample = make_shared<VDecNdkSample>();
    vDecSample->INP_DIR = "/data/test/media/1440_1080_60_10Mb.h264";
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->SF_OUTPUT = false;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecName));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_HWDEC_H264_01_0600
 * @tc.name      : hardware decode 720p frame
 * @tc.desc      : function test
 */
HWTEST_F(HwdecNdkTest, VIDEO_HWDEC_H264_01_0600, TestSize.Level0)
{
    shared_ptr<VDecNdkSample> vDecSample = make_shared<VDecNdkSample>();
    vDecSample->INP_DIR = "/data/test/media/1280_720_60_10M.h264";
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->SF_OUTPUT = false;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecName));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_HWDEC_H264_01_0700
 * @tc.name      : hardware decode non-standard resolution frame
 * @tc.desc      : function test
 */
HWTEST_F(HwdecNdkTest, VIDEO_HWDEC_H264_01_0700, TestSize.Level0)
{
    shared_ptr<VDecNdkSample> vDecSample = make_shared<VDecNdkSample>();
    vDecSample->INP_DIR = "/data/test/media/1232_768_60_10Mb.h264";
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->SF_OUTPUT = false;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecName));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_HWDEC_H264_01_0800
 * @tc.name      : hardware decode non-standard resolution frame
 * @tc.desc      : function test
 */
HWTEST_F(HwdecNdkTest, VIDEO_HWDEC_H264_01_0800, TestSize.Level0)
{
    shared_ptr<VDecNdkSample> vDecSample = make_shared<VDecNdkSample>();
    vDecSample->INP_DIR = "/data/test/media/1152_720_60_10Mb.h264";
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->SF_OUTPUT = false;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecName));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_HWDEC_H264_01_0900
 * @tc.name      : hardware decode non-standard resolution frame
 * @tc.desc      : function test
 */
HWTEST_F(HwdecNdkTest, VIDEO_HWDEC_H264_01_0900, TestSize.Level0)
{
    shared_ptr<VDecNdkSample> vDecSample = make_shared<VDecNdkSample>();
    vDecSample->INP_DIR = "/data/test/media/960_720_60_10Mb.h264";
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->SF_OUTPUT = false;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecName));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_HWDEC_H264_01_1000
 * @tc.name      : hardware decode non-standard resolution frame
 * @tc.desc      : function test
 */
HWTEST_F(HwdecNdkTest, VIDEO_HWDEC_H264_01_1000, TestSize.Level0)
{
    shared_ptr<VDecNdkSample> vDecSample = make_shared<VDecNdkSample>();
    vDecSample->INP_DIR = "/data/test/media/960_544_60_10Mb.h264";
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->SF_OUTPUT = false;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecName));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_HWDEC_H264_01_1100
 * @tc.name      : hardware decode non-standard resolution frame
 * @tc.desc      : function test
 */
HWTEST_F(HwdecNdkTest, VIDEO_HWDEC_H264_01_1100, TestSize.Level0)
{
    shared_ptr<VDecNdkSample> vDecSample = make_shared<VDecNdkSample>();
    vDecSample->INP_DIR = "/data/test/media/880_720_60_10Mb.h264";
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->SF_OUTPUT = false;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecName));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_HWDEC_H264_01_1200
 * @tc.name      : hardware decode non-standard resolution frame
 * @tc.desc      : function test
 */
HWTEST_F(HwdecNdkTest, VIDEO_HWDEC_H264_01_1200, TestSize.Level0)
{
    shared_ptr<VDecNdkSample> vDecSample = make_shared<VDecNdkSample>();
    vDecSample->INP_DIR = "/data/test/media/720_720_60_10Mb.h264";
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->SF_OUTPUT = false;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecName));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_HWDEC_H264_01_1300
 * @tc.name      : hardware decode 480p frame
 * @tc.desc      : function test
 */
HWTEST_F(HwdecNdkTest, VIDEO_HWDEC_H264_01_1300, TestSize.Level0)
{
    shared_ptr<VDecNdkSample> vDecSample = make_shared<VDecNdkSample>();
    vDecSample->INP_DIR = "/data/test/media/720_480_60_10Mb.h264";
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->SF_OUTPUT = false;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecName));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_HWDEC_H264_01_1400
 * @tc.name      : hardware decode non-standard resolution frame
 * @tc.desc      : function test
 */
HWTEST_F(HwdecNdkTest, VIDEO_HWDEC_H264_01_1400, TestSize.Level0)
{
    shared_ptr<VDecNdkSample> vDecSample = make_shared<VDecNdkSample>();
    vDecSample->INP_DIR = "/data/test/media/640_480_60_10Mb.h264";
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->SF_OUTPUT = false;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecName));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_HWDEC_H264_01_1500
 * @tc.name      : hardware decode non-standard resolution frame
 * @tc.desc      : function test
 */
HWTEST_F(HwdecNdkTest, VIDEO_HWDEC_H264_01_1500, TestSize.Level0)
{
    shared_ptr<VDecNdkSample> vDecSample = make_shared<VDecNdkSample>();
    vDecSample->INP_DIR = "/data/test/media/320_240_60_10Mb.h264";
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->SF_OUTPUT = false;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecName));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_HWDEC_H264_01_1600
 * @tc.name      : hardware decode non-standard resolution frame
 * @tc.desc      : function test
 */
HWTEST_F(HwdecNdkTest, VIDEO_HWDEC_H264_01_1600, TestSize.Level0)
{
    shared_ptr<VDecNdkSample> vDecSample = make_shared<VDecNdkSample>();
    vDecSample->INP_DIR = "/data/test/media/352_288_60_10Mb.h264";
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->SF_OUTPUT = false;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecName));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_HWDEC_H264_02_0100
 * @tc.name      : hardware decode 1080p frame
 * @tc.desc      : function test
 */
HWTEST_F(HwdecNdkTest, VIDEO_HWDEC_H264_02_0100, TestSize.Level0)
{
    shared_ptr<VDecNdkSample> vDecSample = make_shared<VDecNdkSample>();
    vDecSample->INP_DIR = "/data/test/media/1920_1080_60_30M.h264";
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->SF_OUTPUT = false;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecName));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_HWDEC_H264_02_0200
 * @tc.name      : hardware decode 1080p frame
 * @tc.desc      : function test
 */
HWTEST_F(HwdecNdkTest, VIDEO_HWDEC_H264_02_0200, TestSize.Level0)
{
    shared_ptr<VDecNdkSample> vDecSample = make_shared<VDecNdkSample>();
    vDecSample->INP_DIR = "/data/test/media/1920_1080_30_30M.h264";
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->SF_OUTPUT = false;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecName));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_HWDEC_H264_02_0300
 * @tc.name      : hardware decode 1080p frame
 * @tc.desc      : function test
 */
HWTEST_F(HwdecNdkTest, VIDEO_HWDEC_H264_02_0300, TestSize.Level0)
{
    shared_ptr<VDecNdkSample> vDecSample = make_shared<VDecNdkSample>();
    vDecSample->INP_DIR = "/data/test/media/1920_1080_10_30Mb.h264";
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->SF_OUTPUT = false;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecName));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_HWDEC_H264_03_0100
 * @tc.name      : hardware decode 1080p frame
 * @tc.desc      : function test
 */
HWTEST_F(HwdecNdkTest, VIDEO_HWDEC_H264_03_0100, TestSize.Level0)
{
    shared_ptr<VDecNdkSample> vDecSample = make_shared<VDecNdkSample>();
    vDecSample->INP_DIR = "/data/test/media/1920_1080_60_30M.h264";
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->SF_OUTPUT = false;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecName));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_HWDEC_H264_03_0200
 * @tc.name      : hardware decode 1080p frame
 * @tc.desc      : function test
 */
HWTEST_F(HwdecNdkTest, VIDEO_HWDEC_H264_03_0200, TestSize.Level0)
{
    shared_ptr<VDecNdkSample> vDecSample = make_shared<VDecNdkSample>();
    vDecSample->INP_DIR = "/data/test/media/1920_1080_60_30M.h264";
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->SF_OUTPUT = false;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecName));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_HWDEC_H264_03_0300
 * @tc.name      : hardware decode 1080p frame
 * @tc.desc      : function test
 */
HWTEST_F(HwdecNdkTest, VIDEO_HWDEC_H264_03_0300, TestSize.Level0)
{
    shared_ptr<VDecNdkSample> vDecSample = make_shared<VDecNdkSample>();
    vDecSample->INP_DIR = "/data/test/media/1920_1080_60_20M.h264";
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->SF_OUTPUT = false;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecName));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_HWDEC_H264_03_0400
 * @tc.name      : hardware decode 1080p frame
 * @tc.desc      : function test
 */
HWTEST_F(HwdecNdkTest, VIDEO_HWDEC_H264_03_0400, TestSize.Level0)
{
    shared_ptr<VDecNdkSample> vDecSample = make_shared<VDecNdkSample>();
    vDecSample->INP_DIR = "/data/test/media/1920_1080_60_3M.h264";
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->SF_OUTPUT = false;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecName));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_HWDEC_H264_03_0500
 * @tc.name      : hardware decode 1080p frame
 * @tc.desc      : function test
 */
HWTEST_F(HwdecNdkTest, VIDEO_HWDEC_H264_03_0500, TestSize.Level0)
{
    shared_ptr<VDecNdkSample> vDecSample = make_shared<VDecNdkSample>();
    vDecSample->INP_DIR = "/data/test/media/1920_1080_60_2M.h264";
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->SF_OUTPUT = false;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecName));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_HWDEC_H264_03_0600
 * @tc.name      : hardware decode 1080p frame
 * @tc.desc      : function test
 */
HWTEST_F(HwdecNdkTest, VIDEO_HWDEC_H264_03_0600, TestSize.Level0)
{
    shared_ptr<VDecNdkSample> vDecSample = make_shared<VDecNdkSample>();
    vDecSample->INP_DIR = "/data/test/media/1920_1080_60_1M.h264";
    vDecSample->DEFAULT_WIDTH = 1920;
    vDecSample->DEFAULT_HEIGHT = 1080;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->SF_OUTPUT = false;
    ASSERT_EQ(AV_ERR_OK, vDecSample->RunVideoDec(g_codecName));
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}
} // namespace