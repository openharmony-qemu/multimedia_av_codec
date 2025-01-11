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
#ifdef USE_EFFICIENCY_MANAGER
#include "syspara/parameters.h"
#include "suspend_manager_client.h"
#include "suspend_state_observer_stub.h"
#endif //USE_EFFICIENCY_MANAGER

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "BackGroundEventHandler"};
} // namespace
namespace OHOS {
namespace MediaAVCodec {
#ifdef USE_EFFICIENCY_MANAGER
class BackGroundEventHandler::SuspendStateObserverStubObj : public SuspendManager::SuspendStateObserverStub {
public:
    void OnActive(const std::vector<int32_t> &pidList, const int32_t uid) override
    {
        AVCODEC_LOGI("OnActive, pidList size:%{public}zu, uid:%{public}d", pidList.size(), uid);
        BackGroundEventHandler::GetInstance().NotifyActive(pidList);
        return;
    }

    void OnDoze(const int32_t uid) override
    {
        AVCODEC_LOGI("OnDoze, uid:%{public}d", uid);
        return;
    }

    void OnFrozen(const std::vector<int32_t> &pidList, const int32_t uid) override
    {
        AVCODEC_LOGI("OnFrozen, pidList size:%{public}zu, uid:%{public}d", pidList.size(), uid);
        BackGroundEventHandler::GetInstance().NotifyFrozen(pidList);
        return;
    }

    void OnFrozenUid(const int32_t uid, const uint32_t reasonId) override
    {
        AVCODEC_LOGI("OnFrozenUid, uid:%{public}d, reasonId:%{public}u", uid, reasonId);
        return;
    }
};
#endif //USE_EFFICIENCY_MANAGER

BackGroundEventHandler::BackGroundEventHandler() {}

void BackGroundEventHandler::RegisterSuspendObserver()
{
#ifdef USE_EFFICIENCY_MANAGER
    bool recycleMemory = OHOS::system::GetBoolParameter("resourceschedule.memmgr.dma.reclaimable", false);
    AVCODEC_LOGI("recycleMemory is %{public}d", recycleMemory);
    if (recycleMemory) {
        suspendObservers_ = sptr<SuspendStateObserverStubObj>(new SuspendStateObserverStubObj());
        CHECK_AND_RETURN_LOG(suspendObservers_ != nullptr, "Create Suspend Observer failed");
        int32_t ret = SuspendManager::SuspendManagerClient::GetInstance().RegisterSuspendObserver(suspendObservers_);
        CHECK_AND_RETURN_LOG(ret == ERR_OK, "RegisterSuspendObserver failed, return: %{public}d", ret);
        AVCODEC_LOGI("RegisterSuspendObserver succeed");
    }
#endif //USE_EFFICIENCY_MANAGER
    return;
}

void BackGroundEventHandler::UnregisterSuspendObserver()
{
#ifdef USE_EFFICIENCY_MANAGER
    if (suspendObservers_ != nullptr) {
        int32_t ret = SuspendManager::SuspendManagerClient::GetInstance().UnregisterSuspendObserver(suspendObservers_);
        CHECK_AND_RETURN_LOG(ret == ERR_OK, "UnRegisterSuspendObserver failed, return: %{public}d", ret);
        suspendObservers_ = nullptr;
        AVCODEC_LOGI("UnRegisterSuspendObserver succeed");
    }
#endif //USE_EFFICIENCY_MANAGER
    return;
}

BackGroundEventHandler& BackGroundEventHandler::GetInstance()
{
    static BackGroundEventHandler instance;
    return instance;
}


std::vector<sptr<IRemoteObject>> BackGroundEventHandler::GetFreezeInfoList(pid_t pid)
{
    std::vector<sptr<IRemoteObject>> instanceList;
    std::vector<std::pair<sptr<IRemoteObject>, InstanceInfo>> instanceInfos =
        AVCodecServerManager::GetInstance().GetInstanceInfoListByPid(pid);
    for (auto &codecStub : instanceInfos) {
        if ((codecStub.second.caller.pid == pid) && (codecStub.second.codecType == AVCODEC_TYPE_VIDEO_DECODER)) {
            instanceList.push_back(codecStub.first);
        }
    }
    return instanceList;
}

void BackGroundEventHandler::NotifyFrozen(const std::vector<int32_t> &pidList)
{
    for (auto pid : pidList) {
        std::vector<sptr<IRemoteObject>> instanceList = GetFreezeInfoList(pid);
        for (auto &instance : instanceList) {
            CHECK_AND_CONTINUE_LOG(instance != nullptr, "instance is nullptr");
            static_cast<CodecServiceStub *>(instance.GetRefPtr())->NotifyMemoryRecycle();
        }
    }
    return;
}

void BackGroundEventHandler::NotifyActive(const std::vector<int32_t> &pidList)
{
    for (auto pid : pidList) {
        std::vector<sptr<IRemoteObject>> instanceList = GetFreezeInfoList(pid);
        for (auto &instance : instanceList) {
            CHECK_AND_CONTINUE_LOG(instance != nullptr, "instance is nullptr");
            static_cast<CodecServiceStub *>(instance.GetRefPtr())->NotifyMemoryWriteBack();
        }
    }
    return;
}
} // namespace MediaAVCodec
} // namespace OHOS