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

#include "instance_memory_update_event_handler.h"

namespace OHOS {
namespace MediaAVCodec {
void InstanceMemoryUpdateEventHandler::OnInstanceMemoryUpdate(const Media::Meta &meta)
{
    (void)meta;
}

void InstanceMemoryUpdateEventHandler::OnInstanceRelease(const Media::Meta &meta)
{
    (void)meta;
}

void InstanceMemoryUpdateEventHandler::UpdateAppMemoryThreshold()
{
    (void)appMemoryThreshold_;
}

std::optional<std::function<uint32_t(uint32_t)>> InstanceMemoryUpdateEventHandler::GetCalculator(const Media::Meta &meta)
{
    (void)meta;
    (void)meta;
    return std::optional<std::function<uint32_t(uint32_t)>>();
}

uint32_t InstanceMemoryUpdateEventHandler::GetBlockCount(const Media::Meta &meta)
{
    (void)meta;
    return 0;
}

int32_t InstanceMemoryUpdateEventHandler::UpdateInstanceMemory(uint32_t instanceId, uint32_t memory)
{
    (void)instanceId;
    (void)memory;
    return 0;
}

int32_t InstanceMemoryUpdateEventHandler::DeleteInstanceMemory(sptr<IRemoteObject> &object)
{
    (void)object;
    return 0;
}

int32_t InstanceMemoryUpdateEventHandler::DeterminAppMemoryLeak(pid_t pid)
{
    (void)pid;
    return 0;
}

void InstanceMemoryUpdateEventHandler::UploadAppMemory(pid_t pid, uint32_t memory)
{
    (void)pid;
    (void)memory;
}
} // namespace MediaAVCodec
} // namespace OHOS