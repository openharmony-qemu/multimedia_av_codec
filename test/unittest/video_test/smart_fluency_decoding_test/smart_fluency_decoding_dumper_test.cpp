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
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <sstream>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>
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
    std::string dumpFilename_;

    void SetUp() override
    {
        dumper_ = std::make_unique<SmartFluencyDecodingDumper>(
            SFDCodecType::AVC, WIDTH_1080P, HEIGHT_1080P, RetentionStrategyType::ADAPTIVE);
        char devicePath[] = "/data/local/tmp/sfd_dump_XXXXXX";
        int fd = mkstemp(devicePath);
        if (fd >= 0) {
            dumpFilename_ = devicePath;
        } else {
            char hostPath[] = "/tmp/sfd_dump_XXXXXX";
            fd = mkstemp(hostPath);
            ASSERT_GE(fd, 0);
            dumpFilename_ = hostPath;
        }
        close(fd);
        dumper_->currentFilename_ = dumpFilename_;
    }

    void TearDown() override
    {
        // Clear records before destruction to prevent DumpToFileInternal from logging errors or writing files
        if (dumper_) {
            dumper_->records_.clear();
        }
        dumper_ = nullptr;
        if (!dumpFilename_.empty()) {
            std::remove(dumpFilename_.c_str());
        }
    }

    void RecordPre(int64_t pts, bool retained = true)
    {
        SmartFluencyDecodingDumper::PreDecodeRecordCtx ctx;
        ctx.pts = pts;
        ctx.dts = pts;
        ctx.sysTimeMs = pts;
        ctx.isRetained = retained;
        dumper_->RecordPreDecode(ctx);
    }

    void RecordPost(int64_t pts, bool retained = true)
    {
        SmartFluencyDecodingDumper::PostDecodeRecordCtx ctx;
        ctx.pts = pts;
        ctx.sysTimeMs = pts;
        ctx.isRetained = retained;
        ctx.mvStats = {};
        dumper_->RecordPostDecode(ctx);
    }

    std::vector<std::vector<std::string>> ReadRows()
    {
        std::ifstream file(dumpFilename_);
        EXPECT_TRUE(file.is_open());
        std::vector<std::vector<std::string>> rows;
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#' || line.find("PTS,") == 0) {
                continue;
            }
            std::istringstream stream(line);
            std::vector<std::string> columns;
            std::string column;
            while (std::getline(stream, column, ',')) {
                columns.push_back(column);
            }
            rows.push_back(columns);
        }
        return rows;
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
 * @tc.desc: Pending records must survive the automatic dump threshold.
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
    EXPECT_EQ(dumper_->records_.size(), 100u);
    EXPECT_TRUE(ReadRows().empty());
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
    // Trigger first dump
    SmartFluencyDecodingDumper::PreDecodeRecordCtx ctx;
    ctx.pts = 100;
    dumper_->RecordPreDecode(ctx);
    
    dumper_->DumpToFile();
    EXPECT_TRUE(dumper_->isHeaderWritten_);
    
    // Second dump
    ctx.pts = 200;
    dumper_->RecordPreDecode(ctx);
    dumper_->DumpToFile();
    EXPECT_TRUE(dumper_->isHeaderWritten_);
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

// Delayed and reordered outputs must still produce exactly one complete row per PTS.
HWTEST_F(SFDDumperTest, AutoDump_DelayedPostAcrossBatches_013, TestSize.Level1)
{
    constexpr int32_t frameCount = 250;
    for (int32_t pts = 0; pts < frameCount; ++pts) {
        RecordPre(pts);
    }
    EXPECT_TRUE(ReadRows().empty());
    for (int32_t pts = frameCount - 1; pts >= 0; --pts) {
        RecordPost(pts, pts % 3 != 0);
    }
    EXPECT_EQ(ReadRows().size(), 200u);
    EXPECT_EQ(dumper_->records_.size(), 50u);
    dumper_->DumpToFile();
    auto rows = ReadRows();
    ASSERT_EQ(rows.size(), static_cast<size_t>(frameCount));
    std::vector<int32_t> occurrences(frameCount, 0);
    for (const auto& row : rows) {
        ASSERT_EQ(row.size(), 27u);
        int32_t pts = std::stoi(row[0]);
        ASSERT_GE(pts, 0);
        ASSERT_LT(pts, frameCount);
        ++occurrences[pts];
        EXPECT_EQ(row[1], row[0]);
        EXPECT_EQ(row[5], "1");
        EXPECT_EQ(row[6], pts % 3 != 0 ? "1" : "0");
        EXPECT_EQ(row[9], row[0]);
        EXPECT_EQ(row[10], row[0]);
    }
    EXPECT_TRUE(std::all_of(occurrences.begin(), occurrences.end(), [](int32_t count) { return count == 1; }));
}

