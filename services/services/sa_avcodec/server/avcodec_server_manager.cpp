/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include "avcodec_server_manager.h"
#include <codecvt>
#include <dlfcn.h>
#include <locale>
#include <thread>
#include <unistd.h>
#include "avcodec_dump_utils.h"
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "avcodec_trace.h"
#include "avcodec_xcollie.h"
#include "system_ability_definition.h"
#ifdef SUPPORT_CODEC
#include "codec_service_stub.h"
#endif
#ifdef SUPPORT_CODECLIST
#include "codeclist_service_stub.h"
#endif

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "AVCodecServerManager"};
} // namespace

namespace OHOS {
namespace MediaAVCodec {
AVCodecServerManager& AVCodecServerManager::GetInstance()
{
    static AVCodecServerManager instance;
    return instance;
}

int32_t AVCodecServerManager::Dump(int32_t fd, const std::vector<std::u16string>& args)
{
    if (fd < 0) {
        return OHOS::NO_ERROR;
    }
    AVCodecXCollie::GetInstance().Dump(fd);

    std::unordered_multimap<pid_t, std::pair<sptr<IRemoteObject>, InstanceInfo>> codecStubMapTemp;
    {
        std::lock_guard<std::shared_mutex> lock(mutex_);
        codecStubMapTemp = codecStubMap_;
    }

    constexpr std::string_view dumpStr = "[Codec_Server]\n";
    write(fd, dumpStr.data(), dumpStr.size());

    int32_t instanceIndex = 0;
    for (auto iter : codecStubMapTemp) {
        std::string instanceStr = std::string("    Instance_") + std::to_string(instanceIndex++) + "_Info\n";
        write(fd, instanceStr.data(), instanceStr.size());
        (void)iter.second.first->Dump(fd, args);
    }

    return OHOS::NO_ERROR;
}

AVCodecServerManager::AVCodecServerManager()
{
    pid_ = getpid();
    Init();
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AVCodecServerManager::~AVCodecServerManager()
{
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

void AVCodecServerManager::Init()
{
    void *handle = dlopen(LIB_PATH, RTLD_NOW);
    CHECK_AND_RETURN_LOG(handle != nullptr, "Load so failed:%{public}s", LIB_PATH);
    libMemMgrClientHandle_ = std::shared_ptr<void>(handle, dlclose);
    notifyProcessStatusFunc_ = reinterpret_cast<NotifyProcessStatusFunc>(dlsym(handle, NOTIFY_STATUS_FUNC_NAME));
    CHECK_AND_RETURN_LOG(notifyProcessStatusFunc_ != nullptr, "Load notifyProcessStatusFunc failed:%{public}s",
                         NOTIFY_STATUS_FUNC_NAME);
    setCriticalFunc_ = reinterpret_cast<SetCriticalFunc>(dlsym(handle, SET_CRITICAL_FUNC_NAME));
    CHECK_AND_RETURN_LOG(setCriticalFunc_ != nullptr, "Load setCriticalFunc failed:%{public}s",
                         SET_CRITICAL_FUNC_NAME);
    return;
}

int32_t AVCodecServerManager::CreateStubObject(StubType type, sptr<IRemoteObject> &object)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    switch (type) {
#ifdef SUPPORT_CODECLIST
        case CODECLIST: {
            return CreateCodecListStubObject(object);
        }
#endif
#ifdef SUPPORT_CODEC
        case CODEC: {
            return CreateCodecStubObject(object);
        }
#endif
        default: {
            AVCODEC_LOGE("default case, av_codec server manager failed");
            return AVCS_ERR_UNSUPPORT;
        }
    }
}

#ifdef SUPPORT_CODECLIST
int32_t AVCodecServerManager::CreateCodecListStubObject(sptr<IRemoteObject> &object)
{
    sptr<CodecListServiceStub> stub = CodecListServiceStub::Create();
    CHECK_AND_RETURN_RET_LOG(stub != nullptr, AVCS_ERR_CREATE_CODECLIST_STUB_FAILED,
        "Failed to create AVCodecListServiceStub");
    object = stub->AsObject();
    CHECK_AND_RETURN_RET_LOG(object != nullptr, AVCS_ERR_CREATE_CODECLIST_STUB_FAILED,
        "Failed to create AVCodecListServiceStub");

    pid_t pid = IPCSkeleton::GetCallingPid();
    codecListStubMap_[object] = pid;
    AVCODEC_LOGD("The number of codeclist services(%{public}zu)", codecListStubMap_.size());
    return AVCS_ERR_OK;
}
#endif
#ifdef SUPPORT_CODEC
int32_t AVCodecServerManager::CreateCodecStubObject(sptr<IRemoteObject> &object)
{
    static std::atomic<int32_t> instanceId = 0;
    sptr<CodecServiceStub> stub = CodecServiceStub::Create(instanceId);
    CHECK_AND_RETURN_RET_LOG(stub != nullptr, AVCS_ERR_CREATE_AVCODEC_STUB_FAILED, "Failed to create CodecServiceStub");

    object = stub->AsObject();
    CHECK_AND_RETURN_RET_LOG(object != nullptr, AVCS_ERR_CREATE_AVCODEC_STUB_FAILED,
        "Failed to create CodecServiceStub");

    pid_t pid = IPCSkeleton::GetCallingPid();
    InstanceInfo instanceInfo = {
        .instanceId = instanceId,
        .caller = { .pid = pid },
    };
    instanceId++;
    if (instanceId == INT32_MAX) {
        instanceId = 0;
    }
    codecStubMap_.emplace(pid, std::make_pair(object, instanceInfo));

    SetCritical(true);
    AVCODEC_LOGD("The number of codec services(%{public}zu)", codecStubMap_.size());
    return AVCS_ERR_OK;
}
#endif

void AVCodecServerManager::DestroyStubObject(StubType type, sptr<IRemoteObject> object)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    pid_t pid = IPCSkeleton::GetCallingPid();
    switch (type) {
        case CODEC: {
            auto it = find_if(codecStubMap_.begin(), codecStubMap_.end(),
                [object] (std::pair<pid_t, std::pair<sptr<IRemoteObject>, InstanceInfo>> objectPair) ->
                    bool { return objectPair.second.first == object; });
            CHECK_AND_BREAK_LOG(it != codecStubMap_.end(), "find codec object failed, pid(%{public}d)", pid);

            auto preSize = codecStubMap_.size();
            AVCODEC_LOGI("codec stub services(%{public}zu->%{public}zu) pid(%{public}d)", preSize, preSize - 1, pid);
            codecStubMap_.erase(it);
            break;
        }
        case CODECLIST: {
            auto it = find_if(codecListStubMap_.begin(), codecListStubMap_.end(),
                [object] (std::pair<sptr<IRemoteObject>, pid_t> objectPair) ->
                    bool { return objectPair.first == object; });
            CHECK_AND_BREAK_LOG(it != codecListStubMap_.end(), "find codeclist object failed, pid(%{public}d)", pid);

            auto preSize = codecListStubMap_.size();
            AVCODEC_LOGI("codeclist stub services(%{public}zu->%{public}zu) pid(%{public}d)", preSize, preSize - 1,
                         pid);
            codecListStubMap_.erase(it);
            break;
        }
        default: {
            AVCODEC_LOGE("default case, av_codec server manager failed, pid(%{public}d)", pid);
            break;
        }
    }
    if (codecStubMap_.size() == 0) {
        SetCritical(false);
    }
}

void AVCodecServerManager::EraseObject(std::map<sptr<IRemoteObject>, pid_t>& stubMap, pid_t pid)
{
    for (auto it = stubMap.begin(); it != stubMap.end();) {
        if (it->second == pid) {
            executor_.Commit(it->first);
            it = stubMap.erase(it);
        } else {
            it++;
        }
    }
    return;
}

void AVCodecServerManager::EraseCodecObjectByPid(pid_t pid)
{
    for (auto it = codecStubMap_.begin(); it != codecStubMap_.end();) {
        if (it->first == pid) {
            executor_.Commit(it->second.first);
            it = codecStubMap_.erase(it);
        } else {
            it++;
        }
    }
    return;
}

void AVCodecServerManager::DestroyStubObjectForPid(pid_t pid)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    auto preSize = codecStubMap_.size();
    EraseCodecObjectByPid(pid);
    AVCODEC_LOGI("codec stub services(%{public}zu->%{public}zu),pid(%{public}d)", preSize, codecStubMap_.size(), pid);

