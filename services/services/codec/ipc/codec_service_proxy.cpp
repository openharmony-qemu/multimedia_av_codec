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

#include "codec_service_proxy.h"
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "avcodec_parcel.h"
#include "avsharedmemory_ipc.h"
#include "buffer_client_producer.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "CodecServiceProxy"};
}

namespace OHOS {
namespace MediaAVCodec {
using namespace Media;
CodecServiceProxy::CodecServiceProxy(const sptr<IRemoteObject> &impl) : IRemoteProxy<IStandardCodecService>(impl)
{
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

CodecServiceProxy::~CodecServiceProxy()
{
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t CodecServiceProxy::SetListenerObject(const sptr<IRemoteObject> &object)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(CodecServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, AVCS_ERR_INVALID_OPERATION, "Write descriptor failed!");

    (void)data.WriteRemoteObject(object);
    int32_t ret =
        Remote()->SendRequest(static_cast<uint32_t>(CodecServiceInterfaceCode::SET_LISTENER_OBJ), data, reply, option);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_INVALID_OPERATION, "Send request failed");

    return reply.ReadInt32();
}

void CodecServiceProxy::SetListener(const sptr<CodecListenerStub> &listener)
{
    listener_ = listener;
}

int32_t CodecServiceProxy::Init(AVCodecType type, bool isMimeType, const std::string &name, Meta &callerInfo)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(CodecServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, AVCS_ERR_INVALID_OPERATION, "Write descriptor failed!");

    callerInfo.ToParcel(data);
    data.WriteInt32(static_cast<int32_t>(type));
    data.WriteBool(isMimeType);
    data.WriteString(name);
    int32_t ret = Remote()->SendRequest(static_cast<uint32_t>(CodecServiceInterfaceCode::INIT), data, reply, option);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_INVALID_OPERATION, "Send request failed");

    return reply.ReadInt32();
}

int32_t CodecServiceProxy::Configure(const Format &format)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(CodecServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, AVCS_ERR_INVALID_OPERATION, "Write descriptor failed!");

    (void)AVCodecParcel::Marshalling(data, format);
    int32_t ret =
        Remote()->SendRequest(static_cast<uint32_t>(CodecServiceInterfaceCode::CONFIGURE), data, reply, option);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_INVALID_OPERATION, "Send request failed");

    return reply.ReadInt32();
}

int32_t CodecServiceProxy::Start()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(CodecServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, AVCS_ERR_INVALID_OPERATION, "Write descriptor failed!");

    int32_t ret = Remote()->SendRequest(static_cast<uint32_t>(CodecServiceInterfaceCode::START), data, reply, option);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_INVALID_OPERATION, "Send request failed");

    return reply.ReadInt32();
}

int32_t CodecServiceProxy::Stop()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(CodecServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, AVCS_ERR_INVALID_OPERATION, "Write descriptor failed!");

    int32_t ret = Remote()->SendRequest(static_cast<uint32_t>(CodecServiceInterfaceCode::STOP), data, reply, option);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_INVALID_OPERATION, "Send request failed");

    return reply.ReadInt32();
}

int32_t CodecServiceProxy::Flush()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(CodecServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, AVCS_ERR_INVALID_OPERATION, "Write descriptor failed!");

    int32_t ret = Remote()->SendRequest(static_cast<uint32_t>(CodecServiceInterfaceCode::FLUSH), data, reply, option);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_INVALID_OPERATION, "Send request failed");

    return reply.ReadInt32();
}

int32_t CodecServiceProxy::NotifyEos()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(CodecServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, AVCS_ERR_INVALID_OPERATION, "Write descriptor failed!");

    int32_t ret =
        Remote()->SendRequest(static_cast<uint32_t>(CodecServiceInterfaceCode::NOTIFY_EOS), data, reply, option);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_INVALID_OPERATION, "Send request failed");

    return reply.ReadInt32();
}

int32_t CodecServiceProxy::Reset()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(CodecServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, AVCS_ERR_INVALID_OPERATION, "Write descriptor failed!");

    int32_t ret = Remote()->SendRequest(static_cast<uint32_t>(CodecServiceInterfaceCode::RESET), data, reply, option);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_INVALID_OPERATION, "Send request failed");
    static_cast<CodecListenerStub *>(listener_.GetRefPtr())->ClearListenerCache();
    return reply.ReadInt32();
}

