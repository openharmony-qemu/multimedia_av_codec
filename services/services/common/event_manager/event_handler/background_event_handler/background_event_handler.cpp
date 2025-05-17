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

#include "background_event_handler.h"
#include "avcodec_log.h"
#include "avcodec_errors.h"
#include "avcodec_server.h"
#include "avcodec_server_manager.h"
#include "codec_service_stub.h"
#include "syspara/parameters.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "BackGroundEventHandler"};
} // namespace
namespace OHOS {
namespace MediaAVCodec {
static std::vector<sptr<IRemoteObject>> GetFreezeInfoList(pid_t pid)
{
    std::vector<sptr<IRemoteObject>> instanceList;
    std::vector<std::pair<sptr<IRemoteObject>, InstanceInfo>> instanceInfos =
        AVCodecServerManager::GetInstance().GetInstanceInfoListByPid(pid);
    for (auto &instance : instanceInfos) {
        if ((instance.second.caller.pid == pid) && (instance.second.codecType == AVCODEC_TYPE_VIDEO_DECODER)) {
            instanceList.push_back(instance.first);
        }
    }
    return instanceList;
}

BackGroundEventHandler::BackGroundEventHandler()
    : recycleMemory(OHOS::system::GetBoolParameter("resourceschedule.memmgr.dma.reclaimable", false)) {}

void BackGroundEventHandler::NotifyFrozen(const std::vector<int32_t> &pidList)
{
    if (!recycleMemory) {
        return;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto pid : pidList) {
        std::vector<sptr<IRemoteObject>> instanceList = GetFreezeInfoList(pid);
        if (!instanceList.empty()) {
            frozenPidList_.insert(pid);
            AVCODEC_LOGI("Freeze pid: %{public}d, frozenPidList_ size: %{public}zu", pid, frozenPidList_.size());
        }
        for (auto &instance : instanceList) {
            CHECK_AND_CONTINUE_LOG(instance != nullptr, "instance is nullptr");
            static_cast<CodecServiceStub *>(instance.GetRefPtr())->NotifyMemoryRecycle();
        }
    }
    return;
}

void BackGroundEventHandler::NotifyActive(const std::vector<int32_t> &pidList)
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto pid : pidList) {
        std::vector<sptr<IRemoteObject>> instanceList = GetFreezeInfoList(pid);
        if (!instanceList.empty()) {
            frozenPidList_.erase(pid);
            AVCODEC_LOGI("Active pid: %{public}d, frozenPidList_ size: %{public}zu", pid, frozenPidList_.size());
        }
        for (auto &instance : instanceList) {
            CHECK_AND_CONTINUE_LOG(instance != nullptr, "instance is nullptr");
            static_cast<CodecServiceStub *>(instance.GetRefPtr())->NotifyMemoryWriteBack();
        }
    }
    return;
}

void BackGroundEventHandler::NotifyActiveAll()
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto pid : frozenPidList_) {
        std::vector<sptr<IRemoteObject>> instanceList = GetFreezeInfoList(pid);
        AVCODEC_LOGI("ActiveAll, frozenPidList_ size: %{public}zu", frozenPidList_.size());
        for (auto &instance : instanceList) {
            CHECK_AND_CONTINUE_LOG(instance != nullptr, "instance is nullptr");
            static_cast<CodecServiceStub *>(instance.GetRefPtr())->NotifyMemoryWriteBack();
        }
    }
    frozenPidList_.clear();
    return;
}

void BackGroundEventHandler::ClearPid(pid_t pid)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = frozenPidList_.find(pid);
    if (it != frozenPidList_.end()) {
        frozenPidList_.erase(it);
        AVCODEC_LOGI("Cleared pid: %{public}d from frozenPidList_, remain size: %{public}zu", pid, frozenPidList_.size());
    } else {
        AVCODEC_LOGI("Pid: %{public}d not found in frozenPidList_", pid);
    }
}

BackGroundEventHandler &BackGroundEventHandler::GetInstance()
{
    static BackGroundEventHandler instance;
    return instance;
}
} // namespace MediaAVCodec
} // namespace OHOS