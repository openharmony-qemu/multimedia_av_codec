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
#include <memory>
#include <optional>
#include "fixed_ratio_retention_strategy.h"
#include "auto_fallback_retention_strategy.h"
#include "adaptive_retention_strategy.h"
#include "full_retention_strategy.h"
#include "retention_strategy_factory.h"

namespace {
constexpr int32_t DEFAULT_TOTAL_FRAMES = 100;
constexpr double DEFAULT_RETENTION_RATIO = 0.8;
constexpr double DEFAULT_TOLERANCE = 0.001;
}

using namespace OHOS::MediaAVCodec::SFD;
using namespace testing::ext;

// --- FixedRatioRetentionStrategy ---

class FixedRatioRetentionStrategyTest : public testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

/**
 * @tc.name: MakeDecision_50PercentRatio_001
 * @tc.desc: Validate that exactly 50% of frames are retained when ratio is 0.5.
 * @tc.type: FUNC
 */
HWTEST_F(FixedRatioRetentionStrategyTest, MakeDecision_50PercentRatio_001, TestSize.Level1)
{
    auto strategy = std::make_unique<FixedRatioRetentionStrategy>(0.5); // 0.5: ratio
    int retainedCount = 0;
    FrameRetentionContext ctx{};
    for (int i = 0; i < DEFAULT_TOTAL_FRAMES; ++i) {
        if (strategy->MakeRetentionDecision(ctx)) retainedCount++;
    }
    EXPECT_EQ(retainedCount, 50); // 50: expected count for 0.5 ratio
}

/**
 * @tc.name: MakeDecision_33PercentRatio_002
 * @tc.desc: Validate that approximately 33% of frames are retained when ratio is 0.33.
 * @tc.type: FUNC
 */
HWTEST_F(FixedRatioRetentionStrategyTest, MakeDecision_33PercentRatio_002, TestSize.Level1)
{
    auto strategy = std::make_unique<FixedRatioRetentionStrategy>(0.33); // 0.33: ratio
    int retainedCount = 0;
    FrameRetentionContext ctx{};
    for (int i = 0; i < DEFAULT_TOTAL_FRAMES; ++i) {
        if (strategy->MakeRetentionDecision(ctx)) {
            retainedCount++;
        }
    }
    EXPECT_NEAR(retainedCount, 33, 1); // 33: expected count
}

/**
 * @tc.name: MakeDecision_100PercentRatio_003
 * @tc.desc: Validate that all frames are retained when ratio is 1.0.
 * @tc.type: FUNC
 */
HWTEST_F(FixedRatioRetentionStrategyTest, MakeDecision_100PercentRatio_003, TestSize.Level1)
{
    auto strategy = std::make_unique<FixedRatioRetentionStrategy>(1.0); // 1.0: ratio
    int retainedCount = 0;
    FrameRetentionContext ctx{};
    for (int i = 0; i < DEFAULT_TOTAL_FRAMES; ++i) {
        if (strategy->MakeRetentionDecision(ctx)) {
            retainedCount++;
        }
    }
    EXPECT_EQ(retainedCount, DEFAULT_TOTAL_FRAMES);
}

/**
 * @tc.name: MakeDecision_1PercentRatio_004
 * @tc.desc: Validate that extremely low ratio is handled correctly.
 * @tc.type: FUNC
 */
HWTEST_F(FixedRatioRetentionStrategyTest, MakeDecision_1PercentRatio_004, TestSize.Level1)
{
    auto strategy = std::make_unique<FixedRatioRetentionStrategy>(0.01); // 0.01: ratio
    int retainedCount = 0;
    int totalFrames = 1000; // 1000: large frame count
    FrameRetentionContext ctx{};
    for (int i = 0; i < totalFrames; ++i) {
        if (strategy->MakeRetentionDecision(ctx)) {
            retainedCount++;
        }
    }
    EXPECT_NEAR(retainedCount, 11, 2); // 11: expected count with tolerance
}

/**
 * @tc.name: GetCurrentRetentionRatio_ReturnsInitializationValue_005
 * @tc.desc: Ensure GetRatio returns the ratio provided at construction.
 * @tc.type: FUNC
 */
