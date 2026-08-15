/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <string>
#include <memory>
#include <unistd.h>
#include "gtest/gtest.h"
#include "native_avcodec_videoencoder.h"
#include "native_averrors.h"
#include "videoenc_api11_sample.h"
#include "native_avcodec_base.h"
#include "avcodec_codec_name.h"
#include "native_avcapability.h"
#include "avcodec_suspend.h"
#include "avcodec_errors.h"

namespace {
OH_AVCapability *cap = nullptr;
constexpr uint32_t CODEC_NAME_SIZE = 128;
char g_codecName[CODEC_NAME_SIZE] = {};
}

namespace OHOS {
namespace MediaAVCodec {
class AdjustmentRateModuleTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp() override;
    void TearDown() override;
};
}
}

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace OHOS::MediaAVCodec;
using namespace testing::ext;

void AdjustmentRateModuleTest::SetUpTestCase()
{
    cap = OH_AVCodec_GetCapabilityByCategory(
        OH_AVCODEC_MIMETYPE_VIDEO_AVC, true, HARDWARE);
    const char *tmpCodecName = OH_AVCapability_GetName(cap);
    if (memcpy_s(g_codecName, sizeof(g_codecName),
        tmpCodecName, strlen(tmpCodecName)) != 0) {
        cout << "memcpy failed" << endl;
    }
    cout << "codecname: " << g_codecName << endl;
}

void AdjustmentRateModuleTest::TearDownTestCase()
{
    cap = nullptr;
    if (memset_s(g_codecName, sizeof(g_codecName), 0, sizeof(g_codecName)) != 0) {
        cout << "memset failed" << endl;
    }
}

void AdjustmentRateModuleTest::SetUp() {}

void AdjustmentRateModuleTest::TearDown() {}

namespace {
constexpr int32_t TEST_EXPECT_OK = AVCS_ERR_OK;
}

/**
 * @tc.number    : SET_CODEC_PERFORMANCE_WITH_PID_0010
 * @tc.name      : SetCodecPerformanceWithPid with CodecPerformanceLevel::HIGH
 * @tc.desc      : Create encoder then call framework API,
 *                 SetCodecPerformanceWithPid returns OK
 *                 with current pid and CodecPerformanceLevel::HIGH
 */
HWTEST_F(AdjustmentRateModuleTest, SET_CODEC_PERFORMANCE_WITH_PID_0010, TestSize.Level1)
{
    auto vEncSample = make_unique<VEncAPI11Sample>();
    ASSERT_EQ(AV_ERR_OK,
        vEncSample->CreateVideoEncoder(g_codecName));
    ASSERT_EQ(AV_ERR_OK,
        vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK,
        vEncSample->ConfigureVideoEncoder());

    pid_t pid = getpid();
    int32_t ret = AVCodecSuspend::SetCodecPerformanceWithPid(pid, CodecPerformanceLevel::HIGH);
    EXPECT_EQ(ret, TEST_EXPECT_OK);
    EXPECT_NE(ret, AVCS_ERR_SERVICE_DIED);
}

/**
 * @tc.number    : SET_CODEC_PERFORMANCE_WITH_PID_0020
 * @tc.name      : SetCodecPerformanceWithPid with CodecPerformanceLevel::DEFAULT
 * @tc.desc      : Create encoder then call framework API,
 *                 SetCodecPerformanceWithPid returns OK
 *                 with current pid and CodecPerformanceLevel::DEFAULT
 */
HWTEST_F(AdjustmentRateModuleTest, SET_CODEC_PERFORMANCE_WITH_PID_0020, TestSize.Level1)
{
    auto vEncSample = make_unique<VEncAPI11Sample>();
    ASSERT_EQ(AV_ERR_OK,
        vEncSample->CreateVideoEncoder(g_codecName));
    ASSERT_EQ(AV_ERR_OK,
        vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK,
        vEncSample->ConfigureVideoEncoder());

    pid_t pid = getpid();
    int32_t ret = AVCodecSuspend::SetCodecPerformanceWithPid(pid, CodecPerformanceLevel::DEFAULT);
    EXPECT_EQ(ret, TEST_EXPECT_OK);
    EXPECT_NE(ret, AVCS_ERR_SERVICE_DIED);
}
