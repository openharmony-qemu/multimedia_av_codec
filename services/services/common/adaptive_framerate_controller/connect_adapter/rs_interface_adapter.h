/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef RS_INTERFACE_ADAPTER_H
#define RS_INTERFACE_ADAPTER_H
#include <string>
#include <vector>

#include <iremote_stub.h>
#include "if_system_ability_manager.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"

#include "avcodec_log_ex.h"
#include "mutex"
namespace OHOS {
namespace MediaAVCodec {

class RSIConnectionToken : public IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"ohos.rosen.RSConnectionToken");

    RSIConnectionToken() = default;
    virtual ~RSIConnectionToken() noexcept = default;
};

class RSAdapterDeathRecipient : public IRemoteObject::DeathRecipient {
public:
    explicit RSAdapterDeathRecipient(std::function<void(const wptr<IRemoteObject> &object)> fun);
    ~RSAdapterDeathRecipient();
    void OnRemoteDied(const wptr<IRemoteObject> &object) override;

private:
    std::function<void(const wptr<IRemoteObject> &object)> mDeathFun;
};

class RSInterfaceAdapter {
private:
    std::recursive_mutex mutex_;
    sptr<IRemoteObject> mHgmRemoteObject = nullptr;
    sptr<IRemoteObject> mRenderServiceRemoteObject = nullptr;
    sptr<RSIConnectionToken> mHgmToken = nullptr;
    sptr<RSAdapterDeathRecipient> mRSInterfaceAdapterDeathRecipient;

private:
    RSInterfaceAdapter();
    bool ConnectRenderService();
    void ResetConnect();
    sptr<IRemoteObject> CreateConnection();

public:
    static RSInterfaceAdapter &GetInstance();
    ~RSInterfaceAdapter() = default;

    RSInterfaceAdapter(const RSInterfaceAdapter &) = delete;

    const RSInterfaceAdapter &operator=(const RSInterfaceAdapter &) = delete;

    void NotifyFrameRateEvent(const std::unordered_map<std::string, std::string>& videoRateInfo);
};

}  // namespace Rosen
}  // namespace OHOS
#endif  // !RS_INTERFACE_ADAPTER_H
