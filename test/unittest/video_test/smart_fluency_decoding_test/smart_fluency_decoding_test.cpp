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
#include "smart_fluency_decoding.h"

namespace {
constexpr int32_t WIDTH_1080P = 1920;
constexpr int32_t HEIGHT_1080P = 1080;
constexpr int32_t WIDTH_720P = 1280;
constexpr int32_t HEIGHT_720P = 720;
}

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
    SFDConfig cfg = MakeConfig(WIDTH_1080P, HEIGHT_1080P, SFDCodecType::AVC, RetentionStrategyType::ADAPTIVE);
    cfg.retentionRatio = 0.5; // 0.5: ratio
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
    SFDConfig cfg = MakeConfig(WIDTH_1080P, HEIGHT_1080P, SFDCodecType::HEVC, RetentionStrategyType::AUTO_RATIO);
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
    SFDConfig cfg = MakeConfig(WIDTH_1080P, HEIGHT_1080P, SFDCodecType::INVALID, RetentionStrategyType::ADAPTIVE);
    int32_t ret = sfd_->Initialize(cfg);
    EXPECT_NE(ret, 0);
}


/**
 * @tc.name: Flush_AfterInit_006
 * @tc.desc: Use white-box testing (private=public hack) to verify all internal states are reset on Flush.
 * @tc.type: FUNC
 */
HWTEST_F(SmartFluencyDecodingTest, Flush_AfterInit_006, TestSize.Level1)
{
    SFDConfig cfg = MakeConfig(WIDTH_720P, HEIGHT_720P, SFDCodecType::HEVC, RetentionStrategyType::AUTO_RATIO);
    sfd_->Initialize(cfg);
    ASSERT_NE(sfd_->dropSyncCoordinator_, nullptr);

    sfd_->dropSyncCoordinator_->writeIndex_ = 50; // 50: mock index
    sfd_->dropSyncCoordinator_->readIndex_ = 10;  // 10: mock index
    sfd_->dropSyncCoordinator_->emaRatio_ = 0.5;   // 0.5: mock ratio
    auto allocator = OHOS::Media::AVAllocatorFactory::CreateSharedAllocator(OHOS::Media::MEMORY_READ_WRITE);
    auto buffer = OHOS::Media::AVBuffer::CreateAVBuffer(allocator, 100);
    sfd_->inBufferMap_[0] = buffer;
    ASSERT_FALSE(sfd_->inBufferMap_.empty());

    sfd_->Flush();
    EXPECT_EQ(sfd_->dropSyncCoordinator_->writeIndex_, 50u);
    EXPECT_EQ(sfd_->dropSyncCoordinator_->readIndex_.load(), sfd_->dropSyncCoordinator_->writeIndex_.load());
    EXPECT_NEAR(sfd_->dropSyncCoordinator_->emaRatio_, 1.0, 0.001);
    EXPECT_TRUE(sfd_->inBufferMap_.empty());
}

/**
 * @tc.name: MakePreDecodeDecision_NoBuffer_007
 * @tc.desc: MakePreDecodeDecision defaults to frame retention if buffer retrieval is empty.
 * @tc.type: FUNC
 */
HWTEST_F(SmartFluencyDecodingTest, MakePreDecodeDecision_NoBuffer_007, TestSize.Level1)
{
    SFDConfig cfg = MakeConfig(WIDTH_1080P, HEIGHT_1080P, SFDCodecType::AVC, RetentionStrategyType::AUTO_RATIO);
    sfd_->Initialize(cfg);
    // No buffer set for index 0
    bool result = sfd_->MakePreDecodeDecision(0);
    EXPECT_TRUE(result);
}

/**
 * @tc.name: InitializeWithCsd_EmptyData_008
 * @tc.desc: Handle empty CSD block gracefully.
 * @tc.type: FUNC
 */
HWTEST_F(SmartFluencyDecodingTest, InitializeWithCsd_EmptyData_008, TestSize.Level1)
{
    SFDConfig cfg = MakeConfig(WIDTH_1080P, HEIGHT_1080P, SFDCodecType::AVC, RetentionStrategyType::AUTO_RATIO);
    sfd_->Initialize(cfg);
    std::vector<uint8_t> emptyCsd;
    EXPECT_FALSE(sfd_->InitializeWithCsd(emptyCsd));
}

/**
 * @tc.name: Initialize_FullMode_009
 * @tc.desc: Validate initialization explicitly allowing Full Passthrough.
 * @tc.type: FUNC
 */
HWTEST_F(SmartFluencyDecodingTest, Initialize_FullMode_009, TestSize.Level1)
{
    SFDConfig cfg = MakeConfig(WIDTH_1080P, HEIGHT_1080P, SFDCodecType::AVC, RetentionStrategyType::FULL);
    int32_t ret = sfd_->Initialize(cfg);
    EXPECT_EQ(ret, 0);
}

/**
 * @tc.name: UpdatePlaybackSpeed_Negative_010
 * @tc.desc: Ensure negative playback speed falls back to DEFAULT_PLAYBACK_SPEED (1.0).
 * @tc.type: FUNC
 */
