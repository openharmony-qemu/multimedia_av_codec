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

#include "media_codec_service_proxy.h"
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "avcodec_parcel.h"
#include "avsharedmemory_ipc.h"
#include "codec_listener_stub.h"
#include "buffer_client_producer.h"
#include "avbuffer_queue_define.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "MediaCodecServiceProxy"};
}

namespace OHOS {
namespace MediaAVCodec {
MediaCodecServiceProxy::MediaCodecServiceProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<IStandardCodecService>(impl)
{
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

MediaCodecServiceProxy::~MediaCodecServiceProxy()
{
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t MediaCodecServiceProxy::SetListenerObject(const sptr<IRemoteObject> &object)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(MediaCodecServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, AVCS_ERR_INVALID_OPERATION, "Write descriptor failed!");

    (void)data.WriteRemoteObject(object);
    int32_t ret =
        Remote()->SendRequest(static_cast<uint32_t>(CodecServiceInterfaceCode::SET_LISTENER_OBJ), data, reply, option);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_INVALID_OPERATION, "Send request failed");

    return reply.ReadInt32();
}

int32_t MediaCodecServiceProxy::Init(bool isEncoder, bool isMimeType, const std::string &name)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(MediaCodecServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, AVCS_ERR_INVALID_OPERATION, "Write descriptor failed!");

    data.WriteInt32(static_cast<int32_t>(isEncoder));
    data.WriteBool(isMimeType);
    data.WriteString(name);
    int32_t ret = Remote()->SendRequest(static_cast<uint32_t>(CodecServiceInterfaceCode::INIT), data, reply, option);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_INVALID_OPERATION, "Send request failed");

    return reply.ReadInt32();
}

int32_t MediaCodecServiceProxy::Configure(const Format &format)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(MediaCodecServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, AVCS_ERR_INVALID_OPERATION, "Write descriptor failed!");

    (void)AVCodecParcel::Marshalling(data, format);
    int32_t ret =
        Remote()->SendRequest(static_cast<uint32_t>(CodecServiceInterfaceCode::CONFIGURE), data, reply, option);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_INVALID_OPERATION, "Send request failed");

    return reply.ReadInt32();
}

int32_t MediaCodecServiceProxy::Start()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(MediaCodecServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, AVCS_ERR_INVALID_OPERATION, "Write descriptor failed!");

    int32_t ret = Remote()->SendRequest(static_cast<uint32_t>(CodecServiceInterfaceCode::START), data, reply, option);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_INVALID_OPERATION, "Send request failed");

    return reply.ReadInt32();
}

int32_t MediaCodecServiceProxy::Prepare()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(MediaCodecServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, AVCS_ERR_INVALID_OPERATION, "Write descriptor failed!");

    int32_t ret = Remote()->SendRequest(static_cast<uint32_t>(CodecServiceInterfaceCode::PREPARE), data, reply, option);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_INVALID_OPERATION, "Send request failed");

    return reply.ReadInt32();
}

int32_t MediaCodecServiceProxy::Stop()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(MediaCodecServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, AVCS_ERR_INVALID_OPERATION, "Write descriptor failed!");

    int32_t ret = Remote()->SendRequest(static_cast<uint32_t>(CodecServiceInterfaceCode::STOP), data, reply, option);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_INVALID_OPERATION, "Send request failed");

    return reply.ReadInt32();
}

int32_t MediaCodecServiceProxy::Flush()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(MediaCodecServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, AVCS_ERR_INVALID_OPERATION, "Write descriptor failed!");

    int32_t ret = Remote()->SendRequest(static_cast<uint32_t>(CodecServiceInterfaceCode::FLUSH), data, reply, option);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_INVALID_OPERATION, "Send request failed");

    return reply.ReadInt32();
}

int32_t MediaCodecServiceProxy::Reset()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(MediaCodecServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, AVCS_ERR_INVALID_OPERATION, "Write descriptor failed!");

    int32_t ret = Remote()->SendRequest(static_cast<uint32_t>(CodecServiceInterfaceCode::RESET), data, reply, option);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_INVALID_OPERATION, "Send request failed");

    return reply.ReadInt32();
}

int32_t MediaCodecServiceProxy::Release()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(MediaCodecServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, AVCS_ERR_INVALID_OPERATION, "Write descriptor failed!");

    int32_t ret = Remote()->SendRequest(static_cast<uint32_t>(CodecServiceInterfaceCode::RELEASE), data, reply, option);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_INVALID_OPERATION, "Send request failed");

    return reply.ReadInt32();
}

