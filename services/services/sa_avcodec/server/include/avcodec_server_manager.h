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

#ifndef AVCODEC_SERVER_MANAGER_H
#define AVCODEC_SERVER_MANAGER_H

#include <memory>
#include <functional>
#include <map>
#include <list>
#include "iremote_object.h"
#include "ipc_skeleton.h"
#include "nocopyable.h"

namespace OHOS {
namespace MediaAVCodec {
class AVCodecServerManager : public NoCopyable {
public:
    static AVCodecServerManager& GetInstance();
    ~AVCodecServerManager();

    enum StubType { CODECLIST, CODEC };
    int32_t CreateStubObject(StubType type, sptr<IRemoteObject> &object);
    void DestroyStubObject(StubType type, sptr<IRemoteObject> object);
    void DestroyStubObjectForPid(pid_t pid);
    int32_t Dump(int32_t fd, const std::vector<std::u16string>& args);
    void NotifyProcessStatus(const int32_t status);
    void SetCritical(const bool isKeyService);

private:
    AVCodecServerManager();

#ifdef SUPPORT_CODEC
    int32_t CreateCodecStubObject(sptr<IRemoteObject> &object);
#endif
#ifdef SUPPORT_CODECLIST
    int32_t CreateCodecListStubObject(sptr<IRemoteObject> &object);
#endif

    void EraseObject(std::map<sptr<IRemoteObject>, pid_t>::iterator& iter,
                     std::map<sptr<IRemoteObject>, pid_t>& stubMap,
                     pid_t pid,
                     const std::string& stubName);
    void EraseObject(std::map<sptr<IRemoteObject>, pid_t>& stubMap, pid_t pid);
    void Init();

    class AsyncExecutor {
    public:
        AsyncExecutor() = default;
        virtual ~AsyncExecutor() = default;
        void Commit(sptr<IRemoteObject> obj);
        void Clear();

    private:
        void HandleAsyncExecution();
        std::list<sptr<IRemoteObject>> freeList_;
        std::mutex listMutex_;
    };

    std::map<sptr<IRemoteObject>, pid_t> codecStubMap_;
    std::map<sptr<IRemoteObject>, pid_t> codecListStubMap_;
    AsyncExecutor executor_;
    pid_t pid_ = 0;
    std::mutex mutex_;
    using NotifyProcessStatusFunc = int32_t(*)(int32_t pid, int32_t type, int32_t status, int32_t saId);
    using SetCriticalFunc = int32_t(*)(int32_t pid, bool critical, int32_t saId);
    static constexpr char LIB_PATH[] = "libmemmgrclient.z.so";
    static constexpr char NOTIFY_STATUS_FUNC_NAME[] = "notify_process_status";
    static constexpr char SET_CRITICAL_FUNC_NAME[] = "set_critical";
    std::shared_ptr<void> libMemMgrClientHandle_ = nullptr;
    NotifyProcessStatusFunc notifyProcessStatusFunc_ = nullptr;
    SetCriticalFunc setCriticalFunc_ = nullptr;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // AVCODEC_SERVER_MANAGER_H