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
#include "native_avcapability.h"
#include "gtest/gtest.h"
#include "videodec_sample.h"
using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;
namespace OHOS {
namespace Media {
class Mpeg2SwdecCompNdkTest : public testing::Test {
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

namespace {
    static OH_AVCapability *cap_mpeg2 = nullptr;
    static string g_codecNameMpeg2 = "";
} // namespace

void Mpeg2SwdecCompNdkTest::SetUpTestCase(void)
{
    cap_mpeg2 = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_MPEG2, false, SOFTWARE);
    g_codecNameMpeg2 = OH_AVCapability_GetName(cap_mpeg2);
    cout << "g_codecNameMpeg2: " << g_codecNameMpeg2 << endl;
}
void Mpeg2SwdecCompNdkTest::TearDownTestCase(void) {}
void Mpeg2SwdecCompNdkTest::SetUp(void) {}
void Mpeg2SwdecCompNdkTest::TearDown(void) {}
} // namespace Media
} // namespace OHOS

namespace {
/**
 * @tc.number    : VIDEO_SWDEC_MPEG2_IPB_0100
 * @tc.name      : software decode all i frame mpeg2 stream
 * @tc.desc      : function test
 */
HWTEST_F(Mpeg2SwdecCompNdkTest, VIDEO_SWDEC_MPEG2_IPB_0100, TestSize.Level2)
{
    if (cap_mpeg2 != nullptr) {
        VDecNdkSample *vDecSample = new VDecNdkSample();
        int32_t ret = vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.MPEG2");
        ASSERT_EQ(AV_ERR_OK, ret);
        vDecSample->INP_DIR = "/data/test/media/all_i_main@high14_level_1280_720_30.m2v";
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        ret = vDecSample->SetVideoDecoderCallback();
        ASSERT_EQ(AV_ERR_OK, ret);
        ret = vDecSample->ConfigureVideoDecoder();
        ASSERT_EQ(AV_ERR_OK, ret);
        ret = vDecSample->StartVideoDecoder();
        ASSERT_EQ(AV_ERR_OK, ret);
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_MPEG2_IPB_0200
 * @tc.name      : software decode i/p frame mpeg2 stream
 * @tc.desc      : function test
 */
HWTEST_F(Mpeg2SwdecCompNdkTest, VIDEO_SWDEC_MPEG2_IPB_0200, TestSize.Level2)
{
    if (cap_mpeg2 != nullptr) {
        VDecNdkSample *vDecSample = new VDecNdkSample();
        int32_t ret = vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.MPEG2");
        ASSERT_EQ(AV_ERR_OK, ret);
        vDecSample->INP_DIR = "/data/test/media/ip_main@high14_level_1280_720_30.m2v";
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        ret = vDecSample->SetVideoDecoderCallback();
        ASSERT_EQ(AV_ERR_OK, ret);
        ret = vDecSample->ConfigureVideoDecoder();
        ASSERT_EQ(AV_ERR_OK, ret);
        ret = vDecSample->StartVideoDecoder();
        ASSERT_EQ(AV_ERR_OK, ret);
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
    }
}

/**
 * @tc.number    : VIDEO_SWDEC_MPEG2_IPB_0300
 * @tc.name      : software decode i/p/b frame mpeg2 stream
 * @tc.desc      : function test
 */
HWTEST_F(Mpeg2SwdecCompNdkTest, VIDEO_SWDEC_MPEG2_IPB_0300, TestSize.Level2)
{
    if (cap_mpeg2 != nullptr) {
        VDecNdkSample *vDecSample = new VDecNdkSample();
        int32_t ret = vDecSample->CreateVideoDecoder("OH.Media.Codec.Decoder.Video.MPEG2");
        ASSERT_EQ(AV_ERR_OK, ret);
        vDecSample->INP_DIR = "/data/test/media/ipb_main@high14_level_1280_720_30.m2v";
        vDecSample->DEFAULT_WIDTH = 1280;
        vDecSample->DEFAULT_HEIGHT = 720;
        ret = vDecSample->SetVideoDecoderCallback();
        ASSERT_EQ(AV_ERR_OK, ret);
        ret = vDecSample->ConfigureVideoDecoder();
        ASSERT_EQ(AV_ERR_OK, ret);
        ret = vDecSample->StartVideoDecoder();
        ASSERT_EQ(AV_ERR_OK, ret);
        vDecSample->WaitForEOS();
        ASSERT_EQ(0, vDecSample->errCount);
    }
}
} // namespace