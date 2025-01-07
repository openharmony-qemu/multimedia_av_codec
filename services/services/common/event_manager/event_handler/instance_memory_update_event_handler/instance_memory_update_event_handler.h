/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef AVCODEC_INSTANCE_MEMORY_UPDATE_EVENT_HANDLER_H
#define AVCODEC_INSTANCE_MEMORY_UPDATE_EVENT_HANDLER_H

#include <optional>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <memory>
#include "avcodec_xcollie.h"
#include "meta.h"
#include "instance_info.h"

namespace OHOS {
namespace MediaAVCodec {
using CalculatorType = std::function<uint32_t(uint32_t)>;
class InstanceMemoryUpdateEventHandler {
public:
    static InstanceMemoryUpdateEventHandler &GetInstance();
    void OnInstanceMemoryUpdate(const Media::Meta &meta);
    void OnInstanceRelease(const Media::Meta &meta);
    void RemoveTimer(pid_t pid);

private:
    std::optional<CalculatorType> GetCalculator(const Media::Meta &meta);

    uint32_t GetBlockCount(const Media::Meta &meta);
    std::optional<InstanceInfo> UpdateInstanceMemory(int32_t instanceId, uint32_t memory);

    void UpdateAppMemoryThreshold();
    static uint32_t GetAppMemory(pid_t callerPid, pid_t actualCallerPid);
    static void ReportAppMemory(pid_t callerPid, pid_t actualCallerPid);
    void DeterminAppMemoryLeak(pid_t callerPid, pid_t forwardCallerPid);

    uint32_t appMemoryThreshold_ = 0;
    std::mutex timerMutex_;
    std::mutex appMemoryLeakListMutex_;
    std::unordered_map<pid_t, std::shared_ptr<AVCodecXcollieTimer>> timerMap_;
    std::unordered_set<pid_t> appMemoryLeakList_;

private:
    class ThresholdParser {
    public:
        static uint32_t GetThreshold();
    };
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // AVCODEC_INSTANCE_MEMORY_UPDATE_EVENT_HANDLER_H