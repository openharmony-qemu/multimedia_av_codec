#include <gtest/gtest.h>
#include <memory>
// Including the private implementation to inspect records_ since BUILD.gn provides -Dprivate=public
#include "smart_fluency_decoding_dumper.h"

using namespace OHOS::MediaAVCodec::SFD;
using namespace testing::ext;

class SFDDumperTest : public testing::Test {
protected:
    std::unique_ptr<SmartFluencyDecodingDumper> dumper_;

    void SetUp() override
    {
        // Bypass Create() directory check by directly invoking the constructor which is public due to -Dprivate=public
        dumper_ = std::unique_ptr<SmartFluencyDecodingDumper>(
            new SmartFluencyDecodingDumper(SFDCodecType::AVC, 1920, 1080, RetentionStrategyType::ADAPTIVE));
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
    ctx.pts = 1000;
    ctx.dts = 900;
    ctx.flags = 1;
    ctx.isNonRef = true;
    ctx.dropped = false;
    ctx.sysTimeMs = 12345;

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
    preCtx.pts = 5000;
    preCtx.dts = 4900;
    dumper_->RecordPreDecode(preCtx);

    SmartFluencyDecodingDumper::PostDecodeRecordCtx postCtx;
    postCtx.pts = 5000;
    postCtx.dropped = true;
    postCtx.preDropCount = 2;
    postCtx.sysTimeMs = 67890;
    
    dumper_->RecordPostDecode(postCtx);

    ASSERT_EQ(dumper_->records_.size(), 1u); // Should update existing, not create new
    EXPECT_TRUE(dumper_->records_[0].isPostDropped);
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
    postCtx.pts = 8000;
    postCtx.dropped = true;
    
    dumper_->RecordPostDecode(postCtx);

    ASSERT_EQ(dumper_->records_.size(), 1u);
    EXPECT_EQ(dumper_->records_[0].pts, 8000);
    EXPECT_TRUE(dumper_->records_[0].isPostDropped);
}

/**
 * @tc.name: UpdateSettings_StateChanged_005
 * @tc.desc: Validate updating speed, mode, and CTU size correctly sets internal variables.
 * @tc.type: FUNC
 */
HWTEST_F(SFDDumperTest, UpdateSettings_StateChanged_005, TestSize.Level1)
{
    dumper_->UpdateMode(RetentionStrategyType::FIXED_RATIO, 0.7);
    dumper_->UpdateSpeed(2.5);
    dumper_->UpdateCtuSize(64);

    EXPECT_EQ(dumper_->currentMode_, RetentionStrategyType::FIXED_RATIO);
    EXPECT_NEAR(dumper_->currentRetentionRate_, 0.7, 0.001);
    EXPECT_NEAR(dumper_->currentSpeed_, 2.5, 0.001);
    EXPECT_EQ(dumper_->currentCtuSize_, 64);
}
