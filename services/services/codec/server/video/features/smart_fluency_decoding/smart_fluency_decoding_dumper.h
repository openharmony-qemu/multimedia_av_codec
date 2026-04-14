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
    bool isPreDropped;
    bool isPostDropped;
    int64_t preSysTimeMs;
    int64_t postSysTimeMs;
    int32_t preDropCount;
    RetentionStrategyType mode;
    double retentionRate;
    double speed;

    FrameDumpRecord() : pts(-1), dts(-1), flags(0), isNonRef(false), isPreDropped(false), isPostDropped(false),
                        preSysTimeMs(-1), postSysTimeMs(-1), preDropCount(0),
                        mode(RetentionStrategyType::FULL), retentionRate(1.0), speed(1.0) {}
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
        bool dropped{false};
        int64_t sysTimeMs{-1};
    };

    struct PostDecodeRecordCtx {
        int64_t pts{-1};
        bool dropped{false};
        int32_t preDropCount{0};
        int64_t sysTimeMs{-1};
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
};
} // namespace SFD
} // namespace MediaAVCodec
} // namespace OHOS
#endif // SMART_FLUENCY_DECODING_DUMPER_H
