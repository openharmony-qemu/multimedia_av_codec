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

#ifndef BACKGROUND_EVENT_HANDLER_H
#define BACKGROUND_EVENT_HANDLER_H

#include "iremote_object.h"

namespace OHOS {
namespace MediaAVCodec {
class BackGroundEventHandler {
public:
    static BackGroundEventHandler& GetInstance();
    BackGroundEventHandler();
    ~BackGroundEventHandler()=default;
    void RegisterSuspendObserver();
    void UnregisterSuspendObserver();

private:
    std::vector<sptr<IRemoteObject>> GetFreezeInfoList(pid_t pid);
    void NotifyFrozen(const std::vector<int32_t> &pidList);
    void NotifyActive(const std::vector<int32_t> &pidList);
#ifdef USE_EFFICIENCY_MANAGER
    class SuspendStateObserverStubObj;
    sptr<SuspendStateObserverStubObj> suspendObservers_ = nullptr;
#endif //USE_EFFICIENCY_MANAGER
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif //BACKGROUND_EVENT_HANDLER_H