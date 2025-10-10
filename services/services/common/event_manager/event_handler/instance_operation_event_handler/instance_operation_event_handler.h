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

#ifndef INSTANCE_OPERATION_EVENT_HANDLER_H
#define INSTANCE_OPERATION_EVENT_HANDLER_H

#include <mutex>
#include <set>
#include <unordered_map>
#include "meta.h"
#include "refbase.h"

namespace OHOS {
namespace MediaAVCodec {
class InstanceOperationEventHandler {
public:
    static InstanceOperationEventHandler &GetInstance();
    void OnInstanceEncodeBegin(Media::Meta &meta);
    void OnInstanceEncodeEnd(Media::Meta &meta);

private:
    InstanceOperationEventHandler() {};
    ~InstanceOperationEventHandler() = default;

    std::recursive_mutex mutex_;
    std::unordered_map<pid_t, std::set<int32_t>> encodeExecutingMap_;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // INSTANCE_OPERATION_EVENT_HANDLER_H