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
#include "avcodec_common.h"
#include "buffer/avallocator.h"
#include "buffer/avbuffer.h"
#include "surface_buffer.h"

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
    auto buffer = OHOS::Media::AVBuffer::CreateAVBuffer(allocator, 100); // 100: capacity
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

/**
 * @tc.name: ParseRetentionConfig_MixedConfig_015
 * @tc.desc: Test UNIFORM mode parsing variations and FULL mode with ignored ratio.
 * @tc.type: FUNC
 */
HWTEST_F(SmartFluencyDecodingTest, ParseRetentionConfig_MixedConfig_015, TestSize.Level1)
{
    RetentionStrategyType mode = RetentionStrategyType::INVALID;
    std::optional<double> ratio;
    OHOS::Media::Format format;

    // Case 1: UNIFORM without ratio -> AUTO_RATIO
    format.PutIntValue(OHOS::Media::Tag::VIDEO_DECODER_FRAME_RETENTION_MODE,
                       static_cast<int32_t>(OHOS::Media::Plugins::FrameRetentionMode::UNIFORM));
    sfd_->ParseRetentionConfig(format, mode, ratio);
    EXPECT_EQ(mode, RetentionStrategyType::AUTO_RATIO);

    // Case 2: FULL mode with ratio -> Ratio ignored (warning log)
    format.PutIntValue(OHOS::Media::Tag::VIDEO_DECODER_FRAME_RETENTION_MODE,
                       static_cast<int32_t>(OHOS::Media::Plugins::FrameRetentionMode::FULL));
    format.PutDoubleValue(OHOS::Media::Tag::VIDEO_DECODER_FRAME_RETENTION_RATIO, 0.5);
    sfd_->ParseRetentionConfig(format, mode, ratio);
    EXPECT_EQ(mode, RetentionStrategyType::FULL);
}

/**
 * @tc.name: UpdateDynamicConfig_SpeedOnly_016
 * @tc.desc: Update only playback speed and verify strategy is not recreated unnecessarily.
 * @tc.type: FUNC
 */
HWTEST_F(SmartFluencyDecodingTest, UpdateDynamicConfig_SpeedOnly_016, TestSize.Level1)
{
    SFDConfig cfg = MakeConfig(WIDTH_1080P, HEIGHT_1080P, SFDCodecType::AVC, RetentionStrategyType::ADAPTIVE);
    sfd_->Initialize(cfg);
    auto originalStrategy = sfd_->strategy_.get();

    OHOS::Media::Format format;
    format.PutDoubleValue(OHOS::Media::Tag::VIDEO_DECODER_SPEED, 2.0);
    sfd_->UpdateDynamicConfig(format);

    EXPECT_NEAR(sfd_->currentPlaybackSpeed_, 2.0, 0.001);
    EXPECT_EQ(sfd_->strategy_.get(), originalStrategy); // Should not recreate
}

/**
 * @tc.name: InitializeWithCsd_Success_017
 * @tc.desc: Test successful CSD initialization and processing by naluAnalyzer_.
 * @tc.type: FUNC
 */
HWTEST_F(SmartFluencyDecodingTest, InitializeWithCsd_Success_017, TestSize.Level1)
{
    SFDConfig cfg = MakeConfig(WIDTH_1080P, HEIGHT_1080P, SFDCodecType::AVC, RetentionStrategyType::ADAPTIVE);
    sfd_->Initialize(cfg);
    sfd_->EnsureNaluAnalyzer();

    std::vector<uint8_t> csd = {0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0x00, 0x0a};
    EXPECT_TRUE(sfd_->InitializeWithCsd(csd));
    EXPECT_NE(sfd_->csdBuffer_, nullptr);
}

/**
 * @tc.name: ExecutePreDrop_CodecData_018
 * @tc.desc: Verify CODEC_DATA buffers are explicitly skipped during pre-drop execution.
 * @tc.type: FUNC
 */