HWTEST_F(FixedRatioRetentionStrategyTest, GetCurrentRetentionRatio_ReturnsInitializationValue_005, TestSize.Level1)
{
    auto strategy = std::make_unique<FixedRatioRetentionStrategy>(0.7); // 0.7: ratio
    EXPECT_NEAR(strategy->GetRatio(), 0.7, 0.01);
}

/**
 * @tc.name: GetMode_ReturnsFixedRatio_006
 * @tc.desc: Ensure GetMode returns FIXED_RATIO.
 * @tc.type: FUNC
 */
HWTEST_F(FixedRatioRetentionStrategyTest, GetMode_ReturnsFixedRatio_006, TestSize.Level1)
{
    auto strategy = std::make_unique<FixedRatioRetentionStrategy>(0.5); // 0.5: ratio
    EXPECT_EQ(strategy->GetMode(), RetentionStrategyType::FIXED_RATIO);
}

/**
 * @tc.name: Reset_RestartsInternalState_007
 * @tc.desc: Ensure Reset functions properly by enforcing the first frame rule.
 * @tc.type: FUNC
 */
HWTEST_F(FixedRatioRetentionStrategyTest, Reset_RestartsInternalState_007, TestSize.Level1)
{
    auto strategy = std::make_unique<FixedRatioRetentionStrategy>(0.1); // 0.1: ratio
    FrameRetentionContext ctx{};
    strategy->MakeRetentionDecision(ctx);
    strategy->Reset();
    EXPECT_TRUE(strategy->MakeRetentionDecision(ctx));
}

/**
 * @tc.name: MakeDecision_WithPreDroppedFrames_008
 * @tc.desc: Ensure pre-dropped frames factor into the calculation.
 * @tc.type: FUNC
 */
HWTEST_F(FixedRatioRetentionStrategyTest, MakeDecision_WithPreDroppedFrames_008, TestSize.Level1)
{
    auto strategy = std::make_unique<FixedRatioRetentionStrategy>(0.5); // 0.5: ratio
    FrameRetentionContext ctx{};
    ctx.preDroppedCount = 2; // 2: pre-dropped count
    EXPECT_TRUE(strategy->MakeRetentionDecision(ctx));
}

// --- AutoFallbackRetentionStrategy ---

class AutoFallbackRetentionStrategyTest : public testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

/**
 * @tc.name: Init_BasicState_001
 * @tc.desc: Validate initial ratio is set to 0.8 default.
 * @tc.type: FUNC
 */
HWTEST_F(AutoFallbackRetentionStrategyTest, Init_BasicState_001, TestSize.Level1)
{
    auto strategy = std::make_unique<AutoFallbackRetentionStrategy>();
    EXPECT_EQ(strategy->GetMode(), RetentionStrategyType::AUTO_RATIO);
    // Verify it indeed degenerates to a 0.8 fixed ratio
    EXPECT_NEAR(strategy->GetRatio(), DEFAULT_RETENTION_RATIO, DEFAULT_TOLERANCE);
}

/**
 * @tc.name: MakeDecision_DelegatesToFixedRatio_002
 * @tc.desc: Verify it drops exactly 20% of frames (retains 8 out of 10) acting as a delegator.
 * @tc.type: FUNC
 */
HWTEST_F(AutoFallbackRetentionStrategyTest, MakeDecision_DelegatesToFixedRatio_002, TestSize.Level1)
{
    auto strategy = std::make_unique<AutoFallbackRetentionStrategy>();
    int retainedCount = 0;
    FrameRetentionContext ctx{};
    // Submit 10 frames
    for (int i = 0; i < 10; ++i) { // 10: small frame count
        if (strategy->MakeRetentionDecision(ctx)) {
            retainedCount++;
        }
    }
    // Since the ratio is 0.8, it should strictly retain 8 frames
    EXPECT_EQ(retainedCount, 8); // 8: expected count
}

/**
 * @tc.name: LastReason_Mapping_003
 * @tc.desc: Validate internal FixedRatio reasons are successfully mapped to AUTO_FALLBACK.
 * @tc.type: FUNC
 */
HWTEST_F(AutoFallbackRetentionStrategyTest, LastReason_Mapping_003, TestSize.Level1)
{
    auto strategy = std::make_unique<AutoFallbackRetentionStrategy>();
    FrameRetentionContext ctx{};
    strategy->MakeRetentionDecision(ctx);
    EXPECT_EQ(strategy->GetLastDecisionReason(), SfdDecisionReason::POST_RETAIN_AUTO_FALLBACK);
}

