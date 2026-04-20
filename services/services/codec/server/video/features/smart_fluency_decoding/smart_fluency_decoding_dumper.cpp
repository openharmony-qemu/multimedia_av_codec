/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#include "smart_fluency_decoding_dumper.h"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>
#include "avcodec_log.h"
#include "securec.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_SFD, "SmartFluencyDecodingDumper"};
constexpr const char* DUMP_DIR = "/data/misc/sfddump/";
// Reduce segment threshold: Flush to file every 100 frames (approx. 3-5 seconds of video)
constexpr size_t MAX_DUMP_RECORDS = 100;
}

namespace OHOS {
namespace MediaAVCodec {
namespace SFD {
SmartFluencyDecodingDumper::SmartFluencyDecodingDumper(
    SFDCodecType codecType, int32_t width, int32_t height, RetentionStrategyType mode)
    : codecType_(codecType), width_(width), height_(height), currentMode_(mode)
{
    records_.reserve(MAX_DUMP_RECORDS);
    currentFilename_ = std::string(DUMP_DIR) + "SFD_" + GetTimestampString() + ".csv";
}

SmartFluencyDecodingDumper::~SmartFluencyDecodingDumper()
{
    DumpToFile();
}

bool SmartFluencyDecodingDumper::IsDirectoryExist(const std::string& path)
{
    struct stat st = {};
    if (stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
        return true;
    }
    return false;
}

std::unique_ptr<SmartFluencyDecodingDumper> SmartFluencyDecodingDumper::Create(
    SFDCodecType codecType, int32_t width, int32_t height, RetentionStrategyType mode)
{
    CHECK_AND_RETURN_RET_LOG(IsDirectoryExist(DUMP_DIR), nullptr,
        "Dump directory %{public}s does not exist. SFD Dumper is disabled.", DUMP_DIR);
    return std::unique_ptr<SmartFluencyDecodingDumper>(
            new SmartFluencyDecodingDumper(codecType, width, height, mode));
}

void SmartFluencyDecodingDumper::UpdateMode(RetentionStrategyType mode, double retentionRate)
{
    std::lock_guard<std::mutex> lock(mutex_);
    currentMode_ = mode;
    currentRetentionRate_ = retentionRate;
}

void SmartFluencyDecodingDumper::UpdateSpeed(double speed)
{
    std::lock_guard<std::mutex> lock(mutex_);
    currentSpeed_ = speed;
}

void SmartFluencyDecodingDumper::UpdateCtuSize(int32_t ctuSize)
{
    std::lock_guard<std::mutex> lock(mutex_);
    currentCtuSize_ = ctuSize;
}

int64_t SmartFluencyDecodingDumper::GetLastOverhead() const
{
    return lastDumperOverhead_.load(std::memory_order_relaxed);
}

void SmartFluencyDecodingDumper::SetLastOverhead(int64_t overhead)
{
    lastDumperOverhead_.store(overhead, std::memory_order_relaxed);
}

void SmartFluencyDecodingDumper::UpdateExistingPreRecord(const PreDecodeRecordCtx& ctx)
{
    auto it = std::find_if(records_.rbegin(), records_.rend(), [ctx](const FrameDumpRecord& rec) {
        return rec.pts == ctx.pts;
    });
    if (it != records_.rend()) {
        it->dts = ctx.dts;
        it->isNonRef = ctx.isNonRef;
        it->flags = ctx.flags;
        it->isPreRetained = ctx.isRetained ? 1 : 0;
        if (!ctx.isRetained) {
            it->isPostRetained = -1;
            it->postReason = SfdDecisionReason::POST_BYPASSED;
        }
        it->preSysTimeMs = ctx.sysTimeMs;
        it->mode = currentMode_;
        it->retentionRate = currentRetentionRate_;
        it->speed = currentSpeed_;
        it->preDecCostUs = ctx.preDecCostUs;
        it->writeIndex = ctx.writeIndex;
        it->emaRatio = ctx.emaRatio;
        it->naluCostUs = ctx.naluCostUs;
        it->preReason = ctx.reason;
    }
}

void SmartFluencyDecodingDumper::CreateNewPreRecord(const PreDecodeRecordCtx& ctx)
{
    FrameDumpRecord rec;
    rec.pts = ctx.pts;
    rec.dts = ctx.dts;
    rec.isNonRef = ctx.isNonRef;
    rec.flags = ctx.flags;
    rec.isPreRetained = ctx.isRetained ? 1 : 0;
    if (!ctx.isRetained) {
        rec.isPostRetained = -1;
        rec.postReason = SfdDecisionReason::POST_BYPASSED;
    }
    rec.preSysTimeMs = ctx.sysTimeMs;
    rec.mode = currentMode_;
    rec.retentionRate = currentRetentionRate_;
    rec.speed = currentSpeed_;
    rec.preDecCostUs = ctx.preDecCostUs;
    rec.writeIndex = ctx.writeIndex;
    rec.emaRatio = ctx.emaRatio;
    rec.naluCostUs = ctx.naluCostUs;
    rec.preReason = ctx.reason;
    records_.push_back(rec);
}

void SmartFluencyDecodingDumper::RecordPreDecode(const PreDecodeRecordCtx& ctx)
{
    bool needDump = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = std::find_if(records_.rbegin(), records_.rend(), [ctx](const FrameDumpRecord& rec) {
            return rec.pts == ctx.pts;
        });
        if (it != records_.rend()) {
            UpdateExistingPreRecord(ctx);
        } else {
            CreateNewPreRecord(ctx);
        }
        if (records_.size() >= MAX_DUMP_RECORDS) {
            needDump = true;
        }
    }
    if (needDump) {
        DumpToFile();
    }
}

void SmartFluencyDecodingDumper::UpdateExistingPostRecord(const PostDecodeRecordCtx& ctx)
{
    auto it = std::find_if(records_.rbegin(), records_.rend(), [ctx](const FrameDumpRecord& rec) {
        return rec.pts == ctx.pts;
    });
    if (it != records_.rend()) {
        it->isPostRetained = ctx.isRetained ? 1 : 0;
        it->preDropCount = ctx.preDropCount;
        it->postSysTimeMs = ctx.sysTimeMs;
        it->postDecCostUs = ctx.postDecCostUs;
        it->readIndex = ctx.readIndex;
        it->emaRatio = ctx.emaRatio;
        it->validBlocks = ctx.mvStats.validBlocks;
        it->perceptualMagnitude = ctx.mvStats.perceptualMagnitude;
        it->motionConsistency = ctx.mvStats.motionConsistency;
        it->zeroMotionRatio = ctx.mvStats.zeroMotionRatio;
        it->copyMvCostUs = ctx.copyMvCostUs;
        it->analyzeMvCostUs = ctx.analyzeMvCostUs;
        it->updateDumperCostUs = ctx.updateDumperCostUs;
        it->postReason = ctx.reason;
    }
}

void SmartFluencyDecodingDumper::CreateNewPostRecord(const PostDecodeRecordCtx& ctx)
{
    FrameDumpRecord rec;
    rec.pts = ctx.pts;
    rec.isPostRetained = ctx.isRetained ? 1 : 0;
    rec.preDropCount = ctx.preDropCount;
    rec.postSysTimeMs = ctx.sysTimeMs;
    rec.mode = currentMode_;
    rec.retentionRate = currentRetentionRate_;
    rec.speed = currentSpeed_;
    rec.postDecCostUs = ctx.postDecCostUs;
    rec.readIndex = ctx.readIndex;
    rec.emaRatio = ctx.emaRatio;
    rec.validBlocks = ctx.mvStats.validBlocks;
    rec.perceptualMagnitude = ctx.mvStats.perceptualMagnitude;
    rec.motionConsistency = ctx.mvStats.motionConsistency;
    rec.zeroMotionRatio = ctx.mvStats.zeroMotionRatio;
    rec.copyMvCostUs = ctx.copyMvCostUs;
    rec.analyzeMvCostUs = ctx.analyzeMvCostUs;
    rec.updateDumperCostUs = ctx.updateDumperCostUs;
    rec.postReason = ctx.reason;
    records_.push_back(rec);
}

void SmartFluencyDecodingDumper::RecordPostDecode(const PostDecodeRecordCtx& ctx)
{
    bool needDump = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = std::find_if(records_.rbegin(), records_.rend(), [ctx](const FrameDumpRecord& rec) {
            return rec.pts == ctx.pts;
        });
        if (it != records_.rend()) {
            UpdateExistingPostRecord(ctx);
        } else {
            CreateNewPostRecord(ctx);
        }
        if (records_.size() >= MAX_DUMP_RECORDS) {
            needDump = true;
        }
    }
    if (needDump) {
        DumpToFile();
    }
}

std::string SmartFluencyDecodingDumper::GetTimestampString()
{
    auto now = std::chrono::system_clock::now();
    auto timeVal = std::chrono::system_clock::to_time_t(now);
    struct tm tmInfo = {};
    localtime_r(&timeVal, &tmInfo);
    std::stringstream ss;
    ss << std::put_time(&tmInfo, "%Y%m%d_%H%M%S");
    return ss.str();
}

void SmartFluencyDecodingDumper::DumpToFile()
{
    std::vector<FrameDumpRecord> chunk;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (records_.empty()) {
            return;
        }
        chunk.swap(records_);
    }
    std::sort(chunk.begin(), chunk.end(), [](const FrameDumpRecord& a, const FrameDumpRecord& b) {
        return a.pts < b.pts;
    });
    std::ofstream ofs(currentFilename_, std::ios::app);
    CHECK_AND_RETURN_LOG(ofs.is_open(), "Failed to open dump file: %{public}s", currentFilename_.c_str());
    WriteHeaderIfNeeded(ofs);
    WriteRecordsToStream(ofs, chunk);
    ofs.close();
    AVCODEC_LOGI("Successfully appended %{public}zu records to %{public}s", chunk.size(), currentFilename_.c_str());
}

