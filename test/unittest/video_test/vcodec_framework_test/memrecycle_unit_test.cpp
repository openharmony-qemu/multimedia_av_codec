/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#include "serverdec_sample.h"
#include "avcodec_video_decoder.h"
#include "meta/format.h"
#include "avcodec_errors.h"
#include "avcodec_common.h"
#include "native_avcapability.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace testing::ext;

namespace {
class HevcswdecInnerApiNdkTest : public testing::Test {
public:
    // SetUpTestCase: Called before all test cases
    static void SetUpTestCase(void);
    // TearDownTestCase: Called after all test case
    static void TearDownTestCase(void);
    // SetUp: Called before each test case
    void SetUp() override;
    // TearDown: Called after each test case
    void TearDown() override;
protected:
    const char *INP_DIR_1080_30 = "/data/test/media/720_1280_25_avcc.h265"; //szj  后期把INP_DIR_1080_30改成INP_DIR_720
};

std::shared_ptr<AVCodecVideoDecoder> vdec_ = nullptr;
std::shared_ptr<VDecInnerSignal> signal_ = nullptr;
std::string g_invalidCodecMime = "avdec_h265";
std::string g_codecMime = "video/hevc";
std::string g_codecName = "";

constexpr uint32_t DEFAULT_WIDTH = 1920;
constexpr uint32_t DEFAULT_HEIGHT = 1080;
constexpr double DEFAULT_FRAME_RATE = 30.0;

void HevcswdecInnerApiNdkTest::SetUpTestCase()
{
    OH_AVCapability *cap = OH_AVCodec_GetCapabilityByCategory(g_codecMime.c_str(), false, SOFTWARE);
    const char *tmpCodecName = OH_AVCapability_GetName(cap);
    g_codecName = tmpCodecName;
    cout << "g_codecName: " << g_codecName << endl;
}

void HevcswdecInnerApiNdkTest::TearDownTestCase() {}

void HevcswdecInnerApiNdkTest::SetUp() 
{
    signal_ = make_shared<VDecInnerSignal>();
}

void HevcswdecInnerApiNdkTest::TearDown()
{
    if (signal_) {
        signal_ = nullptr;
    }

    if (vdec_ != nullptr) {
        vdec_->Release();
        vdec_ = nullptr;
    }
}
} // namespace

namespace {
/**
 * @tc.number    : VIDEO_MEMORYRECYCLE_0100
 * @tc.name      : Normal decoding process
 * @tc.desc      : api test
 */
HWTEST_F(HevcswdecInnerApiNdkTest, VIDEO_MEMORYRECYCLE_0100, TestSize.Level12)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecNdkInnerSample> vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = INP_DIR_1080_30;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = true;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunVideoDecoder(g_codecName));
        vDecSample->WaitForEOS();
    }
}

/**
 * @tc.number    : VIDEO_MEMORYRECYCLE_0200
 * @tc.name      : Normal decoding process
 * @tc.desc      : api test
 */
HWTEST_F(HevcswdecInnerApiNdkTest, VIDEO_MEMORYRECYCLE_0200, TestSize.Level12)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecNdkInnerSample> vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = INP_DIR_1080_30;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = true;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunErrorVideoDecoder(g_codecName));
        vDecSample->WaitForEOS();
    }
}

/**
 * @tc.number    : VIDEO_MEMORYRECYCLE_0300
 * @tc.name      : Normal decoding process
 * @tc.desc      : api test
 */
HWTEST_F(HevcswdecInnerApiNdkTest, VIDEO_MEMORYRECYCLE_0300, TestSize.Level12)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecNdkInnerSample> vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = INP_DIR_1080_30;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = true;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunFcodecVideoDecoder(g_codecName));
        vDecSample->WaitForEOS();
    }
}

/**
 * @tc.number    : VIDEO_MEMORYRECYCLE_0400
 * @tc.name      : Normal decoding process
 * @tc.desc      : api test
 */
HWTEST_F(HevcswdecInnerApiNdkTest, VIDEO_MEMORYRECYCLE_0400, TestSize.Level12)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecNdkInnerSample> vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = INP_DIR_1080_30;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = false;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunFcodecVideoDecoder(g_codecName));
        vDecSample->WaitForEOS();
    }
}

/**
 * @tc.number    : VIDEO_MEMORYRECYCLE_0500
 * @tc.name      : Normal decoding process
 * @tc.desc      : api test
 */
HWTEST_F(HevcswdecInnerApiNdkTest, VIDEO_MEMORYRECYCLE_0400, TestSize.Level12)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecNdkInnerSample> vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->INP_DIR = INP_DIR_1080_30;
        vDecSample->DEFAULT_WIDTH = 1920;
        vDecSample->DEFAULT_HEIGHT = 1080;
        vDecSample->DEFAULT_FRAME_RATE = 30;
        vDecSample->SF_OUTPUT = true;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunFcodecErrorVideoDecoder(g_codecName));
        vDecSample->WaitForEOS();
    }
}

} // namespace