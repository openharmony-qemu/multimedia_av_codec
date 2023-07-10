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
#include "avcodec_service_proxy.h"
#include "avcodec_errors.h"
#include "avcodec_log.h"


namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVCodecServiceProxy"};
}

namespace OHOS {
namespace MediaAVCodec {
AVCodecServiceProxy::AVCodecServiceProxy(const sptr<IRemoteObject> &impl) : IRemoteProxy<IStandardAVCodecService>(impl)
{
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AVCodecServiceProxy::~AVCodecServiceProxy()
{
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

sptr<IRemoteObject> AVCodecServiceProxy::GetSubSystemAbility(IStandardAVCodecService::AVCodecSystemAbility subSystemId,
                                                             const sptr<IRemoteObject> &listener)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (listener == nullptr) {
        AVCODEC_LOGE("listener is nullptr");
        return nullptr;
    }

    if (!data.WriteInterfaceToken(AVCodecServiceProxy::GetDescriptor())) {
        AVCODEC_LOGE("Failed to write descriptor");
        return nullptr;
    }

    (void)data.WriteInt32(static_cast<int32_t>(subSystemId));
    (void)data.WriteRemoteObject(listener);
    int error =
        Remote()->SendRequest(static_cast<uint32_t>(AVCodecServiceInterfaceCode::GET_SUBSYSTEM), data, reply, option);
    if (error != AVCS_ERR_OK) {
        AVCODEC_LOGE("Create av_codec proxy failed, error: %{public}d", error);
        return nullptr;
    }

    return reply.ReadRemoteObject();
}
} // namespace MediaAVCodec
} // namespace OHOS
