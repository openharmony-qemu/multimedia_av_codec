#include <gtest/gtest.h>
#include <memory>
#include <optional>
// #include "strategies/fixed_ratio_retention_strategy.h"
// #include "strategies/auto_fallback_retention_strategy.h"
// #include "strategies/adaptive_retention_strategy.h"
// #include "strategies/full_retention_strategy.h"
// #include "strategies/retention_strategy_factory.h"
#include "fixed_ratio_retention_strategy.h"
#include "auto_fallback_retention_strategy.h"
#include "adaptive_retention_strategy.h"
#include "full_retention_strategy.h"
#include "retention_strategy_factory.h"

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
    auto strategy = std::make_unique<FixedRatioRetentionStrategy>(0.5);
    int retainedCount = 0;
    int totalFrames = 100;
    FrameRetentionContext ctx{};
    for (int i = 0; i < totalFrames; ++i) {
        if (strategy->MakeRetentionDecision(ctx)) retainedCount++;
    }
    EXPECT_EQ(retainedCount, 50);
}

/**
 * @tc.name: MakeDecision_33PercentRatio_002
 * @tc.desc: Validate that approximately 33% of frames are retained when ratio is 0.33.
 * @tc.type: FUNC
 */
HWTEST_F(FixedRatioRetentionStrategyTest, MakeDecision_33PercentRatio_002, TestSize.Level1)
{
    auto strategy = std::make_unique<FixedRatioRetentionStrategy>(0.33);
    int retainedCount = 0;
    int totalFrames = 100;
    FrameRetentionContext ctx{};
    for (int i = 0; i < totalFrames; ++i) {
        if (strategy->MakeRetentionDecision(ctx)) retainedCount++;
    }
    EXPECT_NEAR(retainedCount, 33, 1);
}

/**
 * @tc.name: MakeDecision_100PercentRatio_003
 * @tc.desc: Validate that all frames are retained when ratio is 1.0.
 * @tc.type: FUNC
 */
HWTEST_F(FixedRatioRetentionStrategyTest, MakeDecision_100PercentRatio_003, TestSize.Level1)
{
    auto strategy = std::make_unique<FixedRatioRetentionStrategy>(1.0);
    int retainedCount = 0;
    int totalFrames = 100;
    FrameRetentionContext ctx{};
    for (int i = 0; i < totalFrames; ++i) {
        if (strategy->MakeRetentionDecision(ctx)) retainedCount++;
    }
    EXPECT_EQ(retainedCount, 100);
}

/**
 * @tc.name: MakeDecision_1PercentRatio_004
 * @tc.desc: Validate that extremely low ratio is handled correctly.
 * @tc.type: FUNC
 */
HWTEST_F(FixedRatioRetentionStrategyTest, MakeDecision_1PercentRatio_004, TestSize.Level1)
{
    auto strategy = std::make_unique<FixedRatioRetentionStrategy>(0.01);
    int retainedCount = 0;
    int totalFrames = 1000;
    FrameRetentionContext ctx{};
    for (int i = 0; i < totalFrames; ++i) {
        if (strategy->MakeRetentionDecision(ctx)) retainedCount++;
    }
    EXPECT_NEAR(retainedCount, 11, 2);
}

/**
 * @tc.name: GetCurrentRetentionRatio_ReturnsInitializationValue_005
 * @tc.desc: Ensure GetCurrentRetentionRatio returns the ratio provided at construction.
 * @tc.type: FUNC
 */
HWTEST_F(FixedRatioRetentionStrategyTest, GetCurrentRetentionRatio_ReturnsInitializationValue_005, TestSize.Level1)
{
    auto strategy = std::make_unique<FixedRatioRetentionStrategy>(0.7);
    EXPECT_NEAR(strategy->GetCurrentRetentionRatio(), 0.7, 0.01);
}

/**
 * @tc.name: GetMode_ReturnsFixedRatio_006
 * @tc.desc: Ensure GetMode returns FIXED_RATIO.
 * @tc.type: FUNC
 */
HWTEST_F(FixedRatioRetentionStrategyTest, GetMode_ReturnsFixedRatio_006, TestSize.Level1)
{
    auto strategy = std::make_unique<FixedRatioRetentionStrategy>(0.5);
    EXPECT_EQ(strategy->GetMode(), RetentionStrategyType::FIXED_RATIO);
}

/**
 * @tc.name: Reset_RestartsInternalState_007
 * @tc.desc: Ensure Reset functions properly by enforcing the first frame rule.
 * @tc.type: FUNC
 */