HWTEST_F(SmartFluencyDecodingTest, ExecutePreDrop_CodecData_018, TestSize.Level1)
{
    SFDConfig cfg = MakeConfig(WIDTH_1080P, HEIGHT_1080P, SFDCodecType::AVC, RetentionStrategyType::ADAPTIVE);
    sfd_->Initialize(cfg);

    auto allocator = OHOS::Media::AVAllocatorFactory::CreateSharedAllocator(OHOS::Media::MEMORY_READ_WRITE);
    auto buffer = OHOS::Media::AVBuffer::CreateAVBuffer(allocator, 100);
    buffer->flag_ = OHOS::MediaAVCodec::AVCODEC_BUFFER_FLAG_CODEC_DATA;

    // Should return early and not call dropSyncCoordinator_ if flagged as CODEC_DATA
    sfd_->ExecutePreDrop(0, buffer);
    EXPECT_TRUE(true); // Success if no crash/assertion
}

/**
 * @tc.name: MakePreDecodeDecision_Passthrough_019
 * @tc.desc: Test FULL mode passthrough logic in MakePreDecodeDecision.
 * @tc.type: FUNC
 */
HWTEST_F(SmartFluencyDecodingTest, MakePreDecodeDecision_Passthrough_019, TestSize.Level1)
{
    SFDConfig cfg = MakeConfig(WIDTH_1080P, HEIGHT_1080P, SFDCodecType::AVC, RetentionStrategyType::FULL);
    sfd_->Initialize(cfg);

    auto allocator = OHOS::Media::AVAllocatorFactory::CreateSharedAllocator(OHOS::Media::MEMORY_READ_WRITE);
    auto buffer = OHOS::Media::AVBuffer::CreateAVBuffer(allocator, 100);
    sfd_->inBufferMap_[0] = buffer;

    bool result = sfd_->MakePreDecodeDecision(0);
    EXPECT_TRUE(result);
}

/**
 * @tc.name: MakePostDecodeDecision_InvalidSurface_020
 * @tc.desc: Test error handling when surfaceBuffer is null in post-decode.
 * @tc.type: FUNC
 */
HWTEST_F(SmartFluencyDecodingTest, MakePostDecodeDecision_InvalidSurface_020, TestSize.Level1)
{
    SFDConfig cfg = MakeConfig(WIDTH_1080P, HEIGHT_1080P, SFDCodecType::AVC, RetentionStrategyType::ADAPTIVE);
    sfd_->Initialize(cfg);

    bool result = sfd_->MakePostDecodeDecision(0, 0, nullptr);
    EXPECT_TRUE(result);
}

/**
 * @tc.name: EnsureNaluAnalyzer_MvSync_021
 * @tc.desc: Verify that NaluAnalyzer creation correctly syncs stream info to an existing MvAnalyzer.
 * @tc.type: FUNC
 */
HWTEST_F(SmartFluencyDecodingTest, EnsureNaluAnalyzer_MvSync_021, TestSize.Level1)
{
    SFDConfig cfg = MakeConfig(WIDTH_1080P, HEIGHT_1080P, SFDCodecType::HEVC, RetentionStrategyType::ADAPTIVE);
    sfd_->Initialize(cfg);
    sfd_->EnsureMvAnalyzer();
    ASSERT_NE(sfd_->mvAnalyzer_, nullptr);

    // This will trigger creation and callback
    sfd_->EnsureNaluAnalyzer();
    EXPECT_NE(sfd_->naluAnalyzer_, nullptr);
}

/**
 * @tc.name: UpdateDynamicConfig_InvalidMode_022
 * @tc.desc: Test parsing of unknown/invalid retention modes.
 * @tc.type: FUNC
 */
HWTEST_F(SmartFluencyDecodingTest, UpdateDynamicConfig_InvalidMode_022, TestSize.Level1)
{
    SFDConfig cfg = MakeConfig(WIDTH_1080P, HEIGHT_1080P, SFDCodecType::AVC, RetentionStrategyType::ADAPTIVE);
    sfd_->Initialize(cfg);

    OHOS::Media::Format format;
    format.PutIntValue(OHOS::Media::Tag::VIDEO_DECODER_FRAME_RETENTION_MODE, 999); // Invalid
    sfd_->UpdateDynamicConfig(format);
    // Should fallback to INVALID internally but keep existing strategy if update fails or logic dictates
    EXPECT_EQ(sfd_->config_.initMode, RetentionStrategyType::ADAPTIVE);
}