    preSize = codecListStubMap_.size();
    EraseObject(codecListStubMap_, pid);
    AVCODEC_LOGI("codeclist stub services(%{public}zu->%{public}zu),pid(%{public}d)", preSize, codecListStubMap_.size(),
                 pid);
    executor_.Clear();
    if (codecStubMap_.size() == 0) {
        SetCritical(false);
    } else {
        PrintCodecCallersInfo();
    }
}

void AVCodecServerManager::PrintCodecCallersInfo()
{
    typedef struct CodecCallerInfo {
        InstanceInfo info;
        int32_t count = 0;
    } CodecCallerInfo;
    auto processCodecType = [](auto &infoMap, const char *typeStr) {
        for (auto &[pid, data] : infoMap) {
            const InstanceInfo &info = data.info;
            const bool isForwardCaller = (info.forwardCaller.pid != INVALID_PID);
            const CallerInfo &caller = isForwardCaller ? info.forwardCaller : info.caller;
            if (isForwardCaller) {
                AVCODEC_LOGI("[%{public}s->%{public}s][pid %{public}zu] holding %{public}d %{public}s",
                             caller.processName.c_str(), info.caller.processName.c_str(), caller.pid, data.count,
                             typeStr);
            } else {
                AVCODEC_LOGI("[%{public}s][pid %{public}zu] holding %{public}d %{public}s", caller.processName.c_str(),
                             caller.pid, data.count, typeStr);
            }
        }
    };
    std::map<pid_t, CodecCallerInfo> vencCallerInfo;
    std::map<pid_t, CodecCallerInfo> vdecCallerInfo;
    for (auto &[_, instance] : codecStubMap_) {
        const InstanceInfo &info = instance.second;
        const pid_t actualPid = (info.forwardCaller.pid != INVALID_PID) ? info.forwardCaller.pid : info.caller.pid;
        auto *targetMap = (info.codecType == AVCODEC_TYPE_VIDEO_ENCODER)   ? &vencCallerInfo
                          : (info.codecType == AVCODEC_TYPE_VIDEO_DECODER) ? &vdecCallerInfo
                                                                           : nullptr;
        if (targetMap == nullptr) {
            continue;
        }
        CodecCallerInfo &data = (*targetMap)[actualPid];
        if (data.count == 0) {
            data.info = info;
        }
        data.count++;
    }
    processCodecType(vencCallerInfo, "video encoder");
    processCodecType(vdecCallerInfo, "video decoder");
}

void AVCodecServerManager::NotifyProcessStatus(const int32_t status)
{
    CHECK_AND_RETURN_LOG(notifyProcessStatusFunc_ != nullptr, "notify memory manager is nullptr, %{public}d", status);
    int32_t ret = notifyProcessStatusFunc_(pid_, 1, status, AV_CODEC_SERVICE_ID);
    if (ret == 0) {
        AVCODEC_LOGI("notify memory manager to %{public}d success", status);
    } else {
        AVCODEC_LOGW("notify memory manager to %{public}d fail", status);
    }
}

void AVCodecServerManager::SetMemMgrStatus(const bool isStarted)
{
    memMgrStarted_ = isStarted;
}

void AVCodecServerManager::SetCritical(const bool isKeyService)
{
    CHECK_AND_RETURN_LOG(memMgrStarted_, "Memory manager service is not started");
    CHECK_AND_RETURN_LOG(setCriticalFunc_ != nullptr, "set critical is nullptr, %{public}d", isKeyService);
    int32_t ret = setCriticalFunc_(pid_, isKeyService, AV_CODEC_SERVICE_ID);
    if (ret == 0) {
        AVCODEC_LOGI("set critical to %{public}d success", isKeyService);
    } else {
        AVCODEC_LOGW("set critical to %{public}d fail", isKeyService);
    }
}

uint32_t AVCodecServerManager::GetInstanceCount()
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return codecStubMap_.size() + codecListStubMap_.size();
}

