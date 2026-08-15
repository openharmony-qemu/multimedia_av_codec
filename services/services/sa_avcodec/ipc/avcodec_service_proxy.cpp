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
                                                 sptr<IRemoteObject> &object)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool ret = data.WriteInterfaceToken(AVCodecServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(ret, AVCS_ERR_IPC_GET_SUB_SYSTEM_ABILITY_FAILED, "Failed to write descriptor");

    (void)data.WriteInt32(static_cast<int32_t>(subSystemId));
    int error =
        Remote()->SendRequest(static_cast<uint32_t>(AVCodecServiceInterfaceCode::GET_SUBSYSTEM), data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == 0, AVCS_ERR_IPC_GET_SUB_SYSTEM_ABILITY_FAILED,
        "Create av_codec proxy failed, error: %{public}d", error);

    object = reply.ReadRemoteObject();
    CHECK_AND_RETURN_RET_LOG(object != nullptr, AVCS_ERR_IPC_GET_SUB_SYSTEM_ABILITY_FAILED, "Remote object is nullptr");
    return reply.ReadInt32();
}

int32_t AVCodecServiceProxy::SuspendFreeze(const std::vector<pid_t> &pidList)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool ret = data.WriteInterfaceToken(AVCodecServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(ret, AVCS_ERR_IPC_GET_SUB_SYSTEM_ABILITY_FAILED, "Failed to write descriptor");

    (void)data.WriteInt32Vector(pidList);
    int error =
        Remote()->SendRequest(static_cast<uint32_t>(AVCodecServiceInterfaceCode::FREEZE), data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == 0, AVCS_ERR_IPC_GET_SUB_SYSTEM_ABILITY_FAILED,
        "Create av_codec proxy failed, error: %{public}d", error);
    return reply.ReadInt32();
}

int32_t AVCodecServiceProxy::SuspendActive(const std::vector<pid_t> &pidList)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool ret = data.WriteInterfaceToken(AVCodecServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(ret, AVCS_ERR_IPC_GET_SUB_SYSTEM_ABILITY_FAILED, "Failed to write descriptor");

    (void)data.WriteInt32Vector(pidList);
    int error =
        Remote()->SendRequest(static_cast<uint32_t>(AVCodecServiceInterfaceCode::ACTIVE), data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == 0, AVCS_ERR_IPC_GET_SUB_SYSTEM_ABILITY_FAILED,
        "Create av_codec proxy failed, error: %{public}d", error);
    return reply.ReadInt32();
}

int32_t AVCodecServiceProxy::SuspendActiveAll()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool ret = data.WriteInterfaceToken(AVCodecServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(ret, AVCS_ERR_IPC_GET_SUB_SYSTEM_ABILITY_FAILED, "Failed to write descriptor");

    int error =
        Remote()->SendRequest(static_cast<uint32_t>(AVCodecServiceInterfaceCode::ACTIVEALL), data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == 0, AVCS_ERR_IPC_GET_SUB_SYSTEM_ABILITY_FAILED,
        "Create av_codec proxy failed, error: %{public}d", error);
    return reply.ReadInt32();
}

int32_t AVCodecServiceProxy::VideoCallNotifyPiPState(const std::string &bundleName, const bool isFloatingWindow)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool ret = data.WriteInterfaceToken(AVCodecServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(ret, AVCS_ERR_IPC_GET_SUB_SYSTEM_ABILITY_FAILED, "Failed to write descriptor");
    ret = data.WriteString(bundleName);
    CHECK_AND_RETURN_RET_LOG(ret, AVCS_ERR_IPC_GET_SUB_SYSTEM_ABILITY_FAILED, "Failed to write bundleName");
    ret = data.WriteBool(isFloatingWindow);
    CHECK_AND_RETURN_RET_LOG(ret, AVCS_ERR_IPC_GET_SUB_SYSTEM_ABILITY_FAILED, "Failed to write isFloatingWindow");
    int error =
        Remote()->SendRequest(static_cast<uint32_t>(AVCodecServiceInterfaceCode::VIDEOCALL), data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == 0, AVCS_ERR_IPC_GET_SUB_SYSTEM_ABILITY_FAILED,
        "SendRequest for VideoCallNotifyPiPState failed, error: %{public}d", error);
    return reply.ReadInt32();
}

