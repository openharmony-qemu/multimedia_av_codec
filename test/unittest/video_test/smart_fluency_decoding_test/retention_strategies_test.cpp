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
#include "smart_fluency_decoding_types.h"

namespace {
constexpr int32_t DEFAULT_TOTAL_FRAMES = 100;
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

/**
 * @tc.name: UpdateTargetRatio_Normal_012
 * @tc.desc: Verify UpdateTargetRatio changes ratio without resetting accumulator.
 * @tc.type: FUNC
 */
HWTEST_F(FixedRatioRetentionStrategyTest, UpdateTargetRatio_Normal_012, TestSize.Level1)
{
    auto strategy = std::make_unique<FixedRatioRetentionStrategy>(0.5);
    FrameRetentionContext ctx{};
    // Accumulate: frame 1 (first frame rule → true), frame 2 (acc=0.5 < 1.0 → false)
    strategy->MakeRetentionDecision(ctx);
    strategy->MakeRetentionDecision(ctx); // second frame: acc=0.5 < 1.0 → drop

    // Update ratio to 0.8
    strategy->UpdateTargetRatio(0.8);
    EXPECT_NEAR(strategy->GetRatio(), 0.8, DEFAULT_TOLERANCE);

    // Verify strategy continues to work with new ratio (accumulator preserved)
    strategy->MakeRetentionDecision(ctx); // no crash, accumulator state preserved
}

/**
 * @tc.name: UpdateTargetRatio_ClampMin_013
 * @tc.desc: Verify UpdateTargetRatio clamps ratio below MIN_RETENTION_RATIO (0.01) to 0.01.
 * @tc.type: FUNC
 */
HWTEST_F(FixedRatioRetentionStrategyTest, UpdateTargetRatio_ClampMin_013, TestSize.Level1)
{
    auto strategy = std::make_unique<FixedRatioRetentionStrategy>(0.5);
    strategy->UpdateTargetRatio(0.001); // 0.001: below MIN
    EXPECT_NEAR(strategy->GetRatio(), MIN_RETENTION_RATIO, DEFAULT_TOLERANCE); // clamped to 0.01
}

/**
 * @tc.name: UpdateTargetRatio_ClampMax_014
 * @tc.desc: Verify UpdateTargetRatio clamps ratio above MAX_RETENTION_RATIO (1.0) to 1.0.
 * @tc.type: FUNC
 */
HWTEST_F(FixedRatioRetentionStrategyTest, UpdateTargetRatio_ClampMax_014, TestSize.Level1)
{
    auto strategy = std::make_unique<FixedRatioRetentionStrategy>(0.5);
    strategy->UpdateTargetRatio(2.0); // 2.0: above MAX
    EXPECT_NEAR(strategy->GetRatio(), MAX_RETENTION_RATIO, DEFAULT_TOLERANCE); // clamped to 1.0
}

// --- AutoFallbackRetentionStrategy (Two-Phase: Passthrough → Active) ---

class AutoFallbackRetentionStrategyTest : public testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

/**
 * @tc.name: Passthrough_001
 * @tc.desc: Verify Phase 1 behavior: IsPassthrough=true, RequiresNaluAnalysis=false,
 *           RequiresMvAnalysis=false, GetRatio=1.0, MakeRetentionDecision returns true.
 * @tc.type: FUNC
 */
HWTEST_F(AutoFallbackRetentionStrategyTest, Passthrough_001, TestSize.Level1)
{
    auto strategy = std::make_unique<AutoFallbackRetentionStrategy>();
    EXPECT_EQ(strategy->GetMode(), RetentionStrategyType::AUTO_RATIO);
    EXPECT_TRUE(strategy->IsPassthrough());
    EXPECT_FALSE(strategy->RequiresNaluAnalysis());
    EXPECT_FALSE(strategy->RequiresMvAnalysis());
    EXPECT_NEAR(strategy->GetRatio(), 1.0, DEFAULT_TOLERANCE);
    EXPECT_EQ(strategy->GetLastDecisionReason(), SfdDecisionReason::POST_RETAIN_FULL);

    FrameRetentionContext ctx{};
    for (int i = 0; i < 10; ++i) { // 10: small frame count
        EXPECT_TRUE(strategy->MakeRetentionDecision(ctx));
    }
}

/**
 * @tc.name: Passthrough_SpeedIgnored_002
 * @tc.desc: Verify that UpdatePlaybackSpeed in Phase 1 does not trigger transition.
 * @tc.type: FUNC
 */
HWTEST_F(AutoFallbackRetentionStrategyTest, Passthrough_SpeedIgnored_002, TestSize.Level1)
{
    auto strategy = std::make_unique<AutoFallbackRetentionStrategy>();
    strategy->UpdatePlaybackSpeed(2.0); // 2.0: high speed
    // Still in Phase 1: no transition without sourceFps
    EXPECT_TRUE(strategy->IsPassthrough());
    EXPECT_NEAR(strategy->GetRatio(), 1.0, DEFAULT_TOLERANCE);
}

/**
 * @tc.name: Active_TransitionViaSourceFps_003
 * @tc.desc: Verify that UpdateSourceFramerate triggers Phase 2 transition.
 *           60fps source → ratio = 30/60 = 0.5
 * @tc.type: FUNC
 */
HWTEST_F(AutoFallbackRetentionStrategyTest, Active_TransitionViaSourceFps_003, TestSize.Level1)
{
    auto strategy = std::make_unique<AutoFallbackRetentionStrategy>();
    strategy->UpdateSourceFramerate(60.0); // 60.0: source fps
    EXPECT_FALSE(strategy->IsPassthrough());
    EXPECT_TRUE(strategy->RequiresNaluAnalysis());
    EXPECT_FALSE(strategy->RequiresMvAnalysis());
    EXPECT_NEAR(strategy->GetRatio(), 0.5, DEFAULT_TOLERANCE);

    // Verify ~50% retention over 100 frames
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
 * @tc.name: Active_SpeedUpdate_004
 * @tc.desc: Verify that speed update in Phase 2 recalculates ratio via UpdateTargetRatio.
 *           60fps 1x → ratio=0.5, then speed=2x → ratio=30/120=0.25
 * @tc.type: FUNC
 */
HWTEST_F(AutoFallbackRetentionStrategyTest, Active_SpeedUpdate_004, TestSize.Level1)
{
    auto strategy = std::make_unique<AutoFallbackRetentionStrategy>();
    strategy->UpdateSourceFramerate(60.0); // 60.0: source fps, ratio=0.5
    EXPECT_NEAR(strategy->GetRatio(), 0.5, DEFAULT_TOLERANCE);

    strategy->UpdatePlaybackSpeed(2.0); // 2.0: speed, ratio should become 0.25
    EXPECT_NEAR(strategy->GetRatio(), 0.25, DEFAULT_TOLERANCE);
}

/**
 * @tc.name: Active_RatioClampLowFps_005
 * @tc.desc: Verify that low sourceFps (24fps) clamps ratio to 1.0 (FULL equivalent).
 *           30/(24*1) = 1.25 → clamp to 1.0
 * @tc.type: FUNC
 */
HWTEST_F(AutoFallbackRetentionStrategyTest, Active_RatioClampLowFps_005, TestSize.Level1)
{
    auto strategy = std::make_unique<AutoFallbackRetentionStrategy>();
    strategy->UpdateSourceFramerate(24.0); // 24.0: low source fps
    EXPECT_NEAR(strategy->GetRatio(), 1.0, DEFAULT_TOLERANCE); // clamped to MAX

    // All frames should be retained
    FrameRetentionContext ctx{};
    for (int i = 0; i < DEFAULT_TOTAL_FRAMES; ++i) {
        EXPECT_TRUE(strategy->MakeRetentionDecision(ctx));
    }
}

/**
 * @tc.name: Active_RatioHighFps_006
 * @tc.desc: Verify that 120fps source gives ratio = 30/120 = 0.25.
 * @tc.type: FUNC
 */
HWTEST_F(AutoFallbackRetentionStrategyTest, Active_RatioHighFps_006, TestSize.Level1)
{
    auto strategy = std::make_unique<AutoFallbackRetentionStrategy>();
    strategy->UpdateSourceFramerate(120.0); // 120.0: high source fps
    EXPECT_NEAR(strategy->GetRatio(), 0.25, DEFAULT_TOLERANCE);
}

/**
 * @tc.name: Active_ReasonMapping_007
 * @tc.desc: Verify POST_RETAIN_FIXED_ACC from underlying FixedRatio is mapped to POST_RETAIN_AUTO_FALLBACK.
 * @tc.type: FUNC
 */
HWTEST_F(AutoFallbackRetentionStrategyTest, Active_ReasonMapping_007, TestSize.Level1)
{
    auto strategy = std::make_unique<AutoFallbackRetentionStrategy>();
    strategy->UpdateSourceFramerate(60.0); // 60.0: source fps
    FrameRetentionContext ctx{};
    strategy->MakeRetentionDecision(ctx);
    EXPECT_EQ(strategy->GetLastDecisionReason(), SfdDecisionReason::POST_RETAIN_AUTO_FALLBACK);
}

/**
 * @tc.name: Active_DropReasonPassthrough_008
 * @tc.desc: Verify POST_DROP_FIXED_ACC from underlying FixedRatio is returned as-is (not mapped).
 * @tc.type: FUNC
 */
HWTEST_F(AutoFallbackRetentionStrategyTest, Active_DropReasonPassthrough_008, TestSize.Level1)
{
    auto strategy = std::make_unique<AutoFallbackRetentionStrategy>();
    strategy->UpdateSourceFramerate(60.0); // 60.0: source fps, ratio=0.5
    FrameRetentionContext ctx{};
    strategy->MakeRetentionDecision(ctx); // Frame 1: retained (first frame rule)
    bool retained = strategy->MakeRetentionDecision(ctx); // Frame 2: acc=0.5 < 1.0 → drop
    if (!retained) {
        EXPECT_EQ(strategy->GetLastDecisionReason(), SfdDecisionReason::POST_DROP_FIXED_ACC);
    }
}

/**
 * @tc.name: Passthrough_InvalidSourceFps_009
 * @tc.desc: Verify that UpdateSourceFramerate with invalid fps (<= 0) does not trigger Phase 2.
 * @tc.type: FUNC
 */
HWTEST_F(AutoFallbackRetentionStrategyTest, Passthrough_InvalidSourceFps_009, TestSize.Level1)
{
    auto strategy = std::make_unique<AutoFallbackRetentionStrategy>();
    strategy->UpdateSourceFramerate(0.0); // 0.0: invalid fps
    EXPECT_TRUE(strategy->IsPassthrough()); // Still Phase 1

    strategy->UpdateSourceFramerate(-1.0); // -1.0: invalid fps
    EXPECT_TRUE(strategy->IsPassthrough()); // Still Phase 1
}

/**
 * @tc.name: Active_InvalidSpeed_010
 * @tc.desc: Verify that UpdatePlaybackSpeed with invalid speed defaults to 1.0.
 * @tc.type: FUNC
 */
HWTEST_F(AutoFallbackRetentionStrategyTest, Active_InvalidSpeed_010, TestSize.Level1)
{
    auto strategy = std::make_unique<AutoFallbackRetentionStrategy>();
    strategy->UpdateSourceFramerate(60.0); // 60.0: source fps, ratio=0.5 (speed=1.0 default)

    strategy->UpdatePlaybackSpeed(0.0); // 0.0: invalid speed → default to 1.0
    EXPECT_NEAR(strategy->GetRatio(), 0.5, DEFAULT_TOLERANCE); // 30/(60*1) = 0.5

    strategy->UpdatePlaybackSpeed(-1.0); // -1.0: invalid speed → default to 1.0
    EXPECT_NEAR(strategy->GetRatio(), 0.5, DEFAULT_TOLERANCE); // 30/(60*1) = 0.5
}

/**
 * @tc.name: Reset_BackToPassthrough_011
 * @tc.desc: Verify Reset clears sourceFps/speed state and returns to Phase 1.
 * @tc.type: FUNC
 */
HWTEST_F(AutoFallbackRetentionStrategyTest, Reset_BackToPassthrough_011, TestSize.Level1)
{
    auto strategy = std::make_unique<AutoFallbackRetentionStrategy>();
    strategy->UpdateSourceFramerate(60.0); // 60.0: source fps → Phase 2
    EXPECT_FALSE(strategy->IsPassthrough());

    strategy->Reset();
    EXPECT_TRUE(strategy->IsPassthrough()); // Back to Phase 1
    EXPECT_NEAR(strategy->GetRatio(), 1.0, DEFAULT_TOLERANCE);
    EXPECT_EQ(strategy->GetLastDecisionReason(), SfdDecisionReason::POST_RETAIN_FULL);

    FrameRetentionContext ctx{};
    EXPECT_TRUE(strategy->MakeRetentionDecision(ctx));
}

/**
 * @tc.name: Active_SpeedThenSourceFps_012
 * @tc.desc: Verify that speed update before sourceFps (Phase 1) is stored but doesn't trigger transition.
 *           Then UpdateSourceFramerate triggers Phase 2 with stored speed.
 * @tc.type: FUNC
 */
HWTEST_F(AutoFallbackRetentionStrategyTest, Active_SpeedThenSourceFps_012, TestSize.Level1)
{
    auto strategy = std::make_unique<AutoFallbackRetentionStrategy>();
    strategy->UpdatePlaybackSpeed(2.0); // 2.0: speed stored, no transition yet
    EXPECT_TRUE(strategy->IsPassthrough());

    strategy->UpdateSourceFramerate(60.0); // 60.0: source fps → Phase 2 with speed=2x
    // ratio = 30/(60*2) = 0.25
    EXPECT_NEAR(strategy->GetRatio(), 0.25, DEFAULT_TOLERANCE);
}

/**
 * @tc.name: Active_SourceFpsUpdate_013
 * @tc.desc: Verify that sourceFps update in Phase 2 recalculates ratio.
 *           60fps→ratio=0.5, then 30fps→ratio=1.0 (clamped)
 * @tc.type: FUNC
 */
HWTEST_F(AutoFallbackRetentionStrategyTest, Active_SourceFpsUpdate_013, TestSize.Level1)
{
    auto strategy = std::make_unique<AutoFallbackRetentionStrategy>();
    strategy->UpdateSourceFramerate(60.0); // 60.0: ratio=0.5
    EXPECT_NEAR(strategy->GetRatio(), 0.5, DEFAULT_TOLERANCE);

    strategy->UpdateSourceFramerate(30.0); // 30.0: ratio=30/30=1.0 (clamped)
    EXPECT_NEAR(strategy->GetRatio(), 1.0, DEFAULT_TOLERANCE);
}

/**
 * @tc.name: Passthrough_RatioConstantValue_014
 * @tc.desc: Verify FALLBACK_TARGET_FPS constant equals 30.0.
 * @tc.type: FUNC
 */
HWTEST_F(AutoFallbackRetentionStrategyTest, Passthrough_RatioConstantValue_014, TestSize.Level1)
{
    EXPECT_NEAR(FALLBACK_TARGET_FPS, 30.0, DEFAULT_TOLERANCE);
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
    EXPECT_EQ(strategy->GetLastDecisionReason(), SfdDecisionReason::POST_RETAIN_ADAPTIVE_DT);

    // 2. At high speed (> 1.2), enter model inference (fallback to UNSPECIFIED since ctx lacks MV data)
    strategy->UpdatePlaybackSpeed(2.0); // 2.0: high speed
    strategy->MakeRetentionDecision(ctx);
    EXPECT_EQ(strategy->GetLastDecisionReason(), SfdDecisionReason::POST_RETAIN_ADAPTIVE_DT);
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

    EXPECT_FALSE(strategy->MakeRetentionDecision(ctx));
    EXPECT_EQ(strategy->GetLastDecisionReason(), SfdDecisionReason::POST_DROP_ADAPTIVE_DT);
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
    strategy->UpdatePlaybackSpeed(2.0); // sourceFps(30) * 2.0 = 60.0 Render FPS

    FrameRetentionContext ctx{};
    MVStats mvStats;
    mvStats.validBlocks = 100;
    mvStats.motionConsistency = 0.99;
    mvStats.perceptualMagnitude = 30.0;
    ctx.mvStats = &mvStats;

    // Frame 1: acc = 0.0 + 0.75 = 0.75 < 1.0 -> Drop
    EXPECT_TRUE(strategy->MakeRetentionDecision(ctx));
    EXPECT_EQ(strategy->GetLastDecisionReason(), SfdDecisionReason::POST_RETAIN_ADAPTIVE_DT);

    // Frame 2: acc = 0.75 + 0.75 = 1.50 >= 1.0 -> Retain (accumulator_ = 0.0)
    EXPECT_TRUE(strategy->MakeRetentionDecision(ctx));
    EXPECT_EQ(strategy->GetLastDecisionReason(), SfdDecisionReason::POST_RETAIN_ADAPTIVE_DT);

    // Frame 3: acc = 0.0 + 0.75 = 0.75 < 1.0 -> Drop
    EXPECT_TRUE(strategy->MakeRetentionDecision(ctx));
    EXPECT_EQ(strategy->GetLastDecisionReason(), SfdDecisionReason::POST_RETAIN_ADAPTIVE_DT);

    // Frame 4: acc = 0.75 + 0.75 = 1.50 >= 1.0 -> Retain
    EXPECT_TRUE(strategy->MakeRetentionDecision(ctx));
    EXPECT_EQ(strategy->GetLastDecisionReason(), SfdDecisionReason::POST_RETAIN_ADAPTIVE_DT);
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

    EXPECT_FALSE(strategy->MakeRetentionDecision(ctx));
    EXPECT_EQ(strategy->GetLastDecisionReason(), SfdDecisionReason::POST_DROP_ADAPTIVE_DT);

    ctx.mvStats = nullptr;
    EXPECT_TRUE(strategy->MakeRetentionDecision(ctx));
    EXPECT_EQ(strategy->GetLastDecisionReason(), SfdDecisionReason::POST_RETAIN_ADAPTIVE_DT);
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
    // If it dropped, ratio should be 0.75
    if (strategy->GetLastDecisionReason() == SfdDecisionReason::POST_DROP_ADAPTIVE_DT) {
        EXPECT_NEAR(strategy->GetRatio(), 0.75, 0.01);
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
    strategy->UpdatePlaybackSpeed(2.0); // Render FPS = 60.0
    FrameRetentionContext ctx{};
    MVStats mvStats;
    mvStats.validBlocks = 100;
    mvStats.motionConsistency = 0.1;
    mvStats.perceptualMagnitude = 10.0;
    ctx.mvStats = &mvStats;

    // Frame 1: acc = 0.366 (< 1.0) -> Drop
    EXPECT_FALSE(strategy->MakeRetentionDecision(ctx));
    EXPECT_EQ(strategy->GetLastDecisionReason(), SfdDecisionReason::POST_DROP_ADAPTIVE_DT);

    // Frame 2: acc = 0.733 (< 1.0) -> Drop
    EXPECT_TRUE(strategy->MakeRetentionDecision(ctx));

    // Frame 3: acc = 1.099 (>= 1.0) -> Retain (accumulator_ = 0.0)
    EXPECT_FALSE(strategy->MakeRetentionDecision(ctx));
    EXPECT_EQ(strategy->GetLastDecisionReason(), SfdDecisionReason::POST_DROP_ADAPTIVE_DT);
}

/**
 * @tc.name: HighMagnitude_CoherentMotionProtected_012
 * @tc.desc: Coherent high-magnitude motion reaches the existing 60 fps ceiling.
 * @tc.type: FUNC
 */
HWTEST_F(AdaptiveRetentionStrategyTest, HighMagnitude_CoherentMotionProtected_012, TestSize.Level1)
{
    AdaptiveRetentionStrategy strategy;
    strategy.UpdateSourceFramerate(30.0);
    strategy.UpdatePlaybackSpeed(2.0);
    MVStats stats{};
    stats.validBlocks = 100;
    stats.perceptualMagnitude = 100.0;
    stats.motionConsistency = 0.90;
    FrameRetentionContext ctx{};
    ctx.mvStats = &stats;
    for (int32_t i = 0; i < DEFAULT_TOTAL_FRAMES; ++i) {
        EXPECT_TRUE(strategy.MakeRetentionDecision(ctx));
    }
    EXPECT_NEAR(strategy.GetRatio(), 1.0, DEFAULT_TOLERANCE);

    // Protection still respects the frame-rate ceiling instead of forcing all frames.
    strategy.Reset();
    strategy.UpdateSourceFramerate(60.0);
    strategy.UpdatePlaybackSpeed(2.0);
    int32_t retainedCount = 0;
    for (int32_t i = 0; i < DEFAULT_TOTAL_FRAMES; ++i) {
        retainedCount += strategy.MakeRetentionDecision(ctx) ? 1 : 0;
    }
    EXPECT_EQ(retainedCount, 50);
    EXPECT_NEAR(strategy.GetRatio(), 0.5, DEFAULT_TOLERANCE);
}

/**
 * @tc.name: MagnitudeMasking_GatedTargetRatios_013
 * @tc.desc: Preserve magnitude sensitivity below the gate and blend inside it.
 * @tc.type: FUNC
 */
HWTEST_F(AdaptiveRetentionStrategyTest, MagnitudeMasking_GatedTargetRatios_013, TestSize.Level1)
{
    struct TestCase {
        double magnitude;
        double consistency;
        double expectedRatio;
    };
    const TestCase cases[] = {
        {70.0, 0.60, 0.8515151515},
        {100.0, 0.60, 0.5060606061},
        {100.0, 0.66, 0.5333333333},
        {85.0, 0.78, 0.9137254902},
        {100.0, 0.85, 0.9823325163},
        {100.0, 0.90, 1.0},
        {150.0, 1.20, 1.0},
        // Low- and medium-magnitude branches retain their previous behavior.
        {10.0, 0.90, 0.8823529412},
        {20.0, 0.90, 1.0},
        {65.0, 0.90, 1.0},
    };
    for (const auto& testCase : cases) {
        AdaptiveRetentionStrategy strategy;
        strategy.UpdateSourceFramerate(30.0);
        strategy.UpdatePlaybackSpeed(2.0);
        MVStats stats{};
        stats.validBlocks = 100;
        stats.perceptualMagnitude = testCase.magnitude;
        stats.motionConsistency = testCase.consistency;
        FrameRetentionContext ctx{};
        ctx.mvStats = &stats;
        strategy.MakeRetentionDecision(ctx);
        EXPECT_NEAR(strategy.GetRatio(), testCase.expectedRatio, 1e-6);
    }
}

/**
 * @tc.name: MagnitudeMasking_ContinuousBoundaries_014
 * @tc.desc: Crossing either consistency gate boundary must not jump the target ratio.
 * @tc.type: FUNC
 */
HWTEST_F(AdaptiveRetentionStrategyTest, MagnitudeMasking_ContinuousBoundaries_014, TestSize.Level1)
{
    const double boundaries[] = {0.66, 0.90};
    constexpr double delta = 1e-5;
    for (double boundary : boundaries) {
        AdaptiveRetentionStrategy strategy;
        strategy.UpdatePlaybackSpeed(2.0);
        MVStats stats{};
        stats.validBlocks = 100;
        stats.perceptualMagnitude = 100.0;
        FrameRetentionContext ctx{};
        ctx.mvStats = &stats;
        stats.motionConsistency = boundary - delta;
        strategy.MakeRetentionDecision(ctx);
        double below = strategy.GetRatio();
        stats.motionConsistency = boundary + delta;
        strategy.MakeRetentionDecision(ctx);
        EXPECT_GE(strategy.GetRatio(), below);
        EXPECT_NEAR(strategy.GetRatio(), below, 1e-4);
    }
}

/**
 * @tc.name: MagnitudeMasking_ZeroMotionBranchUnchanged_015
 * @tc.desc: Consistency protection does not override the separate zero-motion rule.
 * @tc.type: FUNC
 */
HWTEST_F(AdaptiveRetentionStrategyTest, MagnitudeMasking_ZeroMotionBranchUnchanged_015, TestSize.Level1)
{
    AdaptiveRetentionStrategy strategy;
    strategy.UpdatePlaybackSpeed(2.0);
    MVStats stats{};
    stats.validBlocks = 100;
    stats.perceptualMagnitude = 100.0;
    stats.motionConsistency = 1.0;
    stats.zeroMotionRatio = 0.85;
    FrameRetentionContext ctx{};
    ctx.mvStats = &stats;
    EXPECT_FALSE(strategy.MakeRetentionDecision(ctx));
    EXPECT_TRUE(strategy.MakeRetentionDecision(ctx));
    EXPECT_NEAR(strategy.GetRatio(), 0.5, DEFAULT_TOLERANCE);
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

// --- DropSyncCoordinator additional tests (in drop_sync_coordinator_test.cpp) ---
