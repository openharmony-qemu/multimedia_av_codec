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
#include "accesstoken_kit.h"
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "avcodec_server_manager.h"
#include "avcodec_xcollie.h"
#include "background_event_handler.h"


namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "AVCodecServiceStub"};
}

namespace OHOS {
namespace MediaAVCodec {
using namespace OHOS::Security::AccessToken;
AVCodecServiceStub::AVCodecServiceStub()
{
    deathRecipientMap_.clear();
    avCodecListenerMap_.clear();
    registeredCallbackPid_ = -1;
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

        registeredCallbackPid_ = pid == registeredCallbackPid_ ? -1 : registeredCallbackPid_;
    }

    BackGroundEventHandler::GetInstance().ErasePid(pid);
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
        case static_cast<uint32_t>(AVCodecServiceInterfaceCode::FREEZE):
            ret = OnSuspendFreeze(data, reply);
            break;
        case static_cast<uint32_t>(AVCodecServiceInterfaceCode::ACTIVE):
            ret = OnSuspendActive(data, reply);
            break;
        case static_cast<uint32_t>(AVCodecServiceInterfaceCode::ACTIVEALL):
            ret = OnSuspendActiveAll(data, reply);
            break;
        case static_cast<uint32_t>(AVCodecServiceInterfaceCode::GET_ACTIVE_SECURE_DECODER_PIDS):
            ret = OnGetActiveSecureDecoderPids(data, reply);
            break;
        case static_cast<uint32_t>(AVCodecServiceInterfaceCode::REPORT_STATISTICS_EVENT):
            ret = AVCS_ERR_OK;
            OnReportStatisticsEvent(data, reply);
            break;
        case static_cast<uint32_t>(AVCodecServiceInterfaceCode::VIDEOCALL):
            ret = OnVideoCall(data, reply);
            break;
        case static_cast<uint32_t>(AVCodecServiceInterfaceCode::CAM_UPDATE_REGISTER_CALLBACK):
            ret = OnNotifyRegisterCallback(data, reply);
            break;
        case static_cast<uint32_t>(AVCodecServiceInterfaceCode::INIT):
            ret = OnInit(data, reply);
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

    sptr<IRemoteObject> stubObj = nullptr;
    int32_t ret = GetSubSystemAbility(id, stubObj);
    (void)reply.WriteRemoteObject(stubObj);
    (void)reply.WriteInt32(ret);

