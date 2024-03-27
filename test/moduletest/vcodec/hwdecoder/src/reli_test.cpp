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
#include "native_avcapability.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;

namespace OHOS {
namespace Media {
class HwdecReliNdkTest : public testing::Test {
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
    const char *inpDir720 = "/data/test/media/1280x720_30_10M.h264";
    const char *inpDir720Array[16] = {
        "/data/test/media//1280_720_30_10M.h264",    "/data/test/media//1280_720_30_10M_1.h264",
        "/data/test/media//1280_720_30_10M_2.h264",  "/data/test/media//1280_720_30_10M_3.h264",
        "/data/test/media//1280_720_30_10M_4.h264",  "/data/test/media//1280_720_30_10M_5.h264",
        "/data/test/media//1280_720_30_10M_6.h264",  "/data/test/media//1280_720_30_10M_7.h264",
        "/data/test/media//1280_720_30_10M_8.h264",  "/data/test/media//1280_720_30_10M_9.h264",
        "/data/test/media//1280_720_30_10M_10.h264", "/data/test/media//1280_720_30_10M_11.h264",
        "/data/test/media//1280_720_30_10M_12.h264",  "/data/test/media//1280_720_30_10M_13.h264",
        "/data/test/media//1280_720_30_10M_14.h264", "/data/test/media//1280_720_30_10M_15.h264"};
    const char *h264Array[16] = {
        "/data/test/media/360p.h264",    "/data/test/media/360p_1.h264",
        "/data/test/media/360p_2.h264",  "/data/test/media/360p_3.h264",
        "/data/test/media/360p_4.h264",  "/data/test/media/360p_5.h264",
        "/data/test/media/360p_6.h264",  "/data/test/media/360p_7.h264",
        "/data/test/media/360p_8.h264",  "/data/test/media/360p_9.h264",
        "/data/test/media/360p_10.h264", "/data/test/media/360p_11.h264",
        "/data/test/media/360p_12.h264",  "/data/test/media/360p_13.h264",
        "/data/test/media/360p_14.h264", "/data/test/media/360p_15.h264"};
    const char *h265Array[16] = {
        "/data/test/media/360p.h264",    "/data/test/media/360p_1.h264",
        "/data/test/media/360p_2.h264",  "/data/test/media/360p_3.h264",
        "/data/test/media/360p_4.h264",  "/data/test/media/360p_5.h264",
        "/data/test/media/360p_6.h264",  "/data/test/media/360p_7.h264",
        "/data/test/media/360p_8.h264",  "/data/test/media/360p_9.h264",
        "/data/test/media/360p_10.h264", "/data/test/media/360p_11.h264",
        "/data/test/media/360p_12.h264",  "/data/test/media/360p_13.h264",
        "/data/test/media/360p_14.h264", "/data/test/media/360p_15.h264"};
    const char *resChangeArray[16] = {
        "/data/test/media/resChange.h264",    "/data/test/media/resChange_1.h264",
        "/data/test/media/resChange_2.h264",  "/data/test/media/resChange_3.h264",
        "/data/test/media/resChange_4.h264",  "/data/test/media/resChange_5.h264",
        "/data/test/media/resChange_6.h264",  "/data/test/media/resChange_7.h264",
        "/data/test/media/resChange_8.h264",  "/data/test/media/resChange_9.h264",
        "/data/test/media/resChange_10.h264", "/data/test/media/resChange_11.h264",
        "/data/test/media/resChange_12.h264",  "/data/test/media/resChange_13.h264",
        "/data/test/media/resChange_14.h264", "/data/test/media/resChange_15.h264"};
};
} // namespace Media
} // namespace OHOS

static string g_codecName;
static string g_hevcName;
static OH_AVCapability *cap = nullptr;
void HwdecReliNdkTest::SetUpTestCase()
{
    cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_AVC, false, HARDWARE);
    g_codecName = OH_AVCapability_GetName(cap);
    OH_AVCapability *cap2 = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_HEVC, false, HARDWARE);
    g_hevcName = OH_AVCapability_GetName(cap2);
    cout << "codecname: " << g_codecName << endl;
}

void HwdecReliNdkTest::TearDownTestCase() {}

void HwdecReliNdkTest::SetUp() {}

void HwdecReliNdkTest::TearDown() {}