std::vector<CodecInstance> AVCodecServerManager::GetInstanceInfoListByPid(pid_t pid)
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    std::vector<CodecInstance> instanceInfoList;
    auto range = codecStubMap_.equal_range(pid);
    for (auto iter = range.first; iter != range.second; iter++) {
        instanceInfoList.emplace_back(iter->second);
    }
    return instanceInfoList;
}

std::vector<CodecInstance> AVCodecServerManager::GetInstanceInfoListByActualPid(pid_t pid)
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    std::vector<CodecInstance> instanceInfoList;
    for (auto iter = codecStubMap_.begin(); iter != codecStubMap_.end(); iter++) {
        auto forwardCallerPid = iter->second.second.forwardCaller.pid;
        auto callerPid = iter->second.second.caller.pid;
        auto actualPid = forwardCallerPid == INVALID_PID ? callerPid : forwardCallerPid;
        if (pid == actualPid) {
            instanceInfoList.emplace_back(iter->second);
        }
    }
    return instanceInfoList;
}

std::optional<InstanceInfo> AVCodecServerManager::GetInstanceInfoByInstanceId(int32_t instanceId)
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    for (auto iter = codecStubMap_.begin(); iter != codecStubMap_.end(); iter++) {
        if (iter->second.second.instanceId == instanceId) {
            return iter->second.second;
        }
    }
    return std::nullopt;
}

std::optional<CodecInstance> AVCodecServerManager::GetCodecInstanceByInstanceId(int32_t instanceId)
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    for (auto iter = codecStubMap_.begin(); iter != codecStubMap_.end(); iter++) {
        if (iter->second.second.instanceId == instanceId) {
            return iter->second;
        }
    }
    return std::nullopt;
}

void AVCodecServerManager::SetInstanceInfoByInstanceId(int32_t instanceId, const InstanceInfo &info)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    for (auto iter = codecStubMap_.begin(); iter != codecStubMap_.end(); iter++) {
        if (iter->second.second.instanceId == instanceId) {
            iter->second.second = info;
        }
    }
}

void AVCodecServerManager::AsyncExecutor::Commit(sptr<IRemoteObject> obj)
{
    std::lock_guard<std::mutex> lock(listMutex_);
    freeList_.push_back(obj);
}

void AVCodecServerManager::AsyncExecutor::Clear()
{
    std::thread(&AVCodecServerManager::AsyncExecutor::HandleAsyncExecution, this).detach();
}

void AVCodecServerManager::AsyncExecutor::HandleAsyncExecution()
{
    std::list<sptr<IRemoteObject>> tempList;
    {
        std::lock_guard<std::mutex> lock(listMutex_);
        freeList_.swap(tempList);
    }
    tempList.clear();
}
} // namespace MediaAVCodec
} // namespace OHOS
