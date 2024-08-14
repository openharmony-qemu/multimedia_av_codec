/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
#include <string>
#include "gtest/gtest.h"
#include "native_avcodec_videodecoder.h"
#include "native_averrors.h"
#include "videodec_api11_sample.h"
#include "native_avcodec_base.h"
#include "avcodec_codec_name.h"
#include "native_avcapability.h"
#include "native_buffer.h"

#define MAX_THREAD 16

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;

namespace OHOS {
namespace Media {
class HwdecHdr2SdrStateNdkTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp() override;
    void TearDown() override;
    void InputFunc();
    void OutputFunc();
    void Release();
    int32_t Stop();
};
} // namespace Media
} // namespace OHOS

VDecAPI11Sample *vDecSample2 = NULL;
void HwdecHdr2SdrStateNdkTest::SetUpTestCase() {}
void HwdecHdr2SdrStateNdkTest::TearDownTestCase() {}

void HwdecHdr2SdrStateNdkTest::SetUp(void)
{
    vDecSample2 = new VDecAPI11Sample();
    vDecSample2->SF_OUTPUT = true;
    if (!access("/system/lib64/media/", 0)) {
        vDecSample2->TRANSFER_FLAG = true;
    } else {
        vDecSample2->TRANSFER_FLAG = false;
    }
    OH_AVCapability *cap_hevc2 = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_HEVC, false, HARDWARE);
    string g_codecNameHEVC2 = OH_AVCapability_GetName(cap_hevc2);
    cout << "g_codecNameHEVC2: " << g_codecNameHEVC2 << endl;
    int32_t ret = vDecSample2->CreateVideoDecoder(g_codecNameHEVC2);
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->SetVideoDecoderCallback();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->ConfigureVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample2->INP_DIR = "/data/test/media/hlgHdrVivid.h265";
}

void HwdecHdr2SdrStateNdkTest::TearDown()
{
    vDecSample2->Release();
    delete vDecSample2;
    vDecSample2 = nullptr;
}

