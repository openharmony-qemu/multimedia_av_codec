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
#ifndef AVCODEC_SERVICE_PROXY_H
#define AVCODEC_SERVICE_PROXY_H

#include "i_standard_avcodec_service.h"
#include "nocopyable.h"

namespace OHOS {
namespace MediaAVCodec {
class AVCodecServiceProxy : public IRemoteProxy<IStandardAVCodecService>, public NoCopyable {
public:
    explicit AVCodecServiceProxy(const sptr<IRemoteObject> &impl);
    virtual ~AVCodecServiceProxy();

    int32_t GetSubSystemAbility(IStandardAVCodecService::AVCodecSystemAbility subSystemId,
                                const sptr<IRemoteObject> &listener, sptr<IRemoteObject> &object) override;

private:
    static inline BrokerDelegator<AVCodecServiceProxy> delegator_;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // AVCODEC_SERVICE_PROXY_H
