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
#include "avcodec_client.h"
#include "avcodec_xcollie.h"
#include "ipc_skeleton.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"
#include "avcodec_trace.h"
#include "avcodec_sysevent.h"

#ifdef SUPPORT_CODEC
#include "i_standard_codec_service.h"
#endif
#ifdef SUPPORT_CODECLIST
#include "i_standard_codeclist_service.h"
#endif

#include "avcodec_errors.h"
#include "avcodec_log.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "AVCodecClient"};
}

namespace OHOS {
namespace MediaAVCodec {
static AVCodecClient g_avCodecClientInstance;
IAVCodecService &AVCodecServiceFactory::GetInstance()
{
    return g_avCodecClientInstance;
}

AVCodecClient::AVCodecClient() noexcept
{
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AVCodecClient::~AVCodecClient()
{
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

bool AVCodecClient::IsAlived()
{
    if (avCodecProxy_ == nullptr) {
        avCodecProxy_ = GetAVCodecProxy();
    }

    return avCodecProxy_ != nullptr;
}
#ifdef SUPPORT_CODEC
int32_t AVCodecClient::CreateCodecService(std::shared_ptr<ICodecService> &codecClient)
{
    std::lock_guard<std::mutex> lock(mutex_);
    bool alived = IsAlived();
    CHECK_AND_RETURN_RET_LOG(alived, AVCS_ERR_SERVICE_DIED, "AVCodec service does not exist.");

    sptr<IRemoteObject> object = nullptr;
    int32_t ret = avCodecProxy_->GetSubSystemAbility(
        IStandardAVCodecService::AVCodecSystemAbility::AVCODEC_CODEC, listenerStub_->AsObject(), object);
    CHECK_AND_RETURN_RET_LOG(object != nullptr, ret, "Create codec proxy object failed.");

    sptr<IStandardCodecService> codecProxy = iface_cast<IStandardCodecService>(object);
    CHECK_AND_RETURN_RET_LOG(codecProxy != nullptr, AVCS_ERR_UNSUPPORT, "Codec proxy is nullptr.");

    ret = CodecClient::Create(codecProxy, codecClient);
    CHECK_AND_RETURN_RET_LOG(codecClient != nullptr, ret, "Failed to create codec client.");

    codecClientList_.push_back(codecClient);
    return AVCS_ERR_OK;
}

int32_t AVCodecClient::DestroyCodecService(std::shared_ptr<ICodecService> codecClient)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecClient != nullptr, AVCS_ERR_NO_MEMORY, "codec client is nullptr.");
    codecClientList_.remove(codecClient);
    return AVCS_ERR_OK;
}
#endif
#ifdef SUPPORT_CODECLIST
std::shared_ptr<ICodecListService> AVCodecClient::CreateCodecListService()
{
    std::lock_guard<std::mutex> lock(mutex_);
    bool alived = IsAlived();
    CHECK_AND_RETURN_RET_LOG(alived, nullptr, "AVCodec service does not exist.");

    sptr<IRemoteObject> object = nullptr;
    (void)avCodecProxy_->GetSubSystemAbility(
        IStandardAVCodecService::AVCodecSystemAbility::AVCODEC_CODECLIST, listenerStub_->AsObject(), object);
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "Create codeclist proxy object failed.");

    sptr<IStandardCodecListService> codecListProxy = iface_cast<IStandardCodecListService>(object);
    CHECK_AND_RETURN_RET_LOG(codecListProxy != nullptr, nullptr, "codeclist proxy is nullptr.");

    std::shared_ptr<CodecListClient> codecListClient = CodecListClient::Create(codecListProxy);
    CHECK_AND_RETURN_RET_LOG(codecListClient != nullptr, nullptr, "failed to create codeclist client.");

    codecListClientList_.push_back(codecListClient);
    return codecListClient;
}

int32_t AVCodecClient::DestroyCodecListService(std::shared_ptr<ICodecListService> codecListClient)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecListClient != nullptr, AVCS_ERR_NO_MEMORY, "codeclist client is nullptr.");
    codecListClientList_.remove(codecListClient);
    return AVCS_ERR_OK;
}
#endif

sptr<IStandardAVCodecService> AVCodecClient::GetAVCodecProxy()
{
    AVCODEC_LOGD("enter");
    sptr<ISystemAbilityManager> samgr = nullptr;
    CLIENT_COLLIE_LISTEN(samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager(),
        "AVCodecClient GetAVCodecProxy");
    CHECK_AND_RETURN_RET_LOG(samgr != nullptr, nullptr, "system ability manager is nullptr.");

    sptr<IRemoteObject> object = nullptr;
    CLIENT_COLLIE_LISTEN(object = samgr->GetSystemAbility(OHOS::AV_CODEC_SERVICE_ID), "AVCodecClient GetAVCodecProxy");
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "avcodec object is nullptr.");

    avCodecProxy_ = iface_cast<IStandardAVCodecService>(object);
    CHECK_AND_RETURN_RET_LOG(avCodecProxy_ != nullptr, nullptr, "avcodec proxy is nullptr.");

    pid_t pid = 0;
    deathRecipient_ = new (std::nothrow) AVCodecDeathRecipient(pid);
    CHECK_AND_RETURN_RET_LOG(deathRecipient_ != nullptr, nullptr, "failed to new AVCodecDeathRecipient.");

    deathRecipient_->SetNotifyCb(std::bind(&AVCodecClient::AVCodecServerDied, std::placeholders::_1));
    bool result = object->AddDeathRecipient(deathRecipient_);
    CHECK_AND_RETURN_RET_LOG(result, nullptr, "Failed to add deathRecipient");

    listenerStub_ = new (std::nothrow) AVCodecListenerStub();
    CHECK_AND_RETURN_RET_LOG(listenerStub_ != nullptr, nullptr, "failed to new AVCodecListenerStub");
    return avCodecProxy_;
}

void AVCodecClient::AVCodecServerDied(pid_t pid)
{
    AVCODEC_LOGE("AVCodec service is died, pid:%{public}d!", pid);
    g_avCodecClientInstance.DoAVCodecServerDied();
    FaultEventWrite(FaultType::FAULT_TYPE_CRASH, "AVCodec service is died", "AVCodecClient");
}

void AVCodecClient::DoAVCodecServerDied()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (avCodecProxy_ != nullptr) {
        (void)avCodecProxy_->AsObject()->RemoveDeathRecipient(deathRecipient_);
        avCodecProxy_ = nullptr;
    }
    listenerStub_ = nullptr;
    deathRecipient_ = nullptr;

#ifdef SUPPORT_CODEC
    for (auto &it : codecClientList_) {
        auto codecClient = std::static_pointer_cast<CodecClient>(it);
        if (codecClient != nullptr) {
            codecClient->AVCodecServerDied();
        }
    }
#endif
#ifdef SUPPORT_CODECLIST
    for (auto &it : codecListClientList_) {
        auto codecListClient = std::static_pointer_cast<CodecListClient>(it);
        if (codecListClient != nullptr) {
            codecListClient->AVCodecServerDied();
        }
    }
#endif
}
} // namespace MediaAVCodec
} // namespace OHOS