int32_t CodecServiceProxy::Release()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(CodecServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, AVCS_ERR_INVALID_OPERATION, "Write descriptor failed!");

    int32_t ret = Remote()->SendRequest(static_cast<uint32_t>(CodecServiceInterfaceCode::RELEASE), data, reply, option);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_INVALID_OPERATION, "Send request failed");

    return reply.ReadInt32();
}

sptr<OHOS::Surface> CodecServiceProxy::CreateInputSurface()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(CodecServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, nullptr, "Write descriptor failed!");

    int32_t ret = Remote()->SendRequest(static_cast<uint32_t>(CodecServiceInterfaceCode::CREATE_INPUT_SURFACE), data,
                                        reply, option);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, nullptr, "Send request failed");

    if (reply.ReadInt32() != AVCS_ERR_OK) {
        return nullptr;
    }
    sptr<IRemoteObject> object = reply.ReadRemoteObject();
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "Read surface object failed");

    sptr<IBufferProducer> producer(new BufferClientProducer(object));
    CHECK_AND_RETURN_RET_LOG(producer != nullptr, nullptr, "Convert object to producer failed");

    return OHOS::Surface::CreateSurfaceAsProducer(producer);
}

int32_t CodecServiceProxy::SetOutputSurface(sptr<OHOS::Surface> surface)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    CHECK_AND_RETURN_RET_LOG(surface != nullptr, AVCS_ERR_NO_MEMORY, "Surface is nullptr");
    sptr<IBufferProducer> producer = surface->GetProducer();
    CHECK_AND_RETURN_RET_LOG(producer != nullptr, AVCS_ERR_NO_MEMORY, "Producer is nullptr");

    sptr<IRemoteObject> object = producer->AsObject();
    CHECK_AND_RETURN_RET_LOG(object != nullptr, AVCS_ERR_NO_MEMORY, "Object is nullptr");

    const std::string surfaceFormat = "SURFACE_FORMAT";
    std::string format = surface->GetUserData(surfaceFormat);
    AVCODEC_LOGD("Surface format is %{public}s!", format.c_str());

    bool token = data.WriteInterfaceToken(CodecServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, AVCS_ERR_INVALID_OPERATION, "Write descriptor failed!");

    (void)data.WriteRemoteObject(object);
    data.WriteString(format);
    int32_t ret = Remote()->SendRequest(static_cast<uint32_t>(CodecServiceInterfaceCode::SET_OUTPUT_SURFACE), data,
                                        reply, option);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_INVALID_OPERATION, "Send request failed");

    return reply.ReadInt32();
}

int32_t CodecServiceProxy::QueueInputBuffer(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(CodecServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, AVCS_ERR_INVALID_OPERATION, "Write descriptor failed!");

    data.WriteUint32(index);

    bool retWrite =
        static_cast<CodecListenerStub *>(listener_.GetRefPtr())->WriteInputMemoryToParcel(index, info, flag, data);
    CHECK_AND_RETURN_RET_LOG(retWrite == true, AVCS_ERR_INVALID_STATE, "Listener write data failed");

    int32_t ret = Remote()->SendRequest(static_cast<uint32_t>(CodecServiceInterfaceCode::QUEUE_INPUT_BUFFER), data,
                                        reply, option);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_INVALID_OPERATION, "Send request failed");

    return reply.ReadInt32();
}

int32_t CodecServiceProxy::QueueInputBuffer(uint32_t index)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(CodecServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, AVCS_ERR_INVALID_OPERATION, "Write descriptor failed!");

    data.WriteUint32(index);

    bool retWrite = static_cast<CodecListenerStub *>(listener_.GetRefPtr())->WriteInputBufferToParcel(index, data);
    CHECK_AND_RETURN_RET_LOG(retWrite == true, AVCS_ERR_INVALID_OPERATION, "Listener write data failed");

    int32_t ret = Remote()->SendRequest(static_cast<uint32_t>(CodecServiceInterfaceCode::QUEUE_INPUT_BUFFER), data,
                                        reply, option);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_INVALID_OPERATION, "Send request failed");

    return reply.ReadInt32();
}

