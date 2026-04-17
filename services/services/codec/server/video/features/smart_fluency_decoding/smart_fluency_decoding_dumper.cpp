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

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_SFD, "SmartFluencyDecodingDumper"};
constexpr const char* DUMP_DIR = "/data/misc/sfddump/";
// 【修改】降低分段阈值，每积累 100 帧（约 3~5 秒视频）就刷入一次文件
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
    // 【新增】在构造时就锁定文件名，该实例的整个生命周期都往这个文件追加数据
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
    if (!IsDirectoryExist(DUMP_DIR)) {
        AVCODEC_LOGD("Dump directory %{public}s does not exist. SFD Dumper is disabled.", DUMP_DIR);
        return nullptr;
    }
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

void SmartFluencyDecodingDumper::RecordPreDecode(const PreDecodeRecordCtx& ctx)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = std::find_if(records_.rbegin(), records_.rend(), [ctx](const FrameDumpRecord& rec) {
        return rec.pts == ctx.pts;
    });
    if (it != records_.rend()) {
        it->dts = ctx.dts;
        it->isNonRef = ctx.isNonRef;
        it->flags = ctx.flags;
        it->isPreRetained = ctx.isRetained ? 1 : 0;
        if (!ctx.isRetained) {
            it->isPostRetained = -1; // 解码前丢弃，解码后拿不到
            it->postReason = SfdDecisionReason::POST_BYPASSED;
        }
        it->preSysTimeMs = ctx.sysTimeMs;
        it->mode = currentMode_;
        it->retentionRate = currentRetentionRate_;
        it->speed = currentSpeed_;
        // 新增字段赋值
        it->preDecCostUs = ctx.preDecCostUs;
        it->writeIndex = ctx.writeIndex;
        it->currentEmaDropRatio = ctx.currentEmaDropRatio;
        it->naluCostUs = ctx.naluCostUs; // --- 新增赋值 ---
        it->preReason = ctx.reason;
    } else {
        FrameDumpRecord rec;
        rec.pts = ctx.pts;
        rec.dts = ctx.dts;
        rec.isNonRef = ctx.isNonRef;
        rec.flags = ctx.flags;
        rec.isPreRetained = ctx.isRetained ? 1 : 0;
        if (!ctx.isRetained) {
            rec.isPostRetained = -1; // 解码前丢弃，解码后拿不到
            rec.postReason = SfdDecisionReason::POST_BYPASSED;
        }
        rec.preSysTimeMs = ctx.sysTimeMs;
        rec.mode = currentMode_;
        rec.retentionRate = currentRetentionRate_;
        rec.speed = currentSpeed_;
        // 新增字段赋值
        rec.preDecCostUs = ctx.preDecCostUs;
        rec.writeIndex = ctx.writeIndex;
        rec.currentEmaDropRatio = ctx.currentEmaDropRatio;
        rec.naluCostUs = ctx.naluCostUs; // --- 新增赋值 ---
        rec.preReason = ctx.reason;
        records_.push_back(rec);
    }
    if (records_.size() >= MAX_DUMP_RECORDS) {
        DumpToFileInternal();
    }
}

void SmartFluencyDecodingDumper::RecordPostDecode(const PostDecodeRecordCtx& ctx)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = std::find_if(records_.rbegin(), records_.rend(), [ctx](const FrameDumpRecord& rec) {
        return rec.pts == ctx.pts;
    });
    if (it != records_.rend()) {
        it->isPostRetained = ctx.isRetained ? 1 : 0;
        it->preDropCount = ctx.preDropCount;
        it->postSysTimeMs = ctx.sysTimeMs;
        // 新增字段赋值
        it->postDecCostUs = ctx.postDecCostUs;
        it->readIndex = ctx.readIndex;
        it->currentEmaDropRatio = ctx.currentEmaDropRatio;
        it->validBlocks = ctx.mvStats.validBlocks;
        it->perceptualMagnitude = ctx.mvStats.perceptualMagnitude;
        it->motionConsistency = ctx.mvStats.motionConsistency;
        it->zeroMotionRatio = ctx.mvStats.zeroMotionRatio;
        // 新增细分耗时赋值
        it->copyMvCostUs = ctx.copyMvCostUs;
        it->analyzeMvCostUs = ctx.analyzeMvCostUs;
        it->updateDumperCostUs = ctx.updateDumperCostUs;
        it->postReason = ctx.reason;
    } else {
        FrameDumpRecord rec;
        rec.pts = ctx.pts;
        rec.isPostRetained = ctx.isRetained ? 1 : 0;
        rec.preDropCount = ctx.preDropCount;
        rec.postSysTimeMs = ctx.sysTimeMs;
        rec.mode = currentMode_;
        rec.retentionRate = currentRetentionRate_;
        rec.speed = currentSpeed_;
        // 新增字段赋值
        rec.postDecCostUs = ctx.postDecCostUs;
        rec.readIndex = ctx.readIndex;
        rec.currentEmaDropRatio = ctx.currentEmaDropRatio;
        rec.validBlocks = ctx.mvStats.validBlocks;
        rec.perceptualMagnitude = ctx.mvStats.perceptualMagnitude;
        rec.motionConsistency = ctx.mvStats.motionConsistency;
        rec.zeroMotionRatio = ctx.mvStats.zeroMotionRatio;
        // 新增细分耗时赋值
        rec.copyMvCostUs = ctx.copyMvCostUs;
        rec.analyzeMvCostUs = ctx.analyzeMvCostUs;
        rec.updateDumperCostUs = ctx.updateDumperCostUs;
        rec.postReason = ctx.reason;
        records_.push_back(rec);
    }
    if (records_.size() >= MAX_DUMP_RECORDS) {
        DumpToFileInternal();
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
    std::lock_guard<std::mutex> lock(mutex_);
    DumpToFileInternal();
}

