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
#include "videoenc_ndk_sample.h"
#include "native_avcodec_videoencoder.h"
#include "native_avcodec_base.h"
#include "native_avcapability.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;

namespace OHOS {
namespace Media {
class HwEncReliNdkTest : public testing::Test {
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
    const char *inpDir720 = "/data/test/media/1280_720_nv.yuv";
    const char *inpDir720Array[16] = {"/data/test/media/1280_720_nv.yuv",    "/data/test/media/1280_720_nv_1.yuv",
                                      "/data/test/media/1280_720_nv_2.yuv",  "/data/test/media/1280_720_nv_3.yuv",
                                      "/data/test/media/1280_720_nv_7.yuv",  "/data/test/media/1280_720_nv_10.yuv",
                                      "/data/test/media/1280_720_nv_13.yuv", "/data/test/media/1280_720_nv_4.yuv",
                                      "/data/test/media/1280_720_nv_8.yuv",  "/data/test/media/1280_720_nv_11.yuv",
                                      "/data/test/media/1280_720_nv_14.yuv", "/data/test/media/1280_720_nv_5.yuv",
                                      "/data/test/media/1280_720_nv_9.yuv",  "/data/test/media/1280_720_nv_12.yuv",
                                      "/data/test/media/1280_720_nv_15.yuv", "/data/test/media/1280_720_nv_6.yuv"};
    bool createCodecSuccess_ = false;
};
} // namespace Media
} // namespace OHOS
namespace {
OH_AVCapability *cap = nullptr;
string g_codecNameAvc;
} // namespace
void HwEncReliNdkTest::SetUpTestCase()
{
    cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_AVC, true, HARDWARE);
    g_codecNameAvc = OH_AVCapability_GetName(cap);
}

void HwEncReliNdkTest::TearDownTestCase() {}

void HwEncReliNdkTest::SetUp() {}

void HwEncReliNdkTest::TearDown() {}

namespace {
/**
 * @tc.number    : VIDEO_HWENC_RELI_WHILE_0100
 * @tc.name      : 720P
 * @tc.desc      : reliable test
 */
HWTEST_F(HwEncReliNdkTest, VIDEO_HWENC_RELI_WHILE_0100, TestSize.Level3)
{
    while (true) {
        shared_ptr<VEncNdkSample> vEncSample = make_shared<VEncNdkSample>();
        vEncSample->INP_DIR = inpDir720;
        vEncSample->DEFAULT_WIDTH = 1280;
        vEncSample->DEFAULT_HEIGHT = 720;
        vEncSample->DEFAULT_FRAME_RATE = 30;
        vEncSample->OUT_DIR = "/data/test/media/1280_720_buffer.h264";
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameAvc.c_str()));
        ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
        ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
        vEncSample->WaitForEOS();
        ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_HWENC_RELI_WHILE_0200
 * @tc.name      : 16 instances
 * @tc.desc      : reliable test
 */
HWTEST_F(HwEncReliNdkTest, VIDEO_HWENC_RELI_WHILE_0200, TestSize.Level3)
{
    for (int i = 0; i < 16; i++) {
        VEncNdkSample *vEncSample = new VEncNdkSample();
        vEncSample->INP_DIR = inpDir720Array[i];
        vEncSample->DEFAULT_WIDTH = 1280;
        vEncSample->DEFAULT_HEIGHT = 720;
        vEncSample->DEFAULT_FRAME_RATE = 30;
        vEncSample->repeatRun = true;
        vEncSample->OUT_DIR = "/data/test/media/1280_720_buffer.h264";
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameAvc.c_str()));
        ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
        ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
        if (i == 15) {
            vEncSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
        }
    }
}

/**
 * @tc.number    : VIDEO_HWENC_RELI_WHILE_0300
 * @tc.name      : long encode
 * @tc.desc      : reliable test
 */
HWTEST_F(HwEncReliNdkTest, VIDEO_HWENC_RELI_WHILE_0300, TestSize.Level3)
{
    shared_ptr<VEncNdkSample> vEncSample = make_shared<VEncNdkSample>();
    vEncSample->INP_DIR = inpDir720;
    vEncSample->DEFAULT_WIDTH = 1280;
    vEncSample->DEFAULT_HEIGHT = 720;
    vEncSample->DEFAULT_FRAME_RATE = 30;
    vEncSample->repeatRun = true;
    vEncSample->OUT_DIR = "/data/test/media/1280_720_buffer.h264";
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameAvc.c_str()));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

/**
 * @tc.number    : VIDEO_HWENC_RELI_WHILE_0400
 * @tc.name      : 16 instances
 * @tc.desc      : reliable test
 */
HWTEST_F(HwEncReliNdkTest, VIDEO_HWENC_RELI_WHILE_0400, TestSize.Level3)
{
    while (true) {
        vector<shared_ptr<VEncNdkSample>> encVec;
        for (int i = 0; i < 16; i++) {
            auto vEncSample = make_shared<VEncNdkSample>();
            encVec.push_back(vEncSample);
            vEncSample->INP_DIR = inpDir720Array[i];
            vEncSample->DEFAULT_WIDTH = 1280;
            vEncSample->DEFAULT_HEIGHT = 720;
            vEncSample->DEFAULT_FRAME_RATE = 30;
            vEncSample->OUT_DIR = "/data/test/media/1280_720_buffer.h264";
            ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameAvc.c_str()));
            ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
            ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
            ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
        }
        for (int i = 0; i < 16; i++) {
            encVec[i]->WaitForEOS();
        }
    }
}
} // namespace