namespace {
/**
 * @tc.number    : VIDEO_HWDEC_STABILITY_0200
 * @tc.name      : confige-start-flush-start-reset 1000 times
 * @tc.desc      : reli test
 */
HWTEST_F(HwdecReliNdkTest, VIDEO_HWDEC_STABILITY_0200, TestSize.Level4)
{
    vdec_ = OH_VideoDecoder_CreateByName(g_codecName.c_str());
    for (int i = 0; i < 1000; i++) {
        ASSERT_NE(nullptr, vdec_);
        OH_AVFormat *format = OH_AVFormat_Create();
        ASSERT_NE(nullptr, format);
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, 1920);
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, 1080);
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_NV12);
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_FRAME_RATE, 30);
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
 * @tc.number    : VIDEO_HWDEC_STABILITY_0400
 * @tc.name      : SetParameter 1000 times
 * @tc.desc      : reli test
 */
HWTEST_F(HwdecReliNdkTest, VIDEO_HWDEC_STABILITY_0400, TestSize.Level4)
{
    vdec_ = OH_VideoDecoder_CreateByName(g_codecName.c_str());
    ASSERT_NE(nullptr, vdec_);
    OH_AVFormat *format = OH_AVFormat_Create();
    ASSERT_NE(nullptr, format);
    int64_t widht = 1920;
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, widht);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, 1080);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_NV12);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_FRAME_RATE, 30);
    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(vdec_, format));
    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Start(vdec_));
    for (int i = 0; i < 1000; i++) {
        widht++;
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, widht);
        ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_SetParameter(vdec_, format));
    }
    OH_AVFormat_Destroy(format);
    OH_VideoDecoder_Stop(vdec_);
    OH_VideoDecoder_Destroy(vdec_);
}

/**
 * @tc.number    : VIDEO_HWDEC_PERFORMANCE_WHILE_0100
 * @tc.name      :
 * @tc.desc      : perf test
 */
HWTEST_F(HwdecReliNdkTest, VIDEO_HWDEC_PERFORMANCE_WHILE_0100, TestSize.Level3)
{
    while (true) {
        shared_ptr<VDecNdkSample> vDecSample = make_shared<VDecNdkSample>();
        vDecSample->SF_OUTPUT = false;
        vDecSample->INP_DIR = inpDir720;
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->sleepOnFPS = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecName.c_str()));
        ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
        ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
        ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
        vDecSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_HWDEC_PERFORMANCE_WHILE_0200
 * @tc.name      :
 * @tc.desc      : perf test
 */
HWTEST_F(HwdecReliNdkTest, VIDEO_HWDEC_PERFORMANCE_WHILE_0200, TestSize.Level3)
{
    for (int i = 0; i < 16; i++) {
        VDecNdkSample *vDecSample = new VDecNdkSample();
        vDecSample->SF_OUTPUT = false;
        vDecSample->INP_DIR = inpDir720Array[i];
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->sleepOnFPS = true;
        vDecSample->repeatRun = true;
        ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecName.c_str()));
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
 * @tc.number    : VIDEO_HWDEC_PERFORMANCE_WHILE_0300
 * @tc.name      :
 * @tc.desc      : perf test
 */
HWTEST_F(HwdecReliNdkTest, VIDEO_HWDEC_PERFORMANCE_WHILE_0300, TestSize.Level3)
{
    shared_ptr<VDecNdkSample> vDecSample = make_shared<VDecNdkSample>();
    vDecSample->SF_OUTPUT = false;
    vDecSample->INP_DIR = inpDir720;
    vDecSample->DEFAULT_WIDTH = 1280;
    vDecSample->DEFAULT_HEIGHT = 720;
    vDecSample->DEFAULT_FRAME_RATE = 30;
    vDecSample->sleepOnFPS = true;
    vDecSample->repeatRun = true;
    ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecName.c_str()));
    ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
    ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
    ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
}

/**
 * @tc.number    : VIDEO_HWDEC_PERFORMANCE_WHILE_0400
 * @tc.name      :
 * @tc.desc      : perf test
 */
HWTEST_F(HwdecReliNdkTest, VIDEO_HWDEC_PERFORMANCE_WHILE_0400, TestSize.Level3)
{
    while (true) {
        vector<shared_ptr<VDecNdkSample>> decVec;
        for (int i = 0; i < 16; i++) {
            auto vDecSample = make_shared<VDecNdkSample>();
            decVec.push_back(vDecSample);
            vDecSample->SF_OUTPUT = false;
            vDecSample->INP_DIR = inpDir720Array[i];
            vDecSample->DEFAULT_WIDTH = 1280;
            vDecSample->DEFAULT_HEIGHT = 720;
            vDecSample->DEFAULT_FRAME_RATE = 30;
            vDecSample->sleepOnFPS = true;
            vDecSample->repeatRun = true;
            ASSERT_EQ(AV_ERR_OK, vDecSample->CreateVideoDecoder(g_codecName.c_str()));
            ASSERT_EQ(AV_ERR_OK, vDecSample->ConfigureVideoDecoder());
            ASSERT_EQ(AV_ERR_OK, vDecSample->SetVideoDecoderCallback());
            ASSERT_EQ(AV_ERR_OK, vDecSample->StartVideoDecoder());
            if (i == 15) {
                vDecSample->WaitForEOS();
                ASSERT_EQ(AV_ERR_OK, vDecSample->errCount);
            }
        }
    }
}