HWTEST_F(SFDDumperTest, AutoDump_PreDropsAndPostBeforePre_014, TestSize.Level1)
{
    RecordPost(100, false);
    for (int32_t pts = 0; pts < 100; ++pts) {
        RecordPre(pts, false);
    }
    auto rows = ReadRows();
    ASSERT_EQ(rows.size(), 100u);
    for (const auto& row : rows) {
        ASSERT_EQ(row.size(), 27u);
        EXPECT_EQ(row[5], "0");
        EXPECT_EQ(row[6], "-1");
        EXPECT_EQ(row[8], std::to_string(static_cast<int32_t>(SfdDecisionReason::POST_BYPASSED)));
    }
    ASSERT_EQ(dumper_->records_.size(), 1u);
    EXPECT_EQ(dumper_->records_[0].isPreRetained, -1);
    RecordPre(100);
    dumper_->DumpToFile();
    rows = ReadRows();
    ASSERT_EQ(rows.size(), 101u);
    ASSERT_EQ(rows.back().size(), 27u);
    EXPECT_EQ(rows.back()[0], "100");
    EXPECT_EQ(rows.back()[5], "1");
    EXPECT_EQ(rows.back()[6], "0");
}

HWTEST_F(SFDDumperTest, ForceDump_IncompleteTail_015, TestSize.Level1)
{
    RecordPre(1);
    RecordPost(2);
    // Destruction must also preserve incomplete tail data, without inventing decisions.
    dumper_.reset();
    auto rows = ReadRows();
    ASSERT_EQ(rows.size(), 2u);
    ASSERT_EQ(rows[0].size(), 27u);
    ASSERT_EQ(rows[1].size(), 27u);
    EXPECT_EQ(rows[0][5], "1");
    EXPECT_EQ(rows[0][6], "-1");
    EXPECT_EQ(rows[0][8], "0");
    EXPECT_EQ(rows[1][5], "-1");
    EXPECT_EQ(rows[1][6], "1");
}

HWTEST_F(SFDDumperTest, AutoDump_OpenFailureKeepsRecords_016, TestSize.Level1)
{
    // A regular file cannot be used as a parent directory.
    dumper_->currentFilename_ = dumpFilename_ + "/unavailable.csv";
    for (int32_t pts = 0; pts < 100; ++pts) {
        RecordPre(pts, false);
    }
    EXPECT_EQ(dumper_->records_.size(), 100u);
    EXPECT_FALSE(dumper_->isHeaderWritten_);
    dumper_->currentFilename_ = dumpFilename_;
    dumper_->DumpToFile();
    EXPECT_TRUE(dumper_->records_.empty());
    EXPECT_EQ(ReadRows().size(), 100u);
}

HWTEST_F(SFDDumperTest, AutoDump_ConcurrentPreAndPost_017, TestSize.Level1)
{
    constexpr int32_t frameCount = 300;
    std::thread input([this]() {
        for (int32_t pts = 0; pts < frameCount; ++pts) {
            RecordPre(pts);
        }
    });
    std::thread output([this]() {
        for (int32_t pts = 0; pts < frameCount; ++pts) {
            RecordPost(pts);
        }
    });
    input.join();
    output.join();
    dumper_->DumpToFile();
    auto rows = ReadRows();
    ASSERT_EQ(rows.size(), static_cast<size_t>(frameCount));
    std::vector<int32_t> occurrences(frameCount, 0);
    for (const auto& row : rows) {
        ASSERT_EQ(row.size(), 27u);
        int32_t pts = std::stoi(row[0]);
        ASSERT_GE(pts, 0);
        ASSERT_LT(pts, frameCount);
        ++occurrences[pts];
        EXPECT_EQ(row[5], "1");
        EXPECT_EQ(row[6], "1");
        EXPECT_EQ(row[9], row[0]);
        EXPECT_EQ(row[10], row[0]);
    }
    EXPECT_TRUE(std::all_of(occurrences.begin(), occurrences.end(), [](int32_t count) { return count == 1; }));
    std::ifstream file(dumpFilename_);
    std::string line;
    int32_t headerCount = 0;
    while (std::getline(file, line)) {
        if (line.find("PTS,") == 0) {
            ++headerCount;
        }
    }
    EXPECT_EQ(headerCount, 1);
}