/**
 * @tc.name: UpdatePlaybackSpeed_DelegateIgnoresSpeed_004
 * @tc.desc: Validate that since it delegates to FixedRatio, speed changes do not crash but ignore updates.
 * @tc.type: FUNC
 */
HWTEST_F(AutoFallbackRetentionStrategyTest, UpdatePlaybackSpeed_DelegateIgnoresSpeed_004, TestSize.Level1)
{
    auto strategy = std::make_unique<AutoFallbackRetentionStrategy>();
    EXPECT_NEAR(strategy->GetRatio(), DEFAULT_RETENTION_RATIO, DEFAULT_TOLERANCE);

    strategy->UpdatePlaybackSpeed(2.0); // 2.0: high speed
    // Playback speed changes have no effect on fixed-ratio delegation mode; it remains 0.8
    EXPECT_NEAR(strategy->GetRatio(), DEFAULT_RETENTION_RATIO, DEFAULT_TOLERANCE);
}


// --- AdaptiveRetentionStrategy ---

class AdaptiveRetentionStrategyTest : public testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

/**
 * @tc.name: GetMode_ReturnsAdaptive_001
 * @tc.desc: Check the returned strategy type mode.
 * @tc.type: FUNC
 */
HWTEST_F(AdaptiveRetentionStrategyTest, GetMode_ReturnsAdaptive_001, TestSize.Level1)
{
    auto strategy = std::make_unique<AdaptiveRetentionStrategy>();
    EXPECT_EQ(strategy->GetMode(), RetentionStrategyType::ADAPTIVE);
}

/**
 * @tc.name: MakeDecision_BasicRetains_002
 * @tc.desc: Validate the default baseline retains frames.
 * @tc.type: FUNC
 */
HWTEST_F(AdaptiveRetentionStrategyTest, MakeDecision_BasicRetains_002, TestSize.Level1)
{
    auto strategy = std::make_unique<AdaptiveRetentionStrategy>();
    FrameRetentionContext ctx{};
    EXPECT_TRUE(strategy->MakeRetentionDecision(ctx));
}

/**
 * @tc.name: UpdatePlaybackSpeed_Behavior_003
 * @tc.desc: Verify that playback speed changes correctly trigger fast-path retention or decision tree logic.
 * @tc.type: FUNC
 */
HWTEST_F(AdaptiveRetentionStrategyTest, UpdatePlaybackSpeed_Behavior_003, TestSize.Level1)
{
    auto strategy = std::make_unique<AdaptiveRetentionStrategy>();
    FrameRetentionContext ctx{};

    // 1. At low speed (<= 1.2), force full retention (Fast-path)
    strategy->UpdatePlaybackSpeed(1.0);
    EXPECT_TRUE(strategy->MakeRetentionDecision(ctx));
    EXPECT_EQ(strategy->GetLastDecisionReason(), SfdDecisionReason::POST_RETAIN_ADAPTIVE_SPEED);

    // 2. At high speed (> 1.2), enter model inference (fallback to UNSPECIFIED since ctx lacks MV data)
    strategy->UpdatePlaybackSpeed(2.0); // 2.0: high speed
    strategy->MakeRetentionDecision(ctx);
    EXPECT_NE(strategy->GetLastDecisionReason(), SfdDecisionReason::POST_RETAIN_ADAPTIVE_SPEED);
}

/**
 * @tc.name: Reset_NoCrash_004
 * @tc.desc: Ensure Reset works.
 * @tc.type: FUNC
 */
HWTEST_F(AdaptiveRetentionStrategyTest, Reset_NoCrash_004, TestSize.Level1)
{
    auto strategy = std::make_unique<AdaptiveRetentionStrategy>();
    strategy->Reset();
    EXPECT_NE(strategy, nullptr);
}

// --- FullRetentionStrategy ---

class FullRetentionStrategyTest : public testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

/**
 * @tc.name: GetMode_ReturnsFull_001
 * @tc.desc: Verify the mode function returns FULL type.
 * @tc.type: FUNC
 */
HWTEST_F(FullRetentionStrategyTest, GetMode_ReturnsFull_001, TestSize.Level1)
{
    auto strategy = std::make_unique<FullRetentionStrategy>();
    EXPECT_EQ(strategy->GetMode(), RetentionStrategyType::FULL);
}

