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
#include <fstream>
#include <cmath>
#include "cJSON.h"
#include "client/memory_collector_client.h"
#include "parameters.h"
#include "avcodec_server_manager.h"
#include "avcodec_log.h"
#include "avcodec_errors.h"
#include "meta/meta_key.h"
#include "event_info_extended_key.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "InstanceMemoryUpdateEventHandler"};
constexpr int32_t MEMORY_LEAK_UPLOAD_TIMEOUT = 180; // seconds
constexpr uint32_t APP_MEMORY_THRESHOLD_MIN = 524'288; // 524288KB, 512MB
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
    auto instanceId = GetInstanceIdFromMeta(meta);
    CHECK_AND_RETURN_LOG(instanceId != INVALID_INSTANCE_ID, "Can not find instance id");

    auto calculator = GetCalculator(meta);
    CHECK_AND_RETURN_LOG(calculator != std::nullopt, "Can not find a calculator");
    auto instanceMemory = calculator.value()(GetBlockCount(meta));

    auto instanceInfo = UpdateInstanceMemory(instanceId, instanceMemory);
    CHECK_AND_RETURN_LOG(instanceInfo != std::nullopt, "Update instance memory failed");

    DeterminAppMemoryLeak(instanceInfo.value().caller.pid, instanceInfo.value().forwardCaller.pid);
}

void InstanceMemoryUpdateEventHandler::OnInstanceRelease(const Media::Meta &meta)
{
    auto instanceId = GetInstanceIdFromMeta(meta);
    auto instanceInfo = AVCodecServerManager::GetInstance().GetInstanceInfoByInstanceId(instanceId);
    DeterminAppMemoryLeak(instanceInfo.value().caller.pid, instanceInfo.value().forwardCaller.pid);
}

void InstanceMemoryUpdateEventHandler::RemoveTimer(pid_t pid)
{
    std::lock_guard<std::mutex> lock(timerMutex_);
    timerMap_.erase(pid);
}

std::optional<std::function<uint32_t(uint32_t)>>
InstanceMemoryUpdateEventHandler::GetCalculator(const Media::Meta &meta)
{
    (void)meta;
    return std::optional<std::function<uint32_t(uint32_t)>>();
}

uint32_t InstanceMemoryUpdateEventHandler::GetBlockCount(const Media::Meta &meta)
{
    int32_t width = 0;
    int32_t length = 0;
    meta.GetData(Media::Tag::VIDEO_WIDTH, width);
    meta.GetData(Media::Tag::VIDEO_WIDTH, length);
    constexpr int32_t blockWidth = 16;
    constexpr int32_t blockLength = 16;
    return std::ceil(width / blockWidth) * std::ceil(length / blockLength);
}

std::optional<InstanceInfo> InstanceMemoryUpdateEventHandler::UpdateInstanceMemory(int32_t instanceId, uint32_t memory)
{
    auto instanceInfo = AVCodecServerManager::GetInstance().GetInstanceInfoByInstanceId(instanceId);
    CHECK_AND_RETURN_RET_LOG(instanceInfo != std::nullopt,
        std::nullopt, "Can not find this instance, id: %{public}d", instanceId);

    instanceInfo.value().memoryUsage = memory;
    AVCodecServerManager::GetInstance().SetInstanceInfoByInstanceId(instanceId, instanceInfo.value());
    AVCODEC_LOGI("The memory usage of instance[%{public}d] has been updated to %{public}u", instanceId, memory);

    return instanceInfo;
}

void InstanceMemoryUpdateEventHandler::UpdateAppMemoryThreshold()
{
    if (appMemoryThreshold_ != 0) {
        return;
    }
    auto threshold = ThresholdParser::GetThreshold();
    appMemoryThreshold_ = threshold > APP_MEMORY_THRESHOLD_MIN ? threshold : APP_MEMORY_THRESHOLD_MIN;
    AVCODEC_LOGI("App memory threshold updated to %{public}u KB", appMemoryThreshold_);
}