int32_t MediaCodecServiceProxy::GetOutputFormat(Format &format)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(MediaCodecServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, AVCS_ERR_INVALID_OPERATION, "Write descriptor failed!");

    int32_t ret =
        Remote()->SendRequest(static_cast<uint32_t>(CodecServiceInterfaceCode::GET_OUTPUT_FORMAT), data, reply, option);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_INVALID_OPERATION, "Send request failed");

    (void)AVCodecParcel::Unmarshalling(reply, format);
    return AVCS_ERR_OK;
}

int32_t MediaCodecServiceProxy::SetParameter(const Format &format)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(MediaCodecServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, AVCS_ERR_INVALID_OPERATION, "Write descriptor failed!");

    (void)AVCodecParcel::Marshalling(data, format);
    int32_t ret =
        Remote()->SendRequest(static_cast<uint32_t>(CodecServiceInterfaceCode::SET_PARAMETER), data, reply, option);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_INVALID_OPERATION, "Send request failed");

    return reply.ReadInt32();
}

sptr<Media::AVBufferQueueProducer> MediaCodecServiceProxy::GetInputBufferQueue()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(MediaCodecServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, AVCS_ERR_INVALID_OPERATION, "Write descriptor failed!");

    int32_t ret =
        Remote()->SendRequest(static_cast<uint32_t>(CodecServiceInterfaceCode::GET_INPUT_BUFFER_QUEUE), data, reply, option);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_INVALID_OPERATION, "Send request failed");

    if (reply.ReadInt32() != AVCS_ERR_OK) {
        return nullptr;
    }
    sptr<IRemoteObject> object = reply.ReadRemoteObject();
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "Read avbuffer queue object failed");

    // sptr<IBrokerListener> listener = iface_cast<IBrokerListener>(object);
    // CHECK_AND_RETURN_RET_LOG(listener != nullptr, nullptr, "Convert object to producer failed");

    // TODO: Call AVBufferQueue creator and return
    return nullptr;
}

int32_t MediaCodecServiceProxy::SetOutputBufferQueue(sptr<Media::AVBufferQueueProducer> bufferQueue)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    (void)bufferQueue;
    return reply.ReadInt32();
}

sptr<OHOS::Surface> MediaCodecServiceProxy::CreateInputSurface()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(MediaCodecServiceProxy::GetDescriptor());
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

int32_t MediaCodecServiceProxy::SetOutputSurface(sptr<OHOS::Surface> surface)
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
    AVCODEC_LOGI("Surface format is %{public}s!", format.c_str());

    bool token = data.WriteInterfaceToken(MediaCodecServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, AVCS_ERR_INVALID_OPERATION, "Write descriptor failed!");

    (void)data.WriteRemoteObject(object);
    data.WriteString(format);
    int32_t ret = Remote()->SendRequest(static_cast<uint32_t>(CodecServiceInterfaceCode::SET_OUTPUT_SURFACE), data,
                                        reply, option);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_INVALID_OPERATION, "Send request failed");

    return reply.ReadInt32();
}



int32_t MediaCodecServiceProxy::NotifyEos()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(MediaCodecServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, AVCS_ERR_INVALID_OPERATION, "Write descriptor failed!");

    int32_t ret =
        Remote()->SendRequest(static_cast<uint32_t>(CodecServiceInterfaceCode::NOTIFY_EOS), data, reply, option);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_INVALID_OPERATION, "Send request failed");

    return reply.ReadInt32();
}

int32_t MediaCodecServiceProxy::SurfaceModeReturnData(uint64_t index, bool available)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(MediaCodecServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, AVCS_ERR_INVALID_OPERATION, "Write descriptor failed!");
    data.WriteUint64(index);
    data.WriteBool(available);
    int32_t ret = Remote()->SendRequest(
        static_cast<uint32_t>(CodecServiceInterfaceCode::SURFACE_MODE_RETURN_DATA), data, reply, option);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_INVALID_OPERATION, "Send request failed");

    return reply.ReadInt32();
    return 0;
}

int32_t MediaCodecServiceProxy::DestroyStub()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    bool token = data.WriteInterfaceToken(MediaCodecServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, AVCS_ERR_INVALID_OPERATION, "Write descriptor failed!");

    int32_t ret =
        Remote()->SendRequest(static_cast<uint32_t>(CodecServiceInterfaceCode::DESTROY_STUB), data, reply, option);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_INVALID_OPERATION, "Send request failed");

    return reply.ReadInt32();
}
} // namespace MediaAVCodec
} // namespace OHOS