HWTEST_F(SmartFluencyDecodingTest, UpdatePlaybackSpeed_Negative_010, TestSize.Level1)
{
    SFDConfig cfg = MakeConfig(WIDTH_1080P, HEIGHT_1080P, SFDCodecType::AVC, RetentionStrategyType::ADAPTIVE);
    sfd_->Initialize(cfg);
    sfd_->UpdatePlaybackSpeed(-1.0);
    EXPECT_NEAR(sfd_->currentPlaybackSpeed_, 1.0, 0.001);
}

/**
 * @tc.name: IsRatioValid_Invalid_011
 * @tc.desc: Verify IsRatioValid correctly identifies out-of-bounds ratios.
 * @tc.type: FUNC
 */
HWTEST_F(SmartFluencyDecodingTest, IsRatioValid_Invalid_011, TestSize.Level1)
{
    EXPECT_FALSE(SmartFluencyDecoding::IsRatioValid(0.005)); // < 0.01
    EXPECT_FALSE(SmartFluencyDecoding::IsRatioValid(1.1));  // > 1.0
    EXPECT_TRUE(SmartFluencyDecoding::IsRatioValid(0.01));
    EXPECT_TRUE(SmartFluencyDecoding::IsRatioValid(1.0));
}

/**
 * @tc.name: UpdateDynamicConfig_ModeTransition_012
 * @tc.desc: Verify UpdateDynamicConfig correctly transitions between different retention modes.
 * @tc.type: FUNC
 */
HWTEST_F(SmartFluencyDecodingTest, UpdateDynamicConfig_ModeTransition_012, TestSize.Level1)
{
    SFDConfig cfg = MakeConfig(WIDTH_1080P, HEIGHT_1080P, SFDCodecType::AVC, RetentionStrategyType::ADAPTIVE);
    sfd_->Initialize(cfg);

    OHOS::Media::Format format;
    // Transition to FIXED_RATIO
    format.PutIntValue(OHOS::Media::Tag::VIDEO_DECODER_FRAME_RETENTION_MODE,
                       static_cast<int32_t>(OHOS::Media::Plugins::FrameRetentionMode::UNIFORM));
    format.PutDoubleValue(OHOS::Media::Tag::VIDEO_DECODER_FRAME_RETENTION_RATIO, 0.6);
    int32_t ret = sfd_->UpdateDynamicConfig(format);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(sfd_->config_.initMode, RetentionStrategyType::FIXED_RATIO);
    EXPECT_NEAR(sfd_->config_.retentionRatio.value(), 0.6, 0.001);
}

/**
 * @tc.name: InitializeWithCsd_NotInitialized_013
 * @tc.desc: Ensure InitializeWithCsd fails if the SFD instance is not yet initialized.
 * @tc.type: FUNC
 */
HWTEST_F(SmartFluencyDecodingTest, InitializeWithCsd_NotInitialized_013, TestSize.Level1)
{
    std::vector<uint8_t> csd = {0x00, 0x00, 0x00, 0x01, 0x67};
    EXPECT_FALSE(sfd_->InitializeWithCsd(csd));
}

/**
 * @tc.name: EvaluatePreDecode_EMA_Unsatisfied_014
 * @tc.desc: Verify EvaluatePreDecode returns false for non-ref frames when EMA ratio exceeds target.
 * @tc.type: FUNC
 */
HWTEST_F(SmartFluencyDecodingTest, EvaluatePreDecode_EMA_Unsatisfied_014, TestSize.Level1)
{
    SFDConfig cfg = MakeConfig(WIDTH_1080P, HEIGHT_1080P, SFDCodecType::AVC, RetentionStrategyType::FIXED_RATIO);
    cfg.retentionRatio = 0.5;
    sfd_->Initialize(cfg);

    // Inject non-ideal EMA ratio (0.7 > 0.5)
    sfd_->dropSyncCoordinator_->emaRatio_ = 0.7;
    auto allocator = OHOS::Media::AVAllocatorFactory::CreateSharedAllocator(OHOS::Media::MEMORY_READ_WRITE);
    auto buffer = OHOS::Media::AVBuffer::CreateAVBuffer(allocator, 100); // 100£ºcapacity
    // FakeIsNonReferenceFrame will return true if we set correct data (from dlfcn_mock)
    uint8_t* addr = buffer->memory_->GetAddr();
    addr[0] = 0x00; // 0: start code
    addr[1] = 0x00; // 1: start code
    addr[2] = 0x00; // 2: start code
    addr[3] = 0x01; // 3: nal type
    addr[4] = 0x01; // 4: nal_ref_idc = 0
    buffer->memory_->SetSize(10);
    
    // Push into map for public API access
    sfd_->inBufferMap_[0] = buffer;
    
    sfd_->EnsureNaluAnalyzer();
    bool shouldRetain = sfd_->MakePreDecodeDecision(0);
    EXPECT_FALSE(shouldRetain);
}