void SmartFluencyDecodingDumper::WriteHeaderIfNeeded(std::ofstream& ofs)
{
    if (isHeaderWritten_) {
        return;
    }
    ofs << "# SFD Execution Report\n# Timestamp: " << GetTimestampString() << "\n";
    ofs << "# CodecType: " << static_cast<int>(codecType_) << ", Resolution: " << width_ << "x" << height_ << "\n";
    ofs << "# Final Mode: " << static_cast<int>(currentMode_) << ", CtuSize: " << currentCtuSize_ << "\n";
    ofs << "# ----------------------------------------------------\n";
    ofs << "PTS,DTS,Flags,IsNonRef,Mode,PreRetained,PostRetained,PreReason,PostReason,PreSysTimeMs,PostSysTimeMs,"
           "PreDropCount,RetentionRate,Speed,PreCostUs,NaluCostUs,PostCostUs,CopyMvUs,AnalyzeMvUs,UpdateDumperUs,"
           "WriteIdx,ReadIdx,EmaRatio,MvValidBlks,MvPerceptMag,MvMotionConsist,MvZeroRatio\n";
    isHeaderWritten_ = true;
}

void SmartFluencyDecodingDumper::WriteRecordsToStream(std::ofstream& ofs, const std::vector<FrameDumpRecord>& records)
{
    for (const auto& rec : records) {
        ofs << rec.pts << "," << rec.dts << "," << rec.flags << "," << (rec.isNonRef ? 1 : 0) << ","
            << static_cast<int>(rec.mode) << "," << rec.isPreRetained << "," << rec.isPostRetained << ","
            << static_cast<int>(rec.preReason) << "," << static_cast<int>(rec.postReason) << ","
            << rec.preSysTimeMs << "," << rec.postSysTimeMs << "," << rec.preDropCount << ",";
            
        ofs << std::fixed << std::setprecision(4) << rec.retentionRate << "," << rec.speed << ",";
        
        ofs << rec.preDecCostUs << "," << rec.naluCostUs << "," << rec.postDecCostUs << ","
            << rec.copyMvCostUs << "," << rec.analyzeMvCostUs << "," << rec.updateDumperCostUs << ","
            << rec.writeIndex << "," << rec.readIndex << "," << rec.emaRatio << ","
            << rec.validBlocks << "," << rec.perceptualMagnitude << "," << rec.motionConsistency << ","
            << rec.zeroMotionRatio << "\n";
    }
    ofs.flush();
}
} // namespace SFD
} // namespace MediaAVCodec
} // namespace OHOS