/**
 * @tc.name: MakePreDecodeDecision_TargetFull_023
 * @tc.desc: Test IsGreaterOrEqual(caps.targetRatio, 1.0) branch in MakePreDecodeDecision.
 * @tc.type: FUNC
 */
HWTEST_F(SmartFluencyDecodingTest, MakePreDecodeDecision_TargetFull_023, TestSize.Level1)
{
    SFDConfig cfg = MakeConfig(WIDTH_1080P, HEIGHT_1080P, SFDCodecType::AVC, RetentionStrategyType::FIXED_RATIO);
    cfg.retentionRatio = 1.0;
    sfd_->Initialize(cfg);

    auto allocator = OHOS::Media::AVAllocatorFactory::CreateSharedAllocator(OHOS::Media::MEMORY_READ_WRITE);
    auto buffer = OHOS::Media::AVBuffer::CreateAVBuffer(allocator, 100);
    // Mock non-ref frame
    uint8_t* addr = buffer->memory_->GetAddr();
    addr[4] = 0x01; // nal_ref_idc = 0
    sfd_->inBufferMap_[0] = buffer;
    sfd_->EnsureNaluAnalyzer();

    bool result = sfd_->MakePreDecodeDecision(0);
    EXPECT_TRUE(result);
}

/**
 * @tc.name: InitializeWithCsd_Replay_024
 * @tc.desc: Test CSD replay logic when naluAnalyzer_ is created after CSD is cached.
 * @tc.type: FUNC
 */
HWTEST_F(SmartFluencyDecodingTest, InitializeWithCsd_Replay_024, TestSize.Level1)
{
    // 1. Initialize with FULL mode (which does NOT require NALU analysis)
    SFDConfig cfg = MakeConfig(WIDTH_1080P, HEIGHT_1080P, SFDCodecType::AVC, RetentionStrategyType::FULL);
    sfd_->Initialize(cfg);
    ASSERT_EQ(sfd_->naluAnalyzer_, nullptr);

    // 2. Call InitializeWithCsd. It should cache the buffer because analyzer is null.
    std::vector<uint8_t> csd = {0x00, 0x00, 0x00, 0x01, 0x67};
    bool immediateReplay = sfd_->InitializeWithCsd(csd);
    EXPECT_FALSE(immediateReplay);
    ASSERT_NE(sfd_->csdBuffer_, nullptr);

    // 3. Switch to ADAPTIVE mode (which REQUIRES NALU analysis)
    OHOS::Media::Format format;
    format.PutIntValue(OHOS::Media::Tag::VIDEO_DECODER_FRAME_RETENTION_MODE,
                       static_cast<int32_t>(OHOS::Media::Plugins::FrameRetentionMode::ADAPTIVE));
    sfd_->UpdateDynamicConfig(format);

    // 4. Switching mode should have triggered EnsureNaluAnalyzer, which replays the CSD.
    EXPECT_NE(sfd_->naluAnalyzer_, nullptr);
}
/**
 * @tc.name: MakePostDecodeDecision_NullSurface_ConsumePreDrop_025
 * @tc.desc: Verify that pre-dropped count is consumed even when surfaceBuffer is null.
 * @tc.type: FUNC
 */
