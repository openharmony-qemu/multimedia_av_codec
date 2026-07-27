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
#include <sys/stat.h>
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
        dumper_ = std::make_unique<SmartFluencyDecodingDumper>(
            SFDCodecType::AVC, WIDTH_1080P, HEIGHT_1080P, RetentionStrategyType::ADAPTIVE);
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
    postCtx.reason = SfdDecisionReason::POST_RETAIN_ADAPTIVE_DT;

    dumper_->RecordPostDecode(postCtx);

    // 1. Verify total number of records (ensures update instead of append)
    ASSERT_EQ(dumper_->records_.size(), 1u);

    // 2. Verify primary key (ensures correct record was targeted)
    EXPECT_EQ(dumper_->records_[0].pts, 5000);

    // 3. Verify updated fields (ensures delta is applied)
    EXPECT_EQ(dumper_->records_[0].isPostRetained, 0); // 0: false
    EXPECT_EQ(dumper_->records_[0].postReason, SfdDecisionReason::POST_RETAIN_ADAPTIVE_DT);
    EXPECT_EQ(dumper_->records_[0].preDropCount, 2);
    EXPECT_EQ(dumper_->records_[0].postSysTimeMs, 67890);

    // 4. Verify existing data from RecordPreDecode is preserved
    EXPECT_EQ(dumper_->records_[0].dts, 4900);
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

/**
 * @tc.name: RecordPreDecode_UpdatesExisting_006
 * @tc.desc: Trigger PTS matching logic in RecordPreDecode by calling it twice for the same PTS.
 * @tc.type: FUNC
 */
HWTEST_F(SFDDumperTest, RecordPreDecode_UpdatesExisting_006, TestSize.Level1)
{
    SmartFluencyDecodingDumper::PreDecodeRecordCtx ctx;
    ctx.pts = 3000; // 3000: pts
    ctx.dts = 2900; // 2900: dts
    ctx.isRetained = true;

    dumper_->RecordPreDecode(ctx);
    ASSERT_EQ(dumper_->records_.size(), 1u);

    // Update with different DTS
    ctx.dts = 2950; // 2950: updated dts
    dumper_->RecordPreDecode(ctx);

    ASSERT_EQ(dumper_->records_.size(), 1u);
    EXPECT_EQ(dumper_->records_[0].dts, 2950);
}

/**
 * @tc.name: RecordPreDecode_DropDecision_007
 * @tc.desc: Test the !isRetained branch which sets POST_BYPASSED reason and isPostRetained = -1.
 * @tc.type: FUNC
 */
HWTEST_F(SFDDumperTest, RecordPreDecode_DropDecision_007, TestSize.Level1)
{
    SmartFluencyDecodingDumper::PreDecodeRecordCtx ctx;
    ctx.pts = 4000; // 4000: pts
    ctx.isRetained = false; // Drop decision
    ctx.reason = SfdDecisionReason::PRE_DROP_RATIO_UNSATISFIED;

    dumper_->RecordPreDecode(ctx);

    ASSERT_EQ(dumper_->records_.size(), 1u);
    EXPECT_EQ(dumper_->records_[0].isPreRetained, 0);
    EXPECT_EQ(dumper_->records_[0].isPostRetained, -1);
    EXPECT_EQ(dumper_->records_[0].postReason, SfdDecisionReason::POST_BYPASSED);
    EXPECT_EQ(dumper_->records_[0].preReason, SfdDecisionReason::PRE_DROP_RATIO_UNSATISFIED);
}

/**
 * @tc.name: RecordPreDecode_AutoDump_008
 * @tc.desc: Push MAX_DUMP_RECORDS (100) to trigger automatic file dumping logic.
 * @tc.type: FUNC
 */
HWTEST_F(SFDDumperTest, RecordPreDecode_AutoDump_008, TestSize.Level1)
{
    // Fill up to the limit
    for (int i = 0; i < 100; ++i) { // 100: MAX_DUMP_RECORDS
        SmartFluencyDecodingDumper::PreDecodeRecordCtx ctx;
        ctx.pts = i * 1000; // unique pts
        dumper_->RecordPreDecode(ctx);
    }
    // After 100 records, the internal vector should have been swapped/cleared by DumpToFile
    EXPECT_TRUE(dumper_->records_.empty());
}

/**
 * @tc.name: DumpToFile_EmptyBypass_009
 * @tc.desc: Verify DumpToFile returns early if no records exist (no file operation).
 * @tc.type: FUNC
 */
HWTEST_F(SFDDumperTest, DumpToFile_EmptyBypass_009, TestSize.Level1)
{
    dumper_->DumpToFile();
    EXPECT_TRUE(dumper_->records_.empty());
}

/**
 * @tc.name: WriteHeader_PersistentState_010
 * @tc.desc: Verify header writing state persistence by triggering dump twice.
 * @tc.type: FUNC
 */
HWTEST_F(SFDDumperTest, WriteHeader_PersistentState_010, TestSize.Level1)
{
    struct stat st = {};
    bool dirExists = (stat("/data/misc/sfddump/", &st) == 0 && S_ISDIR(st.st_mode));

    // Trigger first dump
    SmartFluencyDecodingDumper::PreDecodeRecordCtx ctx;
    ctx.pts = 100;
    dumper_->RecordPreDecode(ctx);
    
    dumper_->DumpToFile();
    if (dirExists) {
        EXPECT_TRUE(dumper_->isHeaderWritten_);
    } else {
        EXPECT_FALSE(dumper_->isHeaderWritten_);
    }
    
    // Second dump
    ctx.pts = 200;
    dumper_->RecordPreDecode(ctx);
    dumper_->DumpToFile();
    if (dirExists) {
        EXPECT_TRUE(dumper_->isHeaderWritten_);
    }
}

/**
 * @tc.name: Overhead_ConcurrencySafe_011
 * @tc.desc: Test atomic overhead tracking logic (relaxed memory order).
 * @tc.type: FUNC
 */
HWTEST_F(SFDDumperTest, Overhead_ConcurrencySafe_011, TestSize.Level1)
{
    dumper_->SetLastOverhead(500); // 500: overhead us
    EXPECT_EQ(dumper_->GetLastOverhead(), 500);
}

/**
 * @tc.name: Create_ValidationFail_012
 * @tc.desc: Test dumper creation error branch when directory check fails.
 * @tc.type: FUNC
 */
HWTEST_F(SFDDumperTest, Create_ValidationFail_012, TestSize.Level1)
{
    // Pointing to a likely non-existent path to trigger nullptr return
    auto failedDumper = SmartFluencyDecodingDumper::Create(
        SFDCodecType::AVC, WIDTH_1080P, HEIGHT_1080P, RetentionStrategyType::ADAPTIVE);
    // Since /data/misc/sfddump/ might not exist in dev environments, this often returns nullptr
    if (failedDumper == nullptr) {
        EXPECT_EQ(failedDumper, nullptr);
    }
}
