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
#include <unordered_set>
#include "avcodec_log.h"
#include "avcodec_errors.h"
#include "avcodec_server_manager.h"
#include "syspara/parameters.h"
#include "av_codec_service_ipc_interface_code.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "BackGroundEventHandler"};
constexpr auto CODEC_STUB_INTERFACE_DESCRIPTOR = u"IStandardCodecService";
} // namespace

namespace OHOS {
namespace MediaAVCodec {
std::vector<CodecInstance> GetDirectlyInvokedCodecInstanceListByPidList(std::vector<pid_t> pidList)
{
    std::vector<CodecInstance> instanceList;
    for (auto pid : pidList) {
        for (const auto &codecInstance : AVCodecServerManager::GetInstance().GetInstanceInfoListByPid(pid)) {
            if ((codecInstance.second.caller.pid == pid) &&
                (codecInstance.second.codecType == AVCODEC_TYPE_VIDEO_DECODER)) {
                instanceList.push_back(codecInstance);
            }
        }
    }
    return instanceList;
}

ObjectList::const_iterator GetObjectFromList(const ObjectList &list, pid_t pid, InstanceId instanceId)
{
    auto range = list.equal_range(pid);
    for (auto iter = range.first; iter != range.second; iter++) {
        if (iter->second == instanceId) {
            return iter;
        }
    }
    return list.end();
}

int32_t SendRequest2CodecStub(sptr<IRemoteObject> &instance, CodecServiceInterfaceCode code)
{
    MessageParcel data;
    data.WriteInterfaceToken(CODEC_STUB_INTERFACE_DESCRIPTOR);
    MessageParcel reply;
    MessageOption option;
    instance->SendRequest(static_cast<uint32_t>(code), data, reply, option);
    return reply.ReadInt32();
}

void MemoryRecycleHandler(ObjectList &list, pid_t actualPid, CodecInstance &codecInstance)
{
    static bool supportMemoryRecycle = OHOS::system::GetBoolParameter("resourceschedule.memmgr.dma.reclaimable", false);
    if (!supportMemoryRecycle) {
        return;
    }
    auto &[instance, instanceInfo] = codecInstance;
    CHECK_AND_RETURN_LOG(instance != nullptr, "instance is nullptr");
    if (GetObjectFromList(list, actualPid, instanceInfo.instanceId) != list.end()) {
        return;
    }
    auto ret = SendRequest2CodecStub(instance, CodecServiceInterfaceCode::NOTIFY_MEMORY_RECYCLE);
    CHECK_AND_RETURN_LOG(ret == AVCS_ERR_OK, "Error, ret: %{public}d", ret);
    list.emplace(actualPid, instanceInfo.instanceId);

    AVCODEC_LOGI("Done, pid: %{public}d, instanceId: %{public}d", actualPid, instanceInfo.instanceId);
}

void MemoryWriteBackHandler(ObjectList &list, pid_t actualPid, CodecInstance &codecInstance)
{
    auto &[instance, instanceInfo] = codecInstance;
    CHECK_AND_RETURN_LOG(instance != nullptr, "instance is nullptr");
    auto recordedInfo = GetObjectFromList(list, actualPid, instanceInfo.instanceId);
    if (recordedInfo == list.end()) {
        return;
    }
    auto ret = SendRequest2CodecStub(instance, CodecServiceInterfaceCode::NOTIFY_MEMORY_WRITE_BACK);
    CHECK_AND_RETURN_LOG(ret == AVCS_ERR_OK, "Error, ret: %{public}d", ret);
    list.erase(recordedInfo);

    AVCODEC_LOGI("Done, pid: %{public}d, instanceId: %{public}d", actualPid, instanceInfo.instanceId);
}

void SuspendHandler(ObjectList &list, pid_t actualPid, CodecInstance &codecInstance)
{
    auto &[instance, instanceInfo] = codecInstance;
    CHECK_AND_RETURN_LOG(instance != nullptr, "instance is nullptr");
    if (GetObjectFromList(list, actualPid, instanceInfo.instanceId) != list.end()) {
        return;
    }
    auto ret = SendRequest2CodecStub(instance, CodecServiceInterfaceCode::NOTIFY_SUSPEND);
    CHECK_AND_RETURN_LOG(ret == AVCS_ERR_OK, "Error, ret: %{public}d", ret);
    list.emplace(actualPid, instanceInfo.instanceId);

    AVCODEC_LOGI("Done, pid: %{public}d, instanceId: %{public}d", actualPid, instanceInfo.instanceId);
}

void ResumeHandler(ObjectList &list, pid_t actualPid, CodecInstance &codecInstance)
{
    auto &[instance, instanceInfo] = codecInstance;
    CHECK_AND_RETURN_LOG(instance != nullptr, "instance is nullptr");
    auto recordedInfo = GetObjectFromList(list, actualPid, instanceInfo.instanceId);
    if (recordedInfo == list.end()) {
        return;
    }
    auto ret = SendRequest2CodecStub(instance, CodecServiceInterfaceCode::NOTIFY_RESUME);
    CHECK_AND_RETURN_LOG(ret == AVCS_ERR_OK, "Error, ret: %{public}d", ret);
    list.erase(recordedInfo);

    AVCODEC_LOGI("Done, pid: %{public}d, instanceId: %{public}d", actualPid, instanceInfo.instanceId);
}

/**************************** BackGroundEventHandler ****************************/

BackGroundEventHandler &BackGroundEventHandler::GetInstance()
{
    static BackGroundEventHandler instance;
    return instance;
}

void BackGroundEventHandler::NotifyFreeze(InstanceId instanceId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto codecInstance = AVCodecServerManager::GetInstance().GetCodecInstanceByInstanceId(instanceId);
    if (codecInstance == std::nullopt) {
        return;
    }
    auto &callerPid = codecInstance->second.caller.pid;
    auto &forwardPid = codecInstance->second.forwardCaller.pid;
    auto &actualPid = forwardPid == MediaAVCodec::INVALID_PID ? callerPid : forwardPid;
    MemoryRecycleHandler(memoryRecycleList_, actualPid, codecInstance.value());
    SuspendHandler(suspendList_, actualPid, codecInstance.value());
}

void BackGroundEventHandler::NotifyFreeze(const std::vector<pid_t> &pidList)
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto &codecInstance : GetDirectlyInvokedCodecInstanceListByPidList(pidList)) {
        auto &callerPid = codecInstance.second.caller.pid;
        auto &forwardPid = codecInstance.second.forwardCaller.pid;
        auto &actualPid = forwardPid == MediaAVCodec::INVALID_PID ? callerPid : forwardPid;
        MemoryRecycleHandler(memoryRecycleList_, actualPid, codecInstance);
        SuspendHandler(suspendList_, actualPid, codecInstance);
    }
}