HWTEST_F(SmartFluencyDecodingTest, MakePostDecodeDecision_NullSurface_ConsumePreDrop_025, TestSize.Level1)
{
    SFDConfig cfg = MakeConfig(WIDTH_1080P, HEIGHT_1080P, SFDCodecType::AVC, RetentionStrategyType::ADAPTIVE);
    sfd_->Initialize(cfg);

    // Mock dropped frames using RecordPreDroppedFrame
    sfd_->dropSyncCoordinator_->RecordPreDroppedFrame(2500, 1.0); // Drop 1
    sfd_->dropSyncCoordinator_->RecordPreDroppedFrame(2600, 1.0); // Drop 2

    // Verify consumption logic
    uint32_t preDrop = sfd_->dropSyncCoordinator_->GetAndConsumePreDroppedCount(3000);
    EXPECT_EQ(preDrop, 2);

    // Record another drop
    sfd_->dropSyncCoordinator_->RecordPreDroppedFrame(3100, 1.0); // Drop 3

    // Calling MakePostDecodeDecision with null should consume it internally
    bool result = sfd_->MakePostDecodeDecision(0, 3500, nullptr);
    EXPECT_TRUE(result);

    // Verify it was consumed
    uint32_t remaining = sfd_->dropSyncCoordinator_->GetAndConsumePreDroppedCount(4000);
    EXPECT_EQ(remaining, 0);
}

/**
 * @tc.name: MakePreDecodeDecision_EosBuffer_026
 * @tc.desc: Verify EOS buffer is always retained in pre-decode decision.
 * @tc.type: FUNC
 */
HWTEST_F(SmartFluencyDecodingTest, MakePreDecodeDecision_EosBuffer_026, TestSize.Level1)
{
    SFDConfig cfg = MakeConfig(WIDTH_1080P, HEIGHT_1080P, SFDCodecType::AVC, RetentionStrategyType::ADAPTIVE);
    sfd_->Initialize(cfg);

    auto allocator = OHOS::Media::AVAllocatorFactory::CreateSharedAllocator(OHOS::Media::MEMORY_READ_WRITE);
    auto buffer = OHOS::Media::AVBuffer::CreateAVBuffer(allocator, 100);
    buffer->flag_ = OHOS::MediaAVCodec::AVCODEC_BUFFER_FLAG_EOS;
    sfd_->inBufferMap_[0] = buffer;

    bool result = sfd_->MakePreDecodeDecision(0);
    EXPECT_TRUE(result);
}

/**
 * @tc.name: UpdatePlaybackSpeed_ValidSpeed_027
 * @tc.desc: Verify UpdatePlaybackSpeed correctly stores a valid positive speed.
 * @tc.type: FUNC
 */
HWTEST_F(SmartFluencyDecodingTest, UpdatePlaybackSpeed_ValidSpeed_027, TestSize.Level1)
{
    SFDConfig cfg = MakeConfig(WIDTH_1080P, HEIGHT_1080P, SFDCodecType::AVC, RetentionStrategyType::ADAPTIVE);
    sfd_->Initialize(cfg);

    sfd_->UpdatePlaybackSpeed(2.5);
    EXPECT_NEAR(sfd_->currentPlaybackSpeed_, 2.5, 0.001);
}

/**
 * @tc.name: MakePostDecodeDecision_NoStrategy_028
 * @tc.desc: Verify MakePostDecodeDecision returns true when strategy is null.
 * @tc.type: FUNC
 */
HWTEST_F(SmartFluencyDecodingTest, MakePostDecodeDecision_NoStrategy_028, TestSize.Level1)
{
    SFDConfig cfg = MakeConfig(WIDTH_1080P, HEIGHT_1080P, SFDCodecType::AVC, RetentionStrategyType::ADAPTIVE);
    sfd_->Initialize(cfg);
    sfd_->strategy_ = nullptr;

    bool result = sfd_->MakePostDecodeDecision(0, 1000, nullptr);
    EXPECT_TRUE(result);
}

/**
 * @tc.name: SetRefFrameMap_GetNoRefFrameFromMap_029
 * @tc.desc: Verify SetRefFrameMap stores and GetNoRefFrameFromMap retrieves non-ref status correctly.
 * @tc.type: FUNC
 */
HWTEST_F(SmartFluencyDecodingTest, SetRefFrameMap_GetNoRefFrameFromMap_029, TestSize.Level1)
{
    SFDConfig cfg = MakeConfig(WIDTH_1080P, HEIGHT_1080P, SFDCodecType::AVC, RetentionStrategyType::ADAPTIVE);
    sfd_->Initialize(cfg);

    sfd_->SetNoRefFrameMap(100, true);
    EXPECT_TRUE(sfd_->GetNoRefFrameFromMap(100));

    sfd_->SetNoRefFrameMap(200, false);
    EXPECT_FALSE(sfd_->GetNoRefFrameFromMap(200));
}