HWTEST_F(FixedRatioRetentionStrategyTest, Reset_RestartsInternalState_007, TestSize.Level1)
{
    auto strategy = std::make_unique<FixedRatioRetentionStrategy>(0.1);
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
    auto strategy = std::make_unique<FixedRatioRetentionStrategy>(0.5);
    FrameRetentionContext ctx{};
    ctx.preDroppedCount = 2; 
    EXPECT_TRUE(strategy->MakeRetentionDecision(ctx));
}

// --- AutoFallbackRetentionStrategy ---

class AutoFallbackRetentionStrategyTest : public testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

/**
 * @tc.name: MakeDecision_BasicInitialState_001
 * @tc.desc: Verify the basic functionality at time 0.
 * @tc.type: FUNC
 */
HWTEST_F(AutoFallbackRetentionStrategyTest, MakeDecision_BasicInitialState_001, TestSize.Level1)
{
    auto strategy = std::make_unique<AutoFallbackRetentionStrategy>();
    FrameRetentionContext ctx{};
    ctx.sysTimeMs = 0;
    ctx.ptsMs = 0;
    EXPECT_TRUE(strategy->MakeRetentionDecision(ctx));
}

/**
 * @tc.name: GetMode_ReturnsAutoRatio_002
 * @tc.desc: Validate GetMode returns correctly.
 * @tc.type: FUNC
 */
HWTEST_F(AutoFallbackRetentionStrategyTest, GetMode_ReturnsAutoRatio_002, TestSize.Level1)
{
    auto strategy = std::make_unique<AutoFallbackRetentionStrategy>();
    EXPECT_EQ(strategy->GetMode(), RetentionStrategyType::AUTO_RATIO);
}

/**
 * @tc.name: ProfilingToStable_Transition_003
 * @tc.desc: Verify the transition from profiling to stable states.
 * @tc.type: FUNC
 */
HWTEST_F(AutoFallbackRetentionStrategyTest, ProfilingToStable_Transition_003, TestSize.Level1)
{
    auto strategy = std::make_unique<AutoFallbackRetentionStrategy>();
    strategy->UpdatePlaybackSpeed(2.0);
    FrameRetentionContext ctx{};
    for (int i = 0; i < 35; ++i) {
        ctx.ptsMs = i * 33;
        ctx.sysTimeMs = i * 33;
        strategy->MakeRetentionDecision(ctx);
    }
    double ratio = strategy->GetCurrentRetentionRatio();
    EXPECT_GT(ratio, 0.0);
    EXPECT_LT(ratio, 1.0);
}

/**
 * @tc.name: UpdatePlaybackSpeed_VariousSpeeds_004
 * @tc.desc: Basic operations for playback speed modifications.
 * @tc.type: FUNC
 */
HWTEST_F(AutoFallbackRetentionStrategyTest, UpdatePlaybackSpeed_VariousSpeeds_004, TestSize.Level1)
{
    auto strategy = std::make_unique<AutoFallbackRetentionStrategy>();
    strategy->UpdatePlaybackSpeed(1.0);
    strategy->UpdatePlaybackSpeed(2.0);
    strategy->UpdatePlaybackSpeed(0.5);
    strategy->UpdatePlaybackSpeed(-1.0);
    EXPECT_NE(strategy, nullptr);
}

/**
 * @tc.name: Reset_RestartsToProfiling_005
 * @tc.desc: Verify that Reset correctly clears state and goes back to profiling mode.
 * @tc.type: FUNC
 */
HWTEST_F(AutoFallbackRetentionStrategyTest, Reset_RestartsToProfiling_005, TestSize.Level1)
{
    auto strategy = std::make_unique<AutoFallbackRetentionStrategy>();
    strategy->UpdatePlaybackSpeed(2.0);
    FrameRetentionContext ctx{};
    for (int i = 0; i < 35; ++i) {
        ctx.ptsMs = i * 33;
        ctx.sysTimeMs = i * 33;
        strategy->MakeRetentionDecision(ctx);
    }
    strategy->Reset();
    EXPECT_NEAR(strategy->GetCurrentRetentionRatio(), 1.0, 0.001);
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
 * @tc.name: UpdatePlaybackSpeed_NoCrash_003
 * @tc.desc: Set playback speed.
 * @tc.type: FUNC
 */
HWTEST_F(AdaptiveRetentionStrategyTest, UpdatePlaybackSpeed_NoCrash_003, TestSize.Level1)
{
    auto strategy = std::make_unique<AdaptiveRetentionStrategy>();
    strategy->UpdatePlaybackSpeed(2.0);
    EXPECT_NE(strategy, nullptr);
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
    for (int i = 0; i < 50; i++) {
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
    auto strategy = RetentionStrategyFactory::CreateStrategy(RetentionStrategyType::FIXED_RATIO, 0.5);
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
