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

#include "instance_memory_update_event_handler.h"
#include <fstream>
#include <cmath>
#include "cJSON.h"
#include "client/memory_collector_client.h"
#include "parameters.h"
#include "avcodec_info.h"
#include "avcodec_server_manager.h"
#include "avcodec_log.h"
#include "avcodec_errors.h"
#include "meta/meta_key.h"
#include "config_policy_utils.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "InstanceMemoryUpdateEventHandler"};
constexpr int32_t MEMORY_LEAK_UPLOAD_TIMEOUT = 180; // seconds
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
    if (appMemoryThreshold_ == UINT32_MAX) {
        return;
    }
    auto instanceId = EventInfoExtentedKey::GetInstanceIdFromMeta(meta);
    CHECK_AND_RETURN_LOG(instanceId != INVALID_INSTANCE_ID, "Can not find instance id");

    auto calculator = GetCalculator(meta);
    CHECK_AND_RETURN_LOG(calculator != std::nullopt, "Can not find a calculator");
    auto instanceMemory = calculator.value()(GetBlockCount(meta));

    auto instanceInfo = UpdateInstanceMemory(instanceId, instanceMemory);
    if (instanceInfo == std::nullopt) {
        return;
    }

    DeterminAppMemoryExceedThresholdAndReport(instanceInfo.value().caller.pid, instanceInfo.value().forwardCaller.pid);
}

void InstanceMemoryUpdateEventHandler::OnInstanceMemoryReset(const Media::Meta &meta)
{
    if (appMemoryThreshold_ == UINT32_MAX) {
        return;
    }
    auto instanceId = EventInfoExtentedKey::GetInstanceIdFromMeta(meta);
    CHECK_AND_RETURN_LOG(instanceId != INVALID_INSTANCE_ID, "Can not find instance id");
    
    auto instanceInfo = UpdateInstanceMemory(instanceId, 0);
    if (instanceInfo == std::nullopt) {
        return;
    }

    DeterminAppMemoryExceedThresholdAndReport(instanceInfo.value().caller.pid, instanceInfo.value().forwardCaller.pid);
}

void InstanceMemoryUpdateEventHandler::OnInstanceRelease(const Media::Meta &meta)
{
    if (appMemoryThreshold_ == UINT32_MAX) {
        return;
    }
    auto callerPid = INVALID_PID;
    auto forwardCallerPid = INVALID_PID;
    meta.GetData(Media::Tag::AV_CODEC_CALLER_PID, callerPid);
    meta.GetData(Media::Tag::AV_CODEC_FORWARD_CALLER_PID, forwardCallerPid);
    DeterminAppMemoryExceedThresholdAndReport(callerPid, forwardCallerPid);
}

void InstanceMemoryUpdateEventHandler::RemoveTimer(pid_t pid)
{
    std::lock_guard<std::shared_mutex> lock(timerMutex_);
    auto num = timerMap_.erase(pid);
    EXPECT_AND_LOGI(num > 0, "Timer for pid %{public}d has been removed", pid);
}

void InstanceMemoryUpdateEventHandler::AddApp2ExceedThresholdList(pid_t pid)
{
    std::lock_guard<std::shared_mutex> lock(appMemoryExceedThresholdListMutex_);
    appMemoryExceedThresholdList_.emplace(pid);
    AVCODEC_LOGI("Pid: %{public}d has been added to exceeded threshold list", pid);
}

InstanceMemoryUpdateEventHandler::InstanceMemoryUpdateEventHandler()
{
    UpdateAppMemoryThreshold();
}

