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

/**
 * @tc.number    : VIDEO_HWENC_RELI_WHILE_0010
 * @tc.name      : 16个线程 分层编码-reset-分层编码
 * @tc.desc      : function test
 */
HWTEST_F(HwEncReliNdkTest, VIDEO_HWENC_RELI_WHILE_0010, TestSize.Level3)
{
    int len = 256;
    int loop = 0;
    while (loop < 1) {
        loop++;
        cout << "loop: "<<loop<<endl;
        vector<shared_ptr<VEncNdkSample>> encVec;
        for (int i = 0; i < 16; i++) {
            char file[256] = {};
            sprintf_s(file,len,"/data/test/media/VIDEO_HWENC_RELI_WHILE_0010_%d.h265",i);
            auto vEncSample = make_shared<VEncNdkSample>();
            vEncSample->OUT_DIR = file;

            encVec.push_back(vEncSample);
            vEncSample->INP_DIR = inpDir720Array[i];
            vEncSample->TEMPORAL_ENABLE = true;
            vEncSample->TEMPORAL_CONFIG = true;
            ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
            ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
            ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder_Temporal(i+2));
            ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
            vEncSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
            ASSERT_EQ(AV_ERR_OK, vEncSample->Reset());
            vEncSample->TEMPORAL_ENABLE = false;
            vEncSample->TEMPORAL_CONFIG = false;

            ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
            ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder_Temporal(i+2));
            ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
            vEncSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
        }
        for (int i = 0; i < 16; i++) {
            encVec[i]->WaitForEOS();
        }
        encVec.clear();
    }
}

/**
 * @tc.number    : VIDEO_HWENC_RELI_WHILE_0020
 * @tc.name      : 16个线程 分层编码-reset-普通编码
 * @tc.desc      : function test
 */
HWTEST_F(HwEncReliNdkTest, VIDEO_HWENC_RELI_WHILE_0020, TestSize.Level3)
{
    int len = 256;
    int loop = 0;
    while (loop < 1) {
        loop++;
        cout << "loop: "<<loop<<endl;
        vector<shared_ptr<VEncNdkSample>> encVec;
        for (int i = 0; i < 16; i++) {
            char file[256] = {};
            sprintf_s(file,len,"/data/test/media/VIDEO_HWENC_RELI_WHILE_0020_%d.h265",i);
            auto vEncSample = make_shared<VEncNdkSample>();
            vEncSample->OUT_DIR = file;

            encVec.push_back(vEncSample);
            vEncSample->INP_DIR = inpDir720Array[i];
            vEncSample->TEMPORAL_ENABLE = true;
            vEncSample->TEMPORAL_CONFIG = true;
            ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
            ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
            ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder_Temporal(i+2));
            ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
            vEncSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
            ASSERT_EQ(AV_ERR_OK, vEncSample->Reset());
            vEncSample->TEMPORAL_ENABLE = false;
            vEncSample->TEMPORAL_CONFIG = false;

            ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
            ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
            ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
            vEncSample->WaitForEOS();
            ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
        }
        for (int i = 0; i < 16; i++) {
            encVec[i]->WaitForEOS();
        }
        encVec.clear();
    }
}

/**
 * @tc.number    : VIDEO_HWENC_RELI_WHILE_0030
 * @tc.name      : h265 buffer 16个线程，8个相邻分层，8个跨帧分层1
 * @tc.desc      : function test
 */
HWTEST_F(HwEncReliNdkTest, VIDEO_HWENC_RELI_WHILE_0030, TestSize.Level3)
{
    int len =256;
    int loop = 0;
    while (loop < 1) {
        loop++;
        cout << "loop: "<<loop<<endl;
        vector<shared_ptr<VEncNdkSample>> encVec;
        for (int i = 0; i < 16; i++) {
            char file[256] = {};
            sprintf_s(file,len,"/data/test/media/VIDEO_HWENC_RELI_WHILE_0030_%d.h265",i);
            auto vEncSample = make_shared<VEncNdkSample>();
            vEncSample->OUT_DIR = file;

            encVec.push_back(vEncSample);
            vEncSample->INP_DIR = inpDir720Array[i];
            vEncSample->TEMPORAL_ENABLE = true;
            vEncSample->TEMPORAL_CONFIG = true;
            if(i % 2 == 0){
                vEncSample->TEMPORAL_JUMP_MODE = true;
            }
            ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
            ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
            ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder_Temporal(i));
            ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
        }
        for (int i = 0; i < 16; i++) {
            encVec[i]->WaitForEOS();
        }
        encVec.clear();
    }
}

/**
 * @tc.number    : VIDEO_HWENC_RELI_WHILE_0040
 * @tc.name      : h265 surface 16个线程，8个相邻分层，8个跨帧分层
 * @tc.desc      : function test
 */
HWTEST_F(HwEncReliNdkTest, VIDEO_HWENC_RELI_WHILE_0040, TestSize.Level3)
{
    int len =256;
    int loop = 0;
    while (loop < 10) {
        loop++;
        cout << "loop: "<<loop<<endl;
        vector<shared_ptr<VEncNdkSample>> encVec;
        for (int i = 0; i < 16; i++) {
            char file[256] = {};
            sprintf_s(file,len,"/data/test/media/VIDEO_HWENC_RELI_WHILE_0040_%d.h265",i);
            auto vEncSample = make_shared<VEncNdkSample>();
            vEncSample->OUT_DIR = file;

            encVec.push_back(vEncSample);
            vEncSample->INP_DIR = inpDir720Array[i];
            vEncSample->SURFACE_INPUT = true;
            vEncSample->TEMPORAL_ENABLE = true;
            vEncSample->TEMPORAL_CONFIG = true;
            if(i % 2 == 0){
                vEncSample->TEMPORAL_JUMP_MODE = true;
            }
            ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecNameHEVC));
            ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
            ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder_Temporal(i));
            ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
        }
        for (int i = 0; i < 16; i++) {
            encVec[i]->WaitForEOS();
        }
        encVec.clear();
    }
}
} // namespace