uint32_t InstanceMemoryUpdateEventHandler::GetAppMemory(pid_t callerPid, pid_t forwardCallerPid)
{
    auto instanceInfoList = AVCodecServerManager::GetInstance().GetInstanceInfoListByPid(callerPid);
    uint32_t appMemoryUsage = 0;
    for (const auto &info : instanceInfoList) {
        if (forwardCallerPid != info.second.forwardCaller.pid) {
            continue;
        }
        appMemoryUsage += info.second.memoryUsage;
    }
    return appMemoryUsage;
}

void InstanceMemoryUpdateEventHandler::UploadAppMemory(pid_t callerPid, pid_t forwardCallerPid)
{
    InstanceMemoryUpdateEventHandler::GetInstance().RemoveTimer(forwardCallerPid);

    auto memoryCollector = HiviewDFX::UCollectClient::MemoryCollector::Create();
    CHECK_AND_RETURN_LOG(memoryCollector != nullptr, "Create Hiview DFX memory collector failed");

    auto memory = GetAppMemory(callerPid, forwardCallerPid);
    std::vector<HiviewDFX::UCollectClient::MemoryCaller> memList;
    HiviewDFX::UCollectClient::MemoryCaller memoryCaller = {
        .pid = forwardCallerPid,
        .resourceType = "AVCodec",
        .limitValue = memory,
    };
    memList.emplace_back(memoryCaller);
    memoryCollector->SetSplitMemoryValue(memList);
    AVCODEC_LOGI("The memory usage of pid %{public}d is %{public}u", forwardCallerPid, memory);
}

void InstanceMemoryUpdateEventHandler::DeterminAppMemoryLeak(pid_t callerPid, pid_t forwardCallerPid)
{
    UpdateAppMemoryThreshold();

    auto memory = GetAppMemory(callerPid, forwardCallerPid);
    if (memory > appMemoryThreshold_ && timerMap_.count(forwardCallerPid) == 0) {
        auto timeName = std::string("Pid_") + std::to_string(forwardCallerPid) + " memory leak";
        AVCodecXcollieTimer timer(timeName, false, MEMORY_LEAK_UPLOAD_TIMEOUT,
            [=](void *) -> void { UploadAppMemory(callerPid, forwardCallerPid); });

        std::lock_guard<std::mutex> lock(timerMutex_);
        timerMap_.emplace(forwardCallerPid, timer);
    } else if (memory <= appMemoryThreshold_ && timerMap_.count(forwardCallerPid) != 0) {
        InstanceMemoryUpdateEventHandler::GetInstance().RemoveTimer(forwardCallerPid);
    }
}

uint32_t InstanceMemoryUpdateEventHandler::ThresholdParser::GetThreshold()
{
    std::ifstream thresholdConfigFile("/system/etc/hiview/kernel_leak_config.json");
    CHECK_AND_RETURN_RET_LOG(thresholdConfigFile.is_open(), UINT32_MAX, "Can not open threshold config json file");

    std::string line;
    std::string configJson;
    while (thresholdConfigFile >> line) {
        configJson += line;
    }
    std::shared_ptr<cJSON> root = std::shared_ptr<cJSON>(cJSON_Parse(configJson.c_str()), cJSON_Delete);
    CHECK_AND_RETURN_RET_LOG(root != nullptr, UINT32_MAX, "Can not parse threshold config json");

    std::string deviceType = OHOS::system::GetParameter("cosnt.product.devicetype", "unknown");
    auto value = cJSON_GetObjectItem(root.get(), "av_codec_config");
    CHECK_AND_RETURN_RET_LOG(value != nullptr && cJSON_IsNumber(value),
        UINT32_MAX, "Can not find av_codec_config from threshold config json");
    return value->valueint;
}
} // namespace MediaAVCodec
} // namespace OHOS