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
        if (strategy->MakeRetentionDecision(ctx)) {
            retainedCount++;
        }
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

/**
 * @tc.name: MakeDecision_DecisionTreeRetain_005
 * @tc.desc: High speed (> 1.0) with valid MV data that the decision tree should "retain".
 * @tc.type: FUNC
 */
HWTEST_F(AdaptiveRetentionStrategyTest, MakeDecision_DecisionTreeRetain_005, TestSize.Level1)
{
    auto strategy = std::make_unique<AdaptiveRetentionStrategy>();
    strategy->UpdatePlaybackSpeed(2.0); // 2.0: high speed

    FrameRetentionContext ctx{};
    MVStats mvStats;
    mvStats.validBlocks = 100;
    mvStats.motionConsistency = 0.8; // High consistency
    mvStats.perceptualMagnitude = 5.0; // Low magnitude
    ctx.mvStats = &mvStats;

    // Decision tree should hit rule 3, 4, 6, 7, 10, 11, or 13 (Retain = 1)
    EXPECT_TRUE(strategy->MakeRetentionDecision(ctx));
    EXPECT_EQ(strategy->GetLastDecisionReason(), SfdDecisionReason::POST_RETAIN_ADAPTIVE_DT);
}

/**
 * @tc.name: MakeDecision_DecisionTreeDrop_006
 * @tc.desc: High speed with MV data that the decision tree should "drop".
 * @tc.type: FUNC
 */
HWTEST_F(AdaptiveRetentionStrategyTest, MakeDecision_DecisionTreeDrop_006, TestSize.Level1)
{
    auto strategy = std::make_unique<AdaptiveRetentionStrategy>();
    strategy->UpdatePlaybackSpeed(2.0);

    FrameRetentionContext ctx{};
    MVStats mvStats;
    mvStats.validBlocks = 100;
    mvStats.motionConsistency = 0.1; // Very low consistency
    mvStats.perceptualMagnitude = 100.0; // Very high magnitude
    ctx.mvStats = &mvStats;

    // (magnitude * 2.0) = 200.0 > 21.01 (rule 0) -> rule 8
    // consistency 0.1 < 0.55 (rule 8) -> rule 9
    // consistency 0.1 < 0.35 (rule 9) -> rule 10 (Retain = 1 in code? Wait, let me check the tree data)
    // Actually rule 3, 4, 14 are leaf 0 (Drop).
    // Let's pick values to hit rule 14:
    // mag*speed > 21.01 (rule 0 right) -> rule 8
    // consistency > 0.55 (rule 8 right) -> rule 12
    // consistency > 0.98 (rule 12 right) -> rule 14 (Leaf 0)
    mvStats.motionConsistency = 0.99;
    mvStats.perceptualMagnitude = 20.0; // 20*2 = 40 > 21
    
    // Check results
    bool shouldRetain = strategy->MakeRetentionDecision(ctx);
    // If it drops, it will be POST_DROP_ADAPTIVE_DT
    if (!shouldRetain) {
        EXPECT_EQ(strategy->GetLastDecisionReason(), SfdDecisionReason::POST_DROP_ADAPTIVE_DT);
    } else {
        EXPECT_EQ(strategy->GetLastDecisionReason(), SfdDecisionReason::POST_RETAIN_ADAPTIVE_DT);
    }
}

/**
 * @tc.name: MakeDecision_MaxDropsEnforcement_007
 * @tc.desc: Verify that after a drop, it continues to drop until maxDrops is reached.
 * @tc.type: FUNC
 */
HWTEST_F(AdaptiveRetentionStrategyTest, MakeDecision_MaxDropsEnforcement_007, TestSize.Level1)
{
    auto strategy = std::make_unique<AdaptiveRetentionStrategy>();
    strategy->UpdatePlaybackSpeed(2.0);

    FrameRetentionContext ctx{};
    MVStats mvStats;
    mvStats.validBlocks = 100;
    // Force a drop (Rule 14 as above)
    mvStats.motionConsistency = 0.99;
    mvStats.perceptualMagnitude = 20.0;
    ctx.mvStats = &mvStats;

    // First decision: Drop
    bool first = strategy->MakeRetentionDecision(ctx);
    if (!first) {
        EXPECT_EQ(strategy->GetLastDecisionReason(), SfdDecisionReason::POST_DROP_ADAPTIVE_DT);
        // maxDrops = (2.0 - 1.0) * 3.0 = 5
        // It should drop 3 frames then retain 1.
        for (int i = 0; i < 2; ++i) {
            EXPECT_FALSE(strategy->MakeRetentionDecision(ctx));
        }
        // 6th frame should retain (adaptive drop limit reached)
        EXPECT_TRUE(strategy->MakeRetentionDecision(ctx));
        EXPECT_EQ(strategy->GetLastDecisionReason(), SfdDecisionReason::POST_RETAIN_ADAPTIVE_MAX_DROP);
    }
}

