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

#ifndef MEM_MGR_PROXY_MOCK_H
#define MEM_MGR_PROXY_MOCK_H

#include <gmock/gmock.h>
#include <memory>
#include <refbase.h>
#include "avcodec_errors.h"
#include "hilog/log.h"
#include "ipc/av_codec_service_ipc_interface_code.h"
#include "iremote_broker.h"
#include "iremote_object.h"
#include "iremote_stub.h"
#include "nocopyable.h"

namespace OHOS {
namespace Memory {
class IStandardMemMgrProxy : public IRemoteBroker {
public:
    virtual ~IStandardMemMgrProxy() = default;

    virtual int32_t NotifyProcessStatus(pid_t pid, int32_t status, int32_t start, int32_t saId) = 0;
    virtual int32_t SetCritical(pid_t pid, bool flag, int32_t saId) = 0;

    DECLARE_INTERFACE_DESCRIPTOR(u"IStandardMemMgrProxy");
};

class MemMgrProxy;
class MemMgrProxyMock {
public:
    MemMgrProxyMock() = default;
    ~MemMgrProxyMock() = default;

    MOCK_METHOD(int32_t, NotifyProcessStatus, (pid_t pid, int32_t, int32_t, int32_t));
    MOCK_METHOD(int32_t, SetCritical, (pid_t pid, bool, int32_t));
};

class MemMgrProxy : public IRemoteStub<IStandardMemMgrProxy>, public NoCopyable {
public:
    static void RegisterMock(std::shared_ptr<MemMgrProxyMock> &mock);
    MemMgrProxy();
    ~MemMgrProxy();
    int32_t NotifyProcessStatus(pid_t pid, int32_t status, int32_t start, int32_t saId) override;
    int32_t SetCritical(pid_t pid, bool flag, int32_t saId) override;
};
} // namespace Memory
} // namespace OHOS
#endif // CODECLIST_SERVICE_STUB_MOCK_H