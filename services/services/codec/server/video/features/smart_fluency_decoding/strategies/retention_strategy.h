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

#ifndef RETENTION_STRATEGY_H
#define RETENTION_STRATEGY_H

#include <memory>
#include "smart_fluency_decoding_types.h"

namespace OHOS {
namespace MediaAVCodec {
namespace SFD {

struct FrameRetentionContext {
    uint32_t preDroppedCount = 0;
    const MVStats* mvStats = nullptr;
};

class IRetentionStrategy {
public:
    virtual ~IRetentionStrategy() = default;

    virtual bool MakeRetentionDecision(const FrameRetentionContext& ctx) = 0;
    virtual void UpdatePlaybackSpeed(double /* speed */) {}
    virtual void UpdateSourceFramerate(double /*sourceFps*/) {}
    virtual void Reset() {}
    virtual RetentionStrategyType GetMode() const = 0;
    virtual double GetRatio() const = 0;
    virtual SfdDecisionReason GetLastDecisionReason() const { return SfdDecisionReason::UNSPECIFIED; }

    virtual bool IsPassthrough() const { return false; }
    virtual bool RequiresMvAnalysis() const { return false; }
    virtual bool RequiresNaluAnalysis() const { return true; }
};

} // namespace SFD
} // namespace MediaAVCodec
} // namespace OHOS

#endif // RETENTION_STRATEGY_H