/**
 * @tc.number    : SURF_CHANGE_RELI_001
 * @tc.name      :
 * @tc.desc      : perf test
 */
HWTEST_F(HwdecReliNdkTest, SURF_CHANGE_RELI_001, TestSize.Level3)
{
    while (true) {
        vector<shared_ptr<VDecNdkSample>> decVec;
        for (int i = 0; i < 16; i++) {
            auto vDecSample = make_shared<VDecNdkSample>();
            decVec.push_back(vDecSample);
            vDecSample->INP_DIR = H264Array[i];
            vDecSample->DEFAULT_WIDTH = 480;
            vDecSample->DEFAULT_HEIGHT = 360;
            vDecSample->DEFAULT_FRAME_RATE = 30;
            vDecSample->SF_OUTPUT = true;
            vDecSample->autoSwitchSurface = true;
            vDecSample->sleepOnFPS = true;
            vDecSample->RunVideoDec_Surface(g_codecName);
        }
        uint32_t errorCount = 0;
        for_each(decVec.begin(), decVec.end(), [&errorCount](auto sample) {
            sample->WaitForEOS();
            errorCount += sample->errCount;
        });
        ASSERT_EQ(AV_ERR_OK, errorCount);
        decVec.clear();
    }
}

/**
 * @tc.number    : SURF_CHANGE_RELI_002
 * @tc.name      :
 * @tc.desc      : perf test
 */
HWTEST_F(HwdecReliNdkTest, SURF_CHANGE_RELI_002, TestSize.Level3)
{
    while (true) {
        vector<shared_ptr<VDecNdkSample>> decVec;
        for (int i = 0; i < 16; i++) {
            auto vDecSample = make_shared<VDecNdkSample>();
            decVec.push_back(vDecSample);
            vDecSample->INP_DIR = H265Array[i];
            vDecSample->DEFAULT_WIDTH = 480;
            vDecSample->DEFAULT_HEIGHT = 360;
            vDecSample->DEFAULT_FRAME_RATE = 30;
            vDecSample->SF_OUTPUT = true;
            vDecSample->autoSwitchSurface = true;
            vDecSample->sleepOnFPS = true;
            vDecSample->RunVideoDec_Surface(g_hevcName);
        }
        uint32_t errorCount = 0;
        for_each(decVec.begin(), decVec.end(), [&errorCount](auto sample) {
            sample->WaitForEOS();
            errorCount += sample->errCount;
        });
        ASSERT_EQ(AV_ERR_OK, errorCount);
        decVec.clear();
    }
}

/**
 * @tc.number    : SURF_CHANGE_RELI_003
 * @tc.name      :
 * @tc.desc      : perf test
 */
HWTEST_F(HwdecReliNdkTest, SURF_CHANGE_RELI_003, TestSize.Level3)
{
    while (true) {
        vector<shared_ptr<VDecNdkSample>> decVec;
        for (int i = 0; i < 16; i++) {
            auto vDecSample = make_shared<VDecNdkSample>();
            decVec.push_back(vDecSample);
            vDecSample->INP_DIR = ResChangeArray[i];
            vDecSample->DEFAULT_WIDTH = 480;
            vDecSample->DEFAULT_HEIGHT = 360;
            vDecSample->DEFAULT_FRAME_RATE = 30;
            vDecSample->SF_OUTPUT = true;
            vDecSample->autoSwitchSurface = true;
            vDecSample->sleepOnFPS = true;
            vDecSample->RunVideoDec_Surface(g_hevcName);
        }
        uint32_t errorCount = 0;
        for_each(decVec.begin(), decVec.end(), [&errorCount](auto sample) {
            sample->WaitForEOS();
            errorCount += sample->errCount;
        });
        ASSERT_EQ(AV_ERR_OK, errorCount);
        decVec.clear();
    }
}
} // namespace