namespace {
/**
 * @tc.number    : HEVC_HW_HDR2SDR_STATE_0099
 * @tc.name      : create-configure-start-EOS-stop-start-surface
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrStateNdkTest, HEVC_HW_HDR2SDR_STATE_0099, TestSize.Level2)
{
    vDecSample2->SF_OUTPUT = true;
    vDecSample2->AFTER_EOS_DESTORY_CODEC = false;
    int32_t ret = vDecSample2->DecodeSetSurface();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->StartVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->StartDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample2->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, ret);
    ASSERT_EQ(0, vDecSample2->errCount);
    ret = vDecSample2->Stop();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Start();
    ASSERT_EQ(AV_ERR_OK, ret);
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_STATE_0100
 * @tc.name      : create-configure-start-EOS-stop-release-surface
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrStateNdkTest, HEVC_HW_HDR2SDR_STATE_0100, TestSize.Level2)
{
    vDecSample2->SF_OUTPUT = true;
    vDecSample2->AFTER_EOS_DESTORY_CODEC = false;
    int32_t ret = vDecSample2->DecodeSetSurface();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->StartVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->StartDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample2->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, ret);
    ASSERT_EQ(0, vDecSample2->errCount);
    ret = vDecSample2->Stop();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Release();
    ASSERT_EQ(AV_ERR_OK, ret);
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_STATE_0200
 * @tc.name      : create-configure-start-EOS-stop-reset-surface
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrStateNdkTest, HEVC_HW_HDR2SDR_STATE_0200, TestSize.Level2)
{
    vDecSample2->SF_OUTPUT = true;
    vDecSample2->AFTER_EOS_DESTORY_CODEC = false;
    int32_t ret = vDecSample2->DecodeSetSurface();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->StartVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->StartDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample2->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, ret);
    ASSERT_EQ(0, vDecSample2->errCount);
    ret = vDecSample2->Stop();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Reset();
    ASSERT_EQ(AV_ERR_OK, ret);
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_STATE_0300
 * @tc.name      : create-configure-start-EOS-flush-surface
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrStateNdkTest, HEVC_HW_HDR2SDR_STATE_0300, TestSize.Level2)
{
    vDecSample2->SF_OUTPUT = true;
    vDecSample2->AFTER_EOS_DESTORY_CODEC = false;
    int32_t ret = vDecSample2->DecodeSetSurface();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->StartVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->StartDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample2->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, ret);
    ASSERT_EQ(0, vDecSample2->errCount);
    ret = vDecSample2->Flush();
    ASSERT_EQ(AV_ERR_OK, ret);
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_STATE_0400
 * @tc.name      : create-configure-start-EOS-flush-start-surface
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrStateNdkTest, HEVC_HW_HDR2SDR_STATE_0400, TestSize.Level2)
{
    vDecSample2->SF_OUTPUT = true;
    vDecSample2->AFTER_EOS_DESTORY_CODEC = false;
    int32_t ret = vDecSample2->DecodeSetSurface();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->StartVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->StartDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample2->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, ret);
    ASSERT_EQ(0, vDecSample2->errCount);
    ret = vDecSample2->Flush();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Start();
    ASSERT_EQ(AV_ERR_OK, ret);
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_STATE_0500
 * @tc.name      : create-configure-start-EOS-flush-stop-surface
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrStateNdkTest, HEVC_HW_HDR2SDR_STATE_0500, TestSize.Level2)
{
    vDecSample2->SF_OUTPUT = true;
    vDecSample2->AFTER_EOS_DESTORY_CODEC = false;
    int32_t ret = vDecSample2->DecodeSetSurface();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->StartVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->StartDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample2->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, ret);
    ASSERT_EQ(0, vDecSample2->errCount);
    ret = vDecSample2->Flush();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Stop();
    ASSERT_EQ(AV_ERR_OK, ret);
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_STATE_0600
 * @tc.name      : create-configure-start-EOS-flush-reset-surface
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrStateNdkTest, HEVC_HW_HDR2SDR_STATE_0600, TestSize.Level2)
{
    vDecSample2->SF_OUTPUT = true;
    vDecSample2->AFTER_EOS_DESTORY_CODEC = false;
    int32_t ret = vDecSample2->DecodeSetSurface();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->StartVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->StartDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample2->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, ret);
    ASSERT_EQ(0, vDecSample2->errCount);
    ret = vDecSample2->Flush();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Reset();
    ASSERT_EQ(AV_ERR_OK, ret);
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_STATE_0700
 * @tc.name      : create-configure-start-EOS-flush-error-surface
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrStateNdkTest, HEVC_HW_HDR2SDR_STATE_0700, TestSize.Level2)
{
    vDecSample2->SF_OUTPUT = true;
    vDecSample2->AFTER_EOS_DESTORY_CODEC = false;
    int32_t ret = vDecSample2->DecodeSetSurface();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->StartVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->StartDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample2->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, ret);
    ASSERT_EQ(0, vDecSample2->errCount);
    ret = vDecSample2->Flush();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Release();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->ConfigureVideoDecoder();
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_STATE_0800
 * @tc.name      : create-configure-start-EOS-reset-configure-surface
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrStateNdkTest, HEVC_HW_HDR2SDR_STATE_0800, TestSize.Level2)
{
    vDecSample2->SF_OUTPUT = true;
    vDecSample2->AFTER_EOS_DESTORY_CODEC = false;
    int32_t ret = vDecSample2->DecodeSetSurface();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->StartVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->StartDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample2->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, ret);
    ASSERT_EQ(0, vDecSample2->errCount);
    ret = vDecSample2->Reset();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->ConfigureVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_STATE_0900
 * @tc.name      : create-configure-start-EOS-reset-release-surface
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrStateNdkTest, HEVC_HW_HDR2SDR_STATE_0900, TestSize.Level2)
{
    vDecSample2->SF_OUTPUT = true;
    vDecSample2->AFTER_EOS_DESTORY_CODEC = false;
    int32_t ret = vDecSample2->DecodeSetSurface();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->StartVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->StartDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample2->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, ret);
    ASSERT_EQ(0, vDecSample2->errCount);
    ret = vDecSample2->Reset();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Release();
    ASSERT_EQ(AV_ERR_OK, ret);
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_STATE_1000
 * @tc.name      : create-configure-start-EOS-reset-error-surface
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrStateNdkTest, HEVC_HW_HDR2SDR_STATE_1000, TestSize.Level2)
{
    vDecSample2->SF_OUTPUT = true;
    vDecSample2->AFTER_EOS_DESTORY_CODEC = false;
    int32_t ret = vDecSample2->DecodeSetSurface();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->StartVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->StartDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample2->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, ret);
    ASSERT_EQ(0, vDecSample2->errCount);
    ret = vDecSample2->Reset();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Start();
    ASSERT_EQ(AV_ERR_INVALID_STATE, ret);
    ret = vDecSample2->Stop();
    ASSERT_EQ(AV_ERR_INVALID_STATE, ret);
    ret = vDecSample2->Flush();
    ASSERT_EQ(AV_ERR_INVALID_STATE, ret);
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_STATE_1100
 * @tc.name      : create-configure-error-surface
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrStateNdkTest, HEVC_HW_HDR2SDR_STATE_1100, TestSize.Level2)
{
    vDecSample2->SF_OUTPUT = true;
    int32_t ret = vDecSample2->DecodeSetSurface();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Stop();
    ASSERT_EQ(AV_ERR_INVALID_STATE, ret);
    ret = vDecSample2->Flush();
    ASSERT_EQ(AV_ERR_INVALID_STATE, ret);
    ret = vDecSample2->SetVideoDecoderCallback();
    ASSERT_EQ(AV_ERR_OK, ret);
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_STATE_1200
 * @tc.name      : create-configure-start-stop-start-surface
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrStateNdkTest, HEVC_HW_HDR2SDR_STATE_1200, TestSize.Level2)
{
    vDecSample2->SF_OUTPUT = true;
    int32_t ret = vDecSample2->DecodeSetSurface();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Prepare();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Start();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Stop();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Start();
    ASSERT_EQ(AV_ERR_OK, ret);
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_STATE_1300
 * @tc.name      : create-configure-start-stop-release-surface
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrStateNdkTest, HEVC_HW_HDR2SDR_STATE_1300, TestSize.Level2)
{
    vDecSample2->SF_OUTPUT = true;
    int32_t ret = vDecSample2->DecodeSetSurface();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Prepare();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Start();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Stop();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Release();
    ASSERT_EQ(AV_ERR_OK, ret);
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_STATE_1400
 * @tc.name      : create-configure-start-stop-reset-surface
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrStateNdkTest, HEVC_HW_HDR2SDR_STATE_1400, TestSize.Level2)
{
    vDecSample2->SF_OUTPUT = true;
    int32_t ret = vDecSample2->DecodeSetSurface();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Prepare();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Start();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Stop();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Reset();
    ASSERT_EQ(AV_ERR_OK, ret);
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_STATE_1500
 * @tc.name      : create-configure-start-stop-error-surface
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrStateNdkTest, HEVC_HW_HDR2SDR_STATE_1500, TestSize.Level2)
{
    vDecSample2->SF_OUTPUT = true;
    int32_t ret = vDecSample2->DecodeSetSurface();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Prepare();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Start();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Stop();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Flush();
    ASSERT_EQ(AV_ERR_INVALID_STATE, ret);
    ret = vDecSample2->SetVideoDecoderCallback();
    ASSERT_EQ(AV_ERR_OK, ret);
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_STATE_1600
 * @tc.name      : create-configure-start-EOS-reset-error-surface
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrStateNdkTest, HEVC_HW_HDR2SDR_STATE_1600, TestSize.Level2)
{
    vDecSample2->SF_OUTPUT = true;
    int32_t ret = vDecSample2->DecodeSetSurface();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->StartVideoDecoder();
    vDecSample2->AFTER_EOS_DESTORY_CODEC = false;
    ret = vDecSample2->StartDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample2->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, ret);
    ASSERT_EQ(0, vDecSample2->errCount);
    ret = vDecSample2->Reset();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Start();
    ASSERT_EQ(AV_ERR_INVALID_STATE, ret);
    ret = vDecSample2->Stop();
    ASSERT_EQ(AV_ERR_INVALID_STATE, ret);
    ret = vDecSample2->Flush();
    ASSERT_EQ(AV_ERR_INVALID_STATE, ret);
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_STATE_1700
 * @tc.name      : create-configure-start-flush-start-flush-surface
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrStateNdkTest, HEVC_HW_HDR2SDR_STATE_1700, TestSize.Level2)
{
    vDecSample2->SF_OUTPUT = true;
    int32_t ret = vDecSample2->DecodeSetSurface();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Prepare();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Start();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Flush();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Start();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Flush();
    ASSERT_EQ(AV_ERR_OK, ret);
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_STATE_1800
 * @tc.name      : create-configure-start-flush-start-eos-surface
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrStateNdkTest, HEVC_HW_HDR2SDR_STATE_1800, TestSize.Level2)
{
    vDecSample2->SF_OUTPUT = true;
    int32_t ret = vDecSample2->DecodeSetSurface();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Prepare();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Start();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Flush();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Start();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->state_EOS();
    ASSERT_EQ(AV_ERR_OK, ret);
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_STATE_1900
 * @tc.name      : create-configure-start-flush-start-stop-surface
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrStateNdkTest, HEVC_HW_HDR2SDR_STATE_1900, TestSize.Level2)
{
    vDecSample2->SF_OUTPUT = true;
    int32_t ret = vDecSample2->DecodeSetSurface();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Prepare();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Start();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Flush();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Start();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Stop();
    ASSERT_EQ(AV_ERR_OK, ret);
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_STATE_2000
 * @tc.name      : create-configure-start-flush-start-reset-surface
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrStateNdkTest, HEVC_HW_HDR2SDR_STATE_2000, TestSize.Level2)
{
    vDecSample2->SF_OUTPUT = true;
    int32_t ret = vDecSample2->DecodeSetSurface();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Prepare();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Start();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Flush();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Start();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Reset();
    ASSERT_EQ(AV_ERR_OK, ret);
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_STATE_2100
 * @tc.name      : create-configure-start-flush-start-error-surface
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrStateNdkTest, HEVC_HW_HDR2SDR_STATE_2100, TestSize.Level2)
{
    vDecSample2->SF_OUTPUT = true;
    int32_t ret = vDecSample2->DecodeSetSurface();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Prepare();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Start();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Flush();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Start();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->ConfigureVideoDecoder();
    ASSERT_EQ(AV_ERR_INVALID_STATE, ret);
    ret = vDecSample2->SetVideoDecoderCallback();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Release();
    ASSERT_EQ(AV_ERR_OK, ret);
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_STATE_2200
 * @tc.name      : create-configure-start-flush-start-error-surface
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrStateNdkTest, HEVC_HW_HDR2SDR_STATE_2200, TestSize.Level2)
{
    vDecSample2->SF_OUTPUT = true;
    int32_t ret = vDecSample2->DecodeSetSurface();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Prepare();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Start();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Flush();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Stop();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Start();
    ASSERT_EQ(AV_ERR_OK, ret);
    cout << "set callback" << endl;
    ret = vDecSample2->SetVideoDecoderCallback();
    ASSERT_EQ(AV_ERR_OK, ret);
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_STATE_2300
 * @tc.name      : create-configure-start-flush-stop-start-surface
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrStateNdkTest, HEVC_HW_HDR2SDR_STATE_2300, TestSize.Level2)
{
    vDecSample2->SF_OUTPUT = true;
    int32_t ret = vDecSample2->DecodeSetSurface();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Prepare();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Start();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Flush();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Stop();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Release();
    ASSERT_EQ(AV_ERR_OK, ret);
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_STATE_2400
 * @tc.name      : create-configure-start-flush-stop-reset-surface
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrStateNdkTest, HEVC_HW_HDR2SDR_STATE_2400, TestSize.Level2)
{
    vDecSample2->SF_OUTPUT = true;
    int32_t ret = vDecSample2->DecodeSetSurface();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Prepare();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Start();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Flush();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Stop();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Reset();
    ASSERT_EQ(AV_ERR_OK, ret);
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_STATE_2500
 * @tc.name      : create-configure-start-flush-stop-error-surface
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrStateNdkTest, HEVC_HW_HDR2SDR_STATE_2500, TestSize.Level2)
{
    vDecSample2->SF_OUTPUT = true;
    int32_t ret = vDecSample2->DecodeSetSurface();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Prepare();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Start();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Flush();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Stop();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Flush();
    ASSERT_EQ(AV_ERR_INVALID_STATE, ret);
    ret = vDecSample2->SetVideoDecoderCallback();
    ASSERT_EQ(AV_ERR_OK, ret);
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_STATE_2600
 * @tc.name      : create-configure-start-flush-reset-configure-surface
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrStateNdkTest, HEVC_HW_HDR2SDR_STATE_2600, TestSize.Level2)
{
    vDecSample2->SF_OUTPUT = true;
    int32_t ret = vDecSample2->DecodeSetSurface();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Prepare();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Start();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Flush();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Reset();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->ConfigureVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_STATE_2700
 * @tc.name      : create-configure-start-flush-reset-release-surface
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrStateNdkTest, HEVC_HW_HDR2SDR_STATE_2700, TestSize.Level2)
{
    vDecSample2->SF_OUTPUT = true;
    int32_t ret = vDecSample2->DecodeSetSurface();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Prepare();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Start();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Flush();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Reset();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Release();
    ASSERT_EQ(AV_ERR_OK, ret);
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_STATE_2800
 * @tc.name      : create-configure-start-flush-reset-error-surface
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrStateNdkTest, HEVC_HW_HDR2SDR_STATE_2800, TestSize.Level2)
{
    vDecSample2->SF_OUTPUT = true;
    int32_t ret = vDecSample2->DecodeSetSurface();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Prepare();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Start();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Flush();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Reset();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Start();
    ASSERT_EQ(AV_ERR_INVALID_STATE, ret);
    ret = vDecSample2->Stop();
    ASSERT_EQ(AV_ERR_INVALID_STATE, ret);
    ret = vDecSample2->Flush();
    ASSERT_EQ(AV_ERR_INVALID_STATE, ret);
    ret = vDecSample2->SetVideoDecoderCallback();
    ASSERT_EQ(AV_ERR_OK, ret);
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_STATE_2900
 * @tc.name      : create-configure-start-reset-configure-surface
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrStateNdkTest, HEVC_HW_HDR2SDR_STATE_2900, TestSize.Level2)
{
    vDecSample2->SF_OUTPUT = true;
    int32_t ret = vDecSample2->DecodeSetSurface();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Prepare();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Start();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Reset();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->ConfigureVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_STATE_3000
 * @tc.name      : create-configure-start-reset-release-surface
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrStateNdkTest, HEVC_HW_HDR2SDR_STATE_3000, TestSize.Level2)
{
    vDecSample2->SF_OUTPUT = true;
    int32_t ret = vDecSample2->DecodeSetSurface();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Prepare();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Start();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Reset();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Release();
    ASSERT_EQ(AV_ERR_OK, ret);
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_STATE_3100
 * @tc.name      : create-configure-start-reset-error-surface
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrStateNdkTest, HEVC_HW_HDR2SDR_STATE_3100, TestSize.Level2)
{
    vDecSample2->SF_OUTPUT = true;
    int32_t ret = vDecSample2->DecodeSetSurface();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Prepare();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Start();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Reset();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Start();
    ASSERT_EQ(AV_ERR_INVALID_STATE, ret);
    ret = vDecSample2->Stop();
    ASSERT_EQ(AV_ERR_INVALID_STATE, ret);
    ret = vDecSample2->Flush();
    ASSERT_EQ(AV_ERR_INVALID_STATE, ret);
    ret = vDecSample2->SetVideoDecoderCallback();
    ASSERT_EQ(AV_ERR_OK, ret);
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_STATE_3200
 * @tc.name      : create-configure-start-error-surface
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrStateNdkTest, HEVC_HW_HDR2SDR_STATE_3200, TestSize.Level2)
{
    vDecSample2->SF_OUTPUT = true;
    int32_t ret = vDecSample2->DecodeSetSurface();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Prepare();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Start();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->ConfigureVideoDecoder();
    ASSERT_EQ(AV_ERR_INVALID_STATE, ret);
    ret = vDecSample2->SetVideoDecoderCallback();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Release();
    ASSERT_EQ(AV_ERR_OK, ret);
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_STATE_3300
 * @tc.name      : create-configure-reset-configure-surface
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrStateNdkTest, HEVC_HW_HDR2SDR_STATE_3300, TestSize.Level2)
{
    vDecSample2->SF_OUTPUT = true;
    int32_t ret = vDecSample2->DecodeSetSurface();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Reset();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->ConfigureVideoDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_STATE_3400
 * @tc.name      : create-configure-release-surface
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrStateNdkTest, HEVC_HW_HDR2SDR_STATE_3400, TestSize.Level2)
{
    vDecSample2->SF_OUTPUT = true;
    int32_t ret = vDecSample2->DecodeSetSurface();
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->Release();
    ASSERT_EQ(AV_ERR_OK, ret);
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_STATE_3500
 * @tc.name      : Flush or stop in buffe decoder callback function-surface
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrStateNdkTest, HEVC_HW_HDR2SDR_STATE_3500, TestSize.Level1)
{
    vDecSample2->SF_OUTPUT = true;
    int32_t ret = vDecSample2->DecodeSetSurface();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample2->inputCallbackFlush = true;
    ret = vDecSample2->StartVideoDecoder();
    vDecSample2->AFTER_EOS_DESTORY_CODEC = false;
    ret = vDecSample2->StartDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample2->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, ret);
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_STATE_3600
 * @tc.name      : Flush or stop in buffe decoder callback function-surface
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrStateNdkTest, HEVC_HW_HDR2SDR_STATE_3600, TestSize.Level1)
{
    vDecSample2->SF_OUTPUT = true;
    int32_t ret = vDecSample2->DecodeSetSurface();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample2->inputCallbackStop = true;
    ret = vDecSample2->StartVideoDecoder();
    vDecSample2->AFTER_EOS_DESTORY_CODEC = false;
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->StartDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample2->WaitForEOS();
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_STATE_3700
 * @tc.name      : Flush or stop in buffe decoder callback function-surface
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrStateNdkTest, HEVC_HW_HDR2SDR_STATE_3700, TestSize.Level1)
{
    vDecSample2->SF_OUTPUT = true;
    int32_t ret = vDecSample2->DecodeSetSurface();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample2->outputCallbackFlush = true;
    ret = vDecSample2->StartVideoDecoder();
    vDecSample2->AFTER_EOS_DESTORY_CODEC = false;
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->StartDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample2->WaitForEOS();
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_STATE_3800
 * @tc.name      : Flush or stop in buffe decoder callback function-surface
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrStateNdkTest, HEVC_HW_HDR2SDR_STATE_3800, TestSize.Level1)
{
    vDecSample2->SF_OUTPUT = true;
    int32_t ret = vDecSample2->DecodeSetSurface();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample2->outputCallbackStop = true;
    ret = vDecSample2->StartVideoDecoder();
    vDecSample2->AFTER_EOS_DESTORY_CODEC = false;
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->StartDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample2->WaitForEOS();
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_STATE_3900
 * @tc.name      : Flush or stop in surf decoder callback function-surface
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrStateNdkTest, HEVC_HW_HDR2SDR_STATE_3900, TestSize.Level1)
{
    vDecSample2->SF_OUTPUT = true;
    int32_t ret = vDecSample2->DecodeSetSurface();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample2->SF_OUTPUT = true;
    vDecSample2->inputCallbackFlush = true;
    ret = vDecSample2->StartVideoDecoder();
    vDecSample2->AFTER_EOS_DESTORY_CODEC = false;
    ret = vDecSample2->StartDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample2->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, ret);
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_STATE_4000
 * @tc.name      : Flush or stop in buffe decoder callback function-surface
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrStateNdkTest, HEVC_HW_HDR2SDR_STATE_4000, TestSize.Level1)
{
    vDecSample2->SF_OUTPUT = true;
    int32_t ret = vDecSample2->DecodeSetSurface();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample2->SF_OUTPUT = true;
    vDecSample2->inputCallbackStop = true;
    ret = vDecSample2->StartVideoDecoder();
    vDecSample2->AFTER_EOS_DESTORY_CODEC = false;
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->StartDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample2->WaitForEOS();
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_STATE_4100
 * @tc.name      : Flush or stop in buffe decoder callback function-surface
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrStateNdkTest, HEVC_HW_HDR2SDR_STATE_4100, TestSize.Level1)
{
    vDecSample2->SF_OUTPUT = true;
    int32_t ret = vDecSample2->DecodeSetSurface();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample2->SF_OUTPUT = true;
    vDecSample2->outputCallbackFlush = true;
    ret = vDecSample2->StartVideoDecoder();
    vDecSample2->AFTER_EOS_DESTORY_CODEC = false;
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->StartDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample2->WaitForEOS();
}

/**
 * @tc.number    : HEVC_HW_HDR2SDR_STATE_4200
 * @tc.name      : Flush or stop in buffe decoder callback function-surface
 * @tc.desc      : function test
 */
HWTEST_F(HwdecHdr2SdrStateNdkTest, HEVC_HW_HDR2SDR_STATE_4200, TestSize.Level1)
{
    vDecSample2->SF_OUTPUT = true;
    int32_t ret = vDecSample2->DecodeSetSurface();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample2->SF_OUTPUT = true;
    vDecSample2->outputCallbackStop = true;
    ret = vDecSample2->StartVideoDecoder();
    vDecSample2->AFTER_EOS_DESTORY_CODEC = false;
    ASSERT_EQ(AV_ERR_OK, ret);
    ret = vDecSample2->StartDecoder();
    ASSERT_EQ(AV_ERR_OK, ret);
    vDecSample2->WaitForEOS();
}
} //namespace