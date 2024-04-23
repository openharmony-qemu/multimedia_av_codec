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
#include "avcodec_service_stub.h"
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "avcodec_server_manager.h"
#include "avcodec_xcollie.h"


namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "AVCodecServiceStub"};
}

namespace OHOS {
namespace MediaAVCodec {
AVCodecServiceStub::AVCodecServiceStub()
{
    deathRecipientMap_.clear();
    avCodecListenerMap_.clear();
    InitStub();
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AVCodecServiceStub::~AVCodecServiceStub()
{
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

void AVCodecServiceStub::InitStub() {}

int32_t AVCodecServiceStub::DestroyStubForPid(pid_t pid)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        sptr<AVCodecDeathRecipient> deathRecipient = nullptr;
        sptr<IStandardAVCodecListener> avCodecListener = nullptr;

        auto itDeath = deathRecipientMap_.find(pid);
        if (itDeath != deathRecipientMap_.end()) {
            deathRecipient = itDeath->second;

            if (deathRecipient != nullptr) {
                deathRecipient->SetNotifyCb(nullptr);
            }

            (void)deathRecipientMap_.erase(itDeath);
        }

        auto itListener = avCodecListenerMap_.find(pid);
        if (itListener != avCodecListenerMap_.end()) {
            avCodecListener = itListener->second;

            if (avCodecListener != nullptr && avCodecListener->AsObject() != nullptr && deathRecipient != nullptr) {
                (void)avCodecListener->AsObject()->RemoveDeathRecipient(deathRecipient);
            }

            (void)avCodecListenerMap_.erase(itListener);
        }
    }

    AVCodecServerManager::GetInstance().DestroyStubObjectForPid(pid);
    return AVCS_ERR_OK;
}

int AVCodecServiceStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    AVCODEC_LOGD("Stub: OnRemoteRequest of code: %{public}u is received", code);

    auto remoteDescriptor = data.ReadInterfaceToken();
    if (AVCodecServiceStub::GetDescriptor() != remoteDescriptor) {
        AVCODEC_LOGE("Invalid descriptor");
        return AVCS_ERR_INVALID_OPERATION;
    }
    int32_t ret = -1;
    switch (code) {
        case static_cast<uint32_t>(AVCodecServiceInterfaceCode::GET_SUBSYSTEM):
            ret = GetSystemAbility(data, reply);
            break;
        default:
            AVCODEC_LOGW("AVCodecServiceStub: no member func supporting, applying default process");
            return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
    }
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Failed to call member func GetSystemAbility");
    return ret;
}

void AVCodecServiceStub::ClientDied(pid_t pid)
{
    AVCODEC_LOGE("client is dead, pid:%{public}d", pid);
    (void)DestroyStubForPid(pid);
}

int32_t AVCodecServiceStub::SetDeathListener(const sptr<IRemoteObject> &object)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(object != nullptr, AVCS_ERR_NO_MEMORY, "set listener object is nullptr");

    pid_t pid = IPCSkeleton::GetCallingPid();
    for (auto it = avCodecListenerMap_.begin(); it != avCodecListenerMap_.end(); ++it) {
        if (it->first == pid) {
            AVCODEC_LOGD("client pid already exits");
            return AVCS_ERR_OK;
        }
    }
    sptr<IStandardAVCodecListener> avCodecListener = iface_cast<IStandardAVCodecListener>(object);
    CHECK_AND_RETURN_RET_LOG(avCodecListener != nullptr, AVCS_ERR_NO_MEMORY,
                             "failed to convert IStandardAVCodecListener");

    sptr<AVCodecDeathRecipient> deathRecipient = new (std::nothrow) AVCodecDeathRecipient(pid);
    CHECK_AND_RETURN_RET_LOG(deathRecipient != nullptr, AVCS_ERR_NO_MEMORY, "failed to new AVCodecDeathRecipient");

    deathRecipient->SetNotifyCb(std::bind(&AVCodecServiceStub::ClientDied, this, std::placeholders::_1));

    if (avCodecListener->AsObject() != nullptr) {
        (void)avCodecListener->AsObject()->AddDeathRecipient(deathRecipient);
    }

    AVCODEC_LOGD("client pid pid:%{public}d", pid);
    avCodecListenerMap_[pid] = avCodecListener;
    deathRecipientMap_[pid] = deathRecipient;
    return AVCS_ERR_OK;
}

int32_t AVCodecServiceStub::GetSystemAbility(MessageParcel &data, MessageParcel &reply)
{
    AVCodecSystemAbility id = static_cast<AVCodecSystemAbility>(data.ReadInt32());
    sptr<IRemoteObject> listenerObj = data.ReadRemoteObject();

    sptr<IRemoteObject> stubObj = nullptr;
    int32_t ret = GetSubSystemAbility(id, listenerObj, stubObj);
    (void)reply.WriteRemoteObject(stubObj);
    (void)reply.WriteInt32(ret);
    
    return AVCS_ERR_OK;
}
} // namespace MediaAVCodec
} // namespace OHOS
