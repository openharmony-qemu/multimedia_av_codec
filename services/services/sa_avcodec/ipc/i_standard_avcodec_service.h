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
#ifndef I_STANDARD_AVCODEC_SERVICE_H
#define I_STANDARD_AVCODEC_SERVICE_H

#include "av_codec_service_ipc_interface_code.h"
#include "ipc_types.h"
#include "iremote_broker.h"
#include "iremote_proxy.h"
#include "iremote_stub.h"

namespace OHOS {
namespace MediaAVCodec {
class IStandardAVCodecService : public IRemoteBroker {
public:
    /**
     * sub system ability ID
     */
    enum AVCodecSystemAbility : int32_t {
        AVCODEC_CODECLIST,
        AVCODEC_CODEC,
    };

    virtual int32_t GetSubSystemAbility(IStandardAVCodecService::AVCodecSystemAbility subSystemId,
        const sptr<IRemoteObject> &listener, sptr<IRemoteObject> &stubObject) = 0;

    DECLARE_INTERFACE_DESCRIPTOR(u"IStandardAVCodecServiceInterface");
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // I_STANDARD_AVCODEC_SERVICE_H