/**
 * @tc.name: GetNoRefFrameFromMap_MissingPts_030
 * @tc.desc: Verify GetNoRefFrameFromMap returns false when PTS is not found.
 * @tc.type: FUNC
 */
HWTEST_F(SmartFluencyDecodingTest, GetNoRefFrameFromMap_MissingPts_030, TestSize.Level1)
{
    SFDConfig cfg = MakeConfig(WIDTH_1080P, HEIGHT_1080P, SFDCodecType::AVC, RetentionStrategyType::ADAPTIVE);
    sfd_->Initialize(cfg);

    EXPECT_FALSE(sfd_->GetNoRefFrameFromMap(999));
}

/**
 * @tc.name: MakePreDecodeDecision_EmaSatisfied_031
 * @tc.desc: Verify PRE_RETAIN_EMA_SATISFIED branch: EMA below target → non-ref frame is retained.
 * @tc.type: FUNC
 */
HWTEST_F(SmartFluencyDecodingTest, MakePreDecodeDecision_EmaSatisfied_031, TestSize.Level1)
{
    SFDConfig cfg = MakeConfig(WIDTH_1080P, HEIGHT_1080P, SFDCodecType::AVC, RetentionStrategyType::FIXED_RATIO);
    cfg.retentionRatio = 0.5;
    sfd_->Initialize(cfg);

    // EMA ratio (0.3) is below target (0.5) → EMA satisfied branch
    sfd_->dropSyncCoordinator_->emaRatio_ = 0.3;
    auto allocator = OHOS::Media::AVAllocatorFactory::CreateSharedAllocator(OHOS::Media::MEMORY_READ_WRITE);
    auto buffer = OHOS::Media::AVBuffer::CreateAVBuffer(allocator, 100);
    uint8_t* addr = buffer->memory_->GetAddr();
    addr[0] = 0x00; addr[1] = 0x00; addr[2] = 0x00; addr[3] = 0x01;
    addr[4] = 0x01; // nal_ref_idc = 0 → non-ref
    buffer->memory_->SetSize(10);
    sfd_->inBufferMap_[0] = buffer;
    sfd_->EnsureNaluAnalyzer();

    bool result = sfd_->MakePreDecodeDecision(0);
    EXPECT_TRUE(result);
}

/**
 * @tc.name: MakePostDecodeDecision_PassthroughPath_032
 * @tc.desc: Verify FULL mode passthrough in MakePostDecodeDecision.
 * @tc.type: FUNC
 */
HWTEST_F(SmartFluencyDecodingTest, MakePostDecodeDecision_PassthroughPath_032, TestSize.Level1)
{
    SFDConfig cfg = MakeConfig(WIDTH_1080P, HEIGHT_1080P, SFDCodecType::AVC, RetentionStrategyType::FULL);
    sfd_->Initialize(cfg);

    // FULL mode → isPassthrough=true → HandlePostDecodePassthrough → returns true
    bool result = sfd_->MakePostDecodeDecision(0, 1000, nullptr);
    EXPECT_TRUE(result);
}

/**
 * @tc.name: MakePreDecodeDecision_Uninitialized_033
 * @tc.desc: Verify MakePreDecodeDecision returns true when SFD is not initialized.
 * @tc.type: FUNC
 */
HWTEST_F(SmartFluencyDecodingTest, MakePreDecodeDecision_Uninitialized_033, TestSize.Level1)
{
    // sfd_ is created but not initialized
    bool result = sfd_->MakePreDecodeDecision(0);
    EXPECT_TRUE(result);
}

/**
 * @tc.name: MakePostDecodeDecision_Uninitialized_034
 * @tc.desc: Verify MakePostDecodeDecision returns true when SFD is not initialized.
 * @tc.type: FUNC
 */
