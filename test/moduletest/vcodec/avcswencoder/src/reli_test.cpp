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
#include "videoenc_api11_sample.h"
#include "native_avcodec_videoencoder.h"
#include "native_avcodec_base.h"
#include "native_avcapability.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;

namespace OHOS {
namespace Media {
class AvcSwEncReliNdkTest : public testing::Test {
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
    const char *inpDir720Array[16] = {"/data/test/media/1280_720_nv.yuv", "/data/test/media/1280_720_nv12.yuv"};
    bool createCodecSuccess_ = false;
};
} // namespace Media
} // namespace OHOS
namespace {
OH_AVCapability *cap = nullptr;
constexpr uint32_t CODEC_NAME_SIZE = 128;
char g_codecName[CODEC_NAME_SIZE] = {};
} // namespace
void AvcSwEncReliNdkTest::SetUpTestCase()
{
    cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_AVC, true, SOFTWARE);
    const char *tmpCodecName = OH_AVCapability_GetName(cap);
    if (memcpy_s(g_codecName, sizeof(g_codecName), tmpCodecName, strlen(tmpCodecName)) != 0)
        cout << "memcpy failed" << endl;
    cout << "codecname: " << g_codecName << endl;
}

void AvcSwEncReliNdkTest::TearDownTestCase() {}

void AvcSwEncReliNdkTest::SetUp() {}

void AvcSwEncReliNdkTest::TearDown() {}

namespace {
/**
 * @tc.number    : VIDEO_HWENC_RELI_WHILE_0100
 * @tc.name      : 720P
 * @tc.desc      : reliable test
 */
HWTEST_F(AvcSwEncReliNdkTest, VIDEO_HWENC_RELI_WHILE_0100, TestSize.Level3)
{
    if (strlen(g_codecName) == 0) {
        return;
    }
    shared_ptr<VEncAPI11Sample> vEncSample = make_shared<VEncAPI11Sample>();
    vEncSample->INP_DIR = inpDir720;
    vEncSample->DEFAULT_WIDTH = 1280;
    vEncSample->DEFAULT_HEIGHT = 720;
    vEncSample->DEFAULT_FRAME_RATE = 30;
    vEncSample->OUT_DIR = "/data/test/media/1280_720_buffer.h264";
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}


/**
 * @tc.number    : VIDEO_HWENC_RELI_WHILE_0200
 * @tc.name      : 2 instances
 * @tc.desc      : reliable test
 */
HWTEST_F(AvcSwEncReliNdkTest, VIDEO_HWENC_RELI_WHILE_0200, TestSize.Level3)
{
    if (strlen(g_codecName) == 0) {
        return;
    }
    vector<shared_ptr<VEncAPI11Sample>> encVec;
    for (int i = 0; i < 2; i++) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        encVec.push_back(vEncSample);
        vEncSample->INP_DIR = inpDir720Array[i];
        vEncSample->DEFAULT_WIDTH = 1280;
        vEncSample->DEFAULT_HEIGHT = 720;
        vEncSample->DEFAULT_FRAME_RATE = 30;
        vEncSample->OUT_DIR = "/data/test/media/1280_720_buffer.h264";
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
        ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
        ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    }
    for (int i = 0; i < 2; i++) {
        encVec[i]->WaitForEOS();
    }
    encVec.clear();
}


/**
 * @tc.number    : RESET_BITRATE_RELI_0100
 * @tc.name      : reset bitrate
 * @tc.desc      : function test
 */
HWTEST_F(AvcSwEncReliNdkTest, RESET_BITRATE_RELI_0100, TestSize.Level3)
{
    if (strlen(g_codecName) == 0) {
        return;
    }
    vector<shared_ptr<VEncAPI11Sample>> encVec;
    for (int i = 0; i < 2; i++) {
        auto vEncSample = make_shared<VEncAPI11Sample>();
        encVec.push_back(vEncSample);
        vEncSample->INP_DIR = inpDir720Array[i];
        vEncSample->DEFAULT_WIDTH = 1280;
        vEncSample->DEFAULT_HEIGHT = 720;
        vEncSample->DEFAULT_FRAME_RATE = 30;
        vEncSample->enableAutoSwitchParam =  true;
        vEncSample->needResetBitrate = true;
        vEncSample->OUT_DIR = "/data/test/media/1280_720_buffer.h264";
        ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(g_codecName));
        ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
        ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
        ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    }
    for (int i = 0; i < 2; i++) {
        encVec[i]->WaitForEOS();
    }
    encVec.clear();
}
} // namespace