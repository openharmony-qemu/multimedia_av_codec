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
#ifndef AVCODEC_SERVICE_STUB_H
#define AVCODEC_SERVICE_STUB_H

#include <map>
#include "i_standard_avcodec_service.h"
#include "i_standard_avcodec_listener.h"
#include "avcodec_death_recipient.h"
#include "nocopyable.h"

namespace OHOS {
namespace MediaAVCodec {
class AVCodecServiceStub : public IRemoteStub<IStandardAVCodecService>, public NoCopyable {
public:
    AVCodecServiceStub();
    virtual ~AVCodecServiceStub();
    int OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override;

protected:
    int32_t SetDeathListener(const sptr<IRemoteObject> &object);

private:
    void InitStub();
    int32_t GetSystemAbility(MessageParcel &data, MessageParcel &reply);
    void ClientDied(pid_t pid);
    int32_t DestroyStubForPid(pid_t pid);

    std::map<pid_t, sptr<AVCodecDeathRecipient>> deathRecipientMap_;
    std::map<pid_t, sptr<IStandardAVCodecListener>> avCodecListenerMap_;
    std::mutex mutex_;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // AVCODEC_SERVICE_STUB_H