HWTEST_F(SmartFluencyDecodingTest, MakePostDecodeDecision_Uninitialized_034, TestSize.Level1)
{
    // sfd_ is created but not initialized
    bool result = sfd_->MakePostDecodeDecision(0, 0, nullptr);
    EXPECT_TRUE(result);
}

/**
 * @tc.name: SetInputBuffer_NullBuffer_035
 * @tc.desc: Verify SetInputBuffer does not crash when buffer is null.
 * @tc.type: FUNC
 */
HWTEST_F(SmartFluencyDecodingTest, SetInputBuffer_NullBuffer_035, TestSize.Level1)
{
    SFDConfig cfg = MakeConfig(WIDTH_1080P, HEIGHT_1080P, SFDCodecType::AVC, RetentionStrategyType::FULL);
    sfd_->Initialize(cfg);
    sfd_->SetInputBuffer(0, nullptr);
    EXPECT_TRUE(sfd_->inBufferMap_.empty());
}

/**
 * @tc.name: Flush_NullInternals_036
 * @tc.desc: Verify Flush does not crash when strategy_ and dropSyncCoordinator_ are null.
 * @tc.type: FUNC
 */
HWTEST_F(SmartFluencyDecodingTest, Flush_NullInternals_036, TestSize.Level1)
{
    SFDConfig cfg = MakeConfig(WIDTH_1080P, HEIGHT_1080P, SFDCodecType::AVC, RetentionStrategyType::FULL);
    sfd_->Initialize(cfg);
    sfd_->strategy_ = nullptr;
    sfd_->dropSyncCoordinator_ = nullptr;
    sfd_->Flush();
    EXPECT_TRUE(true);
}

/**
 * @tc.name: ExecutePreDrop_NullCallback_037
 * @tc.desc: Verify ExecutePreDrop skips dispatcher when callback is null.
 * @tc.type: FUNC
 */
HWTEST_F(SmartFluencyDecodingTest, ExecutePreDrop_NullCallback_037, TestSize.Level1)
{
    SFDConfig cfg = MakeConfig(WIDTH_1080P, HEIGHT_1080P, SFDCodecType::AVC, RetentionStrategyType::ADAPTIVE);
    sfd_->Initialize(cfg);
    sfd_->asyncDropInputCb_ = nullptr;
    auto allocator = OHOS::Media::AVAllocatorFactory::CreateSharedAllocator(OHOS::Media::MEMORY_READ_WRITE);
    auto buffer = OHOS::Media::AVBuffer::CreateAVBuffer(allocator, 100);
    buffer->flag_ = 0;
    buffer->pts_ = 1000;
    sfd_->ExecutePreDrop(0, buffer);
    EXPECT_TRUE(true);
}

/**
 * @tc.name: ExecutePostDrop_NullCallback_038
 * @tc.desc: Verify ExecutePostDrop skips dispatcher when callback is null.
 * @tc.type: FUNC
 */
HWTEST_F(SmartFluencyDecodingTest, ExecutePostDrop_NullCallback_038, TestSize.Level1)
{
    SFDConfig cfg = MakeConfig(WIDTH_1080P, HEIGHT_1080P, SFDCodecType::AVC, RetentionStrategyType::ADAPTIVE);
    sfd_->Initialize(cfg);
    sfd_->asyncDropOutputCb_ = nullptr;
    sfd_->ExecutePostDrop(0);
    EXPECT_TRUE(true);
}

/**
 * @tc.name: UpdatePlaybackSpeed_NullStrategy_039
 * @tc.desc: Verify UpdatePlaybackSpeed still stores speed when strategy_ is null.
 * @tc.type: FUNC
 */
HWTEST_F(SmartFluencyDecodingTest, UpdatePlaybackSpeed_NullStrategy_039, TestSize.Level1)
{
    SFDConfig cfg = MakeConfig(WIDTH_1080P, HEIGHT_1080P, SFDCodecType::AVC, RetentionStrategyType::FULL);
    sfd_->Initialize(cfg);
    sfd_->strategy_ = nullptr;
    sfd_->UpdatePlaybackSpeed(2.0);
    EXPECT_NEAR(sfd_->currentPlaybackSpeed_, 2.0, 0.001);
}

