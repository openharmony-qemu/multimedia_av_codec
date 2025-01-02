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

#ifndef AVCODEC_INSTANCE_INFO_H
#define AVCODEC_INSTANCE_INFO_H

#include <sys/types.h>
#include <string>
#include "avcodec_log.h"

namespace OHOS {
namespace MediaAVCodec {
constexpr pid_t INVALID_PID = -1;
struct CallerInfo {
    pid_t pid = -1;
    uid_t uid = 0;
    std::string processName = "";
};

constexpr int32_t INVALID_INSTANCE_ID = -1;
struct InstanceInfo {
    int32_t instanceId = INVALID_INSTANCE_ID;
    CallerInfo caller;
    CallerInfo forwardCaller;
    uint64_t memoryUsage = 0;

    void Print()
    {
        constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "InstanceInfo"};
        AVCODEC_LOGI("InstanceId: %{public}d, Caller: [%{public}d, %{public}s], ForwardCaller: [%{public}d, %{public}s]"
            ", MemoryUsage: %{public}" PRIu64, instanceId, caller.pid, caller.processName.c_str(), forwardCaller.pid,
            forwardCaller.processName.c_str(), memoryUsage);
    }
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // AVCODEC_INSTANCE_INFO_H