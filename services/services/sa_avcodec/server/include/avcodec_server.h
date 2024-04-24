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

#ifndef AVCODEC_SERVER_H
#define AVCODEC_SERVER_H

#include "avcodec_server_manager.h"
#include "avcodec_service_stub.h"
#include "nocopyable.h"
#include "system_ability.h"


namespace OHOS {
namespace MediaAVCodec {
class AVCodecServer : public SystemAbility, public AVCodecServiceStub {
    DECLARE_SYSTEM_ABILITY(AVCodecServer);
public:
    explicit AVCodecServer(int32_t systemAbilityId, bool runOnCreate = true);
    ~AVCodecServer();

    // IStandardAVCodecService override
    int32_t GetSubSystemAbility(IStandardAVCodecService::AVCodecSystemAbility subSystemId,
                                const sptr<IRemoteObject> &listener, sptr<IRemoteObject> &stubObject) override;

protected:
    // SystemAbility override
    void OnDump() override;
    void OnStart() override;
    void OnStop() override;
    std::optional<AVCodecServerManager::StubType> SwitchSystemId(
        IStandardAVCodecService::AVCodecSystemAbility subSystemId);
    int32_t Dump(int32_t fd, const std::vector<std::u16string> &args) override;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // AVCODEC_SERVER_H