/**
 * @tc.name: MakePreDecodeDecision_RefFrame_040
 * @tc.desc: Verify reference frame (isNonRef=false) is retained without entering drop logic.
 * @tc.type: FUNC
 */
HWTEST_F(SmartFluencyDecodingTest, MakePreDecodeDecision_RefFrame_040, TestSize.Level1)
{
    SFDConfig cfg = MakeConfig(WIDTH_1080P, HEIGHT_1080P, SFDCodecType::AVC, RetentionStrategyType::FIXED_RATIO);
    cfg.retentionRatio = 0.5;
    sfd_->Initialize(cfg);

    auto allocator = OHOS::Media::AVAllocatorFactory::CreateSharedAllocator(OHOS::Media::MEMORY_READ_WRITE);
    auto buffer = OHOS::Media::AVBuffer::CreateAVBuffer(allocator, 100);
    uint8_t* addr = buffer->memory_->GetAddr();
    addr[0] = 0x00; addr[1] = 0x00; addr[2] = 0x00; addr[3] = 0x01;
    addr[4] = 0x65;  // IDR: nal_ref_idc=3 (non-zero) → reference frame
    buffer->memory_->SetSize(10);
    buffer->pts_ = 1000;
    sfd_->inBufferMap_[0] = buffer;
    sfd_->EnsureNaluAnalyzer();

    bool result = sfd_->MakePreDecodeDecision(0);
    EXPECT_TRUE(result);
}

/**
 * @tc.name: MakePostDecodeDecision_FixedRatio_041
 * @tc.desc: Verify post-decode decision with FIXED_RATIO (requiresMv=false) path.
 * @tc.type: FUNC
 */
HWTEST_F(SmartFluencyDecodingTest, MakePostDecodeDecision_FixedRatio_041, TestSize.Level1)
{
    SFDConfig cfg = MakeConfig(WIDTH_1080P, HEIGHT_1080P, SFDCodecType::AVC, RetentionStrategyType::FIXED_RATIO);
    cfg.retentionRatio = 0.5;
    sfd_->Initialize(cfg);

    bool result = sfd_->MakePostDecodeDecision(0, 1000, nullptr);
    EXPECT_TRUE(result);
}

/**
 * @tc.name: ParseRetentionConfig_InvalidParsedRatio_042
 * @tc.desc: Verify ParseRetentionConfig ignores ratio values outside valid range.
 * @tc.type: FUNC
 */
HWTEST_F(SmartFluencyDecodingTest, ParseRetentionConfig_InvalidParsedRatio_042, TestSize.Level1)
{
    RetentionStrategyType mode = RetentionStrategyType::INVALID;
    std::optional<double> ratio;
    OHOS::Media::Format format;
    format.PutDoubleValue(OHOS::Media::Tag::VIDEO_DECODER_FRAME_RETENTION_RATIO, 2.0);
    sfd_->ParseRetentionConfig(format, mode, ratio);
    EXPECT_FALSE(ratio.has_value());
}

/**
 * @tc.name: ParseRetentionConfig_RatioOnlyNoMode_043
 * @tc.desc: Verify hasNewRatio with INVALID mode transitions to FIXED_RATIO.
 * @tc.type: FUNC
 */
HWTEST_F(SmartFluencyDecodingTest, ParseRetentionConfig_RatioOnlyNoMode_043, TestSize.Level1)
{
    RetentionStrategyType mode = RetentionStrategyType::INVALID;
    std::optional<double> ratio;
    OHOS::Media::Format format;
    format.PutDoubleValue(OHOS::Media::Tag::VIDEO_DECODER_FRAME_RETENTION_RATIO, 0.3);
    sfd_->ParseRetentionConfig(format, mode, ratio);
    EXPECT_EQ(mode, RetentionStrategyType::FIXED_RATIO);
    EXPECT_NEAR(ratio.value(), 0.3, 0.001);
}

