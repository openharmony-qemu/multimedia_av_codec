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
#include "avcodec_server_manager.h"
#include "avcodec_log.h"
#include "avcodec_errors.h"
#include "event_info_extended_key.h"


namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "InstanceMemoryUpdateEventHandler"};
constexpr int32_t MEMORY_LEAK_UPLOAD_TIMEOUT = 180; // seconds
} // namespace

namespace OHOS {
namespace MediaAVCodec {
InstanceMemoryUpdateEventHandler &InstanceMemoryUpdateEventHandler::GetInstance()
{
    static InstanceMemoryUpdateEventHandler handler;
    return handler;
}

void InstanceMemoryUpdateEventHandler::OnInstanceMemoryUpdate(const Media::Meta &meta)
{
    auto instanceId = INVALID_INSTANCE_ID;
    auto instanceIdExist = meta.GetData(EventInfoExtentedKey::INSTANCE_ID.data(), instanceId);
    CHECK_AND_RETURN_LOG(instanceIdExist == true && instanceId != INVALID_INSTANCE_ID, "Can not find instance id");

    auto calculator = GetCalculator(meta);
    CHECK_AND_RETURN_LOG(calculator != std::nullopt, "Can not find a calculator");
    auto instanceMemory = calculator.value()(GetBlockCount(meta));

    auto instanceInfo = UpdateInstanceMemory(instanceId, instanceMemory);
    CHECK_AND_RETURN_LOG(instanceInfo != std::nullopt, "Update instance memory failed");

    DeterminAppMemoryLeak(GetActualPidByInstanceInfo(instanceInfo.value()));
}

void InstanceMemoryUpdateEventHandler::OnInstanceRelease(const Media::Meta &meta)
{
    (void)meta;
}

void InstanceMemoryUpdateEventHandler::RemoveTimer(pid_t pid)
{
    std::lock_guard<std::mutex> lock(timerMutex_);
    timerMap_.erase(pid);
}

std::optional<std::function<uint64_t(uint32_t)>> InstanceMemoryUpdateEventHandler::GetCalculator(const Media::Meta &meta)
{
    (void)meta;
    return std::optional<std::function<uint64_t(uint32_t)>>();
}

uint32_t InstanceMemoryUpdateEventHandler::GetBlockCount(const Media::Meta &meta)
{
    (void)meta;
    return 0;
}

pid_t InstanceMemoryUpdateEventHandler::GetActualPidByInstanceInfo(const InstanceInfo &info)
{
    return info.forwardCaller.pid != INVALID_PID ? info.forwardCaller.pid : info.caller.pid;
}

std::optional<InstanceInfo> InstanceMemoryUpdateEventHandler::UpdateInstanceMemory(int32_t instanceId, uint64_t memory)
{
    auto instanceInfo = AVCodecServerManager::GetInstance().GetInstanceInfoByInstanceId(instanceId);
    CHECK_AND_RETURN_RET_LOG(instanceInfo != std::nullopt,
        std::nullopt, "Can not find this instance, id: %{public}d", instanceId);

    instanceInfo.value().memoryUsage = memory;
    AVCodecServerManager::GetInstance().SetInstanceInfoByInstanceId(instanceId, instanceInfo.value());
    AVCODEC_LOGI("The memory usage of instance %{public}d has been updated to %{public}" PRIu64, instanceId, memory);

    return instanceInfo;
}

void InstanceMemoryUpdateEventHandler::UpdateAppMemoryThreshold()
{
    // if (appMemoryThreshold_ == INVALID) {
        (void)appMemoryThreshold_;
    // }
}

uint64_t InstanceMemoryUpdateEventHandler::GetAppMemory(pid_t pid)
{
    return 0;
}

void InstanceMemoryUpdateEventHandler::UploadAppMemory(pid_t pid)
{
    auto memory = GetAppMemory(pid);
    (void)memory;


    InstanceMemoryUpdateEventHandler::GetInstance().RemoveTimer(pid);
}

void InstanceMemoryUpdateEventHandler::DeterminAppMemoryLeak(pid_t pid)
{
    UpdateAppMemoryThreshold();

    auto memory = GetAppMemory(pid);
    if (memory > appMemoryThreshold_ && timerMap_.count(pid) == 0) {
        using namespace std::string_literals;
        auto timeName = "Pid_"s + std::to_string(pid) + " memory leak";
        AVCodecXcollieTimer timer(timeName, false, MEMORY_LEAK_UPLOAD_TIMEOUT,
            [=](void *) -> void { UploadAppMemory(pid); });

        std::lock_guard<std::mutex> lock(timerMutex_);
        timerMap_.emplace(pid, timer);
    } else if (memory <= appMemoryThreshold_ && timerMap_.count(pid) != 0) {
        InstanceMemoryUpdateEventHandler::GetInstance().RemoveTimer(pid);
    }
}
} // namespace MediaAVCodec
} // namespace OHOS