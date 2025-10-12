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

#include <cstdio>
#include <iostream>
#include <atomic>
#include <fstream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

#include "native_avdemuxer.h"
#include "native_avsource.h"
#include "native_avmemory.h"
#include <fcntl.h>

#include "gtest/gtest.h"
#include "videodec_api11_sample.h"
using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;
namespace OHOS {
namespace Media {
class Vc1decStateNdkTest : public testing::Test {
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
void Vc1decStateNdkTest::SetUpTestCase(void) {}
void Vc1decStateNdkTest::TearDownTestCase(void) {}
VDecAPI11Sample *vDecSample = NULL;

void Vc1decStateNdkTest::SetUp(void)
{
    vDecSample = new VDecAPI11Sample();
    const char *file = "/data/test/media/test_vc1.avi";
    vDecSample->getFormat(file);
    string codeName = "OH.Media.Codec.Decoder.Video.VC1";
    int32_t ret = vDecSample->CreateVideoDecoder(codeName);
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->SetVideoDecoderCallback();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->ConfigureVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
}

void Vc1decStateNdkTest::TearDown(void)
{
    vDecSample->Release();
    delete vDecSample;
    vDecSample = nullptr;
}
} // namespace Media
} // namespace OHOS

namespace {
/**
 * @tc.number    : VIDEO_VC1DEC_STATE_0100
 * @tc.name      : create-configure-error
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decStateNdkTest, VIDEO_VC1DEC_STATE_0100, TestSize.Level3)
{
    int32_t ret = vDecSample->Flush();
    ASSERT_EQ(AV_ERR_INVALID_STATE, ret);
    ret = vDecSample->SetVideoDecoderCallback();
    ASSERT_EQ(AV_ERR_OK, ret);
}

/**
 * @tc.number    : VIDEO_VC1DEC_STATE_0200
 * @tc.name      : create-configure-start-stop-start
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decStateNdkTest, VIDEO_VC1DEC_STATE_0200, TestSize.Level3)
{
    int32_t ret = vDecSample->Start();
    cout << " Start ret::" << ret << endl;
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->Stop();
    cout << " Stop ret::" << ret << endl;
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->Start();
    cout << " Start2 ret::" << ret << endl;
    ASSERT_EQ(AV_ERR_OK, ret);
}

/**
 * @tc.number    : VIDEO_VC1DEC_STATE_0300
 * @tc.name      : create-configure-start-stop-release
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decStateNdkTest, VIDEO_VC1DEC_STATE_0300, TestSize.Level3)
{
    int32_t ret = vDecSample->Start();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->Stop();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->Release();
    ASSERT_EQ(AV_ERR_OK, ret);
}

/**
 * @tc.number    : VIDEO_VC1DEC_STATE_0400
 * @tc.name      : create-configure-start-stop-reset
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decStateNdkTest, VIDEO_VC1DEC_STATE_0400, TestSize.Level3)
{
    int32_t ret = vDecSample->Start();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->Stop();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->Reset();
    ASSERT_EQ(AV_ERR_OK, ret);
}

/**
 * @tc.number    : VIDEO_VC1DEC_STATE_0500
 * @tc.name      : create-configure-start-stop-error
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decStateNdkTest, VIDEO_VC1DEC_STATE_0500, TestSize.Level3)
{
    int32_t ret = vDecSample->Start();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->Stop();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->Flush();
    ASSERT_EQ(AV_ERR_INVALID_STATE, ret);
    ret = vDecSample->SetVideoDecoderCallback();
    ASSERT_EQ(AV_ERR_OK, ret);
}

/**
 * @tc.number    : VIDEO_VC1DEC_STATE_0600
 * @tc.name      : create-configure-start-EOS-stop-start
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decStateNdkTest, VIDEO_VC1DEC_STATE_0600, TestSize.Level3)
{
    vDecSample->AFTER_EOS_DESTORY_CODEC = false;
    int32_t ret = vDecSample->StartVideoDecoder();
    cout << " Start ret::" << ret << endl;
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, ret);
    ASSERT_EQ(0, vDecSample->errCount);
    ret = vDecSample->Stop();
    cout << " Start ret::" << ret << endl;
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->Start();
    cout << " Start ret::" << ret << endl;
    ASSERT_EQ(AV_ERR_OK, ret);
}

/**
 * @tc.number    : VIDEO_VC1DEC_STATE_0700
 * @tc.name      : create-configure-start-EOS-stop-release
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decStateNdkTest, VIDEO_VC1DEC_STATE_0700, TestSize.Level3)
{
    int32_t ret = vDecSample->StartVideoDecoder();
    vDecSample->AFTER_EOS_DESTORY_CODEC = false;
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, ret);
    ASSERT_EQ(0, vDecSample->errCount);
    ret = vDecSample->Stop();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->Release();
    ASSERT_EQ(AV_ERR_OK, ret);
}

/**
 * @tc.number    : VIDEO_VC1DEC_STATE_0800
 * @tc.name      : create-configure-start-EOS-stop-reset
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decStateNdkTest, VIDEO_VC1DEC_STATE_0800, TestSize.Level3)
{
    int32_t ret = vDecSample->StartVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample->AFTER_EOS_DESTORY_CODEC = false;
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, ret);
    ASSERT_EQ(0, vDecSample->errCount);
    ret = vDecSample->Stop();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->Reset();
    ASSERT_EQ(AV_ERR_OK, ret);
}

/**
 * @tc.number    : VIDEO_VC1DEC_STATE_0900
 * @tc.name      : create-configure-start-EOS-flush
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decStateNdkTest, VIDEO_VC1DEC_STATE_0900, TestSize.Level3)
{
    int32_t ret = vDecSample->StartVideoDecoder();
    vDecSample->AFTER_EOS_DESTORY_CODEC = false;
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, ret);
    ASSERT_EQ(0, vDecSample->errCount);
    ret = vDecSample->Flush();
    ASSERT_EQ(AV_ERR_OK, ret);
}

/**
 * @tc.number    : VIDEO_VC1DEC_STATE_1000
 * @tc.name      : create-configure-start-EOS-flush-start
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decStateNdkTest, VIDEO_VC1DEC_STATE_1000, TestSize.Level3)
{
    int32_t ret = vDecSample->StartVideoDecoder();
    vDecSample->AFTER_EOS_DESTORY_CODEC = false;
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, ret);
    ASSERT_EQ(0, vDecSample->errCount);
    ret = vDecSample->Flush();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->Start();
    ASSERT_EQ(AV_ERR_OK, ret);
}

/**
 * @tc.number    : VIDEO_VC1DEC_STATE_1100
 * @tc.name      : create-configure-start-EOS-flush-stop
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decStateNdkTest, VIDEO_VC1DEC_STATE_1100, TestSize.Level3)
{
    int32_t ret = vDecSample->StartVideoDecoder();
    vDecSample->AFTER_EOS_DESTORY_CODEC = false;
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, ret);
    ASSERT_EQ(0, vDecSample->errCount);
    ret = vDecSample->Flush();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->Stop();
    ASSERT_EQ(AV_ERR_OK, ret);
}

/**
 * @tc.number    : VIDEO_VC1DEC_STATE_1200
 * @tc.name      : create-configure-start-EOS-flush-reset
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decStateNdkTest, VIDEO_VC1DEC_STATE_1200, TestSize.Level3)
{
    int32_t ret = vDecSample->StartVideoDecoder();
    vDecSample->AFTER_EOS_DESTORY_CODEC = false;
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, ret);
    ASSERT_EQ(0, vDecSample->errCount);
    ret = vDecSample->Flush();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->Reset();
    ASSERT_EQ(AV_ERR_OK, ret);
}

/**
 * @tc.number    : VIDEO_VC1DEC_STATE_1300
 * @tc.name      : create-configure-start-EOS-flush-error
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decStateNdkTest, VIDEO_VC1DEC_STATE_1300, TestSize.Level3)
{
    int32_t ret = vDecSample->StartVideoDecoder();
    vDecSample->AFTER_EOS_DESTORY_CODEC = false;
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, ret);
    ASSERT_EQ(0, vDecSample->errCount);
    ret = vDecSample->Flush();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->Release();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->ConfigureVideoDecoder();
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_VC1DEC_STATE_1400
 * @tc.name      : create-configure-start-EOS-reset-configure
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decStateNdkTest, VIDEO_VC1DEC_STATE_1400, TestSize.Level3)
{
    int32_t ret = vDecSample->StartVideoDecoder();
    vDecSample->AFTER_EOS_DESTORY_CODEC = false;
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, ret);
    ASSERT_EQ(0, vDecSample->errCount);
    ret = vDecSample->Reset();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->ConfigureVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
}

/**
 * @tc.number    : VIDEO_VC1DEC_STATE_1500
 * @tc.name      : create-configure-start-EOS-reset-release
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decStateNdkTest, VIDEO_VC1DEC_STATE_1500, TestSize.Level3)
{
    int32_t ret = vDecSample->StartVideoDecoder();
    vDecSample->AFTER_EOS_DESTORY_CODEC = false;
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, ret);
    ASSERT_EQ(0, vDecSample->errCount);
    ret = vDecSample->Reset();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->Release();
    ASSERT_EQ(AV_ERR_OK, ret);
}

/**
 * @tc.number    : VIDEO_VC1DEC_STATE_1600
 * @tc.name      : create-configure-start-EOS-reset-error
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decStateNdkTest, VIDEO_VC1DEC_STATE_1600, TestSize.Level3)
{
    int32_t ret = vDecSample->StartVideoDecoder();
    vDecSample->AFTER_EOS_DESTORY_CODEC = false;
    vDecSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, ret);
    ASSERT_EQ(0, vDecSample->errCount);
    ret = vDecSample->Reset();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->Start();
    ASSERT_EQ(AV_ERR_INVALID_STATE, ret);
    ret = vDecSample->Stop();
    ASSERT_EQ(AV_ERR_INVALID_STATE, ret);
    ret = vDecSample->Flush();
    ASSERT_EQ(AV_ERR_INVALID_STATE, ret);
}

/**
 * @tc.number    : VIDEO_VC1DEC_STATE_1700
 * @tc.name      : create-configure-start-flush-start-flush
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decStateNdkTest, VIDEO_VC1DEC_STATE_1700, TestSize.Level3)
{
    int32_t ret = vDecSample->Start();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->Flush();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->Start();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->Flush();
    ASSERT_EQ(AV_ERR_OK, ret);
}

/**
 * @tc.number    : VIDEO_VC1DEC_STATE_1800
 * @tc.name      : create-configure-start-flush-start-eos
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decStateNdkTest, VIDEO_VC1DEC_STATE_1800, TestSize.Level3)
{
    int32_t ret = vDecSample->Start();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->Flush();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->Start();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->state_EOS();
    ASSERT_EQ(AV_ERR_OK, ret);
}

/**
 * @tc.number    : VIDEO_VC1DEC_STATE_1900
 * @tc.name      : create-configure-start-flush-start-stop
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decStateNdkTest, VIDEO_VC1DEC_STATE_1900, TestSize.Level3)
{
    int32_t ret = vDecSample->Start();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->Flush();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->Start();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->Stop();
    ASSERT_EQ(AV_ERR_OK, ret);
}

/**
 * @tc.number    : VIDEO_VC1DEC_STATE_2000
 * @tc.name      : create-configure-start-flush-start-reset
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decStateNdkTest, VIDEO_VC1DEC_STATE_2000, TestSize.Level3)
{
    int32_t ret = vDecSample->Start();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->Flush();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->Start();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->Reset();
    ASSERT_EQ(AV_ERR_OK, ret);
}

/**
 * @tc.number    : VIDEO_VC1DEC_STATE_2100
 * @tc.name      : create-configure-start-flush-start-error
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decStateNdkTest, VIDEO_VC1DEC_STATE_2100, TestSize.Level3)
{
    int32_t ret = vDecSample->Start();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->Flush();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->Start();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->ConfigureVideoDecoder();
    ASSERT_EQ(AV_ERR_INVALID_STATE, ret);
    ret = vDecSample->SetVideoDecoderCallback();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->Release();
    ASSERT_EQ(AV_ERR_OK, ret);
}

/**
 * @tc.number    : VIDEO_VC1DEC_STATE_2200
 * @tc.name      : create-configure-start-flush-start
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decStateNdkTest, VIDEO_VC1DEC_STATE_2200, TestSize.Level3)
{
    int32_t ret = vDecSample->Start();
    cout << " Start ret::" << ret << endl;
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->Flush();
    cout << " Flush ret::" << ret << endl;
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->Stop();
    cout << " Stop ret::" << ret << endl;
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->Start();
    cout << " Start2 ret::" << ret << endl;
    ASSERT_EQ(AV_ERR_OK, ret);
}

/**
 * @tc.number    : VIDEO_VC1DEC_STATE_2300
 * @tc.name      : create-configure-start-flush-stop-start
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decStateNdkTest, VIDEO_VC1DEC_STATE_2300, TestSize.Level3)
{
    int32_t ret = vDecSample->Start();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->Flush();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->Stop();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->Release();
    ASSERT_EQ(AV_ERR_OK, ret);
}

/**
 * @tc.number    : VIDEO_VC1DEC_STATE_2400
 * @tc.name      : create-configure-start-flush-stop-reset
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decStateNdkTest, VIDEO_VC1DEC_STATE_2400, TestSize.Level3)
{
    int32_t ret = vDecSample->Start();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->Flush();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->Stop();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->Reset();
    ASSERT_EQ(AV_ERR_OK, ret);
}

/**
 * @tc.number    : VIDEO_VC1DEC_STATE_2500
 * @tc.name      : create-configure-start-flush-stop-error
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decStateNdkTest, VIDEO_VC1DEC_STATE_2500, TestSize.Level3)
{
    int32_t ret = vDecSample->Start();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->Flush();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->Stop();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->Flush();
    ASSERT_EQ(AV_ERR_INVALID_STATE, ret);
    ret = vDecSample->SetVideoDecoderCallback();
    ASSERT_EQ(AV_ERR_OK, ret);
}

/**
 * @tc.number    : VIDEO_VC1DEC_STATE_2600
 * @tc.name      : create-configure-start-flush-reset-configure
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decStateNdkTest, VIDEO_VC1DEC_STATE_2600, TestSize.Level3)
{
    int32_t ret = vDecSample->Start();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->Flush();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->Reset();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->ConfigureVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
}

/**
 * @tc.number    : VIDEO_VC1DEC_STATE_2700
 * @tc.name      : create-configure-start-flush-reset-release
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decStateNdkTest, VIDEO_VC1DEC_STATE_2700, TestSize.Level3)
{
    int32_t ret = vDecSample->Start();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->Flush();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->Reset();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->Release();
    ASSERT_EQ(AV_ERR_OK, ret);
}

/**
 * @tc.number    : VIDEO_VC1DEC_STATE_2800
 * @tc.name      : create-configure-start-flush-reset-error
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decStateNdkTest, VIDEO_VC1DEC_STATE_2800, TestSize.Level3)
{
    int32_t ret = vDecSample->Start();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->Flush();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->Reset();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->Start();
    ASSERT_EQ(AV_ERR_INVALID_STATE, ret);
    ret = vDecSample->Stop();
    ASSERT_EQ(AV_ERR_INVALID_STATE, ret);
    ret = vDecSample->Flush();
    ASSERT_EQ(AV_ERR_INVALID_STATE, ret);
    ret = vDecSample->SetVideoDecoderCallback();
    ASSERT_EQ(AV_ERR_OK, ret);
}

/**
 * @tc.number    : VIDEO_VC1DEC_STATE_2900
 * @tc.name      : create-configure-start-reset-configure
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decStateNdkTest, VIDEO_VC1DEC_STATE_2900, TestSize.Level3)
{
    int32_t ret = vDecSample->Start();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->Reset();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->ConfigureVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
}

/**
 * @tc.number    : VIDEO_VC1DEC_STATE_3000
 * @tc.name      : create-configure-start-reset-release
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decStateNdkTest, VIDEO_VC1DEC_STATE_3000, TestSize.Level3)
{
    int32_t ret = vDecSample->Start();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->Reset();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->Release();
    ASSERT_EQ(AV_ERR_OK, ret);
}

/**
 * @tc.number    : VIDEO_VC1DEC_STATE_3100
 * @tc.name      : create-configure-start-reset-error
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decStateNdkTest, VIDEO_VC1DEC_STATE_3100, TestSize.Level3)
{
    int32_t ret = vDecSample->Start();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->Reset();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->Start();
    ASSERT_EQ(AV_ERR_INVALID_STATE, ret);
    ret = vDecSample->Stop();
    ASSERT_EQ(AV_ERR_INVALID_STATE, ret);
    ret = vDecSample->Flush();
    ASSERT_EQ(AV_ERR_INVALID_STATE, ret);
    ret = vDecSample->SetVideoDecoderCallback();
    ASSERT_EQ(AV_ERR_OK, ret);
}

/**
 * @tc.number    : VIDEO_VC1DEC_STATE_3200
 * @tc.name      : create-configure-start-error
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decStateNdkTest, VIDEO_VC1DEC_STATE_3200, TestSize.Level3)
{
    int32_t ret = vDecSample->Start();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->ConfigureVideoDecoder();
    ASSERT_EQ(AV_ERR_INVALID_STATE, ret);
    ret = vDecSample->SetVideoDecoderCallback();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->Release();
    ASSERT_EQ(AV_ERR_OK, ret);
}

/**
 * @tc.number    : VIDEO_VC1DEC_STATE_3300
 * @tc.name      : create-configure-reset-configure
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decStateNdkTest, VIDEO_VC1DEC_STATE_3300, TestSize.Level3)
{
    int32_t ret = vDecSample->Reset();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample->ConfigureVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
}

/**
 * @tc.number    : VIDEO_VC1DEC_STATE_3400
 * @tc.name      : create-configure-release
 * @tc.desc      : function test
 */
HWTEST_F(Vc1decStateNdkTest, VIDEO_VC1DEC_STATE_3400, TestSize.Level3)
{
    int32_t ret = vDecSample->Release();
    ASSERT_EQ(AV_ERR_OK, ret);
}
} // namespace