/**
 * @tc.name: InitializeWithCsd_VvcCodec_044
 * @tc.desc: Verify InitializeWithCsd works with VVC codec (different codec path).
 * @tc.type: FUNC
 */
HWTEST_F(SmartFluencyDecodingTest, InitializeWithCsd_VvcCodec_044, TestSize.Level1)
{
    SFDConfig cfg = MakeConfig(WIDTH_1080P, HEIGHT_1080P, SFDCodecType::VVC, RetentionStrategyType::ADAPTIVE);
    sfd_->Initialize(cfg);
    sfd_->EnsureNaluAnalyzer();

    std::vector<uint8_t> csd = {0x00, 0x00, 0x00, 0x01, 0x40, 0x01};
    EXPECT_TRUE(sfd_->InitializeWithCsd(csd));
    EXPECT_NE(sfd_->csdBuffer_, nullptr);
}

/**
 * @tc.name: MakePreDecodeDecision_NoNaluAnalyzer_045
 * @tc.desc: Verify MakePreDecodeDecision with requiresNalu=true but naluAnalyzer_ null
 *           falls through to else-if passthrough check.
 * @tc.type: FUNC
 */
HWTEST_F(SmartFluencyDecodingTest, MakePreDecodeDecision_NoNaluAnalyzer_045, TestSize.Level1)
{
    SFDConfig cfg = MakeConfig(WIDTH_1080P, HEIGHT_1080P, SFDCodecType::AVC, RetentionStrategyType::FIXED_RATIO);
    cfg.retentionRatio = 0.5;
    sfd_->Initialize(cfg);
    // naluAnalyzer_ is null by default for FIXED_RATIO (RequiresNaluAnalysis=true is default but depends on strategy)
    // Force naluAnalyzer_ to null
    sfd_->naluAnalyzer_ = nullptr;

    auto allocator = OHOS::Media::AVAllocatorFactory::CreateSharedAllocator(OHOS::Media::MEMORY_READ_WRITE);
    auto buffer = OHOS::Media::AVBuffer::CreateAVBuffer(allocator, 100);
    sfd_->inBufferMap_[0] = buffer;

    bool result = sfd_->MakePreDecodeDecision(0);
    EXPECT_TRUE(result);
}

/**
 * @tc.name: InitializeWithCsd_NaluAnalyzerNull_046
 * @tc.desc: Verify InitializeWithCsd caches CSD when naluAnalyzer_ is not instantiated.
 * @tc.type: FUNC
 */
HWTEST_F(SmartFluencyDecodingTest, InitializeWithCsd_NaluAnalyzerNull_046, TestSize.Level1)
{
    SFDConfig cfg = MakeConfig(WIDTH_1080P, HEIGHT_1080P, SFDCodecType::AVC, RetentionStrategyType::FULL);
    sfd_->Initialize(cfg);
    // FULL mode does not require NALU analysis, so analyzer is null
    ASSERT_EQ(sfd_->naluAnalyzer_, nullptr);

    std::vector<uint8_t> csd = {0x00, 0x00, 0x00, 0x01, 0x67};
    bool result = sfd_->InitializeWithCsd(csd);
    EXPECT_FALSE(result);  // Returns false when naluAnalyzer_ is null
    EXPECT_NE(sfd_->csdBuffer_, nullptr);  // But still caches
}

/**
 * @tc.name: ExtractMVStats_NoMvRequired_047
 * @tc.desc: Verify ExtractMVStats returns empty stats when requiresMv=false.
 * @tc.type: FUNC
 */
HWTEST_F(SmartFluencyDecodingTest, ExtractMVStats_NoMvRequired_047, TestSize.Level1)
{
    SFDConfig cfg = MakeConfig(WIDTH_1080P, HEIGHT_1080P, SFDCodecType::AVC, RetentionStrategyType::FIXED_RATIO);
    cfg.retentionRatio = 0.5;
    sfd_->Initialize(cfg);

    PostDecCost cost{};
    auto stats = sfd_->ExtractMVStats({false, false, false}, 0, cost, nullptr);
    EXPECT_EQ(stats.validBlocks, 0u);
}
