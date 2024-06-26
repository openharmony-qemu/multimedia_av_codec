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
    std::lock_guard<std::mutex> lock(mutex_);

    constexpr std::string_view dumpStr = "[Codec_Server]\n";
    write(fd, dumpStr.data(), dumpStr.size());

    int32_t instanceIndex = 0;
    for (auto iter : codecStubMap_) {
        std::string instanceStr = std::string("    Instance_") + std::to_string(instanceIndex++) + "_Info\n";
        write(fd, instanceStr.data(), instanceStr.size());
        (void)iter.first->Dump(fd, args);
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
    std::lock_guard<std::mutex> lock(mutex_);
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
    AVCODEC_LOGD("The number of codeclist services(%{public}zu).", codecListStubMap_.size());
    return AVCS_ERR_OK;
}
#endif
#ifdef SUPPORT_CODEC
int32_t AVCodecServerManager::CreateCodecStubObject(sptr<IRemoteObject> &object)
{
    sptr<CodecServiceStub> stub = CodecServiceStub::Create();
    CHECK_AND_RETURN_RET_LOG(stub != nullptr, AVCS_ERR_CREATE_AVCODEC_STUB_FAILED, "Failed to create CodecServiceStub");

    object = stub->AsObject();
    CHECK_AND_RETURN_RET_LOG(object != nullptr, AVCS_ERR_CREATE_AVCODEC_STUB_FAILED,
        "Failed to create CodecServiceStub");

    pid_t pid = IPCSkeleton::GetCallingPid();
    codecStubMap_[object] = pid;

    SetCritical(true);
    AVCODEC_LOGD("The number of codec services(%{public}zu).", codecStubMap_.size());
    return AVCS_ERR_OK;
}
#endif

void AVCodecServerManager::EraseObject(std::map<sptr<IRemoteObject>, pid_t>::iterator& iter,
                                       std::map<sptr<IRemoteObject>, pid_t>& stubMap,
                                       pid_t pid,
                                       const std::string& stubName)
{
    if (iter != stubMap.end()) {
        AVCODEC_LOGI("destroy %{public}s stub services(%{public}zu) pid(%{public}d).", stubName.c_str(),
                     stubMap.size(), pid);
        (void)stubMap.erase(iter);
        return;
    }
    AVCODEC_LOGE("find %{public}s object failed, pid(%{public}d).", stubName.c_str(), pid);
    return;
}

void AVCodecServerManager::DestroyStubObject(StubType type, sptr<IRemoteObject> object)
{
    std::lock_guard<std::mutex> lock(mutex_);
    pid_t pid = IPCSkeleton::GetCallingPid();
    auto compare_func = [object](std::pair<sptr<IRemoteObject>, pid_t> objectPair) -> bool {
        return objectPair.first == object;
    };
    switch (type) {
        case CODEC: {
            auto it = find_if(codecStubMap_.begin(), codecStubMap_.end(), compare_func);
            EraseObject(it, codecStubMap_, pid, "codec");
            break;
        }
        case CODECLIST: {
            auto it = find_if(codecListStubMap_.begin(), codecListStubMap_.end(), compare_func);
            EraseObject(it, codecListStubMap_, pid, "codeclist");
            break;
        }
        default: {
            AVCODEC_LOGE("default case, av_codec server manager failed, pid(%{public}d).", pid);
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

void AVCodecServerManager::DestroyStubObjectForPid(pid_t pid)
{
    std::lock_guard<std::mutex> lock(mutex_);
    AVCODEC_LOGI("codec stub services(%{public}zu) pid(%{public}d).", codecStubMap_.size(), pid);
    EraseObject(codecStubMap_, pid);
    AVCODEC_LOGI("codec stub services(%{public}zu).", codecStubMap_.size());

    AVCODEC_LOGI("codeclist stub services(%{public}zu) pid(%{public}d).", codecListStubMap_.size(), pid);
    EraseObject(codecListStubMap_, pid);
    AVCODEC_LOGI("codeclist stub services(%{public}zu).", codecListStubMap_.size());
    executor_.Clear();
    if (codecStubMap_.size() == 0) {
        SetCritical(false);
    }
}

void AVCodecServerManager::NotifyProcessStatus(const int32_t status)
{
    CHECK_AND_RETURN_LOG(notifyProcessStatusFunc_ != nullptr, "notify memory manager is nullptr, %{public}d.", status);
    int32_t ret = notifyProcessStatusFunc_(pid_, 1, status, AV_CODEC_SERVICE_ID);
    if (ret == 0) {
        AVCODEC_LOGI("notify memory manager to %{public}d success.", status);
    } else {
        AVCODEC_LOGW("notify memory manager to %{public}d fail.", status);
    }
}

void AVCodecServerManager::SetCritical(const bool isKeyService)
{
    CHECK_AND_RETURN_LOG(setCriticalFunc_ != nullptr, "set critical is nullptr, %{public}d.", isKeyService);
    int32_t ret = setCriticalFunc_(pid_, isKeyService, AV_CODEC_SERVICE_ID);
    if (ret == 0) {
        AVCODEC_LOGI("set critical to %{public}d success.", isKeyService);
    } else {
        AVCODEC_LOGW("set critical to %{public}d fail.", isKeyService);
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
