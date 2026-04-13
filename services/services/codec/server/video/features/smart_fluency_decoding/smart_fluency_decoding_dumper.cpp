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
constexpr size_t MAX_DUMP_RECORDS = 10000;
}

namespace OHOS {
namespace MediaAVCodec {
namespace SFD {
SmartFluencyDecodingDumper::SmartFluencyDecodingDumper(
    SFDCodecType codecType, int32_t width, int32_t height, RetentionStrategyType mode)
    : codecType_(codecType), width_(width), height_(height), currentMode_(mode)
{
    records_.reserve(MAX_DUMP_RECORDS);
}

SmartFluencyDecodingDumper::~SmartFluencyDecodingDumper()
{
    DumpToFile();
}

bool SmartFluencyDecodingDumper::IsDirectoryExist(const std::string& path)
{
    struct stat st = {0};
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
        it->isPreDropped = ctx.dropped;
        it->preSysTimeMs = ctx.sysTimeMs;
        it->mode = currentMode_;
        it->retentionRate = currentRetentionRate_;
        it->speed = currentSpeed_;
    } else {
        FrameDumpRecord rec;
        rec.pts = ctx.pts;
        rec.dts = ctx.dts;
        rec.isNonRef = ctx.isNonRef;
        rec.flags = ctx.flags;
        rec.isPreDropped = ctx.dropped;
        rec.preSysTimeMs = ctx.sysTimeMs;
        rec.mode = currentMode_;
        rec.retentionRate = currentRetentionRate_;
        rec.speed = currentSpeed_;
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
        it->isPostDropped = ctx.dropped;
        it->preDropCount = ctx.preDropCount;
        it->postSysTimeMs = ctx.sysTimeMs;
    } else {
        FrameDumpRecord rec;
        rec.pts = ctx.pts;
        rec.isPostDropped = ctx.dropped;
        rec.preDropCount = ctx.preDropCount;
        rec.postSysTimeMs = ctx.sysTimeMs;
        rec.mode = currentMode_;
        rec.retentionRate = currentRetentionRate_;
        rec.speed = currentSpeed_;
        records_.push_back(rec);
    }
    if (records_.size() >= MAX_DUMP_RECORDS) {
        DumpToFileInternal();
    }
}

std::string SmartFluencyDecodingDumper::GetTimestampString()
{
    auto now = std::chrono::system_clock::now();
    auto time_t_val = std::chrono::system_clock::to_time_t(now);
    struct tm tm_info = {0};
    localtime_r(&time_t_val, &tm_info);
    std::stringstream ss;
    ss << std::put_time(&tm_info, "%Y%m%d_%H%M%S");
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
    std::string timestamp = GetTimestampString();
    std::string filename = std::string(DUMP_DIR) + "SFD_" + timestamp + ".csv";
    std::ofstream ofs(filename);
    if (!ofs.is_open()) {
        AVCODEC_LOGE("Failed to open dump file: %{public}s", filename.c_str());
        return;
    }
    ofs << "# SFD Execution Report\n";
    ofs << "# Timestamp: " << timestamp << "\n";
    ofs << "# CodecType: " << static_cast<int>(codecType_) << ", Resolution: " << width_ << "x" << height_ << "\n";
    ofs << "# Final Mode: " << static_cast<int>(currentMode_) << "\n";
    ofs << "# CtuSize: " << currentCtuSize_ << "\n";
    ofs << "# ----------------------------------------------------\n";
    ofs << "PTS,DTS,Flags,IsNonRef,Mode,PreDropped,PostDropped,"
           "PreSysTimeMs,PostSysTimeMs,PreDropCount,RetentionRate,Speed\n";
    for (const auto& rec : records_) {
        ofs << rec.pts << ","
            << rec.dts << ","
            << rec.flags << ","
            << (rec.isNonRef ? 1 : 0) << ","
            << static_cast<int>(rec.mode) << ","
            << (rec.isPreDropped ? 1 : 0) << ","
            << (rec.isPostDropped ? 1 : 0) << ","
            << rec.preSysTimeMs << ","
            << rec.postSysTimeMs << ","
            << rec.preDropCount << ","
            << rec.retentionRate << ","
            << rec.speed << "\n";
    }
    ofs.close();
    AVCODEC_LOGI("Successfully dumped %{public}zu records to %{public}s", records_.size(), filename.c_str());
    records_.clear();
}
} // namespace SFD
} // namespace MediaAVCodec
} // namespace OHOS