uint32_t InstanceMemoryUpdateEventHandler::GetBlockCount(const Media::Meta &meta)
{
    int32_t width = 0;
    int32_t height = 0;
    AVCodecType codecType = AVCODEC_TYPE_VIDEO_DECODER;
    meta.GetData(EventInfoExtentedKey::CODEC_TYPE.data(), codecType);
    if (codecType == AVCODEC_TYPE_VIDEO_DECODER) {
        meta.GetData(Media::Tag::VIDEO_PIC_WIDTH, width);
        meta.GetData(Media::Tag::VIDEO_PIC_HEIGHT, height);
    } else {
        meta.GetData(Media::Tag::VIDEO_WIDTH, width);
        meta.GetData(Media::Tag::VIDEO_HEIGHT, height);
    }
    constexpr int32_t blockWidth = 16;
    constexpr int32_t blockHeight = 16;
    return std::ceil(width / blockWidth) * std::ceil(height / blockHeight);
}

std::optional<InstanceInfo> InstanceMemoryUpdateEventHandler::UpdateInstanceMemory(int32_t instanceId, uint32_t memory)
{
    auto instanceInfo = AVCodecServerManager::GetInstance().GetInstanceInfoByInstanceId(instanceId);
    CHECK_AND_RETURN_RET_LOG(instanceInfo != std::nullopt,
        std::nullopt, "Can not find this instance, id: %{public}d", instanceId);

    if (instanceInfo.value().memoryUsage == memory) {
        return std::nullopt;
    }
    instanceInfo.value().memoryUsage = memory;
    AVCodecServerManager::GetInstance().SetInstanceInfoByInstanceId(instanceId, instanceInfo.value());
    AVCODEC_LOGD("The memory usage of instance[%{public}d] has been updated to %{public}u", instanceId, memory);

    return instanceInfo;
}

void InstanceMemoryUpdateEventHandler::UpdateAppMemoryThreshold()
{
    appMemoryThreshold_ = ThresholdParser::GetThreshold();
}

uint32_t InstanceMemoryUpdateEventHandler::SumAppMemory(pid_t callerPid, pid_t actualCallerPid)
{
    auto instanceInfoList = AVCodecServerManager::GetInstance().GetInstanceInfoListByActualPid(actualCallerPid);
    uint32_t appMemoryUsage = 0;
    for (const auto &info : instanceInfoList) {
        if (actualCallerPid != callerPid && actualCallerPid != info.second.forwardCaller.pid) {
            continue;
        }
        appMemoryUsage += info.second.memoryUsage;
    }
    return appMemoryUsage;
}

void InstanceMemoryUpdateEventHandler::ReportAppMemory(pid_t callerPid, pid_t actualCallerPid,
                                                       bool isInTimer, uint32_t memory)
{
    if (memory == 0) {
        memory = SumAppMemory(callerPid, actualCallerPid);
    }
    if (isInTimer) {
        InstanceMemoryUpdateEventHandler::GetInstance().RemoveTimer(actualCallerPid);
        InstanceMemoryUpdateEventHandler::GetInstance().AddApp2ExceedThresholdList(actualCallerPid);
    }

    auto memoryCollector = HiviewDFX::UCollectClient::MemoryCollector::Create();
    CHECK_AND_RETURN_LOG(memoryCollector != nullptr, "Create Hiview DFX memory collector failed");

    std::vector<HiviewDFX::UCollectClient::MemoryCaller> memList;
    HiviewDFX::UCollectClient::MemoryCaller memoryCaller = {
        .pid = actualCallerPid,
        .resourceType = "AVCodec",
        .limitValue = memory,
    };
    memList.emplace_back(memoryCaller);
    memoryCollector->SetSplitMemoryValue(memList);
    AVCODEC_LOGI("The memory usage of pid %{public}d is %{public}u KB, report to hivew", actualCallerPid, memory);
}