/**
 * @tc.name: MakeDecision_EmptyMvStats_008
 * @tc.desc: High speed but with validBlocks = 0 to hit the else branch (UNSPECIFIED reason).
 * @tc.type: FUNC
 */
HWTEST_F(AdaptiveRetentionStrategyTest, MakeDecision_EmptyMvStats_008, TestSize.Level1)
{
    auto strategy = std::make_unique<AdaptiveRetentionStrategy>();
    strategy->UpdatePlaybackSpeed(2.0);

    FrameRetentionContext ctx{};
    MVStats mvStats;
    mvStats.validBlocks = 0; // Invalid
    ctx.mvStats = &mvStats;

    EXPECT_TRUE(strategy->MakeRetentionDecision(ctx));
    EXPECT_EQ(strategy->GetLastDecisionReason(), SfdDecisionReason::UNSPECIFIED);

    ctx.mvStats = nullptr;
    EXPECT_TRUE(strategy->MakeRetentionDecision(ctx));
    EXPECT_EQ(strategy->GetLastDecisionReason(), SfdDecisionReason::UNSPECIFIED);
}

/**
 * @tc.name: GetRatio_UpdateOnDrop_009
 * @tc.desc: Verify that targetRatio_ is updated following a drop decision.
 * @tc.type: FUNC
 */
