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

#ifndef SMART_FLUENCY_DECODING_DUMPER_H
#define SMART_FLUENCY_DECODING_DUMPER_H

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>
#include "smart_fluency_decoding_types.h"

namespace OHOS {
namespace MediaAVCodec {
namespace SFD {
struct FrameDumpRecord {
    int64_t pts;
    int64_t dts;
    uint32_t flags;
    bool isNonRef;
    int32_t isPreRetained;
    int32_t isPostRetained;
    int64_t preSysTimeMs;
    int64_t postSysTimeMs;
    int32_t preDropCount;
    RetentionStrategyType mode;
    double retentionRate;
    double speed;
    SfdDecisionReason preReason{SfdDecisionReason::UNSPECIFIED};
    SfdDecisionReason postReason{SfdDecisionReason::POST_BYPASSED};

    // --- 新增的 Dump 字段 ---
    int64_t preDecCostUs;         // PreDecode 接口耗时 (微秒)
    int64_t postDecCostUs;        // PostDecode 接口耗时 (微秒)
    size_t writeIndex;            // 同步队列的写入位置
    size_t readIndex;             // 同步队列的读取位置
    double currentEmaDropRatio;   // 当前平滑丢帧率
    uint32_t validBlocks;         // MV有效宏块数
    double perceptualMagnitude;   // MV感知运动幅度
    double motionConsistency;     // MV运动一致性
    double zeroMotionRatio;       // 零运动宏块比例

    // --- 新增 NALU 耗时字段 ---
    int64_t naluCostUs;

    // --- 新增 MV 细分耗时 ---
    int64_t copyMvCostUs;
    int64_t analyzeMvCostUs;
    int64_t updateDumperCostUs;

    FrameDumpRecord() : pts(-1), dts(-1), flags(0), isNonRef(false), isPreRetained(1), isPostRetained(1),
                        preSysTimeMs(-1), postSysTimeMs(-1), preDropCount(0),
                        mode(RetentionStrategyType::FULL), retentionRate(1.0), speed(1.0),
                        preDecCostUs(-1), postDecCostUs(-1), writeIndex(0), readIndex(0),
                        currentEmaDropRatio(0.0), validBlocks(0), perceptualMagnitude(0.0),
                        motionConsistency(0.0), zeroMotionRatio(0.0), naluCostUs(-1),
                        copyMvCostUs(-1), analyzeMvCostUs(-1), updateDumperCostUs(-1) {}
};

class SmartFluencyDecodingDumper {
public:
    static std::unique_ptr<SmartFluencyDecodingDumper> Create(
        SFDCodecType codecType, int32_t width, int32_t height, RetentionStrategyType mode);
    ~SmartFluencyDecodingDumper();

    void UpdateMode(RetentionStrategyType mode, double retentionRate);
    void UpdateSpeed(double speed);
    void UpdateCtuSize(int32_t ctuSize);

    struct PreDecodeRecordCtx {
        int64_t pts{-1};
        int64_t dts{-1};
        uint32_t flags{0};
        bool isNonRef{false};
        bool isRetained{true};
        int64_t sysTimeMs{-1};
        int64_t preDecCostUs{-1};
        size_t writeIndex{0};
        double currentEmaDropRatio{0.0};
        // --- 新增 ---
        int64_t naluCostUs{-1};
        SfdDecisionReason reason{SfdDecisionReason::UNSPECIFIED};
    };

    struct PostDecodeRecordCtx {
        int64_t pts{-1};
        bool isRetained{true};
        int32_t preDropCount{0};
        int64_t sysTimeMs{-1};
        int64_t postDecCostUs{-1};
        size_t readIndex{0};
        double currentEmaDropRatio{0.0};
        MVStats mvStats;
        // --- 新增细分耗时 ---
        int64_t copyMvCostUs{-1};
        int64_t analyzeMvCostUs{-1};
        int64_t updateDumperCostUs{-1};
        SfdDecisionReason reason{SfdDecisionReason::UNSPECIFIED};
    };

    void RecordPreDecode(const PreDecodeRecordCtx& ctx);
    void RecordPostDecode(const PostDecodeRecordCtx& ctx);
    void DumpToFile();

private:
    SmartFluencyDecodingDumper(SFDCodecType codecType, int32_t width, int32_t height, RetentionStrategyType mode);
    static bool IsDirectoryExist(const std::string& path);
    std::string GetTimestampString();
    void DumpToFileInternal();

    std::mutex mutex_;
    std::vector<FrameDumpRecord> records_;
    SFDCodecType codecType_{SFDCodecType::INVALID};
    int32_t width_{0};
    int32_t height_{0};
    RetentionStrategyType currentMode_{RetentionStrategyType::FULL};
    double currentRetentionRate_{1.0};
    double currentSpeed_{1.0};
    int32_t currentCtuSize_{0};

    // --- 新增分段追加控制变量 ---
    std::string currentFilename_;
    bool isHeaderWritten_{false};
};
} // namespace SFD
} // namespace MediaAVCodec
} // namespace OHOS
#endif // SMART_FLUENCY_DECODING_DUMPER_H
