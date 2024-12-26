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
#include <mutex>
#include "avcodec_xcollie.h"
#include "meta.h"
#include "instance_info.h"

namespace OHOS {
namespace MediaAVCodec {
class InstanceMemoryUpdateEventHandler {
public:
    static InstanceMemoryUpdateEventHandler &GetInstance();
    void OnInstanceMemoryUpdate(const Media::Meta &meta);
    void OnInstanceRelease(const Media::Meta &meta);
    void RemoveTimer(pid_t pid);

private:
    std::optional<std::function<uint64_t(uint32_t)>> GetCalculator(const Media::Meta &meta);
    uint32_t GetBlockCount(const Media::Meta &meta);
    pid_t GetActualPidByInstanceInfo(const InstanceInfo &info);
    std::optional<InstanceInfo> UpdateInstanceMemory(int32_t instanceId, uint64_t memory);

    void UpdateAppMemoryThreshold();
    static uint64_t GetAppMemory(pid_t pid);
    static void UploadAppMemory(pid_t pid);
    void DeterminAppMemoryLeak(pid_t pid);

    uint32_t appMemoryThreshold_ = 0;
    std::mutex timerMutex_;
    std::unordered_map<pid_t, AVCodecXcollieTimer> timerMap_;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // AVCODEC_INSTANCE_MEMORY_UPDATE_EVENT_HANDLER_H