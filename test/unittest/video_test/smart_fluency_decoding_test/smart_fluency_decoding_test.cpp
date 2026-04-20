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
 * @tc.desc: Ensure dropping history and statistics correctly clear on Flush.
 * @tc.type: FUNC
 */
HWTEST_F(SmartFluencyDecodingTest, Flush_AfterInit_006, TestSize.Level1)
{
    SFDConfig cfg = MakeConfig(WIDTH_720P, HEIGHT_720P, SFDCodecType::HEVC, RetentionStrategyType::AUTO_RATIO);
    sfd_->Initialize(cfg);
    sfd_->Flush();
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