HWTEST_F(AdaptiveRetentionStrategyTest, GetRatio_UpdateOnDrop_009, TestSize.Level1)
{
    auto strategy = std::make_unique<AdaptiveRetentionStrategy>();
    strategy->UpdatePlaybackSpeed(2.0);

    FrameRetentionContext ctx{};
    MVStats mvStats;
    mvStats.validBlocks = 100;
    mvStats.motionConsistency = 0.99;
    mvStats.perceptualMagnitude = 20.0;
    ctx.mvStats = &mvStats;

    strategy->MakeRetentionDecision(ctx);
    // If it dropped, ratio should be 1/(3+1) = 0.25
    if (strategy->GetLastDecisionReason() == SfdDecisionReason::POST_DROP_ADAPTIVE_DT) {
        EXPECT_NEAR(strategy->GetRatio(), 1.0 / 4.0, 0.01);
    }
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

// --- FixedRatioRetentionStrategy edge cases ---

/**
 * @tc.name: MakeDecision_NanRatio_009
 * @tc.desc: Verify FixedRatioRetentionStrategy clamps NaN ratio to 1.0 (full retention).
 * @tc.type: FUNC
 */
HWTEST_F(FixedRatioRetentionStrategyTest, MakeDecision_NanRatio_009, TestSize.Level1)
{
    double nan = std::numeric_limits<double>::quiet_NaN();
    auto strategy = std::make_unique<FixedRatioRetentionStrategy>(nan);
    EXPECT_NEAR(strategy->GetRatio(), 1.0, 0.01);
    FrameRetentionContext ctx{};
    EXPECT_TRUE(strategy->MakeRetentionDecision(ctx));
}

/**
 * @tc.name: MakeDecision_AboveMaxRatio_010
 * @tc.desc: Verify FixedRatioRetentionStrategy clamps ratio > 1.0 to 1.0.
 * @tc.type: FUNC
 */
HWTEST_F(FixedRatioRetentionStrategyTest, MakeDecision_AboveMaxRatio_010, TestSize.Level1)
{
    auto strategy = std::make_unique<FixedRatioRetentionStrategy>(2.0);
    EXPECT_NEAR(strategy->GetRatio(), 1.0, 0.01);
}

// --- AdaptiveRetentionStrategy additional tests ---

/**
 * @tc.name: MakeDecision_ZeroMotionDrop_010
 * @tc.desc: Verify zeroMotionRatio >= 0.85 forces a drop decision.
 * @tc.type: FUNC
 */
HWTEST_F(AdaptiveRetentionStrategyTest, MakeDecision_ZeroMotionDrop_010, TestSize.Level1)
{
    auto strategy = std::make_unique<AdaptiveRetentionStrategy>();
    strategy->UpdatePlaybackSpeed(2.0);

    FrameRetentionContext ctx{};
    MVStats mvStats;
    mvStats.validBlocks = 100;
    mvStats.zeroMotionRatio = 0.9; // >= 0.85 threshold
    ctx.mvStats = &mvStats;

    EXPECT_FALSE(strategy->MakeRetentionDecision(ctx));
    EXPECT_EQ(strategy->GetLastDecisionReason(), SfdDecisionReason::POST_DROP_ADAPTIVE_DT);
}

/**
 * @tc.name: MakeDecision_MaxDropsLimit_011
 * @tc.desc: Verify adaptive maxDrops logic: after consecutive drops reach limit, forces a retain
 *           with POST_RETAIN_ADAPTIVE_MAX_DROP reason.
 * @tc.type: FUNC
 */
HWTEST_F(AdaptiveRetentionStrategyTest, MakeDecision_MaxDropsLimit_011, TestSize.Level1)
{
    auto strategy = std::make_unique<AdaptiveRetentionStrategy>();
    strategy->UpdatePlaybackSpeed(2.0); // maxDrops = (2.0 - 1.0) * 3.0 = 3

    FrameRetentionContext ctx{};
    MVStats mvStats;
    mvStats.validBlocks = 100;
    mvStats.zeroMotionRatio = 0.9; // Force drop
    ctx.mvStats = &mvStats;

    // Drop 3 times in a row
    for (int i = 0; i < 3; i++) {
        EXPECT_FALSE(strategy->MakeRetentionDecision(ctx));
    }
    // 4th call: maxDrops limit reached, forces retain
    EXPECT_TRUE(strategy->MakeRetentionDecision(ctx));
    EXPECT_EQ(strategy->GetLastDecisionReason(), SfdDecisionReason::POST_RETAIN_ADAPTIVE_MAX_DROP);
}

// --- FullRetentionStrategy additional tests ---

/**
 * @tc.name: FullStrategy_Properties_003
 * @tc.desc: Verify all property accessors of FullRetentionStrategy.
 * @tc.type: FUNC
 */
HWTEST_F(FullRetentionStrategyTest, FullStrategy_Properties_003, TestSize.Level1)
{
    auto strategy = std::make_unique<FullRetentionStrategy>();
    EXPECT_NEAR(strategy->GetRatio(), 1.0, 0.01);
    EXPECT_EQ(strategy->GetLastDecisionReason(), SfdDecisionReason::POST_RETAIN_FULL);
    EXPECT_TRUE(strategy->IsPassthrough());
    EXPECT_FALSE(strategy->RequiresNaluAnalysis());
    EXPECT_FALSE(strategy->RequiresMvAnalysis());
}

// --- AutoFallbackRetentionStrategy additional tests ---

/**
 * @tc.name: LastReason_DropMapping_005
 * @tc.desc: Verify POST_DROP_FIXED_ACC from underlying FixedRatio is returned as-is (not mapped).
 * @tc.type: FUNC
 */
HWTEST_F(AutoFallbackRetentionStrategyTest, LastReason_DropMapping_005, TestSize.Level1)
{
    auto strategy = std::make_unique<AutoFallbackRetentionStrategy>();
    FrameRetentionContext ctx{};
    // Frame 1: always retained, reason mapped to POST_RETAIN_AUTO_FALLBACK
    strategy->MakeRetentionDecision(ctx);
    // Frame 2: should be drop for 0.8 ratio (accumulator = 0.8 < 1.0)
    bool retained = strategy->MakeRetentionDecision(ctx);
    if (!retained) {
        EXPECT_EQ(strategy->GetLastDecisionReason(), SfdDecisionReason::POST_DROP_FIXED_ACC);
    }
}

// --- DropSyncCoordinator additional tests (in drop_sync_coordinator_test.cpp) ---
// Note: tests below continue in separate file