void SmartFluencyDecodingDumper::DumpToFileInternal()
{
    if (records_.empty()) {
        return;
    }
    
    // 对当前 Chunk 内的记录按 pts 升序排列
    std::sort(records_.begin(), records_.end(), [](const FrameDumpRecord& a, const FrameDumpRecord& b) {
        return a.pts < b.pts;
    });

    // 【修改】以 追加模式(std::ios::app) 打开固定文件
    std::ofstream ofs(currentFilename_, std::ios::app);
    CHECK_AND_RETURN_LOG(ofs.is_open(), "Failed to open dump file: %{public}s", currentFilename_.c_str());

    // 【修改】控制文件头和元数据只在第一次创建文件时写入
    if (!isHeaderWritten_) {
        ofs << "# SFD Execution Report\n";
        ofs << "# Timestamp: " << GetTimestampString() << "\n";
        ofs << "# CodecType: " << static_cast<int>(codecType_) << ", Resolution: " << width_ << "x" << height_ << "\n";
        ofs << "# Final Mode: " << static_cast<int>(currentMode_) << "\n";
        ofs << "# CtuSize: " << currentCtuSize_ << "\n";
        ofs << "# ----------------------------------------------------\n";
        // 扩展表头，加上 NaluCostUs，并将 Dropped 改为 Retained
        ofs << "PTS,DTS,Flags,IsNonRef,Mode,PreRetained,PostRetained,"
               "PreReason,PostReason,"
               "PreSysTimeMs,PostSysTimeMs,PreDropCount,RetentionRate,Speed,"
               "PreCostUs,NaluCostUs,PostCostUs,CopyMvUs,AnalyzeMvUs,UpdateDumperUs,"
               "WriteIdx,ReadIdx,EmaDropRatio,"
               "MvValidBlks,MvPerceptMag,MvMotionConsist,MvZeroRatio\n";
        isHeaderWritten_ = true;
    }
           
    for (const auto& rec : records_) {
        ofs << rec.pts << ","
            << rec.dts << ","
            << rec.flags << ","
            << (rec.isNonRef ? 1 : 0) << ","
            << static_cast<int>(rec.mode) << ","
            << rec.isPreRetained << ","
            << rec.isPostRetained << ","
            << static_cast<int>(rec.preReason) << ","
            << static_cast<int>(rec.postReason) << ","
            << rec.preSysTimeMs << ","
            << rec.postSysTimeMs << ","
            << rec.preDropCount << ","
            << rec.retentionRate << ","
            << rec.speed << ","
            << rec.preDecCostUs << ","
            << rec.naluCostUs << "," 
            << rec.postDecCostUs << ","
            << rec.copyMvCostUs << ","
            << rec.analyzeMvCostUs << ","
            << rec.updateDumperCostUs << ","
            << rec.writeIndex << ","
            << rec.readIndex << ","
            << rec.currentEmaDropRatio << ","
            << rec.validBlocks << ","
            << rec.perceptualMagnitude << ","
            << rec.motionConsistency << ","
            << rec.zeroMotionRatio << "\n";
    }
    ofs.close();
    AVCODEC_LOGI("Successfully appended %{public}zu records to %{public}s", records_.size(), currentFilename_.c_str());
    
    // 清空当前内存块，准备接收下一波 300 帧
    records_.clear();
}
} // namespace SFD
} // namespace MediaAVCodec
} // namespace OHOS
