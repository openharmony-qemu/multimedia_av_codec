/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef BACKGROUND_EVENT_HANDLER_H
#define BACKGROUND_EVENT_HANDLER_H

#include <mutex>
#include <unordered_map>
#include <vector>
#include "instance_info.h"
#include "refbase.h"

namespace OHOS {
namespace MediaAVCodec {
using ObjectList = std::unordered_multimap<pid_t, InstanceId>;

class BackGroundEventHandler {
public:
    static BackGroundEventHandler &GetInstance();
    void NotifyFreeze(InstanceId instanceId);
    void NotifyFreeze(const std::vector<pid_t> &pidList);
    void NotifyActive(InstanceId instanceId);
    void NotifyActive(const std::vector<pid_t> &pidList);
    void NotifyActiveAll();
    void ErasePid(pid_t pid);
    void EraseInstance(InstanceId instanceId);

private:
    BackGroundEventHandler() {};
    ~BackGroundEventHandler() = default;

    std::recursive_mutex mutex_;
    ObjectList memoryRecycleList_;
    ObjectList suspendList_;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // BACKGROUND_EVENT_HANDLER_H