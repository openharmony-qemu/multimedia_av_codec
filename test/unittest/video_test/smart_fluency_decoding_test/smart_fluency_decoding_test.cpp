#include <gtest/gtest.h>
#include <memory>
#include "smart_fluency_decoding.h"

using namespace OHOS::MediaAVCodec::SFD;
using namespace testing::ext;

class SmartFluencyDecodingTest : public testing::Test {
protected:
    std::unique_ptr<SmartFluencyDecoding> sfd_;

    void SetUp() override
    {
        sfd_ = std::make_unique<SmartFluencyDecoding>();
    }

    void TearDown() override
    {
        sfd_ = nullptr;
    }

    SFDConfig MakeConfig(int32_t w, int32_t h, SFDCodecType codec, RetentionStrategyType mode)
    {
        SFDConfig cfg;
        cfg.width = w;
        cfg.height = h;
        cfg.codecType = codec;
        cfg.initMode = mode;
        return cfg;
    }
};

/**
 * @tc.name: Initialize_Success_001
 * @tc.desc: Validate successful initialization with a complete configuration.
 * @tc.type: FUNC
 */
HWTEST_F(SmartFluencyDecodingTest, Initialize_Success_001, TestSize.Level1)
{
    SFDConfig cfg = MakeConfig(1920, 1080, SFDCodecType::AVC, RetentionStrategyType::ADAPTIVE);
    cfg.retentionRatio = 0.5;
    int32_t ret = sfd_->Initialize(cfg);
    EXPECT_EQ(ret, 0);
}

/**
 * @tc.name: Initialize_AutoRatio_002
 * @tc.desc: Validate initialization without an explicit retention ratio uses AUTO_RATIO successfully.
 * @tc.type: FUNC
 */
HWTEST_F(SmartFluencyDecodingTest, Initialize_AutoRatio_002, TestSize.Level1)
{
    SFDConfig cfg = MakeConfig(1920, 1080, SFDCodecType::HEVC, RetentionStrategyType::AUTO_RATIO);
    int32_t ret = sfd_->Initialize(cfg);
    EXPECT_EQ(ret, 0);
}

/**
 * @tc.name: Initialize_InvalidDimension_003
 * @tc.desc: Ensure initialization fails gracefully when invalid dimensions are provided.
 * @tc.type: FUNC
 */
HWTEST_F(SmartFluencyDecodingTest, Initialize_InvalidDimension_003, TestSize.Level1)
{
    SFDConfig cfg = MakeConfig(0, 0, SFDCodecType::HEVC, RetentionStrategyType::ADAPTIVE);
    int32_t ret = sfd_->Initialize(cfg);
    EXPECT_NE(ret, 0);
}

/**
 * @tc.name: Initialize_InvalidCodec_004
 * @tc.desc: Ensure initialization fails gracefully when an invalid codec type is provided.
 * @tc.type: FUNC
 */
HWTEST_F(SmartFluencyDecodingTest, Initialize_InvalidCodec_004, TestSize.Level1)
{
    SFDConfig cfg = MakeConfig(1920, 1080, SFDCodecType::INVALID, RetentionStrategyType::ADAPTIVE);
    int32_t ret = sfd_->Initialize(cfg);
    EXPECT_NE(ret, 0);
}

/**
 * @tc.name: GetStatistics_InitialState_005
 * @tc.desc: Ensure statistics map appropriately starts at zero.
 * @tc.type: FUNC
 */
HWTEST_F(SmartFluencyDecodingTest, GetStatistics_InitialState_005, TestSize.Level1)
{
    uint32_t total = 0;
    uint32_t dropped = 0;
    sfd_->GetStatistics(total, dropped);
    EXPECT_EQ(total, 0u);
    EXPECT_EQ(dropped, 0u);
}

/**
 * @tc.name: Flush_AfterInit_006
 * @tc.desc: Ensure dropping history and statistics correctly clear on Flush.
 * @tc.type: FUNC
 */
HWTEST_F(SmartFluencyDecodingTest, Flush_AfterInit_006, TestSize.Level1)
{
    SFDConfig cfg = MakeConfig(1280, 720, SFDCodecType::HEVC, RetentionStrategyType::AUTO_RATIO);
    sfd_->Initialize(cfg);
    sfd_->Flush();

    uint32_t total = 0;
    uint32_t dropped = 0;
    sfd_->GetStatistics(total, dropped);
    EXPECT_EQ(total, 0u);
    EXPECT_EQ(dropped, 0u);
}

/**
 * @tc.name: MakePreDecodeDecision_NoBuffer_007
 * @tc.desc: MakePreDecodeDecision defaults to frame retention if buffer retrieval is empty.
 * @tc.type: FUNC
 */
HWTEST_F(SmartFluencyDecodingTest, MakePreDecodeDecision_NoBuffer_007, TestSize.Level1)
{
    SFDConfig cfg = MakeConfig(1920, 1080, SFDCodecType::AVC, RetentionStrategyType::AUTO_RATIO);
    sfd_->Initialize(cfg);
    // No buffer set for index 0
    bool result = sfd_->MakePreDecodeDecision(0);
    EXPECT_TRUE(result);
}

/**
 * @tc.name: InitializeWithCsd_NullPointer_008
 * @tc.desc: Refuse to initialize properly constructed instances with a null CSD block.
 * @tc.type: FUNC
 */
HWTEST_F(SmartFluencyDecodingTest, InitializeWithCsd_NullPointer_008, TestSize.Level1)
{
    SFDConfig cfg = MakeConfig(1920, 1080, SFDCodecType::AVC, RetentionStrategyType::AUTO_RATIO);
    sfd_->Initialize(cfg);
    EXPECT_FALSE(sfd_->InitializeWithCsd(nullptr));
}

/**
 * @tc.name: Initialize_FullMode_009
 * @tc.desc: Validate initialization explicitly allowing Full Passthrough.
 * @tc.type: FUNC
 */
HWTEST_F(SmartFluencyDecodingTest, Initialize_FullMode_009, TestSize.Level1)
{
    SFDConfig cfg = MakeConfig(1920, 1080, SFDCodecType::AVC, RetentionStrategyType::FULL);
    int32_t ret = sfd_->Initialize(cfg);
    EXPECT_EQ(ret, 0);
}