    return AVCS_ERR_OK;
}

int32_t AVCodecServiceStub::OnSuspendFreeze(MessageParcel &data, MessageParcel &reply)
{
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto tokenType = AccessTokenKit::GetTokenTypeFlag(tokenId);
    CHECK_AND_RETURN_RET_LOG(tokenType == ATokenTypeEnum::TOKEN_NATIVE || tokenType == ATokenTypeEnum::TOKEN_SHELL,
                             AVCS_ERR_INVALID_OPERATION, "Only native|shell tokens allowed(%{public}d)",
                             static_cast<int32_t>(tokenType));
    std::vector<pid_t> pidList;
    CHECK_AND_RETURN_RET_LOG(data.ReadInt32Vector(&pidList),
                             AVCS_ERR_IPC_GET_SUB_SYSTEM_ABILITY_FAILED,
                             "Failed to read pidList from MessageParcel");
    int32_t ret = SuspendFreeze(pidList);
    (void)reply.WriteInt32(ret);

    return AVCS_ERR_OK;
}
int32_t AVCodecServiceStub::OnSuspendActive(MessageParcel &data, MessageParcel &reply)
{
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto tokenType = AccessTokenKit::GetTokenTypeFlag(tokenId);
    CHECK_AND_RETURN_RET_LOG(tokenType == ATokenTypeEnum::TOKEN_NATIVE || tokenType == ATokenTypeEnum::TOKEN_SHELL,
                             AVCS_ERR_INVALID_OPERATION, "Only native|shell tokens allowed(%{public}d)",
                             static_cast<int32_t>(tokenType));
    std::vector<pid_t> pidList;
    CHECK_AND_RETURN_RET_LOG(data.ReadInt32Vector(&pidList),
                             AVCS_ERR_IPC_GET_SUB_SYSTEM_ABILITY_FAILED,
                             "Failed to read pidList from MessageParcel");
    int32_t ret = SuspendActive(pidList);
    (void)reply.WriteInt32(ret);

    return AVCS_ERR_OK;
}
int32_t AVCodecServiceStub::OnSuspendActiveAll(MessageParcel &data, MessageParcel &reply)
{
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto tokenType = AccessTokenKit::GetTokenTypeFlag(tokenId);
    CHECK_AND_RETURN_RET_LOG(tokenType == ATokenTypeEnum::TOKEN_NATIVE || tokenType == ATokenTypeEnum::TOKEN_SHELL,
                             AVCS_ERR_INVALID_OPERATION, "Only native|shell tokens allowed(%{public}d)",
                             static_cast<int32_t>(tokenType));
    int32_t ret = SuspendActiveAll();
    (void)reply.WriteInt32(ret);

    return AVCS_ERR_OK;
}

int32_t AVCodecServiceStub::OnGetActiveSecureDecoderPids(MessageParcel &data, MessageParcel &reply)
{
    std::vector<pid_t> pidList;
    int32_t ret = GetActiveSecureDecoderPids(pidList);
    (void)reply.WriteInt32Vector(pidList);
    (void)reply.WriteInt32(ret);

    return AVCS_ERR_OK;
}

void AVCodecServiceStub::OnReportStatisticsEvent(MessageParcel &data, MessageParcel &reply)
{
    uint32_t eventType = 0;
    CHECK_AND_RETURN_LOG(data.ReadUint32(eventType), "Read statistics event type failed");
    Media::Meta meta;
    CHECK_AND_RETURN_LOG(meta.FromParcel(data), "Read statistics event meta failed");

    ReportStatisticsEvent(static_cast<StatisticsEventType>(eventType), meta);
}

int32_t AVCodecServiceStub::OnVideoCall(MessageParcel &data, MessageParcel &reply)
{
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto tokenType = AccessTokenKit::GetTokenTypeFlag(tokenId);
    CHECK_AND_RETURN_RET_LOG(tokenType == ATokenTypeEnum::TOKEN_NATIVE || tokenType == ATokenTypeEnum::TOKEN_SHELL,
                             AVCS_ERR_INVALID_OPERATION, "Only native|shell tokens allowed(%{public}d)",
                             static_cast<int32_t>(tokenType));
    std::string bundleName;
    bool isFloatingWindow;
    bool readRet = data.ReadString(bundleName);
    CHECK_AND_RETURN_RET_LOG(readRet, AVCS_ERR_IPC_GET_SUB_SYSTEM_ABILITY_FAILED,
        "Failed to read bundleName from data");
    readRet = data.ReadBool(isFloatingWindow);
    CHECK_AND_RETURN_RET_LOG(readRet, AVCS_ERR_IPC_GET_SUB_SYSTEM_ABILITY_FAILED,
        "Failed to read isFloatingWindow from data");
    int32_t result = VideoCallNotifyPiPState(bundleName, isFloatingWindow);
    (void)reply.WriteInt32(result);

    return AVCS_ERR_OK;
}

int32_t AVCodecServiceStub::OnNotifyRegisterCallback(MessageParcel &data, MessageParcel &reply)
{
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto tokenType = AccessTokenKit::GetTokenTypeFlag(tokenId);
    CHECK_AND_RETURN_RET_LOG(tokenType == ATokenTypeEnum::TOKEN_NATIVE || tokenType == ATokenTypeEnum::TOKEN_SHELL,
                             AVCS_ERR_INVALID_OPERATION, "Only native|shell tokens allowed(%{public}d)",
                             static_cast<int32_t>(tokenType));
    bool camRegisterFlag{false};
    bool readRet = data.ReadBool(camRegisterFlag);
    CHECK_AND_RETURN_RET_LOG(readRet, AVCS_ERR_IPC_GET_SUB_SYSTEM_ABILITY_FAILED,
        "Failed to read camRegisterFlag from data");
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pid_t pid = IPCSkeleton::GetCallingPid();
        registeredCallbackPid_ = camRegisterFlag ? pid : -1;
    }
    int32_t ret = NotifyCamRegister(camRegisterFlag);
    (void)reply.WriteInt32(ret);
    return ret;
}

int32_t AVCodecServiceStub::NotifyListenerByProcessName(const std::string &bundleName, int32_t codecDecision)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(registeredCallbackPid_ != -1, AVCS_ERR_NO_MEMORY,
                             "There is no valid registered callback pid");
    auto it = avCodecListenerMap_.find(registeredCallbackPid_);
    CHECK_AND_RETURN_RET_LOG(it != avCodecListenerMap_.end(), AVCS_ERR_NO_MEMORY,
                             "failed to find listener for registered pid %{public}d", registeredCallbackPid_);
    auto listener = it->second;
    CHECK_AND_RETURN_RET_LOG(listener != nullptr, AVCS_ERR_NO_MEMORY,
                             "failed to get correct listener from avCodecListenerMap_");
    AVCODEC_LOGI(
        "AVCodecService notify listener: processName=%{public}s, codecDecision=%{public}d",
        bundleName.c_str(), codecDecision);
    return listener->OnOutputStrategyInfo(bundleName, codecDecision);
}

int32_t AVCodecServiceStub::Init(const sptr<IRemoteObject> &listener)
{
    return SetDeathListener(listener);
}

int32_t AVCodecServiceStub::OnInit(MessageParcel &data, MessageParcel &reply)
{
    sptr<IRemoteObject> listener = data.ReadRemoteObject();
    CHECK_AND_RETURN_RET_LOG(listener != nullptr, AVCS_ERR_IPC_GET_SUB_SYSTEM_ABILITY_FAILED,
        "Failed to read listener from data");
    int32_t ret = Init(listener);
    (void)reply.WriteInt32(ret);
    return ret;
}
} // namespace MediaAVCodec
} // namespace OHOS
