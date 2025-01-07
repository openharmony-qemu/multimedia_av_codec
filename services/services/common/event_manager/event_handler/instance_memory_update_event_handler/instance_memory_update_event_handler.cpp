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

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "InstanceMemoryUpdateEventHandler"};
constexpr int32_t MEMORY_LEAK_UPLOAD_TIMEOUT = 180; // seconds
constexpr uint32_t APP_MEMORY_THRESHOLD_MIN = 262'144; // 262144, 256MB
constexpr uint32_t THRESHOLD_CONFIG_FILE_SIZE_MAX = 1024'576; // 1024576, 1M
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
    auto instanceId = EventInfoExtentedKey::GetInstanceIdFromMeta(meta);
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
    auto callerPid = INVALID_PID;
    auto forwardCallerPid = INVALID_PID;
    meta.GetData(Media::Tag::AV_CODEC_CALLER_PID, callerPid);
    meta.GetData(Media::Tag::AV_CODEC_FORWARD_CALLER_PID, forwardCallerPid);
    DeterminAppMemoryLeak(callerPid, forwardCallerPid);
}

void InstanceMemoryUpdateEventHandler::RemoveTimer(pid_t pid)
{
    std::lock_guard<std::mutex> lock(timerMutex_);
    timerMap_.erase(pid);
    AVCODEC_LOGI("Timer for pid %{public}d has been removed", pid);
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
    AVCODEC_LOGD("The memory usage of instance[%{public}d] has been updated to %{public}u", instanceId, memory);

    return instanceInfo;
}

void InstanceMemoryUpdateEventHandler::UpdateAppMemoryThreshold()
{
    if (appMemoryThreshold_ != 0) {
        return;
    }
    auto threshold = ThresholdParser::GetThreshold();
    appMemoryThreshold_ = threshold > APP_MEMORY_THRESHOLD_MIN ? threshold : APP_MEMORY_THRESHOLD_MIN;
}

uint32_t InstanceMemoryUpdateEventHandler::GetAppMemory(pid_t callerPid, pid_t actualCallerPid)
{
    auto instanceInfoList = AVCodecServerManager::GetInstance().GetInstanceInfoListByPid(callerPid);
    uint32_t appMemoryUsage = 0;
    for (const auto &info : instanceInfoList) {
        if (actualCallerPid != callerPid && actualCallerPid != info.second.forwardCaller.pid) {
            continue;
        }
        appMemoryUsage += info.second.memoryUsage;
    }
    return appMemoryUsage;
}

void InstanceMemoryUpdateEventHandler::ReportAppMemory(pid_t callerPid, pid_t actualCallerPid)
{
    InstanceMemoryUpdateEventHandler::GetInstance().RemoveTimer(actualCallerPid);

    auto memoryCollector = HiviewDFX::UCollectClient::MemoryCollector::Create();
    CHECK_AND_RETURN_LOG(memoryCollector != nullptr, "Create Hiview DFX memory collector failed");

    auto memory = GetAppMemory(callerPid, actualCallerPid);
    std::vector<HiviewDFX::UCollectClient::MemoryCaller> memList;
    HiviewDFX::UCollectClient::MemoryCaller memoryCaller = {
        .pid = actualCallerPid,
        .resourceType = "AVCodec",
        .limitValue = memory,
    };
    memList.emplace_back(memoryCaller);
    // memoryCollector->SetSplitMemoryValue(memList);
    AVCODEC_LOGI("The memory usage of pid %{public}d is %{public}u KB, report to hivew", actualCallerPid, memory);
}

void InstanceMemoryUpdateEventHandler::DeterminAppMemoryLeak(pid_t callerPid, pid_t forwardCallerPid)
{
    UpdateAppMemoryThreshold();

    auto actualCallerPid = forwardCallerPid == INVALID_PID ? callerPid : forwardCallerPid;
    auto memory = GetAppMemory(callerPid, actualCallerPid);
    if (memory > appMemoryThreshold_ && timerMap_.count(actualCallerPid) == 0) {
        auto timeName = std::string("Pid_") + std::to_string(actualCallerPid) + " memory leak";
        auto timer = std::make_shared<AVCodecXcollieTimer>(timeName, false, MEMORY_LEAK_UPLOAD_TIMEOUT,
            [=](void *) -> void { ReportAppMemory(callerPid, actualCallerPid); });

        std::lock_guard<std::mutex> lock(timerMutex_);
        timerMap_.emplace(actualCallerPid, timer);
        AVCODEC_LOGI("Determined pid %{public}d memory leak(%{public}u KB), event timer added",
            actualCallerPid, memory);
    } else if (memory <= appMemoryThreshold_ && timerMap_.count(actualCallerPid) != 0) {
        InstanceMemoryUpdateEventHandler::GetInstance().RemoveTimer(actualCallerPid);
    }
}

uint32_t InstanceMemoryUpdateEventHandler::ThresholdParser::GetThreshold()
{
    std::ifstream thresholdConfigFile("/system/etc/hiview/native_leak_config.json");
    CHECK_AND_RETURN_RET_LOG(thresholdConfigFile.is_open(), UINT32_MAX, "Can not open threshold config json file");

    std::string line;
    std::string configJson;
    while (thresholdConfigFile >> line && configJson.size() < THRESHOLD_CONFIG_FILE_SIZE_MAX) {
        configJson += line;
    }
    auto root = std::shared_ptr<cJSON>(cJSON_Parse(configJson.c_str()), cJSON_Delete);
    CHECK_AND_RETURN_RET_LOG(root != nullptr, UINT32_MAX, "Can not parse threshold config json");

    auto deviceType = system::GetDeviceType();
    CHECK_AND_RETURN_RET_LOG(deviceType != "" && deviceType != "unknown", UINT32_MAX, "Can not get device type");

    auto avcodecConfigItem = cJSON_GetObjectItem(root.get(), "av_codec_config");
    CHECK_AND_RETURN_RET_LOG(avcodecConfigItem != nullptr,
        UINT32_MAX, "Can not find av_codec_config from threshold config json");

    auto thresholdItem = cJSON_GetObjectItem(avcodecConfigItem, deviceType.c_str());
    CHECK_AND_RETURN_RET_LOG(thresholdItem != nullptr && cJSON_IsNumber(thresholdItem),
        UINT32_MAX, "Can not find threshold of %{public}s from av_codec_config", deviceType.c_str());

    AVCODEC_LOGI("Got threshold of %{public}s, %{public}u KB", deviceType.c_str(), thresholdItem->valueint);
    return thresholdItem->valueint;
}
} // namespace MediaAVCodec
} // namespace OHOS