void BackGroundEventHandler::NotifyActive(InstanceId instanceId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto codecInstance = AVCodecServerManager::GetInstance().GetCodecInstanceByInstanceId(instanceId);
    if (codecInstance == std::nullopt) {
        return;
    }
    auto &callerPid = codecInstance->second.caller.pid;
    auto &forwardPid = codecInstance->second.forwardCaller.pid;
    auto &actualPid = forwardPid == MediaAVCodec::INVALID_PID ? callerPid : forwardPid;
    MemoryWriteBackHandler(memoryRecycleList_, actualPid, codecInstance.value());
    ResumeHandler(suspendList_, actualPid, codecInstance.value());
}

void BackGroundEventHandler::NotifyActive(const std::vector<pid_t> &pidList)
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto &codecInstance : GetDirectlyInvokedCodecInstanceListByPidList(pidList)) {
        auto &callerPid = codecInstance.second.caller.pid;
        auto &forwardPid = codecInstance.second.forwardCaller.pid;
        auto &actualPid = forwardPid == MediaAVCodec::INVALID_PID ? callerPid : forwardPid;
        MemoryWriteBackHandler(memoryRecycleList_, actualPid, codecInstance);
        ResumeHandler(suspendList_, actualPid, codecInstance);
    }
}

void BackGroundEventHandler::NotifyActiveAll()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (memoryRecycleList_.empty() && suspendList_.empty()) {
        return;
    }
    std::unordered_set<pid_t> pidSet;
    for (const auto &[pid, instanceId] : memoryRecycleList_) {
        pidSet.insert(pid);
    }
    for (const auto &[pid, instanceId] : suspendList_) {
        pidSet.insert(pid);
    }
    std::vector<pid_t> pidList(pidSet.begin(), pidSet.end());
    NotifyActive(pidList);
    AVCODEC_LOGI("Done");
}

void BackGroundEventHandler::ErasePid(pid_t pid)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto eraser = [pid](ObjectList &list, std::string_view name) {
        auto range = list.equal_range(pid);
        for (auto iter = range.first; iter != range.second; iter++) {
            list.erase(iter);
        }
        EXPECT_AND_LOGI(range.first != range.second, "Erased pid: %{public}d from %{public}s", pid, name.data());
    };
    eraser(memoryRecycleList_, "MemoryRecycleList");
    eraser(suspendList_, "SuspendedList");
}

void BackGroundEventHandler::EraseInstance(InstanceId instanceId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto eraser = [instanceId](ObjectList &list, std::string_view name) {
        for (auto iter = list.begin(); iter != list.end();) {
            if (iter->second == instanceId) {
                list.erase(iter);
                AVCODEC_LOGI("Erased instanceId: %{public}d from %{public}s", instanceId, name.data());
                break;
            } else {
                ++iter;
            }
        }
    };
    eraser(memoryRecycleList_, "MemoryRecycleList");
    eraser(suspendList_, "SuspendedList");
}
} // namespace MediaAVCodec
} // namespace OHOS