int32_t CodecServiceProxy::QueueInputParameter(uint32_t index)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(CodecServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, AVCS_ERR_INVALID_OPERATION, "Write descriptor failed!");

    data.WriteUint32(index);

    bool retWrite = static_cast<CodecListenerStub *>(listener_.GetRefPtr())->WriteInputParameterToParcel(index, data);
    CHECK_AND_RETURN_RET_LOG(retWrite == true, AVCS_ERR_INVALID_OPERATION, "Listener write data failed");

    int32_t ret = Remote()->SendRequest(static_cast<uint32_t>(CodecServiceInterfaceCode::QUEUE_INPUT_BUFFER), data,
                                        reply, option);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_INVALID_OPERATION, "Send request failed");

    return reply.ReadInt32();
}

int32_t CodecServiceProxy::GetOutputFormat(Format &format)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(CodecServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, AVCS_ERR_INVALID_OPERATION, "Write descriptor failed!");

    int32_t ret =
        Remote()->SendRequest(static_cast<uint32_t>(CodecServiceInterfaceCode::GET_OUTPUT_FORMAT), data, reply, option);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_INVALID_OPERATION, "Send request failed");

    (void)AVCodecParcel::Unmarshalling(reply, format);
    return AVCS_ERR_OK;
}

int32_t CodecServiceProxy::ReleaseOutputBuffer(uint32_t index, bool render)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(CodecServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, AVCS_ERR_INVALID_OPERATION, "Write descriptor failed!");

    data.WriteUint32(index);
    data.WriteBool(render);
    int32_t ret = Remote()->SendRequest(static_cast<uint32_t>(CodecServiceInterfaceCode::RELEASE_OUTPUT_BUFFER), data,
                                        reply, option);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_INVALID_OPERATION, "Send request failed");

    return reply.ReadInt32();
}

int32_t CodecServiceProxy::SetParameter(const Format &format)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(CodecServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, AVCS_ERR_INVALID_OPERATION, "Write descriptor failed!");

    (void)AVCodecParcel::Marshalling(data, format);
    int32_t ret =
        Remote()->SendRequest(static_cast<uint32_t>(CodecServiceInterfaceCode::SET_PARAMETER), data, reply, option);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_INVALID_OPERATION, "Send request failed");

    return reply.ReadInt32();
}

int32_t CodecServiceProxy::GetInputFormat(Format &format)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(CodecServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, AVCS_ERR_INVALID_OPERATION, "Write descriptor failed!");

    int32_t ret =
        Remote()->SendRequest(static_cast<uint32_t>(CodecServiceInterfaceCode::GET_INPUT_FORMAT), data, reply, option);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_INVALID_OPERATION, "Send request failed");

    (void)AVCodecParcel::Unmarshalling(reply, format);
    return AVCS_ERR_OK;
}

int32_t CodecServiceProxy::DestroyStub()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(CodecServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, AVCS_ERR_INVALID_OPERATION, "Write descriptor failed!");

    int32_t ret =
        Remote()->SendRequest(static_cast<uint32_t>(CodecServiceInterfaceCode::DESTROY_STUB), data, reply, option);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_INVALID_OPERATION, "Send request failed");

    static_cast<CodecListenerStub *>(listener_.GetRefPtr())->ClearListenerCache();
    return reply.ReadInt32();
}

#ifdef SUPPORT_DRM
int32_t CodecServiceProxy::SetDecryptConfig(const sptr<DrmStandard::IMediaKeySessionService> &keySession,
                                            const bool svpFlag)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(CodecServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, AVCS_ERR_INVALID_OPERATION, "Write descriptor failed!");

    CHECK_AND_RETURN_RET_LOG(keySession != nullptr, AVCS_ERR_INVALID_VAL, "keySession nullptr failed!");
    sptr<IRemoteObject> object = keySession->AsObject();
    CHECK_AND_RETURN_RET_LOG(object != nullptr, AVCS_ERR_INVALID_VAL, "keySessionProxy object is null");
    bool status = data.WriteRemoteObject(object);
    CHECK_AND_RETURN_RET_LOG(status, AVCS_ERR_INVALID_OPERATION, "SetDecryptConfig WriteRemoteObject failed");
    data.WriteBool(svpFlag);

    int32_t ret = Remote()->SendRequest(static_cast<uint32_t>(CodecServiceInterfaceCode::SET_DECRYPT_CONFIG), data,
                                        reply, option);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_INVALID_OPERATION, "Send request failed");

    return reply.ReadInt32();
}
#endif
} // namespace MediaAVCodec
} // namespace OHOS