/**
 * @tc.name: MakeDecision_AlwaysRetains_002
 * @tc.desc: Verify the strategy always retains frames.
 * @tc.type: FUNC
 */
HWTEST_F(FullRetentionStrategyTest, MakeDecision_AlwaysRetains_002, TestSize.Level1)
{
    auto strategy = std::make_unique<FullRetentionStrategy>();
    FrameRetentionContext ctx{};
    for (int i = 0; i < 50; i++) { // 50: iteration count
        EXPECT_TRUE(strategy->MakeRetentionDecision(ctx));
    }
}

// --- RetentionStrategyFactory ---

class RetentionStrategyFactoryTest : public testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

/**
 * @tc.name: CreateStrategy_Full_001
 * @tc.desc: Factory correctly instantiates FULL strategy.
 * @tc.type: FUNC
 */
HWTEST_F(RetentionStrategyFactoryTest, CreateStrategy_Full_001, TestSize.Level1)
{
    auto strategy = RetentionStrategyFactory::CreateStrategy(RetentionStrategyType::FULL, std::nullopt);
    ASSERT_NE(strategy, nullptr);
    EXPECT_EQ(strategy->GetMode(), RetentionStrategyType::FULL);
}

/**
 * @tc.name: CreateStrategy_Adaptive_002
 * @tc.desc: Factory correctly instantiates ADAPTIVE strategy.
 * @tc.type: FUNC
 */
HWTEST_F(RetentionStrategyFactoryTest, CreateStrategy_Adaptive_002, TestSize.Level1)
{
    auto strategy = RetentionStrategyFactory::CreateStrategy(RetentionStrategyType::ADAPTIVE, std::nullopt);
    ASSERT_NE(strategy, nullptr);
    EXPECT_EQ(strategy->GetMode(), RetentionStrategyType::ADAPTIVE);
}

/**
 * @tc.name: CreateStrategy_AutoRatio_003
 * @tc.desc: Factory correctly instantiates AUTO_RATIO strategy.
 * @tc.type: FUNC
 */
HWTEST_F(RetentionStrategyFactoryTest, CreateStrategy_AutoRatio_003, TestSize.Level1)
{
    auto strategy = RetentionStrategyFactory::CreateStrategy(RetentionStrategyType::AUTO_RATIO, std::nullopt);
    ASSERT_NE(strategy, nullptr);
    EXPECT_EQ(strategy->GetMode(), RetentionStrategyType::AUTO_RATIO);
}

/**
 * @tc.name: CreateStrategy_FixedRatioWithValidRatio_004
 * @tc.desc: Factory correctly instantiates FIXED_RATIO strategy with a provided ratio.
 * @tc.type: FUNC
 */
HWTEST_F(RetentionStrategyFactoryTest, CreateStrategy_FixedRatioWithValidRatio_004, TestSize.Level1)
{
    auto strategy = RetentionStrategyFactory::CreateStrategy(RetentionStrategyType::FIXED_RATIO, 0.5); // 0.5: ratio
    ASSERT_NE(strategy, nullptr);
    EXPECT_EQ(strategy->GetMode(), RetentionStrategyType::FIXED_RATIO);
}

/**
 * @tc.name: CreateStrategy_FixedRatioMissingRatio_005
 * @tc.desc: Factory gracefully degenerates into AUTO_RATIO when FIXED_RATIO is requested without ratio details.
 * @tc.type: FUNC
 */
HWTEST_F(RetentionStrategyFactoryTest, CreateStrategy_FixedRatioMissingRatio_005, TestSize.Level1)
{
    auto strategy = RetentionStrategyFactory::CreateStrategy(RetentionStrategyType::FIXED_RATIO, std::nullopt);
    ASSERT_NE(strategy, nullptr);
    EXPECT_EQ(strategy->GetMode(), RetentionStrategyType::AUTO_RATIO);
}

/**
 * @tc.name: CreateStrategy_InvalidType_006
 * @tc.desc: Factory returns nullptr when invalid enum provided.
 * @tc.type: FUNC
 */
HWTEST_F(RetentionStrategyFactoryTest, CreateStrategy_InvalidType_006, TestSize.Level1)
{
    auto strategy = RetentionStrategyFactory::CreateStrategy(RetentionStrategyType::INVALID, std::nullopt);
    EXPECT_EQ(strategy, nullptr);
}