void InstanceMemoryUpdateEventHandler::DeterminAppMemoryExceedThresholdAndReport(pid_t callerPid,
                                                                                 pid_t forwardCallerPid)
{
    auto actualCallerPid = forwardCallerPid == INVALID_PID ? callerPid : forwardCallerPid;
    if (actualCallerPid == INVALID_PID) {
        return;
    }
    auto memory = SumAppMemory(callerPid, actualCallerPid);
    auto appMemoryExceedThreshold = memory > appMemoryThreshold_;
    auto appExistTimer = false;
    {
        std::shared_lock<std::shared_mutex> timerLock(timerMutex_);
        appExistTimer = timerMap_.count(actualCallerPid) != 0;
    }
    auto appInExceedThresholdList = false;
    {
        std::shared_lock<std::shared_mutex> appMemoryExceedThresholdListlock(appMemoryExceedThresholdListMutex_);
        appInExceedThresholdList = appMemoryExceedThresholdList_.count(actualCallerPid) != 0;
    }
    if (appMemoryExceedThreshold && !appExistTimer && !appInExceedThresholdList) {
        auto timeName = std::string("Pid_") + std::to_string(actualCallerPid) + " memory exceeded threshold";
        auto timer = std::make_shared<AVCodecXcollieTimer>(timeName, false, MEMORY_LEAK_UPLOAD_TIMEOUT,
            [=](void *) -> void { ReportAppMemory(callerPid, actualCallerPid); });
        std::lock_guard<std::shared_mutex> timerLock(timerMutex_);
        timerMap_.emplace(actualCallerPid, timer);
        AVCODEC_LOGI("Determined pid %{public}d memory(%{public}u KB) exceed threshold(%{public}u KB), "
            "event timer added", actualCallerPid, memory, appMemoryThreshold_);
    } else if (!appMemoryExceedThreshold && appExistTimer && !appInExceedThresholdList) {
        InstanceMemoryUpdateEventHandler::GetInstance().RemoveTimer(actualCallerPid);
    } else if (appMemoryExceedThreshold && !appExistTimer && appInExceedThresholdList) {
        ReportAppMemory(callerPid, actualCallerPid, false, memory);
    } else if (!appMemoryExceedThreshold && !appExistTimer && appInExceedThresholdList) {
        ReportAppMemory(callerPid, actualCallerPid, false, memory);
        std::lock_guard<std::shared_mutex> appMemoryExceedThresholdListlock(appMemoryExceedThresholdListMutex_);
        appMemoryExceedThresholdList_.erase(actualCallerPid);
        AVCODEC_LOGI("Pid: %{public}d has been removed from exceeded threshold list", actualCallerPid);
    }
}

uint32_t InstanceMemoryUpdateEventHandler::ThresholdParser::GetThreshold()
{
    char configFilePathBuf[MAX_PATH_LEN] = {0};
    GetOneCfgFile("etc/reliability/leak_detector_config.json", configFilePathBuf, MAX_PATH_LEN);
    CHECK_AND_RETURN_RET_LOGW(realpath(configFilePathBuf, configFilePathBuf) != nullptr,
        UINT32_MAX, "Can not get real path of threshold config json file");
    std::ifstream configFile(configFilePathBuf);
    CHECK_AND_RETURN_RET_LOG(configFile.is_open(), UINT32_MAX, "Can not open threshold config json file");

    std::string line;
    std::string configJson;
    while (configFile >> line && configJson.size() < THRESHOLD_CONFIG_FILE_SIZE_MAX) {
        configJson += line;
    }

    auto root = std::shared_ptr<cJSON>(cJSON_Parse(configJson.c_str()), cJSON_Delete);
    CHECK_AND_RETURN_RET_LOG(root != nullptr, UINT32_MAX, "Can not parse threshold config json");

    auto commercialConfig = cJSON_GetObjectItem(root.get(), "Commercial");
    CHECK_AND_RETURN_RET_LOG(!cJSON_IsNull(commercialConfig), UINT32_MAX, "Can not parse commercial config");

    auto thresholdItem = cJSON_GetObjectItem(commercialConfig, "av_codec_config");
    CHECK_AND_RETURN_RET_LOG(!cJSON_IsNull(thresholdItem) && cJSON_IsNumber(thresholdItem),
        UINT32_MAX, "Can not find threshold from av_codec_config");

    AVCODEC_LOGI("Got threshold: %{public}u KB", thresholdItem->valueint);
    return thresholdItem->valueint;
}
} // namespace MediaAVCodec
} // namespace OHOS