int32_t AVCodecServiceProxy::NotifyCamRegister(const bool camRegisterFlag)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool ret = data.WriteInterfaceToken(AVCodecServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(ret, AVCS_ERR_IPC_GET_SUB_SYSTEM_ABILITY_FAILED, "Failed to write descriptor");
    ret = data.WriteBool(camRegisterFlag);
    CHECK_AND_RETURN_RET_LOG(ret, AVCS_ERR_IPC_GET_SUB_SYSTEM_ABILITY_FAILED, "Failed to write camRegisterFlag");
    int error =
        Remote()->SendRequest(static_cast<uint32_t>(AVCodecServiceInterfaceCode::CAM_UPDATE_REGISTER_CALLBACK),
                              data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == 0, AVCS_ERR_IPC_GET_SUB_SYSTEM_ABILITY_FAILED,
        "SendRequest for NotifyCamRegister failed, error: %{public}d", error);
    return reply.ReadInt32();
}

int32_t AVCodecServiceProxy::Init(const sptr<IRemoteObject> &listener)
{
    CHECK_AND_RETURN_RET_LOG(listener != nullptr, AVCS_ERR_IPC_GET_SUB_SYSTEM_ABILITY_FAILED, "listener is nullptr");

    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool ret = data.WriteInterfaceToken(AVCodecServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(ret, AVCS_ERR_IPC_GET_SUB_SYSTEM_ABILITY_FAILED, "Failed to write descriptor");
    ret = data.WriteRemoteObject(listener);
    CHECK_AND_RETURN_RET_LOG(ret, AVCS_ERR_IPC_GET_SUB_SYSTEM_ABILITY_FAILED, "Failed to write listener");
    int error =
        Remote()->SendRequest(static_cast<uint32_t>(AVCodecServiceInterfaceCode::INIT), data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == 0, AVCS_ERR_IPC_GET_SUB_SYSTEM_ABILITY_FAILED,
        "Init listener failed, error: %{public}d", error);
    return reply.ReadInt32();
}

int32_t AVCodecServiceProxy::GetActiveSecureDecoderPids(std::vector<pid_t> &pidList)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    
    bool ret = data.WriteInterfaceToken(AVCodecServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(ret, AVCS_ERR_IPC_GET_SUB_SYSTEM_ABILITY_FAILED, "Failed to write descriptor");

    int error = Remote()->SendRequest(static_cast<uint32_t>(
        AVCodecServiceInterfaceCode::GET_ACTIVE_SECURE_DECODER_PIDS), data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == 0, AVCS_ERR_IPC_GET_SUB_SYSTEM_ABILITY_FAILED,
        "Create av_codec proxy failed, error: %{public}d", error);

    (void)reply.ReadInt32Vector(&pidList);
    return reply.ReadInt32();
}

int32_t AVCodecServiceProxy::SetCodecPerformanceWithPid(pid_t pid, CodecPerformanceLevel level)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool ret = data.WriteInterfaceToken(AVCodecServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(ret, AVCS_ERR_IPC_GET_SUB_SYSTEM_ABILITY_FAILED, "Failed to write descriptor");

    ret = data.WriteInt32(pid);
    CHECK_AND_RETURN_RET_LOG(ret, AVCS_ERR_IPC_GET_SUB_SYSTEM_ABILITY_FAILED, "Failed to write pid");
    ret = data.WriteUint8(static_cast<uint8_t>(level));
    CHECK_AND_RETURN_RET_LOG(ret, AVCS_ERR_IPC_GET_SUB_SYSTEM_ABILITY_FAILED, "Failed to write level");
    int error = Remote()->SendRequest(static_cast<uint32_t>(
        AVCodecServiceInterfaceCode::SET_CODEC_PERFORMANCE_WITH_PID), data, reply, option);
    CHECK_AND_RETURN_RET_LOG(error == 0, AVCS_ERR_IPC_GET_SUB_SYSTEM_ABILITY_FAILED,
                             "SetCodecPerformanceWithPid failed, error: %{public}d", error);
    return reply.ReadInt32();
}
} // namespace MediaAVCodec
} // namespace OHOS
