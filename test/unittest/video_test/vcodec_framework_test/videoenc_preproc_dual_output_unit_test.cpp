/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include <gtest/gtest.h>
#include <gtest/hwext/gtest-multithread.h>

#include "native_avcapability.h"
#include "native_avcodec_base.h"
#include "videoenc_preproc_dual_output_sample.h"

using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::MediaAVCodec::PreprocDualOutput;
using namespace testing::ext;

namespace {
class VideoEncPreprocDualOutputTest : public testing::TestWithParam<ScenarioCase> {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
};

bool IsHardwareAvcAvailable()
{
    auto *capability = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_AVC, true, HARDWARE);
    return capability != nullptr;
}
} // namespace

INSTANTIATE_TEST_SUITE_P(, VideoEncPreprocDualOutputTest, testing::ValuesIn(kDualOutputScenarios));

static void RunNoPreprocLikeScenario(const ScenarioCase &scenario)
{
    VideoEncPreprocDualOutputSample sample;
    ASSERT_TRUE(sample.CreatePrimary(OH_AVCODEC_MIMETYPE_VIDEO_AVC)) << scenario.name;
    ASSERT_EQ(sample.ConfigurePrimary(scenario), AV_ERR_OK) << scenario.name;
    ASSERT_EQ(sample.GetPrimarySurface(), AV_ERR_OK) << scenario.name;
    ASSERT_EQ(sample.CreateSecondary(), AV_ERR_OK) << scenario.name;
    ASSERT_EQ(sample.ConfigureSecondary(scenario), AV_ERR_OK) << scenario.name;
    ASSERT_EQ(sample.StartPrimary(), AV_ERR_OK) << scenario.name;
    ASSERT_EQ(sample.StartSecondary(), AV_ERR_OK) << scenario.name;
    ASSERT_EQ(sample.SendPrimaryFrames(8), AV_ERR_OK) << scenario.name;
    ASSERT_TRUE(sample.WaitForDualEos()) << scenario.name;
    EXPECT_TRUE(sample.PrimaryHasData()) << scenario.name;
    EXPECT_TRUE(sample.SecondaryHasData()) << scenario.name;
    EXPECT_TRUE(sample.PrimaryGotEos()) << scenario.name;
    EXPECT_TRUE(sample.SecondaryGotEos()) << scenario.name;
    EXPECT_EQ(sample.PrimaryError(), 0) << scenario.name;
    EXPECT_EQ(sample.SecondaryError(), 0) << scenario.name;
}

static void RunInvalidParameterScenario(const ScenarioCase &scenario)
{
    VideoEncPreprocDualOutputSample sample;
    ASSERT_TRUE(sample.CreatePrimary(OH_AVCODEC_MIMETYPE_VIDEO_AVC)) << scenario.name;
    ASSERT_EQ(sample.ConfigurePrimary(scenario), scenario.expectedResult) << scenario.name;
    EXPECT_FALSE(sample.PrimaryGotEos()) << scenario.name;
    EXPECT_FALSE(sample.SecondaryGotEos()) << scenario.name;
}

static void RunDuplicateSecondaryScenario(const ScenarioCase &scenario)
{
    VideoEncPreprocDualOutputSample sample;
    ASSERT_TRUE(sample.CreatePrimary(OH_AVCODEC_MIMETYPE_VIDEO_AVC)) << scenario.name;
    ASSERT_EQ(sample.ConfigurePrimary(scenario), AV_ERR_OK) << scenario.name;
    ASSERT_EQ(sample.GetPrimarySurface(), AV_ERR_OK) << scenario.name;
    ASSERT_EQ(sample.CreateSecondary(), AV_ERR_OK) << scenario.name;
    ASSERT_EQ(sample.ConfigureSecondary(scenario), AV_ERR_OK) << scenario.name;
    ASSERT_EQ(sample.DuplicateSecondary(), scenario.expectedResult) << scenario.name;
    EXPECT_EQ(sample.DestroySecondary(), AV_ERR_OK) << scenario.name;
    EXPECT_EQ(sample.DestroyPrimary(), AV_ERR_OK);
}

static void RunInvalidLifecycleScenario(const ScenarioCase &scenario)
{
    VideoEncPreprocDualOutputSample sample;
    ASSERT_TRUE(sample.CreatePrimary(OH_AVCODEC_MIMETYPE_VIDEO_AVC)) << scenario.name;
    ASSERT_EQ(sample.ConfigurePrimary(scenario), AV_ERR_OK) << scenario.name;
    ASSERT_EQ(sample.GetPrimarySurface(), AV_ERR_OK) << scenario.name;
    ASSERT_EQ(sample.DestroyPrimary(), AV_ERR_OK) << scenario.name;
    ASSERT_EQ(sample.CreateSecondary(), scenario.expectedResult) << scenario.name;
}

/**
 * @tc.name: VideoEncoder_Preproc_Dual_Output_001
 * @tc.desc: preproc dual-output scenarios on primary surface input.
 * @tc.type: FUNC
 */
HWTEST_P(VideoEncPreprocDualOutputTest, VideoEncoder_Preproc_Dual_Output_001, TestSize.Level1)
{
    const auto &scenario = GetParam();
    if (!IsHardwareAvcAvailable()) {
        GTEST_SKIP() << "hardware AVC capability unavailable";
    }

    switch (scenario.flow) {
        case FlowType::NO_PREPROC:
        case FlowType::PRIMARY_PREPROC:
        case FlowType::MIXED_PREPROC:
            RunNoPreprocLikeScenario(scenario);
            break;
        case FlowType::INVALID_PARAMETER:
            RunInvalidParameterScenario(scenario);
            break;
        case FlowType::INVALID_LIFECYCLE:
            RunInvalidLifecycleScenario(scenario);
            break;
        case FlowType::DUPLICATE_SECONDARY:
            RunDuplicateSecondaryScenario(scenario);
            break;
        default:
            GTEST_FAIL() << "unexpected flow";
            break;
    }
}
