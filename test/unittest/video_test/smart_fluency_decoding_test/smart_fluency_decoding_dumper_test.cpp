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
#include "smart_fluency_decoding_dumper.h"

namespace {
constexpr int32_t WIDTH_1080P = 1920;
constexpr int32_t HEIGHT_1080P = 1080;
constexpr double DEFAULT_TOLERANCE = 0.001;
}

using namespace OHOS::MediaAVCodec::SFD;
using namespace testing::ext;

class SFDDumperTest : public testing::Test {
protected:
    std::unique_ptr<SmartFluencyDecodingDumper> dumper_;

    void SetUp() override
    {
        // Bypass Create() directory check by directly invoking the constructor which is public due to -Dprivate=public
        dumper_ = std::unique_ptr<SmartFluencyDecodingDumper>(
            new SmartFluencyDecodingDumper(SFDCodecType::AVC, WIDTH_1080P, HEIGHT_1080P, RetentionStrategyType::ADAPTIVE));
    }

    void TearDown() override
    {
        // Clear records before destruction to prevent DumpToFileInternal from logging errors or writing files
        if (dumper_) {
            dumper_->records_.clear();
        }
        dumper_ = nullptr;
    }
};

/**
 * @tc.name: Init_BasicValidation_001
 * @tc.desc: Ensure test fixture successfully instantiates the SfdDumper object.
 * @tc.type: FUNC
 */
HWTEST_F(SFDDumperTest, Init_BasicValidation_001, TestSize.Level1)
{
    ASSERT_NE(dumper_, nullptr);
    EXPECT_TRUE(dumper_->records_.empty());
}

/**
 * @tc.name: RecordPreDecode_NewRecord_002
 * @tc.desc: Ensure PreDecode pushes a new record to the internal vector if PTS does not exist.
 * @tc.type: FUNC
 */
HWTEST_F(SFDDumperTest, RecordPreDecode_NewRecord_002, TestSize.Level1)
{
    SmartFluencyDecodingDumper::PreDecodeRecordCtx ctx;
    ctx.pts = 1000; // 1000: pts
    ctx.dts = 900; // 900: dts
    ctx.flags = 1; // 1: dummy flag
    ctx.isNonRef = true;
    ctx.isRetained = true;
    ctx.sysTimeMs = 12345; // 12345: dummy sys time

    dumper_->RecordPreDecode(ctx);

    ASSERT_EQ(dumper_->records_.size(), 1u);
    EXPECT_EQ(dumper_->records_[0].pts, 1000);
    EXPECT_EQ(dumper_->records_[0].dts, 900);
    EXPECT_TRUE(dumper_->records_[0].isNonRef);
}

/**
 * @tc.name: RecordPostDecode_UpdatesExisting_003
 * @tc.desc: Ensure PostDecode correctly locates the matching PTS record and updates it.
 * @tc.type: FUNC
 */
HWTEST_F(SFDDumperTest, RecordPostDecode_UpdatesExisting_003, TestSize.Level1)
{
    SmartFluencyDecodingDumper::PreDecodeRecordCtx preCtx;
    preCtx.pts = 5000; // 5000: pts
    preCtx.dts = 4900; // 4900: dts
    dumper_->RecordPreDecode(preCtx);

    SmartFluencyDecodingDumper::PostDecodeRecordCtx postCtx;
    postCtx.pts = 5000;
    postCtx.isRetained = false;
    postCtx.preDropCount = 2; // 2: pre-drop count
    postCtx.sysTimeMs = 67890; // 67890: dummy sys time

    dumper_->RecordPostDecode(postCtx);

    ASSERT_EQ(dumper_->records_.size(), 1u); // Should update existing, not create new
    EXPECT_FALSE(dumper_->records_[0].isPostRetained);
    EXPECT_EQ(dumper_->records_[0].preDropCount, 2);
    EXPECT_EQ(dumper_->records_[0].postSysTimeMs, 67890);
}

/**
 * @tc.name: RecordPostDecode_NewRecord_004
 * @tc.desc: Ensure PostDecode creates a new record if PTS does not exist.
 * @tc.type: FUNC
 */
HWTEST_F(SFDDumperTest, RecordPostDecode_NewRecord_004, TestSize.Level1)
{
    SmartFluencyDecodingDumper::PostDecodeRecordCtx postCtx;
    postCtx.pts = 8000; // 8000: pts
    postCtx.isRetained = false;

    dumper_->RecordPostDecode(postCtx);

    ASSERT_EQ(dumper_->records_.size(), 1u);
    EXPECT_EQ(dumper_->records_[0].pts, 8000);
    EXPECT_FALSE(dumper_->records_[0].isPostRetained);
}

/**
 * @tc.name: UpdateSettings_StateChanged_005
 * @tc.desc: Validate updating speed, mode, and CTU size correctly sets internal variables.
 * @tc.type: FUNC
 */
HWTEST_F(SFDDumperTest, UpdateSettings_StateChanged_005, TestSize.Level1)
{
    dumper_->UpdateMode(RetentionStrategyType::FIXED_RATIO, 0.7); // 0.7: ratio
    dumper_->UpdateSpeed(2.5); // 2.5: speed
    dumper_->UpdateCtuSize(64); // 64: mock ctu size

    EXPECT_EQ(dumper_->currentMode_, RetentionStrategyType::FIXED_RATIO);
    EXPECT_NEAR(dumper_->currentRetentionRate_, 0.7, DEFAULT_TOLERANCE);
    EXPECT_NEAR(dumper_->currentSpeed_, 2.5, DEFAULT_TOLERANCE);
    EXPECT_EQ(dumper_->currentCtuSize_, 64);
}
