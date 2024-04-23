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
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "AVCodecServiceProxy"};
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

int32_t AVCodecServiceProxy::GetSubSystemAbility(IStandardAVCodecService::AVCodecSystemAbility subSystemId,
                                                 const sptr<IRemoteObject> &listener, sptr<IRemoteObject> &object)
{
    CHECK_AND_RETURN_RET_LOG(listener != nullptr, AVCE_ERR_IPC_GET_SUB_SYSTEM_ABILITY_FAILED, "listener is nullptr");

    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool ret = data.WriteInterfaceToken(AVCodecServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(ret, AVCE_ERR_IPC_GET_SUB_SYSTEM_ABILITY_FAILED, "Failed to write descriptor");

    (void)data.WriteInt32(static_cast<int32_t>(subSystemId));
    (void)data.WriteRemoteObject(listener);
    int error =
        Remote()->SendRequest(static_cast<uint32_t>(AVCodecServiceInterfaceCode::GET_SUBSYSTEM), data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == 0, AVCE_ERR_IPC_GET_SUB_SYSTEM_ABILITY_FAILED,
        "Create av_codec proxy failed, error: %{public}d", error);

    object = reply.ReadRemoteObject();
    CHECK_AND_RETURN_RET_LOG(object != nullptr, AVCE_ERR_IPC_GET_SUB_SYSTEM_ABILITY_FAILED, "Remote object is nullptr");
    return reply.ReadInt32();
}
} // namespace MediaAVCodec
} // namespace OHOS
